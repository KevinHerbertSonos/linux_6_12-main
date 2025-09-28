/*
 * drivers/mtd/devices/dspg-qspic.c
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>

/* common registers */
#define QSPIC_CFG		0x000
#define QSPIC_STS		0x008
#define QSPIC_ISR		0x00c
#define QSPIC_IER		0x010
#define QSPIC_ICR		0x014

#define QSPIC_STS_BUSY		(1UL << 31)
#define QSPIC_STS_IDLE		(1UL <<  0)

#define QSPIC_INT_APBTRSFR_DONE   (1 << 6)

/* write and read buffer exposed via register */
#define QSPIC_FWD		0x020
#define QSPIC_FRD		0x040
#define QSPIC_DATASIZE		0x020

/* per-chipselect register offsets */
#define QSPIC_RDTRSTP		0x03c
#define QSPIC_RDCMD		0x040
#define QSPIC_PPTRSTP		0x044
#define QSPIC_PPCMD		0x048
#define QSPIC_MEMADD		0x054

#define QSPIC_RDTRSTP_OPCODE(x)		((x) <<  0)
#define QSPIC_RDTRSTP_ADD(x)		((x) <<  2)
#define QSPIC_RDTRSTP_DUMMY(x)		((x) <<  4)
#define QSPIC_RDTRSTP_DIN(x)		((x) <<  8)
#define QSPIC_RDTRSTP_ADDMODE(x)	((x) << 10)
#define QSPIC_RDTRSTP_DUMMY_SIZE(x)	((x) << 12)
#define QSPIC_RDTRSTP_HAS_OPCODE	(1   << 19)
#define QSPIC_RDTRSTP_HAS_MODE		(1   << 20)

#define QSPIC_PPTRSTP_OPCODE(x)		((x) <<  0)
#define QSPIC_PPTRSTP_ADD(x)		((x) <<  2)
#define QSPIC_PPTRSTP_DUMMY(x)		((x) <<  4)
#define QSPIC_PPTRSTP_DOUT(x)		((x) <<  6)
#define QSPIC_PPTRSTP_ADDMODE(x)	((x) << 10)
#define QSPIC_PPTRSTP_HAS_OPCODE	(1   << 19)

/* common chip select registers */
#define QSPIC_TRSTP		0x300
#define QSPIC_CMD		0x304
#define QSPIC_ADD		0x308
#define QSPIC_CSTIMING		0x408

#define QSPIC_TRSTP_OPCODE(x)	((x) <<  0)
#define QSPIC_TRSTP_ADD(x)	((x) <<  2)
#define QSPIC_TRSTP_DUMMY(x)	((x) <<  4)
#define QSPIC_TRSTP_DOUT(x)	((x) <<  6)
#define QSPIC_TRSTP_DIN(x)	((x) <<  8)
#define QSPIC_TRSTP_ADDMODE(x)	((x) << 10)
#define QSPIC_TRSTP_HAS_OPCODE	(1   << 19)
#define QSPIC_TRSTP_SIZE(x)	((x) << 20)

#define TRSTP_SERIAL		0
#define TRSTP_DUAL		1
#define TRSTP_QUAD		2

#define TRSTP_ADD_3B		1
#define TRSTP_ADD_4B		2

#define QSPIC_CMD_OPCODE(x)	((x) <<  0)
#define QSPIC_CMD_TYPE(x)	((x) <<  8)
#define QSPIC_CMD_READ		(1   << 13)
#define QSPIC_CMD_WRITE		(1   << 14)
#define QSPIC_CMD_CS(x)		((x) << 30)
#define QSPIC_CMD_START		(1   << 31)

#define QSPIC_CMD_TYPE_SECT_ERASE	QSPIC_CMD_TYPE(0)
#define QSPIC_CMD_TYPE_CHIP_ERASE	QSPIC_CMD_TYPE(1)
#define QSPIC_CMD_TYPE_READ_STATUS	QSPIC_CMD_TYPE(5)
#define QSPIC_CMD_TYPE_WRITE_STATUS	QSPIC_CMD_TYPE(6)
#define QSPIC_CMD_TYPE_OTHER		QSPIC_CMD_TYPE(9)

/* Common SPI commands */
#define CMD_RDSR	(QSPIC_CMD_OPCODE(0x05) | \
			 QSPIC_CMD_TYPE_READ_STATUS | \
			 QSPIC_CMD_READ)
#define CMD_WRSR	(QSPIC_CMD_OPCODE(0x01) | \
			 QSPIC_CMD_TYPE_WRITE_STATUS | \
			 QSPIC_CMD_WRITE)
#define CMD_RDID	(QSPIC_CMD_OPCODE(0x9f) | \
			 QSPIC_CMD_TYPE_OTHER | \
			 QSPIC_CMD_READ)
#define CMD_CE		(QSPIC_CMD_OPCODE(0xc7) | \
			 QSPIC_CMD_TYPE_CHIP_ERASE)
#define CMD_BE		(QSPIC_CMD_OPCODE(0x20) | \
			 QSPIC_CMD_TYPE_SECT_ERASE)
#define CMD_SE		(QSPIC_CMD_OPCODE(0xd8) | \
			 QSPIC_CMD_TYPE_SECT_ERASE)
#define CMD_WREN	(QSPIC_CMD_OPCODE(0x06) | \
			 QSPIC_CMD_TYPE_OTHER)
#define CMD_RSTEN	(QSPIC_CMD_OPCODE(0x66) | \
			 QSPIC_CMD_TYPE_OTHER)
#define CMD_RST		(QSPIC_CMD_OPCODE(0x99) | \
			 QSPIC_CMD_TYPE_OTHER)

#define BUSY	0x01
#define WEL	0x02

#define MAX_READY_WAIT_JIFFIES (4 * HZ)

struct qspic;

struct chipsel {
	struct qspic	*cntlr;
	int		cs;
	struct mtd_info	mtd;

	struct resource *shadow_res;
	void		*shadow_buf;

	int (*wait)(struct chipsel *flash);

	unsigned int	sector_size;	/* erasable unit size */
	unsigned int	n_sectors;	/* number of erase sectors */
	unsigned short	page_size;	/* programmable unit size */
	u32		aux_cfg;

	unsigned short	read_freq;
	unsigned short	write_freq;
};

struct qspic {
	struct platform_device	*pdev;
	struct chipsel		*cs[2];
	void			*regs;
	struct clk		*clk;
	struct mutex		lock;
	struct completion	done;
	int			irq;
	char			buf[256];
	int			max_speed;
};

struct flash_info {
	const char	*name;
	u32		jedec_id;
	u16		ext_id;

	unsigned short	read_freq;	/* maximum read frequency in MHz */
	unsigned short	write_freq;	/* maximum write frequency in MHz */
	unsigned short	page_size;	/* programmable unit size */
	unsigned int	sector_size;	/* erasable unit size */
	unsigned int	n_sectors;	/* number of erase sectors */

	int (*init)(struct chipsel *flash, struct flash_info *info);

	int (*wait)(struct chipsel *flash);

	u32	read_cfg;
	u32	read_cmd;
	u32	write_cfg;
	u32	write_cmd;
	u32	aux_cfg;	/* ADD_MODE and *_TRNSFR_TYPE fields */
};

static inline struct chipsel *
mtd_to_chipsel(struct mtd_info *mtd)
{
	return container_of(mtd, struct chipsel, mtd);
}

static int
dspg_qspic_set_freq(struct qspic *cntlr, unsigned int mhz)
{
	long rate = mhz * 1000000;

	if (cntlr->max_speed && (cntlr->max_speed < rate))
		rate = cntlr->max_speed;

	rate = clk_round_rate(cntlr->clk, rate);
	if (rate < 0)
		return rate;

	return clk_set_rate(cntlr->clk, rate);
}

static void
dspg_qspic_cmd(struct qspic *cntlr, int cs, u32 cmd, u32 cfg)
{
	u32 reg;

	/* wait until controller is idle */
	reg = readl(cntlr->regs + QSPIC_STS);
	while ((reg & QSPIC_STS_BUSY) || !(reg & QSPIC_STS_IDLE)) {
		cond_resched();
		reg = readl(cntlr->regs + QSPIC_STS);
	}

	cfg |= QSPIC_TRSTP_HAS_OPCODE;
	cmd |= QSPIC_CMD_CS(cs) |
	       QSPIC_CMD_START;

	writel(cfg, cntlr->regs + QSPIC_TRSTP);
	writel(cmd, cntlr->regs + QSPIC_CMD);

	wait_for_completion(&cntlr->done);
}

static void
dspg_qspic_readbuf(struct qspic *cntlr, void *dst, unsigned int len)
{
	const u8 *p = (const u8 *)(cntlr->regs + QSPIC_FRD);
	u8 *buf = dst;

	/* the data is backwards, so move to the end of the buffer */
	p += QSPIC_DATASIZE;

	while (len--)
		*buf++ = *--p;
}

static void
dspg_qspic_writebuf(struct qspic *cntlr, const void *src, unsigned int len)
{
	u32 *p = (u32 *)(cntlr->regs + QSPIC_FWD + QSPIC_DATASIZE);
	const u8 *buf = src;
	u32 data;
	u8 *tmp = (u8 *)&data;

	if (len > QSPIC_DATASIZE)
		len = QSPIC_DATASIZE;

	/* the data is backwards, so move to the end of the buffer */
	while (len) {
		data = 0xffffffff;
		if (len) {
			len--;
			tmp[3] = *buf++;
		}
		if (len) {
			len--;
			tmp[2] = *buf++;
		}
		if (len) {
			len--;
			tmp[1] = *buf++;
		}
		if (len) {
			len--;
			tmp[0] = *buf++;
		}
		*--p = data;
	}
}

static void
dspg_qspic_aux_cmd(struct chipsel *flash, u32 cmd)
{
	u32 cfg = flash->aux_cfg;

	cfg &= ~QSPIC_TRSTP_ADDMODE(3); /* no address required */

	dspg_qspic_cmd(flash->cntlr, flash->cs, cmd, cfg);
}

static void
dspg_qspic_aux_read(struct chipsel *flash, u32 cmd, void *buf, unsigned int len)
{
	u32 cfg = flash->aux_cfg;

	cfg &= ~QSPIC_TRSTP_ADDMODE(3); /* no address required */
	cfg |= QSPIC_TRSTP_SIZE(len);

	dspg_qspic_cmd(flash->cntlr, flash->cs, cmd, cfg);
	dspg_qspic_readbuf(flash->cntlr, buf, len);
}

static void
dspg_qspic_aux_write(struct chipsel *flash, u32 cmd, const void *buf,
		     unsigned int len)
{
	u32 cfg = flash->aux_cfg;

	cfg &= ~QSPIC_TRSTP_ADDMODE(3); /* no address required */
	cfg |= QSPIC_TRSTP_SIZE(len);

	dspg_qspic_writebuf(flash->cntlr, buf, len);
	dspg_qspic_cmd(flash->cntlr, flash->cs, cmd, cfg);
}

static int
dspg_qspic_wait(struct chipsel *flash)
{
	unsigned long deadline;
	int ret;

	deadline = jiffies + MAX_READY_WAIT_JIFFIES;

	do {
		ret = flash->wait(flash);
		if (ret <= 0)
			return ret;

		cond_resched();

	} while (!time_after_eq(jiffies, deadline));

	return -ETIMEDOUT;
}

static int
dspg_qspic_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen,
		u_char *buf)
{
	struct chipsel *flash = mtd_to_chipsel(mtd);

	mutex_lock(&flash->cntlr->lock);

	*retlen = len;
	memcpy(buf, flash->shadow_buf + from, len);

	mutex_unlock(&flash->cntlr->lock);

	return 0;
}

static void
dspg_qspic_write_enable(struct chipsel *flash)
{
	u8 sr1;

	dspg_qspic_aux_cmd(flash, CMD_WREN);

	do {
		dspg_qspic_aux_read(flash, CMD_RDSR, &sr1, 1);
	} while ((sr1 & WEL) == 0);
}

