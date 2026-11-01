#
# CMakeLists.txt
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

# Host test harness: builds pim-32's own sources natively on Linux against a
# small fake Circle (shim/), with FluidSynth and Munt built from the same
# submodules and options as the kernel. See README.md.

cmake_minimum_required(VERSION 3.20)
project(pim32_host_tests C CXX)

set(PIM32_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "pim-32 repository root")
get_filename_component(PIM32_ROOT "${PIM32_ROOT}" ABSOLUTE)

# thread           ThreadSanitizer
# address;undefined AddressSanitizer + UndefinedBehaviorSanitizer
# (empty)          no sanitizer
set(PIM32_SANITIZE "thread" CACHE STRING "Sanitizers: thread, address;undefined, or empty")

# Expected outcome of the SoundFont-load race test (concurrency audit C2): "race"
# while main still had it, "fixed" since PR #25 fixed it on main. The
# PR that fixes C2 changes this default. Other expectations are per test below.
set(PIM32_EXPECT_SFLOAD_RACE "fixed" CACHE STRING "C2: race or fixed")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O1 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O1 -g")

set(SANITIZE_FLAGS "")
if(PIM32_SANITIZE)
	list(JOIN PIM32_SANITIZE "," SANITIZE_LIST)
	set(SANITIZE_FLAGS "-fsanitize=${SANITIZE_LIST} -fno-omit-frame-pointer")
	if("undefined" IN_LIST PIM32_SANITIZE)
		string(APPEND SANITIZE_FLAGS " -fno-sanitize-recover=undefined")
	endif()
endif()

# Dependencies get the same sanitizer, so TSan sees both sides of a race inside
# FluidSynth or Munt
set(DEPS_C_FLAGS "-O1 -g ${SANITIZE_FLAGS}")
set(DEPS_CXX_FLAGS "-O1 -g ${SANITIZE_FLAGS}")

set(DEPS_COMMON_ARGS
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
	-DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
	-DCMAKE_BUILD_TYPE=None
	-DCMAKE_C_FLAGS=${DEPS_C_FLAGS}
	-DCMAKE_CXX_FLAGS=${DEPS_CXX_FLAGS}
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON
	-DBUILD_SHARED_LIBS=OFF)

include(ExternalProject)

#
# Munt: the Makefile's options (static, C++ interface only, no tests)
#
set(MUNT_BINARY_DIR "${CMAKE_BINARY_DIR}/munt")
ExternalProject_Add(munt_ext
	SOURCE_DIR "${PIM32_ROOT}/external/munt/mt32emu"
	BINARY_DIR "${MUNT_BINARY_DIR}"
	CMAKE_ARGS ${DEPS_COMMON_ARGS}
		-Dlibmt32emu_C_INTERFACE=FALSE
		-Dlibmt32emu_SHARED=FALSE
		-DBUILD_TESTING=OFF
		-Dlibmt32emu_BUILD_TESTING=OFF
	INSTALL_COMMAND ""
	BUILD_BYPRODUCTS "${MUNT_BINARY_DIR}/libmt32emu.a")

add_library(mt32emu STATIC IMPORTED)
add_dependencies(mt32emu munt_ext)
set_target_properties(mt32emu PROPERTIES IMPORTED_LOCATION "${MUNT_BINARY_DIR}/libmt32emu.a")
file(MAKE_DIRECTORY "${MUNT_BINARY_DIR}/include")
target_include_directories(mt32emu INTERFACE "${MUNT_BINARY_DIR}/include")

#
# FluidSynth: a patched copy of the submodule, configured like the Makefile
# (osal=embedded, threads/network/drivers off), so its mutexes are the same
# no-ops as on the Pi
#
set(FLUID_SOURCE_DIR "${CMAKE_BINARY_DIR}/fluidsynth-src")
set(FLUID_BINARY_DIR "${CMAKE_BINARY_DIR}/fluidsynth")
set(FLUID_PATCH "${PIM32_ROOT}/patches/fluidsynth-2.6.1-circle.patch")
ExternalProject_Add(fluidsynth_ext
	DOWNLOAD_COMMAND ${CMAKE_COMMAND}
		-DSOURCE=${PIM32_ROOT}/external/fluidsynth
		-DDEST=${FLUID_SOURCE_DIR}
		-DPATCH=${FLUID_PATCH}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/apply_patch.cmake
	SOURCE_DIR "${FLUID_SOURCE_DIR}"
	BINARY_DIR "${FLUID_BINARY_DIR}"
	CMAKE_ARGS ${DEPS_COMMON_ARGS}
		-Dosal=embedded
		-Denable-aufile=OFF
		-Denable-dbus=OFF
		-Denable-dsound=OFF
		-Denable-floats=ON
		-Denable-ipv6=OFF
		-Denable-jack=OFF
		-Denable-ladspa=OFF
		-Denable-libsndfile=OFF
		-Denable-midishare=OFF
		-Denable-native-dls=OFF
		-Denable-network=OFF
		-Denable-oboe=OFF
		-Denable-openmp=OFF
		-Denable-opensles=OFF
		-Denable-oss=OFF
		-Denable-pipewire=OFF
		-Denable-pulseaudio=OFF
		-Denable-readline=OFF
		-Denable-sdl3=OFF
		-Denable-signalsmith=OFF
		-Denable-threads=OFF
		-Denable-wasapi=OFF
		-Denable-waveout=OFF
		-Denable-winmidi=OFF
		-Denable-alsa=OFF
		-Denable-portaudio=OFF
		-Denable-systemd=OFF
		-Denable-coreaudio=OFF
		-Denable-coremidi=OFF
	BUILD_COMMAND ${CMAKE_COMMAND} --build . --target libfluidsynth
	INSTALL_COMMAND ""
	BUILD_BYPRODUCTS "${FLUID_BINARY_DIR}/src/libfluidsynth.a")
