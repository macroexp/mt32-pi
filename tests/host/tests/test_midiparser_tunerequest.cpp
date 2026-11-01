//
// test_midiparser_tunerequest.cpp
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

// Known bug O2 (concurrency audit, "Other bugs"): after a Tune Request (F6),
// CMIDIParser::ParseStatusByte() stores the byte after its switch, leaving
// running status F6 and a message length of 1. The next channel message is
// merged into a bogus F6 message. Expected to fail until the fix lands.

#include "check.h"
#include "midiparser.h"

#include <vector>

namespace
{
	class CRecorder : public CMIDIParser
	{
	public:
		std::vector<u32> Messages;

	protected:
		virtual void OnShortMessage(u32 nMessage) override { Messages.push_back(nMessage); }
		virtual void OnSysExMessage(const u8* pData, size_t nSize) override {}
	};
}

TEST(NoteOnAfterTuneRequest)
{
	CRecorder Parser;
	const u8 Bytes[] = {0xF6, 0x90, 0x3C, 0x64};
	Parser.ParseMIDIBytes(Bytes, sizeof(Bytes));
	CHECK_EQ(Parser.Messages.size(), 2u);
	CHECK(Parser.Messages.size() > 0 && Parser.Messages[0] == 0xF6);
	CHECK(Parser.Messages.size() > 1 && Parser.Messages[1] == 0x643C90);
}

TEST(DataBytesAfterTuneRequestAreIgnored)
{
	CRecorder Parser;
	// Tune Request clears running status, so these data bytes mean nothing
	const u8 Bytes[] = {0xF6, 0x3C, 0x64};
	Parser.ParseMIDIBytes(Bytes, sizeof(Bytes));
	CHECK_EQ(Parser.Messages.size(), 1u);
}
