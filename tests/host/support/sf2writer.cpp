//
// sf2writer.cpp
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

// Writes a tiny SoundFont 2 file for the host tests

#include "sf2writer.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace
{
	using TBytes = std::vector<uint8_t>;

	void Put16(TBytes& Out, uint16_t n) { Out.push_back(n & 0xFF); Out.push_back(n >> 8); }
	void Put32(TBytes& Out, uint32_t n) { Put16(Out, n & 0xFFFF); Put16(Out, n >> 16); }
	void PutName(TBytes& Out, const char* pName)
	{
		char Name[20] = {};
		strncpy(Name, pName, sizeof(Name) - 1);
		Out.insert(Out.end(), Name, Name + sizeof(Name));
	}
	void PutFourCC(TBytes& Out, const char* p) { Out.insert(Out.end(), p, p + 4); }

	TBytes Chunk(const char* pId, const TBytes& Data)
	{
		TBytes Out;
		PutFourCC(Out, pId);
		Put32(Out, Data.size());
		Out.insert(Out.end(), Data.begin(), Data.end());
		if (Data.size() & 1)
			Out.push_back(0);
		return Out;
	}

	TBytes List(const char* pType, const std::vector<TBytes>& Chunks)
	{
		TBytes Data;
		PutFourCC(Data, pType);
		for (const auto& C : Chunks)
			Data.insert(Data.end(), C.begin(), C.end());
		return Chunk("LIST", Data);
	}
}

namespace pim32
{
	bool WriteTestSoundFont(const char* pPath, const char* pName, unsigned nHarmonics)
	{
		// 10 cycles of 100 samples at 44 kHz: an exactly looping 440 Hz tone (A4, key 69)
		constexpr uint32_t nSampleRate = 44000;
		constexpr uint32_t nPeriod = 100;
		constexpr uint32_t nFrames = 10 * nPeriod;

		TBytes Samples;
		for (uint32_t i = 0; i < nFrames; ++i)
		{
			double nValue = 0.0;
			for (unsigned h = 1; h <= nHarmonics; ++h)
				nValue += sin(2.0 * M_PI * h * i / nPeriod) / h;
			Put16(Samples, static_cast<uint16_t>(static_cast<int16_t>(nValue / 1.7 * 30000.0)));
		}
		// SoundFont 2 wants at least 46 zero samples after each sample
		Samples.insert(Samples.end(), 46 * 2, 0);

		TBytes ifil, isng, inam;
		Put16(ifil, 2);
		Put16(ifil, 1);
		const char Engine[] = "EMU8000";
		isng.assign(Engine, Engine + sizeof(Engine));
		// FluidSynth rejects odd-sized INFO sub-chunks: pad the name with NULs
		inam.assign(pName, pName + strlen(pName) + 1);
		if (inam.size() & 1)
			inam.push_back(0);

		TBytes phdr, pbag, pmod, pgen, inst, ibag, imod, igen, shdr;

		// Presets: "Test Tone" (bank 0, program 0), "Test Drums" (bank 128, program 0,
		// so channel 10 has a kit) and the terminal record; both use instrument 0
		PutName(phdr, "Test Tone"); Put16(phdr, 0); Put16(phdr, 0); Put16(phdr, 0); Put32(phdr, 0); Put32(phdr, 0); Put32(phdr, 0);
		PutName(phdr, "Test Drums"); Put16(phdr, 0); Put16(phdr, 128); Put16(phdr, 1); Put32(phdr, 0); Put32(phdr, 0); Put32(phdr, 0);
		PutName(phdr, "EOP"); Put16(phdr, 0); Put16(phdr, 0); Put16(phdr, 2); Put32(phdr, 0); Put32(phdr, 0); Put32(phdr, 0);
		Put16(pbag, 0); Put16(pbag, 0);
		Put16(pbag, 1); Put16(pbag, 0);
		Put16(pbag, 2); Put16(pbag, 0);
		pmod.assign(10, 0);
		Put16(pgen, 41); Put16(pgen, 0);	// instrument 0
		Put16(pgen, 41); Put16(pgen, 0);	// instrument 0
		Put16(pgen, 0); Put16(pgen, 0);

		// Instrument: one zone, looped sample 0
		PutName(inst, "Test Tone"); Put16(inst, 0);
		PutName(inst, "EOI"); Put16(inst, 1);
		Put16(ibag, 0); Put16(ibag, 0);
		Put16(ibag, 2); Put16(ibag, 0);
		imod.assign(10, 0);
		Put16(igen, 54); Put16(igen, 1);	// sampleModes: loop
		Put16(igen, 53); Put16(igen, 0);	// sampleID 0
		Put16(igen, 0); Put16(igen, 0);

		PutName(shdr, "Tone");
		Put32(shdr, 0); Put32(shdr, nFrames); Put32(shdr, 0); Put32(shdr, nFrames);
		Put32(shdr, nSampleRate); shdr.push_back(69); shdr.push_back(0); Put16(shdr, 0); Put16(shdr, 1);
		PutName(shdr, "EOS");
		shdr.insert(shdr.end(), 46 - 20, 0);

		TBytes Body;
		PutFourCC(Body, "sfbk");
		for (const auto& L : {
			List("INFO", {Chunk("ifil", ifil), Chunk("isng", isng), Chunk("INAM", inam)}),
			List("sdta", {Chunk("smpl", Samples)}),
			List("pdta", {Chunk("phdr", phdr), Chunk("pbag", pbag), Chunk("pmod", pmod), Chunk("pgen", pgen),
				      Chunk("inst", inst), Chunk("ibag", ibag), Chunk("imod", imod), Chunk("igen", igen), Chunk("shdr", shdr)})})
			Body.insert(Body.end(), L.begin(), L.end());
		const TBytes File = Chunk("RIFF", Body);

		FILE* pFile = fopen(pPath, "wb");
		if (!pFile)
			return false;
		const bool bOK = fwrite(File.data(), 1, File.size(), pFile) == File.size();
		return fclose(pFile) == 0 && bOK;
	}
}
