/*
 * spi-dspg-dmw.c - SPI master driver for the DMW96 style controller
 *
 * Copyright (C) 2012 DSPG Technologies GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/spi/spi_dspg_dmw.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/mm.h>

#define DRIVER_NAME "spi-dspg-dmw"

#define DMW_SPI_CFG_REG		0x00
#define DMW_SPI_RATE_CNTL	0x04
#define DMW_SPI_INT_EN		0x08
#define DMW_SPI_INT_STAT	0x0C
#define DMW_SPI_INT_CLR		0x10
#define DMW_SPI_INT_CAUSE	0x14
#define DMW_SPI_TX_DAT		0x18
#define DMW_SPI_RX_DAT		0x1C
#define DMW_SPI_DEL_VAL		0x20
#define DMW_SPI_RX_BUFF		0x3C
#define DMW_SPI_DBG		0x40

#define DMW_SPI_FIFO_SIZE	16

#define DMW_SPI_NUM_CHIPSELECT	2

#define INT_RX_FIFO_WM		(1<<0)
#define INT_RX_TO		(1<<1)
#define INT_RX_FIFO_UR		(1<<2)
#define INT_TX_FIFO_WM		(1<<8)
#define INT_TX_FIFO_OV		(1<<9)
#define INT_ALL_INTS		(INT_RX_FIFO_WM | INT_RX_TO | \
				 INT_RX_FIFO_UR | INT_TX_FIFO_WM | \
				 INT_TX_FIFO_OV)
#define INT_NO_INTS		0

#define INT_FULL_FIFO_WM	(15<<12)

#define SPI_IDLE		(1<<15)
#define SPI_DONE		(1<<10)

#define CFG_EN			(1<<31)
#define CFG_FIFOS_FLUSH		((1<<17) | (1<<18))
#define CFG_TX_DMA_EN		(1<<16)
#define CFG_RX_DMA_EN		(1<<15)
#define CFG_RX_IGNORE		(1<<8)
#define CFG_DIRECT_CS		(1<<7)
#define CFG_SWCS		(1<<6)
#define CFG_CS1_HOLD		(1<<4)
#define CFG_CS1_DIS		(1<<3)
#define CFG_CS0_HOLD		(1<<2)
#define CFG_CS0_DIS		(1<<1)

struct dspg_dmw_spi {
	/* bitbang has to be first */
	struct spi_bitbang	 bitbang;
	struct completion	 done;

	void __iomem		*regs;
	int			 irq;
	int			 remaining_bytes;

	/* data buffers */
	const unsigned char	*tx_ptr;
	unsigned char		*rx_ptr;

	int			chip_select;
	unsigned int		cs_gpios[DMW_SPI_NUM_CHIPSELECT];
	u32			delays[16];

	struct clk		*clk;
	struct spi_master	*master;

	struct reset_control	*rc;

	struct device		*dev;

	struct dma_chan			*dma_rx_channel;
	struct dma_chan			*dma_tx_channel;
	struct dma_async_tx_descriptor	*rx_descriptor;
	struct dma_async_tx_descriptor	*tx_descriptor;
};

static unsigned int dma = 1;
module_param(dma, uint, 0644);
MODULE_PARM_DESC(dma, "Support for DMA mode (default: 1)");

static void dspg_dmw_spi_debug(struct dspg_dmw_spi *hw);

static void
dspg_dmw_spi_dma_tx_callback(void *context)
{
	struct dspg_dmw_spi *hw = (struct dspg_dmw_spi *)context;

	if (!hw->rx_descriptor)
		complete(&hw->done);
}

static void
dspg_dmw_spi_dma_rx_callback(void *context)
{
	struct dspg_dmw_spi *hw = (struct dspg_dmw_spi *)context;

	complete(&hw->done);
}

static inline struct dspg_dmw_spi *
to_hw(struct spi_device *sdev)
{
	return spi_master_get_devdata(sdev->master);
}

