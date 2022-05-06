/*
 * Copyright 2022, Unikie
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>

/* Local log level */
#define ZF_LOG_LEVEL    ZF_LOG_ERROR
#include <utils/util.h>
#include <utils/zf_log.h>
#include <utils/zf_log_if.h>
#include <utils/debug.h>

#include <camkes.h>
#include <camkes/io.h>
#include <camkes/irq.h>
#include <camkes/dataport.h>
#include <camkes/dma.h>

#include <linux/dt-bindings/mailbox/miv-ihc.h>
#include <linux/mailbox/miv_ihc_message.h>

#include <rpmsg_platform.h>
#include <rpmsg_sel4.h>
#include <ree_tee_msg.h>
#include "ree_comm_defs.h"

struct camkes_app_ctx {
    ps_io_ops_t ops;

    struct sel4_rpmsg_config rpmsg_cfg;

    uint64_t debug_config;
};

static struct camkes_app_ctx app_ctx;

#define SET_REE_HDR(hdr, msg, stat, len) {  \
            (hdr)->msg_type = msg;          \
            (hdr)->status = stat;           \
            (hdr)->length = len;            \
        }

#define REE_HDR_LEN     sizeof(struct ree_tee_hdr)

/* For succesfull operation function allocates memory for reply_msg.
 * Otherwise function sets err_msg and frees all allocated memory
  */
typedef int (*ree_tee_msg_fn)(struct ree_tee_hdr*, struct ree_tee_hdr**, struct ree_tee_hdr*);

#define DECL_MSG_FN(fn_name)                           \
    static int fn_name(struct ree_tee_hdr *ree_msg,    \
                       struct ree_tee_hdr **tee_msg, \
                       struct ree_tee_hdr *tee_err_msg)

DECL_MSG_FN(ree_tee_rng_req);
DECL_MSG_FN(ree_tee_deviceid_req);
DECL_MSG_FN(ree_tee_status_req);
DECL_MSG_FN(ree_tee_config_req);
DECL_MSG_FN(ree_tee_optee_init_req);
DECL_MSG_FN(ree_tee_optee_export_storage_req);

#define FN_LIST_LEN(fn_list)    (sizeof(fn_list) / (sizeof(fn_list[0][0]) * 2))

static uintptr_t ree_tee_fn[][2] = {
    {REE_TEE_RNG_REQ, (uintptr_t)ree_tee_rng_req},
    {REE_TEE_DEVICEID_REQ, (uintptr_t)ree_tee_deviceid_req},
    {REE_TEE_STATUS_REQ, (uintptr_t)ree_tee_status_req},
    {REE_TEE_CONFIG_REQ, (uintptr_t)ree_tee_config_req},
    {REE_TEE_OPTEE_INIT_REQ, (uintptr_t)ree_tee_optee_init_req},
    {REE_TEE_OPTEE_EXPORT_STORAGE_REQ, (uintptr_t)ree_tee_optee_export_storage_req},
};

static int ree_tee_deviceid_req(struct ree_tee_hdr *ree_msg __attribute__((unused)),
                                 struct ree_tee_hdr **tee_msg,
                                 struct ree_tee_hdr *tee_err_msg)
{
    int err = -1;
    int32_t reply_type = REE_TEE_DEVICEID_RESP;
    size_t reply_len = sizeof(struct ree_tee_deviceid_cmd);
    struct ree_tee_deviceid_cmd *reply = NULL;

    uint32_t serial_len = 0;

    ZF_LOGI("REE_TEE_DEVICEID_REQ");

    /* get serial number from sys_ctl */
    err = ipc_sys_ctl_get_serial_number(&serial_len);
    if (err) {
        ZF_LOGE("ERROR ipc_sys_ctl_get_serial_number: %d", err);
        err = TEE_IPC_CMD_ERR;
        goto err_out;
    }

    if (serial_len != DEVICE_ID_LENGTH) {
        ZF_LOGE("ERROR invalid serial number length: %d", serial_len);
        err = TEE_IPC_CMD_ERR;
        goto err_out;
    }

    reply = malloc(reply_len);
    if (!reply) {
        ZF_LOGE("ERROR out of memory");
        err = TEE_OUT_OF_MEMORY;
        goto err_out;
    }

    SET_REE_HDR(&reply->hdr, reply_type, TEE_OK, reply_len);

