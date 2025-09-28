// SPDX-License-Identifier: GPL-2.0
/* Copyright 2017 Synopsys Inc. */
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-vip-isr.h"
#include "hrx-reg.h"
#include "hrx-vip-scl.h"
#include "hrx-video.h"
#include "hrx-vip.h"
#include "hrx-aip.h"
#include "hrx-dmaheap.h"

#define CHECK_INTR_CNT
#define VIP_DHUB_BURST_OPT (256)
#define VIP_DHUB_BURST_MMU_ONLY (64)
#define VIP_AVIOMTR_DHUB_ADDR (0)
#define VIP_AVIOMTR_DHUB_STRIDE (262144)
#define START_2NDDMA_BCM(dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, pbcmbuf) \
	dhub2nd_channel_cfg_vip_bcm((HDL_dhub2d *)dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, 0, 0, 0, 1, pbcmbuf)

int vip_isr_handler(struct syna_hrx_v4l2_dev *hrx_dev, u32 intr_id)
{
	int rc;
	CC_MSG_t msg;

	msg.m_MsgID = 1 << intr_id;
	msg.m_Param1 = 0;
	msg.m_Param2 = 0;

	rc = AMPMsgQ_Add(&hrx_dev->vip_msg_queue, &msg);
	if (rc == S_OK)
		up(&hrx_dev->vip_sem);

	return 0;
}

static void vip_disable_vsync_intr(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 avp1_mask;

	avp1_mask = glb_reg_read(MEMMAP_AVIO_REG_BASE +
		AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE + HDMI_AVPUNIT_1_INT_MASK_N);

	if (avp1_mask & HDMI_AVPUNIT_1_INT_MASK_N_DEFRAMER_VSYNC_MASK) {
		avp1_mask =  (avp1_mask &
			~(HDMI_AVPUNIT_1_INT_MASK_N_DEFRAMER_VSYNC_MASK));
		glb_reg_write(MEMMAP_AVIO_REG_BASE +
			AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE + HDMI_AVPUNIT_1_INT_MASK_N,
			avp1_mask);

		glb_reg_write(MEMMAP_AVIO_REG_BASE +
			 AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE +
			 HDMI_AVPUNIT_1_INT_CLEAR,
			 HDMI_AVPUNIT_1_INT_MASK_N_DEFRAMER_VSYNC_MASK);
	}
}


static int vip_set_dhub_ch_config(struct syna_hrx_v4l2_dev *hrx_dev, int dmaId, u8 uchInverseScanMode, s32 mtuByte)
{
	HDL_dhub *dhub = hrx_dev->dhub_vpp;
	u32 RegAddr;
	T32dHubChannel_CFG cfg, hCfg;

	switch (mtuByte) {
	case dHubChannel_CFG_MTU_8byte:
		dhub->MTUb[dmaId] = 3;
		break;
	case dHubChannel_CFG_MTU_32byte:
		dhub->MTUb[dmaId] = 5;
		break;
	case dHubChannel_CFG_MTU_128byte:
		dhub->MTUb[dmaId] = 7;
		break;
	case dHubChannel_CFG_MTU_1024byte:
		dhub->MTUb[dmaId] = 10;
		break;
	case dHubChannel_CFG_MTU_4096byte:
		dhub->MTUb[dmaId] = 12;
		break;
	}

	Dhub_GetChannelInfo(hrx_dev->vip_dhub2d, dmaId, &hCfg);

	cfg.u32 = 0;
	cfg.uCFG_MTU = mtuByte;
	cfg.uCFG_QoS = hCfg.uCFG_QoS;
	cfg.uCFG_selfLoop = hCfg.uCFG_selfLoop;
	switch (uchInverseScanMode) {
	case 0:
		//Normal Scanning
		cfg.uCFG_hScan = 0; cfg.uCFG_vScan = 0;
		break;
	case 1:
		//Only H inverse scan
		cfg.uCFG_hScan = 1;
		cfg.uCFG_vScan = 0;
		break;
	case 2:
		//Only V inverse Scan
		cfg.uCFG_hScan = 0;
		cfg.uCFG_vScan = 1;
		break;
	case 3:
		//Both HV inverse
		cfg.uCFG_hScan = 1; cfg.uCFG_vScan = 1;
		break;
	default:
		HRX_LOG(VIP_ERROR, "%s: Invalid scanmode\n", __func__);
		return -1;
	}

	RegAddr = dhub->ra + RA_dHubReg_ARR + dmaId*sizeof(SIE_dHubChannel);
	bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf, RegAddr, cfg.u32);

	Dhub_SetChannelMtu(hrx_dev->vip_dhub2d, dmaId, mtuByte);

	return 0;
}

typedef struct VIP_DHUB_CFG_2NDDMA_PARAM_s {
	/* Input params */
	HDL_dhub2d *dhubId;
	int dmaId;
	u32 addr;
	s32 width; /* Number of bytes in a line or tile line */
	s32 stride; /* Number of bytes in a line or tile line */
	s32 height; /* Number of lines or tile lines */
	BCMBUF *pbcmbuf;
	u8 mtrREn;
	u8 updSemId;
	u8 chkSemId;
} VIP_DHUB_CFG_2NDDMA_PARAM;


