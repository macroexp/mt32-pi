//
// fixture.h
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

// Shared set-up for the host tests: a temporary SD card folder with test
// SoundFonts, a config, the zone allocator, and optional MT-32 ROMs.

#ifndef _pim32_fixture_h
#define _pim32_fixture_h

#include <string>

#include "config.h"
#include "zoneallocator.h"

namespace pim32
{
	class CFixture
	{
	public:
		// nSoundFonts test SoundFonts go into SD:soundfonts
		explicit CFixture(unsigned nSoundFonts = 2);
		~CFixture();

		// Links the MT-32 ROMs from $PIM32_MT32_ROMS into SD:roms, or skips the test
		void RequireROMs();

		const std::string& Root() const { return m_Root; }
		std::string SDPath(const char* pRelative) const;

		CConfig Config;
		CZoneAllocator Allocator;

	private:
		std::string m_Root;
	};

	// Polls until bCondition() or the timeout passes; returns bCondition()
	template <class F>
	bool WaitFor(F bCondition, unsigned nTimeoutMillis = 10000);

	void SleepMillis(unsigned nMillis);
}

#include <chrono>
#include <thread>

template <class F>
bool pim32::WaitFor(F bCondition, unsigned nTimeoutMillis)
{
	const auto Deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(nTimeoutMillis);
	while (!bCondition())
	{
		if (std::chrono::steady_clock::now() > Deadline)
			return bCondition();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return true;
}

#endif
