// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 */

#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/debugfs.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_edid.h>
#include "drm_syna_drv.h"
#include "syna_drm_priv.h"
#include "drm_syna_hdmi.h"
#include "vpp_cmd.h"
#include "vpp_api.h"
#include "syna_hdmi_config.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0))
struct drm_edid {
        /* Size allocated for edid */
        size_t size;
        const struct edid *edid;
};
#endif

#define MAX_EDID_BLOCKS 8 //Max EDID blocks supported by syna driver

static char preferred_mode_name[DRM_DISPLAY_MODE_LEN] = "1920x1080";
static char max_supported_mode[DRM_DISPLAY_MODE_LEN] = "\0";

module_param_string(hdmi_preferred_mode,
			preferred_mode_name, DRM_DISPLAY_MODE_LEN, 0444);

MODULE_PARM_DESC(hdmi_preferred_mode,
		 "Specify the preferred mode (if supported), e.g. 1280x1024.");

module_param_string(hdmi_fixed_4K_mode,
			max_supported_mode, DRM_DISPLAY_MODE_LEN, 0444);
MODULE_PARM_DESC(hdmi_fixed_4K_mode,
		"When on fixed mode from driver, select max 4K mode. eg:4K60/4K30");

MODULE_LICENSE("Dual MIT/GPL");

static int hpd_handle_state_get(void *data, u64 *val)
{
	struct drm_connector *connector = data;
	struct syna_conn_hdmi *syna_hdmi = to_syna_conn_hdmi(connector);

	*val = syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.hpdHandlingEnabled;
	return 0;
}

static int hpd_handle_state_set(void *data, u64 val)
{
	struct drm_connector *connector = data;
	struct syna_conn_hdmi *syna_hdmi = to_syna_conn_hdmi(connector);

	if (val) {
		syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.hpdHandlingEnabled = true;
	} else {
		syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.hpdHandlingEnabled = false;
	}

	DRM_DEBUG_DRIVER("%s notification of HPD via drm core\n", val?"enabling":"disabling");
	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(hpd_handle_fops, hpd_handle_state_get, hpd_handle_state_set, "%llu\n");

static int res_handle_state_get(void *data, u64 *val)
{
	struct drm_connector *connector = data;
	struct syna_conn_hdmi *syna_hdmi = to_syna_conn_hdmi(connector);

	*val = syna_hdmi->drm_priv->modeset_enabled;
	return 0;
}

static int res_handle_state_set(void *data, u64 val)
{
	struct drm_connector *connector = data;
	struct syna_conn_hdmi *syna_hdmi = to_syna_conn_hdmi(connector);
	if (val) {
		DRM_DEBUG_DRIVER("enabling resolution change support!\n");
		syna_hdmi->drm_priv->modeset_enabled = true;
		syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.fixedModeSet = false;
	} else {
		DRM_DEBUG_DRIVER("use fixed output mode!\n");
		syna_hdmi->drm_priv->modeset_enabled = false;
		syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.fixedModeSet = true;
	}
	/* takes effect on next HPD*/
	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(res_handle_fops, res_handle_state_get, res_handle_state_set, "%llu\n");

static int syna_hdmi_get_edid (void *data, u8 *buf, unsigned int block, size_t len)
{
	VPP_HDMI_RAW_EDID *edid_buffer;
	int retVal = 0;

	edid_buffer = kzalloc(sizeof(VPP_HDMI_RAW_EDID), GFP_KERNEL);
	if (!edid_buffer) {
		DRM_ERROR("Memory could not be allocated for EDID\n");
		return -ENOMEM;
	}

	retVal = wrap_MV_VPPOBJ_GetHDMIRawEdid(edid_buffer);
	if(retVal) {
		DRM_ERROR("Fail to get EDID from driver:%d\n",retVal);
		goto get_edid_exit;
	}

	if (!edid_buffer->IsValid) {
		DRM_ERROR("Invalid EDID\n");
		retVal = -EINVAL;
		goto get_edid_exit;
	}

	if (block < MAX_EDID_BLOCKS) {
		memcpy(buf, edid_buffer->DataBuf + (block * 128), len);
	} else {
		DRM_ERROR("Could not access requested block\n");
		retVal = -EINVAL;
		goto get_edid_exit;
	}

get_edid_exit:
	kfree(edid_buffer);
	return retVal;
}

static int syna_hdmi_connector_helper_get_modes(struct drm_connector *connector)
{
	struct syna_conn_hdmi *syna_hdmi = to_syna_conn_hdmi(connector);
	int len = strlen(preferred_mode_name);
	int num_modes;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0))
	const struct drm_edid *hdmi_edid = drm_edid_read_custom(connector,
						syna_hdmi_get_edid, NULL);
#else
	struct edid *hdmi_edid = drm_do_get_edid(connector,
						syna_hdmi_get_edid, NULL);
#endif

	if (len)
		DRM_DEBUG_DRIVER("detected hdmi_preferred_mode=%s\n",
				 preferred_mode_name);
	else
		DRM_DEBUG_DRIVER("no hdmi_preferred_mode\n");

	if ((hdmi_edid == NULL) &&
		!syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.hpdHandlingEnabled) {
		/* load dummy edid as bootup without sink and no hpd handling enabled*/
		num_modes = drm_add_modes_noedid(connector, 1920, 1080);
		if (!len) {
			strncpy(preferred_mode_name, "1920x1080", DRM_DISPLAY_MODE_LEN);
			len = strlen(preferred_mode_name);
		}
	} else {
		/* even if hpd handling is disabled, if read EDID can be parsed on
		   startup, use the read edid instead of hardcoded */
		DRM_DEBUG_DRIVER("Edid obtained and parsed \n");

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0))
		drm_connector_update_edid_property(connector, hdmi_edid->edid);
		num_modes = drm_add_edid_modes(connector, (struct edid*)hdmi_edid->edid);
