//
// timer.cpp
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

#include <circle/timer.h>

#include <chrono>
#include <thread>

namespace
{
	using TClock = std::chrono::steady_clock;
	const TClock::time_point StartTime = TClock::now();

	u64 MicrosSinceStart()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(TClock::now() - StartTime).count();
	}
}

CTimer* CTimer::Get()
{
	static CTimer Timer;
	return &Timer;
}

unsigned CTimer::GetTicks() const
{
	return static_cast<unsigned>(MicrosSinceStart() / (1000000 / HZ));
}

unsigned CTimer::GetClockTicks()
{
	return GetClockTicksStatic();
}

unsigned CTimer::GetClockTicksStatic()
{
	// Wraps at 2^32 microseconds, like the Pi's 1 MHz counter
	return static_cast<unsigned>(MicrosSinceStart());
}

u64 CTimer::GetClockTicks64()
{
	return MicrosSinceStart();
}

void CTimer::MsDelay(unsigned nMilliSeconds)
{
	SimpleMsDelay(nMilliSeconds);
}

void CTimer::usDelay(unsigned nMicroSeconds)
{
	SimpleusDelay(nMicroSeconds);
}

void CTimer::SimpleMsDelay(unsigned nMilliSeconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(nMilliSeconds));
}

void CTimer::SimpleusDelay(unsigned nMicroSeconds)
{
	std::this_thread::sleep_for(std::chrono::microseconds(nMicroSeconds));
}
