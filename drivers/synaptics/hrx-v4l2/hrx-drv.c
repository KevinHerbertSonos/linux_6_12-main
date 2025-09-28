// SPDX-License-Identifier: GPL-2.0
/* Copyright 2017 Synopsys Inc. */
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-drv.h"
#include "hrx-video.h"
#include "hrx-hpd.h"
#include "hrx-reg.h"
#include "hrx-edid.h"
#include "hrx-phy.h"
#include "hrx-hdmi.h"
#include "hrx-vip.h"
#include "hrx-aip.h"
#include "hrx-isr.h"
#include "hrx-vip-isr.h"
#include "hrx-aip-isr.h"
#include "hrx-audio.h"

#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0))
#define RET void
#define RETURN
#else
#define RET int
#define RETURN return 0
#endif

resource_size_t hrx_base_glbl;
resource_size_t vip_base_glbl;
struct platform_device *pdev_glbl;

#define HDMI_SF_BYTE	0
#define HDMI_SF_MASK	GENMASK(27, 24)
#define HDMI_SF_OFFSET	24
#define HDMI_SF1_MASK	GENMASK(31, 30)
#define HDMI_SF1_OFFSET	26

/* Audio Sampling width offset in regbank and mask */
#define HDMI_ASW_BYTE	1
#define HDMI_ASW_MASK	GENMASK(3, 0)
#define HDMI_ASW_OFFSET	0

#define HRX_REF_CLK					250
#define TIMING_SANITY_CHECK_HEIGHT  10
#define HDMI_AUDIO_FREQ_RANGE	1000

/* Corresponds to 248.5MHz. Got overflow error with 0x3e8(250MHz) */
#define REFCLK_COUNT 0x3E2

struct refclk_config {
	int vic;
	int cd;
	u32 ref_rate; /* scale to 100 */
	int refclk_change;
};
struct refclk_config refclk_data[] = {
	{97, -1, 5994, -1},        // 4k59.94 all bitdepth
	{2, 36, 5994, 20},         // 480p59.94 12bit
	{2, 30, 5994, 6},          // 480p59.94 10bit
	{2, 24, 5994, 6},          // 480p59.94 8bit
	{2, 36, 6000, 20},         // 480p60 12bit
	{2, 30, 6000, 6},          // 480p60 10bit
	{2, 24, 6000, 6},          // 480p60 8bit
};

#define IsValueInRange(valuetochk, exactval, rangearound) ((valuetochk >= (exactval - rangearound)) \
								&& (valuetochk <= (exactval + rangearound)))
static unsigned int tmds_lost_count;

int hrx_config_channel_spread_en(struct syna_hrx_v4l2_dev *hrx_dev);

/********************************************************************
 * Refresh rate range table
 *********************************************************************
 */

const refresh_rate_range RefreshRateTableVideo[HRX_RR_MAX_RANGE] = {
	{2250,	2398,	HRX_RR_23_97HZ},
	{2398,	2445,	HRX_RR_24HZ},
	{2450,	2600,	HRX_RR_25HZ},
	{2900,	2998,	HRX_RR_29_97HZ},
	{2998,	3100,	HRX_RR_30HZ},
	{4700,	4796,	HRX_RR_47_95HZ},
	{4796,	4900,	HRX_RR_48HZ},
	{4900,	5100,	HRX_RR_50HZ},
	{5500,	5800,	HRX_RR_56HZ},
	{5800,	5995, 	HRX_RR_59_94HZ},
	{5970,	6040,	HRX_RR_60HZ},
	{6400,	6600,	HRX_RR_65HZ},
	{6900,	7100,	HRX_RR_70HZ},
	{7100,	7300,	HRX_RR_72HZ},
	{7400,	7600,	HRX_RR_75HZ},
	{7900,	8100,	HRX_RR_80HZ},
	{8150,	8600,	HRX_RR_85HZ},
	{6015,	6100,	HRX_RR_60_32HZ},
	{10000,	10200,	HRX_RR_100HZ},
	{12000,	12200,	HRX_RR_120HZ},
	{20000,	20200,	HRX_RR_200HZ},
	{24000,	24500,	HRX_RR_240HZ},
};

/*-----------------------------------------------------------------------------
 * Resolution timing tables
 *-----------------------------------------------------------------------------
 */

/*
 * DispResId - Resolution ID
 * HT = H-Total (Pixels)
 * HA = H-Active (Pixels)
 * HFP = H-FrontPorch (Pixels)
 * HSW = H-SyncWidth (Pixels)
 * DACHFrontPorch
 * DACHSyncWidth
 * HNegPol
 * VT = V-Total (Lines)
 * VA = V-Active (Lines)
 * VFP = - V-FrontPorch (lines)
 * VSW = V-SyncWidth (Lines)
 * VNegPol
 * RR - Refresh rate
 * Progressive or not
 * Aspect Ratio
 *
 * HT = HSW + HBP + HA + HFP (where HBP is HorzBackPorch)
 * VT = VSW + VBP + VA + VFP (where VBP is VertBackPorch)
 *
 */
/*  *RR - Timing same but refresh rate is different */
hrx_application_timing_params GfxModeParamTable_HDMI[] = {
	//	ResId,							Ht,		Ha,		Hfp,	Hsw,	Hdfp,	Hdsw,	HSyPol,						Vt,		Va,		Vfp,	Vsw,VSyPol,						RR,				Progr		AspectRatio
	{	HRX_VGA_640X350_85HZ,			832,	640,	32,		64,		32,		64,		HRX_MW_POSITIVE_POLARITY,	445,	350,	32,		3,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_85HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_VGA_640X400_85HZ,			832,	640,	32,		64,		32,		64,		HRX_MW_NEGATIVE_POLARITY,	445,	400,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_85HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_VGA_720X400_85HZ,			936,	720,	36,		72,		36,		72,		HRX_MW_NEGATIVE_POLARITY,	446,	400,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_85HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_VGA_640X350_70HZ,			800,	640,	16,		96,		16,		96,		HRX_MW_POSITIVE_POLARITY,	449,	350,	37,		2,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_70HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_VGA_640X400_60HZ,			800,	640,	16,		96,		16,		96,		HRX_MW_POSITIVE_POLARITY,	449,	400,	12,		2,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_VGA_640X480_75HZ,			840,	640,	16,		64,		16,		64,		HRX_MW_NEGATIVE_POLARITY,	500,	480,	1,		3,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_VGA_640X480_85HZ,			832,	640,	56,		56,		56,		56,		HRX_MW_NEGATIVE_POLARITY,	509,	480,	1,		3,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_85HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_VGA_640X480_72HZ,			832,	640,	24,		40,		16,		40,		HRX_MW_NEGATIVE_POLARITY,	520,	480,	9,		3,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_72HZ,	TRUE,		HRX_ASP_4_3		},
	//{	HRX_VGA_640X480_60HZ,			800,	640,	8,		96,		8,		96,		HRX_MW_NEGATIVE_POLARITY,	525,	480,	8,		2,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_VGA_640X480_60HZ,			800,	640,	16,		96,		8,		96,		HRX_MW_NEGATIVE_POLARITY,	525,	480,	10,		2,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_SVGA_800X600_56HZ,			1024,	800,	24,		72,		24,		72,		HRX_MW_POSITIVE_POLARITY,	625,	600,	1,		2,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_56HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_SVGA_800X600_75HZ,			1056,	800,	16,		80,		16,		80,		HRX_MW_POSITIVE_POLARITY,	625,	600,	1,		3,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	//{	HRX_SVGA_800X600_60HZ,			1056,	800,	40,		128,	40,		128,	HRX_MW_POSITIVE_POLARITY,	628,	600,	1,		4,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_SVGA_800X600_60HZ,			1056,	800,	40,		128,	40,		128,	HRX_MW_POSITIVE_POLARITY,	628,	600,	1,		4,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	/*C*/
	{	HRX_SVGA_800X600_85HZ,			1048,	800,	32,		64,		32,		64,		HRX_MW_POSITIVE_POLARITY,	631,	600,	1,		3,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_85HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_SVGA_832X624_75HZ,			1080,	832,	32,		64,		32,		64,		HRX_MW_NEGATIVE_POLARITY,	655,	624,	1,		3,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_SVGA_800X600_72HZ,			1040,	800,	56,		120,	56,		120,	HRX_MW_POSITIVE_POLARITY,	666,	600,	37,		6,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_72HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_WXGA_1280X768_60HZ,			1440,	1280,	48,		32,		48,		32,		HRX_MW_POSITIVE_POLARITY,	790,	768,	3,		7,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_VGA_1280X768_59_99HZ,		1680,	1280,	64,		136,	64,		136,	HRX_MW_NEGATIVE_POLARITY,	795,	768,	1,		7,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_WXGAP_N_1360X768_60HZ,		1776,	1360,	64,		144,	64,		144,	HRX_MW_NEGATIVE_POLARITY,	795,	768,	1,		5,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_WXGAP_1360X768_60HZ,		1792,	1360,	64,		112,	64,		112,	HRX_MW_POSITIVE_POLARITY,	795,	768,	3,		6,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_WXGAP_CVT_1360X768_60HZ,	1776,	1360,	72,		136,	72,		136,	HRX_MW_POSITIVE_POLARITY,	798,	768,	3,		5,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_59_94HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_XGA_1024X768_72HZ,			1312,	1024,	24,		136,	24,		136,	HRX_MW_POSITIVE_POLARITY,	800,	768,	3,		6,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_72HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_XGA_1024X768_75HZ,			1312,	1024,	16,		96,		16,		96,		HRX_MW_POSITIVE_POLARITY,	800,	768,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_XGA_1024X768_60HZ,			1344,	1024,	24,		136,	24,		136,	HRX_MW_NEGATIVE_POLARITY,	806,	768,	3,		6,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_XGA_1024X768_70HZ,			1328,	1024,	24,		136,	24,		136,	HRX_MW_NEGATIVE_POLARITY,	806,	768,	3,		6,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_70HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_XGA_1024X768_85HZ,			1376,	1024,	48,		96,		48,		96,		HRX_MW_POSITIVE_POLARITY,	808,	768,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_85HZ,	TRUE,		HRX_ASP_4_3		},
	//{	HRX_WXGA_1280X800_60HZ,			1440,	1280,	48,		32,		48,		32,		HRX_MW_POSITIVE_POLARITY,	823,	800,	3,		6,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_WXGA_1280X800_60HZ,			1680,	1280,	64,		136,	64,		136,	HRX_MW_NEGATIVE_POLARITY,	828,	800,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_XGA_1152X864_75HZ,			1600,	1152,	64,		128,	64,		128,	HRX_MW_POSITIVE_POLARITY,	900,	864,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_XGA_1152X870_75HZ,			1600,	1152,	64,		128,	64,		128,	HRX_MW_POSITIVE_POLARITY,	906,	870,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_WXGAP_1366X768_50HZ,		1522,	1366,	80,		32,		80,		32,		HRX_MW_POSITIVE_POLARITY,	920,	768,	2,		5,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_50HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_1440X900_CVT_60HZ,			1904,	1440,	80,		152,	80,		152,	HRX_MW_NEGATIVE_POLARITY,	934,	900,	3,		6,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},//Priya_modified	for	1440x900
	{	HRX_XGA_1280X960_60HZ,			1800,	1280,	96,		112,	96,		112,	HRX_MW_POSITIVE_POLARITY,	1000,	960,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_XGA_1280X960_85HZ,			1728,	1280,	64,		160,	64,		160,	HRX_MW_POSITIVE_POLARITY,	1011,	960,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_85HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_XGA_1280X1024_60HZ,			1688,	1280,	48,		112,	48,		112,	HRX_MW_POSITIVE_POLARITY,	1066,	1024,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_XGA_1280X1024_75HZ,			1688,	1280,	16,		144,	16,		144,	HRX_MW_POSITIVE_POLARITY,	1066,	1024,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_1400X1050_SXGA_60HZ,		1864,	1400,	88,		144,	88,		144,	HRX_MW_NEGATIVE_POLARITY,	1089,	1050,	3,		4,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_XGA_1280X1024_85HZ,			1728,	1280,	64,		160,	64,		160,	HRX_MW_POSITIVE_POLARITY,	1072,	1024,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_85HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_1680X1050_RB_60HZ,			1840,	1680,	48,		32,		48,		32,		HRX_MW_POSITIVE_POLARITY,	1080,	1050,	3,		6,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_1680X1050_RB_N_60HZ,		2256,	1680,	104,	184,	104,	184,	HRX_MW_NEGATIVE_POLARITY,	1087,	1050,	1,		6,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_1680X1050_CVT_60HZ,			2240,	1680,	104,	176,	104,	176,	HRX_MW_NEGATIVE_POLARITY,	1089,	1050,	3,		6,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_16_9	},
	{	HRX_WUXGA_1920X1080_60HZ,		2080,	1920,	48,		32,		48,		32,		HRX_MW_POSITIVE_POLARITY,	1111,	1080,	2,		5,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_WUXGA_1920X1200_RB_60HZ,	2080,	1920,	48,		32,		48,		32,		HRX_MW_POSITIVE_POLARITY,	1235,	1200,	3,		6,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_WUXGA_1920X1200_60HZ,		2592,	1920,	136,	200,	136,	200,	HRX_MW_POSITIVE_POLARITY,	1245,	1200,	3,		6,	HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	/*RR*/
	{	HRX_UXGA_1600X1200_60HZ,		2160,	1600,	64,		192,	64,		192,	HRX_MW_POSITIVE_POLARITY,	1250,	1200,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_UXGA_1600X1200_65HZ,		2160,	1600,	64,		192,	64,		192,	HRX_MW_POSITIVE_POLARITY,	1250,	1200,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_65HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_UXGA_1600X1200_70HZ,		2160,	1600,	64,		192,	64,		192,	HRX_MW_POSITIVE_POLARITY,	1250,	1200,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_70HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_UXGA_1600X1200_75HZ,		2160,	1600,	64,		192,	64,		192,	HRX_MW_POSITIVE_POLARITY,	1250,	1200,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_UXGA_1600X1200_85HZ,		2160,	1600,	64,		192,	64,		192,	HRX_MW_POSITIVE_POLARITY,	1250,	1200,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_85HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_WUXGA_1792X1344_60HZ,		2448,	1792,	128,	200,	128,	200,	HRX_MW_POSITIVE_POLARITY,	1394,	1344,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_WUXGA_1792X1344_75HZ,		2456,	1792,	96,		216,	96,		216,	HRX_MW_POSITIVE_POLARITY,	1417,	1344,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_WUXGA_1856X1392_60HZ,		2528,	1856,	96,		224,	96,		224,	HRX_MW_NEGATIVE_POLARITY,	1439,	1392,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_WUXGA_1856X1392_75HZ,		2560,	1856,	128,	224,	128,	224,	HRX_MW_NEGATIVE_POLARITY,	1500,	1392,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_WUXGA_1920X1440_60HZ,		2600,	1920,	128,	208,	128,	208,	HRX_MW_NEGATIVE_POLARITY,	1500,	1440,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,	TRUE,		HRX_ASP_4_3		},
	{	HRX_WUXGA_1920X1440_75HZ,		2640,	1920,	144,	224,	144,	224,	HRX_MW_NEGATIVE_POLARITY,	1500,	1440,	1,		3,	HRX_MW_POSITIVE_POLARITY,	HRX_RR_75HZ,	TRUE,		HRX_ASP_4_3		},
	/*	Above	timings	are	fixed,	Please	do	not	change.	Hereafter	add	custom	resolutions	if	required	*/
};

/* RR - Timing same but refresh rate is different */
/* Interlaced + Progressive SD, HD Modes */
hrx_application_timing_params VideoModeParamTable[] = {
	//	ResId,						Ht,		Ha,		Hfp,	Hsw,	Hdfp,	Hdsw,	HSyPol,						Vt,		Va,		Vfp,	Vsw,	VSyPol,						RR,					Progr	AspectRatio		ClockMonDiv
	{	HRX_SD_720X480I_59_94HZ,	858,	720,	19,		62,		16,		63,		HRX_MW_NEGATIVE_POLARITY,	262,	240,	4,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_59_94HZ,		FALSE,	HRX_ASP_4_3,	0,	1271	},	//	0x43B
	{	HRX_SD_720X480I_60HZ,		858,	720,	19,		62,		16,		63,		HRX_MW_NEGATIVE_POLARITY,	262,	240,	4,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,		FALSE,	HRX_ASP_4_3,	0,	1271	},	//	0x422
	{	HRX_SD_720X576I_50HZ,		864,	720,	12,		63,		12,		64,		HRX_MW_NEGATIVE_POLARITY,	312,	288,	2,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_50HZ,		FALSE,	HRX_ASP_4_3,	0,	1280	},	//	0
	{	HRX_HD_1920X1080I_59_94HZ,	2200,	1920,	88,		44,		44,		88,		HRX_MW_POSITIVE_POLARITY,	562,	540,	2,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_59_94HZ,		FALSE,	HRX_ASP_16_9,	0,	594		},	//	0x14E
	{	HRX_HD_1920X1080I_60HZ,		2200,	1920,	88,		44,		44,		88,		HRX_MW_POSITIVE_POLARITY,	562,	540,	2,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,		FALSE,	HRX_ASP_16_9,	0,	592		},	//	0x142
	{	HRX_HD_1920X1080I_50HZ,		2640,	1920,	528,	44,		484,	88,		HRX_MW_POSITIVE_POLARITY,	562,	540,	2,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_50HZ,		FALSE,	HRX_ASP_16_9,	0,	711		},	//	0
	{	HRX_SD_720X480P_59_94HZ,	858,	720,	16,		62,		16,		62,		HRX_MW_NEGATIVE_POLARITY,	525,	480,	9,		6,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_59_94HZ,		TRUE,	HRX_ASP_4_3,	0,	636		},	//	0x49E
	{	HRX_SD_720X480P_60HZ,		858,	720,	16,		62,		16,		62,		HRX_MW_NEGATIVE_POLARITY,	525,	480,	9,		6,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_4_3,	0,	635		},	//	0x491
	{	HRX_SD_720X576P_50HZ,		864,	720,	12,		64,		12,		64,		HRX_MW_NEGATIVE_POLARITY,	625,	576,	5,		5,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_50HZ,		TRUE,	HRX_ASP_4_3,	0,	640		},	//	0
	{	HRX_HD_1280X720P_59_94HZ,	1650,	1280,	110,	40,		70,		80,		HRX_MW_POSITIVE_POLARITY,	750,	720,	5,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_59_94HZ,		TRUE,	HRX_ASP_16_9,	0,	444		},	//	0x4BB
	{	HRX_HD_1280X720P_60HZ,		1650,	1280,	110,	40,		70,		80,		HRX_MW_POSITIVE_POLARITY,	750,	720,	5,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_16_9,	0,	445		},	//	0x4B2
	{	HRX_HD_1280X720P_50HZ,		1980,	1280,	440,	40,		400,	80,		HRX_MW_POSITIVE_POLARITY,	750,	720,	5,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_50HZ,		TRUE,	HRX_ASP_16_9,	0,	534		},	//	0
	{	HRX_HD_1920X1080P_59_94HZ,	2200,	1920,	88,		44,		88,		44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_59_94HZ,		TRUE,	HRX_ASP_16_9,	0,	297		},	//	0x327
	{	HRX_HD_1920X1080P_60HZ,		2200,	1920,	88,		44,		88,		44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_16_9,	0,	296		},	//	0x321
	{	HRX_HD_1920X1080P_29_97HZ,	2200,	1920,	88,		44,		88,		44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_29_97HZ,		TRUE,	HRX_ASP_16_9,	0,	593		},	//	0x14E
	{	HRX_HD_1920X1080P_30HZ,		2200,	1920,	88,		44,		88,		44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_30HZ,		TRUE,	HRX_ASP_16_9,	0,	592		},	//	0x142
	{	HRX_HD_1920X1080P_25HZ,		2640,	1920,	528,	44,		528,	44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_25HZ,		TRUE,	HRX_ASP_16_9,	0,	711		},	//	0
	{	HRX_HD_1920X1080P_50HZ,		2640,	1920,	528,	44,		528,	44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_50HZ,		TRUE,	HRX_ASP_16_9,	0,	356		},	//	0
	{	HRX_HD_1920X1080P_23_97HZ,	2750,	1920,	638,	44,		638,	44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_23_97HZ,		TRUE,	HRX_ASP_16_9,	0,	741		},	//	0x2E2
	{	HRX_HD_1920X1080P_24HZ,		2750,	1920,	638,	44,		638,	44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_24HZ,		TRUE,	HRX_ASP_16_9,	0,	741		},	//	0x2D3
	{	HRX_1680X1050_RB_60HZ,		1840,	1680,	48,		32,		48,		32,		HRX_MW_POSITIVE_POLARITY,	1080,	1050,	3,		6,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_16_9	},
	{	HRX_1680X1050_RB_N_60HZ,	2256,	1680,	104,	184,	104,	184,	HRX_MW_NEGATIVE_POLARITY,	1087,	1050,	1,		6,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_16_9	},
	{	HRX_1680X1050_CVT_60HZ,		2240,	1680,	104,	176,	104,	176,	HRX_MW_NEGATIVE_POLARITY,	1089,	1050,	3,		6,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_59_94HZ,		TRUE,	HRX_ASP_16_9	},

	//NEW VICS (Based on VICS)
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{	HRX_SD_1440X240P_59_94HZ,	858,	720,	19,		62,		19,		62,		HRX_MW_NEGATIVE_POLARITY,	262,	240,	4,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_2880X480I_59_94HZ,	858,	720,	19,		62,		19,		62,		HRX_MW_NEGATIVE_POLARITY,	262,	240,	4,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,		FALSE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_2880X240P_59_94HZ,	858,	720,	19,		62,		19,		62,		HRX_MW_NEGATIVE_POLARITY,	262,	240,	4,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_1440X480P_59_94HZ,	858,	720,	16,		62,		16,		62,		HRX_MW_NEGATIVE_POLARITY,	525,	480,	9,		6,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_1440X288P_50HZ,		864,	720,	12,		63,		12,		63,		HRX_MW_NEGATIVE_POLARITY,	312,	288,	2,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_50HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_2880X576I_50HZ,		864,	720,	12,		63,		12,		63,		HRX_MW_NEGATIVE_POLARITY,	312,	288,	2,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_50HZ,		FALSE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_2880X288P_50HZ,		864,	720,	12,		63,		12,		63,		HRX_MW_NEGATIVE_POLARITY,	312,	288,	2,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_50HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_1440X576P_50HZ,		864,	720,	12,		64,		12,		64,		HRX_MW_NEGATIVE_POLARITY,	625,	576,	5,		5,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_50HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_2880X480P_59_94HZ,	858,	720,	16,		62,		16,		62,		HRX_MW_NEGATIVE_POLARITY,	525,	480,	9,		6,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_2880X576P_50HZ,		864,	720,	12,		64,		12,		64,		HRX_MW_NEGATIVE_POLARITY,	625,	576,	5,		5,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_50HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_HD_1920X1250I_50HZ,		2304,	1920,	32,		168,	32,		168,	HRX_MW_POSITIVE_POLARITY,	625,	540,	23,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_50HZ,		FALSE,	HRX_ASP_16_9,	0,	0x9DE	},	//	0x2D3
	{	HRX_HD_1920X1080I_100HZ,	2640,	1920,	528,	44,		528,	44,		HRX_MW_POSITIVE_POLARITY,	562,	540,	2,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_100HZ,		FALSE,	HRX_ASP_16_9,	0,	0x9DE	},	//	0x2D3
	{	HRX_HD_1280X720P_100HZ,		1980,	1280,	440,	40,		440,	40,		HRX_MW_POSITIVE_POLARITY,	750,	720,	5,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_100HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_720X576P_100HZ,		864,	720,	12,		64,		12,		64,		HRX_MW_NEGATIVE_POLARITY,	625,	576,	5,		5,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_100HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_1440X576I_100HZ,		864,	720,	12,		63,		12,		63,		HRX_MW_NEGATIVE_POLARITY,	312,	288,	2,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_100HZ,		FALSE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_HD_1920X1080I_120HZ,	2200,	1920,	88,		44,		88,		44,		HRX_MW_POSITIVE_POLARITY,	562,	540,	2,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_120HZ,		FALSE,	HRX_ASP_16_9,	0,	0x9DE	},	//	0x2D3
	{	HRX_HD_1280X720P_120HZ,		1650,	1280,	110,	40,		110,	40,		HRX_MW_POSITIVE_POLARITY,	750,	720,	5,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_120HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_720X480P_120HZ,		858,	720,	16,		62,		16,		62,		HRX_MW_NEGATIVE_POLARITY,	525,	480,	9,		6,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_120HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_1440X480I_120HZ,		858,	720,	19,		62,		19,		62,		HRX_MW_NEGATIVE_POLARITY,	262,	240,	4,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_120HZ,		FALSE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_720X576P_200HZ,		864,	720,	12,		64,		12,		64,		HRX_MW_NEGATIVE_POLARITY,	625,	576,	5,		5,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_200HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_720X576I_200HZ,		864,	720,	12,		63,		12,		63,		HRX_MW_NEGATIVE_POLARITY,	312,	288,	2,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_200HZ,		FALSE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_720X480P_240HZ,		858,	720,	16,		62,		16,		62,		HRX_MW_NEGATIVE_POLARITY,	525,	480,	9,		6,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_240HZ,		TRUE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_SD_1440X480I_240HZ,		858,	720,	19,		62,		19,		62,		HRX_MW_NEGATIVE_POLARITY,	262,	240,	4,		3,		HRX_MW_NEGATIVE_POLARITY,	HRX_RR_240HZ,		FALSE,	HRX_ASP_4_3,	0,	0x9DE	},	//	0x2D3
	{	HRX_HD_1280X720P_24HZ,		4125,	1280,	2585,	40,		2585,	40,		HRX_MW_POSITIVE_POLARITY,	750,	720,	5,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_24HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},	//	0x2D3
	{	HRX_HD_1280X720P_25HZ,		3960,	1280,	2420,	40,		2420,	40,		HRX_MW_POSITIVE_POLARITY,	750,	720,	5,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_25HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},	//	0x2D3
	{	HRX_HD_1280X720P_30HZ,		3300,	1280,	1760,	40,		1760,	40,		HRX_MW_POSITIVE_POLARITY,	750,	720,	5,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_30HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},	//	0x2D3
	{	HRX_HD_1920X1080P_100HZ,	2640,	1920,	528,	44,		528,	44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_100HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_1920X1080P_120HZ,	2200,	1920,	88,		44,		88,		44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_120HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_23_98HZ,	5500,	3840,	1276,	88,		1276,	88,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_23_97HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_24HZ,		5500,	3840,	1276,	88,		1276,	88,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_24HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_29_97HZ,	4400,	3840,	176,	88,		176,	88,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_29_97HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_30HZ,		4400,	3840,	176,	88,		176,	88,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_30HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_59_94HZ,	4400,	3840,	176,	88,		176,	88,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_59_94HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_60HZ,		4400,	3840,	176,	88,		176,	88,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_59_94HZ,	2200,	1920,	84,		48,		84,		48,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_59_94HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_60HZ,		2200,	1920,	84,		48,		84,		48,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_60HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_25HZ,		5280,	3840,	1056,	88,		1056,	88,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_25HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_50HZ,		5280,	3840,	1056,	88,		1056,	88,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_50HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_HD_3840X2160P_50HZ,		2640,	1920,	528,	44,		528,	44,		HRX_MW_POSITIVE_POLARITY,	2250,	2160,	8,		10,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_50HZ,		TRUE,	HRX_ASP_16_9,	0,	0x9DE	},
	{	HRX_UNKNOWN_MODE,			2750,	1920,	638,	44,		638,	44,		HRX_MW_POSITIVE_POLARITY,	1125,	1080,	4,		5,		HRX_MW_POSITIVE_POLARITY,	HRX_RR_24HZ,		TRUE,	HRX_ASP_16_9,	0,	0		},	//	0
	/*	Above	timings	are	fixed,	Please	do	not	change.	Hereafter	add	custom	resolutions	if	required	*/
};

static void hrx_disable_all_ints(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_reg_write(hrx_dev, 0x0, HDMI_MAINUNIT_0_INT_MASK_N);
	hrx_reg_write(hrx_dev, 0x0, HDMI_MAINUNIT_2_INT_MASK_N);
	hrx_reg_write(hrx_dev, 0x0, HDMI_AVPUNIT_0_INT_MASK_N);
	hrx_reg_write(hrx_dev, 0x0, HDMI_AVPUNIT_1_INT_MASK_N);
	hrx_reg_write(hrx_dev, 0x0, HDMI_AVPUNIT_2_INT_MASK_N);
	hrx_reg_write(hrx_dev, 0x0, HDMI_PKT_0_INT_MASK_N);
	hrx_reg_write(hrx_dev, 0x0, HDMI_PKT_1_INT_MASK_N);
	hrx_reg_write(hrx_dev, 0x0, HDMI_SCDC_INT_MASK_N);
	hrx_reg_write(hrx_dev, 0x0, HDMI_HDCP_INT_MASK_N);
}

static void hrx_clear_ints(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_reg_write(hrx_dev, ~0x0, HDMI_MAINUNIT_0_INT_CLEAR);
	hrx_reg_write(hrx_dev, ~0x0, HDMI_MAINUNIT_1_INT_CLEAR);
	hrx_reg_write(hrx_dev, ~0x0, HDMI_MAINUNIT_2_INT_CLEAR);
	hrx_reg_write(hrx_dev, ~0x0, HDMI_AVPUNIT_0_INT_CLEAR);
	hrx_reg_write(hrx_dev, ~0x0, HDMI_AVPUNIT_1_INT_CLEAR);
	hrx_reg_write(hrx_dev, ~0x0, HDMI_PKT_0_INT_CLEAR);
	hrx_reg_write(hrx_dev, ~0x0, HDMI_PKT_1_INT_CLEAR);
	hrx_reg_write(hrx_dev, ~0x0, HDMI_SCDC_INT_CLEAR);
	hrx_reg_write(hrx_dev, ~0x0, HDMI_HDCP_INT_CLEAR);
	// TODO: Added CEC, to be checked if any impact
	hrx_reg_write(hrx_dev, ~0x0, HDMI_CEC_INT_CLEAR);
}

static void hrx_enable_vital_ints(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_reg_mask_write(hrx_dev, 0x1,
		HDMI_MAINUNIT_2_INT_MASK_N,
		HDMI_MAINUNIT_2_INT_MASK_N_TMDSVALID_STABLE_OFFSET,
		HDMI_MAINUNIT_2_INT_MASK_N_TMDSVALID_STABLE_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1,
		HDMI_MAINUNIT_0_INT_MASK_N,
		HDMI_MAINUNIT_0_INT_MASK_N_TMDSQP_CK_OFF_OFFSET,
		HDMI_MAINUNIT_0_INT_MASK_N_TMDSQP_CK_OFF_MASK);
}

static void hrx_enable_ACR_ints(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
			HDMI_PKT_0_INT_MASK_N_ACR_N_OFFSET,
			HDMI_PKT_0_INT_MASK_N_ACR_N_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
			HDMI_PKT_0_INT_MASK_N_ACR_CTS_OFFSET,
			HDMI_PKT_0_INT_MASK_N_ACR_CTS_MASK);
}

