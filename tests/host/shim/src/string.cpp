//
// string.cpp
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

#include <circle/string.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

CString::CString()
	: m_pBuffer(nullptr)
{
	Set("");
}

CString::CString(const char* pString)
	: m_pBuffer(nullptr)
{
	Set(pString ? pString : "");
}

CString::CString(const CString& String)
	: m_pBuffer(nullptr)
{
	Set(String.m_pBuffer);
}

CString::~CString()
{
	free(m_pBuffer);
}

CString& CString::operator=(const CString& String)
{
	if (this != &String)
		Set(String.m_pBuffer);
	return *this;
}

CString& CString::operator=(const char* pString)
{
	Set(pString ? pString : "");
	return *this;
}

size_t CString::GetLength() const
{
	return strlen(m_pBuffer);
}

void CString::Append(const char* pString)
{
	const size_t nOld = strlen(m_pBuffer);
	const size_t nAdd = strlen(pString);
	char* pNew = static_cast<char*>(malloc(nOld + nAdd + 1));
	memcpy(pNew, m_pBuffer, nOld);
	memcpy(pNew + nOld, pString, nAdd + 1);
	free(m_pBuffer);
	m_pBuffer = pNew;
}

void CString::Format(const char* pFormat, ...)
{
	va_list Args;
	va_start(Args, pFormat);
	FormatV(pFormat, Args);
	va_end(Args);
}

void CString::FormatV(const char* pFormat, va_list Args)
{
	va_list Copy;
	va_copy(Copy, Args);
	const int nLength = vsnprintf(nullptr, 0, pFormat, Copy);
	va_end(Copy);

	char* pNew = static_cast<char*>(malloc(nLength + 1));
	vsnprintf(pNew, nLength + 1, pFormat, Args);
	free(m_pBuffer);
	m_pBuffer = pNew;
}

void CString::Set(const char* pString)
{
	char* pNew = strdup(pString);
	free(m_pBuffer);
	m_pBuffer = pNew;
}
