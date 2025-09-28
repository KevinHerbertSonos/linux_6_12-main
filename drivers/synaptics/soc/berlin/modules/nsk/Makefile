NSK_ABS_PATH := $(srctree)/drivers/synaptics/soc/berlin/modules/nsk

ccflags-y += 	-Werror
ccflags-y +=	-I$(srctree)/drivers/synaptics/include
nsk-objs += 	nsk_driver.o

obj-$(CONFIG_BERLIN_NSK)	:=	nsk.o
