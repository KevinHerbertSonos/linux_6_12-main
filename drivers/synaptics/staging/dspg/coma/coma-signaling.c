/*
 * drivers/staging/dspg/coma/coma-signaling.c - signaling between domains
 *
 * This module provides an infrastructure for signaling new messages to the
 * other domain and for informing the corresponding service of a new message
 * sent by the other domain.
 * Signaling is either implemented through software-triggered interrupts if
 * both domains are hosted on the same CPU (VegaFB) or through the "semaphore
 * exchange center" if each domain runs on a dedicated processer (DW).
 *
 * Copyright (C) 2009 - 2012 DSP Group Inc.
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

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/sec.h>
#include <linux/err.h>
#include <linux/coma/coma.h>

#include <mach/platform.h>

#include "coma-signaling.h"

/*
 * External interface:
 * coma_signaling_init()  - set up signaling infrastructure
 * coma_signaling_exit()  - tear down signaling infrastructure
 * coma_signal(id)        - signal a new outgoing message for a specific
 *                          service to other domain
 *
 * Functions to be provided by the cordless manager:
 * coma_receive(id)       - signal a new incoming message from the other
 *                          domain for a specific service
 */

static struct sec_msg *sec[CONFIG_DSPG_COMA_MAX_SERVICES];

static void sec_handler(int id, unsigned long data, void *context)
{
	coma_receive(id - SEC_MSG_COMA_START);
}

void coma_signal(unsigned int id)
{
	wmb(); /* Make sure all CFIFO changes are committed */
	sec_msg_trigger(sec[id], 0xdeadbeef);
}

int coma_signaling_init(void)
{
	int i, ret;

	BUILD_BUG_ON(SEC_MSG_COMA_END >= SEC_MSG_CSS_PANIC);

	for (i = 0; i < CONFIG_DSPG_COMA_MAX_SERVICES; i++) {
		sec[i] = sec_msg_register(sec_handler, i + SEC_MSG_COMA_START,
					  0, NULL);
		if (IS_ERR(sec[i])) {
			printk(KERN_ERR "coma: cannot register SEC %d\n", i);
			ret = PTR_ERR(sec[i]);
			goto err;
		}
	}

	return 0;

err:
	while (--i >= 0)
		sec_msg_deregister(sec[i]);
	return ret;
}

void coma_signaling_exit(void)
{
	int i;

	for (i = 0; i < CONFIG_DSPG_COMA_MAX_SERVICES; i++)
		sec_msg_deregister(sec[i]);
}
