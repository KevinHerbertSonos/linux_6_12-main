INC_PATH = -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/tee/client_api \
           -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/tee/common \
           -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/include \
           -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd \
           -I$(srctree)/drivers/synaptics/soc/berlin/modules/tzd/tz/common
ccflags-y += -Werror
ccflags-y += $(INC_PATH)
obj-$(CONFIG_MEDIA_DRM) := media_drm.o
