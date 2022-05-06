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

int ipc_ree_comm_init(void)
{
    int err = teeos_init_optee();
    if (err) {
        ZF_LOGE("ERROR teeos_init_optee: %d", err);
    }

    return err;
}

int ipc_ree_comm_export_storage(void)
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

    err = teeos_optee_export_storage(storage->pos,
                                     &storage_len,
                                     storage->payload,
                                     max_size,
                                     &export_len);
    if (err) {
        ZF_LOGE("ERROR teeos_optee_export_storage %d ", err);
        return err;
    }

    storage->storage_len = storage_len;
    storage->payload_len = export_len;

    return err;
}
int run(void)
{
    int err = -1;

    ZF_LOGI("started: optee_service");

    err = teeos_init_crypto();
    if (err) {
        ZF_LOGE("ERROR teeos_init_crypto: %d", err);
        return err;
    }

    return 0;
}