/*
 * spi-dspg-dcx.c - SPI master driver for the DCX style controller
 *
 * Copyright (C) 2013 DSPG Technologies GmbH
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
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/spi/spi_dspg_dcx.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include <mach/gpio.h>

#define DRIVER_NAME "spi-dspg-dcx"

#define DCX_SPI_GLOBAL		0x00
#define DCX_SPI_CON		0x04
#define DCX_SPI_FRM		0x08
#define DCX_SPI_IER		0x0c
#define DCX_SPI_STAT		0x10
#define DCX_SPI_DAT		0x14

/* SPI_GLOBAL register */
#define GLOBAL_FIFO_PTR_RES	(1<<2)
#define GLOBAL_BLRES		(1<<1)

/* SPI_CON register */
#define CON_RX_THR		(1<<15)
#define CON_TX_THR		(1<<14)
#define CON_SHIFT_OFF		(1<<13)
#define CON_SPI_MODE		(1<<8)
#define CON_MS			(1<<7)

/* SPI_IER register */
#define IER_INTCS		(1<<7)
#define IER_INTEOT		(1<<6)
#define IER_INTRBT		(1<<4)
#define IER_INTTBT		(1<<1)

/* SPI_STAT register */
#define STAT_BUSY		(1<<6)
#define STAT_RBF		(1<<5)
#define STAT_RBT		(1<<4)
#define STAT_RBE		(1<<3)
#define STAT_TBF		(1<<2)
#define STAT_TBT		(1<<1)
#define STAT_TBE		(1<<0)

#define DCX_SPI_FIFO_SIZE	32
#define DCX_SPI_THRESHOLD	8

struct dspg_dcx_spi {
	/* bitbang has to be first */
	struct spi_bitbang	 bitbang;
	struct completion	 done;

	void __iomem		*regs;
	int			 irq;
	int			 remaining_bytes;
	int			 current_cs_gpio;
	unsigned short		 con;
	unsigned short		 frm;
	unsigned long		 delay;

	/* data buffers */
	const unsigned short	*tx_wptr;
	const unsigned char	*tx_ptr;
	unsigned short		*rx_wptr;
	unsigned char		*rx_ptr;
	int			n_bytes;

	struct clk		*clk;
	struct reset_control	*reset;
	struct resource		*ioarea;
	struct spi_master	*master;

	struct device		*dev;
};

static inline struct dspg_dcx_spi *
to_hw(struct spi_device *sdev)
{
	return spi_master_get_devdata(sdev->master);
}

static void
dspg_dcx_spi_chipsel(struct spi_device *spi, int is_on)
{
	struct dspg_dcx_spi *hw = to_hw(spi);

	if (hw->current_cs_gpio == -1)
		return;

	if (is_on)
		gpio_set_value(hw->current_cs_gpio, 0);
	else
		gpio_set_value(hw->current_cs_gpio, 1);
}

