#
# apply_patch.cmake
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

# Copies SOURCE to DEST and applies PATCH there, the way the Makefile's
# APPLY_PATCH does: skipped if the copy already has it (the submodule may be
# patched in place by a kernel build that hasn't run "make mrproper" yet).
# The submodule itself is never touched.
#
#   cmake -DSOURCE=... -DDEST=... -DPATCH=... -P apply_patch.cmake

file(REMOVE_RECURSE "${DEST}")
file(COPY "${SOURCE}/" DESTINATION "${DEST}" PATTERN ".git" EXCLUDE)

execute_process(
	COMMAND patch --dry-run --reverse --force --strip 1 --directory "${DEST}" --input "${PATCH}"
	RESULT_VARIABLE ALREADY_APPLIED
	OUTPUT_QUIET ERROR_QUIET)

if(ALREADY_APPLIED EQUAL 0)
	message(STATUS "${PATCH} already applied in the source tree")
else()
	execute_process(
		COMMAND patch --forward --strip 1 --no-backup-if-mismatch --directory "${DEST}" --input "${PATCH}"
		RESULT_VARIABLE PATCH_RESULT)
	if(NOT PATCH_RESULT EQUAL 0)
		message(FATAL_ERROR "Applying ${PATCH} failed")
	endif()
endif()
