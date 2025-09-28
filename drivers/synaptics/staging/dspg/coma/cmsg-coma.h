/*
 * drivers/staging/dspg/coma/cmsg-coma.h - cordless messages / cordless manager
 *
 * Copyright (C) 2009-2012 DSP Group Inc.
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

#ifndef CMSG_COMA_H
#define CMSG_COMA_H

enum cmsg_coma_types {
	CMSG_COMA_REQUEST_INIT = 0,
	CMSG_COMA_REPLY_INIT,
	CMSG_COMA_DEINIT,
	CMSG_COMA_REQUEST_REGISTER,
	CMSG_COMA_REPLY_REGISTER,
	CMSG_COMA_REQUEST_DEREGISTER,
	CMSG_COMA_REPLY_DEREGISTER,
	CMSG_COMA_NUM_TYPES,
};

union cmsg_coma_params {
	/* CMSG_REQUEST_INIT */
	struct  request_init {
		int options;
	} request_init;

	/* CMSG_REPLY_INIT */
	struct  reply_init {
		int result;
	} reply_init;

	/* CMSG_REQUEST_REGISTER */
	struct  request_register {
		int id;
		char name[16];
		unsigned int l2c;
		unsigned int c2l;
	} request_register;

	/* CMSG_REPLY_REGISTER */
	struct  reply_register {
		int id;
		int result;
	} reply_register;

	/* CMSG_REQUEST_DEREGISTER */
	struct  request_deregister {
		int id;
	} request_deregister;

	/* CMSG_REPLY_DEREGISTER */
	struct  reply_deregister {
		int id;
		int result;
	} reply_deregister;
};

#endif /* CMSG_COMA_H */
