/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPSS_MTR_H_
#define _ISPSS_MTR_H_

#include "ispSS_bcmbuf.h"

#define GET_BIT_MASK(N_BIT)     ((1<<N_BIT) - 1)
#define GET_BIT(VALUE, BIT_POS, N_BIT) ((VALUE & (GET_BIT_MASK(N_BIT) << BIT_POS)) >> BIT_POS)
#define SET_BIT(VARIABLE, VALUE, BIT_POS, N_BIT) (VARIABLE = (VARIABLE & \
			(~(GET_BIT_MASK(N_BIT) << BIT_POS))) | \
			((VALUE & GET_BIT_MASK(N_BIT)) << BIT_POS))

#define ISPSS_MTR_TRACE(f_, ...) ISP_INFO((f_), ##__VA_ARGS__)

#define ISP_MTR_REG_BASE ISPSS_MEMMAP_MTR_REG_BASE
#define ISPSS_MTR_DEBUG_EN         (1)
#define ISPSS_MTR_PBM_STRIDE_ALIGN (6)
#define ISPSS_MTR_TILEHEIGHT_ALIGN (2)
#define ISPSS_MTR_BASE_ADDR_ALIGN  (12)
#define ISPSS_MTR_DHUB_ADDR        (0)
#define ISPSS_MTR_DHUB_STRIDE      (0x4000)
// Doc - ispSS_Specification_a0_20200924.pdf
#define ISP_WR4_MCM_S0_W_BIT       0x3
#define ISP_WR4_MCM_S1_W_BIT       0x5
#define ISP_RD2_MCM_RAW_R_BIT      0x3
#define ISP_RD1_TDNR_RAW_R_BIT     0x9
#define ISP_WR3_TDNR_RAW_W_BIT     0x9
#define ISP_WR3_SP2_Y_W_BIT        0x3
#define ISP_WR3_SP2_CB_W_BIT       0x5
#define ISP_WR1_MP_Y_W_BIT         0x2
#define ISP_WR1_MP_CB_W_BIT        0x4

#ifdef ISPSS_MTR_DEBUG_EN
#define ISPSS_MTR_CHECK_RETVAL(cmd)\
	do {\
		uiRetVal = cmd;\
		if (uiRetVal != ISPSS_MTR_SUCCESS)\
		return uiRetVal;\
	} while (0)
#else
#define ISPSS_MTR_CHECK_RETVAL(cmd) cmd
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

enum ISPSS_ERROR {
	ISPSS_MTR_SUCCESS = 0,
	ISPSS_MTR_PARAM_ERROR,
	ISPSS_MTR_ERROR,
	ISPSS_MTR_MAX
};

enum ISP_SRC_FMT {
	ISP_SRCFMT_YUV422SP      = 0,
	ISP_SRCFMT_YUV420SP,
	ISP_SRCFMT_YUV422SP_DWA,
	ISP_SRCFMT_YUV420SP_DWA,
	ISP_SRCFMT_YUV422PACK,
	ISP_SRCFMT_YUV420SP_V8H8,
	ISP_SRCFMT_YUV420SP_V8H6,
	ISP_SRCFMT_YUV422SP_V8H8,
	ISP_SRCFMT_YUV422SP_V8H6,
	ISP_SRCFMT_YUV_MAX = ISP_SRCFMT_YUV422SP_V8H6,
	ISP_SRCFMT_YUV444PACK, // interleaved data can be treated as single component
	ISP_SRCFMT_RGB888PACK,
	ISP_SRCFMT_BAYER_10BIT,
	ISP_SRCFMT_BAYER_10BIT_DWA,
	ISP_SRCFMT_BAYER_12BIT,
	ISP_SRCFMT_BAYER_12BIT_DWA,
	ISP_SRCFMT_BAYER_16BIT,
	ISP_SRCFMT_BAYER_8BIT,
	ISP_SRCFMT_MAX
};

