/*
 *  Driver for DSPG software UART via TDM
 *
 *  Copyright (C) 2015 DSPG Technologies GmbH
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

#include <linux/clk.h>
#include <linux/console.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/sysrq.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/reset.h>
#include <linux/dmw96dma.h>

/* TDM registers */
#define TDM_ENABLE                   0x0000
#define TDM_CONFIG_1                 0x0004
#define TDM_CONFIG_2                 0x0008
#define TDM_CYCLE                    0x000C
#define TDM_FSYNC                    0x0010
#define TDM_FSYNC_DUR                0x0014
#define TDM_FIFO_WM                  0x0018
#define TDM_FIFO_BNK_CNTL            0x001C
#define TDM_INT_EN                   0x0020
#define TDM_TF_INT_MSK               0x0024
#define TDM_INT_CLR                  0x0028
#define TDM_INT_STATUS               0x002C
#define TDM_STATUS                   0x0030
#define TDM_TX_FLAGS                 0x0034
#define TDM_TX_FIFO                  0x0038
#define TDM_RX_FIFO                  0x003C
#define TDM_TX_REG_0                 0x0A00
#define TDM_RX_REG_0                 0x1200
#define TDM_FDA_REG_00               0x1800

#define TDM_CONFIG_1_AUTO_SYNC       (1 << 13)
#define TDM_CONFIG_1_CLK_MSTR_SLV    (1 << 12)
#define TDM_CONFIG_1_FSYNC_MSTR_SLV  (1 << 11)
#define TDM_CONFIG_1_SYNC_EDGE       (1 << 10)
#define TDM_CONFIG_1_RX_EDGE         (1 <<  9)
#define TDM_CONFIG_1_TX_EDGE         (1 <<  8)
#define TDM_CONFIG_1_TIME_SLOTS      0x1F

#define TDM_CONFIG_2_SHIFT           (1 << 9)
#define TDM_CONFIG_2_TRUNCATE        (1 << 8)
#define TDM_CONFIG_2_BANK_SIZE       0x07

#define TDM_INT_EN_RX_FULL           (1 << 3)
#define TDM_INT_EN_TX_EMPTY          (1 << 1)
#define TDM_INT_EN_RX_WM             (1 << 2)
#define TDM_INT_EN_TX_WM             (1 << 0)

#define TDM_INT_CLR_TX_FLAGS         (1 << 3)
#define TDM_INT_CLR_FSYNC            (1 << 2)
#define TDM_INT_CLR_RX_OVRN          (1 << 1)
#define TDM_INT_CLR_TX_UNDRN         (1 << 0)

#define TDM_INT_STATUS_RX_OVRN       (1 << 3)
#define TDM_INT_STATUS_RX_INT        (1 << 2)
#define TDM_INT_STATUS_TX_UNDRN      (1 << 1)
#define TDM_INT_STATUS_TX_INT        (1 << 0)

/*
 * this value should match the burst size of the DMA, currently it is
 * configured to 2*16 samples to collect on each run
 */
#define TDM_NR_SLOTS                 32

/*
 * oversample rate, each TDM bit is a bus sample,
 * each TDM sample is a UART bit
 */
#define OVERSAMPLE_RATE              16
/* maximum frequency the TDM is allowed to run at */
#define MAX_TDM_FREQUENCY_HZ         25000000
/* maximum possible baud rate */
#define MAX_BAUD                     (MAX_TDM_FREQUENCY_HZ / OVERSAMPLE_RATE)

#define TDM_FD_CHAR_BIT(bit)                             \
	(0 |                                             \
	 (OVERSAMPLE_RATE << 0) | /* nr of data bits */  \
	 (0 << 5) | /* Audio data */                     \
	 (1 << 6) | /* TX enabled */                     \
	 (1 << 7) | /* RX enabled */                     \
	 ((bit) << 8) /* bank */                         \
	)

#define NR_PORTS				2

#define SERIAL_SW_MAJOR				TTY_MAJOR
#define SERIAL_SW_MINOR				128
#define SERIAL_SW_NAME				"ttyTDMS"

/* CS8, 1 start bit, 1 stop bit */

#define NR_OF_RX_BUFFERS			32
#define SIZE_OF_RX_BUFFER			(SZ_4K)
#define RX_BUFFER_ELEMENTS			(SIZE_OF_RX_BUFFER / 4)

#define NR_OF_TX_BUFFERS			32
#define SIZE_OF_TX_BUFFER			(SZ_4K)
#define TX_BUFFER_ELEMENTS			(SIZE_OF_TX_BUFFER / 4)

struct buffer_callback {
	struct completion done;
	struct sw_port *uap;
	unsigned int offset;
};

struct sw_port {
	struct uart_port port;
	struct resource *mem_res;
	struct clk *clk;
	unsigned int reset;
	struct ktermios termios;
	unsigned int rx_overrun;
	unsigned int tx_underrun;
	unsigned int tx_buffer_index;
	unsigned int rx_buffer_timeout_ms;
	unsigned int tx_buffer_timeout_ms;

	/* threads for RX/TX */
	struct task_struct	*rx_thread;
	struct task_struct	*tx_thread;
	wait_queue_head_t	thread_queue;
	atomic_t		do_tx;
	atomic_t		do_rx;

