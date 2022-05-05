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

#include <teeos_service.h>

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