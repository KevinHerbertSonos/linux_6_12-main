/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPSS_BE_SCL_MODULE_H_
#define _ISPSS_BE_SCL_MODULE_H_

#include "ovpdnSclWrap.h"
#include "ispSS_fe_client.h"
#include "ispSS_rqstq.h"
#include "ispBE_module_common_priv.h"
#include "ispSS_be_Mtr.h"
#include "ispBE_common.h"
#include "ispBE_module_common.h"
#define T_COEFF_MAX 14 //Update this value if changes made to ISP_FRC_SCL_COEFF_MODE_T

#define SCL_COMMIT_QUEUE_ID_12    (12)
#define SCL_COMMIT_QUEUE_ID_13    (13)

/*Maximum SCL Objects */
#define MAX_SCL_OBJECTS 3

#define MAX_SCL_CLIENTS 1

/* ISP SCL Coeff Macros */
#define ISP_FRC_SCL_MAIN_LAY_MAX 2
#define ISP_FRC_SCL_LAY_MAX      2 /* Max among all the scalers */
#define ISP_FRC_SCL_NUM_OF_PHASES 17
#define ISP_FRC_SCL_NUM_OF_COEFF  8
#define ISP_FRC_SCL_COEFF_TAB_SIZE\
	(ISP_FRC_SCL_NUM_OF_PHASES * ISP_FRC_SCL_NUM_OF_COEFF * sizeof(UINT16))

/* size if calculated as 7 values, 7 address and sizeof(UINT32) for each of them */
#define ISP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS_PERPHASE (7*1)
#define ISP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS\
	(ISP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS_PERPHASE * ISP_FRC_SCL_NUM_OF_PHASES)
#define ISP_FRC_COEFFTAB_BCMBUF_SIZE_PERPHASE\
	(ISP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS_PERPHASE*sizeof(UINT64))
#define ISP_FRC_COEFFTAB_BCMBUF_SIZE\
	(ISP_FRC_COEFFTAB_BCMBUF_SIZE_PERPHASE * ISP_FRC_SCL_NUM_OF_PHASES)

#define BE_SCL_LOGD(...) pr_debug(__VA_ARGS__)
#define BE_SCL_LOGE(...) pr_err(__VA_ARGS__)

#define IS_SRC_FMT_PACKED(srcfmt)\
	((srcfmt == ISPSS_SRCFMT_YUV444P)\
	 || (srcfmt == ISPSS_SRCFMT_RGB444)\
	 || (srcfmt == ISPSS_SRCFMT_YUV422P)\
	 || (srcfmt == ISPSS_SRCFMT_RGB888))

#define IS_SRC_FMT_SP(srcfmt)\
	((srcfmt == ISPSS_SRCFMT_YUV420SP)\
	 || (srcfmt == ISPSS_SRCFMT_YUV422SP)\
	 || (srcfmt == ISPSS_SRCFMT_YUV420SP_DWA))

#define IS_UPSAMPLING_ENABLE(inSrcfmt, outSrcfmt)\
	((IS_SRC_FMT_SP(inSrcfmt)) && (IS_SRC_FMT_PACKED(outSrcfmt)))

typedef int (*ISPSS_INTR_HANDLER)(int intr, void *pArgs);

enum ISPSS_DNSCL_BYPASSPATH {
	DISABLE_BYPASSPATH = 0,
	ENABLE_COMPLETE_BYPASSPATH,
	ENABLE_ONLY_1D_BYPASSPATH
};

enum ISPSS_BE_SCL_OBJ_STATUS_T {
	ISPSS_BE_SCL_OBJ_STATUS_INACTIVE,
	ISPSS_BE_SCL_OBJ_STATUS_ACTIVE,
};

enum ISPSS_BE_SCL_HW_STATUS_T {
	ISPSS_BE_SCL_HW_STATUS_IDLE,
	ISPSS_BE_SCL_HW_STATUS_RUNNING,
};

/* definition of HV scaling coeff selection */
enum {
	MODE_COEFF_SEL_AUTO      = 0,
	MODE_COEFF_SEL_FORCE_H   = 1,
	MODE_COEFF_SEL_FORCE_V   = 2,
	MODE_COEFF_SEL_FORCE_HV  = 3,
};

/*Enum for horizontal scaler position*/
enum ISP_SCL_HSCALER_POS {
	ISP_SCL_HSCALE_ABSENT = 0,
	ISP_SCL_HSCALE_BEFORE = 2,
	ISP_SCL_HSCALE_AFTER,
	ISP_SCL_HSCALE_AUTO
};

/* definition of all the horizontal and vertical scaling coefficient modes */
enum ENUM_SCALAR_COEFF_MODE {
	SCALAR_COEFF_INVALID                            = -1,
	FIRST_SCALAR_COEFF                              = 0,
	SCALAR_COEFF_MAIN_MIN                           = 0,
	SCALAR_COEFF_H_MAIN_ADP_HLUT_CS                 = 0,
	SCALAR_COEFF_H_MAIN_1D_HLUT_BLK5                = 1,
	SCALAR_COEFF_H_MAIN_1D_HLUT_PH8                 = 2,
	SCALAR_COEFF_H_MAIN_1D_HLUT_PH8_SOFT            = 3,
	SCALAR_COEFF_V_MAIN_ADP_HLUT_BS                 = 4,
	SCALAR_COEFF_V_MAIN_1D_VLUT_DBG1                = 5,
	SCALAR_COEFF_V_MAIN_1D_VLUT_2TAP                = 6,
	SCALAR_COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL    = 7,
	SCALAR_COEFF_V_MAIN_1D_VLUT_3TAP_MED            = 8,
	SCALAR_COEFF_V_MAIN_1D_VLUT_3TAP_SOFT           = 9,
	SCALAR_COEFF_V_MAIN_1D_VLUT_BLK5                = 10,
	SCALAR_COEFF_V_MAIN_1D_VLUT_6TAP_SOFT           = 11,
	SCALAR_COEFF_V_MAIN_1D_VLUT_6TAP_SSOFT          = 12,
	SCALAR_COEFF_V_MAIN_1D_VLUT_6TAP                = 13,
	SCALAR_COEFF_MAIN_MAX                           = 13,
	SCALAR_COEFF_GFX_MIN                            = 14,
	SCALAR_COEFF_H_GFX_HLUT_BLK5_GFX                = 14,
	SCALAR_COEFF_H_GFX_HLUT_PV8                     = 15,
	SCALAR_COEFF_V_GFX_VLUT_CS_4TAP                 = 16,
	SCALAR_COEFF_V_GFX_VLUT_3TAP_CUBIC              = 17,
	SCALAR_COEFF_V_GFX_VLUT_1TAP                    = 18,
	SCALAR_COEFF_GFX_MAX                            = 18,
	MAX_NUM_PREDEFINED_COEFFS                       = 19,
	SCALAR_COEFF_AUTO                               = 19,
	MAX_NUM_SCALAR_COEFF_MODES
};

/*Enum for FRC SCL unit number*/
enum ISP_FRC_SCL_NUM {
	ISP_FRC_SCL_OVP_Y = 0,
	ISP_FRC_SCL_OVP_UV,
	ISP_FRC_SCL_MAX
};

struct ISP_VID_BUF_DESC {
	void           *user_data; // User-defined data, not used by the video decoder.
	void           *internal_data; // Reserved. For video decoder internal use only.
	unsigned int    rsvd1; // Reserved. For video decoder internal use only.
	unsigned int    base_addr; // Base address of the frame buffer. (Physical address)
	void           *base_addr_virtual; // Base address of the frame buffer. (Virtual Address)
	unsigned int    size; // buffer size
	unsigned int    rsvd4;// Reserved. For video decoder internal use only.
	/* Buffer ID assigned by the buffer allocator, not used by the video decoder. */
	unsigned int    id;
	unsigned int    sn; // Reserved. For video decoder internal use only.
	unsigned int    rsvd5; // Reserved. For video decoder internal use only.
	void           *rsvd6; // Reserved. For video decoder internal use only.
	unsigned int    tctx_addr; // Reserved. For video decoder internal use only.
	//vdec_pic_desc_t pic[2]; // Picture descriptor pair. [0]: frame/top, [1]: bottom.
};

struct ISP_BE_SCL_WIN {
	/* x-coordination of a ISP window top-left
	 * corner in pixel, index starting from 0
	 */
	int x;
	/* y-coordination of a ISP window top-left
	 * corner in pixle, index starting from 0
	 */
	int y;
	int width;  /* width of a ISP window in pixel */
	int height; /* height of a ISP window in pixel */
};

