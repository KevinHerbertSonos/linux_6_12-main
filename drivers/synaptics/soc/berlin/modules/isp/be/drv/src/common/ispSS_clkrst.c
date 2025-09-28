// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>

#include "ispSS_clkrst.h"

static void ISPSS_CLKRST_getClkRegisterDetails(enum ISPSS_CLKRST_CLK clk,
		UINT32 *pRegAddr, UINT32 *pBitPos)
{
	UINT32 clkStartOffset, regOffset;

	switch (clk) {
	case ISPSS_CLKRST_CLK_ALMSYS:
	case ISPSS_CLKRST_CLK_ALMCFG:
		clkStartOffset = ISPSS_CLKRST_CLK_ALM_START;
		regOffset = RA_CLKRST_AlmCkCtl;
		break;
	case ISPSS_CLKRST_CLK_MTRSYS:
	case ISPSS_CLKRST_CLK_MTRCFG:
		clkStartOffset = ISPSS_CLKRST_CLK_MTR_START;
		regOffset = RA_CLKRST_MtrCkCtl;
		break;
	case ISPSS_CLKRST_CLK_TSBDHUBDY:
	case ISPSS_CLKRST_CLK_TSBDHUBSW:
	case ISPSS_CLKRST_CLK_TSBDHUB:
	case ISPSS_CLKRST_CLK_TSBCFG:
		clkStartOffset = ISPSS_CLKRST_CLK_TSB_START;
		regOffset = RA_CLKRST_TsbCkCtl;
		break;
	case ISPSS_CLKRST_CLK_FWRDHUBDY:
	case ISPSS_CLKRST_CLK_FWRDHUBSW:
	case ISPSS_CLKRST_CLK_FWRDHUB:
	case ISPSS_CLKRST_CLK_FWRCFG:
		clkStartOffset = ISPSS_CLKRST_CLK_FWR_START;
		regOffset = RA_CLKRST_FwrCkCtl;
		break;
	case ISPSS_CLKRST_CLK_BCMCFG:
	case ISPSS_CLKRST_CLK_BCMSYS:
	case ISPSS_CLKRST_CLK_BCMCSI0:
	case ISPSS_CLKRST_CLK_BCMCSI1:
	case ISPSS_CLKRST_CLK_BCMISP:
	case ISPSS_CLKRST_CLK_BCMSCL:
	case ISPSS_CLKRST_CLK_BCMBE:
	case ISPSS_CLKRST_CLK_BCMDPHYCFG:
	case ISPSS_CLKRST_CLK_BCMDPHYPHY:
		clkStartOffset = ISPSS_CLKRST_CLK_BCM_START;
		regOffset = RA_CLKRST_BcmCkCtl;
		break;
	case ISPSS_CLKRST_CLK_DPHYCFG:
	case ISPSS_CLKRST_CLK_DPHYPHY:
	case ISPSS_CLKRST_CLK_CSICFG:
	case ISPSS_CLKRST_CLK_CSICSI0:
	case ISPSS_CLKRST_CLK_CSICSI1:
		clkStartOffset = ISPSS_CLKRST_CLK_CSI_START;
		regOffset = RA_CLKRST_CSICkCtl;
		break;
	case ISPSS_CLKRST_CLK_ISPCFG:
	case ISPSS_CLKRST_CLK_ISPSYS:
	case ISPSS_CLKRST_CLK_ISPISP:
	case ISPSS_CLKRST_CLK_ISPCSI0:
	case ISPSS_CLKRST_CLK_ISPCSI1:
	case ISPSS_CLKRST_CLK_ISPMCM:
		clkStartOffset = ISPSS_CLKRST_CLK_ISP_START;
		regOffset = RA_CLKRST_ISPCkCtl;
		break;
	case ISPSS_CLKRST_CLK_TILECFG:
	case ISPSS_CLKRST_CLK_TILEISP:
	case ISPSS_CLKRST_CLK_TILESYS:
		clkStartOffset = ISPSS_CLKRST_CLK_TILE_START;
		regOffset = RA_CLKRST_TileCkCtl;
		break;
	case ISPSS_CLKRST_CLK_ROTCFG:
	case ISPSS_CLKRST_CLK_ROTSYS:
	case ISPSS_CLKRST_CLK_ROTBE:
		clkStartOffset = ISPSS_CLKRST_CLK_ROT_START;
		regOffset = RA_CLKRST_RotCkCtl;
		break;
	case ISPSS_CLKRST_CLK_DWRPCFG:
	case ISPSS_CLKRST_CLK_DWRPBE:
	case ISPSS_CLKRST_CLK_DWRPSYS:
		clkStartOffset = ISPSS_CLKRST_CLK_DWRP_START;
		regOffset = RA_CLKRST_DwrpCkCtl;
		break;
	case ISPSS_CLKRST_CLK_SCLCFG:
	case ISPSS_CLKRST_CLK_SCLSCL:
	case ISPSS_CLKRST_CLK_SCLSYS:
	case ISPSS_CLKRST_CLK_SCLISP:
		clkStartOffset = ISPSS_CLKRST_CLK_SCL_START;
		regOffset = RA_CLKRST_SclCkCtl;
		break;
	case ISPSS_CLKRST_CLK_MAX:
	default:
		break;
	}

	*pRegAddr = MEMMAP_ISPSS_MISC_CLKRST_BASE_ADDR + regOffset;
	*pBitPos = clk - clkStartOffset;
}

