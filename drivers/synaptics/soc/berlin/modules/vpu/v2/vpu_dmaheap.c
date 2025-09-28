/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
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

#include <linux/module.h>
#include <linux/dma-buf.h>
#include <linux/refcount.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/dma-heap.h>

#include <uapi/bm.h>
#include <berlin_meta.h>

#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-memops.h>

#include <vpu_mfd.h>
#include "vpu_kernel_compatibility_wrap.h"
#include "vpu_dmaheap.h"

static DEFINE_MUTEX(vpu_dh_mutex);
static bool dh_memdev_init = false;
static struct list_head dh_memdevs;

struct syna_vpu_dh_memdev {
	struct device dev;
	struct dma_heap *heap;

	bool is_reserved_mem;
	struct dma_buf *reserved_buf;
	dma_addr_t reserved_base;
	unsigned long luma, chroma;
	unsigned long reserved_size;

	struct list_head link;
};

struct vb2_syna_bm_buf {
	struct berlin_meta *bm_meta;
	struct device *dev;
	void *vaddr;
	unsigned long size;
	void *cookie;
	dma_addr_t dma_addr;
	unsigned long attrs;
	enum dma_data_direction dma_dir;

	struct bm_pt_param pt_param;

	/* MMAP related */
	struct vb2_vmarea_handler handler;
	refcount_t refcount;

	/* DMABUF related */
	struct dma_buf_attachment *db_attach;
	struct sg_table *dma_sgt;

	struct vb2_buffer *vb;
};

struct device *cma_dh_dev = NULL;
struct device *sys_iommu_dev = NULL;
struct device *secure_dh_dev = NULL;

static struct syna_vpu_dh_memdev *alloc_memdev(const char *heap_name,
					       bool add_device)
{
	struct syna_vpu_dh_memdev *memdev;
	struct dma_heap *heap;
	int ret;

	memdev = kzalloc(sizeof(*memdev), GFP_KERNEL);
	if (!memdev) {
		ret = -ENOMEM;
		return NULL;
	}

	device_initialize(&memdev->dev);
	dev_set_name(&memdev->dev, "syna_vpu_dma:%s", heap_name);

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

	if (add_device && device_add(&memdev->dev)) {
		dma_heap_put(memdev->heap);
		put_device(&memdev->dev);
		kfree(memdev);
		return NULL;
	}

	list_add(&memdev->link, &dh_memdevs);
	return memdev;
}

int syna_vpu_dh_memdev_alloc(void)
{
	struct syna_vpu_dh_memdev *memdev;
	const char *cma_heap_name = "CMA-CUST-reserved";
	const char *secure_heap_name = "Secure";
	const char *iommu_heap_name = "system_cust";

	struct list_head *cur, *next;
	int ret;

	ret = mutex_lock_interruptible(&vpu_dh_mutex);
	if (ret)
		return ret;

	if (dh_memdev_init) {
		mutex_unlock(&vpu_dh_mutex);
		return 0;
	}

	INIT_LIST_HEAD(&dh_memdevs);

	memdev = alloc_memdev(cma_heap_name, true);
	if (memdev)
		cma_dh_dev = &memdev->dev;
	else
		goto failed;

	memdev = alloc_memdev(secure_heap_name, true);
	if (memdev)
		secure_dh_dev = &memdev->dev;
	else
		goto failed;

	memdev = alloc_memdev(iommu_heap_name, true);
	if (memdev)
		sys_iommu_dev = &memdev->dev;
	else
		goto failed;

	dh_memdev_init = true;
	mutex_unlock(&vpu_dh_mutex);
	return 0;

failed:
	list_for_each_safe(cur, next, &dh_memdevs) {
		memdev = list_entry(cur, struct syna_vpu_dh_memdev, link);
		dma_heap_put(memdev->heap);
		device_del(&memdev->dev);
		list_del(&memdev->link);
		kfree(memdev);
	}

	return ret;
}

