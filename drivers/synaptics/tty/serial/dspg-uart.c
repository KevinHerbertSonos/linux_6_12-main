/*
 *  linux/drivers/tty/serial/dspg-uart.c
 *
 *  Driver for DSPG DW style serial ports
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  Copyright (C) 2009 DSP Group Inc.
 *  Copyright (C) 2016 DSPG Technologies GmbH
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if defined(CONFIG_SERIAL_DSPG_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/clk.h>
#include <linux/console.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/reset.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>

#include <linux/io.h>
//#include <linux/uaccess.h>

#include <mach/hardware.h>

#define NR_PORTS			4

#ifdef CONFIG_SERIAL_DSPG_TTYDW

#define SERIAL_DVF_MAJOR		254
#define SERIAL_DVF_MINOR		74
#define SERIAL_DVF_NAME			"ttyDW"

#else

#define SERIAL_DVF_MAJOR		TTY_MAJOR
#define SERIAL_DVF_MINOR		64
#define SERIAL_DVF_NAME			"ttyS"

#endif

#define UART_DUMMY_RSR_RX		256
#define UART_FIFO_SIZE			128

#define TCSETRXWM			0x9901
#define TCSETTXWM			0x9902
#define TCGETRXWM			0x9911
#define TCGETTXWM			0x9912

#define DVF_UART_CTL			0x00
#define DVF_UART_CFG			0x04
#define DVF_UART_INT_DIV		0x08
#define DVF_UART_FRAC_DIV		0x0C
#define DVF_UART_TX_WM			0x10
#define DVF_UART_RX_WM			0x14
#define DVF_UART_FIFO_ISR		0x18
#define DVF_UART_FIFO_IER		0x1C
#define DVF_UART_FIFO_ICR		0x20
#define DVF_UART_TX_LVL			0x24
#define DVF_UART_RX_LVL			0x28
#define DVF_UART_TX_DATA		0x2C
#define DVF_UART_RX_DATA		0x30
#define DVF_UART_STAT			0x34

/* Enable the UART block operation. */
#define DVF_UART_CTL_UART_EN		(1 << 0)
/* Disable the UART block operation. */
#define DVF_UART_CTL_UART_DIS		(0 << 0)
/* Reset the UART block operation. */
#define DVF_UART_RESET			(1 << 7)

#define DVF_UART_CFG_DATA_LENGTH_MASK	((1 << 2) | (1 << 3))
/* Data is 8 bits length. */
#define DVF_UART_CFG_DATA_LENGTH_8_VAL	(0 << 2)
/* Data is 7 bits length. */
#define DVF_UART_CFG_DATA_LENGTH_7_VAL	(1 << 2)
/* Data is 9 bits length. */
#define DVF_UART_CFG_DATA_LENGTH_9_VAL	(1 << 3)
/* Number of stop bits. 0=1bit, 1=2bits */
#define DVF_UART_CFG_STOP_CFG		(1 << 4)
/* Parity. 0=even, 1=odd */
#define DVF_UART_CFG_PAR_OD_EVEN	(1 << 5)
/* Parity enabled/disabled. */
#define DVF_UART_CFG_PAR_EN		(1 << 6)
#define DVF_UART_CFG_SW_RST		(1 << 7)
#define DVF_UART_CFG_OD_EN		(1 << 8)
/* RTSN flow control enabled/disabled. */
#define DVF_UART_CFG_RTSN_EN		(1 << 9)
/* CTSN flow control enabled/disabled. */
#define DVF_UART_CFG_CTSN_EN		(1 << 10)
/* BREAK detection logic enabled/disabled. */
#define DVF_UART_CFG_BREAK_EN		(1 << 11)
#define DVF_UART_CFG_IRDA_DTX_INV	(1 << 13)
#define DVF_UART_CFG_IRDA_DRX_INV	(1 << 14)
#define DVF_UART_CFG_IRDA_UART		(1 << 15)

#define DVF_UART_DIV_FRAC_RATE_LOC	(12)
#define DVF_UART_DIV_INT_RATE_MASK	(0x3FF)

/* Tx FIFO over-run interrupt status. */
#define DVF_UART_TX_FIFO_OVER_ISR	(1 << 0)
/* Tx FIFO watermark interrupt status. */
#define DVF_UART_TX_FIFO_WTR_ISR	(1 << 1)
/* Rx FIFO under-run interrupt status. */
#define DVF_UART_RX_FIFO_UNDER_ISR	(1 << 8)
/* Rx FIFO over-run interrupt status. */
#define DVF_UART_RX_FIFO_OVER_ISR	(1 << 9)
/* Rx FIFO watermark interrupt status. */
#define DVF_UART_RX_FIFO_WTR_ISR	(1 << 10)
/* Rx FIFO timeout interrupt status. */
#define DVF_UART_RX_FIFO_TIMEOUT_ISR	(1 << 11)
/* Rx FIFO BREAK interrupt status. */
#define DVF_UART_RX_BREAK_ISR		(1 << 12)

/* Tx FIFO over-run interrupt enable. */
#define DVF_UART_TX_FIFO_OVER_IER	(1 << 0)
/* Tx FIFO watermark interrupt enable. */
#define DVF_UART_TX_FIFO_WTR_IER	(1 << 1)
/* Rx FIFO under-run interrupt enable. */
#define DVF_UART_RX_FIFO_UNDER_IER	(1 << 8)
/* Rx FIFO over-run interrupt enable. */
#define DVF_UART_RX_FIFO_OVER_IER	(1 << 9)
/* Rx FIFO watermark interrupt enable. */
#define DVF_UART_RX_FIFO_WTR_IER	(1 << 10)
/* Rx FIFO timeout interrupt enable. */
#define DVF_UART_RX_FIFO_TIMEOUT_IER	(1 << 11)
/* Rx FIFO BREAK interrupt enable. */
#define DVF_UART_RX_BREAK_IER		(1 << 12)
#define DVF_UART_STARTUP_IER		(DVF_UART_RX_FIFO_OVER_IER | \
					 DVF_UART_RX_FIFO_WTR_IER | \
					 DVF_UART_RX_FIFO_TIMEOUT_IER | \
					 DVF_UART_RX_BREAK_IER)

#define DVF_UART_TX_IER			(DVF_UART_TX_FIFO_OVER_IER | \
					 DVF_UART_TX_FIFO_WTR_IER)
#define DVF_UART_RX_OE_BE_IER		(DVF_UART_RX_FIFO_OVER_IER | \
					 DVF_UART_RX_BREAK_IER)

/* Tx FIFO over-run interrupt clear.*/
#define DVF_UART_TX_FIFO_OVER_ICR	(1 << 0)
/* Rx FIFO under-run interrupt clear.*/
#define DVF_UART_RX_FIFO_UNDER_ICR	(1 << 8)
/* Rx FIFO over-run interrupt clear.*/
#define DVF_UART_RX_FIFO_OVER_ICR	(1 << 9)
/* Rx BREAK interrupt clear.*/
#define DVF_UART_RX_BREAK_ICR		(1 << 12)
#define DVF_UART_CLR_ICR		(DVF_UART_TX_FIFO_OVER_ICR | \
					 DVF_UART_RX_FIFO_UNDER_ICR | \
					 DVF_UART_RX_FIFO_OVER_ICR | \
					 DVF_UART_RX_BREAK_ICR)

/* Tx FIFO current level bits. Spreads on bits [0 : Log2(fifo_depth)]. */
#define DVF_UART_TX_FIFO_LEVEL_MASK	(0x1F)
/* Rx FIFO current level bits. Spreads on bits [0 : Log2(fifo_depth)]. */
#define DVF_UART_RX_FIFO_LEVEL_MASK	(0x1F)

/* UART word from Rx FIFO - received word. */
#define DVF_UART_RX_DATA_WORD		(0x1FF)
/* UART word from Rx FIFO - Parity error bit. */
#define DVF_UART_RX_DATA_PE		(0x200)
/* UART word from Rx FIFO - frame error bit. */
#define DVF_UART_RX_DATA_FE		(0x400)
#define DVF_UART_RX_PE_FE		(DVF_UART_RX_DATA_PE | \
					 DVF_UART_RX_DATA_FE)

/* Tx FIFO completely empty. */
#define DVF_UART_STAT_TX_EMPTY		(1 << 0)
/* Tx FIFO completely full. */
#define DVF_UART_STAT_TX_FULL		(1 << 1)
/* The inverted(!) values of the RTS port. */
#define DVF_UART_STAT_RTS		(1 << 2)
/* The inverted(!) values of the CTS port. */
#define DVF_UART_STAT_CTS		(1 << 3)
/* Rx FIFO completely empty. */
#define DVF_UART_STAT_RX_EMPTY		(1 << 8)
/* Rx FIFO completely full. */
#define DVF_UART_STAT_RX_FULL		(1 << 9)
/* Rx BREAK was detected. */
#define DVF_UART_STAT_BREAK_DETECTED	(1 << 10)

#define DVF_UART_STAT_MASK_LOC_0	(9)
#define DVF_UART_STAT_MASK_LOC_1	(11)
#define DVF_UART_STAT_MASK_LOC_2_3	(7)

#define DVF_NAME_LEN	10

/* arbitrary timeout of 100ms */
#define DVF_UART_TX_TIMEOUT_US		100000

struct dvf_port {
	struct uart_port port;
	struct clk *clk;
	struct reset_control *rc;
	struct tasklet_struct tasklet;
	struct notifier_block clk_change_nb;
	bool should_stop;
	char name[DVF_NAME_LEN];
};

#define nb_to_dvf_port(x)	container_of(x, struct dvf_port, \
					     clk_change_nb)

struct dvf_baudrate {
	unsigned int intgr;
	unsigned int frac;
};

static struct dvf_port dvf_ports[NR_PORTS];
static struct device_node *dvf_uart_nodes[NR_PORTS];

static void dvf_uart_of_enumerate(void);

static void dvf_wait_one_char(unsigned int jiffies, unsigned int fifo_size)
{
	unsigned int char_timeout = jiffies_to_usecs(jiffies) / fifo_size;

	if (char_timeout > 1000)
		mdelay(char_timeout / 1000);
	else
		udelay(char_timeout);
}

static void dvf_uart_get_div(struct uart_port *port, int *intgr, int *frac)
{
	*intgr = readw(port->membase + DVF_UART_INT_DIV);
	*frac  = readw(port->membase + DVF_UART_FRAC_DIV) & 0xf;
}

static void dvf_uart_set_div(struct uart_port *port, int intgr, int frac)
{
	writew(intgr & 0xffff, port->membase + DVF_UART_INT_DIV);
	writew(frac  & 0x000f, port->membase + DVF_UART_FRAC_DIV);
}

