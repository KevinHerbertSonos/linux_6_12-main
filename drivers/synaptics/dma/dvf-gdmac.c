/*
 * driver for DSPG generic DMA controller (GMDA)
 *
 * Copyright (C) 2017 DSPG Technologies GmbH
 * (based on idma64 driver)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/ccu.h>
#include <linux/clk.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_dma.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/reset.h>
#include <linux/pm_runtime.h>

#include "virt-dma.h"

#define CEMSR       0x000 /* channel enable mode and status */
#define CER         0x004 /* channel enable */
#define CDR         0x008 /* channel disable */
#define CTOR        0x00C /* channel timeout */
#define CCIER       0x010 /* channel interrupt enable */
#define CDIER       0x014 /* channel done interrupt enable */
#define CCICR       0x018 /* channel interrupt cause */
#define CDICR       0x01C /* channel done interrupt cause */
#define CCICLR      0x020 /* channel interrupt clear */
#define CDICLR      0x024 /* channel done interrupt clear */
#define CRDR        0x028 /* channel ready */
#define CTOENR      0x02C /* channel timeout enable */
#define CTERENR     0x030 /* channel transfer errors enable */
#define CTOCR       0x034 /* channel timeout cause */
#define CTECR       0x038 /* channel transfer error cause */
#define CTOCLR      0x03C /* channel timeout clear */
#define CTECLR      0x040 /* channel transfer error clear */
#define CRCONF1     0x044 /* channel hardware request configuration 1 */
#define CRCONF2     0x048 /* channel hardware request configuration 2 */
#define CRCONF3     0x04C /* channel hardware request configuration 3 */
#define CRCONF4     0x050 /* channel hardware request configuration 4 */
#define CRCONF5     0x054 /* channel hardware request configuration 5 */
#define CRCONF6     0x058 /* channel hardware request configuration 6 */
#define CDPR        0x05C /* channel descriptors port */
#define C0DESCADR   0x060 /* channel 0 descriptor address */
#define C0RTC       0x0E0 /* channel 0 remaining transfer count */

/* Transfer descriptor mode fields */
#define TD_TMO_EN   BIT(28)
#define TD_INT_EN   BIT(29)
#define TD_OWNER    BIT(30)
#define TD_DIR      BIT(31)

#define TD_MODE_PORT_TRANSFER_SINGLE_HW_BURST  (0x2 << 25)

#define DRV_NAME		"dvf_gdmac"

#define GDMAC_BUSWIDTHS					\
	(BIT(DMA_SLAVE_BUSWIDTH_1_BYTE)		|	\
	 BIT(DMA_SLAVE_BUSWIDTH_2_BYTES)	|	\
	 BIT(DMA_SLAVE_BUSWIDTH_4_BYTES))

#define GDMAC_MAX_BURST   32

struct gdmac {
	struct dma_device dma;
	void __iomem *regs;
	struct gdmac_chan *chan;
	u32 chan_mask;
};

/* real hardware descriptor */
struct gdmac_lli {
	u32 port_a;
	u32 port_b;
	u32 size;
	u32 mode;
	u32 next;
};

/* hardware descriptor container */
struct gdmac_hw_desc {
	struct gdmac_lli *lli;
	dma_addr_t llp;
	dma_addr_t phys;
	unsigned int len;
};

/* logical dma descriptor */
struct gdmac_desc {
	struct virt_dma_desc vdesc;
	enum dma_transfer_direction direction;
	struct gdmac_hw_desc *hw;
	unsigned int ndesc;
	size_t length;
	enum dma_status status;
	unsigned int cyclic;
	unsigned int count;
};

struct gdmac_chip {
	struct device *dev;
	unsigned int nr_chan;
	unsigned int timeout;
	u32 *priorities;
	int done_irq;
	int error_irq;
	void __iomem *regs;
	struct clk *clk;
	struct reset_control *reset;
	struct gdmac *gdmac;
};

struct gdmac_chan {
	struct virt_dma_chan vchan;