struct device *syna_vpu_dh_new_mtr_memdev(struct device *parent,
					  size_t num, size_t luma,
					  size_t chroma)
{
	struct syna_vpu_dh_memdev *memdev;
	const char *cma_heap_name = "CMA-CUST-reserved";
	struct dma_buf *dmabuf;
	struct bm_fb_param fb_param = { 0, };
	struct bm_pt_param pt_param = { 0, };
	size_t size;
	int ret;

	ret = mutex_lock_interruptible(&vpu_dh_mutex);
	if (ret)
		return ERR_PTR(ret);

	if (!dh_memdev_init) {
		mutex_unlock(&vpu_dh_mutex);
		return ERR_PTR(-ENODEV);
	}

	memdev = alloc_memdev(cma_heap_name, false);
	if (!memdev)
		return NULL;

	memdev->is_reserved_mem = true;
	mutex_unlock(&vpu_dh_mutex);

	size = (luma + chroma) * num;
	dmabuf = dma_heap_buffer_alloc(memdev->heap, size, 0, 0);
	if (IS_ERR_OR_NULL(dmabuf)) {
		dev_err(&memdev->dev, "alloc of size %lu failed: %ld\n",
			size, PTR_ERR(dmabuf));
		goto failed;
	}
	fb_param.fb_type = SHM_MMU_GENERIC;
	ret = bm_create_pt(dmabuf, 0, &fb_param, &pt_param);
	if (ret) {
		pr_err("bm refuse to register: %d\n", ret);
		dma_buf_put(dmabuf);
		goto failed;
	}

	memdev->reserved_base = (dma_addr_t) pt_param.mem_id;
	memdev->dev.parent = parent;
	dev_set_name(&memdev->dev, "mtr-%llx", memdev->reserved_base);
	if (device_add(&memdev->dev))
		goto failed;

	memdev->reserved_buf = dmabuf;
	memdev->luma = luma;
	memdev->chroma = chroma;
	memdev->reserved_size = size;

	return &memdev->dev;

failed:
	syna_vpu_dh_free_mtr_memdev(&memdev->dev);
	return ERR_PTR(ret);
}

int syna_vpu_dh_free_mtr_memdev(struct device *dev)
{
	struct syna_vpu_dh_memdev *memdev =
	    container_of(dev, struct syna_vpu_dh_memdev, dev);
	int ret;
	if (!dev)
		return -EINVAL;

	ret = mutex_lock_interruptible(&vpu_dh_mutex);
	if (ret) {
		pr_err("can't lock vpu dma-heap mutex\n");
		mutex_unlock(&vpu_dh_mutex);
		return ret;
	}

	list_del(&memdev->link);
	mutex_unlock(&vpu_dh_mutex);

	dma_buf_put(memdev->reserved_buf);

	device_del(&memdev->dev);
	kfree(memdev);

	return 0;
}

void syna_vpu_dh_memdev_release(void)
{
	struct syna_vpu_dh_memdev *memdev;
	struct list_head *cur, *next;
	int ret;

	ret = mutex_lock_interruptible(&vpu_dh_mutex);
	if (ret) {
		pr_err("can't lock vpu dma-heap mutex\n");
		return;
	}

	if (!dh_memdev_init) {
		mutex_unlock(&vpu_dh_mutex);
		return;
	}

	list_for_each_safe(cur, next, &dh_memdevs) {
		memdev = list_entry(cur, struct syna_vpu_dh_memdev, link);
		dma_heap_put(memdev->heap);
		device_del(&memdev->dev);
		list_del(&memdev->link);
		kfree(memdev);
	}

	dh_memdev_init = false;
	mutex_unlock(&vpu_dh_mutex);
}

/*********************************************/
/*         callbacks for all buffers         */
/*********************************************/

void *vb2_syna_bm_cookie(struct vb2_buffer *vb, void *buf_priv)
{
	struct vb2_syna_bm_buf *buf = buf_priv;

	return &buf->dma_addr;
}