static void dvf_uart_stop_tx(struct uart_port *port)
{
	struct dvf_port *uap = (struct dvf_port *)port;
	unsigned int ier, status;

	if (readw_poll_timeout(uap->port.membase + DVF_UART_STAT, status,
			       status & DVF_UART_STAT_TX_EMPTY, 0,
			       DVF_UART_TX_TIMEOUT_US))
		dev_warn(port->dev, "transmit timeout\n");

	ier = readw(uap->port.membase + DVF_UART_FIFO_IER);
	ier &= ~(DVF_UART_TX_FIFO_OVER_IER|DVF_UART_TX_FIFO_WTR_IER);
	writew(ier, uap->port.membase + DVF_UART_FIFO_IER);
}

static void dvf_uart_start_tx(struct uart_port *port)
{
	struct dvf_port *uap = (struct dvf_port *)port;
	unsigned int ier;

	ier = readw(uap->port.membase + DVF_UART_FIFO_IER);
	ier |= (DVF_UART_TX_FIFO_OVER_IER|DVF_UART_TX_FIFO_WTR_IER);
	writew(ier, uap->port.membase + DVF_UART_FIFO_IER);
}

static void dvf_uart_stop_rx(struct uart_port *port)
{
	struct dvf_port *uap = (struct dvf_port *)port;
	unsigned int mask;

	mask = readw(uap->port.membase + DVF_UART_FIFO_IER);
	mask &= ~(DVF_UART_RX_FIFO_UNDER_IER | DVF_UART_RX_FIFO_OVER_IER |
		  DVF_UART_RX_FIFO_WTR_IER | DVF_UART_RX_FIFO_TIMEOUT_IER |
		  DVF_UART_RX_BREAK_IER);
	writew(mask, uap->port.membase + DVF_UART_FIFO_IER);
}

static void dvf_uart_enable_ms(struct uart_port *port)
{
	/* intentionally left blank */
}

static void
dvf_uart_rx_chars(struct dvf_port *uap)
{
	struct tty_port *tty_port = &uap->port.state->port;
	unsigned int status, ch, flag, rx_err, rx_err_m, pf_err, pf_err_m;
	unsigned int max_count = 256;
	unsigned int value_m;

	status = readw(uap->port.membase + DVF_UART_STAT);
	while ((status & DVF_UART_STAT_RX_EMPTY) == 0 && max_count--) {
		ch = readw(uap->port.membase + DVF_UART_RX_DATA);
		flag = TTY_NORMAL;
		uap->port.icount.rx++;

		/*
		 * Note that the error handling code is
		 * out of the main execution path
		 */
		rx_err = readw(uap->port.membase + DVF_UART_FIFO_ISR);
		rx_err &= DVF_UART_RX_OE_BE_IER;

		pf_err = ch & DVF_UART_RX_PE_FE;

		/*
		 * Join error indicaion bits to one word:
		 *   IER - over-run (bit 9), break (bit 12)
		 *   RX_DATA - parity (bit 9), frame (bit 10)
		 *   rx_err_m -   X  |   X  |break|over-run
		 *                3  |   2  |  1  |    0
		 *   pf_err_m - frame|parity|  X  |    X
		 *                3  |   2  |  1  |    0
		 */
		rx_err_m = (rx_err & DVF_UART_RX_FIFO_OVER_IER) >>
				DVF_UART_STAT_MASK_LOC_0;

		rx_err_m |= (rx_err & DVF_UART_RX_BREAK_IER) >>
				DVF_UART_STAT_MASK_LOC_1;

		rx_err_m &= uap->port.read_status_mask;

		pf_err_m = (pf_err & DVF_UART_RX_PE_FE) >>
				DVF_UART_STAT_MASK_LOC_2_3;

		pf_err_m &= uap->port.read_status_mask;

		if (unlikely(rx_err || pf_err)) {
			if (rx_err & DVF_UART_RX_BREAK_IER) {
				pf_err &= ~DVF_UART_RX_PE_FE;
				uap->port.icount.brk++;
				if (uart_handle_break(&uap->port))
					goto ignore_char;
			} else if (pf_err & DVF_UART_RX_DATA_PE)
				uap->port.icount.parity++;
			else if (pf_err & DVF_UART_RX_DATA_FE)
				uap->port.icount.frame++;
			if (rx_err & DVF_UART_RX_FIFO_OVER_IER)
				uap->port.icount.overrun++;

			if (rx_err_m & (DVF_UART_RX_BREAK_IER >>
						DVF_UART_STAT_MASK_LOC_1))
				flag = TTY_BREAK;

			else if (pf_err_m & (DVF_UART_RX_DATA_PE >>
						DVF_UART_STAT_MASK_LOC_2_3))
				flag = TTY_PARITY;

			else if (pf_err_m & (DVF_UART_RX_DATA_FE >>
						DVF_UART_STAT_MASK_LOC_2_3))
				flag = TTY_FRAME;
		}

		if (uart_handle_sysrq_char(&uap->port, ch & 0xff))
			goto ignore_char;

		value_m = (rx_err_m & pf_err_m) & uap->port.ignore_status_mask;
		if ((value_m & ~DVF_UART_RX_FIFO_OVER_IER) == 0)
			tty_insert_flip_char(tty_port, ch, flag);

		value_m = (rx_err_m & pf_err_m) & ~uap->port.ignore_status_mask;
		if (value_m & DVF_UART_RX_FIFO_OVER_IER) {
			/*
			 * Overrun is special, since it's reported
			 * immediately, and doesn't affect the current
			 * character
			 */
			tty_insert_flip_char(tty_port, 0, TTY_OVERRUN);
		}

ignore_char:
		status = readw(uap->port.membase + DVF_UART_STAT);
	}
	tty_flip_buffer_push(tty_port);
}

