// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */

#include "drm_syna_drv.h"
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/platform_device.h>

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_drv.h>
#include <drm/drm_atomic.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_atomic_helper.h>
#include "syna_vpp.h"

static bool async_flip_enable = true;

module_param(async_flip_enable, bool, 0444);

MODULE_PARM_DESC(async_flip_enable,
		 "Enable support for 'faked' async flipping (default: Y)");
MODULE_LICENSE("Dual MIT/GPL");

void __weak syna_vpp_add_debugfs_entry(struct syna_drm_private *dev_priv)
{
	return;
}

void __weak syna_vpp_remove_debugfs_entry(struct syna_drm_private *dev_priv)
{
	return;
}

void __weak syna_hdmi_add_debugfs_entry(struct syna_drm_private *dev_priv)
{
	return;
}

void __weak syna_hdmi_remove_debugfs_entry(struct syna_drm_private *dev_priv)
{
	return;
}

/*************************************************************************
 * DRM mode config callbacks
 **************************************************************************/

static struct drm_framebuffer *syna_fb_create(struct drm_device *dev,
						  struct drm_file *file,
						  const struct drm_mode_fb_cmd2
						  *mode_cmd)
{
	struct drm_framebuffer *fb;

	switch (mode_cmd->pixel_format) {
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
		break;
	default:
		DRM_ERROR_RATELIMITED
			("pixel format not supported (format = %u)\n",
			 mode_cmd->pixel_format);
		return ERR_PTR(-EINVAL);
	}

	if (mode_cmd->flags & DRM_MODE_FB_INTERLACED) {
		DRM_ERROR_RATELIMITED
			("interlaced framebuffers not supported\n");
		return ERR_PTR(-EINVAL);
	}

	if (mode_cmd->modifier[0] != DRM_FORMAT_MOD_NONE) {
		DRM_ERROR_RATELIMITED
			("format modifier 0x%llx is not supported\n",
			 mode_cmd->modifier[0]);
		return ERR_PTR(-EINVAL);
	}

	fb = drm_gem_fb_create(dev, file, mode_cmd);
	if (IS_ERR(fb))
		goto out;

	DRM_DEBUG_DRIVER("[FB:%d]\n", fb->base.id);

out:
	return fb;
}

static const struct drm_mode_config_funcs syna_mode_config_funcs = {
	.fb_create = syna_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static void syna_atomic_commit_tail(struct drm_atomic_state *state)
{
	struct drm_device *dev = state->dev;

	drm_atomic_helper_commit_modeset_disables(dev, state);
	drm_atomic_helper_commit_modeset_enables(dev, state);
	drm_atomic_helper_commit_planes(dev, state,
					DRM_PLANE_COMMIT_ACTIVE_ONLY);
	drm_atomic_helper_commit_hw_done(state);
	drm_atomic_helper_wait_for_vblanks(dev, state);
	drm_atomic_helper_cleanup_planes(dev, state);
}

static struct drm_mode_config_helper_funcs syna_mode_config_helpers = {
	.atomic_commit_tail = syna_atomic_commit_tail,
};

void syna_vpp_set_res_limit(struct drm_device *dev, u32 hdisplay, u32 vdisplay)
{
	bool update = 0;

	if (hdisplay > dev->mode_config.max_width) {
		dev->mode_config.max_width = hdisplay;
		update = 1;
	}
	if (vdisplay > dev->mode_config.max_height) {
		dev->mode_config.max_height = vdisplay;
		update = 1;
	}

	if (update) {
		dev->mode_config.min_width = SYNA_WIDTH_MIN;
		dev->mode_config.min_height = SYNA_HEIGHT_MIN;
		DRM_INFO("max_width is %d\n", dev->mode_config.max_width);
		DRM_INFO("max_height is %d\n", dev->mode_config.max_height);
	}
}

int syna_modeset_early_init(struct syna_drm_private *dev_priv)
{
	struct drm_device *dev = dev_priv->dev;
	int err;

	drm_mode_config_init(dev);

	dev->mode_config.funcs = &syna_mode_config_funcs;

	syna_vpp_set_res_limit(dev, SYNA_WIDTH_MAX, SYNA_HEIGHT_MAX);

	dev->mode_config.async_page_flip = async_flip_enable;
	dev->mode_config.helper_private = &syna_mode_config_helpers;

	DRM_INFO("%s async flip support is %s\n",
		 dev->driver->name, async_flip_enable ? "enabled" : "disabled");

	syna_modeset_createEntries(dev_priv);

	//Enable zpos to be normalized
	dev->mode_config.normalize_zpos = true;

	DRM_DEBUG_DRIVER("initialised\n");

	return 0;

	drm_mode_config_cleanup(dev);

	return err;
}

int syna_modeset_late_init(struct syna_drm_private *dev_priv)
{
	struct drm_device *ddev = dev_priv->dev;

	drm_mode_config_reset(ddev);
	syna_hdmi_add_debugfs_entry(dev_priv);
	syna_vpp_add_debugfs_entry(dev_priv);

	return 0;
}

void syna_modeset_early_cleanup(struct syna_drm_private *dev_priv)
{
	syna_hdmi_remove_debugfs_entry(dev_priv);
}

void syna_modeset_late_cleanup(struct syna_drm_private *dev_priv)
{
	drm_mode_config_cleanup(dev_priv->dev);
	syna_vpp_remove_debugfs_entry(dev_priv);

	DRM_DEBUG_DRIVER("cleaned up\n");
}
