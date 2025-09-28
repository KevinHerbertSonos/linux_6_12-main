// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */
#include <linux/dma-buf.h>

#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/capability.h>
#include <drm/drm_mm.h>
#include <drm/drm_prime.h>
#include <linux/version.h>

#include "drm_syna_drv.h"
#include "drm_syna_gem.h"
#include "syna_drm.h"
#include "drm_syna_port.h"

static int syna_gem_prime_vmap(struct drm_gem_object *obj, struct iosys_map *map)
{
	struct syna_gem_object *syna_obj = to_syna_obj(obj);

	if (!syna_obj) {
		DRM_ERROR("%s %d  syna obj is NULL!!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (!syna_obj->kernel_vir_addr) {
		DRM_ERROR("%s %d  syna obj->kernel_vir_addr is NULL!!\n", __func__, __LINE__);
		return -EINVAL;
	}

	DRM_DEBUG_PRIME("Mapping to virtual address : %p/%llx\n",
		syna_obj->kernel_vir_addr, (long long int)syna_obj->kernel_vir_addr);

	iosys_map_set_vaddr(map, syna_obj->kernel_vir_addr);

	return 0;
}

static void syna_gem_prime_vunmap(struct drm_gem_object *obj, struct iosys_map *map)
{
	/* Nothing to do if allocated by DMA mapping API. */
}

static const struct drm_gem_object_funcs syna_gem_object_funcs = {
	.free = syna_gem_object_free_priv,
	SYNA_DRM_DRIVER_GEM_VM_OPS_GEM_OBJ_INTERFAES()
	.get_sg_table = syna_gem_prime_get_sg_table,
	.mmap = syna_drm_gem_object_mmap,
	.vmap = syna_gem_prime_vmap,
	.vunmap	= syna_gem_prime_vunmap,
};

int syna_gem_init(struct drm_device *dev)
{
	struct syna_drm_private *dev_priv = dev->dev_private;
	int ret;

	dev_priv->mem_list = kzalloc(sizeof(VPP_MEM_LIST), GFP_KERNEL);

	if (!dev_priv->mem_list) {
		DRM_ERROR("%s alloc mem for mem_list failed\n", __func__);
		return -1;
	}

	dev_priv->mem_list->dev = dev->dev;

	ret = VPP_MEM_InitMemory(dev_priv->mem_list);
	if (ret != 0) {
		DRM_ERROR("Can't Initialize mem_list\n");
		return -1;
	}

	return 0;
}

void syna_gem_deinit(struct drm_device *dev)
{
	struct syna_drm_private *dev_priv = dev->dev_private;

	DRM_DEBUG_PRIME("%s\n", __FUNCTION__);

	if (dev_priv->mem_list != 0) {
		VPP_MEM_DeInitMemory(dev_priv->mem_list);
		kfree(dev_priv->mem_list);
	}
}

static void syna_gem_free_buf(struct drm_gem_object *obj, struct syna_gem_object *syna_obj)
{
	struct syna_drm_private *dev_priv = obj->dev->dev_private;

	if (syna_obj->shm_handle.handle) {
		syna_vpp_reset_buffers(syna_obj);
		VPP_MEM_FreeMemory(dev_priv->mem_list, VPP_MEM_TYPE_DMA, &syna_obj->shm_handle);
	} else if (syna_obj->sgt) {
		DRM_DMABUF_UNMAP(syna_obj)
		dma_buf_end_cpu_access(syna_obj->dma_buf, DMA_BIDIRECTIONAL);
	}
}

void syna_gem_object_free_priv(struct drm_gem_object *obj)
{
	struct syna_gem_object *syna_obj;
	if (!obj) {
		DRM_ERROR("%s %d  obj is NULL!!\n", __func__, __LINE__);
		return;
	}
	syna_obj = to_syna_obj(obj);
	if (!syna_obj) {
		DRM_ERROR("%s %d  syna obj is NULL!!\n", __func__, __LINE__);
		return;
	}

	syna_vpp_reset_buffers(syna_obj);

	drm_gem_free_mmap_offset(obj);

	syna_gem_free_buf(obj, syna_obj);

	if (syna_obj->sgt) {
		drm_prime_gem_destroy(obj, syna_obj->sgt);
	}

	drm_gem_object_release(&syna_obj->base);
	kfree(syna_obj);
}

/*
 * Allocate a sg_table for this GEM object.
 * Note: Both the table's contents, and the sg_table itself must be freed by
 *       the caller.
 * Returns a pointer to the newly allocated sg_table, or an ERR_PTR() error.
 */
struct sg_table *syna_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct syna_gem_object *syna_obj = to_syna_obj(obj);
	struct sg_table *sgt;
	int ret;

	if (!syna_obj) {
		DRM_ERROR("%s %d  syna obj is NULL!!\n", __func__, __LINE__);
		return ERR_PTR(-EINVAL);
	}
	if (syna_obj->sgt) {
		DRM_DEBUG_PRIME("passing the imported sgt for syna_gem_prime_get_sg_table\n");
		sgt = syna_obj->sgt;
	} else {
		if (syna_obj->phyaddr == 0)
			return ERR_PTR(-EINVAL);

		sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
		if (!sgt) {
			DRM_ERROR("failed to allocate sgt\n");
			return ERR_PTR(-ENOMEM);
		}

		ret = sg_alloc_table(sgt, 1, GFP_KERNEL);
		if (ret) {
			DRM_ERROR("failed to allocate sgt, %d\n", ret);
			kfree(sgt);
			return ERR_PTR(ret);
		}

		sg_set_page(sgt->sgl, pfn_to_page(PFN_DOWN(syna_obj->phyaddr)),
				obj->size, 0);
	}

	return sgt;
}