static void hrx_reset_all(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_reg_write(hrx_dev,
		HDMI_GLOBAL_SWRESET_REQUEST_APB |
		HDMI_GLOBAL_SWRESET_REQUEST_CEC |
		HDMI_GLOBAL_SWRESET_REQUEST_DATAPATH |
		HDMI_GLOBAL_SWRESET_REQUEST_AUDIO,
		HDMI_GLOBAL_SWRESET_REQUEST);
}

static void hrx_toggle_scdc_hpd(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_reg_mask_write(hrx_dev, 0x1,
		HDMI_SCDC_CONFIG,
		HDMI_SCDC_CONFIG_HPDLOW_OFFSET,
		HDMI_SCDC_CONFIG_HPDLOW_MASK);
	msleep(100);
	hrx_reg_mask_write(hrx_dev, 0x0,
		HDMI_SCDC_CONFIG,
		HDMI_SCDC_CONFIG_HPDLOW_OFFSET,
		HDMI_SCDC_CONFIG_HPDLOW_MASK);
}

static void hrx_enable_scdc(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_toggle_scdc_hpd(hrx_dev);
	hrx_reg_mask_write(hrx_dev, 0x1,
		HDMI_SCDC_CONFIG,
		HDMI_SCDC_CONFIG_POWERPROVIDED_OFFSET,
		HDMI_SCDC_CONFIG_POWERPROVIDED_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1,
		HDMI_DESCRAND_EN_CONTROL,
		HDMI_DESCRAND_EN_CONTROL_SCRAMB_EN_OFFSET,
		HDMI_DESCRAND_EN_CONTROL_SCRAMB_EN_MASK);
	/*TODO: Find out register and purpose */
	hrx_reg_mask_write(hrx_dev, 0x1,
		0x5bc, 0, GENMASK(7, 0));
	hrx_reg_write(hrx_dev, ~0x0, HDMI_SCDC_INT_MASK_N);
}

static void hrx_config_video(struct syna_hrx_v4l2_dev *hrx_dev)
{
	/* Make HSync and VSync output positive polarity */
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_VIDEO_CONFIG2,
			HDMI_VIDEO_CONFIG2_HSYNC_POL_OVR_OFFSET,
			HDMI_VIDEO_CONFIG2_HSYNC_POL_OVR_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_VIDEO_CONFIG2,
			HDMI_VIDEO_CONFIG2_HSYNC_POL_OVR_EN_OFFSET,
			HDMI_VIDEO_CONFIG2_HSYNC_POL_OVR_EN_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_VIDEO_CONFIG2,
			HDMI_VIDEO_CONFIG2_VSYNC_POL_OVR_OFFSET,
			HDMI_VIDEO_CONFIG2_VSYNC_POL_OVR_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_VIDEO_CONFIG2,
			HDMI_VIDEO_CONFIG2_VSYNC_POL_OVR_EN_OFFSET,
			HDMI_VIDEO_CONFIG2_VSYNC_POL_OVR_EN_MASK);
	/* Switch to Deframer output as video monitor source */
	hrx_reg_mask_write(hrx_dev, 0x4, HDMI_VMON_CONTROL,
			HDMI_VMON_CONTROL_SOURCE_SEL_OFFSET,
			HDMI_VMON_CONTROL_SOURCE_SEL_MASK);
	/*
	 * Video change interrupts will start occur after this function
	 * finishes so set the video stable flag to false. Interrupt handler
	 * will update this value when a video change interrupt occurs.
	 */
	hrx_dev->video_stable = false;
}

static void hrx_config_packets(struct syna_hrx_v4l2_dev *hrx_dev)
{
	/* Enable BCH and checksum error filter */
	hrx_reg_write(hrx_dev, ~0x0, HDMI_PKTEX_BCH_ERRFILT_CONFIG);
	hrx_reg_write(hrx_dev, ~0x0, HDMI_PKTEX_CHKSUM_ERRFILT_CONFIG);
}

static void hrx_reset_audio(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_reg_write(hrx_dev, HDMI_GLOBAL_SWRESET_REQUEST_AUDIO,
			HDMI_GLOBAL_SWRESET_REQUEST);
	HRX_LOG(HRX_DRV_DEBUG, "In this fn %s\n", __func__);
}


static bool hrx_audio_valid(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return hrx_reg_read(hrx_dev, HDMI_MAINUNIT_STATUS) &
			HDMI_MAINUNIT_STATUS_AUDPLL_LOCK_STABLE;
}

static bool has_audio_locked(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return (hrx_reg_read(hrx_dev, HDMI_CMU_STATUS) &
			HDMI_CMU_STATUS_AUDIO_CK_LOCKED);
}

bool hrx_is_5v_connected(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u8 i = 0;
	u32 value;

	value = syna_hrx_get_hrx5v(hrx_dev);
	for (i = 0; i < 4; i++) {
		mdelay(5);
		value = syna_hrx_get_hrx5v(hrx_dev);
	}
	if ((value & 0x01) == 0x00)
		return false;
	else
		return true;
}

int hrx_toggle_hpd(struct syna_hrx_v4l2_dev *hrx_dev, bool hpd)
{
	//HRX_LOG(HRX_DRV_DEBUG,"Toggle HPD %x -> %d\n", hrx_dev->gpio_hpd, hpd);
	syna_hrx_set_hpd(hrx_dev, hpd);

	/* Potential fix for scdc tmds clk ratio not changing*/
	if (hpd)
		hrx_toggle_scdc_hpd(hrx_dev);
	return 0;
}

static void hrx_config_i2c(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 refclk = HRX_REF_CLK;/* hrx_dev->dw_dev->config->iref_clk; MHz */
	u32 target_out = (300 / refclk) * 10; /* 300ns minimum */

	hrx_reg_mask_write(hrx_dev, target_out,
		HDMI_I2C_SLAVE_CONFIG1,
		HDMI_I2C_SLAVE_CONFIG1_SDA_OUT_HOLD_VALUE_OFFSET,
		HDMI_I2C_SLAVE_CONFIG1_SDA_OUT_HOLD_VALUE_MASK);
}

static void hrx_enable_hpd(struct syna_hrx_v4l2_dev *hrx_dev,
	u32 input_mask)
{
	hrx_reg_mask_write(hrx_dev, input_mask,
		HDMI_CORE_CONFIG,
		HDMI_CORE_CONFIG_HPD_OFFSET,
		HDMI_CORE_CONFIG_HPD_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1,
		HDMI_HDCP22_CONFIG,
		HDMI_HDCP22_CONFIG_HPD_OFFSET,
		HDMI_HDCP22_CONFIG_HPD_MASK);
}

static int hrx_hard_irq_handler(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 mu0_stat = hrx_reg_read(hrx_dev, HDMI_MAINUNIT_0_INT_STATUS);

	if ((mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_REGBANK_READY) &&
		(!hrx_dev->regbank_ready)) {
		hrx_dev->regbank_ready = true;
	}

	if ((mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_APB_IF_READY) &&
		(!hrx_dev->apb_if_ready)) {
		hrx_dev->apb_if_ready = true;
	}

	return 0;
}

static int hrx_wait_ready(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int timeout = 50;

	while ((!hrx_dev->regbank_ready || !hrx_dev->apb_if_ready) &&
			timeout--) {
		hrx_hard_irq_handler(hrx_dev);
		mdelay(10);
	}

	if (!hrx_dev->regbank_ready || !hrx_dev->apb_if_ready)
		return -ETIMEDOUT;
	return 0;
}

static void hrx_hdmi_handler(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 mu0_stat = hrx_reg_get_int_val(hrx_dev,
			HDMI_MAINUNIT_0_INT_STATUS,
			HDMI_MAINUNIT_0_INT_MASK_N);
	u32 cmu_stat = hrx_reg_read(hrx_dev, HDMI_CMU_STATUS);

	if ((mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_TIMER_BASE_LOCKED) &&
		(cmu_stat & HDMI_CMU_STATUS_TIMER_BASE_LOCKED))
		hrx_dev->timer_base_locked = true;

	if (!hrx_dev->regbank_ready || !hrx_dev->apb_if_ready)
		return;

	hrx_clear_ints(hrx_dev);
}

static int hrx_set_timer_base(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int timeout = 50;
	u32 iref_clk = HRX_REF_CLK * 1000000;

	hrx_dev->timer_base_locked = false;

	hrx_reg_mask_write(hrx_dev, 0x1,
		HDMI_MAINUNIT_0_INT_CLEAR,
		HDMI_MAINUNIT_0_INT_CLEAR_TIMER_BASE_LOCKED_OFFSET,
		HDMI_MAINUNIT_0_INT_CLEAR_TIMER_BASE_LOCKED_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1,
		HDMI_MAINUNIT_0_INT_MASK_N,
		HDMI_MAINUNIT_0_INT_MASK_N_TIMER_BASE_LOCKED_OFFSET,
		HDMI_MAINUNIT_0_INT_MASK_N_TIMER_BASE_LOCKED_MASK);
	hrx_reg_write(hrx_dev, iref_clk & HDMI_GLOBAL_TIMER_REF_BASE_MASK,
		HDMI_GLOBAL_TIMER_REF_BASE);

	while (!hrx_dev->timer_base_locked && timeout--) {
		hrx_hdmi_handler(hrx_dev);
		mdelay(10);
	}

	hrx_reg_mask_write(hrx_dev, 0x0,
		HDMI_MAINUNIT_0_INT_MASK_N,
		HDMI_MAINUNIT_0_INT_MASK_N_TIMER_BASE_LOCKED_OFFSET,
		HDMI_MAINUNIT_0_INT_MASK_N_TIMER_BASE_LOCKED_MASK);

	if (!hrx_dev->timer_base_locked)
		return -ETIMEDOUT;
	HRX_LOG(HRX_DRV_DEBUG, "timer_base_locked\n");
	return 0;
}

bool hrx_is_hdmi2(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return ((hrx_reg_read(hrx_dev, HDMI_SCDC_REGBANK_STATUS1) &
		HDMI_SCDC_REGBANK_STATUS1_TMDSBITCLKRATIO) ==
		HDMI_SCDC_REGBANK_STATUS1_TMDSBITCLKRATIO);
}

static bool hrx_tmds_valid(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return ((hrx_reg_read(hrx_dev, HDMI_MAINUNIT_STATUS) &
			HDMI_MAINUNIT_STATUS_TMDSVALID_STABLE) ==
			HDMI_MAINUNIT_STATUS_TMDSVALID_STABLE);
}

static void hrx_set_hdcp_version(struct syna_hrx_v4l2_dev *hrx_dev,
				HRX_HDCP_VERSION version)
{
	switch (version) {
	case HRX_HDCP_VERSION_AUTO:
		HRX_LOG(HRX_DRV_DEBUG, "setting hdcp version to auto\n");
		/* disable override of version */
		hrx_reg_mask_write(hrx_dev, 0x0, HDMI_HDCP22_CONFIG, 1, BIT(1));
		break;

	case HRX_HDCP_VERSION_14:
		HRX_LOG(HRX_DRV_DEBUG, "restrict hdcp version to hdcp 1.4\n");
		/* override the version */
		hrx_reg_mask_write(hrx_dev, 0x0, HDMI_HDCP22_CONFIG, 2, BIT(2));
		hrx_reg_mask_write(hrx_dev, 0x1, HDMI_HDCP22_CONFIG, 1, BIT(1));
		break;

	default:
		HRX_LOG(HRX_DRV_ERROR, "invalid version selection : %d\n", version);
	}
}

static void hrx_reset_datapath(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_reg_write(hrx_dev, HDMI_GLOBAL_SWRESET_REQUEST_DATAPATH,
			HDMI_GLOBAL_SWRESET_REQUEST);
}

static int hrx_query_dv_timings(struct syna_hrx_v4l2_dev *hrx_dev,
		struct v4l2_dv_timings *timings)
{
	struct v4l2_bt_timings *bt = &timings->bt;
	bool tmds_valid = 0;
	u32 dframe_stat = 0;

	memset(timings, 0, sizeof(*timings));
	tmds_valid = hrx_tmds_valid(hrx_dev);

	HRX_LOG(HRX_DRV_DEBUG, "is_tmds_valid = %d\n", tmds_valid);
	mdelay(40);

	/* workaround added for random BSTATUS HDMI_MODE=0 error on
	 * HDCP1.4 repeater CTS tests
	 */
	bt->height = hrx_reg_mask_read(hrx_dev,
		HDMI_VMON_STATUS6,
		HDMI_VMON_STATUS6_VACTIVE_OFFSET,
		HDMI_VMON_STATUS6_VACTIVE_MASK);
	dframe_stat = hrx_reg_read(hrx_dev, HDMI_DEFRAMER_STATUS);
	HRX_LOG(HRX_DRV_DEBUG, "dframe_stat = 0x%x, height = %d\n",
					dframe_stat, bt->height);

	if (bt->height < TIMING_SANITY_CHECK_HEIGHT) {
		HRX_LOG(HRX_DRV_INFO, "re-init phy as unexpected timings detected.\n");
		hrx_phy_re_init(hrx_dev);
		return -1;
	}

	if (!tmds_valid && !hrx_tmds_valid(hrx_dev)) {
		HRX_LOG(HRX_DRV_ERROR, "Failed to get tmds_valid stable\n");
		return -1;
	}

	/* timings->type = V4L2_DV_BT_656_1120; */
	bt->width = hrx_reg_mask_read(hrx_dev,
		HDMI_VMON_STATUS3,
		HDMI_VMON_STATUS3_HACTIVE_OFFSET,
		HDMI_VMON_STATUS3_HACTIVE_MASK);
	bt->height = hrx_reg_mask_read(hrx_dev,
		HDMI_VMON_STATUS6,
		HDMI_VMON_STATUS6_VACTIVE_OFFSET,
		HDMI_VMON_STATUS6_VACTIVE_MASK);
	bt->hfrontporch = hrx_reg_mask_read(hrx_dev,
		HDMI_VMON_STATUS1,
		HDMI_VMON_STATUS1_HFRONT_OFFSET,
		HDMI_VMON_STATUS1_HFRONT_MASK);
	bt->hsync = hrx_reg_mask_read(hrx_dev,
		HDMI_VMON_STATUS1,
		HDMI_VMON_STATUS1_HSYNCWIDTH_OFFSET,
		HDMI_VMON_STATUS1_HSYNCWIDTH_MASK);
	bt->hbackporch = hrx_reg_mask_read(hrx_dev,
		HDMI_VMON_STATUS2,
		HDMI_VMON_STATUS2_HBACK_OFFSET,
		HDMI_VMON_STATUS2_HBACK_MASK);
	bt->vfrontporch = hrx_reg_mask_read(hrx_dev,
		HDMI_VMON_STATUS4,
		HDMI_VMON_STATUS4_VFRONT_OFFSET,
		HDMI_VMON_STATUS4_VFRONT_MASK);
	bt->vsync = hrx_reg_mask_read(hrx_dev,
		HDMI_VMON_STATUS4,
		HDMI_VMON_STATUS4_VSYNCWIDTH_OFFSET,
		HDMI_VMON_STATUS4_VSYNCWIDTH_MASK);
	bt->vbackporch = hrx_reg_mask_read(hrx_dev,
		HDMI_VMON_STATUS5,
		HDMI_VMON_STATUS5_VBACK_OFFSET,
		HDMI_VMON_STATUS5_VBACK_MASK);
	bt->interlaced = (hrx_reg_read(hrx_dev, HDMI_VMON_STATUS7) &
		HDMI_VMON_STATUS7_ILACE_DETECT) ?
		V4L2_DV_INTERLACED : V4L2_DV_PROGRESSIVE;

	HRX_LOG(HRX_DRV_INFO, "width = %d, hfrontporch = %d, hsync = %d, hbackporch = %d\n",
		bt->width, bt->hfrontporch, bt->hsync, bt->hbackporch);
	HRX_LOG(HRX_DRV_INFO, "height = %d, vfrontporch = %d, vsync = %d, vbackporch = %d\n",
		bt->height, bt->vfrontporch, bt->vsync, bt->vbackporch);
	HRX_LOG(HRX_DRV_INFO, "interlaced =%d\n", bt->interlaced);

	return 0;
}

int hrx_get_refresh_rate_by_resolution(hrx_display_resolution resolution)
{
	int refresh_rate = 0;
	switch (resolution) {
	case HRX_VGA_640X480_60HZ:
	case HRX_SD_720X480I_60HZ:
	case HRX_SD_720X480P_60HZ:
	case HRX_HD_1280X720P_60HZ:
	case HRX_HD_1920X1080I_60HZ:
	case HRX_HD_1920X1080P_60HZ:
	case HRX_HD_3840X2160P_60HZ:
	case HRX_HD_4096X2160P_60HZ:
	case HRX_HD_3840X2160P_60HZ_420:
	case HRX_HD_4096X2160P_60HZ_420:
		refresh_rate = 60*1000;
		break;
	case HRX_SD_720X480I_59_94HZ:
	case HRX_SD_720X480P_59_94HZ:
	case HRX_HD_1280X720P_59_94HZ:
	case HRX_HD_1920X1080I_59_94HZ:
	case HRX_HD_1920X1080P_59_94HZ:
	case HRX_HD_3840X2160P_59_94HZ:
	case HRX_HD_4096X2160P_59_94HZ:
	case HRX_HD_3840X2160P_59_94HZ_420:
	case HRX_HD_4096X2160P_59_94HZ_420:
		refresh_rate = 59940;
		break;
	case HRX_SD_720X576I_50HZ:
	case HRX_SD_720X576P_50HZ:
	case HRX_HD_1280X720P_50HZ:
	case HRX_HD_1920X1080I_50HZ:
	case HRX_HD_1920X1080P_50HZ:
	case HRX_HD_3840X2160P_50HZ:
	case HRX_HD_4096X2160P_50HZ:
	case HRX_HD_3840X2160P_50HZ_420:
	case HRX_HD_4096X2160P_50HZ_420:
		refresh_rate = 50*1000;
		break;
	case HRX_HD_1280X720P_30HZ:
	case HRX_HD_1920X1080P_30HZ:
	case HRX_HD_3840X2160P_30HZ:
	case HRX_HD_4096X2160P_30HZ:
		refresh_rate = 30*1000;
		break;
	case HRX_HD_1920X1080P_29_97HZ:
	case HRX_HD_3840X2160P_29_97HZ:
	case HRX_HD_4096X2160P_29_97HZ:
		refresh_rate = 29970;
		break;
	case HRX_HD_1280X720P_25HZ:
	case HRX_HD_1920X1080P_25HZ:
	case HRX_HD_3840X2160P_25HZ:
	case HRX_HD_4096X2160P_25HZ:
		refresh_rate = 25*1000;
		break;
	case HRX_HD_1280X720P_24HZ:
	case HRX_HD_1920X1080P_24HZ:
	case HRX_HD_3840X2160P_24HZ:
	case HRX_HD_4096X2160P_24HZ:
		refresh_rate = 24*1000;
		break;
	case HRX_HD_1920X1080P_23_97HZ:
	case HRX_HD_3840X2160P_23_98HZ:
	case HRX_HD_4096X2160P_23_98HZ:
		refresh_rate = 23980;
		break;
	case HRX_HD_1920X1080P_100HZ:
	case HRX_HD_1280X720P_100HZ:
	case HRX_SD_720X576P_100HZ:
		refresh_rate = 100*1000;
		break;
	case HRX_HD_1920X1080P_119_88HZ:
	case HRX_HD_1920X1080P_120HZ:
	case HRX_HD_1280X720P_120HZ:
	case HRX_SD_720X480P_120HZ:
		refresh_rate = 120*1000;
		break;
	case HRX_SD_720X480P_240HZ:
		refresh_rate = 240*1000;
		break;
	case HRX_SD_720X576P_200HZ:
		refresh_rate = 200*1000;
		break;
	default:
		refresh_rate = 60*1000;
		break;
	}

		return refresh_rate;
}


int hrx_get_tg_params(struct syna_hrx_v4l2_dev *hrx_dev, VIP_TG_PARAMS *pParams)
{
	int r = 0;
	hrx_display_resolution res = hrx_dev->video_params.HrxIpTimingParam.DispResId;
	hrx_sig_video_format resMode = hrx_dev->video_params.VideoFrmt;

	HRX_LOG(HRX_DRV_DEBUG, "enter, res = %d, resMode = %d\n", res, resMode);

	pParams->field_flag = 0;
	pParams->mode_3d = 0;
	pParams->sync_type = 0;

	switch (res) {
	case HRX_VGA_640X480_60HZ:
		pParams->start_x = 48;
		pParams->end_x = 688;
		pParams->start_y = 33;
		pParams->end_y = 513;
		pParams->vsamp = 0x10;
		pParams->field_flag = 0;
		pParams->mode_3d = 0;
		pParams->htotal = 800;
		pParams->vtotal = 525;
		break;
	case HRX_SD_720X576I_50HZ:
		pParams->start_x = 69;
		pParams->end_x = 789;
		pParams->start_y = 19;
		pParams->end_y = 307;
		pParams->vsamp = 0x10;
		pParams->field_flag = 1;
		pParams->htotal = 864;
		pParams->vtotal = 625;
		break;
	case HRX_SD_720X480I_60HZ:
	case HRX_SD_720X480I_59_94HZ:
		pParams->start_x = 57;
		pParams->end_x = 777;
		pParams->start_y = 15;
		pParams->end_y = 255;
		pParams->vsamp = 0x10;
		pParams->field_flag = 1;
		pParams->mode_3d = 0;
		pParams->htotal = 858;
		pParams->vtotal = 525;
		break;
	case HRX_SD_720X576P_50HZ:
	case HRX_SD_720X576P_100HZ:
	case HRX_SD_720X576P_200HZ:
		pParams->start_x = 68;
		pParams->end_x = 788;
		pParams->start_y = 39;
		pParams->end_y = 615;
		pParams->vsamp = 0x10;
		pParams->field_flag = 0;
		pParams->mode_3d = resMode;
		pParams->htotal = 864;
		pParams->vtotal = 625;
		break;
	case HRX_SD_720X480P_60HZ:
	case HRX_SD_720X480P_59_94HZ:
	case HRX_SD_720X480P_120HZ:
	case HRX_SD_720X480P_240HZ:
		pParams->start_x = 60;
		pParams->end_x = 780;
		pParams->start_y = 30;
		pParams->end_y = 510;
		pParams->vsamp = 0x10;
		pParams->field_flag = 0;
		pParams->mode_3d = resMode;
		pParams->htotal = 858;
		pParams->vtotal = 525;
		break;
	case HRX_HD_1280X720P_30HZ:
	case HRX_HD_1280X720P_25HZ:
	case HRX_HD_1280X720P_60HZ:
	case HRX_HD_1280X720P_59_94HZ:
	case HRX_HD_1280X720P_50HZ:
	case HRX_HD_1280X720P_24HZ:
	case HRX_HD_1280X720P_100HZ:
	case HRX_HD_1280X720P_120HZ:
		pParams->start_x = 260;
		pParams->end_x = 1540;
		pParams->start_y = 25;
		pParams->end_y = 745;
		pParams->vsamp = 0x40;
		pParams->field_flag = 0;
		pParams->mode_3d = resMode;
		pParams->htotal = 1950;
		pParams->vtotal = 750;
		break;
	case HRX_HD_1920X1080I_60HZ:
	case HRX_HD_1920X1080I_59_94HZ:
	case HRX_HD_1920X1080I_50HZ:
		pParams->start_x = 192;
		pParams->end_x = 2112;
		pParams->start_y = 20;
		pParams->end_y = 560;
		pParams->vsamp = 0x80;
		pParams->field_flag = 1;
		pParams->mode_3d = resMode;
		pParams->htotal = 2200;
		pParams->vtotal = 1124;
		break;
	case HRX_HD_1920X1080P_30HZ:
	case HRX_HD_1920X1080P_29_97HZ:
	case HRX_HD_1920X1080P_25HZ:
	case HRX_HD_1920X1080P_24HZ:
	case HRX_HD_1920X1080P_23_97HZ:
	case HRX_HD_1920X1080P_60HZ:
	case HRX_HD_1920X1080P_59_94HZ:
	case HRX_HD_1920X1080P_50HZ:
	case HRX_HD_1920X1080P_100HZ:
	case HRX_HD_1920X1080P_119_88HZ:
	case HRX_HD_1920X1080P_120HZ:
		pParams->start_x = 192;
		pParams->end_x = 2112;
		pParams->start_y = 41;
		pParams->end_y = 1121;
		pParams->vsamp = 0x80;
		pParams->field_flag = 0;
		pParams->htotal = 2200;
		pParams->vtotal = 1125;
		pParams->mode_3d = resMode;
		pParams->htotal = 2200;
		pParams->vtotal = 1125;
		break;
	case HRX_HD_3840X2160P_23_98HZ:
	case HRX_HD_3840X2160P_24HZ:
	case HRX_HD_4096X2160P_23_98HZ:
	case HRX_HD_4096X2160P_24HZ:
	case HRX_HD_3840X2160P_25HZ:
	case HRX_HD_4096X2160P_25HZ:
	case HRX_HD_3840X2160P_29_97HZ:
	case HRX_HD_3840X2160P_30HZ:
	case HRX_HD_4096X2160P_29_97HZ:
	case HRX_HD_4096X2160P_30HZ:
	case HRX_HD_3840X2160P_50HZ:
	case HRX_HD_4096X2160P_50HZ:
	case HRX_HD_3840X2160P_59_94HZ:
	case HRX_HD_3840X2160P_60HZ:
	case HRX_HD_4096X2160P_59_94HZ:
	case HRX_HD_4096X2160P_60HZ:
	case HRX_HD_3840X2160P_50HZ_420:
	case HRX_HD_4096X2160P_50HZ_420:
	case HRX_HD_3840X2160P_59_94HZ_420:
	case HRX_HD_3840X2160P_60HZ_420:
	case HRX_HD_4096X2160P_59_94HZ_420:
	case HRX_HD_4096X2160P_60HZ_420:
		pParams->start_x = 384;
		pParams->end_x = 4224;
		pParams->start_y = 82;
		pParams->end_y = 2242;
		pParams->vsamp = 0x80;
		pParams->field_flag = 0;
		pParams->mode_3d = 0;
		pParams->htotal = 4400;
		pParams->vtotal = 2250;
		break;
	default:
		HRX_LOG(HRX_DRV_INFO, "doesn't support input resolution, resID:%d\n", res);
			r = -1;
		break;
	}

	//set frame rate
	pParams->refresh_rate = hrx_get_refresh_rate_by_resolution(res);
	pParams->pixel_freq = pParams->htotal * pParams->vtotal * pParams->refresh_rate;
	return r;
}

static void hrx_audio_pll_init(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int timeout = 10;
	int pll_ready = 0, pll_lock = 0;
	T32HDMI_RX_WRAP_AUDIO_PLL_CTRL AudPllCtrl;


	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_GLOBAL_SWENABLE,
		HDMI_GLOBAL_SWENABLE_AUDIOPLL_ENABLE_OFFSET,
		HDMI_GLOBAL_SWENABLE_AUDIOPLL_ENABLE_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_OFFSET,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_MASK);
	hrx_reg_write(hrx_dev, 0x01, HDMI_AUDIO_FIFO_CONTROL);
	hrx_reg_write(hrx_dev, 0x01, HDMI_AUDIO_FIFO_CONFIG);
	hrx_reg_write(hrx_dev, 0x2, HDMI_AUDIO_PROC_CONFIG0);
	hrx_reg_write(hrx_dev, 0x10, HDMI_AUDIO_PROC_CONFIG1);
	hrx_reg_write(hrx_dev, 0x1f103f3, HDMI_AUDIO_PROC_CONFIG2);
	hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_OFFSET,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_MASK);
	HRX_LOG(HRX_DRV_DEBUG, "tmds_valid = %d\n", hrx_tmds_valid(hrx_dev));

	AudPllCtrl.reg = glb_reg_read(HDMIRX_WRAP_REG_BASE+RA_HDMI_RX_WRAP_AUDIO_PLL_CTRL);
	AudPllCtrl.uAUDIO_PLL_CTRL_hdmiRxAudPLL_itest_rst_n = 0;
	glb_reg_write(HDMIRX_WRAP_REG_BASE+RA_HDMI_RX_WRAP_AUDIO_PLL_CTRL, AudPllCtrl.reg);
	udelay(2);
	AudPllCtrl.uAUDIO_PLL_CTRL_hdmiRxAudPLL_itest_rst_n = 1;
	glb_reg_write(HDMIRX_WRAP_REG_BASE+RA_HDMI_RX_WRAP_AUDIO_PLL_CTRL, AudPllCtrl.reg);

	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
		HDMI_PKT_0_INT_MASK_N_ACR_OFFSET,
		HDMI_PKT_0_INT_MASK_N_ACR_MASK);

	hrx_reg_mask_write(hrx_dev, 0xC8, SNPS_AUDPLL_CONFIG1,
		SNPS_AUDPLL_CONFIG1_TARG_FREF_OFFSET,
		SNPS_AUDPLL_CONFIG1_TARG_FREF_MASK);
	hrx_reg_mask_write(hrx_dev, 0x4, SNPS_AUDPLL_CONFIG1,
		SNPS_AUDPLL_CONFIG1_NPIXEL_MODE_OFFSET,
		SNPS_AUDPLL_CONFIG1_NPIXEL_MODE_MASK);

	hrx_reg_mask_write(hrx_dev, REFCLK_COUNT, SNPS_AUDPLL_CONFIG2,
		SNPS_AUDPLL_CONFIG2_REFCLK_CNT_OFFSET,
		SNPS_AUDPLL_CONFIG2_REFCLK_CNT_MASK);

	hrx_reg_write(hrx_dev, 0x1030, SNPS_AUDPLL_CONFIG3);

	HRX_LOG(HRX_DRV_DEBUG, " ******  Waiting for SNPS AUDPLL DigInterface to get stable.... ******");
	pll_ready = (hrx_reg_read(hrx_dev, SNPS_AUDPLL_STATUS) & 0x02);
	while ((pll_ready != 2) && timeout--) {
		udelay(2);
		pll_ready = (hrx_reg_read(hrx_dev, SNPS_AUDPLL_STATUS) & 0x02);
	}
	pll_ready = 0;
	timeout = 10;
	udelay(2);
   //Wait for the 2nd posedge of this signal to guarantee that the tmdsclkcounter value has stabilized and avoid warnings
	pll_ready = (hrx_reg_read(hrx_dev, SNPS_AUDPLL_STATUS) & 0x02);
	while ((pll_ready != 2) && timeout--) {
		udelay(2);
		pll_ready = (hrx_reg_read(hrx_dev, SNPS_AUDPLL_STATUS) & 0x02);
	}

	HRX_LOG(HRX_DRV_DEBUG, " pll_ready = %d\n", pll_ready);
	hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_OFFSET,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_MASK);
	udelay(5);
	hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_PWRON_OFFSET,
		SNPS_AUDPLL_CONFIG3_PWRON_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_REFPLLRST_N_OFFSET,
		SNPS_AUDPLL_CONFIG3_REFPLLRST_N_MASK);
	udelay(5);
	hrx_reg_mask_write(hrx_dev, 0x0, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_OFFSET,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_MASK);
	udelay(5);
	hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_ENP_OFFSET,
		SNPS_AUDPLL_CONFIG3_ENP_MASK);
	udelay(2);
	HRX_LOG(HRX_DRV_DEBUG, " ****** Waiting for SNPS AUDPLL lock...  ******");
	pll_lock = (hrx_reg_read(hrx_dev, SNPS_AUDPLL_STATUS) & 0x01);
	timeout = 500;
	while ((pll_lock != 1) && timeout--) {

		udelay(2);
		pll_lock = (hrx_reg_read(hrx_dev, SNPS_AUDPLL_STATUS) & 0x01);
	}

	HRX_LOG(HRX_DRV_DEBUG, " pll_lock = %d\n", pll_lock);
	hrx_reg_mask_write(hrx_dev, 0x0, HDMI_PKT_0_INT_MASK_N,
		HDMI_PKT_0_INT_MASK_N_ACR_OFFSET,
		HDMI_PKT_0_INT_MASK_N_ACR_MASK);
}

static void hrx_reset_frame_interval(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_dev->video_params.FITotal.numerator = 1;
	hrx_dev->video_params.FITotal.denominator = 1;
	hrx_dev->video_params.FIActive.numerator = 1;
	hrx_dev->video_params.FIActive.denominator = 1;
	hrx_dev->video_params.FIRequested.numerator = 1;
	hrx_dev->video_params.FIRequested.denominator = 1;
}

static void hrx_update_total_frame_interval(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 RefRate = 0;

	RefRate = hrx_get_refresh_rate_by_resolution(hrx_dev->video_params.HrxIpTimingParam.DispResId);
	RefRate += 500;

	hrx_dev->video_params.FITotal.numerator = 1;
	hrx_dev->video_params.FITotal.denominator = RefRate/1000;
	hrx_dev->frame_count = 0;

	// Updated buffer display index w.r.t. Active FPS
	hrx_dev->display_frame_index = (hrx_dev->video_params.FITotal.denominator / hrx_dev->video_params.FIActive.denominator);

	HRX_LOG(HRX_DRV_INFO,
			"HRX[%s] Total %d, Requested %d, Active %d, Display Index %d\n", __func__,
			hrx_dev->video_params.FITotal.denominator,
			hrx_dev->video_params.FIRequested.denominator,
			hrx_dev->video_params.FIActive.denominator,
			hrx_dev->display_frame_index);
}

void hrx_update_active_frame_interval(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int i = 0;
	int requestedDenominator = hrx_dev->video_params.FIRequested.denominator;
	int totalDenominator = hrx_dev->video_params.FITotal.denominator;
	int activeDenominator = hrx_dev->video_params.FIActive.denominator;

	if(hrx_dev->video_params.FIRequested.numerator == hrx_dev->video_params.FIActive.numerator)
	{
		if(requestedDenominator == totalDenominator)
			activeDenominator = requestedDenominator;
		else
		{
			if((totalDenominator % requestedDenominator) == 0)
				activeDenominator = requestedDenominator;
			else
			{
				for(i = 0; i < totalDenominator; i++)
				{
					if(totalDenominator % (requestedDenominator + i) == 0)
					{
						activeDenominator = requestedDenominator + i;
						break;
					}
					else if(totalDenominator % (requestedDenominator - i) == 0)
					{
						activeDenominator = requestedDenominator - i;
						break;
					}
					else
						activeDenominator = totalDenominator;
				}
			}
		}
		hrx_dev->video_params.FIActive.denominator = activeDenominator;
	}
	else
	{
		HRX_LOG(HRX_DRV_ERROR, "Frameinterval numerator %d is not supported\n", hrx_dev->video_params.FIRequested.numerator);
	}

	// Updated buffer display index w.r.t. Active FPS
	hrx_dev->display_frame_index = (hrx_dev->video_params.FITotal.denominator / hrx_dev->video_params.FIActive.denominator);

	HRX_LOG(HRX_DRV_INFO,
			"HRX[%s] Total %d, Requested %d, Active %d, Display Index %d\n", __func__,
			hrx_dev->video_params.FITotal.denominator,
			hrx_dev->video_params.FIRequested.denominator,
			hrx_dev->video_params.FIActive.denominator,
			hrx_dev->display_frame_index);
}

static void hrx_load_default_settings(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u8 edid[256];
	int ret = 0, isTmdsValid;
	u32 val;
	struct v4l2_dv_timings timings;

	hrx_dev->hdmi_state = HDMI_STATE_NO_INIT;
	hrx_dev->hdmi_cstate = HDMI_CSTATE_NOT_CONFIGURED;

	hrx_dev->tmds_valid_wait_count = 20;
	hrx_dev->has_clock_wait_ms = 0;
	hrx_dev->video_stable_wait_ms = 300;

	syna_hrx_set_hpd(hrx_dev, 0);
	syna_hrx_write_default_edid(hrx_dev);
	syna_hrx_read_edid(hrx_dev, edid);
	if (!syna_hrx_validate_edid(edid)) {
		HRX_LOG(HRX_DRV_ERROR, "EDID validation failed\n");
		return;
	};
	mdelay(200);
	syna_hrx_set_hpd(hrx_dev, 1);

	hrx_reset_all(hrx_dev);

	ret = hrx_wait_ready(hrx_dev);
	if (ret)
		HRX_LOG(HRX_DRV_ERROR, "Failed to wait for controller ready ret =%d\n", ret);

	mdelay(50);
	/* Basic integrity check */
	val = hrx_reg_read(hrx_dev, HDMI_CORE_ID);
	if (val != HDMI_CORE_ID_VALUE)
		HRX_LOG(HRX_DRV_ERROR, "!!!!!!!!!!! invalid CORE_ID=0x%x (expected 0x%x)\n",
				val, HDMI_CORE_ID_VALUE);

	/* Controller found and ready: disable interrupts and HPD */
	hrx_disable_all_ints(hrx_dev);
	hrx_clear_ints(hrx_dev);
	hrx_enable_scdc(hrx_dev);
	hrx_config_i2c(hrx_dev);
	hrx_enable_hpd(hrx_dev, 1);

	ret = hrx_set_timer_base(hrx_dev);
	if (ret)
		HRX_LOG(HRX_DRV_ERROR, "Failed to set reference timer base ret =%d\n", ret);

	mdelay(50);

	hrx_dev->phy_cfg_clk = 100;
	ret = hrx_phy_init(hrx_dev, hrx_is_hdmi2(hrx_dev), 1);
	mdelay(50);
	isTmdsValid = hrx_tmds_valid(hrx_dev);

	hrx_set_hdcp_version(hrx_dev, HRX_HDCP_VERSION_14);

	hrx_reg_mask_write(hrx_dev, 0x0,
		HDMI_HDCP14_CONFIG,
		HDMI_HDCP14_CONFIG_KEY_DECRYPT_EN_OFFSET,
		HDMI_HDCP14_CONFIG_KEY_DECRYPT_EN_MASK);
	hrx_reg_mask_write(hrx_dev, 0x0,
		HDMI_HDCP14_CONFIG,
		HDMI_HDCP14_CONFIG_EESS_OESS_SEL_OFFSET,
		HDMI_HDCP14_CONFIG_EESS_OESS_SEL_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1,
		HDMI_HDCP14_CONFIG,
		HDMI_HDCP14_CONFIG_HDMI_RSVD_OFFSET,
		HDMI_HDCP14_CONFIG_HDMI_RSVD_MASK);
	hrx_reg_mask_write(hrx_dev, 0x0,
		HDMI_HDCP14_CONFIG,
		HDMI_HDCP14_CONFIG_FEATURES_1DOT1_OFFSET,
		HDMI_HDCP14_CONFIG_FEATURES_1DOT1_MASK);
	hrx_reg_mask_write(hrx_dev, 0xC,
		HDMI_HDCP14_CONFIG,
		HDMI_HDCP14_CONFIG_EESS_CTL_THR_OFFSET,
		HDMI_HDCP14_CONFIG_EESS_CTL_THR_MASK);

	hrx_audio_pll_init(hrx_dev);
	hrx_reset_datapath(hrx_dev);
	mdelay(50);
	hrx_query_dv_timings(hrx_dev, &timings);
	hrx_reset_frame_interval(hrx_dev);
	hrx_get_infoframes(hrx_dev);
	hrx_dev->hdmi_state = HDMI_STATE_POWER_OFF;
}