/* Definitions for AVIO MTR thread IDs */
enum ISPSS_MTR_THREAD_ID {
	ISPSS_MTR_THREAD_INVALID  = -1,
	ISPSS_MTR_THREAD_MIN      = 0,
	ISPSS_MTR_THREAD_0        = ISPSS_MTR_THREAD_MIN,
	ISPSS_MTR_THREAD_1,
	ISPSS_MTR_THREAD_2,
	ISPSS_MTR_THREAD_3,
	ISPSS_MTR_THREAD_4,
	ISPSS_MTR_THREAD_5,
	ISPSS_MTR_THREAD_6,
	ISPSS_MTR_THREAD_7,
	ISPSS_MTR_THREAD_8,
	ISPSS_MTR_THREAD_9,
	ISPSS_MTR_THREAD_10,
	ISPSS_MTR_THREAD_11,
	ISPSS_MTR_THREAD_12,
	ISPSS_MTR_THREAD_13,
	ISPSS_MTR_THREAD_14,
	ISPSS_MTR_THREAD_15,
	ISPSS_MTR_THREAD_MAX
};

/* definition of VPP input planes */
enum ISPSS_MTR_PATH_ID {
	ISPSS_MTR_PATH_INVALID    = -1,
	ISPSS_MTR_FIRST_PATH      = 0,
	ISPSS_MTR_PATH_MCM0_RD    = 0,
	ISPSS_MTR_PATH_MCM1_RD    = 1,
	ISPSS_MTR_PATH_TDNR0_RD   = 2,
	ISPSS_MTR_PATH_TDNR1_RD   = 3,
	ISPSS_MTR_PATH_TILE_RD    = 4,
	ISPSS_MTR_PATH_DWNSCLR_RD = 5,
	ISPSS_MTR_PATH_MCM0_WR    = 6,
	ISPSS_MTR_PATH_MCM1_WR    = 7,
	ISPSS_MTR_PATH_TDNR0_WR   = 8,
	ISPSS_MTR_PATH_TDNR1_WR   = 9,
	ISPSS_MTR_PATH_MP0_WR     = 10,
	ISPSS_MTR_PATH_MP1_WR     = 11,
	ISPSS_MTR_PATH_SP2_WR0    = 12,
	ISPSS_MTR_PATH_SP2_WR1    = 13,
	ISPSS_MTR_MAX_NUM_PATHS
};

enum ISPSS_MTR_COMPR_MODE {
	ISPSS_MTR_MODE_INVALID,
	ISPSS_MTR_MODE_8B_V4_Y = 0,
	ISPSS_MTR_MODE_8B_V4_UV = 1,
	ISPSS_MTR_MODE_8B_V1_Y = 4,
	ISPSS_MTR_MODE_8B_V1_UV = 5,
	ISPSS_MTR_MODE_8B_V1_ARGB = 7,
	ISPSS_MTR_MODE_P10B_V4_Y = 8,
	ISPSS_MTR_MODE_P10B_V4_UV = 9,
	ISPSS_MTR_MODE_D10B_V1_Y = 12,
	ISPSS_MTR_MODE_D10B_V1_UV = 13,
	ISPSS_MTR_MODE_P10B_V1_ARGB = 15,
	ISPSS_MTR_MODE_R10B_V1_RAW = 16,
	ISPSS_MTR_MODE_R12B_V1_RAW = 17,
	ISPSS_MTR_MODE_R16B_V1_RAW = 18,
	ISPSS_MTR_MODE_R20B_V1_RAW = 19,
	ISPSS_MTR_MAX_MODES,
};

struct ISPSS_MTR_MTRR_CFG {
	UINT8  ucfg_stride;
	UINT8  ucfg_format;
	UINT8  ucfg_enable;
	UINT8  ucfg_mode;
	BOOL   ucfg_hflip;
	BOOL   ucfg_vflip;
	UINT32 ubase_addr;

};

