// SPDX-License-Identifier: GPL-2.0
/* Copyright 2017 Synopsys Inc. */
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_DRV_H
#define HRX_DRV_H

#include <linux/delay.h>
#include <linux/lcm.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/sched/types.h>
#include <linux/videodev2.h>
#include <linux/kfifo.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-core.h>
#include <uapi/linux/sched/types.h>

#include "avio.h"
#include "avioDhub.h"
#include "avioGbl.h"
#include "avio_io.h"
#include "avio_common.h"
#include "avio_type.h"
#include "avio_base.h"
#include "avio_dhub_drv.h"
#include "hal_dhub.h"
#include "hal_dhub_wrap.h"
#include "drv_msg.h"
#include "vpp_mem.h"
#include "shm.h"
#include "hdmi_in_if.h"

#include "hdmirx.h"
#include "hdmirxpipe.h"
#include "vpp_mem.h"
#include "vpp.h"

#include "hrx-vip-bcmbuf.h"
#include "hrx-vip-framequeue.h"

#include "hrx-aip-buffer.h"
#include "hrx-aip-fifo-cmd.h"

#include "hrx-debug.h"
#include "hrx-sig-stat-notify.h"

#define STEPWISE_SUPPORT

/* HRX queue commands */
typedef enum HRX_CMD_ID_T {
	HRX_CMD_HANDLE_5V = 0x00,
	HRX_CMD_PACKET_CHANGE,
	HRX_CMD_CLOCK_CHANGE,
	HRX_CMD_AUDIO_CHANGE,
	HRX_CMD_MAX
} HRX_CMD_ID;

/* HRX internal states */
typedef enum HRX_STATE_T {
	HRX_STATE_DISCONNECTED = 0x00,
	HRX_STATE_UNSTABLE,
	HRX_STATE_PHY_STABLE,
	HRX_STATE_ALL_STABLE,
	HRX_STATE_MAX
} HRX_STATE;

typedef enum hdmi_state_t {
	HDMI_STATE_NO_INIT = 0,
	HDMI_STATE_POWER_OFF,
	HDMI_STATE_PHY_CONFIG,
	HDMI_STATE_CONTROLLER_CONFIG,
	HDMI_STATE_POWER_ON,
	HDMI_STATE_VG_MODE,
} hdmi_state;

typedef enum hdmi_controller_state_t {
	HDMI_CSTATE_NOT_CONFIGURED = 0,
	HDMI_CSTATE_PHY_RECONFIG,
	HDMI_CSTATE_EQUALIZER,
	HDMI_CSTATE_DATAPATH,
	HDMI_CSTATE_AUDIO,
	HDMI_CSTATE_VIDEO_UNSTABLE,
	HDMI_CSTATE_CONFIGURED,
	HDMI_CSTATE_VG_MODE,
} hdmi_controller_state;

typedef enum hrx_v4l2_state_t {
	HRX_V4L2_NO_INIT = 0,
	HRX_V4L2_STREAMING_ON,
	HRX_V4L2_STREAMING_OFF,
} hrx_v4l2_state;

typedef enum hrx_alsa_state_t {
	HRX_ALSA_NO_INIT = 0,
	HRX_ALSA_STREAMING_ON,
	HRX_ALSA_STREAMING_OFF,
} hrx_alsa_state;

typedef enum {
	HRX_POL_POSITIVE = 0,
	HRX_POL_NEGATIVE,
} hrx_sync_polarity;

/********************************************************************
 * Display resolution types
 ********************************************************************
 */
