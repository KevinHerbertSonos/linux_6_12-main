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
#include "kernel_compatibility.h"
#include "drm_syna_common.h"

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_uapi.h>

#define SYNA_DRM_CRTC_VBLANK_INTERFACES() \
	.enable_vblank = syna_enable_vblank, \
	.disable_vblank = syna_crtc_disable_vblank

#define SYNA_DRM_DRIVER_VBLANK_INTERFACES()
#define SYNA_DRM_DRIVER_GEM_PLANE_INTERFACES()
#define SYNA_DRM_DRIVER_GEM_FREE_OBJ_INTERFACES()
#define SYNA_DRM_DRIVER_PRIME_INTERFACES()
#define SYNA_DRM_DRIVER_GEM_VM_OPS_INTERFAES()

#define SYNA_DRM_DRIVER_SET_GEM_OBJ_FUNCS_INTERFACES(obj) \
	obj->funcs = &syna_gem_object_funcs;

#define SYNA_DRM_DRIVER_GEM_VM_OPS_GEM_OBJ_INTERFAES() \
	.vm_ops = &syna_gem_vm_ops,

#define DRM_GEM_OBJECT_PUT(obj)  drm_gem_object_put(obj)
#define DRM_DMABUF_UNMAP(obj) dma_buf_vunmap(obj->dma_buf, (struct iosys_map*)&obj->map);

typedef struct drm_atomic_state syna_drm_crtc_state;
#define syna_get_drm_crtc_state(state, crtc) \
	drm_atomic_get_crtc_state(state, crtc);

typedef struct drm_atomic_state syna_drm_plane_state;
#define syna_get_drm_plane_state(state, plane) \
			drm_atomic_get_plane_state(state, plane)
#define syna_get_drm_plane_state_by_new(state, plane) \
			drm_atomic_get_new_plane_state(state, plane)

#define syna_set_irq_enabled(val)

int syna_enable_vblank(struct drm_crtc *crtc);
void syna_crtc_disable_vblank(struct drm_crtc *crtc);

#ifdef CONFIG_DEBUG_FS
void syna_debugfs_init(struct drm_minor *minor);
#endif //CONFIG_DEBUG_FS

void* syna_drm_get_vaddr_from_gem_obj(struct syna_gem_object *syna_obj);
#endif /* defined(__DRM_SYNA_PORT_H__) */
