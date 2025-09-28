/*
 * DSPG shared memory coma driver
 *
 * Copyright (c) 2018, DSP Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>
#include <linux/completion.h>
#include <linux/debugfs.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/nospec.h>
#include <linux/coma/coma.h>
#include <linux/coma/coma-sharedmem.h>
#include <linux/uaccess.h>
#include <mach/platform.h>
#include "cmsg-sharedmem.h"

/*
 * maximum number of different memories to share
 * (e.g. DRAM, DTCM, ITCM, AHB-RAM, BMP-RAM)
 */
#define MAX_MEMDEVS       5
#define INIT_TIMEOUT      500 /* ms */
#define COMA_SHAREDMEM    "coma-sharedmem"

struct memory_device {
	struct device *dev;
	const char *name;
	int valid;
	dma_addr_t base;
	u32 mask;
	u32 size;
	u32 offset;
	unsigned long va_offset;
	void *reserved_va;
	unsigned long vma;
#ifdef CONFIG_DEBUG_FS
	struct debugfs_blob_wrapper mem_blob;
#endif
};

struct sharedmem_coma {
	int initialized;
	int service_id;
	struct cdev cdev;
	struct device *dev;
	struct device *chardev;
	dev_t chrdev;
	struct class *class;
	struct completion done;
	int status;
	int major;
	struct list_head allocations;

	dma_addr_t base;
	u32 size;

	struct memory_device mem_devs[MAX_MEMDEVS];
	struct sharedmem_coma_memory_info meminfo[MAX_MEMDEVS];
#ifdef CONFIG_DEBUG_FS
	struct dentry *pdentry;
#endif
	unsigned long css_va;
	unsigned int nr_memories;
	unsigned int memories;
	unsigned int has_dram;
};
static struct sharedmem_coma *sharedmem_coma;

struct sharedmem_coma_alloc {
	struct list_head node;
	struct file *file;
	size_t size;
	void *addr;
	dma_addr_t pa;
	unsigned int id;
};

static int
sharedmem_coma_create_message(enum cmsg_sharedmem_types type,
			      union cmsg_sharedmem_params *params)
{
	return coma_cmsg_send(sharedmem_coma->service_id, (int)type,
			      (void *)params, sizeof(*params), NULL, 0);
}

#ifdef CONFIG_DEBUG_FS
static void
sharedmem_coma_debugfs_create_files(struct sharedmem_coma *sharedmem_coma)
{
	int i;
	struct dentry *pdentry, *mdentry;

	pdentry = debugfs_create_dir("sharedmem", NULL);
	sharedmem_coma->pdentry = pdentry;

	for (i = 0; i < sharedmem_coma->nr_memories; i++) {
		struct memory_device *memdev = &sharedmem_coma->mem_devs[i];

		if (!memdev->name)
			continue;

		mdentry = debugfs_create_dir(memdev->name, pdentry);

		memdev->mem_blob.data = ioremap(memdev->base, memdev->size);
		memdev->mem_blob.size = memdev->size;

		if (memdev->mem_blob.data)
			debugfs_create_blob("memory", 0400, mdentry,
					    &memdev->mem_blob);

		debugfs_create_x32("vma", 0444, mdentry,
				   (u32 *)&memdev->vma);
		debugfs_create_x32("pa", 0444, mdentry,
				   (u32 *)&memdev->base);
		debugfs_create_x32("pa_offset", 0444, mdentry,
				   (u32 *)&memdev->offset);
		debugfs_create_x32("initialized", 0444, mdentry,
				   (u32 *)&memdev->valid);
	}
}

static void
sharedmem_coma_debugfs_remove_files(struct sharedmem_coma *sharedmem_coma)
{
	int i;

	for (i = 0; i < sharedmem_coma->nr_memories; i++) {
		struct memory_device *memdev = &sharedmem_coma->mem_devs[i];

		if (memdev->mem_blob.data)
			iounmap(memdev->mem_blob.data);
	}

	if (sharedmem_coma->pdentry)
		debugfs_remove_recursive(sharedmem_coma->pdentry);
}
#else
#define sharedmem_coma_debugfs_create_files(p)
#define sharedmem_coma_debugfs_remove_files(p)
#endif

