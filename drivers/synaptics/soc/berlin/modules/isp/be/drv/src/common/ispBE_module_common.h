/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPBE_MODULE_COMMON_H_
#define _ISPBE_MODULE_COMMON_H_

#include "ispSS_bcmbuf.h"
#include "ispSS_bcmbuf_queue.h"
#include "ispSS_shm.h"

#define ISPSS_GET_BCMBUF(RQST) (RQST->pBcmBuf)

/*ISP supported Rotations*/
enum ISP_ENUM_ROTATION {
	ISPBE_ROTATION_MIN    = 0,
	ISPBE_ROTATION_0D     = 0,
	ISPBE_ROTATION_90D    = 1,
	ISPBE_ROTATION_180D   = 2,
	ISPBE_ROTATION_270D   = 3,
	ISPBE_ROTATION_H_FLIP = 4,
	ISPBE_ROTATION_V_FLIP = 5,
	ISPBE_ROTATION_MAX
};

/* BCM Buffer handlers */
struct SHM_STRUCT {
	SHM_HANDLE  hShm;
	void *addr, *phy_addr;
};

struct ISP_BE_BCM {
	struct SHM_STRUCT shmHandle[4];
	struct BCMBUF  *bcm_buf;
	struct BCMBUF  *clear_bcm_buf;
	struct DHUB_CFGQ *dma_cfgQ;
	struct DHUB_CFGQ *final_bcm_cfgQ;
};

/* ISPSS window */
/* Content window >= active window */
struct ISPSS_WIN {
	/* x-coordination of a vpp window top-left
	 * corner in pixel, index starting from 0
	 */
	int x;
	/* y-coordination of a vpp window top-left
	 * corner in pixle, index starting from 0
	 */
	int y;
	int width;  /* width of a vpp window in pixel */
	int height; /* height of a vpp window in pixel */
};

/* constants; based on frame format
 * V8H8(8bit),V8H6(10bit),V6H8(10bit),YUV420, YUV422
 */
struct ISP_BE_DWP_HW_CONFIG {
	/* LGDC inputs */
	UINT32 cisHwin; //CIS window Horizontal size
	UINT32 cisVwin; //CIS window Vertical size
	UINT32 disHwin; //DIS read window Horizontal size
	UINT32 disVwin; //DIS read window Vertical size
	UINT32 disHgrid;//DIS Image Horizontal Grid/Tile dimension
	UINT32 disVgrid;//DIS Image Vertical Grid/Tile dimension
	BOOL   biCubicOff; //bi-cubic interpolation/Nearest neighborhood interpolation
};

/* Should be derived and filled by DeWarp algo */
struct ISP_BE_DWP_DERIVED {
	UINT32 cisMatSize;
	UINT32 cisBlkHtotal;
	UINT32 cisBlkVtotal;
	UINT32 maxDisBlocks; //Maximum DIS blocks in a stride, Limits the DIS Order LUT
	/* V8H6 is split in to subtiles so that 8 pixel pack for SRAM buffer storage */
	UINT32 disV4H2scan; // 1: V4H2 tiled DIS input 0:H4V2 tiled DIS input
	/* CIS block burst size, Number of CIS blocks
	 * processed at once Sent to Dhub as two conjoined tile
	 */
	UINT32 cisBsize;
	/* Restrict the DIS Chroma write above the Allowed DIS
	 * buffer limit during YUV420
	 */
	UINT32 limitCbuff;
	UINT32 disLumaTiles;
	UINT32 disChromaTiles;
	UINT32 disBlkSize; //Total Pixels in a DIS pixel 2D blocks
	UINT32 disBlkChromaSize;
	UINT32 blankLuma; //Luma value Out of active video
	UINT32 blankChroma; //Chroma value Out of active video
	UINT32 validDisBlks;
};

/* Purely for internal calculation of DeWarp */
struct ISP_BE_DWP_INTERNAL {
	UINT32 sensorID; //Sensor being used, 0/1. Corresponding DISIdxCISMat will be loaded
	UINT32 hRes; //Horizontal Resolution
	UINT32 vRes; //Vertical Resolution
	UINT32 yuvMode;      //YUV processing mode, 0:YUV420 1:YUV422
	UINT32 dwp_src_input_bpp; //Pixel data width. It can be 8 or 10.
	UINT32 dwp_src_output_bpp; //Pixel output bit depth
	UINT32 dwp_src_input_fmt;
	UINT32 dwp_src_output_fmt;
	/* LDI inputs */
	/* Base Address pointing to starting location of DIS Luma
	 * frame in DRAM. Must be in multiple of 64
	 */
	UINT32 dwp_dis_base_y; //Need to be written to CFGD0
	/* Base Address pointing to starting location
	 * of DIS Chroma frame in DRAM. Must be in multiple of 64
	 */
	UINT32 dwp_dis_base_uv; //Need to be written to CFGD1
	UINT32 dwp_cis_base_y;
	UINT32 dwp_cis_base_uv;
	UINT8 tg_mode;//0:mode0 1:mode1 2:mode2 Only for debugging
};

struct ISPSS_FRAME_PARAM {
	struct ISPSS_WIN win;
	UINT32 fmt;
	UINT32 bit_depth;

	//Derived/computed parameters
	UINT32 stride_y;
	UINT32 stride_uv;
	UINT32 size_y;
	UINT32 size_uv;

	//Un-altered parameters
	void *baseAddr_y;
	void *baseAddr_uv;

};

//Structure to get the frame info related to active LUT  and/or derive parameters
struct ISP_BE_DWP_LUT_FRAME_PARAM {
	BOOL dwp_out_path;
	UINT32 sensorID;
	struct ISPSS_FRAME_PARAM in_frame;
	struct ISPSS_FRAME_PARAM out_frame;
};

