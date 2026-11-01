//
// spimaster.h
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

// Fake Circle: SPI master (host test harness)
#ifndef _circle_spimaster_h
#define _circle_spimaster_h

#include <circle/types.h>
#include <cstring>

class CSPIMaster
{
public:
	CSPIMaster(unsigned nClockSpeed = 500000, unsigned CPOL = 0, unsigned CPHA = 0, unsigned nDevice = 0) {}
	boolean Initialize() { return TRUE; }
	void SetClock(unsigned nClockSpeed) {}
	void SetMode(unsigned CPOL, unsigned CPHA) {}
	int Read(unsigned nChipSelect, void* pBuffer, unsigned nCount)
	{
		memset(pBuffer, 0, nCount);
		return static_cast<int>(nCount);
	}
	int Write(unsigned nChipSelect, const void* pBuffer, unsigned nCount) { return static_cast<int>(nCount); }
	int WriteRead(unsigned nChipSelect, const void* pWriteBuffer, void* pReadBuffer, unsigned nCount)
	{
		memset(pReadBuffer, 0, nCount);
		return static_cast<int>(nCount);
	}
};

#endif
