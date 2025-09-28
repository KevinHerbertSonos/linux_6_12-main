// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 * Author: Hsia-Jun(Randy) Li <randy.li@synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/compat.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>

#include "vpu_tee_wrap.h"
#include "vpu_common.h"
#include "vpu_fw.h"
#include "vpu_fw_data.h"
#include "vpu_srv.h"
#include "vpu_dmaheap.h"
#include "vpu_dec_drv.h"
#include "vpu_enc_drv.h"
#include <vpu_mfd.h>
#if IS_ENABLED(CONFIG_OPTEE)
#include "h1_irq.h"
#endif // CONFIG_OPTEE


static unsigned int vdec_enable_clk_gate;
static unsigned int venc_enable_clk_gate;
static struct syna_vpu_dev *g_dec_dev, *g_enc_dev;

static int vpu_set_clock_gate(const char *val, const struct kernel_param *param,
			      struct syna_vpu_dev *vpu)
{
	int err;
	unsigned int enable;
	err = param_set_uint(val, param);
	if (err || !vpu || !vpu->core_clk) {
		pr_err("Failed to set decoder clock gate %d, %p\n", err, vpu);
		return err;
	}

	if (vpu->variant->hw_type == VPU_V2G) {
		enable = vdec_enable_clk_gate;
	} else {
		enable = venc_enable_clk_gate;
	}
	if (enable == 1) {
		dev_info(vpu->dev, "enable clock gate \n");
		clk_disable(vpu->core_clk);
	} else {
		dev_info(vpu->dev, "disable clock gate \n");
		clk_enable(vpu->core_clk);
	}
	return 0;
}

static int vdec_set_clock_gate(const char *val,
			       const struct kernel_param *param)
{
	vpu_set_clock_gate(val, param, g_dec_dev);
	return 0;
}

static int venc_set_clock_gate(const char *val,
			       const struct kernel_param *param)
{
	vpu_set_clock_gate(val, param, g_enc_dev);
	return 0;
}

static const struct kernel_param_ops vdec_clk_gate_ops = {
	.set = vdec_set_clock_gate,
	.get = &param_get_uint,
};

static const struct kernel_param_ops venc_clk_gate_ops = {
	.set = venc_set_clock_gate,
	.get = &param_get_uint,
};
module_param_cb(vdec_clk_gate, &vdec_clk_gate_ops, &vdec_enable_clk_gate, 0644);
module_param_cb(venc_clk_gate, &venc_clk_gate_ops, &venc_enable_clk_gate, 0644);


static irqreturn_t syna_vpu_isr(int irq, void *dev_id)
{
	struct syna_vpu_dev *vpu = dev_id;

	vpu_srv_isr_done(vpu->srv);

	return IRQ_HANDLED;
}

static int syna_vpu_v4l2_connect(struct syna_vpu_auxiliary_device *auxdev)
{
	struct syna_vpu_dev *vpu_dev = dev_get_drvdata(&auxdev->dev);
	struct device *dev = &auxdev->dev;
	int ret;

	ret = devm_request_threaded_irq(dev, vpu_dev->irq, NULL, syna_vpu_isr,
					IRQF_ONESHOT, dev_name(dev), vpu_dev);
	if (ret) {
		dev_err(dev, "register interrupter runtime failed\n");
		return ret;
	}

	return 0;
}

static int syna_vpu_v4l2_disconnect(struct syna_vpu_auxiliary_device *auxdev)
{
	struct syna_vpu_dev *vpu_dev = dev_get_drvdata(&auxdev->dev);

	devm_free_irq(&auxdev->dev, auxdev->irq, (void *)vpu_dev);

	return 0;
}

static bool check_version(struct syna_vpu_dev *vpu)
{
	struct syna_vpu_fw_data *fw_data = &vpu->fw_data;
	if (VPU_TA_VER_MAJOR(fw_data->fw_api_ver) != VPU_TA_VERSION_MAJOR) {
		return false;
	}
	if (vpu->variant->hw_variant & SYNA_VPU_SUPPORT_DEC_MASK) {
		if (VPU_TA_API_VER_MAJOR(fw_data->fw_api_ver) !=
		    VPU_TA_DEC_API_VER_MAJOR) {
			return false;
		}
	}
	if (vpu->variant->hw_variant & SYNA_VPU_SUPPORT_ENC_MASK) {
		if (VPU_TA_API_VER_MAJOR(fw_data->fw_api_ver) !=
		    VPU_TA_ENC_API_VER_MAJOR) {
			return false;
		}
	}
	return true;
}

static int syna_vpu_v4l2_probe(struct syna_vpu_auxiliary_device *auxdev,
			       const struct syna_vpu_variant *variant)
{
	struct device *dev = &auxdev->dev;
	struct syna_vpu_dev *vpu_dev = NULL;
	struct syna_vpu_fw_data *fw_data = NULL;
	u32 vpu_ctx_size;
	u32 *p_memid;
	int ret;

	dev_dbg(dev, "VPU probing\n");

	vpu_dev = devm_kzalloc(dev, sizeof(struct syna_vpu_dev),
			       GFP_KERNEL);
	if (!vpu_dev)
		return -ENOMEM;

	vpu_dev->variant = variant;
	vpu_dev->dev = dev;

	init_rwsem(&vpu_dev->resource_rwsem);

	vpu_dev->vcodec_workq = alloc_ordered_workqueue("%s_task_wq",
							WQ_MEM_RECLAIM
							| WQ_FREEZABLE,
							dev_name(vpu_dev->dev));
	if (!vpu_dev->vcodec_workq) {
		dev_err(dev, "failed to create vcodec workqueue\n");
		ret = -EINVAL;
		goto failed;
	}

	vpu_dev->srv = syna_srv_create(vpu_dev);
	if (IS_ERR_OR_NULL(vpu_dev->srv)) {
		dev_err(dev, "failed to attach the session scheduler\n");
		ret = -ENOMEM;
		goto failed;
	}

	vpu_dev->irq = auxdev->irq;

	vpu_dev->core_clk = devm_clk_get(dev->parent, NULL);
	if (IS_ERR(vpu_dev->core_clk)) {
		dev_err(dev, "failed to get clk from dts\n");
		goto failed_irq_workq;
	}
	clk_prepare_enable(vpu_dev->core_clk);

	ret = syna_vpu_open_tz_session(vpu_dev);
	if (ret) {
		dev_err(dev, "tz initilize failed\n");
		goto failed_session;
	}

	ret = syna_vpu_get_hwdata(vpu_dev);
	if (ret) {
		dev_err(dev, "tz get const failed\n");
		goto failed_hwdata;
	}
	if (!check_version(vpu_dev)) {
		goto failed_hwdata;
	}

	fw_data = &vpu_dev->fw_data;
	dev_info(dev, "FW ver: %d, HW ver: %d\n",
		 (uint32_t)fw_data->hw_info.fw_version, fw_data->hw_info.hw_version);

	vpu_ctx_size = ALIGN(BERLIN_VPU_CFG_RESERVED_SIZE, PAGE_SIZE) +
		       ALIGN(fw_data->fw_info.vpu_context_size, PAGE_SIZE) +
		       ALIGN(fw_data->fw_info.vpu_hw_context_size, PAGE_SIZE);

	/* TODO: move to secure */
	vpu_dev->vpu_ctx_buf = syna_dh_bm_alloc(cma_dh_dev, vpu_ctx_size);

	if (!vpu_dev->vpu_ctx_buf) {
		dev_err(dev, "can't allocate VPU context secure buf\n");
		ret = -ENOMEM;
		goto failed_hwdata;
	}

	p_memid = vb2_syna_bm_cookie(NULL, vpu_dev->vpu_ctx_buf);
	if (!p_memid) {
		dev_err(dev, "no memid for the secure buf\n");
		ret = -ENOMEM;
		goto failed_ctx_buf;
	}

#if IS_ENABLED(CONFIG_OPTEE)
	if (variant->hw_type == VPU_H1_0 && h1_irq_init(vpu_dev->srv)) {
		dev_err(dev, "fail to init h1 irq\n");
		goto failed_ctx_buf;
	}
#endif // CONFIG_OPTEE

        /*
	 * NOTE: a comment from amp vcodec
	 * "need enable H1 clock before open, otherwise board will hang"
	 * I just enable the device clock and power for any variant
	 */
	pm_runtime_get(dev);
	ret = syna_vpu_open(vpu_dev, *p_memid, vpu_ctx_size);
	if (ret) {
		pm_runtime_put(dev);
		goto failed_ctx_buf;
	}
	pm_runtime_put(dev);

	syna_vpu_set_log_level(vpu_dev);

	switch (variant->hw_type) {
	case VPU_V2G:
		ret = syna_vdpu_v4g_init(vpu_dev);
		g_dec_dev = vpu_dev;
		break;
	case VPU_H1_0:
		ret = syna_vepu_h1_init(vpu_dev);
		g_enc_dev = vpu_dev;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	if (ret)
		goto failed_dev_node;

	dev_set_drvdata(&auxdev->dev, vpu_dev);

	return 0;

failed_dev_node:
	syna_vpu_close(vpu_dev);
failed_ctx_buf:
	vb2_syna_dh_bm_put(vpu_dev->vpu_ctx_buf);
failed_hwdata:
	syna_vpu_close_tz_session(vpu_dev);
failed_session:
	clk_disable_unprepare(vpu_dev->core_clk);
failed_irq_workq:
	syna_srv_destroy(&vpu_dev->srv);
	destroy_workqueue(vpu_dev->vcodec_workq);
failed:
	pm_runtime_disable(dev);
	return ret;
}

static void syna_vpu_v4l2_remove(struct syna_vpu_auxiliary_device *auxdev)
{
	struct syna_vpu_dev *vpu_dev = dev_get_drvdata(&auxdev->dev);

	syna_vpu_close(vpu_dev);

	flush_workqueue(vpu_dev->vcodec_workq);
	destroy_workqueue(vpu_dev->vcodec_workq);
	syna_srv_destroy(&vpu_dev->srv);

	switch (vpu_dev->variant->hw_type) {
	case VPU_V2G:
		syna_vdpu_v4g_finalize(vpu_dev);
		g_dec_dev = NULL;
		break;
	case VPU_H1_0:
		syna_vepu_h1_finalize(vpu_dev);
		g_enc_dev = NULL;
#if IS_ENABLED(CONFIG_OPTEE)
		h1_irq_destroy();
#endif // CONFIG_OPTEE
		break;
	default:
		break;
	}

	syna_vpu_close_tz_session(vpu_dev);
	vb2_syna_dh_bm_put(vpu_dev->vpu_ctx_buf);
	clk_disable_unprepare(vpu_dev->core_clk);
}

const struct syna_vpu_auxiliary_driver syna_vpu_v4l2_drv = {
	.probe = &syna_vpu_v4l2_probe,
	.remove = &syna_vpu_v4l2_remove,
	.connect = &syna_vpu_v4l2_connect,
	.disconnect = &syna_vpu_v4l2_disconnect,
};
