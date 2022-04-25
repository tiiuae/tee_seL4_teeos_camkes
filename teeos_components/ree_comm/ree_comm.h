/*
 * Copyright 2022, Unikie
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#ifndef _REE_COMM_H_
#define _REE_COMM_H_

#include <stdint.h>

typedef int (*irq_acknowledge_fn)(void);

void ree_comm_irq_handle(irq_acknowledge_fn tty_irq_ack);
int ree_comm_run(irq_acknowledge_fn tty_irq_ack,
                 void *crashlog_buf);

#endif /* _REE_COMM_H_ */