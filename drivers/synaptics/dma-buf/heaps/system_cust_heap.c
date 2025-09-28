// SPDX-License-Identifier: GPL-2.0
/*
 * DMABUF System cust heap exporter
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (C) 2019, 2020 Linaro Ltd.
 *
 * Portions based off of Andrew Davis' SRAM heap:
 * Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
 *	Andrew F. Davis <afd@ti.com>
 * Copyright (C) 2022, Synaptics, Inc.
 */

#include <linux/dma-buf.h>
#include <linux/dma-direct.h>
#include <linux/dma-mapping.h>
#include <linux/dma-heap.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/iommu.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/scatterlist.h>
#include <linux/swiotlb.h>
#include <linux/vmalloc.h>
#include "berlin_meta.h"
#include "heap_extra.h"
#include <uapi/linux/dma-buf.h>
#include "kernel_compatibility.h"

#define CREATE_TRACE_POINTS
#include "dmabuf_heap_trace.h"

struct sys_cust_heap_s {
	struct dma_heap *sys_heap;
	struct heap_extra heap_extra;
};

static struct sys_cust_heap_s sys_cust_heap;
static struct sys_cust_heap_s sys_cust_uncached_heap;

struct system_cust_heap_buffer {
	struct berlin_meta *meta;
	struct sys_cust_heap_s *heap;
	struct list_head attachments;
	struct mutex lock;
	unsigned long len;
	struct sg_table sg_table;
	int vmap_cnt;
	void *vaddr;

	bool uncached;
};

struct dma_heap_attachment {
	struct device *dev;
	struct sg_table *table;
	struct list_head list;
	bool mapped;

	bool uncached;
};

#define LOW_ORDER_GFP (GFP_HIGHUSER | __GFP_ZERO | __GFP_COMP)
#define HIGH_ORDER_GFP  (((GFP_HIGHUSER | __GFP_ZERO | __GFP_NOWARN \
				| __GFP_NORETRY) & ~__GFP_RECLAIM) \
				| __GFP_COMP)

static gfp_t order_flags[] = {HIGH_ORDER_GFP, HIGH_ORDER_GFP, LOW_ORDER_GFP,
	                      LOW_ORDER_GFP, LOW_ORDER_GFP};
/*
 * The selection of the orders used for allocation (1MB, 64K, 4K) is designed
 * to match with the sizes often found in IOMMUs. Using order 4 pages instead
 * of order 0 pages can significantly improve the performance of many IOMMUs
 * by reducing TLB pressure and time spent updating page tables.
 */
static const unsigned int orders[] = {4, 3, 2, 1, 0};
#define NUM_ORDERS ARRAY_SIZE(orders)

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0))
static bool needs_swiotlb_bounce(struct device *dev, struct sg_table *table)
{
	struct iommu_domain *domain = iommu_get_domain_for_dev(dev);
	struct scatterlist *sg;
	int i;

	for_each_sgtable_dma_sg(table, sg, i) {
		// SG_DMA_SWIOTLB is set only for dma-iommu, not for dma-direct
		if (domain && IS_ENABLED(CONFIG_NEED_SG_DMA_FLAGS)) {
			if (sg_dma_is_swiotlb(table->sgl))
				return true;
		} else {
			phys_addr_t paddr = domain ?
					    iommu_iova_to_phys(domain, sg_dma_address(sg)) :
					    dma_to_phys(dev, sg_dma_address(sg));
			if (swiotlb_find_pool(dev, paddr))
				return true;
		}
	}
	return false;
}
#endif

static struct sg_table *dup_sg_table(struct sg_table *table)
{
	struct sg_table *new_table;
	int ret, i;
	struct scatterlist *sg, *new_sg;

	new_table = kzalloc(sizeof(*new_table), GFP_KERNEL);
	if (!new_table)
		return ERR_PTR(-ENOMEM);

	ret = sg_alloc_table(new_table, table->orig_nents, GFP_KERNEL);
	if (ret) {
		kfree(new_table);
		return ERR_PTR(-ENOMEM);
	}

	new_sg = new_table->sgl;
	for_each_sgtable_sg(table, sg, i) {
		sg_set_page(new_sg, sg_page(sg), sg->length, sg->offset);
		new_sg = sg_next(new_sg);
	}

	return new_table;
}

static int system_cust_heap_attach(struct dma_buf *dmabuf,
			      struct dma_buf_attachment *attachment)
{
	struct system_cust_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a;
	struct sg_table *table;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	table = dup_sg_table(&buffer->sg_table);
	if (IS_ERR(table)) {
		kfree(a);
		return -ENOMEM;
	}

