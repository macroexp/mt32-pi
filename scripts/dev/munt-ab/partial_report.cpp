/*
 * partial_report: play a MIDI file through libmt32emu (2.8 or later) and report
 * every Note On that the partial allocator ignores, and every playing note it
 * cuts short to make room, with the MIDI event that caused it.
 *
 * Usage: partial_report --rom-dir DIR --machine-id ID FILE.mid
 *   ID is a specific machine, e.g. mt32_1_07, mt32_2_04 or cm32l_1_02.
 *
 * Events go through libmt32emu's timestamped MIDI queue, as in mt32-pi and
 * mt32emu-smf2wav, so the library applies its default MIDI cable delay and
 * re-delivers notes that had to wait for a voice to be cut. Each report is
 * attributed to the latest Note On at or before the synth's position.
 *
 * Built by run.sh --report. Licensed GPLv3 like mt32-pi; uses libmt32emu
 * (LGPL) and libsmf (BSD) from Munt.
 */

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <dirent.h>

extern "C" {
#include "smf.h"
}

#define MT32EMU_API_TYPE 3
#include <mt32emu/mt32emu.h>

namespace {

const MT32Emu::Bit32u ADDR_PARTIAL_RESERVE = 0x40004; // System area 10 00 04, 9 bytes
const MT32Emu::Bit32u ADDR_MIDI_CHANNELS = 0x4000D;   // System area 10 00 0D, 9 bytes

struct Event {
	MT32Emu::Bit32u timestamp;
	double seconds;
	int channel, key, velocity;
};

struct Counts {
	unsigned ignored = 0, silenced = 0;
};

class Reporter : public MT32Emu::IReportHandlerV2 {
public:
	std::vector<Event> noteOns; // in timestamp order
	std::map<int, Counts> perChannel;
	unsigned printed = 0;
	MT32Emu::Service *service = nullptr;

	void printDebug(const char *, va_list) override {}
	void onErrorControlROM() override { fprintf(stderr, "Control ROM error\n"); }
	void onErrorPCMROM() override { fprintf(stderr, "PCM ROM error\n"); }
	void showLCDMessage(const char *) override {}
	void onMIDIMessagePlayed() override {}
	bool onMIDIQueueOverflow() override { fprintf(stderr, "MIDI queue overflow; event dropped\n"); return false; }
	void onMIDISystemRealtime(MT32Emu::Bit8u) override {}
	void onDeviceReset() override {}
	void onDeviceReconfig() override {}
	void onNewReverbMode(MT32Emu::Bit8u) override {}
	void onNewReverbTime(MT32Emu::Bit8u) override {}
	void onNewReverbLevel(MT32Emu::Bit8u) override {}
	void onPolyStateChanged(MT32Emu::Bit8u) override {}
	void onProgramChanged(MT32Emu::Bit8u, const char *, const char *) override {}
	void onLCDStateUpdated() override {}
	void onMidiMessageLEDStateUpdated(bool) override {}

	void onNoteOnIgnored(MT32Emu::Bit32u needed, MT32Emu::Bit32u free) override {
		const Event &e = currentNoteOn();
		perChannel[e.channel].ignored++;
		report(e, "IGNORED  ", needed, free);
	}

	void onPlayingPolySilenced(MT32Emu::Bit32u needed, MT32Emu::Bit32u free) override {
		const Event &e = currentNoteOn();
		perChannel[e.channel].silenced++;
		report(e, "CUT SHORT", needed, free);
	}

private:
	const Event &currentNoteOn() {
		static const Event none {0, 0, -1, 0, 0};
		const MT32Emu::Bit32u now = service->getInternalRenderedSampleCount();
		const Event *found = &none;
		for (const Event &e : noteOns) {
			if (MT32Emu::Bit32s(e.timestamp - now) > 0) break;
			found = &e;
		}
		return *found;
	}

