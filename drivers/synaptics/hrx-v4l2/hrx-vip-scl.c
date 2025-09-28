// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-vip-scl.h"
#include "hrx-reg.h"
#include "vip_coeff_adp_scl_soft.h"

/* TODO: made s16 from u16 */
static s16 default_index_coeff_table[COEFF_MAX][VIP_FRC_SCL_NUM_OF_PHASES][VIP_FRC_SCL_NUM_OF_COEFF];

/***************************************************************************************
 * FUNCTION: internal fuction to get the start address of an FRC SCL instance by unit number
 * PARAMS: unit_num - unit number
 * RETURN:  the start address
 ***************************************************************************************/
static u32 vip_frc_scl_get_reg_start_addr(struct syna_hrx_v4l2_dev *hrx_dev, VIP_FRC_SCL_NUM unit_num)
{
	u32 reg_addr = 0;

	/*get register start address */
	switch (unit_num) {
	case VIP_FRC_SCL_Y:
		reg_addr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC;
		break;
	case VIP_FRC_SCL_UV:
		reg_addr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC;
		break;
	default:
		return 0;
	}

	return reg_addr;
}

/***************************************************************************************
 * FUNCTION: load the default register values to FRC SCL the unit
 * PARAMS: unit_num - unit number
 * RETURN:  0 - succeed
 ***************************************************************************************/
static int vip_frc_scl_load_default_val(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num)
{
	int ret = 0;

	hrx_dev->uiSclCfg15[unit_num] = 0;
	hrx_dev->uiSclCfg0[unit_num] = 0;
	hrx_dev->hscl_coeff[unit_num] = -1; // invalid
	hrx_dev->vscl_coeff[unit_num] = -1; // invalid

	/* For customized SCL coeffs */
	hrx_dev->hscl_coeff_update[unit_num] = 0; // No customized
	hrx_dev->vscl_coeff_update[unit_num] = 0; // No customized

	/*Cropping related initilization*/
	hrx_dev->scl_ctrl[unit_num].leftCrop     = 0;
	hrx_dev->scl_ctrl[unit_num].rightCrop    = 0;
	hrx_dev->scl_ctrl[unit_num].uncrop_IHRes = 0;
	hrx_dev->scl_ctrl[unit_num].uiUVswapEn   = 0;

	return ret;
}

/***************************************************************************************
 * FUNCTION:Program Y and UV TGs inside VIP SCL
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 * RETURN:  0 - succeed.
 ***************************************************************************************/
static int vip_offline_scl_otg_prog(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 reg_addr = 0;
	u32 uiWidth = 0, uiHeight = 0;
	T32TG_SIZE tg_size;
	T32TG_HB tg_hb;
	T32TG_HB_CR tg_hb_cr;
	T32TG_HB_CR2 tg_hb_cr2;
	T32TG_VB0 tg_vb0;
	T32TG_VB0_CR tg_vb0_cr;
	T32TG_VB0_CR2 tg_vb0_cr2;

	/* Programe Luma scalar output TG */
	uiWidth  = hrx_dev->scl_res[VIP_FRC_SCL_Y].OHRes;
	uiHeight = hrx_dev->scl_res[VIP_FRC_SCL_Y].OVRes;
	tg_size.uSIZE_Y = uiHeight + 6;
	tg_size.uSIZE_X = uiWidth + 10;
	tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = uiWidth + 8;
	tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;
	tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = uiHeight + 1;
	tg_vb0.uVB0_BE = 0;
	tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 0;

	reg_addr = VPP_BASE_ADDRESS_VIPSCLTOP + RA_VIPSCLTOP_OVPSCL_OTG_Y;
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_SIZE, tg_size.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_HB, tg_hb.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_HB_CR, tg_hb_cr.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_HB_CR2, tg_hb_cr2.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_VB0, tg_vb0.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_VB0_CR, tg_vb0_cr.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_VB0_CR2, tg_vb0_cr2.u32);

	/* Programe UV scalar output TG */
	uiWidth  = hrx_dev->scl_res[VIP_FRC_SCL_UV].OHRes;
	uiHeight = hrx_dev->scl_res[VIP_FRC_SCL_UV].OVRes;
	tg_size.uSIZE_Y = uiHeight + 6;
	tg_size.uSIZE_X = uiWidth + 10;
	tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = uiWidth + 8;
	tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;
	tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = uiHeight + 1;
	tg_vb0.uVB0_BE = 0;
	tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 0;

	reg_addr = VPP_BASE_ADDRESS_VIPSCLTOP + RA_VIPSCLTOP_OVPSCL_OTG_UV;
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_SIZE, tg_size.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_HB, tg_hb.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_HB_CR, tg_hb_cr.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_HB_CR2, tg_hb_cr2.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_VB0, tg_vb0.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_VB0_CR, tg_vb0_cr.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_TG_VB0_CR2, tg_vb0_cr2.u32);

	return 0;
}

/***************************************************************************************
 * FUNCTION:Program Y and UV TGs inside VIP SCL
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 * RETURN:  0 - succeed.
 ***************************************************************************************/
static int  vip_offline_scl_pipe_prog(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 reg_addr = 0;

	reg_addr = VPP_BASE_ADDRESS_VIPSCLTOP;
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_VIPSCLTOP_CTRL2, hrx_dev->scl_res[VIP_FRC_SCL_Y].IHRes * hrx_dev->scl_res[VIP_FRC_SCL_Y].IVRes);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_VIPSCLTOP_CTRL3, hrx_dev->scl_res[VIP_FRC_SCL_UV].IHRes * hrx_dev->scl_res[VIP_FRC_SCL_UV].IVRes);

	reg_addr = VPP_BASE_ADDRESS_VIPSCL_Y;
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_OVPSCL_scl1d_Inpix, hrx_dev->scl_res[VIP_FRC_SCL_Y].IHRes * hrx_dev->scl_res[VIP_FRC_SCL_Y].IVRes);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_OVPSCL_scl1d_Outpix, hrx_dev->scl_res[VIP_FRC_SCL_Y].OHRes * hrx_dev->scl_res[VIP_FRC_SCL_Y].OVRes);

	reg_addr = VPP_BASE_ADDRESS_VIPSCL_UV;
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_OVPSCL_scl1d_Inpix, hrx_dev->scl_res[VIP_FRC_SCL_UV].IHRes * hrx_dev->scl_res[VIP_FRC_SCL_UV].IVRes);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_OVPSCL_scl1d_Outpix, hrx_dev->scl_res[VIP_FRC_SCL_UV].OHRes * hrx_dev->scl_res[VIP_FRC_SCL_UV].OVRes);

	vip_offline_scl_otg_prog(hrx_dev);

	return 0;
}

/***************************************************************************************
 * FUNCTION: Load the scalar UPS coefficients
 * PARAMS:
 *             hrx_dev - vpp object
 *            piCoeff - UPS coefficients
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer
 ***************************************************************************************/
static int vip_scl_config_ups_coeff(struct syna_hrx_v4l2_dev *hrx_dev, int *piCoeff, VIP_FRC_SCL_NUM unit_num)
{
	return 0;
}


/***************************************************************************************
 * FUNCTION: Load the scalar DNS coefficients
 * PARAMS:
 *             hrx_dev - vpp object
 *             piCoeff -  DNS coefficients
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer
 ***************************************************************************************/
static int vip_scl_config_dns_coeff(struct syna_hrx_v4l2_dev *hrx_dev, int *piCoeff, VIP_FRC_SCL_NUM unit_num)
{
	return 0;
}

/***************************************************************************************
 * FUNCTION: Calulate the non linear scaling parameters
 * PARAMS:
 *             hrx_dev - vpp object
 *             pscl_init_ratio - intial DDA increment
 *             pscl_inc_ratio  - Increment to DDA increment
 *             pscl_nlcres     - Central undistorted width of the output image
 * RETURN:  0 - succeed
 ***************************************************************************************/
static int vip_scl_calc_NLscl_params(PVIP_SCL_RES pIORes, s32 *pscl_init_ratio, s32 *pscl_inc_ratio, s32 *pscl_nlcres)
{
	u32 ires1, ores1, ires2, ores2;
	u32 tail_ores;
	u32 c_scale = 0;
	u32 scl_nlcres = 0;
	u32 c_ratio = 0, c_ires = 0, tail_ires = 0, l_ires = 0;
	u32 nl_inc_ratio = 0, nl_init_ratio = 0;

	ires1 = pIORes->IHRes;
	ores1 = pIORes->OHRes;
	ires2 = pIORes->IVRes;
	ores2 = pIORes->OVRes;

	c_scale = 255; /* TODO Get the init val */
	/* check 1: minimum tail length limitation */
	tail_ores = (256-c_scale)*ores1/512; /* o/p tail zone */
	if (tail_ores < 5) {
		tail_ores = 5;
		c_scale = 256 - tail_ores*512/ores1;
	}
	scl_nlcres = ores1 - 2*tail_ores; /* central undistorted o/p res */
	c_ratio = ires2*(1<<20)/ores2; /* central preserved aspect ratio */
	c_ires = (c_ratio * scl_nlcres + (1<<19)) >> 20; /* central undistorted i/p res */
	tail_ires = (ires1 - c_ires)>>1; /* input tail zone */

	nl_inc_ratio = 2*(c_ratio - (1<<20)*tail_ires/tail_ores) / (tail_ores+1);
	nl_init_ratio = c_ratio - (nl_inc_ratio) * tail_ores;

	/* check 2: c_ratio and nl_init_ratio should not be negative, but nl_inc_ratio can. */
	if (nl_init_ratio < 0) {
		nl_init_ratio = 0;
		tail_ores = (((ires2*ores1/ores2)-ires1)<<20)/((c_ratio-nl_init_ratio) == 0?1:(c_ratio-nl_init_ratio));
		if (tail_ores < 5)
			tail_ores = 5;
		else if (tail_ores >= (ores1>>1))
			tail_ores = ((ores1-20+1)>>1);
		c_ratio = ((ires1<<20)-nl_init_ratio*tail_ores)/(ores1-tail_ores);
		scl_nlcres = ores1 - (tail_ores<<1);
		c_ires = (c_ratio*scl_nlcres + (1<<19))>>20;
		l_ires = ((ires1-c_ires)+1)>>1;
		c_scale = 256 - (tail_ores<<9)/ores1;
		nl_inc_ratio = ((c_ratio-nl_init_ratio)+(tail_ores>>1))/tail_ores;
	}
	/* Check 3: limited by max downscaling ratio 255 */
	else if (nl_init_ratio >= (int)(1<<20)*255) {
		nl_init_ratio = (int)(1<<20)*255;
		tail_ores = (((ires2*ores1/ores2)-ires1)<<20)/((c_ratio-nl_init_ratio) == 0?1:(c_ratio-nl_init_ratio));
		if (tail_ores < 5)
			tail_ores = 5;
		else if (tail_ores >= (ores2>>1))
			tail_ores = ((ores2-20+1)>>1);
		c_ratio = ((ires1<<20)-nl_init_ratio*tail_ores)/(ores1-tail_ores);
		scl_nlcres = ores1 - (tail_ores<<1);
		c_ires = ((c_ratio*scl_nlcres) + (1<<19))>>20;
		l_ires = ((ires1-c_ires)+1)>>1;
		c_scale = 256 - (tail_ores<<9)/ores1;
		nl_inc_ratio = ((c_ratio-nl_init_ratio)+(tail_ores>>1))/tail_ores;
	}
	*pscl_inc_ratio = nl_inc_ratio; /* increment to hratio */
	*pscl_init_ratio = nl_init_ratio; /* initial hratio */
	*pscl_nlcres = scl_nlcres;

	return 0;
}