typedef enum tag_hrx_disp_resolution {
	/* Graphics */
	HRX_GFX_RES_FIRST,
	HRX_VGA_640X350_85HZ = HRX_GFX_RES_FIRST,
	HRX_VGA_640X400_85HZ,
	HRX_VGA_720X400_85HZ,
	//HRX_VGA_720X400_70HZ,
	HRX_VGA_640X350_70HZ,
	HRX_VGA_640X400_60HZ,
	HRX_VGA_720X400_70HZ,
	HRX_VGA_640X480_75HZ,
	HRX_VGA_640X480_85HZ,
	HRX_VGA_640X480_72HZ,
	HRX_VGA_640X480_60HZ,
	HRX_SVGA_800X600_56HZ,
	HRX_SVGA_800X600_75HZ,
	HRX_SVGA_800X600_60HZ,
	HRX_SVGA_800X600_85HZ,
	HRX_SVGA_832X624_75HZ,
	HRX_SVGA_800X600_72HZ,
	HRX_SVGA_1366X768_60HZ,
	HRX_VGA_1360X768_59_959HZ,
	HRX_VGA_1280X768_59_995HZ,
	HRX_VGA_1280X768_59_99HZ,
	HRX_WXGAP_1360X768_60HZ,
	HRX_WXGAP_CVT_1360X768_60HZ,
	HRX_WXGA_1280X768_60HZ,
	//HRX_WXGA_N_1280X768_60HZ,
	HRX_WXGAP_N_1360X768_60HZ,
	HRX_1366X768_DMT_60HZ,
	HRX_XGA_1024X768_72HZ,
	HRX_XGA_1024X768_75HZ,
	HRX_WXGA_1280X768_75HZ,
	HRX_XGA_1024X768_60HZ,
	HRX_XGA_1024X768_70HZ,
	HRX_XGA_1024X768_85HZ,
	HRX_WXGA_1280X768_85HZ,
	HRX_WXGA_1366X768_60HZ,
	HRX_WXGA_1280X800_60HZ,
	HRX_WXGA_1280X800_75HZ,
	HRX_WXGA_1280X800_85HZ,
	HRX_XGA_1152X864_75HZ,
	HRX_XGA_1152X870_75HZ,
	HRX_WXGAP_1366X768_50HZ,
	HRX_1440X900_CVT_60HZ,
	HRX_1400X900_CVT_75HZ,
	HRX_1400X900_CVT_85HZ,
	HRX_XGA_1280X960_60HZ,
	HRX_XGA_1280X960_85HZ,
	HRX_XGA_1280X1024_60HZ,
	HRX_XGA_1280X1024_75HZ,
	HRX_1400X1050_SXGA_CVR_60HZ,
	HRX_VGA_1680X1050_59_946HZ,
	HRX_1400X1050_SXGA_60HZ,
	HRX_XGA_1280X1024_85HZ,
	HRX_1680X1050_RB_60HZ,
	HRX_1680X1050_RB_N_60HZ,
	HRX_1680X1050_CVT_60HZ,
	HRX_1400X1050_CVT_75HZ,
	HRX_1680X1050_CVT_75HZ,
	HRX_1400X1050_CVT_85HZ,
	HRX_WUXGA_1920X1080_60HZ,
	HRX_WUXGA_1920X1200_RB_60HZ,
	HRX_WUXGA_1920X1200_60HZ,
	HRX_UXGA_1600X1200_60HZ,
	HRX_UXGA_1600X1200_65HZ,
	HRX_UXGA_1600X1200_70HZ,
	HRX_UXGA_1600X1200_75HZ,
	HRX_UXGA_1600X1200_85HZ,
	HRX_WUXGA_1792X1344_60HZ,
	HRX_WUXGA_1792X1344_75HZ,
	HRX_WUXGA_1856X1392_60HZ,
	HRX_WUXGA_1856X1392_75HZ,
	HRX_WUXGA_1920X1440_60HZ,
	HRX_WUXGA_1920X1440_75HZ,
	HRX_GFX_RES_LAST = HRX_WUXGA_1920X1440_75HZ,

	/* Interlaced Video */
	HRX_VID_RES_FIRST,
	HRX_SD_720X480I_59_94HZ = HRX_VID_RES_FIRST,
	HRX_SD_720X480I_60HZ, //NTSC_M ,NTSC_J
	HRX_SD_720X576I_50HZ, //PAL_B/G,PAL_D/K,PAL_I
	HRX_SD_NTSC_443_720X480I_60HZ,	//NTSC_443, NTSC_DK,NTSC_I
	HRX_SD_PAL60_720X480I_60HZ,	//PAL60
	HRX_SD_PAL_M_720X480I_60HZ,	//PAL_M
	HRX_SD_PAL_N_720X576I_50HZ,	//PAL_N
	HRX_SD_SECAM_720X576I_50HZ,	//SECAM

	HRX_HD_1920X1080I_59_94HZ,
	HRX_HD_1920X1080I_60HZ,
	HRX_HD_1920X1080I_50HZ,

	/* Progressive Video*/
	HRX_SD_720X480P_59_94HZ,
	HRX_SD_720X480P_60HZ,
	HRX_SD_720X576P_50HZ,
	HRX_HD_1280X720P_59_94HZ,
	HRX_HD_1280X720P_60HZ,
	HRX_HD_1280X720P_50HZ,
	HRX_HD_1920X1080P_59_94HZ,
	HRX_HD_1920X1080P_60HZ,
	HRX_HD_1920X1080P_29_97HZ,
	HRX_HD_1920X1080P_30HZ,
	HRX_HD_1920X1080P_25HZ,
	HRX_HD_1920X1080P_50HZ,
	HRX_HD_1920X1080P_23_97HZ,
	HRX_HD_1920X1080P_24HZ,

	/* New VICS */
	HRX_SD_1440X240P_59_94HZ,
	HRX_SD_2880X480I_59_94HZ,
	HRX_SD_2880X240P_59_94HZ,
	HRX_SD_1440X480P_59_94HZ,
	HRX_SD_1440X288P_50HZ,
	HRX_SD_2880X576I_50HZ,
	HRX_SD_2880X288P_50HZ,
	HRX_SD_1440X576P_50HZ,
	HRX_SD_2880X480P_59_94HZ,
	HRX_SD_2880X576P_50HZ,
	HRX_HD_1920X1250I_50HZ,
	HRX_HD_1920X1080I_100HZ,
	HRX_HD_1280X720P_100HZ,
	HRX_SD_720X576P_100HZ,
	HRX_SD_1440X576I_100HZ,
	HRX_HD_1920X1080I_120HZ,
	HRX_HD_1280X720P_120HZ,
	HRX_SD_720X480P_120HZ,
	HRX_SD_1440X480I_120HZ,
	HRX_SD_720X576P_200HZ,
	HRX_SD_720X576I_200HZ,
	HRX_SD_720X480P_240HZ,
	HRX_SD_1440X480I_240HZ,
	HRX_HD_1280X720P_24HZ,
	HRX_HD_1280X720P_25HZ,
	HRX_HD_1280X720P_30HZ,
	HRX_HD_1920X1080P_100HZ,
	HRX_HD_1920X1080P_119_88HZ,
	HRX_HD_1920X1080P_120HZ,
	HRX_HD_3840X2160P_23_98HZ,
	HRX_HD_3840X2160P_24HZ,
	HRX_HD_3840X2160P_29_97HZ,
	HRX_HD_3840X2160P_30HZ,
	HRX_HD_3840X2160P_59_94HZ,
	HRX_HD_3840X2160P_60HZ,
	HRX_HD_3840X2160P_59_94HZ_420,
	HRX_HD_3840X2160P_60HZ_420,
	HRX_HD_3840X2160P_25HZ,
	HRX_HD_3840X2160P_50HZ,
	HRX_HD_3840X2160P_50HZ_420,
	HRX_HD_4096X2160P_23_98HZ,
	HRX_HD_4096X2160P_24HZ,
	HRX_HD_4096X2160P_29_97HZ,
	HRX_HD_4096X2160P_30HZ,
	HRX_HD_4096X2160P_59_94HZ,
	HRX_HD_4096X2160P_60HZ,
	HRX_HD_4096X2160P_59_94HZ_420,
	HRX_HD_4096X2160P_60HZ_420,
	HRX_HD_4096X2160P_25HZ,
	HRX_HD_4096X2160P_50HZ,
	HRX_HD_4096X2160P_50HZ_420,

	HRX_VID_RES_LAST = HRX_HD_4096X2160P_50HZ_420,

	/* Above types are fixed. Hereafter add custom resolutions if required */
	HRX_CUSTOM_RESOLUTIONS,
	HRX_MAX_RESOLUTIONS,
	HRX_UNKNOWN_MODE,
	HRX_INVALID_MODE,
} hrx_display_resolution;

