#
# Makefile for Vivante GPU driver

debug     = 0
profiler  = 0
newsignal = 0

ifeq ($(CONFIG_GPU_VIVANTE_DEBUG),y)
  debug = 1
endif

ifeq ($(CONFIG_GPU_VIVANTE_PROFILER),y)
  profiler = 1
endif

ifeq ($(CONFIG_GPU_VIVANTE_NEWSIGNAL),y)
  newsignal = 1
endif

#
# driver options
#
ccflags-y += -DENUM_WORKAROUND=0
#ccflags-y += -DFLARE_ON

ccflags-y                           += -DDBG=$(debug)
ccflags-$(CONFIG_GPU_VIVANTE_DEBUG) += -DDEBUG -D_DEBUG

#ccflags-y += -DNO_DMA_COHERENT
ccflags-y += -DENABLE_ARM_L2_CACHE=0
#ccflags-y += -DCONFIG_DOVE_GPU=1
ccflags-y += -DgcdNO_POWER_MANAGEMENT=0
ccflags-y += -DUSE_PLATFORM_DRIVER=1
ccflags-y += -DVIVANTE_PROFILER=$(profiler)
ccflags-y += -DENABLE_GPU_CLOCK_BY_DRIVER=1
ccflags-y += -DUSE_NEW_LINUX_SIGNAL=$(newsignal)
ccflags-y += -DNO_USER_DIRECT_ACCESS_FROM_KERNEL=1
ccflags-y += -DgcdkREPORT_VIDMEM_USAGE=0
ccflags-y += -DgcdCACHE_FUNCTION_UNIMPLEMENTED=0
ccflags-y += -DgcdENABLE_VG=0
#ccflags-y += -DVIVANTE_NO_3D

#
# driver includes
#
ccflags-y += \
	-I$(srctree)/drivers/gpu/vivante/hal/inc \
	-I$(srctree)/drivers/gpu/vivante/hal/kernel \
	-I$(srctree)/drivers/gpu/vivante/hal/kernel/inc \
	-I$(srctree)/drivers/gpu/vivante/hal/os/linux/kernel/ \
	-I$(srctree)/drivers/gpu/vivante/hal/user \
	-I$(srctree)/drivers/gpu/vivante/arch/XAQ2/hal/kernel

#
# driver module name
#

#
# driver objects
#l
obj-$(CONFIG_GPU_VIVANTE) += galcore.o
galcore-objs +=\
	hal/os/linux/kernel/gc_hal_kernel_device.o\
	hal/os/linux/kernel/gc_hal_kernel_driver.o\
	 hal/os/linux/kernel/gc_hal_kernel_linux.o\
	hal/os/linux/kernel/gc_hal_kernel_math.o\
	hal/os/linux/kernel/gc_hal_kernel_os.o\
	 hal/kernel/gc_hal_kernel.o \
	hal/kernel/gc_hal_kernel_command.o\
	hal/kernel/gc_hal_kernel_db.o\
	 hal/kernel/gc_hal_kernel_debug.o\
	hal/kernel/gc_hal_kernel_event.o\
	 hal/kernel/gc_hal_kernel_heap.o\
	 hal/kernel/gc_hal_kernel_mmu.o\
	 hal/kernel/gc_hal_kernel_video_memory.o \
	arch/XAQ2/hal/kernel/gc_hal_kernel_context.o \
	arch/XAQ2/hal/kernel/gc_hal_kernel_hardware.o
