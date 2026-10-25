//
// mt32synth.cpp
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

#include <cstdio>

#include <circle/logger.h>
#include <circle/timer.h>

#include "config.h"
#include "lcd/ui.h"
#include "synth/mt32synth.h"
#include "utility.h"

LOGMODULE("mt32synth");

constexpr size_t ROMOffsetVersionStringOld  = 0x4015;
constexpr size_t ROMOffsetVersionString1_07 = 0x4011;
constexpr size_t ROMOffsetVersionStringNew  = 0x2206;
constexpr u32 MemoryAddressMIDIChannels     = 0x4000D;
constexpr u32 MemoryAddressMasterVolume     = 0x40016;
constexpr u32 MemoryAddressPatchTemp        = 0x0C000;

#ifdef MIDI_DEBUG_CHANNEL
// Debug builds: log non-note messages on this MIDI channel (1-16) and its part's state
constexpr u8 DebugChannel = MIDI_DEBUG_CHANNEL - 1;
constexpr u32 MemoryAddressTimbreTemp   = 0x10000; // SysEx 04 00 00
constexpr u32 MemoryAddressTimbreMemory = 0x20000; // SysEx 08 00 00
constexpr size_t TimbreParamSize        = 246;
constexpr size_t PaddedTimbreSize       = 256;

static unsigned int DebugByteSum(const u8* pData, size_t nSize)
{
	unsigned int nSum = 0;
	for (size_t i = 0; i < nSize; ++i)
		nSum += pData[i];
	return nSum;
}

// Fingerprints of Munt's decoded PCM waves, to catch memory corruption on the device
constexpr unsigned int DebugMaxPCMWaves = 256;
static u32 DebugPCMReference[DebugMaxPCMWaves];
static unsigned int nDebugPCMWaves = 0;
static unsigned int nDebugPCMChecks = 0;

static void DebugCapturePCM(MT32Emu::Synth* pSynth)
{
	nDebugPCMWaves = Utility::Min(pSynth->debugPCMWaveCount(), DebugMaxPCMWaves);
	nDebugPCMChecks = 0;
	for (unsigned int i = 0; i < nDebugPCMWaves; ++i)
		DebugPCMReference[i] = pSynth->debugPCMWaveChecksum(i);

	LOGNOTE("PCM: %u waves fingerprinted; wave 39 %08X, wave 42 %08X", nDebugPCMWaves,
		nDebugPCMWaves > 39 ? DebugPCMReference[39] : 0, nDebugPCMWaves > 42 ? DebugPCMReference[42] : 0);
	for (unsigned int i = 0; i < nDebugPCMWaves; i += 8)
		LOGNOTE("PCM %3u: %08X %08X %08X %08X %08X %08X %08X %08X", i,
			DebugPCMReference[i], DebugPCMReference[(i + 1) % nDebugPCMWaves],
			DebugPCMReference[(i + 2) % nDebugPCMWaves], DebugPCMReference[(i + 3) % nDebugPCMWaves],
			DebugPCMReference[(i + 4) % nDebugPCMWaves], DebugPCMReference[(i + 5) % nDebugPCMWaves],
			DebugPCMReference[(i + 6) % nDebugPCMWaves], DebugPCMReference[(i + 7) % nDebugPCMWaves]);
}

static void DebugCheckPCM(MT32Emu::Synth* pSynth)
{
	unsigned int nChanged = 0;
	for (unsigned int i = 0; i < nDebugPCMWaves; ++i)
	{
		const u32 nSum = pSynth->debugPCMWaveChecksum(i);
		if (nSum != DebugPCMReference[i])
		{
			if (nChanged < 8)
				LOGERR("PCM wave %u CHANGED: %08X -> %08X", i, DebugPCMReference[i], nSum);
			++nChanged;
		}
	}

	++nDebugPCMChecks;
	if (nChanged)
		LOGERR("PCM check %u: %u wave(s) CORRUPTED since startup", nDebugPCMChecks, nChanged);
	else
		LOGNOTE("PCM check %u: all %u waves OK (39 %08X, 42 %08X)", nDebugPCMChecks, nDebugPCMWaves,
			nDebugPCMWaves > 39 ? pSynth->debugPCMWaveChecksum(39) : 0,
			nDebugPCMWaves > 42 ? pSynth->debugPCMWaveChecksum(42) : 0);
}