static int vip_scl_main_scl_adp1d_type(struct syna_hrx_v4l2_dev *hrx_dev, PVIP_SCL_RES pIORes, s8 *sel_1D_adp)
{
	*sel_1D_adp = 1;/* 1D Scalar */
	return 0;
}

/*--------------------------
 *-calculate htap and vtap numbers-
 *---------------------------
 */
static int vip_scl_main_scl_cal_taps(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, PVIP_SCL_RES pIORes, s8 *phTap, s8 *pvTap)
{
	s8 sel_1D_adp, hTaps, vTaps;
	u32 max_pixels_in_scl_mem = 1920;

	vip_scl_main_scl_adp1d_type(hrx_dev, pIORes, &sel_1D_adp);
	if (sel_1D_adp == 0) {
		hTaps = 4;
		vTaps = 4;
	} else {
		if ((pIORes->OHRes > pIORes->IHRes) && (pIORes->IHRes > 1920))/* Upscale */
			hTaps = 5;
		else
			hTaps = 8;

		if (unit_num == VPP_FRC_SCL_MAIN)
			max_pixels_in_scl_mem = 1920;
		else
			max_pixels_in_scl_mem = 960;

		if ((pIORes->IVRes == pIORes->OVRes) && (pIORes->OHRes == pIORes->IHRes))
			vTaps = 1;/* horizontalOnly scale modes including bypass modes where scaler.s line buffer SRAM is used as rat control FIFO*/
		else if (pIORes->IVRes == pIORes->OVRes)
			vTaps = 1;/* horizontalOnly scale modes including bypass modes where scaler.s line buffer SRAM is used as rat control FIFO*/
		else if ((pIORes->OHRes > pIORes->IHRes) && (pIORes->IHRes > max_pixels_in_scl_mem))/* Upscale */
			vTaps = 3;
		else if (pIORes->OHRes <= pIORes->IHRes) {
			/* DownScale */
			if (unit_num == VPP_FRC_SCL_MAIN)
				vTaps = 5;//6; /* Faint Line issue fix Bug# 42575 */
			else {
				/*OVP and VIP SCL has memsize max ((575+1)*10) pixles. So if OHRes less than 1152 only can hold 5 lines*/
				if (pIORes->OHRes < 1152)
					vTaps = 5;
				else
					vTaps = 3;
			}
		} else if ((pIORes->OVRes > pIORes->IVRes) && (pIORes->OVRes > 1440))
			/* Fixing the faint line issue for up scaling o/p Vres more than 1440 */
			vTaps = 5;
		else
			vTaps = 6;
	}

	*phTap = hTaps;
	*pvTap = vTaps;

	return 0;
}

/*--------------------------
 *  -Configure Scl for special cases in Deint-
 * ---------------------------
 */
static int vip_scl_config_for_deint(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, PVIP_SCL_RES pIORes, s8 *enDeintSmallUpScale)
{
	*enDeintSmallUpScale = 0;
	return 0;
}

/***************************************************************************************
 * FUNCTION: Load the Main scalar parameters
 * PARAMS:
 *           hrx_dev - vpp object
 *           unit_num - Unit num to identify the plain
 *           pIORes - Input/Output resolution
 *           pSclCtrl - Scalar Control paramters
 * RETURN:  0 - succeed
 ***************************************************************************************/