enum ISPSS_CLKRST_CLK_STATE ISPSS_CLKRST_GetClkEnable(enum ISPSS_CLKRST_CLK clk)
{
	UINT32 regAddr, bitPos, regVal = 0;

	if (clk < ISPSS_CLKRST_CLK_MAX) {
		ISPSS_CLKRST_getClkRegisterDetails(clk, &regAddr, &bitPos);
		ISPSS_REG_READ32(regAddr, &regVal);
		regVal &= (1 << bitPos);
	}

	return regVal ? ISPSS_CLKRST_CLK_STATE_ENABLE : ISPSS_CLKRST_CLK_STATE_DISABLE;
}

void ISPSS_CLKRST_SetClkEnable(enum ISPSS_CLKRST_CLK clk,
		enum ISPSS_CLKRST_CLK_STATE clkState)
{
	UINT32 regAddr, bitPos, regVal = 0;

	if (clk < ISPSS_CLKRST_CLK_MAX) {
		ISPSS_CLKRST_getClkRegisterDetails(clk, &regAddr, &bitPos);
		ISPSS_REG_READ32(regAddr, &regVal);
		if (clkState == ISPSS_CLKRST_CLK_STATE_ENABLE)
			regVal |= (1 << bitPos);
		else
			regVal &= ~(1 << bitPos);
		ISPSS_REG_WRITE32(regAddr, regVal);
		ISPSS_REG_READ32(regAddr, &regVal);//Debug
	}
}

void ISPSS_CLKRST_ISPMiscModuleSetClockGateState(enum ISPMISC_MODULES module,
		enum ISPSS_CLKRST_CLK_STATE clkState)
{
	UINT32 regAddr, bitPos, regVal = 0, currRegVal = 0;
	enum ISPSS_CLKRST_CLK clk;

	if (module < ISPMISC_MODULE_MIN && module >= ISPMISC_MODULE_MAX) {
		pr_err("Invalid module\n");
		return;
	}

	switch (module) {
	case ISPMISC_MODULE_BCM:
		clk = ISPSS_CLKRST_CLK_BCM_END;
		break;
	case ISPMISC_MODULE_DHUB_TSB:
		clk = ISPSS_CLKRST_CLK_TSB_END;
		break;
	case ISPMISC_MODULE_DHUB_FWR:
		clk = ISPSS_CLKRST_CLK_FWR_END;
		break;
	default:
		break;
	}