struct drm_gem_object *syna_gem_prime_import_sg_table(struct drm_device *dev,
		                                      struct dma_buf_attachment *attach,
						      struct sg_table *sgt)
{
	struct syna_gem_object *syna_obj;
	struct scatterlist *sg;
	phys_addr_t phyaddr;
	int i;
	struct page;
	int pageCount = 0;
	ssize_t size = 0;
	struct drm_gem_object *obj;

	for_each_sg(sgt->sgl, sg, sgt->orig_nents, i) {
		phyaddr = sg_phys(sg);
		size += sg->length;
		pageCount++;
		DRM_DEBUG_PRIME("importing sg table: page:%d phyaddr:0x%lx size:%d\n",
			         pageCount, phyaddr, sg->length);
	}

	syna_obj = kzalloc(sizeof(*syna_obj), GFP_KERNEL);
	obj = &syna_obj->base;
	obj->funcs = &syna_gem_object_funcs;
	drm_gem_private_object_init(dev, obj, size);

	syna_obj->sgt = sgt;

	syna_obj->dma_attrs = DMA_ATTR_WRITE_COMBINE|DMA_ATTR_NO_KERNEL_MAPPING;
	syna_obj->dma_buf = attach->dmabuf;
	if (IS_ERR(syna_obj->dma_buf)) {
		DRM_ERROR("%s %d  no dmabuf present in attach !\n", __func__, __LINE__);
		goto err_synaobj_free;
	}

	if (dma_buf_begin_cpu_access(syna_obj->dma_buf, DMA_FROM_DEVICE) != 0) {
		DRM_ERROR("%s:%d dma_buf_begin_cpu_access fail\n",
				__func__, __LINE__);
		goto err_synaobj_free;
	}

	syna_obj->kernel_vir_addr = syna_drm_get_vaddr_from_gem_obj(syna_obj);
	if (IS_ERR(syna_obj->kernel_vir_addr)) {
		DRM_ERROR("%s %d dma buf kmap failed!\n", __func__, __LINE__);
		goto err_cpuacc_rel;
	}

