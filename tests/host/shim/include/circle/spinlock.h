//
// spinlock.h
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

// Fake Circle: CSpinLock as a std::mutex, so ThreadSanitizer understands it
#ifndef _circle_spinlock_h
#define _circle_spinlock_h

#include <circle/types.h>
#include <mutex>

#define TASK_LEVEL 0
#define IRQ_LEVEL 1
#define FIQ_LEVEL 2

class CSpinLock
{
public:
	CSpinLock(unsigned nTargetLevel = IRQ_LEVEL) : m_nTargetLevel(nTargetLevel) {}

	void Acquire();
	void Release();

	// Test hook: number of CSpinLocks the calling thread holds
	static unsigned HeldByThisThread();

private:
	unsigned m_nTargetLevel;
	std::mutex m_Mutex;
};

#endif