	/* hardware configuration */
	enum dma_transfer_direction direction;
	unsigned int mask;
	struct dma_slave_config config;

	void *pool;
	/* dma descriptors */
	struct gdmac_desc *desc;
};

static inline struct gdmac *to_gdmac(struct dma_device *ddev)
{
	return container_of(ddev, struct gdmac, dma);
}

static inline struct gdmac_chan *to_gdmac_chan(struct dma_chan *chan)
{
	return container_of(chan, struct gdmac_chan, vchan.chan);
}

static inline struct gdmac_desc *to_gdmac_desc(struct virt_dma_desc *vdesc)
{
	return container_of(vdesc, struct gdmac_desc, vdesc);
}

static struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}

static void gdmac_reg_write(struct gdmac *gdmac, unsigned int reg, u32 value)
{
	iowrite32(value, gdmac->regs + reg);
}

static u32 gdmac_reg_read(struct gdmac *gdmac, unsigned int reg)
{
	return ioread32(gdmac->regs + reg);
}

static int gdmac_alloc_chan_resources(struct dma_chan *chan)
{
	struct gdmac_chan *gdmacc = to_gdmac_chan(chan);

	/*
	 * Create a pool of consistent memory blocks for hardware descriptors
	 */
	gdmacc->pool = dma_pool_create(dev_name(chan2dev(chan)),
				       chan->device->dev,
				       sizeof(struct gdmac_lli), 32, 0);
	if (!gdmacc->pool) {
		dev_err(chan2dev(chan), "No memory for descriptors\n");
		return -ENOMEM;
	}

	return 0;
}

static void gdmac_free_chan_resources(struct dma_chan *chan)
{
	struct gdmac_chan *gdmacc = to_gdmac_chan(chan);

	vchan_free_chan_resources(to_virt_chan(chan));
	dma_pool_destroy(gdmacc->pool);
	gdmacc->pool = NULL;
}

static struct gdmac_desc *gdmac_alloc_desc(unsigned int ndesc)
{
	struct gdmac_desc *desc;

	desc = kzalloc(sizeof(*desc), GFP_NOWAIT);
	if (!desc)
		return NULL;

	desc->hw = kcalloc(ndesc, sizeof(*desc->hw), GFP_NOWAIT);
	if (!desc->hw) {
		kfree(desc);
		return NULL;
	}

	return desc;
}

static void gdmac_desc_free(struct gdmac_chan *gdmacc, struct gdmac_desc *desc)
{
	struct gdmac_hw_desc *hw;

	if (desc->ndesc) {
		unsigned int i = desc->ndesc;

		do {
			hw = &desc->hw[--i];
			dma_pool_free(gdmacc->pool, hw->lli, hw->llp);
		} while (i);
	}

	kfree(desc->hw);
	kfree(desc);
}

static unsigned int xlate_addr_width(enum dma_slave_buswidth bw)
{
	unsigned int width = 2; /* 32 bits by default */

	switch (bw) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		width = 0;
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		width = 1;
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		width = 2;
		break;
	default:
		WARN(1, "this should not happen");
		break;
	}
	return width << 18;
}

static unsigned int xlate_transfer_length(unsigned int tl)
{
	return ((tl - 1) & 0x1f) << 13;
}

static void gdmac_hw_desc_fill(struct gdmac_hw_desc *hw,
		struct dma_slave_config *config,
		enum dma_transfer_direction direction, dma_addr_t llp)
{
	struct gdmac_lli *lli = hw->lli;

	if (direction == DMA_MEM_TO_DEV) {
		lli->port_a = config->dst_addr; /* pa of device (e.g. fifo) */
		lli->port_b = hw->phys; /* pa of sg buffer */
		lli->mode = xlate_addr_width(config->dst_addr_width);
		lli->mode |= xlate_transfer_length(config->dst_maxburst);
		lli->mode |= TD_DIR;
	} else {	/* DMA_DEV_TO_MEM */
		lli->port_a = config->src_addr;
		lli->port_b = hw->phys;
		lli->mode = xlate_addr_width(config->src_addr_width);
		lli->mode |= xlate_transfer_length(config->src_maxburst);
	}

