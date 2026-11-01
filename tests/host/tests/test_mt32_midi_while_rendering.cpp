//
// test_mt32_midi_while_rendering.cpp
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

// MIDI and SysEx delivery to CMT32Synth while another thread renders
// (concurrency audit F4: since PR #18 every Munt call on these paths takes
// m_Lock). Needs MT-32 ROMs: set PIM32_MT32_ROMS; skipped otherwise (and in CI).

#include "check.h"
#include "fixture.h"

#include "synth/mt32synth.h"

#include <atomic>
#include <cmath>
#include <thread>
#include <vector>

static pim32::CFixture Fixture(0);

TEST(MIDIAndSysExWhileRendering)
{
	Fixture.RequireROMs();

	CMT32Synth Synth(Fixture.Config.AudioSampleRate, 1.0f, 1.0f, CMT32Synth::TResamplerQuality::Good);
	CHECK(Synth.Initialize());

	std::atomic<bool> bStop{false};
	std::atomic<unsigned> nChunks{0};
	std::atomic<float> nPeak{0.0f};

	std::thread AudioCore([&]
	{
		float Buffer[256 * 2];
		while (!bStop)
		{
			Synth.Render(Buffer, 256);
			float nChunkPeak = 0.0f;
			for (float nSample : Buffer)
				nChunkPeak = std::fmax(nChunkPeak, std::fabs(nSample));
			if (nChunkPeak > nPeak)
				nPeak = nChunkPeak;
			++nChunks;
		}
	});

	// Roland DT1 to part 1's patch temp area (timbre group/number), as games send
	const std::vector<u8> PatchSysEx = {0xF0, 0x41, 0x10, 0x16, 0x12, 0x03, 0x00, 0x00, 0x00, 0x05, 0x78, 0xF7};

	for (unsigned i = 0; i < 100; ++i)
	{
		const u8 nNote = 48 + i % 24;
		const u8 nChannel = 1 + i % 8;
		Synth.HandleMIDIShortMessage(0x64 << 16 | nNote << 8 | 0x90 | nChannel);
		Synth.HandleMIDIShortMessage(0x4007B0 | nChannel);
		if (i % 10 == 0)
		{
			Synth.HandleMIDISysExMessage(PatchSysEx.data(), PatchSysEx.size());
			Synth.SetMasterVolume(70 + i % 30);
		}
		const unsigned nTarget = nChunks + 1;
		pim32::WaitFor([&] { return nChunks >= nTarget; });
		Synth.HandleMIDIShortMessage(nNote << 8 | 0x80 | nChannel);
	}

	Synth.SetMIDIChannels(CMT32Synth::TMIDIChannels::Alternate);
	Synth.AllSoundOff();

	bStop = true;
	AudioCore.join();

	CHECK(nPeak > 0.01f);
}
