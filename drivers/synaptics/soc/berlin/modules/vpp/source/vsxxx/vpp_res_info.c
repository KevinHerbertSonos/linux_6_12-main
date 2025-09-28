// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Synaptics Incorporated
 *
 */

#include "vpp_res_info.h"

static const RESOLUTION_INFO m_resinfo_table[MAX_NUM_RESS] = {
	/* { active_width, active_height, width, height, hfrontporch, hsyncwidth, hbackporch, vfrontporch, vsyncwidth, vbackporch, type, scan, frame_rate, flag_3d, freq, pts_per_cnt_4 } */
	/*  0: RES_NTSC_M              */ {     720,          480,         858,    525,    19,          62,         57,         4,           3,          15,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_59P94,    0,  27000,  6006 },
	/*  1: RES_NTSC_J              */ {     720,          480,         858,    525,    19,          62,         57,         4,           3,          15,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_59P94,    0,  27000,  6006 },
	/*  2: RES_PAL_M               */ {     720,          480,         858,    525,    19,          62,         57,         4,           3,          15,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_59P94,    0,  27000,  6006 },
	/*  3: RES_PAL_BGH             */ {     720,          576,         864,    625,    12,          63,         69,         2,           3,          19,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_50,       0,  27000,  7200 },
	/*  4: RES_525I60              */ {     720,          480,         858,    525,    19,          62,         57,         4,           3,          15,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_60,       0,  27027,  6000 },
	/*  5: RES_525I5994            */ {     720,          480,         858,    525,    19,          62,         57,         4,           3,          15,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_59P94,    0,  27000,  6006 },
	/*  6: RES_625I50              */ {     720,          576,         864,    625,    12,          63,         69,         2,           3,          19,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_50,       0,  27000,  7200 },
	/*  7: RES_525P60              */ {     720,          480,         858,    525,    16,          62,         60,         9,           6,          30,   TYPE_SD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0,  27027,  6000 },
	/*  8: RES_525P5994            */ {     720,          480,         858,    525,    16,          62,         60,         9,           6,          30,   TYPE_SD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0,  27000,  6006 },
	/*  9: RES_625P50              */ {     720,          576,         864,    625,    12,          64,         68,         5,           5,          39,   TYPE_SD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      0,  27000,  7200 },
	/* 10: RES_720P30              */ {    1280,          720,         3300,   750,    1760,        40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_30,      0,  74250, 12000 },
	/* 11: RES_720P2997            */ {    1280,          720,         3300,   750,    1760,        40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_29P97,   0,  74176, 12012 },
	/* 12: RES_720P25              */ {    1280,          720,         3960,   750,    2420,        40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_25,      0,  74250, 14400 },
	/* 13: RES_720P60              */ {    1280,          720,         1650,   750,    110,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0,  74250,  6000 },
	/* 14: RES_720P5994            */ {    1280,          720,         1650,   750,    110,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0,  74176,  6006 },
	/* 15: RES_720P50              */ {    1280,          720,         1980,   750,    440,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      0,  74250,  7200 },
	/* 16: RES_1080I60             */ {    1920,         1080,         2200,   1125,   88,          44,         148,        2,           5,          15,   TYPE_HD,   SCAN_INTERLACED,  FRAME_RATE_60,      0, 148500,  6000 },
	/* 17: RES_1080I5994           */ {    1920,         1080,         2200,   1125,   88,          44,         148,        2,           5,          15,   TYPE_HD,   SCAN_INTERLACED,  FRAME_RATE_59P94,   0, 148352,  6006 },
	/* 18: RES_1080I50             */ {    1920,         1080,         2640,   1125,   528,         44,         148,        2,           5,          15,   TYPE_HD,   SCAN_INTERLACED,  FRAME_RATE_50,      0, 148500,  7200 },
	/* 19: RES_1080P30             */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 74250,  12000 },
	/* 20: RES_1080P2997           */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_29P97,   0, 74176,  12012 },
	/* 21: RES_1080P25             */ {    1920,         1080,         2640,   1125,   528,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_25,      0, 74250,  14400 },
	/* 22: RES_1080P24             */ {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_24,      0, 74250,  15000 },
	/* 23: RES_1080P2398           */ {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_23P98,   0, 74176,  15015 },
	/* 24: RES_1080P60             */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 148500,  6000 },
	/* 25: RES_1080P5994           */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0, 148352,  6006 },
	/* 26: RES_1080P50             */ {    1920,         1080,         2640,   1125,   528,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      0, 148500,  7200 },
	/* 27: RES_LVDS_1080P48        */ {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      0, 148500,  7500 },
	/* 28: RES_LVDS_1080P50        */ {    1920,         1080,         2200,   1350,   32,          44,         204,        229,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      0, 148500,  7200 },
	/* 29: RES_LVDS_1080P60        */ {    1920,         1080,         2200,   1125,   32,          44,         204,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 148500,  6000 },
	/* 30: RES_LVDS_2160P12        */ {    1920,         1080,         2200,   1406,   32,          44,         204,        235,         5,          86,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      0, 148470,  7500 },
	/* 31: RES_VGA_480P60          */ {    640,           480,         800,    525,    16,          96,         48,         10,          2,          33,   TYPE_SD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 25200,   6000 },
	/* 32: RES_VGA_480P5994        */ {    640,           480,         800,    525,    16,          96,         48,         10,          2,          33,   TYPE_SD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0, 25175,   6006 },
	/* 33: RES_720P50_3D           */ {    1280,          720,         1980,   750,    440,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_100,     1, 148500,  7200 },
	/* 34: RES_720P60_3D           */ {    1280,          720,         1650,   750,    110,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },
	/* 35: RES_720P5994_3D         */ {    1280,          720,         1650,   750,    110,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_119P88,  1, 148352,  6006 },
	/* 36: RES_1080P24_3D          */ {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      1, 148500, 15000 },
	/* 37: RES_1080P2398_3D        */ {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_47P96,   1, 148352, 15015 },
	/* 38: RES_1080P30_3D          */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      1, 148500, 12000 },
	/* 39: RES_1080P2997_3D        */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   1, 148352, 12012 },
	/* 40: RES_1080P25_3D          */ {    1920,         1080,         2640,   1125,   528,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      1, 148500, 14400 },
	/* 41: RES_1080I60_FP          */ {    1920,         2228,         2200,   2250,    88,         44,         148,        2,           5,          15,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 148500,  12000},
	/* 42: RES_1080I5994_FP        */ {    1920,         2228,         2200,   2250,    88,         44,         148,        2,           5,          15,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 148352,  12000},
	/* 43: RES_1080I50_FP          */ {    1920,         2228,         2640,   2250,   528,         44,         148,        2,           5,          15,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_25,      0, 148500,  14400},
	/* 44: RES_LVDS_1920X540P60_3D */ {    1920,          540,         1980,    625,     8,         44,          8,        16,           5,          64,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },
	/* 45: RES_LVDS_1920X540P30_3D */ {    1920,          540,         1980,    625,     8,         44,          8,        16,           5,          64,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      1, 74250,  12000 },
	/* 46: RES_LVDS_1920X540P24_3D */ {    1920,          540,         2475,    625,   407,         44,        104,        16,           5,          64,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      1, 74250,  15000 },
	/* 47: RES_LVDS_720P100_3D  */    {    1280,          720,         1980,    750,   440,         40,         220,         5,          5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_100,     1, 148500,  7200 },
	/* 48: RES_LVDS_720P120_3D  */    {    1280,          720,         1650,    750,   110,         40,         220,         5,          5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },
	/* 49: RES_LVDS_1080P48_3D */     {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      1, 148500, 15000 },
	/* 50: RES_LVDS_1080P50_3D */     {    1920,         1080,         2200,    1350,   32,         44,         204,        229,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      1, 148500, 14400 },
	/* 51: RES_LVDS_1080P60_3D */     {    1920,         1080,         2200,   1125,   32,          44,         204,          4,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      1, 148500, 12000 },
	/* 52: RES_LVDS_1920x540P100_3D */{    1920,          540,          2200,   675,    32,         44,         204,         94,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_100,     1, 148500,  7200 },
	/* 53: RES_LVDS_1920X540P120_3D */{    1920,          540,          2202,   562,    32,         44,         206,          7,         5,          8,    TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },
	/* 54: RES_LVDS_960x1080P100_3D */{     960,         1080,         1100,   1350,   42,          44,         54,         229,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_100,     1, 148500,  7200 },
	/* 55: RES_LVDS_960x1080P120_3D */{     960,         1080,         1100,   1125,   42,          44,         54,           4,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },

	/* 56: RES_4Kx2K2398    */        {    3840,         2160,         5500,   2250,   1276,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_23P98,   0, 296703, 15015 },
	/* 57: RES_4Kx2K24    */          {    3840,         2160,         5500,   2250,   1276,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_24,      0, 297000, 15000 },
	/* 58: RES_4Kx2K24_SMPTE */       {    4096,         2160,         5500,   2250,   1020,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_24,      0, 297000, 15000 },
	/* 59: RES_4Kx2K25    */          {    3840,         2160,         5280,   2250,   1056,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_25,      0, 297000, 14400 },
	/* 60: RES_4Kx2K2997    */        {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_29P97,   0, 296703, 12012 },
	/* 61: RES_4Kx2K30    */          {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 297000, 12000 },
	/* 62: RES_4Kx2K50    */          {    3840,         2160,         5280,   2250,   1056,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_50,      0, 594000,  7200 },
	/* 63: RES_4Kx2K5994    */        {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0, 593406,  6006 },
	/* 64: RES_4Kx2K60    */          {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 594000,  6000 },
	/* 65: RES_4Kx2K30_HDMI*/         {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 297000, 12000 },
	/* 66: RES_4Kx1K120    */         {    3840,         1080,         4400,   1125,   176,         88,         296,          4,          5,         36,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_120,     0, 594000,  3000 },
	/* RES_RESET   */ {       0,            0,            1,      1,     0,          0,           0,        0,           0,           0,         0,                0, 0,    0 }
};

static int syna_get_fps_map (int fps)
{
	switch (fps) {
		case 24: return FRAME_RATE_24;
		case 25: return FRAME_RATE_25;
		case 30: return FRAME_RATE_30;
		case 50: return FRAME_RATE_50;
		case 60: return FRAME_RATE_60;
		default: return  -1;
	}
}

int syna_get_res_index(int active_width, int active_height, int scan, int freq, int fps)
{
	int i;
	int fps_index;

	fps_index = syna_get_fps_map(fps);
	if (fps_index < 0) {
		return -1;
	}

	for (i = 0; i < RES_MAX_4Kx2K; i++) {
		if ((active_width == m_resinfo_table[i].active_width) &&
				(active_height == m_resinfo_table[i].active_height) &&
				(scan == m_resinfo_table[i].scan) &&
				(freq == m_resinfo_table[i].freq) &&
				(fps_index == m_resinfo_table[i].frame_rate)) {
			break;
		}
	}

	if (i == RES_MAX_4Kx2K) {
		return -1;
	}
	return i;
}

RESOLUTION_INFO* syna_get_res_info(int res_index)
{

	RESOLUTION_INFO *res_info = (RESOLUTION_INFO*) NULL;

	if (res_index < RES_MAX_4Kx2K)
		res_info = (RESOLUTION_INFO*) &m_resinfo_table[res_index];

	return res_info;
}
