//
// multicore.h
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

// Fake Circle: secondary cores are std::threads (host test harness)
#ifndef _circle_multicore_h
#define _circle_multicore_h

#include <circle/memory.h>
#include <circle/sysconfig.h>
#include <circle/types.h>
#include <thread>

class CMultiCoreSupport
{
public:
	CMultiCoreSupport(CMemorySystem* pMemorySystem);
	virtual ~CMultiCoreSupport();

	// Starts cores 1-3, each calling Run(nCore); the caller is core 0
	boolean Initialize();
	virtual void Run(unsigned nCore) = 0;

	static unsigned ThisCore();

	// Test hook: waits for cores 1-3 to return from Run()
	static void JoinSecondaryCores();

private:
	std::thread m_Cores[CORES];
	static CMultiCoreSupport* s_pThis;
};

#endif
