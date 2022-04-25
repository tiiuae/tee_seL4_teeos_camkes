#
# Copyright 2022, Unikie
#
# SPDX-License-Identifier: BSD-2-Clause
#

add_library(ree_comm_lib STATIC
    ree_comm_lib.c
)

target_compile_options(ree_comm_lib PRIVATE
    -Wall
    -Wextra
    -Werror
)

target_link_libraries(ree_comm_lib
    PRIVATE
        sel4runtime
        sel4camkes
        rpmsg_lite
        muslc
)

DeclareCAmkESComponent(ree_comm
    INCLUDES
    SOURCES
        ree_comm.c
    LIBS
        ree_comm_lib
)
