// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/dma-heap.h>
#include <uapi/bm.h>
#include <media/videobuf2-memops.h>
#include "isp_dma_heap.h"

DEFINE_MUTEX(isp_dma_heap_mutex);
static bool isp_dma_heap_memdev_init;
static struct list_head isp_dma_heap_memdevs;


static struct isp_dma_heap_dev *alloc_memdev(const char *heap_name,
		enum memory_type_t mem_type)
{
	struct isp_dma_heap_dev *memdev;
	struct dma_heap *heap;
	int ret;

	memdev = kzalloc(sizeof(struct isp_dma_heap_dev), GFP_KERNEL);
	if (!memdev) {
		ret = -ENOMEM;
		return NULL;
	}

	device_initialize(&memdev->dev);
	dev_set_name(&memdev->dev, "isp_dma:%s", heap_name);

	ret = dma_coerce_mask_and_coherent(&memdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		pr_err("set dma mask 32b failed: %d\n", ret);
		kfree(memdev);
		return NULL;
	}

	heap = dma_heap_find(heap_name);
	if (!heap) {
		kfree(memdev);
		return NULL;
	}
	memdev->heap = heap;
	memdev->mem_type = mem_type;

	if (device_add(&memdev->dev)) {
		dma_heap_put(memdev->heap);
		put_device(&memdev->dev);
		kfree(memdev);
		return NULL;
	}

	list_add_tail(&memdev->link, &isp_dma_heap_memdevs);
	return memdev;
}

static void get_dma_heap_device(struct device **pdev)
{
	int count = 0;
	struct list_head *cur;
	struct isp_dma_heap_dev *memdev;

	list_for_each(cur, &isp_dma_heap_memdevs) {
		memdev = list_entry(cur, struct isp_dma_heap_dev, link);
		pdev[count++] = &memdev->dev;
	}

}

