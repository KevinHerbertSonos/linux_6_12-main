/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __COMMON_TYPE_H__
#define __COMMON_TYPE_H__

#include <linux/slab.h>
#include <linux/string.h>

#include "ctypes.h"

#define SUCCESS				0
#define FAILURE				-1

#ifndef Txxb
#define Txxb
typedef UNSG8	T8b;
typedef UNSG16	T16b;
typedef UNSG32	T32b;
typedef UNSG32	T64b[2];
typedef UNSG32	T96b[3];
typedef UNSG32	T128b[4];
typedef UNSG32	T160b[5];
typedef UNSG32	T192b[6];
typedef UNSG32	T224b[7];
typedef UNSG32	T256b[8];
#endif

#if defined(__linux__)
#elif defined(NON_OS)
/* blank */
#else
#error ERROR: OS not specified
#endif

////////////////////////////////////////////////////////////////////////////////
//! \brief Basic data types and macros
////////////////////////////////////////////////////////////////////////////////
typedef unsigned char		UCHAR;
typedef char		CHAR;
#if defined(NON_OS)
/* blank */
#else
typedef char		WCHAR;
#endif
#ifndef BOOL
#ifndef _typedef___BOOL
//#define _typedef___BOOL
typedef UCHAR		BOOL;
#endif
#endif

typedef UCHAR				BOOLEAN;
typedef short				SHORT;
typedef unsigned short		USHORT;
typedef int					INT;
typedef unsigned int		UINT;
typedef long				LONG;
typedef unsigned long		ULONG;
typedef long long			LONGLONG;
typedef unsigned long long	ULONGLONG;
typedef void				VOID;
typedef void				*PTR;
typedef void				**PHANDLE;
typedef void				*HANDLE;
typedef void				*PVOID;

typedef UCHAR               BYTE;

#ifndef _typedef___INT8
#define _typedef___INT8
typedef CHAR                INT8;
#endif /* _typedef___INT8 */
#ifndef _typedef___UINT8
#define _typedef___UINT8
typedef UCHAR               UINT8;
#endif /* _typedef___UINT8 */
#ifndef _typedef___INT16
#define _typedef___INT16
typedef short               INT16;
#endif /* _typedef___INT16 */
#ifndef _typedef___UINT16
#define _typedef___UINT16
typedef unsigned short      UINT16;
#endif /* _typedef___UINT16 */
#ifndef _typedef___INT32
#define _typedef___INT32
typedef int                 INT32;
#endif /* _typedef___INT32 */
#ifndef _typedef___UINT32
#define _typedef___UINT32
typedef unsigned int        UINT32;
#endif /* _typedef___UINT32 */

#ifndef _typedef___INT64
#define _typedef___INT64
typedef long long           INT64;
#endif

#ifndef _typedef___UINT64
#define _typedef___UINT64
typedef unsigned long long  UINT64;
#endif

typedef unsigned int        SIZE_T;

#ifndef _typedef___HRESULT
#define _typedef___HRESULT
typedef signed int			HRESULT;
#endif /* _typedef___HRESULT */


#ifndef _MAX_PATH
#define _MAX_PATH						260
#endif

#ifndef TRUE
//#define TRUE							(1)
#endif
#ifndef FALSE
//#define FALSE							(0)
#endif

#ifndef true
//#define true							(1)
#endif
#ifndef false
//#define false							(0)
#endif

#ifndef True
//#define True							(1)
#endif
#ifndef False
//#define False							(0)
#endif

#ifndef NULL
#define NULL			((void *)0)
#endif

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef INOUT
#define INOUT
#endif

//typedef unsigned int uint;

#define MV_CAST(type)	type

////////////////////////////////////////////////////////////////////////////////
//! \brief Other data types and macros
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////
//! \brief F16DOT16 data type
//!
//! The fixed-point data type can be used in place of float/double data types.
//! E.g., the F16DOT16 data type consists of a 16-bit signed mantissa and a positive
//! unsigned 16-bit fraction (32-bits total), i.e., fractional accuracy to 1/64K of a unit.
//! Some F16DOT16 values:
//! 0x00010000 is 1.0,
//! 0x00014000 is 1.25,
//! 0x00018000 is 1.5,
//! 0xFFFF0000 is -1.0,
//! 0xFFFF4000 is -0.75 (i.e., -1.0 + 0.25),
//! 0xFFFF8000 is -0.5,
//! 0xFFFE4000 is -1.75
//////////////////////////////
typedef INT32 F16DOT16;
#define F16DOT16_HIBITS 16
#define F16DOT16_LOBITS 16
#define itoF16DOT16(x)  ((x) << F16DOT16_LOBITS)
#define F16DOT16_0      0
#define F16DOT16_1      itoF16DOT16(1)
#define F16DOT16_half   (F16DOT16_1 >> 1)
#define ftoF16DOT16(float_value) ((F16DOT16)((float_value)*F16DOT16_1))

