#!/usr/bin/env python3
"""Compare two directories of WAV renders sample by sample.

For each file present in both directories, reports whether the audio is
bit-identical and, if not, where it first differs, the largest sample
difference and the level of the difference signal relative to full scale.

Supports 16-bit PCM WAV files (mt32emu-smf2wav's default output).

Usage: compare_wavs.py OLD_DIR NEW_DIR
Exit status: 0 if every pair is identical, 1 if any differ, 2 on error.
"""

import math
import sys
import wave
from array import array
from pathlib import Path


def read_wav(path):
    with wave.open(str(path), "rb") as wav:
        if wav.getsampwidth() != 2:
            raise ValueError(f"{path}: only 16-bit PCM is supported")
        params = (wav.getnchannels(), wav.getframerate())
        samples = array("h")
        samples.frombytes(wav.readframes(wav.getnframes()))
    if sys.byteorder == "big":
        samples.byteswap()
    return params, samples


def compare(old_path, new_path):
    (channels, rate), old = read_wav(old_path)
    new_params, new = read_wav(new_path)
    if new_params != (channels, rate):
        return f"format differs: {channels} ch/{rate} Hz vs {new_params}"

    length = min(len(old), len(new))
    first_diff = None
    max_diff = 0
    sum_sq = 0
    for i in range(length):
        diff = new[i] - old[i]
        if diff:
            if first_diff is None:
                first_diff = i
            sum_sq += diff * diff
            max_diff = max(max_diff, abs(diff))

    length_note = ""
    if len(old) != len(new):
        frames = (len(new) - len(old)) // channels
        length_note = f"; length differs by {frames:+d} frames ({frames / rate:+.3f} s)"

    if first_diff is None:
        return "identical" + length_note

    at = first_diff // channels / rate
    rms = math.sqrt(sum_sq / length) if length else 0
    level = 20 * math.log10(rms / 32768) if rms else float("-inf")
    return (
        f"DIFFERS from {at:.3f} s; max sample diff {max_diff}; "
        f"difference level {level:.1f} dBFS{length_note}"
    )


def main():
    if len(sys.argv) != 3:
        print("Usage: compare_wavs.py OLD_DIR NEW_DIR", file=sys.stderr)
        return 2
    old_dir, new_dir = (Path(arg) for arg in sys.argv[1:])
    names = sorted(p.name for p in old_dir.glob("*.wav"))
    if not names:
        print(f"No .wav files in {old_dir}", file=sys.stderr)
        return 2

    any_differs = False
    width = max(len(name) for name in names)
    for name in names:
        new_path = new_dir / name
        if not new_path.exists():
            result = "missing in new render"
            any_differs = True
        else:
            result = compare(old_dir / name, new_path)
            any_differs |= result != "identical"
        print(f"{name:<{width}}  {result}")
    return 1 if any_differs else 0


if __name__ == "__main__":
    sys.exit(main())
