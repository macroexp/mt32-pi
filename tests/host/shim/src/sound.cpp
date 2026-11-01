//
// sound.cpp
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

#include <circle/sound/soundbasedevice.h>

#include <cstring>

namespace
{
	std::atomic<CSoundBaseDevice*> LastDevice{nullptr};
}

CSoundBaseDevice::CSoundBaseDevice(unsigned nSampleRate, unsigned nChunkSize)
{
	LastDevice = this;
}

CSoundBaseDevice::~CSoundBaseDevice()
{
	CSoundBaseDevice* pThis = this;
	LastDevice.compare_exchange_strong(pThis, nullptr);
}

boolean CSoundBaseDevice::AllocateQueueFrames(unsigned nSizeFrames)
{
	m_nQueueFrames = nSizeFrames;
	return TRUE;
}

boolean CSoundBaseDevice::Start()
{
	m_bActive = true;
	return TRUE;
}

void CSoundBaseDevice::Cancel()
{
	m_bActive = false;
}

int CSoundBaseDevice::Write(const void* pBuffer, size_t nCount)
{
	// Track the peak for 32-bit (I2S) or 24-bit packed samples
	const u8* pBytes = static_cast<const u8*>(pBuffer);
	const size_t nSampleBytes = m_Format == SoundFormatSigned24_32 ? 4 : 3;
	s32 nPeak = m_nPeak;
	for (size_t i = 0; i + nSampleBytes <= nCount; i += nSampleBytes)
	{
		s32 nSample = 0;
		memcpy(&nSample, pBytes + i, nSampleBytes);
		if (nSampleBytes == 3)
			nSample = (nSample << 8) >> 8;
		if (nSample < 0)
			nSample = -nSample;
		if (nSample > nPeak)
			nPeak = nSample;
	}
	m_nPeak = nPeak;

	m_nBytesWritten += nCount;
	++m_nWrites;
	return static_cast<int>(nCount);
}

CSoundBaseDevice* CSoundBaseDevice::GetLast()
{
	return LastDevice;
}

s32 CSoundBaseDevice::TakePeak()
{
	return m_nPeak.exchange(0);
}
