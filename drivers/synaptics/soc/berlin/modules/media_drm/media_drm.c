// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2024 Synaptics Incorporated
 * Author:   Long Xiao <Long.Xiao@synaptics.com>
 *           Allan Ai <Allan.Ai@synaptics.com>
 *
 */

#define pr_fmt(fmt) "[media drm]" fmt

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include "tee_client_api.h"

#define TA_WV_UUID {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x0e}}
#define TZ_WIDEVINE_CMD_SUSPEND 73
#define TZ_WIDEVINE_CMD_RESUME  74

static TEEC_UUID wv_ta_uuid = TA_WV_UUID;

struct wv_context {
	bool ctx_created;
	bool session_opened;
	TEEC_Context teec_ctx;
	TEEC_Session teec_session;
};

static struct wv_context wv_ctx;

static int open_wv_session(void)
{
	TEEC_Result ret = TEEC_SUCCESS;

	ret = TEEC_InitializeContext(NULL, &wv_ctx.teec_ctx);
	if (ret != TEEC_SUCCESS) {
		pr_info("%s TEEC_InitializeContext fail with 0x%x.\n", __func__, ret);
		return ret;
	}
	wv_ctx.ctx_created = true;

	/* It depends on userspace load the TA. Otherwise open session will fail! */
	ret = TEEC_OpenSession(&wv_ctx.teec_ctx,
			&wv_ctx.teec_session,
			&wv_ta_uuid, TEEC_LOGIN_USER,
			NULL, NULL, NULL);
	if (ret != TEEC_SUCCESS) {
		pr_err("%s open WV TA session fail: 0x%x\n", __func__, ret);
		goto error;
	}
	wv_ctx.session_opened = true;

error:
	if (ret != TEEC_SUCCESS) {
		TEEC_FinalizeContext(&wv_ctx.teec_ctx);
		wv_ctx.ctx_created = false;
	}
	return ret;
}

static void close_wv_session(void)
{
	if (wv_ctx.session_opened) {
		TEEC_CloseSession(&wv_ctx.teec_session);
		wv_ctx.session_opened = false;
	}

	if (wv_ctx.ctx_created) {
		TEEC_FinalizeContext(&wv_ctx.teec_ctx);
		wv_ctx.ctx_created = false;
	}
}

static int media_drm_suspend(void)
{
	TEEC_Result ret = TEEC_SUCCESS;
	TEEC_Operation op;

	pr_info("%s suspend\n", __func__);

	ret = open_wv_session();
	if (ret)
		return ret;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
			TEEC_NONE, TEEC_NONE);
	op.started = 1;
	ret = TEEC_InvokeCommand(&wv_ctx.teec_session,
			TZ_WIDEVINE_CMD_SUSPEND, &op, NULL);
	if (ret != TEEC_SUCCESS)
		pr_err("%s fail to invoke suspend command to WV TA: 0x%x\n", __func__, ret);

	close_wv_session();

	return ret;
}

static int media_drm_resume(void)
{
	TEEC_Result ret = TEEC_SUCCESS;
	TEEC_Operation op;

	pr_info("%s resume\n", __func__);

	ret = open_wv_session();
	if (ret)
		return ret;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
			TEEC_NONE, TEEC_NONE);
	op.started = 1;
	ret = TEEC_InvokeCommand(&wv_ctx.teec_session,
		TZ_WIDEVINE_CMD_RESUME, &op, NULL);
	if (ret != TEEC_SUCCESS)
		pr_err("%s fail to invoke resume command to WV TA: 0x%x\n", __func__, ret);

	close_wv_session();

	return ret;
}

static int media_drm_power_event(struct notifier_block *this, unsigned long event,
			   void *ptr)
{
	int ret;

	pr_info("%s event %ld\n", __func__, event);
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		ret = media_drm_resume();
		break;
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		ret = media_drm_suspend();
		break;
	default:
		return NOTIFY_DONE;
	}
	return ret ? NOTIFY_STOP : NOTIFY_DONE;
}

static struct notifier_block media_drm_power_notifier = {
	.notifier_call = media_drm_power_event,
};

static int __init media_drm_init(void)
{
	int ret;

	ret = register_pm_notifier(&media_drm_power_notifier);
	if (ret) {
		pr_err("%s register_pm_notifier fail:0x%x\n", __func__, ret);
		return ret;
	}

	pr_info("%s done\n", __func__);
	return ret;
}

static void __exit media_drm_exit(void)
{
	unregister_pm_notifier(&media_drm_power_notifier);
}

module_init(media_drm_init);
module_exit(media_drm_exit);

MODULE_AUTHOR("Synaptics");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("media drm driver");
