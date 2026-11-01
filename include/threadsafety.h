//
// threadsafety.h
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

#ifndef _threadsafety_h
#define _threadsafety_h

#include <circle/spinlock.h>

// Clang thread-safety analysis annotations
// (https://clang.llvm.org/docs/ThreadSafetyAnalysis.html). CI checks them with
// clang -Wthread-safety over the host harness build (tests/host). They expand
// to nothing under GCC, so the kernels are unchanged.
#if defined(__clang__)
#define THREAD_ANNOTATION(x) __attribute__((x))
#else
#define THREAD_ANNOTATION(x)
#endif

#define CAPABILITY(NAME)          THREAD_ANNOTATION(capability(NAME))
#define GUARDED_BY(LOCK)          THREAD_ANNOTATION(guarded_by(LOCK))
#define PT_GUARDED_BY(LOCK)       THREAD_ANNOTATION(pt_guarded_by(LOCK))
#define REQUIRES(...)             THREAD_ANNOTATION(requires_capability(__VA_ARGS__))
#define EXCLUDES(...)             THREAD_ANNOTATION(locks_excluded(__VA_ARGS__))
#define ACQUIRE(...)              THREAD_ANNOTATION(acquire_capability(__VA_ARGS__))
#define RELEASE(...)              THREAD_ANNOTATION(release_capability(__VA_ARGS__))

// Turns the analysis off for one function. Every use names the reason: a
// known race from the punchlist's concurrency audit (remove it with the fix),
// or code that runs before other cores can see the object.
#define NO_THREAD_SAFETY_ANALYSIS THREAD_ANNOTATION(no_thread_safety_analysis)

// Circle's CSpinLock with the annotations the analysis needs. Only Clang (the
// analysis) sees the wrapper; GCC builds use CSpinLock itself, so the kernels'
// code doesn't change at all
#if defined(__clang__)
class CAPABILITY("spinlock") CAnnotatedSpinLock : public CSpinLock
{
public:
	explicit CAnnotatedSpinLock(unsigned nTargetLevel = IRQ_LEVEL)
		: CSpinLock(nTargetLevel)
	{
	}

	void Acquire() ACQUIRE() { CSpinLock::Acquire(); }
	void Release() RELEASE() { CSpinLock::Release(); }
};
#else
using CAnnotatedSpinLock = CSpinLock;
#endif

#endif
