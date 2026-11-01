//
// system.h
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

// Runs the whole of CMT32Pi (src/mt32pi.cpp) on the fake Circle: core 0 is a
// std::thread running Run(0); cores 1-3 are started by CMultiCoreSupport. MIDI
// goes in through the fake serial port; audio comes out of the fake sound
// device. Configure CFixture::Config before Start(); CSystem switches audio
// output to I2S (the maintainer's setup), because the PWM/HDMI 24-bit packing
// in AudioTask() uses misaligned 32-bit stores that UBSan stops on (audit O7).
//
// The main loop only stops on a reboot SysEx, which writes the volatile
// m_bRunning that every core reads (audit F7); tests end with Finish() instead,
// which exits the process without stopping the cores.

#ifndef _pim32_system_h
#define _pim32_system_h

#include "fixture.h"

#include <circle/gpiomanager.h>
#include <circle/i2cmaster.h>
#include <circle/serial.h>
#include <circle/spimaster.h>
#include <circle/usb/usbhcidevice.h>

#include <initializer_list>
#include <thread>

#include "mt32pi.h"

namespace pim32
{
	class CSystem
	{
	public:
		CSystem();

		bool Start();
		void Send(std::initializer_list<u8> Bytes);
		void Send(const u8* pData, size_t nSize);

		// Waits until the serial bytes sent so far have been read by the main loop
		bool WaitForMIDIRead(unsigned nTimeoutMillis = 10000);
		// Waits until the audio core has written at least nChunks more chunks
		bool WaitForAudio(unsigned nChunks, unsigned nTimeoutMillis = 10000);
		// Largest absolute sample since the last call (24-bit scale)
		s32 TakeAudioPeak();

		// Prints the summary and exits with the test result
		[[noreturn]] void Finish();

	private:
		CI2CMaster m_I2C;
		CSPIMaster m_SPI;
		CGPIOManager m_GPIO;
		CSerialDevice m_Serial;
		CUSBHCIDevice m_USB;
		CMT32Pi* m_pPi;
		std::thread m_Core0;
	};
}

#endif