static int
dspg_dcx_spi_setupxfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct dspg_dcx_spi *hw = to_hw(spi);
	unsigned int bpw;
	unsigned long clk_rate = clk_get_rate(hw->clk);
	unsigned int hz, div;
	unsigned short con;

	bpw = t ? t->bits_per_word : 0;
	if (!bpw)
		bpw = spi->bits_per_word;
	hz  = t ? t->speed_hz : 0;
	if (!hz)
		hz = spi->max_speed_hz;

	/* this is due to an unknown issue in the SPI block, frequencies equal
	 * or below 1MHz cause the block to fail
	 */
	if (hz <= 1000000) {
		dev_err(&spi->dev, "invalid speed value (%dHz)\n", hz);
		return -EINVAL;
	}

	dev_dbg(&spi->dev, "hz = %u, clockrate = %lu\n", hz, clk_rate);

	if (hz > spi->max_speed_hz) {
		dev_err(&spi->dev, "speed values > %dHz not supported\n",
			spi->max_speed_hz);
		return -EINVAL;
	}

	if ((bpw < 3) || (bpw > 16)) {
		dev_err(&spi->dev, "invalid bits-per-word (%d)\n", bpw);
		return -EINVAL;
	}

	if ((spi->mode == SPI_MODE_1) || (spi->mode == SPI_MODE_2)) {
		dev_err(&spi->dev, "invalid mode\n");
		return -EINVAL;
	}

	/* double the clock rate as the internal rate is divided by 2 */
	clk_rate = clk_round_rate(hw->clk, hz*2);

	/* set internal divider */
	div = DIV_ROUND_UP(clk_rate, hz) / 2;
	if (div == 0)
		div = 1;
	else if (div > 0x7f)
		div = 0x7f;

	clk_set_rate(hw->clk, clk_rate);
	/* half SPI clock cycle; use double value to round up */
	hw->delay = 1000000UL / (clk_rate / (div * 2));

	hw->current_cs_gpio = (int)spi->controller_data;

	clk_enable(hw->clk);

	con = CON_MS | ((bpw-1) << 9) | div;
	if (spi->mode == SPI_MODE_3)
		con |= CON_SPI_MODE;
	hw->con = con;
	writew(hw->con, hw->regs + DCX_SPI_CON);

	clk_disable(hw->clk);

	dev_dbg(&spi->dev, "setting pre-scaler to %lu (hz %u)\n", clk_rate,
		hz);

	return 0;
}