unsigned long syna_dh_bm_get_size(void *buf_priv)
{
	struct vb2_syna_bm_buf *buf = buf_priv;

	return buf->size;
}

dma_addr_t vb2_syna_bm_get_addr(struct vb2_buffer * vb, int plane_no,
				enum syna_bm_addr_t type)
{
	struct vb2_syna_bm_buf *buf = vb->planes[plane_no].mem_priv;
	switch (type) {
	case BM_DMA:
		return buf->dma_addr;
	case BM_MEMID:
		return buf->pt_param.mem_id;
	case BM_PTPHY:
		return buf->pt_param.phy_addr;
	}
	return buf->dma_addr;
}

bool vb2_syna_bm_has_cache_carer(struct vb2_buffer * vb, int plane_no)
{
	struct vb2_syna_bm_buf *buf = vb->planes[plane_no].mem_priv;

	return buf->dma_sgt == NULL ? false : true;
}

static void *vb2_syna_bm_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct vb2_syna_bm_buf *buf = buf_priv;
	struct iosys_map map;
	int ret;

	if (!buf->vaddr) {
		if (buf->db_attach) {
			ret = dma_buf_vmap(buf->db_attach->dmabuf, &map);
			buf->vaddr = ret ? NULL : map.vaddr;
		} else if (buf->cookie) {
			ret = dma_buf_vmap(buf->cookie, &map);
			buf->vaddr = ret ? NULL : map.vaddr;
		}
	}

	return buf->vaddr;
}

static unsigned int vb2_syna_bm_num_users(void *buf_priv)
{
	struct vb2_syna_bm_buf *buf = buf_priv;

	return refcount_read(&buf->refcount);
}

static void vb2_syna_bm_dh_prepare(void *buf_priv)
{
	struct vb2_syna_bm_buf *buf = buf_priv;

	dma_buf_end_cpu_access(buf->cookie, buf->dma_dir);
}

static void vb2_syna_bm_dh_finish(void *buf_priv)
{
	struct vb2_syna_bm_buf *buf = buf_priv;

	dma_buf_begin_cpu_access(buf->cookie, buf->dma_dir);
}

int vb2_syna_fill_mtr_meta(struct vb2_buffer *vb, int plane_no,
			   void *meta, u32 flags)
{
	struct vb2_syna_bm_buf *buf = vb->planes[plane_no].mem_priv;
	struct mtr_meta *mtrr;

	if (!buf->bm_meta) {
		pr_err("no bm meta\n");
		return -EINVAL;
	}

	mtrr = &buf->bm_meta->mtrr;
	mtrr->mtr_flags = flags;
	memcpy(&mtrr->mtrcfg, meta, sizeof(mtrr->mtrcfg));

	return 0;
}

/*********************************************/
/*        callbacks for MMAP buffers         */
/*********************************************/

void vb2_syna_dh_bm_put(void *buf_priv)
{
	struct vb2_syna_bm_buf *buf = buf_priv;

	if (!refcount_dec_and_test(&buf->refcount))
		return;

	if (buf->db_attach) {
		if (buf->dma_sgt) {
			dma_buf_unmap_attachment(buf->db_attach, buf->dma_sgt,
						 DMA_BIDIRECTIONAL);
			buf->dma_sgt = NULL;
		}
		dma_buf_detach(buf->cookie, buf->db_attach);
	}
	/* heap_extra.free_cb() would free its mem_id */
	dma_buf_put(buf->cookie);

	put_device(buf->dev);
	kfree(buf);
}

int syna_vpu_dh_get_reserved_mem(struct device *dev, dma_addr_t * dma_addr,
				 unsigned int *size)
{
	struct syna_vpu_dh_memdev *memdev =
	    container_of(dev, struct syna_vpu_dh_memdev, dev);
	if (!dev)
		return -EINVAL;

	*dma_addr = memdev->reserved_base;
	*size = memdev->reserved_size;

	return 0;
}