#else
		drm_connector_update_edid_property(connector, hdmi_edid);
		num_modes = drm_add_edid_modes(connector, hdmi_edid);
#endif

		kfree(hdmi_edid);
	}

	if (num_modes && len) {
		struct drm_display_mode *pref_mode_user = NULL;
		struct drm_display_mode *pref_mode_edid = NULL;

		struct drm_display_mode *mode, *t;
		list_for_each_entry_safe(mode, t, &connector->probed_modes, head) {
			if (mode->type & DRM_MODE_TYPE_PREFERRED) {
				DRM_DEBUG_DRIVER("preferred mode from EDID:%s\n", mode->name);
				pref_mode_edid = mode;
				mode->type &= ~DRM_MODE_TYPE_PREFERRED;
			}

			if ((pref_mode_user == NULL) && !strcmp(mode->name, preferred_mode_name)) {
				if (!(mode->flags & DRM_MODE_FLAG_INTERLACE)) {
					pref_mode_user = mode;
				}
			}
		}

		if (pref_mode_user)
			pref_mode_user->type |= DRM_MODE_TYPE_PREFERRED;
		else if (pref_mode_edid)
			pref_mode_edid->type |= DRM_MODE_TYPE_PREFERRED;
	}

	drm_mode_sort(&connector->probed_modes);
	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s] found %d modes\n",
					connector->base.id, connector->name, num_modes);

	return num_modes;
}

static int syna_check_mode_enabled(int res_id, struct syna_conn_hdmi *syna_hdmi)
{
	int i;
	for (i = 1; i < syna_hdmi->count_format_supported; i++)
		if (res_id == syna_hdmi->supported_formats[i])
			return 0;

	return -1;
}

static int syna_hdmi_connector_helper_mode_valid(struct drm_connector *connector,
						struct drm_display_mode *mode)
{
	int res_id = MV_VPP_GetResIndex(mode->hdisplay, mode->vdisplay,
			mode->flags & DRM_MODE_FLAG_INTERLACE, mode->clock,
			drm_mode_vrefresh(mode));
	if (res_id < 0)
		return MODE_NOMODE;

	/*Interlaced and Doublescan mode currently not supported by VPP*/
	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		return MODE_NO_INTERLACE;
	else if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
		return MODE_NO_DBLESCAN;

	if (syna_check_mode_enabled(res_id, to_syna_conn_hdmi(connector)))
		return MODE_NOMODE;

	return MODE_OK;
}

static void syna_hdmi_connector_destroy(struct drm_connector *connector)
{
	struct syna_conn_hdmi *syna_hdmi = to_syna_conn_hdmi(connector);
	struct syna_drm_private *dev_priv = connector->dev->dev_private;
	ENUM_VOUT_CONNECTOR vout_id =
		connector->connector_type == DRM_MODE_CONNECTOR_HDMIA ?
			  VOUT_CONNECTOR_HDMI : VOUT_CONNECTOR_DSI;

	if (!dev_priv) {
		DRM_ERROR("%s %d  device private is NULL!!\n",
			  __func__, __LINE__);
		return;
	}
	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s]\n",
			 connector->base.id, connector->name);

	if (syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.hpdHandlingEnabled)
		kthread_stop(syna_hdmi->hpd_monitor_task);

	kfree(syna_hdmi->supported_formats);

	drm_connector_cleanup(connector);

	kfree(syna_hdmi);
	dev_priv->connector[vout_id] = NULL;
}