	syna_obj->shm_handle.handle = 0;
	if (pageCount == 1) {
		syna_obj->phyaddr = phyaddr;
	}
	return &syna_obj->base;

err_cpuacc_rel:
	dma_buf_end_cpu_access(syna_obj->dma_buf, DMA_FROM_DEVICE);
err_synaobj_free:
	kfree(syna_obj);
	return ERR_PTR(-ENOMEM);
}

static int syna_gem_alloc_buf(struct drm_device *dev, struct syna_gem_object *syna_obj, bool alloc_kmap)
{
	struct drm_gem_object *obj = &syna_obj->base;
	struct syna_drm_private *dev_priv = dev->dev_private;
	int ret;

	if (!obj) {
		DRM_ERROR("%s %d  obj is NULL!!\n", __func__, __LINE__);
		return -EINVAL;
	}

	syna_obj->dma_attrs = DMA_ATTR_WRITE_COMBINE;

	if (!alloc_kmap)
		syna_obj->dma_attrs |= DMA_ATTR_NO_KERNEL_MAPPING;

	syna_obj->shm_handle.size = obj->size;
	ret = VPP_MEM_AllocateMemory(dev_priv->mem_list, VPP_MEM_TYPE_DMA,
				&syna_obj->shm_handle, 0);
	if (ret != 0) {
		DRM_ERROR("%s %d  gem alloc failed!\n", __func__, __LINE__);
		return -ENOMEM;
	}

	syna_obj->phyaddr = (phys_addr_t)syna_obj->shm_handle.p_addr;
	syna_obj->kernel_vir_addr = syna_obj->shm_handle.k_addr;

	DRM_DEBUG_PRIME("Alloc memory for (%d) %lx\n", current->pid,
			syna_obj->phyaddr);

	return 0;
}

static struct drm_gem_object *syna_gem_create_object(struct drm_device *drm,
					      unsigned int size,
					      bool alloc_kmap)
{
	struct syna_gem_object *syna_obj;
	struct drm_gem_object *obj;
	int ret;

	size = round_up(size, PAGE_SIZE);

	syna_obj = kzalloc(sizeof(*syna_obj), GFP_KERNEL);
	if (!syna_obj) {
		DRM_ERROR("%s %d  fail to allocate syna obj!!\n",
			  __func__, __LINE__);
		return ERR_PTR(-ENOMEM);
	}

	obj = &syna_obj->base;

	SYNA_DRM_DRIVER_SET_GEM_OBJ_FUNCS_INTERFACES(obj);

	drm_gem_private_object_init(drm, obj, size);

	ret = syna_gem_alloc_buf(drm, syna_obj, 0);
	if (ret) {
		DRM_ERROR("%s %d  fail to allocate buffer!!\n",
			  __func__, __LINE__);
		goto err_free_syna_obj;
	}

	return &syna_obj->base;

err_free_syna_obj:
	kfree(syna_obj);
	return ERR_PTR(ret);
}

int syna_gem_dumb_create(struct drm_file *file,
			 struct drm_device *dev,
			 struct drm_mode_create_dumb *args)
{
	struct drm_gem_object *obj;
	u32 handle;
	u32 pitch;
	size_t size;
	int err;

	pitch = args->width * (ALIGN(args->bpp, 8) >> 3);
	size = PAGE_ALIGN(pitch * args->height);

	obj = syna_gem_create_object(dev, size, 0);
	if (IS_ERR(obj)) {
		DRM_ERROR("%s:%d syna_gem_create_object fail\n",
			__func__, __LINE__);
		return PTR_ERR(obj);
	}

	err = drm_gem_handle_create(file, obj, &handle);
	if (err) {
		DRM_ERROR("%s %d drm_gem_handle_create fail\n",
				__func__, __LINE__);
		syna_gem_object_free_priv(obj);
		return err;
	}

	args->handle = handle;
	args->pitch = pitch;
	args->size = size;
	DRM_GEM_OBJECT_PUT(obj);
	return 0;
}