	/* DMA */
	struct completion rx_list_done;
	struct completion rx_buf_done[NR_OF_RX_BUFFERS];
	struct completion tx_list_done;

	struct buffer_callback tx_buffer_callback[NR_OF_TX_BUFFERS];

	struct dmw96dma_list *dma_rx_list;
	struct dmw96dma_list *dma_tx_list;
	unsigned int dma_rx_channel;
	unsigned int dma_tx_channel;
	u32 rx_buffer[NR_OF_RX_BUFFERS * RX_BUFFER_ELEMENTS];
	u32 tx_buffer[NR_OF_TX_BUFFERS * TX_BUFFER_ELEMENTS];
};

static struct sw_port sw_ports[NR_PORTS];

static inline struct sw_port *to_ourport(struct uart_port *port)
{
	return container_of(port, struct sw_port, port);
}

static u32 tdm_read(struct uart_port *port, unsigned int reg)
{
	return readl(port->membase + reg);
}

static void tdm_write(struct uart_port *port, u32 value,
		      unsigned int reg)
{
	writel(value, port->membase + reg);
}

static void tdm_tx_char(struct uart_port *port, unsigned char c,
				unsigned int bits, int parity, u32 *buffer)
{
	unsigned int i;
	unsigned int bits_set = 0;

	/* buffer is preset with 0xff, only 0 needs to be written */

	/* start bit */
	*buffer++ = 0x00000000;

	for (i = 0; i < bits; i++) {
		if (!(c & 1))
			*buffer = 0x00000000;
		else
			bits_set++;
		c >>= 1;
		buffer++;
	}
	/* even parity bit */
	if (parity == 0 && (bits_set & 0x1))
		*buffer = 0x00000000;

	/* odd parity bit */
	if (parity == PARODD && !(bits_set & 0x1))
		*buffer = 0x00000000;

	/* stop bit is already done */
}

static unsigned int process_bits(u32 *seq, unsigned int seq_c, unsigned int *ch, unsigned int *s, unsigned int *nr_of_bits)
{
	unsigned int c = 0;
	unsigned int mask;
	unsigned int bits;
	unsigned int offset = 0;
	unsigned int shift;
	unsigned int max = seq_c;
	const unsigned int lbs = fls(seq[0]);
	const unsigned int bit = (1 << (seq_c - 2));
	unsigned int bitcount = 0;
	const unsigned int tmask = (1 << ((OVERSAMPLE_RATE / 2) - 1)) | (1 << (OVERSAMPLE_RATE - 1));
	u32 last = seq[seq_c - 1];
	u32 buf[12];

	/* TODO:
	 *   big issue here if bitlength is abnormally short (50%)
	 *   maybe its better to count the bits till the stop bit and
	 *   calculate the best point to sample in order to compensate the
	 *   different clocks
	 */
	shift = lbs < OVERSAMPLE_RATE? lbs: 0;
	for (bits = 0; bits < (seq_c - 1); bits++) {
		buf[bits] = seq[bits] >> shift;
		buf[bits] |= seq[bits + 1] << (OVERSAMPLE_RATE - shift);
	}

	if (lbs >= (OVERSAMPLE_RATE / 2))
		offset = 1;

	/* decide if last sample must be evaluated again in next run */
	if ((last & tmask) != tmask)
		max--;

	mask = 0xf << (OVERSAMPLE_RATE / 2 - 2);

	for (bits = 0; bits < (seq_c - 1); bits++) {
		c >>= 1;
		if (buf[bits] & mask) {
			c |= bit;
			bitcount++;
		}
	}

	*ch = c;
	*nr_of_bits = bitcount;
	*s = shift;

	return max;
}

static int find_falling_edge(unsigned int data, unsigned int start)
{
	unsigned int first_zero;
	unsigned int mask = 0xfffff >> (20 - OVERSAMPLE_RATE) >> start;

	if (data == 0)
		return start;

	if (((data >> start) & mask) == mask)
		return -1;

	first_zero = ffz(data >> start);
	return first_zero + start;
}

