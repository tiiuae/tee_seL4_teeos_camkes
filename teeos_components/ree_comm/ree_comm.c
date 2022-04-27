/*
 * Copyright 2022, Unikie
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>

#include <camkes.h>
#include <sel4/simple_types.h>
#include <utils/zf_log.h>
#include <utils/zf_log_if.h>

#include "ree_comm.h"
#include "ree_comm_defs.h"

static struct ree_comm_lib_cfg lib_cfg;

/* function prototype aligned with seL4_Wait() */
static void tty_notify_wait(seL4_CPtr src __attribute__((unused)),
                            seL4_Word *sender __attribute__((unused)))
{
    rpmsg_ntf_target_wait();
}

/* function prototype aligned with seL4_IRQHandler_Ack() */
static seL4_Error tty_irq_handler_ack(seL4_IRQHandler _service __attribute__((unused)))
{
    return tty_irq_acknowledge();
}

void pre_init(void)
{
    lib_cfg.rpmsg_cfg.irq_handler_ack = tty_irq_handler_ack;
    lib_cfg.rpmsg_cfg.irq_notify_wait = tty_notify_wait;
    lib_cfg.rpmsg_cfg.vring_va = rpmsg_buf;
    lib_cfg.rpmsg_cfg.vring_pa = RPMSG_BUFFER_PADDR;
    lib_cfg.crashlog_buf = crashlog_buf;
}

void tty_irq_handle(void)
{
    ZF_LOGI("tty_irq_handle");

    rpmsg_ntf_source_emit();
}

int run(void)
{
    return ree_comm_run(&lib_cfg);
}