typedef enum tag_hrx_mw_signal_polarity {
	HRX_MW_POSITIVE_POLARITY = 0,
	HRX_MW_NEGATIVE_POLARITY
} hrx_mw_signal_polarity;

typedef struct hrx_frame_interval {
	u16 numerator;
	u16 denominator;
} hrx_frameinterval;

/********************************************************************
 * Timing parameters which Application will maintain. This will be used
 * as intermidiate struct to deal with GUI as well as Command APIs
 *********************************************************************
 */
typedef struct tag_hrx_app_timing_param {
	hrx_display_resolution	DispResId;
	u16						HTotal;
	u16						HActive;
	u16						HFrontPorch;
	u8						HSyncWidth;
	u16						DACHFrontPorch;
	u16						DACHSyncWidth;
	hrx_mw_signal_polarity	HSyncPol;
	u16						VTotal;
	u16						VActive;
	u16						VFrontPorch;
	u8						VSyncWidth;
	hrx_mw_signal_polarity	VSyncPol;
	u32						RefreshRate; /* scale to 100 */
	bool					Progressive;
	u8						AspectRatio;
	bool					IsProgSegmentFrame;
	u16						HtotalRefClk;
} hrx_application_timing_params;

typedef enum hrx_hdmi_rx_col_fmt_t {
	HRX_HDMI_RX_COL_FMT_RGB = 0,
	HRX_HDMI_RX_COL_FMT_YUV444,
	HRX_HDMI_RX_COL_FMT_YUV422,
	HRX_HDMI_RX_COL_FMT_YUV420,
	HRX_HDMI_RX_COL_FMT_MAX
} hrx_hdmi_rx_col_format_type;