static void hrx_enable_video_ints(struct syna_hrx_v4l2_dev *hrx_dev)
{
	/* Commented GCP interrupt to avoid Rx interrupt (aio_intr8))hang with QD Deep color input */
	/*	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
	 *		HDMI_PKT_0_INT_MASK_N_GCP_OFFSET,
	 *		HDMI_PKT_0_INT_MASK_N_GCP_MASK);
	 */
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
			HDMI_PKT_0_INT_MASK_N_AVIIF_OFFSET,
			HDMI_PKT_0_INT_MASK_N_AVIIF_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
			HDMI_PKT_0_INT_MASK_N_AUDIF_OFFSET,
			HDMI_PKT_0_INT_MASK_N_AUDIF_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
			HDMI_PKT_0_INT_MASK_N_SRCPDIF_OFFSET,
			HDMI_PKT_0_INT_MASK_N_SRCPDIF_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
			HDMI_PKT_0_INT_MASK_N_VSIF_OFFSET,
			HDMI_PKT_0_INT_MASK_N_VSIF_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
			HDMI_PKT_0_INT_MASK_N_DRMIF_OFFSET,
			HDMI_PKT_0_INT_MASK_N_DRMIF_MASK);
}

static void hrx_isr_enable(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 val;
	HDL_semaphore *pSemHandle;

	if (!hrx_dev)
		return;

	pSemHandle = dhub_semaphore(hrx_dev->dhub);
	HRX_REG_WORD32_READ(MEMMAP_AVIO_REG_BASE +
		AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_INTR_EN, &val);
	val |= MSK32HDMIRX_PIPE_INTR_EN_HDMIRX;
	HRX_REG_WORD32_WRITE(MEMMAP_AVIO_REG_BASE +
		AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_INTR_EN, val);

	semaphore_cfg(pSemHandle, avioDhubSemMap_aio64b_aio_intr8, 1, 0);
	semaphore_clr_full(pSemHandle, avioDhubSemMap_aio64b_aio_intr8);
	semaphore_intr_enable(pSemHandle, avioDhubSemMap_aio64b_aio_intr8,
	0/*empty*/, 1/*full*/, 0/*almost empty*/,
	0/*almost full*/, CPU_ID/*cpu id*/);
}


static void hrx_reset(struct syna_hrx_v4l2_dev *hrx_dev)
{
	HRX_LOG(HRX_DRV_DEBUG, "In this fn [%s]\n", __func__);
	hrx_disable_all_ints(hrx_dev);
	hrx_enable_vital_ints(hrx_dev);
	hrx_load_default_settings(hrx_dev);
	hrx_isr_enable(hrx_dev);
	hrx_disable_all_ints(hrx_dev);
	hrx_enable_vital_ints(hrx_dev);

	hrx_dev->audErrCnt = 0;
}

void hrx_intr_reset(struct syna_hrx_v4l2_dev *hrx_dev)
{
	HRX_LOG(HRX_DRV_DEBUG, "In this fn [%s]\n", __func__);
	hrx_disable_all_ints(hrx_dev);
	hrx_enable_vital_ints(hrx_dev);
}

static int hrx_init(struct syna_hrx_v4l2_dev *hrx_dev)
{

	hrx_dev->HrxState = HRX_STATE_DISCONNECTED;
	if (hrx_create_isr_task(hrx_dev) < 0) {
		HRX_LOG(HRX_DRV_ERROR, "hrx_create_isr_task failed\n");
		return -1;
	};

	if (vip_create_isr_task(hrx_dev) < 0) {
		HRX_LOG(HRX_DRV_ERROR, "vip_create_isr_task failed\n");
		return -1;
	};

	hrx_reset(hrx_dev);
	if (hrx_is_5v_connected(hrx_dev))
		hrx_dev->HrxState = HRX_STATE_UNSTABLE;

	return 0;
}

static void hrx_deinit(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_stop_isr_task(hrx_dev);
	vip_stop_isr_task(hrx_dev);
}

static int syna_hrx_init_dhub(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_dev->dhub_vpp = Dhub_GetDhubHandle_ByDhubId(DHUB_ID_VPP_DHUB);
	if (unlikely(hrx_dev->dhub_vpp == NULL)) {
		HRX_LOG(HRX_DRV_ERROR, "hrx_dev->dhub_vpp: get failed\n");
		return -1;
	}
	HRX_LOG(HRX_DRV_DEBUG, "hrx_dev->dhub_vpp: succcessfully\n");

	hrx_dev->dhub = Dhub_GetDhubHandle_ByDhubId(DHUB_ID_AG_DHUB);
	if (unlikely(hrx_dev->dhub == NULL)) {
		HRX_LOG(HRX_DRV_ERROR, "hrx_dev->dhub: get failed\n");
		return -1;
	}

	return 0;
}

static irqreturn_t syna_hrx_isr(int irq, void *arg)
{
	u32 intr_id;
	struct syna_hrx_v4l2_dev *hrx_dev = (struct syna_hrx_v4l2_dev *) arg;
	u32 avp1_stat, avp1_mask, avp1_mask_stat;
	int ret;
	u32 uiInstate;
	HDL_semaphore *pSemHandle = NULL;

	intr_id = (u32)irqd_to_hwirq(irq_get_irq_data(irq));
	pSemHandle = dhub_semaphore(hrx_dev->dhub);
	uiInstate = semaphore_chk_full(pSemHandle, intr_id);

	if (uiInstate) {
		semaphore_pop(pSemHandle, intr_id, 1);
		semaphore_clr_full(pSemHandle, intr_id);
		HRX_LOG(HRX_DRV_DEBUG, "semaphore_pop and clr_full for intr_id = %d\n", intr_id);
	}

	switch (intr_id) {
	case avioDhubSemMap_aio64b_aio_intr7:
	case avioDhubSemMap_aio64b_aio_intr9:
	case avioDhubSemMap_aio64b_aio_intr10:
	case avioDhubSemMap_aio64b_aio_intr11:
	{
		vip_isr_handler(hrx_dev, intr_id);
		ret = IRQ_HANDLED;
		break;
	}
	case avioDhubSemMap_aio64b_aio_intr8:
	{
		avp1_stat = hrx_reg_read(hrx_dev, HDMI_AVPUNIT_1_INT_STATUS);
		avp1_mask = hrx_reg_read(hrx_dev, HDMI_AVPUNIT_1_INT_MASK_N);
		avp1_mask_stat = (avp1_stat & avp1_mask);
		if (avp1_mask_stat & HDMI_AVPUNIT_1_INT_MASK_N_DEFRAMER_VSYNC_MASK)
			vip_isr_handler(hrx_dev, intr_id);

		hrx_isr_handler(hrx_dev);
		hrx_clear_ints(hrx_dev);
		ret = IRQ_HANDLED;
		break;
	}
	case avioDhubChMap_aio64b_MIC3_CH_W:
	{
		aip_resume_cmd(hrx_dev);
		aip_isr_handler(hrx_dev, intr_id);
		ret = IRQ_HANDLED;
		break;
	}
	default:
	{
		HRX_LOG(HRX_DRV_INFO, "Default: %u\n", intr_id);
		ret = IRQ_NONE;
		break;
	}
	}
	return ret;
}

static int syna_hrx_vpp_intr_handler(int irq, void *arg)
{
	int ret;
	u32 intr_num = ffs(irq) - 1;

	switch (intr_num) {
	case avioDhubSemMap_vpp128b_vpp_inr9:
	case avioDhubSemMap_vpp128b_vpp_inr10:
	{
		HRX_LOG(HRX_DRV_INFO, "VPP Ints Handling : %u\n", intr_num);
		ret = IRQ_HANDLED;
		break;
	}
	default:
	{
		HRX_LOG(HRX_DRV_ERROR, "Invalid Intr %u\n", intr_num);
		ret = IRQ_NONE;
		break;
	}
	}
	return ret;
}

static bool hrx_is_hdmi_mode(struct syna_hrx_v4l2_dev *hrx_dev)
{
	bool IsHDMIMode = FALSE;
	u32 val;
	u8 i;

	val = hrx_reg_read(hrx_dev, HDMI_DEFRAMER_STATUS);
	if ((val & HDMI_DEFRAMER_STATUS_AUTOHDMIDVI) != HDMI_DEFRAMER_STATUS_AUTOHDMIDVI) {
		for (i = 0; i < 4; i++) {
			mdelay(20);
			val = hrx_reg_read(hrx_dev, HDMI_DEFRAMER_STATUS);
			if ((val & HDMI_DEFRAMER_STATUS_AUTOHDMIDVI) == HDMI_DEFRAMER_STATUS_AUTOHDMIDVI)
				break;

			IsHDMIMode = (((val & HDMI_DEFRAMER_STATUS_OPMODE_MASK) >> HDMI_DEFRAMER_STATUS_OPMODE_OFFSET) != 0);
			HRX_LOG(HRX_DRV_DEBUG, "HDMI_DEFRAMER_STATUS = 0x%x IsHDMIMode = %d", val, IsHDMIMode);
		}
	}
	IsHDMIMode = (((val & HDMI_DEFRAMER_STATUS_OPMODE_MASK) >> HDMI_DEFRAMER_STATUS_OPMODE_OFFSET) != 0);
	HRX_LOG(HRX_DRV_DEBUG, "HDMI_DEFRAMER_STATUS = 0x%x IsHDMIMode = %d", val, IsHDMIMode);
	return IsHDMIMode;
}

static int hrx_try_read_vendor_pkt(struct syna_hrx_v4l2_dev *hrx_dev, u32 *VideoFrmt)
{
	union hdmi_vendor_any_infoframe *vendor;

	vendor = &hrx_dev->vsif.infoframe.general.vendor;
	if (!hrx_dev->vsif.valid) {
		vendor->hdmi.s3d_struct = HDMI_3D_STRUCTURE_INVALID;
		*VideoFrmt = 0;
		return -1;
	}
	if (vendor->hdmi.s3d_struct != HDMI_3D_STRUCTURE_INVALID) {
		*VideoFrmt = vendor->hdmi.s3d_struct;
		HRX_LOG(HRX_DRV_DEBUG, "HRX_DRV_INFO: Read Vendor Info Frame s3d_struct =%d\n", vendor->hdmi.s3d_struct);
	}
	if (vendor->hdmi.vic) {
		HRX_LOG(HRX_DRV_DEBUG, "HRX_DRV_INFO: Read Vendor Info Frame VIC =%d\n", vendor->hdmi.vic);
		*VideoFrmt = vendor->hdmi.vic;
	}
	return 0;
}

static int get_input_res_pr(struct syna_hrx_v4l2_dev *hrx_dev, hrx_display_resolution *pRes, u8 *pPr)
{
	u16 vic;
	hrx_display_resolution mode;
	union hdmi_vendor_any_infoframe *vendor;
	struct hdmi_avi_infoframe *avi;

	avi = &hrx_dev->aviif.infoframe.general.avi;
	vendor = &hrx_dev->vsif.infoframe.general.vendor;

	*pPr = avi->pixel_repeat;
	vic = avi->video_code;

	switch (vic) {
	case 1:
		mode = HRX_VGA_640X480_60HZ;
		break;
	case 2:
	case 3:
		mode = HRX_SD_720X480P_59_94HZ;
		break;
	case 4:
		mode = HRX_HD_1280X720P_59_94HZ;
		break;
	case 5:
		mode = HRX_HD_1920X1080I_59_94HZ;
		break;
	case 6:
	case 7:
		mode = HRX_SD_720X480I_59_94HZ;
		break;

	case 16:
		mode = HRX_HD_1920X1080P_59_94HZ;
		break;
	case 17:
	case 18:
		mode = HRX_SD_720X576P_50HZ;
		break;
	case 19:
		mode = HRX_HD_1280X720P_50HZ;
		break;
	case 20:
		mode = HRX_HD_1920X1080I_50HZ;
		break;
	case 21:
	case 22:
		mode = HRX_SD_720X576I_50HZ;
		break;

	case 31:
		mode = HRX_HD_1920X1080P_50HZ;
		break;
	case 32:
		mode = HRX_HD_1920X1080P_23_97HZ;
		break;
	case 33:
		mode = HRX_HD_1920X1080P_25HZ;
		break;
	case 34:
		mode = HRX_HD_1920X1080P_29_97HZ;
		break;

		////////////NEW VICS//////////////////

	case 8:
	case 9:
		mode = HRX_SD_1440X240P_59_94HZ;
		break;

	case 10:
	case 11:
		mode = HRX_SD_2880X480I_59_94HZ;
		break;

	case 12:
	case 13:
		mode = HRX_SD_2880X240P_59_94HZ;
		break;

	case 14:
	case 15:
		mode = HRX_SD_1440X480P_59_94HZ;
		break;

	case 23:
	case 24:
		mode = HRX_SD_1440X288P_50HZ;
		break;

	case 25:
	case 26:
		mode = HRX_SD_2880X576I_50HZ;
		break;

	case 27:
	case 28:
		mode = HRX_SD_2880X288P_50HZ;
		break;

	case 29:
	case 30:
		mode = HRX_SD_1440X576P_50HZ;
		break;

	case 35:
	case 36:
		mode = HRX_SD_2880X480P_59_94HZ;
		break;

	case 37:
	case 38:
		mode = HRX_SD_2880X576P_50HZ;
		break;

	case 39:
		mode = HRX_HD_1920X1250I_50HZ;
		break;

	case 40:
		mode = HRX_HD_1920X1080I_100HZ;
		break;

	case 41:
		mode = HRX_HD_1280X720P_100HZ;
		break;

	case 42:
	case 43:
		mode = HRX_SD_720X576P_100HZ;
		break;

	case 44:
	case 45:
		mode = HRX_SD_1440X576I_100HZ;
		break;

	case 46:
		mode = HRX_HD_1920X1080I_120HZ;
		break;

	case 47:
		mode = HRX_HD_1280X720P_120HZ;
		break;

	case 48:
	case 49:
		mode = HRX_SD_720X480P_120HZ;
		break;

	case 50:
	case 51:
		mode = HRX_SD_1440X480I_120HZ;
		break;

	case 52:
	case 53:
		mode = HRX_SD_720X576P_200HZ;
		break;

	case 54:
	case 55:
		mode = HRX_SD_720X576I_200HZ;
		break;

	case 56:
	case 57:
		mode = HRX_SD_720X480P_240HZ;
		break;

	case 58:
	case 59:
		mode = HRX_SD_1440X480I_240HZ;
		break;

	case 60:
		mode = HRX_HD_1280X720P_24HZ;
		break;

	case 61:
		mode = HRX_HD_1280X720P_25HZ;
		break;

	case 62:
		mode = HRX_HD_1280X720P_30HZ;
		break;

	case 93:
	case 103:
		mode = HRX_HD_3840X2160P_24HZ;
		break;

	case 94:
	case 104:
		mode = HRX_HD_3840X2160P_25HZ;
		break;

	case 95:
	case 105:
		mode = HRX_HD_3840X2160P_30HZ;
		break;

	case 96:
	case 106:
		mode = HRX_HD_3840X2160P_50HZ;
		break;

	case 97:
	case 107:
		mode = HRX_HD_3840X2160P_60HZ;
		break;

	case 98:
		mode = HRX_HD_4096X2160P_24HZ;
		break;

	case 99:
		mode = HRX_HD_4096X2160P_25HZ;
		break;

	case 100:
		mode = HRX_HD_4096X2160P_30HZ;
		break;

	case 101:
		mode = HRX_HD_4096X2160P_50HZ;
		break;

	case 102:
		mode = HRX_HD_4096X2160P_60HZ;
		break;

	default:
		mode = HRX_UNKNOWN_MODE;
		break;

	}

	if (mode == HRX_UNKNOWN_MODE) {
		u32 videofmt;

		hrx_try_read_vendor_pkt(hrx_dev, &videofmt);
		HRX_LOG(HRX_DRV_DEBUG, "VSIF videofmt = %d\n", videofmt);
		switch (videofmt) {
		case 3:
			mode = HRX_HD_3840X2160P_24HZ;
			break;

		case 2:
			mode = HRX_HD_3840X2160P_25HZ;
			break;

		case 1:
			mode = HRX_HD_3840X2160P_30HZ;
			break;

		default:
			mode = HRX_UNKNOWN_MODE;
			break;
		}
	}
	*pRes = mode;
	HRX_LOG(HRX_DRV_INFO, "HRX_DRV_INFO: VIC=%d, pixel repetition = %d mode =%d\n", vic, *pPr, mode);
	return 0;

}

static unsigned int hrx_get_color_depth(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 cd = hrx_reg_mask_read(hrx_dev, HDMI_VIDEO_STATUS,
			HDMI_VIDEO_STATUS_CD_CURRENT_OFFSET,
			HDMI_VIDEO_STATUS_CD_CURRENT_MASK);

	switch (cd) {
	case 0x4:
		return 24;
	case 0x5:
		return 30;
	case 0x6:
		return 36;
	case 0x7:
		return 48;
	default:
		return 24;
	}
}

static u32 hrx_read_gcp(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_hdmi_rx_col_depth_type RxColDepth =  HRX_HDMI_RX_COL_DEPTH_8BIT;
	u32 cd =  hrx_get_color_depth(hrx_dev);

	if (cd == 24)
		RxColDepth = HRX_HDMI_RX_COL_DEPTH_8BIT;
	else if (cd == 30)
		RxColDepth = HRX_HDMI_RX_COL_DEPTH_10BIT;
	else if (cd == 36)
		RxColDepth = HRX_HDMI_RX_COL_DEPTH_12BIT;
	else if (cd == 48)
		RxColDepth = HRX_HDMI_RX_COL_DEPTH_16BIT;

	return RxColDepth;

}

static void hrx_detect_hdmi_color_depth(struct syna_hrx_v4l2_dev *hrx_dev, phrx_hdmi_rx_col_depth_type pRxColDepth, u8 *PixRepValue)
{
	hrx_display_resolution DispId;

	get_input_res_pr(hrx_dev, &DispId, PixRepValue);
	*pRxColDepth = hrx_read_gcp(hrx_dev);

}

static u32 hrx_get_n(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 n, pb;

	/* need to read the header first */
	hrx_reg_read(hrx_dev, HDMI_PKTDEC_ACR_PH);

	pb = hrx_reg_read(hrx_dev, HDMI_PKTDEC_ACR_PB(1));
	/* pktdec_acr_pb7_4[3:0],[15:8],[23:16] */
	n = (pb & GENMASK(3, 0)) << 16 | (pb & GENMASK(15, 8)) |
		(pb & GENMASK(23, 16)) >> 16;
	HRX_LOG(HRX_DRV_DEBUG, "N=%d\n", n);
	if(n == 0) {
		/* since N value is 0 reset audio_params */
		memset(&hrx_dev->audio_params,0,sizeof(hrx_auto_detect_audio_params));
	}
	return n;
}

static INT hrx_get_refresh_rate(struct syna_hrx_v4l2_dev *hrx_dev, u32 *refresh_rate)
{
	u32 cd;
	u16 Htotal, Vtotal;
	u64 tmds_freq;
	u32 RefRate = 0;

	Htotal = hrx_reg_mask_read(hrx_dev, HDMI_VMON_STATUS3,
			HDMI_VMON_STATUS3_HTOTAL_OFFSET,
			HDMI_VMON_STATUS3_HTOTAL_MASK);
	Vtotal = hrx_reg_mask_read(hrx_dev, HDMI_VMON_STATUS6,
			HDMI_VMON_STATUS6_VTOTAL_OFFSET,
			HDMI_VMON_STATUS6_VTOTAL_MASK);

	tmds_freq = hrx_get_tmds_clk(hrx_dev);
	//Compute Refresh rate
	if((Htotal > 0) && (Vtotal > 0))
		RefRate = (tmds_freq*100)/(Htotal * Vtotal); /* scale to 100 */

	cd =  hrx_get_color_depth(hrx_dev);
	switch(cd)
	{
		case 36:
			RefRate = ((RefRate * 3) >> 1);
			break;
		case 30:
			RefRate = ((RefRate * 5) >> 2);
			break;
	default:
			break;
	}
	*refresh_rate = RefRate;
	return 0;
}

/* Added to resolve no audio during resolution switch */
static void hrx_audio_pll_config_static_in(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 ref_rate; /* scale to 100 */
	int cd, size, i;
	int ref_count = REFCLK_COUNT;

	hrx_get_refresh_rate(hrx_dev, &ref_rate);
	cd = hrx_get_color_depth(hrx_dev);

	size = sizeof(refclk_data)/sizeof(struct refclk_config);
	for (i = 0;i < size;i++) {
		if((hrx_dev->aviif.infoframe.general.avi.video_code == refclk_data[i].vic) &&
			((ref_rate >= (refclk_data[i].ref_rate-1)) && (ref_rate <= (refclk_data[i].ref_rate+1)))) {
			if ((refclk_data[i].cd == -1) || (refclk_data[i].cd == cd)) {
				ref_count += refclk_data[i].refclk_change;
			}
		}
	}

	HRX_LOG(HRX_DRV_DEBUG, "REFCLK_CNT set to 0x%X\n", ref_count);
	hrx_reg_mask_write(hrx_dev, ref_count, SNPS_AUDPLL_CONFIG2,
		SNPS_AUDPLL_CONFIG2_REFCLK_CNT_OFFSET,
		SNPS_AUDPLL_CONFIG2_REFCLK_CNT_MASK);
}

static void hrx_audio_pll_config_dynamic_in(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int timeout;
	unsigned int pll_lock;

	hrx_reg_mask_write(hrx_dev, 0x0, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_REFPLLRST_N_OFFSET,
		SNPS_AUDPLL_CONFIG3_REFPLLRST_N_MASK);
	hrx_reg_mask_write(hrx_dev, 0x0, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_PWRON_OFFSET,
		SNPS_AUDPLL_CONFIG3_PWRON_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_STANDBY_OFFSET,
		SNPS_AUDPLL_CONFIG3_STANDBY_MASK);
	udelay(5);

	hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_OFFSET,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_MASK);

	udelay(5);
	hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_REFPLLRST_N_OFFSET,
		SNPS_AUDPLL_CONFIG3_REFPLLRST_N_MASK);
	hrx_reg_mask_write(hrx_dev, 0x0, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_STANDBY_OFFSET,
		SNPS_AUDPLL_CONFIG3_STANDBY_MASK);
	udelay(5);
	hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_PWRON_OFFSET,
		SNPS_AUDPLL_CONFIG3_PWRON_MASK);


	hrx_reg_mask_write(hrx_dev, 0x0, SNPS_AUDPLL_CONFIG3,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_OFFSET,
		SNPS_AUDPLL_CONFIG3_GEAR_SHIFT_MASK);

	if (!has_audio_clock(hrx_dev)) {
		HRX_LOG(HRX_DRV_DEBUG, "Audio clock off\n");
		hrx_reg_mask_write(hrx_dev, 0x0, SNPS_AUDPLL_CONFIG3,
			SNPS_AUDPLL_CONFIG3_REFPLLRST_N_OFFSET,
			SNPS_AUDPLL_CONFIG3_REFPLLRST_N_MASK);
		udelay(20);
		hrx_reg_mask_write(hrx_dev, 0x1, SNPS_AUDPLL_CONFIG3,
			SNPS_AUDPLL_CONFIG3_REFPLLRST_N_OFFSET,
			SNPS_AUDPLL_CONFIG3_REFPLLRST_N_MASK);
	}
	pll_lock = (hrx_reg_read(hrx_dev, SNPS_AUDPLL_STATUS) & 0x01);
	timeout = 500;
	while ((pll_lock != 1) && timeout--) {
		udelay(2);
		pll_lock = (hrx_reg_read(hrx_dev, SNPS_AUDPLL_STATUS) & 0x01);
	}
	if (!timeout) {
		HRX_LOG(HRX_DRV_ERROR, "Audio pll lock error. pll lock : 0x%X, audio clock : %d\n",
			hrx_reg_read(hrx_dev, SNPS_AUDPLL_STATUS), has_audio_clock(hrx_dev));
	}
}

