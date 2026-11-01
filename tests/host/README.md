# Host test harness

Builds pim-32's own sources natively on Linux and runs them under
ThreadSanitizer (TSan) or AddressSanitizer + UndefinedBehaviorSanitizer
(ASan/UBSan). Nothing here is part of the kernel build.

- **pim-32 sources, unchanged:** every file in `src/` except `kernel.cpp`,
  `main.cpp`, `net/*` and `pisound.cpp` is compiled as is, including
  `mt32pi.cpp`, so the tests run the real main loop, UI task and audio task.
- **Fake Circle (`shim/`):** only what those sources include. `CSpinLock` is a
  `std::mutex` (TSan understands it), `CLogger` prints to stdout, `CTimer` uses
  `std::chrono`, FatFs maps `SD:`/`USB:` to folders on the host,
  `CMultiCoreSupport` starts cores 1–3 as threads, the sound device accepts
  every frame at once (so the audio task renders continuously), and the serial
  port is fed by the test. Like Circle with `NO_BUSY_WAIT`, `f_read()` yields
  to the scheduler; the fake scheduler counts yields made while holding a
  `CSpinLock` (on the Pi, another core-0 task can then spin forever on it).
  The network, USB and Pisound are absent; `shim/src/stubs.cpp` only provides
  link symbols for the classes `mt32pi.cpp` names.
- **FluidSynth and Munt:** built from the submodules with the Makefile's
  options (FluidSynth `osal=embedded`, threads, network and drivers off, and
  `patches/fluidsynth-2.6.1-circle.patch` applied to a copy in the build
  folder), so FluidSynth's own mutexes are the same no-ops as on the Pi. They
  are built with the same sanitizer, so TSan sees races inside them. The
  optimisation level is `-O1 -g` rather than the kernel's `-Ofast`.
- **SoundFonts:** each test writes two tiny SoundFonts (a few KB, a looped tone
  at bank 0 and bank 128) with `support/sf2writer.cpp`.
- **MT-32 ROMs** are never in the repository. Tests that need them read the
  folder named by `PIM32_MT32_ROMS` and skip themselves without it, as in CI.

## Running

Needs CMake ≥ 3.20, Ninja (optional) and GCC with its sanitizer runtimes (the
Ubuntu `gcc` package has them; Ubuntu's `clang` package lacks the TSan
runtime). From the repository root, after `make submodules`:

```sh
cmake -S tests/host -B build-host-tsan -G Ninja -DPIM32_SANITIZE=thread
cmake --build build-host-tsan
ctest --test-dir build-host-tsan --output-on-failure

cmake -S tests/host -B build-host-asan -G Ninja "-DPIM32_SANITIZE=address;undefined"
cmake --build build-host-asan
ctest --test-dir build-host-asan --output-on-failure
```

With MT-32 ROMs (the tests labelled `roms`):

```sh
PIM32_MT32_ROMS=/path/to/roms ctest --test-dir build-host-tsan -L roms --output-on-failure
```

Add `-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache`
to cache rebuilds. Run one test directly to see its full output, for example
`build-host-tsan/test_sf_switch_race`; `PIM32_LOG_LEVEL=3` shows pim-32's
notices (default: warnings and errors). Thread-leak reports are expected from
the whole-system tests, which exit without stopping the cores; `ctest` turns
them off (`TSAN_OPTIONS=report_thread_leaks=0`).

On some kernels the sanitizers fail at start-up with "unexpected memory
mapping"; `sudo sysctl -w vm.mmap_rnd_bits=28` fixes it (CI does this).

If `patches/fluidsynth-2.6.1-circle.patch` changes, the build folder re-copies
and re-patches FluidSynth on the next build.

## Thread-safety analysis

`include/threadsafety.h` has Clang's capability annotations (`GUARDED_BY`,
`REQUIRES`, `EXCLUDES`, ...) and `CAnnotatedSpinLock`, which the synth and
ring-buffer locks use. Under GCC the macros are empty and `CAnnotatedSpinLock`
is `CSpinLock` itself, so the kernels don't change. With Clang, the
`thread_safety` target compiles pim-32's sources with `-Wthread-safety` as
errors (CI job `thread-safety`; about 10 s, it needs only the dependencies'
generated headers):

```sh
cmake -S tests/host -B build-host-ts -G Ninja -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ -DPIM32_SANITIZE= -DPIM32_THREAD_SAFETY=ON
cmake --build build-host-ts --target thread_safety
```

Known unlocked accesses from the concurrency audit are exempt with
`NO_THREAD_SAFETY_ANALYSIS` and a comment naming the audit item; a fix PR
removes its exemption, so the analysis then guards the fixed code.

## Expected outcomes

Each test is registered in `CMakeLists.txt` with `pim32_test()` and one of:

- `pass` (default): must pass in every build.
- `race`: a known race from the punchlist's concurrency audit. In the TSan
  build, TSan must report a race whose stacks match the given pattern; in the
  other builds the test must pass. Whether TSan catches a race depends on how
  the threads interleave, so a clean run is repeated, up to 5 runs in all. If
  none of them reports the race, the test fails and asks you to flip it to
  `pass`.
- `bug`: a known bug from the audit's "Other bugs" list. The test checks the
  correct behaviour, so it must fail until the bug is fixed; if it passes, it
  asks you to flip it to `pass`.

A fix PR for a race or bug flips its test's expectation in the same PR, so the
test fails before the fix and passes after it.

| Test | Checks | Expect |
| --- | --- | --- |
| `test_midiparser` | running status, SysEx split across buffers, real-time bytes inside messages, illegal status bytes, SysEx overflow | pass |
| `test_midiparser_tunerequest` | a channel message after a Tune Request (`F6`) | bug O2 |
| `test_ringbuffer` | full/empty, wrap-around, producer and consumer threads | pass |
| `test_zoneallocator` | alloc/free/merge, `Realloc()` in-place growth with 0/16/32 bytes left over (PR #17), small shrinks, random operations against a model | pass |
| `test_config` | the shipped `mt32-pi.cfg`, every option type, bad values, missing file | pass |
| `test_config_ip_defaults` | built-in static IP defaults | bug O5 |
| `test_midimonitor_note127` | MIDI note 127 in the level meters | bug O1 |
| `test_ui_format` | a `%` in a system message (SoundFont name) | bug O3 |
| `test_sf_switch_race` | SoundFont switches and MIDI while another thread renders (PR #25) | race C2 on main (`PIM32_EXPECT_SFLOAD_RACE`) |
| `test_sf_midi_while_rendering` | MIDI, SysEx, volume, all-sound-off and `IsActive()` while rendering | pass |
| `test_system_soundfont` | all of `CMT32Pi`: serial MIDI in, main loop, audio task, audio out | pass |
| `test_system_lcd_race` | all of `CMT32Pi` with an SSD1306: UI task vs MIDI and SysEx display messages | race C6/C7 |
| `test_mt32_midi_while_rendering` | MT-32 MIDI and SysEx while rendering (needs ROMs) | pass |
| `test_mt32_isactive_race` | `CMT32Synth::IsActive()` while rendering (needs ROMs) | race C4 |
| `test_mt32_lcd_romswitch_race` | MT-32 LCD update during a ROM-set switch (needs ROMs) | race C3 |
| `test_switch_synth_race` | `SwitchSynth()` while the audio task renders (needs ROMs) | race C5 (or C4 first) |

`test_sf_switch_race` also prints how often a SoundFont load yielded to the
scheduler while holding a `CSpinLock` (audit C1): 0 on main, more than 0 with
PR #25 as first written.