static int vip_scl_set_main_scl_params(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, PVIP_SCL_RES pIORes, PVIP_SCL_CTRL pSclCtrl)
{
	s8 sel_1D_adp = -1;
	s8 up_down = -1;
	u32 HRatio, VRatio;
	u32 minHRes, hRes;
	s8 hTaps = -1, vTaps = -1;
	s8 enDeintSmallUpScale = 0;
	s32 scl_init_ratio, scl_inc_ratio,  scl_nlcres;
	s16 wls = 0, pls = 0, fcp = 0;
	s16 maxMemSize_1D = 0;
	int goutputmode, ginputmode;

	VIP_SCL_CTRL0 stSclCtrl0;
	VIP_SCL_CTRL1 stSclCtrl1;
	int ret = 0;

	T32ADPSCL_CFG0 stSclCfg0;
	T32ADPSCL_CFG1 stSclCfg1;
	T32ADPSCL_CFG2 stSclCfg2;
	T32ADPSCL_CFG3 stSclCfg3 = {0};
	T32ADPSCL_CFG4 stSclCfg4 = {0};
	T32ADPSCL_CFG5 stSclCfg5;
	T32ADPSCL_CFG6 stSclCfg6;
	T32ADPSCL_CFG7 stSclCfg7 = {0};
	T32ADPSCL_CFG8 stSclCfg8;
	T32ADPSCL_CFG9 stSclCfg9;
	T32ADPSCL_CFG10 stSclCfg10;
	T32ADPSCL_CFG11 stSclCfg11;
	T32ADPSCL_CFG12 stSclCfg12;
	T32ADPSCL_CFG15 stSclCfg15;
	T32ADPSCL_CFG16 stSclCfg16;
	T32ADPSCL_CFG17 stSclCfg17;
	T32ADPSCL_CFG18 stSclCfg18;
	T32SCL1D_YC_CFG20 stScl1dYcCfg20;

	u32 reg_addr = 0;
	u32 uncrop_IHRes = pSclCtrl->uncrop_IHRes;

	if (unit_num != VPP_FRC_SCL_MAIN &&
		unit_num != VIP_FRC_SCL_Y &&
		unit_num != VIP_FRC_SCL_UV &&
		unit_num != VPP_FRC_SCL_OVP_Y &&
		unit_num != VPP_FRC_SCL_OVP_UV)
		return -1;

	goutputmode = hrx_dev->vip_omode;
	ginputmode = hrx_dev->vip_imode;

	/* select the scalar */
	vip_scl_config_for_deint(hrx_dev, unit_num, pIORes, &enDeintSmallUpScale);
	vip_scl_main_scl_adp1d_type(hrx_dev, pIORes, &sel_1D_adp);

	/* select the up/down scale mode */
	if ((pIORes->OHRes < pIORes->IHRes) || ((pIORes->OHRes == pIORes->IHRes) &&
		(pIORes->OVRes <  pIORes->IVRes)))
		/* Horizontal down scaling or veritcal only down scaling */
		up_down = 1;/* down scale */
	else
		up_down = 0;/* up scale/1:1(bypass) */

	reg_addr = vip_frc_scl_get_reg_start_addr(hrx_dev, unit_num);/* Following this are SCL programming */

	/* calculate htap and vtap numbers */
	vip_scl_main_scl_cal_taps(hrx_dev, unit_num, pIORes, &hTaps, &vTaps);
	/* set initial phase and ctl0 */
	if (sel_1D_adp == 0) {
		stSclCfg0.uCFG0_even = 0x80;
		stSclCfg0.uCFG0_odd = 0x80;
		stSclCfg0.uCFG0_hinitph = 0x80;
	} else {
		stSclCfg0.uCFG0_even = (vTaps % 2)<<7;
		stSclCfg0.uCFG0_odd = (vTaps % 2)<<7;
		stSclCfg0.uCFG0_hinitph = (hTaps % 2)<<7;
	}

	stSclCtrl0.uchAvg4FiltSel = 0;
	stSclCtrl0.uchCscEn = 0;
	if (pSclCtrl->NLEn == 1)
		stSclCtrl0.uchHnlEn = 1;
	else
		stSclCtrl0.uchHnlEn = 0;

	if (1 == sel_1D_adp && 1 == up_down) {
		/* 1D down scaling */
		stSclCtrl0.uchUpsEn = 1;
		stSclCtrl0.uchDnsEn = 1;
	} else {
		stSclCtrl0.uchUpsEn = 0;
		switch (unit_num) {
		case VIP_FRC_SCL_Y:
		case VIP_FRC_SCL_UV:
			stSclCtrl0.uchDnsEn = 1;
			break;
		default:
			return -1;
		}
	}
	stSclCtrl0.uchDgnlEn = 1;
	stSclCtrl0.uchDemoModeEn = 0;

	stSclCfg0.uCFG0_ctrl0 = stSclCtrl0.uch8;
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG0, stSclCfg0.u32));
	hrx_dev->uiSclCfg0[unit_num] = stSclCfg0.u32;

	/* set input and output resolutions */
	stSclCfg1.uCFG1_ivres = pIORes->IVRes;
	stSclCfg1.uCFG1_ovres = pIORes->OVRes;
	stSclCfg2.uCFG2_ihres = uncrop_IHRes; /* Read the complete uncropped resolution */
	stSclCfg2.uCFG2_ohres = pIORes->OHRes;

	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG1, stSclCfg1.u32));
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG2, stSclCfg2.u32));

	/* calculate horizontal and vertical scaling ratio */
	HRatio = ((u32) pIORes->IHRes << 20) / pIORes->OHRes;
	VRatio = ((u32) pIORes->IVRes << 20) / pIORes->OVRes;

	stSclCfg3.u32 = 0;
	stSclCfg4.u32 = 0;
	stSclCfg3.uCFG3_vratio = VRatio;
	stSclCfg4.uCFG4_hratio = HRatio;
	if (pIORes->OVRes < pIORes->IVRes)
		/* 1: the most bottom pixel of input image is used to interpolate the most bottom pixel of output image */
		stSclCfg3.uCFG3_mbp_align = 1;
	else
		/* Must be 0 if Vertically '1:1 or upscale' */
		stSclCfg3.uCFG3_mbp_align = 0;

	if (pIORes->OHRes < pIORes->IHRes)
		/* 1: the most right pixel of input image is used to interpolate the most right pixel of output image */
		stSclCfg4.uCFG4_mrp_align = 1;
	else
		/* Must be 0 if Horizontally '1:1 or upscale' */
		stSclCfg4.uCFG4_mrp_align = 0;

	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG3, stSclCfg3.u32));
	hrx_dev->uiSclCfg3[unit_num] = stSclCfg3.u32;
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG4, stSclCfg4.u32));

	stSclCfg7.uCFG7_ups_cblank = 0x200;

	stSclCfg7.uCFG7_htap_offset = 0;
	stSclCfg7.uCFG7_vtap_offset = 0;
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG7, stSclCfg7.u32));
	hrx_dev->uiSclCfg7[unit_num] = stSclCfg7.u32;

	/* set SCL control register- if non linear scaling, H scaler is after V scaler */
	if (pSclCtrl->NLEn == 0) {
		stSclCfg8.uCFG8_init_ratio = 0x100000;
		stSclCfg9.uCFG9_inc_ratio  = 0;
		stSclCfg10.uCFG10_nlcres   = 0;
	} else {
		vip_scl_calc_NLscl_params(pIORes, &scl_init_ratio, &scl_inc_ratio, &scl_nlcres);
		stSclCfg8.uCFG8_init_ratio = scl_init_ratio;
		stSclCfg9.uCFG9_inc_ratio  = scl_inc_ratio;
		stSclCfg10.uCFG10_nlcres   = scl_nlcres;
	}
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG8, stSclCfg8.u32));
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG9, stSclCfg9.u32));
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG10, stSclCfg10.u32));

	stSclCfg11.uCFG11_avg4_coeff0 = 0x840;
	stSclCfg11.uCFG11_avg4_coeff1 = 0x240;
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG11, stSclCfg11.u32));

	stSclCfg12.uCFG12_avg4_coeff2 = 0x240;
	stSclCfg12.uCFG12_avg4_coeff3 = 0x840;
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG12, stSclCfg12.u32));
	stSclCtrl1.uch8 = 0;
	stSclCtrl1.uchSel_1D_adp = sel_1D_adp;
	stSclCtrl1.uchUp_down = up_down;
	if (sel_1D_adp == 0) {
		stSclCtrl1.uchVSclInUpsEn    = 1;
		stSclCtrl1.uchVSclOutUpsEn   = 0;
		stSclCtrl1.uchVSclInUpsCoeff = 0;
	} else {
		stSclCtrl1.uchVSclInUpsEn    = 0;
		stSclCtrl1.uchVSclInUpsCoeff = 0;
		switch (unit_num) {
		case VIP_FRC_SCL_Y:
		case VIP_FRC_SCL_UV:
			if (up_down == 1)
				stSclCtrl1.uchVSclOutUpsEn   = 0;
			else
				stSclCtrl1.uchVSclOutUpsEn   = 1;
			break;
		default:
			return -1;
		}
	}

	stSclCfg15.u32 = hrx_dev->uiSclCfg15[unit_num];
	stSclCfg15.uCFG15_ctrl1 = stSclCtrl1.uch8;
	stSclCfg15.uCFG15_frndsel = 1;

	/* Added below change for >>2 operation by scaler */
	if (ginputmode == VIP_IMODE5_12BIT_YC &&
		goutputmode == VIP_OMODE11_12BIT_YC)
		stSclCfg15.uCFG15_hctrl = 0x1C;
	else
		stSclCfg15.uCFG15_hctrl = 0x1A;

	if ((1 == vTaps || 2 == vTaps))
		stSclCfg15.uCFG15_vctrl = 0x0B;
	else
		stSclCfg15.uCFG15_vctrl = 0x1A;

	/* LSB 2 bit for to crop at begining of line, MSB 2 bit to crop at end of line */
	stSclCfg15.uCFG15_crop = ((pSclCtrl->leftCrop&0x3)) | ((pSclCtrl->rightCrop&0x03)<<2);

	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_SCL1D_YC_CFG15, stSclCfg15.u32));

	hrx_dev->uiSclCfg15[unit_num] = stSclCfg15.u32;

	hRes = uncrop_IHRes; /* lsize calculation should be based on complete/uncropped resolution */
	if (sel_1D_adp == 0) {
		stSclCfg17.uCFG17_lsize = (hRes / 10) + ((hRes % 10) ? 1:0);
		stSclCfg17.uCFG17_lsize_A = (hRes / 10) + ((hRes % 10) ? 1:0);
	} else {
		if (hRes < pIORes->OHRes)
			minHRes = hRes;
		else
			minHRes = pIORes->OHRes;
		/* If Scalar chop is enabled, then choose hres as follows
		 * Upscale, -> hres = uncropped ihres
		 * downscale, -> hres = ohres
		 * 1:1, -> hres = uncropped ihres
		 */
		if ((pSclCtrl->leftCrop+pSclCtrl->rightCrop) != 0) {
			if (pIORes->IHRes <= pIORes->OHRes)
				minHRes = hRes;
			else
				minHRes = pIORes->OHRes;
		}
		stSclCfg17.uCFG17_lsize   = (minHRes / 10) + ((minHRes % 10) ? 1:0);
		stSclCfg17.uCFG17_lsize_A = (minHRes / 10) + ((minHRes % 10) ? 1:0);
	}
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_SCL1D_YC_CFG17, stSclCfg17.u32));

	stSclCfg18.u32 = 0;
	if (unit_num == VIP_FRC_SCL_UV) {
		if (pIORes->OHRes < 1920)
			maxMemSize_1D = 287;
		else
			maxMemSize_1D = 191;
	} else
		maxMemSize_1D = 575;

	if (sel_1D_adp == 0)
		stSclCfg5.uCFG5_memsize = 1727;
	else
		stSclCfg5.uCFG5_memsize = maxMemSize_1D;

	stSclCfg5.uCFG5_vwrap   = 0;
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG5, stSclCfg5.u32));

	stSclCfg6.uCFG6_ups_cswap = pSclCtrl->uiUVswapEn;
	stSclCfg6.uCFG6_ups_cshift = 0;
	stSclCfg6.uCFG6_ups_yshift = 0;
	stSclCfg6.uCFG6_ups_yblank = 0;

	if ((pIORes->IHRes > 1920 &&
		((pIORes->IVRes > pIORes->OVRes &&
		((((pIORes->IVRes - pIORes->OVRes) * 100) / (pIORes->IVRes)) <= 13)))))
		 /* vertical only or both vertical and horizontal down scaling till 13% */
		stSclCfg6.uCFG6_fstall = 0x0;
	else
		stSclCfg6.uCFG6_fstall = 0x80;
	stSclCfg6.uCFG6_bstall = 0x80;

	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_CFG6, stSclCfg6.u32));
	hrx_dev->uiSclCfg6[unit_num] = stSclCfg6.u32;

	if (((pIORes->IVRes == pIORes->OVRes) && (vTaps == 1)))
		stSclCfg18.uCFG18_fifo_mode  = 1;
	else
		stSclCfg18.uCFG18_fifo_mode  = 0;

	if ((stSclCfg18.uCFG18_fifo_mode == 1)) {
		if (pIORes->IHRes < pIORes->OHRes)
			minHRes = pIORes->IHRes;
		else
			minHRes = pIORes->OHRes;
		wls = (stSclCfg5.uCFG5_memsize + 1) / stSclCfg17.uCFG17_lsize;
		pls = ((stSclCfg5.uCFG5_memsize + 1) - (stSclCfg17.uCFG17_lsize * wls)) * 5;
		fcp = wls * minHRes + pls;

		/* Hard coding to disable fifo mode to avoid un-recoverable UF condition when multiple plane is enabled
		 *But fifo mode should be 1 for vtaps = 1; Require double check in BG4CT-A0.
		 */
		stSclCfg18.uCFG18_fifo_mode  = 0;

		if ((fcp - 100) > 0x1FFF)/* Register bit width limitation*/
			stSclCfg18.uCFG18_fifo_depth = (0x1FFF - 100);
		else
			stSclCfg18.uCFG18_fifo_depth = (fcp - 100);
		stSclCfg18.uCFG18_fifo_dfst  = minHRes;
	} else {
		stSclCfg18.uCFG18_fifo_mode  = 0;
		stSclCfg18.uCFG18_fifo_depth = 0;
		stSclCfg18.uCFG18_fifo_dfst  = 0;
	}
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_SCL1D_YC_CFG18, stSclCfg18.u32));

	stSclCfg16.u32 = 0;

	if ((up_down == 0))
		stSclCfg16.uCFG16_en_n          = 1;
	else
		stSclCfg16.uCFG16_en_n          = 0;

	if ((pIORes->OHRes > pIORes->IHRes &&  pIORes->OVRes < pIORes->IVRes) ||
		(pIORes->OHRes < pIORes->IHRes &&  pIORes->OVRes > pIORes->IVRes))
		stSclCfg16.uCFG16_xbstall_en    = 0;
	else if (enDeintSmallUpScale == 1)
		stSclCfg16.uCFG16_xbstall_en    = 0;
	else if ((1 == stSclCfg16.uCFG16_en_n && 0 == up_down) || (stSclCfg18.uCFG18_fifo_mode == 1))
		stSclCfg16.uCFG16_xbstall_en    = 1;
	else
		stSclCfg16.uCFG16_xbstall_en    = 0;

	if ((pIORes->OHRes - pIORes->IHRes) >= 30)
		stSclCfg16.uCFG16_xbstall_dly   = 30;
	else
		stSclCfg16.uCFG16_xbstall_dly   = 0;

	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_SCL1D_YC_CFG16, stSclCfg16.u32));

	stScl1dYcCfg20.u32 = 0;
	stScl1dYcCfg20.uCFG20_dpwr_regs = 3;
	stScl1dYcCfg20.uCFG20_syp_yc420_regs = 0;
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_SCL1D_YC_CFG20, stScl1dYcCfg20.u32));

	return ret;
}