	a->table = table;
	a->dev = attachment->dev;
	INIT_LIST_HEAD(&a->list);
	a->mapped = false;
	a->uncached = buffer->uncached;
	attachment->priv = a;

	mutex_lock(&buffer->lock);
	list_add(&a->list, &buffer->attachments);
	mutex_unlock(&buffer->lock);

	return 0;
}

static void system_cust_heap_detach(struct dma_buf *dmabuf,
			       struct dma_buf_attachment *attachment)
{
	struct system_cust_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a = attachment->priv;

	mutex_lock(&buffer->lock);
	list_del(&a->list);
	mutex_unlock(&buffer->lock);

	sg_free_table(a->table);
	kfree(a->table);
	kfree(a);
}

static struct sg_table *system_cust_heap_map_dma_buf(struct dma_buf_attachment *attachment,
						enum dma_data_direction direction)
{
	struct dma_heap_attachment *a = attachment->priv;
	struct sg_table *table = a->table;
	int attr = attachment->dma_map_attrs;
	int ret;

	if (a->uncached)
		attr |= DMA_ATTR_SKIP_CPU_SYNC;

	ret = dma_map_sgtable(attachment->dev, table, direction, attr);
	if (ret)
		return ERR_PTR(ret);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0))
	if (a->uncached && needs_swiotlb_bounce(attachment->dev, table)) {
		pr_err("Cannot map uncached system heap buffer for %s, as it requires SWIOTLB",
			dev_name(attachment->dev));
		dma_unmap_sgtable(attachment->dev, table, direction, attr);
		return ERR_PTR(-EINVAL);
	}
#endif
	a->mapped = true;
	return table;
}

static void system_cust_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
				      struct sg_table *table,
				      enum dma_data_direction direction)
{
	struct dma_heap_attachment *a = attachment->priv;
	int attr = attachment->dma_map_attrs;

	if (a->uncached)
		attr |= DMA_ATTR_SKIP_CPU_SYNC;
	a->mapped = false;
	dma_unmap_sgtable(attachment->dev, table, direction, attr);
}

static int system_cust_heap_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
						enum dma_data_direction direction)
{
	struct system_cust_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		invalidate_kernel_vmap_range(buffer->vaddr, buffer->len);

	if (!buffer->uncached) {
		list_for_each_entry(a, &buffer->attachments, list) {
			if (!a->mapped)
				continue;
			dma_sync_sgtable_for_cpu(a->dev, a->table, direction);
		}
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static int system_cust_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
					      enum dma_data_direction direction)
{
	struct system_cust_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		flush_kernel_vmap_range(buffer->vaddr, buffer->len);

	if (!buffer->uncached) {
		list_for_each_entry(a, &buffer->attachments, list) {
			if (!a->mapped)
				continue;
			dma_sync_sgtable_for_device(a->dev, a->table, direction);
		}
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static int system_cust_heap_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct system_cust_heap_buffer *buffer = dmabuf->priv;
	struct sg_table *table = &buffer->sg_table;
	unsigned long addr = vma->vm_start;
	struct sg_page_iter piter;
	int ret;

	if (buffer->uncached)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	for_each_sgtable_page(table, &piter, vma->vm_pgoff) {
		struct page *page = sg_page_iter_page(&piter);

		ret = remap_pfn_range(vma, addr, page_to_pfn(page), PAGE_SIZE,
				      vma->vm_page_prot);
		if (ret)
			return ret;
		addr += PAGE_SIZE;
		if (addr >= vma->vm_end)
			return 0;
	}
	return 0;
}

static void *system_cust_heap_do_vmap(struct system_cust_heap_buffer *buffer)
{
	struct sg_table *table = &buffer->sg_table;
	int npages = PAGE_ALIGN(buffer->len) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct sg_page_iter piter;
	pgprot_t pgprot = PAGE_KERNEL;
	void *vaddr;

	if (!pages)
		return ERR_PTR(-ENOMEM);

	if (buffer->uncached)
		pgprot = pgprot_writecombine(PAGE_KERNEL);

	for_each_sgtable_page(table, &piter, 0) {
		WARN_ON(tmp - pages >= npages);
		*tmp++ = sg_page_iter_page(&piter);
	}

	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	vfree(pages);

	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	return vaddr;
}

static int system_cust_heap_vmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct system_cust_heap_buffer *buffer = dmabuf->priv;
	void *vaddr;
	int ret = 0;

	mutex_lock(&buffer->lock);
	if (buffer->vmap_cnt) {
		buffer->vmap_cnt++;
		iosys_map_set_vaddr(map, buffer->vaddr);
		goto out;
	}

	vaddr = system_cust_heap_do_vmap(buffer);
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

static void system_cust_heap_vunmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct system_cust_heap_buffer *buffer = dmabuf->priv;

