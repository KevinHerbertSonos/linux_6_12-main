/*
 * drivers/mtd/nand/dspg_nfc.c
 *
 * Driver for the NAND flash controller on DSPG chips.
 * Supports DMA and PIO transfers and different hardware ECC modes.
 *
 * Copyright (C) 2018 DSPG Technologies GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <linux/ccu.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/nand-ecc-sw-hamming.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/reset.h>

#include <linux/dspg_nfc.h>

/*
 *  NAND FLASH/LCD register offsets
 */
#define DW_FC_CTL		0x0000	/* FC control */
#define DW_FC_STATUS		0x0004	/* FC status */
#define DW_FC_STS_CLR		0x0008	/* FC status clear */
#define DW_FC_INT_EN		0x000C	/* FC interrupt enable mask */
#define DW_FC_INT_CAUSE		0x0010	/* FC interrupt enable mask */
#define DW_FC_SEQUENCE		0x0014	/* FC sequence */
#define DW_FC_ADDR_COL		0x0018	/* FC address-column */
#define DW_FC_ADDR_ROW		0x001C	/* FC address-row */
#define DW_FC_CMD1		0x0020	/* FC command code configuration */
#define DW_FC_CMD2		0x0024	/* FC command code configuration */
#define DW_FC_WAIT1		0x0028	/* FC wait time configuration */
#define DW_FC_WAIT2		0x002C	/* FC wait time configuration */
#define DW_FC_WAIT3		0x0030	/* FC wait time configuration */
#define DW_FC_WAIT4		0x0034	/* FC wait time configuration */
#define DW_FC_PULSETIME		0x0038	/* FC pulse time configuration */
#define DW_FC_DCOUNT		0x003C	/* FC data count */
#define DW_FC_FTSR		0x0040	/* FC transfer size */
#define DW_FC_DPR		0x0044	/* FC data pointer */
#define DW_FC_RDPR		0x0048	/* FC redundancy data pointer */
#define DW_FC_TIMEOUT		0x004C	/* FC timeout configuration */
#define DW_FC_BCH		0x0050	/* FC ECC BCH code out */
#define DW_FC_HAMMING		0x0054	/* FC ECC Hamming code out */
#define DW_FC_START		0x0058	/* FC BCH Errors locatoins out */
#define DW_FC_FBYP_CTL		0x005C	/* FC GF FIFO bypass control */
#define DW_FC_FBYP_DATA		0x0060	/* FC GF FIFO bypass data */
#define DW_FC_LCD_HADD		0x0064	/* FC LCDC high address */

#define DW_FC_CHIEN_SEARCH_1	0x0068	/* FC Chien search coefficient 1 */
#define DW_FC_CHIEN_SEARCH_2	0x006C	/* FC Chien search coefficient 2 */
#define DW_FC_CHIEN_SEARCH_3	0x0070	/* FC Chien search coefficient 3 */
#define DW_FC_CHIEN_SEARCH_4	0x0074	/* FC Chien search coefficient 4 */
#define DW_FC_CHIEN_SEARCH_5	0x0078	/* FC Chien search coefficient 5 */
#define DW_FC_CHIEN_SEARCH_6	0x007C	/* FC Chien search coefficient 6 */
#define DW_FC_CHIEN_SEARCH_7	0x0080	/* FC Chien search coefficient 7 */
#define DW_FC_CHIEN_SEARCH_8	0x0084	/* FC Chien search coefficient 8 */
#define DW_FC_CHIEN_SEARCH_9	0x0088	/* FC Chien search coefficient 9 */
#define DW_FC_CHIEN_SEARCH_10	0x008C	/* FC Chien search coefficient 10 */
#define DW_FC_CHIEN_SEARCH_11	0x0090	/* FC Chien search coefficient 11 */
#define DW_FC_CHIEN_SEARCH_12	0x0094	/* FC Chien search coefficient 12 */
#define DW_FC_CHIEN_SEARCH_13	0x0098	/* FC Chien search coefficient 13 */
#define DW_FC_CHIEN_SEARCH_14	0x009C	/* FC Chien search coefficient 14 */
#define DW_FC_CHIEN_SEARCH_15	0x00A0	/* FC Chien search coefficient 15 */
#define DW_FC_CHIEN_SEARCH_16	0x00A4	/* FC Chien search coefficient 16 */
#define DW_FC_CHIEN_SEARCH_17	0x00A8	/* FC Chien search coefficient 17 */
#define DW_FC_CHIEN_SEARCH_18	0x00AC	/* FC Chien search coefficient 18 */
#define DW_FC_CHIEN_SEARCH_19	0x00B0	/* FC Chien search coefficient 19 */
#define DW_FC_CHIEN_SEARCH_20	0x00B4	/* FC Chien search coefficient 20 */
#define DW_FC_CHIEN_SEARCH_21	0x00B8	/* FC Chien search coefficient 21 */
#define DW_FC_CHIEN_SEARCH_22	0x00BC	/* FC Chien search coefficient 22 */
#define DW_FC_CHIEN_SEARCH_23	0x00C0	/* FC Chien search coefficient 23 */
#define DW_FC_CHIEN_SEARCH_24	0x00C4	/* FC Chien search coefficient 24 */

#define DW_FC_AHB_CTL		0x00C8	/* FC AHB DMA control */
#define DW_FC_AHB_DBG_1		0x00CC	/* FC AHB Debug 1 */
#define DW_FC_AHB_DBG_2		0x00D0	/* FC AHB Debug 2 */
#define DW_FC_DBG_1		0x00D4	/* FC NFC Debug 1 */
#define DW_FC_DBG_2		0x00D8	/* FC NFC Debug 2 */
#define DW_FC_ECC_DBG_1		0x00DC	/* FC NFC ECC Debug 1 */
#define DW_FC_ECC_DBG_2		0x00E0	/* FC NFC ECC Debug 2 */
#define DW_FC_ECC_DBG_3		0x00E4	/* FC NFC ECC Debug 3 */
#define DW_FC_ECC_DBG_4		0x00E8	/* FC NFC ECC Debug 4 */
#define DW_FC_ECC_DBG_5		0x00EC	/* FC NFC ECC Debug 5 */

#define DW_FC_CTL_ECC_OP_MODE(a)        (((a) & 0xF) << 0)
#define DW_FC_CTL_LCD_IM(a)             (((a) & 0x1) << 9)
#define DW_FC_CTL_LCD_CLE_INV(a)        (((a) & 0x1) << 10)
#define DW_FC_CTL_CHIEN_CNT_START(a)    (((a) & 0x1FFF) << 16)

#define DW_FC_MODULE_DISABLED       (0x0)
#define DW_FC_1_BIT_ECC             (0x1)
#define DW_FC_4_BIT_ECC             (0x2)
#define DW_FC_8_BIT_ECC             (0x3)
#define DW_FC_12_BIT_ECC            (0x4)

#define DW_FC_STATUS_TRANS_DONE     (1 << 0)
#define DW_FC_STATUS_ECC_DONE       (1 << 1)
#define DW_FC_STATUS_ERR_FOUND      (1 << 2)
#define DW_FC_STATUS_RDY_TIMEOUT    (1 << 3)
#define DW_FC_STATUS_RDY1_RISE      (1 << 4)
#define DW_FC_STATUS_RDY2_RISE      (1 << 5)
#define DW_FC_STATUS_TRANS_BUSY     (1 << 6)
#define DW_FC_STATUS_ECC_BUSY       (1 << 7)
#define DW_FC_STATUS_RDY1_STAT      (1 << 8)
#define DW_FC_STATUS_RDY2_STAT      (1 << 9)
#define DW_FC_STATUS_ECC_ERR_FF     (1 << 10)
#define DW_FC_STATUS_ECC_NC_ERR     (1 << 11)
#define DW_FC_STATUS_ECC_NC_SEQ     (1 << 12) /* 4 bits */
#define DW_FC_STATUS_AHB_ERR        (1 << 16)
#define DW_FC_STATUS_AHB_DONE       (1 << 17)

