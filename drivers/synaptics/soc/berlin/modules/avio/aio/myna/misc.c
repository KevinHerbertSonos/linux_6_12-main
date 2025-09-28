// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include "aio_hal.h"
#include "aio_common.h"
#include "i2s_common.h"
#include "avioDhub.h"
#include "avioGbl.h"
#include <linux/delay.h>

int aio_misc_enable_audio_timer(void *hd, bool en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32AIO_SAMP_CTRL reg;
	u32 offset;

	offset = RA_AIO_SAMP_CTRL;
	reg.u32 = aio_read(aio, offset);
	reg.uSAMP_CTRL_EN_AUDTIMER = en;
	aio_write(aio, offset, reg.u32);
	dev_dbg(aio->dev, "%s ATR Timer Enable %d", __func__, en);
	return 0;
}
EXPORT_SYMBOL(aio_misc_enable_audio_timer);

int aio_misc_get_audio_timer(void *hd, u32 *val)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32AIO_ATR reg;

	reg.u32 = aio_read(aio, RA_AIO_ATR);
	return reg.uATR_TIMER;
}
EXPORT_SYMBOL(aio_misc_get_audio_timer);

int aio_i2s_pause_status(void *hd)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32AIO_APRPR reg;

	reg.u32 = aio_read(aio, RA_AIO_APRPR);
	return reg.u32;
}
EXPORT_SYMBOL(aio_i2s_pause_status);

int aio_i2s_pause(void *hd, u32 id, bool pause, u32 pauseSampleCount)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 offset, offset_PSC;
	T32AIO_APRPR reg;

	offset = RA_AIO_APRPR;
	reg.u32 = aio_read(aio, offset);
	offset_PSC = aio_get_reg_base_addr(id);
	switch (id) {
	case AIO_ID_I2S1_TX:
		offset_PSC += RA_SEC_PSC;
		reg.uAPRPR_SEC1_PE = pause;
		reg.uAPRPR_SEC1_RSE = !pause;
		break;
	case AIO_ID_I2S2_TX:
		offset_PSC += RA_SEC_PSC;
		reg.uAPRPR_SEC2_PE = pause;
		reg.uAPRPR_SEC2_RSE = !pause;
		break;
	case AIO_ID_I2S3_TX:
		offset_PSC += RA_SEC_PSC;
		reg.uAPRPR_SEC3_PE = pause;
		reg.uAPRPR_SEC3_RSE = !pause;
		break;
	case AIO_ID_I2S4_TX:
		offset_PSC += RA_SEC_PSC;
		reg.uAPRPR_SEC4_PE = pause;
		reg.uAPRPR_SEC4_RSE = !pause;
		break;
	case AIO_ID_I2S5_TX:
		offset_PSC += RA_SEC_PSC;
		reg.uAPRPR_SEC5_PE = pause;
		reg.uAPRPR_SEC5_RSE = !pause;
		break;
	case AIO_ID_I2S1_RX:
		offset_PSC += RA_MIC2_PSC;
		reg.uAPRPR_MIC21_PE = pause;
		reg.uAPRPR_MIC21_RSE = !pause;
		break;
	case AIO_ID_I2S2_RX:
		offset_PSC += RA_MIC2_PSC;
		reg.uAPRPR_MIC22_PE = pause;
		reg.uAPRPR_MIC22_RSE = !pause;
		break;
	case AIO_ID_I2S3_RX:
		offset_PSC += RA_MIC2_PSC;
		reg.uAPRPR_MIC23_PE = pause;
		reg.uAPRPR_MIC23_RSE = !pause;
		break;
	case AIO_ID_I2S4_RX:
		offset_PSC += RA_MIC2_PSC;
		reg.uAPRPR_MIC24_PE = pause;
		reg.uAPRPR_MIC24_RSE = !pause;
		break;
	case AIO_ID_I2S5_RX:
		offset_PSC += RA_MIC2_PSC;
		reg.uAPRPR_MIC25_PE = pause;
		reg.uAPRPR_MIC25_RSE = !pause;
		break;
	default:
		dev_err(aio->dev, "%s chid%d not supported\n", __func__, id);
		return -EINVAL;
	}

	if (pause)
		aio_write(aio, offset_PSC, pauseSampleCount);
	aio_write(aio, offset, reg.u32);
	dev_dbg(aio->dev, "%s I2S id %d pause=%08X sampleCnt=%08d\n",
				__func__, id, reg.u32, pauseSampleCount);
	return 0;
}
EXPORT_SYMBOL(aio_i2s_pause);