static u32 vip_dhub_cfg_2nd_dma(struct syna_hrx_v4l2_dev *hrx_dev, VIP_DHUB_CFG_2NDDMA_PARAM *pstCfg2NDDMAParam)
{
	s32 burst = 0, step1 = 0, size1 = 0, step2 = 0, size2 = 0;
	s32 mtu = 0;
	u32 RetVal =0;
	u32 addr;

	if (!(pstCfg2NDDMAParam->width & (VIP_DHUB_BURST_OPT-1))) {
		/* if width is multiple of VIP_DHUB_BURST_OPT. This is a must for compressed input*/
		/* H flip and HV flip can't support 256B burst */
		burst = VIP_DHUB_BURST_OPT;
		mtu   = dHubChannel_CFG_MTU_4096byte;
	} else if (!(pstCfg2NDDMAParam->width & (VIP_DHUB_BURST_MMU_ONLY-1))) {
		/* MMU only enabled case(eg. 420SP on BG5CTpZ1), width should be multiple of VIP_DHUB_BURST_MMU_ONLY */
		burst = VIP_DHUB_BURST_MMU_ONLY;
		mtu   = dHubChannel_CFG_MTU_1024byte;
	} else {
		/* Uncompressed and widht is not a multiple of VIP_DHUB_BURST_OPT */
		burst = pstCfg2NDDMAParam->width;
		mtu   = dHubChannel_CFG_MTU_128byte;
	}

	if (pstCfg2NDDMAParam->mtrREn) {
		addr = VIP_AVIOMTR_DHUB_ADDR;
		step2 = VIP_AVIOMTR_DHUB_STRIDE;
	} else {
		addr  = pstCfg2NDDMAParam->addr;
		step2 = pstCfg2NDDMAParam->stride;
	}
	step1 = burst;
	size1 = pstCfg2NDDMAParam->width / burst;
	size2 = pstCfg2NDDMAParam->height;

	vip_set_dhub_ch_config(hrx_dev, pstCfg2NDDMAParam->dmaId, 0, mtu);

	if (pstCfg2NDDMAParam->mtrREn == 0)
		RetVal =  START_2NDDMA_BCM(pstCfg2NDDMAParam->dhubId, pstCfg2NDDMAParam->dmaId,
						addr, burst, step1, size1, step2, size2,
						pstCfg2NDDMAParam->pbcmbuf);

	return RetVal;
}

static int process_buffer_task(void *param)
{
	int ret = 0;
	struct syna_hrx_v4l2_dev *hrx_dev = (struct syna_hrx_v4l2_dev *)param;
	void *frame_descr;
	while (!kthread_should_stop()) {
		ret = down_interruptible(&hrx_dev->process_buffer_sem);
		if (unlikely(ret < 0))
			goto exit;

		if (hrx_dev->vip_status == VIP_STATUS_EXIT) {
			goto exit;
		}

		if (!kfifo_is_empty(&hrx_dev->processed_buffer_queue)) {
			ret = kfifo_out(&hrx_dev->processed_buffer_queue, &frame_descr, sizeof(void *));
			vb2_buffer_done((struct vb2_buffer *)frame_descr, VB2_BUF_STATE_DONE);
		}
	}

exit:
	// Emptying Queue
	while (!kfifo_is_empty(&hrx_dev->processed_buffer_queue))
		ret = kfifo_out(&hrx_dev->processed_buffer_queue, &frame_descr, sizeof(void *));
	return ret;
}
static bool check_invalid_frame (unsigned char * data, int len, int chk_val)
{
	int i,null_chk = 0, ff_chk = 0xff;

	if(data == NULL)
		return false;

	for(i = 0; i < len ; i++) {
		ff_chk &= data[i];
		null_chk |= data[i];
	}
	if(chk_val == 0)
		return (null_chk == 0x00);
	else
		return (ff_chk == 0xff);
}

