// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/dma-buf.h>
#include <linux/refcount.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/dma-heap.h>
#include <uapi/bm.h>
#include "ispSS_shm.h"
#include "com_type.h"

DEFINE_MUTEX(ispSS_shm_mutex);

struct ispSS_shm_data {
	struct device mem_device[SHM_MAX_MEMORY_TYPE];
	struct dma_heap *heap[SHM_MAX_MEMORY_TYPE];
};

struct ispSS_shm_buf {
	struct device               *dev;
	void                        *vaddr; // Virtual address of kernel space only frmae
	void                        *paddr; // Physical address frame
	void                        *paddr_pt;//Physical address page table
	unsigned long               size;
	void                        *cookie;
	struct sg_table             *dma_sgt;
	refcount_t                  refcount;

	/* DMABUF related */
	struct dma_buf_attachment   *db_attach;
	struct iosys_map            *map;
	struct bm_pt_param          pt_param;
};

static struct ispSS_shm_data *ispSS_shm_data;

static void shm_release(struct device *dev)
{
	/*
	 * Nothing to do, just unregister sysfs here
	 *
	 */
}

/*********************************************/
/*         APIs for all SHM                  */
/*********************************************/
int ispSS_SHM_Init(struct device *dev)
{
	int ret = 0, i = 0, j, k;
	const char *cma_heap_name = "CMA-CUST-reserved";

	ret = mutex_lock_interruptible(&ispSS_shm_mutex);

	if (ret) {
		pr_err("%s: can't lock isp shm mutex\n", __func__);
		ret = -EINVAL;
		goto lock_fail;
	}

	ispSS_shm_data = devm_kzalloc(dev, sizeof(struct ispSS_shm_data), GFP_KERNEL);
	if (!ispSS_shm_data) {
		ret = -ENOMEM;
		goto mem_fail;
	}

	for (i = SHM_NONSECURE_CONTIG, j = -1; i < SHM_MAX_MEMORY_TYPE; i++) {
		device_initialize(&ispSS_shm_data->mem_device[i]);
		dev_set_name(&ispSS_shm_data->mem_device[i], "isp_shm%d", i);
		ispSS_shm_data->mem_device[i].release = shm_release;

		ret = dma_coerce_mask_and_coherent(&ispSS_shm_data->mem_device[i],
				DMA_BIT_MASK(32));
		if (ret) {
			pr_err("set dma mask 32b failed: %d\n", ret);
			goto device_free;
		}

		j++;
		ispSS_shm_data->heap[j] = dma_heap_find(cma_heap_name);
		if (!ispSS_shm_data->heap[j]) {
			ret = -ENOMEM;
			goto device_free;
		}
	}

	mutex_unlock(&ispSS_shm_mutex);
	return 0;

device_free:
	for (k = j; k >= 0; k--) {
		dma_heap_put(ispSS_shm_data->heap[k]);
		ispSS_shm_data->heap[k] = NULL;
	}

	for (k = i; k >= 0; k--)
		put_device(&ispSS_shm_data->mem_device[k]);

	devm_kfree(dev, ispSS_shm_data);
mem_fail:
	mutex_unlock(&ispSS_shm_mutex);
lock_fail:
	return ret;
}

int ispSS_SHM_Deinit(struct device *dev)
{
	int ret = 0, i = 0;

	ret = mutex_lock_interruptible(&ispSS_shm_mutex);

	if (ret) {
		pr_err("%s: can't lock isp shm mutex\n", __func__);
		ret = -EINVAL;
		goto lock_fail;
	}

	for (i = SHM_NONSECURE_CONTIG; i < SHM_MAX_MEMORY_TYPE; i++) {
		if (ispSS_shm_data->heap[i]) {
			dma_heap_put(ispSS_shm_data->heap[i]);
			ispSS_shm_data->heap[i] = NULL;
		}
		put_device(&ispSS_shm_data->mem_device[i]);
	}

	devm_kfree(dev, ispSS_shm_data);
	ispSS_shm_data = NULL;

	mutex_unlock(&ispSS_shm_mutex);
lock_fail:
	return ret;
}

