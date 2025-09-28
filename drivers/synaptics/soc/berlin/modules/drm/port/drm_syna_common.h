
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

extern const struct vm_operations_struct syna_gem_vm_ops;

int syna_crtc_enable_vblank(struct drm_crtc *crtc, bool enable);
int syna_debugfs_init_common(struct drm_minor *minor);
