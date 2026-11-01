//
// memory.h
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

// Fake Circle: CMemorySystem hands out host memory (host test harness)
#ifndef _circle_memory_h
#define _circle_memory_h

#include <circle/sysconfig.h>
#include <circle/types.h>

#define HEAP_LOW 0
#define HEAP_HIGH 1
#define HEAP_ANY 2
#define HEAP_DMA30 3

struct THeapBlockHeader
{
	u32 nMagic;
	u32 nSize;
	THeapBlockHeader* pNext;
	u8 Align[8];
};

class CMemorySystem
{
public:
	static CMemorySystem* Get();

	// Free space reported for HEAP_LOW; tests can shrink it (default 96 MB)
	size_t GetHeapFreeSpace(int nType) const;
	void* HeapAllocate(size_t nSize, int nType);
	void HeapFree(void* pBlock);

	static void SetHeapFreeSpace(size_t nBytes);
};

#endif
