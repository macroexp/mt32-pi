//
// test_system_lcd_race.cpp
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

// The whole of CMT32Pi with an SSD1306 OLED, so the UI task on core 1 draws
// the MIDI monitor and system messages that core 0 writes. Nothing protects
// that state (concurrency audit C6 and C7): ThreadSanitizer must report it.
// Registered as an expected race; flip it when C6/C7 are fixed.

#include "check.h"
#include "system.h"

static pim32::CFixture Fixture;

TEST(UIStateWhilePlaying)
{
	Fixture.Config.LCDType = CConfig::TLCDType::SSD1306I2C;
	Fixture.Config.LCDWidth = 128;
	Fixture.Config.LCDHeight = 64;

	pim32::CSystem System;
	CHECK(System.Start());

	for (unsigned i = 0; i < 200; ++i)
	{
		const u8 nNote = 48 + i % 24;
		System.Send({0x90, nNote, 0x64});
		// Long enough for several LCD updates (every 16 ms) during the test
		pim32::SleepMillis(5);
		System.Send({0x80, nNote, 0x00});
		// A Roland SC-55 display text message (shown by the UI on core 1)
		if (i % 10 == 0)
			System.Send({0xF0, 0x41, 0x10, 0x45, 0x12, 0x10, 0x00, 0x00, 'H', 'i', 0x3F, 0xF7});
	}
	CHECK(System.WaitForMIDIRead());
	System.Finish();
}