    /* IPC call must be done before copying from buffer */
    ipc_sys_ctl_buf_release();

    /* copy serial number from IPC buffer*/
    memcpy(reply->response, ipc_sys_ctl_buf, DEVICE_ID_LENGTH);

    *tee_msg = (struct ree_tee_hdr *)reply;

    return 0;

err_out:
    free(reply);

    SET_REE_HDR(tee_err_msg, reply_type, err, REE_HDR_LEN);

    return err;
}

static int ree_tee_rng_req(struct ree_tee_hdr *ree_msg __attribute__((unused)),
                           struct ree_tee_hdr **tee_msg,
                           struct ree_tee_hdr *tee_err_msg)
{
    int err = -1;

    int32_t reply_type = REE_TEE_RNG_RESP;
    size_t reply_len = sizeof(struct ree_tee_rng_cmd);
    struct ree_tee_rng_cmd *reply = NULL;

    uint32_t rng_len = 0;

    ZF_LOGI("REE_TEE_RNG_REQ");

    /* get random bytes from sys_ctl */
    err = ipc_sys_ctl_get_rng(&rng_len);
    if (err) {
        ZF_LOGE("ERROR ipc_sys_ctl_get_rng: %d", err);
        err = TEE_IPC_CMD_ERR;
        goto err_out;
    }

    if (rng_len != RNG_SIZE_IN_BYTES) {
        ZF_LOGE("ERROR invalid rng len: %d", rng_len);
        err = TEE_IPC_CMD_ERR;
        goto err_out;
    }

    reply = malloc(reply_len);
    if (!reply) {
        ZF_LOGE("ERROR out of memory");
        err = TEE_OUT_OF_MEMORY;
        goto err_out;
    }

    SET_REE_HDR(&reply->hdr, reply_type, TEE_OK, reply_len);

    /* IPC call must be done before copying from buffer */
    ipc_sys_ctl_buf_release();

    /* copy random numbers from IPC buffer*/
    memcpy(reply->response, ipc_sys_ctl_buf, RNG_SIZE_IN_BYTES);

    *tee_msg = (struct ree_tee_hdr *)reply;

    return 0;

err_out:
    free(reply);

    SET_REE_HDR(tee_err_msg, reply_type, err, REE_HDR_LEN);

    return err;
}

static int ree_tee_status_req(struct ree_tee_hdr *ree_msg __attribute__((unused)),
                                 struct ree_tee_hdr **tee_msg,
                                 struct ree_tee_hdr *tee_err_msg)
{
    int err = -1;

    int32_t reply_type = REE_TEE_STATUS_RESP;
    size_t reply_len = REE_HDR_LEN;
    struct ree_tee_hdr *reply = NULL;

    ZF_LOGI("REE_TEE_STATUS_REQ");

    reply = calloc(1, reply_len);
    if (!reply) {
        ZF_LOGE("ERROR out of memory");
        err = TEE_OUT_OF_MEMORY;
        goto err_out;
    }

    SET_REE_HDR(reply, reply_type, TEE_OK, reply_len);

    *tee_msg = (struct ree_tee_hdr *)reply;

    return 0;

err_out:
    free(reply);

    SET_REE_HDR(tee_err_msg, reply_type, err, REE_HDR_LEN);

    return err;
}

static int ree_tee_config_req(struct ree_tee_hdr *ree_msg,
                                 struct ree_tee_hdr **tee_msg,
                                 struct ree_tee_hdr *tee_err_msg)
{
    int err = -1;

    struct ree_tee_config_cmd *req = (struct ree_tee_config_cmd *)ree_msg;

    int32_t reply_type = REE_TEE_CONFIG_RESP;
    size_t reply_len = sizeof(struct ree_tee_config_cmd);
    struct ree_tee_config_cmd *reply = NULL;

    ZF_LOGI("REE_TEE_CONFIG_REQ");

    reply = calloc(1, reply_len);
    if (!reply) {
        ZF_LOGE("ERROR out of memory");
        err = TEE_OUT_OF_MEMORY;
        goto err_out;
    }

    /* Current config in reply */
    if (req->debug_config & (1UL << 63)) {
        reply->debug_config = app_ctx.debug_config;
    } else {
        app_ctx.debug_config = req->debug_config;
        ZF_LOGI("DEBUG config 0x%lx", req->debug_config);
    }

