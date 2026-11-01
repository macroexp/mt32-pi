//
// fixture.cpp
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

// Shared set-up for the host tests

#include "fixture.h"

#include "check.h"
#include "sf2writer.h"

#include <circle/memory.h>
#include <fatfs/ff.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

namespace pim32
{
	CFixture::CFixture(unsigned nSoundFonts)
	{
		char Template[] = "/tmp/pim32-host-XXXXXX";
		const char* pRoot = mkdtemp(Template);
		if (!pRoot)
		{
			perror("mkdtemp");
			abort();
		}
		m_Root = pRoot;
		fs::create_directories(m_Root + "/SD/soundfonts");
		fs::create_directories(m_Root + "/SD/roms");
		fs::create_directories(m_Root + "/USB");
		FakeFatFs::SetRoot(m_Root.c_str());

		for (unsigned i = 0; i < nSoundFonts; ++i)
		{
			const std::string Name = "Test SoundFont " + std::to_string(i + 1);
			const std::string Path = m_Root + "/SD/soundfonts/test" + std::to_string(i + 1) + ".sf2";
			if (!WriteTestSoundFont(Path.c_str(), Name.c_str(), 1 + 2 * i))
				abort();
		}

		// A modest zone heap is plenty for the test SoundFonts
		CMemorySystem::SetHeapFreeSpace(48 * MEGABYTE);
		if (!Allocator.Initialize())
			abort();
	}

	CFixture::~CFixture()
	{
		std::error_code Error;
		fs::remove_all(m_Root, Error);
	}

	void CFixture::RequireROMs()
	{
		const char* pROMs = getenv("PIM32_MT32_ROMS");
		if (!pROMs || !*pROMs)
			Skip("set PIM32_MT32_ROMS to a folder of MT-32 ROMs to run the Munt tests");

		std::error_code Error;
		for (const auto& Entry : fs::directory_iterator(pROMs, Error))
			if (Entry.is_regular_file())
				fs::create_symlink(fs::absolute(Entry.path()), m_Root + "/SD/roms/" + Entry.path().filename().string(), Error);
		if (Error)
			Skip("couldn't read PIM32_MT32_ROMS");
	}

	std::string CFixture::SDPath(const char* pRelative) const
	{
		return m_Root + "/SD/" + pRelative;
	}

	void SleepMillis(unsigned nMillis)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(nMillis));
	}
}
