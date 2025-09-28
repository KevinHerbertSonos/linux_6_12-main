/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Copyright (C) 2020 Synaptics Incorporated
 */
#ifndef __UAPI_SYNA_BM__
#define __UAPI_SYNA_BM__

#include <linux/ioctl.h>
#include <linux/types.h>
#ifdef __KERNEL__
#include <linux/dma-buf.h>
#endif

enum shm_fb_type {
	/* generic physical scatter buffer */
	SHM_MMU_GENERIC,
	/*
	 * UV entry also page aligned,
	 * must set total size and y size together
	 */
	SHM_MMU_UV_ALIGN,
};

enum shm_allocation_flag {
	/* the buffer allocated is a cached buffer */
	SHM_FLAG_CACHED = 0x1,
	/* the buffer allocated is a non-cached buffer */
	SHM_FLAG_NONCACHED = 0x2,
	/* the buffer allocated is non-secure */
	SHM_FLAG_NONSECURE = 0x100,
	/* the buffer allocated is secure */
	SHM_FLAG_SECURE = 0x200,
};

struct bm_pt_param {
	__u64 phy_addr;
	__u64 len;
	__u64 mem_id;
};

struct bm_uv_align_param {
	__u32 y_size;
};

/* extra parameter for frame buffer */
struct bm_fb_param {
	__u32 fb_type;
	struct bm_uv_align_param uva_param;
};

struct bm_fb_data {
	__u32 fd;
	__u32 flags;
	struct bm_fb_param fb_param;
	struct bm_pt_param pt_param;
};

struct bm_pt_data {
	__u32 fd;
	struct bm_pt_param pt_param;
};

#define BM_IOC_ALLOC_PT		_IOWR('B', 1, struct bm_fb_data)
#define BM_IOC_GET_PT		_IOWR('B', 2, struct bm_pt_data)

#ifdef __KERNEL__
int bm_create_pt(struct dma_buf *dmabuf, u32 flags,
		 struct bm_fb_param *fb_param, struct bm_pt_param *pt_param);
int bm_fetch_pt(struct dma_buf *dmabuf, struct bm_pt_param *pt_param);
struct berlin_meta *bm_fetch_meta(struct dma_buf *dmabuf);
#endif

#endif