static void dvf_uart_tx_chars(struct dvf_port *uap)
{
	struct circ_buf *xmit = &uap->port.state->xmit;
	int count;

	if (uap->port.x_char) {
		writew(uap->port.x_char, uap->port.membase + DVF_UART_TX_DATA);
		uap->port.icount.tx++;
		uap->port.x_char = 0;
		return;
	}

	if (uart_circ_empty(xmit) || uart_tx_stopped(&uap->port)) {
		dvf_uart_stop_tx(&uap->port);
		return;
	}

	count = uap->port.fifosize >> 1;
	do {
		writew(xmit->buf[xmit->tail],
		       uap->port.membase + DVF_UART_TX_DATA);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		uap->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&uap->port);

	if (uart_circ_empty(xmit))
		dvf_uart_stop_tx(&uap->port);
}

static void dvf_uart_tasklet_func(unsigned long data)
{
	struct dvf_port *uap = (struct dvf_port *)data;
	unsigned int status, pass_counter = 256;

	spin_lock(&uap->port.lock);

	if ((readw(uap->port.membase + DVF_UART_STAT)) & 0xc)
		pr_info("\nCTS-RTS port is LOW\n");

	status = readw(uap->port.membase + DVF_UART_FIFO_ISR);
	status &= readw(uap->port.membase + DVF_UART_FIFO_IER);

	while (status && !uap->should_stop) {
		/*
		 * rx characters:
		 *  1) above watermark
		 *  2) time out reading from FIFO
		 *  3) over-run
		 *  4) BREAK detection
		 */
		if (status & (DVF_UART_RX_FIFO_WTR_IER |
			      DVF_UART_RX_FIFO_TIMEOUT_IER |
			      DVF_UART_RX_FIFO_OVER_IER |
			      DVF_UART_RX_BREAK_IER)) {
			dvf_uart_rx_chars(uap);
			if (status & DVF_UART_RX_FIFO_OVER_IER)
				writew(DVF_UART_RX_FIFO_OVER_ICR,
				       uap->port.membase + DVF_UART_FIFO_ICR);

			if (status & DVF_UART_RX_BREAK_IER)
				writew(DVF_UART_RX_BREAK_ICR,
				       uap->port.membase + DVF_UART_FIFO_ICR);
		}

		/*
		 * tx characters:
		 *  1) below watermark
		 *  2) over-run
		 */
		if (status & (DVF_UART_TX_FIFO_WTR_IER |
			      DVF_UART_TX_FIFO_OVER_IER)) {
			dvf_uart_tx_chars(uap);

			if (status & DVF_UART_TX_FIFO_OVER_IER)
				writew(DVF_UART_TX_FIFO_OVER_ICR,
				       uap->port.membase + DVF_UART_FIFO_ICR);
		}

		if (pass_counter-- == 0)
			break;

		status = readw(uap->port.membase + DVF_UART_FIFO_ISR);
		status &= readw(uap->port.membase + DVF_UART_FIFO_IER);
	}

	enable_irq(uap->port.irq);
	spin_unlock(&uap->port.lock);
}


static irqreturn_t dvf_uart_interrupt(int irq, void *dev_id)
{
	struct dvf_port *uap = dev_id;

	disable_irq_nosync(uap->port.irq);
	tasklet_schedule(&uap->tasklet);

	return IRQ_HANDLED;
}

static unsigned int dvf_uart_tx_empty(struct uart_port *port)
{
	struct dvf_port *uap = (struct dvf_port *)port;

	unsigned int status = readw(uap->port.membase + DVF_UART_STAT);

	return (status & DVF_UART_STAT_TX_EMPTY) ? TIOCSER_TEMT : 0;
}

static unsigned int dvf_uart_get_mctrl(struct uart_port *port)
{
	struct dvf_port *uap = (struct dvf_port *)port;
	unsigned int result = 0;
	unsigned int status = readw(uap->port.membase + DVF_UART_STAT);

	if (!(status & DVF_UART_STAT_CTS))
		result |= TIOCM_CTS;

	return result;
}

static void dvf_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct dvf_port *uap = (struct dvf_port *)port;
	unsigned int cr;

	cr = readw(uap->port.membase + DVF_UART_CFG);

	writew(cr, uap->port.membase + DVF_UART_CFG);
}

static void dvf_uart_break_ctl(struct uart_port *port, int break_state)
{
	/* intentionally left blank */
}

static int dvf_uart_startup(struct uart_port *port)
{
	struct dvf_port *uap = (struct dvf_port *)port;
	int retval;

	uap->port.uartclk = clk_get_rate(uap->clk);

	/*
	 * Allocate the IRQ
	 */
	retval = request_irq(uap->port.irq, dvf_uart_interrupt, 0, uap->name,
			     uap);
	if (retval)
		goto out;

	writew(DVF_UART_CTL_UART_DIS, uap->port.membase + DVF_UART_CTL);
	writew(DVF_UART_CFG_BREAK_EN, uap->port.membase + DVF_UART_CFG);

	/*
	 * Don't touch the divisor at startup, use the setting from u-boot.
	 * This can be changed later with dvf_uart_set_termios().
	 */

	writew(DVF_UART_CLR_ICR, uap->port.membase + DVF_UART_FIFO_ICR);
	writew(UART_FIFO_SIZE/2, uap->port.membase + DVF_UART_TX_WM);
	writew(UART_FIFO_SIZE/2, uap->port.membase + DVF_UART_RX_WM);
	writew(DVF_UART_CTL_UART_EN, uap->port.membase + DVF_UART_CTL);

	return 0;

out:
	return retval;
}