#define DW_FC_SEQ_CMD1_EN           (1 << 0)
#define DW_FC_SEQ_WAIT0_EN          (1 << 1)
#define DW_FC_SEQ_ADD1_EN           (1 << 2)
#define DW_FC_SEQ_ADD2_EN           (1 << 3)
#define DW_FC_SEQ_ADD3_EN           (1 << 4)
#define DW_FC_SEQ_ADD4_EN           (1 << 5)
#define DW_FC_SEQ_ADD5_EN           (1 << 6)
#define DW_FC_SEQ_WAIT1_EN          (1 << 7)
#define DW_FC_SEQ_CMD2_EN           (1 << 8)
#define DW_FC_SEQ_WAIT2_EN          (1 << 9)
#define DW_FC_SEQ_RW_EN             (1 << 10)
#define DW_FC_SEQ_RW_DIR            (1 << 11)
#define DW_FC_SEQ_DATA_READ_DIR     (0 << 11)
#define DW_FC_SEQ_DATA_WRITE_DIR    (1 << 11)
#define DW_FC_SEQ_DATA_ECC(a)       (((a) & 0x1) << 12)
#define DW_FC_SEQ_CMD3_EN           (1 << 13)
#define DW_FC_SEQ_WAIT3_EN          (1 << 14)
#define DW_FC_SEQ_CMD4_EN           (1 << 15)
#define DW_FC_SEQ_WAIT4_EN          (1 << 16)
#define DW_FC_SEQ_READ_ONCE         (1 << 17)
#define DW_FC_SEQ_CHIP_SEL(a)       (((a) & 0x3) << 18)
#define DW_FC_SEQ_KEEP_CS           (1 << 20)
#define DW_FC_SEQ_MODE8             (0 << 21)
#define DW_FC_SEQ_MODE16            (1 << 21)
#define DW_FC_SEQ_RDY_EN            (1 << 22)
#define DW_FC_SEQ_RDY_SEL(a)        (((a) & 0x1) << 23)
#define DW_FC_SEQ_CMD5_EN           (1 << 24)
#define DW_FC_SEQ_WAIT5_EN          (1 << 25)
#define DW_FC_SEQ_CMD6_EN           (1 << 26)
#define DW_FC_SEQ_WAIT6_EN          (1 << 27)

#define DW_FC_ADD1(a)               (((a) & 0xFF) << 0)
#define DW_FC_ADD2(a)               (((a) & 0xFF) << 8)

#define DW_FC_ADD3(a)               (((a) & 0xFF) << 0)
#define DW_FC_ADD4(a)               (((a) & 0xFF) << 8)
#define DW_FC_ADD5(a)               (((a) & 0xFF) << 16)

#define DW_FC_CMD_1(a)              (((a) & 0xFF) << 0)
#define DW_FC_CMD_2(a)              (((a) & 0xFF) << 8)
#define DW_FC_CMD_3(a)              (((a) & 0xFF) << 16)
#define DW_FC_CMD_4(a)              (((a) & 0xFF) << 24)

#define DW_FC_CMD_5(a)              (((a) & 0xFF) << 0)
#define DW_FC_CMD_6(a)              (((a) & 0xFF) << 8)

#define DW_FC_WAIT0A(a)             (((a) & 0x3F) << 0)
#define DW_FC_WAIT0B(a)             (((a) & 0x3F) << 8)
#define DW_FC_WAIT1A(a)             (((a) & 0x3F) << 0)
#define DW_FC_WAIT1B(a)             (((a) & 0x3F) << 8)
#define DW_FC_WAIT2A(a)             (((a) & 0x3F) << 16)
#define DW_FC_WAIT2B(a)             (((a) & 0x3F) << 24)
#define DW_FC_WAIT3A(a)             (((a) & 0x3F) << 0)
#define DW_FC_WAIT3B(a)             (((a) & 0x3F) << 8)
#define DW_FC_WAIT4A(a)             (((a) & 0x3F) << 16)
#define DW_FC_WAIT4B(a)             (((a) & 0x3F) << 24)
#define DW_FC_WAIT5A(a)             (((a) & 0x3F) << 0)
#define DW_FC_WAIT5B(a)             (((a) & 0x3F) << 8)
#define DW_FC_WAIT6A(a)             (((a) & 0x3F) << 16)
#define DW_FC_WAIT6B(a)             (((a) & 0x3F) << 24)

#define DW_FC_PT_READ_LOW(a)        (((a) & 0xF) << 0)
#define DW_FC_PT_READ_HIGH(a)       (((a) & 0xF) << 4)
#define DW_FC_PT_WRITE_LOW(a)       (((a) & 0xF) << 8)
#define DW_FC_PT_WRITE_HIGH(a)      (((a) & 0xF) << 12)
#define DW_FC_CLE_SETUP(a)          (((a) & 0xF) << 16)
#define DW_FC_ALE_SETUP(a)          (((a) & 0xF) << 20)
#define DW_FC_PT_SPARE_WORDS(a)     (((a) & 0x3F) << 24)

#define DW_FC_DCOUNT_VP_SIZE(a)     (((a) & 0x1FFF) << 0)
#define DW_FC_DCOUNT_SPARE_N3(a)    (((a) & 0x7) << 13)
#define DW_FC_DCOUNT_SPARE_N2(a)    (((a) & 0xFF) << 16)
#define DW_FC_DCOUNT_SPARE_N1(a)    (((a) & 0xF) << 24)
#define DW_FC_DCOUNT_PAGE_CNT(a)    (((a) & 0xF) << 28)

#define DW_FC_FTSR_TRANSFER_SIZE(a) (((a) & 0x1FFF) << 0)
#define DW_FC_FTSR_SEQ_NUM(a)       (((a) & 0xF) << 16)
#define DW_FC_FTSR_VP_NUM(a)        (((a) & 0xF) << 20)

#define DW_FC_TIMEOUT_RDY_CNT(a)    (((a) & 0x7F) << 0)
#define DW_FC_TIMEOUT_RDY_EN        (1 << 8)

#define DW_FC_BCH_H                 (0x7FFFF << 0)
#define DW_FC_BCH_DEC_BUF           (0x7 << 20)

#define DW_FC_HAMMING_CODE_OUT      (0x1FFF << 0)

#define DW_FC_FBYP_CTL_BUSY             (1 << 0)
#define DW_FC_FBYP_CTL_BP_EN            (1 << 1)
#define DW_FC_FBYP_CTL_BP_WRITE         (1 << 2)
#define DW_FC_FBYP_CTL_BP_READ_REG      (1 << 3)
#define DW_FC_FBYP_CTL_BP_READ_FLASH    (1 << 4)

#define DW_FC_FBYP_DATA_BYPASS_DATA       (0xFFFF << 0)

#define DSPG_MAX_PAGESIZE           8192
#define DSPG_MAX_OOBSIZE            256
#define VIRTUAL_PAGESIZE            512
#define VIRTUAL_PAGESIZE_SHIFT        9
#define DSPG_NFC_MAX_ERRORS_PER_VP  12
#define DSPG_NFC_MAX_ERRORS_PER_PAGE \
	((DSPG_MAX_PAGESIZE >> VIRTUAL_PAGESIZE_SHIFT) * \
	 DSPG_NFC_MAX_ERRORS_PER_VP)

/* Time values in CLK Cycles */
#ifdef CONFIG_MTD_NAND_DSPG_CONFIGURE_TIMING
#define WAIT_READY_0_FALL CONFIG_MTD_NAND_DSPG_WAIT_READY_0_FALL
#define WAIT_READY_0_RISE CONFIG_MTD_NAND_DSPG_WAIT_READY_0_RISE
#define WAIT_READY_1_FALL CONFIG_MTD_NAND_DSPG_WAIT_READY_1_FALL
#define WAIT_READY_1_RISE CONFIG_MTD_NAND_DSPG_WAIT_READY_1_RISE
#define WAIT_READY_2_FALL CONFIG_MTD_NAND_DSPG_WAIT_READY_2_FALL
#define WAIT_READY_2_RISE CONFIG_MTD_NAND_DSPG_WAIT_READY_2_RISE
#define WAIT_READY_3_FALL CONFIG_MTD_NAND_DSPG_WAIT_READY_3_FALL
#define WAIT_READY_3_RISE CONFIG_MTD_NAND_DSPG_WAIT_READY_3_RISE
#define WAIT_READY_4_FALL CONFIG_MTD_NAND_DSPG_WAIT_READY_4_FALL
#define WAIT_READY_4_RISE CONFIG_MTD_NAND_DSPG_WAIT_READY_4_RISE
#define WAIT_READY_5_FALL CONFIG_MTD_NAND_DSPG_WAIT_READY_5_FALL
#define WAIT_READY_5_RISE CONFIG_MTD_NAND_DSPG_WAIT_READY_5_RISE
#define WAIT_READY_6_FALL CONFIG_MTD_NAND_DSPG_WAIT_READY_6_FALL
#define WAIT_READY_6_RISE CONFIG_MTD_NAND_DSPG_WAIT_READY_6_RISE

#define PULSE_ALE_SETUP   CONFIG_MTD_NAND_DSPG_PULSE_ALE_SETUP
#define PULSE_CLE_SETUP   CONFIG_MTD_NAND_DSPG_PULSE_CLE_SETUP
#define PULSE_WR_HIGH     CONFIG_MTD_NAND_DSPG_PULSE_WR_HIGH
#define PULSE_WR_LOW      CONFIG_MTD_NAND_DSPG_PULSE_WR_LOW
#define PULSE_RD_HIGH     CONFIG_MTD_NAND_DSPG_PULSE_RD_HIGH
#define PULSE_RD_LOW      CONFIG_MTD_NAND_DSPG_PULSE_RD_LOW

#define READY_TIMEOUT     CONFIG_MTD_NAND_DSPG_READY_TIMEOUT
#else
/* WaitRdy values must be 2-15 if READYi line is used, 0 otherwise. */
#define WAIT_READY_0_FALL 4
#define WAIT_READY_0_RISE 4
#define WAIT_READY_1_FALL 4
#define WAIT_READY_1_RISE 4
#define WAIT_READY_2_FALL 4
#define WAIT_READY_2_RISE 4
#define WAIT_READY_3_FALL 4
#define WAIT_READY_3_RISE 4
#define WAIT_READY_4_FALL 4
#define WAIT_READY_4_RISE 4
#define WAIT_READY_5_FALL 4
#define WAIT_READY_5_RISE 4
#define WAIT_READY_6_FALL 4
#define WAIT_READY_6_RISE 4

