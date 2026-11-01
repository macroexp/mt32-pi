//
// logger.cpp
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

#include <circle/logger.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

namespace
{
	std::mutex LogMutex;
	std::atomic<unsigned> Counts[LogDebug + 1];
	const char* const SeverityNames[] = {"PANIC", "ERROR", "WARN", "NOTE", "DEBUG"};
}

CLogger::CLogger()
	: m_Level(LogWarning)
{
	if (const char* pLevel = getenv("PIM32_LOG_LEVEL"))
		m_Level = static_cast<TLogSeverity>(atoi(pLevel));
}

CLogger* CLogger::Get()
{
	static CLogger Logger;
	return &Logger;
}

void CLogger::Write(const char* pSource, TLogSeverity Severity, const char* pMessage, ...)
{
	va_list Args;
	va_start(Args, pMessage);
	WriteV(pSource, Severity, pMessage, Args);
	va_end(Args);
}

void CLogger::WriteV(const char* pSource, TLogSeverity Severity, const char* pMessage, va_list Args)
{
	if (Severity > LogDebug)
		Severity = LogDebug;
	++Counts[Severity];

	if (Severity <= m_Level)
	{
		std::lock_guard<std::mutex> Lock(LogMutex);
		fprintf(stdout, "[%s] %s: ", SeverityNames[Severity], pSource);
		vfprintf(stdout, pMessage, Args);
		fputc('\n', stdout);
		fflush(stdout);
	}

	// Circle halts the system on a panic
	if (Severity == LogPanic)
	{
		fprintf(stderr, "Panic logged by %s; aborting\n", pSource);
		abort();
	}
}

int CLogger::Read(void* pBuffer, unsigned nCount, bool bClear)
{
	if (nCount)
		static_cast<char*>(pBuffer)[0] = '\0';
	return 0;
}

unsigned CLogger::GetCount(TLogSeverity Severity) const
{
	return Counts[Severity];
}
