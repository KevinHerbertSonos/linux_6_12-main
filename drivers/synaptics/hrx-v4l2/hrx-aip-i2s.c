// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-aip-i2s.h"

#define delay_i2s_ms(ms) delay_us((ms)*1000)
#define bTST(x, b) (((x) >> (b)) & 1)

#define BFM_HOST_Bus_Write32(offset, val) GA_REG_WORD32_WRITE(offset, val)
#define BFM_HOST_Bus_Read32(offset, holder) GA_REG_WORD32_READ(offset, holder)

#define MUTE_ON_AUDCH(I2S_AUD_CTRL) \
{															\
	T32AUDCH_CTRL reg;										\
	BFM_HOST_Bus_Read32((I2S_AUD_CTRL), &(reg.u32));		\
	reg.uCTRL_MUTE = AUDCH_CTRL_MUTE_MUTE_ON;				\
	BFM_HOST_Bus_Write32((I2S_AUD_CTRL), (reg.u32));		\
}

#define FLUSH_ON_AUDCH(I2S_AUD_CTRL) \
{															\
	T32AUDCH_CTRL reg;										\
	BFM_HOST_Bus_Read32((I2S_AUD_CTRL), &(reg.u32));		\
	reg.uCTRL_FLUSH = AUDCH_CTRL_FLUSH_ON;				\
	BFM_HOST_Bus_Write32((I2S_AUD_CTRL), (reg.u32));		\
}

#define FLUSH_OFF_AUDCH(I2S_AUD_CTRL) \
{															\
	T32AUDCH_CTRL reg;										\
	BFM_HOST_Bus_Read32((I2S_AUD_CTRL), &(reg.u32));		\
	reg.uCTRL_FLUSH = AUDCH_CTRL_FLUSH_OFF;				\
	BFM_HOST_Bus_Write32((I2S_AUD_CTRL), (reg.u32));		\
}

#define MUTE_OFF_AUDCH(I2S_AUD_CTRL) \
{															\
	T32AUDCH_CTRL reg;										\
	BFM_HOST_Bus_Read32((I2S_AUD_CTRL), &(reg.u32));		\
	reg.uCTRL_MUTE = AUDCH_CTRL_MUTE_MUTE_OFF;				\
	BFM_HOST_Bus_Write32((I2S_AUD_CTRL), (reg.u32));		\
}

#define SET_AND_ENABLE_RSD(I2S_MIC_RSD_CTRL, DEBUG_MODE, LRSW)	\
{														\
	T32AUDCH_CTRL reg;									\
	BFM_HOST_Bus_Read32((I2S_MIC_RSD_CTRL), &(reg.u32));	\
	reg.uCTRL_ENABLE = AUDCH_CTRL_ENABLE_ENABLE;		\
	reg.uCTRL_MUTE = AUDCH_CTRL_MUTE_MUTE_OFF;			\
	reg.uCTRL_LRSWITCH = LRSW;							\
	reg.uCTRL_DEBUGEN = DEBUG_MODE;						\
	BFM_HOST_Bus_Write32((I2S_MIC_RSD_CTRL), (reg.u32));	\
}

I2SConfigType i2sConfig = {

	//MIC3 cfg
	PRIAUD_CTRL_LEFTJFY_LEFT,
	PRIAUD_CTRL_TFM_I2S,
	PRIAUD_CTRL_TCF_24CFM,
	PRIAUD_CTRL_TDM_24DFM,			//note MIC1 only supports 16DFM
	PRIAUD_CLKDIV_SETTING_DIV8,		//slowed down bit clk because fpga
	PRIAUD_CTRL_INVFS_NORMAL,		//rx side invert fs, INVFS at MIC inverts FS signal, but the MIC still sample the first
									//data as Left in I2S mode. So it doesn't swap left/right samples. However, when disable
									//MIC, it will stop at a LOW FS signal and fill one more HI FS sample, if INVFS is 1, meaning
									//the LOW FS signal is a right sample, the MIC stops at the right state that has a LEFT sample
									//as its last one. When MIC starts again, it will have LEFT/RIGHT sample swapped because it
									//is expecting a RIGHT sample but in fact a LEFT sample is coming in.
	PRIAUD_CTRL_INVCLK_NORMAL,		//rx side invert clk, note that rx and tx settings need to be different to get good data
	PRIAUD_CTRL_TDMMODE_TDMMODE_OFF,
	PRIAUD_CTRL_TDMCHCNT_CHCNT_8,
	PRIAUD_CTRL_TLSB_MSB_FIRST,
	AUDCH_CTRL_LRSWITCH_SWITCH_OFF,
	PRI_AUD_CTL_TDM_HIGH_MIN,

	.i2s1_fb_bck = 0,
	.i2s1_fb_lr = 0,
	.i2s2_fb_bck = 0,
	.i2s2_fb_lr = 0,
	.i2s3_fb_bck = 0,
	.i2s3_fb_lr = 0,
};

