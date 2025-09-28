/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPSS_BE_MTR_H_
#define _ISPSS_BE_MTR_H_

#define ISPSS_BE_MTR_DEBUG_EN (1)
#define ISPSS_BE_MTR_BASE_ADDR_ALIGN (12)

#define ISP_BE_MTR_BASE_ADDR  (MEMMAP_ISP_REG_BASE + ISPSS_MEMMAP_MTR_REG_BASE)
enum ISPSS_ERROR_E {
	ISPSS_BE_MTR_SUCCESS = 0,
	ISPSS_BE_MTR_ERROR,
	ISPSS_BE_MTR_MAX
};

#ifdef ISPSS_BE_MTR_DEBUG_EN
#define ISPSS_BE_MTR_CHECK_RETVAL(cmd) \
	do { \
		uiRetVal = cmd; \
		if (uiRetVal != ISPSS_BE_MTR_SUCCESS) \
			return uiRetVal; \
	} while (0)
#else
#define ISPSS_BE_MTR_CHECK_RETVAL(cmd) cmd
#endif

/* Definitions for ISPSS BE MTR PATHS  */
enum ISPSS_BE_MTR_PATHS {
	ISPSS_BE_DNSCL_RD = 0,
	ISPSS_BE_ROTATE_RD,
	ISPSS_BE_DEWARP_RD,
	ISPSS_BE_DNSCL_WR,
	ISPSS_BE_TILE_WR,
	ISPSS_BE_ROTATE_WR,
	ISPSS_BE_DEWARP_WR,
	ISPSS_BE_MTR_PATH_MAX
};

/* Definitions for AVIO MTR thread IDs */
enum ISPSS_BE_MTR_THREAD_ID {
	ISPSS_BE_MTR_THREAD_INVALID  = -1,
	ISPSS_BE_MTR_THREAD_MIN      = 0,
	ISPSS_BE_MTR_THREAD_0        = ISPSS_BE_MTR_THREAD_MIN,
	ISPSS_BE_MTR_THREAD_1,
	ISPSS_BE_MTR_THREAD_2,
	ISPSS_BE_MTR_THREAD_3,
	ISPSS_BE_MTR_THREAD_4,
	ISPSS_BE_MTR_THREAD_5,
	ISPSS_BE_MTR_THREAD_6,
	ISPSS_BE_MTR_THREAD_7,
	ISPSS_BE_MTR_THREAD_8,
	ISPSS_BE_MTR_THREAD_9,
	ISPSS_BE_MTR_THREAD_10,
	ISPSS_BE_MTR_THREAD_11,
	ISPSS_BE_MTR_THREAD_12,
	ISPSS_BE_MTR_THREAD_13,
	ISPSS_BE_MTR_THREAD_14,
	ISPSS_BE_MTR_THREAD_15,
	ISPSS_BE_MTR_THREAD_MAX
};

struct ISPSS_BE_MTR_MTRR_CFG {
	UINT8  ucfg_stride;
	UINT8  ucfg_format;
	UINT8  ucfg_enable;
	UINT8  ucfg_mode;
	BOOL   ucfg_hflip;
	BOOL   ucfg_vflip;
	UINT32 ubase_addr;
};

struct ISPSS_BE_MTR_MMU_CFG {
	UINT8  upbm_shy_bw;
	UINT8  upbm_shy_pos;
	UINT32 upbm_stride_64B;
	UINT8  upbm_tileMode;
	UINT8  upbm_pm_enable;
	UINT8  upbm_shuffle_en;
	UINT8  upbm_bm_enable;
	UINT8  uvm_enable;
	UINT64 uvm_base;
};

struct ISPSS_BE_MTR_MTRR_Prog {
	struct ISPSS_BE_MTR_MTRR_CFG ispmtrMtrrCfg;
	struct ISPSS_BE_MTR_MTRR_CFG ispmtrMtrwCfg;
	struct ISPSS_BE_MTR_MMU_CFG ispmtrMmuCfg;
};

struct ISPSS_BE_MTR_BUFF_DESC {
	UINT8  m_MtrrCfgCfg_stride;
	UINT8  m_MtrrCfgCfg_format;
	UINT8  m_MtrrCfgCfg_mode;
	UINT32 m_MtrrCfgBase_addr;
	UINT8  m_MmuCfgPbm_shy_bw;
	UINT8  m_MmuCfgPbm_shy_pos;
	UINT8  m_MmuCfgPbm_pm_enable;
	UINT8  m_MmuCfgPbm_shuffle_en;
	UINT8  m_MmuCfgPbm_bm_enable;
	UINT32 m_MmuCfgVm_enable;
};

struct ISPSS_BE_MTR {
	UINT32 uiIspmtrKickoff;   //MTR kickoff
	UINT8 m_mtrR_mode;
	UINT8 m_mtrR_chkSemId_Y;
	UINT8 m_mtrR_chkSemId_UV;
	UINT8 m_mtrR_updSemId_Y;
	UINT8 m_mtrR_updSemId_UV;
	struct ISPSS_BE_MTR_BUFF_DESC m_mtr_buf_desc_Y;
	struct ISPSS_BE_MTR_BUFF_DESC m_mtr_buf_desc_UV;
	UINT32 m_buf_stride;
	UINT32 m_buf_stride_UV;
	VOID   *m_pbuf_start;
	UINT32 m_buf_pbuf_start_UV;
	UINT32 m_bits_per_pixel;
	UINT32  m_active_width;
	UINT32  m_active_height;
	UINT32  m_content_width;
	UINT32  m_content_height;
	UINT32  m_srcfmt;
	UINT32  m_path;
	UINT32  mtrMode;
};

UINT32 ISPSS_BE_MTR_kickOff(struct BCMBUF *pbcmbuf, UINT32 kickOff);
void ISPSS_BE_MTR_UpdateDesc(struct BCMBUF *pbcmbuf,
		struct ISPSS_BE_MTR *psIspmtr, int path);
UINT32 ISPSS_BE_MTR_Set_Global_Registers(int path, int enable);
UINT32 ISPSS_BE_MTR_Set_Global_Registers_bcm(struct BCMBUF *pbcmbuf, int path);
UINT32 ISPSS_BE_MTR_Set_Dhub_Channel_Scaling(struct BCMBUF *pbcmbuf,
		UINT8 in_mtrMode, UINT8 out_mtrMode);

#endif