/*scaler input and output resolution*/
struct ISP_BE_SCL_RES {
	UINT32 IVRes;
	UINT32 IHRes;
	UINT32 OVRes;
	UINT32 OHRes;
};

/*data loader crop*/
struct ISPSS_BE_SCL_INPUT_CROP {
	UINT32 leftCrop;
	UINT32 rightCrop;
	UINT32 topCrop;
	UINT32 bottomCrop;
};

/*data structure for scaler control*/
struct ISP_BE_SCL_CTRL {
	UINT8  HScalePos;  //vertical scaler position, usually set to auto
	UINT8  InputClr;   //input color space
	UINT8  BitMode;    //input and out put bit mode
	UINT8  I565;       //whether 18 bits 656 input
	UINT8  NLEn;
	UINT8  HTapNum;
	UINT8  VTapNum;

	/* 1: dynamically load coefficient table,
	 * 0: don't load coefficient table dynamically.
	 */
	UINT8  DynamicLoad;
	/* 0: secect coefficients by scaling ratio.
	 * 1: force selection scaler H coeff,
	 * 2: force selection scaler V coeff,
	 * 3: force selection both
	 */
	UINT8  ForceSel;
	INT  HSclMode; //if set ForceSel to 1, HSclMode should be set
	INT  VSclMode; //if set ForceSel to 1, VSclMode should be set


	//cropping related variables
	UINT8  leftCrop;        //for odd start x
	UINT8  rightCrop;       //for odd width
	//complete/uncropped resolution input to SCL - used for lsize calculation
	UINT32 uncrop_IHRes;
	UINT8  uiUVswapEn;

	/* Crop after Scaling */
	UINT8 cropAfterSclLeft;
	UINT8 cropAfterSclRight;
	UINT8 cropAfterSclTop;
	UINT8 cropAfterSclBottom;

};
/************************************************/
/*********** ISP BE SCL HW data structures *********/
/************************************************/
struct ISP_BE_SCL_HW_STRUCT {

	SHM_HANDLE scl_coeffs_handle[MAX_NUM_PREDEFINED_COEFFS];
	UINT64 *scl_coeffs[MAX_NUM_PREDEFINED_COEFFS];
	//For customized SCL coeff
	SHM_HANDLE cust_scl_coeffs_handle[ISP_FRC_SCL_LAY_MAX][MAX_NUM_PREDEFINED_COEFFS];
	SHM_HANDLE hw_handle;
	unsigned short *cust_scl_coeffs[ISP_FRC_SCL_LAY_MAX][MAX_NUM_PREDEFINED_COEFFS];
	BOOL    cust_scl_id_flag[ISP_FRC_SCL_LAY_MAX][MAX_NUM_PREDEFINED_COEFFS];
	BOOL   cust_sc_bPrepared[ISP_FRC_SCL_LAY_MAX][MAX_NUM_PREDEFINED_COEFFS];
	BOOL   cust_sc_bLoaded[ISP_FRC_SCL_LAY_MAX][MAX_NUM_PREDEFINED_COEFFS];

	/* SCL shadow registers */
	UINT32 uiSclFrcCfg0[ISP_FRC_SCL_MAX];
	UINT32 uiSclFrcCfg1[ISP_FRC_SCL_MAX];
	UINT32 uiSclCfg0[ISP_FRC_SCL_MAX];
	UINT32 uiSclCfg3[ISP_FRC_SCL_MAX];
	UINT32 uiSclCfg6[ISP_FRC_SCL_MAX];
	UINT32 uiSclCfg7[ISP_FRC_SCL_MAX];
	UINT32 uiSclCfg8[ISP_FRC_SCL_MAX];
	UINT32 uiSclCfg15[ISP_FRC_SCL_MAX];
	UINT32 uiSclCfg18[ISP_FRC_SCL_MAX];
	T32OVPSCL_CTRL ovp_y_scl_ctrl;
	T32OVPSCL_CTRL ovp_uv_scl_ctrl;
	T32OVPDNSCLWRAP_CTRL ovpsclwrap_ctrl;
	T32OVPDNSCLWRAP_CFG0 ovp_cfg0;
	T32OVPDNSCLWRAP_tg_ctrl ovp_tg_ctrl;
	T32OVPDNSCLWRAP_CFG2 ovp_cfg2;
	T32OVPDNSCLWRAP_CFG8 ovp_cfg8;
	T32OVPDNSCLWRAP_CFG9 ovp_cfg9;
	T32DITHER_CFG0 ovp_dither;
	T32OVPDNSCLWRAP_sclOFIFO_YInpix ovp_sclOFIFO_Y;
	T32OVPDNSCLWRAP_sclOFIFO_UVInpix ovp_sclOFIFO_UV;
	T32OVPDNSCLWRAP_INTR_EN ovp_IntrEn;
	T32OVPDNSCLWRAP_CFG1 ovp_cfg1;

