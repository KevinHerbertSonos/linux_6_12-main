/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPISS_FE_CLIENT_H_
#define _ISPISS_FE_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ispSS_memmap.h"
#include "ispSS_bcmbuf.h"

enum ISP_CLR_SPACE {
	Y_CHANNEL = 0,
	UV_CHANNEL,
	ISP_CHANNEL_MAX
};

enum ISP_SRC_FMT {
	ISP_SRCFMT_YUV422SP = 0,
	ISP_SRCFMT_YUV420SP,
	ISP_SRCFMT_YUV422SP_DWA,
	ISP_SRCFMT_YUV420SP_DWA,
	ISP_SRCFMT_YUV422PACK,
	ISP_SRCFMT_YUV444PACK,
	ISP_SRCFMT_YUV420SP_V8H8,
	ISP_SRCFMT_YUV420SP_V8H6,
	ISP_SRCFMT_YUV422SP_V8H8,
	ISP_SRCFMT_YUV422SP_V8H6,
	ISP_SRCFMT_RGB888PACK,
	ISP_SRCFMT_MAX
};

enum ISP_WR_CHANNEL_ID {
	ISP_OVPDNSCLWRAP_WR0 = 0,
	ISP_OVPDNSCLWRAP_WR1,
	ISP_TILE_WR0,
	ISP_TILE_WR1,
	ISP_DEWARP_WR0,
	ISP_DEWARP_WR1,
	ISP_ROT_WR0,
	ISP_ROT_WR1,
	ISP_WR_CHANNEL_MAX
};

enum ISP_RD_CHANNEL_ID {
	ISP_OVPDNSCLWRAP_RD0 = 0,
	ISP_OVPDNSCLWRAP_RD1,
	ISP_TILE_RD0,
	ISP_TILE_RD1,
	ISP_DEWARP_RD0,
	ISP_DEWARP_RD1,
	ISP_DTILE_RD0,
	ISP_DTILE_RD1,
	ISP_RD_CHANNEL_MAX
};

enum ISP_BIT_DEPTH {
	IN_BIT_DEPTH_8BIT = 0,
	IN_BIT_DEPTH_10BIT,
	BIT_DEPTH_MAX
};

/*************************************************/
/********************** FE_CLIENT APIs ******************/
/*************************************************/

/*******************************************************************
 * FUNCTION: Enable/Start write Client
 * PARAMS:   ChannelID - Write channel of which needs to be configured
 *           pbcmbuf   - BCM buffer pointer
 * RETURN:   ISPSS_OK
 ******************************************************************/
INT ISPSS_FE_WR_CLIENT_EnableChannel(struct BCMBUF *pbcmbuf, INT ChannelID);

/*********************************************************************
 * FUNCTION: Clear the channel
 * PARAMS:   ChannelID - Write channel of which needs to be configured
 *           pbcmbuf   - BCM buffer pointer
 * RETURN:   ISPSS_OK
 ********************************************************************/
INT ISPSS_FE_WR_CLIENT_ClearChannel(struct BCMBUF *pbcmbuf, INT ChannelID);

/*********************************************************************
 * FUNCTION: Configure write client blocks
 * PARAMS:   pbcmbuf      - BCM buffer pointer
 *           ChannelID    - channel of which needs to be configured
 *           uiSrcFmt     - Input source format
 *           uiWidth      - Width of the input frame
 *           uiHeight     - Height of the input frame
 *           bpp          - Bit per pixel of the frame
 * RETURN:   ISPSS_OK
 ********************************************************************/
INT ISPSS_FE_WR_CLIENT_ClientConfig(struct BCMBUF *pbcmbuf, INT ChannelID,
		UINT32 uiSrcFmt, UINT32 uiWidth, UINT32 uiHeight, INT bpp);

/*********************************************************************
 * FUNCTION: Enable/Start Read  channel
 * PARAMS:   pbcmbuf      - BCM buffer pointer
 *           ChannelID    - channel of which needs to be configured
 * RETURN:   ISPSS_OK
 ********************************************************************/
INT ISPSS_FE_RD_CLIENT_EnableChannelSCLInstance(struct BCMBUF *pbcmbuf, INT ChannelID);

/*********************************************************************
 * FUNCTION: Clear Read channel
 * PARAMS:   pbcmbuf      - BCM buffer pointer
 *           ChannelID    - channel of which needs to be configured
 * RETURN:   ISPSS_OK
 *********************************************************************/
INT ISPSS_FE_RD_CLIENT_ClearChannelSCLInstance(struct BCMBUF *pbcmbuf, INT ChannelID);

/*********************************************************************
 * FUNCTION: Configure Read client blocks
 * PARAMS:   pbcmbuf      - BCM buffer pointer
 *           ChannelID    - Read channel of which needs to be configured
 *           uiSrcFmt     - Input source format
 *           uiWidth      - Width of the input frame
 *           uiHeight     - Height of the input frame
 *           bpp          - Bit per pixel of the frame
 * RETURN:   ISPSS_OK
 ********************************************************************/
INT ISPSS_FE_RD_CLIENT_ClientConfigSCLInstance(struct BCMBUF *pbcmbuf, INT ChannelID,
		UINT32 uiSrcFmt, UINT32 uiWidth, UINT32 uiHeight, INT bpp);

#ifdef __cplusplus
}
#endif

#endif