static int
dspg_dcx_spi_setup(struct spi_device *spi)
{
	int ret;

	if (!spi->bits_per_word)
		spi->bits_per_word = 8;

	ret = dspg_dcx_spi_setupxfer(spi, NULL);
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
dspg_dcx_spi_disable_ints(struct dspg_dcx_spi *hw)
{
	writew(0x0000, hw->regs + DCX_SPI_IER);
}

static void
dspg_dcx_spi_set_frame_count(struct dspg_dcx_spi *hw, int count)
{
	if (count > 1)
		writew(count, hw->regs + DCX_SPI_FRM);
	else
		writew(0, hw->regs + DCX_SPI_FRM);

	hw->frm = count;
}

static void
dspg_dcx_spi_enable_ints(struct dspg_dcx_spi *hw, unsigned short ints)
{
	writew(ints, hw->regs + DCX_SPI_IER);
}

static void
dspg_dcx_spi_tx_fifo(struct dspg_dcx_spi *hw, int count)
{
	int i;
	unsigned short dat = 0;

	for (i = 0; i < count; i++) {
		if (hw->tx_wptr)
			dat = *hw->tx_wptr++;
		else if (hw->tx_ptr)
			dat = *hw->tx_ptr++;
		writew(dat, hw->regs + DCX_SPI_DAT);
	}
}

static void
dspg_dcx_spi_rx_fifo(struct dspg_dcx_spi *hw, int count)
{
	int i;
	unsigned short dat = 0;

	for (i = 0; i < count; i++) {
		dat = readw(hw->regs + DCX_SPI_DAT);
		if (hw->rx_wptr)
			*hw->rx_wptr++ = dat;
		else if (hw->rx_ptr)
			*hw->rx_ptr++ = dat & 0xff;
	}
}

static int
dspg_dcx_spi_txrx(struct spi_device *spi, struct spi_transfer *t)
{
	struct dspg_dcx_spi *hw = to_hw(spi);
	unsigned int bpw;
	int count;

	bpw = t->bits_per_word;
	if (!bpw)
		bpw = spi->bits_per_word;

	if (bpw > 8) {
		hw->tx_wptr = t->tx_buf;
		hw->rx_wptr = t->rx_buf;
		hw->tx_ptr = NULL;
		hw->rx_ptr = NULL;
		hw->n_bytes = 2;
	} else {
		hw->tx_wptr = NULL;
		hw->rx_wptr = NULL;
		hw->tx_ptr = t->tx_buf;
		hw->rx_ptr = t->rx_buf;
		hw->n_bytes = 1;
	}
	hw->remaining_bytes = t->len;
	count = hw->remaining_bytes / hw->n_bytes;
	reinit_completion(&hw->done);

	dev_dbg(&spi->dev, "transfer %d frames (%s %s)\n", t->len,
		t->tx_buf ? "tx" : "  ", t->rx_buf ? "rx" : "  ");

	clk_enable(hw->clk);

	if (count > DCX_SPI_FIFO_SIZE)
		count = DCX_SPI_FIFO_SIZE;

	hw->remaining_bytes -= count * hw->n_bytes;
	dspg_dcx_spi_set_frame_count(hw, count);
	dspg_dcx_spi_enable_ints(hw, IER_INTEOT);
	dspg_dcx_spi_tx_fifo(hw, count);

	if (wait_for_completion_timeout(&hw->done, HZ) == 0) {
		dev_err(&spi->dev,
		"timeout during SPI transfer (%d/%d bytes (%s %s), stat 0x%x\n",
			t->len,
			hw->remaining_bytes,
			t->tx_buf ? "tx" : "  ",
			t->rx_buf ? "rx" : "  ",
			readw(hw->regs + DCX_SPI_STAT));
	}

	clk_disable(hw->clk);

	return t->len - hw->remaining_bytes;
}

irqreturn_t
dspg_dcx_spi_irq(int irq, void *dev)
{
	struct dspg_dcx_spi *hw = dev;
	int count;

	dspg_dcx_spi_disable_ints(hw);

	/* wait after EOT interrupt */
	if (hw->delay)
		udelay(hw->delay);

	/* read rx fifo without enabling the bus clock */
	writew(hw->con | CON_SHIFT_OFF, hw->regs + DCX_SPI_CON);
	dspg_dcx_spi_rx_fifo(hw, hw->frm);

	if (hw->remaining_bytes != 0) {
		count = hw->remaining_bytes / hw->n_bytes;

		if (count > DCX_SPI_FIFO_SIZE)
			count = DCX_SPI_FIFO_SIZE;

		dspg_dcx_spi_set_frame_count(hw, count);
		dspg_dcx_spi_enable_ints(hw, IER_INTEOT);
		dspg_dcx_spi_tx_fifo(hw, count);

		hw->remaining_bytes -= count * hw->n_bytes;
	} else {
		complete(&hw->done);
	}

	return IRQ_HANDLED;
}

static int
dspg_dcx_spi_probe(struct platform_device *pdev)
{
	struct dspg_dcx_spi *hw;
	struct spi_master *master;
	struct resource *res;
	int err = 0;

	master = spi_alloc_master(&pdev->dev, sizeof(struct dspg_dcx_spi));
	if (master == NULL) {
		dev_err(&pdev->dev, "No memory for spi_master\n");
		return -ENOMEM;
	}

	master->bus_num = -1;
	master->num_chipselect = 255;
	master->dev.of_node = pdev->dev.of_node;

	hw = spi_master_get_devdata(master);
	memset(hw, 0, sizeof(struct dspg_dcx_spi));

	hw->master = spi_master_get(master);
	hw->dev = &pdev->dev;

	if (pdev->dev.platform_data) {
		struct spi_dspg_dcx_info *pdata = pdev->dev.platform_data;

		master->bus_num = pdata->bus_num;
	}

	platform_set_drvdata(pdev, hw);
	init_completion(&hw->done);

	/* setup the state for the bitbang driver */
	hw->bitbang.master         = hw->master;
	hw->bitbang.setup_transfer = dspg_dcx_spi_setupxfer;
	hw->bitbang.chipselect     = dspg_dcx_spi_chipsel;
	hw->bitbang.txrx_bufs      = dspg_dcx_spi_txrx;
	hw->bitbang.master->setup  = dspg_dcx_spi_setup;
	hw->bitbang.master->dev.of_node = pdev->dev.of_node;

	dev_dbg(hw->dev, "bitbang at %p\n", &hw->bitbang);

	/* find and map our resources */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Cannot get memory resource\n");
		err = -ENOENT;
		goto put_master;
	}

	hw->ioarea = devm_request_mem_region(&pdev->dev, res->start,
					(res->end - res->start)+1,
					DRIVER_NAME);

	if (hw->ioarea == NULL) {
		dev_err(&pdev->dev, "Cannot reserve region\n");
		err = -ENXIO;
		goto put_master;
	}

	hw->regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (hw->regs == NULL) {
		dev_err(&pdev->dev, "Cannot map IO\n");
		err = -ENXIO;
		goto put_master;
	}

	hw->irq = platform_get_irq(pdev, 0);
	if (hw->irq < 0) {
		dev_err(&pdev->dev, "No IRQ specified\n");
		goto put_master;
	}

	hw->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(hw->clk)) {
		dev_err(&pdev->dev, "No clock for device\n");
		err = PTR_ERR(hw->clk);
		goto put_master;
	}
	clk_prepare(hw->clk);

	hw->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(hw->reset)) {
		dev_err(&pdev->dev, "cannot get reset control\n");
		err = PTR_ERR(hw->reset);
		goto put_master;
	}

	err = reset_control_deassert(hw->reset);
	if (err) {
		dev_err(&pdev->dev, "cannot deassert reset\n");
		goto put_master;
	}

	err = devm_request_irq(&pdev->dev, hw->irq, dspg_dcx_spi_irq,
			       IRQF_TRIGGER_RISING,
			       DRIVER_NAME, hw);
	if (err) {
		dev_err(&pdev->dev, "Cannot claim IRQ\n");
		goto put_master;
	}
	irq_set_irq_type(hw->irq, IRQ_TYPE_EDGE_RISING);

	clk_enable(hw->clk);

	writew(GLOBAL_BLRES | GLOBAL_FIFO_PTR_RES, hw->regs + DCX_SPI_GLOBAL);
	while (readw(hw->regs + DCX_SPI_GLOBAL))
		;
	udelay(100);

	clk_disable(hw->clk);

	/* register our spi controller */
	err = spi_bitbang_start(&hw->bitbang);
	if (err) {
		dev_err(&pdev->dev, "Failed to register SPI master\n");
		return err;
	}

	return 0;