static void syna_hrx_buf_validate_and_process(struct syna_hrx_v4l2_dev *hrx_dev, void * frame_descr)
{
	bool dis_frame = false;
	static int null_data_chk;
	static int discard_frame_count;
	unsigned char *Yaddr = NULL;
	unsigned char *UVaddr = NULL;

	int width_byte = (hrx_dev->format.width + 7) / 8 * hrx_dev->vip_bits_per_pixel;

	Yaddr = (unsigned char*)syna_hrx_dh_plane_vaddr(frame_descr, 0);
	if ((hrx_dev->vip_omode == VIP_OMODE2_8BIT_YUV420) ||
		(hrx_dev->vip_omode == VIP_OMODE3_10BIT_YUV420) ||
		(hrx_dev->vip_omode == VIP_OMODE4_12BIT_YUV420))
		UVaddr = Yaddr + (width_byte * hrx_dev->format.height);

	if(discard_frame_count < 5)
		goto trigger;

	if ((null_data_chk < 5) &&
		(check_invalid_frame((unsigned char *)Yaddr, 10, 0) ||
		(check_invalid_frame((unsigned char *)UVaddr, 10, 0)) ||
		(check_invalid_frame((unsigned char *)UVaddr, 10, 1)))) {
		dis_frame = true;
		if(++null_data_chk == 5) {
			hrx_dev->null_data_count++;
			HRX_LOG(VIP_DEBUG,"Triggering Scalar Reset due to wrong color \n");
			hrx_dev->trig_scl_reset = true;
		}
	}

trigger:
	if(hrx_dev->trig_scl_reset) {
			vip_scl_reset(hrx_dev);
			HRX_LOG(VIP_DEBUG,"Triggered Scalar Reset \n");
			hrx_dev->trig_scl_reset = false;
			dis_frame = true;
			null_data_chk = 0;
			discard_frame_count = 0;
	}
	if(discard_frame_count++ < 5)
		dis_frame = true;

	if(dis_frame || (hrx_dev->HrxState != HRX_STATE_ALL_STABLE))
		syna_hrx_buf_unused(hrx_dev, (struct vb2_buffer *)frame_descr);
	else
		syna_hrx_buf_processed(hrx_dev, (struct vb2_buffer *)frame_descr);
}