int isp_dma_heap_dev_alloc(struct device **pdev)
{
	struct isp_dma_heap_dev *memdev;
	const char *cma_heap_name = "CMA-CUST-reserved";
	const char *cma_non_contag_heap_name = "system_cust";
	int ret = 0;

	ret = mutex_lock_interruptible(&isp_dma_heap_mutex);
	if (ret)
		return ret;

	if (isp_dma_heap_memdev_init) {
		get_dma_heap_device(pdev);
		mutex_unlock(&isp_dma_heap_mutex);
		return 0;
	}

	INIT_LIST_HEAD(&isp_dma_heap_memdevs);

	memdev = alloc_memdev(cma_heap_name, SHM_NONSECURE_CONTIG);
	if (!memdev) {
		ret = -ENOMEM;
		goto failed;
	}

	pdev[0] = &memdev->dev;

	memdev = alloc_memdev(cma_non_contag_heap_name, SHM_NONSECURE_NON_CONTIG);
	if (!memdev) {
		ret = -ENOMEM;
		goto failed;
	}

	pdev[1] = &memdev->dev;

	isp_dma_heap_memdev_init = true;
failed:
	mutex_unlock(&isp_dma_heap_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(isp_dma_heap_dev_alloc);

void isp_dma_heap_dev_release(void)
{
	struct isp_dma_heap_dev *memdev;
	struct list_head *cur, *next;
	int ret;

	ret = mutex_lock_interruptible(&isp_dma_heap_mutex);
	if (ret) {
		pr_err("can't lock isp dma-heap mutex\n");
		return;
	}

	if (!isp_dma_heap_memdev_init) {
		mutex_unlock(&isp_dma_heap_mutex);
		return;
	}

	list_for_each_safe(cur, next, &isp_dma_heap_memdevs) {
		memdev = list_entry(cur, struct isp_dma_heap_dev, link);
		dma_heap_put(memdev->heap);
		device_del(&memdev->dev);
		list_del(&memdev->link);
		kfree(memdev);
	}

	isp_dma_heap_memdev_init = false;
	mutex_unlock(&isp_dma_heap_mutex);
}
EXPORT_SYMBOL_GPL(isp_dma_heap_dev_release);

/*********************************************/
/*         callbacks for all buffers         */
/*********************************************/

static void *vb2_isp_dma_heap_cookie(struct vb2_buffer *vb, void *buf_priv)
{
	struct isp_dma_buf *buf = buf_priv;
	return buf;
}

static void *vb2_isp_dma_heap_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct isp_dma_buf *buf = buf_priv;

	return buf->vaddr;
}

static unsigned int vb2_isp_dma_heap_num_users(void *buf_priv)
{
	struct isp_dma_buf *buf = buf_priv;

	return refcount_read(&buf->refcount);
}

void vb2_isp_dma_heap_prepare(void *buf_priv)
{
	struct isp_dma_buf *buf = buf_priv;

	dma_buf_end_cpu_access(buf->cookie, buf->dma_dir);
}
EXPORT_SYMBOL(vb2_isp_dma_heap_prepare);

void vb2_isp_dma_heap_finish(void *buf_priv)
{
	struct isp_dma_buf *buf = buf_priv;

	dma_buf_begin_cpu_access(buf->cookie, buf->dma_dir);
}
EXPORT_SYMBOL(vb2_isp_dma_heap_finish);

int vb2_isp_dma_heap_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct isp_dma_buf *buf = buf_priv;
	int ret;

	if (!buf) {
		pr_err("%s: No buffer to map\n", __func__);
		return -EINVAL;
	}

	vm_flags_set(vma, (vm_flags_t)(vma->vm_flags & (~VM_PFNMAP)));

	ret = dma_buf_mmap(buf->cookie, vma, 0);
	if (ret) {
		pr_err("%s: Remapping memory failed, error: %d\n", __func__, ret);
		return ret;
	}
	vm_flags_set(vma, (vm_flags_t)(vma->vm_flags | VM_DONTEXPAND | VM_DONTDUMP));
	vma->vm_private_data = &buf->handler;
	vma->vm_ops = &vb2_common_vm_ops;

	vma->vm_ops->open(vma);

	return 0;
}
EXPORT_SYMBOL(vb2_isp_dma_heap_mmap);

/*********************************************/
/*        callbacks for MMAP buffers         */
/*********************************************/

void isp_dma_heap_free(void *buf_priv)
{
	struct isp_dma_buf *buf = buf_priv;

	if (!refcount_dec_and_test(&buf->refcount))
		return;

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
}
EXPORT_SYMBOL(isp_dma_heap_free);

static void vb2_isp_dma_heap_put(void *buf_priv)
{
	isp_dma_heap_free(buf_priv);
}

