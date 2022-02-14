#
# Copyright 2022, Unikie
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

include(settings.cmake)

project(teeos_amp_camk C CXX ASM)

set(LibUtilsDefaultZfLogLevel 3 CACHE INTERNAL "")

set(KernelRootCNodeSizeBits 13 CACHE INTERNAL "")

set(ElfloaderImage "binary" CACHE STRING "" FORCE)

# rdtime-counter frequency
config_set(HwTimerFreq HW_TIMER_FREQ 1000000)

set(UserLinkerGCSections OFF CACHE BOOL "" FORCE)

set(LibNanopb ON CACHE BOOL "" FORCE)

set(ExtraCompileOptions
#    -Werror # spinlock.h: warning: unused parameter
    -Wall
    -Wextra
    -g
)

find_package(camkes-tool REQUIRED)
camkes_tool_setup_camkes_build_environment()

add_subdirectory(teeos_components)

# Generate HSS payload binary. Only seL4 image is included.
find_package(gen_hss_payload REQUIRED)
gen_hss_payload_import_project()