/***************************************************************************************
 * FUNCTION: Load the required Coeff from pre calculated table
 * PARAMS: unit_num - unit number(0, 1, 2, 3, 4, 5)
 *               pIORes -pointer to I&O resolution
 *               pSclCtrl - pointer to scaler control information
 * RETURN:  0 - succeed
 ***************************************************************************************/
static int vip_frc_scl_set_scl_coeff(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, PVIP_SCL_RES pIORes, PVIP_SCL_CTRL pSclCtrl)
{
	int tempHorSclCoeffMode, tempVerSclCoeffMode;
	u32 reg_addr;
	int ret = 0;
	s8 hTaps = -1, vTaps = -1;
	s8 sclLayer;
	T32ADPSCL_COEFF_CFG0 stAdpSclCoeffCfg0;
	u32 *ptr = NULL;

	CHECK_FRC_SCL_NUM(unit_num);

	/* All scalar calculation should be based on cropped resolution */
	pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);

	if (unit_num == VIP_FRC_SCL_Y)
		reg_addr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
	else if (unit_num == VIP_FRC_SCL_UV)
		reg_addr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;

	if (pIORes != NULL) {
		switch (unit_num) {
		case VIP_FRC_SCL_Y:
		case VIP_FRC_SCL_UV:
			vip_scl_main_scl_cal_taps(hrx_dev, unit_num, pIORes, &hTaps, &vTaps);
			break;
		}
	}

	/* Revert back the modification done to 'pIORes->IHRes' */
	pIORes->IHRes = pSclCtrl->uncrop_IHRes;

	if (!(pSclCtrl->ForceSel & VIP_MODE_COEFF_SEL_FORCE_H)) {
		if (pIORes != NULL) {
			if (unit_num == VPP_FRC_SCL_MAIN ||
				unit_num == VIP_FRC_SCL_Y ||
				unit_num == VIP_FRC_SCL_UV ||
				unit_num == VPP_FRC_SCL_OVP_Y ||
				unit_num == VPP_FRC_SCL_OVP_UV) {
				switch (hTaps) {
				case 4:
					tempHorSclCoeffMode = COEFF_H_MAIN_ADP_HLUT_CS;
					break;
				case 5:
					tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_BLK5;
					break;
				case 8:
					/* Added Soft coeff for UV scaler to avoid color distortion
					 * with content from PC HDMIRx input
					 */
					if ((unit_num == VIP_FRC_SCL_UV) && (pIORes->OHRes < pIORes->IHRes))
						tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_PH8_SOFT;
					else
						tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_PH8;
					break;
				default:
					tempHorSclCoeffMode = -1;
					HRX_LOG(VIP_ERROR, "%d %s Unsupported HLut table\n", __LINE__, __func__);
					return -1;
				}
			} else {
				switch (hTaps) {
				case 5:
					tempHorSclCoeffMode = COEFF_H_GFX_HLUT_BLK5_GFX;
					break;
				case 8:
					tempHorSclCoeffMode = COEFF_H_GFX_HLUT_PV8;
					break;
				default:
					tempHorSclCoeffMode = -1;
					HRX_LOG(VIP_ERROR, "%d %s Unsupported HLut table\n", __LINE__, __func__);
					return -1;
				}
			}
		} else
			tempHorSclCoeffMode = -1;
	} else {
		/* force the user selection. */
		tempHorSclCoeffMode = pSclCtrl->HSclMode;
		hTaps = 0;
		HRX_LOG(VIP_ERROR, "%d %s Unsupported now. Needs prg updates\n", __LINE__, __func__);
		return -1;
	}

	if (!(pSclCtrl->ForceSel & VIP_MODE_COEFF_SEL_FORCE_V)) {
		if (pIORes != NULL) {
			if (unit_num == VPP_FRC_SCL_MAIN ||
				unit_num == VIP_FRC_SCL_Y ||
				unit_num == VIP_FRC_SCL_UV ||
				unit_num == VPP_FRC_SCL_OVP_Y ||
				unit_num == VPP_FRC_SCL_OVP_UV) {
				switch (vTaps) {
				case 1:
					tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_DBG1;
					break;
				case 2:
					tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_2TAP;
					break;
				case 3:
					tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL;
					break;
				case 4:
					tempVerSclCoeffMode = COEFF_V_MAIN_ADP_HLUT_BS;
					break;
				case 5:
					tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_BLK5;
					break;
				case 6:
					tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_6TAP;
					break;
				default:
					tempVerSclCoeffMode = -1;
					HRX_LOG(VIP_ERROR, "%d %s Unsupported VLut table\n", __LINE__, __func__);
					return -1;
				}
			} else {
				switch (vTaps) {
				case 1:
					tempVerSclCoeffMode = COEFF_V_GFX_VLUT_1TAP;
					break;
				case 3:
					tempVerSclCoeffMode = COEFF_V_GFX_VLUT_3TAP_CUBIC;
					break;
				case 4:
					tempVerSclCoeffMode = COEFF_V_GFX_VLUT_CS_4TAP;
					break;
				default:
					tempVerSclCoeffMode = -1;
					HRX_LOG(VIP_ERROR, "%d %s Unsupported VLut table\n", __LINE__, __func__);
					return -1;
				}
			}
		} else
			tempVerSclCoeffMode = -1;
	} else {
		/* force the user selection. */
		tempVerSclCoeffMode = pSclCtrl->VSclMode;
		vTaps = 0;
		HRX_LOG(VIP_ERROR, "%d %s Unsupported now. Needs prg updates\n", __LINE__, __func__);
		return -1;
	}

	/* Calculating sclLayer of scl_coeffs table */
	if (unit_num == VIP_FRC_SCL_Y)
		sclLayer = 0;
	else if (unit_num == VIP_FRC_SCL_UV)
		sclLayer = 1;

	if ((-1 != tempHorSclCoeffMode) &&
		(hrx_dev->hscl_coeff[unit_num] != tempHorSclCoeffMode ||
		hrx_dev->hscl_coeff_update[unit_num] == 1)) {

		hrx_dev->hscl_coeff[unit_num] = tempHorSclCoeffMode;
		hrx_dev->hscl_coeff_update[unit_num] = 0;
		ptr = hrx_dev->scl_coeffs[tempHorSclCoeffMode];
		ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;
		bcmbuf_write_block(hrx_dev->pcurr_vbi_bcm_buf, ptr, VPP_FRC_COEFFTAB_BCMBUF_SIZE);
	}

	if ((-1 != tempVerSclCoeffMode) &&
		(hrx_dev->vscl_coeff[unit_num] != tempVerSclCoeffMode ||
		 hrx_dev->vscl_coeff_update[unit_num] == 1)) {

		hrx_dev->vscl_coeff[unit_num] = tempVerSclCoeffMode;
		hrx_dev->vscl_coeff_update[unit_num] = 0;
		ptr = hrx_dev->scl_coeffs[tempVerSclCoeffMode];
		ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;
		bcmbuf_write_block(hrx_dev->pcurr_vbi_bcm_buf, ptr, VPP_FRC_COEFFTAB_BCMBUF_SIZE);
	}

	switch (unit_num) {
	case VIP_FRC_SCL_Y:
	case VIP_FRC_SCL_UV:
		stAdpSclCoeffCfg0.uCFG0_htap        = hTaps;
		stAdpSclCoeffCfg0.uCFG0_vtap        = vTaps;
		stAdpSclCoeffCfg0.uCFG0_coeff_index = 0;
		stAdpSclCoeffCfg0.uCFG0_coeff_hvsel = 0;
		stAdpSclCoeffCfg0.uCFG0_coeffread   = 0;
		stAdpSclCoeffCfg0.uCFG0_coeffload   = 0;
		CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_COEFF_CFG0,
					stAdpSclCoeffCfg0.u32));
		break;
	}

	return 0;
}

/***************************************************************************************
 * FUNCTION: set control information and I&O resolution to scaler
 * PARAMS: unit_num - unit number(0, 1, 2, 3, 4, 5)
 *               pIORes -pointer to I&O resolution
 *               pSclCtrl - pointer to scaler control information
 * RETURN:  0 - succeed
 ***************************************************************************************/
static int vip_frc_scl_set_scl_ctrl_params(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, PVIP_SCL_RES pIORes, PVIP_SCL_CTRL pSclCtrl)
{
	int ret = 0;

	/* Take backup of the uncropped resolution - so that IHRes can be reverted at later function calls also */
	pSclCtrl->uncrop_IHRes = pIORes->IHRes;
	/* All scalar calculation should be based on cropped resolution */
	pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);

	CHECK_FRC_SCL_NUM(unit_num);

	switch (unit_num) {
	case VIP_FRC_SCL_Y:
	case VIP_FRC_SCL_UV:
		CHECK_VIP_SCL_RETVAL(vip_scl_set_main_scl_params(hrx_dev, unit_num, pIORes, pSclCtrl));
		break;
	default:
		return -1;
	}

	/* Revert back the modification done to 'pIORes->IHRes' */
	pIORes->IHRes = pSclCtrl->uncrop_IHRes;

	/* load HV filter coefficiets*/
	if (pSclCtrl->DynamicLoad)
		vip_frc_scl_set_scl_coeff(hrx_dev, unit_num, pIORes, pSclCtrl);

	return ret;
}