static void dvf_uart_shutdown(struct uart_port *port)
{
	struct dvf_port *uap = (struct dvf_port *)port;

	/*
	 * disable all interrupts
	 */
	spin_lock_irq(&uap->port.lock);
	writew(0, uap->port.membase + DVF_UART_FIFO_IER);
	writew(DVF_UART_CLR_ICR, uap->port.membase + DVF_UART_FIFO_ICR);
	spin_unlock_irq(&uap->port.lock);

	/*
	 * Free the interrupt
	 */
	free_irq(uap->port.irq, uap);

	/*
	 * disable the port
	 */
	writew(DVF_UART_CTL_UART_DIS, uap->port.membase + DVF_UART_CTL);
}

static void
dvf_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		    struct ktermios *old)
{
	unsigned int cfg;
	unsigned long flags;
	unsigned int baud;
	struct dvf_baudrate quot;
	unsigned long divider;
	int is_101 = of_machine_is_compatible("dspg,dvf101");

	/*
	 * Ask the core to calculate the divisor for us.
	 */
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/16);

	if (port->custom_divisor)
		divider = port->custom_divisor;
	else
		divider = port->uartclk / baud;

	/* The minimum integer divider is 3 (DVF97/99) or 1 (DVF101). */
	if (divider < 3 * 16 && !is_101) {
		divider = 3 * 16 + 1;
	} else if (divider < 16 && is_101) {
		divider = 16;
	} else {
		int diff1 = (port->uartclk / divider) - baud;
		int diff2 = baud - (port->uartclk / (divider + 1));

		/* The fractional divider must not be 0 on the DVF97/99. */
		if (((divider & 0xf) == 0 && !is_101) ||
		    (!((divider & 0xf) == 15 && !is_101) && diff1 > diff2))
			divider++;
	}

	quot.intgr = divider >> 4;
	quot.frac = divider & 0xf;

	/* byte size */
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		cfg = 0;
		break;
	case CS6:
		cfg = 0;
		break;
	case CS7:
		cfg = DVF_UART_CFG_DATA_LENGTH_7_VAL;
		break;
	default:
		cfg = DVF_UART_CFG_DATA_LENGTH_8_VAL;
		break;
	}

	/* stop bit */
	if (termios->c_cflag & CSTOPB)
		cfg |= DVF_UART_CFG_STOP_CFG;

	/* parity */
	if (termios->c_cflag & PARENB) {
		cfg |= DVF_UART_CFG_PAR_EN;
		if (termios->c_cflag & PARODD)
			cfg |= DVF_UART_CFG_PAR_OD_EVEN;
	}

	/* hardware handshake */
	if (termios->c_cflag & CRTSCTS) {
		cfg |= (DVF_UART_CFG_RTSN_EN|DVF_UART_CFG_CTSN_EN);
		//cfg |= DVF_UART_CFG_RTSN_EN;
	}

	if (!(termios->c_iflag & IGNBRK))
		cfg |= DVF_UART_CFG_BREAK_EN;
	else
		cfg &= ~DVF_UART_CFG_BREAK_EN;


	spin_lock_irqsave(&port->lock, flags);

	/*
	 * Update the per-port timeout.
	 */
	uart_update_timeout(port, termios->c_cflag, baud);

	port->read_status_mask =
		/* bit 9 in IER -> bit 0 */
		(DVF_UART_RX_FIFO_OVER_IER >> DVF_UART_STAT_MASK_LOC_0);

	if (termios->c_iflag & INPCK)
		port->read_status_mask |=
			/* bits 9-10 in RX_DATA -> bits 2-3 */
			(DVF_UART_RX_PE_FE >> DVF_UART_STAT_MASK_LOC_2_3);

	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |=
			/* bit 12 in IER -> bit 1 */
			(DVF_UART_RX_BREAK_IER >> DVF_UART_STAT_MASK_LOC_1);

	/*
	 * Characters to ignore
	 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |=
			(DVF_UART_RX_PE_FE >> DVF_UART_STAT_MASK_LOC_2_3);

	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |=
			(DVF_UART_RX_BREAK_IER >> DVF_UART_STAT_MASK_LOC_1);

		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |=
				(DVF_UART_RX_FIFO_OVER_IER >>
					DVF_UART_STAT_MASK_LOC_0);
	}

	if (UART_ENABLE_MS(port, termios->c_cflag))
		dvf_uart_enable_ms(port);

	writew(DVF_UART_CTL_UART_DIS, port->membase + DVF_UART_CTL);
	dvf_uart_set_div(port, quot.intgr, quot.frac);
	writew(cfg, port->membase + DVF_UART_CFG);
	writew(DVF_UART_STARTUP_IER, port->membase + DVF_UART_FIFO_IER);
	writew(DVF_UART_CTL_UART_EN, port->membase + DVF_UART_CTL);

	spin_unlock_irqrestore(&port->lock, flags);
}

static const char *dvf_uart_type(struct uart_port *port)
{
	struct dvf_port *sport = (struct dvf_port *)port;

	return sport->port.type == PORT_DSPG ? "DSPG_UART" : NULL;
}

/*
 * Release the memory region(s) being used by 'port'
 */
static void dvf_uart_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, port->mapsize);
}

/*
 * Request the memory region(s) being used by 'port'
 */
static int dvf_uart_request_port(struct uart_port *port)
{
	if (port->membase)
		return 0;

	if (!request_mem_region(port->mapbase, port->mapsize, "DVF_UART")) {
		dev_err(port->dev, "Memory region busy\n");
		return -EBUSY;
	}

	port->membase = ioremap(port->mapbase, port->mapsize);
	if (!port->membase) {
		dev_err(port->dev, "Unable to map registers\n");
		release_mem_region(port->mapbase, port->mapsize);
		return -EBUSY;
	}

	return 0;
}

