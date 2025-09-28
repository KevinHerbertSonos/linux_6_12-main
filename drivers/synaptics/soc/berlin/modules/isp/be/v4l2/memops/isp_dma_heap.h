/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ISP_DMAHEAP_H__
#define __ISP_DMAHEAP_H__

#include <linux/dma-mapping.h>
#include <linux/dma-heap.h>
#include <uapi/bm.h>
#include <media/videobuf2-memops.h>
#include "ispSS_shm.h"

struct isp_dma_heap_dev {
	struct device dev;
	struct dma_heap *heap;
	struct list_head link;
	enum memory_type_t mem_type;
};

struct isp_dma_buf {
	struct device               *dev;
	void                        *vaddr;//Virtual address of kernel space only frmae
	void                        *paddr;//Physical address frame
	void                        *paddr_pt;//Physical address page table
	unsigned long               size;
	void                        *cookie;
	unsigned long               attrs;
	enum dma_data_direction     dma_dir;
	struct sg_table             *dma_sgt;

	/* MMAP related */
	struct vb2_vmarea_handler   handler;
	refcount_t                  refcount;

	/* DMABUF related */
	struct dma_buf_attachment   *db_attach;
	struct iosys_map            *map;

	enum memory_type_t          mem_type;
	struct bm_pt_param          pt_param;
	struct vb2_buffer           *vb;
};

static inline void *
isp_dma_heap_plane_vaddr(struct vb2_buffer *vb, unsigned int plane_no)
{
	void *addr = vb2_plane_vaddr(vb, plane_no);

	return addr;
}

static inline void *
isp_dma_heap_plane_cookie(struct vb2_buffer *vb, unsigned int plane_no)
{
	void *addr = vb2_plane_cookie(vb, plane_no);

	return addr;
}

int isp_dma_heap_dev_alloc(struct device **pdev);
void isp_dma_heap_dev_release(void);
void *get_isp_dma_heap_alloc(struct isp_dma_heap_dev *memdev,
		unsigned long size);
void isp_dma_heap_free(void *buf_priv);
void *isp_dma_heap_get_phyaddr(void *handle);
void *isp_dma_heap_get_pagetbl_phyaddr(void *handle);
int vb2_isp_dma_heap_mmap(void *buf_priv, struct vm_area_struct *vma);
void vb2_isp_dma_heap_prepare(void *buf_priv);
void vb2_isp_dma_heap_finish(void *buf_priv);

extern const struct vb2_mem_ops isp_dma_contig_memops;

#endif
