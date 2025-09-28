// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifdef CONFIG_DMABUF_HEAPS_EXTRA

#include <linux/module.h>
#include <linux/dma-buf.h>
#include <linux/refcount.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/dma-heap.h>
#include "uapi/bm.h"

#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-memops.h>

#include "hrx-dmaheap.h"
#include "kernel_compatibility.h"


DEFINE_MUTEX(hrx_dh_mutex);
static bool dh_memdev_init;
static struct list_head dh_memdevs;

struct syna_hrx_dh_memdev {
	struct device dev;
	struct dma_heap *heap;
	struct list_head link;
	memory_type_t mem_type;
};

struct syna_hrx_dhub_buf {
	struct device		*dev;
	void				*vaddr;   //Virtual address of kernel space only frmae
	void				*paddr;   //Physical address frame
	void				*paddr_pt;   //Physical address page table
	unsigned long		size;
	void				*cookie;
	dma_addr_t			dma_addr;
	unsigned long		attrs;
	enum				dma_data_direction dma_dir;
	struct sg_table		*dma_sgt;

	/* MMAP related */
	struct vb2_vmarea_handler	handler;
	refcount_t			refcount;

	/* DMABUF related */
	struct dma_buf_attachment	*db_attach;
	struct iosys_map		*map;

	memory_type_t			mem_type;
	struct bm_pt_param		pt_param;
	struct vb2_buffer		*vb;
};

static struct syna_hrx_dh_memdev *alloc_memdev(const char *heap_name, memory_type_t mem_type)
{
	struct syna_hrx_dh_memdev *memdev;
	struct dma_heap *heap;
	int ret;

	memdev = kzalloc(sizeof(struct syna_hrx_dh_memdev), GFP_KERNEL);
	if (!memdev) {
		ret = -ENOMEM;
		return NULL;
	}

	device_initialize(&memdev->dev);
	dev_set_name(&memdev->dev, "syna_hrx_dma:%s", heap_name);

	ret = dma_coerce_mask_and_coherent(&memdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(&memdev->dev, "set dma mask 32b failed: %d\n", ret);
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

	list_add(&memdev->link, &dh_memdevs);
	return memdev;
}

int syna_hrx_dh_memdev_alloc(struct device **pdev)
{
	struct syna_hrx_dh_memdev *memdev;
	const char *cma_heap_name = "CMA-CUST-reserved";
	int ret = 0;

	ret = mutex_lock_interruptible(&hrx_dh_mutex);
	if (ret)
		return ret;

	if (dh_memdev_init) {
		mutex_unlock(&hrx_dh_mutex);
		return 0;
	}

	INIT_LIST_HEAD(&dh_memdevs);

	memdev = alloc_memdev(cma_heap_name, SHM_NONSECURE_CONTIG);
	if (!memdev) {
		ret = -ENOMEM;
		goto failed;
	}

	*pdev = &memdev->dev;

	dh_memdev_init = true;
failed:
	mutex_unlock(&hrx_dh_mutex);
	return ret;
}

void syna_hrx_dh_memdev_release(void)
{
	struct syna_hrx_dh_memdev *memdev;
	struct list_head *cur, *next;
	int ret;

	ret = mutex_lock_interruptible(&hrx_dh_mutex);
	if (ret) {
		dev_err(&memdev->dev, "can't lock hrx dma-heap mutex\n");
		return;
	}

	if (!dh_memdev_init) {
		mutex_unlock(&hrx_dh_mutex);
		return;
	}

	list_for_each_safe(cur, next, &dh_memdevs) {
		memdev = list_entry(cur, struct syna_hrx_dh_memdev, link);
		dma_heap_put(memdev->heap);
		device_del(&memdev->dev);
		list_del(&memdev->link);
		kfree(memdev);
	}

	dh_memdev_init = false;
	mutex_unlock(&hrx_dh_mutex);
}

/*********************************************/
/*         callbacks for all buffers         */
/*********************************************/

static void *vb2_syna_hrx_dh_cookie(struct vb2_buffer *vb, void *buf_priv)
{
	struct syna_hrx_dhub_buf *buf = buf_priv;
	return buf;
}

static void *vb2_syna_hrx_dh_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct syna_hrx_dhub_buf *buf = buf_priv;