/*
 * Configure/autoconfigure the port.
 */
static void dvf_uart_config_port(struct uart_port *port, int flags)
{
	struct dvf_port *sport = (struct dvf_port *)port;

	if (flags & UART_CONFIG_TYPE &&
	    dvf_uart_request_port(&sport->port) == 0)
		sport->port.type = PORT_DSPG;
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int
dvf_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_DSPG)
		ret = -EINVAL;
	if (ser->irq < 0)
		ret = -EINVAL;
	if (ser->baud_base < 9600)
		ret = -EINVAL;
	return ret;
}

static int
dvf_uart_ioctl(struct uart_port *port, unsigned int cmd, unsigned long arg)
{
	unsigned long data;

	switch (cmd) {
	case TCSETRXWM:
		if (!get_user(data, (unsigned long *)arg)) {
			writew(data, port->membase + DVF_UART_RX_WM);
			return 0;
		} else {
			return -EINVAL;
		}
	case TCSETTXWM:
		if (!get_user(data, (unsigned long *)arg)) {
			writew(data, port->membase + DVF_UART_TX_WM);
			return 0;
		} else {
			return -EINVAL;
		}
	case TCGETRXWM:
		data = readw(port->membase + DVF_UART_RX_WM);
		return put_user(data, (unsigned long __user *)arg);
	case TCGETTXWM:
		data = readw(port->membase + DVF_UART_TX_WM);
		return put_user(data, (unsigned long __user *)arg);
	default:
		return -ENOIOCTLCMD;
	}
}

static void
dvf_uart_pm(struct uart_port *port, unsigned int state, unsigned int oldstate)
{
	struct dvf_port *sport = (struct dvf_port *)port;

	if (!state) {
		clk_enable(sport->clk);
	} else {
		while (!(readw(port->membase + DVF_UART_STAT) &
		       DVF_UART_STAT_TX_EMPTY))
			cpu_relax();

		/* wait for the last char in shift register */
		dvf_wait_one_char(port->timeout, port->fifosize);

		clk_disable(sport->clk);
	}
}

static const struct uart_ops dvf_uart_pops = {
	.pm		= dvf_uart_pm,
	.tx_empty	= dvf_uart_tx_empty,
	.set_mctrl	= dvf_uart_set_mctrl,
	.get_mctrl	= dvf_uart_get_mctrl,
	.stop_tx	= dvf_uart_stop_tx,
	.start_tx	= dvf_uart_start_tx,
	.stop_rx	= dvf_uart_stop_rx,
	.enable_ms	= dvf_uart_enable_ms,
	.break_ctl	= dvf_uart_break_ctl,
	.startup	= dvf_uart_startup,
	.shutdown	= dvf_uart_shutdown,
	.set_termios	= dvf_uart_set_termios,
	.type		= dvf_uart_type,
	.release_port	= dvf_uart_release_port,
	.request_port	= dvf_uart_request_port,
	.config_port	= dvf_uart_config_port,
	.verify_port	= dvf_uart_verify_port,
	.ioctl		= dvf_uart_ioctl,
};

static int dvf_uart_claim_ressources(struct device_node *np,
				    struct platform_device *pdev, int id,
				    unsigned int *console)
{
	struct dvf_port *uap;
	struct resource mem_res;
	struct clk *clk;
	int ret = 0;
	int irq;
	char tty[6];

	if (!np)
		return -EINVAL;

	if (id < 0) {
		/*
		 * Determine port number from the device tree 'aliases'
		 */
		id = of_alias_get_id(np, "serial");
		if (id < 0) {
			/* port id not found in device-tree aliases:
			 * auto-enumerate it
			 */
			id = 0;
			while (id < NR_PORTS && dvf_ports[id].port.mapbase)
				id++;
		}
	}

	if (id >= NR_PORTS)
		return -ENODEV;

	ret = of_address_to_resource(np, 0, &mem_res);
	if (ret)
		return ret;

	irq = of_irq_get(np, 0);
	if (irq < 0)
		return irq;

	uap = &dvf_ports[id];

	/* console already mapped this ressource */
	if (uap->port.mapbase) {
		if (console)
			*console = 1;
		return id;
	}

	/*
	 * some dvf platforms do not support the common clock framework
	 */
	snprintf(tty, 6, "ttyS%d", id);
	snprintf(uap->name, DVF_NAME_LEN, "dvf-uart%d", id);

	clk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(clk)) {
		if (pdev)
			clk = clk_get(&pdev->dev, NULL);
		else
			clk = clk_get(NULL, tty);
	}
	if (IS_ERR_OR_NULL(clk)) {
		if (!uap->clk)
			return -ENODEV;
		return PTR_ERR(uap->clk);
	}

	/* release from reset */
	uap->rc = of_reset_control_get(np, NULL);
	if (IS_ERR(uap->rc)) {
		if (PTR_ERR(uap->rc) != -ENOTSUPP &&
		    PTR_ERR(uap->rc) != -ENOENT &&
		    PTR_ERR(uap->rc) != -EPROBE_DEFER)
			return PTR_ERR(uap->rc);
	} else {
		ret = reset_control_deassert(uap->rc);
		if (ret)
			return ret;
	}

	if (pdev)
		uap->port.dev = &pdev->dev;

	uap->port.mapbase =  mem_res.start;
	uap->port.mapsize =  resource_size(&mem_res);
	uap->port.iotype =   UPIO_MEM;
	uap->port.fifosize = UART_FIFO_SIZE;
	uap->port.ops =      &dvf_uart_pops;
	uap->port.flags =    UPF_BOOT_AUTOCONF;
	uap->port.line =     id;
	uap->port.irq =      irq;
	uap->clk =           clk;

	clk_prepare(clk);

	return id;
}