static void range_convert_refrate_hdmi(u32 RefRate, hrx_refresh_rate *RefRateIndex)
{
	unsigned char Index;

	*RefRateIndex = HRX_RR_INVALID;

	// Match the RefreshRate from the RefreshRateTable.
	for (Index = 0; Index < HRX_RR_MAX_RANGE; Index++) {
		if ((RefRate >= RefreshRateTableVideo[Index].RefCountMin) &&
				(RefRate <= RefreshRateTableVideo[Index].RefCountMax)) {
			*RefRateIndex = RefreshRateTableVideo[Index].RefRate;
			break;
		}
	}
}

static bool hdmi_rx_look_mode_param_table(u16 HTot,
		u16 VTot,
		hrx_refresh_rate RefRate,
		u16 LoopCnt,
		hrx_refresh_rate *pRefreshRate,
		hrx_application_timing_params *pModeParamTable,
		hrx_display_resolution *DispResId)
{
	u16 Index;
	bool ModeDet = FALSE;

	for (Index = 0; Index < LoopCnt; Index++) {
		if (IsValueInRange(VTot, pModeParamTable[Index].VTotal, TIMING_TOL_VAL)) {
			if (IsValueInRange(HTot, pModeParamTable[Index].HTotal, TIMING_TOL_VAL)) {
				if (RefRate == pModeParamTable[Index].RefreshRate) {
					ModeDet = TRUE;
					break;
				}
			}
		}
	}

	if (ModeDet) {
		*DispResId = pModeParamTable[Index].DispResId;
		*pRefreshRate = pModeParamTable[Index].RefreshRate;
	} else
		*DispResId = HRX_UNKNOWN_MODE;

	return ModeDet;
}


static int hrx_hdmi_detect_mode(void *Data, hrx_refresh_rate *pRefreshRate, hrx_display_resolution *DispResId)
{
	hrx_application_timing_params *pModeParamTable = NULL;
	hrx_signal_param_status *pDetectionParam;
	bool   ModeDet = FALSE;
	u16 LastIndex = 0;
	u32 RawRefRate; /* scale to 100 */
	hrx_refresh_rate RefRate;

	pDetectionParam = (phrx_signal_param_status)Data;

	RawRefRate = pDetectionParam->RefRate;
	range_convert_refrate_hdmi(RawRefRate, &RefRate);


	pModeParamTable = (hrx_application_timing_params *)VideoModeParamTable;
	LastIndex = VID_TOTAL_RES_IDS;

	ModeDet = hdmi_rx_look_mode_param_table(pDetectionParam->HTotal,
			pDetectionParam->VTotal,
			RefRate,
			LastIndex,
			pRefreshRate,
			pModeParamTable,
			DispResId);

	if (!ModeDet) {
		pModeParamTable = (hrx_application_timing_params *)GfxModeParamTable_HDMI;
		LastIndex = GFX_TOTAL_RES_IDS;

		if (RefRate == HRX_RR_59_94HZ)
			RefRate = HRX_RR_60HZ;

		ModeDet = hdmi_rx_look_mode_param_table(pDetectionParam->HTotal,
				pDetectionParam->VTotal,
				RefRate,
				LastIndex,
				pRefreshRate,
				pModeParamTable,
				DispResId);
	}

	return ModeDet;
}

static int hdmi_rx_look_up_table(struct syna_hrx_v4l2_dev *hrx_dev, hrx_application_timing_params *pTiming_param,
	hrx_auto_detect_video_params *pVideoParams)
{

	u16 Htotal, Vtotal;
	hrx_hdmi_rx_col_depth_type RxColDepth = HRX_HDMI_RX_COL_DEPTH_8BIT;
	u8 PixRepValue = 0;
	hrx_display_resolution DispId;
	hrx_refresh_rate RefRateIndex;
	u8 pr;
	u64 tmds_freq;
	u64 fPixelClkFreq = 0; /* scale to 100 */
	u32 RefRate = 0; /* scale to 100 */
	hrx_signal_param_status rawData;
	struct v4l2_dv_timings timings;
	struct hdmi_avi_infoframe *avi;
	union hdmi_vendor_any_infoframe *vendor;
	int ret = 0;
	hrx_display_resolution TimingDispId = HRX_UNKNOWN_MODE;

	hrx_dev->IsAVIChkSumValid = FALSE;
	DispId = HRX_UNKNOWN_MODE;

	ret = hrx_query_dv_timings(hrx_dev, &timings);
	if (ret == -1) {
		HRX_LOG(HRX_DRV_ERROR, "Failed to query_dv_timings\n");
		return ret;
	}
	if (hrx_is_hdmi_mode(hrx_dev)) {
		hrx_get_infoframes(hrx_dev);
		get_input_res_pr(hrx_dev, &DispId, &pr);
		avi = &hrx_dev->aviif.infoframe.general.avi;
		vendor = &hrx_dev->vsif.infoframe.general.vendor;
		pVideoParams->Vic = avi->video_code;

		if (!hrx_dev->vsif.valid)
			vendor->hdmi.s3d_struct = HDMI_3D_STRUCTURE_INVALID;
		pVideoParams->VideoFrmt = (hrx_sig_video_format) vendor->hdmi.s3d_struct;
		HRX_LOG(HRX_DRV_DEBUG, "HRX_DRV_INFO: ResID=%d\n", DispId);
	}

	Htotal = hrx_reg_mask_read(hrx_dev, HDMI_VMON_STATUS3,
		HDMI_VMON_STATUS3_HTOTAL_OFFSET,
		HDMI_VMON_STATUS3_HTOTAL_MASK);
	Vtotal = hrx_reg_mask_read(hrx_dev, HDMI_VMON_STATUS6,
		HDMI_VMON_STATUS6_VTOTAL_OFFSET,
		HDMI_VMON_STATUS6_VTOTAL_MASK);

	pTiming_param->HTotal = Htotal;
	pTiming_param->HActive = timings.bt.width;
	pTiming_param->HFrontPorch = timings.bt.hfrontporch;
	pTiming_param->HSyncWidth = timings.bt.hsync;

	pTiming_param->VTotal = Vtotal;
	pTiming_param->VActive = timings.bt.height;
	pTiming_param->VFrontPorch = timings.bt.vfrontporch;
	pTiming_param->VSyncWidth = timings.bt.vsync;
	pTiming_param->Progressive = (timings.bt.interlaced?0:1);

	tmds_freq = hrx_get_tmds_clk(hrx_dev);
	//Compute Refresh rate
	if ((Htotal > 0) && (Vtotal > 0))
		RefRate = ((tmds_freq * 100)/(Htotal * Vtotal));

	if (hrx_is_hdmi_mode(hrx_dev))
		hrx_detect_hdmi_color_depth(hrx_dev, &RxColDepth, &PixRepValue);
	else
		pVideoParams->VideoFrmt = (hrx_sig_video_format) HDMI_3D_STRUCTURE_INVALID;

	Htotal = Htotal/(1+PixRepValue);
	fPixelClkFreq = ((tmds_freq *100) / (1+PixRepValue));

	switch (RxColDepth) {
	case HRX_HDMI_RX_COL_DEPTH_12BIT:
		{
			RefRate = ((RefRate * 3) >> 1);
		}
		break;

	case HRX_HDMI_RX_COL_DEPTH_10BIT:
		{
			RefRate = ((RefRate * 5) >> 2);
		}
		break;

	case HRX_HDMI_RX_COL_DEPTH_8BIT:
	default:
		break;
	}
	//TODO:: correct 3d frmat
	if (pVideoParams->VideoFrmt == HRX_SIG_VID_FMT_3D_FRAME_PACKING)
		Vtotal = Vtotal>>1;
	HRX_LOG(HRX_DRV_DEBUG, "HRX_DRV_INFO: Htotal=%d, Vtotal=%d, RefRate=%u,\n PixRepValue=%d, RxColDepth=%d VideoFrmt = %d fPixelClkFreq : %llu tmds_freq : %llu\n",
		Htotal, Vtotal, RefRate, PixRepValue, RxColDepth, pVideoParams->VideoFrmt, fPixelClkFreq, tmds_freq);

	//Lookup table
	rawData.HTotal = Htotal;
	rawData.VTotal = Vtotal;
	rawData.RefRate = RefRate;

	hrx_hdmi_detect_mode(&rawData, &RefRateIndex, &TimingDispId);
	HRX_LOG(HRX_DRV_DEBUG, "HRX_DRV_INFO:  VIC DispId = %d, TimingDispId = %d",
		DispId, TimingDispId);

	if (TimingDispId < HRX_CUSTOM_RESOLUTIONS)
		pTiming_param->DispResId = TimingDispId;
	else
		pTiming_param->DispResId = DispId;
	pTiming_param->RefreshRate = RefRate;
	pVideoParams->HrxIpBitDepthType = RxColDepth;
	pVideoParams->PixelReptFactor = PixRepValue;
	pVideoParams->HrxIpPixelClkFreq = fPixelClkFreq;
	hrx_dev->hdmiRxDrv.RxColDepth = RxColDepth;
	hrx_dev->hdmiRxDrv.tmds_clock = (u64)(tmds_freq * 1000);

	if ((DispId == HRX_UNKNOWN_MODE) && (TimingDispId == HRX_UNKNOWN_MODE))
		return -1;
	else
		return 0;
}

static int hrx_correct_syncPolarity(struct syna_hrx_v4l2_dev *hrx_dev, hrx_application_timing_params *pTimingParams)
{
	u32 val = 0;
	int hsync_width = 0;
	int vsync_width = 0;

	hrx_mw_signal_polarity HSyncPol_timing;
	hrx_mw_signal_polarity VSyncPol_timing;
	hrx_mw_signal_polarity HSyncPol_detect;
	hrx_mw_signal_polarity VSyncPol_detect;

	HSyncPol_timing = pTimingParams->HSyncPol;
	VSyncPol_timing = pTimingParams->VSyncPol;

	HRX_LOG(HRX_DRV_DEBUG, "HRX_Correct_SyncPolarity\n");

	val = (hrx_reg_read(hrx_dev, HDMI_VMON_STATUS1) & HDMI_VMON_STATUS1_HSYNCWIDTH_MASK);
	hsync_width = (val >> HDMI_VMON_STATUS1_HSYNCWIDTH_OFFSET);
	val = (hrx_reg_read(hrx_dev, HDMI_VMON_STATUS4) & HDMI_VMON_STATUS4_VSYNCWIDTH_MASK);
	vsync_width = (val >> HDMI_VMON_STATUS4_VSYNCWIDTH_OFFSET);

	/*positive sync had been detected*/
	if (hsync_width < pTimingParams->HActive)
		HSyncPol_detect = HRX_MW_POSITIVE_POLARITY;
	else
		HSyncPol_detect = HRX_MW_NEGATIVE_POLARITY;

	/*positive sync had been detected*/
	if (vsync_width < pTimingParams->VActive)
		VSyncPol_detect = HRX_MW_POSITIVE_POLARITY;
	else
		VSyncPol_detect = HRX_MW_NEGATIVE_POLARITY;

	if (HSyncPol_detect != HSyncPol_timing) {
		HRX_LOG(HRX_DRV_DEBUG, "Inverted HSync polarity had been detected, correct to %s\n",
			HSyncPol_detect == HRX_MW_POSITIVE_POLARITY ? "POS":"NEG");
		pTimingParams->HSyncPol = HSyncPol_detect;
	}

	if (VSyncPol_detect != VSyncPol_timing) {
		HRX_LOG(HRX_DRV_DEBUG, "Inverted VSync polarity had been detected, correct to %s\n",
			VSyncPol_detect == HRX_MW_POSITIVE_POLARITY ? "POS":"NEG");
		pTimingParams->VSyncPol = VSyncPol_detect;
	}
	return 0;
}

static void read_spd_infoframe(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int i;
	u32 val, addr;

	addr = HDMI_PKTDEC_SRCPDIF_PH;
	val = hrx_reg_read(hrx_dev, addr);

	hrx_dev->spd_info[0] = val & 0xff;
	hrx_dev->spd_info[1] = (val >> 8) & 0xff;

	for (i = 1; i < 8; i++) {
		addr += 4;
		val = hrx_reg_read(hrx_dev, addr);
		hrx_dev->spd_info[i*4-2] = val & 0xff;
		hrx_dev->spd_info[i*4-1] = (val >> 8) & 0xff;
		hrx_dev->spd_info[i*4] = (val >> 16) & 0xff;
		hrx_dev->spd_info[i*4+1] = (val >> 24) & 0xff;
	}
}

static void hrx_avi_get_col_info(struct syna_hrx_v4l2_dev *hrx_dev, hrx_hdmi_rx_col_format_type *pRxColFormat, hrx_hdmi_rx_col_std *pRxColStd)
{

	u32 val_l = 0x0, val_h = 0x0;
	struct hdmi_avi_infoframe *avi;
	u8 base_colorm, ext_colorm;
	*pRxColStd = HRX_HDMI_RX_COL_STD_NONE;

	avi = &hrx_dev->aviif.infoframe.general.avi;
	switch (avi->colorspace) {
	case HDMI_COLORSPACE_RGB:
		*pRxColFormat = HRX_HDMI_RX_COL_FMT_RGB;
		break;
	case HDMI_COLORSPACE_YUV422:
		*pRxColFormat = HRX_HDMI_RX_COL_FMT_YUV422;
		val_h = 0x00000f00;
		val_l = 0x00007f00;
		break;
	case HDMI_COLORSPACE_YUV444:
		*pRxColFormat = HRX_HDMI_RX_COL_FMT_YUV444;
		val_h = 0x00000f00;
		val_l = 0x7f007f00;
		break;
	case HDMI_COLORSPACE_YUV420:
		*pRxColFormat = HRX_HDMI_RX_COL_FMT_YUV420;
		val_h = 0x00000f00;
		val_l = 0x0f007f00;
		break;
	default:
		*pRxColFormat = HRX_HDMI_RX_COL_FMT_YUV422;
		val_h = 0x00000f00;
		val_l = 0x00007f00;
		break;
	}

	hrx_reg_write(hrx_dev, val_l, HDMI_VIDEO_MUTE_VALUE_L);
	hrx_reg_write(hrx_dev, val_h, HDMI_VIDEO_MUTE_VALUE_H);

	base_colorm =  avi->colorimetry;
	ext_colorm = avi->extended_colorimetry;
	//GetColInfo(hrx_dev, &base_colorm, &ext_colorm);
	if (*pRxColFormat != HRX_HDMI_RX_COL_FMT_RGB) {
		if (base_colorm == 0x03) {
			ext_colorm = ext_colorm & 0x07;
			ext_colorm = ext_colorm + 0x03;
			if (ext_colorm != HRX_HDMI_RX_COL_STD_EXT_AdobeRGB)
				*pRxColStd = ext_colorm;
			else
				*pRxColStd = HRX_HDMI_RX_COL_STD_RSVD;
		} else if ((base_colorm == HRX_HDMI_RX_COL_STD_NONE) || (base_colorm == HRX_HDMI_RX_COL_STD_ITUR709))
			*pRxColStd = HRX_HDMI_RX_COL_STD_ITUR709;
		else
			*pRxColStd = HRX_HDMI_RX_COL_STD_SMPTE170M;
	} else {
		if (base_colorm == 0x03) {
			ext_colorm = ext_colorm & 0x07;
			ext_colorm = ext_colorm + 0x03;
			if ((ext_colorm == HRX_HDMI_RX_COL_STD_EXT_AdobeRGB) || (ext_colorm == HRX_HDMI_RX_COL_STD_EXT_BT2020YCC))
				*pRxColStd = ext_colorm;
			else
				*pRxColStd = HRX_HDMI_RX_COL_STD_RSVD;
		} else if (base_colorm == HRX_HDMI_RX_COL_STD_NONE) {
			*pRxColStd = HRX_HDMI_RX_COL_STD_NONE;
		} else {
			*pRxColStd = HRX_HDMI_RX_COL_STD_RSVD;
		}
	}

}

static int hrx_configure_timing_params(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u8	PixelReptFactor = 0;
	//Configure HDMI-Rx
	bool	IsTmdsValid;
	int rc = 0;

	IsTmdsValid = hrx_tmds_valid(hrx_dev);
	if (IsTmdsValid) {
		hrx_dev->video_params.VideoFrmt = HRX_SIG_VID_FMT_2D; // update when get the ven pkt
		// try to read vendor pkt if status is ok
		hrx_try_read_vendor_pkt(hrx_dev, &hrx_dev->video_params.VideoFrmt); // TODO can get ext resolution from vendor pkt

		rc = hdmi_rx_look_up_table(hrx_dev, &hrx_dev->video_params.HrxIpTimingParam, &hrx_dev->video_params);
		if (rc != 0) {
			//Trying once again
			HRX_LOG(HRX_DRV_DEBUG, "HRX_DRV_INFO: HDMIRX_LookUpTable failed first time - Retry\n");
			rc = hdmi_rx_look_up_table(hrx_dev, &hrx_dev->video_params.HrxIpTimingParam, &hrx_dev->video_params);
			if (rc != 0)
				return -1;
		}
		hrx_dev->video_params.IsHdmiMode = hrx_is_hdmi_mode(hrx_dev);
		if (!hrx_dev->video_params.IsHdmiMode) {
			if (false == hrx_dev->Is14EdidLoaded) {
				/* Some old BD players (Sony BDP-S370, BDP-S470) having issue in
				 * parsing EDID with multiple VSDB.
				 * Load EDID without HF-VSDB for coming out of DVI mode for such
				 * players
				 */

				HRX_LOG(HRX_DRV_INFO, "IsHdmiMode = %d load EDID without HF-VSDB not support\n",
					hrx_dev->video_params.IsHdmiMode);
			}
		}
	} else
		return -1;

	hrx_correct_syncPolarity(hrx_dev, &hrx_dev->video_params.HrxIpTimingParam);
	if (hrx_dev->video_params.IsHdmiMode) {
		PixelReptFactor = hrx_dev->video_params.PixelReptFactor;
		read_spd_infoframe(hrx_dev);
	} else {
		hrx_dev->video_params.HrxIpBitDepthType = HRX_HDMI_RX_COL_DEPTH_8BIT;
		PixelReptFactor = 0;
	}

	//Step 4 -> get the color format
	if (hrx_dev->video_params.IsHdmiMode)
		hrx_avi_get_col_info(hrx_dev, &hrx_dev->video_params.HrxIpColorFormatType,
			&hrx_dev->video_params.HrxIpColorStd);
	else
		hrx_dev->video_params.HrxIpColorFormatType = HRX_HDMI_RX_COL_FMT_RGB;

	return 0;

}

static int syna_hrx_interrupt_config(struct platform_device *pdev)
{
	struct syna_hrx_v4l2_dev *hrx_dev;
	int ret;
	int irq;

	hrx_dev = (struct syna_hrx_v4l2_dev *)dev_get_drvdata(&pdev->dev);
	irq = platform_get_irq_byname(pdev, "otg");
	if (irq < 0) {
		HRX_LOG(HRX_DRV_ERROR, "fail to get irq for otg\n");
		ret = irq;
		goto err;
	} else {
		hrx_dev->otg_intr = irq;
	}

	irq = platform_get_irq_byname(pdev, "hdmirx");
	if (irq < 0) {
		HRX_LOG(HRX_DRV_ERROR, "fail to get irq for hdmirx\n");
		ret = irq;
		goto err;
	} else {
		hrx_dev->hdmirx_intr = irq;
	}

	irq = platform_get_irq_byname(pdev, "ytg");
	if (irq < 0) {
		HRX_LOG(HRX_DRV_ERROR, "fail to get irq for ytg\n");
		ret = irq;
		goto err;
	} else {
		hrx_dev->ytg_intr = irq;
	}

	irq = platform_get_irq_byname(pdev, "uvtg");
	if (irq < 0) {
		HRX_LOG(HRX_DRV_ERROR, "fail to get irq for uvtg\n");
		ret = irq;
		goto err;
	} else {
		hrx_dev->uvtg_intr = irq;
	}

	irq = platform_get_irq_byname(pdev, "itg");
	if (irq < 0) {
		HRX_LOG(HRX_DRV_ERROR, "fail to get irq for itg\n");
		ret = irq;
		goto err;
	} else {
		hrx_dev->itg_intr = irq;
	}

	irq = platform_get_irq_byname(pdev, "mic3");
	if (irq < 0) {
		HRX_LOG(HRX_DRV_ERROR, "fail to get irq for mic3\n");
		ret = irq;
		goto err;
	} else {
		hrx_dev->mic3_intr = irq;
	}

	HRX_LOG(HRX_DRV_INFO, "otg_intr = %d [%d]\n", hrx_dev->otg_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx_dev->otg_intr)));
	HRX_LOG(HRX_DRV_INFO, "hdmirx_intr = %d [%d]\n", hrx_dev->hdmirx_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx_dev->hdmirx_intr)));
	HRX_LOG(HRX_DRV_INFO, "ytg_intr = %d [%d]\n", hrx_dev->ytg_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx_dev->ytg_intr)));
	HRX_LOG(HRX_DRV_INFO, "uvtg_intr = %d [%d]\n", hrx_dev->uvtg_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx_dev->uvtg_intr)));
	HRX_LOG(HRX_DRV_INFO, "itg_intr = %d [%d]\n", hrx_dev->itg_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx_dev->itg_intr)));
	HRX_LOG(HRX_DRV_INFO, "mic3_intr = %d [%d]\n", hrx_dev->mic3_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx_dev->mic3_intr)));

	/* register and enable VIP ISR */
	ret = devm_request_irq(&pdev->dev,hrx_dev->otg_intr,
					  syna_hrx_isr,
					  IRQF_SHARED, "ADHUB VIP OutTG", hrx_dev);
	if (ret)
		goto err;

	ret = devm_request_irq(&pdev->dev,hrx_dev->hdmirx_intr,
					  syna_hrx_isr,
					  IRQF_SHARED, "ADHUB HDMIRX", hrx_dev);
	if (ret)
		goto err;

	ret = devm_request_irq(&pdev->dev,hrx_dev->ytg_intr,
					  syna_hrx_isr,
					  IRQF_SHARED, "ADHUB VIP YTG", hrx_dev);
	if (ret)
		goto err;

	ret = devm_request_irq(&pdev->dev,hrx_dev->uvtg_intr,
					  syna_hrx_isr,
					  IRQF_SHARED, "ADHUB VIP UVTG", hrx_dev);
	if (ret)
		goto err;

	ret = devm_request_irq(&pdev->dev,hrx_dev->itg_intr,
					  syna_hrx_isr,
					  IRQF_SHARED, "ADHUB VIP InTG", hrx_dev);
	if (ret)
		goto err;

	ret = devm_request_irq(&pdev->dev,hrx_dev->mic3_intr,
					  syna_hrx_isr,
					  IRQF_SHARED, "ADHUB AIP MIC3", hrx_dev);
	if (ret)
		goto err;

	Dhub_IntrRegisterHandler(DHUB_ID_VPP_DHUB, avioDhubSemMap_vpp128b_vpp_inr9, hrx_dev, (DHUB_INTR_HANDLER)syna_hrx_vpp_intr_handler);
	Dhub_IntrRegisterHandler(DHUB_ID_VPP_DHUB, avioDhubSemMap_vpp128b_vpp_inr10, hrx_dev, (DHUB_INTR_HANDLER)syna_hrx_vpp_intr_handler);

	HRX_LOG(HRX_DRV_INFO, "irqs registered succcessfully\n");
	return 0;
err:
	return ret;
}

