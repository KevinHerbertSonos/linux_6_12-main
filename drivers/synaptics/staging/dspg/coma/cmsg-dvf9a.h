/*
 * drivers/staging/dspg/coma/cmsg-dvf9a.h - dvf9a cmsg layer
 *
 *  Copyright (C) 2012 DSP Group Inc.
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

#ifndef CMSG_DVF9A_H
#define CMSG_DVF9A_H

enum cmsg_dvf9a_types {
	CMSG_DVF9A_REQUEST_REGISTER_IRQ = 0,
	CMSG_DVF9A_REPLY_REGISTER_IRQ,
	CMSG_DVF9A_REQUEST_RELEASE_IRQ,
	CMSG_DVF9A_REPLY_RELEASE_IRQ,
	CMSG_DVF9A_INTERRUPT,
	CMSG_DVF9A_ACK_INTERRUPT,
	CMSG_DVF9A_ACQUIRE,
	CMSG_DVF9A_ACK_ACQUIRE,
};

union cmsg_dvf9a_params {
	/* CMSG_DVF9A_REQUEST_REGISTER_IRQ */
	struct request_register_irq {
		int irq;
	} request_register_irq;

	/* CMSG_DVF9A_REPLY_REGISTER_IRQ */
	struct reply_register_irq {
		int result;
	} reply_register_irq;

	/* CMSG_DVF9A_REQUEST_RELEASE_IRQ */
	struct request_release_irq {
		int irq;
	} request_release_irq;

	/* CMSG_DVF9A_REPLY_RELEASE_IRQ */
	struct reply_release_irq {
		int result;
	} reply_release_irq;

	/* CMSG_DVF9A_INTERRUPT */
	struct interrupt {
		int irq;
	} interrupt;

	/* CMSG_DVF9A_ACK_INTERRUPT */
	struct ack_interrupt {
		int irq;
	} ack_interrupt;
};

#endif /* CMSG_DATAHANDLER_H */
