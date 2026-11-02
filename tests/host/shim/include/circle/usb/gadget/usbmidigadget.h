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

// Fake Circle: USB MIDI gadget; it never comes up in the harness
#ifndef _circle_usb_gadget_usbmidigadget_h
#define _circle_usb_gadget_usbmidigadget_h

#include <circle/interrupt.h>
#include <circle/types.h>
#include <circle/usb/usb.h>
#include <circle/usb/usbcontroller.h>

#include <cstddef>

class CUSBMIDIGadget : public CUSBController
{
public:
	CUSBMIDIGadget(CInterruptSystem* pInterruptSystem, u16 usVendorID, u16 usProductID) {}

	boolean Initialize(boolean bScanDevices = TRUE) override { return FALSE; }
	boolean UpdatePlugAndPlay() override { return FALSE; }

protected:
	virtual const void* GetDescriptor(u16 wValue, u16 wIndex, size_t* pLength) { return nullptr; }
	const void* ToStringDescriptor(const char* pString, size_t* pLength)
	{
		*pLength = 0;
		return nullptr;
	}
};

#endif
