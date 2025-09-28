// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated
 */

#ifndef _HAL_VPP_MIPI_H_
#define _HAL_VPP_MIPI_H_

#define MIPI_PANEL_NAME_SIZE_MAX  16
#define MAX_CMD_BUF_SIZE          (4 * 1024)
#define MAX_MIPI_STANDBY_CMD_SIZE (512)
#define MAX_MIPI_INIT_CMD_SIZE    (MAX_CMD_BUF_SIZE - MAX_MIPI_STANDBY_CMD_SIZE)
#define MIPI_CMD_HEADER_SIZE      sizeof(VPP_MIPI_CMD_HEADER)
#define MIPI_INIT_CMD_START       0
#define MIPI_STANDBY_CMD_START    (MIPI_INIT_CMD_START + MAX_MIPI_INIT_CMD_SIZE)

typedef enum VPP_MIPI_CMD_TYPE_T {
	VPP_CMD_TYPE_INIT,
	VPP_CMD_TYPE_STANDBY
} VPP_MIPI_CMD_TYPE;

typedef struct VPP_MIPI_CMD_HEADER_T {
	UINT8 cmd_type;
	UINT8 cmd_rsvd;
	UINT16 cmd_size;
}VPP_MIPI_CMD_HEADER;

typedef struct RESOLUTION_INFO_T {
	int active_width;    /* active width of channel in pixel */
	int active_height;   /* active height of channel in pixel */
	int width;  /* width of channel in pixel */
	int height; /* height of channel in pixel */
	int hfrontporch; /* front porch of hsync */
	int hsyncwidth; /* hsync width */
	int hbackporch; /* back porch of hsync */
	int vfrontporch; /* front porch of vsync */
	int vsyncwidth; /* vsync width */
	int vbackporch; /* back porch of vsync */
	int type;   /* resolution type: HD or SD */
	int scan;   /* scan mode: progressive or interlaced */
	int frame_rate;   /* frame rate */
	int flag_3d;   /* 1 for 3D, 0 for 2D */
	int freq;   /* pixel frequency */
	int pts_per_cnt_4;   /* time interval in term of PTS for every 4 frames */
} RESOLUTION_INFO;

typedef struct CPCB_TG_TIMING_PARAMS_T {
	UINT8 tg_vb_min;       /* calibration params for the vertical blank */
	UINT8 tg_hb_min;       /* calibration params for the horizontal blank */
	UINT8 tg_size_v_off_p; /* calibration params  */
	UINT8 tg_size_h_off_p; /* calibration params horizontal  */
	UINT8 tg_hb_vop_off;   /* calibration params  */
	UINT8 tg_vb0_vop_off;  /* calibration params  */
	UINT8 tg_hb_be;        /* calibration params  */
	UINT8 tg_vb0_be;       /* calibration params  */
	UINT8 tg_vb_fp;        /* calibration params for the vertical Front porch */
	UINT8 tg_hb_fp;        /* calibration params for the horizontal Front porch */
	UINT32 pixel_clock;  /*Pixel clock [KHZ]*/
}CPCB_TG_TIMING_PARAMS;

typedef struct RESINFO_TIMING_DESCRIPTION_T {
	RESOLUTION_INFO resInfo;
	CPCB_TG_TIMING_PARAMS tgParams;
}RESINFO_TIMING_DESCRIPTION;

typedef struct VPP_MIPI_INIT_PARAMS_T {
	/* Resid of the resolution */
	UINT16 resId;
	/* Name of the panel </NAME/_resolution/ => ex: HAIER_800x1280> */
	char panelname[MIPI_PANEL_NAME_SIZE_MAX];
	/** Number of lanes used to send current video */
	UINT8 no_of_lanes;
	/** Data Lane polarity - whether to use P/N or N/P for each lane -
	Bit 0 corresponds Lane 0, bit 1 - lane 1 ... bit n - lane n
	Bit is set - N/P be used
	Bit is clear - P/N be used*/
	UINT8 data_lane_polarity;
	/* Clk lane polarity P/N - 0, N/P - 1 */
	UINT8 clk_lane_polarity;
	/** Virtual channel number to send this video stream */
	UINT8 virtual_channel;
	/* Video Mode non burst Mode with sync or Pulse OR burst mode */
	UINT8 video_mode;
	/** Enable non coninuous clock for energy saving
	* - Clock lane will go to LS while not transmitting video */
	UINT8 non_continuous_clock;
	/** Enable receiving of ack packets */
	UINT8 receive_ack_packets;
	/** Byte (lane) clock [KHz] */
	UINT32 byte_clock;
	/** Colour coding - BPP configuration */
	UINT8 color_coding;
	/** Is 18-bit loosely packets (valid only when BPP == 18) */
	UINT8 is_18_loosely;
	/** Data enable signal (dpidaten) whether it is active high or low */
	UINT8 data_en_polarity;
	/** Horizontal synchronisation signal (dpihsync) whether it is active high or low */
	UINT8 h_polarity;
	/** Vertical synchronisation signal (dpivsync) whether it is active high or low */
	UINT8 v_polarity;
	/** When set to 1, this bit enables the EoRp transmission */
	UINT8 eotp_rx_en;
	/** When set to 1, this bit enables the EoTp transmission */
	UINT8 eotp_tx_en;
	/** This register configures the number of chunks to use */
	UINT32 no_of_chunks;
	/** This register configures the size of null packets */
	UINT16 null_packet_size;
	/* Enable low power */
	UINT8 dpi_lp_cmd_en;
}VPP_MIPI_INIT_PARAMS;

typedef struct VPP_MIPI_CMD_PARAMS_T {
	UINT16 bufsize;
	ARCH_PTR_TYPE pcmd;
}VPP_MIPI_CMD_PARAMS;

typedef struct VPP_MIPI_CONFIG_PARAM_T
{
	VPP_MIPI_INIT_PARAMS         initparams;
	RESINFO_TIMING_DESCRIPTION   infoparams;
	VPP_MIPI_CMD_PARAMS          vppMipiCmd;
}VPP_MIPI_CONFIG_PARAMS;

typedef struct VPP_MIPI_LOAD_CONFIG_T
{
	UINT32           noOfresID;     /* ENUM_CPCB_TG_RES */
	ARCH_PTR_TYPE    vppMipiCfgPA;  /* Address of the panel configuration */
}VPP_MIPI_LOAD_CONFIG;
#endif
