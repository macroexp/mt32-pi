//
// netsubsystem.h
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

// Fake Circle: network subsystem; never comes up (host test harness)
#ifndef _circle_net_netsubsystem_h
#define _circle_net_netsubsystem_h

#include <circle/net/ipaddress.h>
#include <circle/types.h>

enum TNetDeviceType
{
	NetDeviceTypeEthernet,
	NetDeviceTypeWLAN,
	NetDeviceTypeAny,
	NetDeviceTypeUnknown
};

class CNetConfig
{
public:
	const CIPAddress* GetIPAddress() const { return &m_IPAddress; }

private:
	CIPAddress m_IPAddress;
};

class CNetDevice
{
public:
	static CNetDevice* GetNetDevice(TNetDeviceType Type) { return nullptr; }
	boolean IsLinkUp() { return FALSE; }
};

class CNetSubSystem
{
public:
	CNetSubSystem(const u8* pIPAddress = 0, const u8* pNetMask = 0, const u8* pDefaultGateway = 0, const u8* pDNSServer = 0,
		      const char* pHostname = "raspberrypi", TNetDeviceType DeviceType = NetDeviceTypeEthernet) {}
	boolean Initialize(boolean bWaitForActivate = TRUE) { return FALSE; }
	boolean IsRunning() const { return FALSE; }
	CNetConfig* GetConfig() { return &m_Config; }
	static CNetSubSystem* Get() { return nullptr; }

private:
	CNetConfig m_Config;
};

#endif
