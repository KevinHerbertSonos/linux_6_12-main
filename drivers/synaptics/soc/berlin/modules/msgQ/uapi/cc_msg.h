/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _CC_MSG_H_
#define _CC_MSG_H_

typedef struct {
	/*Message ID*/
	u32 m_MsgID;
	/*Message 1st Parameter*/
	u32 m_Param1;
	/*Message 2nd Parameter*/
	u64 m_Param2;
} CC_MSG_t;

#endif //_CC_MSG_H_
