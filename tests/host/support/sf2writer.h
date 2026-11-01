//
// sf2writer.h
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

// Writes a tiny SoundFont 2 file (a few KB) for the host tests: presets at
// bank 0 program 0 and bank 128 program 0 (drums) playing a looped tone.

#ifndef _pim32_sf2writer_h
#define _pim32_sf2writer_h

namespace pim32
{
	// nHarmonics: 1 = sine, more = brighter tone; lets two test SoundFonts differ
	bool WriteTestSoundFont(const char* pPath, const char* pName, unsigned nHarmonics = 1);
}

#endif