/* NOTE: it is a fake allocator, it won't report the correct offset */
static void *vb2_syna_dh_mtr_alloc(struct syna_vpu_dh_memdev *memdev,
				   struct vb2_syna_bm_buf *buf,
				   unsigned long size)
{
	buf->cookie = memdev->reserved_buf;
	get_dma_buf(memdev->reserved_buf);
	buf->dma_addr = memdev->reserved_base;
	buf->pt_param.mem_id = memdev->reserved_base;

	buf->dev = get_device(&memdev->dev);
	buf->size = size;

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = vb2_syna_dh_bm_put;
	buf->handler.arg = buf;

	refcount_set(&buf->refcount, 1);

	return buf;
}

static void *vb2_syna_dh_bm_alloc(struct vb2_buffer *vb, struct device *dev,
				  unsigned long size)
{
	struct syna_vpu_dh_memdev *memdev;
	struct vb2_syna_bm_buf *buf;
	struct bm_fb_param fb_param = { 0, };
	int ret;

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	memdev = container_of(dev, struct syna_vpu_dh_memdev, dev);

	buf->attrs = vb->vb2_queue->dma_attrs;
	buf->cookie = dma_heap_buffer_alloc(memdev->heap, size, 0, 0);
	if (IS_ERR_OR_NULL(buf->cookie)) {
		dev_err(dev, "alloc of size %lu failed: %ld\n", size,
			PTR_ERR(buf->cookie));
		kfree(buf);
		return ERR_PTR(-ENOMEM);
	}

	fb_param.fb_type = SHM_MMU_GENERIC;
	ret = bm_create_pt(buf->cookie, 0, &fb_param, &buf->pt_param);
	if (ret) {
		pr_err("bm refuse to register: %d\n", ret);
		dma_buf_put(buf->cookie);
		kfree(buf);
		return ERR_PTR(-ret);
	}

	buf->bm_meta = bm_fetch_meta(buf->cookie);
	if (!buf->bm_meta) {
		pr_err("bm refuse to fetch meta\n");
	}

	buf->dma_addr = (dma_addr_t) buf->pt_param.mem_id;

	if ((buf->attrs & DMA_ATTR_NO_KERNEL_MAPPING) == 0)
		buf->vaddr = buf->cookie;

	buf->db_attach = dma_buf_attach(buf->cookie, dev);
	if (IS_ERR_OR_NULL(buf->db_attach)) {
		pr_err("attach dmabuf %p fail, err=%ld\n", buf->cookie,
		       PTR_ERR(buf->db_attach));
		dma_buf_put(buf->cookie);
		kfree(buf);
		return ERR_PTR(-PTR_ERR(buf->db_attach));
	}

	buf->dma_sgt =
	    dma_buf_map_attachment(buf->db_attach, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(buf->dma_sgt)) {
		pr_err("map dmabuf %p fail, err=%ld\n", buf->cookie,
		       PTR_ERR(buf->dma_sgt));
		dma_buf_detach(buf->cookie, buf->db_attach);
		dma_buf_put(buf->cookie);
		kfree(buf);
		return ERR_PTR(-PTR_ERR(buf->dma_sgt));
	}

	/* Prevent the device from being released while the buffer is used */
	buf->dev = get_device(dev);
	buf->size = size;
	buf->dma_dir = vb->vb2_queue->dma_dir;

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = vb2_syna_dh_bm_put;
	buf->handler.arg = buf;
	buf->vb = vb;

	refcount_set(&buf->refcount, 1);
	return buf;
}

