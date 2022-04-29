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

int ipc_sys_ctl_get_serial_number(uint32_t *serial_len)
{
    int err = get_serial_number((uint8_t *)ipc_sys_ctl_buf);
    if (err) {
        ZF_LOGE("ERROR get_serial_number: %d", err);
        return err;
    }

    *serial_len = MSS_SYS_SERIAL_NUMBER_RESP_LEN;

    return err;
}

int ipc_sys_ctl_get_rng(uint32_t *rng_len)
{
    int err = nonce_service((uint8_t *)ipc_sys_ctl_buf);
    if (err) {
        ZF_LOGE("ERROR nonce_service: %d", err);
        return err;
    }

    *rng_len = MSS_SYS_NONCE_SERVICE_RESP_LEN;

    return err;
}

void pre_init(void)
{
    uint32_t *sys_ctl_base = (uint32_t *)sys_ctl_reg;
    uint32_t *sys_ctl_mailbox = (uint32_t *)(sys_ctl_reg + SYS_CTL_MB_OFFSET);

    set_sys_ctl_address(sys_ctl_base, sys_ctl_mailbox, UNUSED_VALUE);
}

int run(void)
{
    ZF_LOGI("started: sys_ctl");
    return 0;
}