static int
dspg_qspic_write_sect(struct chipsel *flash, u32 off, u32 len,
		      const u_char *buf)
{
	unsigned long flags;
	unsigned long src = (unsigned long)&flash->cntlr->buf;
	unsigned long dest = (unsigned long)(flash->shadow_buf + off);
	u32 real_len = len;
	int i;

	if (real_len & 3)
		real_len = (real_len + 3) & ~3;

	dspg_qspic_write_enable(flash);

	/*
	 * Disable interrupts because the write must not be disturbed.
	 * Otherwise the write will end prematurely when the FIFO runs empty.
	 * Copy the data first to make sure it is in the cache (DRAM access
	 * might be too slow).
	 */
	local_irq_save(flags);

	/* memcpy(flash->cntlr->buf + len, flash->shadow_buf + off + len,
	 *        real_len - len);
	 * memcpy(flash->cntlr->buf, buf, len);
	 */
	*(unsigned long *)(src + real_len - 4) =
				*(unsigned long *)(dest + real_len - 4);

	for (i = 0; i < real_len; i += 4)
		*(unsigned long *)(src + i) = *(unsigned long *)(buf + i);

	for (i = 0; i < real_len; i += 4)
		*(unsigned long *)(dest + i) = *(unsigned long *)(src + i);

	local_irq_restore(flags);

	return dspg_qspic_wait(flash);
}

static int
dspg_qspic_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen,
		 const u_char *buf)
{
	struct chipsel *flash = mtd_to_chipsel(mtd);
	u32 page_offset, page_size;
	int ret;

	if (len == 0)
		return 0;

	mutex_lock(&flash->cntlr->lock);

	*retlen = 0;
	/* do all the bytes fit onto one page? */
	page_offset = to & (flash->page_size - 1);
	if (page_offset + len <= flash->page_size) {
		ret = dspg_qspic_write_sect(flash, to, len, buf);
		if (ret == 0)
			*retlen = len;
	} else {
		page_size = flash->page_size - page_offset;
		while (len) {
			ret = dspg_qspic_write_sect(flash, to, page_size, buf);
			if (ret)
				goto out;

			len -= page_size;
			to += page_size;
			buf += page_size;
			*retlen += page_size;

			page_size = (len > flash->page_size) ?
				flash->page_size : len;
		}
	}

out:
	mutex_unlock(&flash->cntlr->lock);
	return ret;
}

static int
dspg_qspic_erase_chip(struct chipsel *flash)
{
	dspg_qspic_write_enable(flash);
	dspg_qspic_aux_cmd(flash, CMD_CE);

	return dspg_qspic_wait(flash);
}

static int
dspg_qspic_erase_sector(struct chipsel *flash, u32 addr)
{
	dspg_qspic_write_enable(flash);

	writel(addr, flash->cntlr->regs + QSPIC_ADD);
	dspg_qspic_cmd(flash->cntlr, flash->cs, CMD_SE, flash->aux_cfg);

	return dspg_qspic_wait(flash);
}

static int
dspg_qspic_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct chipsel *flash = mtd_to_chipsel(mtd);
	u32 addr, len;
	uint32_t rem;
	int ret;

	div_u64_rem(instr->len, mtd->erasesize, &rem);
	if (rem)
		return -EINVAL;

	addr = instr->addr;
	len = instr->len;

	mutex_lock(&flash->cntlr->lock);

	if (len == flash->mtd.size) {
		ret = dspg_qspic_erase_chip(flash);
	} else {
		while (len) {
			ret = dspg_qspic_erase_sector(flash, addr);
			if (ret) {
				instr->fail_addr = addr;
				goto out;
			}

			addr += mtd->erasesize;
			len -= mtd->erasesize;
		}
	}

out:
	mutex_unlock(&flash->cntlr->lock);

	return ret;
}

static irqreturn_t
dspg_qspic_irq_handler(int irq, void *dev)
{
	struct qspic *cntlr = (struct qspic *)dev;
	int pending;

	pending = readl(cntlr->regs + QSPIC_ISR);
	if (pending & QSPIC_INT_APBTRSFR_DONE) {
		writel(QSPIC_INT_APBTRSFR_DONE, cntlr->regs + QSPIC_ICR);
		complete(&cntlr->done);
		while (readl(cntlr->regs + QSPIC_ISR) &
		       QSPIC_INT_APBTRSFR_DONE)
			;
		writel(0, cntlr->regs + QSPIC_ICR); /* not self clearing */

		return IRQ_HANDLED;
	} else
		return IRQ_NONE;
}

static int
dspg_qspic_wait_generic(struct chipsel *flash)
{
	u8 status;

	dspg_qspic_aux_read(flash, CMD_RDSR, &status, 1);

	return status & (WEL | BUSY);
}

static void
dspg_qspic_wait_write_disable(struct chipsel *flash)
{
	do {
		udelay(1000);
	} while (dspg_qspic_wait_generic(flash));
}

static void
dspg_qspic_reset_generic(struct chipsel *flash)
{
	dspg_qspic_aux_cmd(flash, CMD_RSTEN);
	dspg_qspic_aux_cmd(flash, CMD_RST);
}

/*
 * ATMEL specific functions...
 */

static int
dspg_qspic_init_atmel(struct chipsel *flash, struct flash_info *info)
{
	u8 status = 0;

	/* un-protect all sectors */
	dspg_qspic_write_enable(flash);
	dspg_qspic_aux_write(flash, CMD_WRSR, &status, 1);

	return 0;
}

static int
dspg_qspic_wait_atmel(struct chipsel *flash)
{
	u8 status;

	dspg_qspic_aux_read(flash, CMD_RDSR, &status, 1);

	/* first check ready/busy */
	if (status & BUSY)
		return 1;

	/* if ready, check Erase/Program Error */
	return (status & (1 << 5)) ? -EIO : 0;
}