static int sw_uart_rx_thread(void *data)
{
	struct sw_port *uap = (struct sw_port *)data;
	struct tty_struct *tty = uap->port.state->port.tty;
	unsigned int ch, flag;
	unsigned int i;
	/* max is 8 data bits, 2 stop bits, 1 start bit, 1 parity bit + 1 */
	u32 seq[13];
	unsigned int seq_c = 0;
	/* 2 instead of 1 because the data might be spread between two slots */
	unsigned int max_seq_c = 2;
	unsigned int start = 0;
	unsigned int buffer = 0;
	unsigned int cbi = 0;
	unsigned int offset = 11;
	int shift = 0;
	unsigned int fifo_mask = (0xfffff >> (20 - OVERSAMPLE_RATE)) << 12;
	long ret;
	unsigned int bits = 8;
	int parity = -1;
	unsigned int stopb = 1;
	unsigned int data_mask;
	unsigned int bitcount;
	unsigned int stop_shift = 1;
	unsigned int stop_mask;

	/* check byte size */
	switch (uap->termios.c_cflag & CSIZE) {
	case CS5:
		bits = 5;
		break;
	case CS6:
		bits = 6;
		break;
	case CS7:
		bits = 7;
		break;
	default:
		break;
	}
	max_seq_c += bits;
	data_mask = 0xff >> (8 - bits);
	stop_shift += bits;

	if (uap->termios.c_cflag & PARENB)
		stop_shift++;

	/* check parity generation */
	if ((uap->termios.c_cflag & PARENB) &&
	    !(uap->termios.c_iflag & IGNPAR)) {
		parity = uap->termios.c_cflag & PARODD;
		max_seq_c++;
	}

	/* check stop bit generation */
	if (uap->termios.c_cflag & CSTOPB) {
		stopb = 2;
		stop_shift++;
		stop_mask = 0x3 << stop_shift;
	} else
		stop_mask = 0x1 << stop_shift;
	max_seq_c += stopb;

	dev_dbg(uap->port.dev, "%s: bits %u parity %d stopb %u max %u",
		 __func__, bits, parity, stopb, max_seq_c);

	while (!kthread_should_stop()) {

		ret = wait_event_interruptible_timeout(uap->thread_queue,
						       atomic_read(&uap->do_rx),
						       msecs_to_jiffies(50));
		if (!ret || ret == -ERESTARTSYS)
			continue;

		if (kthread_should_stop())
			break;

		/* TODO: calc real timeout */
		if (!wait_for_completion_timeout(&uap->rx_buf_done[buffer],
						 msecs_to_jiffies(100)))
			continue;

		for (i = buffer * RX_BUFFER_ELEMENTS;
		     i < (buffer * RX_BUFFER_ELEMENTS + RX_BUFFER_ELEMENTS);
		     i++) {
again:
			if (start) {
				seq[seq_c++] = (uap->rx_buffer[i] & fifo_mask) >> 12;
				if (seq_c == max_seq_c) {
					start = 0;
					offset = process_bits(seq, max_seq_c, &ch, &shift, &bitcount);
					cbi++;
					flag = TTY_NORMAL;
					uap->port.icount.rx++;

					if (parity == 0) {
						/* even parity bit */
						if ((bitcount & 0x1) == ((ch >> (bits + 1)) & 0x1))
							flag = TTY_PARITY;
					} else if (parity == PARODD) {
						/* odd parity bit */
						if ((bitcount & 0x1) != ((ch >> (bits + 1)) & 0x1))
							flag = TTY_PARITY;
					} else if ((ch & stop_mask) != stop_mask) {
						flag = TTY_FRAME;
					}
					/* XXX TTY_BREAK, TTY_OVERRUN */

					/* XXX what about all the errors */
					if (uart_handle_sysrq_char(&uap->port, ch)) {
						cbi--;
						goto ignore_char;
					}

					uart_insert_char(&uap->port,
							 0, /* status */
							 0, /* overrun mask */
							 /* shift out the start bit, and mask out stop/parity bits */
							 (ch >> 1) & data_mask,
							 flag);

					/* while in the loop push rx data in
					 * chunks to reduce the load
					 */
					if (cbi == 128) {
						cbi = 0;
						tty_flip_buffer_push(tty);
					}
				}
			}
ignore_char:
			if (!start) {
				if (offset == (max_seq_c - 1))
					seq[0] = seq[max_seq_c - 2];
				else
					seq[0] = (uap->rx_buffer[i] & fifo_mask) >> 12;

				shift = find_falling_edge(seq[0], shift);

				if (shift != -1) {
					start = 1;
					seq_c = 1;
				}

				shift = 0;
				if (offset == (max_seq_c - 1)) {
					offset = max_seq_c;
					goto again;
				}
			}
		}
		buffer = (buffer + 1) % NR_OF_RX_BUFFERS;
		/* push rest of the rx data */
		if (cbi)
			tty_flip_buffer_push(tty);
		cbi = 0;
	}
	return 0;
}

