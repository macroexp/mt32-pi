//
// machineinfo.h
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

// Fake Circle: machine info; the harness poses as a Pi Zero 2 W (the pi3-64 kernel)
#ifndef _circle_machineinfo_h
#define _circle_machineinfo_h

enum TMachineModel
{
	MachineModelZero,
	MachineModelZeroW,
	MachineModelZero2W,
	MachineModel3B,
	MachineModel3APlus,
	MachineModel3BPlus,
	MachineModel4B,
	MachineModelUnknown
};

class CMachineInfo
{
public:
	static CMachineInfo* Get()
	{
		static CMachineInfo Instance;
		return &Instance;
	}

	TMachineModel GetMachineModel() const { return MachineModelZero2W; }
};

#endif
