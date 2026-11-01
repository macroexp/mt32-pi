//
// scheduler.cpp
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

// Fake Circle: cooperative scheduler hooks (host test harness)

#include <circle/sched/scheduler.h>
#include <circle/spinlock.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace
{
	std::atomic<unsigned> YieldsHoldingLock{0};
}

CScheduler* CScheduler::Get()
{
	static CScheduler Scheduler;
	return &Scheduler;
}

void CScheduler::Yield()
{
	if (CSpinLock::HeldByThisThread())
		++YieldsHoldingLock;
	std::this_thread::yield();
}

void CScheduler::Sleep(unsigned nSeconds)
{
	MsSleep(nSeconds * 1000);
}

void CScheduler::MsSleep(unsigned nMilliSeconds)
{
	usSleep(nMilliSeconds * 1000);
}

void CScheduler::usSleep(unsigned nMicroSeconds)
{
	if (CSpinLock::HeldByThisThread())
		++YieldsHoldingLock;
	std::this_thread::sleep_for(std::chrono::microseconds(nMicroSeconds));
}

unsigned CScheduler::YieldsWhileHoldingSpinLock()
{
	return YieldsHoldingLock;
}

void CScheduler::ResetCounters()
{
	YieldsHoldingLock = 0;
}