void i2s_rx_mic3_flush(unsigned int regBase)
{
	FLUSH_ON_AUDCH(I2S_MIC3_RSD0_CTRL(regBase));
	FLUSH_ON_AUDCH(I2S_MIC3_RSD1_CTRL(regBase));
	FLUSH_ON_AUDCH(I2S_MIC3_RSD2_CTRL(regBase));
	FLUSH_ON_AUDCH(I2S_MIC3_RSD3_CTRL(regBase));

	FLUSH_OFF_AUDCH(I2S_MIC3_RSD0_CTRL(regBase));
	FLUSH_OFF_AUDCH(I2S_MIC3_RSD1_CTRL(regBase));
	FLUSH_OFF_AUDCH(I2S_MIC3_RSD2_CTRL(regBase));
	FLUSH_OFF_AUDCH(I2S_MIC3_RSD3_CTRL(regBase));
}

static void i2s_rx_mic3_enable(unsigned long regBase)
{
    // enable MIC
	T32MIC3_RXPORT reg3;

	BFM_HOST_Bus_Read32((I2S_MIC3_RXPORT(regBase)), &(reg3.u32));
	reg3.uRXPORT_ENABLE = MIC3_RXPORT_ENABLE_ENABLE;
	BFM_HOST_Bus_Write32((I2S_MIC3_RXPORT(regBase)), (reg3.u32));
}

static void i2s_rx_mic3_disable(unsigned long regBase)
{
    // disable MIC
	T32MIC3_RXPORT reg3;

	BFM_HOST_Bus_Read32((I2S_MIC3_RXPORT(regBase)), &(reg3.u32));
	reg3.uRXPORT_ENABLE = MIC3_RXPORT_ENABLE_DISABLE;
	BFM_HOST_Bus_Write32((I2S_MIC3_RXPORT(regBase)), (reg3.u32));

}