typedef enum hrx_hdmi_rx_col_std_t {
	HRX_HDMI_RX_COL_STD_NONE = 0,
	HRX_HDMI_RX_COL_STD_SMPTE170M,
	HRX_HDMI_RX_COL_STD_ITUR709,
	HRX_HDMI_RX_COL_STD_EXT_xvYCC601,
	HRX_HDMI_RX_COL_STD_EXT_xvYCC709,
	HRX_HDMI_RX_COL_STD_EXT_sYCC601,
	HRX_HDMI_RX_COL_STD_EXT_AdobeYCC601,
	HRX_HDMI_RX_COL_STD_EXT_AdobeRGB,
	HRX_HDMI_RX_COL_STD_EXT_BT2020cYCC,
	HRX_HDMI_RX_COL_STD_EXT_BT2020YCC,
	HRX_HDMI_RX_COL_STD_RSVD,
} hrx_hdmi_rx_col_std;

typedef enum hrx_hdmi_rx_col_depth_t {
	HRX_HDMI_RX_COL_DEPTH_8BIT = 0,
	HRX_HDMI_RX_COL_DEPTH_10BIT,
	HRX_HDMI_RX_COL_DEPTH_12BIT,
	HRX_HDMI_RX_COL_DEPTH_16BIT,
	HRX_HDMI_RX_COL_DEPTH_MAX
} hrx_hdmi_rx_col_depth_type, *phrx_hdmi_rx_col_depth_type;

typedef enum hrx_sig_video_format_t {
	HRX_SIG_VID_FMT_2D = 0,
	HRX_SIG_VID_FMT_3D_FRAME_PACKING,
	HRX_SIG_VID_FMT_3D_SIDE_BY_SIDE,
	HRX_SIG_VID_FMT_3D_TOP_AND_BOTTOM,
	HRX_SIG_VID_FMT_3D_SIDE_BY_SIDE_FULL,
	HRX_SIG_VID_FMT_3D_FIELD_ALT,
	HRX_SIG_VID_FMT_3D_LINE_ALT,
	HRX_SIG_VID_FMT_3D_L_DEPTH,
	HRX_SIG_VID_FMT_EXT_VIDEO_4K_2K // for 4k x2k
} hrx_sig_video_format;

typedef struct hrx_hdr_infoframe_t {
	unsigned char eotf;
	unsigned char metadata_type;
	unsigned short display_prim_x[3];
	unsigned short display_prim_y[3];
	unsigned short white_point_x;
	unsigned short white_point_y;
	unsigned short max_dml;
	unsigned short min_dml;
	unsigned short max_cll;
	unsigned short max_fll;
} hrx_hdr_infoframe;

/********************************************************************
 * Aspect ratios
 ********************************************************************/
typedef enum hrx_aspect_ratio_t {
	HRX_ASP_NONE = 0,
	HRX_ASP_4_3,
	HRX_ASP_16_9,
	//HRX_ASP_13_9,
	//HRX_ASP_14_9,
	//HRX_ASP_15_9,
	HRX_ASP_21_9,
	HRX_ASP_ODD
} hrx_aspect_ratio;

/********************************************************************
 * Standard refresh rates
 ********************************************************************/
typedef enum hrx_refresh_rate_t {
	HRX_RR_MIN_RANGE = 0,
	HRX_RR_23_97HZ = HRX_RR_MIN_RANGE,
	HRX_RR_24HZ,
	HRX_RR_25HZ,
	HRX_RR_29_97HZ,
	HRX_RR_30HZ,
	HRX_RR_47_95HZ,
	HRX_RR_48HZ,
	HRX_RR_50HZ,
	HRX_RR_56HZ,
	// HRX_RR_59_949HZ,
	HRX_RR_59_94HZ,
	HRX_RR_60HZ,
	HRX_RR_65HZ,
	HRX_RR_70HZ,
	HRX_RR_72HZ,
	HRX_RR_75HZ,
	HRX_RR_80HZ,
	HRX_RR_85HZ,
	HRX_RR_60_32HZ,
	HRX_RR_60_004HZ,
	HRX_RR_60_020HZ,
	HRX_RR_59_86HZ,
	HRX_RR_59_789HZ,
	HRX_RR_60_015HZ,
	HRX_RR_59_883HZ,
	HRX_RR_74_867HZ,
	HRX_RR_74_892HZ,
	HRX_RR_59_954HZ,
	HRX_RR_59_888HZ,
	HRX_RR_59_978HZ,
	HRX_RR_70_087HZ,
	HRX_RR_100HZ,
	HRX_RR_120HZ,
	HRX_RR_200HZ,
	HRX_RR_240HZ,
	HRX_RR_56_250HZ,
	HRX_RR_59_87HZ,
	HRX_RR_59_887HZ,
	HRX_RR_59_948HZ,
	HRX_RR_59_95HZ,
	HRX_RR_59_995HZ,
	HRX_RR_60_317HZ,
	HRX_RR_70_069HZ,
	HRX_RR_72_188HZ,
	HRX_RR_72_808HZ,
	HRX_RR_74_893HZ,
	HRX_RR_74_934HZ,
	HRX_RR_74_984HZ,
	HRX_RR_74_997HZ,
	HRX_RR_75_025HZ,
	HRX_RR_75_029HZ,
	HRX_RR_84_837HZ,
	HRX_RR_84_842HZ,
	HRX_RR_84_880HZ,
	HRX_RR_84_960HZ,
	HRX_RR_84_997HZ,
	HRX_RR_85_002HZ,
	HRX_RR_85_008HZ,
	HRX_RR_85_024HZ,
	HRX_RR_85_039HZ,
	HRX_RR_85_061HZ,
	HRX_RR_85_080HZ,
	HRX_RR_MAX_RANGE,
	HRX_RR_INVALID
} hrx_refresh_rate;

