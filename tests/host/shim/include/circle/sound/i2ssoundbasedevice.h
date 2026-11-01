//
// i2ssoundbasedevice.h
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

// Fake Circle: I2S sound device (host test harness)
#ifndef _circle_sound_i2ssoundbasedevice_h
#define _circle_sound_i2ssoundbasedevice_h

#include <circle/i2cmaster.h>
#include <circle/sound/soundbasedevice.h>

class CI2SSoundBaseDevice : public CSoundBaseDevice
{
public:
	CI2SSoundBaseDevice(CInterruptSystem* pInterrupt, unsigned nSampleRate = 192000, unsigned nChunkSize = 8192,
			    boolean bSlave = FALSE, CI2CMaster* pI2CMaster = nullptr, u8 ucI2CAddress = 0)
		: CSoundBaseDevice(nSampleRate, nChunkSize) {}
};

#endif
