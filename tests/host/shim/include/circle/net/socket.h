//
// socket.h
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

// Fake Circle: sockets; the harness has no network (host test harness)
#ifndef _circle_net_socket_h
#define _circle_net_socket_h

#include <circle/net/in.h>
#include <circle/net/ipaddress.h>
#include <circle/sysconfig.h>
#include <circle/types.h>

class CNetSubSystem;

class CSocket
{
public:
	CSocket(CNetSubSystem* pNetSubSystem, int nProtocol) {}
	virtual ~CSocket() = default;
	int Bind(u16 nOwnPort) { return -1; }
	int Connect(CIPAddress& ForeignIP, u16 nForeignPort) { return -1; }
	int Listen(unsigned nBackLog = 4) { return -1; }
	CSocket* Accept(CIPAddress* pForeignIP, u16* pForeignPort) { return nullptr; }
	int Send(const void* pBuffer, unsigned nLength, int nFlags) { return -1; }
	int Receive(void* pBuffer, unsigned nLength, int nFlags) { return -1; }
	int SendTo(const void* pBuffer, unsigned nLength, int nFlags, CIPAddress& ForeignIP, u16 nForeignPort) { return -1; }
	int ReceiveFrom(void* pBuffer, unsigned nLength, int nFlags, CIPAddress* pForeignIP, u16* pForeignPort) { return -1; }
};

#endif