/********************************************************************
 * Refresh rate max-min range
 ********************************************************************
 */
typedef struct refresh_rate_range_t {
	u32 RefCountMin; /* scale to 100 */
	u32 RefCountMax; /* scale to 100 */
	hrx_refresh_rate RefRate;
} refresh_rate_range;

typedef struct hrx_signal_param_status_t {
	unsigned short		HTotal;
	unsigned char		HSyncWidth;
	unsigned short		VTotal;
	unsigned char		VSyncWidth;
	u32					RefRate; /* scale to 100 */
	u16					HtotalRefClk;
} hrx_signal_param_status, *phrx_signal_param_status;

typedef struct tag_hrx_auto_detect_vid_params {
	hrx_application_timing_params	HrxIpTimingParam;
	u8								IsHdmiMode;
	u8								IsValid;
	u32								Vic;
	hrx_hdmi_rx_col_format_type		HrxIpColorFormatType;
	hrx_hdmi_rx_col_depth_type		HrxIpBitDepthType;
	hrx_sig_video_format			VideoFrmt;
	hrx_hdmi_rx_col_std				HrxIpColorStd;
	hrx_aspect_ratio				AspectRatio;
	hrx_frameinterval				FITotal;
	hrx_frameinterval				FIRequested;
	hrx_frameinterval				FIActive;
	u8								ActiveFmt;
	u8								hdmiVSIF[31];
	u8								hdmiAVIF[16];
	u8								hdmiAudioIF[8];
	u8								hdmiChStatus[6];
	u64								HrxIpPixelClkFreq; /* scale to 100 */
	u8								hdmiSPDIF[28];
	bool							videoFullrange;
	unsigned char					PixelReptFactor;
	hrx_hdr_infoframe				hdmiHDRIF;
} hrx_auto_detect_video_params;

typedef struct tag_hrx_auto_detect_aud_params {
	unsigned int	N;
	unsigned int	CTS;
	unsigned int	SampFreq;
	unsigned int	rx_tmdsfreq;
	unsigned int	NumOfChannels;
	unsigned int	SampleSize;
	bool			IsLinearPCM;
	bool			IsHBR;
} hrx_auto_detect_audio_params, *phrx_auto_detect_audio_params;

enum dw_earctx_audio_format {
	EARCTX_AUDIO_FORMAT_ASP = 0,
	EARCTX_AUDIO_FORMAT_OBA = 1,
	EARCTX_AUDIO_FORMAT_DTS = 2,
	EARCTX_AUDIO_FORMAT_HBR = 3,
	EARCTX_AUDIO_FORMAT_MSA = 4,
	EARCTX_AUDIO_FORMAT_OBMSA = 5,
	EARCTX_AUDIO_FORMAT_3D = 6,
	EARCTX_AUDIO_FORMAT_3D_OBA = 7,
};

// To provide range to Vt and Ht
#define VTOT_TOLARANCE_VAL	5
#define HTOT_TOLARANCE_VAL	15
#define HSW_TOLARANCE_VAL	3
#define VSW_TOLARANCE_VAL	1

#define TIMING_TOL_VAL		3
#define TIMING_TOL_VAL_FP	3
#define TOL_VAL_CLK			2
#define VAL_WITHIN_RANGE	30

// Please ckeck here and update if you have changed table below
#define GFX_RES_ID_START	HRX_VGA_640X350_85HZ
#define GFX_RES_ID_END		HRX_WUXGA_1920X1440_75HZ
#define GFX_TOTAL_RES_IDS	(GFX_RES_ID_END - GFX_RES_ID_START + 1)

