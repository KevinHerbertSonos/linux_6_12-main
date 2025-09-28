// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Shanmugam Ramachandran <Shanmugam.Ramachandran@synaptics.com>
 *
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/component.h>
#include <linux/of_platform.h>
#include <linux/kthread.h>
#include <drm/drm_gem.h>
#include <drm/drm_atomic_helper.h>

#include "drm_syna_drv.h"
#include "drm_syna_gem.h"
#include "syna_drm.h"
#include "syna_vpp.h"

int syna_enable_vblank(struct drm_device *dev, unsigned int crtc_num)
{
	struct syna_drm_private *dev_priv;
	struct drm_crtc *crtc;

	dev_priv = dev->dev_private;
	crtc = dev_priv->crtc[crtc_num];

	return syna_crtc_enable_vblank(crtc, true);
}

void syna_disable_vblank(struct drm_device *dev, unsigned int crtc)
{
	struct syna_drm_private *dev_priv;
	struct drm_crtc *crtc;

	dev_priv = dev->dev_private;
	crtc = dev_priv->crtc[crtc_num];

	syna_crtc_enable_vblank(crtc, false);
}

#ifdef CONFIG_DEBUG_FS
int syna_debugfs_init(struct drm_minor *minor)
{
	return syna_debugfs_init_common(minor);
}
#endif //CONFIG_DEBUG_FS

void syna_gem_object_free(struct drm_gem_object *obj)
{
	syna_gem_object_free_priv(obj);
}

void* syna_drm_get_vaddr_from_gem_obj(struct syna_gem_object *syna_obj)
{
	return dma_buf_kmap(syna_obj->dma_buf, 0);
}
