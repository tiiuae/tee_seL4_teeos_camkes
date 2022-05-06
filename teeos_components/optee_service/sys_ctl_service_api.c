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

int get_serial_number(uint8_t *serial_number)
{
    int err = -1;

    uint32_t serial_len = 0;

    if (!serial_number) {
        ZF_LOGE("ERROR invalid param");
        return -EINVAL;
    }

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
    memcpy(serial_number, ipc_sys_ctl_buf, serial_len);

    return 0;
}

int puf_emulation_service (uint8_t *challenge, /* 128bit input*/
                           uint8_t op_type,
                           uint8_t *response   /* 256bit response */)
{
    int err = -1;

    if (!challenge || !response) {
        ZF_LOGE("ERROR invalid param");
        return -EINVAL;
    }

    /* copy challenge to IPC shared memory */
    memcpy(ipc_sys_ctl_buf, challenge, FEK_SIZE);

    /* copy to buffer before IPC call */
    ipc_sys_ctl_buf_release();

    err = ipc_sys_ctl_puf_emulation_service(
                0,       /* challenge in the beginning of buffer*/
                op_type,
                FEK_SIZE /* response placed right after challenge */
    );
    if (err) {
        ZF_LOGE("ERROR ipc_sys_ctl_puf_emulation_service: %d", err);
        return err;
    }

    /* IPC call before copy from buffer */
    ipc_sys_ctl_buf_release();

    /* copy response from IPC shared memory */
    memcpy(response,
           ipc_sys_ctl_buf + FEK_SIZE,
           MSS_SYS_PUF_EMULATION_SERVICE_RESP_LEN);

    return 0;
}

int nonce_service(uint8_t *nonce)
{
    int err = -1;

    uint32_t rng_len = 0;

    if (!nonce) {
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
    memcpy(nonce, ipc_sys_ctl_buf, rng_len);

    return 0;

}

int secure_nvm_write(uint8_t format,
                     uint8_t snvm_module,
                     uint8_t *data,
                     uint8_t *user_key)
{
    int err = -1;

    if (format != MSS_SYS_SNVM_NON_AUTHEN_TEXT_REQUEST_CMD ||
        user_key) {
        ZF_LOGE("ERROR authenticated writes not supported");
        return -EPERM;
    }

    if (!data) {
        ZF_LOGE("ERROR invalid param");
        return -EINVAL;
    }

    /* copy challenge to IPC shared memory */
    memcpy(ipc_sys_ctl_buf, data, NVM_PAGE_SIZE);

    /* copy to buffer before IPC call */
    ipc_sys_ctl_buf_release();

    err = ipc_sys_ctl_secure_nvm_write(format, snvm_module);
    if (err) {
        ZF_LOGE("ERROR ipc_sys_ctl_secure_nvm_write: %d", err);
        return err;
    }

    return 0;
}

int secure_nvm_read(uint8_t snvm_module,
                    uint8_t *user_key,
                    uint8_t *admin,
                    uint8_t *data,
                    uint16_t data_len)
{
    int err = -1;

    if (!admin || !data) {
        ZF_LOGE("ERROR invalid param");
        return -EINVAL;
    }

    if (user_key) {
        ZF_LOGE("ERROR authenticated reads not supported");
        return -EPERM;
    }

    err = ipc_sys_ctl_secure_nvm_read(snvm_module,
                                      0, /* admin in the beginning of IPC buffer */
                                      sizeof(uint32_t), /* data placed after admin */
                                      data_len);
    if (err) {
        ZF_LOGE("ERROR ipc_sys_ctl_secure_nvm_write: %d", err);
        return err;
    }

    /* IPC call before copy from buffer */
    ipc_sys_ctl_buf_release();

    memcpy(admin, ipc_sys_ctl_buf, sizeof(uint32_t));

    memcpy(data, ipc_sys_ctl_buf + sizeof(uint32_t), data_len);

    return 0;
}
