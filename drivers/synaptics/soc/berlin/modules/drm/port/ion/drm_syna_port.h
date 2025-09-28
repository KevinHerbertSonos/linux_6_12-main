// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Shanmugam Ramachandran <Shanmugam.Ramachandran@synaptics.com>
 *
 */
#if !defined(__DRM_SYNA_PORT_H__)
#define __DRM_SYNA_PORT_H__

#include "drm_syna_common.h"

#define SYNA_DRM_CRTC_VBLANK_INTERFACES()

#define SYNA_DRM_DRIVER_VBLANK_INTERFACES() \
	.enable_vblank = syna_enable_vblank,    \
	.disable_vblank = syna_disable_vblank,

#define SYNA_DRM_DRIVER_GEM_PLANE_INTERFACES() \
	.prepare_fb = drm_gem_fb_prepare_fb,

#define SYNA_DRM_DRIVER_GEM_FREE_OBJ_INTERFACES() \
	.gem_free_object = syna_gem_object_free,

#define SYNA_DRM_DRIVER_PRIME_INTERFACES() \
	.gem_prime_export = drm_gem_prime_export, \
	.gem_prime_import = drm_gem_prime_import, \
	.gem_prime_get_sg_table = syna_gem_prime_get_sg_table,

#define SYNA_DRM_DRIVER_GEM_VM_OPS_INTERFAES() \
	.gem_vm_ops = &syna_gem_vm_ops,

#define SYNA_DRM_DRIVER_GEM_VM_OPS_GEM_OBJ_INTERFAES() \
	.vm_ops = &syna_gem_vm_ops,

#define SYNA_DRM_DRIVER_SET_GEM_OBJ_FUNCS_INTERFACES(obj)

#define DRM_GEM_OBJECT_PUT(obj)  drm_gem_object_put_unlocked(obj)
#define DRM_DMABUF_UNMAP(obj) dma_buf_kunmap(obj->dma_buf, 0, obj->kernel_vir_addr);

typedef struct drm_crtc_state syna_drm_crtc_state;
#define syna_get_drm_crtc_state(state, crtc) \
	state;

typedef struct drm_plane_state syna_drm_plane_state;
#define syna_get_drm_plane_state(state, plane) state
#define syna_get_drm_plane_state_by_new(state, plane) state->plane

#define syna_set_irq_enabled(val) dev->irq_enabled = val;

int syna_enable_vblank(struct drm_device *dev, unsigned int crtc);
void syna_disable_vblank(struct drm_device *dev, unsigned int crtc);

void syna_gem_object_free(struct drm_gem_object *obj);
#ifdef CONFIG_DEBUG_FS
int syna_debugfs_init(struct drm_minor *minor);
#endif //CONFIG_DEBUG_FS

void* syna_drm_get_vaddr_from_gem_obj(struct syna_gem_object *syna_obj);
#endif /* defined(__DRM_SYNA_PORT_H__) */
