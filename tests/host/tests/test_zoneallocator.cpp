//
// test_zoneallocator.cpp
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

// CZoneAllocator: allocation, free/merge, and Realloc() edge cases, including
// the in-place growth with 0 or 16 bytes left over that Phase 4.5 fixed (PR #17)

#include "check.h"
#include "fixture.h"

#include <circle/logger.h>
#include <cstring>
#include <map>
#include <random>
#include <vector>

static pim32::CFixture Fixture(0);

namespace
{
	CZoneAllocator& Zone() { return Fixture.Allocator; }

	// Block size the allocator uses for a request: header (32) + end magic (4), rounded to 16
	size_t BlockSize(size_t nRequest) { return (nRequest + 32 + 4 + 0xF) & ~size_t(0xF); }

	// Request size that gives exactly this block size
	size_t RequestFor(size_t nBlockSize) { return nBlockSize - 32 - 4; }

	// Dump() logs a warning for every block whose magic numbers are wrong
	bool HeapLooksIntact()
	{
		const unsigned nBefore = CLogger::Get()->GetCount(LogWarning);
		Zone().Dump();
		return CLogger::Get()->GetCount(LogWarning) == nBefore;
	}

	// After everything is freed the heap must be one free block again: no leaked gaps
	bool WholeHeapAllocatable()
	{
		void* p = Zone().Alloc(RequestFor(16 * MEGABYTE - 64), TZoneTag::Uncategorized);
		if (!p)
			return false;
		Zone().Free(p);
		return true;
	}
}

TEST(AllocFreeMerge)
{
	Zone().Clear();
	void* a = Zone().Alloc(100, TZoneTag::FluidSynth);
	void* b = Zone().Alloc(200, TZoneTag::FluidSynth);
	void* c = Zone().Alloc(300, TZoneTag::FluidSynth);
	CHECK(a && b && c);
	CHECK_EQ(Zone().GetAllocCount(), 3u);
	CHECK(Zone().Alloc(0, TZoneTag::FluidSynth) == nullptr);
	CHECK(Zone().Alloc(10, TZoneTag::Free) == nullptr);

	Zone().Free(b);
	Zone().Free(a);
	Zone().Free(c);
	CHECK_EQ(Zone().GetAllocCount(), 0u);
	CHECK(HeapLooksIntact());
	CHECK(WholeHeapAllocatable());
}

// Grow A in place into the free block B that follows it, leaving nLeftOver bytes
static void GrowInPlace(size_t nLeftOver)
{
	Zone().Clear();
	const size_t nA = BlockSize(100), nB = BlockSize(100);
	auto* a = static_cast<u8*>(Zone().Alloc(100, TZoneTag::FluidSynth));
	void* b = Zone().Alloc(100, TZoneTag::FluidSynth);
	auto* c = static_cast<u8*>(Zone().Alloc(64, TZoneTag::FluidSynth));
	memset(a, 0xAA, 100);
	memset(c, 0xCC, 64);
	Zone().Free(b);

	const size_t nNewBlock = nA + nB - nLeftOver;
	auto* a2 = static_cast<u8*>(Zone().Realloc(a, RequestFor(nNewBlock), TZoneTag::FluidSynth));
	CHECK(a2 == a);

	// The block after the free space must be untouched
	bool bIntact = true;
	for (unsigned i = 0; i < 64; ++i)
		bIntact &= c[i] == 0xCC;
	CHECK(bIntact);
	CHECK(a2 && a2[0] == 0xAA && a2[99] == 0xAA);
	CHECK(HeapLooksIntact());

	Zone().Free(a2);
	Zone().Free(c);
	CHECK(HeapLooksIntact());
	CHECK(WholeHeapAllocatable());
}

TEST(ReallocGrowInPlaceNothingLeftOver) { GrowInPlace(0); }
TEST(ReallocGrowInPlace16BytesLeftOver) { GrowInPlace(16); }
TEST(ReallocGrowInPlace32BytesLeftOver) { GrowInPlace(32); }