static void
dspg_dmw_spi_set_cs(struct dspg_dmw_spi *hw, unsigned int chip_select,
		    int val, int pol)
{
	unsigned int ctrl;
	unsigned int bit = CFG_CS0_HOLD;

	clk_prepare_enable(hw->clk);

	ctrl = readl(hw->regs + DMW_SPI_CFG_REG);

	if (hw->chip_select != 0)
		bit = CFG_CS1_HOLD;

	ctrl |= CFG_DIRECT_CS;
	if ((pol && (val == BITBANG_CS_ACTIVE)) ||
	   (!pol && (val == BITBANG_CS_INACTIVE))) {
		ctrl |=  CFG_SWCS;
		/* ctrl &= ~bit; */
		/* don't negate chip select at end of transaction */
	} else {
		ctrl &= ~CFG_SWCS;
		/* ctrl |=  bit; */
		/* negate chip select at end of transaction */
	}

	writel(ctrl, hw->regs + DMW_SPI_CFG_REG);

	clk_disable_unprepare(hw->clk);
}

static void
dspg_dmw_spi_chipsel(struct spi_device *spi, int is_on)
{
	struct dspg_dmw_spi *hw = to_hw(spi);
	unsigned int cspol = spi->mode & SPI_CS_HIGH ? 1 : 0;
	unsigned int chip_select = spi->chip_select;

	if (spi->master->cs_gpios &&
	    gpio_is_valid(spi->master->cs_gpios[chip_select]))
		gpio_direction_output(spi->master->cs_gpios[chip_select],
				      is_on ? cspol : !cspol);
	else if (chip_select < DMW_SPI_NUM_CHIPSELECT)
		dspg_dmw_spi_set_cs(hw, chip_select, is_on, cspol);
	else
		dev_warn(&spi->dev, "could not set CS %u\n",
			 chip_select);
}

static int
dspg_dmw_spi_setupxfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct dspg_dmw_spi *hw = to_hw(spi);
	unsigned int bpw;
	unsigned long clk_rate = clk_get_rate(hw->clk);
	unsigned int hz;
	unsigned int div;
	unsigned int div_code;
	unsigned int ctrl = 0;
	unsigned int msb_first = ~(spi->mode & SPI_LSB_FIRST);
	unsigned int clk_polarity =
			 spi->mode & SPI_CPOL ? 0 : 1; /* 0 if active high */

	bpw = t ? t->bits_per_word : 0;
	if (!bpw)
		bpw = spi->bits_per_word;
	hz  = t ? t->speed_hz : 0;
	if (!hz)
		hz = spi->max_speed_hz;

	dev_dbg(&spi->dev, "hz = %u, clockrate = %lu, clk_polarity = %s\n",
		hz, clk_rate, clk_polarity ? "active high" : "active low");

	if (!hz) {
		dev_err(&spi->dev, "invalid speed value (%dHz)\n", hz);
		return -EINVAL;
	}

	if (hz > spi->max_speed_hz) {
		dev_err(&spi->dev, "speed values > %dHz not supported\n",
			spi->max_speed_hz);
		return -EINVAL;
	}

	if (bpw != 8) {
		dev_err(&spi->dev, "invalid bits-per-word (%d)\n", bpw);
		return -EINVAL;
	}

	if ((spi->mode == SPI_MODE_0) || (spi->mode == SPI_MODE_2)) {
		dev_err(&spi->dev,
			"invalid mode (modes 1 and 3 are supported)\n");
		return -EINVAL;
	}

	div = (clk_rate + hz/2 - 1) / hz;
	if (div < 2)
		div = 2;

	div_code = 0;
	while ((div > 2) && (div_code < 10)) {
		div_code++;
		div = (div + 1) / 2; /* round up */
	}

	dev_dbg(&spi->dev,
		"setting pre-scaler to %d, code %u (hz %u, clk %lu)\n",
		div, div_code, hz, clk_rate);

	clk_prepare_enable(hw->clk);

	/* FIXME check if correct */
	ctrl = readl(hw->regs + DMW_SPI_CFG_REG);
	ctrl &= ~(CFG_EN);
	writel(ctrl, hw->regs + DMW_SPI_CFG_REG);

	writel(div_code, hw->regs + DMW_SPI_RATE_CNTL);

	ctrl = hw->delays[div_code];
	if (ctrl < 123)
		ctrl = 123; /* FIXME: get correct delay table for DMA */
	writel(ctrl, hw->regs + DMW_SPI_DEL_VAL);

	hw->chip_select = spi->chip_select;

	/* important: keep cs at the same configuration, since setup_transfer
	 * might be called between 2 consecutive transfers on the same device
	 * (for example: spi_write_then_read)
	 */
	ctrl = readl(hw->regs + DMW_SPI_CFG_REG) & CFG_SWCS;
	ctrl |= CFG_FIFOS_FLUSH |
	       ((msb_first & 0x1) << 5) | (clk_polarity & 0x1) | CFG_DIRECT_CS;
	if (hw->chip_select == 0)
		ctrl |= CFG_CS1_DIS;
	else
		ctrl |= CFG_CS0_DIS;
	writel(ctrl, hw->regs + DMW_SPI_CFG_REG);

	ctrl &= ~CFG_FIFOS_FLUSH;
	writel(ctrl, hw->regs + DMW_SPI_CFG_REG);

	mutex_lock(&hw->bitbang.lock);
	if (!hw->bitbang.busy) {
		hw->bitbang.chipselect(spi, BITBANG_CS_INACTIVE);
		/* need to ndelay for 0.5 clocktick ? */
	}

	mutex_unlock(&hw->bitbang.lock);

	clk_disable_unprepare(hw->clk);

	return 0;
}