	lli->mode |= TD_MODE_PORT_TRANSFER_SINGLE_HW_BURST;
	lli->size = hw->len; /* TODO check if 4 byte aligned */
	lli->next = llp;
	lli->mode |= TD_OWNER;
}

static void gdmac_desc_fill(struct gdmac_chan *gdmacc, struct gdmac_desc *desc)
{
	struct dma_slave_config *config = &gdmacc->config;
	unsigned int i = desc->ndesc;
	struct gdmac_hw_desc *hw = &desc->hw[i - 1];
	struct gdmac_lli *lli = hw->lli; /* last transfer descriptor */
	dma_addr_t llp = 0;

	/* Fill the hardware descriptors and link them to a list */
	do {
		hw = &desc->hw[--i];
		gdmac_hw_desc_fill(hw, config, desc->direction, llp);
		llp = hw->llp;
		desc->length += hw->len;
		if (desc->cyclic) {
			/* issue interrupt for all lli's */
			hw->lli->mode |= TD_INT_EN;
			hw->lli->mode |= TD_TMO_EN;
		}
	} while (i);

	if (desc->cyclic) {
		/* chain last and first */
		lli->next = llp;
	} else {
		/* Trigger an interrupt after the last block is transferred */
		lli->mode |= TD_INT_EN;
		/* Trigger an interrupt on timeout */
		lli->mode |= TD_TMO_EN;
	}
}

static struct dma_async_tx_descriptor *gdmac_prep_slave_sg(
		struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct gdmac_chan *gdmacc = to_gdmac_chan(chan);
	struct gdmac_desc *desc;
	struct scatterlist *sg;
	unsigned int i;

	desc = gdmac_alloc_desc(sg_len);
	if (!desc)
		return NULL;

	for_each_sg(sgl, sg, sg_len, i) {
		struct gdmac_hw_desc *hw = &desc->hw[i];

		/* Allocate DMA capable memory for hardware descriptor */
		hw->lli = dma_pool_alloc(gdmacc->pool, GFP_NOWAIT, &hw->llp);
		if (!hw->lli) {
			desc->ndesc = i;
			gdmac_desc_free(gdmacc, desc);
			return NULL;
		}

		hw->phys = sg_dma_address(sg);
		hw->len = sg_dma_len(sg);
	}

	desc->ndesc = sg_len;
	desc->direction = direction;
	desc->status = DMA_IN_PROGRESS;
	desc->cyclic = 0;

	gdmac_desc_fill(gdmacc, desc);
	return vchan_tx_prep(&gdmacc->vchan, &desc->vdesc, flags);
}


static struct dma_async_tx_descriptor *gdmac_prep_dma_cyclic(
		struct dma_chan *chan, dma_addr_t buf_addr, size_t buf_len,
		size_t period_len, enum dma_transfer_direction direction,
		unsigned long flags)
{

	struct gdmac_chan *gdmacc = to_gdmac_chan(chan);
	struct gdmac_desc *desc;
	unsigned int i;
	unsigned int nr_periods;

	if (!buf_len) {
		dev_err(chan2dev(chan), "invalid buffer length\n");
		return NULL;
	}

	if (!period_len) {
		dev_err(chan2dev(chan), "invalid period length\n");
		return NULL;
	}

	if (buf_len % period_len) {
		dev_err(chan2dev(chan),
			"buf_len must be a multiple of period_len\n");
		return NULL;
	}

	nr_periods = buf_len / period_len;

	desc = gdmac_alloc_desc(nr_periods);
	if (!desc)
		return NULL;

	for (i = 0; i < nr_periods; i++) {
		struct gdmac_hw_desc *hw = &desc->hw[i];

		/* Allocate DMA capable memory for hardware descriptor */
		hw->lli = dma_pool_alloc(gdmacc->pool, GFP_NOWAIT, &hw->llp);
		if (!hw->lli) {
			desc->ndesc = i;
			gdmac_desc_free(gdmacc, desc);
			return NULL;
		}

		hw->phys = buf_addr;
		hw->len = period_len;
		buf_addr += period_len;
	}

	desc->ndesc = nr_periods;
	desc->direction = direction;
	desc->status = DMA_IN_PROGRESS;
	desc->cyclic = 1;

	gdmac_desc_fill(gdmacc, desc);
	return vchan_tx_prep(&gdmacc->vchan, &desc->vdesc, flags);
}

