#!/usr/bin/env bash
#
# Offline A/B test for Munt (libmt32emu) upgrades.
#
# Builds Munt's mt32emu-smf2wav renderer twice from the same renderer source,
# once against the OLD library and once against the NEW one, renders every
# MIDI file with both, and compares the results sample by sample. Because the
# renderer is identical, any difference comes from the library alone.
#
# Requires: git, cmake, a C++ compiler, pkg-config, glib 2 development files
# (Debian/Ubuntu: sudo apt-get install build-essential cmake git pkg-config libglib2.0-dev),
# python3, and your own MT-32 / CM-32L ROM files.
#
# Usage:
#   run.sh --roms DIR [--machine ID] [--midi DIR] [--old REF] [--new REF] [--work DIR]
#
#   --roms DIR     Directory containing MT-32 and/or CM-32L ROM files (required)
#   --machine ID   mt32emu machine ID, e.g. mt32, cm32l, mt32_1_07 (default: mt32).
#                  Old-gen units (mt32_1_04 ... mt32_1_07, mt32_bluer) show the
#                  partial allocator changes; run once with each ROM set you use.
#   --midi DIR     Directory of .mid files to render (default: generated test set)
#   --old REF      Munt git ref for the old library (default: libmt32emu_2_7_0)
#   --new REF      Munt git ref for the new library (default: libmt32emu_2_8_3)
#   --work DIR     Working directory (default: ./munt-ab-work)
#
# Render settings mirror mt32-pi: COARSE analog filter, 48 kHz output, GOOD
# resampler quality, other options at library defaults.

set -euo pipefail

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROMS=""
MACHINE="mt32"
MIDI=""
OLD_REF="libmt32emu_2_7_0"
NEW_REF="libmt32emu_2_8_3"
WORK="./munt-ab-work"
MUNT_URL="https://github.com/munt/munt.git"

usage() {
	sed -n '/^# Usage:/,/^# Render settings/p' "$0" | sed 's/^# \{0,1\}//' | sed '$d' >&2
	exit 2
}

while [ $# -gt 0 ]; do
	case "$1" in
	--roms) ROMS="$2"; shift 2 ;;
	--machine) MACHINE="$2"; shift 2 ;;
	--midi) MIDI="$2"; shift 2 ;;
	--old) OLD_REF="$2"; shift 2 ;;
	--new) NEW_REF="$2"; shift 2 ;;
	--work) WORK="$2"; shift 2 ;;
	-h | --help) usage ;;
	*) echo "Unknown option: $1" >&2; usage ;;
	esac
done

[ -n "$ROMS" ] || { echo "--roms is required" >&2; usage; }
[ -d "$ROMS" ] || { echo "ROM directory not found: $ROMS" >&2; exit 2; }

mkdir -p "$WORK"
WORK=$(cd "$WORK" && pwd)
ROMS=$(cd "$ROMS" && pwd)

# 1. Munt source
REPO="$WORK/munt-repo"
if [ ! -d "$REPO/.git" ]; then
	echo "== Cloning Munt"
	git clone --quiet --filter=blob:none "$MUNT_URL" "$REPO"
fi
git -C "$REPO" fetch --quiet --tags origin

# 2. Build the renderer twice: OLD renderer + OLD library, OLD renderer + NEW library
build_renderer() {
	local name="$1" lib_ref="$2"
	local src="$WORK/src-$name" build="$WORK/build-$name"
	if [ -x "$build/mt32emu_smf2wav/mt32emu-smf2wav" ] && [ -f "$build/.ref" ] &&
		[ "$(cat "$build/.ref")" = "$OLD_REF:$lib_ref" ]; then
		return
	fi
	echo "== Building renderer ($name: library $lib_ref)"
	rm -rf "$src" "$build"
	mkdir -p "$src"
	git -C "$REPO" archive "$OLD_REF" | tar -x -C "$src"
	rm -rf "$src/mt32emu"
	git -C "$REPO" archive "$lib_ref" mt32emu | tar -x -C "$src"
	cmake -S "$src" -B "$build" -DCMAKE_BUILD_TYPE=Release \
		-Dmunt_WITH_MT32EMU_QT=OFF -Dmunt_WITH_MT32EMU_SMF2WAV=ON \
		-DBUILD_TESTING=OFF >"$WORK/cmake-$name.log"
	cmake --build "$build" -j "$(nproc 2>/dev/null || echo 4)" >"$WORK/build-$name.log"
	echo "$OLD_REF:$lib_ref" >"$build/.ref"
}
build_renderer old "$OLD_REF"
build_renderer new "$NEW_REF"

# 3. MIDI files
if [ -z "$MIDI" ]; then
	MIDI="$WORK/midi"
	python3 "$HERE/make_test_midi.py" "$MIDI" >/dev/null
fi

# 4. Render
render() {
	local name="$1" out="$WORK/render-$MACHINE-$1"
	local tool="$WORK/build-$name/mt32emu_smf2wav/mt32emu-smf2wav"
	mkdir -p "$out"
	echo "== Rendering with $name library ($("$tool" --help 2>&1 | grep -o 'Library Version [0-9.]*'))"
	shopt -s nullglob
	local files=("$MIDI"/*.mid "$MIDI"/*.MID)
	[ ${#files[@]} -gt 0 ] || { echo "No .mid files in $MIDI" >&2; exit 2; }
	for midi in "${files[@]}"; do
		local base
		base=$(basename "${midi%.*}")
		"$tool" --quiet --force --rom-dir="$ROMS" --machine-id="$MACHINE" \
			--analog-output-mode=1 --sample-rate=48000 --src-quality=2 \
			--record-max-start-silence=-1 \
			--output="$out/$base.wav" "$midi"
	done
}
render old
render new

# 5. Compare
echo "== Comparing ($MACHINE)"
python3 "$HERE/compare_wavs.py" "$WORK/render-$MACHINE-old" "$WORK/render-$MACHINE-new"
