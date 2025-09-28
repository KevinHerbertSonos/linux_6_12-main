// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "ispSS_fe_client_prv.h"
#include "ispbe_err.h"

/*********************************************************************
 * FUNCTION: Get Write client base address
 * PARAMS:   ChannelID    -  Write Client channel of which needs to be configured
 *           uiRegAddr    -  Pointer to Store client Address
 * RETURN:   ISPSS_OK, ISPSS_EBADPARAM
 ********************************************************************/
static INT ISPSS_FE_WR_CLIENT_GetClientAddr(INT ChannelID, UINT32 *uiRegAddr)
{
	if (ChannelID < ISP_WR_CHANNEL_MAX) {
		*uiRegAddr = wrClientAddress[ChannelID];
		return ISPSS_OK;
	} else {
		return ISPSS_EBADPARAM;
	}
}

/*********************************************************************
 * FUNCTION: Get width in bytes
 * PARAMS:   uiSrcFmt     - Input source format
 *           uiWidth      - Width of the input frame
 *           bpp          - Bit per pixel of source
 * RETURN:   Width in bytes
 *********************************************************************/
static INT ISPSS_FE_CLIENT_GetWidthInBytes(UINT32 uiSrcFmt, UINT32 uiWidth, INT bpp)
{
	UINT32 widthInBytes;

	switch (uiSrcFmt) {
	case ISPSS_SRCFMT_YUV420SP_DWA:
		widthInBytes = (uiWidth * bpp * 32) / (30 * 8);
		break;
	case ISPSS_SRCFMT_YUV422P:
		widthInBytes = (uiWidth * bpp * 2) / 8;
		break;
	case ISPSS_SRCFMT_YUV422SP_DWA:
		widthInBytes = (uiWidth * bpp * 32) / (30 * 8);
		break;
	case ISPSS_SRCFMT_RGB444:
	case ISPSS_SRCFMT_YUV444P:
	case ISPSS_SRCFMT_RGB888:
		widthInBytes = (uiWidth * bpp * 3) / 8;
		break;
	case ISPSS_SRCFMT_YUV420SP_TILED_V8H8:
	case ISPSS_SRCFMT_YUV420SP_TILED_V8H6:
	case ISPSS_SRCFMT_YUV420SP_TILED_V6H8:
	default:
		widthInBytes = (uiWidth * bpp) / 8;
		break;
	}

	return widthInBytes;
}

/*********************************************************************
 * FUNCTION:  Get Write Client Pack sel
 * PARAMS:    uiSrcFmt          - Input source format
 *            bpp               - Bit per pixel of source
 *            stWriteClientPack - Write Client Register structure Pointer
 * RETURN:    ISPSS_OK, ISPSS_EBADPARAM
 ********************************************************************/