static int sw_uart_tx_thread(void *data)
{
	struct sw_port *uap = (struct sw_port *)data;
	struct circ_buf *xmit = &uap->port.state->xmit;
	unsigned int buffer = 0;
	int last_buffer = -1;
	unsigned int i;
	long ret;
	struct buffer_callback *bc;
	unsigned int bits = 8;
	int parity = -1;
	unsigned int stopb = 1;
	unsigned int buffer_entries = 1;

	/* check byte size */
	switch (uap->termios.c_cflag & CSIZE) {
	case CS5:
		bits = 5;
		break;
	case CS6:
		bits = 6;
		break;
	case CS7:
		bits = 7;
		break;
	default:
		break;
	}
	buffer_entries += bits;

	/* check parity generation */
	if (uap->termios.c_cflag & PARENB) {
		parity = uap->termios.c_cflag & PARODD;
		buffer_entries++;
	}

	/* check stop bit generation */
	if (uap->termios.c_cflag & CSTOPB)
		stopb = 2;
	buffer_entries += stopb;

	dev_dbg(uap->port.dev, "%s: bits %u parity %d stopb %u", __func__, bits, parity, stopb);

	bc = &uap->tx_buffer_callback[buffer];

	while (!kthread_should_stop()) {

		/* wait until we need to do TX */
		ret = wait_event_interruptible_timeout(uap->thread_queue,
						       atomic_read(&uap->do_tx),
						       msecs_to_jiffies(100));
		if (!ret || ret == -ERESTARTSYS) {
			buffer = (uap->tx_buffer_index + 2) % NR_OF_TX_BUFFERS;
			continue;
		}

		if (kthread_should_stop())
			break;

		bc = &uap->tx_buffer_callback[buffer];

		while (try_wait_for_completion(&bc->done))
			;

		if (!wait_for_completion_timeout(&bc->done,
						 msecs_to_jiffies(100)))
			continue;

		/* FIXME cleanup spinlocks */

		/* clear buffer with 0xff to keep TX line high */
		memset(&uap->tx_buffer[bc->offset], 0xff, SIZE_OF_TX_BUFFER);
		last_buffer = buffer;
		for (i = buffer * TX_BUFFER_ELEMENTS;
		     i < (buffer * TX_BUFFER_ELEMENTS + TX_BUFFER_ELEMENTS - buffer_entries);
		     i += buffer_entries) {

			if (uap->port.x_char) {
				tdm_tx_char(&uap->port,
					    uap->port.x_char,
					    bits,
					    parity,
					    &uap->tx_buffer[i]);
				uap->port.icount.tx++;
				uap->port.x_char = 0;
				continue;
			}

			if (uart_circ_empty(xmit) || uart_tx_stopped(&uap->port))
				break;

			tdm_tx_char(&uap->port,
				    xmit->buf[xmit->tail],
				    bits,
				    parity,
				    &uap->tx_buffer[i]);
			xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
			uap->port.icount.tx++;

			if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
				uart_write_wakeup(&uap->port);
		}
		buffer = (buffer + 1) % NR_OF_TX_BUFFERS;
	}
	if (last_buffer != -1) {
		/* wait until last buffer has been sent */
		/* XXX fix the timeout */
		ret = wait_for_completion_timeout(&uap->tx_buffer_callback[last_buffer].done,
						  msecs_to_jiffies(5000));
		memset(&uap->tx_buffer[bc->offset], 0xff, SIZE_OF_TX_BUFFER);
		dev_dbg(uap->port.dev, "%s: last buffer: %u, %ld", __func__, last_buffer, ret);
	}
	return 0;
}

static void sw_uart_stop_tx(struct uart_port *port)
{
	struct sw_port *uap = to_ourport(port);

	dev_dbg(uap->port.dev, "%s", __func__);

	atomic_set(&uap->do_tx, 0);
	wake_up(&uap->thread_queue);
}

static void sw_uart_start_tx(struct uart_port *port)
{
	struct sw_port *uap = to_ourport(port);

	atomic_set(&uap->do_tx, 1);
	wake_up(&uap->thread_queue);
}

static void sw_uart_stop_rx(struct uart_port *port)
{
	struct sw_port *uap = to_ourport(port);

	atomic_set(&uap->do_rx, 0);
	wake_up(&uap->thread_queue);
}

static void sw_uart_enable_ms(struct uart_port *port)
{
	return;
}

static irqreturn_t sw_uart_interrupt(int irq, void *dev_id)
{
	struct sw_port *uap = dev_id;
	u32 status;

	status = tdm_read(&uap->port, TDM_INT_STATUS);

	tdm_write(&uap->port,
		  TDM_INT_CLR_RX_OVRN | TDM_INT_CLR_TX_UNDRN,
		  TDM_INT_CLR);

	if (status & TDM_INT_STATUS_RX_OVRN)
		uap->rx_overrun++;

	if (status & TDM_INT_STATUS_TX_UNDRN)
		uap->tx_underrun++;

	return IRQ_HANDLED;
}

static unsigned int sw_uart_tx_empty(struct uart_port *port)
{
	/* XXX */
	return TIOCSER_TEMT;
}

static unsigned int sw_uart_get_mctrl(struct uart_port *port)
{
	return TIOCM_CTS;
}

static void sw_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
}

static void sw_uart_break_ctl(struct uart_port *port, int break_state)
{
	return;
}

/* DMA callbacks */
static void sw_uart_rx_list_callback(int status, void *context)
{
	struct sw_port *uap = (struct sw_port *)context;

	complete(&uap->rx_list_done);
}

static void sw_uart_rx_buffer_callback(int status, void *context)
{
	struct completion *done = (struct completion *)context;

	/* XXX TTY_OVERFLOW */
	if (done->done && !status)
		;

	if (!status)
		complete(done);
	else if (status != -EINTR)
		;
}

static void sw_uart_tx_list_callback(int status, void *context)
{
	struct sw_port *uap = (struct sw_port *)context;

	complete(&uap->tx_list_done);
}

static void sw_uart_tx_buffer_callback(int status, void *context)
{
	struct buffer_callback *bc = (struct buffer_callback *)context;

	bc->uap->tx_buffer_index = (bc->uap->tx_buffer_index + 1) % NR_OF_TX_BUFFERS;

	if (!status)
		complete(&bc->done);
	else if (status != -EINTR)
		;
}

