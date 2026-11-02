//
// system.cpp
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2026 pim-32 contributors
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

// Runs the whole of CMT32Pi on the fake Circle

#include "system.h"

#include "check.h"

#include <circle/interrupt.h>
#include <circle/sound/soundbasedevice.h>
#include <cstdio>
#include <unistd.h>

namespace pim32
{
	CSystem::CSystem()
		: m_pPi(nullptr)
	{
	}

	bool CSystem::Start()
	{
		CConfig::Get()->AudioOutputDevice = CConfig::TAudioOutputDevice::I2S;

		m_pPi = new CMT32Pi(&m_I2C, &m_SPI, CInterruptSystem::Get(), &m_GPIO, &m_Serial);
		if (!m_pPi->Initialize(true))
			return false;

		m_Core0 = std::thread([this] { m_pPi->Run(0); });
		return true;
	}

	void CSystem::Send(std::initializer_list<u8> Bytes)
	{
		Send(Bytes.begin(), Bytes.size());
	}

	void CSystem::Send(const u8* pData, size_t nSize)
	{
		m_Serial.Inject(pData, nSize);
	}

	bool CSystem::WaitForMIDIRead(unsigned nTimeoutMillis)
	{
		return WaitFor([this] { return m_Serial.IsDrained(); }, nTimeoutMillis);
	}

	bool CSystem::WaitForAudio(unsigned nChunks, unsigned nTimeoutMillis)
	{
		CSoundBaseDevice* const pSound = CSoundBaseDevice::GetLast();
		if (!pSound)
			return false;
		const unsigned nTarget = pSound->GetWriteCount() + nChunks;
		return WaitFor([pSound, nTarget] { return pSound->GetWriteCount() >= nTarget; }, nTimeoutMillis);
	}

	s32 CSystem::TakeAudioPeak()
	{
		CSoundBaseDevice* const pSound = CSoundBaseDevice::GetLast();
		return pSound ? pSound->TakePeak() : 0;
	}

	void CSystem::Finish()
	{
		const unsigned nFailures = FailureCount();
		printf("%u failed checks\n", nFailures);
		fflush(stdout);
		fflush(stderr);
		_exit(nFailures ? 1 : 0);
	}
}