// First 10 bytes of a timbre are its name
static void DebugDescribeTimbre(const u8* pTimbre, char* pName)
{
	for (size_t i = 0; i < 10; ++i)
		pName[i] = (pTimbre[i] >= 0x20 && pTimbre[i] < 0x7F) ? static_cast<char>(pTimbre[i]) : '.';
	pName[10] = '\0';
}
#endif

// SysEx commands for setting MIDI channel assignment (no SysEx framing, just 3-byte address and 9 channel values)
const u8 CMT32Synth::StandardMIDIChannelsSysEx[] = { 0x10, 0x00, 0x0D, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };
const u8 CMT32Synth::AlternateMIDIChannelsSysEx[] = { 0x10, 0x00, 0x0D, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09 };

CMT32Synth::CMT32Synth(unsigned nSampleRate, float nGain, float nReverbGain, TResamplerQuality ResamplerQuality)
	: CSynthBase(nSampleRate),

	  m_pSynth(nullptr),

	  m_nGain(nGain),
	  m_nReverbGain(nReverbGain),

	  m_ResamplerQuality(ResamplerQuality),
	  m_pSampleRateConverter(nullptr),

	  m_CurrentROMSet(TMT32ROMSet::Any),
	  m_pControlROMImage(nullptr),
	  m_pPCMROMImage(nullptr),

	  m_LCDTextBuffer{'\0'}
{
}

CMT32Synth::~CMT32Synth()
{
	if (m_pSynth)
		delete m_pSynth;

	if (m_pSampleRateConverter)
		delete m_pSampleRateConverter;
}

bool CMT32Synth::Initialize()
{
	if (!m_ROMManager.ScanROMs())
		return false;

	// Try to load user's preferred initial ROM set, otherwise fall back on first available
	TMT32ROMSet InitialROMSet = CConfig::Get()->MT32EmuROMSet;
	if (!m_ROMManager.HaveROMSet(InitialROMSet))
		InitialROMSet = TMT32ROMSet::Any;

	if (!m_ROMManager.GetROMSet(InitialROMSet, m_CurrentROMSet, m_pControlROMImage, m_pPCMROMImage))
		return false;

	m_pSynth = new MT32Emu::Synth(this);

	if (!m_pSynth->open(*m_pControlROMImage, *m_pPCMROMImage))
		return false;

#ifdef MIDI_DEBUG_CHANNEL
	DebugCapturePCM(m_pSynth);
#endif

	m_pSynth->setOutputGain(m_nGain);
	m_pSynth->setReverbOutputGain(m_nReverbGain);

	if (m_ResamplerQuality != TResamplerQuality::None)
	{
		auto quality = MT32Emu::SamplerateConversionQuality_GOOD;
		switch (m_ResamplerQuality)
		{
			case TResamplerQuality::Fastest:
				quality = MT32Emu::SamplerateConversionQuality_FASTEST;
				break;

			case TResamplerQuality::Fast:
				quality = MT32Emu::SamplerateConversionQuality_FAST;
				break;

			case TResamplerQuality::Good:
				quality = MT32Emu::SamplerateConversionQuality_GOOD;
				break;

			case TResamplerQuality::Best:
				quality = MT32Emu::SamplerateConversionQuality_BEST;
				break;

			default:
				break;
		}

		m_pSampleRateConverter = new MT32Emu::SampleRateConverter(*m_pSynth, m_nSampleRate, quality);
	}

	return true;
}

void CMT32Synth::HandleMIDIShortMessage(u32 nMessage)
{
#ifdef MIDI_DEBUG_CHANNEL
	const u8 nStatus  = nMessage & 0xFF;
	const u8 nCommand = nStatus & 0xF0;
	if (nStatus < 0xF0 && (nStatus & 0x0F) == DebugChannel && nCommand != 0x80 && nCommand != 0x90)
		LOGNOTE("ch%d in: %02X %02X %02X", DebugChannel + 1, nStatus, (nMessage >> 8) & 0xFF, (nMessage >> 16) & 0xFF);
#endif

	if (!m_pSynth->playMsg(nMessage))
		LOGWARN("mt32emu rejected MIDI message %06X", static_cast<unsigned int>(nMessage));

	// Update MIDI monitor
	CSynthBase::HandleMIDIShortMessage(nMessage);
}

void CMT32Synth::HandleMIDISysExMessage(const u8* pData, size_t nSize)
{
#ifdef MIDI_DEBUG_CHANNEL
	if (nSize >= 9)
		LOGNOTE("SysEx in: %u bytes, dev %02X cmd %02X addr %02X %02X %02X", static_cast<unsigned int>(nSize), pData[2], pData[4], pData[5], pData[6], pData[7]);
	else
		LOGNOTE("SysEx in: %u bytes", static_cast<unsigned int>(nSize));
#endif

	if (!m_pSynth->playSysex(pData, nSize))
		LOGWARN("mt32emu rejected SysEx message (%u bytes)", static_cast<unsigned int>(nSize));
}

void CMT32Synth::AllSoundOff()
{
	// Stop all sound immediately; mt32emu treats CC 0x7C like "All Sound Off", ignoring pedal
	for (uint8_t i = 0; i < 8; ++i)
		m_pSynth->playMsgOnPart(i, 0x0B, 0x7C, 0);

	// Reset MIDI monitor
	CSynthBase::AllSoundOff();
}

void CMT32Synth::SetMasterVolume(u8 nVolume)
{
	const u8 SetVolumeSysEx[] = { 0x10, 0x00, 0x16, nVolume };
	m_pSynth->writeSysex(0x10, SetVolumeSysEx, sizeof(SetVolumeSysEx));
}

size_t CMT32Synth::Render(s16* pOutBuffer, size_t nFrames)
{
	m_Lock.Acquire();
	if (m_pSampleRateConverter)
		m_pSampleRateConverter->getOutputSamples(pOutBuffer, nFrames);
	else
		m_pSynth->render(pOutBuffer, nFrames);
	m_Lock.Release();

	return nFrames;
}

size_t CMT32Synth::Render(float* pOutBuffer, size_t nFrames)
{
	m_Lock.Acquire();
	if (m_pSampleRateConverter)
		m_pSampleRateConverter->getOutputSamples(pOutBuffer, nFrames);
	else
		m_pSynth->render(pOutBuffer, nFrames);
	m_Lock.Release();

	return nFrames;
}

void CMT32Synth::ReportStatus() const
{
	if (m_pUI)
		m_pUI->ShowSystemMessage(GetControlROMName());
}

void CMT32Synth::UpdateLCD(CLCD& LCD, unsigned int nTicks)
{
	bool bNarrowPartStateText = false;

	const u8 nWidth = LCD.Width();
	const u8 nHeight = LCD.Height();
	u8 nStatusRow, nBarHeight;

	if (LCD.GetType() == CLCD::TType::Character)
	{
		nStatusRow = nHeight - 1;
		nBarHeight = nHeight - 1;

		// For 16-character wide LCDs
		if (nWidth < 20)
			bNarrowPartStateText = true;
	}
	else
	{
		nStatusRow = nHeight / 16 - 1;
		nBarHeight = nHeight - 16;
	}

#ifdef MIDI_DEBUG_CHANNEL
	static unsigned int nLastDebugTicks = 0;
	if (nTicks - nLastDebugTicks >= Utility::MillisToTicks(2000u))
	{
		nLastDebugTicks = nTicks;
		DebugCheckPCM(m_pSynth);
		u8 Channels[9];
		m_pSynth->readMemory(MemoryAddressMIDIChannels, sizeof(Channels), Channels);
		for (u8 nPart = 0; nPart < 8; ++nPart)
		{
			if (Channels[nPart] != DebugChannel)
				continue;
			u8 PatchTemp[16], Keys[256], Velocities[256];
			m_pSynth->readMemory(MemoryAddressPatchTemp + nPart * sizeof(PatchTemp), sizeof(PatchTemp), PatchTemp);
			const unsigned int nPlaying = m_pSynth->getPlayingNotes(nPart, Keys, Velocities);
			LOGNOTE("ch%d part %d: timbre %c%02d level %d pan %d playing %u", DebugChannel + 1, nPart + 1,
				"ABMR"[PatchTemp[0] & 3], PatchTemp[1] + 1, PatchTemp[8], PatchTemp[9], nPlaying);

			// The part's working copy of its timbre (what it actually plays)
			u8 Timbre[TimbreParamSize];
			char Name[11];
			m_pSynth->readMemory(MemoryAddressTimbreTemp + nPart * TimbreParamSize, TimbreParamSize, Timbre);
			DebugDescribeTimbre(Timbre, Name);
			LOGNOTE("  temp \"%s\" sum %u", Name, DebugByteSum(Timbre, TimbreParamSize));

			// Timbre Memory entry, if the part uses an uploaded (group M) timbre
			if ((PatchTemp[0] & 3) == 2)
			{
				m_pSynth->readMemory(MemoryAddressTimbreMemory + (PatchTemp[1] & 0x3F) * PaddedTimbreSize, TimbreParamSize, Timbre);
				DebugDescribeTimbre(Timbre, Name);
				LOGNOTE("  mem  \"%s\" sum %u", Name, DebugByteSum(Timbre, TimbreParamSize));
			}
		}

		// Every 10 s: every part's instrument, as "part:timbre temp-sum/memory-sum"; "!" if they differ
		if (nDebugPCMChecks % 5 == 1)
		{
			char Line[2][128];
			for (u8 nHalf = 0; nHalf < 2; ++nHalf)
			{
				size_t nLength = 0;
				for (u8 nPart = nHalf * 4; nPart < nHalf * 4 + 4; ++nPart)
				{
					u8 PatchTemp[16], Timbre[TimbreParamSize];
					m_pSynth->readMemory(MemoryAddressPatchTemp + nPart * sizeof(PatchTemp), sizeof(PatchTemp), PatchTemp);
					m_pSynth->readMemory(MemoryAddressTimbreTemp + nPart * TimbreParamSize, TimbreParamSize, Timbre);
					const unsigned int nTempSum = DebugByteSum(Timbre, TimbreParamSize);
					unsigned int nMemSum = nTempSum;
					if ((PatchTemp[0] & 3) == 2)
					{
						m_pSynth->readMemory(MemoryAddressTimbreMemory + (PatchTemp[1] & 0x3F) * PaddedTimbreSize, TimbreParamSize, Timbre);
						nMemSum = DebugByteSum(Timbre, TimbreParamSize);
					}
					nLength += snprintf(Line[nHalf] + nLength, sizeof(Line[nHalf]) - nLength, " %u:%c%02d %u/%u%s",
						nPart + 1, "ABMR"[PatchTemp[0] & 3], PatchTemp[1] + 1, nTempSum, nMemSum, nTempSum == nMemSum ? "" : "!");
				}
			}
			LOGNOTE("parts%s", Line[0]);
			LOGNOTE("parts%s", Line[1]);
		}
	}
#endif

	float PartLevels[9], PartPeaks[9];
	GetPartLevels(nTicks, PartLevels, PartPeaks);
	CUserInterface::DrawChannelLevels(LCD, nBarHeight, PartLevels, PartPeaks, 9, false);

	m_pSynth->getDisplayState(m_LCDTextBuffer, bNarrowPartStateText);

	// Remap active part indicator character
	for (size_t i = 0; i < Utility::ArraySize(m_LCDTextBuffer) - 1; ++i)
		if (m_LCDTextBuffer[i] == 1)
			m_LCDTextBuffer[i] = '\xFF';

	LCD.Print(m_LCDTextBuffer, 0, nStatusRow, true, false);
}

void CMT32Synth::SetMIDIChannels(TMIDIChannels Channels)
{
	if (Channels == TMIDIChannels::Standard)
		m_pSynth->writeSysex(0x10, StandardMIDIChannelsSysEx, sizeof(StandardMIDIChannelsSysEx));
	else
		m_pSynth->writeSysex(0x10, AlternateMIDIChannelsSysEx, sizeof(AlternateMIDIChannelsSysEx));
}