static int sw_uart_startup(struct uart_port *port)
{
	struct sw_port *uap = to_ourport(port);
	struct tty_struct *tty = port->state->port.tty;
	int retval;
	unsigned int i;
	long rate;
	unsigned int baud;
	unsigned int bits_per_ms = 1;

	baud = tty_termios_baud_rate(&uap->termios);

	dev_dbg(uap->port.dev, "%s: UART clk: %uHZ", __func__, uap->port.uartclk);

	reset_set(uap->reset);
	mdelay(1);
	reset_release(uap->reset);
	/*
	 * Allocate the IRQ
	 */
	retval = request_irq(uap->port.irq, sw_uart_interrupt, 0,
	                     tty ? tty->name : "sw-tdm-uart", uap);
	if (retval)
		goto out;

	/* enable the clock */
	rate = clk_round_rate(uap->clk, baud * OVERSAMPLE_RATE);
	if (rate < 0) {
		dev_err(uap->port.dev, "%s: cannot round UART rate %dHZ",
			 __func__, baud * OVERSAMPLE_RATE);
		goto out_err_free_irq;
	}
	clk_set_rate(uap->clk, rate);
	clk_enable(uap->clk);
	/* calculate buffer time */
	if (rate >= 1000)
		bits_per_ms = rate / 1000;
	uap->rx_buffer_timeout_ms =
		(RX_BUFFER_ELEMENTS * OVERSAMPLE_RATE + bits_per_ms - 1) /
		bits_per_ms;
	uap->tx_buffer_timeout_ms =
		(TX_BUFFER_ELEMENTS * OVERSAMPLE_RATE + bits_per_ms - 1) /
		bits_per_ms;

	dev_dbg(uap->port.dev, "%s: set clock to: %luHZ for %u baud\n",
		 __func__, rate, baud);
	dev_dbg(uap->port.dev, "%s: rx buffer timeout: %ums",
		 __func__, uap->rx_buffer_timeout_ms);
	dev_dbg(uap->port.dev, "%s: tx buffer timeout: %ums",
		 __func__, uap->tx_buffer_timeout_ms);

	/* configure all slots */
	for (i = 0; i < TDM_NR_SLOTS; i++) {
		tdm_write(port, TDM_FD_CHAR_BIT(i), TDM_FDA_REG_00 + (4 * i));
		dev_dbg(uap->port.dev, "%s: FDA %u - %u\n", __func__, i, __LINE__);
	}

	/* set config 1 */
	tdm_write(port,
		  0 |
		  TDM_CONFIG_1_CLK_MSTR_SLV | /* TDM drives the serial clock */
		  TDM_CONFIG_1_FSYNC_MSTR_SLV | /* TDM drives the FSYNC */
		  (TDM_NR_SLOTS - 1), /* nr of time slots */
		  TDM_CONFIG_1);

	/* set config 2 */
	tdm_write(port,
		  4, //3, /* bank size (16 entries per line) */
		  TDM_CONFIG_2);

	/* tdm cycle */
	tdm_write(port,
		  0,
		  TDM_CYCLE);

	/* set watermarks */
	tdm_write(port,
		  (1 << 8) | /* RX trigger DMA on 1 full bank */
		  (1 << 0), /* TX trigger DMA on 2 empty banks */
		  TDM_FIFO_WM);

	/* enable interrupts */
	tdm_write(port,
		  TDM_INT_EN_RX_FULL | TDM_INT_EN_TX_EMPTY,
		  TDM_INT_EN);

	/* clear any pending interrupts */
	tdm_write(&uap->port, 0xf, TDM_INT_CLR);

	/* fill TX fifo */
	while (tdm_read(port, TDM_STATUS) & 0x3f)
		tdm_write(port, 0xffffffff, TDM_TX_FIFO);

	/* empty RX fifo */
	while (tdm_read(port, TDM_STATUS) & (0x3f << 8))
		tdm_read(port, TDM_RX_FIFO);

	/* initialize DMA related variables for RX */
	init_completion(&uap->rx_list_done);
	for (i = 0; i < NR_OF_RX_BUFFERS; i++)
		init_completion(&uap->rx_buf_done[i]);

	/* initialize DMA related variables for TX */
	init_completion(&uap->tx_list_done);
	for (i = 0; i < NR_OF_TX_BUFFERS; i++) {
		init_completion(&uap->tx_buffer_callback[i].done);
		uap->tx_buffer_callback[i].uap = uap;
		uap->tx_buffer_callback[i].offset = i * TX_BUFFER_ELEMENTS;
	}

	/* reset TX buffer index */
	uap->tx_buffer_index = 0;

	/* preset TX buffers */
	memset(uap->tx_buffer, 0xff, NR_OF_TX_BUFFERS * SIZE_OF_TX_BUFFER);

	/* prepare the DMA for RX */
	uap->dma_rx_list = dmw96dma_alloc_io_list(1,
						  uap->dma_rx_channel,
						  sw_uart_rx_list_callback,
						  (void *)uap);
	if (!uap->dma_rx_list) {
		dev_err(uap->port.dev, "Out of memory for DMA\n");
		goto out_err_clk_off;
	}

	dev_dbg(uap->port.dev,
		 "setting up RX DMA transfer for %d bytes\n",
		 SIZE_OF_RX_BUFFER * NR_OF_RX_BUFFERS);

	for (i = 0; i < NR_OF_RX_BUFFERS; i++) {
		retval = dmw96dma_add_io_transfer_buf(
				uap->dma_rx_list,
				(void *)&uap->rx_buffer[i * RX_BUFFER_ELEMENTS],
				SIZE_OF_RX_BUFFER,
				sw_uart_rx_buffer_callback,
				(void *)&uap->rx_buf_done[i]);
		if (retval)
			goto out_err_free_rx_list;
	}

	/* prepare the DMA for TX */
	uap->dma_tx_list = dmw96dma_alloc_io_list(1,
						  uap->dma_tx_channel,
						  sw_uart_tx_list_callback,
						  (void *)uap);
	if (!uap->dma_tx_list) {
		dev_err(uap->port.dev, "Out of memory for DMA\n");
		goto out_err_free_rx_list;
	}

	dev_dbg(uap->port.dev,
		 "setting up TX DMA transfer for %d bytes\n",
		 SIZE_OF_TX_BUFFER * NR_OF_TX_BUFFERS);

	for (i = 0; i < NR_OF_TX_BUFFERS; i++) {
		retval = dmw96dma_add_io_transfer_buf(
				uap->dma_tx_list,
				(void *)&uap->tx_buffer[i * TX_BUFFER_ELEMENTS],
				SIZE_OF_TX_BUFFER,
				sw_uart_tx_buffer_callback,
				(void *)&uap->tx_buffer_callback[i]);
		if (retval)
			goto out_err_free_tx_list;
	}

	retval = dmw96dma_submit(uap->dma_rx_list);
	if (retval) {
		dev_err(uap->port.dev, "failed to submit DMA RX list: %d\n",
			retval);
		goto out_err_free_tx_list;
	}
	retval = dmw96dma_submit(uap->dma_tx_list);
	if (retval) {
		dev_err(uap->port.dev, "failed to submit DMA TX list: %d\n",
			retval);
		goto out_err_cancel_rx_list;
	}

	atomic_set(&uap->do_rx, 1);

	/* enable TDM */
	tdm_write(port, 1, TDM_ENABLE);


	/* start RX thread */
	uap->rx_thread = kthread_run(sw_uart_rx_thread,
				     (void *)uap,
				     "sw uart rx thread");
	if (IS_ERR(uap->rx_thread)) {
		retval = PTR_ERR(uap->rx_thread);
		dev_err(uap->port.dev, "failed to start RX thread: %d\n", retval);
		goto out_err_cancel_tx_list;
	}

	/* start TX thread */
	uap->tx_thread = kthread_run(sw_uart_tx_thread,
				     (void *)uap,
				     "sw uart tx thread");
	if (IS_ERR(uap->tx_thread)) {
		retval = PTR_ERR(uap->tx_thread);
		dev_err(uap->port.dev, "failed to start TX thread: %d\n", retval);
		goto out_err_stop_rx_thread;
	}

	dev_dbg(uap->port.dev, "%s: UART running\n", __func__);

	return 0;

out_err_stop_rx_thread:
	kthread_stop(uap->rx_thread);
	/* XXX */
	uap->rx_thread = NULL;
out_err_cancel_tx_list:
	dmw96dma_cancel(uap->dma_tx_list);
	tdm_write(port, 0, TDM_ENABLE);
	atomic_set(&uap->do_rx, 0);
out_err_cancel_rx_list:
	dmw96dma_cancel(uap->dma_rx_list);
out_err_free_tx_list:
	dmw96dma_free_list(uap->dma_tx_list);
out_err_free_rx_list:
	dmw96dma_free_list(uap->dma_rx_list);
out_err_clk_off:
	clk_disable(uap->clk);
out_err_free_irq:
	free_irq(uap->port.irq, uap);
out:
	/* put TDM back to reset */
	reset_set(uap->reset);
	return retval;
}

