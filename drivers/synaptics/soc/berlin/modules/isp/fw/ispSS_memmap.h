/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef ispSS_memmap_h
#define ispSS_memmap_h (){}
#pragma pack(1)
#ifdef __cplusplus
extern "C" {
#endif
#define        MEMMAP_ISP_REG_BASE                         0xF9100000
#ifndef _DOCC_H_BITOPS_
#define _DOCC_H_BITOPS_ (){}
#define _bSETMASK_(b)                                      ((b)<32 ? (1<<((b)&31)) : 0)
#define _NSETMASK_(msb,lsb)                                (_bSETMASK_((msb)+1)-_bSETMASK_(lsb))
#define _bCLRMASK_(b)                                      (~_bSETMASK_(b))
#define _NCLRMASK_(msb,lsb)                                (~_NSETMASK_(msb,lsb))
#define _BFGET_(r,msb,lsb)                                 (_NSETMASK_((msb)-(lsb),0)&((r)>>(lsb)))
#define _BFSET_(r,msb,lsb,v)                               do { (r)&=_NCLRMASK_(msb,lsb); (r)|=_NSETMASK_(msb,lsb)&((v)<<(lsb)); } while(0)
#endif
#ifndef h_ISPSS_MEMMAP
#define h_ISPSS_MEMMAP (){}
#define        ISPSS_MEMMAP_TSB_DHUB_REG_BASE              0x0
#define        ISPSS_MEMMAP_TSB_DHUB_REG_SIZE              0x20000
#define        ISPSS_MEMMAP_TSB_DHUB_REG_DEC_BIT           0xF
#define        ISPSS_MEMMAP_FWR_DHUB_REG_BASE              0x20000
#define        ISPSS_MEMMAP_FWR_DHUB_REG_SIZE              0x20000
#define        ISPSS_MEMMAP_FWR_DHUB_REG_DEC_BIT           0x11
#define        ISPSS_MEMMAP_GLB_REG_BASE                   0x40000
#define        ISPSS_MEMMAP_GLB_REG_SIZE                   0x10000
#define        ISPSS_MEMMAP_GLB_REG_DEC_BIT                0xA
#define        ISPSS_MEMMAP_MIPI_REG_BASE                  0x50000
#define        ISPSS_MEMMAP_MIPI_REG_SIZE                  0x10000
#define        ISPSS_MEMMAP_MIPI_REG_DEC_BIT               0x10
#define        ISPSS_MEMMAP_ISP_REG_BASE                   0x60000
#define        ISPSS_MEMMAP_ISP_REG_SIZE                   0x10000
#define        ISPSS_MEMMAP_ISP_REG_DEC_BIT                0x10
#define        ISPSS_MEMMAP_BCM_REG_BASE                   0x70000
#define        ISPSS_MEMMAP_BCM_REG_SIZE                   0x10000
#define        ISPSS_MEMMAP_BCM_REG_DEC_BIT                0x9
#define        ISPSS_MEMMAP_TILE_REG_BASE                  0x80000
#define        ISPSS_MEMMAP_TILE_REG_SIZE                  0x10000
#define        ISPSS_MEMMAP_TILE_REG_DEC_BIT               0xC
#define        ISPSS_MEMMAP_ROT_REG_BASE                   0x90000
#define        ISPSS_MEMMAP_ROT_REG_SIZE                   0x10000
#define        ISPSS_MEMMAP_ROT_REG_DEC_BIT                0xC
#define        ISPSS_MEMMAP_DNSCL_REG_BASE                 0xA0000
#define        ISPSS_MEMMAP_DNSCL_REG_SIZE                 0x10000
#define        ISPSS_MEMMAP_DNSCL_REG_DEC_BIT              0xA
#define        ISPSS_MEMMAP_FACEDET_REG_BASE               0xB0000
#define        ISPSS_MEMMAP_FACEDET_REG_SIZE               0x10000
#define        ISPSS_MEMMAP_FACEDET_REG_DEC_BIT            0x8
#define        ISPSS_MEMMAP_DEWARP_REG_BASE                0xC0000
#define        ISPSS_MEMMAP_DEWARP_REG_SIZE                0x10000
#define        ISPSS_MEMMAP_DEWARP_REG_DEC_BIT             0xC
#define        ISPSS_MEMMAP_MTR_REG_BASE                   0xD0000
#define        ISPSS_MEMMAP_MTR_REG_SIZE                   0x2000
#define        ISPSS_MEMMAP_MTR_REG_DEC_BIT                0xB
#define        ISPSS_MEMMAP_ALM_REG_BASE                   0xD2000
#define        ISPSS_MEMMAP_ALM_REG_SIZE                   0x2000
#define        ISPSS_MEMMAP_ALM_REG_DEC_BIT                0xA
#define        ISPSS_MEMMAP_ISPSS_REG_SIZE                 0x100000
#define        ISPSS_MEMMAP_ISPSS_REG_DEC_BIT              0x14
#endif
#ifdef __cplusplus
}
#endif
#pragma  pack()
#endif
