// SPDX-License-Identifier: GPL-2.0
/*
 * DMABUF berlin heap exporter
 * ported from drivers/staging/android/ion/ion_berlin_heap.c
 *
 * Copyright (C) 2022 Synaptics Incorporated
 *
 */
#include <linux/vmalloc.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/dma-heap.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <berlin_meta.h>
#include "heap_extra.h"
#include <uapi/linux/dma-buf.h>
#include "kernel_compatibility.h"

#define BERLIN_ALLOCATE_FAIL	-1
#define BEST_FIT_ATTRIBUTE		BIT(8)
#define MAX_ATTR_BUF_LEN		32

/* Warning: if you want change following MACROs, please make sure you already
 * reviewed and updated following places at same time:
 * 1. ion.h in OSAL: about ION_A_XX definition;
 * 2. berlin_heap.c in kernel: about ION_A_XX definition;
 * 3. mem_region_userdata.h in bootloader: ION_A_XX and memory region userdata
 *                                         definition.
 * 4. mem_region_userdata.h in tee: ION_A_XX and memory region userdata
 *                                  definition.
 */

/* The ion memory pool attribute flag bits */
#define ION_A_FS                0x0001      /* For secure memory */
#define ION_A_NS                0x0002      /* For non-secure memory */
#define ION_A_FC                0x0004      /* For cacheable memory */
#define ION_A_NC                0x0008      /* For non-cacheable memory */
#define ION_A_FD                0x0010      /* For dynamic memory */
#define ION_A_ND                0x0020      /* For static memory */
#define ION_A_CC                0x0100      /* For control (class) memory */
#define ION_A_CV                0x0200      /* For video (class) memory */
#define ION_A_CG                0x0400      /* For graphics (class) memory */
#define ION_A_CO                0x0800      /* For other (class) memory */

struct berlin_heap {
	struct dma_heap *heap;
	struct heap_extra heap_extra;
	struct gen_pool *pool;
	struct device *dev;
	bool uncached;
	bool secure;
};

struct berlin_heap_buffer {
	struct berlin_meta *meta;
	struct berlin_heap *heap;
	struct list_head attachments;
	struct mutex lock;
	unsigned long len;
	struct sg_table *table;
	pgoff_t pagecount;
	int vmap_cnt;
	void *vaddr;
};

struct platform_heap {
	const char *name;
	phys_addr_t base;
	size_t size;
	u32 attribute[2];
};

struct berlin_info {
	int heap_num;
	struct platform_heap *heaps_data;
	struct dma_heap **heaps;
};

struct dma_heap_attachment {
	struct device *dev;
	struct sg_table *table;
	struct list_head list;
	bool mapped;

	bool uncached;
	bool secure;
};

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

static phys_addr_t berlin_allocate(struct berlin_heap *heap,
					unsigned long size)
{
	unsigned long offset = gen_pool_alloc(heap->pool, size);

	if (!offset)
		return BERLIN_ALLOCATE_FAIL;

	return offset;
}

static void berlin_free(struct berlin_heap *heap, phys_addr_t addr,
			    unsigned long size)
{
	if (addr == BERLIN_ALLOCATE_FAIL)
		return;

	gen_pool_free(heap->pool, addr, size);
}

static void berlin_heap_free(struct dma_buf *dmabuf)
{
	struct berlin_heap_buffer *buffer = dmabuf->priv;
	struct berlin_heap *berlin_heap = buffer->heap;

	struct sg_table *table = buffer->table;
	struct page *page = sg_page(table->sgl);
	phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

	berlin_free(berlin_heap, paddr, buffer->len);
	sg_free_table(table);
	kfree(table);
	kfree(buffer->meta);
	kfree(buffer);
}