static void sw_uart_shutdown(struct uart_port *port)
{
	struct sw_port *uap = to_ourport(port);

	dev_dbg(uap->port.dev, "%s", __func__);

	/* stop RX thread if still running */
	if (uap->rx_thread) {
		kthread_stop(uap->rx_thread);
		uap->rx_thread = NULL;
	}

	/* stop TX thread if still running */
	if (uap->tx_thread) {
		kthread_stop(uap->tx_thread);
		uap->tx_thread = NULL;
	}
	/*
	 * disable TDM and put it into reset
	 */
	tdm_write(port, 0, TDM_ENABLE);
	reset_set(uap->reset);

	clk_disable(uap->clk);

	free_irq(uap->port.irq, uap);

	dmw96dma_cancel(uap->dma_rx_list);
	wait_for_completion(&uap->rx_list_done);
	dmw96dma_free_list(uap->dma_rx_list);

	dmw96dma_cancel(uap->dma_tx_list);
	wait_for_completion(&uap->tx_list_done);
	dmw96dma_free_list(uap->dma_tx_list);
	dev_dbg(uap->port.dev, "%s: UART stopped\n", __func__);
}

static void
sw_uart_set_termios(struct uart_port *port, struct ktermios *termios,
                    struct ktermios *old)
{
	unsigned int baud;
	long max_rate;
	long min_rate;
	int ret;
	unsigned int bits = 0;
	struct sw_port *sport = to_ourport(port);

	dev_dbg(sport->port.dev, "%s", __func__);

	memcpy(&sport->termios, termios, sizeof(*termios));

