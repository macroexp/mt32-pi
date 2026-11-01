#
# run_test.cmake
#
# mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
# Copyright (C) 2026 pim-32 contributors
#
# This file is part of mt32-pi.
#
# mt32-pi is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# mt32-pi is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# mt32-pi. If not, see <http://www.gnu.org/licenses/>.
#

# Runs one test executable and checks its outcome against MODE:
#   pass  exit code 0
#   race  ThreadSanitizer reported a data race (exit code 66) matching MATCH
#   bug   a known bug: any failure (the test checks correct behaviour)
# Exit code 77 means the test skipped itself (e.g. no MT-32 ROMs); CTest sees
# "PIM32_TEST_SKIPPED" through the test's SKIP_REGULAR_EXPRESSION.

execute_process(
	COMMAND "${EXE}"
	RESULT_VARIABLE RESULT
	OUTPUT_VARIABLE OUTPUT
	ERROR_VARIABLE OUTPUT)

message("${OUTPUT}")

if(RESULT EQUAL 77)
	message("PIM32_TEST_SKIPPED")
	return()
endif()

if(MODE STREQUAL "pass")
	if(NOT RESULT EQUAL 0)
		message(FATAL_ERROR "FAILED (exit ${RESULT})")
	endif()
elseif(MODE STREQUAL "race")
	if(RESULT EQUAL 0)
		message(FATAL_ERROR "Expected ThreadSanitizer to report this known race, but the test passed. "
			"If the fix has landed, flip the expectation in tests/host/CMakeLists.txt.")
	endif()
	if(NOT RESULT EQUAL 66 OR NOT OUTPUT MATCHES "ThreadSanitizer: data race")
		message(FATAL_ERROR "Expected a ThreadSanitizer data race report, got exit ${RESULT}")
	endif()
	if(MATCH AND NOT OUTPUT MATCHES "${MATCH}")
		message(FATAL_ERROR "ThreadSanitizer reported a race, but not the expected one (${MATCH})")
	endif()
	message("Known race reported as expected")
elseif(MODE STREQUAL "bug")
	if(RESULT EQUAL 0)
		message(FATAL_ERROR "This known bug no longer reproduces. If the fix has landed, "
			"change the test's EXPECT to pass in tests/host/CMakeLists.txt.")
	endif()
	message("Known bug reproduced as expected (exit ${RESULT})")
else()
	message(FATAL_ERROR "Unknown MODE ${MODE}")
endif()
