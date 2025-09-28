// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */
#if !defined(__SYNA_VPP_H__)
#define __SYNA_VPP_H__

#include <linux/device.h>
#include <linux/types.h>
#include <drm/drm_framebuffer.h>
#include "uapi/bm.h"
#include "berlin_meta.h"
#include "drm_syna_gem.h"
#include "vpp_defines.h"
#include "avio_type.h"
#include "vpp_vbuf.h"
#include "hal_vpp_wrap.h"

#ifndef CONFIG_SYNA_DRM_DISABLE_ROTATION
extern long device_rotate;
#endif

#define SYNA_WIDTH_MAX  3840
#define SYNA_HEIGHT_MAX 2160

#define SYNA_WIDTH_MIN  50
#define SYNA_HEIGHT_MIN 50

void syna_vpp_set_res_limit(struct drm_device *dev, u32 hdisplay, u32 vdisplay);

bool syna_vpp_clocks_set(struct device *dev,
			 void __iomem *syna_reg, u32 clock_in_mhz,
			 u32 hdisplay, u32 vdisplay);

void syna_vpp_set_updates_enabled(struct device *dev, void __iomem *syna_reg,
				  bool enable);

void syna_vpp_set_syncgen_enabled(struct device *dev, void __iomem *syna_reg,
				  bool enable);

void syna_vpp_set_powerdwn_enabled(struct device *dev, void __iomem *syna_reg,
				   bool enable);

void syna_vpp_set_vblank_enabled(struct device *dev, void __iomem *syna_reg,
				 bool enable);

bool syna_vpp_check_and_clear_vblank(struct device *dev,
				     void __iomem *syna_reg);

void syna_vpp_set_plane_enabled(struct device *dev, void __iomem *syna_reg,
				u32 plane, bool enable);

void syna_vpp_reset_planes(struct device *dev, void __iomem *syna_reg);

void syna_vpp_set_surface(struct drm_device *dev, void __iomem *syna_reg,
			  u32 plane, struct drm_framebuffer *fb,
			  u32 posx, u32 posy);

void syna_vpp_mode_set(struct device *dev, void __iomem *syna_reg,
		       u32 h_display, u32 v_display,
		       u32 hbps, u32 ht, u32 has,
		       u32 hlbs, u32 hfps, u32 hrbs,
		       u32 vbps, u32 vt, u32 vas,
		       u32 vtbs, u32 vfps, u32 vbbs, bool nhsync, bool nvsync);

void syna_vpp_exit(struct drm_device *dev);
int syna_vpp_dev_init(struct drm_device *dev);
void syna_vpp_wait_vsync(int Id);
void syna_vpp_load_config(int devID, void *pconfig);
void syna_vpp_reset_buffers(struct syna_gem_object *syna_obj);
void syna_vpp_push_buildin_frame(u32 plane);
void syna_vpp_push_buildin_null_frame(u32 plane);
void syna_vpp_dev_init_priv(struct drm_device *dev);
int syna_vpp_get_bm_details(struct dma_buf *dma_buf,
		       struct bm_pt_param *pt_param,
		       struct berlin_meta **bm_meta);
#endif /* __SYNA_VPP_H__ */
