PWD := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

vpp_mem-y := vpp_mem_core.o

ifeq ($(CONFIG_ION), y)
vpp_mem-y += vpp_mem_ion.o
HEADER_PATH  += -I$(srctree)/drivers/staging/android/
else
ifeq ($(CONFIG_DMABUF_HEAPS), y)
vpp_mem-y += vpp_mem_dmabuf.o
else
vpp_mem-y += vpp_mem_kmem.o
endif
endif

obj-$(CONFIG_BERLIN_VPP_MEM) += vpp_mem.o

HEADER_PATH  += -I$(PWD)include/ \
		-I$(srctree)/drivers/synaptics/include/

ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror

