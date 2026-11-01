//
// soundfontsynth.h
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

#ifndef _soundfontsynth_h
#define _soundfontsynth_h

#include <circle/types.h>

#include <fluidsynth.h>

#include "soundfontmanager.h"
#include "synth/fxprofile.h"
#include "synth/synthbase.h"

class CSoundFontSynth : public CSynthBase
{
public:
	CSoundFontSynth(unsigned nSampleRate);
	virtual ~CSoundFontSynth() override;

	// CSynthBase
	virtual bool Initialize() override;
	virtual void HandleMIDIShortMessage(u32 nMessage) EXCLUDES(m_Lock) override;
	virtual void HandleMIDISysExMessage(const u8* pData, size_t nSize) EXCLUDES(m_Lock) override;
	virtual bool IsActive() EXCLUDES(m_Lock) override;
	virtual void AllSoundOff() EXCLUDES(m_Lock) override;
	virtual void SetMasterVolume(u8 nVolume) EXCLUDES(m_Lock) override;
	virtual size_t Render(s16* pOutBuffer, size_t nFrames) EXCLUDES(m_Lock) override;
	virtual size_t Render(float* pOutBuffer, size_t nFrames) EXCLUDES(m_Lock) override;
	virtual void ReportStatus() const override;
	virtual void UpdateLCD(CLCD& LCD, unsigned int nTicks) override;

	bool SwitchSoundFont(size_t nIndex);
	size_t GetSoundFontIndex() const { return m_nCurrentSoundFontIndex; }
	CSoundFontManager& GetSoundFontManager() { return m_SoundFontManager; }

private:
	// Builds and loads the new synth privately, then installs it under m_Lock (PR #25)
	bool Reinitialize(const char* pSoundFontPath, const TFXProfile* pFXProfile) EXCLUDES(m_Lock);
	void ResetMIDIMonitor();
#ifndef NDEBUG
	void DumpFXSettings(fluid_synth_t* pSynth) const;
#endif
	bool ParseGMSysEx(const u8* pData, size_t nSize);
	bool ParseRolandSysEx(const u8* pData, size_t nSize);
	bool ParseYamahaSysEx(const u8* pData, size_t nSize);

	// Per-channel MIDI state, recorded as messages arrive and replayed into a new
	// synth when the SoundFont changes, so a song keeps its instruments across a
	// switch. Only touched on the core that handles MIDI (core 0).
	static constexpr u8 TrackedControllers[] = { 1, 7, 10, 11, 91, 93 };	// mod, volume, pan, expression, reverb, chorus
	static constexpr size_t TrackedControllerCount = sizeof(TrackedControllers);
	static constexpr u8 TrackedRPNCount = 3;				// pitch bend range, fine tune, coarse tune
	static constexpr u8 NullRPN = 0x7F;

	struct TChannelState
	{
		bool bBankMSBSet, bBankLSBSet, bProgramSet, bPitchBendSet;
		u8 nBankMSB, nBankLSB, nProgram;		// bank selects as last received
		bool bProgramBankMSBSet, bProgramBankLSBSet;	// bank in effect when the program was chosen
		u8 nProgramBankMSB, nProgramBankLSB;
		u16 nPitchBend;
		u8 nControllerMask;
		u8 Controllers[TrackedControllerCount];
		u8 nRPNMSB, nRPNLSB;					// current RPN selection
		u8 nRPNCoarseMask, nRPNFineMask;
		u8 RPNCoarse[TrackedRPNCount], RPNFine[TrackedRPNCount];
	};

	void ResetChannelState();
	void RecordChannelState(u8 nStatus, u8 nChannel, u8 nData1, u8 nData2) REQUIRES(m_Lock);
	void ReplayChannelState(fluid_synth_t* pSynth) const REQUIRES(m_Lock);

	fluid_settings_t* m_pSettings;
	fluid_synth_t* m_pSynth GUARDED_BY(m_Lock) PT_GUARDED_BY(m_Lock);

	u8 m_nVolume;
	float m_nInitialGain;

	u16 m_nPercussionMask;
	size_t m_nCurrentSoundFontIndex;

	TChannelState m_ChannelState[16];

	CSoundFontManager m_SoundFontManager;

	static void FluidSynthLogCallback(int nLevel, const char* pMessage, void* pUser);
};

#endif
