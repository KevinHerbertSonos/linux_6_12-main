// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Shanmugam Ramachandran <Shanmugam.Ramachandran@synaptics.com>
 *
 */

#include "drm_syna_common.h"

const struct vm_operations_struct syna_gem_vm_ops = {
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

int syna_crtc_enable_vblank(struct drm_crtc *crtc, bool enable)
{
	struct syna_crtc *syna_crtc;
	int crtc_num;

	syna_crtc = to_syna_crtc(crtc);
	crtc_num = syna_crtc->number;

	if (crtc_num > MAX_CRTC) {
		DRM_ERROR("invalid crtc %d\n", syna_crtc->number);
		return -EINVAL;
	}

	syna_crtc_set_vblank_enabled(crtc, enable);

	DRM_DEBUG("vblank interrupts enabled for crtc %d\n", crtc_num);

	return 0;
}