static int syna_hrx_parse_dt(struct platform_device *pdev)
{
	struct syna_hrx_v4l2_dev *hrx_dev;
	struct resource *edid_resource;
	struct resource *hrx_addr_resource;
	struct resource *vip_addr_resource;
	void __iomem *edid_regs;
	int ret = -1;

	hrx_dev = (struct syna_hrx_v4l2_dev *)dev_get_drvdata(&pdev->dev);
	edid_resource = platform_get_resource_byname(pdev,
		IORESOURCE_MEM, "edid_base");

	hrx_dev->edid_resource = edid_resource;

	edid_regs = devm_ioremap_resource(&pdev->dev, hrx_dev->edid_resource);
	if (IS_ERR(edid_regs)) {
		HRX_LOG(HRX_DRV_ERROR, "could not remap edid address space: %ld\n", PTR_ERR(edid_regs));
		return PTR_ERR(edid_regs);
	} else {
		hrx_dev->edid_regs = edid_regs;
	}

	hrx_addr_resource = platform_get_resource_byname(pdev,
		IORESOURCE_MEM, "hrx_base");

	hrx_dev->hrx_base = hrx_addr_resource->start;

	vip_addr_resource = platform_get_resource_byname(pdev,
		IORESOURCE_MEM, "vip_base");

	hrx_dev->vip_base = vip_addr_resource->start;
	hrx_base_glbl = hrx_dev->hrx_base;
	vip_base_glbl = hrx_dev->vip_base;

	hrx_dev->gpiod_hrxhpd = devm_gpiod_get_optional(&pdev->dev,
				"hrxhpd", GPIOD_OUT_LOW);
	if (IS_ERR(hrx_dev->gpiod_hrxhpd)) {
		HRX_LOG(HRX_DRV_ERROR, "could not get hdmirx hpd gpio, err : %ld\n",
				PTR_ERR(hrx_dev->gpiod_hrxhpd));
		return PTR_ERR(hrx_dev->gpiod_hrxhpd);
	}

	hrx_dev->gpiod_hrx5v = devm_gpiod_get_optional(&pdev->dev,
				"hrx5v", GPIOD_IN);
	if (IS_ERR(hrx_dev->gpiod_hrx5v)) {
		HRX_LOG(HRX_DRV_ERROR, "could not get hdmirx 5v gpio, err : %ld\n", PTR_ERR(hrx_dev->gpiod_hrx5v));
		ret = PTR_ERR(hrx_dev->gpiod_hrx5v);
		hrx_dev->gpiod_hrx5v = NULL;
		return ret;
	}

	return 0;
}

static int hrx_wait_video_stable(struct syna_hrx_v4l2_dev *hrx_dev)
{
	/*
	 * Empiric value. Video should be stable way longer before the end
	 * of this sleep time. Though, we can have some video change interrupts
	 * before the video is stable so filter them by sleeping.
	 */
	mdelay(hrx_dev->video_stable_wait_ms);
	return 0;
}

static int hrx_wait_audio_lock_poll(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int timeout = 10;

	while (!hrx_audio_valid(hrx_dev) && timeout-- && !hrx_dev->force_off)
		udelay(5);

	if (!has_audio_clock(hrx_dev))
		return 0;
	if (!hrx_audio_valid(hrx_dev))
		return -ETIMEDOUT;
	return 0;
}

static void hrx_restart_audio_fifo(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AUDIO_FIFO_CONTROL,
			HDMI_AUDIO_FIFO_CONTROL_INIT_P_OFFSET,
			HDMI_AUDIO_FIFO_CONTROL_INIT_P_MASK);
}


static int hrx_config_audio(struct syna_hrx_v4l2_dev *hrx_dev, bool just_reconfig)
{
	int ret;
	u32 n = hrx_get_n(hrx_dev);


	/* configure audio pll */
	if (!just_reconfig)
		hrx_audio_pll_config_static_in(hrx_dev);

	if (n != 0) {
		hrx_audio_pll_config_dynamic_in(hrx_dev);
		ret = hrx_wait_audio_lock_poll(hrx_dev);
		if (ret) {
			HRX_LOG(HRX_DRV_ERROR, "failed to wait for audio pll lock\n");
			return ret;
		}
	} else {
		HRX_LOG(HRX_DRV_ERROR, "no audio detected N=0\n");
	}

	/* Config */
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AUDIO_PROC_CONFIG0,
		HDMI_AUDIO_PROC_CONFIG0_SPDIF_EN_OFFSET,
		HDMI_AUDIO_PROC_CONFIG0_SPDIF_EN_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AUDIO_PROC_CONFIG0,
		HDMI_AUDIO_PROC_CONFIG0_I2S_EN_OFFSET,
		HDMI_AUDIO_PROC_CONFIG0_I2S_EN_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AUDIO_FIFO_CONFIG,
		HDMI_AUDIO_FIFO_CONFIG_FILL_RESTART_OFFSET,
		HDMI_AUDIO_FIFO_CONFIG_FILL_RESTART_MASK);
	hrx_reg_mask_write(hrx_dev, 0x4, HDMI_PKTDEC_ACR_CONFIG,
		HDMI_PKTDEC_ACR_CONFIG_DELTACTS_THR_OFFSET,
		HDMI_PKTDEC_ACR_CONFIG_DELTACTS_THR_MASK);

	/* configure channel spread enable */
	hrx_config_channel_spread_en(hrx_dev);

	/* Start */
	hrx_restart_audio_fifo(hrx_dev);
	HRX_LOG(HRX_DRV_DEBUG, "audio fifo enable :%d\n", __LINE__);
	hrx_get_infoframes(hrx_dev);

	return 0;
}

static unsigned int hrx_get_sample_width(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 lstat[HDMI_AUDIO_PROC_CHSTAT_SIZE];
	u32 rstat[HDMI_AUDIO_PROC_CHSTAT_SIZE];
	u8 lsf, rsf;
	int i;

	if (!hrx_dev->audioif.valid)
		return 0;
	if (hrx_reg_read(hrx_dev, HDMI_AUDIO_PROC_STATUS1) &
			HDMI_AUDIO_PROC_STATUS1_MUTE)
		return 0;

	if (!hrx_get_n(hrx_dev))
		return 0;

	for (i = 0; i < HDMI_AUDIO_PROC_CHSTAT_SIZE; i++) {
		lstat[i] = hrx_reg_read(hrx_dev, HDMI_AUDIO_PROC_CHSTAT_SP0_L(i));
		rstat[i] = hrx_reg_read(hrx_dev, HDMI_AUDIO_PROC_CHSTAT_SP0_R(i));
	}

	lsf = (lstat[HDMI_ASW_BYTE] & HDMI_ASW_MASK) >> HDMI_ASW_OFFSET;
	rsf = (rstat[HDMI_ASW_BYTE] & HDMI_ASW_MASK) >> HDMI_ASW_OFFSET;

	/* from spec bit32-35 (word length) = "0100" 16bits / “0011” 17bits
	 *  “0010” 18bits / "0001" 19bits / "0101" or "1100" 20bits
	 *  "1011" 21bits / "1010" 22bits / "1001" 23bits / "1101" 24bits
	 */
	switch (lsf) {
	case 2: return 16;
	case 12: return 17;
	case 4: return 18;
	case 8: return 19;
	case 10: return 20;
	case 3: return 20;
	case 13: return 21;
	case 5: return 22;
	case 9: return 23;
	case 11: return 24;
	default:
		switch (rsf) {
		case 2: return 16;
		case 12: return 17;
		case 4: return 18;
		case 8: return 19;
		case 10: return 20;
		case 3: return 20;
		case 13: return 21;
		case 5: return 22;
		case 9: return 23;
		case 11: return 24;
		default:
			return 0;
		}
	}
}

static u32 hrx_get_audio_format(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return hrx_reg_mask_read(hrx_dev, HDMI_AUDIO_PROC_STATUS1,
			       HDMI_AUDIO_PROC_STATUS1_FMT_OFFSET,
			       HDMI_AUDIO_PROC_STATUS1_FMT_MASK);
}

static u32 hrx_get_audio_layout(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return hrx_reg_mask_read(hrx_dev, HDMI_AUDIO_PROC_STATUS1,
			       HDMI_AUDIO_PROC_STATUS1_LAYOUT_OFFSET,
			       HDMI_AUDIO_PROC_STATUS1_LAYOUT_MASK);
}

static inline bool hdmi_is_on(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return hrx_dev->hdmi_state == HDMI_STATE_POWER_ON ||
		hrx_dev->hdmi_state == HDMI_STATE_VG_MODE;
}

static int hrx_wait_phy_unlock_poll(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int timeout = hrx_dev->tmds_valid_wait_count;
	bool has_timeout = timeout > 0;

	while (hrx_tmds_valid(hrx_dev) && !hrx_dev->force_off) {
		/* Timeout handling */
		if (has_timeout) {
			timeout--;
			if (!timeout)
				break;
		}

		udelay(500);
	}

	if (hrx_tmds_valid(hrx_dev))
		return -ETIMEDOUT;
	return 0;
}

static int hrx_wait_phy_lock_poll(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int timeout = hrx_dev->tmds_valid_wait_count;
	bool has_timeout = timeout > 0;

	while (!hrx_tmds_valid(hrx_dev) && !hrx_dev->force_off) {
		/* Timeout handling */
		if (has_timeout) {
			timeout--;
			if (!timeout)
				break;
		}
		udelay(500);
	}

	if (!hrx_tmds_valid(hrx_dev))
		return -ETIMEDOUT;
	if (!has_clock(hrx_dev))
		return -EINVAL;
	return 0;
}

static void hrx_enable_audio_ints(struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AVPUNIT_1_INT_MASK_N,
			HDMI_AVPUNIT_1_INT_MASK_N_AFIFO_THR_PASS_OFFSET,
			HDMI_AVPUNIT_1_INT_MASK_N_AFIFO_THR_PASS_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AVPUNIT_1_INT_MASK_N,
			HDMI_AVPUNIT_1_INT_MASK_N_AUD_CHSTATUS_SP0_OFFSET,
			HDMI_AVPUNIT_1_INT_MASK_N_AUD_CHSTATUS_SP0_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AVPUNIT_1_INT_MASK_N,
			HDMI_AVPUNIT_1_INT_MASK_N_AUD_CHSTATUS_SP1_OFFSET,
			HDMI_AVPUNIT_1_INT_MASK_N_AUD_CHSTATUS_SP1_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AVPUNIT_1_INT_MASK_N,
			HDMI_AVPUNIT_1_INT_MASK_N_AUD_CHSTATUS_SP2_OFFSET,
			HDMI_AVPUNIT_1_INT_MASK_N_AUD_CHSTATUS_SP2_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AVPUNIT_1_INT_MASK_N,
			HDMI_AVPUNIT_1_INT_MASK_N_AUD_CHSTATUS_SP3_OFFSET,
			HDMI_AVPUNIT_1_INT_MASK_N_AUD_CHSTATUS_SP3_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AVPUNIT_1_INT_MASK_N,
			HDMI_AVPUNIT_1_INT_MASK_N_AUD_MUTE_OFFSET,
			HDMI_AVPUNIT_1_INT_MASK_N_AUD_MUTE_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AVPUNIT_1_INT_MASK_N,
			HDMI_AVPUNIT_1_INT_MASK_N_AFIFO_UNDERFLOW_OFFSET,
			HDMI_AVPUNIT_1_INT_MASK_N_AFIFO_UNDERFLOW_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AVPUNIT_1_INT_MASK_N,
			HDMI_AVPUNIT_1_INT_MASK_N_AFIFO_OVERFLOW_OFFSET,
			HDMI_AVPUNIT_1_INT_MASK_N_AFIFO_OVERFLOW_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
			HDMI_PKT_0_INT_MASK_N_ACR_N_OFFSET,
			HDMI_PKT_0_INT_MASK_N_ACR_N_MASK);
	hrx_reg_mask_write(hrx_dev, 0x1, HDMI_PKT_0_INT_MASK_N,
			HDMI_PKT_0_INT_MASK_N_ACR_CTS_OFFSET,
			HDMI_PKT_0_INT_MASK_N_ACR_CTS_MASK);
}

/* Mute audio if both audio sample present and sample flat are enabled in ASP */
static bool hrx_check_sample_flat_enabled(struct syna_hrx_v4l2_dev *hrx_dev)
{

	u32 sample_flat = hrx_reg_mask_read(hrx_dev, HDMI_AUDIO_PROC_STATUS1,
								HDMI_AUDIO_PROC_STATUS1_SAMPLE_FLAT_OFFSET,
								HDMI_AUDIO_PROC_STATUS1_SAMPLE_FLAT_MASK);
	u32 sample_present = hrx_reg_mask_read(hrx_dev, HDMI_AUDIO_PROC_STATUS1,
								HDMI_AUDIO_PROC_STATUS1_SAMPLE_PRESENT_OFFSET,
								HDMI_AUDIO_PROC_STATUS1_SAMPLE_PRESENT_MASK);

	if ((sample_flat & sample_present) != 0) {
		if (hrx_dev->audFifoDis == 0) {
			hrx_reg_mask_write(hrx_dev, 0x1, HDMI_AUDIO_FIFO_CONFIG,
				HDMI_AUDIO_FIFO_CONFIG_FILL_STOP_OFFSET,
				HDMI_AUDIO_FIFO_CONFIG_FILL_STOP_MASK);
			hrx_dev->audFifoDis = 1;
			HRX_LOG(HRX_DRV_DEBUG, "audio fifo disable\n");
			return 0;
		}
	} else if (hrx_dev->audFifoDis == 1) {
		hrx_reg_mask_write(hrx_dev, 0x0, HDMI_AUDIO_FIFO_CONFIG,
			HDMI_AUDIO_FIFO_CONFIG_FILL_STOP_OFFSET,
			HDMI_AUDIO_FIFO_CONFIG_FILL_STOP_MASK);
		hrx_restart_audio_fifo(hrx_dev);
		hrx_dev->audFifoDis = 0;
		HRX_LOG(HRX_DRV_DEBUG, "audio fifo enable\n");
		return 1;
	}
	return 1;
}

static u32 round_freq(int freq)
{
	static const u32 base_freqs[] = { 32000, 44100, 48000, 88200, 96000,
		176400, 192000, 768000, 0 };
	int i = 0;

	for (i = 0; base_freqs[i]; i++) {
		if ((freq <= (base_freqs[i] + HDMI_AUDIO_FREQ_RANGE)) &&
		    (freq >= (base_freqs[i] - HDMI_AUDIO_FREQ_RANGE))) {
			return base_freqs[i];
		}
	}

	return 0;
}

static u32 hrx_get_cts(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 cts, pb;

	/* need to read the header first */
	hrx_reg_read(hrx_dev, HDMI_PKTDEC_ACR_PH);

	pb = hrx_reg_read(hrx_dev, HDMI_PKTDEC_ACR_PB(0));
	/* pktdec_acr_pb3_0[11:8],[23:16],[31:24] */
	cts = (pb & GENMASK(11, 8)) << 8 | (pb & GENMASK(23, 16)) >> 8 |
		(pb & GENMASK(31, 24)) >> 24;
	if (cts)
		hrx_dev->cts = cts;

	HRX_LOG(HRX_DRV_DEBUG, "CTS=%u\n", cts);

	return hrx_dev->cts;
}

static unsigned int hrx_get_sample_freq_n_cts(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 sf;
	u32 n;
	u32 cts;
	unsigned long tmp;

	n = hrx_get_n(hrx_dev);
	cts = hrx_get_cts(hrx_dev);

	HRX_LOG(HRX_DRV_DEBUG, "cts: %u, n: %u\n", cts, n);

	if (!n || !cts)
		return 0;

	HRX_LOG(HRX_DRV_DEBUG, "tmds_clk: %d\n", hrx_get_tmds_clk(hrx_dev));

	/* regenerate the audio clock from tmds clock */
	tmp = (unsigned long)hrx_get_tmds_clk(hrx_dev);
	do_div(tmp, cts);
	tmp *= (unsigned long)n;
	do_div(tmp, 128);
	sf = tmp;

	HRX_LOG(HRX_DRV_DEBUG, "sf: %d\n", sf);
	sf = round_freq(sf);
	HRX_LOG(HRX_DRV_DEBUG, "sf(round): %d\n", sf);

	return sf;
}

static unsigned int hrx_get_sample_freq(struct syna_hrx_v4l2_dev *hrx_dev, bool check_aud_mute)
{
	u32 lstat[HDMI_AUDIO_PROC_CHSTAT_SIZE];
	u32 rstat[HDMI_AUDIO_PROC_CHSTAT_SIZE];
	bool ch_status_ready = false;
	u32 audio_proc_status;
	u8 lsf, rsf;
	u32 sf_n_cts;
	int i;
	u32 n = hrx_get_n(hrx_dev);

	audio_proc_status = hrx_reg_read(hrx_dev, HDMI_AUDIO_PROC_STATUS1);
	if (check_aud_mute && (audio_proc_status &
				HDMI_AUDIO_PROC_STATUS1_MUTE)) {
		HRX_LOG(HRX_DRV_DEBUG, "audio mute...\n");
		if ((n != 0) && (has_clock(hrx_dev)) &&
				(!has_audio_clock(hrx_dev))) {
			HRX_LOG(HRX_DRV_DEBUG, "Recover audio...\n");
			hrx_audio_pll_config_dynamic_in(hrx_dev);
			hrx_wait_audio_lock_poll(hrx_dev);
		}
		return 0;
	}

	/* based on N/CTS */
	sf_n_cts = hrx_get_sample_freq_n_cts(hrx_dev);
	if (hrx_dev->sample_freq_source == HDMI_SAMPLE_FREQ_SOURCE_N_AND_CTS)
		return sf_n_cts;

	if (!hrx_get_n(hrx_dev))
		HRX_LOG(HRX_DRV_DEBUG, "audio N=0 ignored...\n");

	for (i = 0; i < HDMI_AUDIO_PROC_CHSTAT_SIZE; i++) {
		lstat[i] = hrx_reg_read(hrx_dev, HDMI_AUDIO_PROC_CHSTAT_SP0_L(i));
		rstat[i] = hrx_reg_read(hrx_dev, HDMI_AUDIO_PROC_CHSTAT_SP0_R(i));

		if (lstat[i] || rstat[i])
			ch_status_ready = true;
	}

	if (!ch_status_ready) {
		HRX_LOG(HRX_DRV_DEBUG, "audio ch status not ready...\n");
		return 0;
	}

    /* Consider Bit 30-31 to calculate sampling fresuency [31:30 27:24] */
	lsf = (lstat[HDMI_SF_BYTE] & HDMI_SF_MASK) >> HDMI_SF_OFFSET |
			(lstat[HDMI_SF_BYTE] & HDMI_SF1_MASK) >> HDMI_SF1_OFFSET;
	rsf = (rstat[HDMI_SF_BYTE] & HDMI_SF_MASK) >> HDMI_SF_OFFSET |
			(rstat[HDMI_SF_BYTE] & HDMI_SF1_MASK) >> HDMI_SF1_OFFSET;


	switch (lsf) {
	case 0x3: return 32000;
	case 0x0: return 44100;
	case 0x2: return 48000;
	case 0x8: return 88200;
	case 0xa: return 96000;
	case 0xc: return 176400;
	case 0xe: return 192000;
	case 0x2d: return 705600;
	case 0x9: return 768000;
	default:
		switch (rsf) {
		case 0x3: return 32000;
		case 0x0: return 44100;
		case 0x2: return 48000;
		case 0x8: return 88200;
		case 0xa: return 96000;
		case 0xc: return 176400;
		case 0xe: return 192000;
		case 0x2d: return 705600;
		case 0x9: return 768000;
		default:
			break;
		}
	}

	if (!hrx_dev->audioif.valid) {
		HRX_LOG(HRX_DRV_DEBUG, "audio infoframe invalid...\n");
		return 0;
	}

	switch (hrx_dev->audioif.infoframe.general.audio.sample_frequency) {
	case HDMI_AUDIO_SAMPLE_FREQUENCY_32000: return 32000;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_44100: return 44100;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_48000: return 48000;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_88200: return 88200;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_96000: return 96000;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_176400: return 176400;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_192000: return 192000;
	default:
		HRX_LOG(HRX_DRV_DEBUG, "audio freq unknown (= 0)...\n");
		return 0;
	}
}

static void hrx_handle_audio_mute_change(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned int sf_notify;
	u32 ch_status_sp0;
	u32 audio_format;
	u32 audio_layout;
	bool is_lpcm_2ch;
	unsigned int sf;
	bool is_lpcm;

	if (hrx_check_sample_flat_enabled(hrx_dev) == 0)
		return;

	sf = hrx_get_sample_freq(hrx_dev, true);

	/* check the audio is L-PCM with 2channels */
	audio_format = hrx_get_audio_format(hrx_dev);
	audio_layout = hrx_get_audio_layout(hrx_dev);
	ch_status_sp0 = hrx_reg_read(hrx_dev, HDMI_AUDIO_PROC_CHSTAT_SP0_L(0));
	is_lpcm = ((ch_status_sp0 & BIT(1)) == 0);
	is_lpcm_2ch = (audio_format == EARCTX_AUDIO_FORMAT_ASP &&
		       audio_layout == 0 && is_lpcm);

	/* to filter non basic audio */
	if (sf <= 48000 && is_lpcm_2ch)
		sf_notify = sf;
	else
		sf_notify = 0;

	hrx_dev->audio_sf = sf;
}

