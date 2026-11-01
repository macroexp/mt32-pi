//
// test_midimonitor_note127.cpp
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

// Known bug O1 (concurrency audit, "Other bugs"): CMIDIMonitor::NoteCount is
// 127, so note number 127 indexes one past Notes[] and overwrites the next
// channel's state (its volume and expression). Expected to fail until the fix
// lands. (UBSan doesn't flag it: Notes[] is the struct's last member, which GCC
// treats like a flexible array.)

#include "check.h"
#include "midimonitor.h"

#include <circle/timer.h>

#include <thread>

TEST(Note127DoesNotTouchTheNextChannel)
{
	CMIDIMonitor Monitor;

	// Note 127 on channel 1, then an ordinary note on channel 2
	Monitor.OnShortMessage(0x7F7F90);
	Monitor.OnShortMessage(0x7F3C91);

	// Past the 20 ms attack
	std::this_thread::sleep_for(std::chrono::milliseconds(40));

	float Levels[16], Peaks[16];
	Monitor.GetChannelLevels(CTimer::GetClockTicks(), Levels, Peaks);

	// Channel 2's level comes from its own note at full velocity and default volume (100)
	CHECK(Levels[1] > 0.5f);
}

TEST(NoteNumbersUpTo127AreTracked)
{
	CMIDIMonitor Monitor;
	Monitor.OnShortMessage(0x7F7F95);
	std::this_thread::sleep_for(std::chrono::milliseconds(40));

	float Levels[16], Peaks[16];
	Monitor.GetChannelLevels(CTimer::GetClockTicks(), Levels, Peaks);
	CHECK(Levels[5] > 0.5f);
}
