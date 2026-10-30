//
// usbmidigadget.cpp
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

#include <circle/machineinfo.h>
#include <circle/usb/usb.h>

#include "usbmidigadget.h"

// pid.codes test IDs (https://pid.codes); replace with an assigned product ID before release.
// Windows caches drivers by vendor/product ID, so change the product ID if the descriptors change.
#ifndef MT32_PI_USB_VENDOR_ID
#define MT32_PI_USB_VENDOR_ID 0x1209
#endif
#ifndef MT32_PI_USB_PRODUCT_ID
#define MT32_PI_USB_PRODUCT_ID 0x0001
#endif

// String descriptor indices used by Circle's device descriptor
constexpr u8 ManufacturerStringIndex = 1;
constexpr u8 ProductStringIndex      = 2;

const char ManufacturerString[] = "pim-32";
const char ProductString[]      = "mt32-pi";

CMT32PiUSBMIDIGadget::CMT32PiUSBMIDIGadget(CInterruptSystem* pInterrupt)
	: CUSBMIDIGadget(pInterrupt, MT32_PI_USB_VENDOR_ID, MT32_PI_USB_PRODUCT_ID)
{
}

bool CMT32PiUSBMIDIGadget::IsSupported()
{
	switch (CMachineInfo::Get()->GetMachineModel())
	{
		case MachineModelZero:
		case MachineModelZeroW:
		case MachineModelZero2W:
		case MachineModel3APlus:
		case MachineModel4B:
			return true;

		default:
			return false;
	}
}

const void* CMT32PiUSBMIDIGadget::GetDescriptor(u16 wValue, u16 wIndex, size_t* pLength)
{
	if ((wValue >> 8) == DESCRIPTOR_STRING)
	{
		const u8 nIndex = wValue & 0xFF;
		if (nIndex == ManufacturerStringIndex)
			return ToStringDescriptor(ManufacturerString, pLength);
		if (nIndex == ProductStringIndex)
			return ToStringDescriptor(ProductString, pLength);
	}

	return CUSBMIDIGadget::GetDescriptor(wValue, wIndex, pLength);
}
