#
# for USB OTG silicon based on Mentor Graphics INVENTRA designs
#

# define_trace.h needs to know how to find our header
CFLAGS_musb_trace.o    := -I$(src)

obj-$(CONFIG_USB_MUSB_FDRC) += musb_fdrc.o
musb_fdrc-y := musb_core.o musb_trace.o
musb_fdrc-$(CONFIG_USB_GADGET)			+= musb_gadget_ep0.o musb_gadget.o
musb_fdrc-y					+= musb_virthub.o musb_host.o
musb_fdrc-$(CONFIG_DEBUG_FS)			+= musb_debugfs.o

# Hardware Glue Layer
obj-$(CONFIG_USB_MUSB_DVF99)	+= dvf99_musb_hal.o
# the kconfig must guarantee that only one of the
# possible I/O schemes will be enabled at a time ...
# PIO only, or DMA (several potential schemes).
# though PIO is always there to back up DMA, and for ep0

musb_fdrc-$(CONFIG_USB_MUSB_FDRC_INVENTRA_DMA)	+= musbhsdma.o
musb_fdrc-$(CONFIG_USB_TI_CPPI_DMA)		+= cppi_dma.o