/***************************************************************************************
 * FUNCTION: Load the scalar FRC parameters
 * PARAMS:
 *           hrx_dev - vpp object
 *           unit_num - Unit num to identify the plain
 *           pFrcRes - FRC input and output resolutions
 * RETURN:  0 - succeed
 ***************************************************************************************/
static int vip_frc_scl_set_frc_params(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, PVIP_SCL_RES pFrcRes)
{
	u32 reg_addr;
	VIP_FRC_CTRL0_0 stFrcCtrl0_0;
	VIP_FRC_CTRL0_1 stFrcCtrl0_1;
	VIP_FRC_CTRL1_2 stFrcCtrl1_2;
	T32ADPSCL_FRC_CFG0 stSclFrcCfg0;
	T32ADPSCL_FRC_CFG1 stSclFrcCfg1;
	T32ADPSCL_FRC_CFG2 stSclFrcCfg2;
	T32ADPSCL_FRC_CFG3 stSclFrcCfg3;
	int ret = 0;
	UNSG16 temp_uCFG1_dly_de_lrst = 0;
	s8 enDeintSmallUpScale = 0;

	CHECK_FRC_SCL_NUM(unit_num);

	/*get register start address of the FRC_SCL unit */
	switch (unit_num) {
	case VIP_FRC_SCL_Y:
		reg_addr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC;
		break;
	case VIP_FRC_SCL_UV:
		reg_addr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC;
		break;
	default:
		return -1;
	}

	stFrcCtrl0_0.uchFrcMode = 3;
	stFrcCtrl0_0.uchMosiacMode = 0;
	stFrcCtrl0_0.uchBitMode_16_24 = 1;
	stFrcCtrl0_0.uchSpMode = 0;

	stFrcCtrl0_1.uchLoadRead = 0;
	stFrcCtrl0_1.uchLoadWrite = 0;
	stFrcCtrl0_1.uchReadCtrl = 0;
	stFrcCtrl0_1.uchWriteCtrl = 0;
	stFrcCtrl0_1.uchFreeze = 0;
	stFrcCtrl0_1.uchInvFeFld = 0;
	stFrcCtrl0_1.uchInvBeFld = 0;
	stFrcCtrl0_1.uchBeFrstResetEn = 0;

	stSclFrcCfg0.uCFG0_sclclk_ctrl = 0x80;
	stSclFrcCfg0.uCFG0_ctrl0 = stFrcCtrl0_0.uch8;
	stSclFrcCfg0.uCFG0_ctrl1 = stFrcCtrl0_1.uch8;
	stSclFrcCfg0.uCFG0_sclclk_ctrl1 = 0x80;

	stFrcCtrl1_2.uchTreaLessReadCtrl = 0;
	stFrcCtrl1_2.uchTreaLessWriteCtrl = 0;
	stSclFrcCfg1.uCFG1_ctrl2 = stFrcCtrl1_2.uch8;
	stSclFrcCfg1.uCFG1_dly_frst_de = 0x14;

	if (enDeintSmallUpScale == 1)
		temp_uCFG1_dly_de_lrst = 0x0;
	/* Fix For exact 2x downscaling for 4K 600MHz */
	else if ((pFrcRes->IHRes*100)/(pFrcRes->OHRes) > (6*100))
		/* When down scale increases de_lrst should be increased  */
		temp_uCFG1_dly_de_lrst = 0x100;
	else if ((unit_num == VPP_FRC_SCL_MAIN)
			&& (pFrcRes->IHRes < pFrcRes->OHRes) && (pFrcRes->IVRes > pFrcRes->OVRes))
		/* Fix for H-Up and V-Down sclaing(Eg: 90/270Degree rotation) on BG4CDp A0
		 * (In A0 vppSysClk reduced from 660MHz to 600MHz)
		 */
		temp_uCFG1_dly_de_lrst = 0x10;
	else
		temp_uCFG1_dly_de_lrst = 0xA0;

	if (pFrcRes->OHRes > pFrcRes->IHRes)
		stSclFrcCfg1.uCFG1_dly_de_lrst = temp_uCFG1_dly_de_lrst;
	else if (hrx_dev->scl_ctrl[unit_num].NLEn == 1)
		stSclFrcCfg1.uCFG1_dly_de_lrst = 0x200;
	else
		stSclFrcCfg1.uCFG1_dly_de_lrst = temp_uCFG1_dly_de_lrst;

	if ((pFrcRes->OHRes < pFrcRes->IHRes) || ((pFrcRes->OHRes == pFrcRes->IHRes) && (pFrcRes->OVRes <  pFrcRes->IVRes)))
		stSclFrcCfg1.uCFG1_auto_lrst = 1;
	else
		stSclFrcCfg1.uCFG1_auto_lrst = 0;

	stSclFrcCfg2.uCFG2_bevres = pFrcRes->IVRes;
	stSclFrcCfg2.uCFG2_fevres = pFrcRes->OVRes;
	stSclFrcCfg2.uCFG2_dly_lrst_de = 1;

	stSclFrcCfg3.uCFG3_behres = pFrcRes->IHRes;
	stSclFrcCfg3.uCFG3_clnt_ctrl = 3;
	stSclFrcCfg3.uCFG3_ctrl = 0;
	stSclFrcCfg3.uCFG3_rff_ctrl = 0;

	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_FRC_CFG0, stSclFrcCfg0.u32));
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_FRC_CFG1, stSclFrcCfg1.u32));
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_FRC_CFG2, stSclFrcCfg2.u32));
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr + RA_ADPSCL_FRC_CFG3, stSclFrcCfg3.u32));

	hrx_dev->uiSclFrcCfg1[unit_num] = stSclFrcCfg1.u32;
	hrx_dev->uiSclFrcCfg0[unit_num] = stSclFrcCfg0.u32;
	return ret;
}

/***************************************************************************************
 * FUNCTION: set delay between DE  to line reset.
 * PARAMS: unit_num - unit number
 *               DeLrstDelay -delay value
 * RETURN:  0 - succeed
 ***************************************************************************************/
static int vip_frc_scl_set_de_linereset_delay(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, u32 DeLrstDelay)
{
	u32 reg_addr;
	u32 ret = 0;
	T32ADPSCL_FRC_CFG1 stSclFrcCfg1;

	CHECK_FRC_SCL_NUM(unit_num);

	/* get register start address */
	switch (unit_num) {
	case VIP_FRC_SCL_Y:
		reg_addr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC + RA_ADPSCL_FRC_CFG1;
		break;
	case VIP_FRC_SCL_UV:
		reg_addr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC + RA_ADPSCL_FRC_CFG1;
		break;
	default:
		return -1;
	}

	stSclFrcCfg1.u32 = hrx_dev->uiSclFrcCfg1[unit_num];

	stSclFrcCfg1.uCFG1_dly_de_lrst = DeLrstDelay;

	/* set it to BCM buffer */
	CHECK_VIP_SCL_RETVAL(bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, reg_addr, stSclFrcCfg1.u32));

	hrx_dev->uiSclFrcCfg1[unit_num] = stSclFrcCfg1.u32;

	return  ret;
}

static int generate_scl_main_coefficiena(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num,
	unsigned int **pptr, int hv, u16 (*coeffs)[VIP_FRC_SCL_NUM_OF_COEFF])
{
	u32 reg_addr = 0;
	s16 i;
	int ret = 0;
	unsigned int *ptr = *pptr;

	T32ADPSCL_COEFF_CFG0 stSclCoeffCfg0;
	T32ADPSCL_COEFF_CFG1 stSclCoeffCfg1;
	T32ADPSCL_COEFF_CFG2 stSclCoeffCfg2;
	T32ADPSCL_COEFF_CFG3 stSclCoeffCfg3;
	T32ADPSCL_COEFF_CFG4 stSclCoeffCfg4;

	switch (unit_num) {
	case VIP_FRC_SCL_Y:
		reg_addr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
		break;
	case VIP_FRC_SCL_UV:
		reg_addr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
		break;
	default:
		return -1;
	}

	/*Coeff table loading*/
	for (i = 0; i < VIP_FRC_SCL_NUM_OF_PHASES; i++) {
		stSclCoeffCfg1.uCFG1_coeff0 = coeffs[i][0];
		stSclCoeffCfg1.uCFG1_coeff1 = coeffs[i][1];
		stSclCoeffCfg2.uCFG2_coeff2 = coeffs[i][2];
		stSclCoeffCfg2.uCFG2_coeff3 = coeffs[i][3];
		stSclCoeffCfg3.uCFG3_coeff4 = coeffs[i][4];
		stSclCoeffCfg3.uCFG3_coeff5 = coeffs[i][5];
		stSclCoeffCfg4.uCFG4_coeff6 = coeffs[i][6];
		stSclCoeffCfg4.uCFG4_coeff7 = coeffs[i][7];

		*ptr++ = stSclCoeffCfg1.u32;
		*ptr++ = reg_addr + RA_ADPSCL_COEFF_CFG1;
		*ptr++ = stSclCoeffCfg2.u32;
		*ptr++ = reg_addr + RA_ADPSCL_COEFF_CFG2;
		*ptr++ = stSclCoeffCfg3.u32;
		*ptr++ = reg_addr + RA_ADPSCL_COEFF_CFG3;
		*ptr++ = stSclCoeffCfg4.u32;
		*ptr++ = reg_addr + RA_ADPSCL_COEFF_CFG4;

		stSclCoeffCfg0.uCFG0_htap        = 0;
		stSclCoeffCfg0.uCFG0_vtap        = 0;
		stSclCoeffCfg0.uCFG0_coeff_index = i;
		stSclCoeffCfg0.uCFG0_coeff_hvsel = (hv ^ 1); /* Invert it as the vpp_api.h and vpp.h has different */
		stSclCoeffCfg0.uCFG0_coeffread   = 0;

		stSclCoeffCfg0.uCFG0_coeffload   = 0;
		*ptr++ = stSclCoeffCfg0.u32;
		*ptr++ = reg_addr + RA_ADPSCL_COEFF_CFG0;
		stSclCoeffCfg0.uCFG0_coeffload   = 1;
		*ptr++ = stSclCoeffCfg0.u32;
		*ptr++ = reg_addr + RA_ADPSCL_COEFF_CFG0;
		stSclCoeffCfg0.uCFG0_coeffload   = 0;
		*ptr++ = stSclCoeffCfg0.u32;
		*ptr++ = reg_addr + RA_ADPSCL_COEFF_CFG0;
	}

	*pptr = ptr;

	return ret;
}

