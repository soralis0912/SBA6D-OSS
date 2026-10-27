/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2017 MediaTek Inc.
 */

#ifndef __NFIECC_REGS_H__
#define __NFIECC_REGS_H__

#define NFIECC_ENCCON           0x000
/* NFIECC_DECCON has same bit define */
#define         ECC_OP_EN               BIT(0)
#define NFIECC_ENCCNFG          0x004
#define         ENCCNFG_MS_SHIFT        16
#define         ENC_BURST_EN            BIT(8)
#define NFIECC_ENCDIADDR        0x008
#define NFIECC_ENCIDLE          0x00c
#define NFIECC_ENCSTA           0x07c
#define         ENC_FSM_IDLE            1
#define NFIECC_ENCIRQEN         0x080
/* NFIECC_DECIRQEN has same bit define */
#define         ECC_IRQEN               BIT(0)
#define         ECC_PG_IRQ_SEL          BIT(1)
#define NFIECC_ENCIRQSTA        0x084
#define         ENC_IRQSTA_GEN          BIT(0)
#define NFIECC_PIO_DIRDY        0x090
#define         PIO_DI_RDY              BIT(0)
#define NFIECC_PIO_DI           0x094
#define NFIECC_DECCON           0x100
#define NFIECC_DECCNFG          0x104
#define         DEC_BURST_EN            BIT(8)
#define         DEC_EMPTY_EN            BIT(31)
#define         DEC_CON_SHIFT           12
#define         DECCNFG_MS_SHIFT        16
#define NFIECC_DECDIADDR        0x108
#define NFIECC_DECIDLE          0x10c
#define NFIECC_DECFER           0x110
#define NFIECC_DECENUM(x)       (0x114 + (x) * 4)
#define NFIECC_DECDONE          0x124
#define NFIECC_DECIRQEN         0x200
#define NFIECC_DECIRQSTA        0x204
#define NFIECC_DECFSM           0x208
#define         FSM_MASK                0x3f3fff0f
#define         FSM_IDLE                0x01011101
#define NFIECC_BYPASS           0x20c
#define         NFIECC_BYPASS_EN        BIT(0)
#define NFIECC_DEBUG2           0x21c
#define NFIECC_ENCPAR(x)        (0x300 + (x) * 4)
#define NFIECC_DECEL(x)         (0x500 + (x) * 4)

#endif /* __NFIECC_REGS_H__ */