#define F16DOT16_LOMASK (F16DOT16_1 - 1)
#define F16DOT16_HIMASK itoF16DOT16((1 << F16DOT16_HIBITS) - 1)
#define F16DOT16_SIGNBIT      (1 << (F16DOT16_HIBITS+F16DOT16_LOBITS-1))

#define MAKE_F16DOT16(float_value) ((F16DOT16)((float_value)*F16DOT16_1))
#define F16DOT16_to_int(value) ((int)((F16DOT16)(value)) >> F16DOT16_LOBITS)
#define F16DOT16_to_float(value) (((float)1.0f * (F16DOT16)(value))/F16DOT16_1)
#define F16DOT16_to_double(value) (((double)1.0 * (F16DOT16)(value))/F16DOT16_1)
#define F16DOT16_mul(a, b) ((F16DOT16)(((long long)(a)*(b)) >> F16DOT16_LOBITS))
#define F16DOT16_div(a, b) ((F16DOT16)((((long long)(a)) << F16DOT16_LOBITS)/(b)))



////////////////////////////////////////////////////////////////////////////////
//! \brief Memory allocation and related...
////////////////////////////////////////////////////////////////////////////////
#define GaloisCalloc		calloc

#define GALOIS_CACHE_LINE			32

#define GaloisMalloc(s)		kmalloc(s, GFP_KERNEL)
#define GaloisCalloc		calloc
#define GaloisFree(p)               \
	do {                            \
		if ((p) != NULL) {          \
			kfree((void *)(p));      \
			(p) = NULL;             \
		}                           \
	} while (0)

#define GaloisAlignMalloc   Galois_align_malloc
#define GaloisAlignFree     Galois_align_free

#if defined(__linux__) || !defined(SMALL_MEM_POOL_SUPPORT)
#define	CommonMemPoolMalloc	GaloisMalloc
#define CommonMemPoolFree	GaloisFree
#endif

// BD IG/PG and GFX Memory malloc/free functions for small buffer.
// By Wanyong, 20090605.
#define GFXMalloc	CommonMemPoolMalloc
#define GFXFree		CommonMemPoolFree
#define BDRESUBMalloc		CommonMemPoolMalloc
#define BDRESUBFree		CommonMemPoolFree

#define GaloisMemmove		memmove
#define GaloisMemcpy		memcpy
#define GaloisMemClear(buf, n)	memset((buf), 0, (n))
#define GaloisMemSet(buf, c, n)	memset((buf), (c), (n))
#define GaloisMemComp(buf1, buf2, n)	memcmp((buf1), (buf2), (n))

#define UINT_ADDR(a, b)		((UINT *)(a) + ((b) >> 2))
#define USHORT_ADDR(a, b)	((USHORT *)(a) + ((b) >> 1))
#define UCHAR_ADDR(a, b)	((UCHAR *)(a) + (b))
#define INT_ADDR(a, b)		((INT *)(a) + ((b) >> 2))
#define SHORT_ADDR(a, b)		((SHORT *)(a) + ((b) >> 1))
#define CHAR_ADDR(a, b)		((CHAR *)(a) + (b))



////////////////////////////////////////////////////////////////////////////////
//! \brief Some useful macros and defs
////////////////////////////////////////////////////////////////////////////////
#ifndef MIN
#define MIN(A, B)		((A) < (B) ? (A) : (B))
#endif
#ifndef MAX
#define MAX(A, B)		((A) > (B) ? (A) : (B))
#endif

#ifndef SWAP
#define SWAP(a, b) \
	do { \
		(a) ^= (b); \
		(b) ^= (a); \
		(a) ^= (b); \
	} while (0)
#endif

#ifndef ASSERT
#define ASSERT(x)           MV_ASSERT(x)
#endif  // ASSERT

/*common definition*/
#define KB					(0x400)
#define MB					(0x100000)

#ifdef CPU_BIG_ENDIAN
#define FCC_GEN(a, b, c, d)		(((UINT32)(UINT8)(a) << 24) | \
		((UINT32)(UINT8)(b) << 16) | \
		((UINT32)(UINT8)(c) << 8) | \
		((UINT32)(UINT8)d))
#else
#define FCC_GEN(a, b, c, d)     (((UINT32)(UINT8)(d) << 24) | \
		((UINT32)(UINT8)(c) << 16) | \
		((UINT32)(UINT8)(b) << 8) | \
		((UINT32)(UINT8)a))
#endif

// Definitions for array operations
#define GaloisMemberIndexInArray(array, member) \
	(((unsigned long)(member) - (unsigned long)(array)) / sizeof(*(member)))

	// Get number of items in an array;
	// The input parameter 'array' has to be a 'c' array, not a pointer or 'c++' array
