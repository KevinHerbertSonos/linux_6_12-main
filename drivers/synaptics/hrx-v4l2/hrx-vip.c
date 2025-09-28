// SPDX-License-Identifier: GPL-2.0
/* Copyright 2017 Synopsys Inc. */
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-vip.h"
#include "hrx-reg.h"
#include "hrx-vip-scl.h"
#include "hrx-video.h"
#include "hrx-vip-isr.h"

//#define dhub2d_channel_clear_seq tz_dhub2d_channel_clear_seq
//#define dhub2d_channel_start_seq tz_dhub2d_channel_start_seq
#define dhub2nd_channel_clear_seq wrap_dhub2nd_channel_clear_seq
#define dhub2nd_channel_start_seq wrap_dhub2nd_channel_start_seq
#define BCM_SCHED_PushCmd wrap_BCM_SCHED_PushCmd
#define BCM_SCHED_SetMux wrap_BCM_SCHED_SetMux

/**********************************************************************
 *VOP Coefficients and Offsets for AHD/ASD/HDMI/TTL24/TTL30 Channel CSC           *
 **********************************************************************
 */
const u32 vip_csc_coeff[VIP_CSC_MAX_MODES][VIP_CSC_MAX_COEFF_INDEX - 1] = {
	/*  C0       C1      C2      C3      C4      C5      C6      C7      C8   */
	{	0x1000, 0,       0,       0,       0x1000,  0,       0,       0,       0x1000 }, //Bypass Mode
	{	0x1050, 0,       0x1D8,   0x101D8, 0x1000,  0x10350, 0x130,   0,       0x1068 }, //601->709
	{	0xFD8,  0,       0x101C8, 0x198,   0x1000,  0x310,   0x10128, 0,       0xFB8  }, //709->601
	{	0x830,  0x10650, 0x101E0, 0x128,   0xB70,   0x368,   0x100C0, 0x10770, 0x830  }, //RGB->YUV(709)
	{	0x1D10, 0x1000,  0,       0x102F0, 0x1000,  0x10758, 0,       0x1000,  0x18A0 }, //YUV(709)->RGB
	{	0x830,  0x10570, 0x102C0, 0x1D0,   0x968,   0x4C8,   0x10150, 0x106D8, 0x830  }, //RGB->YUV(601)
	{	0x1BC0, 0x1000,  0x10008, 0x10560, 0x1000,  0x10B30, 0,       0x1000,  0x15F0 }, //YUV(601)->RGB
	{	0x707,  0x1056B, 0x1019C, 0xFE,   0x9D4,   0x2EC,   0x100A5, 0x10662, 0x707  }, //sRGB->YUV(709) RGB709(full)  ->YUV709(narrow)
	{	0x21CC, 0x12A1,  0,       0x10369, 0x12A1,  0x10887, 0,       0x12A1,  0x1CAF }, //YUV(709)->sRGB YUV709(narrow)->RGB709(full)
	{	0x708,  0x104A8, 0x10260, 0x190,   0x810,   0x420,   0x10128, 0x105E0, 0x708  }, //sRGB->YUV(601)
	{	0x2040, 0x12A0,  0,       0x10648, 0x12A0,  0x10D00, 0,       0x12A0,  0x1988 }, //YUV(601)->sRGB
	{	0,      0,       0x1000,  0,       0x1000,  0,       0x1000,  0,       0      },     // U <-> V swap mode
};

const u32 vip_csc_offset[VIP_CSC_MAX_MODES][VIP_CSC_MAX_OFFSET_INDEX] = {
	/* A0       A1          A2      */
	{ 0,           0,         0        }, //Bypass Mode
	{ 0x808878,    0x14B40,   0x8066C8 }, //601->709
	{ 0x7BB0,      0x812A00,  0x5B30   }, //709->601
	{ 0x40000,     0,         0x40000  }, //RGB->YUV(709)
	{ 0x874278,   0x29140,  0x862A10 }, //YUV(709)->RGB
	{ 0x40000,     0,         0x40000  }, //RGB->YUV(601)
	{ 0x86ED90,    0x423C0,   0x857C50 }, //YUV(601)->RGB
	{ 0x100000,     0x20000,    0x100000  }, //sRGB->YUV(709) RGB709(full)  ->YUV709(narrow) 12 bit
	{ 0xA42090,    0x99C19,   0x9F033B }, //YUV(709)->sRGB YUV709(narrow)->RGB709(full) 12 bit
	{ 0x40000,     0x8000,    0x40000  }, //sRGB->YUV(601)
	{ 0x88AC88,    0x44320,   0x86FD20 }, //YUV(601)->sRGB
	{ 0,           0,         0        }, //U <-> V swap Mode
};

/* set CSC coefficients */
static int vip_csc_set_coeff(struct syna_hrx_v4l2_dev *hrx_dev, unsigned int csc_mode)
{
	u32 RegAddr;
	u32 LowBits;
	//u32 HighBits;
	int Count;
	T32HDMIRX_PIPE_BYPASS_CTRL bypass_ctrl;

	if ((csc_mode < VIP_CSC_BYPASS_MODE) || (csc_mode > VIP_CSC_MAX_MODES))
		return -1;

	/*no need to change CSC coeffs and offsets*/
	if (csc_mode == VIP_CSC_MODES_NO_CHANGE)
		return 0;

	bypass_ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_BYPASS_CTRL);

	if (csc_mode == VIP_CSC_BYPASS_MODE) {
		bypass_ctrl.uBYPASS_CTRL_csc_bypass = 1;
	} else {
		bypass_ctrl.uBYPASS_CTRL_csc_bypass = 0;
		/*get register address of VOP CSC coefficient 0 register*/
		RegAddr = RA_HDMIRX_PIPE_VIP_CSC + RA_CSC_C17O24_CFG0;

		/*put all CSC coeffs into local buffer and BCM buffer*/
		for (Count = 0; Count < VIP_CSC_MAX_COEFF_INDEX-1; Count++) {
			/*get 32 bits*/
			LowBits = vip_csc_coeff[csc_mode][Count];
			//HighBits = vip_csc_coeff[csc_mode][Count+1];

			vip_reg_write(hrx_dev, RegAddr, LowBits);

		/*update address*/
			RegAddr += 4;
		}
		/*put all CSC coeffs into local buffer and BCM buffer*/
		for (Count = 0; Count < VIP_CSC_MAX_OFFSET_INDEX; Count++) {
			/*get 32 bits*/
			LowBits = vip_csc_offset[csc_mode][Count];

			vip_reg_write(hrx_dev, RegAddr, LowBits);

			RegAddr += 4;
		}
	}

	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_BYPASS_CTRL, bypass_ctrl.u32);
	return 0;
}


/* program CSC registers with default values */
static int vip_csc_reset(struct syna_hrx_v4l2_dev *hrx_dev)
{
	/* default CSC register programming: bypass */
	vip_csc_set_coeff(hrx_dev, VIP_CSC_BYPASS_MODE);
	return 0;
}

static int vip_fetg_ups_dns_reset(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return 0;
}

static int vip_dither_reset(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return 0;
}

//VIP_DITHER_SetMode(hrx_dev, VIP_DITHER_ROUND);
static void vip_dither_set_mode(struct syna_hrx_v4l2_dev *hrx_dev, VIP_DITHER_MODE mode)
{
	T32DITHER_CFG0 reg;
	u32 uiRegAddr;

	uiRegAddr = RA_HDMIRX_PIPE_VIP_DITHER + RA_DITHER_CFG0;
	reg.u32 = vip_reg_read(hrx_dev, uiRegAddr);

	if (mode == VIP_DITHER_TRUNCATE)
		reg.uCFG0_mode = 0;
	else if (mode == VIP_DITHER_ROUND)
		reg.uCFG0_mode = 0x1;
	else if (mode == VIP_DITHER_1D_ERR_DIFFUSION_TRUNCATE)
		reg.uCFG0_mode = 0x2;
	else if (mode == VIP_DITHER_1D_ERR_DIFFUSION_ROUND)
		reg.uCFG0_mode = 0x3;
	else
		reg.uCFG0_mode = 0x2; //Dafault value from Reg. spec

	vip_reg_write(hrx_dev, uiRegAddr, reg.u32);
}
//VIP_DITHER_SetYCMode(hrx_dev, VIP_DITHER_YCMODE_422_420);
static void vip_dither_set_ycmode(struct syna_hrx_v4l2_dev *hrx_dev, VIP_DITHER_YCMODE ycmode)
{
	T32DITHER_CFG0 reg;
	u32 uiRegAddr;

	uiRegAddr = RA_HDMIRX_PIPE_VIP_DITHER + RA_DITHER_CFG0;

	reg.u32 = vip_reg_read(hrx_dev, uiRegAddr);

	if (ycmode == VIP_DITHER_YCMODE_422_420)
		reg.uCFG0_ycmode = 0x1;
	else if (ycmode == VIP_DITHER_YCMODE_444)
		reg.uCFG0_ycmode = 0x0;

	vip_reg_write(hrx_dev, uiRegAddr, reg.u32);
}

