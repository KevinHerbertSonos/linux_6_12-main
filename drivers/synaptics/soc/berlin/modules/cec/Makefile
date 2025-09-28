HEADER_PATH +=  \
    -I$(srctree)/drivers/synaptics/soc/berlin/modules/msgQ \
    -I$(srctree)/drivers/synaptics/include
ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror
obj-$(CONFIG_BERLIN_CEC) += berlin-cec.o
