#
# Copyright 2022, Unikie
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

set(project_dir "${CMAKE_CURRENT_LIST_DIR}/../..")
file(GLOB project_modules ${project_dir}/projects/*)
list(
    APPEND
        CMAKE_MODULE_PATH
        ${project_dir}/kernel
        ${project_dir}/tools/seL4/cmake-tool/helpers/
        ${project_dir}/tools/seL4/elfloader-tool/
        ${project_dir}/tools/seL4/gen_hss_payload/
        ${project_modules}
)

set(TOMMATH_SRC_FOLDER "${project_dir}/projects/crypto/libtommath" CACHE INTERNAL "")
set(TOMCRYPT_SRC_FOLDER "${project_dir}/projects/crypto/optee_os/core/lib/libtomcrypt" CACHE INTERNAL "")
set(OPTEE_SRC_FOLDER "${project_dir}/projects/crypto/optee_os" CACHE INTERNAL "")

set(SEL4_CONFIG_DEFAULT_ADVANCED ON)

set(PLATFORM "polarfire" CACHE STRING "" FORCE)
set(UseRiscVOpenSBI OFF CACHE BOOL "")
set(LibUtilsDefaultZfLogLevel 3 CACHE INTERNAL "")

set(ElfloaderImage "binary" CACHE STRING "" FORCE)
# Elfloader binary start address. This setting seems to have no effect and elfloader
# start location defined by HSS
set(IMAGE_START_ADDR "0x88000000" CACHE STRING "")

set(KernelPlatform "${PLATFORM}" CACHE STRING "" FORCE)
set(KernelRiscvExtD ON CACHE BOOL "")
set(KernelRootCNodeSizeBits 13 CACHE INTERNAL "")
set(KernelVerificationBuild OFF CACHE BOOL "" FORCE)
set(KernelDebugBuild ON CACHE BOOL "" FORCE)
set(KernelPrinting ON CACHE BOOL "" FORCE)
set(KernelCustomDTSOverlay "${CMAKE_CURRENT_LIST_DIR}/overlay-polarfire_amp.dts" CACHE FILEPATH "" FORCE)
add_compile_definitions(KERNEL_ELF_PADDR_BASE_START=0x0)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)

set(CapDLLoaderPrintDeviceInfo ON)
set(CapDLLoaderPrintUntypeds ON)
set(CapDLLoaderPrintCapDLObjects ON)

find_package(seL4 REQUIRED)
sel4_configure_platform_settings()

##############################################################################
project(teeos_camkes C CXX ASM)

# rdtime-counter frequency
config_set(HwTimerFreq HW_TIMER_FREQ 1000000)

set(ExtraCompileOptions
    -Werror
    -Wall
    -Wextra
    -g
)

find_package(camkes-tool REQUIRED)
camkes_tool_setup_camkes_build_environment()

add_subdirectory(teeos_components)

# Generate HSS payload binary for testing purposes. Only seL4 image is included.
find_package(gen_hss_payload REQUIRED)
gen_hss_payload_import_project()