struct ISPSS_MTR_MMU_CFG {
	UINT8  upbm_shy_bw;
	UINT8  upbm_shy_pos;
	UINT32 upbm_stride_64B;
	UINT8  upbm_tileMode;
	UINT8  upbm_pm_enable;
	UINT8  upbm_shuffle_en;
	UINT8  upbm_bm_enable;

	UINT8  uvm_enable;
	UINT32 uvm_base;

};

struct ISPSS_MTR_MTRR_Prog {
	struct ISPSS_MTR_MTRR_CFG ispSSMtrrCfg;
	struct ISPSS_MTR_MMU_CFG ispSSMtrMmuCfg;
};

struct ISP_MTR_OBJ {
	INT path;
	UINT base_addr_ispMtr;
	UINT32 uiIspMtrKickoff;
};

struct ISP_MTR_BUFF_DESC {
	UINT8  m_MtrrCfgCfg_stride;
	UINT8  m_MtrrCfgCfg_format;
	UINT8  m_MtrrCfgCfg_mode;

	UINT32 m_MtrrCfgBase_addr;

	UINT8  m_MmuCfgPbm_shy_bw;
	UINT8  m_MmuCfgPbm_shy_pos;
	UINT8  m_MmuCfgPbm_pm_enable;
	UINT8  m_MmuCfgPbm_shuffle_en;
	UINT8  m_MmuCfgPbm_bm_enable;

	UINT8  m_MmuCfgVm_enable;
};

struct ISP_MTR_BUF_INFO {
	struct ISP_MTR_BUFF_DESC m_mtr_buf_desc_Y;
	struct ISP_MTR_BUFF_DESC m_mtr_buf_desc_UV;
	struct BCMBUF *pBcmBuf;
	enum ISP_SRC_FMT m_srcfmt;
	UINT32 m_is_compressed;
	UINT32 m_buf_stride;
	UINT32 m_pbuf_start;
	UINT32 leftCrop;
	UINT32 topCrop;
	UINT32 m_active_width;
	UINT32 m_active_height;
	UINT32 m_buf_stride_UV;
	UINT32 m_buf_pbuf_start_UV;
	UINT32 m_bits_per_pixel;
};

struct ISPSS_MTR_CONFIG_PARAM {
	struct ISP_MTR_OBJ ispMtrObj;
	struct ISP_MTR_BUF_INFO ispMtrInfo;
	void *dumpPTVirtPtr;
};

INT32 ISPSS_MTR_Set_Global_Registers(struct ISP_MTR_OBJ *mtrObj, enum ISPSS_MTR_PATH_ID path,
		bool enable);
UINT32 ISPSS_MTR_Reset(struct ISP_MTR_OBJ *mtrObj, struct BCMBUF *pbcmbuf);
UINT32 ISPSS_MTR_Config(struct ISP_MTR_OBJ *mtrObj, int config, struct BCMBUF *pbcmbuf);
UINT32 ISPSS_MTR_kickOff(struct ISP_MTR_OBJ *mtrObj, struct ISP_MTR_BUF_INFO *pIspBufInfo);
UINT32 ISPSS_MTR_UpdateDesc(struct ISP_MTR_OBJ *mtrObj, struct ISP_MTR_BUF_INFO *pIspBufInfo,
		enum ISPSS_MTR_PATH_ID path);
UINT32 ISPSS_MTR_UpdateReadDesc(struct ISP_MTR_OBJ *mtrObj,
		struct ISP_MTR_BUF_INFO *pIspBufInfo);
UINT32 ISPSS_MTR_UpdateWriteDesc(struct ISP_MTR_OBJ *mtrObj,
		struct ISP_MTR_BUF_INFO *pIspBufInfo);
UINT32 ISPSS_MTR_PrintRegisters(struct ISP_MTR_OBJ *mtrObj);
UINT32 ISPSS_MTR_GetStride(struct ISP_MTR_BUF_INFO *pIspBufInfo, UINT32 *stride_64B);
#endif //_ISPSS_MTR_H_