static int
sharedmem_coma_initialize(size_t size, unsigned long vma)
{
	union cmsg_sharedmem_params params;
	int ret = 0;
	unsigned long timedout;

	if (sharedmem_coma->service_id < 0) {
		dev_err(sharedmem_coma->dev,
			"sharedmem service not yet initialized\n");
		return -1;
	}

	memset(&params, 0, sizeof(params));

	params.init.virt_base = vma;
	params.init.phys_base = sharedmem_coma->mem_devs[0].base;
	params.init.size = (unsigned int)size;
	params.init.memories = sharedmem_coma->memories;
	ret = sharedmem_coma_create_message(CMSG_SHAREDMEM_INIT, &params);
	if (ret != 0) {
		dev_err(sharedmem_coma->dev,
			"coma communication failed: %d\n", ret);
		return ret;
	}

	reinit_completion(&sharedmem_coma->done);
	timedout = wait_for_completion_timeout(&sharedmem_coma->done,  HZ);

	if (sharedmem_coma->status) {
		dev_err(sharedmem_coma->dev,
			"initialization (MMU mapping) on CSS failed: %d\n",
			sharedmem_coma->status);
		ret = -1;
	}

	if (timedout <= 0) {
		dev_err(sharedmem_coma->dev,
			"initialization (MMU mapping) on CSS timed out\n");
		ret = -1;
	}

	return ret;
}

static void *
sharedmem_coma_alloc(struct device *memdev, size_t size, dma_addr_t *handle)
{
	void *addr;

	if (sharedmem_coma->service_id < 0) {
		dev_err(sharedmem_coma->dev,
			"sharedmem service not yet initialized\n");
		return NULL;
	}

	addr = dma_alloc_coherent(memdev, size, handle, GFP_KERNEL);

	return addr;
}

static int
sharedmem_coma_free(struct device *memdev, size_t size, void *cpu_addr,
		    dma_addr_t handle)
{
	if (sharedmem_coma->service_id < 0)
		return -EFAULT;

	dma_free_coherent(memdev, size, cpu_addr, handle);

	return 0;
}

static void
sharedmem_coma_process_message(void *arg, struct cmsg *cmsg)
{
	int ret = -1;
	unsigned int id;
	union cmsg_sharedmem_params *params =
			(union cmsg_sharedmem_params *)cmsg_params(cmsg);

	switch (cmsg->type) {
	case CMSG_SHAREDMEM_INIT_REPLY:
		sharedmem_coma->status = params->init_reply.status;
		complete(&sharedmem_coma->done);
		ret = 0;
		break;
	case CMSG_SHAREDMEM_MEMINFO_REPLY:
		id = params->meminfo_reply.id;
		sharedmem_coma->status = params->meminfo_reply.status;
		if (!sharedmem_coma->status) {
			sharedmem_coma->meminfo[id].base =
						params->meminfo_reply.base;
			sharedmem_coma->meminfo[id].size =
						params->meminfo_reply.size;
		}
		complete(&sharedmem_coma->done);
		ret = 0;
		break;
	default:
		dev_err(sharedmem_coma->dev, "invalid message type\n");
		break;
	}

	if (ret)
		dev_err(sharedmem_coma->dev, "coma_cmsg_send failed\n");
}

static void
sharedmem_coma_remove_service(void *arg)
{
	sharedmem_coma->service_id = -1;
	sharedmem_coma->initialized = 0;
	sharedmem_coma_debugfs_remove_files(sharedmem_coma);
}

int sharedmem_coma_service_init(void)
{
	int ret;

	if (!sharedmem_coma)
		return -ENODEV;

	if (sharedmem_coma->service_id >= 0) {
		ret = -EBUSY;
		goto out;
	}

	ret = coma_register("sharedmem", 1024, NULL,
			    sharedmem_coma_process_message,
			    sharedmem_coma_remove_service, NULL);
	if (ret < 0) {
		dev_err(sharedmem_coma->dev, "coma_register failed\n");
		goto out;
	}
	sharedmem_coma->service_id = ret;

	sharedmem_coma_debugfs_create_files(sharedmem_coma);

	ret = 0;
out:
	return ret;
}
EXPORT_SYMBOL(sharedmem_coma_service_init);

static const struct vm_operations_struct mmap_mem_ops = {
};

static void
sharedmem_coma_fix_memdev_va(void)
{
	int i;
	unsigned long va_offset = 0;

	for (i = 0; i < sharedmem_coma->nr_memories; i++) {
		struct memory_device *memdev = &sharedmem_coma->mem_devs[i];

		if (!memdev->valid)
			continue;

		memdev->va_offset = va_offset;
		va_offset += memdev->size;
	}
}

static void
sharedmem_coma_prepare_memdev_allocations(void)
{
	int i, p, pages;
	void *addr;

	for (i = 0; i < sharedmem_coma->nr_memories; i++) {
		struct memory_device *memdev = &sharedmem_coma->mem_devs[i];

		if (memdev->valid >= 2)
			continue;

		if (!memdev->offset)
			continue;

		/*
		 * Occupy memory in chunks of page size to make sure we only
		 * allocate as little memory as possible. This is because
		 * otherwise the function dma_mark_declared_memory_occupied
		 * would allocate 2^order * PAGE_SIZE bytes of memory which
		 * will allocate the whole memory if the size is 128KiB and
		 * the occupied space is e.g. 80KiB
		 */
		pages = (memdev->offset + PAGE_SIZE - 1) / PAGE_SIZE;
		for (p = 0; p < pages; p++) {
			addr = dma_mark_declared_memory_occupied(
						memdev->dev,
						memdev->base + p * PAGE_SIZE,
						PAGE_SIZE);
			if (!memdev->reserved_va)
				memdev->reserved_va = addr;
		}

		memdev->valid = 2;
	}
}

static int
sharedmem_coma_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret, i;
	size_t size = vma->vm_end - vma->vm_start;
	unsigned long phys;
	unsigned long mbase, mstart = vma->vm_start;
	unsigned long moffset = vma->vm_pgoff << PAGE_SHIFT, offset;
	size_t msize, osize = 0;
	size_t dram_size = 0;

	offset = moffset;

	vma->vm_page_prot = __pgprot_modify(vma->vm_page_prot, L_PTE_MT_MASK,
					    L_PTE_MT_DEV_WC);
	vma->vm_ops = &mmap_mem_ops;

	sharedmem_coma->memories = 0;

	for (i = 0; i < sharedmem_coma->nr_memories; i++) {
		/* if valid, then map it */
		struct memory_device *memdev = &sharedmem_coma->mem_devs[i];

		if (!memdev->valid)
			continue;

		mbase = (memdev->base + memdev->offset) >> PAGE_SHIFT;
		osize += memdev->size;
		msize = memdev->size;
		if (msize > size)
			msize = size;

		if (moffset > msize) {
			moffset -= msize;
			continue;
		}

		msize -= moffset;

		phys = mbase + (moffset >> PAGE_SHIFT);

		if (remap_pfn_range(vma,
				    mstart,
				    phys,
				    msize,
				    vma->vm_page_prot)) {
			dev_err(sharedmem_coma->chardev, "remap failed\n");
			return -EAGAIN;
		}
		memdev->vma = mstart;
		mstart += msize;
		size -= msize;
		moffset = 0;

		sharedmem_coma->memories |= 1 << i;

		if (size == 0)
			break;
	}

	if (!sharedmem_coma->initialized) {
		if (sharedmem_coma->has_dram)
			dram_size = sharedmem_coma->mem_devs[0].size;

		sharedmem_coma->css_va = vma->vm_start - offset;
		ret = sharedmem_coma_initialize(dram_size,
						sharedmem_coma->css_va);
		if (ret != 0) {
			zap_page_range(vma, vma->vm_start, size);
			return -EBUSY;
		}
		sharedmem_coma->initialized = 1;
	}

	return 0;
}

static int
sharedmem_coma_ioctl_alloc(void __user *argp, struct file *file)
{
	struct sharedmem_coma_handle handle;
	struct sharedmem_coma_alloc *alloc;
	struct memory_device *memdev;
	void *addr;

	if (!sharedmem_coma->initialized)
		return -EFAULT;

	if (copy_from_user(&handle, argp, sizeof(handle)))
		return -EFAULT;

	if (handle.id >= sharedmem_coma->nr_memories)
		return -EINVAL;

	memdev = &sharedmem_coma->mem_devs[array_index_nospec(handle.id,
							      MAX_MEMDEVS)];
	if (!memdev)
		return -EINVAL;

	if (!memdev->valid)
		return -EBUSY;

	addr = sharedmem_coma_alloc(memdev->dev, handle.size,
				    (dma_addr_t *)&handle.pa);
	if (!addr)
		return -ENOMEM;

	handle.css_va = sharedmem_coma->css_va + memdev->va_offset +
			(handle.pa - (memdev->base + memdev->offset));
	handle.mask = memdev->mask;

	alloc = kmalloc(sizeof(*alloc), GFP_KERNEL);
	if (!alloc)
		return -ENOMEM;

	alloc->size = handle.size;
	alloc->addr = addr;
	alloc->pa = handle.pa;
	alloc->file = file;
	alloc->id = handle.id;
	list_add_tail(&alloc->node, &sharedmem_coma->allocations);

	return copy_to_user(argp, &handle, sizeof(handle));
}