static int
dspg_qspic_init_sst16_qpi(struct chipsel *flash, struct flash_info *info)
{
	u8 unlock[6] = { 0, 0, 0, 0, 0, 0 };

	/* enter QPI mode */
	dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0x38) |
			   QSPIC_CMD_TYPE_OTHER);

	/* we are now in QPI mode! */
	flash->aux_cfg = info->aux_cfg;

	/* un-protect all sectors */
	dspg_qspic_write_enable(flash);
	dspg_qspic_aux_write(flash, 0x42, &unlock, 6);

	return 0;
}

static int
dspg_qspic_init_sst32_qpi(struct chipsel *flash, struct flash_info *info)
{
	u8 unlock[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	/* enter QPI mode */
	dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0x38) |
			   QSPIC_CMD_TYPE_OTHER);

	/* we are now in QPI mode! */
	flash->aux_cfg = info->aux_cfg;

	/* un-protect all sectors */
	dspg_qspic_write_enable(flash);
	dspg_qspic_aux_write(flash, 0x42, &unlock, 10);

	return 0;
}

static int
dspg_qspic_wait_sst(struct chipsel *flash)
{
	u8 status;

	dspg_qspic_aux_read(flash, CMD_RDSR, &status, 1);

	return status & (1<<7); /* check WIP */
}

/*
 * WINBOND specific functions...
 */

static int
dspg_qspic_init_winbond_qpi(struct chipsel *flash, struct flash_info *info)
{
	u8 sr2, params;

	/* volatile set QE in status register 2 */
	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x35) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &sr2, 1);
	sr2 |= 1 << 1;
	dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0x50) |
			   QSPIC_CMD_TYPE_OTHER);
	dspg_qspic_aux_write(flash, QSPIC_CMD_OPCODE(0x31) |
			     QSPIC_CMD_TYPE_WRITE_STATUS | QSPIC_CMD_WRITE,
			     &sr2, 1);

	/* enter QPI mode */
	dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0x38) |
			   QSPIC_CMD_TYPE_OTHER);

	/* we are now in QPI mode! */
	flash->aux_cfg = info->aux_cfg;

	/* set read parameters: 6 dummy cyles; must be done in QPI mode */
	params = 0x2 << 4;
	dspg_qspic_aux_write(flash, QSPIC_CMD_OPCODE(0xc0) | QSPIC_CMD_WRITE |
			     QSPIC_CMD_TYPE_OTHER, &params, 1);

	return 0;
}

static int
dspg_qspic_init_winbond16_qpi(struct chipsel *flash, struct flash_info *info)
{
	u8 sr[2];

	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x05) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &sr[0], 1);

	/* volatile set QE in status register 2 */
	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x35) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &sr[1], 1);
	sr[1] |= 1 << 1;
	dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0x50) |
			   QSPIC_CMD_TYPE_OTHER);
	dspg_qspic_aux_write(flash, QSPIC_CMD_OPCODE(0x01) |
			     QSPIC_CMD_TYPE_WRITE_STATUS | QSPIC_CMD_WRITE,
			     sr, 2);

	/* we are now in QSPI mode! */
	flash->aux_cfg = info->aux_cfg;

	return 0;
}

static int
dspg_qspic_init_winbond256_qpi(struct chipsel *flash, struct flash_info *info)
{
	u8 sr2, sr3, params;

	dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0xe9) |
			   QSPIC_CMD_TYPE_OTHER);

	/* volatile set QE in status register 2 */
	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x35) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &sr2, 1);
	sr2 |= 1 << 1;
	dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0x50) |
			   QSPIC_CMD_TYPE_OTHER);
	dspg_qspic_aux_write(flash, QSPIC_CMD_OPCODE(0x31) |
		QSPIC_CMD_TYPE_WRITE_STATUS | QSPIC_CMD_WRITE, &sr2, 1);

	if (info->read_cfg & QSPIC_RDTRSTP_ADDMODE(TRSTP_ADD_4B)) {
		/* enter 4B mode */
		do {
			dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0xb7) |
					   QSPIC_CMD_TYPE_OTHER);
			dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x15) |
					    QSPIC_CMD_TYPE_READ_STATUS |
					    QSPIC_CMD_READ, &sr3, 1);
		} while ((sr3 & 0x1) != 0x1);
	}

	/* enter QPI mode */
	dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0x38) |
			   QSPIC_CMD_TYPE_OTHER);

	/* we are now in QPI & 4B mode! */
	flash->aux_cfg = info->aux_cfg;

	/* set read parameters: 6 dummy cyles; must be done in QPI mode */
	params = 0x2 << 4;
	dspg_qspic_aux_write(flash, QSPIC_CMD_OPCODE(0xc0) | QSPIC_CMD_WRITE |
			     QSPIC_CMD_TYPE_OTHER, &params, 1);

	return 0;
}

static int
dspg_qspic_init_mx66l_qpi(struct chipsel *flash, struct flash_info *info)
{
	u8 sr[2];

	dspg_qspic_aux_read(flash, CMD_RDSR, &sr[0], 1);
	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x15) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &sr[1], 1);

	sr[0] &= ~(0x3c); /* disable block protection */
	sr[0] |= 1 << 6;  /* set QE */
	sr[1] &= 0x3f;    /* set 6 dummy cycles */

	dspg_qspic_write_enable(flash);
	dspg_qspic_aux_write(flash, CMD_WRSR, sr, 2);
	dspg_qspic_wait_write_disable(flash);

	if (info->read_cfg & QSPIC_RDTRSTP_ADDMODE(TRSTP_ADD_4B)) {
		/* enter 4B mode */
		dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0xb7) |
				   QSPIC_CMD_TYPE_OTHER);
		do {
			dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x15) |
					    QSPIC_CMD_TYPE_READ_STATUS |
					    QSPIC_CMD_READ, &sr[0], 1);
		} while ((sr[0] & 0x20) != 0x20);
	}

	/* enable QPI mode */
	dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0x35) |
			   QSPIC_CMD_TYPE_OTHER);

	/* we are now in QPI mode! */
	flash->aux_cfg = info->aux_cfg;

	return 0;
}

