//
// test_switch_synth_race.cpp
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

// Concurrency audit C5: SwitchSynth() on core 0 stores m_pCurrentSynth without
// a lock or an atomic, while the audio task on core 2 calls Render() through it.
// Runs the whole of CMT32Pi and switches synths with the custom SysEx
// (F0 7D 03 xx F7) while notes play. ThreadSanitizer must report it; while C4
// is unfixed it may report that race first (the main loop calls IsActive() on
// the MT-32 synth). Needs MT-32 ROMs: set PIM32_MT32_ROMS; skipped otherwise
// (and in CI).

#include "check.h"
#include "system.h"

static pim32::CFixture Fixture;

TEST(SwitchSynthWhileRendering)
{
	Fixture.RequireROMs();

	pim32::CSystem System;
	CHECK(System.Start());

	for (unsigned i = 0; i < 40; ++i)
	{
		System.Send({0xF0, 0x7D, 0x03, static_cast<u8>(i % 2), 0xF7});
		System.Send({0x91, 0x3C, 0x64});
		System.WaitForAudio(4);
		System.Send({0x81, 0x3C, 0x00});
	}
	CHECK(System.WaitForMIDIRead());
	System.Finish();
}