put_master:
	spi_master_put(master);

	return err;
}

static int
dspg_dcx_spi_remove(struct platform_device *dev)
{
	struct dspg_dcx_spi *hw = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);

	spi_unregister_master(hw->master);

	clk_unprepare(hw->clk);
	clk_put(hw->clk);
	reset_control_put(hw->reset);

	free_irq(hw->irq, hw);
	iounmap(hw->regs);

	release_resource(hw->ioarea);
	kfree(hw->ioarea);

	spi_master_put(hw->master);

	return 0;
}

static const struct of_device_id spi_dspg_dcx_dt_ids[] = {
	{ .compatible = "dspg,spi-dcx", },
	{ /* sentinel */ }
};

static struct platform_driver dspg_dcx_spidrv = {
	.probe		= dspg_dcx_spi_probe,
	.remove		= dspg_dcx_spi_remove,

	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = spi_dspg_dcx_dt_ids,
	},
};

static int __init
dspg_dcx_spi_init(void)
{
	return platform_driver_register(&dspg_dcx_spidrv);
}

static void __exit
dspg_dcx_spi_exit(void)
{
	platform_driver_unregister(&dspg_dcx_spidrv);
}

subsys_initcall(dspg_dcx_spi_init);
module_exit(dspg_dcx_spi_exit);

MODULE_DESCRIPTION("DSPG DCX style SPI Driver");
MODULE_AUTHOR("Andreas Weissel <andreas.weissel@dpsg.com>");
MODULE_LICENSE("GPL");