static void
sharedmem_coma_file_free(struct file *file)
{
	struct list_head *cur, *tmp;
	struct sharedmem_coma_alloc *alloc;
	struct device *memdev;

	list_for_each_safe(cur, tmp, &sharedmem_coma->allocations) {
		alloc = list_entry(cur, struct sharedmem_coma_alloc, node);
		if (alloc->file == file) {
			list_del(&alloc->node);

			memdev = sharedmem_coma->mem_devs[alloc->id].dev;

			(void)sharedmem_coma_free(memdev,
						  alloc->size,
						  alloc->addr,
						  alloc->pa);
			kfree(alloc);
		}
	}
}

static void
sharedmem_coma_free_addr(struct file *file,
			 struct sharedmem_coma_handle *handle)
{
	struct list_head *cur, *tmp;
	struct sharedmem_coma_alloc *alloc;
	struct device *memdev;

	list_for_each_safe(cur, tmp, &sharedmem_coma->allocations) {
		alloc = list_entry(cur, struct sharedmem_coma_alloc, node);
		if (alloc->file == file && alloc->pa == handle->pa &&
		    alloc->id == handle->id && alloc->size == handle->size) {
			list_del(&alloc->node);

			memdev = sharedmem_coma->mem_devs[alloc->id].dev;

			(void)sharedmem_coma_free(memdev,
						  alloc->size,
						  alloc->addr,
						  alloc->pa);
			kfree(alloc);
		}
	}
}

static int
sharedmem_coma_query_memory(unsigned int id,
			    struct sharedmem_coma_memory_info *info)
{
	union cmsg_sharedmem_params params;
	int ret;
	unsigned long timedout;

	if (sharedmem_coma->service_id < 0) {
		dev_err(sharedmem_coma->dev,
			"sharedmem service not yet initialized\n");
		return -1;
	}

	params.meminfo.id = id;
	ret = sharedmem_coma_create_message(CMSG_SHAREDMEM_MEMINFO, &params);
	if (ret != 0) {
		dev_err(sharedmem_coma->dev,
			"coma communication failed: %d\n", ret);
		return ret;
	}

	reinit_completion(&sharedmem_coma->done);
	timedout = wait_for_completion_timeout(&sharedmem_coma->done,  HZ);
	if (timedout <= 0) {
		dev_err(sharedmem_coma->dev, "memory query timed out\n");
		return -EIO;
	}

	if (sharedmem_coma->status < 0)
		return -ENOENT;

	info->base = sharedmem_coma->meminfo[id].base;
	info->size = sharedmem_coma->meminfo[id].size;
	return ret;
}

static int
sharedmem_swap_and_correct_mem_dev(unsigned int css_i, unsigned int app_i)
{
	int i;
	u32 base;
	struct memory_device mdev, *p;
	struct sharedmem_coma_memory_info *info =
						&sharedmem_coma->meminfo[css_i];

	/* swap entries, correct memdev -> offset, size, set valid */
	for (i = 1; i < sharedmem_coma->nr_memories; i++) {
		p = &sharedmem_coma->mem_devs[i];
		if (p->valid)
			continue;
		/*
		 * mem_devs->base can be different from CSS, so the mask must
		 * be applied before comparing
		 */
		base = p->base & p->mask;

		if (info->base >= base &&
		    info->base <= (base + p->offset + p->size)) {
			/* found entry */
			/* correct offset and size */
			p->offset = info->base - base;
			p->size = info->size;
			p->valid = 1;

			if (i != app_i) {
				/* needs swap */
				memcpy(&mdev, p, sizeof(*p));
				memcpy(p, &sharedmem_coma->mem_devs[app_i],
				       sizeof(*p));
				memcpy(&sharedmem_coma->mem_devs[app_i], &mdev,
				       sizeof(*p));
			}
			return 0;
		}
	}
	return -ENOENT;
}

