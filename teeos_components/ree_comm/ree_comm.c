/*
 * Copyright 2022, Unikie
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>

#include <camkes.h>
#include <utils/zf_log.h>
#include <utils/zf_log_if.h>

#include "ree_comm.h"

void tty_irq_handle(void)
{
    ree_comm_irq_handle(tty_irq_acknowledge);
}

int run(void)
{
    return ree_comm_run(tty_irq_acknowledge, crashlog_buf);
}
