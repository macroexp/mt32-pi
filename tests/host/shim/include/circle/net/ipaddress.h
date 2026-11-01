//
// ipaddress.h
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

// Fake Circle: IP address (host test harness)
#ifndef _circle_net_ipaddress_h
#define _circle_net_ipaddress_h

#include <circle/string.h>
#include <circle/types.h>
#include <cstring>

#define IP_ADDRESS_SIZE 4

class CIPAddress
{
public:
	CIPAddress() { Set(0u); }
	CIPAddress(u32 nAddress) { Set(nAddress); }
	CIPAddress(const u8* pAddress) { Set(pAddress); }

	CIPAddress& operator=(u32 nAddress)
	{
		Set(nAddress);
		return *this;
	}

	void Set(u32 nAddress) { memcpy(m_Address, &nAddress, sizeof(m_Address)); }
	void Set(const u8* pAddress) { memcpy(m_Address, pAddress, sizeof(m_Address)); }
	void Set(const CIPAddress& Address) { Set(Address.m_Address); }
	const u8* Get() const { return m_Address; }
	operator u32() const
	{
		u32 nAddress;
		memcpy(&nAddress, m_Address, sizeof(nAddress));
		return nAddress;
	}
	boolean operator==(const CIPAddress& Other) const { return memcmp(m_Address, Other.m_Address, sizeof(m_Address)) == 0; }
	boolean operator!=(const CIPAddress& Other) const { return !(*this == Other); }

	void Format(CString* pString) const { pString->Format("%u.%u.%u.%u", m_Address[0], m_Address[1], m_Address[2], m_Address[3]); }

private:
	u8 m_Address[IP_ADDRESS_SIZE];
};

#endif
