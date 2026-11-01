//
// test_config.cpp
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

// CConfig: parsing the shipped mt32-pi.cfg and every option type

#include "check.h"
#include "fixture.h"

#include <circle/logger.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

static pim32::CFixture Fixture(0);

namespace
{
	void WriteFile(const std::string& Path, const char* pText)
	{
		FILE* pFile = fopen(Path.c_str(), "w");
		fputs(pText, pFile);
		fclose(pFile);
	}

	std::string Format(const CIPAddress& Address)
	{
		CString String;
		Address.Format(&String);
		return static_cast<const char*>(String);
	}
}

TEST(ShippedConfigParses)
{
	std::filesystem::copy_file(PIM32_SOURCE_ROOT "/sdcard/mt32-pi.cfg", Fixture.SDPath("mt32-pi.cfg"), std::filesystem::copy_options::overwrite_existing);

	const unsigned nWarnings = CLogger::Get()->GetCount(LogWarning);
	CConfig Config;
	CHECK(Config.Initialize("SD:mt32-pi.cfg"));
	CHECK_EQ(CLogger::Get()->GetCount(LogWarning), nWarnings);

	// A sample of the shipped values
	CHECK(Config.SystemDefaultSynth == CConfig::TSystemDefaultSynth::MT32);
	CHECK(Config.AudioOutputDevice == CConfig::TAudioOutputDevice::PWM);
	CHECK_EQ(Config.AudioSampleRate, 48000);
	CHECK_EQ(Config.AudioChunkSize, 256);
	CHECK(Config.MT32EmuROMSet == TMT32ROMSet::MT32Old);
	CHECK_EQ(Config.FluidSynthPolyphony, 200);
	CHECK(Config.LCDType == CConfig::TLCDType::None);
	CHECK_EQ(Config.LCDWidth, 128);
	CHECK_EQ(Config.LCDHeight, 32);
	CHECK(Config.NetworkMode == CConfig::TNetworkMode::Off);
	CHECK(Format(Config.NetworkIPAddress) == "192.168.1.100");
	CHECK(!Config.NetworkFTPServer);
	CHECK(!strcmp(Config.NetworkHostname, "mt32-pi"));
}

TEST(EveryOptionType)
{
	WriteFile(Fixture.SDPath("custom.cfg"),
		"; comment\n"
		"[system]\n"
		"verbose = ON\n"
		"default_synth = SoundFont\n"
		"usb = 0\n"
		"power_save_timeout = 0\n"
		"[audio]\n"
		"output_device = i2s\n"
		"chunk_size = 128\n"
		"[mt32emu]\n"
		"gain = 1.5\n"
		"resampler_quality = best\n"
		"rom_set = cm32l\n"
		"[lcd]\n"
		"type = ssd1306_i2c\n"
		"i2c_lcd_address = 3d\n"
		"rotation = inverted\n"
		"[network]\n"
		"mode = wifi\n"
		"ip_address = 10.0.0.42\n"
		"hostname = pim-32\n"
		"unknown_key = ignored\n"
		"[unknown_section]\n"
		"verbose = off\n");

	CConfig Config;
	CHECK(Config.Initialize("SD:custom.cfg"));
	CHECK(Config.SystemVerbose);
	CHECK(Config.SystemDefaultSynth == CConfig::TSystemDefaultSynth::SoundFont);
	CHECK(!Config.SystemUSB);
	CHECK_EQ(Config.SystemPowerSaveTimeout, 0);
	CHECK(Config.AudioOutputDevice == CConfig::TAudioOutputDevice::I2S);
	CHECK_EQ(Config.AudioChunkSize, 128);
	CHECK(Config.MT32EmuGain == 1.5f);
	CHECK(Config.MT32EmuResamplerQuality == CMT32Synth::TResamplerQuality::Best);
	CHECK(Config.MT32EmuROMSet == TMT32ROMSet::CM32L);
	CHECK(Config.LCDType == CConfig::TLCDType::SSD1306I2C);
	CHECK_EQ(Config.LCDI2CLCDAddress, 0x3D);
	CHECK(Config.LCDRotation == CSSD1306::TLCDRotation::Inverted);
	CHECK(Config.NetworkMode == CConfig::TNetworkMode::WiFi);
	CHECK(Format(Config.NetworkIPAddress) == "10.0.0.42");
	CHECK(!strcmp(Config.NetworkHostname, "pim-32"));
}

TEST(InvalidValuesKeepDefaults)
{
	WriteFile(Fixture.SDPath("bad.cfg"),
		"[system]\n"
		"verbose = maybe\n"
		"default_synth = organ\n"
		"[network]\n"
		"ip_address = 10.0\n");

	CConfig Defaults;
	CConfig Config;
	CHECK(Config.Initialize("SD:bad.cfg"));
	CHECK(Config.SystemVerbose == Defaults.SystemVerbose);
	CHECK(Config.SystemDefaultSynth == Defaults.SystemDefaultSynth);
	CHECK(Format(Config.NetworkIPAddress) == Format(Defaults.NetworkIPAddress));
}

TEST(MissingFile)
{
	CConfig Config;
	CHECK(!Config.Initialize("SD:does-not-exist.cfg"));
}