static void syna_hdmi_connector_force(struct drm_connector *connector)
{
}

static enum drm_connector_status syna_hdmi_hotplug_detect(
                        struct drm_connector *connector,
                        bool force)
{
	unsigned char device_connected = false;
	struct syna_conn_hdmi *syna_hdmi = to_syna_conn_hdmi(connector);

	(void)force;
	/* read the HPD status from vpp hal, which is updated
	   based on HPD IRQ from dhub */

	if (syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.hpdHandlingEnabled)
		wrap_MV_VPPOBJ_GetHPDStatus(&device_connected, false);
	else
		device_connected = true; /*assume sink is connected always*/

	return (device_connected ? connector_status_connected :
					connector_status_disconnected);
}

static int syna_configure_def_res(void)
{
	VPP_HDMI_SINK_CAPS sinkCaps;
	int retVal;
	VPP_DISP_OUT_PARAMS dispParams;
	int len = strlen(max_supported_mode);

	MV_VPP_GetDispOutParams(CPCB_1, &dispParams);

	/*configure the default resolution set on HDMI connection*/
	retVal = wrap_MV_VPPOBJ_GetHDMISinkFeatureMap(&sinkCaps);
	if ((retVal == MV_VPP_OK)) {
		/*Fixed mode only: Hardcode resolution to 4K30 on 4K TV, otherwise to 1080p60*/
		if (sinkCaps & ((1<<VPP_HDMI_SINKCAP_BITMASK_FULL4K)|(1<<VPP_HDMI_SINKCAP_BITMASK_4K30))) {
			dispParams.uiResId = (sinkCaps & (1<<VPP_HDMI_SINKCAP_BITMASK_PREF50FPS)) ?
				HDMI_MAX_RES_ENABLED_50_25:HDMI_MAX_RES_ENABLED_60_30;
			if (len) {
				if (!strcmp("4K60", max_supported_mode))
					dispParams.uiResId = RES_4Kx2K60;
					else if (!strcmp("4K50", max_supported_mode))
						dispParams.uiResId = RES_4Kx2K50;
					else if (!strcmp("4K30", max_supported_mode))
						dispParams.uiResId = RES_4Kx2K30;
					else
						dispParams.uiResId = RES_1080P60;

					DRM_DEBUG_DRIVER("select resIndex:%d from user config\n", dispParams.uiResId);
				}
			} else if (sinkCaps & ((1<<VPP_HDMI_SINKCAP_BITMASK_FHD)))
				dispParams.uiResId = (sinkCaps & (1<<VPP_HDMI_SINKCAP_BITMASK_PREF50FPS)) ?
					RES_1080P50:RES_1080P60;
			else
				dispParams.uiResId = (sinkCaps & (1<<VPP_HDMI_SINKCAP_BITMASK_PREF50FPS)) ?
					RES_720P50:RES_720P60;

		DRM_DEBUG_DRIVER("HDMI_HPD detected, setting res to resId:%d colorfmt:%d bidepth:%d sinkcaps:%d\n",
				dispParams.uiResId,dispParams.uiColorFmt,dispParams.uiBitDepth,
				(retVal)?-1:sinkCaps);

		//Set the display resolution
		retVal = MV_VPP_SetDisplayResolution(CPCB_1, dispParams, 1);
		if (retVal != MV_VPP_OK)
			DRM_DEBUG_DRIVER("%s:%d: MV_VPP_SetDisplayResolution FAILED, error: 0x%x\n",
					__func__, __LINE__, retVal);
			/* don't handle error */
	} else {
		DRM_ERROR("wrap_MV_VPPOBJ_GetHDMISinkFeatureMap FAIL with %d\n",retVal);
	}

	return retVal;
}