static INT ISPSS_FE_WR_CLIENT_GetPackSel(UINT32 uiSrcFmt, INT bpp,
		T32WriteClient_pack *stWriteClientPack, INT ChannelID)
{
	INT channelMode, packSel;

	if (ChannelID % 2 == 0)
		channelMode = 0;
	else
		channelMode = 1;

	if (bpp == 8)
		packSel = wrClientPackSel[uiSrcFmt][channelMode][IN_BIT_DEPTH_8BIT];
	else
		packSel = wrClientPackSel[uiSrcFmt][channelMode][IN_BIT_DEPTH_10BIT];

	if (packSel != INVALID_PACK_SEL) {
		stWriteClientPack->upack_Sel = packSel;
		return ISPSS_OK;
	} else {
		return ISPSS_EBADPARAM;
	}
}

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
		UINT32 uiSrcFmt, UINT32 uiWidth, UINT32 uiHeight, INT bpp)
{
	UINT32 uiRegAddr = 0, widthInBytes;
	INT Ret = ISPSS_OK;
	T32WriteClient_pix stWriteClientPix;
	T32WriteClient_NonStdRes stWriteClientNonStdRes;
	T32WriteClient_pack stWriteClientPack;

	stWriteClientPix.u32          = 0;
	stWriteClientNonStdRes.u32    = 0;
	stWriteClientPack.u32         = 0;

	if ((uiSrcFmt == ISPSS_SRCFMT_YUV420SP) && (ChannelID == ISP_OVPDNSCLWRAP_WR1))
		stWriteClientPix.upix_tot = (uiWidth * uiHeight)/2;
	else if ((uiSrcFmt == ISPSS_SRCFMT_YUV422SP) && (ChannelID == ISP_OVPDNSCLWRAP_WR1))
		stWriteClientPix.upix_tot = (uiWidth * uiHeight)/2;
	else
		stWriteClientPix.upix_tot = uiWidth * uiHeight;

	Ret = ISPSS_FE_WR_CLIENT_GetClientAddr(ChannelID, &uiRegAddr);
	if (Ret != ISPSS_OK) {
		pr_err("%s, Client ADDR InCorrect\n", __func__);
		return Ret;
	}

	widthInBytes = ISPSS_FE_CLIENT_GetWidthInBytes(uiSrcFmt, uiWidth, bpp);

	if ((widthInBytes % 16) != 0) {
		stWriteClientNonStdRes.uNonStdRes_enable = 1;
		stWriteClientNonStdRes.uNonStdRes_pixlineTot = uiWidth;
	} else {
		stWriteClientNonStdRes.uNonStdRes_enable = 0;
		stWriteClientNonStdRes.uNonStdRes_pixlineTot = 0;
	}

	ISPSS_FE_WR_CLIENT_GetPackSel(uiSrcFmt, bpp, &stWriteClientPack, ChannelID);
	if ((ChannelID == ISP_TILE_WR0) &&
		(uiSrcFmt == ISPSS_SRCFMT_YUV422P) && (bpp == 10))
		stWriteClientPack.u32 = 4;

	ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_WriteClient_pix,
		stWriteClientPix.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_WriteClient_NonStdRes,
		stWriteClientNonStdRes.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_WriteClient_pack,
		stWriteClientPack.u32);

	return Ret;
}
EXPORT_SYMBOL(ISPSS_FE_WR_CLIENT_ClientConfig);

/*********************************************************************
 * FUNCTION: Get Read client base address
 * PARAMS:   ChannelID    -  channel of which needs to be configured
 *           uiRegAddr    -  Pointer to Store client Address
 * RETURN:   ISPSS_OK, ISPSS_EBADPARAM
 ********************************************************************/
static INT ISPSS_FE_RD_CLIENT_GetClientAddr(INT ChannelID, UINT32 *uiRegAddr)
{
	if (ChannelID < ISP_RD_CHANNEL_MAX) {
		*uiRegAddr = rdClientAddress[ChannelID];
		return ISPSS_OK;
	} else {
		return ISPSS_EBADPARAM;
	}
}

/*********************************************************************
 * FUNCTION:  Get Pack sel for Read client
 * PARAMS:    uiSrcFmt          - Input source format
 *            bpp               - Bit per pixel of source
 *            ChannelID    -  channel of which needs to be configured
 * RETURN:    PackSel value, ISPSS_EBADPARAM
 ********************************************************************/
static INT ISPSS_FE_RD_CLIENT_GetPackSel(UINT32 uiSrcFmt, INT bpp, INT channelID)
{
	UINT32 channelMode;
	INT packSel;

	if (channelID % 2 == 0)
		channelMode = 0;
	else
		channelMode = 1;

	if (bpp == 8)
		packSel = rdClientPackSel[uiSrcFmt][channelMode][IN_BIT_DEPTH_8BIT];
	else
		packSel = rdClientPackSel[uiSrcFmt][channelMode][IN_BIT_DEPTH_10BIT];

	if (packSel != INVALID_PACK_SEL)
		return packSel;
	else
		return ISPSS_EBADPARAM;
}

/*********************************************************************
 * FUNCTION: Enable/Start Read  channel
 * PARAMS:   pbcmbuf      - BCM buffer pointer
 *           ChannelID    - channel of which needs to be configured
 * RETURN:   ISPSS_OK
 ********************************************************************/
