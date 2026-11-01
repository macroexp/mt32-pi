//
// serial.h
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

// Fake Circle: serial device fed by the test (host test harness)
#ifndef _circle_serial_h
#define _circle_serial_h

#include <circle/device.h>
#include <circle/interrupt.h>
#include <circle/types.h>
#include <deque>
#include <mutex>

#define SERIAL_ERROR_BREAK 1
#define SERIAL_ERROR_OVERRUN 2
#define SERIAL_ERROR_FRAMING 3

class CSerialDevice : public CDevice
{
public:
	CSerialDevice(CInterruptSystem* pInterruptSystem = nullptr, boolean bUseFIQ = FALSE, unsigned nDevice = 0) {}
	boolean Initialize(unsigned nBaudrate = 115200) { return TRUE; }

	virtual int Read(void* pBuffer, size_t nCount) override;
	virtual int Write(const void* pBuffer, size_t nCount) override { return static_cast<int>(nCount); }

	// Test hooks: bytes for Read() to return, and whether they have all been read
	void Inject(const u8* pData, size_t nSize);
	bool IsDrained();

private:
	std::mutex m_Mutex;
	std::deque<u8> m_RxQueue;
};

#endif