    SET_REE_HDR(&reply->hdr, reply_type, TEE_OK, reply_len);

    *tee_msg = (struct ree_tee_hdr *)reply;

    ZF_LOGI("reply->debug_config 0x%lx", reply->debug_config);

    return 0;

err_out:
    free(reply);

    SET_REE_HDR(tee_err_msg, reply_type, err, REE_HDR_LEN);

    return err;
}

static int ree_tee_optee_init_req(struct ree_tee_hdr *ree_msg __attribute__((unused)),
                                 struct ree_tee_hdr **tee_msg,
                                 struct ree_tee_hdr *tee_err_msg)
{
    int err = -1;

    int32_t reply_type = REE_TEE_OPTEE_INIT_RESP;
    size_t reply_len = REE_HDR_LEN;
    struct ree_tee_hdr *reply = NULL;

    ZF_LOGI("REE_TEE_OPTEE_INIT_REQ");

    err = ipc_optee_init();
    if (err) {
        ZF_LOGE("ERROR ipc_optee_init_optee: %d", err);
        err = TEE_IPC_CMD_ERR;
        goto err_out;
    }

    reply = calloc(1, reply_len);
    if (!reply) {
        ZF_LOGE("ERROR out of memory");
        err = TEE_OUT_OF_MEMORY;
        goto err_out;
    }

    SET_REE_HDR(reply, reply_type, TEE_OK, reply_len);

    *tee_msg = (struct ree_tee_hdr *)reply;

    return 0;

err_out:
    free(reply);

    SET_REE_HDR(tee_err_msg, reply_type, err, REE_HDR_LEN);

    return err;
}

static int ree_tee_optee_storage(struct ree_tee_hdr *ree_msg,
                               struct ree_tee_hdr **tee_msg,
                               struct ree_tee_hdr *tee_err_msg,
                               int32_t ree_reply_type)
{
    int err = -1;

    struct ree_tee_optee_storage_cmd *req =
        (struct ree_tee_optee_storage_cmd *)ree_msg;

    struct ree_tee_optee_storage_cmd *reply = NULL;

    struct ree_tee_optee_storage_bin *storage =
        (struct ree_tee_optee_storage_bin *)ipc_optee_buf;

    uint32_t ipc_len = 0;
    uint32_t reply_len = 0;

    if (ree_msg->length < sizeof(struct ree_tee_optee_storage_cmd)) {
        ZF_LOGE("Invalid Message size: %d", ree_msg->length);
        err = TEE_INVALID_MSG_SIZE;
        goto err_out;
    }

    /* req IPC length */
    ipc_len = req->hdr.length - REE_HDR_LEN;

    if (ipc_len > ipc_optee_buf_size) {
        ZF_LOGE("Payload overflow: %d / %d", ipc_len, ipc_optee_buf_size);
        err = TEE_PAYLOAD_OVERFLOW;
        goto err_out;
    }

    memcpy(storage, &req->storage, ipc_len);

    /* data must be copied to buffer before IPC call */
    ipc_optee_buf_release();

    err = ipc_optee_export_storage();
    if (err) {
        ZF_LOGE("ERROR ipc_optee_export_storage: %d", err);
        err = TEE_IPC_CMD_ERR;
        goto err_out;
    }

    /* IPC call must be done before reply length calculation */
    ipc_optee_buf_release();

    /* IPC resp length */
    ipc_len = sizeof(struct ree_tee_optee_storage_bin) + storage->payload_len;

    if (ipc_len > ipc_optee_buf_size) {
        ZF_LOGE("invalid IPC size: %d / %d", ipc_len, ipc_optee_buf_size);
        err = TEE_IPC_CMD_ERR;
        goto err_out;
    }

    reply_len = sizeof(struct ree_tee_optee_storage_cmd) + storage->payload_len;

    reply = calloc(1, reply_len);
    if (!reply) {
        ZF_LOGE("ERROR out of memory");
        err = TEE_OUT_OF_MEMORY;
        goto err_out;
    }

    SET_REE_HDR(&reply->hdr, ree_reply_type, TEE_OK, reply_len);

    memcpy(&reply->storage, storage, ipc_len);

    *tee_msg = (struct ree_tee_hdr *)reply;

    return 0;

err_out:
    free(reply);

    SET_REE_HDR(tee_err_msg, ree_reply_type, err, REE_HDR_LEN);

    return err;
}