static int dvf_uart_clk_change_cb(struct notifier_block *nb,
				  unsigned long event, void *data)
{
	struct clk_notifier_data *ndata = data;
	unsigned int baud, ibrd, fbrd;
	struct dvf_baudrate quot;
	struct uart_port *port;
	unsigned long flags = 0;
	struct dvf_port *dvf_port = nb_to_dvf_port(nb);

	port = &dvf_port->port;

	switch (event) {
	case PRE_RATE_CHANGE:
		spin_lock_irqsave(&port->lock, flags);
		while (!(readw(port->membase + DVF_UART_STAT) &
		       DVF_UART_STAT_TX_EMPTY))
			;
		dvf_wait_one_char(port->timeout, port->fifosize);
		disable_irq(port->irq);
		dvf_port->should_stop = true;
		spin_unlock_irqrestore(&port->lock, flags);

		break;
	case POST_RATE_CHANGE:
		if (!(readw(port->membase + DVF_UART_CTL) &
		      DVF_UART_CTL_UART_EN))
			break;

		dvf_uart_get_div(port, &ibrd, &fbrd);
		baud = port->uartclk * 4 / (64 * ibrd + fbrd);

		port->uartclk = ndata->new_rate;
		quot.intgr = (port->uartclk / baud) >> 4;
		quot.frac = (port->uartclk / baud) & 0x000f;
		dvf_uart_set_div(port, quot.intgr, quot.frac);

		dvf_port->should_stop = false;
		enable_irq(dvf_port->port.irq);

		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

#ifdef CONFIG_SERIAL_DSPG_CONSOLE

static void dvf_uart_console_putchar(struct uart_port *port, int ch)
{
	struct dvf_port *uap = (struct dvf_port *)port;

	while (readw(uap->port.membase + DVF_UART_STAT) &
		     DVF_UART_STAT_TX_FULL)
		cpu_relax();

	writew(ch, uap->port.membase + DVF_UART_TX_DATA);
}

static void
dvf_uart_console_write(struct console *co, const char *s, unsigned int count)
{
	struct dvf_port *uap = &dvf_ports[co->index];

	uart_console_write(&uap->port, s, count, dvf_uart_console_putchar);

	while (!(readw(uap->port.membase + DVF_UART_STAT) &
		DVF_UART_STAT_TX_EMPTY))
		cpu_relax();

	/* wait for the last char in the shift register */
	dvf_wait_one_char(uap->port.timeout, uap->port.fifosize);
}

static void __init
dvf_uart_console_get_options(struct dvf_port *uap, int *baud, int *parity,
			    int *bits)
{
	unsigned int cfg, ibrd, fbrd, data_len;

	cfg = readw(uap->port.membase + DVF_UART_CTL);

	if (!(cfg & DVF_UART_CTL_UART_EN))
		return;

	cfg = readw(uap->port.membase + DVF_UART_CFG);

	*parity = 'n';
	if (cfg & DVF_UART_CFG_PAR_EN) {
		if (cfg & DVF_UART_CFG_OD_EN)
			*parity = 'o';
		else
			*parity = 'e';
	}

	*bits = 0;
	data_len = cfg & DVF_UART_CFG_DATA_LENGTH_MASK;
	if (data_len == DVF_UART_CFG_DATA_LENGTH_7_VAL)
		*bits = 7;
	else if (data_len == DVF_UART_CFG_DATA_LENGTH_8_VAL)
		*bits = 8;
	else if (data_len == DVF_UART_CFG_DATA_LENGTH_9_VAL)
		*bits = 9;

	dvf_uart_get_div(&uap->port, &ibrd, &fbrd);
	*baud = uap->port.uartclk * 4 / (64 * ibrd + fbrd);
}

static int __init dvf_uart_console_setup(struct console *co, char *options)
{
	struct dvf_port *uap;
	struct device_node *np;
	struct resource *res;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';
	int ret;

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (co->index >= NR_PORTS)
		co->index = 0;

	np = dvf_uart_nodes[co->index];

	ret = dvf_uart_claim_ressources(np, NULL, co->index, NULL);
	if (ret < 0)
		return ret;
	uap = &dvf_ports[ret];

	res = request_mem_region(uap->port.mapbase, uap->port.mapsize,
				 "DVF_UART");
	if (!res)
		return -EBUSY;

	uap->port.membase = ioremap(uap->port.mapbase, uap->port.mapsize);
	if (!uap->port.membase) {
		release_region(uap->port.mapbase, uap->port.mapsize);
		return -EBUSY;
	}

	uap->port.uartclk = clk_get_rate(uap->clk);

	clk_enable(uap->clk);

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else
		dvf_uart_console_get_options(uap, &baud, &parity, &bits);

	ret = uart_set_options(&uap->port, co, baud, parity, bits, flow);

	uap->port.fifosize = UART_FIFO_SIZE;
	uart_update_timeout(&uap->port, co->cflag, baud);

	return ret;
}

static struct uart_driver dvf_uart;

static struct console dvf_console = {
	.name		= SERIAL_DVF_NAME,
	.write		= dvf_uart_console_write,
	.device		= uart_console_device,
	.setup		= dvf_uart_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &dvf_uart,
};

static int __init dvf_rs_console_init(void)
{
	dvf_uart_of_enumerate();
	register_console(&dvf_console);
	return 0;
}
console_initcall(dvf_rs_console_init);

#define DVF_CONSOLE	(&dvf_console)
#else
#define DVF_CONSOLE	NULL
#endif

/*
 * early console support
 */
static void
dvf_serial_early_write(struct console *con, const char *s, unsigned int n)
{
	struct earlycon_device *dev = con->data;

	uart_console_write(&dev->port, s, n, dvf_uart_console_putchar);
}

static int __init
dvf_serial_early_console_setup(struct earlycon_device *device, const char *opt)
{
	if (!device->port.membase)
		return -ENODEV;

	device->con->write = dvf_serial_early_write;
	return 0;
}
EARLYCON_DECLARE(dvf_serial, dvf_serial_early_console_setup);
OF_EARLYCON_DECLARE(dvf_serial, "dspg,uart", dvf_serial_early_console_setup);

static struct uart_driver dvf_uart = {
	.owner		= THIS_MODULE,
	.driver_name	= "dvf-uart",
	.dev_name	= SERIAL_DVF_NAME,
	.major		= SERIAL_DVF_MAJOR,
	.minor		= SERIAL_DVF_MINOR,
	.nr		= NR_PORTS,
	.cons		= DVF_CONSOLE,
};

static int dvf_uart_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct dvf_port *uap;
	int ret;
	unsigned int console = 0;

	ret = dvf_uart_claim_ressources(np, pdev, -1, &console);
	if (ret < 0)
		return ret;

	uap = &dvf_ports[ret];

	/* disable the clock if this is a console */
	if (console)
		clk_disable(uap->clk);

	uap->clk_change_nb.notifier_call = dvf_uart_clk_change_cb;
	uap->clk_change_nb.next = NULL;

	ret = clk_notifier_register(uap->clk, &uap->clk_change_nb);
	if (ret) {
		dev_err(&pdev->dev, "could not register clock notifier\n");
		uap->clk_change_nb.notifier_call = NULL;
	}

	tasklet_init(&uap->tasklet, dvf_uart_tasklet_func, (unsigned long)uap);

	uap->port.dev = &pdev->dev;

	ret = uart_add_one_port(&dvf_uart, &uap->port);
	if (ret) {
		dev_err(&pdev->dev, "adding port failed; err=%d\n", ret);
		uap->port.mapbase = 0;
		clk_put(uap->clk);
		return ret;
	}
	platform_set_drvdata(pdev, uap);

	if (console)
		dev_notice(&pdev->dev, "disable clock enabled by console\n");

	dev_info(&pdev->dev, "successfully probed\n");

	return ret;
}

static int dvf_uart_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct dvf_port *sport = platform_get_drvdata(pdev);

	return uart_suspend_port(&dvf_uart, &sport->port);
}

