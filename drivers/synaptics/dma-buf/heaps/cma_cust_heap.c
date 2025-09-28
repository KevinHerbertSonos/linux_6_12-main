// SPDX-License-Identifier: GPL-2.0
/*
 * DMABUF CMA cust heap exporter
 *
 * Copyright (C) 2012, 2019, 2020 Linaro Ltd.
 * Author: <benjamin.gaignard@linaro.org> for ST-Ericsson.
 *
 * Also utilizing parts of Andrew Davis' SRAM heap:
 * Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
 *	Andrew F. Davis <afd@ti.com>
 * Copyright (C) 2022 Synaptics, Inc
 */
#include <linux/cma.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/dma-map-ops.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "berlin_meta.h"
#include "heap_extra.h"
#include <uapi/linux/dma-buf.h>
#include "kernel_compatibility.h"

struct cma_heap {
	struct dma_heap *heap;
	struct cma *cma;
	struct heap_extra heap_extra;
};

struct cma_cust_heap_buffer {
	struct berlin_meta *meta;
	struct cma_heap *heap;
	struct list_head attachments;
	struct mutex lock;
	unsigned long len;
	struct page *cma_pages;
	struct page **pages;
	pgoff_t pagecount;
	int vmap_cnt;
	void *vaddr;
};

struct dma_heap_attachment {
	struct device *dev;
	struct sg_table table;
	struct list_head list;
	bool mapped;
};

static int cma_cust_heap_attach(struct dma_buf *dmabuf,
			   struct dma_buf_attachment *attachment)
{
	struct cma_cust_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a;
	int ret;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	ret = sg_alloc_table_from_pages(&a->table, buffer->pages,
					buffer->pagecount, 0,
					buffer->pagecount << PAGE_SHIFT,
					GFP_KERNEL);
	if (ret) {
		kfree(a);
		return ret;
	}

	a->dev = attachment->dev;
	INIT_LIST_HEAD(&a->list);
	a->mapped = false;

	attachment->priv = a;

	mutex_lock(&buffer->lock);
	list_add(&a->list, &buffer->attachments);
	mutex_unlock(&buffer->lock);

	return 0;
}

static void cma_cust_heap_detach(struct dma_buf *dmabuf,
			    struct dma_buf_attachment *attachment)
{
	struct cma_cust_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a = attachment->priv;

	mutex_lock(&buffer->lock);
	list_del(&a->list);
	mutex_unlock(&buffer->lock);

	sg_free_table(&a->table);
	kfree(a);
}

static struct sg_table *cma_cust_heap_map_dma_buf(struct dma_buf_attachment *attachment,
					     enum dma_data_direction direction)
{
	struct dma_heap_attachment *a = attachment->priv;
	struct sg_table *table = &a->table;
	int attrs = attachment->dma_map_attrs;
	int ret;

	ret = dma_map_sgtable(attachment->dev, table, direction, attrs);
	if (ret)
		return ERR_PTR(-ENOMEM);
	a->mapped = true;
	return table;
}

static void cma_cust_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
				   struct sg_table *table,
				   enum dma_data_direction direction)
{
	struct dma_heap_attachment *a = attachment->priv;
	int attrs = attachment->dma_map_attrs;

	a->mapped = false;
	dma_unmap_sgtable(attachment->dev, table, direction, attrs);
}

static int cma_cust_heap_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
					     enum dma_data_direction direction)
{
	struct cma_cust_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		invalidate_kernel_vmap_range(buffer->vaddr, buffer->len);

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sgtable_for_cpu(a->dev, &a->table, direction);
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static int cma_cust_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
					   enum dma_data_direction direction)
{
	struct cma_cust_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		flush_kernel_vmap_range(buffer->vaddr, buffer->len);

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sgtable_for_device(a->dev, &a->table, direction);
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static vm_fault_t cma_cust_heap_vm_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct cma_cust_heap_buffer *buffer = vma->vm_private_data;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0))
	if (vmf->pgoff >= buffer->pagecount)
		return VM_FAULT_SIGBUS;

	return vmf_insert_pfn(vma, vmf->address, page_to_pfn(buffer->pages[vmf->pgoff]));
#else
	if (vmf->pgoff > buffer->pagecount)
		return VM_FAULT_SIGBUS;

	vmf->page = buffer->pages[vmf->pgoff];
	get_page(vmf->page);

	return 0;
#endif
}

