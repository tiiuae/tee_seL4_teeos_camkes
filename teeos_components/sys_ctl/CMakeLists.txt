#
# Copyright 2022, Unikie
#
# SPDX-License-Identifier: BSD-2-Clause
#

add_library(sys_ctl_lib STATIC
    sys_ctl.c
)

target_compile_options(sys_ctl_lib PRIVATE
    -Wall
    -Wextra
    -Werror
)

target_include_directories(sys_ctl_lib
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/include"
        "${SEL4_TEEOS_INCLUDE}"
)

target_link_libraries(sys_ctl_lib
    PRIVATE
        sel4runtime
        sel4camkes
        muslc
        sys_ctl_services
        teeos_service
        ipc_api_lib
)

DeclareCAmkESComponent(Sys_ctl
    INCLUDES
    LIBS
        sys_ctl_lib
    C_FLAGS
        -Wall
        -Werror
)
