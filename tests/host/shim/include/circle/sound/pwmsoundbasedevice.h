//
// pwmsoundbasedevice.h
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

// Fake Circle: PWM sound device (host test harness)
#ifndef _circle_sound_pwmsoundbasedevice_h
#define _circle_sound_pwmsoundbasedevice_h

#include <circle/sound/soundbasedevice.h>

class CPWMSoundBaseDevice : public CSoundBaseDevice
{
public:
	CPWMSoundBaseDevice(CInterruptSystem* pInterrupt, unsigned nSampleRate = 44100, unsigned nChunkSize = 2048, boolean bStereoMS = TRUE)
		: CSoundBaseDevice(nSampleRate, nChunkSize) {}
};

#endif