static const struct vm_operations_struct dma_cust_heap_vm_ops = {
	.fault = cma_cust_heap_vm_fault,
};

static int cma_cust_heap_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct cma_cust_heap_buffer *buffer = dmabuf->priv;
	int ret;

	if ((vma->vm_flags & (VM_SHARED | VM_MAYSHARE)) == 0)
		return -EINVAL;

	ret = remap_pfn_range(vma, vma->vm_start,
			      page_to_pfn(buffer->pages[vma->vm_pgoff]),
			      vma->vm_end - vma->vm_start, vma->vm_page_prot);
	if (ret)
		return ret;

	vma->vm_ops = &dma_cust_heap_vm_ops;
	vma->vm_private_data = buffer;

	return 0;
}

static void *cma_cust_heap_do_vmap(struct cma_cust_heap_buffer *buffer)
{
	void *vaddr;

	vaddr = vmap(buffer->pages, buffer->pagecount, VM_MAP, PAGE_KERNEL);
	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	return vaddr;
}

static int cma_cust_heap_vmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct cma_cust_heap_buffer *buffer = dmabuf->priv;
	void *vaddr;
	int ret = 0;

	mutex_lock(&buffer->lock);
	if (buffer->vmap_cnt) {
		buffer->vmap_cnt++;
		iosys_map_set_vaddr(map, buffer->vaddr);
		goto out;
	}

	vaddr = cma_cust_heap_do_vmap(buffer);
	if (IS_ERR(vaddr)) {
		ret = PTR_ERR(vaddr);
		goto out;
	}

	buffer->vaddr = vaddr;
	buffer->vmap_cnt++;
	iosys_map_set_vaddr(map, buffer->vaddr);
out:
	mutex_unlock(&buffer->lock);

	return ret;
}

static void cma_cust_heap_vunmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct cma_cust_heap_buffer *buffer = dmabuf->priv;

	mutex_lock(&buffer->lock);
	if (!--buffer->vmap_cnt) {
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}
	mutex_unlock(&buffer->lock);
	iosys_map_clear(map);
}

static int cma_cust_heap_get_flags(struct dma_buf *dmabuf, unsigned long *flags)
{
	*flags = DMA_BUF_FLAG_CUST_HEAP;
	return 0;
}

static void cma_cust_heap_free(struct dma_buf *dmabuf)
{
	struct cma_cust_heap_buffer *buffer = dmabuf->priv;
	struct cma_heap *cma_heap = buffer->heap;

	/* free page list */
	kfree(buffer->pages);
	/* release memory */
	cma_release(cma_heap->cma, buffer->cma_pages, buffer->pagecount);
	kfree(buffer->meta);
	kfree(buffer);
	return;
}

static void cma_cust_heap_dma_buf_release(struct dma_buf *dmabuf)
{
	struct cma_cust_heap_buffer *buffer = dmabuf->priv;
	struct cma_heap *cma_heap = buffer->heap;
	int (*free_cb)(struct dma_buf *dmabuf);

	if (buffer->vmap_cnt > 0) {
		WARN(1, "%s: buffer still mapped in the kernel\n", __func__);
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}
	free_cb = cma_heap->heap_extra.free_cb;
	if (free_cb) {
		if (!free_cb(dmabuf))
			cma_cust_heap_free(dmabuf);
	} else
		cma_cust_heap_free(dmabuf);
}

static const struct dma_buf_ops cma_cust_heap_buf_ops = {
	.attach = cma_cust_heap_attach,
	.detach = cma_cust_heap_detach,
	.map_dma_buf = cma_cust_heap_map_dma_buf,
	.unmap_dma_buf = cma_cust_heap_unmap_dma_buf,
	.begin_cpu_access = cma_cust_heap_dma_buf_begin_cpu_access,
	.end_cpu_access = cma_cust_heap_dma_buf_end_cpu_access,
	.mmap = cma_cust_heap_mmap,
	.vmap = cma_cust_heap_vmap,
	.vunmap = cma_cust_heap_vunmap,
	.release = cma_cust_heap_dma_buf_release,
	.get_flags = cma_cust_heap_get_flags,
};

