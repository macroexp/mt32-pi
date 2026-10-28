//
// kernel.cpp
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2020-2023 Dale Whinham <daleyo@gmail.com>
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

#include "config.h"
#include "kernel.h"

#ifndef MT32_PI_VERSION
#define MT32_PI_VERSION "<unknown>"
#endif

CKernel::CKernel(void)
	: CStdlibApp("mt32-pi"),

#if RASPPI >= 5
	  // No FIQ on the Pi 5, and the default UART there is the debug connector,
	  // so ask for UART0 (GPIO 14/15, "ttyS1" as on other boards) on the IRQ
	  m_Serial(&mInterrupt, false, 0),
#else
	  m_Serial(&mInterrupt, true),
#endif
#ifdef HDMI_CONSOLE
	  m_Screen(mOptions.GetWidth(), mOptions.GetHeight()),
#endif

	  m_Timer(&mInterrupt),
	  m_Logger(mOptions.GetLogLevel(), &m_Timer),
	  m_USBHCI(&mInterrupt, &m_Timer, true),
	  m_EMMC(&mInterrupt, &m_Timer, &mActLED),
	  m_SDFileSystem{},

	  m_I2CMaster(1, true),
	  m_GPIOManager(&mInterrupt),

	  m_MT32Pi(&m_I2CMaster, &m_SPIMaster, &mInterrupt, &m_GPIOManager, &m_Serial, &m_USBHCI)
{
}

bool CKernel::Initialize(void)
{
	if (!CStdlibApp::Initialize())
		return false;

#ifdef HDMI_CONSOLE
	if (!m_Screen.Initialize())
		return false;
#endif

	const char* pLogDeviceName = mOptions.GetLogDevice();
	const bool bSerialMIDIAvailable = strcmp(pLogDeviceName, "ttyS1") != 0;

	// Init serial port early if used for logging
	if (!bSerialMIDIAvailable && !m_Serial.Initialize(115200))
		return false;

	CDevice* pLogTarget = mDeviceNameService.GetDevice(pLogDeviceName, false);

	if (!pLogTarget)
		pLogTarget = &mNullDevice;

	if (!m_Logger.Initialize(pLogTarget))
		return false;

	if (!m_Timer.Initialize())
		return false;

	if (!m_EMMC.Initialize())
		return false;

	if (f_mount(&m_SDFileSystem, "SD:", 1) != FR_OK)
	{
		m_Logger.Write(GetKernelName(), LogError, "Failed to mount SD card");
		return false;
	}

	// Load configuration file
	if (!m_Config.Initialize("mt32-pi.cfg"))
		m_Logger.Write(GetKernelName(), LogWarning, "Unable to find or parse config file; using defaults");

	// Init serial port for MIDI with preferred baud rate if not used for logging
	if (bSerialMIDIAvailable && !m_Serial.Initialize(m_Config.MIDIGPIOBaudRate))
		return false;

#if RASPPI >= 5
	// The Pi 5's RP1 I2C controller (DesignWare) must be disabled before it is
	// configured, and Initialize() also sets up its FIFO thresholds
	if (!m_I2CMaster.Initialize())
		m_Logger.Write(GetKernelName(), LogWarning, "Failed to initialize I2C");
#endif

	// Init I2C; on Pi 1-4 don't bother with Initialize() as it only sets the clock to 100/400KHz
	m_I2CMaster.SetClock(m_Config.SystemI2CBaudRate);

	// Debug (branch pi5-i2c-scan, never merge): scan the I2C bus and log what answers
	{
		const char* pName = GetKernelName();
		m_Logger.Write(pName, LogNotice, "I2C scan at %u Hz (bus 1, GPIO 2/3)", m_Config.SystemI2CBaudRate);
		unsigned nFound = 0, nErrors = 0;
		for (u8 nAddress = 0x08; nAddress <= 0x77; ++nAddress)
		{
			u8 nByte;
			const int nResult = m_I2CMaster.Read(nAddress, &nByte, 1);
			if (nResult >= 0)
			{
				m_Logger.Write(pName, LogNotice, "I2C: device at 0x%02X (read %d)", nAddress, nResult);
				++nFound;
			}
			else if (nResult != -2)
			{
				m_Logger.Write(pName, LogNotice, "I2C: 0x%02X error %d", nAddress, nResult);
				// Timeouts take 0.5 s each; don't stall the boot for a minute
				if (++nErrors >= 5)
				{
					m_Logger.Write(pName, LogNotice, "I2C: too many errors, scan stopped");
					break;
				}
			}
		}
		m_Logger.Write(pName, LogNotice, "I2C scan done, %u device(s)", nFound);

		const u8 DisplayOff[] = { 0x80, 0xAE };
		const int nWrite = m_I2CMaster.Write(m_Config.LCDI2CLCDAddress, DisplayOff, sizeof(DisplayOff));
		m_Logger.Write(pName, LogNotice, "I2C: 2-byte write to LCD address 0x%02X returned %d", m_Config.LCDI2CLCDAddress, nWrite);
	}

	// Init SPI
	if (!m_SPIMaster.Initialize())
		return false;

	// Init GPIO manager
	if (!m_GPIOManager.Initialize())
		return false;

	// Init custom memory allocator
	if (!m_Allocator.Initialize())
		return false;

	if (!m_MT32Pi.Initialize(bSerialMIDIAvailable))
		return false;

	return true;
}

CStdlibApp::TShutdownMode CKernel::Run(void)
{
	m_Logger.Write(GetKernelName(), LogNotice, "mt32-pi " MT32_PI_VERSION);
	m_Logger.Write(GetKernelName(), LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_MT32Pi.Run(0);

	return ShutdownReboot;
}