static int generate_scl_coefficiena(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, unsigned int **pptr, int hv, u16 (*coeffs)[VIP_FRC_SCL_NUM_OF_COEFF])
{
	switch (unit_num) {
	case VIP_FRC_SCL_Y:
	case VIP_FRC_SCL_UV:
		generate_scl_main_coefficiena(hrx_dev, unit_num, pptr, hv, coeffs);
		break;
	default:
		return -1;
	}

	return 0;
}

/***************************************************************************************
 * FUNCTION: program SCL scaler coeffs with customized coeffs table
 * PARAMS:
 *            coeffID - scalar coeffs mode
 *              hv     - indicating it is for horizontal(0) or vertical(1) scaler
 *              unit_num    - indicating which plane the coeff is for
 *        coeffs    - customized coeffs talbe's pointer;
 *                      Note: if it is NULL, it means recover default coeffs table
 * RETURN:  0 - succeed
 ***************************************************************************************/
static int vip_customize_coefficients(struct syna_hrx_v4l2_dev *hrx_dev, int coeffID, int hv, int unit_num)
{
	/* TODO: Changed from hardcoded 12 to define */
	u16 (*coeffs)[VIP_FRC_SCL_NUM_OF_COEFF];
	unsigned int *ptr;
	int sclLayer;

	/* Calculating sclLayer of scl_coeffs table */
	if (unit_num == VIP_FRC_SCL_Y)
		sclLayer = 0;
	else if (unit_num == VIP_FRC_SCL_UV)
		sclLayer = 1;
	else {
		HRX_LOG(VIP_ERROR, "%s: Invalid request\n", __func__);
		return -1;
	}


	if (coeffID < FIRST_SCL_COEFF || coeffID >= COEFF_MAX)
		return -1;

	ptr = hrx_dev->scl_coeffs[coeffID];
	ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;

	/* Always use default ones */
	coeffs = (u16 (*)[VIP_FRC_SCL_NUM_OF_COEFF])default_index_coeff_table[coeffID];
	hrx_dev->cust_sc_bLoaded[sclLayer][coeffID] = FALSE;

	generate_scl_coefficiena(hrx_dev, unit_num, &ptr, hv, coeffs);
	VPP_MEM_FlushCache(hrx_dev->mem_list, &hrx_dev->scl_coeffs_mem_handle[coeffID], 0);

	return 0;
}

static int frc_scl_generate_main_adp_coeff_tab_hv(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, s16 coeffID, s16 hvSel, int format, s16 (*lut)[4])
{
	s16 cf1[32][4], cf2;
	s16 i, j;
	int ret = 0;

	/* Convert into sign magnitude */
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 4; j++) {
			cf2 = (lut[i][3-j])&0x800;
			cf1[i][j] = (lut[i][3-j])&0x7ff;
			cf1[i][j] = (cf2 ? -cf1[i][j] : cf1[i][j])&0x7ff;
			cf1[i][j] = ((cf2 | cf1[i][j])&0xfff);
		}
	}
	/*Coeff table loading*/
	for (i = 0; i < VIP_FRC_SCL_NUM_OF_PHASES; i++) {
		default_index_coeff_table[coeffID][i][0] = cf1[i][3];
		default_index_coeff_table[coeffID][i][1] = cf1[i][2];
		default_index_coeff_table[coeffID][i][2] = cf1[i][1];
		default_index_coeff_table[coeffID][i][3] = cf1[i][0];
		default_index_coeff_table[coeffID][i][4] = 0;
		default_index_coeff_table[coeffID][i][5] = 0;
		default_index_coeff_table[coeffID][i][6] = 0;
		default_index_coeff_table[coeffID][i][7] = 0;
	}
	return ret;
}

static int frc_scl_generate_main_1d_coeff_tab_h(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, s16 coeffID, s16 hvSel, int format, s16 (*hlut)[8])
{
	s16 hcf1[32][8], hcf2;
	s16 i, j;
	int ret = 0;

	/* Convert into sign magnitude */
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 8; j++) {
			hcf2 = (hlut[i][7-j])&0x800;
			hcf1[i][j] = (hlut[i][7-j])&0x7ff;
			hcf1[i][j] = (hcf2 ? -hcf1[i][j] : hcf1[i][j])&0x7ff;
			hcf1[i][j] = ((hcf2 | hcf1[i][j])&0xfff);
		}
	}

	/*Horizontal Coeff table loading*/
	for (i = 0; i < VIP_FRC_SCL_NUM_OF_PHASES; i++) {
		default_index_coeff_table[coeffID][i][0] = hcf1[i][7];
		default_index_coeff_table[coeffID][i][1] = hcf1[i][6];
		default_index_coeff_table[coeffID][i][2] = hcf1[i][5];
		default_index_coeff_table[coeffID][i][3] = hcf1[i][4];
		default_index_coeff_table[coeffID][i][4] = hcf1[i][3];
		default_index_coeff_table[coeffID][i][5] = hcf1[i][2];
		default_index_coeff_table[coeffID][i][6] = hcf1[i][1];
		default_index_coeff_table[coeffID][i][7] = hcf1[i][0];
	}

	return ret;

}

static int frc_scl_generate_main_1d_coeff_tab_v(struct syna_hrx_v4l2_dev *hrx_dev, int unit_num, s16 coeffID, s16 hvSel, int format, s16 (*vlut)[6])
{
	s16 vcf1[32][8], vcf2;
	s16 i, j;
	int ret = 0;
	int vc[6];
	s16 vTaps;
	/* Convert into sign magnitude */
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 6; j++) {
			vcf2 = (vlut[i][5-j])&0x800;
			vcf1[i][j] = (vlut[i][5-j])&0x7ff;
			vcf1[i][j] = (vcf2 ? -vcf1[i][j] : vcf1[i][j])&0x7ff;
			vcf1[i][j] = ((vcf2 | vcf1[i][j])&0xfff);
		}
	}

	switch (coeffID) {
	case COEFF_V_MAIN_1D_VLUT_DBG1:
		vTaps = 1;
		break;
	case COEFF_V_MAIN_1D_VLUT_2TAP:
		vTaps = 2;
		break;
	case COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL:
		vTaps = 3;
		break;
	case COEFF_V_MAIN_1D_VLUT_BLK5:
		vTaps = 5;
		break;
	case COEFF_V_MAIN_1D_VLUT_6TAP:
		vTaps = 6;
		break;
	default:
		vTaps = -1;
		break;
	}

	/* Fitting tap position based on number vtaps */
	for (i = 0; i < 6; i++) {
		vc[i] = 6 + i - vTaps;
		if (vc[i] >= 6)
			vc[i] = 0;
	}

	/*Vertical Coeff table loading*/
	for (i = 0; i < VIP_FRC_SCL_NUM_OF_PHASES; i++) {
		default_index_coeff_table[coeffID][i][0] = vcf1[i][vc[5]];
		default_index_coeff_table[coeffID][i][1] = vcf1[i][vc[4]];
		default_index_coeff_table[coeffID][i][2] = vcf1[i][vc[3]];
		default_index_coeff_table[coeffID][i][3] = vcf1[i][vc[2]];
		default_index_coeff_table[coeffID][i][4] = vcf1[i][vc[1]];
		default_index_coeff_table[coeffID][i][5] = vcf1[i][vc[0]];
		default_index_coeff_table[coeffID][i][6] = 0;
		default_index_coeff_table[coeffID][i][7] = 0;
	}

	return ret;

}

static int vip_frc_scl_generate_coeff_tables(struct syna_hrx_v4l2_dev *hrx_dev)
{

	frc_scl_generate_main_adp_coeff_tab_hv(hrx_dev, VPP_FRC_SCL_MAIN, COEFF_H_MAIN_ADP_HLUT_CS, 1, 0x1A, vip_scl_hlut_cs);
	frc_scl_generate_main_adp_coeff_tab_hv(hrx_dev, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_ADP_HLUT_BS, 0, 0x1A, vip_scl_hlut_bs);

	frc_scl_generate_main_1d_coeff_tab_h(hrx_dev, VPP_FRC_SCL_MAIN, COEFF_H_MAIN_1D_HLUT_BLK5, 1, 0x1A, vip_scl_hlut_blk5);
	frc_scl_generate_main_1d_coeff_tab_h(hrx_dev, VPP_FRC_SCL_MAIN, COEFF_H_MAIN_1D_HLUT_PH8, 1, 0x1A, vip_scl_hlut_ph8);
	frc_scl_generate_main_1d_coeff_tab_h(hrx_dev, VPP_FRC_SCL_MAIN, COEFF_H_MAIN_1D_HLUT_PH8_SOFT, 1, 0x1A, vip_scl_hlut_ph8_soft);
	frc_scl_generate_main_1d_coeff_tab_v(hrx_dev, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_1D_VLUT_DBG1, 0, 0x0B, vip_scl_vlut_dbg1);
	frc_scl_generate_main_1d_coeff_tab_v(hrx_dev, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_1D_VLUT_2TAP, 0, 0x0B, vip_scl_vlut_2tap);
	frc_scl_generate_main_1d_coeff_tab_v(hrx_dev, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL, 0, 0x1A, vip_scl_vlut_3tap_cubic_1DScl);
	frc_scl_generate_main_1d_coeff_tab_v(hrx_dev, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_1D_VLUT_BLK5, 0, 0x1A, vip_scl_vlut_blk5);
	frc_scl_generate_main_1d_coeff_tab_v(hrx_dev, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_1D_VLUT_6TAP, 0, 0x1A, vip_scl_vlut_6tap);

	vip_customize_coefficients(hrx_dev, COEFF_H_MAIN_ADP_HLUT_CS, 0, VIP_FRC_SCL_Y);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_ADP_HLUT_BS, 1, VIP_FRC_SCL_Y);

	vip_customize_coefficients(hrx_dev, COEFF_H_MAIN_1D_HLUT_BLK5, 0, VIP_FRC_SCL_Y);
	vip_customize_coefficients(hrx_dev, COEFF_H_MAIN_1D_HLUT_PH8, 0, VIP_FRC_SCL_Y);
	vip_customize_coefficients(hrx_dev, COEFF_H_MAIN_1D_HLUT_PH8_SOFT, 0, VIP_FRC_SCL_Y);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_1D_VLUT_DBG1, 1, VIP_FRC_SCL_Y);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_1D_VLUT_2TAP, 1, VIP_FRC_SCL_Y);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL, 1, VIP_FRC_SCL_Y);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_1D_VLUT_BLK5, 1, VIP_FRC_SCL_Y);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_1D_VLUT_6TAP, 1, VIP_FRC_SCL_Y);

	vip_customize_coefficients(hrx_dev, COEFF_H_MAIN_ADP_HLUT_CS, 0, VIP_FRC_SCL_UV);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_ADP_HLUT_BS, 1, VIP_FRC_SCL_UV);

	vip_customize_coefficients(hrx_dev, COEFF_H_MAIN_1D_HLUT_BLK5, 0, VIP_FRC_SCL_UV);
	vip_customize_coefficients(hrx_dev, COEFF_H_MAIN_1D_HLUT_PH8, 0, VIP_FRC_SCL_UV);
	vip_customize_coefficients(hrx_dev, COEFF_H_MAIN_1D_HLUT_PH8_SOFT, 0, VIP_FRC_SCL_UV);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_1D_VLUT_DBG1, 1, VIP_FRC_SCL_UV);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_1D_VLUT_2TAP, 1, VIP_FRC_SCL_UV);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL, 1, VIP_FRC_SCL_UV);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_1D_VLUT_BLK5, 1, VIP_FRC_SCL_UV);
	vip_customize_coefficients(hrx_dev, COEFF_V_MAIN_1D_VLUT_6TAP, 1, VIP_FRC_SCL_UV);

	return 0;
}

