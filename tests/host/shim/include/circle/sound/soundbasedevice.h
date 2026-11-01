//
// soundbasedevice.h
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

// Fake Circle: sound device that accepts every frame at once (host test harness)
//
// The queue is always empty, so the audio task renders continuously. Tests can
// read how many frames were written and the last samples.
#ifndef _circle_sound_soundbasedevice_h
#define _circle_sound_soundbasedevice_h

#include <atomic>
#include <circle/interrupt.h>
#include <circle/types.h>

enum TSoundFormat
{
	SoundFormatUnsigned8,
	SoundFormatSigned16,
	SoundFormatSigned24,
	SoundFormatSigned24_32,
	SoundFormatUnsigned32,
	SoundFormatUnknown
};

#define IEC958_SUBFRAMES_PER_BLOCK 384

class CSoundBaseDevice
{
public:
	CSoundBaseDevice(unsigned nSampleRate = 48000, unsigned nChunkSize = 256);
	virtual ~CSoundBaseDevice();

	void SetWriteFormat(TSoundFormat Format, unsigned nChannels = 2) { m_Format = Format; }
	boolean AllocateQueueFrames(unsigned nSizeFrames);

	virtual boolean Start();
	virtual void Cancel();
	virtual boolean IsActive() const { return m_bActive; }

	int Write(const void* pBuffer, size_t nCount);
	unsigned GetQueueSizeFrames() const { return m_nQueueFrames; }
	unsigned GetQueueFramesAvail() { return 0; }

	// Test hooks
	static CSoundBaseDevice* GetLast();
	u64 GetBytesWritten() const { return m_nBytesWritten; }
	unsigned GetWriteCount() const { return m_nWrites; }
	// Largest absolute 32-bit sample value written since the last call
	s32 TakePeak();

private:
	TSoundFormat m_Format = SoundFormatSigned16;
	unsigned m_nQueueFrames = 0;
	std::atomic<bool> m_bActive{false};
	std::atomic<u64> m_nBytesWritten{0};
	std::atomic<unsigned> m_nWrites{0};
	std::atomic<s32> m_nPeak{0};
};

#endif
