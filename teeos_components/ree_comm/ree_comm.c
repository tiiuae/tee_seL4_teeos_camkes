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

#include <linux/dt-bindings/mailbox/miv-ihc.h>
#include <linux/mailbox/miv_ihc_message.h>

#include <rpmsg_platform.h>
#include <rpmsg_sel4.h>
#include "ree_comm_defs.h"

struct camkes_app_ctx {
    ps_io_ops_t ops;

    struct sel4_rpmsg_config rpmsg_cfg;
};

static struct camkes_app_ctx app_ctx;

static int setup_rpsmg(struct sel4_rpmsg_config *cfg)
{
    int err = -1;

    /* IHC memory allocation */
    cfg->ihc_buf_va = camkes_dma_alloc(sizeof(struct ihc_sbi_msg), 0, 0);
    if (!cfg->ihc_buf_va) {
        ZF_LOGF("camkes_dma_alloc error");
        return -ENOMEM;
    }

    cfg->ihc_buf_pa = camkes_dma_get_paddr(cfg->ihc_buf_va);
    if (!cfg->ihc_buf_pa) {
        ZF_LOGF("camkes_dma_get_paddr error");
        return -EIO;
    }

    /* Create RPMSG remote endpoint and wait for master to come online */
    err = rpmsg_create_sel4_ept(cfg);
    if (err) {
        return err;
    }

    /* Announce RPMSG TTY endpoint to linux */
    err = rpmsg_announce_sel4_ept();
    if (err) {
        return err;
    }

    return err;
}

static int wait_ree_rpmsg_msg()
{
    int err = -1;
    char *msg = NULL;
    uint32_t msg_len = 0;

    while (1) {
        ZF_LOGI("waiting REE msg...");

        /* function allocates memory for msg */
        err = rpmsg_wait_ree_msg(&msg, &msg_len);
        if (err) {
            ZF_LOGE("ERROR rpmsg_wait_ree_msg: %d", err);
            continue;
        }

        ZF_LOGI("REE msg: %d", msg_len);

        free(msg);
        msg = NULL;
    }

    return err;
}

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

void tty_irq_handle(void)
{
    ZF_LOGI("tty_irq_handle");

    rpmsg_ntf_source_emit();
}

int run()
{
    int err = -1;
    uint32_t rng_len = 0;

    ZF_LOGI("started: ree_comm");

    /* sys_ctl ping */
    err = ipc_sys_ctl_get_rng(&rng_len);
    if (err) {
        ZF_LOGE("ipc_sys_ctl_get_rng: %d", err);
        return err;
    }

    ZF_LOGI("ipc_sys_ctl_get_rng [%d]", rng_len);
    utils_memory_dump(ipc_sys_ctl_buf, rng_len, 1);

    err = camkes_io_ops(&app_ctx.ops);
    if (err) {
        ZF_LOGF("camkes_io_ops: %d", err);
        return err;
    }

    app_ctx.rpmsg_cfg.irq_handler_ack = tty_irq_handler_ack;
    app_ctx.rpmsg_cfg.irq_notify_wait = tty_notify_wait;
    app_ctx.rpmsg_cfg.vring_va = rpmsg_buf;
    app_ctx.rpmsg_cfg.vring_pa = RPMSG_BUFFER_PADDR;

    err = setup_rpsmg(&app_ctx.rpmsg_cfg);
    if (err) {
        return err;
    }

    return wait_ree_rpmsg_msg();
}