/*******************************************************************
 * FUNCTION: Reset all VIP SCL blocks
 * PARAMS:   *hrx_dev - pointer to VIP object
 * RETURN: 0
 *****************************************************************
 */
int vip_scl_reset(struct syna_hrx_v4l2_dev *hrx_dev)
{
	vip_frc_scl_load_default_val(hrx_dev, VIP_FRC_SCL_Y);
	vip_frc_scl_load_default_val(hrx_dev, VIP_FRC_SCL_UV);
	return 0;
}


/**********************************************************************
 * FUNCTION: initialize all VIP SCL blocks according to configuration
 * PARAMS:   *hrx_dev - pointer to VIP object
 * RETURN: 0
 ********************************************************************
 */
int vip_scl_config(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int upsCoeff[] = {80, 0, -304, 0, 1248, 2048, 1248};
	int dnsCoeff[] = {80, 0, -304, 0, 1248, 2048};

	vip_frc_scl_generate_coeff_tables(hrx_dev);

	/* config FRC-SCLs */
	vip_frc_scl_set_de_linereset_delay(hrx_dev, VIP_FRC_SCL_Y, 80);
	vip_frc_scl_set_de_linereset_delay(hrx_dev, VIP_FRC_SCL_UV, 80);

	vip_scl_config_ups_coeff(hrx_dev, upsCoeff, VIP_FRC_SCL_Y);
	vip_scl_config_dns_coeff(hrx_dev, dnsCoeff, VIP_FRC_SCL_Y);

	return 0;
}

static void vip_tg_set_otg_params(struct syna_hrx_v4l2_dev *hrx_dev, VIP_TG_PARAMS *pParams)
{
	T32TG_SIZE tg_size;
	T32TG_HB tg_hb;
	T32TG_HB_CR tg_hb_cr;
	T32TG_HB_CR2 tg_hb_cr2;
	T32TG_VB0 tg_vb0;
	T32TG_VB0_CR tg_vb0_cr;
	T32TG_VB0_CR2 tg_vb0_cr2;
	u32 uiRegAddr = 0;

	tg_size.uSIZE_Y = pParams->height+6;
	tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = pParams->width+8;
	tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;

	tg_size.uSIZE_X = pParams->width+10;

	tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = pParams->height+1;
	tg_vb0.uVB0_BE = tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 0;

	uiRegAddr = hrx_dev->vip_base + RA_HDMIRX_PIPE_VIP_OTG;
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, uiRegAddr + RA_TG_SIZE, tg_size.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, uiRegAddr + RA_TG_HB, tg_hb.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, uiRegAddr + RA_TG_HB_CR, tg_hb_cr.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, uiRegAddr + RA_TG_HB_CR2, tg_hb_cr2.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, uiRegAddr + RA_TG_VB0, tg_vb0.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, uiRegAddr + RA_TG_VB0_CR, tg_vb0_cr.u32);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, uiRegAddr + RA_TG_VB0_CR2, tg_vb0_cr2.u32);
}

