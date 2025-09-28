# SPDX-License-Identifier: GPL-2.0
# Makefile for the Linux video drivers.
# 5 Aug 1999, James Simmons, <mailto:jsimmons@users.sf.net>
# Rewritten to use lists instead of if-statements.

# Each configuration option enables a list of files.
obj-$(CONFIG_FB_DSPG)             += dspg_fb.o
obj-$(CONFIG_FB_ST7528)		  += st7528.o
obj-$(CONFIG_FB_MPMC)		  += mpmc.o


ifeq ($(CONFIG_KERNEL_MODE_NEON),y)
  NEON_FLAGS			:= -mfloat-abi=softfp -mfpu=neon
  AFLAGS_rot_neon.o		+= $(NEON_FLAGS)
  dspg_fb-y			:= dspgfb.o rot_neon.o
else
  dspg_fb-y			:= dspgfb.o
endif
ifeq ($(CONFIG_KERNEL_MODE_NEON),y)
  NEON_FLAGS			:= -mfloat-abi=softfp -mfpu=neon
  AFLAGS_rot_neon.o		+= $(NEON_FLAGS)
  dspg_fb-y			:= dspgfb.o rot_neon.o
else
  dspg_fb-y			:= dspgfb.o
endif