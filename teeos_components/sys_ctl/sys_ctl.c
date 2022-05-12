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
#include "sys_ctl_defs.h"
#include "sel4_crashlog.h"

#define UNUSED_VALUE        0x0

static struct crashlog_ctx crashlog = { 0 };

static int ipc_get_serial_number(uint32_t *serial_len, uint8_t *buf)
{
    int err = get_serial_number(buf);
    if (err) {
        ZF_LOGE("ERROR get_serial_number: %d", err);
        return err;
    }

    *serial_len = MSS_SYS_SERIAL_NUMBER_RESP_LEN;

    return err;
}

int ipc_ree_comm_get_serial_number(uint32_t *serial_len)
{
    return ipc_get_serial_number(serial_len, (uint8_t *)ipc_ree_comm_buf);
}

int ipc_optee_get_serial_number(uint32_t *serial_len)
{
    return ipc_get_serial_number(serial_len, (uint8_t *)ipc_optee_buf);
}

static int ipc_comm_get_rng(uint32_t *rng_len, uint8_t *buf)
{
    int err = nonce_service(buf);
    if (err) {
        ZF_LOGE("ERROR nonce_service: %d", err);
        return err;
    }

    *rng_len = MSS_SYS_NONCE_SERVICE_RESP_LEN;

    return err;
}

int ipc_ree_comm_get_rng(uint32_t *rng_len)
{
    return ipc_comm_get_rng(rng_len, (uint8_t *)ipc_ree_comm_buf);
}

int ipc_optee_get_rng(uint32_t *rng_len)
{
    return ipc_comm_get_rng(rng_len, (uint8_t *)ipc_optee_buf);
}

static int ipc_puf_emulation_service(
    uint8_t *ipc_buf,
    uint32_t ipc_len,
    uint32_t challenge_pos,
    uint8_t op_type,
    uint32_t resp_buf_pos)
{
    if (challenge_pos + FEK_SIZE > ipc_len ||
        resp_buf_pos < FEK_SIZE ||
        resp_buf_pos + MSS_SYS_PUF_EMULATION_SERVICE_RESP_LEN > ipc_len) {
        ZF_LOGE("invalid parameters: %d, %d", challenge_pos, resp_buf_pos);
        return -EINVAL;
    }

    return puf_emulation_service(ipc_buf + challenge_pos,
                                 op_type,
                                 ipc_buf + resp_buf_pos);
}

int ipc_ree_comm_puf_emulation_service(
    uint32_t challenge_pos,
    uint8_t op_type,
    uint32_t resp_buf_pos)
{
    return ipc_puf_emulation_service((uint8_t *)ipc_ree_comm_buf,
                                     ipc_ree_comm_buf_size,
                                     challenge_pos,
                                     op_type,
                                     resp_buf_pos);
}

int ipc_optee_puf_emulation_service(
    uint32_t challenge_pos,
    uint8_t op_type,
    uint32_t resp_buf_pos)
{
    return ipc_puf_emulation_service((uint8_t *)ipc_optee_buf,
                                     ipc_optee_buf_size,
                                     challenge_pos,
                                     op_type,
                                     resp_buf_pos);
}

static int ipc_secure_nvm_write(uint8_t format,
                                uint8_t snvm_module,
                                uint8_t *p_data,
                                uint8_t *p_user_key)
{
    return secure_nvm_write(format,
                            snvm_module,
                            p_data,
                            p_user_key);
}

int ipc_ree_comm_secure_nvm_write(uint8_t format, uint8_t snvm_module)
{
    return ipc_secure_nvm_write(format,
                                snvm_module,
                                (uint8_t *)ipc_ree_comm_buf,
                                NULL);
}

int ipc_optee_secure_nvm_write(uint8_t format, uint8_t snvm_module)
{
    return ipc_secure_nvm_write(format,
                                snvm_module,
                                (uint8_t *)ipc_optee_buf,
                                NULL);
}

static int ipc_secure_nvm_read(uint8_t snvm_module,
                               uint8_t *user_key,
                               uint8_t *admin,
                               uint8_t *data,
                               uint16_t data_len)
{
    return secure_nvm_read(snvm_module,
                           user_key,
                           admin,
                           data,
                           data_len);

}

int ipc_ree_comm_secure_nvm_read(uint8_t snvm_module,
                                 uint32_t admin_offset,
                                 uint32_t data_offset,
                                 uint32_t data_len)
{
    if (admin_offset + sizeof(uint32_t) > ipc_ree_comm_buf_size ||
        data_offset + data_len > ipc_ree_comm_buf_size) {
        ZF_LOGE("invalid parameters: %d, %d, %d", admin_offset, data_offset, data_len);
        return -EINVAL;
    }

    return ipc_secure_nvm_read(
                    snvm_module,
                    NULL,
                    (uint8_t *)ipc_ree_comm_buf + admin_offset,
                    (uint8_t *)ipc_ree_comm_buf + data_offset,
                    data_len);
}

int ipc_optee_secure_nvm_read(uint8_t snvm_module,
                              uint32_t admin_offset,
                              uint32_t data_offset,
                              uint32_t data_len)
{
    if (admin_offset + sizeof(uint32_t) > ipc_optee_buf_size ||
        data_offset + data_len > ipc_optee_buf_size) {
        ZF_LOGE("invalid parameters: %d, %d, %d", admin_offset, data_offset, data_len);
        return -EINVAL;
    }

    return ipc_secure_nvm_read(
                    snvm_module,
                    NULL,
                    (uint8_t *)ipc_optee_buf + admin_offset,
                    (uint8_t *)ipc_optee_buf + data_offset,
                    data_len);
}


void pre_init(void)
{
    uint32_t *sys_ctl_base = (uint32_t *)sys_ctl_reg;
    uint32_t *sys_ctl_mailbox = (uint32_t *)(sys_ctl_reg + SYS_CTL_MB_OFFSET);

    set_sys_ctl_address(sys_ctl_base, sys_ctl_mailbox, UNUSED_VALUE);
}

int run(void)
{
    /* Wait until ree_com has intialized crashlog area before
     * setting up ZF-callback.
     */
    crashlog_ready_wait();
    sel4_crashlog_setup_cb(&crashlog, crashlog_buf);

    ZF_LOGI("started: sys_ctl");

    return 0;
}