void *syna_dh_bm_alloc(struct device *dev, unsigned long size)
{
	struct syna_vpu_dh_memdev *memdev;
	struct vb2_syna_bm_buf *buf;
	struct bm_fb_param fb_param = { 0, };
	int ret;

	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	memdev = container_of(dev, struct syna_vpu_dh_memdev, dev);
	if (memdev->is_reserved_mem)
		return vb2_syna_dh_mtr_alloc(memdev, buf, size);

	buf->cookie = dma_heap_buffer_alloc(memdev->heap, size, 0, 0);
	if (IS_ERR_OR_NULL(buf->cookie)) {
		dev_err(dev, "alloc of size %lu failed: %ld\n", size,
			PTR_ERR(buf->cookie));
		kfree(buf);
		return ERR_PTR(-ENOMEM);
	}

	/* bm will attach buffer */
	fb_param.fb_type = SHM_MMU_GENERIC;
	ret = bm_create_pt(buf->cookie, 0, &fb_param, &buf->pt_param);
	if (ret) {
		pr_err("bm refuse to register: %d\n", ret);
		dma_buf_put(buf->cookie);
		kfree(buf);
		return ERR_PTR(-ret);
	}
	buf->dma_addr = (dma_addr_t) buf->pt_param.mem_id;

	buf->dev = get_device(dev);
	buf->size = size;

	refcount_set(&buf->refcount, 1);

	return buf;
}

static int vb2_syna_dh_bm_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct vb2_syna_bm_buf *buf = buf_priv;
	int ret;

	if (!buf) {
		printk(KERN_ERR "No buffer to map\n");
		return -EINVAL;
	}
	vm_flags_clear(vma, VM_PFNMAP);

	ret = dma_buf_mmap(buf->cookie, vma, 0);
	if (ret) {
		pr_err("Remapping memory failed, error: %d\n", ret);
		return ret;
	}
	vm_flags_set(vma, VM_DONTEXPAND | VM_DONTDUMP);

	vma->vm_private_data = &buf->handler;
	vma->vm_ops = &vb2_common_vm_ops;

	vma->vm_ops->open(vma);

	return 0;
}

/*********************************************/
/*         DMABUF ops for exporters          */
/*********************************************/
static int vb2_syna_dh_dmabuf_ops_attach(struct dma_buf *dbuf, struct dma_buf_attachment
					 *dbuf_attach)
{
	struct vb2_syna_bm_buf *buf = dbuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;
	int ret = 0;

	if (dh_dbuf->ops->attach)
		ret = dh_dbuf->ops->attach(dh_dbuf, dbuf_attach);

	return ret;
}

static void vb2_syna_dh_dmabuf_ops_detach(struct dma_buf *dbuf,
					  struct dma_buf_attachment *db_attach)
{
	struct vb2_syna_bm_buf *buf = dbuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;

	if (dh_dbuf->ops->detach)
		dh_dbuf->ops->detach(dh_dbuf, db_attach);

	return;
}

static struct sg_table *vb2_syna_dh_dmabuf_ops_map(struct dma_buf_attachment
						   *db_attach,
						   enum dma_data_direction
						   dma_dir)
{
	struct dma_buf *vb2_dbuf = db_attach->dmabuf;
	struct vb2_syna_bm_buf *buf = vb2_dbuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;
	struct sg_table *sgt = NULL;

	if (dh_dbuf->ops->map_dma_buf) {
		db_attach->dmabuf = dh_dbuf;
		sgt = dh_dbuf->ops->map_dma_buf(db_attach, dma_dir);
		db_attach->dmabuf = vb2_dbuf;
	}

	return sgt;
}

static void vb2_syna_dh_dmabuf_ops_unmap(struct dma_buf_attachment *db_attach,
					 struct sg_table *sgt,
					 enum dma_data_direction dma_dir)
{
	/* nothing to be done here */
}

static int
vb2_syna_dh_dmabuf_ops_begin_cpu_access(struct dma_buf *dbuf,
					enum dma_data_direction direction)
{
	struct vb2_syna_bm_buf *buf = dbuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;

	if (dh_dbuf->ops->begin_cpu_access)
		return dh_dbuf->ops->begin_cpu_access(dh_dbuf, direction);