INT ISPSS_FE_RD_CLIENT_EnableChannelSCLInstance(struct BCMBUF *pbcmbuf, INT ChannelID)
{
	UINT32 channelMode;
	UINT32 uiRegAddr = 0;
	INT Ret = ISPSS_OK;
	T32OVPDNSCLWRAP_CTRL0 stReadClientRd;

	Ret = ISPSS_FE_RD_CLIENT_GetClientAddr(ChannelID, &uiRegAddr);
	if (Ret != ISPSS_OK) {
		pr_info("%s, %d, Client Addr Incorrect\n", __func__, __LINE__);
		return Ret;
	}

	if (ChannelID % 2 == 0)
		channelMode = 0;
	else
		channelMode = 1;

	stReadClientRd.u32 = 0;

	if (!channelMode) {
		stReadClientRd.uCTRL0_ClientR0_start = 1;
		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL0, stReadClientRd.u32);
		stReadClientRd.uCTRL0_ClientR0_start = 0;
		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL0, stReadClientRd.u32);
	} else {
		stReadClientRd.uCTRL0_ClientR1_start = 1;
		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL0, stReadClientRd.u32);
		stReadClientRd.uCTRL0_ClientR1_start = 0;
		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL0, stReadClientRd.u32);
	}

	return Ret;
}
EXPORT_SYMBOL(ISPSS_FE_RD_CLIENT_EnableChannelSCLInstance);

/*********************************************************************
 * FUNCTION: Clear Read channel
 * PARAMS:   pbcmbuf      - BCM buffer pointer
 *           ChannelID    - channel of which needs to be configured
 * RETURN:   ISPSS_OK
 *********************************************************************/