static int berlin_heap_attach(struct dma_buf *dmabuf,
			   struct dma_buf_attachment *attachment)
{
	struct berlin_heap_buffer *buffer = dmabuf->priv;
	struct berlin_heap *berlin_heap = buffer->heap;
	struct dma_heap_attachment *a;
	struct sg_table *table;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	table = dup_sg_table(buffer->table);
	if (IS_ERR(table)) {
		kfree(a);
		return -ENOMEM;
	}

	a->table = table;
	a->dev = attachment->dev;
	INIT_LIST_HEAD(&a->list);
	a->mapped = false;
	a->uncached = berlin_heap->uncached;
	a->secure = berlin_heap->secure;

	attachment->priv = a;

	mutex_lock(&buffer->lock);
	list_add(&a->list, &buffer->attachments);
	mutex_unlock(&buffer->lock);

	return 0;
}

static void berlin_heap_detach(struct dma_buf *dmabuf,
			    struct dma_buf_attachment *attachment)
{
	struct berlin_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a = attachment->priv;

	mutex_lock(&buffer->lock);
	list_del(&a->list);
	mutex_unlock(&buffer->lock);

	sg_free_table(a->table);
	kfree(a->table);
	kfree(a);
}

static struct sg_table *berlin_heap_map_dma_buf(struct dma_buf_attachment *attachment,
					enum dma_data_direction direction)
{
	struct dma_heap_attachment *a = attachment->priv;
	struct sg_table *table;
	unsigned long attrs = attachment->dma_map_attrs;

	table = a->table;

	if (a->secure)
		attrs |= DMA_ATTR_NO_KERNEL_MAPPING;

	if (a->uncached)
		attrs |= DMA_ATTR_SKIP_CPU_SYNC;

	if (!dma_map_sg_attrs(attachment->dev, table->sgl, table->nents,
			      direction, attrs))
		return ERR_PTR(-ENOMEM);

	a->mapped = true;

	return table;
}

static void berlin_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
				struct sg_table *table,
				enum dma_data_direction direction)
{
	struct dma_heap_attachment *a = attachment->priv;

	if (a->mapped) {
		unsigned long attrs = attachment->dma_map_attrs;

		a->mapped = false;

		if (a->uncached)
			attrs |= DMA_ATTR_SKIP_CPU_SYNC;

		dma_unmap_sg_attrs(attachment->dev, table->sgl,
				   table->nents,
				   direction, attrs);
	}
}

static int berlin_heap_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
						enum dma_data_direction direction)
{
	struct berlin_heap_buffer *buffer = dmabuf->priv;
	struct berlin_heap *heap = buffer->heap;
	struct dma_heap_attachment *a;

	if (heap->secure)
		return -EPERM;

	mutex_lock(&buffer->lock);
	if (heap->uncached)
		goto unlock;

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sg_for_cpu(a->dev, a->table->sgl, a->table->nents,
				    direction);
	}
unlock:
	mutex_unlock(&buffer->lock);
	return 0;
}

static int berlin_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
						enum dma_data_direction direction)
{
	struct berlin_heap_buffer *buffer = dmabuf->priv;
	struct berlin_heap *heap = buffer->heap;
	struct dma_heap_attachment *a;

	if (heap->secure)
		return -EPERM;

	mutex_lock(&buffer->lock);
	if (heap->uncached)
		goto unlock;

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sg_for_device(a->dev, a->table->sgl, a->table->nents,
				       direction);
	}
unlock:
	mutex_unlock(&buffer->lock);
	return 0;
}

static int berlin_heap_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct berlin_heap_buffer *buffer = dmabuf->priv;
	struct berlin_heap *heap = buffer->heap;
	struct sg_table *table = buffer->table;
	unsigned long addr = vma->vm_start;
	struct sg_page_iter piter;
	int ret;

	if (heap->secure) {
		pr_err("<%s>: Process <%s> try to map secure mem.\n",
			__func__, current->comm);
		return -EPERM;
	}

	if (heap->uncached)
		vma->vm_page_prot =
			pgprot_writecombine(vma->vm_page_prot);

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

