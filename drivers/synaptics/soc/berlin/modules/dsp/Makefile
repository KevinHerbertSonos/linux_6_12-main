INC_PATH = -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/tee/client_api \
           -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/tee/common \
           -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/include
HEADER_PATH += \
	-I$(srctree)/drivers/synaptics/soc/berlin/modules/msgQ
ccflags-y += $(INC_PATH)
ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror

obj-$(CONFIG_HIFI_DSP) += dsp.o