/* Write & Read pulse shape: value = n --> (n+1) wait cycles generated */
#define PULSE_ALE_SETUP   4
#define PULSE_CLE_SETUP   4
#define PULSE_WR_HIGH     4
#define PULSE_WR_LOW      4
#define PULSE_RD_HIGH     4
#define PULSE_RD_LOW      4

/* Wait for Ready back high - overall transaction timeout */
#define READY_TIMEOUT     32 /* timeout > 10 mSec */
#endif

struct dspg_nfc_hw_config {
	uint16_t *chien_codes;
	unsigned num_chien_codes;
	int eccmode_bits[4];
};

struct dspg_nfc_info {
	struct nand_controller controller;
	struct clk *clk;
	struct reset_control *reset;
	struct nand_chip *chip;
	void __iomem *regs;
	struct dspg_nfc_hw_config *hw_config;
	int chipsel_nr;
	int ready_nr;
	int buswidth;
	int irq;

#define DSPG_NFC_FLAG_NEED_TRANS_DONE 0x00000001
#define DSPG_NFC_FLAG_NEED_AHB_DONE   0x00000002
#define DSPG_NFC_FLAG_NEED_ECC_DONE   0x00000004
	unsigned long flags;
	struct completion transfer_complete;
	atomic_t counter;
	unsigned long wait_counter;

	unsigned char n1len, n2len, n3len;
	int dirtypos;

	unsigned char *data_buff;
	dma_addr_t data_buff_phys;

	unsigned long offset;
	int ecc;
	int use_ecc;
	int ecc_mode;

	/* queue of ecc correction codes */
	uint32_t ecc_codes[DSPG_NFC_MAX_ERRORS_PER_PAGE];
	int ecc_numcodes;

	int wait_ready_0_fall;
	int wait_ready_0_rise;
	int wait_ready_1_fall;
	int wait_ready_1_rise;
	int wait_ready_2_fall;
	int wait_ready_2_rise;
	int wait_ready_3_fall;
	int wait_ready_3_rise;
	int wait_ready_4_fall;
	int wait_ready_4_rise;
	int wait_ready_5_fall;
	int wait_ready_5_rise;
	int wait_ready_6_fall;
	int wait_ready_6_rise;

	int pulse_ale_setup;
	int pulse_cle_setup;
	int pulse_wr_high;
	int pulse_wr_low;
	int pulse_rd_high;
	int pulse_rd_low;

	int ready_timeout;

	int max_bitflips;
};

static const char dspg_nfc_name[] = "dmw_nand";

static uint16_t dspg_nfc_chien_codes_rev0[] = {
	0x0B19,
	0x14E2,
	0x0D8D,
	0x01E8,
	0x0D7D,
	0x1B24,
	0x1351,
	0x14AE,
	0x1EA6,
	0x0E12,
	0x1512,
	0x09E9,
	0x14A1,
	0x11ED,
	0x1C13,
	0x0CE5,
	0x0D18,
	0x02C4,
	0x11B9,
	0x0FBC,
	0x0D62,
	0x1A03,
	0x0772,
	0x1346,
};

static uint16_t dspg_nfc_chien_codes_rev1[] = {
	0x0B19,
	0x14E2,
	0x0D8D,
	0x01E8,
	0x0D7D,
	0x1B24,
	0x1351,
	0x14AE,
	0x1EA6,
	0x0E12,
	0x1512,
	0x09E9,
	0x1365,
	0x0AE3,
	0x1E4A,
	0x11B8,
	0x14A1,
	0x11ED,
	0x1C13,
	0x0CE5,
	0x0D18,
	0x02C4,
	0x11B9,
	0x0FBC,
	0x0D62,
	0x1A03,
	0x0772,
	0x1346,
	0x1980,
	0x1DF5,
	0x1140,
	0x1D45,
};

static struct dspg_nfc_hw_config dspg_nfc_hw_config_revs[] = {
	{
		.chien_codes = dspg_nfc_chien_codes_rev0,
		.num_chien_codes = ARRAY_SIZE(dspg_nfc_chien_codes_rev0),
		.eccmode_bits = {1, 4, 8, 12},
	},
	{
		.chien_codes = dspg_nfc_chien_codes_rev1,
		.num_chien_codes = ARRAY_SIZE(dspg_nfc_chien_codes_rev1),
		.eccmode_bits = {1, 4, 8, 16},
	},
};

#define NAND_DATABUF_SIZE      (DSPG_MAX_PAGESIZE + DSPG_MAX_OOBSIZE)
#define NAND_CMD_READPARAMPAGE 0xEC

/* Restrictions:
 *
 * - FC_DCOUNT.VIRTUAL_PAGE_COUNT >= FC_FTSR.TRANSFER_SIZE
 * - FC_DCOUNT.SPARE_N3_COUNT < FC_DCOUNT.SPARE_N1_COUNT
 * - FC_DCOUNT.SPARE_N1_COUNT even only numbers in range of 0..12
 * - VP_SIZE * VIRTUAL_PAGE_COUNT <= 65535
 * - if NFC HW stalled because ECC_ERR_FF (Fifo full), SW should read the
 *   errors to allow HW to continue
 */

static unsigned
dmw96_nfc_eccbits(struct dspg_nfc_info *info, unsigned ecc_mode)
{
	BUG_ON(ecc_mode >= ARRAY_SIZE(info->hw_config->eccmode_bits));

	return info->hw_config->eccmode_bits[ecc_mode];
}

static unsigned
dmw96_nfc_eccsize(struct dspg_nfc_info *info, unsigned ecc_mode)
{
	if (ecc_mode == 0)
		return 3;

	return ((dmw96_nfc_eccbits(info, ecc_mode) * 13) + 7) / 8;
}

static int
dspg_nfc_ooblayout_ecc(struct mtd_info *mtd, int section,
		       struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd->priv;
	struct dspg_nfc_info *info = chip->priv;
	int vp_count = mtd->writesize >> VIRTUAL_PAGESIZE_SHIFT;
	int vp_oobsize = mtd->oobsize / vp_count;

	if (section >= vp_count)
		return -ERANGE;

	oobregion->offset = section * vp_oobsize + info->n1len;
	oobregion->length = dmw96_nfc_eccsize(info, info->ecc_mode);

	return 0;
}

static int
dspg_nfc_ooblayout_free(struct mtd_info *mtd, int section,
			struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd->priv;
	struct dspg_nfc_info *info = chip->priv;
	int vp_count = mtd->writesize >> VIRTUAL_PAGESIZE_SHIFT;
	int vp_oobsize = mtd->oobsize / vp_count;
	int eccsize = dmw96_nfc_eccsize(info, info->ecc_mode);

	if (section >= vp_count)
		return -ERANGE;

	oobregion->offset = section * vp_oobsize + info->n1len + eccsize;
	oobregion->length = vp_oobsize - info->n1len - eccsize;

	return 0;
}

static const struct mtd_ooblayout_ops dspg_nfc_ooblayout_ops = {
	.ecc	= dspg_nfc_ooblayout_ecc,
	.free	= dspg_nfc_ooblayout_free,
};

static int
dmw96_nfc_eccfind(struct dspg_nfc_info *info, unsigned bits)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(info->hw_config->eccmode_bits); i++) {
		if (info->hw_config->eccmode_bits[i] >= bits)
			return i;
	}

	return -EINVAL;
}

static unsigned long
dspg_nfc_readl(struct dspg_nfc_info *info, int reg)
{
	return readl(info->regs + reg);
}

static void
dspg_nfc_writel(int value, struct dspg_nfc_info *info, int reg)
{
	writel(value, info->regs + reg);
}

static void
dspg_nfc_clear_status(struct dspg_nfc_info *info)
{
	dspg_nfc_writel(0xffffffff, info, DW_FC_STS_CLR);
}

static void
dspg_nfc_start(struct dspg_nfc_info *info)
{
	uint32_t reg;

	clk_enable(info->clk);

	reg = dspg_nfc_readl(info, DW_FC_SEQUENCE);
	info->flags = DSPG_NFC_FLAG_NEED_TRANS_DONE;

	if (reg & DW_FC_SEQ_RW_EN)
		info->flags |= DSPG_NFC_FLAG_NEED_AHB_DONE;

	if (reg & DW_FC_SEQ_DATA_ECC(1))
		info->flags |= DSPG_NFC_FLAG_NEED_ECC_DONE;

	dspg_nfc_clear_status(info);
	info->wait_counter = atomic_read(&info->counter);
	dspg_nfc_writel(0x1, info, DW_FC_START);
}

static void
dspg_nfc_configure_ready_timeout(struct dspg_nfc_info *info)
{
#if 0
	dspg_nfc_writel(DW_FC_TIMEOUT_RDY_EN |
			DW_FC_TIMEOUT_RDY_CNT(info->ready_timeout),
			info, DW_FC_TIMEOUT);
#endif
}