/* init hw channel */
static void gdmac_chan_init(struct gdmac *gdmac, struct gdmac_chan *gdmacc)
{
	/* write first descriptor to C<channel>DESCADR */
	gdmac_reg_write(gdmac, C0DESCADR + __ffs(gdmacc->mask) * 4,
			gdmacc->desc->hw[0].llp);
}

/* deinit hw channel */
static void gdmac_chan_deinit(struct gdmac *gdmac, struct gdmac_chan *gdmacc)
{
	/* write zero descriptor to C<channel>DESCADR */
	gdmac_reg_write(gdmac, C0DESCADR + __ffs(gdmacc->mask) * 4, 0);
}

/* stop hw channel */
static void gdmac_chan_stop(struct gdmac *gdmac, struct gdmac_chan *gdmacc)
{
	/*
	 * set bit in CDR register, take care, dma might finish current
	 * transfer size
	 */
	gdmac_reg_write(gdmac, CDR, gdmacc->mask);
	/* wait till gdmac has finished current transfer size */
	while (ioread32(gdmac->regs + CEMSR) & gdmacc->mask)
		cpu_relax();
	ccu_barrier(0);
}

/* start hw channel */
static void gdmac_chan_start(struct gdmac *gdmac, struct gdmac_chan *gdmacc)
{
	/* set bit in CER */
	gdmac_reg_write(gdmac, CER, gdmacc->mask);
}

static void gdmac_start_transfer(struct gdmac_chan *gdmacc)
{
	struct gdmac *gdmac = to_gdmac(gdmacc->vchan.chan.device);
	struct virt_dma_desc *vdesc;

	/* Get the next descriptor */
	vdesc = vchan_next_desc(&gdmacc->vchan);
	if (!vdesc) {
		gdmacc->desc = NULL;
		return;
	}

	list_del(&vdesc->node);
	gdmacc->desc = to_gdmac_desc(vdesc);

	pm_runtime_get_sync(gdmac->dma.dev);

	/* Configure the channel */
	gdmac_chan_init(gdmac, gdmacc);

	/* Start the channel with a new descriptor */
	gdmac_chan_start(gdmac, gdmacc);
}

static void gdmac_issue_pending(struct dma_chan *chan)
{
	struct gdmac_chan *gdmacc = to_gdmac_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&gdmacc->vchan.lock, flags);
	if (vchan_issue_pending(&gdmacc->vchan) && !gdmacc->desc)
		gdmac_start_transfer(gdmacc);
	spin_unlock_irqrestore(&gdmacc->vchan.lock, flags);
}

static size_t gdmac_active_desc_size(struct gdmac_chan *gdmacc)
{
	struct gdmac *gdmac = to_gdmac(gdmacc->vchan.chan.device);
	struct gdmac_desc *desc = gdmacc->desc;
	struct gdmac_hw_desc *hw;
	size_t bytes = desc->length;
	unsigned int i = 0;
	u32 llp;

	llp = gdmac_reg_read(gdmac, C0DESCADR + __ffs(gdmacc->mask) * 4);
	do {
		hw = &desc->hw[i];
		if (hw->llp == llp)
			break;
		bytes -= hw->len;
	} while (++i < desc->ndesc);

	if (!i)
		return bytes;

	/* The current chunk is not fully transfered yet */
	bytes += desc->hw[--i].len;

	return bytes;
}