// /VIP_DITHER_SetCtrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
static void vip_dither_set_ctrl(struct syna_hrx_v4l2_dev *hrx_dev,
	unsigned int input_bit_depth, unsigned int output_bit_depth)
{
	T32DITHER_CFG0 reg;
	u32 uiRegAddr, uiRegAddr_bypass_ctrl;
	T32HDMIRX_PIPE_BYPASS_CTRL bypass_ctrl;

	uiRegAddr = RA_HDMIRX_PIPE_VIP_DITHER + RA_DITHER_CFG0;
	reg.u32 = vip_reg_read(hrx_dev, uiRegAddr);

	uiRegAddr_bypass_ctrl = RA_HDMIRX_PIPE_BYPASS_CTRL;
	bypass_ctrl.u32 = vip_reg_read(hrx_dev, uiRegAddr_bypass_ctrl);

	if (output_bit_depth == input_bit_depth) {
		bypass_ctrl.uBYPASS_CTRL_dither_bypass = 1;
		reg.uCFG0_ctrl = 0x2;
	} else {
		bypass_ctrl.uBYPASS_CTRL_dither_bypass = 0;

		if (output_bit_depth == VIP_OUTPUT_8BIT) {
			if (input_bit_depth == VIP_INPUT_10BIT)
				reg.uCFG0_ctrl = 0x0; //for 2 bit
			else if (input_bit_depth == VIP_INPUT_12BIT)
				reg.uCFG0_ctrl = 0x1; //for 4 bit
		} else if (output_bit_depth == VIP_OUTPUT_10BIT) {
			if (input_bit_depth == VIP_INPUT_12BIT)
				reg.uCFG0_ctrl = 0x0; // for 2 bit
		} else {
			//Dither OFF
			reg.uCFG0_ctrl = 0x2;
		}
	}
	vip_reg_write(hrx_dev, uiRegAddr_bypass_ctrl, bypass_ctrl.u32);
	vip_reg_write(hrx_dev, uiRegAddr, reg.u32);
}

//VIP_UPS_EnableUPS(hrx_dev, 1, VIP_OUTPUT_8BIT);
static void vip_ups_enableups(struct syna_hrx_v4l2_dev *hrx_dev, int enable, int bitdepth)
{
	T32UPS_420_422_HDMI_12b_CFG0 reg0;
	T32UPS_420_422_HDMI_12b_CFG5 reg5;
	T32HDMIRX_PIPE_BYPASS_CTRL bypass_ctrl;

	bypass_ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_BYPASS_CTRL);

	if (enable) {
		// enable UPS
		reg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_VIP_UPS420_HDMI+RA_UPS_420_422_HDMI_12b_CFG0);
		reg5.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_VIP_UPS420_HDMI+RA_UPS_420_422_HDMI_12b_CFG5);

		if (bitdepth == VIP_OUTPUT_8BIT)
			reg0.uCFG0_enable = 1;
		else
			reg0.uCFG0_enable = 0;

		bypass_ctrl.uBYPASS_CTRL_ups420_bypass = 0;
		reg5.uCFG5_auto_pixcnt = 1;
		reg0.uCFG0_enable = 1;

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_UPS420_HDMI+RA_UPS_420_422_HDMI_12b_CFG0, reg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_UPS420_HDMI+RA_UPS_420_422_HDMI_12b_CFG5, reg5.u32);
	} else {
		// disable UPS
		bypass_ctrl.uBYPASS_CTRL_ups420_bypass = 1;
	}

	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_BYPASS_CTRL, bypass_ctrl.u32);
}
// /VIP_UPS_EnableUPS420(hrx_dev, 1);
static void vip_ups_enableups420(struct syna_hrx_v4l2_dev *hrx_dev, int enable)
{
	T32UPS_420_422_HDMI_12b_CFG0 reg0;

	reg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_VIP_UPS420_HDMI+RA_UPS_420_422_HDMI_12b_CFG0);

	if (enable)
		reg0.uCFG0_hdmi_to_sp_en = 1;
	else
		reg0.uCFG0_hdmi_to_sp_en = 0;

	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_UPS420_HDMI+RA_UPS_420_422_HDMI_12b_CFG0, reg0.u32);
}

//VIP_DNS_EnableDNS(hrx_dev, 1, VIP_OUTPUT_8BIT);
static void vip_dns_enabledns(struct syna_hrx_v4l2_dev *hrx_dev, int enable, int bitdepth)
{
	T32HDMIRX_PIPE_BYPASS_CTRL bypass_ctrl;

	bypass_ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_BYPASS_CTRL);

	if (enable)
		bypass_ctrl.uBYPASS_CTRL_dns444_422_bypass = 0;
	else
		bypass_ctrl.uBYPASS_CTRL_dns444_422_bypass = 1;

	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_BYPASS_CTRL, bypass_ctrl.u32);
}

static void vip_set_tgparams(struct syna_hrx_v4l2_dev *hrx_dev)
{
	VIP_TG_PARAMS tg_param;
	int goutputmode, input_mode;
	T32TG_SIZE tg_size;
	T32TG_HB tg_hb;
	T32TG_HB_CR tg_hb_cr;
	T32TG_HB_CR2 tg_hb_cr2;
	T32TG_VB0 tg_vb0;
	T32TG_VB0_CR tg_vb0_cr;
	T32TG_VB0_CR2 tg_vb0_cr2;

	goutputmode = hrx_dev->vip_omode;
	input_mode = hrx_dev->vip_imode;

	hrx_get_tg_params(hrx_dev, &tg_param);

	tg_param.width = tg_param.end_x - tg_param.start_x;
	tg_param.height = tg_param.end_y - tg_param.start_y;

	tg_param.pixel_freq = tg_param.htotal * tg_param.vtotal * tg_param.refresh_rate;

	HRX_LOG(VIP_DEBUG, "%s:  %d %d %d %d %d %d %d %d %d %d %d\n", __func__, tg_param.width,
					tg_param.height,
					tg_param.htotal,
					tg_param.vtotal,
					tg_param.start_x,
					tg_param.end_x,
					tg_param.start_y,
					tg_param.end_y,
					tg_param.vsamp,
					tg_param.pixel_freq,
					tg_param.refresh_rate);

	hrx_dev->vip_hwidth = tg_param.width;
	hrx_dev->vip_vheight = tg_param.height;
	hrx_dev->vip_htotal = tg_param.htotal;
	hrx_dev->vip_vtotal = tg_param.vtotal;
	//hrx_dev->vip_vsw = tg_param.vsw;
	//hrx_dev->vip_hsw = tg_param.hsw;
	//hrx_dev->hvsync_negative = tg_param.hvsync_negative;
	//hrx_dev->ref_rate = tg_param.refresh_rate
	hrx_dev->vip_field_flag = tg_param.field_flag = tg_param.field_flag;
	//hrx_dev->mode_3d = tg_param.mode_3d =  tg_param.mode_3d;
	hrx_dev->vip_sync_type = tg_param.sync_type = tg_param.sync_type;

	/* VIP_TG_SetTGParams */

	tg_size.uSIZE_Y = tg_param.height+3;
	tg_size.uSIZE_X = tg_param.width+20; /* Changed from 20 to 40 to fix 4K SCL issues */
	tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = tg_param.width+8;
	tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;
	tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = tg_param.height+3;
	tg_vb0.uVB0_BE = tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 2;
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_TG + RA_TG_SIZE, tg_size.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_TG + RA_TG_HB, tg_hb.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_TG + RA_TG_HB_CR, tg_hb_cr.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_TG + RA_TG_HB_CR2, tg_hb_cr2.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_TG + RA_TG_VB0, tg_vb0.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_TG + RA_TG_VB0_CR, tg_vb0_cr.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_TG + RA_TG_VB0_CR2, tg_vb0_cr2.u32);

	tg_size.uSIZE_Y = tg_param.height+6;
	tg_size.uSIZE_X = tg_param.width+10;
	tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = tg_param.height+1;
	tg_vb0.uVB0_BE = tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 0;

	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_OTG + RA_TG_SIZE, tg_size.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_OTG + RA_TG_HB, tg_hb.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_OTG + RA_TG_HB_CR, tg_hb_cr.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_OTG + RA_TG_HB_CR2, tg_hb_cr2.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_OTG + RA_TG_VB0, tg_vb0.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_OTG + RA_TG_VB0_CR, tg_vb0_cr.u32);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_OTG + RA_TG_VB0_CR2, tg_vb0_cr2.u32);

	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pix, tg_param.width*tg_param.height);
	if (goutputmode == VIP_OMODE5_8BIT_YCBCR ||
		goutputmode == VIP_OMODE6_10BIT_YCBCR ||
		goutputmode == VIP_OMODE7_12BIT_YCBCR ||
		goutputmode == VIP_OMODE8_8BIT_RGB ||
		goutputmode == VIP_OMODE9_10BIT_RGB ||
		goutputmode == VIP_OMODE10_12BIT_RGB)
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_WriteClient_pix, tg_param.width*tg_param.height);
	else
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_WriteClient_pix, tg_param.width*tg_param.height/2);

	if (input_mode == VIP_IMODE9_8BIT_YUV420 ||
		input_mode == VIP_IMODE10_10BIT_YUV420 ||
		input_mode == VIP_IMODE11_12BIT_YUV420)
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_hdmirxpipe_Inpix, tg_param.width*tg_param.height/2);
	else
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_hdmirxpipe_Inpix, tg_param.width*tg_param.height);

	/*Added for VIP SCL. Recommented value */
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_scl1d_Inpix, tg_param.width*tg_param.height);

	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_sclOFIFO_YInpix, tg_param.width*tg_param.height);
	if (!hrx_dev->vip_enable_scaler && (goutputmode == VIP_OMODE2_8BIT_YUV420 ||
		goutputmode == VIP_OMODE3_10BIT_YUV420))
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_sclOFIFO_UVInpix, tg_param.width*tg_param.height/2);
	else
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_sclOFIFO_UVInpix, tg_param.width*tg_param.height);
}