# Re-copy and re-patch when the patch changes
ExternalProject_Add_StepDependencies(fluidsynth_ext download "${FLUID_PATCH}")

add_library(fluidsynth STATIC IMPORTED)
add_dependencies(fluidsynth fluidsynth_ext)
set_target_properties(fluidsynth PROPERTIES IMPORTED_LOCATION "${FLUID_BINARY_DIR}/src/libfluidsynth.a")
file(MAKE_DIRECTORY "${FLUID_BINARY_DIR}/include" "${FLUID_SOURCE_DIR}/include")
target_include_directories(fluidsynth INTERFACE "${FLUID_BINARY_DIR}/include" "${FLUID_SOURCE_DIR}/include")
target_link_libraries(fluidsynth INTERFACE m)

#
# pim-32 sources, unchanged, against the fake Circle
#
set(PIM32_SOURCES
	src/config.cpp
	src/control/control.cpp
	src/control/mister.cpp
	src/control/rotaryencoder.cpp
	src/control/simplebuttons.cpp
	src/control/simpleencoder.cpp
	src/lcd/drivers/hd44780.cpp
	src/lcd/drivers/hd44780fourbit.cpp
	src/lcd/drivers/hd44780i2c.cpp
	src/lcd/drivers/sh1106.cpp
	src/lcd/drivers/ssd1306.cpp
	src/lcd/ui.cpp
	src/midimonitor.cpp
	src/midiparser.cpp
	src/mt32pi.cpp
	src/power.cpp
	src/rommanager.cpp
	src/soundfontmanager.cpp
	src/synth/mt32synth.cpp
	src/synth/soundfontsynth.cpp
	src/zoneallocator.cpp
	external/inih/ini.c)
list(TRANSFORM PIM32_SOURCES PREPEND "${PIM32_ROOT}/")

set(SHIM_SOURCES
	shim/src/fatfs.cpp
	shim/src/logger.cpp
	shim/src/memory.cpp
	shim/src/multicore.cpp
	shim/src/peripherals.cpp
	shim/src/scheduler.cpp
	shim/src/sound.cpp
	shim/src/spinlock.cpp
	shim/src/string.cpp
	shim/src/timer.cpp
	shim/src/stubs.cpp)

# Definitions and include paths for compiling pim-32 sources against the shim
add_library(pim32_headers INTERFACE)
# RASPPI/AARCH as for pi3-64 (the Zero 2 W kernel)
target_compile_definitions(pim32_headers INTERFACE RASPPI=3 AARCH=64 PIM32_HOST_TEST=1 MT32_PI_VERSION="host-test")
target_include_directories(pim32_headers INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/shim/include"
	"${PIM32_ROOT}/include"
	"${PIM32_ROOT}"
	"${PIM32_ROOT}/external/inih"
	"${MUNT_BINARY_DIR}/include"
	"${FLUID_BINARY_DIR}/include"
	"${FLUID_SOURCE_DIR}/include")

add_library(pim32 STATIC ${PIM32_SOURCES} ${SHIM_SOURCES})
target_link_libraries(pim32 PUBLIC pim32_headers)
separate_arguments(SANITIZE_FLAGS_LIST UNIX_COMMAND "${SANITIZE_FLAGS}")
# NDEBUG must stay undefined whatever the build type: pim-32 calls the synths'
# render functions inside assert(), and the kernel keeps asserts on
target_compile_options(pim32 PUBLIC -UNDEBUG ${SANITIZE_FLAGS_LIST})
target_link_options(pim32 PUBLIC ${SANITIZE_FLAGS_LIST})
find_package(Threads REQUIRED)
target_link_libraries(pim32 PUBLIC fluidsynth mt32emu Threads::Threads)

