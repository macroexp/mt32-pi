//
// test_ui_format.cpp
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

// Known bug O3 (concurrency audit, "Other bugs"): CUserInterface::
// ShowSystemMessage() passes the message to snprintf() as the format, so a
// SoundFont or ROM name containing '%' is mangled (and reads stray varargs).
// Expected to fail until the fix lands.

#include "check.h"

#include "lcd/lcd.h"
#include "lcd/ui.h"
#include "synth/synthbase.h"

#include <circle/timer.h>

#include <string>

namespace
{
	// A 20x4 character LCD that records the text printed on each row
	class CTextLCD : public CLCD
	{
	public:
		CTextLCD() : CLCD(20, 4) {}
		virtual bool Initialize() override { return true; }
		virtual TType GetType() const override { return TType::Character; }
		virtual void Print(const char* pText, u8 nCursorX, u8 nCursorY, bool bClearLine, bool bImmediate) override
		{
			if (nCursorY < 4)
				Rows[nCursorY] = pText;
		}

		std::string Rows[4];
	};

	class CNullSynth : public CSynthBase
	{
	public:
		CNullSynth() : CSynthBase(48000) {}
		virtual bool Initialize() override { return true; }
		virtual void HandleMIDISysExMessage(const u8* pData, size_t nSize) override {}
		virtual bool IsActive() override { return false; }
		virtual void SetMasterVolume(u8 nVolume) override {}
		virtual size_t Render(s16* pOutBuffer, size_t nFrames) override { return nFrames; }
		virtual size_t Render(float* pOutBuffer, size_t nFrames) override { return nFrames; }
		virtual void ReportStatus() const override {}
		virtual void UpdateLCD(CLCD& LCD, unsigned int nTicks) override {}
	};
}

TEST(PercentSignsArePrintedLiterally)
{
	CTextLCD LCD;
	CNullSynth Synth;
	CUserInterface UI;

	// Control: a plain message lands on row 1 of a 4-row LCD
	UI.ShowSystemMessage("Plain Piano");
	UI.Update(LCD, Synth, CTimer::GetClockTicks());
	CHECK(LCD.Rows[1] == "Plain Piano");

	UI.ShowSystemMessage("100%d Piano");
	UI.Update(LCD, Synth, CTimer::GetClockTicks());
	if (LCD.Rows[1] != "100%d Piano")
		fprintf(stderr, "  printed: \"%s\"\n", LCD.Rows[1].c_str());
	CHECK(LCD.Rows[1] == "100%d Piano");
}
