/*
 * Copyright 2022, Unikie
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#ifndef _REE_COMM_DEFS_H_
#define _REE_COMM_DEFS_H_

#include "miv_ihc_add_mapping.h"

/* RPMSG vring reservation
 *
 *     rpmsg@a2400000 {
 *         reg = <0x0 0xa2400000 0x0 0x50000>; // rpmsg + rpmsg_dma_reserved
 *     };
 */
#define RPMSG_BUFFER_PADDR  0xA2400000
#define RPMSG_BUFFER_LEN    0x50000

#define IHC_REG_BASE_ADDR       IHC_LOCAL_H0_REMOTE_H1
/* Aligned up to page size: IHC reg cluster * 5 = 0x1900 => 0x2000 */
#define IHC_REG_LEN             0x2000

#endif /* _REE_COMM_DEFS_H_ */