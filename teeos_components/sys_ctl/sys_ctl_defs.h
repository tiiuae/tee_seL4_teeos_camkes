/*
 * Copyright 2022, Unikie
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#ifndef _SYS_CTL_DEFS_H_
#define _SYS_CTL_DEFS_H_

/*
 *  mailbox@37020800 {
 *       compatible = "microchip,mpfs-mailbox";
 *       reg = <0x0 0x37020000 0x0 0x2000>;
 *       interrupt-parent = <&L1>;
 *       interrupts = <96>;
 *       #mbox-cells = <1>;
 *   };
 */

#define SYS_CTL_REG_PADDR       0x37020000
#define SYS_CTL_REG_LEN         0x2000
#define SYS_CTL_MB_OFFSET       0x800 /* mailbox offset */

#endif /* _SYS_CTL_DEFS_H_ */
