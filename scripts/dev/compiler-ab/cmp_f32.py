#!/usr/bin/env python3
"""Compare two raw stereo float32 48 kHz renders: overall and per-second."""

import sys

import numpy as np

SR = 48000


def db(x):
    return 20 * np.log10(max(x, 1e-12))


def main(a_path, b_path):
    a = np.fromfile(a_path, dtype="<f4").reshape(-1, 2)
    b = np.fromfile(b_path, dtype="<f4").reshape(-1, 2)
    n = min(len(a), len(b))
    if len(a) != len(b):
        print(f"length differs: {len(a)} vs {len(b)} frames")
    a, b = a[:n].astype(np.float64), b[:n].astype(np.float64)
    d = a - b
    exact = np.count_nonzero(d == 0) / d.size
    first = np.argmax(np.any(d != 0, axis=1)) if np.any(d) else None
    print(f"frames {n} ({n / SR:.1f} s), identical samples {exact * 100:.2f}%")
    if first is not None:
        print(f"first difference at {first / SR:.4f} s")
    peak, rms = db(np.abs(a).max()), db(np.sqrt((a**2).mean()))
    print(f"peak A {peak:.1f} dBFS, rms A {rms:.1f} dBFS")
    print(
        f"max |diff| {np.abs(d).max():.3e} ({db(np.abs(d).max()):.1f} dBFS), "
        f"rms diff {db(np.sqrt((d ** 2).mean())):.1f} dBFS"
    )
    worst = []
    for s in range(0, n // SR):
        seg_a = a[s * SR : (s + 1) * SR]
        seg_d = d[s * SR : (s + 1) * SR]
        ra = np.sqrt((seg_a**2).mean())
        rd = np.sqrt((seg_d**2).mean())
        worst.append((db(rd) - db(ra) if ra > 1e-9 else -999, s, db(ra), db(rd)))
    worst.sort(reverse=True)
    print("worst seconds (diff relative to signal):")
    for rel, s, ra, rd in worst[:5]:
        print(
            f"  {s:3d}s  signal {ra:6.1f} dBFS  diff {rd:6.1f} dBFS  rel {rel:6.1f} dB"
        )


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
