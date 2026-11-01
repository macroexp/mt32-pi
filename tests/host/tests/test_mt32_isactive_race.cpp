//
// test_mt32_isactive_race.cpp
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

// Concurrency audit C4: CMT32Synth::IsActive() calls Munt's isActive() without
// m_Lock on core 0 (every main-loop pass) while core 2 renders; it reads the
// queue, partials and reverb buffers and can clear 'activated'. ThreadSanitizer
// must report it; registered as an expected race until the fix lands.
// Needs MT-32 ROMs: set PIM32_MT32_ROMS; skipped otherwise (and in CI).

#include "check.h"
#include "fixture.h"

#include "synth/mt32synth.h"

#include <atomic>
#include <thread>

static pim32::CFixture Fixture(0);

TEST(IsActiveWhileRendering)
{
	Fixture.RequireROMs();

	CMT32Synth Synth(Fixture.Config.AudioSampleRate, 1.0f, 1.0f, CMT32Synth::TResamplerQuality::Good);
	CHECK(Synth.Initialize());

	std::atomic<bool> bStop{false};
	std::atomic<unsigned> nChunks{0};
	std::thread AudioCore([&]
	{
		float Buffer[256 * 2];
		while (!bStop)
		{
			Synth.Render(Buffer, 256);
			++nChunks;
		}
	});

	// Like the main loop: MIDI, then IsActive() on every pass
	for (unsigned i = 0; i < 200; ++i)
	{
		const u8 nNote = 48 + i % 24;
		Synth.HandleMIDIShortMessage(0x64 << 16 | nNote << 8 | 0x91);
		for (unsigned j = 0; j < 50; ++j)
			Synth.IsActive();
		Synth.HandleMIDIShortMessage(nNote << 8 | 0x81);
	}

	bStop = true;
	AudioCore.join();
}