static int
dspg_qspic_init_n25q_qpi(struct chipsel *flash, struct flash_info *info)
{
	unsigned char sr;

	/* status register */
	dspg_qspic_aux_read(flash, CMD_RDSR, &sr, 1);
	sr &= ~(0x5c); /* disable block protection */
	dspg_qspic_write_enable(flash);
	dspg_qspic_aux_write(flash, CMD_WRSR, &sr, 1);
	dspg_qspic_wait_write_disable(flash);

	/* volatile configuration register */
	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x85) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &sr, 1);
	sr &= ~(0xf0);
	sr |= 6 << 4; /* set 6 dummy cycles */
	dspg_qspic_write_enable(flash);
	dspg_qspic_aux_write(flash, QSPIC_CMD_OPCODE(0x81) |
		QSPIC_CMD_TYPE_WRITE_STATUS | QSPIC_CMD_WRITE, &sr, 1);
	dspg_qspic_wait_write_disable(flash);

	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x85) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &sr, 1);

	/* enhanced volatile configuration register */
	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x65) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &sr, 1);
	sr &= ~(1 << 7);  /* set QE */
	sr |=   1 << 6;   /* disable dual mode */
	dspg_qspic_write_enable(flash);
	dspg_qspic_aux_write(flash, QSPIC_CMD_OPCODE(0x61) |
		QSPIC_CMD_TYPE_WRITE_STATUS | QSPIC_CMD_WRITE, &sr, 1);

	flash->aux_cfg = info->aux_cfg;

	dspg_qspic_wait_write_disable(flash);

	if (info->read_cfg & QSPIC_RDTRSTP_ADDMODE(TRSTP_ADD_4B)) {
		/* enter 4B mode */
		dspg_qspic_write_enable(flash);
		dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0xb7) |
				   QSPIC_CMD_TYPE_OTHER);
		dspg_qspic_wait_write_disable(flash);
	}

	/* we are now in QPI mode! */
	flash->aux_cfg = info->aux_cfg;

	return 0;
}

static int
dspg_qspic_init_mx25_qpi(struct chipsel *flash, struct flash_info *info)
{
	u8 sr[3], tmp[3];

	/* status register */
	dspg_qspic_aux_read(flash, CMD_RDSR, &sr[0], 1);
	/* configuration register */
	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x15) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &sr[1], 2);

	sr[0] &= ~0xbc;   /* disable SRWD and block protection */
	sr[0] |= 1 << 6;  /* QE enable */
	sr[2] = 2;        /* high-performance mode */

	dspg_qspic_write_enable(flash);
	dspg_qspic_aux_write(flash, CMD_WRSR, &sr[0], 3);
	dspg_qspic_wait_write_disable(flash);

	/* status register */
	dspg_qspic_aux_read(flash, CMD_RDSR, &tmp[0], 1);
	/* configuration register */
	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x15) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &tmp[1], 2);

	/* we are now in 4IO mode! */
	flash->aux_cfg = info->aux_cfg;

	return 0;
}

static int
dspg_qspic_init_gd25q_qspi(struct chipsel *flash, struct flash_info *info)
{
	u8 sr;

	dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x05) |
		QSPIC_CMD_TYPE_READ_STATUS | QSPIC_CMD_READ, &sr, 1);

	/* non-volatile set QE in status register 1 */
	if (!(sr & (1 << 6)) || (sr & 0x3C)) {
		sr |= 1 << 6;
		sr &= ~0x3C;
		dspg_qspic_write_enable(flash);
		udelay(100);
		dspg_qspic_aux_write(flash, QSPIC_CMD_OPCODE(0x01) |
			QSPIC_CMD_TYPE_WRITE_STATUS | QSPIC_CMD_WRITE, &sr, 1);
		dspg_qspic_wait_write_disable(flash);
	}

	if (info->read_cfg & QSPIC_RDTRSTP_ADDMODE(TRSTP_ADD_4B)) {
		/* enter 4B mode */
		dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0xb7) |
				   QSPIC_CMD_TYPE_OTHER);
		do {
			dspg_qspic_aux_read(flash, QSPIC_CMD_OPCODE(0x35) |
					    QSPIC_CMD_TYPE_READ_STATUS |
					    QSPIC_CMD_READ, &sr, 1);
		} while ((sr & 0x20) != 0x20);
	}

	/* we are now in QSPI mode! */
	flash->aux_cfg = info->aux_cfg;

	return 0;
}

#define FLASH_INFO(_name, id, ext, fr, fw, sects)	\
	.name = (_name),				\
	.jedec_id = (id),				\
	.ext_id = (ext),				\
	.read_freq = (fr),				\
	.write_freq = (fw),				\
	.page_size = 256,				\
	.sector_size = 65536,				\
	.n_sectors = (sects)

#define FLASH_QSPI(read, write, addr, dummy)			\
	.read_cfg = QSPIC_RDTRSTP_OPCODE(TRSTP_SERIAL) |	\
		    QSPIC_RDTRSTP_ADD(TRSTP_SERIAL) |		\
		    QSPIC_RDTRSTP_DUMMY(TRSTP_SERIAL) |		\
		    QSPIC_RDTRSTP_DIN(TRSTP_QUAD) |		\
		    QSPIC_RDTRSTP_ADDMODE(addr)	|		\
		    QSPIC_RDTRSTP_DUMMY_SIZE(dummy) |		\
		    QSPIC_RDTRSTP_HAS_OPCODE,			\
	.read_cmd = (read),					\
	.write_cfg = QSPIC_PPTRSTP_OPCODE(TRSTP_SERIAL) |	\
		     QSPIC_PPTRSTP_ADD(TRSTP_SERIAL) |		\
		     QSPIC_PPTRSTP_DOUT(TRSTP_QUAD) |		\
		     QSPIC_PPTRSTP_ADDMODE(addr) |		\
		     QSPIC_PPTRSTP_HAS_OPCODE,			\
	.write_cmd = (write),					\
	.aux_cfg = QSPIC_TRSTP_OPCODE(TRSTP_SERIAL) |		\
		   QSPIC_TRSTP_ADD(TRSTP_SERIAL) |		\
		   QSPIC_TRSTP_DOUT(TRSTP_SERIAL) |		\
		   QSPIC_TRSTP_DIN(TRSTP_SERIAL) |		\
		   QSPIC_TRSTP_ADDMODE(addr)

