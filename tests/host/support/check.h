//
// check.h
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

// Minimal test framework for the host harness

#ifndef _pim32_check_h
#define _pim32_check_h

#include <cstdio>

namespace pim32
{
	using TTestFunction = void (*)();

	struct TRegistrar
	{
		TRegistrar(const char* pName, TTestFunction pFunction);
	};

	void ReportFailure(const char* pFile, int nLine, const char* pExpression);
	unsigned FailureCount();

	// Exit code 77: CTest reports the test as skipped
	[[noreturn]] void Skip(const char* pReason);
}

#define TEST(NAME)                                              \
	static void NAME();                                         \
	static ::pim32::TRegistrar NAME##_registrar(#NAME, NAME);   \
	static void NAME()

#define CHECK(EXPR)                                                     \
	do                                                                  \
	{                                                                   \
		if (!(EXPR))                                                    \
			::pim32::ReportFailure(__FILE__, __LINE__, #EXPR);          \
	} while (0)

#define CHECK_EQ(A, B)                                                                       \
	do                                                                                       \
	{                                                                                        \
		const auto _a = (A);                                                                 \
		const auto _b = (B);                                                                 \
		if (!(_a == _b))                                                                     \
		{                                                                                    \
			fprintf(stderr, "  %s = %lld, %s = %lld\n", #A, (long long)_a, #B, (long long)_b); \
			::pim32::ReportFailure(__FILE__, __LINE__, #A " == " #B);                        \
		}                                                                                    \
	} while (0)

#endif
