#
# Copyright 2022, Unikie
#
# SPDX-License-Identifier: BSD-2-Clause
#

find_package(sel4_teeos REQUIRED)
sel4_teeos_import_project()

CAmkESAddCPPInclude(${SEL4_TEEOS_INCLUDE})
CAmkESAddCPPInclude("${CMAKE_CURRENT_LIST_DIR}/ipc_interface")

CAmkESAddImportPath("${CMAKE_CURRENT_LIST_DIR}/ipc_interface")

add_library(ipc_api_lib INTERFACE)
target_include_directories(ipc_api_lib INTERFACE "${CMAKE_CURRENT_LIST_DIR}/ipc_interface")

add_subdirectory(ree_comm)
add_subdirectory(sys_ctl)
add_subdirectory(optee_service)

DeclareCAmkESRootserver(teeos_amp.camkes)

GenerateCAmkESRootserver()
