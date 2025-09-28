/*
 *  drivers/misc/dspg_ccu.c
 *
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

#include <linux/init.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/kernel.h>
#include <linux/slab.h>

/*
 * Adjust this to the memory controller FIFO depth (in words) to get best
 * performance.
 */
#define MAX_FIFO_DEPTH                 16

#define CCU_CONT                       0x000
#define CCU_CONT_MODE_RaW              (0ul << 17)
#define CCU_CONT_MODE_WriteN           (1ul << 17)
#define CCU_CONT_SWEN                  (1ul << 16)
#define CCU_STAT                       0x004
#define CCU_STAT_STATE_MASK            (3ul << 30)
#define CCU_STAT_STATE_IDLE            (0ul << 30)
#define CCU_STAT_STATE_WAIT            (1ul << 30)
#define CCU_STAT_STATE_BUSY            (2ul << 30)
#define CCU_STAT_ERROR                 (1ul << 29)
#define CCU_STAT_SW_REQ                (1ul << 16)
#define CCU_REQ                        0x008
#define CCU_SWTRIG                     0x00C
#define CCU_SWTRIG_MAGIC               0x90EE
#define CCU_DUMADR                     0x010
#define CCU_DUMDAT                     0x014
#define CCU_RDAT                       0x018

enum ccu_mode {
	CCU_MODE_N_WRITES,
	CCU_MODE_READ_AFTER_WRITE,
};

struct ccu {
	void __iomem *base;
	unsigned int fifo_depth;
	enum ccu_mode mode;
	struct clk *clk;
};

static struct ccu *ccus = NULL;
static unsigned int nr_ccus = 0;

static unsigned long ccu_victim[MAX_FIFO_DEPTH] __attribute__ ((aligned(64)));

/*
 * Initiate a CCU transaction to guarantee coherency wrt. writes of other bus
 * masters. Might be called recursively from different threads.
 */
void ccu_barrier(unsigned int id)
{
	struct ccu *ccu;

	if (id >= nr_ccus)
		return;

	ccu = &ccus[id];

	switch (ccu->mode) {
	case CCU_MODE_N_WRITES:
		/* wait for CCU to be idle */
		while (readl(ccu->base + CCU_STAT) & CCU_STAT_SW_REQ)
			cpu_relax();

		/* initiate coherency transaction */
		writel(CCU_SWTRIG_MAGIC, ccu->base + CCU_SWTRIG);

		/* wait for CCU to finish request */
		while (readl(ccu->base + CCU_STAT) & CCU_STAT_SW_REQ)
			cpu_relax();
		break;
	case CCU_MODE_READ_AFTER_WRITE:
		break;
	}
}

static void __init ccu_init(struct device_node *np)
{
	struct ccu *ccu;
	unsigned int id;
	struct reset_control *rc;

	if (of_property_read_u32(np, "id", &id))
		panic("%s: reg property invalid\n", np->name);

	if (id >= nr_ccus) {
		panic("%s: invalid ccu id %u\n", np->name, id);
	}
	ccu = &ccus[id];

	ccu->clk = of_clk_get(np, 0);
	if (IS_ERR(ccu->clk)) {
		pr_warn("%s: unable to get clocks\n", np->name);
		ccu->clk = NULL;
	}

	rc = of_reset_control_get_shared(np, NULL);
	if (IS_ERR(rc))
		panic("%s: unable to get reset control\n", np->name);
	if (reset_control_deassert(rc))
		panic("%s: unable to reset ccu\n", np->name);

	ccu->base = of_iomap(np, 0);
	if (!ccu->base)
		panic("%s: unable to map io memory\n", np->name);

	if (ccu->clk)
		clk_prepare_enable(ccu->clk);

	if (of_find_property(np, "n-writes-mode", NULL)) {
		ccu->mode = CCU_MODE_N_WRITES;
		if (of_property_read_u32(np, "fifo-depth", &ccu->fifo_depth))
			panic("%s: no fifo-depth specified\n", np->name);
		if (ccu->fifo_depth > MAX_FIFO_DEPTH || !ccu->fifo_depth)
			panic("%s: fifo-depth must be in the range of 1-%u\n",
			      np->name, MAX_FIFO_DEPTH);

		/* enable software requests with 'N Writes' mode */
		writel(((ccu->fifo_depth-1) << 18) | CCU_CONT_MODE_WriteN | CCU_CONT_SWEN,
			ccu->base + CCU_CONT);
		writel((virt_to_phys(ccu_victim) + 0x3f) & ~0x3f,
			ccu->base + CCU_DUMADR);
		writel(0xDEADBEEF, ccu->base + CCU_DUMDAT);
		pr_info("%s-%u: n-writes-mode fifo-depth %u successfully probed\n",
			np->name, id, ccu->fifo_depth);
	} else
		panic("%s: unsupported ccu mode\n", np->name);

	of_node_put(np);
}

static int __init of_ccu_init(void)
{
	struct device_node *np;

	for_each_compatible_node(np, NULL, "dspg,ccu")
		nr_ccus++;

	if (!nr_ccus)
		return 0;

	ccus = kzalloc(nr_ccus * sizeof(*ccus), GFP_KERNEL);
	if (!ccus)
		panic("no memory for %u ccus\n", nr_ccus);

	for_each_compatible_node(np, NULL, "dspg,ccu")
		ccu_init(np);

	return 0;
}
subsys_initcall(of_ccu_init);
