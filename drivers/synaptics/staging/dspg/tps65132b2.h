/*
 * This file is part of DSPG Technologies' TPS65132B2 driver.
 *
 * The TPS65132B2 driver is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * The TPS65132B2 driver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the TPS65132B2 driver.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TPS65132B2_H
#define __TPS65132B2_H

#include <linux/types.h>
#include <linux/i2c.h>

#define REG_VPOS	0x00
#define REG_VNEG	0x01
#define REG_DLYX	0x02
#define REG_DIV		0x03
#define REG_CONTROL	0xFF

#define TPS6513_REGS	4

#define DLYX_0_MS	0
#define DLYX_1_MS	1
#define DLYX_5_MS	2
#define DLYX_10_MS	3

#define DIV_DISN(x)	(((x) & 0x1) << 0)
#define DIV_DISP(x)	(((x) & 0x1) << 1)
#define DIV_SEQD(x)	(((x) & 0x3) << 2)
#define DIV_SEQU(x)	(((x) & 0x3) << 4)
#define DIV_APPS(x)	(((x) & 0x1) << 6)

#define SEQ_SIM		0	/* simultaneous */
#define SEQ_P_N		1	/* V_p -> V_n TODO correct? */
#define SEQ_N_P		2	/* V_n -> V_p TODO correct? */
#define SEQ_UNKNOWN	3	/* TODO */

#define DLYN1(x)	(((x) & 0x3) << 0)
#define DLYP1(x)	(((x) & 0x3) << 2)
#define DLYN2(x)	(((x) & 0x3) << 4)
#define DLYP2(x)	(((x) & 0x3) << 6)

#define WRITE_EEPROM	(1 << 7)

#define VOUT_MASK	0x1F
#define VOUT(val)	((val) - 40)

struct tps65132b2_data {
	struct device		*dev;
	struct i2c_client	*i2c_client;
	u32			pos_out;
	u32			neg_out;
	u8			up_seq;
	u8			up_dly_p;
	u8			up_dly_n;
	u8			down_seq;
	u8			down_dly_p;
	u8			down_dly_n;
};

#endif /* __TPS65132B2_H */