int aio_get_scr(void *hd, u32 id)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 offset, offsetSCR, uiSCR;
	T32AIO_SAMPINFO_REQ reg;

	offset = RA_AIO_SAMPINFO_REQ;
	reg.u32 = 0;
	switch (id) {
	case AIO_ID_I2S1_TX:
		offsetSCR = RA_AIO_SCR;
		reg.uSAMPINFO_REQ_I2STX1 = true;
		break;
	case AIO_ID_I2S2_TX:
		offsetSCR = RA_AIO_SCR1;
		reg.uSAMPINFO_REQ_I2STX2 = true;
		break;
	case AIO_ID_I2S3_TX:
		offsetSCR = RA_AIO_SCR2;
		reg.uSAMPINFO_REQ_I2STX3 = true;
		break;
	case AIO_ID_I2S4_TX:
		offsetSCR = RA_AIO_SCR3;
		reg.uSAMPINFO_REQ_I2STX4 = true;
		break;
	case AIO_ID_I2S5_TX:
		offsetSCR = RA_AIO_SCR4;
		reg.uSAMPINFO_REQ_I2STX5 = true;
		break;
	case AIO_ID_I2S1_RX:
		offsetSCR = RA_AIO_SCR5;
		reg.uSAMPINFO_REQ_I2SRX1 = true;
		break;
	case AIO_ID_I2S2_RX:
		offsetSCR = RA_AIO_SCR6;
		reg.uSAMPINFO_REQ_I2SRX2 = true;
		break;
	case AIO_ID_I2S3_RX:
		offsetSCR = RA_AIO_SCR7;
		reg.uSAMPINFO_REQ_I2SRX3 = true;
		break;
	case AIO_ID_I2S4_RX:
		offsetSCR = RA_AIO_SCR8;
		reg.uSAMPINFO_REQ_I2SRX4 = true;
		break;
	case AIO_ID_I2S5_RX:
		offsetSCR = RA_AIO_SCR9;
		reg.uSAMPINFO_REQ_I2SRX5 = true;
		break;
	default:
		dev_err(aio->dev, "%s id%d not supported\n", __func__, id);
		return -EINVAL;
	}
	aio_write(aio, offset, reg.u32);

	uiSCR = aio_read(aio, offsetSCR);
	dev_dbg(aio->dev, "%s id %d sampleReq en:%08X SCR=%08X\n", __func__, id, reg.u32, uiSCR);
	return uiSCR;
}
EXPORT_SYMBOL(aio_get_scr);

int aio_en_scr_str(void *hd, u32 id, bool en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 offset;
	T32AIO_SAMP_CTRL reg;

	offset = RA_AIO_SAMP_CTRL;
	reg.u32 = aio_read(aio, offset);
	switch (id) {
	case AIO_ID_I2S1_TX:
		reg.uSAMP_CTRL_EN_I2STX1 = en;
		reg.uSAMP_CTRL_PAUSE_I2STX1 = true;
		break;
	case AIO_ID_I2S2_TX:
		reg.uSAMP_CTRL_EN_I2STX2 = en;
		reg.uSAMP_CTRL_PAUSE_I2STX2 = true;
		break;
	case AIO_ID_I2S3_TX:
		reg.uSAMP_CTRL_EN_I2STX3 = en;
		reg.uSAMP_CTRL_PAUSE_I2STX3 = true;
		break;
	case AIO_ID_I2S4_TX:
		reg.uSAMP_CTRL_EN_I2STX4 = en;
		reg.uSAMP_CTRL_PAUSE_I2STX4 = true;
		break;
	case AIO_ID_I2S5_TX:
		reg.uSAMP_CTRL_EN_I2STX5 = en;
		reg.uSAMP_CTRL_PAUSE_I2STX5 = true;
		break;
	case AIO_ID_I2S1_RX:
		reg.uSAMP_CTRL_EN_I2SRX1 = en;
		reg.uSAMP_CTRL_PAUSE_I2SRX1 = true;
		break;
	case AIO_ID_I2S2_RX:
		reg.uSAMP_CTRL_EN_I2SRX2 = en;
		reg.uSAMP_CTRL_PAUSE_I2SRX2 = true;
		break;
	case AIO_ID_I2S3_RX:
		reg.uSAMP_CTRL_EN_I2SRX3 = en;
		reg.uSAMP_CTRL_PAUSE_I2SRX3 = true;
		break;
	case AIO_ID_I2S4_RX:
		reg.uSAMP_CTRL_EN_I2SRX4 = en;
		reg.uSAMP_CTRL_PAUSE_I2SRX4 = true;
		break;
	case AIO_ID_I2S5_RX:
		reg.uSAMP_CTRL_EN_I2SRX5 = en;
		reg.uSAMP_CTRL_PAUSE_I2SRX5 = true;
		break;
	case AIO_ID_PDM_IN:
		reg.uSAMP_CTRL_EN_DMICRX = en;
		break;
	default:
		dev_err(aio->dev, "%s id%d not supported\n", __func__, id);
		return -EINVAL;
	}
	aio_write(aio, offset, reg.u32);
	pr_debug("%s id %d SCR STR en:%08X\n", __func__, id, reg.u32);
	return 0;
}
EXPORT_SYMBOL(aio_en_scr_str);

