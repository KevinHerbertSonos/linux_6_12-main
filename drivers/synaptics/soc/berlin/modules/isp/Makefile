obj-$(CONFIG_SYNA_ISP) += isp.o
obj-$(CONFIG_SYNA_ISP_DMA_HEAP) += isp_dma_heap.o
obj-$(CONFIG_SYNA_ISP_BE) += isp_be.o
obj-$(CONFIG_SYNA_ISP_SCALER) += scaler.o
obj-$(CONFIG_VIDEO_VVCAM_DRIVER) += vvcam_isp.o
obj-$(CONFIG_VIDEO_VVCAM_DRIVER) += vvcam_vb.o
obj-$(CONFIG_VIDEO_VVCAM_DRIVER) += vvcam_video.o
obj-$(CONFIG_VIDEO_VVCAM_DRIVER) += vvcam_isp_subdev.o
obj-$(CONFIG_VIDEO_VVCAM_DRIVER) += mcm_subdev.o

ISP_ABS_PATH := $(srctree)/drivers/synaptics/soc/berlin/modules/isp

include $(ISP_ABS_PATH)/isp/vvcam/Param.mk
include $(ISP_ABS_PATH)/isp/vvcam/isp_subdev.mk

ifeq ($(CONFIG_ISP_DOLPHIN),y)
	ccflags-y += -D DOLPHIN
endif

ifeq ($(CONFIG_SYNA_ISP_SCALER), m)
	ccflags-y += -DSYNA_ISP_BE_A0
endif

HEADER_PATH +=  $(ISP_INC_PATH)    \
				-I$(ISP_ABS_PATH)/ \
				-I$(ISP_ABS_PATH)/isp/drv \
				-I$(ISP_ABS_PATH)/isp/drv/dhub \
				-I$(ISP_ABS_PATH)/fw \
				-I$(srctree)/drivers/synaptics/soc/berlin/modules/msgQ \
				-I$(srctree)/drivers/synaptics/include

ccflags-y += $(HEADER_PATH)

isp-objs += isp/drv/isp_driver.o \
	isp/drv/drv_isp.o \
	isp/drv/dhub/drv_ispdhub.o \
	isp/drv/dhub/ispdhub_sema.o \
	isp/drv/dhub/configs/dolphin/isp_dhub_cfg.o

HEADER_PATH +=  $(ISP_INC_PATH)    \
				-I$(ISP_ABS_PATH)/be/drv/src/scl \
				-I$(ISP_ABS_PATH)/be/drv/src/isp \
				-I$(ISP_ABS_PATH)/be/drv/src/common \
				-I$(ISP_ABS_PATH)/be/drv/inc \
				-I$(ISP_ABS_PATH)/be/v4l2/memops \
				-I$(ISP_ABS_PATH)/be/v4l2/scaler

isp_be-objs += be/drv/src/intf/ispbe_api.o \
	be/drv/src/common/ispSS_shm.o \
	be/drv/src/common/ispSS_fe_client.o \
	be/drv/src/common/ispSS_bcmbuf.o \
	be/drv/src/common/ispSS_bcmbuf_debug.o \
	be/drv/src/common/ispSS_bcmbuf_queue.o \
	be/drv/src/common/ispSS_clkrst.o \
	be/drv/src/common/ispSS_api_dhub_config.o \
	be/drv/src/common/ispSS_be_Mtr.o \
	be/drv/src/common/ispSS_dhub_drv.o \
	be/drv/src/common/ispSS_rqstq.o \
	be/drv/src/common/ispSS_reg.o \
	be/drv/src/common/hal_dhub.o \
	be/drv/src/isp/mtr_isp_wrap.o \
	be/drv/src/isp/mtr_isp.o \
	be/drv/src/isp/isp_bcm.o \
	be/drv/src/common/isp_be_driver.o

isp_dma_heap-objs += be/v4l2/memops/isp_dma_heap.o

scaler-objs += be/v4l2/scaler/m2m_scaler.o \
	be/v4l2/scaler/scaler_dbg.o \
	be/v4l2/scaler/scaler_drv_api.o \
	be/drv/src/scl/ispSS_be_frc_scl.o \
	be/drv/src/scl/ispSS_be_scl_api.o \
	be/drv/src/scl/ispSS_be_scl_apifuncs.o

HEADER_PATH += -I$(ISP_ABS_PATH)/isp/vvcam/common \
			   -I$(ISP_ABS_PATH)/isp/vvcam/v4l2/isp/isp_ctrl \
			   -I$(ISP_ABS_PATH)/isp/vvcam/v4l2/isp \
			   -I$(ISP_ABS_PATH)/isp/vvcam/v4l2/common/ \
			   -I$(ISP_ABS_PATH)/be/v4lsubdev

ccflags-y += $(HEADER_PATH)

vvcam_isp-objs += isp/vvcam/isp/vvcam_isp_driver.o \
	isp/vvcam/isp/vvcam_isp_hal.o \
	isp/vvcam/isp/vvcam_isp_procfs.o \
	isp/vvcam/common/vvcam_event.o

vvcam_vb-objs += isp/vvcam/vb/vvcam_vb_driver.o \
	isp/vvcam/vb/vvcam_mmz.o \
	isp/vvcam/vb/vvcam_cma.o \
	isp/vvcam/vb/vvcam_vb_procfs.o

vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/vvcam_isp_driver.o \
	isp/vvcam/v4l2/isp/vvcam_isp_event.o \
	isp/vvcam/v4l2/isp/vvcam_isp_procfs.o \
	isp/vvcam/v4l2/isp/isp_ctrl/vvcam_isp_ctrl.o

vvcam_video-objs += isp/vvcam/v4l2/video/vvcam_video_driver.o \
	isp/vvcam/v4l2/video/vvcam_video_register.o \
	isp/vvcam/v4l2/video/vvcam_video_m2m_register.o \
	isp/vvcam/v4l2/video/vvcam_video_event.o

mcm_subdev-objs += be/v4lsubdev/mcm_subdev.o \
	be/v4lsubdev/mcm_event.o