static enum dma_status gdmac_tx_status(struct dma_chan *chan,
		dma_cookie_t cookie, struct dma_tx_state *state)
{
	struct gdmac_chan *gdmacc = to_gdmac_chan(chan);
	struct virt_dma_desc *vdesc;
	enum dma_status status;
	size_t bytes;
	unsigned long flags;

	status = dma_cookie_status(chan, cookie, state);
	if (status == DMA_COMPLETE)
		return status;

	spin_lock_irqsave(&gdmacc->vchan.lock, flags);
	vdesc = vchan_find_desc(&gdmacc->vchan, cookie);
	if (gdmacc->desc && cookie == gdmacc->desc->vdesc.tx.cookie) {
		bytes = gdmac_active_desc_size(gdmacc);
		dma_set_residue(state, bytes);
		status = gdmacc->desc->status;
	} else if (vdesc) {
		bytes = to_gdmac_desc(vdesc)->length;
		dma_set_residue(state, bytes);
	}
	spin_unlock_irqrestore(&gdmacc->vchan.lock, flags);

	return status;
}

static int gdmac_slave_config(struct dma_chan *chan,
		struct dma_slave_config *config)
{
	struct gdmac_chan *gdmacc = to_gdmac_chan(chan);

	/* Check if chan will be configured for slave transfers */
	if (!is_slave_direction(config->direction))
		return -EINVAL;

	if (config->direction == DMA_DEV_TO_MEM) {
		if (config->src_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED)
			return -EINVAL;

		if (config->src_addr_width > DMA_SLAVE_BUSWIDTH_4_BYTES)
			return -EINVAL;

		if (config->src_maxburst > GDMAC_MAX_BURST)
			return -EINVAL;
	} else {
		if (config->dst_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED)
			return -EINVAL;

		if (config->dst_addr_width > DMA_SLAVE_BUSWIDTH_4_BYTES)
			return -EINVAL;

		if (config->dst_maxburst > GDMAC_MAX_BURST)
			return -EINVAL;
	}

	memcpy(&gdmacc->config, config, sizeof(gdmacc->config));

	return 0;
}

static void gdmac_vdesc_free(struct virt_dma_desc *vdesc)
{
	struct gdmac_chan *gdmacc = to_gdmac_chan(vdesc->tx.chan);

	gdmac_desc_free(gdmacc, to_gdmac_desc(vdesc));
}

static void gdmac_stop_transfer(struct gdmac_chan *gdmacc)
{
	struct gdmac *gdmac = to_gdmac(gdmacc->vchan.chan.device);

	gdmac_chan_stop(gdmac, gdmacc);
	gdmac_chan_deinit(gdmac, gdmacc);

	pm_runtime_put_sync(gdmac->dma.dev);
}

static int gdmac_terminate_all(struct dma_chan *chan)
{
	struct gdmac_chan *gdmacc = to_gdmac_chan(chan);
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock_irqsave(&gdmacc->vchan.lock, flags);
	gdmac_stop_transfer(gdmacc);
	if (gdmacc->desc) {
		gdmac_vdesc_free(&gdmacc->desc->vdesc);
		gdmacc->desc = NULL;
	}
	vchan_get_all_descriptors(&gdmacc->vchan, &head);
	spin_unlock_irqrestore(&gdmacc->vchan.lock, flags);

	vchan_dma_desc_free_list(&gdmacc->vchan, &head);
	return 0;
}

