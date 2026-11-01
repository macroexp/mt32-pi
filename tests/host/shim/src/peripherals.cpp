//
// peripherals.cpp
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

// Fake Circle: singletons and the test-fed serial device (host test harness)

#include <circle/actled.h>
#include <circle/cputhrottle.h>
#include <circle/devicenameservice.h>
#include <circle/interrupt.h>
#include <circle/serial.h>

#include <algorithm>

CInterruptSystem* CInterruptSystem::Get()
{
	static CInterruptSystem InterruptSystem;
	return &InterruptSystem;
}

CActLED* CActLED::Get()
{
	static CActLED ActLED;
	return &ActLED;
}

CDeviceNameService* CDeviceNameService::Get()
{
	static CDeviceNameService DeviceNameService;
	return &DeviceNameService;
}

CCPUThrottle* CCPUThrottle::Get()
{
	static CCPUThrottle CPUThrottle;
	return &CPUThrottle;
}

int CSerialDevice::Read(void* pBuffer, size_t nCount)
{
	std::lock_guard<std::mutex> Lock(m_Mutex);
	const size_t nBytes = std::min(nCount, m_RxQueue.size());
	u8* pOut = static_cast<u8*>(pBuffer);
	for (size_t i = 0; i < nBytes; ++i)
	{
		pOut[i] = m_RxQueue.front();
		m_RxQueue.pop_front();
	}
	return static_cast<int>(nBytes);
}

void CSerialDevice::Inject(const u8* pData, size_t nSize)
{
	std::lock_guard<std::mutex> Lock(m_Mutex);
	m_RxQueue.insert(m_RxQueue.end(), pData, pData + nSize);
}

bool CSerialDevice::IsDrained()
{
	std::lock_guard<std::mutex> Lock(m_Mutex);
	return m_RxQueue.empty();
}