	/* if (!buf->vaddr)
	 * if buffer is created via alloc, then vaddr is already set
	 */
	return buf->vaddr;
}

static unsigned int vb2_syna_hrx_dh_num_users(void *buf_priv)
{
	struct syna_hrx_dhub_buf *buf = buf_priv;

	return refcount_read(&buf->refcount);
}

static void vb2_syna_hrx_dh_prepare(void *buf_priv)
{
	struct syna_hrx_dhub_buf *buf = buf_priv;

	dma_buf_end_cpu_access(buf->cookie, buf->dma_dir);
}

static void vb2_syna_hrx_dh_finish(void *buf_priv)
{
	struct syna_hrx_dhub_buf *buf = buf_priv;

	dma_buf_begin_cpu_access(buf->cookie, buf->dma_dir);
}

static int vb2_syna_hrx_dh_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct syna_hrx_dhub_buf *buf = buf_priv;
	int ret;

	if (!buf) {
		pr_err("%s: No buffer to map\n", __func__);
		return -EINVAL;
	}

	vm_flags_clear(vma, VM_PFNMAP);

	ret = dma_buf_mmap(buf->cookie, vma, 0);
	if (ret) {
		pr_err("%s: Remapping memory failed, error: %d\n", __func__, ret);
		return ret;
	}
	vm_flags_set(vma, VM_DONTEXPAND | VM_DONTDUMP);
	vma->vm_private_data = &buf->handler;
	vma->vm_ops = &vb2_common_vm_ops;

	vma->vm_ops->open(vma);

	return 0;
}

/*********************************************/
/*        callbacks for MMAP buffers         */
/*********************************************/

static void syna_hrx_dh_free(void *buf_priv)
{
	struct syna_hrx_dhub_buf *buf = buf_priv;

	if (!refcount_dec_and_test(&buf->refcount))
		return;

	if (buf->mem_type == SHM_NONSECURE_CONTIG) {
		/* The map is already mapped which needs to be removed */
		if (buf->vaddr)
			dma_buf_vunmap(buf->cookie, buf->map);

		if (buf->db_attach) {
			dma_buf_unmap_attachment(buf->db_attach, buf->dma_sgt, DMA_BIDIRECTIONAL);
			dma_buf_detach(buf->cookie, buf->db_attach);
			buf->db_attach = NULL;
		}
	}

	if (buf->cookie)
		dma_heap_buffer_free(buf->cookie);

	/* heap_extra.free_cb() would free its mem_id */
	dma_buf_put(buf->cookie);

	put_device(buf->dev);
	kfree(buf->map);
	kfree(buf);
}

static void vb2_syna_hrx_dh_put(void *buf_priv)
{
	syna_hrx_dh_free(buf_priv);
}