#define VID_RES_ID_START	HRX_SD_720X480I_59_94HZ
#define VID_RES_ID_END		HRX_HD_3840X2160P_50HZ
#define VID_TOTAL_RES_IDS	(VID_RES_ID_END - VID_RES_ID_START + 1)

extern resource_size_t hrx_base_glbl;
extern resource_size_t vip_base_glbl;

extern struct platform_device *pdev_glbl;

typedef struct hdmi_rx_drv_t {
// fill it with driver priv information
	bool IsHDMIMode;
	hrx_hdmi_rx_col_depth_type RxColDepth;
	u64 tmds_clock;
} hdmi_rx_drv;

struct syna_hrx_vid_fmt {
	const char *name;
	const char *description;
	u32 fourcc;
	u32 code;
	u8 depth;
	u8 fps;
	bool has_csc;
	bool is_codec;
#ifdef STEPWISE_SUPPORT
	struct v4l2_frmsize_stepwise frmsize;
#endif
	struct v4l2_frmsize_discrete dis_frmsize;
};

struct syna_hrx_v4l2_dev {
	struct v4l2_device		v4l2_dev;
	struct video_device		*vdev;
	struct v4l2_m2m_dev		*m2m_dev;
	struct vb2_buffer		*reserved_buf;
	struct mutex			mutex;
	struct mutex			vip_mutex;
	struct list_head		queued_bufs;
	spinlock_t				queued_lock;
	spinlock_t				aip_spinlock;

	struct semaphore process_buffer_sem;
	struct semaphore vip_sem;
	struct semaphore hrx_sem;
	struct semaphore aip_sem;
	struct semaphore aip_main_sem;

	AMPMsgQ_t vip_msg_queue;
	AMPMsgQ_t hrx_msg_queue;
	AMPMsgQ_t aip_msg_queue;
	AMPMsgQ_t aip_main_msg_queue;
	struct kfifo processed_buffer_queue;

	struct task_struct *process_buffer_task;
	struct task_struct *hrx_isr_task;
	struct task_struct *vip_isr_task;
	struct task_struct *vip_watcher_task;
	struct task_struct *aip_isr_task;
	struct task_struct *aip_main_task;
	struct task_struct *aip_watcher_task;

	/* Interrupts */
	u32 otg_intr;
	u32 hdmirx_intr;
	u32 ytg_intr;
	u32 uvtg_intr;
	u32 itg_intr;
	u32 mic3_intr;

	HDL_dhub *dhub;
	HDL_dhub *dhub_vpp;

	/* Status */
	bool regbank_ready;
	bool apb_if_ready;
	bool timer_base_locked;

	/* Configurations */
	u32 phy_cfg_clk;
	bool is_hdmi2;
	bool phy_reset;
	u8 current_vic;
	bool current_vic_is_4k;

	struct hrx_infoframe aviif;
	struct hrx_infoframe spdif;
	struct hrx_infoframe audioif;
	struct hrx_infoframe vsif;
	struct hrx_infoframe hdrif;

	/* video capture */
	u32	width;
	u32	height;
	u32	outfmt;
	u32	bpp;
	struct v4l2_pix_format_mplane pix_fmt;

	resource_size_t hrx_base;
	resource_size_t vip_base;
	struct resource *edid_resource;
	void __iomem *edid_regs;
	struct gpio_desc *gpiod_hrxhpd;
	struct gpio_desc *gpiod_hrx5v;

	/*file ops*/
	struct v4l2_fh fh;
	struct v4l2_pix_format_mplane format;
	const struct syna_hrx_vid_fmt *fmtinfo; /* External format with CSC */
	struct vb2_queue dst_vq;
	struct device *dev;
	wait_queue_head_t vblank_wq;
	struct task_struct *dum_thread;
	atomic_t start_thread;

	/* VIP */
	u32 ui_pipe_ctrl;
	HDL_dhub2d *vip_dhub2d;
	HDL_dhub2d *vip_ag_dhub2d;

	/* VIP DHUB DMA channel IDs */
	s32 dvi_dmaWID[2];	// VIP video frame write client DMA channel ID
	s32 dvi_pip_dmaWID;	// VIP video frame pip write client DMA channel ID
	s32 vbi_dmaWID;	// VIP ancillary packet write client DMA channel ID

	VPP_MEM_LIST *mem_list;
	VPP_MEM_LIST *aip_mem_list;


	//We need 2 buffers: prepare one while waiting for HW to execute one
	BCMBUF vbi_bcm_buf[2];
	BCMBUF *pcurr_vbi_bcm_buf;	//pointer to the VBI BCM buffer in use