static void gdmac_chan_irq(struct gdmac *gdmac, unsigned short c,
			   u32 status_err, u32 status_xfer, u32 status_ch)
{
	struct gdmac_chan *gdmacc = &gdmac->chan[c];
	struct gdmac_desc *desc;
	struct gdmac_hw_desc *hw;
	unsigned long flags;

	spin_lock_irqsave(&gdmacc->vchan.lock, flags);
	desc = gdmacc->desc;
	if (desc) {
		if (status_err & gdmacc->mask) {
			desc->status = DMA_ERROR;
		} else {
			if ((status_xfer | status_ch) & gdmacc->mask) {
				if (desc->cyclic) {
					hw = &desc->hw[desc->count];
					/* recycle descriptor */
					hw->lli->mode &= ~BIT(1);
					hw->lli->mode |= TD_OWNER;
					gdmac_chan_start(gdmac, gdmacc);

					vchan_cyclic_callback(&desc->vdesc);
				} else {
					desc->status = DMA_COMPLETE;
					vchan_cookie_complete(&desc->vdesc);
					gdmac_start_transfer(gdmacc);
				}
			}
		}

		/* gdmac_start_transfer() updates gdmacc->desc */
		if (gdmacc->desc == NULL || desc->status == DMA_ERROR)
			gdmac_stop_transfer(gdmacc);

		desc->count = (desc->count + 1) % desc->ndesc;
	}
	spin_unlock_irqrestore(&gdmacc->vchan.lock, flags);
}

static irqreturn_t gdmac_done_irq(int irq, void *dev)
{
	struct gdmac *gdmac = dev;
	u32 status = gdmac_reg_read(gdmac, CCICR);
	u32 status_xfer;
	unsigned int i;

	dev_vdbg(gdmac->dma.dev, "%s: status=%#x\n", __func__, status);

	/* Check if we have any interrupt from the DMA controller */
	if (!status)
		return IRQ_NONE;

	/* read transfer done interrupt status */
	status_xfer = gdmac_reg_read(gdmac, CDICR);

	/* clear all pending interrupts */
	gdmac_reg_write(gdmac, CCICLR, status);
	gdmac_reg_write(gdmac, CDICLR, status_xfer);

	/*
	 * ATTENTION: The following call will guarantee that all memory
	 * transactions indicated by the device irq flags have actually hit the
	 * memory. Just process the flags which have been sampled. When reading
	 * the status again any new IRQ flags MUST NOT be processed until
	 * ccu_barrier() is called again!
	 */
	ccu_barrier(0);

	for (i = 0; i < gdmac->dma.chancnt; i++)
		gdmac_chan_irq(gdmac, i, 0, status_xfer, status);

	return IRQ_HANDLED;
}

/* transfer or timeout errors */
static irqreturn_t gdmac_error_irq(int irq, void *dev)
{
	struct gdmac *gdmac = dev;
	u32 status;
	u32 status_err;
	unsigned int i;

	/* read and clear transfer error and timeout interrupts */
	status = gdmac_reg_read(gdmac, CTECR);
	gdmac_reg_write(gdmac, CTECLR, status);
	status_err = status;
	dev_vdbg(gdmac->dma.dev, "%s: te status=%#x\n", __func__, status_err);
	status |= gdmac_reg_read(gdmac, CTOCR);
	gdmac_reg_write(gdmac, CTOCLR, status);
	status_err |= status;

	dev_vdbg(gdmac->dma.dev, "%s: to status=%#x\n", __func__, status_err);

	/* Check if we have any interrupt from the DMA controller */
	if (!status_err)
		return IRQ_NONE;

	/*
	 * ATTENTION: The following call will guarantee that all memory
	 * transactions indicated by the device irq flags have actually hit the
	 * memory. Just process the flags which have been sampled. When reading
	 * the status again any new IRQ flags MUST NOT be processed until
	 * ccu_barrier() is called again!
	 */
	ccu_barrier(0);

	for (i = 0; i < gdmac->dma.chancnt; i++)
		gdmac_chan_irq(gdmac, i, status_err, 0, 0);

	return IRQ_HANDLED;
}

static void gdmac_enable_all_ints(struct gdmac *gdmac)
{
	u32 mask = gdmac->chan_mask;

	gdmac_reg_write(gdmac, CCIER, mask);
	gdmac_reg_write(gdmac, CDIER, mask);
	gdmac_reg_write(gdmac, CTERENR, mask);
	gdmac_reg_write(gdmac, CTOENR, mask);
}