	return 0;
}

static int
vb2_syna_dh_dmabuf_ops_end_cpu_access(struct dma_buf *dbuf,
				      enum dma_data_direction direction)
{
	struct vb2_syna_bm_buf *buf = dbuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;

	if (dh_dbuf->ops->end_cpu_access)
		return dh_dbuf->ops->end_cpu_access(dh_dbuf, direction);

	return 0;
}

static int vb2_syna_dh_dmabuf_ops_vmap(struct dma_buf *dbuf,
				       struct iosys_map *map)
{
	struct vb2_syna_bm_buf *buf = dbuf->priv;
	int ret = -EINVAL;

	if (!buf->vaddr) {
		if (buf->db_attach) {
			ret = dma_buf_vmap(buf->db_attach->dmabuf, map);
			buf->vaddr = ret ? NULL : map->vaddr;
		} else if (buf->cookie) {
			ret = dma_buf_vmap(buf->cookie, map);
			buf->vaddr = ret ? NULL : map->vaddr;
		}

		return ret;
	}

	iosys_map_set_vaddr(map, buf->vaddr);

	return 0;
}

static void vb2_syna_dh_dmabuf_ops_vunmap(struct dma_buf *dbuf,
					  struct iosys_map *map)
{
	struct vb2_syna_bm_buf *buf = dbuf->priv;

	if (buf->vaddr) {
		if (buf->db_attach)
			dma_buf_vunmap(buf->db_attach->dmabuf, map);
		else if (buf->cookie)
			dma_buf_vunmap(buf->cookie, map);
	}

	return;
}

static int vb2_syna_dh_dmabuf_ops_mmap(struct dma_buf *dbuf,
				       struct vm_area_struct *vma)
{
	return vb2_syna_dh_bm_mmap(dbuf->priv, vma);
}

static void vb2_syna_dh_dmabuf_ops_release(struct dma_buf *dbuf)
{
	vb2_syna_dh_bm_put(dbuf->priv);
}

static int vb2_syna_dh_get_flags(struct dma_buf *dmabuf, unsigned long *flags)
{
	struct vb2_syna_bm_buf *buf = dmabuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;

	if (dh_dbuf->ops->get_flags)
		return dh_dbuf->ops->get_flags(dh_dbuf, flags);

	return 0;
}

static const struct dma_buf_ops vb2_syna_dh_dmabuf_ops = {
	.attach = vb2_syna_dh_dmabuf_ops_attach,
	.detach = vb2_syna_dh_dmabuf_ops_detach,
	.map_dma_buf = vb2_syna_dh_dmabuf_ops_map,
	.unmap_dma_buf = vb2_syna_dh_dmabuf_ops_unmap,
	.begin_cpu_access = vb2_syna_dh_dmabuf_ops_begin_cpu_access,
	.end_cpu_access = vb2_syna_dh_dmabuf_ops_end_cpu_access,
	.vmap = vb2_syna_dh_dmabuf_ops_vmap,
	.vunmap = vb2_syna_dh_dmabuf_ops_vunmap,
	.mmap = vb2_syna_dh_dmabuf_ops_mmap,
	.release = vb2_syna_dh_dmabuf_ops_release,
	.get_flags = vb2_syna_dh_get_flags,
};

