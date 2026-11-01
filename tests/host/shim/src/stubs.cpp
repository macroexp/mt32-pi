//
// stubs.cpp
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

// Link stubs for pim-32 classes the harness doesn't build (network, Pisound).
// The harness never brings the network up and has no SPI hardware, so these
// are never constructed in a test; CMT32Pi only needs the symbols.

#include "net/applemidi.h"
#include "net/ftpdaemon.h"
#include "net/udpmidi.h"
#include "pisound.h"

CAppleMIDIParticipant::CAppleMIDIParticipant(CBcmRandomNumberGenerator* pRandom, CAppleMIDIHandler* pHandler)
	: m_pRandom(pRandom),
	  m_pControlSocket(nullptr),
	  m_pMIDISocket(nullptr),
	  m_nForeignControlPort(0),
	  m_nForeignMIDIPort(0),
	  m_nInitiatorControlPort(0),
	  m_nInitiatorMIDIPort(0),
	  m_nControlResult(0),
	  m_nMIDIResult(0),
	  m_pHandler(pHandler),
	  m_State(TState::ControlInvitation)
{
}

CAppleMIDIParticipant::~CAppleMIDIParticipant() = default;
bool CAppleMIDIParticipant::Initialize() { return false; }
void CAppleMIDIParticipant::Run() {}

CUDPMIDIReceiver::CUDPMIDIReceiver(CUDPMIDIHandler* pHandler)
	: m_pMIDISocket(nullptr),
	  m_pHandler(pHandler)
{
}

CUDPMIDIReceiver::~CUDPMIDIReceiver() = default;
bool CUDPMIDIReceiver::Initialize() { return false; }
void CUDPMIDIReceiver::Run() {}

CFTPDaemon::CFTPDaemon(const char* pUser, const char* pPassword)
	: m_pListenSocket(nullptr),
	  m_pUser(pUser),
	  m_pPassword(pPassword)
{
}

CFTPDaemon::~CFTPDaemon() = default;
bool CFTPDaemon::Initialize() { return false; }
void CFTPDaemon::Run() {}

CPisound::CPisound(CSPIMaster* pSPIMaster, CGPIOManager* pGPIOManager, unsigned nSamplerate)
	: m_pSPIMaster(pSPIMaster),
	  m_nSamplerate(nSamplerate),
	  m_pReceiveHandler(nullptr)
{
}

CPisound::~CPisound() = default;

// No Pisound attached
bool CPisound::Initialize() { return false; }
