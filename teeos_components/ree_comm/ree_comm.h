/*
 * Copyright 2022, Unikie
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#ifndef _REE_COMM_H_
#define _REE_COMM_H_

#include <stdint.h>
#include <rpmsg_sel4.h>

struct ree_comm_lib_cfg {
    struct sel4_rpmsg_config rpmsg_cfg;

    void *crashlog_buf;
};

int ree_comm_run(struct ree_comm_lib_cfg *lib_cfg);

#endif /* _REE_COMM_H_ */