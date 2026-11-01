//
// synchronize.h
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

// Fake Circle: barriers and events (host test harness)
#ifndef _circle_synchronize_h
#define _circle_synchronize_h

#include <atomic>
#include <thread>

#define DataSyncBarrier() std::atomic_thread_fence(std::memory_order_seq_cst)
#define DataMemBarrier() std::atomic_thread_fence(std::memory_order_seq_cst)
#define InstructionSyncBarrier() std::atomic_signal_fence(std::memory_order_seq_cst)
#define WaitForEvent() std::this_thread::yield()
#define SendEvent() ((void)0)
#define EnterCritical(...) ((void)0)
#define LeaveCritical() ((void)0)

#endif