static int hrx_config_controller(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int ret, eq_ret;
	bool just_reconfig_audio = (hrx_dev->hdmi_cstate == HDMI_CSTATE_AUDIO);

	while (1) {
		/* Give up if we are forcing off */
		if (hrx_dev->force_off)
			return -ETIMEDOUT;
		/* Give up if input is disconnected */
		if (!hrx_is_5v_connected(hrx_dev)) {
			ret = -ETIMEDOUT;
			break;
		}
		/* Give up if state changed */
		if (hrx_dev->hdmi_state != HDMI_STATE_CONTROLLER_CONFIG) {
			ret = -ETIMEDOUT;
			break;
		}
		/* Give up if already configured */
		if (hrx_dev->hdmi_cstate == HDMI_CSTATE_CONFIGURED)
			return 0;

		switch (hrx_dev->hdmi_cstate) {
		case HDMI_CSTATE_NOT_CONFIGURED:
			hrx_disable_all_ints(hrx_dev);
			hrx_dev->hdmi_cstate = HDMI_CSTATE_EQUALIZER;
			break;
		case HDMI_CSTATE_PHY_RECONFIG:
			ret = hrx_wait_phy_unlock_poll(hrx_dev);
			if (ret)
				HRX_LOG(HRX_DRV_ERROR, "Failed to wait for PHY Unlock\n");

			hrx_phy_tmds_clock_ratio(hrx_dev, hrx_is_hdmi2(hrx_dev));
			hrx_dev->hdmi_cstate = HDMI_CSTATE_EQUALIZER;
			break;
		case HDMI_CSTATE_EQUALIZER:
			hrx_disable_all_ints(hrx_dev);
			if (hrx_dev->is_hdmi2 != hrx_is_hdmi2(hrx_dev)) {
				if (!hrx_dev->tmds2_reconfig) {
					//dw_hdmi_phy_e701_fpga_config(hrx_dev);
					hrx_phy_tmds_clock_ratio(hrx_dev, hrx_is_hdmi2(hrx_dev));
				} else {
					hrx_dev->hdmi_cstate = HDMI_CSTATE_PHY_RECONFIG;
					break;
				}
			}
			/* Do not force equalizer */
			hrx_dev->phy_eq_force = false;

			/* Clear equalizer error status if not on */
			if (!hrx_dev->phy_eq_on)
				eq_ret = 0;

			/* Then, check if we have TMDSVALID */
			ret = hrx_wait_phy_lock_poll(hrx_dev);

			if (ret || eq_ret) {
				if (ret || eq_ret == -ETIMEDOUT) {
					/*
					 * No TMDSVALID signal:
					 *	- force equalizer algorithm
					 */
					hrx_dev->phy_eq_force = true;
				}
				HRX_LOG(HRX_DRV_DEBUG, "[TMDS] is NOT valid! tmds_lost_count = %d\n",
						tmds_lost_count);
				/* 1.4 repeater CTS, phy re-init early to get TMDS valid high */
				if (tmds_lost_count == 3)
					hrx_phy_re_init(hrx_dev);
				if (tmds_lost_count < HRX_TMDS_DETECT_THRESHOLD) {
					ret = hrx_phy_init(hrx_dev, hrx_is_hdmi2(hrx_dev),
						false);
				} else if (tmds_lost_count < HRX_6G_TMDS_DETECT_THRESHOLD) {
					ret = hrx_phy_init(hrx_dev, true,
						false);
					HRX_LOG(HRX_DRV_DEBUG, "Force 6G mode\n");
				}	else if (tmds_lost_count == HRX_6G_TMDS_DETECT_THRESHOLD) {
					ret = hrx_phy_init(hrx_dev, true,
						false);
					if (hrx_is_5v_connected(hrx_dev))
						hrx_toggle_scdc_hpd(hrx_dev);
					HRX_LOG(HRX_DRV_ERROR, "hrx_toggle_scdc_hpd\n");
				}
				if (tmds_lost_count++ > HRX_6G_TMDS_DETECT_THRESHOLD) {
					hrx_enable_vital_ints(hrx_dev);
					ret = -ETIMEDOUT;
					tmds_lost_count = 0;
					goto out;
				}
				break;
			}

			/* If we reached this point then TMDS is valid */
			HRX_LOG(HRX_DRV_DEBUG, "[TMDS] is valid!\n");
			tmds_lost_count = 0;
			hrx_dev->hdmi_cstate = HDMI_CSTATE_DATAPATH;
			break;
		case HDMI_CSTATE_DATAPATH:
			hrx_config_packets(hrx_dev);
			hrx_config_video(hrx_dev);
			hrx_reset_datapath(hrx_dev);
			/*Commented below call to be able to receive audio in
			 *AIP - To be checked
			 */
			hrx_reset_audio(hrx_dev);
			hrx_clear_ints(hrx_dev);
			hrx_enable_vital_ints(hrx_dev);
			hrx_enable_ACR_ints(hrx_dev);
			hrx_dev->hdmi_cstate = HDMI_CSTATE_VIDEO_UNSTABLE;
			break;
		case HDMI_CSTATE_VIDEO_UNSTABLE:
			ret = hrx_wait_video_stable(hrx_dev);
			if (ret)
				break;
			hrx_dev->hdmi_cstate = HDMI_CSTATE_AUDIO;
			break;
		case HDMI_CSTATE_AUDIO:
			hrx_get_infoframes(hrx_dev);
			ret = hrx_config_audio(hrx_dev, just_reconfig_audio);
			if (ret)
				break;
			if (!just_reconfig_audio) {
				hrx_enable_video_ints(hrx_dev);
				hrx_enable_audio_ints(hrx_dev);
			}
			hrx_dev->audErrCnt = 0;
			hrx_dev->hdmi_cstate = HDMI_CSTATE_CONFIGURED;
			break;
		case HDMI_CSTATE_CONFIGURED:
			break;
		default:
			ret = -EINVAL;
			goto out;
		}

		if (hrx_dev->hdmi_cstate == HDMI_CSTATE_CONFIGURED) {
			HRX_LOG(HRX_DRV_INFO, "[INFO] controller configured\n");
			return 0;
		}
	}

out:
	hrx_dev->hdmi_cstate = HDMI_CSTATE_NOT_CONFIGURED;
	return ret;
}

static int hrx_config(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int ret;
	bool just_reconfig_audio = (hrx_dev->hdmi_cstate == HDMI_CSTATE_AUDIO);

	while (1) {
		/* Give up silently if we are forcing off */

		if (hrx_dev->force_off)
			return 0;
		/* Give up silently if input is disconnected */
		if (!hrx_is_5v_connected(hrx_dev)) {
			ret = 0;
			break;
		}
		switch (hrx_dev->hdmi_state) {
		case HDMI_STATE_NO_INIT:
			{
				HRX_LOG(HRX_DRV_DEBUG, "Hdmi RX State Not Initialised\n");
				return -EINVAL;
			}
		case HDMI_STATE_POWER_OFF:
			/* Initial state */
			hrx_disable_all_ints(hrx_dev);
			hrx_dev->hdmi_state = HDMI_STATE_PHY_CONFIG;
			break;
		case HDMI_STATE_PHY_CONFIG:
			HRX_LOG(HRX_DRV_DEBUG, "[SCDC] frl_rate=0x%x\n",
					(hrx_reg_read(hrx_dev, 0x58c) >> 8) & 0xf);
			/* Configure Phy */
			hrx_dev->is_hdmi2 = hrx_is_hdmi2(hrx_dev);
			hrx_dev->is_scrambled = is_scrambled(hrx_dev);
			hrx_dev->is_frl = is_frl(hrx_dev);
			ret = hrx_phy_init(hrx_dev, hrx_dev->is_hdmi2,
					false);
			if (ret)
				break;
			hrx_dev->hdmi_state = HDMI_STATE_CONTROLLER_CONFIG;
			break;
		case HDMI_STATE_CONTROLLER_CONFIG:
			/* Configure Controller */
			ret = hrx_config_controller(hrx_dev);
			if (ret == -ETIMEDOUT) {
				HRX_LOG(HRX_DRV_ERROR, "controller config TIMEDOUT\n");
				ret = -EINVAL;
				goto out;
			}
			if (ret)
				break;
			hrx_dev->hdmi_state = HDMI_STATE_POWER_ON;
			break;
		case HDMI_STATE_POWER_ON:
			break;
		default:
			HRX_LOG(HRX_DRV_ERROR, "driver is in an invalid state\n");
			ret = -EINVAL;
			goto out;
		}

		if (hrx_dev->hdmi_state == HDMI_STATE_POWER_ON) {
			HRX_LOG(HRX_DRV_INFO, "[INFO] HDMI-RX QP configured :%d\n", just_reconfig_audio);
			if (!just_reconfig_audio)
				hrx_handle_audio_mute_change(hrx_dev);
			hrx_enable_vital_ints(hrx_dev);
			return 0;
		}
	}


out:
	hrx_dev->hdmi_state = HDMI_STATE_POWER_OFF;
	hrx_dev->hdmi_cstate = HDMI_CSTATE_NOT_CONFIGURED;
	hrx_enable_vital_ints(hrx_dev);
	return ret;
}

static void hrx_force_off(struct syna_hrx_v4l2_dev *hrx_dev)
{

	hrx_dev->force_off = true;

	hrx_dev->force_off = false;
	hrx_dev->pending_config = false;
}

static void hrx_datapath_power_off(struct syna_hrx_v4l2_dev *hrx_dev)
{
	HRX_LOG(HRX_DRV_DEBUG, "In this fn %s\n", __func__);
	hrx_force_off(hrx_dev);

	hrx_dev->hdmi_state = HDMI_STATE_CONTROLLER_CONFIG;
	hrx_dev->hdmi_cstate = HDMI_CSTATE_EQUALIZER;
	/* Reset variables */
	hrx_dev->audio_sf = 0;
}

static void hrx_power_off(struct syna_hrx_v4l2_dev *hrx_dev)
{
	HRX_LOG(HRX_DRV_DEBUG, "In this fn %s\n", __func__);
//	hrx_force_off(hrx_dev);
	hrx_dev->hdmi_state = HDMI_STATE_POWER_OFF;
	hrx_dev->hdmi_cstate = HDMI_CSTATE_NOT_CONFIGURED;

	if (hrx_dev->vip_status == VIP_STATUS_START) {
		HRX_LOG(HRX_DRV_DEBUG, "Stopping VIP\n");
		vip_stop(hrx_dev);
	}

	if (hrx_dev->aip_status == AIP_STATUS_START) {
		HRX_LOG(HRX_DRV_DEBUG, "Stopping AIP\n");
		aip_stop(hrx_dev);
	}

//	 Reset variables
	hrx_dev->phy_locked = false;
	hrx_dev->audio_locked = false;
	hrx_dev->video_stable = false;
	hrx_dev->phy_eq_force = true;
	hrx_dev->audio_sf = 0;
	hrx_dev->cts = 0;
	hrx_dev->is_frl = 0;
	memset(&hrx_dev->audio_params,0,sizeof(hrx_auto_detect_audio_params));
}

static int hrx_power_on(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int ret;

	HRX_LOG(HRX_DRV_DEBUG, "In this fn %s\n", __func__);
	ret = hrx_config(hrx_dev);
	if (ret)
		HRX_LOG(HRX_DRV_ERROR, "failed to configure HDMI\n");
	hrx_dev->pending_config = false;

	return ret;
}

static void hrx_handle_tmds_change(struct syna_hrx_v4l2_dev *hrx_dev)
{
	bool prev_phy_locked = hrx_dev->phy_locked;

	hrx_dev->phy_locked = hrx_tmds_valid(hrx_dev);
	HRX_LOG(HRX_DRV_DEBUG, "[TMDSVALID] %d -> %d\n",
			prev_phy_locked, hrx_dev->phy_locked);

	if (hrx_dev->phy_locked) {
		/*Recheck tmds_valid as randomly it is getting unstable after
		 *sometime during resolution change
		 */
		msleep_interruptible(20);
		hrx_dev->phy_locked = hrx_tmds_valid(hrx_dev);
		HRX_LOG(HRX_DRV_DEBUG, "[TMDSVALID] %d -> %d\n",
				prev_phy_locked, hrx_dev->phy_locked);
	}

	if (!hrx_dev->phy_locked) { /* TMDSVALID lost */
		hrx_datapath_power_off(hrx_dev);
		if (prev_phy_locked != hrx_dev->phy_locked) {
			if (!hrx_dev->is_hdmi2)
				hrx_phy_re_init(hrx_dev);
		} else {
			if (hrx_is_5v_connected(hrx_dev))
				hrx_power_on(hrx_dev);
		}
	} else /* TMDSVALID gained */
		hrx_power_on(hrx_dev);
}

void hrx_handle_clock_change(struct syna_hrx_v4l2_dev *hrx_dev)
{
	if ((!has_clock(hrx_dev)) || !(hrx_is_5v_connected(hrx_dev))) {
		HRX_LOG(HRX_DRV_DEBUG, "[TMDS] clock is off\n");
		hrx_phy_tmds_clock_ratio(hrx_dev, hrx_is_hdmi2(hrx_dev));
		hrx_enable_vital_ints(hrx_dev);
	} else {
		HRX_LOG(HRX_DRV_DEBUG, "[TMDS] clock is on\n");
		if (!hrx_tmds_valid(hrx_dev))
			hrx_phy_re_init(hrx_dev);
		hrx_power_on(hrx_dev);
	}
}

static int hrx_auto_detect_audio(struct syna_hrx_v4l2_dev *hrx_dev, phrx_auto_detect_audio_params pAudioParam)
{
	int RetStatus = 0;
	hrx_auto_detect_audio_params tmpAudioParam;
	u32 ch_status_sp0;

	if (hrx_dev->video_params.IsValid == false)
		return -1;

	if (!hrx_dev->video_params.IsHdmiMode) {
		pAudioParam->IsLinearPCM = true;
		pAudioParam->SampFreq = 48000;
		pAudioParam->NumOfChannels = 2;
		return 0;// for DVI
	}

	memcpy(&tmpAudioParam, pAudioParam, sizeof(hrx_auto_detect_audio_params));
	pAudioParam->N =  hrx_get_n(hrx_dev);
	// depends on the video resolution
	// to resonstruct audio clk
	pAudioParam->CTS =  hrx_get_cts(hrx_dev);
	pAudioParam->SampFreq = hrx_get_sample_freq(hrx_dev, FALSE);
	pAudioParam->SampleSize = hrx_get_sample_width(hrx_dev);
	ch_status_sp0 = hrx_reg_read(hrx_dev, HDMI_AUDIO_PROC_CHSTAT_SP0_L(0));
	pAudioParam->IsLinearPCM  = ((ch_status_sp0 & BIT(1)) == 0);
	pAudioParam->rx_tmdsfreq = hrx_get_tmds_clk(hrx_dev);
	pAudioParam->IsHBR = (pAudioParam->SampFreq > 192000?1:0);
	if (pAudioParam->IsHBR)
		pAudioParam->SampFreq = pAudioParam->SampFreq/4;
	if (pAudioParam->IsLinearPCM)
		pAudioParam->NumOfChannels = hrx_dev->audioif.infoframe.general.audio.channels;
	else if (pAudioParam->IsHBR)
		pAudioParam->NumOfChannels = 8;
	else
		pAudioParam->NumOfChannels = 2;

	if (2 > pAudioParam->NumOfChannels || 8 < pAudioParam->NumOfChannels) {
		HRX_LOG(HRX_DRV_DEBUG, "HRX_DRV_INFO: invalid audio channel num (%d), set it to 2 channels\n", pAudioParam->NumOfChannels);
		pAudioParam->NumOfChannels = 2;
	} else if (2 < pAudioParam->NumOfChannels && 6 > pAudioParam->NumOfChannels) {
		HRX_LOG(HRX_DRV_DEBUG, "HRX_DRV_INFO: invalid input channel num (%d), set it to 6 channels\n", pAudioParam->NumOfChannels);
		pAudioParam->NumOfChannels = 6;
	} else if (6 < pAudioParam->NumOfChannels && 8 > pAudioParam->NumOfChannels) {
		HRX_LOG(HRX_DRV_DEBUG, "HRX_DRV_INFO: invalid input channel num (%d), set it to 8 channels\n", pAudioParam->NumOfChannels);
		pAudioParam->NumOfChannels = 8;
	}

	HRX_LOG(HRX_DRV_DEBUG, "HRX_DRV_INFO: Audio param samFre=%d Channel No = %d IsLinearPCM = %d N = %d CTS :%d\n", pAudioParam->SampFreq, pAudioParam->NumOfChannels,
		pAudioParam->IsLinearPCM, pAudioParam->N, pAudioParam->CTS);
	return RetStatus;

}

static void hrx_print_channel_status(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 chstatus;
	int i, j;

	for (i = 0; i < HDMI_AUDIO_PROC_CHSTAT_SIZE; i++) {
		chstatus = hrx_reg_read(hrx_dev, HDMI_AUDIO_PROC_CHSTAT_SP0_L(i));
		for (j = 0; j < 4; j++) {
			HRX_LOG(HRX_DRV_DEBUG, "0x%02x ",
				(chstatus >> (j * 8)) & 0xff);
		}
	}

	for (i = 0; i < HDMI_AUDIO_PROC_CHSTAT_SIZE; i++) {
		chstatus = hrx_reg_read(hrx_dev, HDMI_AUDIO_PROC_CHSTAT_SP0_R(i));
		for (j = 0; j < 4; j++) {
			HRX_LOG(HRX_DRV_DEBUG, "0x%02x ",
				(chstatus >> (j * 8)) & 0xff);
		}
	}
}

static void hrx_audio_power_off(struct syna_hrx_v4l2_dev *hrx_dev)
{
	//dw_hdmi_force_off(dw_dev);
	hrx_dev->force_off = true;
	//flush_workqueue(hrx_dev->dw_dev->wq);
	mdelay(100);
	hrx_dev->force_off = false;

	hrx_dev->pending_config = false;

	hrx_dev->hdmi_state = HDMI_STATE_CONTROLLER_CONFIG;
	hrx_dev->hdmi_cstate = HDMI_CSTATE_AUDIO;

	/* Reset variables */
	hrx_dev->audio_sf = 0;
}

void hrx_audio_reset (struct syna_hrx_v4l2_dev *hrx_dev)
{
	hrx_datapath_power_off(hrx_dev);
	hrx_power_on(hrx_dev);
	hrx_dev->vip_restart = true;
	hrx_dev->aip_restart = true;
	hrx_auto_detect_audio(hrx_dev, &hrx_dev->audio_params);
	HRX_LOG(HRX_DRV_DEBUG, "Reset audio since N value is 0 \n");
}

static int hrx_hbr_mute_wa(struct syna_hrx_v4l2_dev *hrx_dev, int val)
{
	u32 audio_format = 0;
	int ret = 0;

	audio_format = hrx_get_audio_format(hrx_dev);

	if (val == 1 || audio_format != EARCTX_AUDIO_FORMAT_ASP) {
		hrx_reg_mask_write(hrx_dev, val,
				HDMI_AUDIO_PROC_CONFIG2,
				HDMI_AUDIO_PROC_CONFIG2_AUD_FMT_CHG_MUTEMASK_N_OFFSET,
				HDMI_AUDIO_PROC_CONFIG2_AUD_FMT_CHG_MUTEMASK_N_MASK);
	}

	HRX_LOG(HRX_DRV_DEBUG, "%s: val=%d\n", __func__, val);

	return ret;
}

int hrx_config_channel_spread_en(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 aud_chan_spread_en = 0;
	u32 hbr_mute_val = 0x1;
	int ret = -EINVAL;
	u32 audio_format;
	u32 audio_layout;

	audio_format = hrx_get_audio_format(hrx_dev);
	audio_layout = hrx_get_audio_layout(hrx_dev);

	/* configure channel spread enable */
	switch (audio_format) {
	case EARCTX_AUDIO_FORMAT_ASP:
		//TODO:
		break;
	case EARCTX_AUDIO_FORMAT_HBR:
		aud_chan_spread_en = 1;
		hbr_mute_val = 0x0;
		ret = 0;
		break;
	case EARCTX_AUDIO_FORMAT_OBA:
		HRX_LOG(HRX_DRV_DEBUG, "The audio fmt OBA is not supported");
		break;
	case EARCTX_AUDIO_FORMAT_DTS:
		HRX_LOG(HRX_DRV_DEBUG, "The audio fmt 3D_OBA is not supported");
		break;
	case EARCTX_AUDIO_FORMAT_MSA:
		HRX_LOG(HRX_DRV_DEBUG, "The audio fmt MSA is not supported");
		break;
	case EARCTX_AUDIO_FORMAT_OBMSA:
		HRX_LOG(HRX_DRV_DEBUG, "The audio fmt OBMSA is not supported");
		break;
	case EARCTX_AUDIO_FORMAT_3D:
		HRX_LOG(HRX_DRV_DEBUG, "The audio fmt 3D is not supported");
		break;
	case EARCTX_AUDIO_FORMAT_3D_OBA:
		HRX_LOG(HRX_DRV_DEBUG, "The audio fmt 3D_OBA is not supported");
		break;
	default:
		HRX_LOG(HRX_DRV_DEBUG, "The audio format is not supported");
		break;
	}

	hrx_reg_mask_write(hrx_dev, aud_chan_spread_en,
			   HDMI_AUDIO_PROC_CONFIG0,
			   HDMI_AUDIO_PROC_CONFIG0_CHAN_SPREAD_EN_OFFSET,
			   HDMI_AUDIO_PROC_CONFIG0_CHAN_SPREAD_EN_MASK);

	HRX_LOG(HRX_DRV_DEBUG, "%s: enable=%d\n", __func__, aud_chan_spread_en);

	return ret;
}

int hrx_get_aud_err(struct syna_hrx_v4l2_dev *hrx_dev, u32 *pAudErr)
{
    /* Monitor to enable audio fifo */
	if (hrx_dev->audFifoDis == 1)
		hrx_check_sample_flat_enabled(hrx_dev);

	if (hrx_dev->audErrCnt > 5) {
		HRX_LOG(HRX_DRV_ERROR, "audio error count : %d\n", hrx_dev->audErrCnt);
		*pAudErr = 1;
		hrx_dev->audErrCnt = 0;
	} else
		*pAudErr = 0;

	return 0;
}

static void hrx_handle_video_change(struct syna_hrx_v4l2_dev *hrx_dev)
{
	struct v4l2_dv_timings timings;

	if (hrx_tmds_valid(hrx_dev) && hdmi_is_on(hrx_dev)) {
		HRX_LOG(HRX_DRV_DEBUG, "[VIDEO] video change interrupt\n");
		hrx_query_dv_timings(hrx_dev, &timings);
		hrx_datapath_power_off(hrx_dev);
		hrx_power_on(hrx_dev);
	} else
		hrx_power_off(hrx_dev);

	hrx_dev->video_stable = true;
}

static void hrx_handle_audio_change(struct syna_hrx_v4l2_dev *hrx_dev,
		u32 avp1_stat, u32 pkt0_stat)
{
	bool restart = true;

	if ((avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AUD_CHSTATUS_SP0) ||
	    (avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AUD_CHSTATUS_SP1) ||
	    (avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AUD_CHSTATUS_SP2) ||
	    (avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AUD_CHSTATUS_SP3)) {
		HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] audio ch status (0x%x)\n",
			avp1_stat);
		hrx_print_channel_status(hrx_dev);
		hrx_handle_audio_mute_change(hrx_dev);
		hrx_config_channel_spread_en(hrx_dev);
		restart = false;
	}
	if (pkt0_stat & HDMI_PKT_0_INT_STATUS_ACR) {
		hrx_reg_read(hrx_dev, HDMI_PKTDEC_ACR_PH);
		HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] ACR change (CTS=%d,N=%d)\n",
			hrx_reg_read(hrx_dev, HDMI_PKTDEC_ACR_PB(0)),
			hrx_reg_read(hrx_dev, HDMI_PKTDEC_ACR_PB(1)));
		restart = false;
	}
	if (avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AFIFO_UNDERFLOW) {
		/* to avoid audio format change loop restarting fifo in
		 * overflow due to keep old aufio infoframes values that affect
		 * audio fifo rate extraction
		 */
		HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] fifo underflow\n");
		restart = true;
	}
	if (avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AFIFO_OVERFLOW) {
		/* to avoid audio format change loop restarting fifo in
		 * overflow due to keep old aufio infoframes values that affect
		 * audio fifo rate extraction
		 */
		HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] fifo overflow\n");
		restart = true;
		hrx_dev->audErrCnt++;
		HRX_LOG(HRX_DRV_DEBUG, "Audio err count :%d\n", hrx_dev->audErrCnt);
	}
	if (avp1_stat & HDMI_AVPUNIT_1_INT_MASK_N_AUD_FMT_CHG_MASK) {
		HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] format change\n");
		restart = true;
	}
	if (pkt0_stat & HDMI_PKT_0_INT_STATUS_ACR_N) {
		HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] N change (%d)\n",
			hrx_get_n(hrx_dev));
		restart = true;
		if (hdmi_is_on(hrx_dev) || hrx_dev->hdmi_cstate == HDMI_CSTATE_AUDIO) {
			hrx_audio_power_off(hrx_dev);
			hrx_power_on(hrx_dev);
		}
	}
	if (pkt0_stat & HDMI_PKT_0_INT_STATUS_ACR_CTS) {
		HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] CTS change (%u)\n",
			hrx_get_cts(hrx_dev));
		restart = true;
	}
	if (avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AFIFO_THR_PASS) {
		hrx_hbr_mute_wa(hrx_dev, 1);
		HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] TRH Pass\n");
		restart = false;
	}

	if ((pkt0_stat & HDMI_PKT_0_INT_STATUS_AUDIF) &&
	    (avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AUD_MUTE)) {
		/* to avoid audio format change loop restarting fifo in
		 * overflow due to keep old aufio infoframes values that affect
		 * audio fifo rate extraction
		 */
		HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] audio infoframe for new audio type\n");
		restart = true;
	}

	if (restart) {
		hrx_config_channel_spread_en(hrx_dev);
		hrx_reg_write(hrx_dev, 0x1, HDMI_PKTDEC_ACR_MAXMIN_CLEAR);
		hrx_restart_audio_fifo(hrx_dev);
		HRX_LOG(HRX_DRV_DEBUG, "audio fifo enable :%d\n", __LINE__);
	}
}