static void dspg_nfc_ecc_fetch(struct dspg_nfc_info *info);

static void
dspg_nfc_wait_trans_done(struct dspg_nfc_info *info)
{
	wait_for_completion(&info->transfer_complete);
}

static int
dspg_nfc_check_timeout(struct dspg_nfc_info *info)
{
#if 0

	/* check if there was a timeout (chip blocked) */
	if (readl(info->regs + DW_FC_STATUS) & DW_FC_STATUS_RDY_TIMEOUT)
		return 1;

#endif

	return 0;
}

static void
dspg_nfc_set_fixed_regs(struct dspg_nfc_info *info)
{
	int i;

	dspg_nfc_writel(DW_FC_WAIT1A(info->wait_ready_1_fall) |
			DW_FC_WAIT1B(info->wait_ready_1_rise) |
			DW_FC_WAIT2A(info->wait_ready_2_fall) |
			DW_FC_WAIT2B(info->wait_ready_2_rise),
			info, DW_FC_WAIT1);

	dspg_nfc_writel(DW_FC_WAIT3A(info->wait_ready_3_fall) |
			DW_FC_WAIT3B(info->wait_ready_3_rise) |
			DW_FC_WAIT4A(info->wait_ready_4_fall) |
			DW_FC_WAIT4B(info->wait_ready_4_rise),
			info, DW_FC_WAIT2);

	dspg_nfc_writel(DW_FC_WAIT5A(info->wait_ready_5_fall) |
			DW_FC_WAIT5B(info->wait_ready_5_rise) |
			DW_FC_WAIT6A(info->wait_ready_6_fall) |
			DW_FC_WAIT6B(info->wait_ready_6_rise),
			info, DW_FC_WAIT3);

	dspg_nfc_writel(DW_FC_WAIT0A(info->wait_ready_0_fall) |
			DW_FC_WAIT0B(info->wait_ready_0_rise),
			info, DW_FC_WAIT4);

	dspg_nfc_writel(DW_FC_PT_READ_LOW(info->pulse_rd_low) |
			DW_FC_PT_READ_HIGH(info->pulse_rd_high)  |
			DW_FC_PT_WRITE_LOW(info->pulse_wr_low)   |
			DW_FC_PT_WRITE_HIGH(info->pulse_wr_high) |
			DW_FC_CLE_SETUP(info->pulse_cle_setup)   |
			DW_FC_ALE_SETUP(info->pulse_ale_setup),
			info, DW_FC_PULSETIME);

	for (i = 0; i < info->hw_config->num_chien_codes; i++) {
		dspg_nfc_writel(info->hw_config->chien_codes[i], info,
				DW_FC_CHIEN_SEARCH_1 + i*4);
	}
}

static void
dspg_nfc_sequence(struct dspg_nfc_info *info, int column, int page_addr,
				  int cmd)
{
	struct mtd_info *mtd = nand_to_mtd(info->chip);
	struct nand_chip *chip = mtd->priv;
	int sequence = 0;

	int cycle1 = 0;
	int cycle2 = 0;
	int cycle3 = 0;
	int cycle4 = 0;
	int cycle5 = 0;

	int cmd1 = 0;
	int cmd2 = 0;
	int cmd3 = 0;
	int cmd4 = 0;
	int cmd5 = 0;
	int cmd6 = 0;

	if (column != -1) {
		/* emulate NAND_CMD_READOOB for large page devices */
		if ((mtd->writesize > 512) && (cmd == NAND_CMD_READOOB))
			column += mtd->writesize;

		/* on 16bit bus we do not have A0 -> shift */
		if (chip->options & NAND_BUSWIDTH_16)
			column >>= 1;

		/*
		 * on small page flashes only cycle1 is used (A0 - A7). the
		 * remaining address line is controlled by either issueing
		 * NAND_CMD_READ0 or NAND_CMD_READ1 for reading the first or
		 * the second half of the page respectively
		 */
		cycle1 = column & 0xff;
		cycle2 = (column >> 8) & 0xff;
	}

	sequence = DW_FC_SEQ_CMD1_EN | DW_FC_SEQ_WAIT0_EN;
	sequence |= DW_FC_SEQ_CHIP_SEL(info->chipsel_nr);
	sequence |= DW_FC_SEQ_RDY_EN | DW_FC_SEQ_RDY_SEL(info->ready_nr);

	if ((cmd == NAND_CMD_READ0) || (cmd == NAND_CMD_READOOB) ||
		(cmd == NAND_CMD_SEQIN)) {
		sequence |= DW_FC_SEQ_ADD1_EN | DW_FC_SEQ_RW_EN;

		/* also enable add2 for large page devices */
		if (likely(mtd->writesize > 512))
			sequence |= DW_FC_SEQ_ADD2_EN;
	}

	if (chip->options & NAND_BUSWIDTH_16)
		sequence |= DW_FC_SEQ_MODE16;

	if ((cmd == NAND_CMD_READ0) || (cmd == NAND_CMD_READOOB)) {
		if (mtd->writesize > 512) {
			cmd1 = NAND_CMD_READ0;
			cmd2 = NAND_CMD_READSTART;
			cmd5 = NAND_CMD_RNDOUT;
			cmd6 = NAND_CMD_RNDOUTSTART;

			sequence &= ~DW_FC_SEQ_WAIT0_EN;

			sequence |= DW_FC_SEQ_CMD2_EN | DW_FC_SEQ_WAIT2_EN;
			sequence |= DW_FC_SEQ_CMD5_EN | DW_FC_SEQ_WAIT5_EN;
			sequence |= DW_FC_SEQ_CMD6_EN | DW_FC_SEQ_WAIT6_EN;
		} else {
			cmd1 = cmd; /* NAND_CMD_READ0 or NAND_CMD_READOOB */
		}

		sequence |= DW_FC_SEQ_DATA_READ_DIR | DW_FC_SEQ_WAIT1_EN;
	} else if (cmd == NAND_CMD_SEQIN) {
		if (mtd->writesize > 512) {
			cmd1 = NAND_CMD_SEQIN;
			cmd3 = NAND_CMD_PAGEPROG; /* after data write */
			cmd4 = NAND_CMD_STATUS;   /* after data write */
			cmd5 = NAND_CMD_RNDIN;

			sequence |= DW_FC_SEQ_CMD3_EN | DW_FC_SEQ_WAIT3_EN;
			sequence |= DW_FC_SEQ_CMD4_EN | DW_FC_SEQ_WAIT4_EN;
			sequence |= DW_FC_SEQ_CMD5_EN | DW_FC_SEQ_WAIT5_EN;

			sequence |= DW_FC_SEQ_DATA_WRITE_DIR;

			sequence &= ~DW_FC_SEQ_WAIT0_EN;
			sequence |= DW_FC_SEQ_WAIT6_EN;
			sequence |= DW_FC_SEQ_READ_ONCE;
		} else {
			cmd1 = NAND_CMD_SEQIN;
			cmd3 = NAND_CMD_PAGEPROG; /* after data write */

			sequence |= DW_FC_SEQ_DATA_WRITE_DIR |
				    DW_FC_SEQ_CMD3_EN | DW_FC_SEQ_WAIT3_EN;
		}
	} else if (cmd == NAND_CMD_ERASE1) {
		cmd1 = NAND_CMD_ERASE1;
		cmd2 = NAND_CMD_ERASE2;
		cmd4 = NAND_CMD_STATUS;

		sequence |= DW_FC_SEQ_CMD2_EN | DW_FC_SEQ_WAIT2_EN |
			    DW_FC_SEQ_CMD4_EN | DW_FC_SEQ_WAIT4_EN;
		sequence |= DW_FC_SEQ_DATA_READ_DIR;
		sequence |= DW_FC_SEQ_READ_ONCE;
	}

	cycle3 = page_addr & 0xff;
	cycle4 = (page_addr >> 8) & 0xff;
	sequence |= DW_FC_SEQ_ADD3_EN | DW_FC_SEQ_ADD4_EN;

	if ((nanddev_target_size(&chip->base) >> chip->page_shift) > (256*256))  {
		cycle5 = (page_addr >> 16) & 0xff;
		sequence |= DW_FC_SEQ_ADD5_EN;
	}

	if (info->use_ecc && info->ecc_mode > 0)
		sequence |= DW_FC_SEQ_DATA_ECC(1);

	if (info->use_ecc && (info->ecc_mode > 0)) {
		dspg_nfc_writel(DW_FC_CTL_ECC_OP_MODE(info->ecc_mode) |
				DW_FC_CTL_CHIEN_CNT_START(0x0e6c), /* predef */
				info, DW_FC_CTL);
	}

	dspg_nfc_writel(sequence, info, DW_FC_SEQUENCE);

	/* configure address registers */
	dspg_nfc_writel(DW_FC_ADD1(cycle1) | DW_FC_ADD2(cycle2),
					info, DW_FC_ADDR_COL);

	dspg_nfc_writel(DW_FC_ADD3(cycle3) | DW_FC_ADD4(cycle4) |
					DW_FC_ADD5(cycle5),
					info, DW_FC_ADDR_ROW);

	/* configure command register */
	dspg_nfc_writel(DW_FC_CMD_1(cmd1) | DW_FC_CMD_2(cmd2) |
					DW_FC_CMD_3(cmd3) | DW_FC_CMD_4(cmd4),
					info, DW_FC_CMD1);

	dspg_nfc_writel(DW_FC_CMD_5(cmd5) | DW_FC_CMD_6(cmd6),
					info, DW_FC_CMD2);

	dspg_nfc_writel(info->data_buff_phys, info, DW_FC_DPR);
	dspg_nfc_writel(info->data_buff_phys + mtd->writesize,
					info, DW_FC_RDPR);

	dspg_nfc_configure_ready_timeout(info);
}