static void *berlin_heap_do_vmap(struct berlin_heap_buffer *buffer)
{
	struct sg_table *table = buffer->table;
	struct berlin_heap *heap = buffer->heap;
	int npages = PAGE_ALIGN(buffer->len) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct sg_page_iter piter;
	pgprot_t pgprot = PAGE_KERNEL;
	void *vaddr;

	if (!pages)
		return ERR_PTR(-ENOMEM);

	if (heap->uncached)
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

static int berlin_heap_vmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct berlin_heap_buffer *buffer = dmabuf->priv;
	struct berlin_heap *heap = buffer->heap;
	void *vaddr;
	int ret = 0;

	mutex_lock(&buffer->lock);
	if (heap->secure) {
		pr_err("<%s>: try to map secure mem.\n", __func__);
		dump_stack();
		ret = -EPERM;
		goto out;
	}

	if (buffer->vmap_cnt) {
		buffer->vmap_cnt++;
		iosys_map_set_vaddr(map, buffer->vaddr);
		goto out;
	}
	vaddr = berlin_heap_do_vmap(buffer);
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

static void berlin_heap_vunmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct berlin_heap_buffer *buffer = dmabuf->priv;

	mutex_lock(&buffer->lock);
	if (!--buffer->vmap_cnt) {
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}
	mutex_unlock(&buffer->lock);
	iosys_map_clear(map);
}

static void berlin_heap_dma_buf_release(struct dma_buf *dmabuf)
{
	struct berlin_heap_buffer *buffer = dmabuf->priv;
	struct berlin_heap *berlin_heap;
	int (*free_cb)(struct dma_buf *dmabuf);

	berlin_heap = buffer->heap;
	free_cb = berlin_heap->heap_extra.free_cb;
	if (free_cb) {
		if (!free_cb(dmabuf))
			berlin_heap_free(dmabuf);
	} else
		berlin_heap_free(dmabuf);
}

static int berlin_heap_get_flags(struct dma_buf *dmabuf, unsigned long *flags)
{
	*flags = DMA_BUF_FLAG_CUST_HEAP;

	return 0;
}

static const struct dma_buf_ops berlin_heap_buf_ops = {
	.attach = berlin_heap_attach,
	.detach = berlin_heap_detach,
	.map_dma_buf = berlin_heap_map_dma_buf,
	.unmap_dma_buf = berlin_heap_unmap_dma_buf,
	.begin_cpu_access = berlin_heap_dma_buf_begin_cpu_access,
	.end_cpu_access = berlin_heap_dma_buf_end_cpu_access,
	.mmap = berlin_heap_mmap,
	.vmap = berlin_heap_vmap,
	.vunmap = berlin_heap_vunmap,
	.release = berlin_heap_dma_buf_release,
	.get_flags = berlin_heap_get_flags,
};

static struct dma_buf *berlin_heap_allocate(struct dma_heap *heap,
				    unsigned long len,
				    FD_FLAGS_TYPE fd_flags,
				    HEAP_FLAGS_TYPE heap_flags)
{
	phys_addr_t paddr;
	struct sg_table *table;
	int ret = -ENOMEM;
	struct berlin_heap *berlin_heap = dma_heap_get_drvdata(heap);
	struct berlin_heap_buffer *buffer;
	struct dma_buf *dmabuf;
	char tmpbuf[64];
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&buffer->attachments);
	mutex_init(&buffer->lock);
	buffer->len = len;

	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		goto err_free_buffer;

	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto err_free_table;

	paddr = berlin_allocate(berlin_heap, len);
	if (paddr == BERLIN_ALLOCATE_FAIL) {
		ret = -ENOMEM;
		goto err_free_sg_table;
	}
	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(paddr)), len, 0);
	buffer->heap = berlin_heap;

	buffer->table = table;

	buffer->meta = kzalloc(sizeof(struct berlin_meta), GFP_KERNEL);
	if (!buffer->meta) {
		ret = -ENOMEM;
		goto err_free_berlin_heap;
	}

	/* create the dmabuf */
	exp_info.exp_name = dma_heap_get_name(heap);
	exp_info.ops = &berlin_heap_buf_ops;
	exp_info.size = buffer->len;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;
	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		ret = PTR_ERR(dmabuf);
		goto err_free_meta;
	}
	snprintf(tmpbuf, sizeof(tmpbuf), "%d %d %s\n", task_tgid_vnr(current),
				task_pid_vnr(current), current->comm);
	dma_buf_set_name(dmabuf, tmpbuf);
	return dmabuf;