#
# Clang thread-safety analysis (CI job thread-safety): compiles pim-32's
# sources with -Wthread-safety as errors, so an access to lock-guarded state
# without the lock fails the build. Needs only the dependencies' generated
# headers, not their libraries: cmake --build <dir> --target thread_safety
#
option(PIM32_THREAD_SAFETY "Add the thread_safety target (Clang only)" OFF)
if(PIM32_THREAD_SAFETY)
	if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		message(FATAL_ERROR "PIM32_THREAD_SAFETY needs Clang (-DCMAKE_CXX_COMPILER=clang++)")
	endif()
	ExternalProject_Add_StepTargets(munt_ext configure)
	ExternalProject_Add_StepTargets(fluidsynth_ext configure)
	add_library(thread_safety OBJECT EXCLUDE_FROM_ALL ${PIM32_SOURCES})
	target_link_libraries(thread_safety PRIVATE pim32_headers)
	target_compile_options(thread_safety PRIVATE -Wthread-safety -Werror=thread-safety)
	add_dependencies(thread_safety munt_ext-configure fluidsynth_ext-configure)
endif()

#
# Tests
#
enable_testing()

add_library(testsupport STATIC
	support/check.cpp
	support/sf2writer.cpp
	support/fixture.cpp
	support/system.cpp)
target_include_directories(testsupport PUBLIC support)
target_compile_definitions(testsupport PUBLIC PIM32_SOURCE_ROOT="${PIM32_ROOT}")
target_link_libraries(testsupport PUBLIC pim32)

set(TEST_ENV
	"TSAN_OPTIONS=halt_on_error=1 exitcode=66 second_deadlock_stack=1 report_thread_leaks=0"
	"ASAN_OPTIONS=detect_leaks=0 abort_on_error=0"
	"UBSAN_OPTIONS=print_stacktrace=1 halt_on_error=1")

# pim32_test(NAME [EXPECT pass|race|bug] [MATCH regex] [LABELS ...])
#   pass  the test must pass
#   race  ThreadSanitizer must report a data race whose report matches MATCH
#         (only in the thread build; it must pass in the others)
#   bug   a known bug (punchlist O1-O4): the test must fail, in every build
function(pim32_test NAME)
	cmake_parse_arguments(T "" "EXPECT;MATCH" "LABELS" ${ARGN})
	if(NOT T_EXPECT)
		set(T_EXPECT pass)
	endif()
	add_executable(${NAME} tests/${NAME}.cpp)
	target_link_libraries(${NAME} PRIVATE testsupport)
	set(MODE ${T_EXPECT})
	if(MODE STREQUAL "race" AND NOT "thread" IN_LIST PIM32_SANITIZE)
		set(MODE pass)
	endif()
	add_test(NAME ${NAME}
		COMMAND ${CMAKE_COMMAND}
			-DEXE=$<TARGET_FILE:${NAME}>
			-DMODE=${MODE}
			"-DMATCH=${T_MATCH}"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_test.cmake)
	set_tests_properties(${NAME} PROPERTIES
		ENVIRONMENT "${TEST_ENV}"
		LABELS "${T_LABELS}"
		SKIP_REGULAR_EXPRESSION "PIM32_TEST_SKIPPED"
		TIMEOUT 300)
endfunction()

# Unit tests
pim32_test(test_midiparser)
pim32_test(test_midiparser_tunerequest EXPECT bug)	# O2
pim32_test(test_ringbuffer)
pim32_test(test_zoneallocator)
pim32_test(test_config)
pim32_test(test_config_ip_defaults EXPECT bug)	# O5
pim32_test(test_midimonitor_note127 EXPECT bug)	# O1
pim32_test(test_ui_format EXPECT bug)		# O3

# Concurrency tests
if(PIM32_EXPECT_SFLOAD_RACE STREQUAL "race")
	set(SFLOAD_EXPECT race)
else()
	set(SFLOAD_EXPECT pass)
endif()
pim32_test(test_sf_switch_race EXPECT ${SFLOAD_EXPECT}
	MATCH "fluid_synth_sfload|fluid_defsfloader_load|CSoundFontSynth::Reinitialize")
pim32_test(test_sf_midi_while_rendering)
pim32_test(test_system_soundfont)
pim32_test(test_system_lcd_race EXPECT race MATCH "CMIDIMonitor|CUserInterface")	# C6, C7

# Need MT-32 ROMs ($PIM32_MT32_ROMS); they skip themselves without them
pim32_test(test_mt32_midi_while_rendering LABELS roms)
pim32_test(test_mt32_isactive_race EXPECT race MATCH "CMT32Synth::IsActive" LABELS roms)	# C4
pim32_test(test_mt32_lcd_romswitch_race EXPECT race MATCH "getDisplayState|readMemory|CMT32Synth::UpdateLCD|GetPartLevels" LABELS roms)	# C3
pim32_test(test_switch_synth_race EXPECT race MATCH "CMT32Pi::SwitchSynth|CMT32Pi::AudioTask|CMT32Synth::IsActive" LABELS roms)	# C5 (C4)