int ispSS_SHM_Allocate(unsigned int  eMemType, unsigned int uiSize,
		unsigned int uiAlign, SHM_HANDLE *phShm, enum memory_type_t mem_type)
{
	struct ispSS_shm_buf *buf;
	struct bm_fb_param fb_param = { 0 };
	int ret = 0;
	struct device *dev;

	if (mem_type >= SHM_MAX_MEMORY_TYPE)
		return -EINVAL;

	dev = &ispSS_shm_data->mem_device[mem_type];

	ret = mutex_lock_interruptible(&ispSS_shm_mutex);

	if (ret) {
		pr_err("%s: can't lock isp shm mutex\n", __func__);
		ret = -EINVAL;
		goto clean_lock;
	}

	buf = kzalloc(sizeof(struct ispSS_shm_buf), GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto clean_buf;
	}

	buf->map = kzalloc(sizeof(struct iosys_map), GFP_KERNEL);
	if (IS_ERR_OR_NULL(buf->map)) {
		pr_err("%s: kzalloc failed for dma_map\n", __func__);
		ret = -ENOMEM;
		goto clean_alloc1;
	}

	buf->cookie = dma_heap_buffer_alloc(ispSS_shm_data->heap[mem_type], uiSize, 0, 0);

	if (IS_ERR_OR_NULL(buf->cookie)) {
		pr_err("%s: alloc of size %d failed: %ld\n", __func__,
				uiSize, PTR_ERR(buf->cookie));
		ret = -ENOMEM;
		goto clean_alloc2;
	}

	buf->db_attach = dma_buf_attach(buf->cookie, dev);
	if (IS_ERR_OR_NULL(buf->db_attach)) {
		pr_err("%s:dma attach failed\n",  __func__);
		ret = -ENOMEM;
		goto clean_attach;
	}

	buf->dma_sgt = dma_buf_map_attachment(buf->db_attach, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(buf->dma_sgt)) {
		pr_err("%s: dma map attachment failed for sgt\n", __func__);
		ret = -ENOMEM;
		goto clean_attachment;
	}

	if (dma_buf_begin_cpu_access(buf->cookie, DMA_BIDIRECTIONAL) != 0) {
		pr_err("%s: dma_buf_begin_cpu_access failed\n", __func__);
		ret = -ENOMEM;
		goto clean_cpu_access;
	}

	if (mem_type == SHM_NONSECURE_NON_CONTIG) {
		/* bm will attach buffer */
		fb_param.fb_type = SHM_MMU_GENERIC;
		ret = bm_create_pt(buf->cookie, 0, &fb_param, &buf->pt_param);
		if (ret) {
			pr_err("bm refuse to register: %d\n", ret);
			goto clean_cpu_access;
		}

		buf->paddr_pt = (void *)buf->pt_param.phy_addr;

		ret = dma_buf_vmap(buf->db_attach->dmabuf, buf->map);
		if (!ret && buf->map->vaddr) {
			buf->vaddr = buf->map->vaddr;
		} else {
			pr_err("%s: dma_buf_vmap failed\n", __func__);
			goto clean_vmap;
		}
	} else if (mem_type == SHM_NONSECURE_CONTIG) {

		ret = dma_buf_vmap(buf->db_attach->dmabuf, buf->map);
		if (!ret && buf->map->vaddr) {
			buf->vaddr = buf->map->vaddr;
		} else {
			pr_err("%s: dma_buf_vmap failed\n", __func__);
			goto clean_vmap;
		}

		/* get the physical address of the memory */
		buf->paddr = (void *)sg_dma_address(buf->dma_sgt->sgl);
	}
	/* Prevent the device from being released while the buffer is used */
	buf->dev = get_device(dev);
	buf->size = uiSize;

	pr_debug("%s :The vaddr %lx paddr %lx size %d\n",
			__func__, (unsigned long)buf->vaddr, (unsigned long)buf->paddr, uiSize);

	refcount_set(&buf->refcount, 1);

	*phShm = (SHM_HANDLE) buf;
	mutex_unlock(&ispSS_shm_mutex);
	return 0;

clean_vmap:
	dma_buf_vunmap(buf->cookie, buf->map);
	dma_buf_end_cpu_access(buf->cookie, DMA_BIDIRECTIONAL);
clean_cpu_access:
	dma_buf_unmap_attachment(buf->db_attach, buf->dma_sgt, DMA_BIDIRECTIONAL);
clean_attachment:
	dma_buf_detach(buf->cookie, buf->db_attach);
clean_attach:
	dma_heap_buffer_free(buf->cookie);
clean_alloc2:
	kfree(buf->map);
clean_alloc1:
	kfree(buf);
clean_buf:
	mutex_unlock(&ispSS_shm_mutex);
clean_lock:
	*phShm = (SHM_HANDLE) NULL;
	return ret;
}
EXPORT_SYMBOL(ispSS_SHM_Allocate);