	/* check byte size */
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		bits += 5;
		break;
	case CS6:
		bits += 6;
		break;
	case CS7:
		bits += 7;
		break;
	default:
		bits += 8;
		break;
	}

	/* parity bit */
	if (termios->c_cflag & PARENB)
		bits++;

	/* 1 or 2 stop bits */
	if (termios->c_cflag & CSTOPB)
		bits += 2;
	else
		bits ++;

	max_rate = clk_round_rate(sport->clk, OVERSAMPLE_RATE * MAX_BAUD);
	if (max_rate <= 0) {
		dev_err(port->dev,
			"%s: failed to get max rate: %ld", __func__,
			max_rate);
	}
	dev_dbg(port->dev, "%s: max rate %ld", __func__, max_rate);

	min_rate = clk_round_rate(sport->clk, 1);
	if (min_rate <= 0) {
		dev_err(port->dev,
			"%s: failed to get min rate: %ld", __func__,
			min_rate);
	}
	dev_dbg(port->dev, "%s: min rate %ld", __func__, min_rate);

	/*
	 * Ask the core to calculate the divisor for us.
	 */
	baud = uart_get_baud_rate(port,
				  termios,
				  old,
				  min_rate / OVERSAMPLE_RATE,
				  max_rate / OVERSAMPLE_RATE);

	/* TODO: support XON/XOFF? */

	/*
	 * Update the per-port timeout.
	 */
	sport->port.fifosize = (NR_OF_TX_BUFFERS * TX_BUFFER_ELEMENTS / bits);
	uart_update_timeout(port, termios->c_cflag, baud);

	dev_dbg(port->dev, "%s: baud = %u ", __func__, baud);

	/* restart everything */
	sw_uart_shutdown(port);
	ret = sw_uart_startup(port);
	if (ret)
		dev_err(port->dev, "%s: failed to startup the UART", __func__);

	dev_dbg(port->dev, "%s: done\n", __func__);
}

static const char *sw_uart_type(struct uart_port *port)
{
	struct sw_port *uap = to_ourport(port);

	return uap->port.type == PORT_DSPG ? "DSPG_SW_UART" : NULL;
}

/*
 * Release the memory region(s) being used by 'port'
 */
static void sw_uart_release_port(struct uart_port *port)
{
	struct sw_port *uap = to_ourport(port);

	release_mem_region(port->mapbase,
			   uap->mem_res->end - uap->mem_res->start + 1);
}

/*
 * Request the memory region(s) being used by 'port'
 */
static int sw_uart_request_port(struct uart_port *port)
{
	struct sw_port *uap = to_ourport(port);

	if (!request_mem_region(port->mapbase,
				uap->mem_res->end - uap->mem_res->start + 1,
				"TDM_SW_UART")) {
		dev_err(port->dev, "Memory region busy\n");
		return -EBUSY;
	}

	port->membase = ioremap(port->mapbase,
				uap->mem_res->end - uap->mem_res->start + 1);
	if (!port->membase) {
		dev_err(port->dev, "Unable to map registers\n");
		release_mem_region(port->mapbase,
				   uap->mem_res->end - uap->mem_res->start + 1);
		return -EBUSY;
	}

	return 0;
}

/*
 * Configure/autoconfigure the port.
 */
static void
sw_uart_config_port(struct uart_port *port, int flags)
{
	struct sw_port *sport = to_ourport(port);

	if (flags & UART_CONFIG_TYPE && sw_uart_request_port(&sport->port) == 0)
		sport->port.type = PORT_DSPG;
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int
sw_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_DSPG)
		ret = -EINVAL;
	if (ser->irq < 0 || ser->irq >= NR_IRQS)
		ret = -EINVAL;
	if (ser->baud_base < 9600)
		ret = -EINVAL;
	return ret;
}

static int
sw_uart_ioctl(struct uart_port *port, unsigned int cmd, unsigned long arg)
{
	return -ENOIOCTLCMD;
}

static void
sw_uart_pm(struct uart_port *port, unsigned int state, unsigned int oldstate)
{
	struct sw_port *sport = to_ourport(port);

	/* TODO: check if the TDM is still functional after this */
	if (!state)
		clk_enable(sport->clk);
	else
		clk_disable(sport->clk);
}

static struct uart_ops sw_uart_pops = {
	.pm		= sw_uart_pm,
	.tx_empty	= sw_uart_tx_empty,
	.set_mctrl	= sw_uart_set_mctrl,
	.get_mctrl	= sw_uart_get_mctrl,
	.stop_tx	= sw_uart_stop_tx,
	.start_tx	= sw_uart_start_tx,
	.stop_rx	= sw_uart_stop_rx,
	.enable_ms	= sw_uart_enable_ms,
	.break_ctl	= sw_uart_break_ctl,
	.startup	= sw_uart_startup,
	.shutdown	= sw_uart_shutdown,
	.set_termios	= sw_uart_set_termios,
	.type		= sw_uart_type,
	.release_port	= sw_uart_release_port,
	.request_port	= sw_uart_request_port,
	.config_port	= sw_uart_config_port,
	.verify_port	= sw_uart_verify_port,
	.ioctl		= sw_uart_ioctl,
};

static struct uart_driver sw_uart = {
	.owner		= THIS_MODULE,
	.driver_name	= "sw-uart",
	.dev_name	= SERIAL_SW_NAME,
	.major		= SERIAL_SW_MAJOR,
	.minor		= SERIAL_SW_MINOR,
	.nr		= ARRAY_SIZE(sw_ports),
};