#define FLASH_4IO(read, write, addr, dummy)			\
	.read_cfg = QSPIC_RDTRSTP_OPCODE(TRSTP_SERIAL) |	\
		    QSPIC_RDTRSTP_ADD(TRSTP_QUAD) |		\
		    QSPIC_RDTRSTP_DUMMY(TRSTP_QUAD) |		\
		    QSPIC_RDTRSTP_DIN(TRSTP_QUAD) |		\
		    QSPIC_RDTRSTP_ADDMODE(addr)	|		\
		    QSPIC_RDTRSTP_DUMMY_SIZE(dummy) |		\
		    QSPIC_RDTRSTP_HAS_OPCODE,			\
	.read_cmd = (read),					\
	.write_cfg = QSPIC_PPTRSTP_OPCODE(TRSTP_SERIAL) |	\
		     QSPIC_PPTRSTP_ADD(TRSTP_QUAD) |		\
		     QSPIC_PPTRSTP_DOUT(TRSTP_QUAD) |		\
		     QSPIC_PPTRSTP_ADDMODE(addr) |		\
		     QSPIC_PPTRSTP_HAS_OPCODE,			\
	.write_cmd = (write),					\
	.aux_cfg = QSPIC_TRSTP_OPCODE(TRSTP_SERIAL) |		\
		   QSPIC_TRSTP_ADD(TRSTP_SERIAL) |		\
		   QSPIC_TRSTP_DOUT(TRSTP_SERIAL) |		\
		   QSPIC_TRSTP_DIN(TRSTP_SERIAL) |		\
		   QSPIC_TRSTP_ADDMODE(addr)

#define FLASH_QPI(read, write, addr, dummy)			\
	.read_cfg = QSPIC_RDTRSTP_OPCODE(TRSTP_QUAD) |		\
		    QSPIC_RDTRSTP_ADD(TRSTP_QUAD) |		\
		    QSPIC_RDTRSTP_DUMMY(TRSTP_QUAD) |		\
		    QSPIC_RDTRSTP_DIN(TRSTP_QUAD) |		\
		    QSPIC_RDTRSTP_ADDMODE(addr)	|		\
		    QSPIC_RDTRSTP_DUMMY_SIZE(dummy) |		\
		    QSPIC_RDTRSTP_HAS_OPCODE,			\
	.read_cmd = (read),					\
	.write_cfg = QSPIC_PPTRSTP_OPCODE(TRSTP_QUAD) |		\
		     QSPIC_PPTRSTP_ADD(TRSTP_QUAD) |		\
		     QSPIC_PPTRSTP_DOUT(TRSTP_QUAD) |		\
		     QSPIC_PPTRSTP_ADDMODE(addr) |		\
		     QSPIC_PPTRSTP_HAS_OPCODE,			\
	.write_cmd = (write),					\
	.aux_cfg = QSPIC_TRSTP_OPCODE(TRSTP_QUAD) |		\
		   QSPIC_TRSTP_ADD(TRSTP_QUAD) |		\
		   QSPIC_TRSTP_DOUT(TRSTP_QUAD) |		\
		   QSPIC_TRSTP_DIN(TRSTP_QUAD) |		\
		   QSPIC_TRSTP_ADDMODE(addr)

#define TO4KSEC(mbit)	((mbit) * (1024*1024/8) / 4096)
#define TO64KSEC(mbit)	((mbit) * (1024*1024/8) / (4096*16))

static struct flash_info known_flashes[] = {
	{
		FLASH_INFO("at25dq321a", 0x1f8800, 0, 70, 85, TO64KSEC(32)),
		FLASH_QSPI(0x6b, 0x32, TRSTP_ADD_3B, 1),
		.init = dspg_qspic_init_atmel,
		.wait = dspg_qspic_wait_atmel,
	},
	{
		FLASH_INFO("w25q16fv", 0xef4015, 0, 80, 80, TO64KSEC(16)),
		FLASH_QSPI(0x6b, 0x32, TRSTP_ADD_3B, 1),
		.init = dspg_qspic_init_winbond16_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("w25q64fv", 0xef4017, 0, 80, 80, TO64KSEC(64)),
		FLASH_QSPI(0x6b, 0x32, TRSTP_ADD_3B, 1),
		.init = dspg_qspic_init_winbond16_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("w25q64fv", 0xef6017, 0, 80, 80, TO64KSEC(64)),
		FLASH_QSPI(0x6b, 0x32, TRSTP_ADD_3B, 1),
		.init = dspg_qspic_init_winbond16_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("w25q128fv", 0xef4018, 0, 80, 80, TO64KSEC(128)),
		FLASH_QPI(0x0b, 0x02, TRSTP_ADD_3B, 3),
		.init = dspg_qspic_init_winbond_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("w25q128fv", 0xef6018, 0, 80, 80, TO64KSEC(128)),
		FLASH_QPI(0x0b, 0x02, TRSTP_ADD_3B, 3),
		.init = dspg_qspic_init_winbond_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("w25q256fv", 0xef4019, 0, 80, 80, TO64KSEC(256)),
		FLASH_QPI(0xeb, 0x02, TRSTP_ADD_4B, 3),
		.init = dspg_qspic_init_winbond256_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("w25q256fv", 0xef6019, 0, 80, 80, TO64KSEC(256)),
		FLASH_QPI(0xeb, 0x02, TRSTP_ADD_4B, 3),
		.init = dspg_qspic_init_winbond256_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("sst26vf016", 0xbf2601, 0, 80, 80, TO64KSEC(16)),
		FLASH_QPI(0x0b, 0x02, TRSTP_ADD_3B, 1),
		.init = dspg_qspic_init_sst16_qpi,
		.wait = dspg_qspic_wait_sst,
	},
	{
		FLASH_INFO("sst26vf032", 0xbf2602, 0, 80, 80, TO64KSEC(32)),
		FLASH_QPI(0x0b, 0x02, TRSTP_ADD_3B, 1),
		.init = dspg_qspic_init_sst32_qpi,
		.wait = dspg_qspic_wait_sst,
	},
	{
		FLASH_INFO("mx66l51235f", 0xc2201a, 0, 80, 80, TO64KSEC(512)),
		FLASH_QPI(0xeb, 0x02, TRSTP_ADD_4B, 3),
		.init = dspg_qspic_init_mx66l_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("n25q128a", 0x20ba18, 0, 80, 80, TO64KSEC(128)),
		FLASH_QPI(0x0b, 0x02, TRSTP_ADD_3B, 3),
		.init = dspg_qspic_init_n25q_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("n25q256a", 0x20ba19, 0, 80, 80, TO64KSEC(256)),
		FLASH_QPI(0x0b, 0x02, TRSTP_ADD_4B, 3),
		.init = dspg_qspic_init_n25q_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("mx25r6435f", 0xc22817, 0, 60, 60, TO64KSEC(64)),
		FLASH_4IO(0xeb, 0x38, TRSTP_ADD_3B, 3),
		.init = dspg_qspic_init_mx25_qpi,
		.wait = dspg_qspic_wait_generic,
	},
	{
		FLASH_INFO("gd25q256c", 0xc84019, 0, 80, 80, TO64KSEC(256)),
		FLASH_QSPI(0x6b, 0x32, TRSTP_ADD_4B, 1),
		.init = dspg_qspic_init_gd25q_qspi,
		.wait = dspg_qspic_wait_generic,
	},
};

