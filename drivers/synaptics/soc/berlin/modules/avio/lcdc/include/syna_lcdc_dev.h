// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <prem.anand@synaptics.com>
 *
 */
#include "avio_type.h"
#include "framequeue.h"
#include "bcmbuf.h"
#include "avioDhub.h"
#include "vpp_vbuf.h"
#include "syna_lcdc_reg.h"
#include "vpp_mem.h"

#define SYNA_LCDC_INTF_TYPE_DPI    (SYNA_LCDC_TYPE_DPI_MCU | SYNA_LCDC_TYPE_DPI_RGB)
#define SYNA_LCDC_INTF_TYPE_DSI    (SYNA_LCDC_TYPE_DSI_CMD)

#define BCM_BUF_COUNT        0x2
#define BCM_BUFFER_SIZE        (4 * 1024)
#define DMA_CMD_BUFFER_SIZE    (200)

#define INT_FRAME_START 0x1
#define INT_FRAME_DONE  0x2
#define INT_UNDERRUN    0x4

#define SYNA_MEMMAP_AVIO_GBL_BASE    (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE)

#define LCDC_CMD_SIZE       0xA
#define DSI_CMD_MODE_POS    14
#define DSI_WAIT_TE_POS     15

#define LCDC_DSI_CMD_MODE    (1 << DSI_CMD_MODE_POS)
#define LCDC_WAIT_FOR_TE    (1 << DSI_WAIT_TE_POS)

#define CURR_VBI_BCM_BUF    (&dev->bcmbuf[dev->bufferCurSet].vbi_bcm_buf)
#define CURR_VBI_DMA_CFGQ   (&(dev->bcmbuf[dev->bufferCurSet].vbi_cfgQ[SYNA_DHUB_CFGQ_TYPE_DMA]))
#define CURR_VBI_BCM_CFGQ   (&(dev->bcmbuf[dev->bufferCurSet].vbi_cfgQ[SYNA_DHUB_CFGQ_TYPE_BCM]))

typedef enum SYNA_LCDC_ERROR_t {
	SYNA_LCDC_OK            = 0x0000,   /**< Success. */
	SYNA_LCDC_EBADPARAM     = 0x0001,   /**< Function parameter error. */
	SYNA_LCDC_ENOMEM        = 0x0002,   /**< Not enough memory. */
	SYNA_LCDC_EUNSUPPORT    = 0x0003,   /**< plane not connected in configuration*/
	SYNA_LCDC_ECMDQFULL     = 0x0004,   /**< Command Queue is full */
	SYNA_LCDC_EFRAMEQFULL   = 0x0005,   /**< Frame Queue is full */
	SYNA_LCDC_EBCMBUFFULL   = 0x0006,   /**< BCM Buffer is full */
	SYNA_LCDC_EVBIBUFFULL   = 0x0007   /**< VBI Buffer is full */
} SYNA_LCDC_ERROR;

enum syna_lcdc_pix_fmt {
	SYNA_LCDC_PIXFMT_ARGB32,
	SYNA_LCDC_PIXFMT_ARGB32_PM,
	SYNA_LCDC_PIXFMT_RGB565,
	SYNA_LCDC_PIXFMT_RGB888,
	SYNA_LCDC_PIXFMT_MAX
};

enum syna_lcdc_pix_order {
	SYNA_LCDC_PIXORDER_XRGB = 0,
	SYNA_LCDC_PIXORDER_XBGR = 1,
	SYNA_LCDC_PIXORDER_RGBX = 2,
	SYNA_LCDC_PIXORDER_BGRX = 3,
	SYNA_LCDC_PIXORDER_MAX
};

typedef enum syna_lcdc_interface_t_ {
	SYNA_LCDC_TYPE_DPI_RGB   = 0x1, /*TFT + DSI_VIDEO*/
	SYNA_LCDC_TYPE_DPI_MCU   = 0x2,
	SYNA_LCDC_TYPE_DSI_CMD   = 0x4
} syna_lcdc_interface_t;

typedef enum syna_lcdc_output_mode_t_ {
	SYNA_LCDC_MODE_0    = 0x0,
	SYNA_LCDC_MODE_1    = 0x1,
	SYNA_LCDC_MODE_2    = 0x2,
	SYNA_LCDC_MODE_3    = 0x3
} syna_lcdc_output_mode_t;

enum SYNA_LCDC_NUM {
	SYNA_LCDC_1 = 0,
	SYNA_LCDC_2 = 1,
	SYNA_LCDC_MAX = 2
};