	INT dmaRID[ISP_CHANNEL_MAX];         //  inline read DMA channel ID
	UINT64 dmaDhubID;     // ID of Dhub on which inline DMA channel is connected
	INT dmaWID[ISP_CHANNEL_MAX];  //  offline write-back DMA channel ID

	INT hscl_coeff_update[ISP_FRC_SCL_MAX];
	INT vscl_coeff_update[ISP_FRC_SCL_MAX];
	INT hscl_coeff[ISP_FRC_SCL_MAX];
	INT vscl_coeff[ISP_FRC_SCL_MAX];
	/* 0:auto selection, 1:force selection scaler H coeff,
	 * 2:force selection scaler V coeff, 3:force selection both
	 */
	INT scl_force_sel[ISP_FRC_SCL_MAX];
	INT scl_coeff_update[ISP_FRC_SCL_MAX];
};

/************************************************/
/*********** ISP BE SCL object data structures *********/
/************************************************/
struct ISP_BE_SCL_OBJ {
	//Base address of DNSCL
	INT base_addr;
	//Status of object (running:idle)
	INT objStatus;
	//Priority of object
	INT objPriority;
	INT clientID;

	UINT8 ovp_win_updated;

	INT order;
	INT m_iValid;
	struct ISP_BE_SCL_WIN inputWin;
	struct ISP_BE_SCL_WIN contentWin;
	struct ISP_BE_SCL_WIN outputWin;
	UINT32 inSrcFrmt;
	UINT32 outSrcFrmt;
	UINT64 inAddr_y;
	UINT64 inAddr_UV;
	UINT64 outAddr_Y;
	UINT64 outAddr_UV;
	INT in_bit_depth;
	INT output_bit_depth;
	struct ISP_BE_SCL_RES scl_res[ISP_FRC_SCL_MAX];
	struct ISP_BE_SCL_CTRL scl_ctrl[ISP_FRC_SCL_MAX];
	// current frame in video display mode
	struct ISP_VID_BUF_DESC *curr_frame;
	// prev current frame in video display mode for back buffer
	struct ISP_VID_BUF_DESC *prev_curr_frame;
	INT ovp_win_stride;
	INT in_win_stride;
	/* current scaled cropped display content location and size in display window */
	struct ISP_BE_SCL_WIN crop_scl_actv_win;
	/* i/o resolution configured for offline scaling */
	struct ISP_BE_SCL_RES sclResCfg;
	struct ISPSS_BE_SCL_INPUT_CROP tg_crop;
	struct ISPBE_CLIENT_QUEUE m_queue;
	INT in_mtrMode;
	struct ISPSS_BE_MTR out_mtrConfigDesc;
	struct ISPSS_BE_MTR in_mtrConfigDesc;
	UINT16 def_idx_coeff_tbl[T_COEFF_MAX][ISP_FRC_SCL_NUM_OF_PHASES][ISP_FRC_SCL_NUM_OF_COEFF];
};

struct ISP_BE_SCL {
	INT     m_sclNoOfObjs;      //Number of SCL objects created.
	INT     m_sclCurrObjUnderExec;  //Current object under execution
	INT     m_iStatus;          //Status of SCL
	INT     m_sclHwStatus;          //HW Status of SCL
	struct ISP_REQUEST_QUEUE m_sclOutFrameQ; //Frames processed
	struct mutex isr_lock;
	struct ISP_BE_SCL_OBJ *isp_be_scl_obj[MAX_SCL_OBJECTS];
	SHM_HANDLE scl_handle[MAX_SCL_OBJECTS];
	SHM_HANDLE isp_be_scl_handle;
	SHM_HANDLE prv_msg_handle;
	ISPSS_INTR_HANDLER IntrHandler;
	void *IntrHandlerArgs;
	int IntCnt;
	INT commit_QId;
	struct ISPSS_BCMBUF_QUEUE bcmbufQ;
} __packed;

#endif