static int
dspg_dmw_spi_setup(struct spi_device *spi)
{
	int ret;

	if (!spi->bits_per_word)
		spi->bits_per_word = 8;

	ret = dspg_dmw_spi_setupxfer(spi, NULL);
	if (ret < 0) {
		dev_err(&spi->dev, "setupxfer returned %d\n", ret);
		return ret;
	}

	dev_dbg(&spi->dev, "%s: mode %d, %u bpw, %d hz\n",
		__func__, spi->mode, spi->bits_per_word,
		spi->max_speed_hz);

	return 0;
}

static void
dspg_dmw_spi_enable(struct dspg_dmw_spi *hw)
{
	unsigned long val;

	while (!(readl(hw->regs + DMW_SPI_INT_STAT) & SPI_DONE))
		;

	val = readl(hw->regs + DMW_SPI_CFG_REG);
	val |= CFG_EN;
	writel(val, hw->regs + DMW_SPI_CFG_REG);
}

static void
dspg_dmw_spi_dma(struct dspg_dmw_spi *hw, int flags)
{
	unsigned long val;

	val = readl(hw->regs + DMW_SPI_CFG_REG);
	val &= ~(CFG_TX_DMA_EN | CFG_RX_DMA_EN);
	if (val & CFG_EN) {
		while (!(readl(hw->regs + DMW_SPI_INT_STAT) & SPI_DONE))
			;

		val &= ~CFG_EN;
		writel(val, hw->regs + DMW_SPI_CFG_REG);
	}

	if (flags) {
		val |=  CFG_FIFOS_FLUSH;
		writel(val, hw->regs + DMW_SPI_CFG_REG);

		udelay(1);

		val &= ~CFG_FIFOS_FLUSH;
	}
	writel(val, hw->regs + DMW_SPI_CFG_REG);

	val |= flags;
	if ((flags & (CFG_RX_DMA_EN|CFG_TX_DMA_EN)) == CFG_TX_DMA_EN)
		val |=  CFG_RX_IGNORE;
	else
		val &= ~CFG_RX_IGNORE;
	writel(val, hw->regs + DMW_SPI_CFG_REG);

	if (flags) {
		val |= CFG_EN;
		writel(val, hw->regs + DMW_SPI_CFG_REG);
	}
}

static void
dspg_dmw_spi_watermarks(struct dspg_dmw_spi *hw, int tx, int rx)
{
	writel((((tx-1) & 0xF) << 12) | (((rx-1) & 0xF) << 4),
	       hw->regs + DMW_SPI_INT_EN);
}

static void
dspg_dmw_spi_disable_and_clear_ints(struct dspg_dmw_spi *hw)
{
	writel(INT_NO_INTS,  hw->regs + DMW_SPI_INT_EN);
	writel(INT_ALL_INTS, hw->regs + DMW_SPI_INT_CLR);
}

static void
dspg_dmw_spi_enable_rx_wm_int(struct dspg_dmw_spi *hw, int count)
{
	writel(INT_RX_FIFO_WM | (((count-1) & 0xf)<<4),
	       hw->regs + DMW_SPI_INT_EN);
}

static int
dspg_dmw_spi_rx_not_empty(struct dspg_dmw_spi *hw)
{
	return !!(readl(hw->regs + DMW_SPI_INT_STAT) & INT_RX_FIFO_WM);
}

