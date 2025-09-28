obj-$(CONFIG_BERLIN_BM) := berlin_bm.o
berlin_bm-objs += bm.o

ifeq ($(CONFIG_OPTEE),y)
berlin_bm-objs += ptm_wrapper_optee.o
else
INC_PATH = -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/tee/client_api \
           -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/tee/common \
           -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/include \
           -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd \
           -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/tz/common
berlin_bm-objs += ptm_wrapper_tzd.o
endif

INC_PATH += -I$(srctree)/drivers/synaptics/include/

ccflags-y += -Werror
ccflags-y += $(INC_PATH)