#define GaloisSizeOfArray(array) (ARRAY_SIZE(array)/ARRAY_SIZE(array[0]))

////////////////////////////////////////////////////////////////////////////////
//! \brief definition: MV_CC_DEFAULT_TIMEOUT
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_CC_DEFAULT_NOWAIT		(0)			// Time out = 0 , no wait
#define MV_CC_DEFAULT_TIMEOUT_64	(20*1000ll*1000*1000)	// 1s = 1000ms * 1000us * 1000ns
#define MV_CC_DEFAULT_TIMEOUT_U64	(20*1000ull*1000*1000)	// 1s = 1000ms * 1000us * 1000ns
#define MV_CC_DEFAULT_TIMEOUTMS		(20*1000)				// 1s = 1000ms

#define MV_CC_NSEC_PER_SEC_64		(1*1000ll*1000*1000)	// 1s = 1000ms * 1000us * 1000ns
#define MV_CC_USEC_PER_SEC_64		(1*1000ll*1000)			// 1s = 1000ms * 1000us
#define MV_CC_MSEC_PER_SEC_64		(1*1000ll)				// 1s = 1000ms
#define MV_CC_NSEC_PER_MSEC_64		(1*1000ll*1000)			// 1ms = 1000us * 1000ns

#define MV_CC_NSEC_PER_SEC_U64		(1*1000ull*1000*1000)	// 1s = 1000ms * 1000us * 1000ns
#define MV_CC_USEC_PER_SEC_U64		(1*1000ull*1000)		// 1s = 1000ms * 1000us
#define MV_CC_MSEC_PER_SEC_U64		(1*1000ull)				// 1s = 1000ms
#define MV_CC_NSEC_PER_MSEC_U64		(1*1000ull*1000)		// 1ms = 1000us * 1000ns

///////////////////////////////////////////////////////////////////////////////
//! Purpose:
//!	Defines 3 MACRO to unsigned char array into short,
//!	integer, and 64bit integer
//! Note:
//!	Since all info structures are stored in file as unsigned char array,
//!	the parser will swap bytes in order to move the first byte of array
//!	into MSB position.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//! \brief MACRO: B2N_16
//!
//! Description: Swap two bytes to make them as a 16bit integer
//!
//! \param x (IN/OUT) : -- 16bit integer input variable and put the result into same variable
//!
//! \return Return: none
///////////////////////////////////////////////////////////////////////////////
#define B2N_16(x)                               \
	(x = ((((x) & 0xff00) >> 8) |                  \
		  (((x) & 0x00ff) << 8)))

///////////////////////////////////////////////////////////////////////////////
//! \brief MACRO: B2N_32
//!
//! Description: Swap four bytes to make them as a 32bit integer
//!
//! \param x (IN/OUT) : -- 32bit integer input variable and put the result into same variable
//!
//! \return Return: none
///////////////////////////////////////////////////////////////////////////////
#define B2N_32(x)                               \
	(x = ((((x) & 0xff000000) >> 24) |             \
		  (((x) & 0x00ff0000) >>  8) |             \
		  (((x) & 0x0000ff00) <<  8) |             \
		  (((x) & 0x000000ff) << 24)))

///////////////////////////////////////////////////////////////////////////////
//! \brief MACRO: B2N_64
//!
//! Description: Swap eight bytes to make them as a 64bit integer
//!
//! \param x (IN/OUT) : -- 64bit integer input variable and put the result into same variable
//!
//! \return Return: none
//!
//! Note: so far no one is using this.
///////////////////////////////////////////////////////////////////////////////
#define B2N_64(x)                               \
	(x = ((((x) & 0xff00000000000000) >> 56) |     \
		  (((x) & 0x00ff000000000000) >> 40) |     \
		  (((x) & 0x0000ff0000000000) >> 24) |     \
		  (((x) & 0x000000ff00000000) >>  8) |     \
		  (((x) & 0x00000000ff000000) <<  8) |     \
		  (((x) & 0x0000000000ff0000) << 24) |     \
		  (((x) & 0x000000000000ff00) << 40) |     \
		  (((x) & 0x00000000000000ff) << 56)))

#define DBGPRINTF(x)        (printf x)
#define ERRPRINTF(x)        (printf x)
#define LOGPRINTF(x)        (printf x)
#define dbg_Out_Log(x, ...)

#define MV_ASSERT(x)			do {} \
	while (0)
#define GaloisMemmove           memmove
#define GaloisMemcpy            memcpy
#define GaloisMemClear(buf, n)  memset((buf), 0, (n))
#define GaloisMemSet(buf, c, n) memset((buf), (c), (n))
#define GaloisMemComp(buf1, buf2, n)    memcmp((buf1), (buf2), (n))

#endif	// #ifndef __COMMON_TYPE__