static int sw_uart_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *mem_res, *irq_res, *dma_res;
	struct sw_port *uap;
	int ret = 0;
	int id;
	struct clk *clk;

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_res) {
		dev_err(&pdev->dev, "failed to get IO resource MEM\n");
		return -ENODEV;
	}

	irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq_res) {
		dev_err(&pdev->dev, "failed to get IO resource IRQ\n");
		return -ENODEV;
	}

	/*
	 * Determine port number. Either get it from the device tree 'aliases'
	 * section or from the platform_device.id field.
	 */
	if (np)
		id = of_alias_get_id(np, "serial");
	else
		id = pdev->id;

	if (id < 0) {
		/* port id not found in platform data nor device-tree aliases:
		 * auto-enumerate it */
		id = 0;
		while (id < ARRAY_SIZE(sw_ports) && sw_ports[id].port.mapbase)
			id++;
	}

	if (id > ARRAY_SIZE(sw_ports)) {
		dev_err(&pdev->dev, "number of supported ports exceeded\n");
		return -ENODEV;
	}

	uap = &sw_ports[id];
	uap->mem_res = mem_res;

	if (uap->port.mapbase) {
		dev_err(&pdev->dev, "port %d already in use\n", id);
		return -EBUSY;
	}

	uap->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(uap->clk)) {
		ret = PTR_ERR(uap->clk);
		dev_err(&pdev->dev, "no clock: %d\n", ret);
		return ret;
	}

	if (!np) {
		dev_err(&pdev->dev, "missing device tree node\n");
		return -EINVAL;
	}
	ret = of_property_read_u32(np, "reset", &uap->reset);
	if (ret) {
		dev_err(&pdev->dev, "missing 'reset' property!\n");
		return -EINVAL;
	}

	uap->port.dev =         &pdev->dev;
	uap->port.mapbase =     mem_res->start;
	uap->port.membase =     NULL;
	uap->port.iotype =      UPIO_MEM;
	uap->port.irq =         irq_res->start;
	/*
	 * TX fifo size
	 *  assuming maximum settins her, 8 data bit, 1 parity, start, 2 stop + 1
	 *  (13)
	 */
	uap->port.fifosize =    (NR_OF_TX_BUFFERS * TX_BUFFER_ELEMENTS / 13);
	uap->port.ops =         &sw_uart_pops;
	uap->port.flags =       UPF_BOOT_AUTOCONF;
	uap->port.line =        id;
	uap->port.uartclk =     0;

	clk = clk_get_parent(uap->clk);
	if (!IS_ERR(clk))
		uap->port.uartclk = clk_get_rate(clk);

	/* get DMA channel for RX */
	dma_res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!dma_res) {
		ret = -ENODEV;
		goto err_clk_put;
	}
	uap->dma_rx_channel = dma_res->start;

	/* get DMA channel for TX */
	dma_res = platform_get_resource(pdev, IORESOURCE_DMA, 1);
	if (!dma_res) {
		ret = -ENODEV;
		goto err_clk_put;
	}
	uap->dma_tx_channel = dma_res->start;

	init_waitqueue_head(&uap->thread_queue);

	/* encode initial baud rate into private termios */
	tty_termios_encode_baud_rate(&uap->termios, 9600, 9600);

	ret = uart_add_one_port(&sw_uart, &uap->port);
	if (ret) {
		dev_err(&pdev->dev, "adding port failed: %d\n", ret);
		goto err_clk_put;
	}
	platform_set_drvdata(pdev, uap);

	dev_info(&pdev->dev,
		 "software UART%d via TDM successfully probed\n", id);

	ret = 0;
	goto out;
err_clk_put:
	clk_put(uap->clk);
	uap->clk = NULL;
out:
	return ret;
}

static int sw_uart_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sw_port *sport = platform_get_drvdata(pdev);

	return uart_suspend_port(&sw_uart, &sport->port);
}

static int sw_uart_resume(struct platform_device *pdev)
{
	struct sw_port *sport = platform_get_drvdata(pdev);

	return uart_resume_port(&sw_uart, &sport->port);
}

static int sw_uart_remove(struct platform_device *pdev)
{
	struct sw_port *sport = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

	uart_remove_one_port(&sw_uart, &sport->port);
	clk_put(sport->clk);

	return 0;
}

static struct of_device_id sw_uart_match_table[] = {
	{ .compatible = "dspg,tdm-uart", },
	{}
};

static struct platform_driver sw_uart_driver = {
	.probe		= sw_uart_probe,
	.remove		= sw_uart_remove,
	.suspend	= sw_uart_suspend,
	.resume		= sw_uart_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "dspg-tdm-uart",
		.of_match_table = sw_uart_match_table,
	},
};

static int __init sw_uart_init(void)
{
	int ret;

	ret = uart_register_driver(&sw_uart);
	if (ret)
		return ret;

	ret = platform_driver_register(&sw_uart_driver);
	if (ret)
		uart_unregister_driver(&sw_uart);

	return ret;
}

static void __exit sw_uart_exit(void)
{
	platform_driver_unregister(&sw_uart_driver);
	uart_unregister_driver(&sw_uart);
}

module_init(sw_uart_init);
module_exit(sw_uart_exit);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("Software Serial Port driver");
MODULE_LICENSE("GPL");
