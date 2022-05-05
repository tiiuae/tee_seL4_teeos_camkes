#
# Copyright 2022, Unikie
#
# SPDX-License-Identifier: BSD-2-Clause
#

add_library(optee_service STATIC
    optee_service.c
)

target_compile_options(optee_service PRIVATE
    -Wall
    -Wextra
    -Werror
)

target_include_directories(optee_service
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/include"
        "${SEL4_TEEOS_INCLUDE}"
)

target_link_libraries(optee_service
    PRIVATE
        sel4runtime
        sel4camkes
        muslc
        teeos_service
)

DeclareCAmkESComponent(Optee_service
    INCLUDES
    LIBS
        optee_service
    C_FLAGS
        -Wall
        -Werror
)