static struct flash_info *
dspg_qspic_identify(struct chipsel *flash)
{
	u8 buffer[5];
	u32 jedec_id;
	u16 ext_id;
	int i;
	struct flash_info *info;

	dspg_qspic_aux_read(flash, CMD_RDID, &buffer, sizeof(buffer));

	jedec_id = ((u32)buffer[0] << 16) |
		   ((u32)buffer[1] <<  8) |
		    (u32)buffer[2];
	ext_id = ((u16)buffer[3] << 8) | (u16)buffer[4];

	dev_dbg(&flash->cntlr->pdev->dev,
		"CS%d: jedecid: 0x%06x extid: 0x%04x\n", flash->cs, jedec_id,
		ext_id);

	/* search for matching flash in table */
	for (i = ARRAY_SIZE(known_flashes), info = known_flashes; i > 0;
	     i--, info++) {
		if (info->jedec_id == jedec_id) {
			if (info->ext_id != 0 && info->ext_id != ext_id)
				continue;
			return info;
		}
	}
	return NULL;
}

static int
dspg_qspic_probe_cs(struct qspic *cntlr, int cs)
{
	struct chipsel *flash;
	struct flash_info *info;
	int ret;
	void *regs;
	struct resource *res;
	unsigned long size;
	unsigned char name[6];

	res = platform_get_resource(cntlr->pdev, IORESOURCE_MEM, 1 + cs);
	if (!res) {
		dev_err(&cntlr->pdev->dev, "missing CS%d io range\n", cs);
		return -EINVAL;
	}

	flash = devm_kzalloc(&cntlr->pdev->dev, sizeof(*flash), GFP_KERNEL);
	if (!flash)
		return -ENOMEM;

	/* Start up in standard SPI mode */
	flash->cntlr = cntlr;
	flash->cs = cs;
	flash->aux_cfg = QSPIC_TRSTP_OPCODE(TRSTP_SERIAL) |
			   QSPIC_TRSTP_ADD(TRSTP_SERIAL) |
			   QSPIC_TRSTP_DOUT(TRSTP_SERIAL) |
			   QSPIC_TRSTP_DIN(TRSTP_SERIAL) |
			   QSPIC_TRSTP_ADDMODE(TRSTP_ADD_3B);

	info = dspg_qspic_identify(flash);
	if (!info) {
		/* Try QPI with 4B mode (Winbond flashes) */
		dspg_qspic_reset_generic(flash);

		udelay(1000);

		/* Exit 4-byte mode */
		dspg_qspic_aux_cmd(flash, QSPIC_CMD_OPCODE(0xe9) |
				   QSPIC_CMD_TYPE_OTHER);

		flash->aux_cfg = QSPIC_TRSTP_OPCODE(TRSTP_QUAD) |
				 QSPIC_TRSTP_ADD(TRSTP_QUAD) |
				 QSPIC_TRSTP_DOUT(TRSTP_QUAD) |
				 QSPIC_TRSTP_DIN(TRSTP_QUAD) |
				 QSPIC_TRSTP_ADDMODE(TRSTP_ADD_3B);

		info = dspg_qspic_identify(flash);
	}
	if (!info)
		return 0; /* no error */

	size = info->sector_size * info->n_sectors;
	if (size > resource_size(res)) {
		dev_err(&cntlr->pdev->dev, "CS%d flash too big\n", cs);
		return -E2BIG;
	}

	flash->wait = info->wait;
	flash->sector_size = info->sector_size;
	flash->n_sectors = info->n_sectors;
	flash->page_size = info->page_size;
	flash->read_freq = info->read_freq;
	flash->write_freq = info->write_freq;

	sprintf(name, "qspi%d", cs);
	flash->mtd.name = name;
	flash->mtd.type = MTD_NORFLASH;
	flash->mtd.writesize = 4;
	flash->mtd.flags = MTD_CAP_NORFLASH;
	flash->mtd.size = size;
	flash->mtd._read = dspg_qspic_read;
	flash->mtd._write = dspg_qspic_write;
	flash->mtd._erase = dspg_qspic_erase;
	flash->mtd.erasesize = info->sector_size;
	flash->mtd.dev.parent = &cntlr->pdev->dev;
	flash->mtd.writebufsize = info->page_size;

	regs = cntlr->regs + (cs ? 0x200 : 0x100);
	writel(info->read_cfg, regs + QSPIC_RDTRSTP);
	writel(info->read_cmd, regs + QSPIC_RDCMD);
	writel(info->write_cfg, regs + QSPIC_PPTRSTP);
	writel(info->write_cmd, regs + QSPIC_PPCMD);
	writel(0xFFFF0000, regs + QSPIC_MEMADD); /* we verify ourselves */

	/* map chip select buffer */
	flash->shadow_res = devm_request_mem_region(&cntlr->pdev->dev,
						    res->start, size, cs ?
						    "dspg-qspic-cs1" :
						    "dspg-qspic-cs0");
	if (!flash->shadow_res)
		return -EBUSY;

	flash->shadow_buf = devm_ioremap(&cntlr->pdev->dev, res->start, size);
	if (!flash->shadow_buf) {
		dev_err(&cntlr->pdev->dev, "cannot map CS%d shadow buffer\n",
			cs);
		return -ENOMEM;
	}

	if (info->init) {
		ret = info->init(flash, info);
		if (ret)
			return ret;
	}

	/*
	 * Now send auxiliary commands with flash specific settings. Some init
	 * callbacks may have already done this.
	 */
	flash->aux_cfg = info->aux_cfg;

	ret = mtd_device_parse_register(&flash->mtd, NULL, NULL, NULL, 0);
	if (ret)
		return ret;

	if (flash->write_freq > flash->read_freq)
		ret = dspg_qspic_set_freq(flash->cntlr, flash->read_freq);
	else
		ret = dspg_qspic_set_freq(flash->cntlr, flash->write_freq);
	if (ret)
		return ret;

	dev_info(&cntlr->pdev->dev, "cs%d:%s\n", cs, info->name);

	cntlr->cs[cs] = flash;

	return 0;
}

