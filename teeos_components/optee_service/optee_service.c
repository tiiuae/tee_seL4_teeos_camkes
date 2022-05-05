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

int ipc_ree_comm_init(void)
{
    int err = teeos_init_optee();
    if (err) {
        ZF_LOGE("ERROR teeos_init_optee: %d", err);
    }

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