	mutex_lock(&buffer->lock);
	if (!--buffer->vmap_cnt) {
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}
	mutex_unlock(&buffer->lock);
	iosys_map_clear(map);
}

static void system_cust_heap_buf_free(struct dma_buf *dmabuf)
{
	struct system_cust_heap_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct scatterlist *sg;
	int i;
	u64 start_time;
	bool uncached = false;
	unsigned long buf_size;

	start_time = ktime_get_ns();

	buf_size = buffer->len;
	uncached = buffer->uncached;
	table = &buffer->sg_table;
	for_each_sgtable_sg(table, sg, i) {
		struct page *page = sg_page(sg);

		__free_pages(page, compound_order(page));
	}
	sg_free_table(table);
	kfree(buffer->meta);
	kfree(buffer);
	trace_sys_cust_free(uncached, buf_size/1024, (ktime_get_ns()-start_time)/1000);
}

static int system_cust_heap_get_flags(struct dma_buf *dmabuf, unsigned long *flags)
{
	*flags = DMA_BUF_FLAG_CUST_HEAP;
	return 0;
}

static void system_cust_heap_dma_buf_release(struct dma_buf *dmabuf)
{
	struct system_cust_heap_buffer *buffer = dmabuf->priv;
	int (*free_cb)(struct dma_buf *dmabuf);
	struct dma_heap *heap;

	free_cb = ((struct sys_cust_heap_s *) (buffer->heap))->heap_extra.free_cb;
	heap = ((struct sys_cust_heap_s *) (buffer->heap))->sys_heap;

	if (free_cb) {
		if (!free_cb(dmabuf))
			system_cust_heap_buf_free(dmabuf);
	} else
		system_cust_heap_buf_free(dmabuf);
}

static const struct dma_buf_ops system_cust_heap_buf_ops = {
	.attach = system_cust_heap_attach,
	.detach = system_cust_heap_detach,
	.map_dma_buf = system_cust_heap_map_dma_buf,
	.unmap_dma_buf = system_cust_heap_unmap_dma_buf,
	.begin_cpu_access = system_cust_heap_dma_buf_begin_cpu_access,
	.end_cpu_access = system_cust_heap_dma_buf_end_cpu_access,
	.mmap = system_cust_heap_mmap,
	.vmap = system_cust_heap_vmap,
	.vunmap = system_cust_heap_vunmap,
	.release = system_cust_heap_dma_buf_release,
	.get_flags = system_cust_heap_get_flags,
};

static struct page *alloc_largest_available(unsigned long size,
					    unsigned int max_order)
{
	struct page *page;
	int i;

	for (i = 0; i < NUM_ORDERS; i++) {
		if (size <  (PAGE_SIZE << orders[i]))
			continue;
		if (max_order < orders[i])
			continue;

		page = alloc_pages(order_flags[i], orders[i]);
		if (!page)
			continue;
		return page;
	}
	return NULL;
}

static struct dma_buf *system_cust_heap_do_allocate(struct dma_heap *heap,
					       unsigned long len,
					       FD_FLAGS_TYPE fd_flags,
					       HEAP_FLAGS_TYPE heap_flags,
					       bool uncached)
{
	struct system_cust_heap_buffer *buffer;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	unsigned long size_remaining = len;
	unsigned int max_order = orders[0];
	struct dma_buf *dmabuf;
	struct sg_table *table;
	struct scatterlist *sg;
	struct list_head pages;
	struct page *page, *tmp_page;
	char tmpbuf[64];
	int i, ret = -ENOMEM;
	u64 start_time;
	start_time = ktime_get_ns();

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&buffer->attachments);
	mutex_init(&buffer->lock);
	buffer->heap = dma_heap_get_drvdata(heap);
	buffer->len = len;
	buffer->uncached = uncached;

	INIT_LIST_HEAD(&pages);
	i = 0;
	while (size_remaining > 0) {
		/*
		 * Avoid trying to allocate memory if the process
		 * has been killed by SIGKILL
		 */
		if (fatal_signal_pending(current)) {
			ret = -EINTR;
			goto free_buffer;
		}

		page = alloc_largest_available(size_remaining, max_order);
		if (!page)
			goto free_buffer;

		list_add_tail(&page->lru, &pages);
		size_remaining -= page_size(page);
		max_order = compound_order(page);
		i++;
	}

	table = &buffer->sg_table;
	if (sg_alloc_table(table, i, GFP_KERNEL))
		goto free_buffer;

