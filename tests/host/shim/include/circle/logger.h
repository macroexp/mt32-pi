//
// logger.h
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

// Fake Circle: CLogger writes to stdout (host test harness)
#ifndef _circle_logger_h
#define _circle_logger_h

#include <circle/sysconfig.h>
#include <circle/types.h>
#include <cstdarg>

enum TLogSeverity
{
	LogPanic,
	LogError,
	LogWarning,
	LogNotice,
	LogDebug
};

#define LOGGER_BUFSIZE 0x4000

class CLogger
{
public:
	using TPanicHandler = void (*)();

	static CLogger* Get();

	void Write(const char* pSource, TLogSeverity Severity, const char* pMessage, ...);
	void WriteV(const char* pSource, TLogSeverity Severity, const char* pMessage, va_list Args);
	int Read(void* pBuffer, unsigned nCount, bool bClear = true);
	void RegisterPanicHandler(TPanicHandler pHandler) { m_pPanicHandler = pHandler; }

	// Test hooks: lowest severity printed (default LogWarning, or PIM32_LOG_LEVEL), and counters
	void SetLevel(TLogSeverity Level) { m_Level = Level; }
	unsigned GetCount(TLogSeverity Severity) const;

private:
	CLogger();
	TLogSeverity m_Level;
	TPanicHandler m_pPanicHandler = nullptr;
};

#define LOGMODULE(name) static const char From[] = name
#define LOGPANIC(...) CLogger::Get()->Write(From, LogPanic, __VA_ARGS__)
#define LOGERR(...) CLogger::Get()->Write(From, LogError, __VA_ARGS__)
#define LOGWARN(...) CLogger::Get()->Write(From, LogWarning, __VA_ARGS__)
#define LOGNOTE(...) CLogger::Get()->Write(From, LogNotice, __VA_ARGS__)
#define LOGDBG(...) CLogger::Get()->Write(From, LogDebug, __VA_ARGS__)

#endif
