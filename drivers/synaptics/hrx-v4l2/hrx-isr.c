// SPDX-License-Identifier: GPL-2.0
/* Copyright 2017 Synopsys Inc. */
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-isr.h"
#include "hrx-reg.h"
#include "hrx-vip.h"

int hrx_isr_handler(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;
	int hrx_intr = 0xff;
	CC_MSG_t msg;
	u32 irq = 0;
	u32 mu0_mask, mu2_mask;
//	u32 hdcp_stat;

	u32 mu0_stat = hrx_reg_get_int_val(hrx_dev, HDMI_MAINUNIT_0_INT_STATUS,
		HDMI_MAINUNIT_0_INT_MASK_N);
	u32 mu2_stat = hrx_reg_get_int_val(hrx_dev, HDMI_MAINUNIT_2_INT_STATUS,
		HDMI_MAINUNIT_2_INT_MASK_N);
	u32 pkt0_stat = hrx_reg_get_int_val(hrx_dev, HDMI_PKT_0_INT_STATUS,
		HDMI_PKT_0_INT_MASK_N);
	u32 avp1_stat = hrx_reg_get_int_val(hrx_dev, HDMI_AVPUNIT_1_INT_STATUS,
		HDMI_AVPUNIT_1_INT_MASK_N);
	u32 hdcp_stat = hrx_reg_get_int_val(hrx_dev, HDMI_HDCP_INT_STATUS,
		HDMI_HDCP_INT_MASK_N);

	irq = (mu0_stat & MAINUNIT_0_INTR)|((mu2_stat & MAINUNIT_2_INTR) << 2)|
			(avp1_stat & AVPUNIT1_INTR)|(hdcp_stat & HDCP_INTR) |
			(pkt0_stat & PKT_0_INTR) | ((pkt0_stat & PKT_0_ACR) << 27) |
			((pkt0_stat & HDMI_PKT_0_INT_STATUS_GCP) << 25) |
			((pkt0_stat & HDMI_PKT_0_INT_STATUS_AUDIF) << 12);

	if ((mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_TMDSQP_CK_OFF) ||
		(mu2_stat & HDMI_MAINUNIT_2_INT_STATUS_TMDSVALID_STABLE)) {
		hrx_intr = HDMIRX_INTR_SYNC;
		mu0_mask = hrx_reg_read(hrx_dev, HDMI_MAINUNIT_0_INT_MASK_N);
		mu2_mask = hrx_reg_read(hrx_dev, HDMI_MAINUNIT_2_INT_MASK_N);
		mu0_mask = (mu0_mask & (~mu0_stat));
		mu2_mask = (mu2_mask & (~mu2_stat));
		hrx_reg_write(hrx_dev, mu0_mask, HDMI_MAINUNIT_0_INT_MASK_N);
		hrx_reg_write(hrx_dev, mu2_mask, HDMI_MAINUNIT_2_INT_MASK_N);
	} else if ((pkt0_stat & HDMI_PKT_0_INT_STATUS_AVIIF) ||
			 (pkt0_stat & HDMI_PKT_0_INT_STATUS_VSIF)) {
		hrx_intr = HDMIRX_INTR_PKT;
	} else if ((mu2_stat & HDMI_MAINUNIT_2_INT_STATUS_AUDPLL_LOCK_STABLE) ||
		(mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_AUDIO_CK_OFF) ||
		(mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_AUDIO_CK_LOCKED) ||
		(avp1_stat & HDMI_IRQ_AUDIO_AVP1_FLAG) ||
		(pkt0_stat & HDMI_IRQ_AUDIO_PKT0_FLAG) ||
		(pkt0_stat & HDMI_PKT_0_INT_STATUS_AUDIF)) {
		HRX_LOG(HRX_DRV_DEBUG, "HDMIRX_INTR_CHNL_STS %d: %d : %d : %d : %d : %d\n",
			mu2_stat & HDMI_MAINUNIT_2_INT_STATUS_AUDPLL_LOCK_STABLE,
			mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_AUDIO_CK_OFF,
			mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_AUDIO_CK_LOCKED,
			avp1_stat & HDMI_IRQ_AUDIO_AVP1_FLAG,
			pkt0_stat & HDMI_IRQ_AUDIO_PKT0_FLAG,
			pkt0_stat & HDMI_PKT_0_INT_STATUS_AUDIF);
		hrx_intr = HDMIRX_INTR_CHNL_STS;
	} else if ((hdcp_stat & HDMI_HDCP_INT_STATUS_ENCDIS) ||
			 (hdcp_stat & HDMI_HDCP_INT_STATUS_ENCEN)) {
		hrx_intr = HDMIRX_INTR_HDCP;
	}

	/* HDMI Rx interrupt */
	if (hrx_intr != 0xff) {
		msg.m_MsgID = 0;
		msg.m_Param1 = hrx_intr;
		msg.m_Param2 = irq;
		rc = AMPMsgQ_Add(&hrx_dev->hrx_msg_queue, &msg);
		if (likely(rc == S_OK))
			up(&hrx_dev->hrx_sem);
	}

	return 0;
}

static int hrx_isr_task(void *param)
{
	CC_MSG_t msg;
	HRESULT rc = S_OK;
	struct syna_hrx_v4l2_dev *hrx_dev = (struct syna_hrx_v4l2_dev *)param;

#if 0
	struct task_struct *task = current;
	struct sched_param task_param = {.sched_priority = 1};

	sched_setscheduler(task, SCHED_RR, &task_param);
#endif

	while (!kthread_should_stop()) {
		rc = down_interruptible(&hrx_dev->hrx_sem);
		if (unlikely(rc < 0))
			return rc;
		rc = AMPMsgQ_ReadTry(&hrx_dev->hrx_msg_queue, &msg);
		if (unlikely(rc != S_OK)) {
			HRX_LOG(HRX_DRV_DEBUG, "%s:%d Failed to read from msgQ\n", __func__, __LINE__);
			return -EFAULT;
		}
		AMPMsgQ_ReadFinish(&hrx_dev->hrx_msg_queue);
		hrx_isr_state_update(hrx_dev, msg);
	}

	return 0;
}

int hrx_create_isr_task(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned int ret;

	sema_init(&hrx_dev->hrx_sem, 0);

	ret = AMPMsgQ_Init(&hrx_dev->hrx_msg_queue, HRX_ISR_MSGQ_SIZE);
	if (unlikely(ret != S_OK)) {
		HRX_LOG(HRX_DRV_ERROR, "hrx_msg_queue init: failed, err:%8x\n", ret);
		return -1;
	}

	hrx_dev->hrx_isr_task = kthread_run(hrx_isr_task, hrx_dev, "HRX ISR Thread");
	if (IS_ERR(hrx_dev->hrx_isr_task))
		return -1;

	return 0;
}

void hrx_stop_isr_task(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned int ret;
	CC_MSG_t msg;

	up(&hrx_dev->hrx_sem);
	kthread_stop(hrx_dev->hrx_isr_task);
	do {
		ret = AMPMsgQ_DequeueRead(&hrx_dev->hrx_msg_queue, &msg);
	} while (likely(ret == 1));
	sema_init(&hrx_dev->hrx_sem, 0);
	ret = AMPMsgQ_Destroy(&hrx_dev->hrx_msg_queue);
	if (unlikely(ret != S_OK))
		HRX_LOG(HRX_DRV_ERROR, "%s:%d: HRX MsgQ Destroy FAILED, err:%8x\n", __func__, __LINE__, ret);
}
