/*
 * Copyright 2022, Unikie
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>

#include <camkes/io.h>
#include <camkes/irq.h>
#include <camkes/dataport.h>
#include <camkes/dma.h>
#include <utils/zf_log.h>
#include <utils/zf_log_if.h>

#include <linux/dt-bindings/mailbox/miv-ihc.h>
#include <linux/mailbox/miv_ihc_message.h>

#include <rpmsg_platform.h>
#include <rpmsg_sel4.h>
#include "ree_comm.h"
#include "ree_comm_defs.h"

struct camkes_app_ctx {
    ps_io_ops_t ops;

    void *ihc_buf_va;
    uintptr_t ihc_buf_pa;

    struct sel4_rpmsg_config temp_rpmsg_cfg;
};

static struct camkes_app_ctx app_ctx;

#define SBI_EXT_IHC_RX 0x2

/* Temporary implementation to ack IHC interrupts and enable linux boot */
void ree_comm_irq_handle(irq_acknowledge_fn tty_irq_ack)
{
    int err = -1;

    struct ihc_sbi_msg *resp = app_ctx.ihc_buf_va;

    ZF_LOGI("irq_handle");

    if (!tty_irq_ack) {
        ZF_LOGF("Invalid params");
        return;
    }

    memset(app_ctx.ihc_buf_va, 0xFF, sizeof(struct ihc_sbi_msg));

    seL4_HssIhcCall(SBI_EXT_IHC_RX, IHC_CONTEXT_A, app_ctx.ihc_buf_pa);

    switch (resp->irq_type) {
    case IHC_MP_IRQ:
        ZF_LOGI("IHC_MP_IRQ: [0x%x 0x%x]", resp->ihc_msg.msg[0],
                resp->ihc_msg.msg[1]);
        break;
    case IHC_ACK_IRQ:
        ZF_LOGI("IHC_ACK_IRQ");
        break;
    default:
        ZF_LOGI("IRQ N/A [0x%x]", resp->irq_type);
    }

    err = tty_irq_ack();
    ZF_LOGF_IF(err, "irq_acknowledge");
}


/* run the control thread */
int ree_comm_run(irq_acknowledge_fn tty_irq_ack,
                    void *crashlog_buf) 
{
    int err = -1;
    uint8_t *crashlog = (uint8_t *) crashlog_buf;

    if (!tty_irq_ack || !crashlog_buf) {
        ZF_LOGF("Invalid params");
        return -EINVAL;
    }

    ZF_LOGI("started: ree_comm");

    err = camkes_io_ops(&app_ctx.ops);
    if (err) {
        ZF_LOGF("camkes_io_ops: %d", err);
        return err;
    }

    /* Temporary config struct for rpmsg to setup IRQ */
    ZF_LOGI("platform_init");
    platform_init_sel4(&app_ctx.temp_rpmsg_cfg);

    err = platform_init();
    if (err) {
        ZF_LOGF("platform_init: %d", err);
        return err;
    }

    /* IHC memory allocation */
    app_ctx.ihc_buf_va = camkes_dma_alloc(sizeof(struct ihc_sbi_msg), 0, 0);
    if (!app_ctx.ihc_buf_va) {
        ZF_LOGF("camkes_dma_alloc error");
        return -ENOMEM;
    }

    app_ctx.ihc_buf_pa = camkes_dma_get_paddr(app_ctx.ihc_buf_va);
    if (!app_ctx.ihc_buf_pa) {
        ZF_LOGF("camkes_dma_get_paddr error");
        return -EIO;
    }

    ZF_LOGI("IHC buff va: %p [pa: %p]", app_ctx.ihc_buf_va, (void*) app_ctx.ihc_buf_pa);

    memset(app_ctx.ihc_buf_va, 0x0, sizeof(struct ihc_sbi_msg));

    memset(crashlog, 0x0, SEL4_CRASHLOG_LEN);
    memcpy(crashlog, "seL4 alive!!", 12);

    /* Handle next irq */
    ZF_LOGI("Ack IRQ");
    err = tty_irq_ack();
    if (err) {
        ZF_LOGF("irq_acknowledge: %d", err);
        return err;
    }

    return 0;
}
