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

#include <pkcs11_service.h>
#include <sel4_optee_serializer.h>
#include <teeos_service.h>
#include <ree_tee_msg.h>
#include "sel4_crashlog.h"

static struct crashlog_ctx crashlog = { 0 };

int ipc_ree_comm_init(void)
{
    int err = teeos_init_optee();
    if (err) {
        ZF_LOGE("ERROR teeos_init_optee: %d", err);
    }

    return err;
}

int ipc_ree_comm_storage_req(int32_t ree_req_type)
{
    int err = -1;

    uint32_t max_size =
        ipc_ree_comm_buf_size - sizeof(struct ree_tee_optee_storage_bin);

    struct ree_tee_optee_storage_bin *storage =
        (struct ree_tee_optee_storage_bin *)ipc_ree_comm_buf;

    uint32_t storage_len = 0;
    uint32_t export_len = 0;

    /* 16 byte alignment for crypto algorithm */
    max_size = max_size - (max_size % 16);

    if (storage->payload_len > max_size) {
        ZF_LOGE("Invalid payload length: %d", storage->payload_len);
        return -EINVAL;
    }

    if (ree_req_type == REE_TEE_OPTEE_EXPORT_STORAGE_REQ) {
        err = teeos_optee_export_storage(storage->pos,
                                         &storage_len,
                                         storage->payload,
                                         max_size,
                                         &export_len);
    }

    if (ree_req_type == REE_TEE_OPTEE_IMPORT_STORAGE_REQ) {
        err = teeos_optee_import_storage(storage->payload,
                                         storage->payload_len,
                                         storage->storage_len);
    }

    /* unknown ree_req_type fails here */
    if (err) {
        ZF_LOGE("ERROR teeos_optee_storage %d ", err);
        return err;
    }

    /* Import resp has storage and payload length set to zero */
    storage->storage_len = storage_len;
    storage->payload_len = export_len;

    return err;
}

int ipc_ree_comm_request(uint32_t req_len, uint32_t *reply_len)
{
    int err = -1;

    /* reply_len: NONNULL */

    err = sel4_optee_handle_cmd((uint8_t *)ipc_ree_comm_buf,
                                req_len,
                                reply_len,
                                ipc_ree_comm_buf_size);

    if (err) {
        ZF_LOGE("ERROR sel4_optee_handle_cmd: %d", err);
    }

    return err;
}

int run(void)
{
    int err = -1;

    /* Wait until ree_com has intialized crashlog area before
     * setting up ZF-callback.
     */
    crashlog_ready_wait();
    sel4_crashlog_setup_cb(&crashlog, crashlog_buf);

    ZF_LOGI("started: optee_service");

    err = teeos_init_crypto();
    if (err) {
        ZF_LOGE("ERROR teeos_init_crypto: %d", err);
        return err;
    }

    return 0;
}