static int
dspg_dmw_spi_tx_not_full(struct dspg_dmw_spi *hw)
{
	return !!(readl(hw->regs + DMW_SPI_INT_STAT) & INT_TX_FIFO_WM);
}

static int
dspg_dmw_spi_fill_tx_fifo(struct dspg_dmw_spi *hw)
{
	int written = 0;
	unsigned char dat = 0;

	while (dspg_dmw_spi_tx_not_full(hw) && (hw->remaining_bytes > 0) &&
	       (written < DMW_SPI_FIFO_SIZE)) {
		if (hw->tx_ptr)
			dat = *hw->tx_ptr++;
		writel(dat, hw->regs + DMW_SPI_TX_DAT);
		hw->remaining_bytes--;
		written++;
	}

	return written;
}

static bool
dspg_dmw_spi_can_dma(struct spi_master *master, struct spi_device *spi,
			    struct spi_transfer *xfer)
{
	size_t dma_align = dma_get_cache_alignment();

	if (xfer->len < DMW_SPI_FIFO_SIZE)
		return false;

	if (xfer->rx_buf && !xfer->tx_buf)
		return false;

	if (xfer->rx_buf && !IS_ALIGNED((size_t)xfer->rx_buf, dma_align))
		return false;

	if (xfer->tx_buf && !IS_ALIGNED((size_t)xfer->tx_buf, dma_align))
		return false;

	return dma;
}

static unsigned int
dspg_dmw_spi_adapt_sg(struct sg_table *sg, int remain)
{
	unsigned int nents = sg->nents;
	struct scatterlist *last = sg_last(sg->sgl, nents);

	if (remain) {
		last->length &= ~(DMW_SPI_FIFO_SIZE - 1);
		if (!last->length)
			nents--;
	}

	return nents;
}

static int
dspg_dmw_spi_txrx(struct spi_device *spi, struct spi_transfer *t)
{
	struct dspg_dmw_spi *hw = to_hw(spi);
	unsigned int remain = t->len % DMW_SPI_FIFO_SIZE;
	dma_cookie_t rx_cookie, tx_cookie;
	unsigned int nents;

	hw->chip_select = spi->chip_select;
	hw->tx_ptr = t->tx_buf;
	hw->rx_ptr = t->rx_buf;
	hw->remaining_bytes = t->len;
	reinit_completion(&hw->done);

	dev_dbg(&spi->dev, "transfer %d bytes (%s %s)\n", t->len,
		t->tx_buf ? "tx" : "  ", t->rx_buf ? "rx" : "  ");

	clk_prepare_enable(hw->clk);
	dspg_dmw_spi_disable_and_clear_ints(hw);

	if (t->rx_sg.sgl) {
		nents = dspg_dmw_spi_adapt_sg(&t->rx_sg, remain);

		hw->rx_descriptor =
			dmaengine_prep_slave_sg(hw->dma_rx_channel,
						t->rx_sg.sgl, nents,
						DMA_DEV_TO_MEM,
						DMA_PREP_INTERRUPT);
		hw->rx_descriptor->callback = dspg_dmw_spi_dma_rx_callback;
		hw->rx_descriptor->callback_param = hw;
	}

	if (t->tx_sg.sgl) {
		nents = dspg_dmw_spi_adapt_sg(&t->tx_sg, remain);

		hw->tx_descriptor =
			dmaengine_prep_slave_sg(hw->dma_tx_channel,
						t->tx_sg.sgl, nents,
						DMA_MEM_TO_DEV,
						DMA_PREP_INTERRUPT);
		hw->tx_descriptor->callback = dspg_dmw_spi_dma_tx_callback;
		hw->tx_descriptor->callback_param = hw;

		dspg_dmw_spi_watermarks(hw, DMW_SPI_FIFO_SIZE,
					DMW_SPI_FIFO_SIZE);

		if (hw->rx_descriptor) {
			rx_cookie = dmaengine_submit(hw->rx_descriptor);
			dma_async_issue_pending(hw->dma_rx_channel);
		}

		tx_cookie = dmaengine_submit(hw->tx_descriptor);
		dma_async_issue_pending(hw->dma_tx_channel);

		if (hw->rx_descriptor)
			dspg_dmw_spi_dma(hw, CFG_RX_DMA_EN | CFG_TX_DMA_EN);
		else
			dspg_dmw_spi_dma(hw, CFG_TX_DMA_EN);
	} else {
		int count;

		dspg_dmw_spi_enable(hw);

		count = dspg_dmw_spi_fill_tx_fifo(hw);

		dspg_dmw_spi_enable_rx_wm_int(hw, count);
	}

	wait_for_completion(&hw->done);

	if (t->tx_sg.sgl) {
		struct dma_tx_state state;
		enum dma_status status;

		dspg_dmw_spi_dma(hw, 0);

		if (hw->rx_descriptor) {
			status = dmaengine_tx_status(hw->dma_rx_channel,
						     rx_cookie, &state);
			if (status != DMA_COMPLETE)
				dev_err(hw->dev, "rx failed\n");
			hw->remaining_bytes = state.residue;
			hw->rx_descriptor = NULL;
		}

		if (hw->tx_descriptor) {
			status = dmaengine_tx_status(hw->dma_tx_channel,
						     tx_cookie, &state);
			if (status != DMA_COMPLETE)
				dev_err(hw->dev, "tx failed\n");
			hw->remaining_bytes = state.residue;
			hw->tx_descriptor = NULL;
		}

		/* if any remainder, transfer now */
		if (remain) {
			int count;

			hw->tx_ptr = t->tx_buf ? t->tx_buf + t->len - remain :
				     NULL;
			hw->rx_ptr = t->rx_buf ? t->rx_buf + t->len - remain :
				     NULL;
			hw->remaining_bytes = remain;
			init_completion(&hw->done);

			dspg_dmw_spi_disable_and_clear_ints(hw);
			dspg_dmw_spi_enable(hw);

			count = dspg_dmw_spi_fill_tx_fifo(hw);

			dspg_dmw_spi_enable_rx_wm_int(hw, count);
			wait_for_completion(&hw->done);
		}
	}

	dspg_dmw_spi_disable_and_clear_ints(hw);
	clk_disable_unprepare(hw->clk);

	return t->len - hw->remaining_bytes;
}