	ISPSS_CLKRST_getClkRegisterDetails(clk, &regAddr, &bitPos);
	ISPSS_REG_READ32(regAddr, &regVal);
	currRegVal = regVal;
	if (clkState == ISPSS_CLKRST_CLK_STATE_ENABLE)
		regVal |= ((1 << (bitPos + 1)) - 1);
	else
		regVal &= ~((1 << (bitPos + 1)) - 1);
	ISPSS_REG_WRITE32(regAddr, regVal);
	pr_debug("\nispmiscmodule:%d regAddr:%X currRegVal:%X newregVal:%X State:%d bitPos:%d",
			module, regAddr, currRegVal, regVal, clkState, bitPos);
}

void ISPSS_CLKRST_ISPBeModuleSetClockGateState(enum ISPBE_MODULES module,
		enum ISPSS_CLKRST_CLK_STATE clkState)
{
	UINT32 regAddr, bitPos, regVal = 0, currRegVal = 0;
	enum ISPSS_CLKRST_CLK clk;

	if (module < ISPBE_MODULE_MIN && module >= ISPBE_MODULE_MAX) {
		pr_err("Invalid module\n");
		return;
	}

	switch (module) {
	case ISPBE_MODULE_TILER:
		clk = ISPSS_CLKRST_CLK_TILE_END;
		break;
	case ISPBE_MODULE_DEWARP:
		clk = ISPSS_CLKRST_CLK_DWRP_END;
		break;
	case ISPBE_MODULE_DNSCL3:
		clk = ISPSS_CLKRST_CLK_SCL_END;
		break;
	case ISPBE_MODULE_ROT:
		clk = ISPSS_CLKRST_CLK_ROT_END;
		break;
	default:
		break;
	}

	ISPSS_CLKRST_getClkRegisterDetails(clk, &regAddr, &bitPos);
	ISPSS_REG_READ32(regAddr, &regVal);
	currRegVal = regVal;
	if (clkState == ISPSS_CLKRST_CLK_STATE_ENABLE)
		regVal |= ((1 << (bitPos + 1)) - 1);
	else
		regVal &= ~((1 << (bitPos + 1)) - 1);
	ISPSS_REG_WRITE32(regAddr, regVal);
	pr_debug("module:%d regAddr:%X currRegVal:%X updatedRegVal:%X State:%d bitPos:%d\n",
			module, regAddr, currRegVal, regVal, clkState, bitPos);
}

static enum ISPSS_CLKRST_RESET_STATE ISPSS_CLKRST_GetReset(UINT32 regAddr,
		enum ISPSS_CLKRST_FUNC_BLK funcBlk)
{
	UINT32 bitMask, regVal = 0;

	if (funcBlk < ISPSS_CLKRST_FUNC_MAX) {
		bitMask = (0x01<<funcBlk);
		ISPSS_REG_READ32(regAddr, &regVal);
		regVal &= bitMask;
	}

	return regVal ? ISPSS_CLKRST_RESET_STATE_ENABLE : ISPSS_CLKRST_RESET_STATE_DISABLE;
}

static void ISPSS_CLKRST_SetReset(UINT32 regAddr, enum ISPSS_CLKRST_FUNC_BLK funcBlk,
		enum ISPSS_CLKRST_RESET_STATE rstState)
{
	UINT32 bitMask, regVal = 0;

	if (funcBlk < ISPSS_CLKRST_FUNC_MAX) {
		bitMask = (0x01<<funcBlk);
		ISPSS_REG_READ32(regAddr, &regVal);
		if (rstState == ISPSS_CLKRST_RESET_STATE_ENABLE)
			regVal |= (bitMask);
		else
			regVal &= ~(bitMask);
		ISPSS_REG_WRITE32(regAddr, regVal);
	}
}

static void ISPSS_CLKRST_DoReset(UINT32 regAddr, enum ISPSS_CLKRST_FUNC_BLK funcBlk)
{
	ISPSS_CLKRST_SetReset(regAddr, funcBlk, ISPSS_CLKRST_RESET_STATE_ENABLE);
	//TBD:Delay Required?
	ISPSS_CLKRST_SetReset(regAddr, funcBlk, ISPSS_CLKRST_RESET_STATE_DISABLE);
}

enum ISPSS_CLKRST_RESET_STATE ISPSS_CLKRST_GetFuncReset(enum ISPSS_CLKRST_FUNC_BLK funcBlk)
{
	return ISPSS_CLKRST_GetReset(MEMMAP_ISPSS_MISC_CLKRST_BASE_ADDR +
			RA_CLKRST_FuncReset, funcBlk);
}