static void
dspg_nfc_set_datacount(struct dspg_nfc_info *info, int page_size,
					   int spare_size)
{
	struct mtd_info *mtd = nand_to_mtd(info->chip);
	int vp_size, n1, n2, n3, page_count;
	int transfer_size;
	int spare_words = ((info->n1len + info->n2len + 3) & ~3L) >> 2;
	uint32_t reg;

	n1 = n2 = n3 = page_count = 0;

	if (likely(info->use_ecc && (info->ecc_mode > 0))) {
		vp_size = VIRTUAL_PAGESIZE;
		n1 = info->n1len;
		n2 = info->n2len;
		n3 = info->n3len;
		transfer_size = (page_size >> VIRTUAL_PAGESIZE_SHIFT);
		page_count = transfer_size - 1;
	} else {
		if (likely(mtd->writesize > 512)) {
			int spare_offset = page_size;

			if (info->chip->options & NAND_BUSWIDTH_16)
				spare_offset >>= 1;

			vp_size = spare_offset;
			n2 = spare_size;
			transfer_size = page_size;
		} else {
			vp_size = page_size;
			transfer_size = page_size + spare_size;
		}
	}

	dspg_nfc_writel(DW_FC_DCOUNT_VP_SIZE(vp_size) |
					DW_FC_DCOUNT_SPARE_N3(n3) |
					DW_FC_DCOUNT_SPARE_N2(n2) |
					DW_FC_DCOUNT_SPARE_N1(n1) |
					DW_FC_DCOUNT_PAGE_CNT(page_count),
					info, DW_FC_DCOUNT);

	dspg_nfc_writel(DW_FC_FTSR_TRANSFER_SIZE(transfer_size),
					info, DW_FC_FTSR);

	reg = dspg_nfc_readl(info, DW_FC_PULSETIME);
	reg &= ~DW_FC_PT_SPARE_WORDS(0x3f);
	reg |= DW_FC_PT_SPARE_WORDS(spare_words);
	dspg_nfc_writel(reg, info, DW_FC_PULSETIME);
}

static void
dspg_nfc_select_chip(struct nand_chip *chip, int chipnr)
{
}

static void
dspg_nfc_read_buf(struct nand_chip *chip, uint8_t *buf, int len)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct dspg_nfc_info *info = chip->priv;

	if (info->offset + len <= mtd->writesize + mtd->oobsize) {
		memcpy(buf, info->data_buff + info->offset, len);
		info->offset += len;
	}
}

static void
dspg_nfc_write_buf(struct nand_chip *chip, const uint8_t *buf, int len)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct dspg_nfc_info *info = chip->priv;

	if (info->offset + len <= mtd->writesize + mtd->oobsize) {
		memcpy(info->data_buff + info->offset, buf, len);
		info->offset += len;
	}
}

static uint8_t
dspg_nfc_read_byte(struct nand_chip *chip)
{
	uint16_t val;

	dspg_nfc_read_buf(chip, (uint8_t *)&val, 1);
	return (uint8_t)val;
}

static void
dspg_nfc_simple_command(struct dspg_nfc_info *info, int command, int column,
						int len)
{
	int sequence;

	sequence = DW_FC_SEQ_CMD1_EN | DW_FC_SEQ_WAIT1_EN |
			   DW_FC_SEQ_CHIP_SEL(info->chipsel_nr) |
			   DW_FC_SEQ_RDY_EN | DW_FC_SEQ_RDY_SEL(info->ready_nr);

	/* Obey bus width, except for READID which is always done with 8 bit. */
	if (info->chip->options & NAND_BUSWIDTH_16 &&
	    command != NAND_CMD_READID)
		sequence |= DW_FC_SEQ_MODE16;

	if (likely(len))
		sequence |= DW_FC_SEQ_RW_EN | DW_FC_SEQ_DATA_READ_DIR;

	if (column != -1)
		sequence |= DW_FC_SEQ_ADD1_EN | DW_FC_SEQ_WAIT1_EN;

	dspg_nfc_writel(sequence, info, DW_FC_SEQUENCE);

	if (column == -1)
		dspg_nfc_writel(0, info, DW_FC_ADDR_COL);
	else
		dspg_nfc_writel(column, info, DW_FC_ADDR_COL);

	dspg_nfc_writel(0, info, DW_FC_DCOUNT);
	dspg_nfc_writel(0, info, DW_FC_TIMEOUT);
	dspg_nfc_writel(len, info, DW_FC_FTSR);
	dspg_nfc_writel(command, info, DW_FC_CMD1);

	dspg_nfc_writel(info->data_buff_phys, info, DW_FC_DPR);

	dspg_nfc_start(info);
	dspg_nfc_wait_trans_done(info);
}

static void
dspg_nfc_reset(struct dspg_nfc_info *info)
{
	dspg_nfc_simple_command(info, NAND_CMD_RESET, -1, 0);
}

/* This function prepares the program transaction for small-page chips
 * (<= 512 bytes)
 */
static int
dspg_nfc_prepare_small_write(struct dspg_nfc_info *info, int column)
{
	struct mtd_info *mtd = nand_to_mtd(info->chip);
	unsigned int res = column;
	unsigned int page_size = mtd->writesize;
	unsigned int read_cmd;
	unsigned int sequence;

	if (page_size > 512)
		BUG();

	if (column >= page_size) {
		/* OOB area */
		res -= page_size;
		read_cmd = NAND_CMD_READOOB;
	} else if (column >= 256) {
		res -= 256;
		read_cmd = NAND_CMD_READ1;
	} else {
		read_cmd = NAND_CMD_READ0;
	}

	sequence = DW_FC_SEQ_CMD4_EN | DW_FC_SEQ_WAIT4_EN | DW_FC_SEQ_KEEP_CS |
			   DW_FC_SEQ_CHIP_SEL(info->chipsel_nr) |
			   DW_FC_SEQ_RDY_SEL(info->ready_nr);

	if (info->chip->options & NAND_BUSWIDTH_16)
		sequence |= DW_FC_SEQ_MODE16;

	dspg_nfc_writel(sequence, info, DW_FC_SEQUENCE);
	dspg_nfc_writel(DW_FC_CMD_1(0) | DW_FC_CMD_2(0) |
					DW_FC_CMD_3(0) | DW_FC_CMD_4(read_cmd),
					info, DW_FC_CMD1);

	dspg_nfc_configure_ready_timeout(info);

	info->use_ecc = 0;
	dspg_nfc_set_datacount(info, 0, 0);

	dspg_nfc_start(info);
	dspg_nfc_wait_trans_done(info);

	return res; /* return column; */
}

static void
dspg_nfc_erase(struct dspg_nfc_info *info, int page_addr)
{
	info->use_ecc = 0;

	dspg_nfc_sequence(info, -1, page_addr, NAND_CMD_ERASE1);
	dspg_nfc_writel(0, info, DW_FC_DCOUNT);
	dspg_nfc_writel(0, info, DW_FC_FTSR);

	dspg_nfc_start(info);
}

static void
dspg_nfc_ecc_flush(struct dspg_nfc_info *info)
{
	int ecc_reg = info->ecc_mode ? DW_FC_BCH : DW_FC_HAMMING;

	while (dspg_nfc_readl(info, DW_FC_STATUS) & DW_FC_STATUS_ERR_FOUND)
		dspg_nfc_readl(info, ecc_reg);
}

static void
dspg_nfc_ecc_fix_bit(struct dspg_nfc_info *info, int sequence, int location)
{
	struct mtd_info *mtd = nand_to_mtd(info->chip);
	u8 *p = info->data_buff;
	int byte, bit;

	bit   = location & 0x7;
	byte  = location >> 3;

	if (byte >= VIRTUAL_PAGESIZE) {
		/* we are fixing an ecc byte */
		byte += mtd->writesize - VIRTUAL_PAGESIZE;
		byte += sequence * (info->n1len + info->n2len);
	} else
		byte += sequence * VIRTUAL_PAGESIZE;

	if (byte >= mtd->writesize + mtd->oobsize) {
		pr_err("%s(): byte = %d\n", __func__, byte);
		/* BUG(); */
		return;
	}

	p[byte] ^= (1 << bit);
}

