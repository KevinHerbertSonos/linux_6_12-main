# SPDX-License-Identifier: GPL-2.0-only

#
# kernel module: hrx-v4l2.ko
#

obj-$(CONFIG_SYNAPTICS_HRX_V4L2) := hrx-v4l2.o

AVIO_ABS_PATH := $(srctree)/drivers/synaptics/soc/berlin/modules/avio
AVIO_INC_PATH := -I$(AVIO_ABS_PATH)/include
ifeq ($(CONFIG_AVIO_DOLPHIN),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/fw_inc/dolphin
endif
ifeq ($(CONFIG_AVIO_DOLPHIN_A0),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/fw_inc/dolphina0
endif
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/common
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/dhub/include
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/dhub
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/aio/
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)

MSGQ_INC_PATH := -I$(srctree)/drivers/synaptics/soc/berlin/modules/msgQ
EXTERN_INC_PATH := $(MSGQ_INC_PATH)

EXTERN_INC_PATH += -I$(srctree)/drivers/synaptics/soc/berlin/modules/shm
EXTERN_INC_PATH += -I$(srctree)/drivers/synaptics/soc/berlin/modules/berlin_inc

VPP_ABS_PATH := $(srctree)/drivers/synaptics/soc/berlin/modules/vpp
VPP_INC_PATH += -I$(VPP_ABS_PATH)/include
VPP_MEM_ABS_PATH := $(srctree)/drivers/synaptics/soc/berlin/modules/vpp_mem
VPP_MEM_INC_PATH += -I$(VPP_MEM_ABS_PATH)/include

ALSA_ABS_PATH := $(srctree)/drivers/synaptics/sound/soc/syna
ALSA_INC_PATH += -I$(ALSA_ABS_PATH)/inc

KERNEL_INC_PATH := -I$(srctree)/drivers/synaptics/include

HEADER_PATH +=  $(AVIO_INC_PATH)    \
				$(EXTERN_INC_PATH)  \
				$(VPP_INC_PATH)	\
				$(VPP_MEM_INC_PATH)	\
				$(ALSA_INC_PATH) \
				$(KERNEL_INC_PATH)

ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror -flarge-source-files
CFLAGS_REMOVE_hrx-drv.o += -mgeneral-regs-only

hrx-v4l2-y := hrx-drv.o hrx-video.o hrx-edid.o
hrx-v4l2-y += hrx-reg.o hrx-phy.o hrx-hdmi.o hrx-vip.o
hrx-v4l2-y += hrx-vip-framequeue.o hrx-vip-bcmbuf.o
hrx-v4l2-y += hrx-isr.o hrx-vip-isr.o hrx-dmaheap.o
hrx-v4l2-y += hrx-vip-scl.o
hrx-v4l2-y += hrx-audio.o hrx-aip.o hrx-aip-buffer.o
hrx-v4l2-y += hrx-aip-fifo-cmd.o hrx-aip-isr.o
hrx-v4l2-y += hrx-aip-hal.o hrx-aip-i2s.o hrx-aip-alsa-if.o
hrx-v4l2-y += hrx-debug.o hrx-sig-stat-notify.o