err_free_meta:
	kfree(buffer->meta);
err_free_berlin_heap:
	berlin_free(berlin_heap, paddr, len);
err_free_sg_table:
	sg_free_table(table);
err_free_table:
	kfree(table);
err_free_buffer:
	kfree(buffer);

	return ERR_PTR(ret);
}

static struct dma_heap_ops berlin_heap_ops = {
	.allocate = berlin_heap_allocate,
};

static struct berlin_heap *berlin_heap_create(struct platform_heap *heap_data,
					struct device *dev)
{
	struct berlin_heap *berlin_heap;

	berlin_heap = kzalloc(sizeof(*berlin_heap), GFP_KERNEL);
	if (!berlin_heap)
		return ERR_PTR(-ENOMEM);

	berlin_heap->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!berlin_heap->pool) {
		kfree(berlin_heap);
		return ERR_PTR(-ENOMEM);
	}

	if (heap_data->attribute[0] & BEST_FIT_ATTRIBUTE)
		gen_pool_set_algo(berlin_heap->pool, gen_pool_best_fit, NULL);

	berlin_heap->dev = dev;
	gen_pool_add(berlin_heap->pool, heap_data->base, heap_data->size,
		     -1);
	if (heap_data->attribute[1] & ION_A_FS)
		berlin_heap->secure = 1;
	if (heap_data->attribute[1] & ION_A_NC)
		berlin_heap->uncached = 1;

	return berlin_heap;
}

static void berlin_heap_destroy(struct dma_heap *heap)
{
	struct berlin_heap *berlin_heap = dma_heap_get_drvdata(heap);

	heap_extra_rm_heap(&berlin_heap->heap_extra);
	gen_pool_destroy(berlin_heap->pool);
	kfree(berlin_heap);
}

static void *berlin_malloc_info(struct device *dev, int heap_num)
{
	int size = 0;
	unsigned char *p = NULL;
	struct berlin_info *info = NULL;
	int info_len = sizeof(*info);
	int heaps_data_len = sizeof(struct platform_heap);
	int heaps_len = sizeof(struct dma_heap *);

	size =  info_len + heaps_data_len * heap_num
		+ heaps_len * heap_num;
	p = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!p) {
		pr_err("%s fail\n", __func__);
		return p;
	}

	info = (struct berlin_info *)(p);
	info->heap_num = heap_num;
	info->heaps_data = (struct platform_heap *)(p + info_len);
	info->heaps = (struct dma_heap **)(p + info_len +
			heaps_data_len * heap_num);

	return (void *)(p);
}

static int berlin_get_info(struct device *dev,
			       struct berlin_info **info)
{
	int i, res = -ENODEV;
	int heap_num = 0;
	struct device_node *np;
	struct resource r;
	struct berlin_info *tmp_info;
	int attri_num = 0;
	unsigned int attr_buf[MAX_ATTR_BUF_LEN];

	np = dev->of_node;
	if (!np)
		goto err_node;

	res = of_property_read_u32(np, "pool-num", &heap_num);
	if (res)
		goto err_node;

	tmp_info = (struct berlin_info *)berlin_malloc_info(dev, heap_num);
	if (!tmp_info) {
		res = -ENOMEM;
		goto err_node;
	}

	res = of_property_read_u32(np, "attributes-num-per-pool",
				   &attri_num);
	if (res)
		goto err_node;

