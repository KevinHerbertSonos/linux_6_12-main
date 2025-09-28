// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include "drv_hdmitx.h"
#include "../../../../../extcon/extcon.h"

int drv_hdmitx_init(VPP_CTX *hVppCtx)
{
	int ret = -EFAULT;
	static const unsigned int hdmitx_cable[] = {
		EXTCON_DISP_HDMI,
		EXTCON_NONE,
	};

	if (IS_ERR(hVppCtx->dev)) {
		avio_error("%s hVppCtx->dev is null.\n", __func__);
		return ret;
	}

	hVppCtx->hdmitx_dev = devm_extcon_dev_allocate(hVppCtx->dev, hdmitx_cable);
	if (IS_ERR(hVppCtx->hdmitx_dev)) {
		avio_error("%s extcon allocate dev fail\n",__func__);
		return ret;
	}

	hVppCtx->hdmitx_dev->name = "hdmi_audio";
	ret = devm_extcon_dev_register(hVppCtx->dev, hVppCtx->hdmitx_dev);
	if (ret < 0) {
		avio_error("%s extcon register dev fail, ret=%d\n", __func__, ret);
	}

	return ret;
}

int drv_hdmitx_set_state(struct extcon_dev *hdmitx_dev, unsigned int state)
{
	int ret = -EFAULT;

	if (!IS_ERR(hdmitx_dev)) {
		ret = extcon_set_state_sync(hdmitx_dev, EXTCON_DISP_HDMI, state);
		if (ret < 0) {
			avio_error("%s extcon set state fail, ret=%d\n", __func__, ret);
		}
	}

	return ret;
}