bool CMT32Synth::SwitchROMSet(TMT32ROMSet ROMSet)
{
	const MT32Emu::ROMImage* pControlROMImage;
	const MT32Emu::ROMImage* pPCMROMImage;

	// Is this ROM set already active?
	if (ROMSet == m_CurrentROMSet)
	{
		if (m_pUI)
			m_pUI->ShowSystemMessage("Already selected!");
		return false;
	}

	// Get ROM set if available
	if (!m_ROMManager.GetROMSet(ROMSet, m_CurrentROMSet, pControlROMImage, pPCMROMImage))
	{
		if (m_pUI)
			m_pUI->ShowSystemMessage("ROM set not avail!");
		return false;
	}

	// Reopen synth with new ROMs
	m_Lock.Acquire();
	m_pSynth->close();
	assert(m_pSynth->open(*pControlROMImage, *pPCMROMImage));
#ifdef MIDI_DEBUG_CHANNEL
	DebugCapturePCM(m_pSynth);
#endif
	m_pSynth->setOutputGain(m_nGain);
	m_pSynth->setReverbOutputGain(m_nReverbGain);
	m_Lock.Release();

	m_pControlROMImage = pControlROMImage;
	m_pPCMROMImage     = pPCMROMImage;

	return true;
}

TMT32ROMSet CMT32Synth::GetROMSet() const
{
	return m_CurrentROMSet;
}

bool CMT32Synth::NextROMSet()
{
	const u8 nCurrentROMSetIndex = static_cast<u8>(m_CurrentROMSet);
	u8 nNextROMSetIndex = (static_cast<u8>(m_CurrentROMSet) + 1) % 3;

	// Find the next available ROM set
	while (nNextROMSetIndex != nCurrentROMSetIndex && !m_ROMManager.HaveROMSet(static_cast<TMT32ROMSet>(nNextROMSetIndex)))
		nNextROMSetIndex = (nNextROMSetIndex + 1) % 3;

	if (nNextROMSetIndex == nCurrentROMSetIndex)
	{
		if (m_pUI)
			m_pUI->ShowSystemMessage("No other ROM sets!");
		return false;
	}

	return SwitchROMSet(static_cast<TMT32ROMSet>(nNextROMSetIndex));
}

const char* CMT32Synth::GetControlROMName() const
{
	// +5 to skip 'ctrl_'
	const char* pShortName = m_pControlROMImage->getROMInfo()->shortName + 5;
	const MT32Emu::Bit8u* pROMData = m_pControlROMImage->getFile()->getData();
	size_t nOffset;

	// Find version strings from ROMs
	// FIXME: Missing offset for CM-32LN ROM
	if (strstr(pShortName, "cm32l") || strstr(pShortName, "2_04") || strstr(pShortName, "2_06") || strstr(pShortName, "2_07"))
		nOffset = ROMOffsetVersionStringNew;
	else if (strstr(pShortName, "1_07") || strstr(pShortName, "bluer"))
		nOffset = ROMOffsetVersionString1_07;
	else
		nOffset = ROMOffsetVersionStringOld;

	return reinterpret_cast<const char*>(pROMData + nOffset);
}

u8 CMT32Synth::GetMasterVolume() const
{
	u8 nVolume;
	m_pSynth->readMemory(MemoryAddressMasterVolume, 1, &nVolume);
	return nVolume;
}

void CMT32Synth::GetPartLevels(unsigned int nTicks, float PartLevels[9], float PartPeaks[9])
{
	float ChannelLevels[16], ChannelPeaks[16];
	u8 MIDIChannelPartMap[9];
	u16 nPercussionMask;

	// Find which MIDI channels each MT-32 part is mapped to and identify percussion channel
	m_pSynth->readMemory(MemoryAddressMIDIChannels, 9, MIDIChannelPartMap);
	nPercussionMask = 1 << MIDIChannelPartMap[8];

	// Map channel levels to part levels
	m_MIDIMonitor.GetChannelLevels(nTicks, ChannelLevels, ChannelPeaks, nPercussionMask);
	for (u8 nPart = 0; nPart < 9; ++nPart)
	{
		const u8 nChannel = MIDIChannelPartMap[nPart];
		PartLevels[nPart] = ChannelLevels[nChannel];
		PartPeaks[nPart] = ChannelPeaks[nChannel];
	}
}

bool CMT32Synth::onMIDIQueueOverflow()
{
	LOGERR("MIDI queue overflow");
	return false;
}

void CMT32Synth::printDebug(const char* pFmt, va_list pList)
{
	//LOGDBG(pFmt, pList);
}

void CMT32Synth::showLCDMessage(const char* pMessage)
{
	LOGNOTE("LCD: %s", pMessage);
}

void CMT32Synth::onDeviceReset()
{
	LOGDBG("MT-32 reset");
	m_MIDIMonitor.AllNotesOff();
	m_MIDIMonitor.ResetControllers(false);
}
