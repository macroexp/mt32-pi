//
// memory.cpp
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

#include <circle/memory.h>

#include <atomic>
#include <cstdlib>

namespace
{
	std::atomic<size_t> HeapFreeSpace{96 * MEGABYTE};
}

CMemorySystem* CMemorySystem::Get()
{
	static CMemorySystem MemorySystem;
	return &MemorySystem;
}

size_t CMemorySystem::GetHeapFreeSpace(int nType) const
{
	return nType == HEAP_LOW ? HeapFreeSpace.load() : 0;
}

void* CMemorySystem::HeapAllocate(size_t nSize, int nType)
{
	return aligned_alloc(64, (nSize + 63) & ~size_t(63));
}

void CMemorySystem::HeapFree(void* pBlock)
{
	free(pBlock);
}

void CMemorySystem::SetHeapFreeSpace(size_t nBytes)
{
	HeapFreeSpace = nBytes;
}