static int vip_isr_service(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int ret = 0;
	void *pframe_buf = NULL;
	int *cfgQ = NULL;
	int width, height, width_byte;
	int stride_byte;
	static int top;
	int pixel_bits;
	static int scl_count;
	int ginputmode, goutputmode;
	struct vb2_buffer *buf;
	phys_addr_t phys_addr;

	VIP_DHUB_CFG_2NDDMA_PARAM stCfg2NDDMAParam;
	T32HDMI_RX_WRAP_HDMI_CONTROLLER_STATUS status;

	mutex_lock(&hrx_dev->vip_mutex);

	if (hrx_dev->vip_status != VIP_STATUS_START) {
		mdelay(2);
		HRX_LOG(VIP_ERROR, "VIP is not running\n");
		ret = -EBUSY;
		goto EXIT;
	}

	ginputmode = hrx_dev->vip_imode;
	goutputmode = hrx_dev->vip_omode;
	hrx_dev->vip_intr_num++;

	pixel_bits = hrx_dev->vip_bits_per_pixel;

	/* reset current VBI BCM buffer */
	bcmbuf_reset(hrx_dev->pcurr_vbi_bcm_buf);
	/* reset current DHUB DMA channel CFG */
	bcmbuf_DHUB_CFGQ_Reset(hrx_dev->pcurr_dhub_dma_cfgQ);
	/* reset current DHUB BCM channel CFG */
	bcmbuf_DHUB_CFGQ_Reset(hrx_dev->pcurr_dhub_bcm_cfgQ);

	/* real ISR comes here */
	hrx_dev->vip_signal_status = VIP_SIGNAL_STABLE;

	/* Adding clear seq bcm helped to get video for 1080p60.
	 * so opened it from USEBCM
	 */
	dhub2nd_channel_clear_seq_bcm(hrx_dev->vip_dhub2d,
		hrx_dev->dvi_dmaWID[0], (VIP_BCMBUF *) hrx_dev->pcurr_vbi_bcm_buf);
	dhub2nd_channel_clear_seq_bcm(hrx_dev->vip_dhub2d,
		hrx_dev->dvi_dmaWID[1], (VIP_BCMBUF *) hrx_dev->pcurr_vbi_bcm_buf);

#ifdef VIP_DEBUG
	if (hrx_dev->vip_intr_num == 1)
		HRX_LOG(VIP_INFO, "VIP_EVENT_STABLE_SYNC\n");
#endif

	if (hrx_dev->vip_signal_status == VIP_SIGNAL_STABLE) {
		if (hrx_dev->vip_field_flag) {
			status.reg = glb_reg_read(MEMMAP_AVIO_REG_BASE +
				AVIO_MEMMAP_AVIO_HDMIRX_WRAP_REG_BASE +
				RA_HDMI_RX_WRAP_HDMI_CONTROLLER_STATUS);
			hrx_dev->vip_top = (status.uHDMI_CONTROLLER_STATUS_opvo_field == 2?0:1);
			top = hrx_dev->vip_top;
		}

		/* 4th stable frame: send frame to display/recycle frame */
		if (vip_frmq_isdirty(&hrx_dev->frmq)) {
			// a new frame is captured
			vip_frmq_push_commit(&(hrx_dev->frmq));

			/* Observed DHUB write was not completed in some cases.
			 * So let give DHUB one more frame time for write operation
			 * so that no partial frame will be notified to AVIN
			 */

			if (!hrx_dev->vip_first_frame)
				hrx_dev->vip_first_frame = 1;
			else {
				void *frame_descr;
				vip_frmq_pop(&hrx_dev->frmq, &frame_descr);
				syna_hrx_buf_validate_and_process(hrx_dev,frame_descr);
			}
		}

		/* 3rd stable frame: push frame into FIFO */
		if (hrx_dev->vip_dma_cmd_issued) {
			/* for interlace mode, only push FIFO
			 * when 2 fields received
			 */
			if ((hrx_dev->vip_field_flag && top) ||
				(!hrx_dev->vip_field_flag)) {
				/* new DMA is in execution,
				 * update temporary write pointer
				 */

				if (vip_frmq_push(&hrx_dev->frmq, hrx_dev->vip_curr_frame_descr)) {
					/* if FIFO is not overflow, clear "vip_curr_frame_descr" */
					hrx_dev->vip_curr_frame_descr = NULL;
				}
			}
		}

		/* 2nd stable frame: allocate buffer/setup DMA */
		if (vip_frmq_isfull(&hrx_dev->frmq)) {
			/* frame queue is full */
			hrx_dev->vip_dma_cmd_issued = 0;
		} else {
			bool scl_update_required = false;

			width = hrx_dev->vip_hwidth;
			height = hrx_dev->vip_vheight;

			if (hrx_dev->vip_enable_scaler && (scl_count == 0)) {
				/* TODO: Update only once */
				scl_update_required = true;
				scl_count = 1;
			}

			if (hrx_dev->vip_enable_scaler) {
				/* TODO: Update only once */
				vip_prog_scl(hrx_dev, &width, &height, true);
				scl_update_required = false;
			} else {
				if (ginputmode == VIP_IMODE9_8BIT_YUV420 ||
					ginputmode == VIP_IMODE10_10BIT_YUV420 ||
					ginputmode == VIP_IMODE11_12BIT_YUV420) {
					T32HDMIRX_PIPE_CTRL0 ctrl0;

					ctrl0.u32 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_CTRL0);
					ctrl0.uCTRL0_scl_OFifo_fullCtrl = 0;
					ctrl0.uCTRL0_rdy0_sts_ctrl = 1;
					ctrl0.uCTRL0_rdy1_sts_ctrl = 1;
					bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
						hrx_dev->vip_base + RA_HDMIRX_PIPE_CTRL0,
						ctrl0.u32);
				}
			}

			width_byte = (width + 7) / 8 * pixel_bits;

			stride_byte = width_byte;

			/* only allocate new buffer when previous
			 * one was successfully pushed into FIFO.
			 */
			if (hrx_dev->vip_curr_frame_descr == NULL) {
				int buf_size;

				pframe_buf = NULL;

				if ((hrx_dev->vip_omode == VIP_OMODE2_8BIT_YUV420) ||
					(hrx_dev->vip_omode == VIP_OMODE3_10BIT_YUV420) ||
					(hrx_dev->vip_omode == VIP_OMODE4_12BIT_YUV420)) {
					buf_size = (width_byte * (hrx_dev->vip_field_flag ? height * 2 : height) * 3) / 2;
				} else {
					buf_size = width_byte * (hrx_dev->vip_field_flag ? height * 2 : height);
				}

				buf = syna_hrx_get_free_buf(hrx_dev);

				if (buf) {
					void *handle = syna_hrx_dh_plane_cookie(buf, 0);

					phys_addr = (u64)syna_hrx_get_frame_phyaddr(handle);
					hrx_dev->vip_curr_frame_descr = buf;
				} else {
					HRX_LOG(VIP_ERROR, "Failed to get buffer from v4l2\n");
					hrx_dev->vip_dma_cmd_issued = 0;
					goto EXIT;
				}
			}
			/* setup DHUB DMA */
			cfgQ = hrx_dev->pcurr_dhub_dma_cfgQ->addr +
				hrx_dev->pcurr_dhub_dma_cfgQ->len * 2;

			if (hrx_dev->vip_field_flag) {
				stCfg2NDDMAParam.dhubId  = hrx_dev->vip_dhub2d;
				stCfg2NDDMAParam.dmaId   = hrx_dev->dvi_dmaWID[0];
				stCfg2NDDMAParam.addr    = (u32) phys_addr;
				stCfg2NDDMAParam.width   = width_byte;
				stCfg2NDDMAParam.stride  = stride_byte*2;
				stCfg2NDDMAParam.height  = height;
				stCfg2NDDMAParam.pbcmbuf = hrx_dev->pcurr_vbi_bcm_buf;
				stCfg2NDDMAParam.mtrREn  = 0;
				hrx_dev->pcurr_dhub_dma_cfgQ->len += vip_dhub_cfg_2nd_dma(hrx_dev, &stCfg2NDDMAParam);
			} else {
				if ((hrx_dev->vip_omode == VIP_OMODE2_8BIT_YUV420) ||
					(hrx_dev->vip_omode == VIP_OMODE3_10BIT_YUV420)) {
					T32HDMIRX_PIPE_tg_ctrl_420 tg_ctrl_420;

					stCfg2NDDMAParam.dhubId  = hrx_dev->vip_dhub2d;
					stCfg2NDDMAParam.dmaId   = hrx_dev->dvi_dmaWID[1];
					stCfg2NDDMAParam.addr    = (u32) (phys_addr + (stride_byte * height));
					stCfg2NDDMAParam.width   = width_byte;
					stCfg2NDDMAParam.stride  = stride_byte;
					stCfg2NDDMAParam.height  = height/2;
					stCfg2NDDMAParam.pbcmbuf = hrx_dev->pcurr_vbi_bcm_buf;
					stCfg2NDDMAParam.mtrREn  = 0;
					hrx_dev->pcurr_dhub_dma_cfgQ->len += vip_dhub_cfg_2nd_dma(hrx_dev, &stCfg2NDDMAParam);

					if (ginputmode == VIP_IMODE9_8BIT_YUV420 ||
						ginputmode == VIP_IMODE10_10BIT_YUV420 ||
						ginputmode == VIP_IMODE11_12BIT_YUV420) {
						tg_ctrl_420.u32 = vip_reg_read(hrx_dev,
							RA_HDMIRX_PIPE_tg_ctrl_420);
						tg_ctrl_420.utg_ctrl_420_rd_start = 1;
						bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
							hrx_dev->vip_base +
							RA_HDMIRX_PIPE_tg_ctrl_420,
							tg_ctrl_420.u32);
					}
					bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
						hrx_dev->vip_base +
						RA_HDMIRX_PIPE_VIP_CRCH_WR +
						RA_WriteClient_Wr, 2);
					bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
						hrx_dev->vip_base +
						RA_HDMIRX_PIPE_VIP_CRCH_WR +
						RA_WriteClient_Wr, 0);
					bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
						hrx_dev->vip_base +
						RA_HDMIRX_PIPE_VIP_CRCH_WR +
						RA_WriteClient_Wr, 1);
				}

				stCfg2NDDMAParam.dhubId  = hrx_dev->vip_dhub2d;
				stCfg2NDDMAParam.dmaId   = hrx_dev->dvi_dmaWID[0];
				stCfg2NDDMAParam.addr    = (u32) phys_addr;
				stCfg2NDDMAParam.width   = width_byte;
				stCfg2NDDMAParam.stride  = stride_byte;
				stCfg2NDDMAParam.height  = height;
				stCfg2NDDMAParam.pbcmbuf = hrx_dev->pcurr_vbi_bcm_buf;
				stCfg2NDDMAParam.mtrREn  = 0;
				hrx_dev->pcurr_dhub_dma_cfgQ->len += vip_dhub_cfg_2nd_dma(hrx_dev, &stCfg2NDDMAParam);
			}

			bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
				hrx_dev->vip_base + RA_HDMIRX_PIPE_VIP_WR +
				RA_WriteClient_Wr, 2);
			bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
				hrx_dev->vip_base + RA_HDMIRX_PIPE_VIP_WR +
				RA_WriteClient_Wr, 0);
			bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
				hrx_dev->vip_base + RA_HDMIRX_PIPE_VIP_WR +
				RA_WriteClient_Wr, 1);
			/* VIP-Out TG is not used for YUV420.
			 * Use Y-TG/UV-TG for YUV420 if SCL is enabled.
			 * Use VIP-In TG if SCL is bypassed
			 */
			if ((goutputmode != VIP_OMODE2_8BIT_YUV420 &&
				goutputmode != VIP_OMODE3_10BIT_YUV420)) {
				bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
					hrx_dev->vip_base + RA_HDMIRX_PIPE_VIP_CTRL, 2);
				bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
					hrx_dev->vip_base + RA_HDMIRX_PIPE_VIP_CTRL, 0);
				bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
					hrx_dev->vip_base + RA_HDMIRX_PIPE_VIP_CTRL, 1);
			}
			if (hrx_dev->vip_enable_scaler) {
				T32VIPSCLTOP_CTRL0 stVipSclTopCtrl0;

				stVipSclTopCtrl0.u32 = 0;
				stVipSclTopCtrl0.uCTRL0_clear = 1;
				stVipSclTopCtrl0.uCTRL0_Y_otg_clear = 1;
				stVipSclTopCtrl0.uCTRL0_UV_otg_clear = 1;
				bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
					hrx_dev->vip_base + RA_HDMIRX_PIPE_VIPSCLTOP +
					RA_VIPSCLTOP_CTRL0, stVipSclTopCtrl0.u32);
				stVipSclTopCtrl0.u32 = 0;
				bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
					hrx_dev->vip_base + RA_HDMIRX_PIPE_VIPSCLTOP +
				RA_VIPSCLTOP_CTRL0, stVipSclTopCtrl0.u32);
				stVipSclTopCtrl0.uCTRL0_Y_otg_start = 1;
				stVipSclTopCtrl0.uCTRL0_UV_otg_start = 1;
				stVipSclTopCtrl0.uCTRL0_start_scl = 1;
				bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
					hrx_dev->vip_base + RA_HDMIRX_PIPE_VIPSCLTOP +
					RA_VIPSCLTOP_CTRL0, stVipSclTopCtrl0.u32);
			}
			bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
				hrx_dev->vip_base + RA_HDMIRX_PIPE_tg_ctrl, 2);
			bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
				hrx_dev->vip_base + RA_HDMIRX_PIPE_tg_ctrl, 0);
			bcmbuf_write(hrx_dev->pcurr_vbi_bcm_buf,
				hrx_dev->vip_base + RA_HDMIRX_PIPE_tg_ctrl, 1);
			if (hrx_dev->vip_intr_num == 1)
				hrx_dev->vip_first_intr = 1;
			/* generate and issue a new DMA cmd
			 * this cmd will be triggered at next VDE falling edge
			 */
			hrx_dev->vip_dma_cmd_issued = 1;
		}
	} else if (hrx_dev->vip_signal_status == VIP_SIGNAL_UNSTABLE) {
		HRX_LOG(VIP_INFO, "Entered unstable state of VIP ISR\n");
		hrx_dev->vip_signal_status = VIP_SIGNAL_INTERMEDIATE;
		/* read SSMON status/feedback at the same time
		 * we need to read 3 times to get correct status
		 */

	} else if (hrx_dev->vip_signal_status == VIP_SIGNAL_INTERMEDIATE) {
		HRX_LOG(VIP_INFO, "Entered intermediate state of VIP ISR\n");
		hrx_dev->vip_signal_status = VIP_SIGNAL_STABLE;
	}

	/* push current DHUB DMA channel CFG info to current BCM CFGQ*/
	bcmbuf_CFGQ_To_CFGQ(hrx_dev->vip_ag_dhub2d,
		avioDhubChMap_aio64b_BCM_R,
		BCM_SCHED_Q13,
		hrx_dev->pcurr_dhub_dma_cfgQ,
		hrx_dev->pcurr_dhub_bcm_cfgQ);

	/* push current VBI BCM BUF info to current BCM CFGQ*/
	bcmbuf_to_CFGQ(hrx_dev->vip_ag_dhub2d,
		avioDhubChMap_aio64b_BCM_R,
		BCM_SCHED_Q13,
		hrx_dev->pcurr_vbi_bcm_buf,
		hrx_dev->pcurr_dhub_bcm_cfgQ,
		hrx_dev->mem_list);

	bcmbuf_DHUB_CFGQ_Commit(hrx_dev->vip_ag_dhub2d,
		avioDhubChMap_aio64b_BCM_R,
		BCM_SCHED_Q5,
		hrx_dev->pcurr_dhub_bcm_cfgQ,
		0,
		DVI_VDE_INTR,
		&(hrx_dev->vip_first_intr));

	/* swap VBI BCM buffer */
	if (hrx_dev->pcurr_vbi_bcm_buf == &hrx_dev->vbi_bcm_buf[0])
		hrx_dev->pcurr_vbi_bcm_buf = &hrx_dev->vbi_bcm_buf[1];
	else
		hrx_dev->pcurr_vbi_bcm_buf = &hrx_dev->vbi_bcm_buf[0];

	/* swap DHUB DMA channel CFG */
	if (hrx_dev->pcurr_dhub_dma_cfgQ == &hrx_dev->dhub_dma_cfgQ[0])
		hrx_dev->pcurr_dhub_dma_cfgQ = &hrx_dev->dhub_dma_cfgQ[1];
	else
		hrx_dev->pcurr_dhub_dma_cfgQ = &hrx_dev->dhub_dma_cfgQ[0];

	/* swap DHUB BCM channel CFG */
	if (hrx_dev->pcurr_dhub_bcm_cfgQ == &hrx_dev->dhub_bcm_cfgQ[0])
		hrx_dev->pcurr_dhub_bcm_cfgQ = &hrx_dev->dhub_bcm_cfgQ[1];
	else
		hrx_dev->pcurr_dhub_bcm_cfgQ = &hrx_dev->dhub_bcm_cfgQ[0];

