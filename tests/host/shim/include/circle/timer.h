//
// timer.h
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

// Fake Circle: CTimer on std::chrono (host test harness)
#ifndef _circle_timer_h
#define _circle_timer_h

#include <circle/types.h>

#define HZ 100
#define MSEC2HZ(msec) ((msec) * HZ / 1000)
#define CLOCKHZ 1000000

class CTimer
{
public:
	static CTimer* Get();

	// 1/HZ ticks since start
	unsigned GetTicks() const;

	static unsigned GetClockTicks();
	static u64 GetClockTicks64();

	void MsDelay(unsigned nMilliSeconds);
	void usDelay(unsigned nMicroSeconds);
	static void SimpleMsDelay(unsigned nMilliSeconds);
	static void SimpleusDelay(unsigned nMicroSeconds);

private:
	static unsigned GetClockTicksStatic();
};

#endif
