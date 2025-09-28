/*
 * drivers/staging/dspg/coma/coma-dvf9a.h - service for access to the APU (DAIF)
 *
 * Copyright (C) 2012 DSP Group Inc.
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

#ifndef _COMA_DVF9A_H
#define _COMA_DVF9A_H

#ifdef CONFIG_DSPG_CSS_DVF9A
int dvf9a_coma_service_init(void);
#else
static inline int dvf9a_coma_service_init(void)
{
	return 0;
}
#endif

#endif /* _COMA_DVF9A_H */
