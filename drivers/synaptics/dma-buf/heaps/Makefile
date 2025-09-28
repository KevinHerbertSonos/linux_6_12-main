# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_DMABUF_HEAPS_CMA_CUST)	+= cma_cust_heap.o
obj-$(CONFIG_DMABUF_HEAPS_SYSTEM_CUST)	+= system_cust_heap.o
obj-$(CONFIG_DMABUF_HEAPS_BERLIN)	+= berlin_heap.o
obj-$(CONFIG_DMABUF_HEAPS_EXTRA)	+= heap_extra.o

INC_PATH = -I$(srctree)/drivers/synaptics/include/

ccflags-y += -Werror
ccflags-y += $(INC_PATH)