EXIT:
	mutex_unlock(&hrx_dev->vip_mutex);
	return ret;
}

static int vip_isr_task(void *param)
{
	CC_MSG_t msg;
	HRESULT rc = S_OK;
	struct syna_hrx_v4l2_dev *hrx_dev = (struct syna_hrx_v4l2_dev *)param;
	int EOFIntrCnt = 0;

#if 0
	struct task_struct *task = current;
	struct sched_param task_param = {.sched_priority = 1};

	sched_setscheduler(task, SCHED_RR, &task_param);
#endif

	while (!kthread_should_stop()) {
		rc = down_interruptible(&hrx_dev->vip_sem);
		if (unlikely(rc < 0))
			return rc;

		if (hrx_dev->vip_status == VIP_STATUS_EXIT) {
			HRX_LOG(VIP_DEBUG, "%s:%d Stopped\n", __func__, __LINE__);
			return 0;
		}

		memset(&msg, 0, sizeof(CC_MSG_t));
		rc = AMPMsgQ_ReadTry(&hrx_dev->vip_msg_queue, &msg);
		if (unlikely(rc != S_OK)) {
			HRX_LOG(VIP_ERROR, "%s:%d Failed to read from msgQ\n", __func__, __LINE__);
			return -EFAULT;
		}
		AMPMsgQ_ReadFinish(&hrx_dev->vip_msg_queue);

		if (msg.m_MsgID == (1 << avioDhubSemMap_aio64b_aio_intr7)) {
			if (hrx_dev->vip_omode != VIP_OMODE2_8BIT_YUV420 &&
				hrx_dev->vip_omode != VIP_OMODE3_10BIT_YUV420)
				vip_isr_service(hrx_dev);
			++EOFIntrCnt;

			if (hrx_dev->vip_intr_num < 5)
				vip_disable_vsync_intr(hrx_dev);
		} else if (msg.m_MsgID == (1 << avioDhubSemMap_aio64b_aio_intr10)) {
			if (hrx_dev->vip_omode == VIP_OMODE2_8BIT_YUV420 ||
				hrx_dev->vip_omode == VIP_OMODE3_10BIT_YUV420)
				vip_isr_service(hrx_dev);
			++EOFIntrCnt;

			/* Randomly VSYNC intr is remaining enabled and causing
			 * continuous VSYNC intr - leading to tmds intr miss
			 */
			if (hrx_dev->vip_intr_num < 5)
				vip_disable_vsync_intr(hrx_dev);
		} else if (msg.m_MsgID == (1 << avioDhubSemMap_aio64b_aio_intr11)) {
			if (!hrx_dev->vip_enable_scaler &&
				(hrx_dev->vip_omode == VIP_OMODE2_8BIT_YUV420 ||
				hrx_dev->vip_omode == VIP_OMODE3_10BIT_YUV420))
				vip_isr_service(hrx_dev);
			++EOFIntrCnt;

			if (hrx_dev->vip_intr_num < 5)
				vip_disable_vsync_intr(hrx_dev);
		} else if (msg.m_MsgID == (1 << avioDhubSemMap_aio64b_aio_intr8)) {
			T32HDMIRX_PIPE_CFG0 cfg0;

			if (hrx_dev->vip_intr_num == 0) {
				int ret = 0;

 #ifdef VIP_DEBUG
				HRX_LOG(VIP_INFO, "%d: HDMI VSYNC ISR Arrived\n", __LINE__);
 #endif
				/* Clear and disable the HDMI Rx VSync interrupt once it has
				 * come for first time and start the TG to enable the EOF intr
				 */
				vip_disable_vsync_intr(hrx_dev);
				cfg0.u32 = glb_reg_read(MEMMAP_AVIO_REG_BASE +
					AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE +
					RA_HDMIRX_PIPE_CFG0);
					cfg0.uCFG0_vsync_cnt_val = 2;
					cfg0.uCFG0_start_mask_reset = 0;
				glb_reg_write(MEMMAP_AVIO_REG_BASE +
					AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE +
					RA_HDMIRX_PIPE_CFG0, cfg0.u32);
					cfg0.uCFG0_start_mask_reset = 1;
				glb_reg_write(MEMMAP_AVIO_REG_BASE +
					AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE +
					RA_HDMIRX_PIPE_CFG0, cfg0.u32);
					cfg0.uCFG0_start_mask_reset = 0;
				glb_reg_write(MEMMAP_AVIO_REG_BASE +
					AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE +
					RA_HDMIRX_PIPE_CFG0, cfg0.u32);

				ret = vip_isr_service(hrx_dev);
				if (ret == -1) {
					unsigned int avp1_mask;
					/* Clear and re-enable VSYNC intr as VIP ISR is not ready */
					glb_reg_write(MEMMAP_AVIO_REG_BASE +
						AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE +
						HDMI_AVPUNIT_1_INT_CLEAR,
						HDMI_AVPUNIT_1_INT_MASK_N_DEFRAMER_VSYNC_MASK);

					avp1_mask = glb_reg_read(MEMMAP_AVIO_REG_BASE +
						AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE +
						HDMI_AVPUNIT_1_INT_MASK_N);
					avp1_mask =  (avp1_mask | HDMI_AVPUNIT_1_INT_MASK_N_DEFRAMER_VSYNC_MASK);
					glb_reg_write(MEMMAP_AVIO_REG_BASE +
						AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE +
						HDMI_AVPUNIT_1_INT_MASK_N,
						avp1_mask);
				}
			}
		} else if (msg.m_MsgID == (1 << avioDhubSemMap_aio64b_aio_intr9)) {
		} else
			HRX_LOG(VIP_ERROR, "Invalid ISR MSgType: %d\n", msg.m_MsgID);
	}
	return 0;
}

