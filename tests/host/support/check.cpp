//
// check.cpp
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

#include "check.h"

#include <cstdlib>
#include <vector>

namespace
{
	struct TTest
	{
		const char* pName;
		pim32::TTestFunction pFunction;
	};

	std::vector<TTest>& Tests()
	{
		static std::vector<TTest> List;
		return List;
	}

	unsigned Failures = 0;
}

namespace pim32
{
	TRegistrar::TRegistrar(const char* pName, TTestFunction pFunction)
	{
		Tests().push_back({pName, pFunction});
	}

	void ReportFailure(const char* pFile, int nLine, const char* pExpression)
	{
		fprintf(stderr, "%s:%d: check failed: %s\n", pFile, nLine, pExpression);
		++Failures;
	}

	unsigned FailureCount()
	{
		return Failures;
	}

	void Skip(const char* pReason)
	{
		printf("SKIPPED: %s\n", pReason);
		fflush(stdout);
		exit(77);
	}
}

int main()
{
	for (const auto& Test : Tests())
	{
		const unsigned nBefore = Failures;
		printf("[ RUN  ] %s\n", Test.pName);
		fflush(stdout);
		Test.pFunction();
		printf("[ %s ] %s\n", Failures == nBefore ? " OK " : "FAIL", Test.pName);
		fflush(stdout);
	}

	printf("%zu tests, %u failed checks\n", Tests().size(), Failures);
	return Failures ? 1 : 0;
}
