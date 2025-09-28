/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ISPSS_SHM_H__
#define __ISPSS_SHM_H__

#include "com_type.h"
#define SHM_NONSECURE	0

typedef UINT64 SHM_HANDLE;

enum memory_type_t {
	SHM_NONSECURE_CONTIG  = 0,
	SHM_NONSECURE_NON_CONTIG,
	SHM_MAX_MEMORY_TYPE,
};

int ispSS_SHM_Allocate(unsigned int  eMemType,
		unsigned int uiSize, unsigned int uiAlign, SHM_HANDLE *phShm,
		enum memory_type_t mem_type);
int ispSS_SHM_Release(SHM_HANDLE phShm);
int ispSS_SHM_CleanCache(SHM_HANDLE phShm, unsigned int offset, unsigned int size);
int ispSS_SHM_InvalidCache(SHM_HANDLE phShm);
int ispSS_SHM_GetVirtualAddress(SHM_HANDLE phShm, int uiOffset, void **pVirtAddr);
int ispSS_SHM_GetPhysicalAddress(SHM_HANDLE phShm, int uiOffset, void **pPhyAddr);
int ispSS_SHM_GetPageTableAddress(SHM_HANDLE phShm, void **pPhyAddr);
int ispSS_SHM_Init(struct device *dev);
int ispSS_SHM_Deinit(struct device *dev);
int ispSS_SHM_mmap(SHM_HANDLE phShm, struct vm_area_struct *vma);

#endif