int vip_create_isr_task(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned int ret;

	sema_init(&hrx_dev->vip_sem, 0);

	ret = AMPMsgQ_Init(&hrx_dev->vip_msg_queue, HRX_ISR_MSGQ_SIZE);
	if (unlikely(ret != S_OK)) {
		HRX_LOG(VIP_ERROR, "vip_msg_queue init: failed, err:%8x\n", ret);
		goto exit;
	}

	hrx_dev->vip_isr_task = kthread_run(vip_isr_task, hrx_dev, "VIP ISR Thread");
	if (IS_ERR(hrx_dev->vip_isr_task))
		goto exit1;

	sema_init(&hrx_dev->process_buffer_sem, 0);
	hrx_dev->process_buffer_task = kthread_run(process_buffer_task, hrx_dev, "Process Buffer Thread");
	if (IS_ERR(hrx_dev->process_buffer_task))
		goto exit2;

	ret = kfifo_alloc(&hrx_dev->processed_buffer_queue, 16 * sizeof(void *), GFP_KERNEL);
	if (ret) {
		HRX_LOG(VIP_ERROR, "Failed to allocate memory for kfifo\n");
		goto exit3;
	}

	return 0;

exit3:
	up(&hrx_dev->process_buffer_sem);
	kthread_stop(hrx_dev->process_buffer_task);
exit2:
	up(&hrx_dev->vip_sem);
	kthread_stop(hrx_dev->vip_isr_task);
exit1:
	AMPMsgQ_Destroy(&hrx_dev->vip_msg_queue);
exit:
	return -1;
}