static long
sharedmem_coma_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int size, ret, i;
	void __user *argp = (void __user *)arg;
	struct sharedmem_coma_handle handle;

	if (_IOC_TYPE(cmd) != SHAREDMEM_IOC_MAGIC)
		return -EINVAL;

	if (_IOC_NR(cmd) > SHAREDMEM_IOC_MAXNR)
		return -EINVAL;

	size = _IOC_SIZE(cmd);

	if (!access_ok(argp, size))
		return -EFAULT;

	switch (cmd) {
	case SHAREDMEM_IOCALLOC: {
		return sharedmem_coma_ioctl_alloc(argp, file);
	}

	case SHAREDMEM_IOCFREE: {
		if (!sharedmem_coma->initialized)
			return -EFAULT;

		if (copy_from_user(&handle, argp, sizeof(handle)))
			return -EFAULT;

		sharedmem_coma_free_addr(file, &handle);

		return 0;
	}

	case SHAREDMEM_IOCNRMEMS: {
		struct sharedmem_coma_memories mems;

		mems.nr = sharedmem_coma->nr_memories;

		return copy_to_user(argp, &mems, sizeof(mems));
	}

	case SHAREDMEM_IOCMEMINFO: {
		int app_i = 0;
		int doffset = 0;
		struct sharedmem_coma_memory_info info[MAX_MEMDEVS];

		memset(info, 0, sizeof(info));

		if (sharedmem_coma->has_dram) {
			/* dram entry */
			memset(info[0].name, 0, sizeof(info[0].name));
			strncpy(info[0].name, sharedmem_coma->mem_devs[0].name,
				MAX_MEM_NAME);
			info[0].base = sharedmem_coma->mem_devs[0].base;
			info[0].size = sharedmem_coma->mem_devs[0].size;
			info[0].va_offset =
					sharedmem_coma->mem_devs[0].va_offset;

			if (sharedmem_coma->mem_devs[0].valid == 0)
				sharedmem_coma->mem_devs[0].valid = 1;

			app_i = 1;
			doffset = 1;
		}
		for (i = doffset; i < MAX_MEMDEVS; i++) {
			ret = sharedmem_coma_query_memory(i - doffset,
							  &info[app_i]);
			if (ret < 0 && ret != -ENOENT)
				return ret;

			/* in case of ENOENT, just skip this entry */
			if (ret != 0)
				continue;

			if (app_i >= sharedmem_coma->nr_memories)
				break;

			memset(info[app_i].name, 0, sizeof(info[app_i].name));
			ret = sharedmem_swap_and_correct_mem_dev(i - doffset,
								 app_i);
			if (ret != 0)
				continue;

			strncpy(info[app_i].name,
				sharedmem_coma->mem_devs[app_i].name,
				MAX_MEM_NAME);

			info[app_i].base |=
					~sharedmem_coma->mem_devs[app_i].mask;
			app_i++;
		}

		sharedmem_coma_fix_memdev_va();

		/* copy va offsets to user structure */
		for (i = doffset; i < sharedmem_coma->nr_memories; i++) {
			info[i].va_offset =
				sharedmem_coma->mem_devs[i].va_offset;
		}

		sharedmem_coma_prepare_memdev_allocations();

		return copy_to_user(argp, &info,
				sizeof(info[0]) * sharedmem_coma->nr_memories);
	}

	default:
		return -EINVAL;
	}
}

static int
sharedmem_coma_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
sharedmem_coma_release(struct inode *inode, struct file *file)
{
	int i, p, pages;
	void *addr;

	sharedmem_coma_file_free(file);

	/* free reserved memory portions */
	for (i = 0; i < sharedmem_coma->nr_memories; i++) {
		struct memory_device *memdev = &sharedmem_coma->mem_devs[i];

		if (!memdev->valid)
			continue;
		memdev->valid = 0;

		if (!memdev->reserved_va)
			continue;

		pages = (memdev->offset + PAGE_SIZE - 1) / PAGE_SIZE;
		addr = memdev->reserved_va;
		for (p = 0; p < pages; p++) {
			dma_release_from_dev_coherent(memdev->dev, 0, addr);
			addr += PAGE_SIZE;
		}
	}

	return 0;
}

