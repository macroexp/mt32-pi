//
// test_midiparser.cpp
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

// CMIDIParser: running status, SysEx split across buffers, real-time bytes
// inside messages, illegal status bytes, SysEx overflow

#include "check.h"
#include "midiparser.h"

#include <vector>

namespace
{
	struct TMessage
	{
		bool bSysEx;
		u32 nShort;
		std::vector<u8> SysEx;
	};

	class CRecorder : public CMIDIParser
	{
	public:
		std::vector<TMessage> Messages;
		unsigned nUnexpected = 0;
		unsigned nOverflows = 0;

		void Feed(std::initializer_list<u8> Bytes, bool bIgnoreNoteOns = false)
		{
			std::vector<u8> Buffer(Bytes);
			ParseMIDIBytes(Buffer.data(), Buffer.size(), bIgnoreNoteOns);
		}

		void Feed(const std::vector<u8>& Buffer) { ParseMIDIBytes(Buffer.data(), Buffer.size()); }

		bool IsShort(size_t i, u32 nMessage) const { return i < Messages.size() && !Messages[i].bSysEx && Messages[i].nShort == nMessage; }

	protected:
		virtual void OnShortMessage(u32 nMessage) override { Messages.push_back({false, nMessage, {}}); }
		virtual void OnSysExMessage(const u8* pData, size_t nSize) override { Messages.push_back({true, 0, std::vector<u8>(pData, pData + nSize)}); }
		virtual void OnUnexpectedStatus() override { ++nUnexpected; }
		virtual void OnSysExOverflow() override { ++nOverflows; }
	};
}

TEST(ShortMessages)
{
	CRecorder Parser;
	Parser.Feed({0x90, 0x3C, 0x64, 0xC0, 0x05, 0xE1, 0x00, 0x40});
	CHECK_EQ(Parser.Messages.size(), 3u);
	CHECK(Parser.IsShort(0, 0x643C90));
	CHECK(Parser.IsShort(1, 0x0005C0));
	CHECK(Parser.IsShort(2, 0x4000E1));
}

TEST(RunningStatus)
{
	CRecorder Parser;
	// Note on, then two more notes with running status, one of them split across buffers
	Parser.Feed({0x90, 0x3C, 0x64, 0x3E, 0x64, 0x40});
	Parser.Feed({0x00});
	// Running status for a two-byte message
	Parser.Feed({0xC1, 0x01, 0x02});
	CHECK_EQ(Parser.Messages.size(), 5u);
	CHECK(Parser.IsShort(0, 0x643C90));
	CHECK(Parser.IsShort(1, 0x643E90));
	CHECK(Parser.IsShort(2, 0x004090));
	CHECK(Parser.IsShort(3, 0x0001C1));
	CHECK(Parser.IsShort(4, 0x0002C1));
}

TEST(SysExClearsRunningStatus)
{
	CRecorder Parser;
	Parser.Feed({0x90, 0x3C, 0x64, 0xF0, 0x7D, 0x01, 0xF7, 0x3E, 0x64});
	CHECK_EQ(Parser.Messages.size(), 2u);
	CHECK(Parser.IsShort(0, 0x643C90));
	CHECK(Parser.Messages.size() > 1 && Parser.Messages[1].bSysEx);
}

TEST(SysExSplitAcrossBuffers)
{
	CRecorder Parser;
	const std::vector<u8> SysEx = {0xF0, 0x41, 0x10, 0x16, 0x12, 0x20, 0x00, 0x00, 0x41, 0x42, 0x43, 0x5B, 0xF7};
	for (u8 nByte : SysEx)
		Parser.Feed({nByte});
	CHECK_EQ(Parser.Messages.size(), 1u);
	CHECK(Parser.Messages.size() == 1 && Parser.Messages[0].bSysEx && Parser.Messages[0].SysEx == SysEx);
}