void ISPSS_CLKRST_SetFuncReset(enum ISPSS_CLKRST_FUNC_BLK funcBlk,
		enum ISPSS_CLKRST_RESET_STATE rst)
{
	ISPSS_CLKRST_SetReset(MEMMAP_ISPSS_MISC_CLKRST_BASE_ADDR +
			RA_CLKRST_FuncReset, funcBlk, rst);
}

void ISPSS_CLKRST_DoFuncReset(enum ISPSS_CLKRST_FUNC_BLK funcBlk)
{
	ISPSS_CLKRST_DoReset(MEMMAP_ISPSS_MISC_CLKRST_BASE_ADDR +
			RA_CLKRST_FuncReset, funcBlk);
}

enum ISPSS_CLKRST_RESET_STATE ISPSS_CLKRST_GetBiuReset(enum ISPSS_CLKRST_FUNC_BLK funcBlk)
{
	return ISPSS_CLKRST_GetReset(MEMMAP_ISPSS_MISC_CLKRST_BASE_ADDR
			+ RA_CLKRST_BiuReset, funcBlk);
}

void ISPSS_CLKRST_SetBiuReset(enum ISPSS_CLKRST_FUNC_BLK funcBlk,
		enum ISPSS_CLKRST_RESET_STATE rst)
{
	ISPSS_CLKRST_SetReset(MEMMAP_ISPSS_MISC_CLKRST_BASE_ADDR
			+ RA_CLKRST_BiuReset, funcBlk, rst);
}

void ISPSS_CLKRST_DoBiuReset(enum ISPSS_CLKRST_FUNC_BLK funcBlk)
{
	ISPSS_CLKRST_DoReset(MEMMAP_ISPSS_MISC_CLKRST_BASE_ADDR
			+ RA_CLKRST_BiuReset, funcBlk);
}

static void ISPSS_CLKRST_getIsoRegisterDetails(enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		enum ISPSS_CLKRST_ISO_OPER_TYPE isoOperType,
		UINT32 funcAxiBiu, UINT32 *pRegAddr, UINT32 *pBitPos)
{
	UINT32 regOffset;

	switch (isoFuncType) {
	case ISPSS_CLKRST_ISO_FUNC_TYPE_AXI:
		regOffset = RA_CLKRST_AxiIsoEnable;
		break;
	case ISPSS_CLKRST_ISO_FUNC_TYPE_OCP:
		regOffset = RA_CLKRST_OcpIsoEnable;
		break;
	case ISPSS_CLKRST_ISO_FUNC_TYPE_CFG:
		regOffset = RA_CLKRST_CfgIsoEnable;
		break;
	case ISPSS_CLKRST_ISO_FUNC_TYPE_MAX:
	default:
		break;
	}
	*pRegAddr = MEMMAP_ISPSS_MISC_CLKRST_BASE_ADDR + regOffset + (4 * isoOperType);
	*pBitPos = (1 << funcAxiBiu);
}

enum ISPSS_CLKRST_ISO_STATE ISPSS_CLKRST_GetIsoEnable(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		UINT32 funcAxiBiu, UINT32 maxFuncAxiBiu)
{
	UINT32 regVal = 0x0;

	if ((isoFuncType < ISPSS_CLKRST_ISO_FUNC_TYPE_MAX)
			&& (funcAxiBiu < maxFuncAxiBiu)) {
		UINT32 regAddr, bitPos;

		ISPSS_CLKRST_getIsoRegisterDetails(isoFuncType,
				ISPSS_CLKRST_ISO_OPER_TYPE_ENABLE,
				funcAxiBiu, &regAddr, &bitPos);
		ISPSS_REG_READ32(regAddr, &regVal);
		regVal &= (1<<bitPos);
	}

	return regVal ? ISPSS_CLKRST_ISO_STATE_ENABLE : ISPSS_CLKRST_ISO_STATE_DISABLE;
}