void i2s_rx_mic3_start(unsigned long regBase, int dbg_mode, I2SConfigType *pI2sConfig, int NumofChannel)
{
    // set up MIC control, input channel
	i2s_rx_mic3_disable(regBase);

    //  setup AIO MIC port clk div
	{
		T32PRIAUD_CLKDIV reg;

		BFM_HOST_Bus_Read32((I2S_MIC3_PRIAUD_CLKDIV(regBase)), &(reg.u32));
		reg.uCLKDIV_SETTING = pI2sConfig->Mic3AudClockDiv;
		BFM_HOST_Bus_Write32((I2S_MIC3_PRIAUD_CLKDIV(regBase)), (reg.u32));

	}

    // setup AIO MIC port pri control
	{
		T32PRIAUD_CTRL reg;
		T32PRIAUD_CTRL1 reg1;
		unsigned char mic3TDMChCnt;

		mic3TDMChCnt = (NumofChannel == 2) ? PRIAUD_CTRL_TDMCHCNT_CHCNT_2
										: ((NumofChannel == 6) ? PRIAUD_CTRL_TDMCHCNT_CHCNT_6
										: PRIAUD_CTRL_TDMCHCNT_CHCNT_8);

		BFM_HOST_Bus_Read32((I2S_MIC3_PRIAUD_CTRL(regBase)), &(reg.u32));
		reg.uCTRL_TDM = pI2sConfig->Mic3AudTDM;
		reg.uCTRL_TCF = pI2sConfig->Mic3AudTCF;
		reg.uCTRL_TFM = pI2sConfig->Mic3AudTFM;
		reg.uCTRL_LEFTJFY = pI2sConfig->Mic3LeftJfy;
		reg.uCTRL_INVFS = pI2sConfig->Mic3INVFS;
		reg.uCTRL_TDMWSHIGH = pI2sConfig->Mic3TDMHigh;
		reg.uCTRL_INVCLK = pI2sConfig->Mic3INVCLK;
		reg.uCTRL_TDMMODE = pI2sConfig->Mic3TDMMode;
		reg.uCTRL_TDMCHCNT = mic3TDMChCnt;
		reg.uCTRL_TLSB = pI2sConfig->Mic3TLSB;

		{
			T32MIC3_RXDATA reg3;

			BFM_HOST_Bus_Read32((I2S_MIC3_RXDATA(regBase)), &(reg3.u32));
			if (pI2sConfig->hdmi_rx_hd_en == 1 && NumofChannel == 8)
				reg3.uRXDATA_HBR = 1;
			else
				reg3.uRXDATA_HBR = 0;

			BFM_HOST_Bus_Write32((I2S_MIC3_RXDATA(regBase)), (reg3.u32));
		}

		BFM_HOST_Bus_Write32((I2S_MIC3_PRIAUD_CTRL(regBase)), (reg.u32));

		//Read and check
		{
			BFM_HOST_Bus_Read32((I2S_MIC3_PRIAUD_CTRL1(regBase)), &(reg1.u32));
			if (NumofChannel == 1) // PCM MONO
				reg1.uCTRL_PCM_MONO_CH = 1;
			else
				reg1.uCTRL_PCM_MONO_CH = 0;

			BFM_HOST_Bus_Write32((I2S_MIC3_PRIAUD_CTRL1(regBase)), (reg1.u32));
		}
	}

	{
		T32MIC3_INTLMODE reg_int;

		BFM_HOST_Bus_Read32(I2S_MIC3_INTLMODE(regBase), &(reg_int.u32));

		reg_int.uINTLMODE_PORT0_EN = 1;
		reg_int.uINTLMODE_PORT1_EN = 0;
		reg_int.uINTLMODE_PORT2_EN = 0;
		reg_int.uINTLMODE_PORT3_EN = 0;

		SET_AND_ENABLE_RSD(I2S_MIC3_RSD0_CTRL(regBase), dbg_mode, pI2sConfig->Mic3LRSw);

		switch (NumofChannel) {
		case 2:
			//No need of any extra settings
			break;
		case 4:
			reg_int.uINTLMODE_PORT1_EN = 1;
			SET_AND_ENABLE_RSD(I2S_MIC3_RSD1_CTRL(regBase), dbg_mode, pI2sConfig->Mic3LRSw);
			break;
		case 6:
			reg_int.uINTLMODE_PORT1_EN = 1;
			reg_int.uINTLMODE_PORT2_EN = 1;
			SET_AND_ENABLE_RSD(I2S_MIC3_RSD1_CTRL(regBase), dbg_mode, pI2sConfig->Mic3LRSw);
			SET_AND_ENABLE_RSD(I2S_MIC3_RSD2_CTRL(regBase), dbg_mode, pI2sConfig->Mic3LRSw);
			break;
		case 8:
			reg_int.uINTLMODE_PORT1_EN = 1;
			reg_int.uINTLMODE_PORT2_EN = 1;
			reg_int.uINTLMODE_PORT3_EN = 1;
			SET_AND_ENABLE_RSD(I2S_MIC3_RSD1_CTRL(regBase), dbg_mode, pI2sConfig->Mic3LRSw);
			SET_AND_ENABLE_RSD(I2S_MIC3_RSD2_CTRL(regBase), dbg_mode, pI2sConfig->Mic3LRSw);
			SET_AND_ENABLE_RSD(I2S_MIC3_RSD3_CTRL(regBase), dbg_mode, pI2sConfig->Mic3LRSw);
			break;
		default:
			HRX_LOG(AIP_ERROR, "Invalid NumofChannel\n");
			break;
		}
		BFM_HOST_Bus_Write32(I2S_MIC3_INTLMODE(regBase), (reg_int.u32));
	}

	i2s_rx_mic3_enable(regBase);
}

void i2s_rx_mic3_clock_gate_enable(int enable)
{
	T32AIO_CLK_GATE_EN reg;

	BFM_HOST_Bus_Read32(I2S_CLK_GATE_EN, &(reg.u32));

	reg.uCLK_GATE_EN_MIC3 = enable&0x1;

	BFM_HOST_Bus_Write32(I2S_CLK_GATE_EN, (reg.u32));
}