static struct file_operations sharedmem_coma_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= sharedmem_coma_ioctl,
	.open		= sharedmem_coma_open,
	.release	= sharedmem_coma_release,
	.mmap		= sharedmem_coma_mmap,
};

static int
sharedmem_get_size_and_base(struct device *dev, struct memory_device *memdev,
			    unsigned int index)
{
	void *addr;
	struct device_node *np = dev->of_node;
	struct device_node *mnp;
	u32 values[2];

	/* get devicenode of reserved memory area */
	mnp = of_parse_phandle(np, "memory-region", index);
	if (!mnp) {
		dev_err(dev,
			"cannot read phandle of reserved memory at index %u\n",
			index);
		return -EIO;
	}

	/* read size property */
	if (of_property_read_u32(mnp, "size", &memdev->size)) {
		/* if no size property is present, read the "reg" property */
		if (of_property_read_u32_array(mnp, "reg", values, 2)) {
			dev_err(dev,
				"cannot determine size of memory at index %u\n",
				index);
			return -EIO;
		}
		memdev->size = values[1];
	}

	if (of_property_read_u32_index(np, "memory-mask", index,
				       &memdev->mask)) {
		dev_err(dev, "could not get mask of shared memory from DT\n");
		return -EIO;
	}

	if (of_property_read_string_index(np, "memory-name", index,
					  &memdev->name)) {
		dev_err(dev, "could not name of shared memory from DT\n");
		return -EIO;
	}

	addr = dma_alloc_coherent(memdev->dev, PAGE_SIZE,
				  &memdev->base, GFP_KERNEL);
	if (!addr) {
		dev_err(dev, "could not allocate shared memory\n");
		return -EIO;
	}

	dma_free_coherent(memdev->dev, memdev->size, addr, memdev->base);
	return 0;
}

static void
sharedmem_coma_free_memdevs(void)
{
	int i;

	for (i = 0; i < sharedmem_coma->nr_memories; i++) {
		struct memory_device *memdev = &sharedmem_coma->mem_devs[i];

		if (memdev->dev)
			device_unregister(memdev->dev);
	}
}

