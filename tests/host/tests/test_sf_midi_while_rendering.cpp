//
// test_sf_midi_while_rendering.cpp
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

// MIDI and SysEx delivery to CSoundFontSynth while another thread renders
// (concurrency audit F4): every FluidSynth call is under m_Lock, so this must
// be clean under ThreadSanitizer.

#include "check.h"
#include "fixture.h"

#include "synth/soundfontsynth.h"

#include <atomic>
#include <cmath>
#include <thread>
#include <vector>

static pim32::CFixture Fixture;

namespace
{
	// GS reset, GM on, and a Roland GS "use for rhythm part" message (Roland checksum included)
	const std::vector<u8> GSReset = {0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7};
	const std::vector<u8> GMOn = {0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7};
	const std::vector<u8> RhythmPart = {0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x11, 0x15, 0x02, 0x18, 0xF7};
	// GM master volume (universal real-time)
	const std::vector<u8> MasterVolume = {0xF0, 0x7F, 0x7F, 0x04, 0x01, 0x00, 0x60, 0xF7};
}

TEST(MIDIAndSysExWhileRendering)
{
	CSoundFontSynth Synth(Fixture.Config.AudioSampleRate);
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

	for (unsigned i = 0; i < 200; ++i)
	{
		const u8 nChannel = i % 16;
		const u8 nNote = 48 + i % 24;
		Synth.HandleMIDIShortMessage(0x64 << 16 | nNote << 8 | 0x90 | nChannel);
		Synth.HandleMIDIShortMessage(0x2000E0 | nChannel);
		Synth.HandleMIDIShortMessage(0x400AB0 | nChannel);
		if (i % 20 == 0)
		{
			Synth.HandleMIDISysExMessage(GSReset.data(), GSReset.size());
			Synth.HandleMIDISysExMessage(RhythmPart.data(), RhythmPart.size());
			Synth.HandleMIDISysExMessage(MasterVolume.data(), MasterVolume.size());
		}
		if (i % 50 == 0)
		{
			Synth.HandleMIDISysExMessage(GMOn.data(), GMOn.size());
			Synth.SetMasterVolume(80 + i % 20);
		}
		Synth.IsActive();
		const unsigned nTarget = nChunks + 1;
		pim32::WaitFor([&] { return nChunks >= nTarget; });
		Synth.HandleMIDIShortMessage(nNote << 8 | 0x80 | nChannel);
	}

	Synth.AllSoundOff();
	Synth.HandleMIDIShortMessage(0xFF);	// System reset

	bStop = true;
	AudioCore.join();

	CHECK(nChunks >= 200u);
	CHECK(nPeak > 0.01f);
}
