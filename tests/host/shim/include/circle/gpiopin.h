//
// gpiopin.h
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

// Fake Circle: GPIO pins read high (pulled up, nothing pressed) (host test harness)
#ifndef _circle_gpiopin_h
#define _circle_gpiopin_h

#include <circle/types.h>

#define LOW 0
#define HIGH 1
#define GPIO_PINS 54

enum TGPIOMode
{
	GPIOModeInput,
	GPIOModeOutput,
	GPIOModeInputPullUp,
	GPIOModeInputPullDown,
	GPIOModeAlternateFunction0,
	GPIOModeUnknown
};

enum TGPIOInterrupt
{
	GPIOInterruptOnRisingEdge,
	GPIOInterruptOnFallingEdge,
	GPIOInterruptOnBothEdges,
	GPIOInterruptOnHighLevel,
	GPIOInterruptOnLowLevel,
	GPIOInterruptOnAsyncRisingEdge,
	GPIOInterruptOnAsyncFallingEdge,
	GPIOInterruptUnknown
};

class CGPIOManager;
using TGPIOInterruptHandler = void(void* pParam);

class CGPIOPin
{
public:
	CGPIOPin() = default;
	CGPIOPin(unsigned nPin, TGPIOMode Mode, CGPIOManager* pManager = nullptr) : m_nPin(nPin), m_Mode(Mode) {}

	void AssignPin(unsigned nPin) { m_nPin = nPin; }
	void SetMode(TGPIOMode Mode, boolean bInitPin = TRUE) { m_Mode = Mode; }
	void Write(unsigned nValue) { m_nValue = nValue; }
	unsigned Read() const { return m_Mode == GPIOModeOutput ? m_nValue : HIGH; }
	void Invert() { m_nValue ^= 1; }

	void ConnectInterrupt(TGPIOInterruptHandler* pHandler, void* pParam, boolean bAutoAck = TRUE) {}
	void DisconnectInterrupt() {}
	void EnableInterrupt(TGPIOInterrupt Interrupt) {}
	void DisableInterrupt() {}

	static u32 ReadAll() { return 0xFFFFFFFF; }

private:
	unsigned m_nPin = 0;
	TGPIOMode m_Mode = GPIOModeUnknown;
	unsigned m_nValue = LOW;
};

#endif
