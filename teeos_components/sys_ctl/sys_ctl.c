/*
 * Copyright 2022, Unikie
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>

#include <camkes.h>
#include <camkes/io.h>
#include <camkes/irq.h>
#include <camkes/dataport.h>
#include <camkes/dma.h>
#include <utils/zf_log.h>
#include <utils/zf_log_if.h>
#include <utils/debug.h>

#include <sys_ctl_service.h>
#include "sys_ctl_defs.h"

#define UNUSED_VALUE        0x0

int run(void)
{
    int err = -1;
    uint8_t rng[32] = { 0 };

    uint32_t *sys_ctl_base = (uint32_t *)sys_ctl_reg;
    uint32_t *sys_ctl_mailbox = (uint32_t *)(sys_ctl_reg + SYS_CTL_MB_OFFSET);

    ZF_LOGI("started: sys_ctl");

    set_sys_ctl_address(sys_ctl_base, sys_ctl_mailbox, UNUSED_VALUE);

    err = nonce_service(rng);

    ZF_LOGI("nonce_service: %d", err);
    utils_memory_dump(rng, sizeof(rng), 1);

    return 0;
}