static void
dspg_dmw_spi_debug(struct dspg_dmw_spi *hw)
{
	int val;

	dev_info(hw->dev, "CFG_REG   = 0x%x\n",
		 readl(hw->regs + DMW_SPI_CFG_REG));
	dev_info(hw->dev, "RATE_CNTL = 0x%x\n",
		 readl(hw->regs + DMW_SPI_RATE_CNTL));
	dev_info(hw->dev, "DEL_VAL   = 0x%x\n",
		 readl(hw->regs + DMW_SPI_DEL_VAL));
	dev_info(hw->dev, "INT_EN =    0x%x\n",
		 readl(hw->regs + DMW_SPI_INT_EN));
	dev_info(hw->dev, "INT_STAT =  0x%x\n",
		 readl(hw->regs + DMW_SPI_INT_STAT));
	dev_info(hw->dev, "INT_CLR =   0x%x\n",
		 readl(hw->regs + DMW_SPI_INT_CLR));
	dev_info(hw->dev, "INT_CAUSE = 0x%x\n",
		 readl(hw->regs + DMW_SPI_INT_CAUSE));

	val = readl(hw->regs + DMW_SPI_DBG);

	dev_info(hw->dev, "TX_BUSY:       %d\n", (val & (0x1 << 31)) >> 31);
	dev_info(hw->dev, "SPI_FSM_CUR:   %d\n", (val & (0x3 << 18)) >> 18);
	dev_info(hw->dev, "SPI_FSM_NXT:   %d\n", (val & (0x3 << 16)) >> 16);
	dev_info(hw->dev, "TX_FIFO_LVL:   %d\n", (val & (0x1f << 9)) >> 9);
	dev_info(hw->dev, "TX_FIFO_EMPTY: %d\n", (val & (0x1 << 8)) >> 8);
	dev_info(hw->dev, "TX_FIFO_FULL:  %d\n", (val & (0x1 << 7)) >> 7);
	dev_info(hw->dev, "RX_FIFO_LVL:   %d\n", (val & (0x1f << 2)) >> 2);
	dev_info(hw->dev, "RX_FIFO_EMPTY: %d\n", (val & (0x1 << 1)) >> 1);
	dev_info(hw->dev, "RX_FIFO_FULL:  %d\n", val & 0x1);
}

