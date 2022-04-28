#
# Copyright 2022, Unikie
#
# SPDX-License-Identifier: BSD-2-Clause
#

add_library(ree_comm_lib STATIC
    ree_comm.c
)

target_compile_options(ree_comm_lib PRIVATE
    -Wall
    -Wextra
    -Werror
)

target_include_directories(ree_comm_lib
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/include"
)

target_link_libraries(ree_comm_lib
    PRIVATE
        sel4runtime
        sel4camkes
        rpmsg_lite
        muslc
)

DeclareCAmkESComponent(Ree_comm
    INCLUDES
    LIBS
        rpmsg_lite
        ree_comm_lib
    C_FLAGS
        -Wall
        -Werror
)
