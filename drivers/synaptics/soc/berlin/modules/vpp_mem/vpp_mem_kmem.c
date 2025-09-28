// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated
 *
 */

#include "vpp_mem.h"

/* Kernel MEMORY Public APIs */
int VPP_KMEM_IsReady(void)
{
	return VPP_MEM_ERROR_TYPE_OK;
}

int VPP_KMEM_AllocMem(VPP_MEM_NODE *shm_node, VPP_MEM *shm_handle, gfp_t flags)
{
	void *alloc_addr;

	if (shm_node->shm_type == VPP_MEM_TYPE_DMA) {
		flags &= ~GFP_DMA32;
		flags |= GFP_DMA;

	} else if (shm_node->shm_type == VPP_MEM_TYPE_NS_NC) {
		flags |= GFP_HIGHUSER;
	} else {
		flags |= GFP_KERNEL;
	}

	alloc_addr = (void *)__get_free_pages(flags,
			get_order(roundup(shm_handle->size, PAGE_SIZE)));
	if (unlikely(!alloc_addr)) {
		pr_err("%s: get free pages failed\n", __func__);
		return VPP_MEM_ERROR_TYPE_ENOMEM;
	}

	shm_handle->k_addr = alloc_addr;
	shm_handle->u_addr = NULL;
	shm_handle->p_addr = (void *)virt_to_phys(alloc_addr);

	return VPP_MEM_ERROR_TYPE_OK;
}

int VPP_KMEM_InitMemory(VPP_MEM_LIST *shm_list)
{
	return VPP_MEM_ERROR_TYPE_OK;
}

void VPP_KMEM_DeinitMemory(VPP_MEM_LIST *shm_list)
{
}

void VPP_KMEM_FreeMemory(VPP_MEM *shm_handle)
{
	if (likely(shm_handle && shm_handle->k_addr))
		free_pages((unsigned long)shm_handle->k_addr,
			get_order(roundup(shm_handle->size, PAGE_SIZE)));
}

void VPP_MEM_probe(VPP_MEM_LIST *shm_list)
{
	shm_list->ops->VPP_MEM_IsReady = VPP_KMEM_IsReady;
	shm_list->ops->VPP_MEM_InitMemory = VPP_KMEM_InitMemory;
	shm_list->ops->VPP_MEM_DeInitMemory = VPP_KMEM_DeinitMemory;
	shm_list->ops->VPP_MEM_AllocateMemory = VPP_KMEM_AllocMem;
	shm_list->ops->VPP_MEM_FreeMemory = VPP_KMEM_FreeMemory;
}
