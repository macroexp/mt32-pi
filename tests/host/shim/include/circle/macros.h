//
// macros.h
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

// Fake Circle: macros (host test harness)
#ifndef _circle_macros_h
#define _circle_macros_h

#define PACKED __attribute__((packed))
#define ALIGN(n) __attribute__((aligned(n)))
#define NORETURN __attribute__((noreturn))
#define MAYBE_UNUSED __attribute__((unused))
#define likely(exp) __builtin_expect(!!(exp), 1)
#define unlikely(exp) __builtin_expect(!!(exp), 0)

#endif
