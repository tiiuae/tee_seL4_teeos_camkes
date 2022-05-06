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
#include <teeos_service.h>

int get_serial_number(uint8_t *p_serial_number)
{
    int err = -1;

    uint32_t serial_len = 0;

    /* get serial number from sys_ctl */
    err = ipc_sys_ctl_get_serial_number(&serial_len);
    if (err) {
        ZF_LOGE("ERROR ipc_sys_ctl_get_serial_number: %d", err);
        return err;
    }

    if (serial_len != MSS_SYS_SERIAL_NUMBER_RESP_LEN) {
        ZF_LOGE("ERROR invalid serial number length: %d", serial_len);
        return -EINVAL;
    }

    /* copy serial number from IPC shared memory */
    memcpy(p_serial_number, ipc_sys_ctl_buf, serial_len);

    return 0;
}

int puf_emulation_service
(
    uint8_t * p_challenge,  /* 128bit input*/
    uint8_t op_type,
    uint8_t* p_response /* 256bit response */
)
{
    int err = -1;

    /* copy challenge to IPC shared memory */
    memcpy(ipc_sys_ctl_buf, p_challenge, FEK_SIZE);

    err = ipc_sys_ctl_puf_emulation_service(
                0,       /* challenge in the beginning of buffer*/
                op_type,
                FEK_SIZE /* response placed right after challenge */
    );
    if (err) {
        ZF_LOGE("ERROR ipc_sys_ctl_puf_emulation_service: %d", err);
        return err;
    }

    /* copy response from IPC shared memory */
    memcpy(p_response,
           ipc_sys_ctl_buf + FEK_SIZE,
           MSS_SYS_PUF_EMULATION_SERVICE_RESP_LEN);

    return 0;
}

int nonce_service(uint8_t *p_nonce)
{
    int err = -1;

    uint32_t rng_len = 0;

    if (!p_nonce) {
        ZF_LOGE("ERROR invalid param");
        return -EINVAL;
    }

    /* get rng from sys_ctl */
    err = ipc_sys_ctl_get_rng(&rng_len);
    if (err) {
        ZF_LOGE("ERROR ipc_sys_ctl_get_rng: %d", err);
        return err;
    }

    if (rng_len != MSS_SYS_NONCE_SERVICE_RESP_LEN) {
        ZF_LOGE("ERROR invalid rng length: %d", rng_len);
        return -EINVAL;
    }

    /* copy rng from IPC shared memory */
    memcpy(p_nonce, ipc_sys_ctl_buf, rng_len);

    return 0;

}

int secure_nvm_write(uint8_t format,
                     uint8_t snvm_module,
                     uint8_t *p_data,
                     uint8_t *p_user_key)
{
    int err = -1;

    if (format != MSS_SYS_SNVM_NON_AUTHEN_TEXT_REQUEST_CMD ||
        p_user_key) {
        ZF_LOGE("ERROR authenticated writes not supported");
        return -EPERM;
    }

    if (!p_data) {
        ZF_LOGE("ERROR invalid param");
        return -EINVAL;
    }

    /* copy challenge to IPC shared memory */
    memcpy(ipc_sys_ctl_buf, p_data, NVM_PAGE_SIZE);

    /* copy to buffer before IPC call */
    ipc_sys_ctl_buf_release();

    err = ipc_sys_ctl_secure_nvm_write(format, snvm_module);
    if (err) {
        ZF_LOGE("ERROR ipc_sys_ctl_secure_nvm_write: %d", err);
        return err;
    }

    return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
int read_nvm_parameters(uint8_t *resp)
{
    ZF_LOGE("NOT IMPLEMENTED");
    return -EPERM;
}



int secure_nvm_read
(
    uint8_t snvm_module,
    uint8_t* p_user_key,
    uint8_t* p_admin,
    uint8_t* p_data,
    uint16_t data_len
)
{
    ZF_LOGE("NOT IMPLEMENTED");
    return -EPERM;
}

#pragma GCC diagnostic pop
