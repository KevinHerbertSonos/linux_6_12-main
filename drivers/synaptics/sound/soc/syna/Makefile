# Berlin Platform Support

ccflags-y += -I$(srctree)/drivers/synaptics/sound/soc/syna/inc
ccflags-y += -Werror

ccflags-y += -I$(srctree)/drivers/synaptics/include/
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/include
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/common
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/dhub
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/dhub/include
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/aio
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/vpp
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/vpp/include/
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/vpp_mem/include/
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/vpp/ca/include
ifeq ($(CONFIG_AVIO_DOLPHIN),y)
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/fw_inc/dolphin
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/aio/vsxxx/dolphin
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/vpp/ca/include/vsxxx
endif
ifeq ($(CONFIG_AVIO_PLATYPUS),y)
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/fw_inc/platypus
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/aio/vsxxx/platypus
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/vpp/ca/include/vsxxx
else ifeq ($(CONFIG_AVIO_DOLPHIN_A0),y)
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/fw_inc/dolphina0
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/aio/vsxxx/dolphin_a0
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/vpp/ca/include/vsxxx
endif
ifeq ($(CONFIG_AVIO_MYNA),y)
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/fw_inc/myna
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/aio/myna
endif
ifeq ($(CONFIG_AVIO_MYNA2),y)
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/fw_inc/myna2
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/aio/myna2
endif
ifeq ($(CONFIG_AVIO_AS370),y)
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/fw_inc/as370
ccflags-y += -I$(srctree)/drivers/synaptics/soc/berlin/modules/avio/aio/as370
endif

snd-soc-berlin-asoc-objs   += berlin_asoc.o
snd-soc-berlin-pcm-objs    += berlin_pcm.o \
			      berlin_capture.o \
			      berlin_util.o \
			      berlin_playback.o \
			      berlin_spdif.o
snd-soc-berlin-dmic-objs   += dmic.o
snd-soc-berlin-pri-objs    += i2s_pri.o
snd-soc-berlin-i2s-dai-objs   += i2s_dai.o aud_clk.o
snd-soc-berlin-hdmi-outdai-objs += hdmi_outdai.o
snd-soc-berlin-hdmi-indai-objs += hdmi_indai.o
snd-soc-berlin-sec-objs    += i2s_sec.o
snd-soc-berlin-spdifo-objs += spdif_outdai.o
snd-soc-berlin-pdmi-objs   += pdm_pdmi.o
snd-soc-berlin-mic1-objs   += i2s_mic1.o
snd-soc-berlin-mic2-objs   += i2s_mic2.o
snd-soc-berlin-mic6-objs   += i2s_mic6.o
snd-soc-berlin-spdifi-objs += spdif_spdifi.o
snd-soc-berlin-pri-loopback-objs += i2s_pri_loopback.o
snd-soc-berlin-hdmi-loopback-objs += i2s_hdmi_loopback.o
snd-soc-ptrack-objs += ptrack.o
snd-soc-drt-objs += drt.o
snd-soc-dclass-objs += dclass.o

obj-$(CONFIG_SND_SOC_BERLIN_PCM)    += snd-soc-berlin-pcm.o
obj-$(CONFIG_SND_SOC_BERLIN_DMIC)   += snd-soc-berlin-dmic.o
obj-$(CONFIG_SND_SOC_BERLIN_PRI)    += snd-soc-berlin-pri.o
obj-$(CONFIG_SND_SOC_BERLIN_I2S_DAI) += snd-soc-berlin-i2s-dai.o
obj-$(CONFIG_SND_SOC_BERLIN_SPDIFO) += snd-soc-berlin-spdifo.o
obj-$(CONFIG_SND_SOC_BERLIN_HDMI)   += snd-soc-berlin-hdmi-outdai.o
obj-$(CONFIG_SND_SOC_BERLIN_HDMI_RX)   += snd-soc-berlin-hdmi-indai.o
obj-$(CONFIG_SND_SOC_BERLIN_SEC)    += snd-soc-berlin-sec.o
obj-$(CONFIG_SND_SOC_BERLIN_PDMI)   += snd-soc-berlin-pdmi.o
obj-$(CONFIG_SND_SOC_BERLIN_MIC1)   += snd-soc-berlin-mic1.o
obj-$(CONFIG_SND_SOC_BERLIN_MIC2)   += snd-soc-berlin-mic2.o
obj-$(CONFIG_SND_SOC_BERLIN_MIC6)   += snd-soc-berlin-mic6.o
obj-$(CONFIG_SND_SOC_BERLIN_SPDIFI) += snd-soc-berlin-spdifi.o
obj-$(CONFIG_SND_SOC_BERLIN_PRI_LOOPBACK)  += snd-soc-berlin-pri-loopback.o
obj-$(CONFIG_SND_SOC_BERLIN_HDMI_LOOPBACK) += snd-soc-berlin-hdmi-loopback.o
obj-$(CONFIG_SND_SOC_BERLIN_ASOC)   += snd-soc-berlin-asoc.o
obj-$(CONFIG_SND_SOC_PTRACK)        += snd-soc-ptrack.o
obj-$(CONFIG_SND_SOC_DRT)           += snd-soc-drt.o
obj-$(CONFIG_SND_SOC_DCLASS)        += snd-soc-dclass.o