static void vip_enable_int(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned int reg;
	int goutputmode;
	HDL_semaphore *pSemHandle, *pSemHandle_AG;

	dhub2nd_channel_clear_seq(hrx_dev->vip_dhub2d, hrx_dev->dvi_dmaWID[0]);
	dhub2nd_channel_clear_seq(hrx_dev->vip_dhub2d, hrx_dev->dvi_dmaWID[1]);

	dhub2nd_channel_start_seq(hrx_dev->vip_dhub2d, hrx_dev->dvi_dmaWID[0]);
	dhub2nd_channel_start_seq(hrx_dev->vip_dhub2d, hrx_dev->dvi_dmaWID[1]);

	goutputmode = hrx_dev->vip_omode;

	if (goutputmode == VIP_OMODE2_8BIT_YUV420 ||
		goutputmode == VIP_OMODE3_10BIT_YUV420) {
		if (hrx_dev->vip_enable_scaler) {
			/* Y-TG is used for YUV420 VIP output frames */
			BCM_SCHED_SetMux(BCM_SCHED_Q5, 23);
		} else {
			/* VIP Input-TG is used for YUV420 VIP output frames */
			BCM_SCHED_SetMux(BCM_SCHED_Q5, 25);
		}
	} else {
		/* VIP-OTG is used for YUV422/YUV444 VIP output frames */
		BCM_SCHED_SetMux(BCM_SCHED_Q5, 13);
	}

	/* Enable the Vsync Interrupt */
	reg = glb_reg_read(MEMMAP_AVIO_REG_BASE +
			 AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE + HDMI_AVPUNIT_1_INT_MASK_N);
	reg |=  HDMI_AVPUNIT_1_INT_MASK_N_DEFRAMER_VSYNC_MASK;
	glb_reg_write(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE +
			HDMI_AVPUNIT_1_INT_MASK_N, reg);

	//GLB_REG_WRITE32(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE +
	//            HDMI_AVPUNIT_1_INT_CLEAR, 1);

	/* set signal status to unstable on start */
	hrx_dev->vip_signal_status = VIP_SIGNAL_UNSTABLE;

	/* no DMA command pending */
	hrx_dev->vip_dma_cmd_issued = 0;
	/* current frame descriptor is NULL */
	//hrx_dev->vip_curr_frame_descr = NULL;

	/* Reset the first frame status */
	//hrx_dev->bFirstFrame = 0;
	/* VIP_ISR_Enable*/

	if (goutputmode == VIP_OMODE2_8BIT_YUV420 ||
		goutputmode == VIP_OMODE3_10BIT_YUV420 ||
		goutputmode == VIP_OMODE4_12BIT_YUV420) {
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_INTR_EN,
			hrx_dev->vip_enable_scaler ? 0x073 : 0x043);
		if (hrx_dev->vip_enable_scaler)
			vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIPSCLTOP +
			RA_VIPSCLTOP_INTR_EN, 0x03);
	} else {
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_INTR_EN, 0x3);
	}

	pSemHandle = dhub_semaphore(&(hrx_dev->vip_dhub2d->dhub));
	pSemHandle_AG = dhub_semaphore(&(hrx_dev->vip_ag_dhub2d->dhub));

	/* configure and enable DVI VDE interrupt */
	semaphore_cfg(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr7, 1, 0);
	semaphore_clr_full(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr7);
	semaphore_intr_enable(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr7,
		0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/,
		0/*cpu id*/);


	semaphore_cfg(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr9, 1, 0);
	semaphore_clr_full(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr9);
	semaphore_intr_enable(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr9,
		0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/,
		0/*cpu id*/);

	semaphore_cfg(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr10, 1, 0);
	semaphore_clr_full(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr10);
	semaphore_intr_enable(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr10,
		0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/,
		0/*cpu id*/);

	semaphore_cfg(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr11, 1, 0);
	semaphore_clr_full(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr11);
	semaphore_intr_enable(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr11,
		0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/,
		0/*cpu id*/);

	hrx_dev->vip_status = VIP_STATUS_START;
}

