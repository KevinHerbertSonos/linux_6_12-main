/*
 * drivers/staging/dspg/coma/cmsg-sharedmem.h - shared memory allocation cmsg layer
 *
 *  Copyright (C) 2013 DSP Group Inc.
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

#ifndef CMSG_SHAREDMEM_H
#define CMSG_SHAREDMEM_H

enum cmsg_sharedmem_types {
	CMSG_SHAREDMEM_INIT = 0,
	CMSG_SHAREDMEM_INIT_REPLY,
	CMSG_SHAREDMEM_MEMINFO,
	CMSG_SHAREDMEM_MEMINFO_REPLY,
};

union cmsg_sharedmem_params {
	/* CMSG_SHAREDMEM_INIT */
	struct init {
		unsigned long virt_base;
		unsigned long phys_base;
		unsigned int size;
		unsigned int memories;
	} init;
	/* CMSG_SHAREDMEM_INIT_REPLY */
	struct init_reply {
		int status;
	} init_reply;
	/* CMSG_SHAREDMEM_MEMINFO */
	struct meminfo {
		unsigned int id;
	} meminfo;
	/* CMSG_SHAREDMEM_MEMINFO_REPLY */
	struct meminfo_reply {
		int status;
		unsigned int id;
		unsigned long base;
		unsigned int size;
	} meminfo_reply;
};

#endif