static struct dma_buf *vb2_syna_dh_get_dmabuf(struct vb2_buffer *vb,
					      void *buf_priv,
					      unsigned long flags)
{
	struct vb2_syna_bm_buf *buf = buf_priv;
	struct dma_buf *dbuf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	exp_info.ops = &vb2_syna_dh_dmabuf_ops;
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

/*********************************************/
/*       callbacks for DMABUF buffers        */
/*********************************************/

static int vb2_syna_bm_map_dmabuf(void *mem_priv)
{
	struct vb2_syna_bm_buf *buf = mem_priv;
	struct sg_table *sgt;

	if (WARN_ON(buf->dma_addr)) {
		pr_err("dmabuf buffer is already pinned\n");
		return 0;
	}

	if (buf->pt_param.mem_id) {
		buf->dma_addr = (dma_addr_t) buf->pt_param.mem_id;
	} else {
		if (WARN_ON(!buf->db_attach)) {
			pr_err("trying to pin a non attached buffer\n");
			return -EINVAL;
		}
		sgt = dma_buf_map_attachment(buf->db_attach, buf->dma_dir);
		if (IS_ERR(sgt)) {
			pr_err("Error getting dmabuf scatterlist\n");
			return -EINVAL;
		}
		buf->dma_addr = sg_dma_address(sgt->sgl);
		buf->dma_sgt = sgt;
	}

	buf->vaddr = NULL;
	return 0;
}

static void vb2_syna_bm_unmap_dmabuf(void *mem_priv)
{
	struct vb2_syna_bm_buf *buf = mem_priv;
	if (WARN_ON(!buf->db_attach)) {
		pr_err("trying to unpin a not attached buffer\n");
		return;
	}

	if (buf->vaddr) {
		dma_buf_vunmap(buf->db_attach->dmabuf, buf->vaddr);
		buf->vaddr = NULL;
	}

	if (buf->dma_sgt) {
		dma_buf_unmap_attachment(buf->db_attach, buf->dma_sgt,
					 buf->dma_dir);
		buf->dma_sgt = NULL;
	}
	buf->dma_addr = 0;
	buf->pt_param.mem_id = 0;
}

static void vb2_syna_bm_detach_dmabuf(void *mem_priv)
{
	struct vb2_syna_bm_buf *buf = mem_priv;

	/* if vb2 works correctly you should never detach mapped buffer */
	if (WARN_ON(buf->dma_addr))
		vb2_syna_bm_unmap_dmabuf(buf);

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

static void *vb2_syna_bm_attach_dmabuf(struct vb2_buffer *vb,
				       struct device *dev, struct dma_buf *dbuf,
				       unsigned long size)
{
	struct vb2_syna_bm_buf *buf;
	struct dma_buf_attachment *dba;

	if (dbuf->size < size)
		return ERR_PTR(-EFAULT);

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->cookie = dbuf;
	if (bm_fetch_pt(dbuf, &buf->pt_param)) {
		pr_info("can't find this dmabuf in bm module, not cust-heap\n");
		kfree(buf);
		return ERR_PTR(-EINVAL);
	}

	if (V4L2_TYPE_IS_CAPTURE(vb->type)) {
		buf->bm_meta = bm_fetch_meta(dbuf);
		if (!buf->bm_meta) {
			pr_err("bm refuse to fetch meta\n");
		}
	}

	/* TODO: request the permission to access this buffer  */

	buf->dev = dev;
	buf->vb = vb;
	/* FIXME: I don't think I need to do this */
	/* create attachment for the dmabuf with the user device */
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

const struct vb2_mem_ops syna_bm_dh_memops = {
	.alloc = vb2_syna_dh_bm_alloc,
	.put = vb2_syna_dh_bm_put,
	.get_dmabuf = vb2_syna_dh_get_dmabuf,
	.cookie = vb2_syna_bm_cookie,
	.vaddr = vb2_syna_bm_vaddr,
	.prepare = vb2_syna_bm_dh_prepare,
	.finish = vb2_syna_bm_dh_finish,
	.attach_dmabuf = vb2_syna_bm_attach_dmabuf,
	.detach_dmabuf = vb2_syna_bm_detach_dmabuf,
	.map_dmabuf = vb2_syna_bm_map_dmabuf,
	.unmap_dmabuf = vb2_syna_bm_unmap_dmabuf,
	.num_users = vb2_syna_bm_num_users,
	.mmap = vb2_syna_dh_bm_mmap,
};

MODULE_IMPORT_NS(SYNA_BM);
MODULE_IMPORT_NS(DMA_BUF);
