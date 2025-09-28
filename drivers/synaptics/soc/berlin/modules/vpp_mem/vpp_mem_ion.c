// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated
 *
 */

#include "vpp_mem.h"
#include <linux/version.h>

#define NW_SHM_POOL_ATTR	(ION_A_FC | ION_A_NS)
#define NC_SHM_POOL_ATTR	(ION_A_NC | ION_A_NS)

static unsigned int heap_id_mask;
static unsigned int heap_id_mask_nc;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
#define DMA_BUF_KERNEL_MAP(ion_dma_buf) dma_buf_vmap(ion_dma_buf)
#define DMA_BUF_KERNEL_UNMAP(ion_dma_buf, k_addr) dma_buf_vunmap(ion_dma_buf, k_addr)
#else
#define DMA_BUF_KERNEL_MAP(ion_dma_buf) dma_buf_kmap(ion_dma_buf, 0)
#define DMA_BUF_KERNEL_UNMAP(ion_dma_buf, k_addr) dma_buf_kunmap(ion_dma_buf, 0, k_addr)
#endif

/* ION MEMORY local functions*/
static int Ion_get_heap_mask_by_attr(unsigned int heap_attr, unsigned int *heap_id_mask)
{
	int heap_num, i;
	struct ion_heap_data *hdata;
	unsigned int heap_type;

	hdata = kmalloc(sizeof(*hdata) * ION_NUM_MAX_HEAPS, GFP_KERNEL);
	if (!hdata) {
		pr_err("%s alloc mem failed\n", __func__);
		return VPP_MEM_ERROR_TYPE_ENOMEM;
	}

	heap_type = ((heap_attr & NW_SHM_POOL_ATTR) == NW_SHM_POOL_ATTR) ?
			ION_HEAP_TYPE_DMA : ION_HEAP_TYPE_BERLIN_NC;
	heap_num = ion_query_heaps_kernel(hdata, ION_NUM_MAX_HEAPS);

	*heap_id_mask = 0;
	for (i = 0; i < heap_num; i++) {
		if (hdata[i].type == heap_type)
			*heap_id_mask |= 1 << hdata[i].heap_id;
	}
	kfree(hdata);

	return VPP_MEM_ERROR_TYPE_OK;
}

static void * Ion_ion_dmabuf_get_phy(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct page *page;

	table = buffer->sg_table;
	page = sg_page(table->sgl);
	return (void *)PFN_PHYS(page_to_pfn(page));
}

static int Ion_InitIonMem(int index)
{
	int ret;

	ret = Ion_get_heap_mask_by_attr(NW_SHM_POOL_ATTR, &heap_id_mask);
	if (ret != VPP_MEM_ERROR_TYPE_OK) {
		pr_err("%s: Ion_get_heap_mask_by_attr failed\n", __func__);
		return ret;
	}
	return VPP_MEM_ERROR_TYPE_OK;
}

static int Ion_InitIonMem_NonCached(int index)
{
	int ret;

	ret = Ion_get_heap_mask_by_attr(NC_SHM_POOL_ATTR, &heap_id_mask_nc);
	if (ret < 0) {
		pr_err("%s: Ion_get_heap_mask_by_attr failed\n", __func__);
		return ret;
	}
	return VPP_MEM_ERROR_TYPE_OK;
}

static void Ion_DeinitIonMem(int index)
{
}

static int Ion_AllocateMemory(VPP_MEM *shm_handle, unsigned int heap_id)
{
	struct dma_buf *ion_dma_buf;

	ion_dma_buf = ion_alloc(shm_handle->size, heap_id, ION_FLAG_CACHED);

	if (IS_ERR(ion_dma_buf)) {
		pr_err("%s: ion alloc failed, size=%ld", __func__, shm_handle->size);
		return VPP_MEM_ERROR_TYPE_ENOMEM;
	}

	if (dma_buf_begin_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL) != 0) {
		pr_err("%s: dma_buf_begin_cpu_access failed\n", __func__);
		dma_buf_put(ion_dma_buf);
		return VPP_MEM_ERROR_TYPE_ENOMEM;
	}

	shm_handle->k_addr = DMA_BUF_KERNEL_MAP(ion_dma_buf);
	if (IS_ERR_OR_NULL(shm_handle->k_addr)) {
		pr_err("%s: ion memory mapping failed - %ld\n", __func__,
				PTR_ERR(shm_handle->k_addr));
		goto err;
	}
	shm_handle->p_addr = Ion_ion_dmabuf_get_phy(ion_dma_buf);
	if (shm_handle->p_addr == 0) {
		pr_err("%s: get physical address failed %ld\n", __func__,
				PTR_ERR(shm_handle->p_addr));
		goto err;
	}

	shm_handle->handle = ion_dma_buf;
	return VPP_MEM_ERROR_TYPE_OK;

