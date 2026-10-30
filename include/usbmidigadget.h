//
// usbmidigadget.h
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

#ifndef _usbmidigadget_h
#define _usbmidigadget_h

#include <circle/interrupt.h>
#include <circle/types.h>
#include <circle/usb/gadget/usbmidigadget.h>

// Circle's USB MIDI gadget with our own IDs and names, so the host shows the port as "mt32-pi"
class CMT32PiUSBMIDIGadget : public CUSBMIDIGadget
{
public:
	CMT32PiUSBMIDIGadget(CInterruptSystem* pInterrupt);

	// True on boards whose DWC USB controller can be wired to a host: Zero (W), Zero 2 W, 3A+, 4B.
	// On the 2B, 3B and 3B+ it sits behind the onboard USB hub/Ethernet chip.
	static bool IsSupported();

protected:
	const void* GetDescriptor(u16 wValue, u16 wIndex, size_t* pLength) override;
};

#endif