static int dvf_gdmac_probe(struct gdmac_chip *chip)
{
	struct gdmac *gdmac;
	unsigned int i;
	int ret;

	gdmac = devm_kzalloc(chip->dev, sizeof(*gdmac), GFP_KERNEL);
	if (!gdmac)
		return -ENOMEM;

	gdmac->regs = chip->regs;
	chip->gdmac = gdmac;

	gdmac->chan = devm_kcalloc(chip->dev, chip->nr_chan,
				   sizeof(*gdmac->chan), GFP_KERNEL);
	if (!gdmac->chan)
		return -ENOMEM;

	gdmac->chan_mask = (1 << chip->nr_chan) - 1;

	ret = devm_request_irq(chip->dev, chip->done_irq, gdmac_done_irq,
			       IRQF_SHARED, dev_name(chip->dev), gdmac);
	if (ret)
		return ret;

	ret = devm_request_irq(chip->dev, chip->error_irq, gdmac_error_irq,
			       IRQF_SHARED, dev_name(chip->dev), gdmac);
	if (ret)
		return ret;

	INIT_LIST_HEAD(&gdmac->dma.channels);
	for (i = 0; i < chip->nr_chan; i++) {
		struct gdmac_chan *gdmacc = &gdmac->chan[i];

		gdmacc->vchan.desc_free = gdmac_vdesc_free;
		vchan_init(&gdmacc->vchan, &gdmac->dma);

		gdmacc->mask = BIT(chip->priorities[i]);
	}

	dma_cap_set(DMA_SLAVE, gdmac->dma.cap_mask);
	dma_cap_set(DMA_CYCLIC, gdmac->dma.cap_mask);
	dma_cap_set(DMA_PRIVATE, gdmac->dma.cap_mask);

	gdmac->dma.device_alloc_chan_resources = gdmac_alloc_chan_resources;
	gdmac->dma.device_free_chan_resources = gdmac_free_chan_resources;

	gdmac->dma.device_prep_slave_sg = gdmac_prep_slave_sg;
	gdmac->dma.device_prep_dma_cyclic = gdmac_prep_dma_cyclic;

	gdmac->dma.device_issue_pending = gdmac_issue_pending;
	gdmac->dma.device_tx_status = gdmac_tx_status;

	gdmac->dma.device_config = gdmac_slave_config;
	gdmac->dma.device_terminate_all = gdmac_terminate_all;

	gdmac->dma.src_addr_widths = GDMAC_BUSWIDTHS;
	gdmac->dma.dst_addr_widths = GDMAC_BUSWIDTHS;
	gdmac->dma.max_burst = GDMAC_MAX_BURST;
	gdmac->dma.directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV);
	gdmac->dma.residue_granularity = DMA_RESIDUE_GRANULARITY_SEGMENT;

	gdmac->dma.dev = chip->dev;

	dma_set_max_seg_size(gdmac->dma.dev, DMA_BIT_MASK(32));

	ret = clk_prepare_enable(chip->clk);
	if (ret) {
		dev_err(chip->dev, "failed to enable clock: %d\n", ret);
		return ret;
	}

	/* set timeout */
	if (chip->timeout)
		gdmac_reg_write(gdmac, CTOR, chip->timeout);

	for (i = 0; i < chip->nr_chan; i++) {
		u32 priority;
		u32 reg;

		if (chip->nr_chan > 16) {
			priority = (chip->priorities[i] & 0x1f) << (i % 6) * 5;
			reg = gdmac_reg_read(gdmac, CRCONF1 + 4 * (i / 6));

			reg &= ~(0x1f << (i % 6) * 5);
			reg |= priority;
			gdmac_reg_write(gdmac, CRCONF1 + 4 * (i / 6), reg);
		} else {
			priority = (chip->priorities[i] & 0xf) << (i % 8) * 4;
			reg = gdmac_reg_read(gdmac, CRCONF1 + 4 * (i / 8));

			reg &= ~(0xf << (i % 8) * 4);
			reg |= priority;
			gdmac_reg_write(gdmac, CRCONF1 + 4 * (i / 8), reg);
		}
	}

	gdmac_enable_all_ints(gdmac);

	/* port B controls the descriptors */
	gdmac_reg_write(gdmac, CDPR, 1);

	ret = dma_async_device_register(&gdmac->dma);
	if (ret)
		return ret;

	ret = of_dma_controller_register(chip->dev->of_node,
					 of_dma_xlate_by_chan_id, gdmac);
	if (ret) {
		dev_err(chip->dev, "failed to register dma controller: %d\n",
			ret);
		dma_async_device_unregister(&gdmac->dma);
		return ret;
	}

	dev_info(chip->dev, "found DSPG GDMAC DMA controller\n");
	return 0;
}