typedef enum _SYNA_DHUB_CFGQ_TYPE_ {
	SYNA_DHUB_CFGQ_TYPE_DMA = 0x0,
	SYNA_DHUB_CFGQ_TYPE_BCM = 0x1,
	SYNA_DHUB_CFGQ_TYPE_MAX = 0x2
} SYNA_DHUB_CFGQ_TYPE;

typedef struct syna_lcdc_panel_t {
	unsigned char intf_type;     /*DPI TFT or CPU, DSI Video or CMD*/
	unsigned int bits_per_pixel; /*16, 18 or 24-bits per pixel*/
	unsigned char mode;      /*refer lcdc_output.xlsx*/
	unsigned char ext_te;    /*External tearing Effect*/
	unsigned int te_delay;
	unsigned char iclk;      /*lpclock polarity*/
	unsigned char rgb_swap;
	unsigned char rotation;      /*0, 90, 180, 270*/

	/*htotal = hsync_len + left_margin + xres + right_margin*/
	unsigned int hsync_len;
	unsigned int right_margin; //FP
	unsigned int xres;
	unsigned int left_margin;  //BP

	/*vtotal = vsync_len + upper_margin + yres + lower_margin*/
	unsigned int vsync_len;
	unsigned int lower_margin; //FP
	unsigned int yres;
	unsigned int upper_margin; //BP

	unsigned int refresh;
	/*htotal * vtotal * refresh*/
	unsigned int pixclock;

	/*htotal + hskip*/
	unsigned int hskip;
	/*vtotal + vskip*/
	unsigned int vskip;
} SYNA_LCDC_PANEL;

typedef struct _SYNA_BCMBUF_DATA_T_ {
	BCMBUF    vbi_bcm_buf;
	DHUB_CFGQ vbi_cfgQ[SYNA_DHUB_CFGQ_TYPE_MAX];   //O : DMA, 1: BCM
	VPP_MEM   vpp_mem_handle[SYNA_DHUB_CFGQ_TYPE_MAX];
} SYNA_BCMBUF_DATA;

struct syna_lcdc_dev {
	unsigned char lcdcID;     /*to identify the LCDC1 and LCDC2*/
	unsigned char bcm_enable; /* Use BCM engine or not for programming registers */
	unsigned char bcm_autopush_en;
	unsigned char en_intr_handler;
	unsigned int core_addr;
	struct syna_lcdc_panel_t *panel;

	VPP_VBUF *curr_frame; /*Slot for new frame*/
	VPP_VBUF *prev_frame;  /*Slot for current display frame*/
	int dmaRID;      /*DMA Read channel ID*/
	int dhubID;      /*DHUB channel handle*/
	int intrID;      /*Main interrupt*/
	int intrNo;      /*GIC interrupt number*/

	unsigned int interrupts;
	unsigned int underruns;
	unsigned int bufferCurSet;

	SYNA_BCMBUF_DATA bcmbuf[BCM_BUF_COUNT];

	FRAMEQUEUE inputq;     // input frame buffer queue of this plane
	FRAMEQUEUE outputq;    // recycle frame buffer queue of this plane

	bool     isTGConfig;
	unsigned int m_srcfmt;
	unsigned int m_order;
	unsigned int m_content_width;
	unsigned int m_content_height;
	unsigned int m_bits_per_pixel;
	unsigned char u8Gamma[33]; //GAMMA Table

	VPP_MEM_LIST   *vpp_mem_list;
	int is_first_frame;
};

extern HDL_dhub2d VPP_dhubHandle;
extern HDL_dhub2d AG_dhubHandle;

unsigned long syna_lcdc_readl(struct syna_lcdc_dev *dev, unsigned long addr);
void syna_lcdc_writel(struct syna_lcdc_dev *dev, unsigned long addr, unsigned long val);
int syna_lcdc_pushframe(int planeID, void *pnew);
int syna_lcdc_dlr_create(struct syna_lcdc_dev *dev, int num);
void syna_lcdc_dlr_handler(struct syna_lcdc_dev *dev);
void syna_bcmbuf_flip(struct syna_lcdc_dev *dev);
void syna_bcmbuf_submit(struct syna_lcdc_dev *dev, int use_vbi);
void syna_lcdc_irq(int intrMask);
int syna_lcdc_waitVsync(int lcdcId);
void syna_lcdc_releaseVsync(int lcdcId);
void syna_lcdc_hw_config(int lcdcID, SYNA_LCDC_PANEL *panel);
void syna_lcdc_dlr_destroy(struct syna_lcdc_dev *dev);
int syna_lcdc_suspend (int enable);