int ispSS_SHM_Release(SHM_HANDLE phShm)
{
	struct ispSS_shm_buf *buf = (struct ispSS_shm_buf *)phShm;
	int ret = 0;

	ret = mutex_lock_interruptible(&ispSS_shm_mutex);

	if (ret) {
		pr_err("%s: can't lock isp shm mutex\n", __func__);
		ret = -EINVAL;
		goto lock_fail;
	}

	if (!refcount_dec_and_test(&buf->refcount))
		goto non_zero_ref;

	if (buf->vaddr)
		dma_buf_vunmap(buf->cookie, buf->map);

	if (buf->db_attach) {
		dma_buf_unmap_attachment(buf->db_attach, buf->dma_sgt, DMA_BIDIRECTIONAL);
		dma_buf_detach(buf->cookie, buf->db_attach);
		buf->db_attach = NULL;
	}

	if (buf->cookie)
		dma_heap_buffer_free(buf->cookie);

	/* heap_extra.free_cb() would free its mem_id */
	dma_buf_put(buf->cookie);

	put_device(buf->dev);
	kfree(buf->map);
	kfree(buf);

non_zero_ref:
	mutex_unlock(&ispSS_shm_mutex);
lock_fail:
	return ret;
}
EXPORT_SYMBOL(ispSS_SHM_Release);

int ispSS_SHM_mmap(SHM_HANDLE phShm, struct vm_area_struct *vma)
{
	struct ispSS_shm_buf *buf;
	int ret = 0;

	buf =  (struct ispSS_shm_buf *) phShm;
	if (!buf) {
		pr_err("%s: No buffer to map\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	vm_flags_set(vma, (vm_flags_t)(vma->vm_flags & (~VM_PFNMAP)));

	ret = dma_buf_mmap(buf->cookie, vma, 0);
	if (ret) {
		pr_err("%s: Remapping memory failed, error: %d\n", __func__, ret);
		goto out;
	}

	vm_flags_set(vma, (vm_flags_t)(vma->vm_flags | VM_DONTEXPAND | VM_DONTDUMP));
out:
	return ret;
}
EXPORT_SYMBOL(ispSS_SHM_mmap);

int ispSS_SHM_CleanCache(SHM_HANDLE phShm, unsigned int offset, unsigned int size)
{
	struct ispSS_shm_buf *buf;
	int ret = 0;

	buf =  (struct ispSS_shm_buf *) phShm;
	if (!buf) {
		pr_err("%s: Invalid handle\n", __func__);
		goto out;
	}
	dma_buf_end_cpu_access(buf->cookie, DMA_BIDIRECTIONAL);

out:
	return ret;
}
EXPORT_SYMBOL(ispSS_SHM_CleanCache);

int ispSS_SHM_InvalidCache(SHM_HANDLE phShm)
{
	struct ispSS_shm_buf *buf;
	int ret = 0;

	buf =  (struct ispSS_shm_buf *) phShm;
	if (!buf) {
		pr_err("%s: Invalid handle\n", __func__);
		goto out;
	}
	if (dma_buf_begin_cpu_access(buf->cookie, DMA_BIDIRECTIONAL) != 0) {
		pr_err("%s: dma_buf_begin_cpu_access failed\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

out:
	return ret;
}
EXPORT_SYMBOL(ispSS_SHM_InvalidCache);

int ispSS_SHM_GetVirtualAddress(SHM_HANDLE phShm, int uiOffset, void **pVirtAddr)
{
	struct ispSS_shm_buf *buf;
	int ret = 0;

	buf =  (struct ispSS_shm_buf *)phShm;
	if (!buf) {
		ret = -EINVAL;
		pr_err("%s: Invalid handle\n", __func__);
		goto out;
	}
	*pVirtAddr = buf->vaddr;
out:
	return ret;
}
EXPORT_SYMBOL(ispSS_SHM_GetVirtualAddress);

int ispSS_SHM_GetPhysicalAddress(SHM_HANDLE phShm, int uiOffset, void **pPhyAddr)
{
	struct ispSS_shm_buf *buf;
	int ret = 0;

	buf =  (struct ispSS_shm_buf *)phShm;
	if (!buf) {
		ret = -EINVAL;
		pr_err("%s: Invalid handle\n", __func__);
		goto out;
	}
	*pPhyAddr = buf->paddr;
out:
	return ret;
}
EXPORT_SYMBOL(ispSS_SHM_GetPhysicalAddress);

int ispSS_SHM_GetPageTableAddress(SHM_HANDLE phShm, void **pPhyAddr)
{
	struct ispSS_shm_buf *buf;
	int ret = 0;

	buf =  (struct ispSS_shm_buf *)phShm;
	if (!buf) {
		ret = -EINVAL;
		pr_err("%s: Invalid handle\n", __func__);
		goto out;
	}
	*pPhyAddr = buf->paddr_pt;
out:
	return ret;
}
EXPORT_SYMBOL(ispSS_SHM_GetPageTableAddress);

MODULE_IMPORT_NS(DMA_BUF);