static int
dspg_nfc_ecc_fix_hamming(struct dspg_nfc_info *info)
{
	int stat = 0;
	uint32_t hamming;
	int sequence;
	int location;

	while (info->ecc_numcodes) {
		hamming = info->ecc_codes[info->ecc_numcodes - 1];
		sequence = (hamming >> 14) & 0xf;
		location = hamming & 0x3fff;

		if ((hamming & 1) == 0) {
			info->ecc_numcodes = 0;
			return -1;
		}

		location = (location - 1) >> 1;

		dspg_nfc_ecc_fix_bit(info, sequence, location);
		stat++;

		info->ecc_numcodes--;
	}

	return stat;
}

/* we use one of the reserved bits of FC_BCH for non-correctable indication */
#define DSPG_NFC_BCH_NCMASK (1 << 31)

static int
dspg_nfc_ecc_fix_bch(struct dspg_nfc_info *info)
{
	int stat = 0;
	uint32_t bch;
	int sequence, location;
	int vector_length;

	/* see suggested formula in spec */
	vector_length  = (4096 + (info->n1len * 8));
	vector_length += dmw96_nfc_eccbits(info, info->ecc_mode) * 13 - 1;

	while (info->ecc_numcodes) {
		bch = info->ecc_codes[info->ecc_numcodes - 1];

		sequence = (bch >> 13) & 0xf;
		location = bch & 0x1fff;

		location = vector_length - location;

		if ((bch & DSPG_NFC_BCH_NCMASK) || location < 0) {
			info->ecc_numcodes = 0;
			return -1;
		}

		dspg_nfc_ecc_fix_bit(info, sequence, location);
		stat++;

		info->ecc_numcodes--;
	}

	return stat;
}

/*
 * this fetches the ecc codes during the transfer so that we can correct
 * erroneous bits with dspg_nfc_ecc_fix_errors() after the data transfer to
 * the memory has finished
 */
static void
dspg_nfc_ecc_fetch(struct dspg_nfc_info *info)
{
	int code_reg = info->ecc_mode ? DW_FC_BCH : DW_FC_HAMMING;
	uint32_t status;
	int sequence;

	status = dspg_nfc_readl(info, DW_FC_STATUS);

	/* special case for bch */
	if (unlikely(status & DW_FC_STATUS_ECC_NC_ERR)) {

		/* we manually craft and push a bch code here */
		sequence = (status >> 12) & 0xf;
		info->ecc_codes[0] = DSPG_NFC_BCH_NCMASK | sequence << 13;
		info->ecc_numcodes = 1;

		dspg_nfc_writel(DW_FC_STATUS_ECC_NC_ERR, info, DW_FC_STS_CLR);
		dspg_nfc_ecc_flush(info);
		return;
	}

	while (dspg_nfc_readl(info, DW_FC_STATUS) & DW_FC_STATUS_ERR_FOUND) {
		info->ecc_codes[info->ecc_numcodes++] =
			dspg_nfc_readl(info, code_reg);
	}
}

static int
dspg_nfc_num_zero_bits(uint8_t val)
{
	int i, ret = 0;

	for (i = 7; i >= 0 ; i--)
		if (!(0x1 & (val >> i)))
			ret++;

	return ret;
}

static int
dspg_nfc_is_corrupted_blank(struct dspg_nfc_info *info, uint8_t *buf)
{
	struct nand_chip *chip = info->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	int i;
	int zero_bits = 0;

	for (i = 0; i < mtd->writesize + info->n1len; i++) {
		if (buf[i] != 0xff)
			zero_bits += dspg_nfc_num_zero_bits(buf[i]);
	}

	if (zero_bits > chip->ecc.strength)
		return -1;

	return zero_bits;
}

static int
dspg_nfc_page_is_erased(struct dspg_nfc_info *info)
{
	struct nand_chip *chip = info->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	uint8_t dirty, *buf;
	int i, zero_bits = 0;

	dirty = info->data_buff[mtd->writesize + info->dirtypos];

	if (dirty == 0xff)
		return 1;
	else if (dirty == 0x00)
		return 0;

	/* The dirty marker is ambiguous (has bit flips). Check the ECC code.
	 * If it is an erased page, it is likely that the ECC code is mostly
	 * 1's (except for some bit flips).
	 */
	buf = &info->data_buff[mtd->writesize + info->n1len];
	for (i = 0; i < dmw96_nfc_eccsize(info, info->ecc_mode); i++) {
		if (buf[i] != 0xff)
			zero_bits += dspg_nfc_num_zero_bits(buf[i]);
	}

	if (zero_bits > chip->ecc.strength)
		return 0;

	return 1;
}

static void
dspg_nfc_ecc_fix_errors(struct dspg_nfc_info *info)
{
	struct mtd_info *mtd = nand_to_mtd(info->chip);
	int stat;

	/* fetch remaining ecc codes */
	dspg_nfc_ecc_fetch(info);

	/* check if this is a blank page; if so, fix bit flips in software */
	if (dspg_nfc_page_is_erased(info)) {
		info->ecc_numcodes = 0;

		stat = dspg_nfc_is_corrupted_blank(info, info->data_buff);
		if (stat > 0)
			memset(info->data_buff, 0xff,
			       mtd->writesize + mtd->oobsize);

		goto out;
	}

	if (info->ecc_mode == 0)
		stat = dspg_nfc_ecc_fix_hamming(info);
	else
		stat = dspg_nfc_ecc_fix_bch(info);

out:
	if (stat < 0) {
		mtd->ecc_stats.failed++;
	} else {
		mtd->ecc_stats.corrected += stat;

		if (stat > info->max_bitflips)
			info->max_bitflips = stat;
	}
}

static void
dspg_nfc_readpage(struct dspg_nfc_info *info, int column, int page_addr)
{
	struct mtd_info *mtd = nand_to_mtd(info->chip);
	int datalen, sparelen;

	info->max_bitflips = 0;

	if (column >= mtd->writesize) {
		info->use_ecc = 0;
		datalen = mtd->oobsize;
		sparelen = 0;
		dspg_nfc_sequence(info, 0, page_addr, NAND_CMD_READOOB);
		dspg_nfc_set_datacount(info, datalen, sparelen);
	} else {
		info->use_ecc = info->ecc;
		datalen = mtd->writesize;
		sparelen = mtd->oobsize;
		dspg_nfc_sequence(info, 0, page_addr, NAND_CMD_READ0);
		dspg_nfc_set_datacount(info, datalen, sparelen);
	}

	dspg_nfc_start(info);
	dspg_nfc_wait_trans_done(info);
	dspg_nfc_writel(1, info, DW_FC_STS_CLR);

	/*
	 * ATTENTION: The following call will guarantee that all memory
	 * transactions done by the device have actually hit the memory.
	 */
	ccu_barrier(0);

	if (dspg_nfc_check_timeout(info)) {
		pr_err("%s(): nand timeout, unrecoverable!\n", __func__);

		while (1)
			;
	}

	if (info->use_ecc && (info->ecc_mode > 0))
		dspg_nfc_ecc_fix_errors(info);

	info->use_ecc = info->ecc;
}

static void
dspg_nfc_writepage(struct dspg_nfc_info *info, int column, int page_addr)
{
	struct mtd_info *mtd = nand_to_mtd(info->chip);
	int datalen, sparelen;

	if (mtd->writesize <= 512)
		column = dspg_nfc_prepare_small_write(info, column);

	if (column >= mtd->writesize) {
		info->use_ecc = 0;
		datalen = mtd->oobsize;
		sparelen = 0;
		dspg_nfc_set_datacount(info, datalen, sparelen);
	} else {
		info->use_ecc = info->ecc;
		datalen = mtd->writesize;
		sparelen = mtd->oobsize;
		dspg_nfc_set_datacount(info, datalen, sparelen);
	}

	dspg_nfc_sequence(info, column, page_addr, NAND_CMD_SEQIN);
}

static void
dspg_nfc_complete_writepage(struct dspg_nfc_info *info)
{
	dspg_nfc_start(info);
}

static void
dspg_nfc_cmdfunc(struct nand_chip *chip, unsigned command,
				 int column, int page_addr)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct dspg_nfc_info *info;

	if (!mtd)
		return;

	info = chip->priv;

	clk_enable(info->clk);

	switch (command) {
	case NAND_CMD_RESET:
		dspg_nfc_reset(info);
		break;

	case NAND_CMD_READID:
		info->offset = 0;
		dspg_nfc_simple_command(info, command, column, 8);
		break;

	case NAND_CMD_READPARAMPAGE:
		info->offset = 0;
		dspg_nfc_simple_command(info, command, 0, 256*3);
		break;

	case NAND_CMD_STATUS:
		info->offset = 0;
		dspg_nfc_simple_command(info, command, -1, 1);
		break;

	case NAND_CMD_READ0:
		info->offset = column;
		dspg_nfc_readpage(info, column, page_addr);
		break;

	case NAND_CMD_READOOB:
		info->offset = column;
		dspg_nfc_readpage(info, mtd->writesize, page_addr);
		break;

	case NAND_CMD_SEQIN:
		/*
		 * NOTE: since we only support full writes right now, column
		 * must either be zero (full page write) or mtd->writesize
		 * (oob write only)
		 */
		BUG_ON(column && column != mtd->writesize);

		info->offset = 0;
		dspg_nfc_writepage(info, column, page_addr);
		break;

	case NAND_CMD_PAGEPROG:
		dspg_nfc_complete_writepage(info);
		break;

	case NAND_CMD_ERASE1:
		dspg_nfc_erase(info, page_addr);
		break;

	case NAND_CMD_ERASE2:
		break;

	default:
		pr_warn("unsupported command 0x%x\n", command);
		break;
	}

	clk_disable(info->clk);
}