void vip_init(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 val;
	BCMBUF bcm_buf;
	int cnt = 0;

	val = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);
	hrx_dev->ui_pipe_ctrl = val;

	hrx_dev->vip_dhub2d = Dhub_GetDhub2dHandle_ByDhubId(DHUB_ID_VPP_DHUB);
	hrx_dev->vip_ag_dhub2d = Dhub_GetDhub2dHandle_ByDhubId(DHUB_ID_AG_DHUB);

	// VIP video frame write client
	hrx_dev->dvi_dmaWID[0] = avioDhubChMap_vpp128b_1DSCL_W0;
	// VIP pip video frame write client
	hrx_dev->dvi_dmaWID[1] = avioDhubChMap_vpp128b_1DSCL_W1;

	/* create VBI BCM buffer (dual-buffer for VIP register programming) */
	if (bcmbuf_create(&bcm_buf, VIP_BCM_BUFFER_SIZE, hrx_dev->mem_list) != 0)
		HRX_LOG(VIP_ERROR, "vbi0 bcmbuf_create failed\n");
	hrx_dev->vbi_bcm_buf[0] = bcm_buf;

	if (bcmbuf_create(&bcm_buf, VIP_BCM_BUFFER_SIZE, hrx_dev->mem_list) != 0)
		HRX_LOG(VIP_ERROR, "vbi1 bcmbuf_create failed\n");

	hrx_dev->vbi_bcm_buf[1] = bcm_buf;

	hrx_dev->dhub_bcm_mem_handle[0].size = VIP_DHUB_CFGQ_SIZE;
	hrx_dev->dhub_bcm_mem_handle[1].size = VIP_DHUB_CFGQ_SIZE;
	hrx_dev->dhub_dma_mem_handle[0].size = VIP_DHUB_CFGQ_SIZE;
	hrx_dev->dhub_dma_mem_handle[1].size = VIP_DHUB_CFGQ_SIZE;

	/* create a VIP DHUB BCM channel CFGQ */
	if (create_bcmbuf_cfgq(&hrx_dev->dhub_bcm_cfgQ[0], &hrx_dev->dhub_bcm_mem_handle[0], hrx_dev->mem_list) != 0)
		HRX_LOG(VIP_ERROR, "bcmbuf_cfgq_create failed\n");
	if (create_bcmbuf_cfgq(&hrx_dev->dhub_bcm_cfgQ[1], &hrx_dev->dhub_bcm_mem_handle[1], hrx_dev->mem_list) != 0)
		HRX_LOG(VIP_ERROR, "bcmbuf_cfgq_create failed\n");
	/* create a VIP DHUB DMA channel CFGQ */
	if (create_bcmbuf_cfgq(&hrx_dev->dhub_dma_cfgQ[0], &hrx_dev->dhub_dma_mem_handle[0], hrx_dev->mem_list) != 0)
		HRX_LOG(VIP_ERROR, "bcmbuf_cfgq_create failed\n");
	if (create_bcmbuf_cfgq(&hrx_dev->dhub_dma_cfgQ[1], &hrx_dev->dhub_dma_mem_handle[1], hrx_dev->mem_list) != 0)
		HRX_LOG(VIP_ERROR, "bcmbuf_cfgq_create failed\n");

	/* TODO: Alloc memory for SCL */
	for (cnt = VIP_FIRST_SCALAR_COEFF; cnt < VIP_MAX_NUM_PREDEFINED_COEFFS; cnt++) {
		int sclLayer;
		int numOfSclLayers = 0;

		if (cnt >= VIP_SCALAR_COEFF_MAIN_MIN  &&
			cnt <= VIP_SCALAR_COEFF_MAIN_MAX) {
			numOfSclLayers = VIP_FRC_SCL_MAIN_LAY_MAX;
		}

		if (!hrx_dev->scl_coeffs[cnt]) {
			hrx_dev->scl_coeffs_mem_handle[cnt].size = VIP_FRC_COEFFTAB_BCMBUF_SIZE * numOfSclLayers * 32;
			VPP_MEM_AllocateMemory(hrx_dev->mem_list, VPP_MEM_TYPE_DMA,
				&hrx_dev->scl_coeffs_mem_handle[cnt], 0);
			hrx_dev->scl_coeffs[cnt] = hrx_dev->scl_coeffs_mem_handle[cnt].k_addr;
		}

		for (sclLayer = 0; sclLayer < numOfSclLayers; sclLayer++) {
			if (!hrx_dev->cust_scl_coeffs[sclLayer][cnt]) {
				hrx_dev->cust_scl_coeffs_mem_handle[sclLayer][cnt].size = VIP_FRC_SCL_COEFF_TAB_SIZE * 32;
				VPP_MEM_AllocateMemory(hrx_dev->mem_list, VPP_MEM_TYPE_DMA,
					&hrx_dev->cust_scl_coeffs_mem_handle[sclLayer][cnt], 0);
				hrx_dev->cust_scl_coeffs[sclLayer][cnt] = hrx_dev->cust_scl_coeffs_mem_handle[sclLayer][cnt].k_addr;
			}
		}
	}
	/* End to allocate memory for SCL Coeff */

	/* TODO: Create ISR Task waiting on sem */

	/* Extra start code */
	vip_reset(hrx_dev);
}

static int syna_hrx_get_inputMode(struct syna_hrx_v4l2_dev *hrx_dev, ENUM_VIP_IMODE *pMode)
{
	int r = 0;

	ENUM_VIP_IMODE inputMode = VIP_IMODE6_8BIT_RGB;

	switch (hrx_dev->video_params.HrxIpColorFormatType) {
	case HRX_HDMI_RX_COL_FMT_RGB:
		if (hrx_dev->video_params.HrxIpBitDepthType == HRX_HDMI_RX_COL_DEPTH_8BIT)
			inputMode = VIP_IMODE6_8BIT_RGB;
		else if (hrx_dev->video_params.HrxIpBitDepthType == HRX_HDMI_RX_COL_DEPTH_10BIT)
			inputMode = VIP_IMODE7_10BIT_RGB;
		else if (hrx_dev->video_params.HrxIpBitDepthType == HRX_HDMI_RX_COL_DEPTH_12BIT)
			inputMode = VIP_IMODE8_12BIT_RGB;
		else
			HRX_LOG(VIP_ERROR, "unsupported color depth: 0x%x\n", hrx_dev->video_params.HrxIpBitDepthType);
		break;
	case HRX_HDMI_RX_COL_FMT_YUV444:
		if (hrx_dev->video_params.HrxIpBitDepthType == HRX_HDMI_RX_COL_DEPTH_8BIT)
			inputMode = VIP_IMODE0_8BIT_YCBCR;
		else if (hrx_dev->video_params.HrxIpBitDepthType == HRX_HDMI_RX_COL_DEPTH_10BIT)
			inputMode = VIP_IMODE1_10BIT_YCBCR;
		else if (hrx_dev->video_params.HrxIpBitDepthType == HRX_HDMI_RX_COL_DEPTH_12BIT)
			inputMode = VIP_IMODE2_12BIT_YCBCR;
		else if (hrx_dev->video_params.HrxIpBitDepthType == HRX_HDMI_RX_COL_DEPTH_16BIT) {
			HRX_LOG(VIP_ERROR, "not support HRX_HDMI_RX_COL_DEPTH_16BIT\n");
		} else {
			HRX_LOG(VIP_ERROR, "unsupported color depth: 0x%x\n", hrx_dev->video_params.HrxIpBitDepthType);
		}
		break;
	case HRX_HDMI_RX_COL_FMT_YUV422:
		inputMode = VIP_IMODE3_8BIT_YC;
		break;
	case HRX_HDMI_RX_COL_FMT_YUV420:
		if (hrx_dev->video_params.HrxIpBitDepthType == HRX_HDMI_RX_COL_DEPTH_8BIT)
			inputMode = VIP_IMODE9_8BIT_YUV420;
		else if (hrx_dev->video_params.HrxIpBitDepthType == HRX_HDMI_RX_COL_DEPTH_10BIT)
			inputMode = VIP_IMODE10_10BIT_YUV420;
		else if (hrx_dev->video_params.HrxIpBitDepthType == HRX_HDMI_RX_COL_DEPTH_12BIT)
			inputMode = VIP_IMODE11_12BIT_YUV420;
		else
			HRX_LOG(VIP_ERROR, "unsupported color depth: 0x%x\n", hrx_dev->video_params.HrxIpBitDepthType);
		break;
	default:
		r = -1;
		HRX_LOG(VIP_ERROR, "unsupported color format: 0x%x\n", hrx_dev->video_params.HrxIpColorFormatType);
		break;
	}

	*pMode = inputMode;

	return r;
}

static bool is_valid_input_output_mode(ENUM_VIP_IMODE imode, ENUM_VIP_OMODE omode)
{
	bool ret = true;

	switch (imode) {
	case VIP_IMODE0_8BIT_YCBCR:
	case VIP_IMODE1_10BIT_YCBCR:
	case VIP_IMODE2_12BIT_YCBCR:
	case VIP_IMODE6_8BIT_RGB:
	case VIP_IMODE7_10BIT_RGB:
	case VIP_IMODE8_12BIT_RGB:
		break;
	case VIP_IMODE3_8BIT_YC:
	case VIP_IMODE4_10BIT_YC:
	case VIP_IMODE5_12BIT_YC:
		{
			if ((omode == VIP_OMODE5_8BIT_YCBCR) || (omode == VIP_OMODE6_10BIT_YCBCR) || (omode == VIP_OMODE7_12BIT_YCBCR))
				ret = false;
		}
	break;
	case VIP_IMODE9_8BIT_YUV420:
	case VIP_IMODE10_10BIT_YUV420:
	case VIP_IMODE11_12BIT_YUV420:
	{
		if ((omode == VIP_OMODE5_8BIT_YCBCR) || (omode == VIP_OMODE6_10BIT_YCBCR) || (omode == VIP_OMODE7_12BIT_YCBCR) ||
			(omode == VIP_OMODE0_8BIT_YC) || (omode == VIP_OMODE1_10BIT_YC) || (omode == VIP_OMODE11_12BIT_YC))
			ret = false;
	}
	break;
	default:
		ret = false;
		break;
	}

	return ret;
}

