//
// test_system_soundfont.cpp
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

// The whole of CMT32Pi (src/mt32pi.cpp) with the SoundFont synth: MIDI in on
// the serial port, the main loop on core 0, the audio task rendering on core 2.
// No ROMs, so it falls back from the default MT-32 to the SoundFont synth.
// Must be clean under ThreadSanitizer.

#include "check.h"
#include "system.h"

static pim32::CFixture Fixture;

TEST(MIDIInAudioOut)
{
	pim32::CSystem System;
	CHECK(System.Start());
	CHECK(System.WaitForAudio(10));
	System.TakeAudioPeak();

	// Silence before any note
	CHECK(System.WaitForAudio(20));
	CHECK(System.TakeAudioPeak() == 0);

	// A note on channel 1 is heard. WaitForMIDIRead() returns once the bytes have
	// been read; the fake sound device takes audio as fast as it is rendered, so
	// keep listening (up to 1000 chunks) until the note reaches the synth
	System.Send({0x90, 0x45, 0x7F});
	CHECK(System.WaitForMIDIRead());
	s32 nPeak = 0;
	for (unsigned i = 0; i < 50 && nPeak <= 100000; ++i)
	{
		CHECK(System.WaitForAudio(20));
		const s32 nChunkPeak = System.TakeAudioPeak();
		if (nChunkPeak > nPeak)
			nPeak = nChunkPeak;
	}
	CHECK(nPeak > 100000);

	// Running status, controllers, a GS reset and a custom SysEx (MT-32 reversed
	// stereo; ignored without the MT-32 synth), then all notes off
	System.Send({0x45, 0x00, 0xB0, 0x07, 0x64, 0x0A, 0x20});
	System.Send({0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7});
	System.Send({0xF0, 0x7D, 0x04, 0x01, 0xF7});
	System.Send({0x99, 0x24, 0x64, 0x89, 0x24, 0x00});
	CHECK(System.WaitForMIDIRead());

	// Once the release and the reverb tail have died away (within 10 s of
	// audio), it is silent again
	bool bSilent = false;
	for (unsigned i = 0; i < 100 && !bSilent; ++i)
	{
		System.TakeAudioPeak();
		CHECK(System.WaitForAudio(20));
		bSilent = System.TakeAudioPeak() < 1000;
	}
	CHECK(bSilent);

	System.Finish();
}
