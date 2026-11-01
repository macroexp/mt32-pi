//
// cputhrottle.h
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

// Fake Circle: CPU throttle (host test harness)
#ifndef _circle_cputhrottle_h
#define _circle_cputhrottle_h

#include <circle/types.h>

enum TCPUSpeed
{
	CPUSpeedLow,
	CPUSpeedMaximum,
	CPUSpeedUnknown
};

class CCPUThrottle
{
public:
	static CCPUThrottle* Get();

	boolean SetSpeed(TCPUSpeed Speed, boolean bWait = TRUE) { m_Speed = Speed; return TRUE; }
	TCPUSpeed GetSpeed() const { return m_Speed; }
	unsigned GetTemperature() { return 45; }
	void DumpStatus(boolean bAll = TRUE) {}
	boolean Update() { return TRUE; }

private:
	TCPUSpeed m_Speed = CPUSpeedMaximum;
};

#endif