static struct dma_buf *cma_cust_heap_allocate(struct dma_heap *heap,
					 unsigned long len,
					 FD_FLAGS_TYPE fd_flags,
					 HEAP_FLAGS_TYPE heap_flags)
{
	struct cma_heap *cma_heap = dma_heap_get_drvdata(heap);
	struct cma_cust_heap_buffer *buffer;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	size_t size = PAGE_ALIGN(len);
	pgoff_t pagecount = size >> PAGE_SHIFT;
	unsigned long align = get_order(size);
	struct page *cma_pages;
	struct dma_buf *dmabuf;
	char tmpbuf[64];
	int ret = -ENOMEM;
	pgoff_t pg;

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&buffer->attachments);
	mutex_init(&buffer->lock);
	buffer->len = size;

	if (align > CONFIG_CMA_ALIGNMENT)
		align = CONFIG_CMA_ALIGNMENT;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
	cma_pages = cma_alloc(cma_heap->cma, pagecount, align, false);
#else
	cma_pages = cma_alloc(cma_heap->cma, pagecount, align, GFP_KERNEL);
#endif
	if (!cma_pages)
		goto free_buffer;

	/* Clear the cma pages */
	if (PageHighMem(cma_pages)) {
		unsigned long nr_clear_pages = pagecount;
		struct page *page = cma_pages;

		while (nr_clear_pages > 0) {
			void *vaddr = kmap_atomic(page);

			memset(vaddr, 0, PAGE_SIZE);
			kunmap_atomic(vaddr);
			/*
			 * Avoid wasting time zeroing memory if the process
			 * has been killed by SIGKILL
			 */
			if (fatal_signal_pending(current))
				goto free_cma;
			page++;
			nr_clear_pages--;
		}
	} else {
		memset(page_address(cma_pages), 0, size);
	}

	buffer->pages = kmalloc_array(pagecount, sizeof(*buffer->pages), GFP_KERNEL);
	if (!buffer->pages) {
		ret = -ENOMEM;
		goto free_cma;
	}

	for (pg = 0; pg < pagecount; pg++)
		buffer->pages[pg] = &cma_pages[pg];

	buffer->cma_pages = cma_pages;
	buffer->heap = cma_heap;
	buffer->pagecount = pagecount;

	buffer->meta = kzalloc(sizeof(struct berlin_meta), GFP_KERNEL);
	if (!buffer->meta) {
		ret = -ENOMEM;
		goto free_pages;
	}

	/* create the dmabuf */
	exp_info.exp_name = dma_heap_get_name(heap);
	exp_info.ops = &cma_cust_heap_buf_ops;
	exp_info.size = buffer->len;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;
	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		ret = PTR_ERR(dmabuf);
		goto free_meta;
	}
	snprintf(tmpbuf, sizeof(tmpbuf), "%d %d %s\n", task_tgid_vnr(current),
						task_pid_vnr(current), current->comm);
	dma_buf_set_name(dmabuf, tmpbuf);
	return dmabuf;

free_meta:
	kfree(buffer->meta);
free_pages:
	kfree(buffer->pages);
free_cma:
	cma_release(cma_heap->cma, cma_pages, pagecount);
free_buffer:
	kfree(buffer);

	return ERR_PTR(ret);
}

static const struct dma_heap_ops cma_cust_heap_ops = {
	.allocate = cma_cust_heap_allocate,
};

static int __add_cma_cust_heap(struct cma *cma, void *data)
{
	struct cma_heap *cma_heap;
	struct dma_heap_export_info exp_info;

	cma_heap = kzalloc(sizeof(*cma_heap), GFP_KERNEL);
	if (!cma_heap)
		return -ENOMEM;
	cma_heap->cma = cma;

	exp_info.name = kasprintf(GFP_KERNEL, "CMA-CUST-%s", cma_get_name(cma));
	if (!exp_info.name) {
		kfree(cma_heap);
		return -ENOMEM;
	}
	exp_info.ops = &cma_cust_heap_ops;
	exp_info.priv = cma_heap;

	cma_heap->heap = dma_heap_add(&exp_info);
	if (IS_ERR(cma_heap->heap)) {
		int ret = PTR_ERR(cma_heap->heap);

		kfree(cma_heap);
		return ret;
	}
	heap_extra_add_heap(&cma_heap->heap_extra);

	return 0;
}

static int add_default_cma_cust_heap(void)
{
	struct cma *default_cma = dev_get_cma_area(NULL);
	int ret = 0;

	if (default_cma)
		ret = __add_cma_cust_heap(default_cma, NULL);

	return ret;
}
module_init(add_default_cma_cust_heap);
MODULE_DESCRIPTION("DMA-BUF CMA Cust Heap");
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(DMA_BUF);