static void *get_hrx_dh_alloc(struct syna_hrx_dh_memdev *memdev,
		unsigned long size)
{
	struct syna_hrx_dhub_buf *buf;
	struct device *dev;
	int ret;

	buf = kzalloc(sizeof(struct syna_hrx_dhub_buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->map = kzalloc(sizeof(struct iosys_map), GFP_KERNEL);
	if (IS_ERR_OR_NULL(buf->map)) {
		dev_err(&memdev->dev, "%s: kzalloc failed\n", __func__);
		ret = -ENOMEM;
		goto failed_alloc1;
	}

	dev = &memdev->dev;
	buf->cookie = dma_heap_buffer_alloc(memdev->heap, size, 0, 0);

	if (IS_ERR_OR_NULL(buf->cookie)) {
		dev_err(&memdev->dev, "%s: alloc of size %lu failed: %ld\n", __func__,
				size, PTR_ERR(buf->cookie));
		ret = -ENOMEM;
		goto failed_alloc2;
	}

	dev_dbg(dev, "The name of DMA buf %s\n", ((struct dma_buf *)(buf->cookie))->exp_name);
	buf->mem_type = memdev->mem_type;

	if (memdev->mem_type == SHM_NONSECURE_CONTIG) {

		buf->db_attach = dma_buf_attach(buf->cookie, dev);
		if (IS_ERR_OR_NULL(buf->db_attach)) {
			dev_err(&memdev->dev, "%s:dma attach failed\n",  __func__);
			ret = -ENOMEM;
			goto failed_attach;
		}

		buf->dma_sgt = dma_buf_map_attachment(buf->db_attach, DMA_BIDIRECTIONAL);
		if (IS_ERR_OR_NULL(buf->dma_sgt)) {
			dev_err(&memdev->dev, "%s: dma map attachment failed for sgt\n", __func__);
			ret = -ENOMEM;
			goto failed_attachment;
		}

		if (dma_buf_begin_cpu_access(buf->cookie, DMA_BIDIRECTIONAL) != 0) {
			dev_err(&memdev->dev, "%s: dma_buf_begin_cpu_access failed\n", __func__);
			ret = -ENOMEM;
			goto failed_cpu_access;
		}

		ret = dma_buf_vmap(buf->db_attach->dmabuf, buf->map);
		buf->dma_addr = (dma_addr_t) buf->map->vaddr;
		if (buf->dma_addr == (dma_addr_t) NULL) {
			dev_err(&memdev->dev, "%s: dma_buf_vmap failed\n", __func__);
			ret = -ENOMEM;
			goto failed_vmap;
		}

		dev_dbg(&memdev->dev, "The new mapped Kaddr %llx and cookie %lx\n",
					buf->dma_addr, (unsigned long)buf->cookie);

		if ((buf->attrs & DMA_ATTR_NO_KERNEL_MAPPING) == 0)
			buf->vaddr = (void *) buf->dma_addr;

		//Get the physical address of the memory
		buf->paddr = (void *)sg_dma_address(buf->dma_sgt->sgl);

	}

	/* Prevent the device from being released while the buffer is used */
	buf->dev = get_device(dev);
	buf->size = size;

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = vb2_syna_hrx_dh_put;
	buf->handler.arg = buf;

	dev_dbg(&memdev->dev, "%s :The vaddr %lx paddr %lx pt_addr %lx size %lx\n",
			__func__, (unsigned long)buf->vaddr, (unsigned long)buf->paddr,
			(unsigned long)buf->paddr_pt, (unsigned long) buf->pt_param.len);

	refcount_set(&buf->refcount, 1);

	return buf;
failed_vmap:
	dma_buf_vunmap(buf->cookie, buf->map);
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

static void *vb2_syna_hrx_dh_alloc(struct vb2_buffer *vb, struct device *dev,
		unsigned long size)
{
	struct syna_hrx_dhub_buf *buf;
	struct syna_hrx_dh_memdev *memdev;
	int ret;

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	memdev = container_of(dev, struct syna_hrx_dh_memdev, dev);

	buf  =  get_hrx_dh_alloc(memdev, size);
	if (IS_ERR_OR_NULL(buf)) {
		ret = -ENOMEM;
		return ERR_PTR(ret);
	}

	buf->attrs = vb->vb2_queue->dma_attrs;
	if ((buf->attrs & DMA_ATTR_NO_KERNEL_MAPPING) == 0)
		buf->vaddr = (void *) buf->dma_addr;

	/* Prevent the device from being released while the buffer is used */
	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->vb = vb;

	refcount_set(&buf->refcount, 1);

	return buf;
}

/*********************************************/
/*         DMABUF ops for exporters          */
/*********************************************/
static int vb2_syna_dh_dmabuf_ops_attach(struct dma_buf *dbuf, struct dma_buf_attachment
		*dbuf_attach)
{
	struct syna_hrx_dhub_buf *buf = dbuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;
	int ret = 0;

	if (dh_dbuf->ops->attach)
		ret = dh_dbuf->ops->attach(dh_dbuf, dbuf_attach);

	return ret;
}

static void vb2_syna_dh_dmabuf_ops_detach(struct dma_buf *dbuf,
		struct dma_buf_attachment *db_attach)
{
	struct syna_hrx_dhub_buf *buf = dbuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;

	if (dh_dbuf->ops->detach)
		dh_dbuf->ops->detach(dh_dbuf, db_attach);
}

static struct sg_table *vb2_syna_dh_dmabuf_ops_map(struct dma_buf_attachment
		*db_attach,
		enum dma_data_direction
		dma_dir)
{
	struct dma_buf *vb2_dbuf = db_attach->dmabuf;
	struct syna_hrx_dhub_buf *buf = vb2_dbuf->priv;
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
	struct syna_hrx_dhub_buf *buf = dbuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;

	if (dh_dbuf->ops->begin_cpu_access)
		return dh_dbuf->ops->begin_cpu_access(dh_dbuf, direction);

	return 0;
}

	static int
vb2_syna_dh_dmabuf_ops_end_cpu_access(struct dma_buf *dbuf,
		enum dma_data_direction direction)
{
	struct syna_hrx_dhub_buf *buf = dbuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;

	if (dh_dbuf->ops->end_cpu_access)
		return dh_dbuf->ops->end_cpu_access(dh_dbuf, direction);

	return 0;
}

static int vb2_syna_dh_dmabuf_ops_vmap(struct dma_buf *dbuf,
		struct iosys_map *map)
{
	struct syna_hrx_dhub_buf *buf = dbuf->priv;
	int ret = 0;

	if (!buf->vaddr) {
		buf->db_attach = dma_buf_attach(buf->cookie, buf->dev);
		if (IS_ERR_OR_NULL(buf->db_attach)) {
			pr_err("%s:dma attach failed\n",  __func__);
			ret = -ENOMEM;
			goto failed_attach;
		}

		ret = dma_buf_vmap(buf->db_attach->dmabuf, buf->map);

		buf->dma_addr = (dma_addr_t) buf->map->vaddr;
		if (buf->dma_addr == (dma_addr_t) NULL) {
			pr_err("%s: dma_buf_vmap failed\n", __func__);
			ret = -ENOMEM;
			goto failed_vmap;
		}
		memcpy(map, buf->map, sizeof(struct iosys_map));
	} else {
		iosys_map_set_vaddr(map, buf->vaddr);
	}

	return 0;
failed_vmap:
	dma_buf_vunmap(buf->cookie, buf->map);
failed_attach:
	return ret;
}

static void vb2_syna_dh_dmabuf_ops_vunmap(struct dma_buf *dbuf,
		struct iosys_map *map)
{
	struct syna_hrx_dhub_buf *buf = dbuf->priv;

	if (!buf->vaddr &&  buf->db_attach) {
		dma_buf_vunmap(buf->db_attach->dmabuf, map);
		dma_buf_detach(buf->cookie, buf->db_attach);
		buf->db_attach = NULL;
	}
}

static int vb2_syna_dh_dmabuf_ops_mmap(struct dma_buf *dbuf,
		struct vm_area_struct *vma)
{
	return vb2_syna_hrx_dh_mmap(dbuf->priv, vma);
}

static void vb2_syna_dh_dmabuf_ops_release(struct dma_buf *dbuf)
{
	vb2_syna_hrx_dh_put(dbuf->priv);
}

static int vb2_syna_dh_get_flags(struct dma_buf *dmabuf, unsigned long *flags)
{
	struct syna_hrx_dhub_buf *buf = dmabuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;

	if (dh_dbuf->ops->get_flags)
		return dh_dbuf->ops->get_flags(dh_dbuf, flags);

	return 0;
}
/*
static void *vb2_syna_dh_get_meta(struct dma_buf *dbuf)
{
	struct syna_hrx_dhub_buf *buf = dbuf->priv;
	struct dma_buf *dh_dbuf = buf->cookie;

	if (dh_dbuf->ops->get_meta)
		return dh_dbuf->ops->get_meta(dh_dbuf);

	return 0;
}
*/
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
	/*.get_meta = vb2_syna_dh_get_meta,*/
};


/*********************************************/
/*       callbacks for DMABUF buffers        */
/*********************************************/

static struct dma_buf *vb2_syna_hrx_dh_get_dmabuf(struct vb2_buffer *vb,
		void *buf_priv,
		unsigned long flags)
{
	struct syna_hrx_dhub_buf *buf = buf_priv;
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

static int vb2_syna_hrx_dh_map_dmabuf(void *mem_priv)
{
	struct syna_hrx_dhub_buf *buf = mem_priv;
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

static void vb2_syna_hrx_dh_unmap_dmabuf(void *mem_priv)
{
	struct syna_hrx_dhub_buf *buf = mem_priv;

	if (WARN_ON(!buf->db_attach)) {
		pr_err("%s: trying to unpin a not attached buffer\n", __func__);
		return;
	}

	dma_buf_unmap_attachment(buf->db_attach, buf->dma_sgt, DMA_BIDIRECTIONAL);
	buf->paddr = 0;
}

static void vb2_syna_hrx_dh_detach_dmabuf(void *mem_priv)
{
	struct syna_hrx_dhub_buf *buf = mem_priv;

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

static void *vb2_syna_hrx_dh_attach_dmabuf(struct vb2_buffer *vb,
		struct device *dev, struct dma_buf *dbuf,
		unsigned long size)
{
	struct syna_hrx_dhub_buf *buf;
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
		dev_err(dev, "bm refuse to register: %d\n", ret);
		kfree(buf);
		return ERR_PTR(-EINVAL);
	}

	buf->paddr_pt = (void *)buf->pt_param.phy_addr;
	buf->dev = dev;
	buf->vb = vb;
	buf->cookie = dbuf;
	dba = dma_buf_attach(dbuf, buf->dev);
	if (IS_ERR(dba)) {
		dev_err(dev, "failed to attach dmabuf\n");
		kfree(buf);
		return dba;
	}

	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->size = size;
	buf->db_attach = dba;

	return buf;
}

/*********************************************/
/*       hrx DMA CONTIG exported functions       */
/*********************************************/

const struct vb2_mem_ops syna_hrx_dma_contig_memops = {
	.alloc		= vb2_syna_hrx_dh_alloc,
	.put		= vb2_syna_hrx_dh_put,
	.mmap		= vb2_syna_hrx_dh_mmap,
	.cookie		= vb2_syna_hrx_dh_cookie,
	.vaddr		= vb2_syna_hrx_dh_vaddr,
	.prepare	= vb2_syna_hrx_dh_prepare,
	.finish		= vb2_syna_hrx_dh_finish,
	.num_users	= vb2_syna_hrx_dh_num_users,
	.get_dmabuf	= vb2_syna_hrx_dh_get_dmabuf,
	.map_dmabuf	= vb2_syna_hrx_dh_map_dmabuf,
	.unmap_dmabuf	= vb2_syna_hrx_dh_unmap_dmabuf,
	.attach_dmabuf	= vb2_syna_hrx_dh_attach_dmabuf,
	.detach_dmabuf	= vb2_syna_hrx_dh_detach_dmabuf,
};
EXPORT_SYMBOL_GPL(syna_hrx_dma_contig_memops);


void *syna_hrx_get_frame_phyaddr(void *handle)
{
	struct syna_hrx_dhub_buf *buf;

	buf =  (struct syna_hrx_dhub_buf *) handle;
	if (!buf)
		return NULL;

	return buf->paddr;
}
EXPORT_SYMBOL_GPL(syna_hrx_get_frame_phyaddr);

void *syna_hrx_get_pt_phyaddr(void *handle)
{
	struct syna_hrx_dhub_buf *buf;

	buf =  (struct syna_hrx_dhub_buf *) handle;
	if (!buf)
		return NULL;

	return buf->paddr_pt;
}
EXPORT_SYMBOL_GPL(syna_hrx_get_pt_phyaddr);

MODULE_IMPORT_NS(SYNA_BM);
MODULE_IMPORT_NS(DMA_BUF);
#endif
