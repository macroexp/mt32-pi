//
// multicore.cpp
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

#include <circle/multicore.h>

namespace
{
	thread_local unsigned CurrentCore = 0;
}

CMultiCoreSupport* CMultiCoreSupport::s_pThis = nullptr;

CMultiCoreSupport::CMultiCoreSupport(CMemorySystem* pMemorySystem)
{
	s_pThis = this;
}

CMultiCoreSupport::~CMultiCoreSupport()
{
	JoinSecondaryCores();
	s_pThis = nullptr;
}

boolean CMultiCoreSupport::Initialize()
{
	for (unsigned nCore = 1; nCore < CORES; ++nCore)
		m_Cores[nCore] = std::thread([this, nCore]
		{
			CurrentCore = nCore;
			Run(nCore);
		});

	return TRUE;
}

unsigned CMultiCoreSupport::ThisCore()
{
	return CurrentCore;
}

void CMultiCoreSupport::JoinSecondaryCores()
{
	if (!s_pThis)
		return;

	for (unsigned nCore = 1; nCore < CORES; ++nCore)
		if (s_pThis->m_Cores[nCore].joinable())
			s_pThis->m_Cores[nCore].join();
}
