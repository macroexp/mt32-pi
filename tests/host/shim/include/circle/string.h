//
// string.h
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

// Fake Circle: CString (host test harness)
//
// Like Circle's, it owns one heap buffer and never points into itself:
// Utility::Swap() swaps CStrings bytewise, which std::string's small-string
// buffer would not survive.
#ifndef _circle_string_h
#define _circle_string_h

#include <circle/sysconfig.h>
#include <circle/types.h>
#include <cstdarg>

class CString
{
public:
	CString();
	CString(const char* pString);
	CString(const CString& String);
	~CString();

	CString& operator=(const CString& String);
	CString& operator=(const char* pString);

	operator const char*() const { return m_pBuffer; }

	size_t GetLength() const;
	void Append(const char* pString);
	void Format(const char* pFormat, ...) __attribute__((format(printf, 2, 3)));
	void FormatV(const char* pFormat, va_list Args);

private:
	void Set(const char* pString);

	char* m_pBuffer;
};

#endif