	DHUB_CFGQ dhub_bcm_cfgQ[2]; // dual DHUB BCM CFGQs
	DHUB_CFGQ *pcurr_dhub_bcm_cfgQ; // pointer to current DHUB BCM channel CFGQ in use
	VPP_MEM dhub_bcm_mem_handle[2];

	DHUB_CFGQ dhub_dma_cfgQ[2]; // dual DHUB DMA CFGQs
	DHUB_CFGQ *pcurr_dhub_dma_cfgQ; // pointer to current DHUB DMA channel CFGQ in use
	VPP_MEM   dhub_dma_mem_handle[2];

	ENUM_VIP_STATUS vip_status;
	ENUM_VIP_IMODE vip_imode;
	ENUM_VIP_OMODE vip_omode;
	ENUM_VIP_SIGNAL_STATUS vip_signal_status;
	bool vip_dma_cmd_issued;
	bool vip_enable_scaler;
	bool vip_first_frame;
	bool vip_first_intr;
	int vip_bits_per_pixel;
	u32 ui_tunnel_mode;
	u32 ui_md_dump_enable;

	VIP_FRAMEQUEUE frmq;		// DVI frame buffer queue
	void *vip_curr_frame_descr;	// current frame descriptor allocated
	struct vb2_buffer *vip_curr_free_buf;
	phys_addr_t vip_curr_phys_addr;
	u32 vip_intr_num;
	int vip_field_flag;		// 0-progressive; 1-interlace
	int vip_mode_3d;		//input mode: 0-2D; 1-3D frame packing; 2-3D sbs; 3-3D top and bottom;
	int vip_ext_data;		//3D ext-data
	int vip_hwidth_orig;	// VIP input original width in pixel
	int vip_vheight_orig;	// VIP input original height in pixel
	int vip_hwidth;			// VIP output width in pixel
	int vip_vheight;		// VIP output height in pixel
	int vip_htotal;			// VIP input total width in pixel
	int vip_vtotal;			// VIP input total height in pixel
	int vip_sync_type;
	int vip_top;			//field status of interlace mode
	u32 display_frame_index;
	u32 frame_count;

	/* SCL handles */
	VPP_MEM scl_coeffs_mem_handle[VIP_MAX_NUM_PREDEFINED_COEFFS];
	u32 *scl_coeffs[VIP_MAX_NUM_PREDEFINED_COEFFS];
	VPP_MEM cust_scl_coeffs_mem_handle[VIP_FRC_SCL_MAIN_LAY_MAX][VIP_MAX_NUM_PREDEFINED_COEFFS];
	u32 *cust_scl_coeffs[VIP_FRC_SCL_MAIN_LAY_MAX][VIP_MAX_NUM_PREDEFINED_COEFFS];

	/* SCL shadow registers */
	u32 uiSclFrcCfg0[VIP_FRC_SCL_MAX];
	u32 uiSclFrcCfg1[VIP_FRC_SCL_MAX];
	u32 uiSclCfg0[VIP_FRC_SCL_MAX];
	u32 uiSclCfg3[VIP_FRC_SCL_MAX];
	u32 uiSclCfg6[VIP_FRC_SCL_MAX];
	u32 uiSclCfg7[VIP_FRC_SCL_MAX];
	u32 uiSclCfg8[VIP_FRC_SCL_MAX];
	u32 uiSclCfg15[VIP_FRC_SCL_MAX];

	int hscl_coeff[VIP_FRC_SCL_MAX];
	int vscl_coeff[VIP_FRC_SCL_MAX];
	/* For customized SCL coeffs */
	int hscl_coeff_update[VIP_FRC_SCL_MAX];
	int vscl_coeff_update[VIP_FRC_SCL_MAX];
	VIP_SCL_RES scl_res[VIP_FRC_SCL_MAX];
	VIP_SCL_CTRL scl_ctrl[VIP_FRC_SCL_MAX];
	bool cust_scl_id_flag[VIP_FRC_SCL_LAY_MAX][VIP_MAX_NUM_PREDEFINED_COEFFS];
	bool cust_sc_bPrepared[VIP_FRC_SCL_LAY_MAX][VIP_MAX_NUM_PREDEFINED_COEFFS];
	bool cust_sc_bLoaded[VIP_FRC_SCL_LAY_MAX][VIP_MAX_NUM_PREDEFINED_COEFFS];

	struct device *alloc_dev;
	HRX_STATE HrxState;
	HRX_CMD_ID hrx_cmd_id;
	hdmi_state hdmi_state;
	hdmi_controller_state hdmi_cstate;
	bool is_scrambled;
	bool is_frl;
	int tmds2_reconfig;
	bool phy_locked;
	bool force_off;
	bool pending_config;
	int video_stable_wait_ms;
	int has_clock_wait_ms;
	int tmds_valid_wait_count;
	bool phy_eq_force;
	bool phy_eq_on;
	bool video_stable;
	hrx_v4l2_state hrx_v4l2_state;