static int
sharedmem_coma_probe(struct platform_device *pdev)
{
	int nr_memories, i, ret;
	struct device_node *np = pdev->dev.of_node;

	sharedmem_coma = devm_kzalloc(&pdev->dev, sizeof(*sharedmem_coma),
				      GFP_KERNEL);
	if (!sharedmem_coma) {
		dev_err(&pdev->dev, "Out of memory\n");
		ret = -ENOMEM;
		goto out_err;
	}
	sharedmem_coma->dev = &pdev->dev;
	sharedmem_coma->service_id = -1;
	INIT_LIST_HEAD(&sharedmem_coma->allocations);

	nr_memories = of_property_count_u32_elems(np, "memory-region");
	if (nr_memories <= 0 || nr_memories > MAX_MEMDEVS)
		return -EINVAL;

	sharedmem_coma->nr_memories = nr_memories;

	/*
	 * by default all these memdevs are not valid. They become valid if
	 * the correct base and size has been queried from the CSS
	 */
	for (i = 0; i < nr_memories; i++) {
		struct memory_device *memdev = &sharedmem_coma->mem_devs[i];

		memdev->dev = devm_kzalloc(sharedmem_coma->dev,
					   sizeof(*memdev->dev), GFP_KERNEL);

		device_initialize(memdev->dev);
		dev_set_name(memdev->dev, "%s:mem%d", dev_name(&pdev->dev),
			     i);
		memdev->dev->parent = &pdev->dev;
		memdev->dev->coherent_dma_mask = pdev->dev.coherent_dma_mask;
		memdev->dev->dma_mask = pdev->dev.dma_mask;
		memdev->dev->release = of_reserved_mem_device_release;

		if (device_add(memdev->dev) == 0) {
			ret = of_reserved_mem_device_init_by_idx(memdev->dev,
								 np, i);
			if (ret != 0) {
				device_del(memdev->dev);
				goto out_free_reserved;
			}

			if (sharedmem_get_size_and_base(sharedmem_coma->dev,
							memdev, i)) {
				ret = -EINVAL;
				goto out_free_memdev;
			}
			memdev->offset = 0;

			if (strlen(memdev->name) == 4 &&
			    strcasecmp(memdev->name, "dram") == 0)
				sharedmem_coma->has_dram = 1;

			/*
			 * copy over to memory info struct, these are corrected
			 * at a later stage by querying the CSS
			 */
			sharedmem_coma->meminfo[i].base = memdev->base;
			sharedmem_coma->meminfo[i].size = memdev->size;
		}
	}
	sharedmem_coma_fix_memdev_va();

	ret = alloc_chrdev_region(&sharedmem_coma->chrdev, 0, 1,
				  COMA_SHAREDMEM);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"error %d adding character device region\n", ret);
		goto out_free_memdev;
	}

	cdev_init(&sharedmem_coma->cdev, &sharedmem_coma_fops);
	sharedmem_coma->cdev.owner = THIS_MODULE;
	ret = cdev_add(&sharedmem_coma->cdev, sharedmem_coma->chrdev, 1);
	if (ret) {
		dev_err(&pdev->dev, "error %d adding character device", ret);
		goto out_free_chrdev_region;
	}

	/* Create a sysfs class */
	sharedmem_coma->class = class_create(THIS_MODULE, "sharedmem");
	if (IS_ERR(sharedmem_coma->class)) {
		ret = PTR_ERR(sharedmem_coma->class);
		dev_err(&pdev->dev, "error %d creating the voice class", ret);
		goto out_free_cdev;
	}

	sharedmem_coma->chardev = device_create(
					sharedmem_coma->class, &pdev->dev,
					MKDEV(MAJOR(sharedmem_coma->chrdev), 0),
					NULL, "sharedmem");
	if (IS_ERR(sharedmem_coma->dev)) {
		ret = PTR_ERR(sharedmem_coma->dev);
		dev_err(&pdev->dev, "could not create device");
		goto out_destroy_class;
	}

	init_completion(&sharedmem_coma->done);

	platform_set_drvdata(pdev, sharedmem_coma);

	dev_info(&pdev->dev, "successfully registered\n");
	return 0;

out_destroy_class:
	class_destroy(sharedmem_coma->class);
out_free_cdev:
	cdev_del(&sharedmem_coma->cdev);
out_free_chrdev_region:
	unregister_chrdev_region(sharedmem_coma->chrdev, 1);
out_free_memdev:
	sharedmem_coma_free_memdevs();
out_free_reserved:
	of_reserved_mem_device_release(sharedmem_coma->dev);
out_err:
	sharedmem_coma = NULL;
	return ret;
}

static int __exit
sharedmem_coma_remove(struct platform_device *dev)
{
	/* FIXME: racy wrt. sharedmem_coma_remove_service callback */
	if (sharedmem_coma->service_id >= 0) {
		coma_deregister(sharedmem_coma->service_id);
		sharedmem_coma_remove_service(NULL);
	}
	device_destroy(sharedmem_coma->class,
		       MKDEV(MAJOR(sharedmem_coma->chrdev), 0));
	class_destroy(sharedmem_coma->class);
	cdev_del(&sharedmem_coma->cdev);
	unregister_chrdev_region(sharedmem_coma->chrdev, 1);
	of_reserved_mem_device_release(sharedmem_coma->dev);

	sharedmem_coma_free_memdevs();

	return 0;
}

static struct of_device_id sharedmem_coma_of_ids[] = {
	{ .compatible = "dspg,sharedmem-coma" },
	{ },
};

static struct platform_driver sharedmem_coma_platform_driver = {
	.driver = {
		.name	= COMA_SHAREDMEM,
		.owner	= THIS_MODULE,
		.of_match_table = sharedmem_coma_of_ids,
	},
	.probe		= sharedmem_coma_probe,
	.remove		= __exit_p(sharedmem_coma_remove),
};

static int __init sharedmem_coma_init(void)
{
	return platform_driver_register(&sharedmem_coma_platform_driver);
}

static void __exit sharedmem_coma_exit(void)
{
	platform_driver_unregister(&sharedmem_coma_platform_driver);
}

module_init(sharedmem_coma_init);
module_exit(sharedmem_coma_exit);

MODULE_DESCRIPTION("DSPG SHAREDMEM-COMA driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_ALIAS("platform:sharedmem-coma");
