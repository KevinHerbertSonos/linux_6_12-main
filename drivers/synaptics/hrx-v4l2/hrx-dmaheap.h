// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef __HRX_DMAHEAP_H__
#define __HRX_DMAHEAP_H__

#include <media/videobuf2-v4l2.h>
#include <linux/dma-mapping.h>

#ifdef CONFIG_DMABUF_HEAPS_EXTRA

typedef enum {
	SHM_NONSECURE_CONTIG  = 0,
	SHM_MAX_MEMORY_TYPE,
} memory_type_t;

static inline void *
syna_hrx_dh_plane_vaddr(struct vb2_buffer *vb, unsigned int plane_no)
{
	void *addr = vb2_plane_vaddr(vb, plane_no);

	return addr;
}

static inline void *
syna_hrx_dh_plane_cookie(struct vb2_buffer *vb, unsigned int plane_no)
{
	void *addr = vb2_plane_cookie(vb, plane_no);

	return addr;
}

int syna_hrx_dh_memdev_alloc(struct device **pdev);
void syna_hrx_dh_memdev_release(void);
void *syna_hrx_get_frame_phyaddr(void *handle);
void *syna_hrx_get_pt_phyaddr(void *handle);

extern const struct vb2_mem_ops syna_hrx_dma_contig_memops;

#endif
/* CONFIG_DMABUF_HEAPS_EXTRA */

#endif
