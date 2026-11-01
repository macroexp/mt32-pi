//
// test_sf_switch_race.cpp
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

// Regression test for the SoundFont-switch crash (concurrency audit C2, PR #25).
//
// One thread renders CSoundFontSynth continuously, as the audio core does;
// the other switches SoundFonts repeatedly and sends MIDI, as core 0 does.
// On main, Reinitialize() releases m_Lock before fluid_synth_sfload(), so the
// renderer runs inside a synth that is being loaded: ThreadSanitizer reports
// the race (the test is registered as an expected race until the fix lands).

#include "check.h"
#include "fixture.h"

#include "synth/soundfontsynth.h"

#include <atomic>
#include <circle/sched/scheduler.h>
#include <cmath>
#include <thread>

static pim32::CFixture Fixture;

TEST(SwitchSoundFontsWhileRendering)
{
	CSoundFontSynth Synth(Fixture.Config.AudioSampleRate);
	CHECK(Synth.Initialize());
	CHECK_EQ(Synth.GetSoundFontManager().GetSoundFontCount(), 2u);

	std::atomic<bool> bStop{false};
	std::atomic<unsigned> nChunks{0};
	std::atomic<bool> bHeardSound{false};

	std::thread AudioCore([&]
	{
		float Buffer[256 * 2];
		while (!bStop)
		{
			Synth.Render(Buffer, 256);
			for (float nSample : Buffer)
				if (std::fabs(nSample) > 1e-4f)
					bHeardSound = true;
			++nChunks;
		}
	});

	CScheduler::ResetCounters();
	constexpr unsigned nSwitches = 20;
	for (unsigned i = 0; i < nSwitches; ++i)
	{
		CHECK(Synth.SwitchSoundFont((i + 1) % 2));

		// Some MIDI between switches: program change, notes, a controller
		Synth.HandleMIDIShortMessage(0x0000C0);
		Synth.HandleMIDIShortMessage(0x643C90);
		Synth.HandleMIDIShortMessage(0x404090);
		Synth.HandleMIDIShortMessage(0x5007B0);
		const unsigned nTarget = nChunks + 4;
		pim32::WaitFor([&] { return nChunks >= nTarget; });
		Synth.HandleMIDIShortMessage(0x003C80);
		Synth.HandleMIDIShortMessage(0x004080);
	}

	bStop = true;
	AudioCore.join();

	CHECK_EQ(Synth.GetSoundFontIndex(), nSwitches % 2 ? 1u : 0u);
	CHECK(nChunks > nSwitches);
	CHECK(bHeardSound);

	// For the record (audit C1): how often a load yielded to the scheduler while
	// holding a CSpinLock. On main this is 0; PR #25 as written makes it > 0.
	printf("Scheduler yields while holding a CSpinLock: %u\n", CScheduler::YieldsWhileHoldingSpinLock());
}