void vip_stop_isr_task(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned int ret;
	CC_MSG_t msg;

	up(&hrx_dev->vip_sem);
	kthread_stop(hrx_dev->vip_isr_task);

	up(&hrx_dev->process_buffer_sem);
	kthread_stop(hrx_dev->process_buffer_task);
	kfifo_free(&hrx_dev->processed_buffer_queue);

	do {
		ret = AMPMsgQ_DequeueRead(&hrx_dev->vip_msg_queue, &msg);
	} while (likely(ret == 1));

	sema_init(&hrx_dev->vip_sem, 0);
	ret = AMPMsgQ_Destroy(&hrx_dev->vip_msg_queue);
	if (unlikely(ret != S_OK))
		HRX_LOG(VIP_ERROR, "%s:%d: VIP MsgQ Destroy FAILED, err:%8x\n", __func__, __LINE__, ret);
}

static int vip_watcher_task(void *param)
{
	struct syna_hrx_v4l2_dev *hrx_dev = (struct syna_hrx_v4l2_dev *)param;
#ifndef CHECK_INTR_CNT
	unsigned int status1 = 0, status2 = 0;
#endif
	unsigned int vcount_vip_tg = 0;
	static unsigned int prev_vcount_vip_tg;
	static unsigned int restart_count;
	//static unsigned int no_change_count;

#if 0
	struct task_struct *task = current;
	struct sched_param task_param = {.sched_priority = 5};

	sched_setscheduler(task, SCHED_RR, &task_param);
#endif

	while (!kthread_should_stop()) {
		msleep_interruptible(300);

		/* No need to watch if VIP is not running */
		if (hrx_dev->vip_status != VIP_STATUS_START)
			continue;

		if(restart_count > 3) {
			HRX_LOG(VIP_DEBUG, "Toggle HPD\n");
			hrx_toggle_hpd(hrx_dev, 0);
			msleep(200);
			hrx_toggle_hpd(hrx_dev, 1);
			msleep(800);
			restart_count = 0;
		}

#ifdef CHECK_INTR_CNT
		vcount_vip_tg = hrx_dev->vip_intr_num;
#else
		if (hrx_dev->vip_omode == VIP_OMODE2_8BIT_YUV420 ||
			hrx_dev->vip_omode == VIP_OMODE3_10BIT_YUV420 ||
			hrx_dev->vip_omode == VIP_OMODE4_12BIT_YUV420) {
			status2 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_STATUS2);
			vcount_vip_tg = status2 & MSK32HDMIRX_PIPE_STATUS2_VIP_YTg;
		} else {
			status1 = vip_reg_read(hrx_dev, RA_HDMIRX_PIPE_STATUS1);
			vcount_vip_tg = status1 & MSK32HDMIRX_PIPE_STATUS1_HDRX_oTg;
		}
#endif
		if (prev_vcount_vip_tg != vcount_vip_tg) {
			prev_vcount_vip_tg = vcount_vip_tg;
			restart_count = 0;
		}
		else {
			if (syna_hrx_is_vip_stable(hrx_dev)) {
				HRX_LOG(VIP_DEBUG, "Restarting vip\n");
				vip_stop(hrx_dev);
				vip_start(hrx_dev);
				restart_count++;
			}
		}
	}
	return 0;
}

int vip_create_watcher_task(struct syna_hrx_v4l2_dev *hrx_dev)
{

	hrx_dev->vip_watcher_task = kthread_run(vip_watcher_task, hrx_dev, "VIP Watcher Thread");
	if (IS_ERR(hrx_dev->vip_watcher_task))
		return -1;

	return 0;
}

void vip_stop_watcher_task(struct syna_hrx_v4l2_dev *hrx_dev)
{
	//TODO: Fix to exit from thread
	kthread_stop(hrx_dev->vip_watcher_task);
}