void vip_start(struct syna_hrx_v4l2_dev *hrx_dev)
{
	mutex_lock(&hrx_dev->vip_mutex);
	hrx_dev->vip_intr_num = 0;
	hrx_dev->vip_first_frame = 0;

	syna_hrx_get_inputMode(hrx_dev, &hrx_dev->vip_imode);
	if (!is_valid_input_output_mode(hrx_dev->vip_imode, hrx_dev->vip_omode)) {
		HRX_LOG(VIP_ERROR, "imode : %d and omode :%d are not compatible\n", hrx_dev->vip_imode, hrx_dev->vip_omode);
		goto EXIT;
	}
	vip_config(hrx_dev);

	vip_set_tgparams(hrx_dev);
#ifdef GET_EARLY_BUFFER
	hrx_dev->vip_curr_free_buf = syna_hrx_get_free_buf(hrx_dev);

	if (!hrx_dev->vip_curr_free_buf) {
		HRX_LOG(VIP_ERROR, "Can't get free buffer\n");
	} else {
		handle = syna_hrx_dh_plane_cookie(hrx_dev->vip_curr_free_buf, 0);
		hrx_dev->vip_curr_phys_addr = (u64)syna_hrx_get_frame_phyaddr(handle);
	}
#endif
	/* TODO: check output window */
	//MV_VIPOBJ_SetOutputWin(thisd->hVip, VIP_FRC_SCL_Y, &vipDispWin);

	vip_enable_int(hrx_dev);
	HRX_LOG(VIP_DEBUG, "imode : %d and omode :%d are set\n", hrx_dev->vip_imode, hrx_dev->vip_omode);

EXIT:
	mutex_unlock(&hrx_dev->vip_mutex);
}

void vip_stop(struct syna_hrx_v4l2_dev *hrx_dev)
{
	HDL_semaphore *pSemHandle;
	HDL_semaphore *pSemHandle_AG;

	mutex_lock(&hrx_dev->vip_mutex);
	hrx_dev->vip_status = VIP_STATUS_STOP;
	//vip_stop_isr_task(hrx_dev);
	hrx_dev->vip_intr_num = 0;

	BCM_SCHED_SetMux(BCM_SCHED_Q5, BCM_SCHED_TRIG_NONE);
	/* VIP_ISR_Disable */
	pSemHandle = dhub_semaphore(&(hrx_dev->vip_dhub2d->dhub));
	pSemHandle_AG = dhub_semaphore(&(hrx_dev->vip_ag_dhub2d->dhub));

	semaphore_cfg(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr7, 1, 0);
	semaphore_clr_full(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr7);
	semaphore_intr_enable(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr7,
			0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/,
			0/*cpu id*/);

	semaphore_cfg(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr9, 1, 0);
	semaphore_clr_full(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr9);
	semaphore_intr_enable(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr9,
			0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/,
			0/*cpu id*/);

	semaphore_cfg(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr10, 1, 0);
	semaphore_clr_full(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr10);
	semaphore_intr_enable(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr10,
			0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/,
			0/*cpu id*/);

	semaphore_cfg(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr11, 1, 0);
	semaphore_clr_full(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr11);
	semaphore_intr_enable(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr11,
			0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/,
			0/*cpu id*/);

	/* VIP_ISR_Disable done */

	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_tg_ctrl, 0);
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_tg_ctrl, 2);

	//clear dHub BCM CFGQ
	//VIP_DHUB_CFGQ_Clear(vip_obj, &vip_obj->dhub_bcm_cfgQ[0]);
	//VIP_DHUB_CFGQ_Clear(vip_obj, &vip_obj->dhub_bcm_cfgQ[1]);

	dhub2nd_channel_clear_seq(hrx_dev->vip_dhub2d, hrx_dev->dvi_dmaWID[0]);
	dhub2nd_channel_clear_seq(hrx_dev->vip_dhub2d, hrx_dev->dvi_dmaWID[1]);

	if (vip_frmq_isdirty(&hrx_dev->frmq))
		// a new frame is captured
		vip_frmq_push_commit(&(hrx_dev->frmq));

	if ((hrx_dev->vip_curr_frame_descr != NULL) && (hrx_dev->vip_dma_cmd_issued != 0)) {
		syna_hrx_buf_unused(hrx_dev, (struct vb2_buffer *)hrx_dev->vip_curr_frame_descr);
		hrx_dev->vip_curr_frame_descr = NULL;
	}
		/* set signal status to unstable on start */
	hrx_dev->vip_signal_status = VIP_SIGNAL_UNSTABLE;

	/* no DMA command pending */
	hrx_dev->vip_dma_cmd_issued = 0;

	while (!vip_frmq_isempty(&hrx_dev->frmq)) {
		void *frame_descr;

		vip_frmq_pop(&hrx_dev->frmq, &frame_descr);
		syna_hrx_buf_unused(hrx_dev, (struct vb2_buffer *)frame_descr);
	}
	mutex_unlock(&hrx_dev->vip_mutex);
}

void vip_reset(struct syna_hrx_v4l2_dev *hrx_dev)
{
	// reset VIP object variable
	hrx_dev->vip_status = VIP_STATUS_INACTIVE;
	hrx_dev->vip_imode = VIP_IMODE_INVALID;
	hrx_dev->vip_omode = VIP_OMODE_INVALID;

	hrx_dev->vip_signal_status = VIP_SIGNAL_UNSTABLE;
	hrx_dev->vip_dma_cmd_issued = 0;
	hrx_dev->vip_curr_frame_descr = NULL;

	hrx_dev->vip_field_flag = 0;
	hrx_dev->vip_top = 0;
	hrx_dev->ui_tunnel_mode = 0;

	/* reset VBI BCM buffer */
	bcmbuf_reset(&(hrx_dev->vbi_bcm_buf[0]));
	bcmbuf_reset(&(hrx_dev->vbi_bcm_buf[1]));
	hrx_dev->pcurr_vbi_bcm_buf = &hrx_dev->vbi_bcm_buf[0];

	/* reset DHUB BCM channel CFGQs */
	bcmbuf_DHUB_CFGQ_Reset(&(hrx_dev->dhub_bcm_cfgQ[0]));
	bcmbuf_DHUB_CFGQ_Reset(&(hrx_dev->dhub_bcm_cfgQ[1]));
	hrx_dev->pcurr_dhub_bcm_cfgQ = &hrx_dev->dhub_bcm_cfgQ[0];
	/* reset DHUB DMA channel CFGQs */
	bcmbuf_DHUB_CFGQ_Reset(&(hrx_dev->dhub_dma_cfgQ[0]));
	bcmbuf_DHUB_CFGQ_Reset(&(hrx_dev->dhub_dma_cfgQ[1]));
	hrx_dev->pcurr_dhub_dma_cfgQ = &hrx_dev->dhub_dma_cfgQ[0];

	//hrx_dev->DVI_dma_cmdQ = 0;

	/* reset frame descriptor queues */
	vip_frmq_reset(&(hrx_dev->frmq));

	vip_csc_reset(hrx_dev);
	vip_fetg_ups_dns_reset(hrx_dev);
	vip_dither_reset(hrx_dev);
	vip_scl_reset(hrx_dev);
}