void *get_isp_dma_heap_alloc(struct isp_dma_heap_dev *memdev,
		unsigned long size)
{
	struct isp_dma_buf *buf;
	struct device *dev;
	struct bm_fb_param fb_param = { 0, };
	int ret;

	buf = kzalloc(sizeof(struct isp_dma_buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->map = kzalloc(sizeof(struct iosys_map), GFP_KERNEL);
	if (IS_ERR_OR_NULL(buf->map)) {
		pr_err("%s: kzalloc failed\n", __func__);
		ret = -ENOMEM;
		goto failed_alloc1;
	}

	dev = &memdev->dev;
	buf->cookie = dma_heap_buffer_alloc(memdev->heap, size, 0, 0);

	if (IS_ERR_OR_NULL(buf->cookie)) {
		pr_err("%s: alloc of size %lu failed: %ld\n", __func__,
				size, PTR_ERR(buf->cookie));
		ret = -ENOMEM;
		goto failed_alloc2;
	}

	pr_debug("The name of DMA buf %s\n", ((struct dma_buf *)(buf->cookie))->exp_name);
	buf->mem_type = memdev->mem_type;

	buf->db_attach = dma_buf_attach(buf->cookie, dev);
	if (IS_ERR(buf->db_attach)) {
		pr_err("%s:dma attach failed\n",  __func__);
		ret = PTR_ERR(buf->db_attach);
		goto failed_attach;
	}

	buf->dma_sgt = dma_buf_map_attachment(buf->db_attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(buf->dma_sgt)) {
		pr_err("%s: dma map attachment failed for sgt\n", __func__);
		ret = PTR_ERR(buf->dma_sgt);
		goto failed_attachment;
	}

	ret = dma_buf_begin_cpu_access(buf->cookie, DMA_BIDIRECTIONAL);
	if (ret != 0) {
		pr_err("%s: dma_buf_begin_cpu_access failed\n", __func__);
		goto failed_cpu_access;
	}

	if (memdev->mem_type == SHM_NONSECURE_NON_CONTIG) {
		/* bm will attach buffer */
		fb_param.fb_type = SHM_MMU_GENERIC;
		ret = bm_create_pt(buf->cookie, 0, &fb_param, &buf->pt_param);
		if (ret) {
			pr_err("bm refuse to register: %d\n", ret);
			goto failed_cpu_access;
		}

		buf->paddr_pt = (void *)buf->pt_param.phy_addr;
	} else if (memdev->mem_type == SHM_NONSECURE_CONTIG) {

		ret = dma_buf_vmap(buf->db_attach->dmabuf, buf->map);
		if (!ret && buf->map->vaddr) {
			buf->vaddr = buf->map->vaddr;
		} else {
			pr_err("%s: dma_buf_vmap failed\n", __func__);
			goto failed_vmap;
		}

		/* get the physical address of the memory */
		buf->paddr = (void *)sg_dma_address(buf->dma_sgt->sgl);
	}

	/* Prevent the device from being released while the buffer is used */
	buf->dev = get_device(dev);
	buf->size = size;

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = vb2_isp_dma_heap_put;
	buf->handler.arg = buf;

	pr_debug("%s :The vaddr %lx paddr %lx pt_addr %lx size %lx size1 %lx\n",
			__func__, (unsigned long)buf->vaddr, (unsigned long)buf->paddr,
			(unsigned long)buf->paddr_pt, (unsigned long) buf->pt_param.len, size);

	refcount_set(&buf->refcount, 1);

	return buf;
failed_vmap:
	dma_buf_end_cpu_access(buf->cookie, DMA_BIDIRECTIONAL);
failed_cpu_access:
	dma_buf_unmap_attachment(buf->db_attach, buf->dma_sgt, DMA_BIDIRECTIONAL);
failed_attachment:
	dma_buf_detach(buf->cookie, buf->db_attach);
failed_attach:
	dma_heap_buffer_free(buf->cookie);
failed_alloc2:
	kfree(buf->map);
failed_alloc1:
	kfree(buf);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL(get_isp_dma_heap_alloc);

static void *vb2_isp_dma_heap_alloc(struct vb2_buffer *vb, struct device *dev,
		unsigned long size)
{
	struct isp_dma_buf *buf;
	struct isp_dma_heap_dev *memdev;
	int ret;

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	memdev = container_of(dev, struct isp_dma_heap_dev, dev);

	buf  =  get_isp_dma_heap_alloc(memdev, size);
	if (IS_ERR_OR_NULL(buf)) {
		ret = -ENOMEM;
		return ERR_PTR(ret);
	}

	buf->attrs = vb->vb2_queue->dma_attrs;

	/* Prevent the device from being released while the buffer is used */
	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->vb = vb;

	refcount_set(&buf->refcount, 1);

	return buf;
}

/*********************************************/
/*         DMABUF ops for exporters          */
/*********************************************/
static int vb2_dma_heap_dmabuf_ops_attach(struct dma_buf *dbuf, struct dma_buf_attachment
		*dbuf_attach)
{
	struct isp_dma_buf *buf = dbuf->priv;
	struct dma_buf *isp_dma_heap_dbuf = buf->cookie;
	int ret = 0;

	if (isp_dma_heap_dbuf->ops->attach)
		ret = isp_dma_heap_dbuf->ops->attach(isp_dma_heap_dbuf, dbuf_attach);

	return ret;
}

static void vb2_dma_heap_dmabuf_ops_detach(struct dma_buf *dbuf,
		struct dma_buf_attachment *db_attach)
{
	struct isp_dma_buf *buf = dbuf->priv;
	struct dma_buf *isp_dma_heap_dbuf = buf->cookie;

	if (isp_dma_heap_dbuf->ops->detach)
		isp_dma_heap_dbuf->ops->detach(isp_dma_heap_dbuf, db_attach);
}

static struct sg_table *vb2_dma_heap_dmabuf_ops_map(struct dma_buf_attachment
		*db_attach,
		enum dma_data_direction
		dma_dir)
{
	struct dma_buf *vb2_dbuf = db_attach->dmabuf;
	struct isp_dma_buf *buf = vb2_dbuf->priv;
	struct dma_buf *isp_dma_heap_dbuf = buf->cookie;
	struct sg_table *sgt = NULL;

	if (isp_dma_heap_dbuf->ops->map_dma_buf) {
		db_attach->dmabuf = isp_dma_heap_dbuf;
		sgt = isp_dma_heap_dbuf->ops->map_dma_buf(db_attach, dma_dir);
		db_attach->dmabuf = vb2_dbuf;
	}

	return sgt;
}

static void vb2_dma_heap_dmabuf_ops_unmap(struct dma_buf_attachment *db_attach,
		struct sg_table *sgt,
		enum dma_data_direction dma_dir)
{
	/* nothing to be done here */
}

	static int
vb2_dma_heap_dmabuf_ops_begin_cpu_access(struct dma_buf *dbuf,
		enum dma_data_direction direction)
{
	struct isp_dma_buf *buf = dbuf->priv;
	struct dma_buf *isp_dma_heap_dbuf = buf->cookie;

	if (isp_dma_heap_dbuf->ops->begin_cpu_access)
		return isp_dma_heap_dbuf->ops->begin_cpu_access(isp_dma_heap_dbuf, direction);

	return 0;
}

	static int
vb2_dma_heap_dmabuf_ops_end_cpu_access(struct dma_buf *dbuf,
		enum dma_data_direction direction)
{
	struct isp_dma_buf *buf = dbuf->priv;
	struct dma_buf *isp_dma_heap_dbuf = buf->cookie;

	if (isp_dma_heap_dbuf->ops->end_cpu_access)
		return isp_dma_heap_dbuf->ops->end_cpu_access(isp_dma_heap_dbuf, direction);

	return 0;
}

static int vb2_dma_heap_dmabuf_ops_vmap(struct dma_buf *dbuf,
		struct iosys_map *map)
{
	struct isp_dma_buf *buf = dbuf->priv;
	int ret = 0;

	if (!buf->vaddr) {
		WARN_ON(!buf->db_attach);
		ret = dma_buf_vmap(buf->db_attach->dmabuf, buf->map);

		if (!ret && buf->map->vaddr) {
			buf->vaddr = buf->map->vaddr;
		} else {
			pr_err("%s: dma_buf_vmap failed\n", __func__);
			goto failed_vmap;
		}
		memcpy(map, buf->map, sizeof(struct iosys_map));
	} else {
		iosys_map_set_vaddr(map, buf->vaddr);
	}

	return 0;
failed_vmap:
	return ret;
}

static void vb2_dma_heap_dmabuf_ops_vunmap(struct dma_buf *dbuf,
		struct iosys_map *map)
{
	struct isp_dma_buf *buf = dbuf->priv;

	if (buf->vaddr &&  buf->db_attach) {
		dma_buf_vunmap(buf->db_attach->dmabuf, map);
		buf->vaddr = NULL;
	}
}

static int vb2_dma_heap_dmabuf_ops_mmap(struct dma_buf *dbuf,
		struct vm_area_struct *vma)
{
	return vb2_isp_dma_heap_mmap(dbuf->priv, vma);
}

static void vb2_dma_heap_dmabuf_ops_release(struct dma_buf *dbuf)
{
	vb2_isp_dma_heap_put(dbuf->priv);
}

static int vb2_dma_heap_get_flags(struct dma_buf *dmabuf, unsigned long *flags)
{
	struct isp_dma_buf *buf = dmabuf->priv;
	struct dma_buf *isp_dma_heap_dbuf = buf->cookie;

	if (isp_dma_heap_dbuf->ops->get_flags)
		return isp_dma_heap_dbuf->ops->get_flags(isp_dma_heap_dbuf, flags);

	return 0;
}

/* static void *vb2_dma_heap_get_meta(struct dma_buf *dbuf)
 * {
 *		struct isp_dma_buf *buf = dbuf->priv;
 *		struct dma_buf *isp_dma_heap_dbuf = buf->cookie;
 *		if (isp_dma_heap_dbuf->ops->get_meta)
 *			return isp_dma_heap_dbuf->ops->get_meta(isp_dma_heap_dbuf);
 *		return 0;
 *	}
 */

static const struct dma_buf_ops vb2_dma_heap_dmabuf_ops = {
	.attach = vb2_dma_heap_dmabuf_ops_attach,
	.detach = vb2_dma_heap_dmabuf_ops_detach,
	.map_dma_buf = vb2_dma_heap_dmabuf_ops_map,
	.unmap_dma_buf = vb2_dma_heap_dmabuf_ops_unmap,
	.begin_cpu_access = vb2_dma_heap_dmabuf_ops_begin_cpu_access,
	.end_cpu_access = vb2_dma_heap_dmabuf_ops_end_cpu_access,
	.vmap = vb2_dma_heap_dmabuf_ops_vmap,
	.vunmap = vb2_dma_heap_dmabuf_ops_vunmap,
	.mmap = vb2_dma_heap_dmabuf_ops_mmap,
	.release = vb2_dma_heap_dmabuf_ops_release,
	.get_flags = vb2_dma_heap_get_flags,
	//.get_meta = vb2_dma_heap_get_meta,
};


/*********************************************/
/*       callbacks for DMABUF buffers        */
/*********************************************/

static struct dma_buf *vb2_isp_dma_heap_get_dmabuf(struct vb2_buffer *vb,
		void *buf_priv,
		unsigned long flags)
{
	struct isp_dma_buf *buf = buf_priv;
	struct dma_buf *dbuf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	exp_info.ops = &vb2_dma_heap_dmabuf_ops;
	exp_info.size = buf->size;
	exp_info.flags = flags;
	exp_info.priv = buf;

	if (WARN_ON(!buf->cookie))
		return NULL;

	dbuf = dma_buf_export(&exp_info);
	if (IS_ERR(dbuf))
		return NULL;

	/* dmabuf keeps reference to vb2 buffer */
	refcount_inc(&buf->refcount);

	return dbuf;
}

static int vb2_isp_dma_heap_map_dmabuf(void *mem_priv)
{
	struct isp_dma_buf *buf = mem_priv;
	int ret = 0;

	if (!buf) {
		pr_err("%s: No buffer to map\n", __func__);
		return -EINVAL;
	}

	if (!buf->db_attach) {
		pr_err("%s: No attachment to map\n", __func__);
		return -EINVAL;
	}

	buf->dma_sgt = dma_buf_map_attachment(buf->db_attach, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(buf->dma_sgt)) {
		pr_err("%s: dma map attachment failed for sgt\n", __func__);
		return -ENOMEM;
	}

	//Get the physical address of the memory
	buf->paddr = (void *)sg_dma_address(buf->dma_sgt->sgl);
	pr_debug("%s: is called with pt %llx paddr %llx\n", __func__,
			(unsigned long long)buf->paddr_pt, (unsigned long long) buf->paddr);
	buf->vaddr = NULL;

	return ret;
}

static void vb2_isp_dma_heap_unmap_dmabuf(void *mem_priv)
{
	struct isp_dma_buf *buf = mem_priv;

	if (WARN_ON(!buf->db_attach)) {
		pr_err("%s: trying to unpin a not attached buffer\n", __func__);
		return;
	}

	dma_buf_unmap_attachment(buf->db_attach, buf->dma_sgt, DMA_BIDIRECTIONAL);
	buf->paddr = 0;
}

static void vb2_isp_dma_heap_detach_dmabuf(void *mem_priv)
{
	struct isp_dma_buf *buf = mem_priv;

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

static void *vb2_isp_dma_heap_attach_dmabuf(struct vb2_buffer *vb,
		struct device *dev, struct dma_buf *dbuf,
		unsigned long size)
{
	struct isp_dma_buf *buf;
	struct dma_buf_attachment *dba;
	int ret = 0;

	if (dbuf->size < size)
		return ERR_PTR(-EFAULT);

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	ret = bm_fetch_pt(dbuf, &buf->pt_param);
	if (ret) {
		pr_debug("bm refuse to register: %d\n", ret);
		/* At this point we don't know if the memory is contiguous or not. For the
		 * contiguous case, error should not be returned, attach should still happen
		 */
	} else {
		buf->paddr_pt = (void *)buf->pt_param.phy_addr;
	}
	buf->dev = dev;
	buf->vb = vb;
	buf->cookie = dbuf;
	dba = dma_buf_attach(dbuf, buf->dev);
	if (IS_ERR(dba)) {
		pr_err("failed to attach dmabuf\n");
		kfree(buf);
		return dba;
	}

	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->size = size;
	buf->db_attach = dba;

	return buf;
}

/*********************************************/
/*       ISP DMA CONTIG exported functions       */
/*********************************************/

const struct vb2_mem_ops isp_dma_contig_memops = {
	.alloc		= vb2_isp_dma_heap_alloc,
	.put		= vb2_isp_dma_heap_put,
	.mmap		= vb2_isp_dma_heap_mmap,
	.cookie		= vb2_isp_dma_heap_cookie,
	.vaddr		= vb2_isp_dma_heap_vaddr,
	.prepare	= vb2_isp_dma_heap_prepare,
	.finish		= vb2_isp_dma_heap_finish,
	.num_users	= vb2_isp_dma_heap_num_users,
	.get_dmabuf	= vb2_isp_dma_heap_get_dmabuf,
	.map_dmabuf	= vb2_isp_dma_heap_map_dmabuf,
	.unmap_dmabuf	= vb2_isp_dma_heap_unmap_dmabuf,
	.attach_dmabuf	= vb2_isp_dma_heap_attach_dmabuf,
	.detach_dmabuf	= vb2_isp_dma_heap_detach_dmabuf,
};
EXPORT_SYMBOL_GPL(isp_dma_contig_memops);

void *isp_dma_heap_get_phyaddr(void *handle)
{
	struct isp_dma_buf *buf;

	buf =  (struct isp_dma_buf *) handle;
	if (!buf)
		return NULL;

	return buf->paddr;
}
EXPORT_SYMBOL_GPL(isp_dma_heap_get_phyaddr);

void *isp_dma_heap_get_pagetbl_phyaddr(void *handle)
{
	struct isp_dma_buf *buf;

	buf =  (struct isp_dma_buf *) handle;
	if (!buf)
		return NULL;

	return buf->paddr_pt;
}
EXPORT_SYMBOL_GPL(isp_dma_heap_get_pagetbl_phyaddr);

MODULE_IMPORT_NS(SYNA_BM);
MODULE_IMPORT_NS(DMA_BUF);
MODULE_AUTHOR("Synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DMA Heap memory driver");