static int ree_tee_optee_export_storage_req(struct ree_tee_hdr *ree_msg,
                               struct ree_tee_hdr **reply_msg,
                               struct ree_tee_hdr *reply_err)
{
    ZF_LOGI("REE_TEE_OPTEE_EXPORT_STORAGE_REQ");

    return ree_tee_optee_storage(ree_msg, reply_msg, reply_err,
                                 REE_TEE_OPTEE_EXPORT_STORAGE_RESP);
}

static int handle_rpmsg_msg(struct ree_tee_hdr *ree_msg,
                            struct ree_tee_hdr **tee_msg,
                            struct ree_tee_hdr *tee_err_msg)
{
    int err = -1;

    ree_tee_msg_fn msg_fn = NULL;

    ZF_LOGI("msg type: %d, len: %d", ree_msg->msg_type, ree_msg->length);

    /* Find msg handler callback */
    for (int i = 0; i < (ssize_t)FN_LIST_LEN(ree_tee_fn); i++) {
        /* Check if msg type is found from callback list */
        if (ree_tee_fn[i][0] != (uint32_t)ree_msg->msg_type) {
            continue;
        }

        /* Call msg handler function */
        msg_fn = (ree_tee_msg_fn)ree_tee_fn[i][1];
        err = msg_fn(ree_msg, tee_msg, tee_err_msg);
        if (err) {
            ZF_LOGE("ERROR msg_fn: %d", err);
        }
        break;
    }

    /* Unknown message */
    if (!msg_fn) {
        ZF_LOGE("ERROR unknown msg: %d", ree_msg->msg_type);
        err = TEE_UNKNOWN_MSG;

        /* Use received unknown msg type in response */
        SET_REE_HDR(tee_err_msg, ree_msg->msg_type, err, REE_HDR_LEN);
    }

    return err;
}

static int wait_ree_rpmsg_msg()
{
    int err = -1;
    char *msg = NULL;
    uint32_t msg_len = 0;
    struct ree_tee_hdr *tee_msg = NULL;
    struct ree_tee_hdr tee_err_msg = { 0 };

    struct ree_tee_hdr *send_msg = NULL;

    while (1) {
        ZF_LOGV("waiting REE msg...");

        /* function allocates memory for msg */
        err = rpmsg_wait_ree_msg(&msg, &msg_len);
        if (err) {
            ZF_LOGE("ERROR rpmsg_wait_ree_msg: %d", err);

            /* try to send error response, best effort only */
            SET_REE_HDR(&tee_err_msg, REE_TEE_STATUS_RESP, err, REE_HDR_LEN);
            rpmsg_send_ree_msg((char *)&tee_err_msg, tee_err_msg.length);

            continue;
        }

        /* function allocates memory for reply or returns err_msg */
        err = handle_rpmsg_msg((struct ree_tee_hdr*)msg, &tee_msg, &tee_err_msg);

        if (err) {
            ZF_LOGE("ERROR handle_rpmsg_msg: %d", err);
            send_msg = &tee_err_msg;
        } else {
            send_msg = tee_msg;
        }

        /* msg buffer not needed anymore */
        free(msg);
        msg = NULL;

        ZF_LOGV("resp type %d, len %d", send_msg->msg_type, send_msg->length);

        err = rpmsg_send_ree_msg((char *)send_msg, send_msg->length);
        if (err) {
            /* These are more or less fatal errors, abort*/
            ZF_LOGF("ERROR rpmsg_send_ree_msg: %d", err);
            return err;
        }

        free(tee_msg);
        tee_msg = NULL;
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

    rpmsg_ntf_source_emit();
}

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

int run()
{
    int err = -1;
    uint32_t rng_len = 0;

    ZF_LOGE("started: ree_comm, build date %s-%s", __DATE__, __TIME__);

    /* sys_ctl ping */
    err = ipc_sys_ctl_get_rng(&rng_len);
    if (err) {
        ZF_LOGE("ERROR ipc_sys_ctl_get_rng: %d", err);
        return err;
    }

    err = camkes_io_ops(&app_ctx.ops);
    if (err) {
        ZF_LOGF("ERROR camkes_io_ops: %d", err);
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
