// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Shanmugam Ramachandran <Shanmugam.Ramachandran@synaptics.com>
 *
 */

#include "drm_syna_port.h"
#include "drm_syna_common.h"

int syna_enable_vblank(struct drm_crtc *crtc)
{
	return syna_crtc_enable_vblank(crtc, true);
}

void syna_crtc_disable_vblank(struct drm_crtc *crtc)
{
	syna_crtc_enable_vblank(crtc, false);
}

#ifdef CONFIG_DEBUG_FS
void syna_debugfs_init(struct drm_minor *minor)
{
	syna_debugfs_init_common(minor);
}
#endif //CONFIG_DEBUG_FS

void* syna_drm_get_vaddr_from_gem_obj(struct syna_gem_object *syna_obj)
{
	int ret;

	ret = dma_buf_vmap(syna_obj->dma_buf, (struct iosys_map*)&syna_obj->map);

	return syna_obj->map.vaddr;
}