int syna_drm_gem_object_mmap(struct drm_gem_object *obj,
		struct vm_area_struct *vma)
{
	int ret;
	int i;
	size_t size = 0;
	size_t map_size = 0;
	phys_addr_t phyaddr;
	size_t page_offset = 0;
	struct scatterlist *sg;
	unsigned long   vm_start;
	unsigned long   vm_pgoff;

	struct syna_gem_object *syna_obj = to_syna_obj(obj);

	if (!syna_obj) {
		DRM_ERROR("%s:%d syna obj is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0))
	vma->vm_flags &= ~VM_PFNMAP;
#endif

	if (syna_obj->sgt) {
		/* gem buffer imported from external object, could be scattered */
		vm_start = vma->vm_start;
		vm_pgoff = vma->vm_pgoff;
		for_each_sg(syna_obj->sgt->sgl, sg, syna_obj->sgt->orig_nents, i) {
			phyaddr = sg_phys(sg);
			size += sg->length;
			if ((vm_pgoff >= page_offset) && (vm_pgoff <= size/PAGE_SIZE)) {
				if ((vma->vm_end - vm_start) < (size-(page_offset*PAGE_SIZE))) {
					map_size = vma->vm_end - vm_start;
				} else {
					map_size = sg->length;
				}
				ret = remap_pfn_range(vma,
						vm_start,
						(phyaddr >> PAGE_SHIFT) + (vm_pgoff-page_offset),
						map_size, vma->vm_page_prot);
				if (ret) {
					DRM_ERROR("%s:%d Map fail\n", __func__, __LINE__);
					return -EAGAIN;
				}
				DRM_DEBUG_PRIME("mapping addr 0x%lx (%ld bytes) at virtual 0x%lx\n",
						(phyaddr >> PAGE_SHIFT)+(vm_pgoff-page_offset),
						map_size, vm_start);

				vm_start += map_size;
				vm_pgoff += map_size/PAGE_SIZE;
				if (vm_start >= vma->vm_end) {
					DRM_DEBUG_PRIME("%ld bytes mapped to 0x%lx\n",
							size, vma->vm_start);
					break;
				}
			}
			page_offset += sg->length/PAGE_SIZE;
		}
	} else {
		size = vma->vm_end - vma->vm_start;

		vma->vm_pgoff = 0;
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
		ret = remap_pfn_range(vma,
				vma->vm_start,
				(syna_obj->phyaddr >> PAGE_SHIFT) + vma->vm_pgoff,
				size, vma->vm_page_prot);
		if (ret) {
			DRM_ERROR("%s:%d Map fail\n", __func__, __LINE__);
			return -EAGAIN;
		}
	}

	DRM_DEBUG_PRIME("%s %d PID(%d)  Map %lx to %lx\n", __func__,
			__LINE__, current->pid, syna_obj->phyaddr,
			vma->vm_start);

	return ret;
}

int syna_gem_dumb_map_offset(struct drm_file *file,
			     struct drm_device *dev,
			     uint32_t handle, uint64_t *offset)
{
	struct drm_gem_object *obj = NULL;
	int err = 0;

	mutex_lock(&dev->struct_mutex);

	obj = drm_gem_object_lookup(file, handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		goto exit_unlock;
	}

	err = drm_gem_create_mmap_offset(obj);
	if (err)
		goto exit_obj_unref;

	*offset = drm_vma_node_offset_addr(&obj->vma_node);
	DRM_DEBUG_PRIME("%s (PID:%d) map handle(%d) to %llx\n", __func__,
		 current->pid, handle, *offset);
exit_obj_unref:
	DRM_GEM_OBJECT_PUT(obj);
exit_unlock:
	mutex_unlock(&dev->struct_mutex);
	return err;
}

u64 syna_gem_get_dev_addr(struct drm_gem_object *obj)
{
	struct syna_gem_object *syna_obj = to_syna_obj(obj);

	if (!syna_obj) {
		DRM_ERROR("%s:%d syna obj is NULL!!\n", __func__, __LINE__);
		return 0;
	}
	return syna_obj->dev_addr;
}

int syna_gem_object_create_ioctl_priv(struct drm_device *dev,
				      void *data, struct drm_file *file)
{
	struct drm_syna_gem_create *args = data;
	struct drm_gem_object *obj;
	int err;

	if (args->flags) {
		DRM_ERROR("invalid flags: %#08x\n", args->flags);
		return -EINVAL;
	}

	if (args->handle) {
		DRM_ERROR("invalid handle (this should always be 0)\n");
		return -EINVAL;
	}

	obj = syna_gem_create_object(dev, PAGE_ALIGN(args->size), args->flags);
	if (IS_ERR(obj))
		return PTR_ERR(obj);

	err = drm_gem_handle_create(file, obj, &args->handle);
	if (err) {
		DRM_ERROR("%s %d drm_gem_handle_create fail\n",
			__func__, __LINE__);
		syna_gem_object_free_priv(obj);
		return err;
	}

	DRM_GEM_OBJECT_PUT(obj);
	return err;
}

int syna_gem_object_mmap_ioctl(struct drm_device *dev, void *data,
			       struct drm_file *file)
{
	struct drm_syna_gem_mmap *args = (struct drm_syna_gem_mmap *)data;

	if (args->pad) {
		DRM_ERROR("invalid pad (this should always be 0)\n");
		return -EINVAL;
	}

	if (args->offset) {
		DRM_ERROR("invalid offset (this should always be 0)\n");
		return -EINVAL;
	}

	return syna_gem_dumb_map_offset(file, dev, args->handle, &args->offset);
}

int syna_gem_object_cpu_prep_ioctl(struct drm_device *dev, void *data,
				   struct drm_file *file)
{
	struct drm_syna_gem_cpu_prep *args =
	    (struct drm_syna_gem_cpu_prep *)data;
	struct drm_gem_object *obj;
	struct syna_gem_object *syna_obj;
	int err = 0;

	if (args->flags & ~(SYNA_GEM_CPU_PREP_READ |
				SYNA_GEM_CPU_PREP_WRITE |
				SYNA_GEM_CPU_PREP_NOWAIT)) {
		DRM_ERROR("invalid flags: %#08x\n", args->flags);
		return -EINVAL;
	}

	mutex_lock(&dev->struct_mutex);

	obj = drm_gem_object_lookup(file, args->handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		goto exit_unlock;
	}

	syna_obj = to_syna_obj(obj);

	if (!syna_obj) {
		err = -EINVAL;
		goto exit_unref;
	}
	if (syna_obj->cpu_prep) {
		err = -EBUSY;
		goto exit_unref;
	}

	if (!err)
		syna_obj->cpu_prep = true;

exit_unref:
	DRM_GEM_OBJECT_PUT(obj);
exit_unlock:
	mutex_unlock(&dev->struct_mutex);
	return err;
}

int syna_gem_object_cpu_fini_ioctl(struct drm_device *dev, void *data,
				   struct drm_file *file)
{
	struct drm_syna_gem_cpu_fini *args =
	    (struct drm_syna_gem_cpu_fini *)data;
	struct drm_gem_object *obj;
	struct syna_gem_object *syna_obj;
	int err = 0;

	if (args->pad) {
		DRM_ERROR("invalid pad (this should always be 0)\n");
		return -EINVAL;
	}

	mutex_lock(&dev->struct_mutex);

	obj = drm_gem_object_lookup(file, args->handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		goto exit_unlock;
	}

	syna_obj = to_syna_obj(obj);

	if (!syna_obj || !syna_obj->cpu_prep) {
		err = -EINVAL;
		goto exit_unref;
	}

	syna_obj->cpu_prep = false;

exit_unref:
	DRM_GEM_OBJECT_PUT(obj);
exit_unlock:
	mutex_unlock(&dev->struct_mutex);
	return err;
}