	sg = table->sgl;
	list_for_each_entry_safe(page, tmp_page, &pages, lru) {
		sg_set_page(sg, page, page_size(page), 0);
		sg = sg_next(sg);
		list_del(&page->lru);
	}
	buffer->meta = kzalloc(sizeof(struct berlin_meta), GFP_KERNEL);
	if (!buffer->meta) {
		ret = -ENOMEM;
		goto free_pages;
	}

	/* create the dmabuf */
	exp_info.exp_name = dma_heap_get_name(heap);
	exp_info.ops = &system_cust_heap_buf_ops;
	exp_info.size = buffer->len;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;
	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		ret = PTR_ERR(dmabuf);
		goto free_meta;
	}

	/*
	 * For uncached buffers, we need to initially flush cpu cache, since
	 * the __GFP_ZERO on the allocation means the zeroing was done by the
	 * cpu and thus it is likely cached. Map (and implicitly flush) and
	 * unmap it now so we don't get corruption later on.
	 */
	if (buffer->uncached) {
		dma_map_sgtable(dma_heap_get_dev(heap), table, DMA_BIDIRECTIONAL, 0);
		dma_unmap_sgtable(dma_heap_get_dev(heap), table, DMA_BIDIRECTIONAL, 0);
	}

	snprintf(tmpbuf, sizeof(tmpbuf), "%d %d %s\n", task_tgid_vnr(current),
				task_pid_vnr(current), current->comm);
	dma_buf_set_name(dmabuf, tmpbuf);
	trace_sys_cust_alloc(uncached, len/1024, (ktime_get_ns() - start_time)/1000);
	return dmabuf;

free_meta:
	kfree(buffer->meta);
free_pages:
	for_each_sgtable_sg(table, sg, i) {
		struct page *p = sg_page(sg);
		__free_pages(p, compound_order(p));
	}
	sg_free_table(table);
free_buffer:
	list_for_each_entry_safe(page, tmp_page, &pages, lru)
		__free_pages(page, compound_order(page));
	kfree(buffer);

	return ERR_PTR(ret);
}

static struct dma_buf *system_cust_heap_allocate(struct dma_heap *heap,
					    unsigned long len,
					    FD_FLAGS_TYPE fd_flags,
					    HEAP_FLAGS_TYPE heap_flags)
{
	return system_cust_heap_do_allocate(heap, len, fd_flags, heap_flags, false);
}

static const struct dma_heap_ops system_cust_heap_ops = {
	.allocate = system_cust_heap_allocate,
};

static struct dma_buf *system_cust_uncached_heap_allocate(struct dma_heap *heap,
						     unsigned long len,
						     FD_FLAGS_TYPE fd_flags,
						     HEAP_FLAGS_TYPE heap_flags)
{
	return system_cust_heap_do_allocate(heap, len, fd_flags, heap_flags, true);
}

/* Dummy function to be used until we can call coerce_mask_and_coherent */
static struct dma_buf *system_cust_uncached_heap_not_initialized(struct dma_heap *heap,
							    unsigned long len,
							    FD_FLAGS_TYPE fd_flags,
							    HEAP_FLAGS_TYPE heap_flags)
{
	return ERR_PTR(-EBUSY);
}

static struct dma_heap_ops system_cust_uncached_heap_ops = {
	/* After system_cust_heap_create is complete, we will swap this */
	.allocate = system_cust_uncached_heap_not_initialized,
};

static int system_cust_heap_create(void)
{
	struct dma_heap_export_info exp_info;

	exp_info.name = "system_cust";
	exp_info.ops = &system_cust_heap_ops;
	exp_info.priv = &sys_cust_heap;

	sys_cust_heap.sys_heap = dma_heap_add(&exp_info);
	if (IS_ERR(sys_cust_heap.sys_heap))
		return PTR_ERR(sys_cust_heap.sys_heap);
	heap_extra_add_heap(&sys_cust_heap.heap_extra);

	exp_info.name = "system-cust-uncached";
	exp_info.ops = &system_cust_uncached_heap_ops;
	exp_info.priv = &sys_cust_uncached_heap;

	sys_cust_uncached_heap.sys_heap = dma_heap_add(&exp_info);
	if (IS_ERR(sys_cust_uncached_heap.sys_heap))
		return PTR_ERR(sys_cust_uncached_heap.sys_heap);
	heap_extra_add_heap(&sys_cust_uncached_heap.heap_extra);

	dma_coerce_mask_and_coherent(dma_heap_get_dev(sys_cust_uncached_heap.sys_heap),
					DMA_BIT_MASK(64));
	mb(); /* make sure we only set allocate after dma_mask is set */
	system_cust_uncached_heap_ops.allocate = system_cust_uncached_heap_allocate;

	return 0;
}
module_init(system_cust_heap_create);
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(DMA_BUF);