static int
dspg_qspic_remove_cs(struct chipsel *flash)
{
	int ret;

	ret = mtd_device_unregister(&flash->mtd);
	if (ret)
		return ret;

	return 0;
}

static int
dspg_qspic_probe(struct platform_device *pdev)
{
	struct qspic *cntlr;
	int irq, ret;
	struct resource *mem_res;

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_res) {
		dev_err(&pdev->dev, "missing register io range\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no IRQ\n");
		return irq;
	}

	if (!devm_request_mem_region(&pdev->dev, mem_res->start,
				     resource_size(mem_res), pdev->name))
		return -EBUSY;

	cntlr = devm_kzalloc(&pdev->dev, sizeof(*cntlr), GFP_KERNEL);
	if (!cntlr)
		return -ENOMEM;

	ret = of_property_read_u32(pdev->dev.of_node, "max_speed",
				   &cntlr->max_speed);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'max_speed'\n");
		return ret;
	}

	cntlr->pdev = pdev;
	cntlr->irq = irq;
	mutex_init(&cntlr->lock);
	init_completion(&cntlr->done);
	platform_set_drvdata(pdev, cntlr);

	cntlr->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(cntlr->clk))
		return PTR_ERR(cntlr->clk);

	ret = dspg_qspic_set_freq(cntlr, 33);
	if (ret)
		return ret;

	ret = clk_prepare_enable(cntlr->clk);
	if (ret)
		return ret;

	cntlr->regs = devm_ioremap(&pdev->dev, mem_res->start,
				   resource_size(mem_res));
	if (!cntlr->regs) {
		ret = -ENOMEM;
		goto out_clk_disable;
	}

	writel(0x300, cntlr->regs + QSPIC_CSTIMING); /* enlarged cs timing */
	/* long CS deselect time, spi mode 3, qspic enable */
	writel(0x10009, cntlr->regs + QSPIC_CFG);
	writel(QSPIC_INT_APBTRSFR_DONE, cntlr->regs + QSPIC_IER);

	ret = devm_request_irq(&pdev->dev, irq, dspg_qspic_irq_handler,
			       IRQF_SHARED, "dspg-qspic", cntlr);
	if (ret)
		goto out_clk_disable;

	/* keep mutex locked until we finished probing all chip selects */
	mutex_lock(&cntlr->lock);

	/* detect flashes */
	ret = dspg_qspic_probe_cs(cntlr, 0);
	if (ret)
		goto out_mutex_unlock;

	ret = dspg_qspic_probe_cs(cntlr, 1);
	if (ret) {
		if (cntlr->cs[0])
			dspg_qspic_remove_cs(cntlr->cs[0]);
		goto out_mutex_unlock;
	}

	mutex_unlock(&cntlr->lock);

	if (!cntlr->cs[0] && !cntlr->cs[1]) {
		ret = -ENODEV;
		dev_notice(&pdev->dev, "no devices found\n");
		goto out_clk_disable;
	}

	return 0;

out_mutex_unlock:
	mutex_unlock(&cntlr->lock);
out_clk_disable:
	clk_disable_unprepare(cntlr->clk);

	return ret;
}

static int
dspg_qspic_remove(struct platform_device *pdev)
{
	struct qspic *cntlr = platform_get_drvdata(pdev);
	int ret;

	if (cntlr->cs[1]) {
		ret = dspg_qspic_remove_cs(cntlr->cs[1]);
		if (ret)
			return ret;
	}

	if (cntlr->cs[0]) {
		ret = dspg_qspic_remove_cs(cntlr->cs[0]);
		if (ret)
			return ret;
	}

	clk_disable_unprepare(cntlr->clk);

	return 0;
}

static const struct of_device_id dspg_qspic_dt_match[] = {
	{ .compatible = "dspg,qspic", },
	{}
};

static struct platform_driver dspg_qspic_driver = {
	.probe  = dspg_qspic_probe,
	.remove = dspg_qspic_remove,
	.driver = {
		.name = "dspg-qspic",
		.of_match_table = dspg_qspic_dt_match,
		.owner = THIS_MODULE,
	},
};
module_platform_driver(dspg_qspic_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DSPG QSPI controller driver");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_ALIAS("platform:dspg-qspic");
