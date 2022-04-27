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

    struct sel4_rpmsg_config rpmsg_cfg;
};

static struct camkes_app_ctx app_ctx;

static int setup_rpsmg(struct camkes_app_ctx *ctx, 
                       struct ree_comm_lib_cfg *lib_cfg)
{
    int err = -1;

    /* Copy rpmsg_cfg BEFORE allocating IHC buffer */
    memcpy(&ctx->rpmsg_cfg, &lib_cfg->rpmsg_cfg, sizeof(struct sel4_rpmsg_config));

    /* IHC memory allocation */
    ctx->rpmsg_cfg.ihc_buf_va = camkes_dma_alloc(sizeof(struct ihc_sbi_msg), 0, 0);
    if (!ctx->rpmsg_cfg.ihc_buf_va) {
        ZF_LOGF("camkes_dma_alloc error");
        return -ENOMEM;
    }

    ctx->rpmsg_cfg.ihc_buf_pa = camkes_dma_get_paddr(ctx->rpmsg_cfg.ihc_buf_va);
    if (!ctx->rpmsg_cfg.ihc_buf_pa) {
        ZF_LOGF("camkes_dma_get_paddr error");
        return -EIO;
    }

    /* Create RPMSG remote endpoint and wait for master to come online */
    err = rpmsg_create_sel4_ept(&ctx->rpmsg_cfg);
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

int ree_comm_run(struct ree_comm_lib_cfg *lib_cfg) 
{
    int err = -1;

    if(!lib_cfg ||
       !lib_cfg->crashlog_buf ||
       !lib_cfg->rpmsg_cfg.vring_va ||
       !lib_cfg->rpmsg_cfg.vring_pa ||
       !lib_cfg->rpmsg_cfg.irq_notify_wait ||
       !lib_cfg->rpmsg_cfg.irq_handler_ack) {
        ZF_LOGF("Invalid params");
        return -EINVAL;
    }

    ZF_LOGI("started: ree_comm");

    err = camkes_io_ops(&app_ctx.ops);
    if (err) {
        ZF_LOGF("camkes_io_ops: %d", err);
        return err;
    }

    err = setup_rpsmg(&app_ctx, lib_cfg);
    if (err) {
        return err;
    }

    return wait_ree_rpmsg_msg();
}