static int syna_hdmi_hpd_monitor(void *param)
{
	struct drm_connector *connector = (struct drm_connector *)param;
	struct syna_conn_hdmi *syna_hdmi = to_syna_conn_hdmi(connector);
	struct drm_device *dev = connector->dev;
	unsigned char activeHpdStatus;
	unsigned char hpdStatus = false;
	int retVal;

	connector->status = syna_hdmi_hotplug_detect(connector, false);
	activeHpdStatus = (connector->status == connector_status_connected) ? true : false;
	DRM_DEBUG_DRIVER("startup HDMI connection state : %d\n", activeHpdStatus);
	if (activeHpdStatus)
		syna_configure_def_res();

	while (!kthread_should_stop()) {
		retVal = wrap_MV_VPP_WaitHdmiConnChange(&hpdStatus);
		if (retVal != 0)
			continue;

		if ((hpdStatus != activeHpdStatus) &&
			syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.hpdHandlingEnabled) {
			DRM_INFO("HDMI connection state changed to : %d\n", hpdStatus);
			activeHpdStatus = hpdStatus;
			connector->status = hpdStatus ? connector_status_connected :
							connector_status_disconnected;

			if (syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.fixedModeSet
				       && (hpdStatus==connector_status_connected))
				syna_configure_def_res();
			drm_kms_helper_hotplug_event(dev);
		}
	}

	return 0;
}

static struct drm_connector_helper_funcs syna_hdmi_connector_helper_funcs = {
	.get_modes = syna_hdmi_connector_helper_get_modes,
	.mode_valid = syna_hdmi_connector_helper_mode_valid,
};

static const struct drm_connector_funcs syna_hdmi_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = syna_hdmi_connector_destroy,
	.force = syna_hdmi_connector_force,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.dpms = drm_helper_connector_dpms,
	.detect = syna_hdmi_hotplug_detect,
};

struct drm_connector *syna_hdmi_connector_create(struct drm_device *dev)
{
	struct syna_conn_hdmi *syna_hdmi;
	struct drm_connector *connector;
	int retVal;

	syna_hdmi = kzalloc(sizeof(*syna_hdmi), GFP_KERNEL);
	if (!syna_hdmi)
		return ERR_PTR(-ENOMEM);

	retVal = syna_hdmi_tx_read_config(syna_hdmi);
	if (retVal) {
		kfree(syna_hdmi);
		return ERR_PTR(retVal);
	}

	syna_hdmi->drm_priv = dev->dev_private;
	syna_hdmi->drm_priv->modeset_enabled =
		syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.fixedModeSet ? false : true;

	connector = &syna_hdmi->base;
	drm_connector_init(dev, connector, &syna_hdmi_connector_funcs,
				DRM_MODE_CONNECTOR_HDMIA);
	drm_connector_helper_add(connector, &syna_hdmi_connector_helper_funcs);

	connector->dpms = DRM_MODE_DPMS_OFF;
	connector->interlace_allowed = false;
	connector->doublescan_allowed = false;
	connector->display_info.subpixel_order = SubPixelHorizontalRGB;

	if (syna_hdmi->syna_hdmi_conf.hdmiTxConfigFields.hpdHandlingEnabled) {
		syna_hdmi->hpd_monitor_task = kthread_run(syna_hdmi_hpd_monitor, connector, "HDMI HPD monitor thread");
		if (!IS_ERR(syna_hdmi->hpd_monitor_task)) {
			connector->polled = DRM_CONNECTOR_POLL_HPD;
			DRM_DEBUG_DRIVER("syna_drm:enable HDMI HPD polling \n");
		}
	} else {
		DRM_DEBUG_DRIVER("hpd handling not enabled. assume connected always\n");
		connector->status = connector_status_connected;
	}

	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s]\n", connector->base.id,
			 connector->name);

	return connector;
}

void syna_hdmi_add_debugfs_entry(struct syna_drm_private *dev_priv)
{
	struct drm_connector *connector = dev_priv->connector[VOUT_CONNECTOR_HDMI];
	struct syna_conn_hdmi *syna_hdmi = connector ? to_syna_conn_hdmi(connector) : NULL;
	struct dentry *root = connector ? connector->debugfs_entry : NULL;

	if (!syna_hdmi || !root)
		return;

	/* control HPD handling */
	syna_hdmi->debugfs_hpd_node = debugfs_create_file("enable_hpd_handle", S_IRUGO | S_IWUSR, root, connector,
			&hpd_handle_fops);

	/* control fixedModeset */
	syna_hdmi->debugfs_res_node = debugfs_create_file("enable_res_config", S_IRUGO | S_IWUSR, root, connector,
			&res_handle_fops);
}

void syna_hdmi_remove_debugfs_entry(struct syna_drm_private *dev_priv)
{
	struct drm_connector *connector = dev_priv->connector[VOUT_CONNECTOR_HDMI];
	struct syna_conn_hdmi *syna_hdmi = to_syna_conn_hdmi(connector);

	debugfs_remove(syna_hdmi->debugfs_hpd_node);
	debugfs_remove(syna_hdmi->debugfs_res_node);
}
