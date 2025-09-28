// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated
 *
 */


#ifndef _HAL_VPP_H_
#define _HAL_VPP_H_            "_HAL_VPP_H_ >>>"
/**    hal_vpp.h for NTZ
 */

#include "avio_type.h"
#include "vpp_vbuf.h"
#include "vpp_priv.h"
#include "hal_vpp_mipi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MV_DISP_SUCCESS		(0)
#define MV_DISP_E_RES_CFG	(1)
#define MV_DISP_E_CREATE	(2)
#define MV_DISP_E_CFG		(3)
#define MV_DISP_E_RST		(4)
#define MV_DISP_E_WIN		(5)
#define MV_DISP_E_DISP		(6)
#define MV_DISP_E_NOMEM     (7)
#define MV_DISP_E_FAIL		(8)

//fastlogo.ta always uses Internal memory manager
#define VPP_ENABLE_INTERNAL_MEM_MGR

#define VPP_SETFORMAT_SIZE		(sizeof(VPP_DISP_OUT_PARAMS) + sizeof(INT))

#define VPP_ATTR_PARAM_GET_GLOBALPHA(val)          BFGET(val,31,16)
#define VPP_ATTR_PARAM_GET_PLANEID(val)            BFGET(val,15,0)
#define VPP_ATTR_PARAM_SET_GLOBALPHA(addr,val)     BFSET(addr,31,16,val)
#define VPP_ATTR_PARAM_SET_PLANEID(addr,val)       BFSET(addr,15,0,val)

/*definition of global alpha enable or disable (disable means: per pixel alpha)*/
typedef enum {
	GLOBAL_ALPHA_FLAG_DISABLE = 0,
	GLOBAL_ALPHA_FLAG_ENABLE,
} ENUM_GLOBAL_ALPHA_FLAG;


/*definition of various UUID supported*/
typedef enum {
	TA_UUID_FASTLOGO = 0,
	TA_UUID_VPP,
} ENUM_TA_UUID_TYPE;

/* definition of VOP2 input mode */
typedef enum {
	VPP_VOUT_SINGLE_MODE_PRI  = 0,/**< Primary overlay output on Primary Display */
	VPP_VOUT_SINGLE_MODE_SEC  = 1,/**< Primary overlay output on Secondary Display */
	VPP_VOUT_DUAL_MODE_PIP    = 2,/**< Secondary output with PIP plane. */
	VPP_VOUT_DUAL_MODE_GFX0   = 3,/**< Secondary output with GFX1 plane. */
	VPP_VOUT_DUAL_MODE_GFX1   = 4,/**< Secondary output with GFX2 plane. */
	VPP_VOUT_MODE_MAX         = 5/**< Output type Max */
} VOUT_DISP_MODE;

//struct    --
typedef struct VPP_WIN_T {
	int x;      /* x-coordination of a vpp window top-left corner in pixel, index starting from 0 */
	int y;      /* y-coordination of a vpp window top-left corner in pixle, index starting from 0 */
	int width;  /* width of a vpp window in pixel */
	int height; /* height of a vpp window in pixel */
} VPP_WIN;

typedef struct VPP_WIN_ATTR_T {
	int bgcolor;    /* background color of a vpp window */
	int alpha;      /* global alpha of a vpp window */
	ENUM_GLOBAL_ALPHA_FLAG globalAlphaFlag; /*1:enable plane/global alpha otherwise use per pixel alpha*/
} VPP_WIN_ATTR;

typedef struct VPP_ZORDER_CTRL_T {
	int main;
	int pip;
	int gfx1;
} VPP_ZORDER_CTRL;

typedef struct VPP_INIT_PARM_T {
	int iHDMIEnable;
	int iVdacEnable;
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
	//Physical address of the shared memory - used for heap/SHM allocation
	unsigned int uiShmPA;
	//Size of the shared memory
	unsigned int uiShmSize;
	unsigned int phShm;
	unsigned int gMemhandle;
#endif
	unsigned int g_vpp;
	void *dev;
} VPP_INIT_PARM;

typedef struct VPP_RESOLUTION_DESCRIPTION_T {
	unsigned int   uiActiveWidth;
	unsigned int   uiActiveHeight;
	unsigned int   uiWidth;
	unsigned int   uiHeight;
	unsigned int   uiRefreshRate;
	unsigned int   uiIsInterlaced;
	unsigned int   uiIsDDD;
} VPP_RESOLUTION_DESCRIPTION;

typedef struct VPP_DISP_OUT_PARAMS_T {
	ENUM_VOUT_ID uiDispId;
	int uiResId;
	UINT32 uiBitDepth;
	UINT32 uiColorFmt;
	INT32 iPixelRepeat;
	UINT32 uiDisplayMode;
} VPP_DISP_OUT_PARAMS;

/*HDMI Data*/
typedef struct VPP_HDMI_RAW_EDID_T{
   UINT8    DataBuf[1024];
   UINT8    IsValid;
   UINT8    Extblks;
}VPP_HDMI_RAW_EDID;

/*move to full VPP_HDMI_SINK_CAPS in vpp if requirement arise*/
typedef int VPP_HDMI_SINK_CAPS;
typedef enum {
	VPP_HDMI_SINKCAP_BITMASK_HDMIMODE,
	VPP_HDMI_SINKCAP_BITMASK_FULL4K,
	VPP_HDMI_SINKCAP_BITMASK_4K30,
	VPP_HDMI_SINKCAP_BITMASK_FHD,
	VPP_HDMI_SINKCAP_BITMASK_PREF50FPS,
} VPP_HDMI_SINKCAP_BITMASK;

typedef enum
{
	VPP_BLOCK_START = 0,
	VPP_BLOCK_CPCB_TG = VPP_BLOCK_START,
	VPP_BLOCK_PLANE,
	VPP_BLOCK_CHAN,
	VPP_BLOCK_VOUT,
	VPP_BLOCK_VPPOBJ,
	VPP_BLOCK_BE,

	VPP_BLOCK_MAX,
} ENUM_VPP_BLOCK;

#ifdef __cplusplus
}
#endif
/** _HAL_VPP_H_
 */
#endif
/** ENDOFFILE: hal_vpp.h
 */
