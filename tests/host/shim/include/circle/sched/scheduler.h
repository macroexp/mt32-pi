//
// scheduler.h
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

// Fake Circle: cooperative scheduler (host test harness)
//
// Yield() is where Circle switches to another task on core 0. With the
// kernel's NO_BUSY_WAIT, SD and USB reads yield too, so the fake FatFs calls
// Yield() from f_read(). The harness counts yields made while the calling
// thread holds a CSpinLock: on the Pi, another core-0 task that then takes
// that lock spins forever (see the concurrency audit, C1).
#ifndef _circle_sched_scheduler_h
#define _circle_sched_scheduler_h

#include <circle/types.h>

class CScheduler
{
public:
	static CScheduler* Get();
	static boolean IsActive() { return TRUE; }

	void Yield();
	void Sleep(unsigned nSeconds);
	void MsSleep(unsigned nMilliSeconds);
	void usSleep(unsigned nMicroSeconds);

	// Test hooks
	static unsigned YieldsWhileHoldingSpinLock();
	static void ResetCounters();
};

#endif
