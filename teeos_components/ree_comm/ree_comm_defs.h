/*
 * Copyright 2022, Unikie
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#ifndef _REE_COMM_DEFS_H_
#define _REE_COMM_DEFS_H_

/* RPMSG vring reservation
 *
 *     rpmsg@a2400000 {
 *         reg = <0x0 0xa2400000 0x0 0x50000>; // rpmsg + rpmsg_dma_reserved
 *     };
 */
#define RPMSG_BUFFER_PADDR  0xA2400000
#define RPMSG_BUFFER_LEN    0x50000

/*
 * Reserved memory area for seL4 crashlog.
 *
 * CIRC-header + power of 2 buffer
 *
 *     sel4_crashlog@a2450000 {
 *         reg = <0x0 0xa2450000 0x0 0x2000>;
 *     };
 */
#define SEL4_CRASHLOG_PADDR     0xA2450000
#define SEL4_CRASHLOG_LEN       0x2000

#endif /* _REE_COMM_DEFS_H_ */