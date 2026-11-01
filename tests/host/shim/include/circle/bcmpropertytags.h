//
// bcmpropertytags.h
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

// Fake Circle: firmware property tags (host test harness)
#ifndef _circle_bcmpropertytags_h
#define _circle_bcmpropertytags_h

#include <circle/types.h>

#define PROPTAG_GET_THROTTLED 0x00030046

struct TPropertyTag
{
	u32 nTagId;
	u32 nValueBufSize;
	u32 nValueLength;
};

struct TPropertyTagSimple
{
	TPropertyTag Tag;
	u32 nValue;
};

class CBcmPropertyTags
{
public:
	boolean GetTag(u32 nTagId, void* pTag, unsigned nTagSize, unsigned nRequestParmSize = 0)
	{
		// No throttling, no undervoltage
		static_cast<TPropertyTagSimple*>(pTag)->nValue = 0;
		return TRUE;
	}
};

#endif