static void hrx_handle_audio_pll_clk_lkd_change(struct syna_hrx_v4l2_dev *hrx_dev)
{
	bool prev_audio_locked = hrx_dev->audio_locked;

	hrx_dev->audio_locked = hrx_audio_valid(hrx_dev) &&
				has_audio_clock(hrx_dev) &&
				has_audio_locked(hrx_dev);
	HRX_LOG(HRX_DRV_DEBUG, "[AUDIO] audio_locked=%d->%d (audpll)\n",
			prev_audio_locked, hrx_dev->audio_locked);
}

static void hrx_handle_aip_vip_restart(struct syna_hrx_v4l2_dev *hrx_dev)
{
	if ((hrx_dev->vip_status != VIP_STATUS_STOP) && (hrx_dev->vip_restart == true)) {
		HRX_LOG(HRX_DRV_DEBUG, "[Stopping vip]\n");
		vip_stop(hrx_dev);
	}

	if ((hrx_dev->aip_status != AIP_STATUS_STOP) && (hrx_dev->aip_restart == true)) {
		HRX_LOG(HRX_DRV_DEBUG, "[Stopping Aip]\n");
		aip_stop(hrx_dev);
	}

	if ((hrx_dev->hrx_v4l2_state == HRX_V4L2_STREAMING_ON) && (hrx_dev->hdmi_state == HDMI_STATE_POWER_ON) && (hrx_dev->vip_restart == true)) {
		HRX_LOG(HRX_DRV_DEBUG, "[Starting vip]\n");
		vip_start(hrx_dev);
	}

	if ((hrx_dev->hrx_alsa_state == HRX_ALSA_STREAMING_ON) && (hrx_dev->hdmi_state == HDMI_STATE_POWER_ON) && (hrx_dev->aip_restart == true)) {
		if (hrx_dev->aip_status != AIP_STATUS_START) {
			HRX_LOG(HRX_DRV_DEBUG, "[Starting Aip]\n");
			aip_start(hrx_dev, hrx_dev->in_data.channels);
		}
	}

	hrx_dev->vip_restart = false;
	hrx_dev->aip_restart = false;
}

static unsigned int hrx_isr_irq_handler(struct syna_hrx_v4l2_dev *hrx_dev, u32 irq)
{
	u32 mu0_stat = 0, mu2_stat = 0, pkt0_stat = 0;
	u32 avp1_stat = 0, scdc_stat = 0, hdcp_stat = 0;
	u32 avp1_check, pkt0_check, avp1_mask, pkt0_mask;
	u32 cmu_stat;

	hrx_dev->vip_restart = false;
	hrx_dev->aip_restart = false;

	HRX_LOG(HRX_DRV_DEBUG, "%s: irq : %x\n", __func__, irq);

	mu0_stat = 0x0 | (irq & HDMI_MAINUNIT_0_INT_STATUS_TMDSQP_CK_OFF);
	mu2_stat = 0x0 | ((irq >> 2) & HDMI_MAINUNIT_2_INT_STATUS_TMDSVALID_STABLE);
	pkt0_stat = 0x0 | ((irq >> 12) & HDMI_PKT_0_INT_STATUS_AUDIF) |
			   (irq & PKT_0_INTR) | ((irq >> 27) & PKT_0_ACR);
	scdc_stat = 0x0;
	hdcp_stat = 0x0 | (irq & HDCP_INTR) | ((irq >> 4) & HDMI_HDCP_INT_STATUS_AKSV_RCV);
	avp1_stat = irq & AVPUNIT1_INTR;
	HRX_LOG(HRX_DRV_DEBUG, "%s: m0:%x, m2:%x, pkt0:%x, scdc:%x, avp1:%x, hdcp:%x\n", __func__,
					mu0_stat, mu2_stat, pkt0_stat, scdc_stat, avp1_stat, hdcp_stat);

	cmu_stat = hrx_reg_read(hrx_dev, HDMI_CMU_STATUS);


	if ((mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_TIMER_BASE_LOCKED) &&
		(cmu_stat & HDMI_CMU_STATUS_TIMER_BASE_LOCKED))
		hrx_dev->timer_base_locked = true;

	if (!hrx_dev->regbank_ready || !hrx_dev->apb_if_ready ||
			(hrx_dev->hdmi_state == HDMI_STATE_NO_INIT)) {
		return 0;
	}

	avp1_check = hrx_reg_read(hrx_dev, HDMI_AVPUNIT_1_INT_STATUS);
	pkt0_check = hrx_reg_read(hrx_dev, HDMI_PKT_0_INT_STATUS);
	if ((avp1_check & (AVPUNIT1_INTR)) | (pkt0_check & (PKT_0_INTR | HDMI_PKT_0_INT_STATUS_AUDIF))) {
		avp1_mask = hrx_reg_read(hrx_dev, HDMI_AVPUNIT_1_INT_MASK_N);
		pkt0_mask = hrx_reg_read(hrx_dev, HDMI_PKT_0_INT_MASK_N);
		HRX_LOG(HRX_DRV_DEBUG, "Error : %x(%x):%x(%x)\n", avp1_check, avp1_mask, pkt0_check, pkt0_mask);
		hrx_reg_write(hrx_dev, avp1_mask, HDMI_AVPUNIT_1_INT_MASK_N);
		hrx_reg_write(hrx_dev, pkt0_mask, HDMI_PKT_0_INT_MASK_N);
		hrx_clear_ints(hrx_dev);
		hrx_dev->vip_restart = true;
		hrx_dev->aip_restart = true;
	}

	if (mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_TMDSQP_CK_OFF) {
		hrx_handle_clock_change(hrx_dev);
		hrx_dev->vip_restart = true;
		hrx_dev->aip_restart = true;
	}

	if (mu2_stat & HDMI_MAINUNIT_2_INT_STATUS_TMDSVALID_STABLE) {
		hrx_handle_tmds_change(hrx_dev);
		hrx_dev->vip_restart = true;
		hrx_dev->aip_restart = true;
	}

	if (avp1_stat & HDMI_IRQ_VIDEO_CHG_FLAG) {
		hrx_handle_video_change(hrx_dev);
		hrx_dev->vip_restart = true;
		hrx_dev->aip_restart = true;
	}


	if (mu2_stat & HDMI_MAINUNIT_2_INT_STATUS_AUDPLL_LOCK_STABLE)
		hrx_handle_audio_pll_clk_lkd_change(hrx_dev);

	if (mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_AUDIO_CK_OFF)
		hrx_handle_audio_pll_clk_lkd_change(hrx_dev);

	if (mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_AUDIO_CK_LOCKED)
		hrx_handle_audio_pll_clk_lkd_change(hrx_dev);

	if ((avp1_stat & HDMI_IRQ_AUDIO_AVP1_FLAG) ||
		(pkt0_stat & HDMI_IRQ_AUDIO_PKT0_FLAG)) {
		HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] audio\n");
		hrx_handle_audio_change(hrx_dev, avp1_stat, pkt0_stat);
		hrx_auto_detect_audio(hrx_dev, &hrx_dev->audio_params);
	}

	if ((avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AFIFO_THR_PASS) ||
	    (avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AUD_MUTE) ||
	    (pkt0_stat & HDMI_PKT_0_INT_STATUS_GCP)) {
		hrx_handle_audio_mute_change(hrx_dev);
		if (avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_AUD_MUTE) {
			HRX_LOG(HRX_DRV_DEBUG, "[interrupt:audio] audio mute\n");
		}
	}

	if (pkt0_stat & HDMI_IRQ_IF_FLAG)
		hrx_get_infoframes(hrx_dev);

	if (scdc_stat) {
		if (hrx_dev->is_hdmi2 != hrx_is_hdmi2(hrx_dev)) {
			HRX_LOG(HRX_DRV_INFO, "[SCDC] hdmi2=%d->%d, scrambling=%d->%d, frl=%d->%d\n",
					hrx_dev->is_hdmi2, hrx_is_hdmi2(hrx_dev),
					hrx_dev->is_scrambled, is_scrambled(hrx_dev),
					hrx_dev->is_frl, is_frl(hrx_dev));

			hrx_datapath_power_off(hrx_dev);
			hrx_power_on(hrx_dev);
			hrx_dev->vip_restart = true;
			hrx_dev->aip_restart = true;
		}

		HRX_LOG(HRX_DRV_INFO, "[SCDC] stat = 0x%x, 0x584 = 0x%x, frl=0x%x\n",
			 scdc_stat, hrx_reg_read(hrx_dev, 0x584), is_frl(hrx_dev));
	}

	if (avp1_stat & HDMI_AVPUNIT_1_INT_STATUS_DEFRAMER_GBDET_ERR)
		HRX_LOG(HRX_DRV_ERROR, "[ERROR] Guard Band Error\n");

	return 0;
}

int hrx_isr_state_update(struct syna_hrx_v4l2_dev *hrx_dev, CC_MSG_t msg)
{
	int hrx_intr = 0xff;
	u32 irq = 0;
	int ret, iter;
	int hrx_dev_prev_state;
	static u32 curr_vic;
	static struct hdmi_rx_input_change_event hrx_prev_event;
	static u32 prev_vic;
	struct hdmi_rx_input_change_event hrx_in_event = {0,0,0,0,0};

	hrx_intr = msg.m_Param1;
	irq = msg.m_Param2;
	prev_vic = curr_vic;

	HRX_LOG(HRX_DRV_DEBUG, "intr = %x irq : %llxx\n", msg.m_Param1, (long long)msg.m_Param2);
	hrx_dev_prev_state = hrx_dev->HrxState;
	switch(hrx_intr) {
		case HDMIRX_INTR_SYNC:
			hrx_dev->hrx_cmd_id = HRX_CMD_CLOCK_CHANGE;
			break;
		case HDMIRX_INTR_PKT:
			hrx_dev->hrx_cmd_id = HRX_CMD_PACKET_CHANGE;
			break;
		case HDMIRX_INTR_CHNL_STS:
			hrx_dev->hrx_cmd_id = HRX_CMD_AUDIO_CHANGE;
			break;
		default:
			HRX_LOG(HRX_DRV_ERROR,"Invalid intr : %d \n",hrx_intr);
			return 0;

	}

	switch (hrx_dev->HrxState) {
	case HRX_STATE_DISCONNECTED:
	if (hrx_dev->hrx_cmd_id == HRX_CMD_CLOCK_CHANGE) {
		if (!hrx_is_5v_connected(hrx_dev)) {
			hrx_isr_irq_handler(hrx_dev, irq);
			HRX_LOG(HRX_DRV_DEBUG, "invalid video, disconnected -> disconnected\n");
		} else {
			hrx_dev->HrxState = HRX_STATE_UNSTABLE;
			hrx_dev->video_params.IsValid = false;
			hrx_isr_irq_handler(hrx_dev, irq);
			ret = hrx_configure_timing_params(hrx_dev);
			if (ret == 0) {
				hrx_dev->HrxState = HRX_STATE_ALL_STABLE;
				hrx_dev->video_params.IsValid = true;
				hrx_dev->video_params.Vic = hrx_dev->video_params.HrxIpTimingParam.DispResId;
				curr_vic = hrx_dev->video_params.Vic;
				HRX_LOG(HRX_DRV_DEBUG, " state change disconnect -> stable\n");
			} else
				HRX_LOG(HRX_DRV_DEBUG, " state change disconnect -> unstable\n");
		}
	}
		break;
	case HRX_STATE_UNSTABLE:
	if (hrx_dev->hrx_cmd_id == HRX_CMD_CLOCK_CHANGE) {
		if (!hrx_is_5v_connected(hrx_dev)) {
			hrx_dev->HrxState = HRX_STATE_DISCONNECTED;
			HRX_LOG(HRX_DRV_DEBUG, "clk invalid video, unstable -> disconnected\n");
			hrx_isr_irq_handler(hrx_dev, irq);
		} else {
			hrx_dev->video_params.IsValid = false;
			hrx_isr_irq_handler(hrx_dev, irq);
			ret = hrx_configure_timing_params(hrx_dev);
			if (ret == 0) {
				hrx_dev->HrxState = HRX_STATE_ALL_STABLE;
				hrx_dev->video_params.IsValid = true;
				hrx_dev->video_params.Vic = hrx_dev->video_params.HrxIpTimingParam.DispResId;
				curr_vic = hrx_dev->video_params.Vic;
				HRX_LOG(HRX_DRV_DEBUG, " clk state change unstable -> stable\n");

			} else {
				hrx_dev->video_params.IsValid = false;
				HRX_LOG(HRX_DRV_DEBUG, "clk invalid video, unstable -> unstable\n");

				for (iter = 0; iter < 40; iter++) {
					if (has_clock(hrx_dev))
						break;
					if (!hrx_is_5v_connected(hrx_dev)) {
						hrx_dev->HrxState = HRX_STATE_DISCONNECTED;
				  // call back disconnected
						HRX_LOG(HRX_DRV_DEBUG, "Additional check for 5V triggered at iter = %d\n", iter);
						HRX_LOG(HRX_DRV_DEBUG, "clk invalid video, unstable -> disconnected\n");
						break;
					}
				}
			}
		}
	}
		break;
	case HRX_STATE_ALL_STABLE:
	if (hrx_dev->hrx_cmd_id == HRX_CMD_CLOCK_CHANGE) {
		if (!hrx_is_5v_connected(hrx_dev)) {
			hrx_dev->HrxState = HRX_STATE_DISCONNECTED;
			HRX_LOG(HRX_DRV_DEBUG, "clk invalid video, allstable -> disconnected\n");
			hrx_isr_irq_handler(hrx_dev, irq);
		} else {
			hrx_dev->video_params.IsValid = false;
			hrx_dev->HrxState = HRX_STATE_UNSTABLE;
			hrx_isr_irq_handler(hrx_dev, irq);
			ret = hrx_configure_timing_params(hrx_dev);
			if (ret == 0) {
				hrx_dev->HrxState = HRX_STATE_ALL_STABLE;
				hrx_dev->video_params.IsValid = true;
				hrx_dev->video_params.Vic = hrx_dev->video_params.HrxIpTimingParam.DispResId;
				curr_vic = hrx_dev->video_params.Vic;
				HRX_LOG(HRX_DRV_DEBUG, "clk, all stable -> all stable\n");
				hrx_dev->trig_scl_reset = true;
			} else {
				hrx_dev->video_params.IsValid = false;
				HRX_LOG(HRX_DRV_DEBUG, "clk change, all stable -> unstable\n");
			}
		}
	} else if (hrx_dev->hrx_cmd_id == HRX_CMD_PACKET_CHANGE) {
// process these pakcets and give call back
		if (hrx_tmds_valid(hrx_dev)) {
			hrx_isr_irq_handler(hrx_dev, irq);
			hrx_configure_timing_params(hrx_dev);
			if (irq & 0x2000000) {
				HRX_LOG(HRX_DRV_INFO, "Received Audio infoframe\n");
				if (hrx_get_sample_freq(hrx_dev, FALSE) == 0)
					HRX_LOG(HRX_DRV_DEBUG, "Sample  freq is 0 not notifying\n");
				else
					HRX_LOG(HRX_DRV_DEBUG, "Sample notifying\n");
			}
		HRX_LOG(HRX_DRV_DEBUG, "packet change, all stable -> all stable\n");
		} else {
			hrx_dev->HrxState = HRX_STATE_UNSTABLE;
			HRX_LOG(HRX_DRV_DEBUG, "packet change, all_stable ->unstable\n");
		}
	} else if (hrx_dev->hrx_cmd_id == HRX_CMD_AUDIO_CHANGE) {
		if (hrx_tmds_valid(hrx_dev)) {
			hrx_isr_irq_handler(hrx_dev, irq);
			if (hrx_get_sample_freq(hrx_dev, FALSE) == 0)
				HRX_LOG(HRX_DRV_DEBUG, "Sample  freq is 0 not notifying\n");
		} else {
			      //HRX_SetState(hrx_dev,HRX_STATE_UNSTABLE);
			HRX_LOG(HRX_DRV_DEBUG, "Ignore Audio change as tmds is not valid\n");
		}
		HRX_LOG(HRX_DRV_DEBUG, "audio change, all_stable ->all stable\n");
	}
		break;
	default:
		HRX_LOG(HRX_DRV_ERROR, "Invalid state\n");
		return 0;
	}


	if (hrx_dev->HrxState == HRX_STATE_ALL_STABLE) {
		hrx_update_active_frame_interval(hrx_dev);
		hrx_update_total_frame_interval(hrx_dev);
		hrx_handle_aip_vip_restart(hrx_dev);
		hrx_in_event.hrx_stable_state = 1;
		hrx_in_event.width = hrx_dev->video_params.HrxIpTimingParam.HActive;
		hrx_in_event.height = hrx_dev->video_params.HrxIpTimingParam.VActive;
		hrx_in_event.fi_num = hrx_dev->video_params.FITotal.numerator;
		hrx_in_event.fi_den = hrx_dev->video_params.FITotal.denominator;
		HRX_LOG(HRX_DRV_DEBUG, "curr_vic : %u\n",curr_vic);
	}

	if ((hrx_dev->hrx_cmd_id == HRX_CMD_CLOCK_CHANGE) &&
		((hrx_prev_event.hrx_stable_state != hrx_in_event.hrx_stable_state) ||
		(hrx_prev_event.width != hrx_in_event.width) ||
		(hrx_prev_event.height != hrx_in_event.height) ||
		(hrx_prev_event.fi_num != hrx_in_event.fi_num) ||
		(hrx_prev_event.fi_den != hrx_in_event.fi_den) ||
		(curr_vic != prev_vic))) {
		hrx_sig_stat_set_attr (&hrx_in_event);
		memcpy(&hrx_prev_event,&hrx_in_event,sizeof(struct hdmi_rx_input_change_event));
	}
	return 0;
}

static int syna_hrx_v4l2_probe(struct platform_device *pdev)
{
	struct syna_hrx_v4l2_dev *hrx_dev;
	struct device *dev = &pdev->dev;
	int ret = -1;

	/* Defer probe until dependent soc module/s are probed/initialized */
	if (!is_avio_driver_initialized() || !Dhub_GetDhubHandle_ByDhubId(DHUB_ID_AG_DHUB)) {
		HRX_LOG(HRX_DRV_INFO, "Defering probe\n");
		return -EPROBE_DEFER;
	}

	hrx_dev = devm_kzalloc(&pdev->dev, sizeof(*hrx_dev), GFP_KERNEL);
	if (!hrx_dev) {
		return -ENOMEM;
	}

	dev_set_drvdata(dev, (void *)hrx_dev);

	pdev_glbl = pdev;

	hrx_dev->dev = dev;

	hrx_dev->regbank_ready = false;
	hrx_dev->apb_if_ready = false;
	hrx_dev->reserved_buf = NULL;

	mutex_init(&hrx_dev->vip_mutex);

	ret = hrx_sig_stat_create();
	if (ret < 0)
		goto EXIT;

	ret = syna_hrx_parse_dt(pdev);
	if (ret < 0)
		goto EXIT1;

	ret = syna_hrx_init_dhub(hrx_dev);
	if (ret < 0)
		goto EXIT1;

	ret = syna_hrx_interrupt_config(pdev);
	if (ret < 0)
		goto EXIT1;

	ret = syna_hrx_video_init(hrx_dev);
	if (ret < 0)
		goto EXIT1;

	ret = syna_hrx_audio_init(hrx_dev);
	if (ret < 0) {
		HRX_LOG(HRX_DRV_ERROR, "Can't Initialize audio\n");
		goto EXIT2;
	} else
		HRX_LOG(HRX_DRV_INFO, "audio init succcessfully\n");

	ret = hrx_init(hrx_dev);
	if (ret < 0)
		goto EXIT3;

	ret = syna_hrx_get_hrx5v(hrx_dev);
	if (ret < 0) {
		goto EXIT4;
	}

	hrx_dev->mem_list = devm_kzalloc(&pdev->dev, sizeof(VPP_MEM_LIST), GFP_KERNEL);

	if (!hrx_dev->mem_list) {
		ret = -ENOMEM;
		goto EXIT4;
	}

	hrx_dev->mem_list->dev = dev;

	ret = VPP_MEM_InitMemory(hrx_dev->mem_list);
	if (ret != 0) {
		ret = -ENOMEM;
		goto EXIT4;
	}

	hrx_dev->aip_mem_list = devm_kzalloc(&pdev->dev, sizeof(VPP_MEM_LIST), GFP_KERNEL);

	if (!hrx_dev->aip_mem_list) {
		ret = -ENOMEM;
		goto EXIT5;
	}

	hrx_dev->aip_mem_list->dev = dev;

	ret = VPP_MEM_InitMemory(hrx_dev->aip_mem_list);
	if (ret != 0) {
		ret = -ENOMEM;
		goto EXIT5;
	}

	vip_init(hrx_dev);
	aip_init(hrx_dev);

	aip_set_audio_mono_mode(hrx_dev, AIP_STEREO);
	hrx_dev->null_data_count = 0;

	ret = hrx_debug_create();
	if (ret != 0)
		goto EXIT6;

	return 0;
EXIT6:
	VPP_MEM_DeInitMemory(hrx_dev->aip_mem_list);
EXIT5:
	VPP_MEM_DeInitMemory(hrx_dev->mem_list);
EXIT4:
	hrx_deinit(hrx_dev);
EXIT3:
	syna_hrx_audio_exit(hrx_dev);
EXIT2:
	syna_hrx_video_finalize(hrx_dev);
EXIT1:
	hrx_sig_stat_remove();
EXIT:
	mutex_destroy(&hrx_dev->vip_mutex);
	return ret;
}

static RET syna_hrx_v4l2_remove(struct platform_device *pdev)
{
	struct syna_hrx_v4l2_dev *hrx_dev;

	hrx_dev = (struct syna_hrx_v4l2_dev *)dev_get_drvdata(&pdev->dev);

	aip_close(hrx_dev);
	vip_close(hrx_dev);
	hrx_deinit(hrx_dev);
	syna_hrx_audio_exit(hrx_dev);
	syna_hrx_video_finalize(hrx_dev);

	Dhub_IntrRegisterHandler(DHUB_ID_VPP_DHUB, avioDhubSemMap_vpp128b_vpp_inr9, hrx_dev, NULL);
	Dhub_IntrRegisterHandler(DHUB_ID_VPP_DHUB, avioDhubSemMap_vpp128b_vpp_inr10, hrx_dev, NULL);

	if (hrx_dev->mem_list) {
		VPP_MEM_DeInitMemory(hrx_dev->mem_list);
	}

	if (hrx_dev->aip_mem_list) {
		VPP_MEM_DeInitMemory(hrx_dev->aip_mem_list);
	}

	hrx_debug_remove();

	hrx_sig_stat_remove();

	mutex_destroy(&hrx_dev->vip_mutex);
	RETURN;
}

#ifdef CONFIG_PM_SLEEP
static int syna_hrx_v4l2_suspend(struct device *dev)
{
	return 0;
}

static int syna_hrx_v4l2_resume(struct device *dev)
{
	return 0;
}
static SIMPLE_DEV_PM_OPS(syna_hrx_v4l2_pmops, syna_hrx_v4l2_suspend,
						syna_hrx_v4l2_resume);
 #endif

static const struct of_device_id hrx_match[] = {
	{
		.compatible = "syna,berlin-hrx",
	},
	{},
};
MODULE_DEVICE_TABLE(of, hrx_match);

struct platform_driver syna_hrx_v4l2_drv = {
	.probe = &syna_hrx_v4l2_probe,
	.remove = &syna_hrx_v4l2_remove,
	.driver = {
		.name   = "syna-hrx-v4l2",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(hrx_match),
 #ifdef CONFIG_PM_SLEEP
		.pm = &syna_hrx_v4l2_pmops,
 #endif
	},
};

module_platform_driver(syna_hrx_v4l2_drv);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Synaptics HRX V4L2 kernel module");