TEST(ReallocShrinkBySmallAmountLeavesNoGap)
{
	Zone().Clear();
	void* a = Zone().Alloc(RequestFor(256), TZoneTag::FluidSynth);
	void* b = Zone().Alloc(100, TZoneTag::FluidSynth);
	void* a2 = Zone().Realloc(a, RequestFor(240), TZoneTag::FluidSynth);
	CHECK(a2 == a);
	Zone().Free(a2);
	Zone().Free(b);
	CHECK(HeapLooksIntact());
	CHECK(WholeHeapAllocatable());
}

TEST(ReallocNullAndMove)
{
	Zone().Clear();
	auto* a = static_cast<u8*>(Zone().Realloc(nullptr, 50, TZoneTag::FluidSynth));
	CHECK(a != nullptr);
	memset(a, 0x5A, 50);
	void* blocker = Zone().Alloc(10, TZoneTag::FluidSynth);
	// Can't grow in place: the contents move
	auto* a2 = static_cast<u8*>(Zone().Realloc(a, 5000, TZoneTag::FluidSynth));
	CHECK(a2 && a2 != a && a2[0] == 0x5A && a2[49] == 0x5A);
	Zone().Free(a2);
	Zone().Free(blocker);
	CHECK(WholeHeapAllocatable());
}

// Random alloc/realloc/free against a model; every live block keeps its fill
TEST(RandomOperationsKeepContents)
{
	std::mt19937 Random(1234);
	for (unsigned nRun = 0; nRun < 50; ++nRun)
	{
		Zone().Clear();
		std::map<u8*, std::pair<size_t, u8>> Live;
		u8 nFill = 1;
		bool bContentsOK = true;

		for (unsigned nOp = 0; nOp < 400; ++nOp)
		{
			const unsigned nKind = Random() % 3;
			if (nKind == 0 || Live.empty())
			{
				const size_t nSize = 1 + Random() % 600;
				auto* p = static_cast<u8*>(Zone().Alloc(nSize, TZoneTag::FluidSynth));
				if (!p)
					continue;
				memset(p, nFill, nSize);
				Live[p] = {nSize, nFill++};
			}
			else
			{
				auto It = Live.begin();
				std::advance(It, Random() % Live.size());
				u8* p = It->first;
				const auto [nSize, nByte] = It->second;
				Live.erase(It);

				if (nKind == 1)
				{
					// Grow or shrink by small and large amounts, including multiples of 16
					const size_t nNewSize = 1 + Random() % 700;
					auto* p2 = static_cast<u8*>(Zone().Realloc(p, nNewSize, TZoneTag::FluidSynth));
					if (!p2)
						continue;
					for (size_t i = 0; i < std::min(nSize, nNewSize); ++i)
						bContentsOK &= p2[i] == nByte;
					memset(p2, nByte, nNewSize);
					Live[p2] = {nNewSize, nByte};
				}
				else
					Zone().Free(p);
			}

			for (const auto& [p, Info] : Live)
				for (size_t i = 0; i < Info.first; ++i)
					bContentsOK &= p[i] == Info.second;
		}

		CHECK(bContentsOK);
		CHECK(HeapLooksIntact());
		for (const auto& [p, Info] : Live)
			Zone().Free(p);
		CHECK_EQ(Zone().GetAllocCount(), 0u);
		CHECK(WholeHeapAllocatable());
	}
}

TEST(FreeTag)
{
	Zone().Clear();
	for (unsigned i = 0; i < 10; ++i)
	{
		Zone().Alloc(100, TZoneTag::FluidSynth);
		Zone().Alloc(100, TZoneTag::Uncategorized);
	}
	Zone().FreeTag(TZoneTag::FluidSynth);
	CHECK_EQ(Zone().GetAllocCount(), 10u);
	Zone().FreeTag(TZoneTag::Uncategorized);
	CHECK_EQ(Zone().GetAllocCount(), 0u);
	CHECK(WholeHeapAllocatable());
}