static int
dspg_nfc_read_page_hwecc(struct nand_chip *chip,
			 uint8_t *buf, int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct nand_device *base = &chip->base;
	struct dspg_nfc_info *info = chip->priv;
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_calc = chip->ecc.calc_buf;
	uint8_t *ecc_code = chip->ecc.code_buf;

	nand_read_page_op(chip, page, 0, buf, mtd->writesize);
	dspg_nfc_read_buf(chip, buf, mtd->writesize);
	dspg_nfc_read_buf(chip, chip->oob_poi, mtd->oobsize);

	if (!info->use_ecc)
		return 0;

	if (info->ecc_mode > 0)
		return info->max_bitflips;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
		nand_ecc_sw_hamming_calculate(base, p, &ecc_calc[i]);

	mtd_ooblayout_set_eccbytes(mtd, chip->oob_poi, ecc_code,
				   0, chip->ecc.total);

	eccsteps = chip->ecc.steps;
	p = buf;

	for (i = 0 ; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		stat = nand_ecc_sw_hamming_correct(base, p, &ecc_code[i], &ecc_calc[i]);

		if (stat < 0)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;
	}

	return info->max_bitflips;
}

static int
dspg_nfc_write_page_hwecc(struct nand_chip *chip, const uint8_t *buf,
			  int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct nand_device *base = &chip->base;
	struct dspg_nfc_info *info = chip->priv;

	/* set dirty marker */
	chip->oob_poi[info->dirtypos] = 0;

	if (info->use_ecc && (info->ecc_mode == 0)) {
		int i, eccsize = chip->ecc.size;
		int eccbytes = chip->ecc.bytes;
		int eccsteps = chip->ecc.steps;
		uint8_t *ecc_calc = chip->ecc.calc_buf;
		const uint8_t *p = buf;

		/* Software ecc calculation */
		for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
			nand_ecc_sw_hamming_calculate(base, p, &ecc_calc[i]);

		mtd_ooblayout_set_eccbytes(mtd, ecc_calc, chip->oob_poi,
					   0, chip->ecc.total);
	}

	nand_prog_page_begin_op(chip, page, 0, buf, mtd->writesize);
	dspg_nfc_write_buf(chip, buf, mtd->writesize);
	dspg_nfc_write_buf(chip, chip->oob_poi, mtd->oobsize);

	return nand_prog_page_end_op(chip);
}

static int
dspg_nfc_nand_wait(struct nand_chip *chip)
{
	struct dspg_nfc_info *info = (struct dspg_nfc_info *)chip->priv;
	int status;

	clk_enable(info->clk);

	dspg_nfc_wait_trans_done(info);

	if (dspg_nfc_check_timeout(info)) {
		pr_err("%s(): nand timeout, unrecoverable!\n", __func__);

		while (1)
			;
	}

	status = dspg_nfc_readl(info, DW_FC_FBYP_DATA);

	if (!(status & NAND_STATUS_READY))
		pr_err("%s(): status not ready - OMFG\n", __func__);

	clk_disable(info->clk);

	return status;
}

static irqreturn_t
dspg_nfc_isr(int irq, void *dev_id)
{
	struct dspg_nfc_info *info = (struct dspg_nfc_info *)dev_id;
	uint32_t pending = dspg_nfc_readl(info, DW_FC_INT_CAUSE);

	if (pending & DW_FC_STATUS_TRANS_DONE)
		info->flags &= ~DSPG_NFC_FLAG_NEED_TRANS_DONE;

	if (pending & DW_FC_STATUS_ECC_DONE)
		info->flags &= ~DSPG_NFC_FLAG_NEED_ECC_DONE;

	if (pending & DW_FC_STATUS_AHB_DONE)
		info->flags &= ~DSPG_NFC_FLAG_NEED_AHB_DONE;

	if (pending & DW_FC_STATUS_AHB_ERR) {
		pr_err("%s(): ahb err\n", __func__);

		while (1)
			;
	}

	if (pending & (DW_FC_STATUS_ERR_FOUND | DW_FC_STATUS_ECC_NC_ERR))
		dspg_nfc_ecc_fetch(info);

	dspg_nfc_writel(pending, info, DW_FC_STS_CLR);

	if (pending && !info->flags) {
		clk_disable(info->clk);
		complete(&info->transfer_complete);
	}

	if (!pending)
		pr_err("%s(): spurious interrupt\n", __func__);

	return IRQ_HANDLED;
}

static int
dspg_nfc_bootrom_version(void)
{
	unsigned int version = 2;
	void __iomem *rom_code;

	rom_code = ioremap(0, 0x12000);

	if (!rom_code)
		return 2;

	if (*(unsigned int *)(rom_code + 0x5464) == 0x0A302E31)
		version = 1;
	else if (*(unsigned int *)(rom_code + 0x112E4) == 0x0A302E32)
		version = 2;
	else
		version = 3;

	iounmap(rom_code);

	return version;
}

static int
dspg_nfc_attach_chip(struct nand_chip *chip)
{
	const struct nand_ecc_props *requirements =
		nanddev_get_ecc_requirements(&chip->base);
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct dspg_nfc_info *info;
	int res, version;

	info = chip->priv;

	dspg_nfc_reset(info);

	version = dspg_nfc_bootrom_version();

	/* scan for ONFI chip */
	chip->options = info->buswidth == 16 ? NAND_BUSWIDTH_16 : 0;

	/* select appropriate ecc mode */
	if (chip->parameters.onfi) {
		res = dmw96_nfc_eccfind(info, requirements->strength);

		if (res < 0) {
			pr_err("Unsupported number of correctable bits: %d\n",
			       requirements->strength);
			return -ENODEV;
		}

		info->ecc_mode = res;

		if ((version >= 3) &&
			(mtd->oobsize >= 64) && (info->ecc_mode == 0))
			info->ecc_mode = 1;
	} else {
		/* use 4bit BCH on largepage devices */
		if (mtd->oobsize >= 64)
			info->ecc_mode = 1;
		else
			info->ecc_mode = 0;

		/* TODO: when to use 8bit BCH? */
	}

	/* Flashes with a page size of 512 byte usually do not support
	 * random access (NAND_CMD_RNDIN/OUT), which is required for ECC done
	 * by the controller. In this case we must use software-Hamming-ECC.
	 */
	if (mtd->writesize <= 512)
		info->ecc_mode = 0;
	chip->ecc.strength = dmw96_nfc_eccbits(info, info->ecc_mode);

	/* we can only calculate the ECC per page, not per sub-page */
	chip->options |= NAND_NO_SUBPAGE_WRITE;

	if (version < 3) {
		info->dirtypos = (chip->badblockpos & ~1) + 2;
		info->n1len = info->dirtypos + 2;
		info->n3len = info->n1len - 2;
	} else {
		info->dirtypos = 1;
		info->n1len = (chip->badblockpos & ~1) + 2;
		info->n3len = 1;
	}

	if (info->n1len > 12) {
		pr_err("invalid N1 length setting\n");
		BUG();
	}

	info->n2len = mtd->oobsize / (mtd->writesize >> VIRTUAL_PAGESIZE_SHIFT);
	info->n2len -= info->n1len;

	return 0;
}

static const struct nand_controller_ops dspg_nfc_controller_ops = {
	.attach_chip = dspg_nfc_attach_chip,
};

static int
dspg_nfc_remove(struct platform_device *pdev)
{
	struct dspg_nfc_info *info = platform_get_drvdata(pdev);
	struct mtd_info *mtd;
	struct nand_chip *chip;
	int ret;

	platform_set_drvdata(pdev, NULL);

	/* first thing we need to do is release all our mtds
	 * and their partitions, then go through freeing the
	 * resources used
	 */
	chip = info->chip;
	if (chip) {
		mtd = nand_to_mtd(chip);
		ret = mtd_device_unregister(mtd);
		nand_cleanup(chip);
	}

	clk_disable_unprepare(info->clk);

	return 0;
}

static const struct of_device_id dspg_nfc_match_table[] = {
	{
		.compatible = "dspg,nfc-rev0",
		.data = &(dspg_nfc_hw_config_revs[0])
	},
	{
		.compatible = "dspg,nfc-rev1",
		.data = &(dspg_nfc_hw_config_revs[1])
	},
	{}
};

static void
dspg_nfc_of_read_u32(const struct device_node *np, const char *propname,
		     u32 *out_value, u32 dvalue)
{
	if (of_property_read_u32(np, propname, out_value))
		*out_value = dvalue;
}