irqreturn_t
dspg_dmw_spi_irq(int irq, void *dev)
{
	struct dspg_dmw_spi *hw = dev;
	int cause;
	unsigned char dat;

	cause = readl(hw->regs + DMW_SPI_INT_CAUSE);
	dspg_dmw_spi_disable_and_clear_ints(hw);

	if (cause & INT_RX_FIFO_WM) { /* A transmit has completed */
		while (dspg_dmw_spi_rx_not_empty(hw)) {
			/* Read out all the data from the Rx FIFO */
			dat = readl(hw->regs + DMW_SPI_RX_DAT);
			if (hw->rx_ptr)
				*hw->rx_ptr++ = dat;
		}

		if (hw->remaining_bytes > 0) {
			int count = dspg_dmw_spi_fill_tx_fifo(hw);

			dspg_dmw_spi_enable_rx_wm_int(hw, count);
		} else {
			/* No more data to send.
			 * Indicate the transfer is completed.
			 */

			complete(&hw->done);
		}
	} else {
		dev_err(hw->dev, "unhandled or ghost interrupt (0x%x)\n",
			cause);
		dspg_dmw_spi_debug(hw);
	}

	return IRQ_HANDLED;
}

static int
dspg_dmw_spi_probe(struct platform_device *pdev)
{
	struct dspg_dmw_spi *hw;
	struct spi_master *master;
	struct resource *res;
	int err = 0;
	u32 rate;
	struct device_node *np = pdev->dev.of_node;
	struct dma_slave_config slave_cfg;

	master = spi_alloc_master(&pdev->dev, sizeof(struct dspg_dmw_spi));
	if (master == NULL) {
		dev_err(&pdev->dev, "No memory for spi_master\n");
		return -ENOMEM;
	}

	master->bus_num = -1;
	master->num_chipselect = DMW_SPI_NUM_CHIPSELECT;
	master->dev.of_node = pdev->dev.of_node;

	hw = spi_master_get_devdata(master);
	memset(hw, 0, sizeof(struct dspg_dmw_spi));

	hw->master = spi_master_get(master);
	hw->dev = &pdev->dev;

	err = of_property_read_u32_array(np, "delays", &hw->delays[0], 16);
	if (err) {
		dev_err(&pdev->dev, "missing delay values\n");
		goto put_master;
	}

	platform_set_drvdata(pdev, hw);
	init_completion(&hw->done);

	/* setup the state for the bitbang driver */
	hw->bitbang.master         = hw->master;
	hw->bitbang.setup_transfer = dspg_dmw_spi_setupxfer;
	hw->bitbang.chipselect     = dspg_dmw_spi_chipsel;
	hw->bitbang.txrx_bufs      = dspg_dmw_spi_txrx;
	hw->bitbang.master->setup  = dspg_dmw_spi_setup;
	master->bus_num            = pdev->id;

	dev_dbg(hw->dev, "bitbang at %p\n", &hw->bitbang);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "Cannot get IORESOURCE_MEM\n");
		err = -ENOENT;
		goto put_master;
	}

	hw->bitbang.master->can_dma = dspg_dmw_spi_can_dma;
	hw->bitbang.master->dma_alignment = dma_get_cache_alignment();

	hw->dma_rx_channel = dma_request_slave_channel(hw->dev, "rx");
	if (!hw->dma_rx_channel) {
		dev_err(hw->dev, "could not get DMA channel for RX\n");
		goto put_master;
	}
	slave_cfg.direction = DMA_DEV_TO_MEM;
	slave_cfg.src_addr = res->start + DMW_SPI_RX_DAT;
	slave_cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	slave_cfg.src_maxburst = DMW_SPI_FIFO_SIZE / 4;
	err = dmaengine_slave_config(hw->dma_rx_channel, &slave_cfg);
	if (err) {
		dev_err(hw->dev,
			"could not configure DMA channel for RX: %d\n", err);
		goto put_master; // TODO
	}

	hw->dma_tx_channel = dma_request_slave_channel(hw->dev, "tx");
	if (!hw->dma_tx_channel) {
		dev_err(hw->dev, "could not get DMA channel for TX\n");
		goto put_master; // TODO
	}
	slave_cfg.direction = DMA_MEM_TO_DEV;
	slave_cfg.dst_addr = res->start + DMW_SPI_TX_DAT;
	slave_cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	slave_cfg.dst_maxburst = DMW_SPI_FIFO_SIZE / 4;
	err = dmaengine_slave_config(hw->dma_tx_channel, &slave_cfg);
	if (err) {
		dev_err(hw->dev,
			"could not configure DMA channel for TX: %d\n", err);
		goto put_master; // TODO
	}

	hw->regs = devm_ioremap_resource(&pdev->dev, res);
	if (hw->regs == NULL) {
		dev_err(&pdev->dev, "Cannot map IO\n");
		err = -ENXIO;
		goto put_master;
	}

	hw->irq = platform_get_irq(pdev, 0);
	if (hw->irq < 0) {
		dev_err(&pdev->dev, "No IRQ specified\n");
		err = -ENOENT;
		goto put_master;
	}

	hw->rc = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(hw->rc)) {
		dev_err(&pdev->dev, "Unable to obtain reset control\n");
		err = PTR_ERR(hw->rc);
		goto put_master;
	}

	err = reset_control_deassert(hw->rc);
	if (err)
		dev_err(&pdev->dev, "Unable to release reset control\n");

	hw->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(hw->clk)) {
		dev_err(&pdev->dev, "No clock for device\n");
		err = PTR_ERR(hw->clk);
		goto put_master;
	}

	err = of_property_read_u32(np, "clock-rate", &rate);
	if (!err) {
		dev_info(&pdev->dev, "setting clock to %uHz\n", rate);
		err = clk_set_rate(hw->clk, rate);
		if (err) {
			dev_err(&pdev->dev, "setting clock failed\n");
			goto put_master;
		}
	}
	clk_prepare_enable(hw->clk);

	dspg_dmw_spi_disable_and_clear_ints(hw);

	err = devm_request_irq(&pdev->dev, hw->irq, dspg_dmw_spi_irq, 0,
			       DRIVER_NAME, hw);
	if (err) {
		dev_err(&pdev->dev, "Cannot claim IRQ\n");
		goto put_master;
	}

	/* register our spi controller */
	err = spi_bitbang_start(&hw->bitbang);
	if (err) {
		dev_err(&pdev->dev, "Failed to register SPI master\n");
		goto put_master;
	}

	clk_disable_unprepare(hw->clk);

	dev_info(&pdev->dev, "successfully registered\n");
	return 0;