int aio_set_extPortCfg(void *hd, u32 id, u32 aprktMode, bool enAprpr)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32MIC2_RXPORT reg_micrxport;
	T32SEC_SECPORT reg_secport;

	switch (id) {
	case AIO_ID_I2S1_TX:
	case AIO_ID_I2S2_TX:
	case AIO_ID_I2S3_TX:
	case AIO_ID_I2S4_TX:
	case AIO_ID_I2S5_TX:
		address = aio_get_reg_base_addr(id) + RA_SEC_SECPORT;
		reg_secport.u32 = aio_read(aio, address);
		reg_secport.uSECPORT_APRKT_ON   = !(aprktMode == AIO_I2S_APRKT_OFF);
		reg_secport.uSECPORT_APRKT_ABSO = (aprktMode == AIO_I2S_APRKT_ON_ABSO);
		reg_secport.uSECPORT_APRKT_ASAP = (aprktMode == AIO_I2S_APRKT_ON_ASAP);
		reg_secport.uSECPORT_APRKT_RELA = (aprktMode == AIO_I2S_APRKT_ON_RELA);
		reg_secport.uSECPORT_APRPR_ON = enAprpr;
		reg_secport.uSECPORT_FIFO_CG_ON = 1;
		aio_write(aio, address, reg_secport.u32);
		dev_dbg(aio->dev, "%s chid%d TX aprktMode %d APRPR %d", __func__, id,
				aprktMode, reg_secport.uSECPORT_APRPR_ON);
		break;
	case AIO_ID_I2S1_RX:
	case AIO_ID_I2S2_RX:
	case AIO_ID_I2S3_RX:
	case AIO_ID_I2S4_RX:
	case AIO_ID_I2S5_RX:
		address = aio_get_reg_base_addr(id) + RA_MIC2_RXPORT;
		reg_micrxport.u32 = aio_read(aio, address);
		reg_micrxport.uRXPORT_APRKT_ON   = !(aprktMode == AIO_I2S_APRKT_OFF);
		reg_micrxport.uRXPORT_APRKT_ABSO = (aprktMode == AIO_I2S_APRKT_ON_ABSO);
		reg_micrxport.uRXPORT_APRKT_ASAP = (aprktMode == AIO_I2S_APRKT_ON_ASAP);
		reg_micrxport.uRXPORT_APRKT_RELA = (aprktMode == AIO_I2S_APRKT_ON_RELA);
		reg_micrxport.uRXPORT_APRPR_ON = enAprpr;
		aio_write(aio, address, reg_micrxport.u32);
		dev_dbg(aio->dev, "%s id %d RX aprktMode %d APRPR %d", __func__, id,
				aprktMode, reg_micrxport.uRXPORT_APRPR_ON);
		break;
	default:
		dev_err(aio->dev, "%s chid%d not supported\n", __func__, id);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(aio_set_extPortCfg);

int aio_set_sync_play(void *hd, u32 aprktENIdx, bool play)
{
	struct aio_priv *aio = hd_to_aio(hd);
	int i;
	u32 aprktEN = 0;
	T32AIO_APRKT_EN aprkt_en;
	u32 chIdxAprktEnMask[] = {
			MSK32AIO_APRKT_EN_SEC1, MSK32AIO_APRKT_EN_MIC21, MSK32AIO_APRKT_EN_SEC2,
			MSK32AIO_APRKT_EN_MIC22, MSK32AIO_APRKT_EN_SEC3, MSK32AIO_APRKT_EN_MIC23,
			MSK32AIO_APRKT_EN_SEC4,  MSK32AIO_APRKT_EN_MIC24, -1, MSK32AIO_APRKT_EN_SEC5,
			MSK32AIO_APRKT_EN_MIC25};
	aprkt_en.u32 = aio_read(aio, RA_AIO_APRKT_EN);
	for (i = 0; aprktENIdx; ++i) {
		aprktEN |= (aprktENIdx & 1) ? chIdxAprktEnMask[i] : false;
		aprktENIdx = (aprktENIdx >> 1);
	}
	if (play)
		aprkt_en.u32 |= aprktEN;
	else
		aprkt_en.u32 = ((~aprktEN) & aprkt_en.u32);
	aio_write(aio, RA_AIO_APRKT_EN, aprkt_en.u32);
	dev_dbg(aio->dev, "%s aprktEN %d APRKT_EN = 0x%04X play %d\n",
				__func__, aprktEN, aprkt_en.u32, play);
	return 0;
}
EXPORT_SYMBOL(aio_set_sync_play);