	void report(const Event &current, const char *what, MT32Emu::Bit32u needed, MT32Emu::Bit32u free) {
		if (++printed > 200) {
			if (printed == 201) printf("  ... (further events counted in the summary only)\n");
			return;
		}
		printf("%9.3f s  %s ch %2d key %3d vel %3d  needed %u, free %u  | notes per part:",
			current.seconds, what, current.channel + 1, current.key, current.velocity, needed, free);
		for (MT32Emu::Bit8u part = 0; part < 9; part++) {
			MT32Emu::Bit8u keys[256], velocities[256];
			printf(" %u", service->getPlayingNotes(part, keys, velocities));
		}
		printf("\n");
	}
};

bool loadROMs(MT32Emu::Service &service, const char *romDir, const char *machineID) {
	DIR *dir = opendir(romDir);
	if (!dir) {
		fprintf(stderr, "Can't open ROM directory %s\n", romDir);
		return false;
	}
	while (dirent *entry = readdir(dir)) {
		if (entry->d_name[0] == '.') continue;
		std::string path = std::string(romDir) + "/" + entry->d_name;
		service.addMachineROMFile(machineID, path.c_str());
	}
	closedir(dir);
	mt32emu_rom_info info;
	service.getROMInfo(&info);
	if (!info.control_rom_id || !info.pcm_rom_id) {
		fprintf(stderr, "No complete ROM set for machine %s in %s\n", machineID, romDir);
		return false;
	}
	printf("Control ROM: %s (%s)\nPCM ROM:     %s\n", info.control_rom_id, info.control_rom_description, info.pcm_rom_id);
	return true;
}

void printSystemState(MT32Emu::Service &service, const char *label) {
	MT32Emu::Bit8u reserve[9], channels[9];
	service.readMemory(ADDR_PARTIAL_RESERVE, 9, reserve);
	service.readMemory(ADDR_MIDI_CHANNELS, 9, channels);
	printf("%s partial reserve (parts 1-8, rhythm):", label);
	for (int i = 0; i < 9; i++) printf(" %u", reserve[i]);
	printf("  | MIDI channels:");
	for (int i = 0; i < 9; i++) printf(" %u", channels[i] + 1);
	printf("\n");
}

} // namespace

int main(int argc, char **argv) {
	const char *romDir = nullptr, *machineID = nullptr, *midiFile = nullptr;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--rom-dir") && i + 1 < argc) romDir = argv[++i];
		else if (!strcmp(argv[i], "--machine-id") && i + 1 < argc) machineID = argv[++i];
		else midiFile = argv[i];
	}
	if (!romDir || !machineID || !midiFile) {
		fprintf(stderr, "Usage: %s --rom-dir DIR --machine-id ID FILE.mid\n", argv[0]);
		return 2;
	}

	Reporter reporter;
	MT32Emu::Service service;
	reporter.service = &service;
	service.createContext(reporter);
	if (!loadROMs(service, romDir, machineID)) return 1;
	service.setStereoOutputSampleRate(0); // native rate, no resampling needed for this
	if (service.openSynth() != MT32EMU_RC_OK) {
		fprintf(stderr, "Failed to open synth\n");
		return 1;
	}
	const double rate = service.getActualStereoOutputSamplerate();

	smf_t *smf = smf_load(midiFile);
	if (!smf) {
		fprintf(stderr, "Failed to load %s\n", midiFile);
		return 1;
	}
	printf("File: %s (%.1f s)\n\n", midiFile, smf_get_length_seconds(smf));

	// Queue every event with its timestamp; the synth applies MIDI cable delay
	// to short messages and replays notes that waited for a voice to be cut.
	bool reservePrinted = false;
	double lastQueuedSeconds = 0;
	std::vector<MT32Emu::Bit16s> buffer(2 * 4096);
	auto renderUntil = [&](MT32Emu::Bit32u target) {
		while (MT32Emu::Bit32s(target - service.getInternalRenderedSampleCount()) > 0) {
			MT32Emu::Bit32u frames = target - service.getInternalRenderedSampleCount();
			if (frames > 4096) frames = 4096;
			service.renderBit16s(buffer.data(), frames);
		}
	};

	while (smf_event_t *event = smf_get_next_event(smf)) {
		if (smf_event_is_metadata(event)) continue;
		const unsigned char *buf = event->midi_buffer;
		const int len = event->midi_buffer_length;
		if (len < 1) continue;
		const MT32Emu::Bit32u ts = MT32Emu::Bit32u(event->time_seconds * rate);
		lastQueuedSeconds = event->time_seconds;

		// Render up to this event first, so the queue only ever holds the current moment
		renderUntil(ts);

		if (buf[0] == 0xF0) {
			service.playSysexAt(buf, MT32Emu::Bit32u(len), ts);
			continue;
		}
		if (len > 3) continue;
		MT32Emu::Bit32u msg = 0;
		for (int i = 0; i < len; i++) msg |= MT32Emu::Bit32u(buf[i]) << (8 * i);
		if ((buf[0] & 0xF0) == 0x90 && len == 3 && buf[2] > 0) {
			if (!reservePrinted) {
				printSystemState(service, "At first note:");
				printf("\n");
				reservePrinted = true;
			}
			reporter.noteOns.push_back(Event {ts, event->time_seconds, buf[0] & 0x0F, buf[1], buf[2]});
		}
		service.playMsgAt(msg, ts);
	}
	renderUntil(MT32Emu::Bit32u((lastQueuedSeconds + 2.0) * rate));
	printSystemState(service, "At end:       ");

	printf("\nSummary (MIDI channel: notes ignored / playing notes cut short):\n");
	if (reporter.perChannel.empty()) printf("  none - every note found partials\n");
	for (auto &entry : reporter.perChannel) {
		printf("  ch %2d: %u ignored, %u cut short\n", entry.first + 1, entry.second.ignored, entry.second.silenced);
	}
	smf_delete(smf);
	service.closeSynth();
	service.freeContext();
	return 0;
}