put_master:
	if (hw->dma_tx_channel)
		dma_release_channel(hw->dma_tx_channel);

	if (hw->dma_rx_channel)
		dma_release_channel(hw->dma_rx_channel);

	spi_master_put(master);

	return err;
}

static int
dspg_dmw_spi_remove(struct platform_device *dev)
{
	struct dspg_dmw_spi *hw = platform_get_drvdata(dev);
	unsigned long val;

	platform_set_drvdata(dev, NULL);

	spi_unregister_master(hw->master);

	val = readl(hw->regs + DMW_SPI_CFG_REG);
	val &= ~(CFG_EN);
	writel(val, hw->regs + DMW_SPI_CFG_REG);

	clk_disable_unprepare(hw->clk);

	reset_control_put(hw->rc);

	spi_master_put(hw->master);

	return 0;
}

static const struct of_device_id spi_dspg_dmw_dt_ids[] = {
	{ .compatible = "dspg,spi-dmw", },
	{ /* sentinel */ }
};

static struct platform_driver dspg_dmw_spidrv = {
	.probe		= dspg_dmw_spi_probe,
	.remove		= dspg_dmw_spi_remove,

	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = spi_dspg_dmw_dt_ids,
	},
};

static int __init
dspg_dmw_spi_init(void)
{
	return platform_driver_register(&dspg_dmw_spidrv);
}

static void __exit
dspg_dmw_spi_exit(void)
{
	platform_driver_unregister(&dspg_dmw_spidrv);
}

subsys_initcall(dspg_dmw_spi_init);
module_exit(dspg_dmw_spi_exit);

MODULE_DESCRIPTION("DSPG DMW style SPI Driver");
MODULE_AUTHOR("Andreas Weissel <andreas.weissel@dpsg.com>");
MODULE_LICENSE("GPL");