static int dvf_dma_platform_probe(struct platform_device *pdev)
{
	struct gdmac_chip *chip;
	struct device *dev = &pdev->dev;
	const struct device_node *np = pdev->dev.of_node;
	struct resource *mem;
	int ret;
	unsigned int i;

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	ret = of_property_read_u32(np, "dma-channels", &chip->nr_chan);
	if (ret) {
		dev_err(dev, "dma-channels invalid or missing: %d\n", ret);
		return ret;
	}

	of_property_read_u32(np, "timeout", &chip->timeout);

	chip->priorities = devm_kzalloc(dev, sizeof(u32) * chip->nr_chan,
					GFP_KERNEL);
	if (!chip->priorities)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, "priorities", chip->priorities,
					 chip->nr_chan);
	if (ret) {
		/* set defaults */
		for (i = 0; i < chip->nr_chan; i++)
			chip->priorities[i] = i;
	}

	chip->done_irq = platform_get_irq(pdev, 0);
	if (chip->done_irq < 0)
		return chip->done_irq;

	chip->error_irq = platform_get_irq(pdev, 1);
	if (chip->error_irq < 0)
		return chip->error_irq;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(chip->regs))
		return PTR_ERR(chip->regs);

	chip->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(chip->clk)) {
		ret = PTR_ERR(chip->clk);
		dev_err(dev, "no clock for device: %d\n", ret);
		return ret;
	}

	chip->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(chip->reset)) {
		dev_err(&pdev->dev, "cannot get reset control\n");
		return PTR_ERR(chip->reset);
	}

	ret = reset_control_deassert(chip->reset);
	if (ret) {
		dev_err(&pdev->dev, "cannot deassert reset\n");
		return ret;
	}

	ret = dma_coerce_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	chip->dev = dev;

	/* enable runtime power management */
	pm_runtime_use_autosuspend(dev);
	pm_runtime_irq_safe(dev);
	pm_runtime_enable(dev);

	ret = dvf_gdmac_probe(chip);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, chip);

	dev_info(&pdev->dev, "successfully probed\n");
	return 0;
}

#ifdef CONFIG_PM
static int dvf_dma_runtime_suspend(struct device *dev)
{
	struct gdmac_chip *chip = dev_get_drvdata(dev);

	clk_disable_unprepare(chip->clk);

	return 0;
}

static int dvf_dma_runtime_resume(struct device *dev)
{
	struct gdmac_chip *chip = dev_get_drvdata(dev);

	return clk_prepare_enable(chip->clk);
}
#endif

static const struct dev_pm_ops dvf_dma_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(dvf_dma_runtime_suspend, dvf_dma_runtime_resume,
			   NULL)
};

static const struct of_device_id dvf_dma_dt_ids[] = {
	{ .compatible = "dspg,gdmac", },
	{ /* sentinel */ }
};

static struct platform_driver dvf_dma_platform_driver = {
	.probe		= dvf_dma_platform_probe,
	.driver = {
		.name	= DRV_NAME,
		.pm	= &dvf_dma_dev_pm_ops,
		.of_match_table = dvf_dma_dt_ids,
	},
};

static int __init dvf_dma_init(void)
{
	return platform_driver_register(&dvf_dma_platform_driver);
}
arch_initcall_sync(dvf_dma_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DVF DMA Controller driver");