	hrx_auto_detect_video_params video_params;
	bool IsAVIChkSumValid;
	hdmi_rx_drv hdmiRxDrv;
	u8	spd_info[30];
	bool Is14EdidLoaded;
	u32 gpio_hpd;

	u32 cts;
	int sample_freq_source;
	bool audio_locked;
	unsigned int audio_sf;
	hrx_auto_detect_audio_params audio_params;
	bool audFifoDis;
	int audErrCnt;
	struct semaphore aip_main_task_sem;
	struct semaphore aip_freeze_sem;
	AIP_BUFFER aip_buf;
	AIP_FIFO aip_fifo;

	int nDmaChunkSize;
	ENUM_AIP_SOURCE_TYPE eSourceType;

	int nCmdSeqId;
	int nCmdResult;

	AIP_OBJ_STATUS aip_status;
	AIP_SIGNAL_STATUS eSignalStatus;
	AIP_SIGNAL_STATUS ePreSignalStatus;
	int nInputThres;
	ENUM_AIP_MONO_MODE eMonoMode;  /**< mono mode, only valid when input is stereo */
	unsigned int nMode;

	// For Freeze
	bool bFreeze;
	bool bInitTimeUpdated;

	AIP_TIME_STAMP sInitTime;
	AIP_TIME_STAMP sCurrentTime;

	bool bSourceTypeUpdated;
	int aip_i2s_pair;
	struct shm_client *client;
	struct aip_shm shm;
	hrx_alsa_state hrx_alsa_state;

	aip_frame_alloc_cb pFrameAllocCB;
	aip_frame_free_cb pFrameFreeCB;
	aip_event_cb pEventCB;
	void *pFrameAllocCtx;
	void *pFrameFreeCtx;
	void *pEventCtx;
	AIP_ALSA_CMD_DATA in_data;
	bool aip_hold_enable;

	bool vip_restart;
	bool aip_restart;
	int null_data_count;
	bool trig_scl_reset;
	int frame_drop_count;

};

int hrx_isr_state_update(struct syna_hrx_v4l2_dev *hrx_dev, CC_MSG_t msg);
void hrx_intr_reset(struct syna_hrx_v4l2_dev *hrx_dev);
bool hrx_is_hdmi2(struct syna_hrx_v4l2_dev *hrx_dev);
int hrx_get_tg_params(struct syna_hrx_v4l2_dev *hrx_dev, VIP_TG_PARAMS *pParams);
int hrx_get_refresh_rate_by_resolution(hrx_display_resolution resolution);

int syna_hrx_audio_init(struct syna_hrx_v4l2_dev *hrx_dev);
void syna_hrx_audio_exit(struct syna_hrx_v4l2_dev *hrx_dev);

int hrx_get_aud_err(struct syna_hrx_v4l2_dev *hrx_dev, u32 *pAudErr);
int hrx_toggle_hpd(struct syna_hrx_v4l2_dev *hrx_dev, bool hpd);
bool hrx_is_5v_connected(struct syna_hrx_v4l2_dev *hrx_dev);
int aip_alsa_get_hrx_status(ENUM_HRX_STATUS *hrx_status);
void hrx_update_active_frame_interval(struct syna_hrx_v4l2_dev *hrx_dev);
void hrx_audio_reset(struct syna_hrx_v4l2_dev *hrx_dev);

extern int aip_alsa_set_ops(const struct alsa_aip_ops *ptr);

static inline int
syna_hrx_is_vip_stable(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return ((hrx_dev->hdmi_state == HDMI_STATE_POWER_ON) &&
					(hrx_dev->hrx_v4l2_state == HRX_V4L2_STREAMING_ON) && (hrx_dev->HrxState == HRX_STATE_ALL_STABLE));
}

#define HRX_ISR_MSGQ_SIZE						128
#define MEMMAP_AVIO_REG_BASE					0xF7400000
//#define AVIO_MEMMAP_AVIO_HDMIRX_WRAP_REG_BASE	0xD800
#define HDMIRX_WRAP_REG_BASE \
	(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_HDMIRX_WRAP_REG_BASE)
#define HDMIRX_PIPE_REG_BASE \
	(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE)
#define BCM_REG_BASE \
	(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_BCM_REG_BASE)
#define HRX_AUDIO_FIFO_ERROR_THRESHOLD 1000
#define HRX_TMDS_DETECT_THRESHOLD 200
#define HRX_6G_TMDS_DETECT_THRESHOLD 300
#endif //HRX_DRV_H