int vip_prog_scl(struct syna_hrx_v4l2_dev *hrx_dev, int *width, int *height, bool update_required)
{
	int sclUnitNum = -1;
	T32HDMIRX_PIPE_CTRL ctrl;
	T32HDMIRX_PIPE_CTRL0 ctrl0;
	T32HDMIRX_PIPE_CFG4 cfg4;
	T32VIPSCLTOP_CTRL4 ctrl4;
	VIP_TG_PARAMS tg_param;
	int goutputmode, ginputmode;
	u32 IHRes, IVRes, OHRes, OVRes;

	/* VIP configuration */
	ginputmode = hrx_dev->vip_imode;
	goutputmode = hrx_dev->vip_omode;
	IHRes = hrx_dev->vip_hwidth;
	IVRes = hrx_dev->vip_vheight;
	OHRes = hrx_dev->format.width;
	OVRes = hrx_dev->format.height;

	tg_param.width = OHRes;
	tg_param.height = OVRes;
	vip_tg_set_otg_params(hrx_dev, &tg_param);

	sclUnitNum = VIP_FRC_SCL_Y;
	if (update_required == true) {
		hrx_dev->scl_res[sclUnitNum].IHRes = IHRes;
		hrx_dev->scl_res[sclUnitNum].IVRes = IVRes;
		hrx_dev->scl_res[sclUnitNum].OHRes = OHRes;
		hrx_dev->scl_res[sclUnitNum].OVRes = OVRes;

		hrx_dev->scl_ctrl[sclUnitNum].NLEn          = 0;
		hrx_dev->scl_ctrl[sclUnitNum].DynamicLoad   = 1;
		hrx_dev->scl_ctrl[sclUnitNum].ForceSel      = 0;
		hrx_dev->scl_ctrl[sclUnitNum].HSclMode      = 0;
		hrx_dev->scl_ctrl[sclUnitNum].VSclMode      = 0;
		hrx_dev->scl_ctrl[sclUnitNum].CenterFrac    = 0;
		hrx_dev->scl_ctrl[sclUnitNum].CenterRatio   = 0;
		hrx_dev->scl_ctrl[sclUnitNum].leftCrop      = 0;
		hrx_dev->scl_ctrl[sclUnitNum].rightCrop     = 0;
		hrx_dev->scl_ctrl[sclUnitNum].uncrop_IHRes  = 0;
		hrx_dev->scl_ctrl[sclUnitNum].uiUVswapEn    = 0;

		vip_frc_scl_set_scl_ctrl_params(hrx_dev, sclUnitNum,
			&hrx_dev->scl_res[sclUnitNum], &hrx_dev->scl_ctrl[sclUnitNum]);
		vip_frc_scl_set_frc_params(hrx_dev, sclUnitNum,
			&hrx_dev->scl_res[sclUnitNum]);
	}

	*width  = hrx_dev->scl_res[sclUnitNum].OHRes;
	*height = hrx_dev->scl_res[sclUnitNum].OVRes;

	sclUnitNum = VIP_FRC_SCL_UV;

	if (update_required == true) {
		/* For DV Tunnel mode input (RGB)
		 * Program uv otg in 444 mode -> OHRES=OHRES, OVRES=OVRES, as compared to
		 * OHRES/2, OVRES/2 in 422 mode
		 * OHRes = hrx_dev->vip_hwidth VIP_OMODE11_12BIT_YC
		 * IHRes = hrx_dev->hwidth for VIP_IMODE5_12BIT_YC (From vip_reg_seq.out)
		 */

		if (ginputmode == VIP_IMODE9_8BIT_YUV420 ||
			ginputmode == VIP_IMODE10_10BIT_YUV420 ||
			ginputmode == VIP_IMODE11_12BIT_YUV420 ||
			ginputmode == VIP_IMODE3_8BIT_YC ||
			ginputmode == VIP_IMODE4_10BIT_YC ||
			((ginputmode == VIP_IMODE5_12BIT_YC) &&
			(hrx_dev->ui_tunnel_mode == 0)))
			hrx_dev->scl_res[sclUnitNum].IHRes = (IHRes / 2);
		else
			hrx_dev->scl_res[sclUnitNum].IHRes = IHRes;

		if (ginputmode == VIP_IMODE9_8BIT_YUV420 ||
				ginputmode == VIP_IMODE10_10BIT_YUV420 ||
				ginputmode == VIP_IMODE11_12BIT_YUV420)
			hrx_dev->scl_res[sclUnitNum].IVRes = (IVRes / 2);
		else
			hrx_dev->scl_res[sclUnitNum].IVRes = IVRes;

		if (goutputmode == VIP_OMODE2_8BIT_YUV420 ||
			goutputmode == VIP_OMODE3_10BIT_YUV420 ||
			goutputmode == VIP_OMODE4_12BIT_YUV420 ||
			goutputmode == VIP_OMODE0_8BIT_YC ||
			goutputmode == VIP_OMODE1_10BIT_YC ||
			((goutputmode == VIP_OMODE11_12BIT_YC) &&
			(hrx_dev->ui_tunnel_mode == 0)))
			hrx_dev->scl_res[sclUnitNum].OHRes = (OHRes / 2);
		else
			hrx_dev->scl_res[sclUnitNum].OHRes = OHRes;

		if (goutputmode == VIP_OMODE2_8BIT_YUV420 ||
			goutputmode == VIP_OMODE3_10BIT_YUV420 ||
			goutputmode == VIP_OMODE4_12BIT_YUV420)
			hrx_dev->scl_res[sclUnitNum].OVRes = (OVRes / 2);
		else
			hrx_dev->scl_res[sclUnitNum].OVRes = OVRes;

		hrx_dev->scl_ctrl[sclUnitNum].NLEn          = 0;
		hrx_dev->scl_ctrl[sclUnitNum].DynamicLoad   = 1;
		hrx_dev->scl_ctrl[sclUnitNum].ForceSel      = 0;
		hrx_dev->scl_ctrl[sclUnitNum].HSclMode      = 0;
		hrx_dev->scl_ctrl[sclUnitNum].VSclMode      = 0;
		hrx_dev->scl_ctrl[sclUnitNum].CenterFrac    = 0;
		hrx_dev->scl_ctrl[sclUnitNum].CenterRatio   = 0;
		hrx_dev->scl_ctrl[sclUnitNum].leftCrop      = 0;
		hrx_dev->scl_ctrl[sclUnitNum].rightCrop     = 0;
		hrx_dev->scl_ctrl[sclUnitNum].uncrop_IHRes  = 0;
		hrx_dev->scl_ctrl[sclUnitNum].uiUVswapEn    = 0;

		vip_frc_scl_set_scl_ctrl_params(hrx_dev, sclUnitNum,
			&hrx_dev->scl_res[sclUnitNum], &hrx_dev->scl_ctrl[sclUnitNum]);
		vip_frc_scl_set_frc_params(hrx_dev, sclUnitNum,
			&hrx_dev->scl_res[sclUnitNum]);
	}
	vip_offline_scl_pipe_prog(hrx_dev);

	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
		(hrx_dev->vip_base + RA_HDMIRX_PIPE_VIP_WR + RA_WriteClient_pix),
		(hrx_dev->vip_hwidth * hrx_dev->vip_vheight));

	if (goutputmode == VIP_OMODE5_8BIT_YCBCR ||
		goutputmode == VIP_OMODE6_10BIT_YCBCR ||
		goutputmode == VIP_OMODE7_12BIT_YCBCR ||
		goutputmode == VIP_OMODE8_8BIT_RGB ||
		goutputmode == VIP_OMODE9_10BIT_RGB ||
		goutputmode == VIP_OMODE10_12BIT_RGB)
		bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
			(hrx_dev->vip_base + RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_WriteClient_pix),
			(OHRes * OVRes));
	else
		bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
			(hrx_dev->vip_base + RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_WriteClient_pix),
			(OHRes * (OVRes / 2)));

	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
		(hrx_dev->vip_base + RA_HDMIRX_PIPE_scl1d_Inpix),
		(IHRes * IVRes));
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
		(hrx_dev->vip_base + RA_HDMIRX_PIPE_scl1d_Outpix),
		(OHRes * OVRes));
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
		(hrx_dev->vip_base + RA_HDMIRX_PIPE_sclOFIFO_YInpix),
		(OHRes * OVRes));
	if (goutputmode == VIP_OMODE5_8BIT_YCBCR ||
		goutputmode == VIP_OMODE6_10BIT_YCBCR ||
		goutputmode == VIP_OMODE7_12BIT_YCBCR ||
		goutputmode == VIP_OMODE8_8BIT_RGB ||
		goutputmode == VIP_OMODE9_10BIT_RGB ||
		goutputmode == VIP_OMODE10_12BIT_RGB ||
		((goutputmode == VIP_OMODE11_12BIT_YC) &&
		(hrx_dev->ui_tunnel_mode == 1)))
		bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
			(hrx_dev->vip_base + RA_HDMIRX_PIPE_sclOFIFO_UVInpix),
			(OHRes * OVRes));
	else
		bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
			(hrx_dev->vip_base + RA_HDMIRX_PIPE_sclOFIFO_UVInpix),
			(OHRes * (OVRes / 2)));

	ctrl.u32 = hrx_dev->ui_pipe_ctrl;

	if (ginputmode == VIP_IMODE0_8BIT_YCBCR ||
		ginputmode == VIP_IMODE1_10BIT_YCBCR ||
		ginputmode == VIP_IMODE2_12BIT_YCBCR ||
		ginputmode == VIP_IMODE6_8BIT_RGB ||
		ginputmode == VIP_IMODE7_10BIT_RGB ||
		ginputmode == VIP_IMODE8_12BIT_RGB)
		ctrl.uCTRL_uvPack_sel = 0;
	else
		ctrl.uCTRL_uvPack_sel = 1;

	if ((hrx_dev->ui_md_dump_enable) &&
		(hrx_dev->ui_tunnel_mode == 1))
		ctrl.uCTRL_uvPack_sel = 0;

	ctrl.uCTRL_scl_InFifo_Yctrl = 1;
	ctrl.uCTRL_scl_InFifo_UVctrl = 1;
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
		(hrx_dev->vip_base + RA_HDMIRX_PIPE_CTRL), ctrl.u32);
	hrx_dev->ui_pipe_ctrl = ctrl.u32;

	/* Added vip_intr_num check below to avoid overwrite of RA_HDMIRX_PIPE_CTRL0 */
	if (hrx_dev->vip_intr_num < 10) {
		ctrl0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL0);
		ctrl0.uCTRL0_scl_OFifo_fullCtrl = 0;

		/* ctrl0.uCTRL0_hdmirx_metadata_wrbk = 1;
		 * set edr_hdmirx_sel = 0 for native mode to fix slight chroma shift
		 * with 4K input
		 */
		if (hrx_dev->ui_md_dump_enable)
			ctrl0.uCTRL0_edr_hdmirx_sel = hrx_dev->ui_tunnel_mode;
		bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
			(hrx_dev->vip_base + RA_HDMIRX_PIPE_CTRL0),
			ctrl0.u32);
	}

	cfg4.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CFG4);
	cfg4.uCFG4_scl_OFifoY_aEmpCtrl = 1;
	cfg4.uCFG4_scl_OFifoUV_aEmpCtrl = 1;

	if (ginputmode == VIP_IMODE0_8BIT_YCBCR ||
			ginputmode == VIP_IMODE1_10BIT_YCBCR ||
			ginputmode == VIP_IMODE2_12BIT_YCBCR ||
			ginputmode == VIP_IMODE6_8BIT_RGB ||
			ginputmode == VIP_IMODE7_10BIT_RGB ||
			ginputmode == VIP_IMODE8_12BIT_RGB) {
		if (goutputmode == VIP_OMODE0_8BIT_YC ||
			goutputmode == VIP_OMODE1_10BIT_YC ||
			goutputmode == VIP_OMODE11_12BIT_YC
		)
			cfg4.uCFG4_uv422_444_sel = 0;
		else
			cfg4.uCFG4_uv422_444_sel = 1;
		cfg4.uCFG4_uvrd_en_mask = 0;
		if (goutputmode == VIP_OMODE5_8BIT_YCBCR ||
			goutputmode == VIP_OMODE6_10BIT_YCBCR ||
			goutputmode == VIP_OMODE7_12BIT_YCBCR ||
			goutputmode == VIP_OMODE8_8BIT_RGB ||
			goutputmode == VIP_OMODE9_10BIT_RGB ||
			goutputmode == VIP_OMODE10_12BIT_RGB) {
			cfg4.uCFG4_uv422_444_sel = 1;
			cfg4.uCFG4_uvrd_en_mask = 1;
		}
	} else {
		cfg4.uCFG4_uv422_444_sel = 0;
		cfg4.uCFG4_uvrd_en_mask = 0;
	}

	if ((hrx_dev->ui_md_dump_enable) &&
		(hrx_dev->ui_tunnel_mode == 1))
		cfg4.uCFG4_uvrd_en_mask = 1;

	if (goutputmode == VIP_OMODE2_8BIT_YUV420 ||
		goutputmode == VIP_OMODE3_10BIT_YUV420 ||
		goutputmode == VIP_OMODE4_12BIT_YUV420)
		cfg4.uCFG4_UV_swap_regs = 1;
	else
		cfg4.uCFG4_UV_swap_regs = 0;

	if (ginputmode == VIP_IMODE0_8BIT_YCBCR ||
		ginputmode == VIP_IMODE1_10BIT_YCBCR ||
		ginputmode == VIP_IMODE2_12BIT_YCBCR ||
		ginputmode == VIP_IMODE6_8BIT_RGB ||
		ginputmode == VIP_IMODE7_10BIT_RGB ||
		ginputmode == VIP_IMODE8_12BIT_RGB)
		cfg4.uCFG4_mode_yc42x = 0;
	else
		cfg4.uCFG4_mode_yc42x = 1;
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
			hrx_dev->vip_base + RA_HDMIRX_PIPE_CFG4, cfg4.u32);

	ctrl4.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_VIPSCLTOP +
			RA_VIPSCLTOP_CTRL4);
	ctrl4.uCTRL4_out8_420sp_sel = 0;
	if (goutputmode == VIP_OMODE0_8BIT_YC)
		ctrl4.uCTRL4_out_422_sel = 1;
	else if (goutputmode == VIP_OMODE1_10BIT_YC)
		ctrl4.uCTRL4_out_422_sel = 2;
	else if (goutputmode == VIP_OMODE2_8BIT_YUV420) {
		ctrl4.uCTRL4_out_422_sel = 0;
		ctrl4.uCTRL4_out8_420sp_sel = 1;
	} else if (goutputmode == VIP_OMODE3_10BIT_YUV420)
		ctrl4.uCTRL4_out_422_sel = 0;
	else
		ctrl4.uCTRL4_out_422_sel = 0;

	if (ginputmode == VIP_IMODE5_12BIT_YC &&
		goutputmode == VIP_OMODE11_12BIT_YC)
		ctrl4.uCTRL4_out_422_sel = 1;

	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
		hrx_dev->vip_base + RA_HDMIRX_PIPE_VIPSCLTOP +
		RA_VIPSCLTOP_CTRL4, ctrl4.u32);

	return 0;
}