void ISPSS_CLKRST_SetIsoEnable(enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		UINT32 funcAxiBiu, UINT32 maxFuncAxiBiu,
		enum ISPSS_CLKRST_ISO_STATE isoEnable)
{
	UINT32 regVal = 0x0;

	if ((isoFuncType < ISPSS_CLKRST_ISO_FUNC_TYPE_MAX)
			&& (funcAxiBiu < maxFuncAxiBiu)) {
		UINT32 regAddr, bitPos;

		ISPSS_CLKRST_getIsoRegisterDetails(isoFuncType,
				ISPSS_CLKRST_ISO_OPER_TYPE_ENABLE,
				funcAxiBiu, &regAddr, &bitPos);
		ISPSS_REG_READ32(regAddr, &regVal);
		if (isoEnable == ISPSS_CLKRST_ISO_STATE_ENABLE)
			regVal |= (1 << bitPos);
		else
			regVal &= ~(1 << bitPos);
		ISPSS_REG_WRITE32(regAddr, regVal);
	}
}

enum ISPSS_CLKRST_ISO_STATE ISPSS_CLKRST_GetIsoClear(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		UINT32 funcAxiBiu, UINT32 maxFuncAxiBiu)
{
	UINT32 regVal = 0x0;

	if ((isoFuncType < ISPSS_CLKRST_ISO_FUNC_TYPE_MAX)
			&& (funcAxiBiu < maxFuncAxiBiu)) {
		UINT32 regAddr, bitPos;

		ISPSS_CLKRST_getIsoRegisterDetails(isoFuncType,
				ISPSS_CLKRST_ISO_OPER_TYPE_CLEAR,
				funcAxiBiu, &regAddr, &bitPos);
		ISPSS_REG_READ32(regAddr, &regVal);
		regVal &= (1 << bitPos);
	}

	return regVal ? ISPSS_CLKRST_ISO_STATE_ENABLE : ISPSS_CLKRST_ISO_STATE_DISABLE;
}

void ISPSS_CLKRST_SetIsoClear(enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		UINT32 funcAxiBiu, UINT32 maxFuncAxiBiu,
		enum ISPSS_CLKRST_ISO_STATE isoEnable)
{
	UINT32 regVal = 0x0;

	if ((isoFuncType < ISPSS_CLKRST_ISO_FUNC_TYPE_MAX)
			&& (funcAxiBiu < maxFuncAxiBiu)) {
		UINT32 regAddr, bitPos;

		ISPSS_CLKRST_getIsoRegisterDetails(isoFuncType,
				ISPSS_CLKRST_ISO_OPER_TYPE_CLEAR,
				funcAxiBiu, &regAddr, &bitPos);
		ISPSS_REG_READ32(regAddr, &regVal);
		if (isoEnable == ISPSS_CLKRST_ISO_STATE_ENABLE)
			regVal |= (1 << bitPos);
		else
			regVal &= ~(1 << bitPos);
		ISPSS_REG_WRITE32(regAddr, regVal);
	}
}

enum ISPSS_CLKRST_ISO_STATE_MACHINE ISPSS_CLKRST_GetIsoState(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		UINT32 funcAxiBiu, UINT32 maxFuncAxiBiu)
{
	UINT32 regVal = 0x0;

	if ((isoFuncType < ISPSS_CLKRST_ISO_FUNC_TYPE_MAX)
			&& (funcAxiBiu < maxFuncAxiBiu)) {
		UINT32 regAddr, bitPos;

		ISPSS_CLKRST_getIsoRegisterDetails(isoFuncType,
				ISPSS_CLKRST_ISO_OPER_TYPE_ST, funcAxiBiu, &regAddr, &bitPos);
		ISPSS_REG_READ32(regAddr, &regVal);
		regVal &= (1 << bitPos);
	}

	return (UINT32)(regVal ? ISPSS_CLKRST_ISO_STATE_ENABLE : ISPSS_CLKRST_ISO_STATE_DISABLE);
}