err:
	if (ion_dma_buf) {
		if (shm_handle->k_addr)
			DMA_BUF_KERNEL_UNMAP(ion_dma_buf, shm_handle->k_addr);
		dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(ion_dma_buf);
	}

	return VPP_MEM_ERROR_TYPE_ENOMEM;
}

/* ION MEMORY Public APIs */
int VPP_ION_IsReady(void)
{
	int ret = VPP_MEM_ERROR_TYPE_OK;
	unsigned int heap_id;
	struct dma_buf *ion_dma_buf;

	ret = Ion_InitIonMem(0);
	if (ret == VPP_MEM_ERROR_TYPE_OK) {
		heap_id = heap_id_mask;
		ion_dma_buf = ion_alloc(shm_handle->size, heap_id, ION_FLAG_CACHED);

		if (IS_ERR(ion_dma_buf)) {
			ret = VPP_MEM_ERROR_TYPE_ENOMEM;
		} else {
			dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
			dma_buf_put(ion_dma_buf);
		}
	}

	return ret;
}

int VPP_ION_AllocMem(VPP_MEM_NODE *shm_node, VPP_MEM *shm_handle, gfp_t flags)
{
	unsigned int heap_id;

	if (shm_node->shm_type == VPP_MEM_TYPE_DMA) {
		heap_id = heap_id_mask;
	} else if (shm_node->shm_type == VPP_MEM_TYPE_NS_NC) {
		heap_id = heap_id_mask_nc;
	} else {
		pr_err("%s: FAILED - Invalid shm type ..!!!\n", __func__);
		return VPP_MEM_ERROR_TYPE_ENOMEM;
	}

	return Ion_AllocateMemory(shm_handle, heap_id_mask);
}

int VPP_ION_InitMemory(VPP_MEM_LIST *shm_list)
{
	int ret;

	ret = Ion_InitIonMem(0);
	if (ret != VPP_MEM_ERROR_TYPE_OK) {
		pr_err("%s: InitIonMem FAILED ..!!!\n", __func__);
		return ret;
	}

	ret = Ion_InitIonMem_NonCached(0);
	if (ret != VPP_MEM_ERROR_TYPE_OK) {
		pr_err("%s: InitIonMem_NonCached FAILED ..!!!\n", __func__);
		return ret;
	}

	return VPP_MEM_ERROR_TYPE_OK;
}

void VPP_ION_DeinitMemory(VPP_MEM_LIST *shm_list)
{
	Ion_DeinitIonMem(0);
}

void VPP_ION_FreeMemory(VPP_MEM *shm_handle)
{
	if (!IS_ERR_OR_NULL(shm_handle->handle)) {
		struct dma_buf *ion_dma_buf = (struct dma_buf *) shm_handle->handle;

		if (!IS_ERR_OR_NULL(shm_handle->k_addr))
			DMA_BUF_KERNEL_UNMAP(ion_dma_buf, shm_handle->k_addr);
		dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(ion_dma_buf);
	}

	shm_handle->handle = NULL;
}


void VPP_MEM_probe(VPP_MEM_LIST *shm_list)
{
	shm_list->ops->VPP_MEM_IsReady = VPP_ION_IsReady;
	shm_list->ops->VPP_MEM_InitMemory = VPP_ION_InitMemory;
	shm_list->ops->VPP_MEM_DeInitMemory = VPP_ION_DeinitMemory;
	shm_list->ops->VPP_MEM_AllocateMemory = VPP_ION_AllocMem;
	shm_list->ops->VPP_MEM_FreeMemory = VPP_ION_FreeMemory;
}
