//
// bcmrandom.h
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

// Fake Circle: random number generator (host test harness)
#ifndef _circle_bcmrandom_h
#define _circle_bcmrandom_h

#include <circle/types.h>

class CBcmRandomNumberGenerator
{
public:
	u32 GetNumber(boolean bWaitForReady = TRUE) { return m_nState = m_nState * 1664525 + 1013904223; }

private:
	u32 m_nState = 1;
};

#endif
