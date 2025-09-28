PWD := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BERLIN_INC := $(srctree)/drivers/synaptics/soc/berlin/modules/
ION_INC := $(srctree)/drivers/staging/android/
SYNA_VPP_TYPE :=

ifneq ($(filter y, $(CONFIG_AVIO_PLATYPUS) $(CONFIG_AVIO_DOLPHIN) $(CONFIG_AVIO_DOLPHIN_A0)), )
    SYNA_VPP_TYPE = vsxxx
else ifneq ($(filter y, $(CONFIG_AVIO_MYNA2) $(CONFIG_AVIO_MYNA)), )
    SYNA_VPP_TYPE = asxxx
endif

###############################################################################
#
# Common VPP module for FB and DRM
#
###############################################################################
obj-$(CONFIG_BERLIN_VPP) := vpp.o

vpp-objs += source/vpp.o       \
          source/vpp_isr.o      \
          source/vpp_fb.o       \
          source/vpp_api.o

ifeq ($(SYNA_VPP_TYPE), vsxxx)
vpp-objs += source/$(SYNA_VPP_TYPE)/vpp_api_priv.o
vpp-objs += source/$(SYNA_VPP_TYPE)/vpp_res_info.o
endif

###############################################################################
#
# Common AVIOUTIL kernel module objects
#
###############################################################################
AVIO_INC_PATH := -I$(BERLIN_INC)/avio/include
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/vpp/ca/include
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/common
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/dhub
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/dhub/include
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/fastcall/include
AVIO_INC_PATH += -I$(BERLIN_INC)/vpp_mem/include

###############################################################################
#
# Chip specific section
#
###############################################################################
ifneq ($(filter y, $(CONFIG_AVIO_PLATYPUS) $(CONFIG_AVIO_DOLPHIN) $(CONFIG_AVIO_DOLPHIN_A0)), )
ifeq ($(CONFIG_AVIO_PLATYPUS), y)
ccflags-y += -DUSE_PLATYPUS=1
FIRMWARE_INC_DIR_LOCAL=  $(BERLIN_INC)/avio/fw_inc/platypus/
else ifeq ($(CONFIG_AVIO_DOLPHIN), y)
ccflags-y += -DUSE_DOLPHIN=1
FIRMWARE_INC_DIR_LOCAL=  $(BERLIN_INC)/avio/fw_inc/dolphin/
else ifeq ($(CONFIG_AVIO_DOLPHIN_A0), y)
ccflags-y += -DUSE_DOLPHIN=1
FIRMWARE_INC_DIR_LOCAL=  $(BERLIN_INC)/avio/fw_inc/dolphina0/
endif
HEADER_PATH  += -I$(PWD)include/configs/chip-vsxxx/
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/vpp/ca/include/vsxxx
endif

ifneq ($(filter y, $(CONFIG_AVIO_MYNA2)), )
FIRMWARE_INC_DIR_LOCAL=  $(BERLIN_INC)/avio/fw_inc/myna2/
HEADER_PATH  += -I$(PWD)include/configs/chip-myna/
HEADER_PATH  += -I$(BERLIN_INC)/lcdc/include/
HEADER_PATH  += -I$(BERLIN_INC)/bcmbuf/include/
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/vpp/ca/include/asxxx
ccflags-y += -DUSE_MYNA=1
endif

###############################################################################
HEADER_PATH += -I$(BERLIN_INC)/tzd/include \
               -I$(BERLIN_INC)/tzd \
               -I$(BERLIN_INC)/tzd/tz/common

HEADER_PATH +=  \
              -I$(FIRMWARE_INC_DIR_LOCAL)  \
               $(AVIO_INC_PATH)\
              -I$(PWD)include/common/ \
              -I$(srctree)/drivers/synaptics/soc/berlin/modules/msgQ

HEADER_PATH  += -I$(PWD)include/
HEADER_PATH  += -I$(ION_INC)	\
		-I$(srctree)/drivers/synaptics/include/

ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror
