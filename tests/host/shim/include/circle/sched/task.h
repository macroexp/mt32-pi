//
// task.h
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

// Fake Circle: tasks are never started in the harness (host test harness)
#ifndef _circle_sched_task_h
#define _circle_sched_task_h

#include <circle/sysconfig.h>
#include <circle/types.h>

#define TASK_STACK_SIZE 0x8000

class CTask
{
public:
	CTask(unsigned nStackSize = TASK_STACK_SIZE, boolean bCreateSuspended = FALSE) {}
	virtual ~CTask() = default;
	virtual void Run() {}
	void Start() {}
	void Suspend() {}
	void Resume() {}
	void SetName(const char* pName) {}
};

#endif
