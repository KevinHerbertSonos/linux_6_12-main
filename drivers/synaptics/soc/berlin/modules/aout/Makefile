AVIO_ABS_PATH := $(srctree)/drivers/synaptics/soc/berlin/modules/avio
AVIO_INC_PATH := -I$(AVIO_ABS_PATH)/include
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/common -I$(AVIO_ABS_PATH)/vpp
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/dhub/include
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/dhub
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/common/regmap
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)

ifeq ($(CONFIG_AVIO_DOLPHIN),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/fw_inc/dolphin
else ifeq ($(CONFIG_AVIO_PLATYPUS),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/fw_inc/platypus
else ifeq ($(CONFIG_AVIO_DOLPHIN_A0),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/fw_inc/dolphina0
endif

AVIO_CFG_PATH += -I$(AVIO_ABS_PATH)/aio

MSGQ_INC_PATH := -I$(srctree)/drivers/synaptics/soc/berlin/modules/msgQ
EXTERN_INC_PATH := $(MSGQ_INC_PATH)

EXTERN_INC_PATH += -I$(srctree)/drivers/synaptics/soc/berlin/modules/shm \
		   -I$(srctree)/drivers/synaptics/include


AOUT_ALL_OBJS = drv_aout.o

aout-objs += $(AOUT_ALL_OBJS) \

HEADER_PATH +=  $(AVIO_INC_PATH)    \
                    $(EXTERN_INC_PATH)  \
                    $(AVIO_CFG_PATH)    \


ccflags-y += $(MV_DEFINE) $(HEADER_PATH)
ccflags-y += -Werror

obj-$(CONFIG_BERLIN_AOUT) += aout.o