INT ISPSS_FE_RD_CLIENT_ClearChannelSCLInstance(struct BCMBUF *pbcmbuf, INT ChannelID)
{
	UINT32 uiRegAddr = 0;
	INT Ret = ISPSS_OK;
	T32OVPDNSCLWRAP_CTRL0 stReadClientRd;
	UINT32 channelMode;

	if (ChannelID % 2 == 0)
		channelMode = 0;
	else
		channelMode = 1;

	stReadClientRd.u32 = 0;

	Ret = ISPSS_FE_RD_CLIENT_GetClientAddr(ChannelID, &uiRegAddr);
	if (Ret != ISPSS_OK) {
		pr_debug("%s, %d, Client Addr Incorrect\n", __func__, __LINE__);
		return Ret;
	}

	if (!channelMode) {
		stReadClientRd.uCTRL0_ClientR0_clear = 1;
		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL0, stReadClientRd.u32);
		stReadClientRd.uCTRL0_ClientR0_clear = 0;
		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL0, stReadClientRd.u32);
	} else {
		stReadClientRd.uCTRL0_ClientR1_clear = 1;
		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL0, stReadClientRd.u32);
		stReadClientRd.uCTRL0_ClientR1_clear = 0;
		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL0, stReadClientRd.u32);
	}

	return Ret;
}
EXPORT_SYMBOL(ISPSS_FE_RD_CLIENT_ClearChannelSCLInstance);

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
		UINT32 uiSrcFmt, UINT32 uiWidth, UINT32 uiHeight, INT bpp)
{
	UINT32 uiRegAddr = 0, widthInBytes, channelMode;
	INT packSel, Ret = ISPSS_OK;

	Ret = ISPSS_FE_RD_CLIENT_GetClientAddr(ChannelID, &uiRegAddr);
	if (Ret != ISPSS_OK) {
		pr_debug("%s, %d, Client Addr Incorrect\n", __func__, __LINE__);
		return Ret;
	}

	if (ChannelID % 2 == 0)
		channelMode = 0;
	else
		channelMode = 1;

	if (!channelMode) {
		T32OVPDNSCLWRAP_CTRL1 stReadClientCtrl1;
		T32OVPDNSCLWRAP_CTRL3 stReadClientCtrl3;

		stReadClientCtrl1.u32 = 0;
		stReadClientCtrl3.u32 = 0;

		stReadClientCtrl1.uCTRL1_pixlineTot_R0 = uiWidth;
		stReadClientCtrl1.uCTRL1_nonStdResEn_R0 = 0;

		if ((uiSrcFmt == ISPSS_SRCFMT_YUV444P) || (uiSrcFmt == ISPSS_SRCFMT_RGB444) ||
				(uiSrcFmt == ISPSS_SRCFMT_RGB888))
			stReadClientCtrl3.uCTRL3_wordTot_R0 =
				(((uiWidth * 24) + 127) / 128) * uiHeight;
		else if ((uiSrcFmt == ISPSS_SRCFMT_YUV422P) && (bpp == 8))
			stReadClientCtrl3.uCTRL3_wordTot_R0 =
				(((uiWidth * 16) + 127) / 128) * uiHeight;
		else if ((uiSrcFmt == ISPSS_SRCFMT_YUV422P) && (bpp == 10))
			stReadClientCtrl3.uCTRL3_wordTot_R0 =
				(((uiWidth * 20) + 127) / 128) * uiHeight;
		else
			stReadClientCtrl3.uCTRL3_wordTot_R0 =
				(((uiWidth * bpp) + 127) / 128) * uiHeight;

		widthInBytes = ISPSS_FE_CLIENT_GetWidthInBytes(uiSrcFmt, uiWidth, bpp);
		if ((widthInBytes % 16) != 0) {
			pr_debug("%s %d ThroughNon-std Res\n", __func__, __LINE__);
			stReadClientCtrl1.uCTRL1_nonStdResEn_R0 = 1;
			stReadClientCtrl1.uCTRL1_pixlineTot_R0  = uiWidth;
		}

		stReadClientCtrl1.uCTRL1_flushCnt_R0 = 5;
		stReadClientCtrl1.uCTRL1_ovp_flush_sel = 1;
		stReadClientCtrl1.uCTRL1_ovp_intr_sel = 1;
		packSel = ISPSS_FE_RD_CLIENT_GetPackSel(uiSrcFmt, bpp, channelMode);
		stReadClientCtrl1.uCTRL1_packSel_R0 = packSel;

		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL1,
			stReadClientCtrl1.u32);
		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL3,
			stReadClientCtrl3.u32);
	} else {
		T32OVPDNSCLWRAP_CTRL2 stReadClientCtrl2;
		T32OVPDNSCLWRAP_CTRL4 stReadClientCtrl4;

		stReadClientCtrl2.u32 = 0;
		stReadClientCtrl4.u32 = 0;

		stReadClientCtrl2.uCTRL2_pixlineTot_R1 = uiWidth;
		stReadClientCtrl2.uCTRL2_nonStdResEn_R1 =  0;
		widthInBytes = ISPSS_FE_CLIENT_GetWidthInBytes(uiSrcFmt, uiWidth, bpp);

		if ((widthInBytes % 16) != 0) {
			pr_debug("%s %d ThroughNon-std Res\n", __func__, __LINE__);
			stReadClientCtrl2.uCTRL2_nonStdResEn_R1 =  1;
			stReadClientCtrl2.uCTRL2_pixlineTot_R1  =  uiWidth;
		}

		stReadClientCtrl2.uCTRL2_flushCnt_R1 = 5;
		stReadClientCtrl4.uCTRL4_wordTot_R1 = (((uiWidth * bpp) + 127) / 128) * uiHeight;

		packSel = ISPSS_FE_RD_CLIENT_GetPackSel(uiSrcFmt, bpp, channelMode);
		stReadClientCtrl2.uCTRL2_packSel_R1 = packSel;

		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL2,
			stReadClientCtrl2.u32);
		ISPSS_BCMBUF_Write(pbcmbuf, uiRegAddr + RA_OVPDNSCLWRAP_CTRL4,
			stReadClientCtrl4.u32);
	}

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_FE_RD_CLIENT_ClientConfigSCLInstance);