static void
dspg_nfc_of_read_timings(struct dspg_nfc_info *info,
			 struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;

	dspg_nfc_of_read_u32(np, "dspg,wait_ready_0_fall",
			     &info->wait_ready_0_fall, WAIT_READY_0_FALL);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_0_rise",
			     &info->wait_ready_0_rise, WAIT_READY_0_RISE);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_1_fall",
			     &info->wait_ready_1_fall, WAIT_READY_1_FALL);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_1_rise",
			     &info->wait_ready_1_rise, WAIT_READY_1_RISE);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_2_fall",
			     &info->wait_ready_2_fall, WAIT_READY_2_FALL);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_2_rise",
			     &info->wait_ready_2_rise, WAIT_READY_2_RISE);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_3_fall",
			     &info->wait_ready_3_fall, WAIT_READY_3_FALL);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_3_rise",
			     &info->wait_ready_3_rise, WAIT_READY_3_RISE);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_4_fall",
			     &info->wait_ready_4_fall, WAIT_READY_4_FALL);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_4_rise",
			     &info->wait_ready_4_rise, WAIT_READY_4_RISE);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_5_fall",
			     &info->wait_ready_5_fall, WAIT_READY_5_FALL);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_5_rise",
			     &info->wait_ready_5_rise, WAIT_READY_5_RISE);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_6_fall",
			     &info->wait_ready_6_fall, WAIT_READY_6_FALL);
	dspg_nfc_of_read_u32(np, "dspg,wait_ready_6_rise",
			     &info->wait_ready_6_rise, WAIT_READY_6_RISE);

	dspg_nfc_of_read_u32(np, "dspg,pulse_ale_setup",
			     &info->pulse_ale_setup, PULSE_ALE_SETUP);
	dspg_nfc_of_read_u32(np, "dspg,pulse_cle_setup",
			     &info->pulse_cle_setup, PULSE_CLE_SETUP);
	dspg_nfc_of_read_u32(np, "dspg,pulse_wr_high",
			     &info->pulse_wr_high, PULSE_WR_HIGH);
	dspg_nfc_of_read_u32(np, "dspg,pulse_wr_low",
			     &info->pulse_wr_low, PULSE_WR_LOW);
	dspg_nfc_of_read_u32(np, "dspg,pulse_rd_high",
			     &info->pulse_rd_high, PULSE_RD_HIGH);
	dspg_nfc_of_read_u32(np, "dspg,pulse_rd_low",
			     &info->pulse_rd_low, PULSE_RD_LOW);

	dspg_nfc_of_read_u32(np, "dspg,ready_timeout",
			     &info->ready_timeout, READY_TIMEOUT);
}

static int
dspg_nfc_probe(struct platform_device *pdev)
{
	struct dspg_nfc_info *info = NULL;
	struct mtd_info *mtd = NULL;
	struct nand_chip *chip = NULL;
	struct resource *regs;
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *match;
	u32 val;
	int ret;
	int irq;

	if (!np) {
		dev_err(&pdev->dev, "devicetree node\n");
		ret = -EINVAL;
		goto out;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		ret = -ENXIO;
		goto out;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto out;
	}

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		ret = -ENOMEM;
		goto out;
	}

	info->chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);

	if (!info->chip) {
		ret = -ENOMEM;
		goto out;
	}

	info->data_buff = dma_alloc_coherent(&pdev->dev, NAND_DATABUF_SIZE,
					     &info->data_buff_phys, GFP_KERNEL);

	if (info->data_buff == NULL) {
		dev_err(&pdev->dev, "failed to allocate dma buffer\n");
		ret = -ENOMEM;
		goto out;
	}

	dspg_nfc_of_read_timings(info, pdev);

	match = of_match_node(dspg_nfc_match_table, np);
	info->hw_config = (struct dspg_nfc_hw_config *)match->data;

	info->buswidth = info->chip->options & NAND_BUSWIDTH_16 ? 16 : 8;

	val = 0;
	ret = of_property_read_u32(np, "dspg,chip-select", &val);

	if ((ret && ret != -EINVAL) || val > 1) {
		dev_err(&pdev->dev, "invalid 'dspg,chip-select'\n");
		goto out_buf;
	}

	info->chipsel_nr = val;

	val = 0;
	ret = of_property_read_u32(np, "dspg,ready-select", &val);

	if ((ret && ret != -EINVAL) || val > 1) {
		dev_err(&pdev->dev, "invalid 'dspg,ready-select'\n");
		goto out_buf;
	}

	info->ready_nr = val;

	info->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(info->clk)) {
		dev_err(&pdev->dev, "failed to get nand clock\n");
		ret = PTR_ERR(info->clk);
		goto out_buf;
	}

	info->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(info->reset)) {
		dev_err(&pdev->dev, "cannot get reset control\n");
		ret = PTR_ERR(info->reset);
		goto out_buf;
	}

	ret = reset_control_deassert(info->reset);
	if (ret) {
		dev_err(&pdev->dev, "failed to deassert reset\n");
		goto out_buf;
	}

	clk_prepare_enable(info->clk);

	platform_set_drvdata(pdev, info);

	if (!devm_request_mem_region(&pdev->dev, regs->start,
				     resource_size(regs), "regs")) {
		dev_err(&pdev->dev, "request_mem_region failed\n");
		ret = -EBUSY;
		goto out_clk;
	}

	info->regs = devm_ioremap(&pdev->dev, regs->start, resource_size(regs));
	if (!info->regs) {
		dev_err(&pdev->dev, "unable to map registers\n");
		ret = -ENOMEM;
		goto out_clk;
	}

	/* initialise chip data struct */
	chip = info->chip;

	chip->options     = 0;
	chip->legacy.cmdfunc     = dspg_nfc_cmdfunc;
	chip->legacy.read_byte   = dspg_nfc_read_byte;
	chip->legacy.read_buf    = dspg_nfc_read_buf;
	chip->legacy.write_buf   = dspg_nfc_write_buf;
	chip->legacy.select_chip = dspg_nfc_select_chip;
	chip->legacy.waitfunc    = dspg_nfc_nand_wait;
	chip->legacy.set_features = nand_get_set_features_notsupp;
	chip->legacy.get_features = nand_get_set_features_notsupp;
	chip->ecc.engine_type = NAND_ECC_ENGINE_TYPE_ON_HOST;
	chip->ecc.size = VIRTUAL_PAGESIZE;
	chip->ecc.read_page = dspg_nfc_read_page_hwecc;
	chip->ecc.write_page = dspg_nfc_write_page_hwecc;

	info->ecc = 1;
	info->use_ecc = info->ecc;

	/* initialise mtd info data struct */
	mtd = nand_to_mtd(chip);
	mtd->priv = chip;
	mtd->owner = THIS_MODULE;
	mtd->name = dspg_nfc_name;

	/* Initialize write size and oob size */
	/* dspg_nfc_read_buf fails without this fix */
	/* For non ONFI flashes, read ID scenario */
	mtd->writesize = DSPG_MAX_PAGESIZE;
	mtd->oobsize = DSPG_MAX_OOBSIZE;
	/* initialize the hardware */
	nand_controller_init(&info->controller);
	info->controller.ops = &dspg_nfc_controller_ops;
	nand_set_controller_data(chip, info);
	chip->controller = &info->controller;

	dspg_nfc_clear_status(info);
	dspg_nfc_set_fixed_regs(info);

	init_completion(&info->transfer_complete);

	ret = devm_request_irq(&pdev->dev, irq, dspg_nfc_isr, IRQF_ONESHOT,
			       dspg_nfc_name, info);
	if (ret < 0) {
		dev_err(&pdev->dev, "cannot request interrupt\n");
		ret = -EBUSY;
		goto out_clk;
	}

	info->irq = irq;
	dspg_nfc_writel(DW_FC_STATUS_TRANS_DONE | DW_FC_STATUS_ECC_DONE |
			DW_FC_STATUS_ERR_FOUND | DW_FC_STATUS_ECC_NC_ERR |
			DW_FC_STATUS_AHB_ERR | DW_FC_STATUS_AHB_DONE,
			info, DW_FC_INT_EN);

	mtd_set_ooblayout(mtd, &dspg_nfc_ooblayout_ops);
	if (nand_scan(chip, 1)) {
		ret = -ENXIO;
		goto out_clk;
	}

	mtd_device_register(mtd, NULL, 0);

	clk_disable(info->clk);
	dev_info(&pdev->dev, "initialized\n");

	return 0;

out_clk:
	clk_disable_unprepare(info->clk);
out_buf:
	dma_free_coherent(&pdev->dev, NAND_DATABUF_SIZE, info->data_buff,
					  info->data_buff_phys);
out:
	return ret;
}

static struct platform_driver dspg_nfc_driver = {
	.probe		= dspg_nfc_probe,
	.remove		= dspg_nfc_remove,
	.driver		= {
		.name	= dspg_nfc_name,
		.owner	= THIS_MODULE,
		.of_match_table = dspg_nfc_match_table,
	},
};
module_platform_driver(dspg_nfc_driver);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("DSPG NFC driver");
MODULE_LICENSE("GPL");
