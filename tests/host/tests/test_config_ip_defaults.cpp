//
// test_config_ip_defaults.cpp
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

// Known bug O5 (concurrency audit, "Other bugs"): config.def gives the static
// IP defaults as u32 literals such as 0xc0a80164, but CIPAddress keeps a u32
// in network byte order, so on the (little-endian) Pi the built-in default
// address reads 100.1.168.192 instead of 192.168.1.100. It only matters with
// dhcp = off and no ip_address line. Expected to fail until the fix lands.

#include "check.h"
#include "config.h"

#include <string>

namespace
{
	std::string Format(const CIPAddress& Address)
	{
		CString String;
		Address.Format(&String);
		return static_cast<const char*>(String);
	}
}

TEST(BuiltInStaticAddressDefaults)
{
	CConfig Config;
	CHECK(Format(Config.NetworkIPAddress) == "192.168.1.100");
	CHECK(Format(Config.NetworkSubnetMask) == "255.255.255.0");
	CHECK(Format(Config.NetworkDefaultGateway) == "192.168.1.1");
	CHECK(Format(Config.NetworkDNSServer) == "192.168.1.1");
}
