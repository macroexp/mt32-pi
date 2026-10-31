//
// power.cpp
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2020-2023 Dale Whinham <daleyo@gmail.com>
//
// This file is part of mt32-pi.
//
// mt32-pi is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// mt32-pi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// mt32-pi. If not, see <http://www.gnu.org/licenses/>.
//

#include <circle/bcmpropertytags.h>
#include <circle/cputhrottle.h>
#include <circle/logger.h>
#include <circle/timer.h>

#include "power.h"

LOGMODULE("power");

#if RASPPI >= 5
// Pi 5 fan connector: GPIO 45, active low (as Circle's CCPUThrottle drives it).
// Switch on at FanOnTemperature, off again below FanOffTemperature.
constexpr unsigned FanGPIOPin          = 45;
constexpr unsigned FanOnTemperature    = 60;
constexpr unsigned FanOffTemperature   = 50;
#endif

// Bits in the throttled status response
constexpr u32 UnderVoltageOccurredBit = 1 << 16;
constexpr u32 ThrottlingOccurredBit   = 1 << 18;

CPower::CPower()
	: m_nPowerSaveTimeout(300),
	  m_nLastActivityTime(0),
	  m_State(TState::Normal),
	  m_LastThrottledStatus(0)
#if RASPPI >= 5
	  , m_nLastThrottledCheckTime(0),
	  m_bFanOn(false)
#endif
{
#if RASPPI >= 5
	// Start with the fan off; UpdateFan() switches it on when the SoC gets warm
	m_FanPin.AssignPin(FanGPIOPin);
	m_FanPin.SetMode(GPIOModeOutput, false);
	m_FanPin.Write(HIGH);
#endif
}

void CPower::Update()
{
	unsigned int nTicks = CTimer::Get()->GetTicks();

	// Power save timeout
	if (m_State == TState::Normal && m_nPowerSaveTimeout && (nTicks - m_nLastActivityTime) >= m_nPowerSaveTimeout * HZ)
	{
		CCPUThrottle::Get()->SetSpeed(TCPUSpeed::CPUSpeedLow);
		m_State = TState::PowerSaving;
		OnEnterPowerSavingMode();
	}

	// Check for undervoltage and throttling
#if RASPPI >= 5
	// This is a firmware mailbox call; the main loop runs thousands of times a
	// second, and on the Pi 5 that many calls disturb the Wi-Fi driver, which
	// runs between them on this core. Once a second is plenty.
	if (nTicks - m_nLastThrottledCheckTime < HZ)
		return;
	m_nLastThrottledCheckTime = nTicks;

	UpdateFan();
#endif
	UpdateThrottledStatus();
}

#if RASPPI >= 5
void CPower::UpdateFan()
{
	// SoC temperature in degrees Celsius from the firmware; 0 on failure
	const unsigned nTemperature = CCPUThrottle::Get()->GetTemperature();
	if (nTemperature == 0)
		return;

	if (!m_bFanOn && nTemperature >= FanOnTemperature)
	{
		m_FanPin.Write(LOW);
		m_bFanOn = true;
		LOGNOTE("Fan on (%uC)", nTemperature);
	}
	else if (m_bFanOn && nTemperature < FanOffTemperature)
	{
		m_FanPin.Write(HIGH);
		m_bFanOn = false;
		LOGNOTE("Fan off (%uC)", nTemperature);
	}
}
#endif

void CPower::Awaken()
{
	m_nLastActivityTime = CTimer::Get()->GetTicks();

	if (m_State == TState::Normal)
		return;

	CCPUThrottle::Get()->SetSpeed(TCPUSpeed::CPUSpeedMaximum);
	m_State = TState::Normal;

	OnExitPowerSavingMode();
}

void CPower::OnEnterPowerSavingMode()
{
	LOGNOTE("Entering power saving mode");
}

void CPower::OnExitPowerSavingMode()
{
	LOGNOTE("Leaving power saving mode");
}

void CPower::OnThrottleDetected()
{
	LOGWARN("CPU throttling by firmware detected; check power supply/cooling");
}

void CPower::OnUnderVoltageDetected()
{
	LOGWARN("Undervoltage detected; check power supply");
}

void CPower::UpdateThrottledStatus()
{
	// Get throttled status from the firmware and clear status bits
	TPropertyTagSimple ThrottledStatus;
	ThrottledStatus.nValue = 0xFFFF;
	m_Tags.GetTag(PROPTAG_GET_THROTTLED, &ThrottledStatus, sizeof(ThrottledStatus), sizeof(ThrottledStatus.nValue));

	bool bNewVal = ThrottledStatus.nValue & ThrottlingOccurredBit;
	bool bOldVal = m_LastThrottledStatus & ThrottlingOccurredBit;

	if (bNewVal && bOldVal != bNewVal)
		OnThrottleDetected();

	bNewVal = ThrottledStatus.nValue & UnderVoltageOccurredBit;
	bOldVal = m_LastThrottledStatus & UnderVoltageOccurredBit;

	if (bNewVal && bOldVal != bNewVal)
		OnUnderVoltageDetected();

	m_LastThrottledStatus = ThrottledStatus.nValue;
}