static int dvf_uart_resume(struct platform_device *pdev)
{
	struct dvf_port *sport = platform_get_drvdata(pdev);

	return uart_resume_port(&dvf_uart, &sport->port);
}

static int dvf_uart_remove(struct platform_device *pdev)
{
	struct dvf_port *sport = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

	if (sport) {
		uart_remove_one_port(&dvf_uart, &sport->port);

		if (sport->clk_change_nb.notifier_call)
			clk_notifier_unregister(sport->clk,
						&sport->clk_change_nb);
	}

	return 0;
}

static const struct of_device_id dspg_uart_match_table[] = {
	{ .compatible = "dspg,uart" },
	{}
};
MODULE_DEVICE_TABLE(of, dspg_uart_match_table);

static void
dvf_uart_of_assign(struct device_node *np)
{
	int i;

	/* Find the first free UART number */
	for (i = 0; i < NR_PORTS; i++) {
		if (dvf_uart_nodes[i] == NULL) {
			of_node_get(np);
			dvf_uart_nodes[i] = np;
			return;
		}
	}
}

static void
dvf_uart_of_enumerate(void)
{
	static int enum_done;
	struct device_node *np;
	const struct of_device_id *match;
	int i;

	if (enum_done)
		return;

	/* Assign index to each UART slot in device tree */
	for_each_matching_node(np, dspg_uart_match_table) {
		match = of_match_node(dspg_uart_match_table, np);
		dvf_uart_of_assign(np);
	}

	enum_done = 1;

	for (i = 0; i < NR_PORTS; i++) {
		if (dvf_uart_nodes[i])
			pr_debug("%s assigned to ttyPSC%x\n",
				dvf_uart_nodes[i]->full_name, i);
	}
}

static struct platform_driver dvf_uart_driver = {
	.probe		= dvf_uart_probe,
	.remove		= dvf_uart_remove,
	.suspend	= dvf_uart_suspend,
	.resume		= dvf_uart_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "dspg-uart",
		.of_match_table = dspg_uart_match_table,
	},
};

static int __init dvf_uart_init(void)
{
	int ret;

	ret = uart_register_driver(&dvf_uart);
	if (ret)
		return ret;

	ret = platform_driver_register(&dvf_uart_driver);
	if (ret)
		uart_unregister_driver(&dvf_uart);

	return ret;
}

static void __exit dvf_uart_exit(void)
{
	platform_driver_unregister(&dvf_uart_driver);
	uart_unregister_driver(&dvf_uart);

}

module_init(dvf_uart_init);
module_exit(dvf_uart_exit);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("DVF serial port driver");
MODULE_LICENSE("GPL");