static __maybe_unused enum ISPSS_CLKRST_ISO_STATE ISPSS_CLKRST_GetAxiIsoEnable(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType, UINT32 funcAxiBiu)
{
	return ISPSS_CLKRST_GetIsoEnable(isoFuncType, funcAxiBiu,
			ISPSS_CLKRST_ISO_FUNC_AXI_MAX);
}

static __maybe_unused void ISPSS_CLKRST_SetAxiIsoEnable(enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		UINT32 funcAxiBiu, enum ISPSS_CLKRST_ISO_STATE isoEnable)
{
	ISPSS_CLKRST_SetIsoEnable(isoFuncType, funcAxiBiu,
		ISPSS_CLKRST_ISO_FUNC_AXI_MAX, isoEnable);
}

static __maybe_unused enum ISPSS_CLKRST_ISO_STATE ISPSS_CLKRST_GetAxiIsoClear(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType, UINT32 funcAxiBiu)
{
	return ISPSS_CLKRST_GetIsoClear(isoFuncType, funcAxiBiu,
			ISPSS_CLKRST_ISO_FUNC_AXI_MAX);
}

static __maybe_unused void ISPSS_CLKRST_SetAxiIsoClear(enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		UINT32 funcAxiBiu, enum ISPSS_CLKRST_ISO_STATE isoEnable)
{
	ISPSS_CLKRST_SetIsoClear(isoFuncType, funcAxiBiu,
		ISPSS_CLKRST_ISO_FUNC_AXI_MAX, isoEnable);
}

static __maybe_unused enum ISPSS_CLKRST_ISO_STATE_MACHINE ISPSS_CLKRST_GetAxiIsoState(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType, UINT32 funcAxiBiu)
{
	return ISPSS_CLKRST_GetIsoState(isoFuncType, funcAxiBiu,
			ISPSS_CLKRST_ISO_FUNC_AXI_MAX);
}

static __maybe_unused enum ISPSS_CLKRST_ISO_STATE ISPSS_CLKRST_GetOcpIsoEnable(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType, UINT32 funcAxiBiu)
{
	return ISPSS_CLKRST_GetIsoEnable(isoFuncType, funcAxiBiu,
			ISPSS_CLKRST_ISO_FUNC_OCP_MAX);
}

static __maybe_unused enum ISPSS_CLKRST_ISO_STATE ISPSS_CLKRST_GetCfgIsoEnable(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType, UINT32 funcCfgBiu)
{
	return ISPSS_CLKRST_GetIsoEnable(isoFuncType, funcCfgBiu,
			ISPSS_CLKRST_ISO_FUNC_CFG_MAX);
}

static __maybe_unused void ISPSS_CLKRST_SetCfgIsoEnable(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		UINT32 funcCfgBiu, enum ISPSS_CLKRST_ISO_STATE isoEnable)
{
	ISPSS_CLKRST_SetIsoEnable(isoFuncType, funcCfgBiu,
		ISPSS_CLKRST_ISO_FUNC_CFG_MAX, isoEnable);
}

static __maybe_unused enum ISPSS_CLKRST_ISO_STATE ISPSS_CLKRST_GetCfgIsoClear(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		UINT32 funcCfgBiu)
{
	return ISPSS_CLKRST_GetIsoClear(isoFuncType, funcCfgBiu,
			ISPSS_CLKRST_ISO_FUNC_CFG_MAX);
}

static __maybe_unused void ISPSS_CLKRST_SetCfgIsoClear(enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType,
		UINT32 funcCfgBiu, enum ISPSS_CLKRST_ISO_STATE isoEnable)
{
	ISPSS_CLKRST_SetIsoClear(isoFuncType, funcCfgBiu,
		ISPSS_CLKRST_ISO_FUNC_CFG_MAX, isoEnable);
}

static __maybe_unused enum ISPSS_CLKRST_ISO_STATE_MACHINE ISPSS_CLKRST_GetCfgIsoState(
		enum ISPSS_CLKRST_ISO_FUNC_TYPE isoFuncType, UINT32 funcCfgBiu)
{
	return ISPSS_CLKRST_GetIsoState(isoFuncType, funcCfgBiu,
			ISPSS_CLKRST_ISO_FUNC_CFG_MAX);
}

