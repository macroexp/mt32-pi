//
// test_mt32_lcd_romswitch_race.cpp
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

// Concurrency audit C3: CMT32Synth::UpdateLCD() (core 1) calls Munt's
// getDisplayState() and readMemory() without m_Lock, while a ROM-set switch on
// core 0 closes and reopens the synth, deleting the display and memory
// regions. ThreadSanitizer must report it; registered as an expected race
// until the fix lands. Needs MT-32 ROMs for at least two ROM sets: set
// PIM32_MT32_ROMS; skipped otherwise (and in CI).

#include "check.h"
#include "fixture.h"

#include "lcd/lcd.h"
#include "synth/mt32synth.h"

#include <atomic>
#include <circle/timer.h>
#include <thread>

static pim32::CFixture Fixture(0);

namespace
{
	class CBlankLCD : public CLCD
	{
	public:
		CBlankLCD() : CLCD(128, 64) {}
		virtual bool Initialize() override { return true; }
		virtual TType GetType() const override { return TType::Graphical; }
	};
}

TEST(UpdateLCDDuringROMSetSwitch)
{
	Fixture.RequireROMs();

	CMT32Synth Synth(Fixture.Config.AudioSampleRate, 1.0f, 1.0f, CMT32Synth::TResamplerQuality::Good);
	CHECK(Synth.Initialize());

	std::atomic<bool> bStop{false};
	std::thread UICore([&]
	{
		CBlankLCD LCD;
		while (!bStop)
			Synth.UpdateLCD(LCD, CTimer::GetClockTicks());
	});

	unsigned nSwitches = 0;
	for (unsigned i = 0; i < 20; ++i)
		nSwitches += Synth.NextROMSet();

	bStop = true;
	UICore.join();

	if (nSwitches == 0)
		pim32::Skip("only one MT-32 ROM set available");
}