/* Non Polynomial Lens params */
struct ISP_BE_DWP_NON_POLY_LENS_PARAM {
	float kvalue;
	float hscale;
	float vscale;
	float hstretch;
	float vstretch;
	UINT32 coref;
	UINT32 focal;
	UINT32 cent_x;
	UINT32 cent_y;
	UINT32 htilt;
	UINT32 vtilt;
	UINT32 angle;
	UINT32 zoom;
	UINT32 rotate; //0:0 deg 1: 90 deg 2: 180 deg 3: 270 deg
	UINT32 CisHoffset;
	UINT32 CisVoffset;
	UINT32 CisHres;
	UINT32 CisVres;
};

/* Polynomial Lens Params */
struct ISP_BE_DWP_POLY_LENS_PARAM {
	float XC;
	float YC;
	float FX;
	float FY;
	double K0;
	double K1;
	double K2;
	double P1;
	double P2;
	double K3;
	double K4;
	float iXC;
	float iYC;
	double iFX;
	double iFY;
	UINT32 rotate; //0:0 deg 1: 90 deg 2: 180 deg 3: 270 deg
	UINT32 CisHoffset;
	UINT32 CisVoffset;
	UINT32 CisHres;
	UINT32 CisVres;
};

struct ISPBE_DEWARP_LUT_CFG {
	struct ISP_BE_DWP_LUT_FRAME_PARAM frameParam; /* frame parameters */
	struct ISP_BE_DWP_HW_CONFIG hwParams; /* load from CFG file */
	/* currently load from CFG file, later Algo to fill*/
	struct ISP_BE_DWP_DERIVED derivedParam;
	union {
		struct ISP_BE_DWP_NON_POLY_LENS_PARAM np;
		struct ISP_BE_DWP_POLY_LENS_PARAM p;
	} lensParam;/* Lens params; load from CFG file */
};

struct ISP_BE_DEWARP_RQST_MSG {
	BOOL dwp_out_path;
	struct ISPBE_DEWARP_LUT_CFG lutCFG;
};

/* Tiler specific Info */
struct ISP_BE_TILE_RQST_MSG {
	UINT32 tileHwin;
	UINT32 tileVwin;
	UINT32 TileFalseInt; //Only for debugging purpose
	/* 0:No RS to tile conversion, Output will be RS, 1:RS to Tile Conversion */
	BOOL isRsToTile;
	BOOL dataAlignment; // 0: LSB Aligned, 1:MSB Aligned
	BOOL isSwapEnable; //0:U&V same as input, 1:Swap U&V; uv_swap_reg
};

/* DNSCL specific Info */
struct ISP_BE_DNSCL_RQST_MSG {
	BOOL isInputFrmIsp; //0:Memory;1:ISP
	UINT32 isEnableSclBypass;
};

/* Request Message */
struct ISP_BE_ROT_RQST_MSG {
	INT DtileEn;
	enum ISP_ENUM_ROTATION RotationAngle;
};

struct ISP_BE_RQST_MSG {
	SHM_HANDLE rqst_shmHandle;
	UINT8 m_BuffID;   //Buffer Id to identify to which client this buffer belongs to
	UINT8 m_Priority; //Priority of this client

	struct ISP_BE_BCM bcmBuf;
	struct ISP_BE_BCM *pBcmBuf;
	int bcm_prepared;
	struct ISPSS_BCMBUF_ITEM *pCurrBcmBuf;

	UINT32 sensorID; //Sensor being used, 0/1. Corresponding DISIdxCISMat will be loaded

	/* input/output frame memory handles */
	SHM_HANDLE in_shmHandle;
	void *InputFrameAddr_Y;
	void *InputFrameAddr_UV;
	unsigned int in_frame_Ysize;
	unsigned int in_frame_UVsize;

	unsigned int in_frame_Ystride;
	unsigned int in_frame_UVstride;

	unsigned int in_frame_fmt;
	unsigned int in_frame_bit_depth;

	//active, content window for cropping
	struct ISPSS_WIN active, content, outwin;

	SHM_HANDLE out_shmHandle;
	void *OutputFrameAddr_Y;
	void *OutputFrameAddr_UV;
	unsigned int out_frame_Ysize;
	unsigned int out_frame_UVsize;

	unsigned int out_frame_Ystride;
	unsigned int out_frame_UVstride;

	unsigned int out_frame_fmt;
	unsigned int out_frame_bit_depth;

	UINT32 SubmitHw; //0: Do not submit to hardware;1: Submit to hardware processing
	UINT32 WaitforIntr; //0: Don't wait for interrupt, 1:wait for interrupt
	UINT32 FrameState; //0: Not processed; 1: processed, set on receiving interrupt
	INT32 ErrorCode; //Error code
	UINT32 debugFlag;   //Debug Purpose

	UINT8 in_mtrMode;   //bit 0: MTR, bit 1: MMU
	UINT8 out_mtrMode;   //bit 0: MTR, bit 1: MMU
	UINT32  in_mtrAddr_Y;
	UINT32  in_mtrAddr_UV;
	UINT32  out_mtrAddr_Y;
	UINT32  out_mtrAddr_UV;

	union {
		struct ISP_BE_DEWARP_RQST_MSG dwp;
		struct ISP_BE_DNSCL_RQST_MSG dnscl;
		struct ISP_BE_TILE_RQST_MSG tile;
		//ISP_BE_CVP_RQST_MSG cvp;
		struct ISP_BE_ROT_RQST_MSG rot;
	};

	void *user_data;

};

#endif