void vip_config(struct syna_hrx_v4l2_dev *hrx_dev)
{
	//T32HDMIRX_PIPE_CFG4 cfg4;
	T32VIPSCLTOP_CTRL5 ctrl5;
	T32HDMIRX_PIPE_CTRL ctrl;
	T32HDMIRX_PIPE_BYPASS_CTRL bypass_ctrl;
	T32avioGbl_SWRST_CTRL stavioGbl_SWRST_CTRL;
	T32ReadClient_NonStdRes NonStdRes;
	u32 addr;
	ENUM_VIP_IMODE input_mode;
	ENUM_VIP_OMODE output_mode;
	// TODO: Check value of g_pck594 g_enscaler
	int g_pck594 = 0;
	int g_enscaler = 1;

	//TODO MUST: Check if required: Clock recovery
	//AVIN_ClockRecoveryReset(thisp);

	input_mode = hrx_dev->vip_imode;
	output_mode = hrx_dev->vip_omode;

	hrx_dev->vip_enable_scaler = g_enscaler;

	/* reset VBI BCM buffer */
	bcmbuf_reset(&(hrx_dev->vbi_bcm_buf[0]));
	bcmbuf_reset(&(hrx_dev->vbi_bcm_buf[1]));
	hrx_dev->pcurr_vbi_bcm_buf = &hrx_dev->vbi_bcm_buf[0];

	/* reset DHUB BCM channel CFGQs */
	bcmbuf_DHUB_CFGQ_Reset(&(hrx_dev->dhub_bcm_cfgQ[0]));
	bcmbuf_DHUB_CFGQ_Reset(&(hrx_dev->dhub_bcm_cfgQ[1]));
	hrx_dev->pcurr_dhub_bcm_cfgQ = &hrx_dev->dhub_bcm_cfgQ[0];
	/* reset DHUB DMA channel CFGQs */
	bcmbuf_DHUB_CFGQ_Reset(&(hrx_dev->dhub_dma_cfgQ[0]));
	bcmbuf_DHUB_CFGQ_Reset(&(hrx_dev->dhub_dma_cfgQ[1]));
	hrx_dev->pcurr_dhub_dma_cfgQ = &hrx_dev->dhub_dma_cfgQ[0];

	//TODO: setup different modules
	/* Reset some modules first: bypass CSC/DITHER; disable FETG/DNS */
	vip_fetg_ups_dns_reset(hrx_dev);

	addr = MEMMAP_AVIO_REG_BASE +
		AVIO_MEMMAP_AVIO_GBL_BASE +
		RA_avioGbl_SWRST_CTRL;
	stavioGbl_SWRST_CTRL.u32 = glb_reg_read(addr);
	stavioGbl_SWRST_CTRL.uSWRST_CTRL_vipPipeSyncRstn = 0;
	glb_reg_write(addr, stavioGbl_SWRST_CTRL.u32);
	stavioGbl_SWRST_CTRL.uSWRST_CTRL_vipPipeSyncRstn = 1;
	glb_reg_write(addr, stavioGbl_SWRST_CTRL.u32);

	addr = MEMMAP_AVIO_REG_BASE +
		AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE +
		RA_HDMIRX_PIPE_CTRL;
	ctrl.u32 = glb_reg_read(addr);
	ctrl.uCTRL_read_sel = 0;
	glb_reg_write(addr, ctrl.u32);
	hrx_dev->ui_pipe_ctrl = ctrl.u32;

	/* Set default value for NonStdRes_enable.
	 * When it is enabled from VPP offline scaler, it is causing VIP TG interrupt issue
	 * after return to Rx
	 */

	addr = RA_HDMIRX_PIPE_VIP_WR + RA_VIP_WriteClient_NonStdRes;
	NonStdRes.u32 = vip_reg_read(hrx_dev, addr);
	NonStdRes.uNonStdRes_enable = 0;
	NonStdRes.uNonStdRes_pixlineTot = 0x64;
	vip_reg_write(hrx_dev, addr, NonStdRes.u32);

	addr = RA_HDMIRX_PIPE_VIPSCLTOP + RA_VIPSCLTOP_CTRL5;
	ctrl5.u32 = vip_reg_read(hrx_dev, addr);

	if (output_mode == VIP_OMODE2_8BIT_YUV420 ||
		output_mode == VIP_OMODE3_10BIT_YUV420 ||
		output_mode == VIP_OMODE4_12BIT_YUV420) {

		ctrl5.uCTRL5_ofifo_sts0_ctrl = 1;
		ctrl5.uCTRL5_ofifo_sts1_ctrl = 1;
		ctrl5.uCTRL5_sclOutrdy_sts0_en = 1;
		ctrl5.uCTRL5_sclOutrdy_sts1_en = 1;
		ctrl5.uCTRL5_sclOfifo_Y_stsCtrl = 0;
		ctrl5.uCTRL5_sclOifo_UV_stsCtrl = 0;
		if (output_mode == VIP_OMODE3_10BIT_YUV420) {
			ctrl5.uCTRL5_chroma_data_sel = 0;
			ctrl5.uCTRL5_luma_data_sel = 0;
		} else {
			ctrl5.uCTRL5_chroma_data_sel = 1;
			ctrl5.uCTRL5_luma_data_sel = 1;
		}
		ctrl5.uCTRL5_idata_frmt_ctrl = 0;
		if (input_mode != VIP_IMODE9_8BIT_YUV420 &&
			input_mode != VIP_IMODE10_10BIT_YUV420 &&
			input_mode != VIP_IMODE11_12BIT_YUV420)
			ctrl5.uCTRL5_hde_msk_en = 1;
		else
			ctrl5.uCTRL5_hde_msk_en = 0;
		ctrl5.uCTRL5_hde_initval0 = 1;
		ctrl5.uCTRL5_hde_initval1 = 1;
	} else {
		ctrl5.uCTRL5_sclOutrdy_sts0_en = 0;
		ctrl5.uCTRL5_sclOutrdy_sts1_en = 0;
		//ctrl5.uCTRL5_ovpscl_ictrl_sel = 1;
		ctrl5.uCTRL5_hde_msk_en = 1;
		ctrl5.uCTRL5_chroma_data_sel = 0;
		ctrl5.uCTRL5_luma_data_sel = 0;
		ctrl5.uCTRL5_hde_initval0 = 0;
		ctrl5.uCTRL5_sclOfifo_Y_stsCtrl = 1;
		ctrl5.uCTRL5_sclOifo_UV_stsCtrl = 1;
	}
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIPSCLTOP + RA_VIPSCLTOP_CTRL5, ctrl5.u32);

	vip_scl_config(hrx_dev);

	bypass_ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_BYPASS_CTRL);
	if (input_mode == VIP_IMODE9_8BIT_YUV420 ||
		input_mode == VIP_IMODE10_10BIT_YUV420 ||
		input_mode == VIP_IMODE11_12BIT_YUV420) {
		bypass_ctrl.uBYPASS_CTRL_scl1d_bypass = 0;
		bypass_ctrl.uBYPASS_CTRL_ups420_bypass = 0;
	} else {
		bypass_ctrl.uBYPASS_CTRL_scl1d_bypass = 0;
		bypass_ctrl.uBYPASS_CTRL_ups420_bypass = 1;
	}
	bypass_ctrl.uBYPASS_CTRL_csc_bypass = 1;
	bypass_ctrl.uBYPASS_CTRL_dns444_422_bypass = 1;
	bypass_ctrl.uBYPASS_CTRL_dither_bypass = 1;

	if (output_mode == VIP_OMODE5_8BIT_YCBCR ||
		output_mode == VIP_OMODE6_10BIT_YCBCR) {
		bypass_ctrl.uBYPASS_CTRL_scl1d_bypass = 1;
	}
	vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_BYPASS_CTRL, bypass_ctrl.u32);

	if (output_mode == VIP_OMODE2_8BIT_YUV420) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CFG01 cfg01;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		cfg01.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG01);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);

		cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		cfg0.uCFG0_write_sel_8b = 0;
		if (g_pck594)
			cfg0.uCFG0_hdmipk_420_sel = 1;
		else
			cfg0.uCFG0_hdmipk_420_sel = 0;
		cfg01.uCFG0_sp_wrbk_8b = 0;

		ctrl.uCTRL_hdmi420sp_wrbk = 1;
		cfg0.uCFG0_CfgSelAlgn = 0x91;

		if (input_mode == VIP_IMODE9_8BIT_YUV420 ||
			input_mode == VIP_IMODE10_10BIT_YUV420 ||
			input_mode == VIP_IMODE11_12BIT_YUV420) {
			cfg0.uCFG0_CfgSel_12_10_8 = 0xC;
			cfg0.uCFG0_fifo_rd_sel = 1;
			if (hrx_dev->vip_enable_scaler)
				ctrl.uCTRL_scl1d_in = 1;
			else
				ctrl.uCTRL_scl1d_in = 0;
		} else {
			cfg0.uCFG0_CfgSel_12_10_8 = 0;
			cfg0.uCFG0_fifo_rd_sel = 0;
			ctrl.uCTRL_scl1d_in = 0;
		}

		hrx_dev->vip_bits_per_pixel = 8;

		if (input_mode == VIP_IMODE9_8BIT_YUV420) {
			vip_ups_enableups(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_ups_enableups420(hrx_dev, 1);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
		} else if (input_mode == VIP_IMODE10_10BIT_YUV420) {
			vip_ups_enableups(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_ups_enableups420(hrx_dev, 1);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_422_420);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
		} else if (input_mode == VIP_IMODE11_12BIT_YUV420) {
			vip_ups_enableups(hrx_dev, 1, VIP_OUTPUT_12BIT);
			vip_ups_enableups420(hrx_dev, 1);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_422_420);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
		} else if (input_mode == VIP_IMODE0_8BIT_YCBCR) {
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;//YVU
			cfg0.uCFG0_hdmi_pk_8b_10b = 0;
		} else if (input_mode == VIP_IMODE1_10BIT_YCBCR) {
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;//YVU
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
		} else if (input_mode == VIP_IMODE2_12BIT_YCBCR) {
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;//YVU
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		} else if (input_mode == VIP_IMODE3_8BIT_YC) {
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 0;
		} else if (input_mode == VIP_IMODE4_10BIT_YC) {
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_422_420);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
		} else if (input_mode == VIP_IMODE5_12BIT_YC) {
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_422_420);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		} else if (input_mode == VIP_IMODE6_8BIT_RGB) {
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 0;
		} else if (input_mode == VIP_IMODE7_10BIT_RGB) {
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
		} else if (input_mode == VIP_IMODE8_12BIT_RGB) {
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		} else {
		}
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG01, cfg01.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 2);
		if (hrx_dev->vip_enable_scaler)
			vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_WriteClient_pack, 3);
		else /* Scaler bypass case */
			vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_WriteClient_pack, 0);
	} else if (output_mode == VIP_OMODE3_10BIT_YUV420) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CFG01 cfg01;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		cfg01.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG01);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);

		cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		cfg0.uCFG0_write_sel_8b = 1;
		if (g_pck594)
			cfg0.uCFG0_hdmipk_420_sel = 1;
		else
			cfg0.uCFG0_hdmipk_420_sel = 0;
		cfg01.uCFG0_sp_wrbk_8b = 1;

		ctrl.uCTRL_hdmi420sp_wrbk = 1;
		cfg0.uCFG0_CfgSelAlgn = 0x91;

		if (input_mode == VIP_IMODE9_8BIT_YUV420 ||
			input_mode == VIP_IMODE10_10BIT_YUV420 ||
			input_mode == VIP_IMODE11_12BIT_YUV420) {
			cfg0.uCFG0_CfgSel_12_10_8 = 0xC;
			cfg0.uCFG0_fifo_rd_sel = 1;
			ctrl.uCTRL_scl1d_in = 1;
		} else {
			cfg0.uCFG0_CfgSel_12_10_8 = 0;
			cfg0.uCFG0_fifo_rd_sel = 0;
			ctrl.uCTRL_scl1d_in = 0;
		}

		hrx_dev->vip_bits_per_pixel = 10;

		if (input_mode == VIP_IMODE10_10BIT_YUV420) {
			vip_ups_enableups(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_ups_enableups420(hrx_dev, 1);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
		} else if (input_mode == VIP_IMODE11_12BIT_YUV420) {
			vip_ups_enableups(hrx_dev, 1, VIP_OUTPUT_12BIT);
			vip_ups_enableups420(hrx_dev, 1);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_422_420);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
		} else if (input_mode == VIP_IMODE1_10BIT_YCBCR) {
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
		} else if (input_mode == VIP_IMODE2_12BIT_YCBCR) {
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		} else if (input_mode == VIP_IMODE4_10BIT_YC) {
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
		} else if (input_mode == VIP_IMODE5_12BIT_YC) {
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_422_420);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		} else if (input_mode == VIP_IMODE7_10BIT_RGB) {
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
		} else if (input_mode == VIP_IMODE8_12BIT_RGB) {
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		} else {
		}
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG01, cfg01.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 3);
		if (hrx_dev->vip_enable_scaler)
			vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_WriteClient_pack, 4);
		else /* Scaler bypass case */
			vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_WriteClient_pack, 1);
	} else if (output_mode == VIP_OMODE4_12BIT_YUV420) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CFG01 cfg01;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		cfg01.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG01);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);

		cfg0.uCFG0_CfgSel_12_10_8 = 0xC;
		cfg0.uCFG0_fifo_rd_sel = 1;
		cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		cfg0.uCFG0_write_sel_8b = 1;
		if (g_pck594)
			cfg0.uCFG0_hdmipk_420_sel = 1;
		else
			cfg0.uCFG0_hdmipk_420_sel = 0;
		cfg01.uCFG0_sp_wrbk_8b = 1;

		ctrl.uCTRL_hdmi420sp_wrbk = 1;
		cfg0.uCFG0_CfgSelAlgn = 0x91;

		hrx_dev->vip_bits_per_pixel = 12;

		vip_ups_enableups(hrx_dev, 1, VIP_OUTPUT_10BIT);
		vip_ups_enableups420(hrx_dev, 1);

		if (input_mode == VIP_IMODE11_12BIT_YUV420) {
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);
		} else if (input_mode == VIP_IMODE2_12BIT_YCBCR) {
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		} else if (input_mode == VIP_IMODE5_12BIT_YC) {
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		} else if (input_mode == VIP_IMODE8_12BIT_RGB) {
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		} else {
		}
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG01, cfg01.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 3);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_WriteClient_pack, 1);
	} else if (output_mode == VIP_OMODE0_8BIT_YC) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);
		cfg0.uCFG0_write_sel_8b = 0;
		cfg0.uCFG0_CfgSelAlgn = 0x91;
		ctrl.uCTRL_scl1d_in = 0;
		ctrl.uCTRL_hdmi420sp_wrbk = 0;
		cfg0.uCFG0_CfgSel_12_10_8 = 0;
		cfg0.uCFG0_fifo_rd_sel = 0;
		cfg0.uCFG0_hdmi_pk_8b_10b = 0;
		cfg0.uCFG0_hdmipk_420_sel = 0;
		hrx_dev->vip_bits_per_pixel = 16;

		switch (input_mode) {
		case VIP_IMODE0_8BIT_YCBCR:
			//vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;//YVU
			cfg0.uCFG0_hdmi_pk_8b_10b = 0;
			break;

		case VIP_IMODE1_10BIT_YCBCR:
			//vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE2_12BIT_YCBCR:
			//vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		case VIP_IMODE3_8BIT_YC:
			cfg0.uCFG0_CfgSelAlgn = 0x61;//UVY
			cfg0.uCFG0_hdmi_pk_8b_10b = 0;
			break;

		case VIP_IMODE4_10BIT_YC:
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_422_420);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE5_12BIT_YC:
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_422_420);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		case VIP_IMODE6_8BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 0;
			break;

		case VIP_IMODE7_10BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE8_12BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		default:
			break;
		}

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 0);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;
	} else if (output_mode == VIP_OMODE1_10BIT_YC) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);
		cfg0.uCFG0_write_sel_8b = 1;
		cfg0.uCFG0_CfgSelAlgn = 0x91;
		ctrl.uCTRL_scl1d_in = 0;
		ctrl.uCTRL_hdmi420sp_wrbk = 0;
		cfg0.uCFG0_CfgSel_12_10_8 = 0;
		cfg0.uCFG0_fifo_rd_sel = 0;
		cfg0.uCFG0_hdmi_pk_8b_10b = 0;
		cfg0.uCFG0_hdmipk_420_sel = 0;

		hrx_dev->vip_bits_per_pixel = 20;

		switch (input_mode) {
		case VIP_IMODE1_10BIT_YCBCR:
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE2_12BIT_YCBCR:
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		case VIP_IMODE4_10BIT_YC:
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
			break;

		case VIP_IMODE5_12BIT_YC:
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_422_420);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
			break;

		case VIP_IMODE7_10BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			break;

		case VIP_IMODE8_12BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		default:
			break;
		}

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 1);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;
	} else if (output_mode == VIP_OMODE11_12BIT_YC) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);
		cfg0.uCFG0_write_sel_8b = 1;
		cfg0.uCFG0_CfgSelAlgn = 0x91;
		ctrl.uCTRL_scl1d_in = 0;
		ctrl.uCTRL_hdmi420sp_wrbk = 0;
		cfg0.uCFG0_CfgSel_12_10_8 = 0;
		cfg0.uCFG0_fifo_rd_sel = 0;
		cfg0.uCFG0_hdmi_pk_8b_10b = 0;
		cfg0.uCFG0_hdmipk_420_sel = 0;

		hrx_dev->vip_bits_per_pixel = 24;

		switch (input_mode) {
		case VIP_IMODE2_12BIT_YCBCR:
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		case VIP_IMODE5_12BIT_YC:
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);

			if (hrx_dev->ui_tunnel_mode) {
				cfg0.uCFG0_CfgSelAlgn = 0x92;
				cfg0.uCFG0_CfgSel_12_10_8 = 1;
			} else {
				cfg0.uCFG0_CfgSelAlgn = 0x61;//UVY
			}
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			cfg0.uCFG0_write_sel_8b = 2;

			break;

		case VIP_IMODE8_12BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		default:
			break;
		}

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 5);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;
	} else if (output_mode == VIP_OMODE5_8BIT_YCBCR) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);
		cfg0.uCFG0_write_sel_8b = 0;
		cfg0.uCFG0_CfgSelAlgn = 0x91;
		ctrl.uCTRL_scl1d_in = 0;
		ctrl.uCTRL_hdmi420sp_wrbk = 0;
		cfg0.uCFG0_CfgSel_12_10_8 = 0;
		cfg0.uCFG0_fifo_rd_sel = 0;
		cfg0.uCFG0_hdmi_pk_8b_10b = 0;
		hrx_dev->vip_bits_per_pixel = 24;
		cfg0.uCFG0_hdmipk_420_sel = 0;

		switch (input_mode) {
		case VIP_IMODE0_8BIT_YCBCR:
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;//YVU
			cfg0.uCFG0_hdmi_pk_8b_10b = 0;
			break;

		case VIP_IMODE1_10BIT_YCBCR:
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE2_12BIT_YCBCR:
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		case VIP_IMODE6_8BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 0;
			break;

		case VIP_IMODE7_10BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE8_12BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		default:
			break;
		}

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 5);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;
	} else if (output_mode == VIP_OMODE6_10BIT_YCBCR) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);
		cfg0.uCFG0_write_sel_8b = 1;
		cfg0.uCFG0_CfgSelAlgn = 0x91;
		ctrl.uCTRL_scl1d_in = 0;
		ctrl.uCTRL_hdmi420sp_wrbk = 0;
		cfg0.uCFG0_CfgSel_12_10_8 = 0;
		cfg0.uCFG0_fifo_rd_sel = 0;
		cfg0.uCFG0_hdmi_pk_8b_10b = 0;
		cfg0.uCFG0_hdmipk_420_sel = 0;

		hrx_dev->vip_bits_per_pixel = 30;

		switch (input_mode) {
		case VIP_IMODE0_8BIT_YCBCR:
			break;

		case VIP_IMODE1_10BIT_YCBCR:
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE2_12BIT_YCBCR:
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		case VIP_IMODE6_8BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			break;

		case VIP_IMODE7_10BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE8_12BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		default:
			break;
		}

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 6);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;
	} else if (output_mode == VIP_OMODE7_12BIT_YCBCR) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);
		cfg0.uCFG0_write_sel_8b = 2;
		cfg0.uCFG0_CfgSelAlgn = 0x61;
		ctrl.uCTRL_scl1d_in = 0;
		ctrl.uCTRL_hdmi420sp_wrbk = 0;
		cfg0.uCFG0_CfgSel_12_10_8 = 0;
		cfg0.uCFG0_fifo_rd_sel = 0;
		cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		cfg0.uCFG0_hdmipk_420_sel = 0;

		hrx_dev->vip_bits_per_pixel = 36;

		switch (input_mode) {
		case VIP_IMODE2_12BIT_YCBCR:
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		case VIP_IMODE8_12BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_sRGB_TO_YUV_709);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		default:
			break;
		}

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 8);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;
	} else if (output_mode == VIP_OMODE8_8BIT_RGB) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);
		cfg0.uCFG0_write_sel_8b = 0;
		cfg0.uCFG0_CfgSelAlgn = 0x91;
		ctrl.uCTRL_scl1d_in = 0;
		ctrl.uCTRL_hdmi420sp_wrbk = 0;
		cfg0.uCFG0_CfgSel_12_10_8 = 0x0;
		cfg0.uCFG0_fifo_rd_sel = 0;
		cfg0.uCFG0_hdmi_pk_8b_10b = 0;
		hrx_dev->vip_bits_per_pixel = 24;
		cfg0.uCFG0_hdmipk_420_sel = 0;

		switch (input_mode) {
		case VIP_IMODE0_8BIT_YCBCR:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_YUV_709_TO_sRGB);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;//YVU
			cfg0.uCFG0_hdmi_pk_8b_10b = 0;
			break;

		case VIP_IMODE1_10BIT_YCBCR:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_YUV_709_TO_sRGB);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE2_12BIT_YCBCR:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_YUV_709_TO_sRGB);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		case VIP_IMODE6_8BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_BYPASS_MODE);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_8BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 0;
			break;

		case VIP_IMODE7_10BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_BYPASS_MODE);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE8_12BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_BYPASS_MODE);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_8BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		default:
			break;
		}

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 5);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;
	} else if (output_mode == VIP_OMODE9_10BIT_RGB) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);
		cfg0.uCFG0_write_sel_8b = 1;
		cfg0.uCFG0_CfgSelAlgn = 0x91;
		ctrl.uCTRL_scl1d_in = 0;
		ctrl.uCTRL_hdmi420sp_wrbk = 0;
		cfg0.uCFG0_CfgSel_12_10_8 = 0;
		cfg0.uCFG0_fifo_rd_sel = 0;
		cfg0.uCFG0_hdmi_pk_8b_10b = 0;
		cfg0.uCFG0_hdmipk_420_sel = 0;

		hrx_dev->vip_bits_per_pixel = 30;

		switch (input_mode) {
		case VIP_IMODE0_8BIT_YCBCR:
			break;

		case VIP_IMODE1_10BIT_YCBCR:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_YUV_709_TO_sRGB);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE2_12BIT_YCBCR:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_YUV_709_TO_sRGB);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		case VIP_IMODE6_8BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_BYPASS_MODE);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			break;

		case VIP_IMODE7_10BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_BYPASS_MODE);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_10BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 1;
			break;

		case VIP_IMODE8_12BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_BYPASS_MODE);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_dither_set_mode(hrx_dev, VIP_DITHER_ROUND);
			vip_dither_set_ycmode(hrx_dev, VIP_DITHER_YCMODE_444);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_10BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		default:
			break;
		}

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 6);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;
	} else if (output_mode == VIP_OMODE10_12BIT_RGB) {
		T32HDMIRX_PIPE_CFG0 cfg0;
		T32HDMIRX_PIPE_CTRL ctrl;

		cfg0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG0);
		ctrl.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL);
		cfg0.uCFG0_write_sel_8b = 2;
		cfg0.uCFG0_CfgSelAlgn = 0x61;
		ctrl.uCTRL_scl1d_in = 0;
		ctrl.uCTRL_hdmi420sp_wrbk = 0;
		cfg0.uCFG0_CfgSel_12_10_8 = 0;
		cfg0.uCFG0_fifo_rd_sel = 0;
		cfg0.uCFG0_hdmi_pk_8b_10b = 2;
		cfg0.uCFG0_hdmipk_420_sel = 0;

		hrx_dev->vip_bits_per_pixel = 36;

		switch (input_mode) {
		case VIP_IMODE2_12BIT_YCBCR:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_YUV_709_TO_sRGB);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_8BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x61;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		case VIP_IMODE8_12BIT_RGB:
			vip_csc_set_coeff(hrx_dev, VIP_CSC_BYPASS_MODE);
			vip_dns_enabledns(hrx_dev, 1, VIP_OUTPUT_10BIT);
			vip_dither_set_ctrl(hrx_dev, VIP_INPUT_12BIT, VIP_OUTPUT_12BIT);
			cfg0.uCFG0_CfgSelAlgn = 0x19;
			cfg0.uCFG0_hdmi_pk_8b_10b = 2;
			break;

		default:
			break;
		}

		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CFG0, cfg0.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_CTRL, ctrl.u32);
		vip_reg_write(hrx_dev, RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pack, 8);
		hrx_dev->ui_pipe_ctrl = ctrl.u32;
	}

	/* set VIP module to be configured */
	hrx_dev->vip_status = VIP_STATUS_ACTIVE;
}

void vip_close(struct syna_hrx_v4l2_dev *hrx_dev)
{
	/* set VIP module to be EXIT */
	hrx_dev->vip_status = VIP_STATUS_EXIT;
}
