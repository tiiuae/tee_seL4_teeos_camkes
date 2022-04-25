#
# Copyright 2022, Unikie
#
# SPDX-License-Identifier: BSD-2-Clause
#

find_package(sel4_teeos REQUIRED)
sel4_teeos_import_project()

CAmkESAddCPPInclude(${SEL4_TEEOS_INCLUDE})

add_subdirectory(ree_comm)

DeclareCAmkESRootserver(teeos_amp.camkes)

GenerateCAmkESRootserver()