	if ((heap_num * attri_num) > MAX_ATTR_BUF_LEN) {
		pr_err("%d * %d exceed attribute buffer\n", heap_num, attri_num);
		goto err_node;
	}

	res = of_property_read_u32_array(np, "pool-attributes",
					 attr_buf, heap_num * attri_num);
	if (res) {
		pr_err("get pool-attributes fail\n");
		goto err_node;
	}

	for (i = 0; i < heap_num; i++) {
		res = of_address_to_resource(np, i, &r);
		if (res)
			goto err_node;
		(tmp_info->heaps_data + i)->base = r.start;
		(tmp_info->heaps_data + i)->size = resource_size(&r);
		(tmp_info->heaps_data + i)->name = r.name;
		(tmp_info->heaps_data + i)->attribute[0] = attr_buf[i*attri_num];
		(tmp_info->heaps_data + i)->attribute[1] = attr_buf[i*attri_num + 1];
		pr_info("memory start[%08llx] len[%08llx] name[%s] attri1[%08x] attri2[%08x]\n",
		r.start, resource_size(&r), r.name, attr_buf[i*attri_num], attr_buf[i*attri_num +1]);
	}

	*info = tmp_info;
	return 0;

err_node:
	pr_err("%s exit with error %d\n", __func__, res);
	return res;
}

static int berlin_heap_probe(struct platform_device *pdev)
{
	int res = 0;
	int i = 0;
	struct berlin_info *info;
	struct berlin_heap *berlin_heap;
	struct dma_heap_export_info exp_info;

	pr_info("%s enter\n", __func__);
	res = berlin_get_info(&pdev->dev, &info);
	if (res) {
		pr_err("%s with ret %d\n", __func__, res);
		return res;
	}
	for (i = 0; i < info->heap_num; i++) {
		struct platform_heap *heap_data = (info->heaps_data + i);

		berlin_heap = berlin_heap_create(heap_data, &pdev->dev);
		if (IS_ERR_OR_NULL(berlin_heap)) {
			res = PTR_ERR(berlin_heap);
			goto err_create_heap;
		}
		exp_info.name = heap_data->name;
		exp_info.ops = &berlin_heap_ops;
		exp_info.priv = berlin_heap;

		berlin_heap->heap = dma_heap_add(&exp_info);
		if (IS_ERR(berlin_heap->heap))
			return PTR_ERR(berlin_heap->heap);
		pr_info("%s with heap name %s\n", __func__, heap_data->name);
		info->heaps[i] = berlin_heap->heap;
		heap_extra_add_heap(&berlin_heap->heap_extra);
	}
	platform_set_drvdata(pdev, info);
	dev_info(&pdev->dev, "berlin_probe %d dma buf heaps done\n", info->heap_num);
	return 0;

err_create_heap:
	for (i = 0; i < info->heap_num; i++) {
		if (info->heaps[i])
			berlin_heap_destroy(info->heaps[i]);
	}

	return res;
}

static RET_TYPE berlin_heap_remove(struct platform_device *pdev)
{
	int i = 0;
	struct berlin_info *info;

	info = (struct berlin_info *)dev_get_drvdata(&pdev->dev);

	for (i = 0; i < info->heap_num; i++) {
		if (info->heaps[i])
			berlin_heap_destroy(info->heaps[i]);
	}
	RETURN_VALUE;
}

static const struct of_device_id berlin_heaps_of_match[] = {
	{ .compatible = "syna,berlin-heaps", },
	{},
};

static struct platform_driver berlin_heap_driver = {
	.probe = berlin_heap_probe,
	.remove = berlin_heap_remove,
	.driver = {
		.name	= "berlin-dmaheaps",
		.of_match_table = berlin_heaps_of_match,
	},
};

static int __init berlin_heap_init(void)
{
	return platform_driver_register(&berlin_heap_driver);
}
device_initcall(berlin_heap_init);
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(DMA_BUF);

