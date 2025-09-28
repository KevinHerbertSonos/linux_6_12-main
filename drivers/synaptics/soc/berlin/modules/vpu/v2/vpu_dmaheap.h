/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2023 Synaptics Incorporated
 * Author: Hsia-Jun(Randy) Li <randy.li@synaptics.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __VPU_DMAHEAP_H__
#define __VPU_DMAHEAP_H__

#include <media/videobuf2-v4l2.h>
#include <linux/dma-mapping.h>

#define MTR_DEFAULT_N_BUFS		(16U)

enum syna_bm_addr_t { BM_DMA = 0, BM_MEMID = 1, BM_PTPHY = 2};

struct device *syna_vpu_dh_new_mtr_memdev(struct device *parent, size_t num,
					   size_t luma, size_t chroma);
int syna_vpu_dh_free_mtr_memdev(struct device *dev);
int syna_vpu_dh_get_reserved_mem(struct device *dev, dma_addr_t * dma_addr,
				  unsigned int *size);

static inline dma_addr_t
syna_vpu_bm_dh_plane_memid(struct vb2_buffer *vb, unsigned int plane_no)
{
	dma_addr_t *addr = vb2_plane_cookie(vb, plane_no);
	if (!addr)
		return 0;

	return *addr;
}

void *vb2_syna_bm_cookie(struct vb2_buffer *vb, void *buf_priv);
dma_addr_t vb2_syna_bm_get_addr(struct vb2_buffer *vb, int plane_no,
			    enum syna_bm_addr_t type);
bool vb2_syna_bm_has_cache_carer(struct vb2_buffer *vb, int plane_no);

void vb2_syna_dh_bm_put(void *buf_priv);
void *syna_dh_bm_alloc(struct device *dev, unsigned long size);
unsigned long syna_dh_bm_get_size(void *buf_priv);
int vb2_syna_fill_mtr_meta(struct vb2_buffer *vb, int plane_no,
			    void *meta, u32 flags);

extern struct device *cma_dh_dev;
extern struct device *sys_iommu_dev;
extern struct device *secure_dh_dev;

extern const struct vb2_mem_ops syna_bm_dh_memops;


#endif