TEST(RealTimeInsideSysExAndShortMessages)
{
	CRecorder Parser;
	// Clock (F8) and active sensing (FE) inside a SysEx and inside a note on;
	// undefined F9/FD are dropped
	Parser.Feed({0xF0, 0x41, 0xF8, 0x10, 0xFE, 0xF7, 0x90, 0xF8, 0x3C, 0xF9, 0x64, 0xFD});
	CHECK_EQ(Parser.Messages.size(), 5u);
	CHECK(Parser.IsShort(0, 0xF8));
	CHECK(Parser.IsShort(1, 0xFE));
	CHECK(Parser.Messages.size() > 2 && Parser.Messages[2].bSysEx && Parser.Messages[2].SysEx == std::vector<u8>({0xF0, 0x41, 0x10, 0xF7}));
	CHECK(Parser.IsShort(3, 0xF8));
	CHECK(Parser.IsShort(4, 0x643C90));
}

TEST(IllegalStatusInsideMessage)
{
	CRecorder Parser;
	// A note on cut short by a program change: the note is dropped, the program change kept
	Parser.Feed({0x90, 0x3C, 0xC0, 0x05});
	CHECK_EQ(Parser.nUnexpected, 1u);
	CHECK_EQ(Parser.Messages.size(), 1u);
	CHECK(Parser.IsShort(0, 0x0005C0));
}

TEST(IllegalStatusInsideSysEx)
{
	CRecorder Parser;
	// A SysEx cut short by a note on: the SysEx is dropped, the note kept
	Parser.Feed({0xF0, 0x41, 0x10, 0x90, 0x3C, 0x64});
	CHECK_EQ(Parser.nUnexpected, 1u);
	CHECK_EQ(Parser.Messages.size(), 1u);
	CHECK(Parser.IsShort(0, 0x643C90));
}

TEST(StrayEndOfExclusiveAndUndefinedSystemCommon)
{
	CRecorder Parser;
	// F7 without F0, and undefined F4/F5, are ignored and clear running status
	Parser.Feed({0x90, 0x3C, 0x64, 0xF7, 0x3E, 0x64, 0xF4, 0xF5, 0x80, 0x3C, 0x00});
	CHECK_EQ(Parser.Messages.size(), 2u);
	CHECK(Parser.IsShort(0, 0x643C90));
	CHECK(Parser.IsShort(1, 0x003C80));
}

TEST(DataBytesWithoutStatusAreIgnored)
{
	CRecorder Parser;
	Parser.Feed({0x3C, 0x64, 0x90, 0x3C, 0x64});
	CHECK_EQ(Parser.Messages.size(), 1u);
	CHECK(Parser.IsShort(0, 0x643C90));
}

TEST(IgnoreNoteOns)
{
	CRecorder Parser;
	Parser.Feed({0x90, 0x3C, 0x64, 0x80, 0x3C, 0x00}, true);
	CHECK_EQ(Parser.Messages.size(), 1u);
	CHECK(Parser.IsShort(0, 0x003C80));
}

TEST(SysExOverflow)
{
	CRecorder Parser;
	// 1000 bytes fit (F0 + 998 + F7); one more data byte overflows
	std::vector<u8> SysEx(1000, 0x01);
	SysEx.front() = 0xF0;
	SysEx.back() = 0xF7;
	Parser.Feed(SysEx);
	CHECK_EQ(Parser.Messages.size(), 1u);
	CHECK_EQ(Parser.nOverflows, 0u);

	std::vector<u8> TooLong(1001, 0x01);
	TooLong.front() = 0xF0;
	TooLong.back() = 0xF7;
	Parser.Feed(TooLong);
	CHECK_EQ(Parser.nOverflows, 1u);
	CHECK_EQ(Parser.Messages.size(), 1u);

	// The parser recovers
	Parser.Feed({0x90, 0x3C, 0x64});
	CHECK(Parser.IsShort(1, 0x643C90));
}
