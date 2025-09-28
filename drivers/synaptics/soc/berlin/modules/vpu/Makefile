obj-$(CONFIG_BERLIN_VPU)	+= vpu.o

vpu-y				:= vpu_mfd.o
vpu-$(CONFIG_BERLIN_VPU_AMP)	+= v1/vpu.o
vpu-$(CONFIG_BERLIN_VPU_V4L2)	+= v2/vpu_drv.o \
				   v2/vpu_srv.o \
				   v2/vpu_dmaheap.o \
				   v2/vpu_enc_drv.o \
				   v2/vpu_dec_drv.o \
				   v2/vpu_enc_ctrls.o

ccflags-y += -I$(srctree)/drivers/synaptics/include
ccflags-y += -Werror

ifneq ($(filter y m,$(CONFIG_OPTEE)),)
vpu-$(CONFIG_BERLIN_VPU_V4L2)	+= v2/vpu_fw_optee.o \
				   v2/h1_irq.o
else
vpu-$(CONFIG_BERLIN_VPU_V4L2)	+= v2/vpu_fw_tzk.o
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/tee/client_api \
	     -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/tee/common \
	     -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/include
endif
