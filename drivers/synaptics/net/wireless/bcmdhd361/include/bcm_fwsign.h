/*
 * bcm_fwsign.h
 *
 *
 * Copyright (C) 2025 Synaptics Incorporated. All rights reserved.
 *
 * This software is licensed to you under the terms of the
 * GNU General Public License version 2 (the "GPL") with Broadcom special exception.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION
 * DOES NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES,
 * SYNAPTICS' TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT
 * EXCEED ONE HUNDRED U.S. DOLLARS
 *
 * Copyright (C) 2025, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Dual:>>
 */

/* This header defines data types to support firmware signing. Firmware signing
 * functionality is provided by a bootloader in ROM. Firmware signing feature
 * is described here (note: the link may be updated)
 *    https://drive.google.com/open?id=17_y7wOrmJr8nvB2ClxTw9gHokIjXfbSLOLRhXzWCQEg
 */

#ifndef _BCM_FWSIGN_H_
#define _BCM_FWSIGN_H_

#include <typedefs.h>
#ifdef BCMDRIVER
#include <osl.h>
#else
#include <stddef.h>
#endif

#include <bcmutils.h>
#include <bcmtlv.h>
#include <bcmerror.h>

/* General encoding rules
 *    -- There are no special alignment requirements
 *    -- Integers are encoded in little endian order, except signature and keys are
 *       encoded in big endian order.
 *    -- All pad bytes unless otherwise specificed are 0
 */

/* versioning */
#define BCM_FWSIGN_VERSION 0x0002u
typedef uint16 bcm_fwsign_version_t;

/* chip id */
typedef uint32 bcm_fwsign_chip_id_t;
typedef uint32 bcm_fwsign_chip_lot_t;
typedef uint32 bcm_fwsign_chip_wafer_t;
typedef uint32 bcm_fwsign_chip_x_t;
typedef uint32 bcm_fwsign_chip_y_t;

#if defined(DONGLEBUILD) || defined(BCMDONGLEHOST)
typedef uint32 bcm_fwsign_ptr_t;
#else
typedef uint8 *bcm_fwsign_ptr_t;
#endif /* DONGLEBUILD */

/* Memory layout
 * Addresses have to be uint32 instead of uint8* because the host and dongle
 * may have different pointer sizes.
 */
struct bcm_fwsign_mem_region {
	bcm_fwsign_ptr_t start;	/* start of region */
	bcm_fwsign_ptr_t end;	/* location after the region */
};
typedef struct bcm_fwsign_mem_region bcm_fwsign_mem_region_t;

#define BCM_FWSIGN_MEM_REGION_LEN(_r) ((_r)->end > (_r)->start ? \
	(_r)->end - (_r)->start : 0)

/* memory regions  - not all of these are included in signature */
struct bcm_fwsign_mem_info {
	bcm_fwsign_mem_region_t reset_vec;	/* read-only - jmp to BL */
	bcm_fwsign_mem_region_t int_vec;	/* copied from RAM, jmp here on success */
	/* ... */
	bcm_fwsign_mem_region_t rom;		/* Bootloader at ROM start */
	/* ... */
	/* start of RAM */
	bcm_fwsign_mem_region_t mmap;		/* struct/memory map written by host/dhd */
	bcm_fwsign_mem_region_t vstatus;	/* verification status */
	/* ... */
	bcm_fwsign_mem_region_t firmware;	/* firmware data and code */
	bcm_fwsign_mem_region_t signature;	/* signature and signing information */
	bcm_fwsign_mem_region_t heap;		/* region for heap allocations */
	/* ... */
	bcm_fwsign_mem_region_t stack;		/* stack allocations region */
	bcm_fwsign_mem_region_t prng;		/* PRNG data - may be 0 len */
	bcm_fwsign_mem_region_t nvram;		/* nvram data */
	/* end of RAM */
};
typedef struct bcm_fwsign_mem_info bcm_fwsign_mem_info_t;

/* key ids 0.. Max - 1, Correspond to OTP key not valid bits 0.. max - 1  */
enum bcm_fwsign_key_id {
	BCM_FWSIGN_KEY_ID_BCM_MIN	= 0,	/* Broadcom range start */
	BCM_FWSIGN_KEY_ID_BCM_MAX	= 7,	/* Broadcom range end */
	BCM_FWSIGN_KEY_ID_CUST_MIN	= 8,	/* Customer use */
	BCM_FWSIGN_KEY_ID_CUST_MAX	= 11,	/* Customer use */
	BCM_FWSIGN_KEY_ID_MAX		= 12	/* max supported */
};
typedef uint8 bcm_fwsign_key_id_t;

/* key 0 is reserved for development use */
#define BCM_FWSIGN_KEY_ID_DEV  0u
#define BCM_FWSIGN_KEY_ID_INVALID 0xffu

/* cipher suites */
enum bcm_fwsign_cs_id {
	BCM_FWSIGN_CS_ID_NONE			= 0,
	BCM_FWSIGN_CS_ID_ECP256_SHA256_DSA	= 1
};
typedef uint8 bcm_fwsign_cs_id_t;

/* key compression types */
enum bcm_fwsign_key_compress {
	BCM_FWSIGN_KEY_COMPRESS_ECG_YEVEN = 2,
	BCM_FWSIGN_KEY_COMPRESS_ECG_YODD  = 3,
	BCM_FWSIGN_KEY_COMPRESS_NONE      = 4,
	BCM_FWSIGN_KEY_COMPRESS_INVALID   = 0xff
};
typedef uint8 bcm_fwsign_key_compress_t;

/* key use */
enum bcm_fwsign_key_use {
	BCM_FWSIGN_KEY_USE_NONE		= 0,
	BCM_FWSIGN_KEY_USE_DEV		= 1,	/* BRCM development key */
	BCM_FWSIGN_KEY_USE_PROD		= 2,	/* Product key */
	BCM_FWSIGN_KEY_USE_CUST_DEV	= 3,	/* Customer development key */
	BCM_FWSIGN_KEY_USE_UQID		= 4,
	BCM_FWSIGN_KEY_USE_KEY_SIGN	= 5	/* key used to sign keys */
};
typedef uint8 bcm_fwsign_key_use_t;

/* signature options */
enum bcm_fwsign_options {
	BCM_FWSIGN_OPT_NONE		= 0x00000000,
	BCM_FWSIGN_OPT_ROM		= 0x00000001,
	BCM_FWSIGN_OPT_NVRAM		= 0x00000002,
	BCM_FWSIGN_OPT_PERSONALIZATION	= 0x00000004,
	BCM_FWSIGN_OPT_DEMOTION		= 0x00000008,
	/* additional options above */
	BCM_FWSIGN_OPT_ALL		= 0xffffffff
};
typedef uint32 bcm_fwsign_options_t;

#define BCM_FWSIGN_MANDATORY_OPTIONS (BCM_FWSIGN_OPT_FIRMWARE)

enum bcm_fwsign_tlv {
	BCM_FWSIGN_XTLV_NONE		= 0,
	BCM_FWSIGN_XTLV_CS_SIGNATURE	= 1,	/* Cipher suite dependent - must be last */
	BCM_FWSIGN_XTLV_KEY_INFO	= 2,
	BCM_FWSIGN_XTLV_KEY_INFOS	= 3,	/* array of KEY_INFO */
	BCM_FWSIGN_XTLV_SIGNING_PUB_KEY	= 4,	/* signing public key */
	BCM_FWSIGN_XTLV_FWENC_HDR	= 5,	/* firmware encryption info */
	/* FW_TAG which is GCM tag for AES-GCM encrypted firmware or sha256 for */
	/* AES-CBC and not encrypted firmware, resolved by FWENC_HDR xtlv */
	BCM_FWSIGN_XTLV_FW_TAG		= 6,
};
typedef bcm_xtlv_t bcm_fwsign_tlv_t;

struct bcm_fwsign_const_tlp {
	uint16 type;
	uint16 len;
	const uint8 *datap;
};
typedef struct bcm_fwsign_const_tlp bcm_fwsign_const_tlp_t;

/* tlv detail */

#define BCM_FWSIGN_ECP256_PRIME_LEN_BYTES 32u
#define BCM_FWSIGN_ECP256_ORDER_LEN_BYTES 32u

/* BCM_FWSIGN_XTLV_CSID_SIGNATURE - big endian */
struct bcm_fwsign_ecp256_sha256_dsa {
	uint8 r[BCM_FWSIGN_ECP256_ORDER_LEN_BYTES];
	uint8 s[BCM_FWSIGN_ECP256_ORDER_LEN_BYTES];
};
typedef struct bcm_fwsign_ecp256_sha256_dsa bcm_fwsign_ecp256_sha256_dsa_t;

typedef uint8 bcm_fwsign_ecg_type_t;

/* compressed ec public key - big endian */
struct bcm_fwsign_ec_pub_key {
	bcm_fwsign_ecg_type_t	ecg_type;	/* ECG type as defined in bcm_ec.h (NISP_256/384/etc) */
	bcm_fwsign_key_compress_t compress;	/* y is odd/even */
	uint8 x[];				/* x coord of the public key, size is determined by ecg */
};
typedef struct bcm_fwsign_ec_pub_key bcm_fwsign_ec_pub_key_t;

/* maximum key length - P384 etc. needs update. It is unlikely we'll
 * have a mix of key lengths as lower key length will weaken stronger
 * keys...
 */
#define BCM_FWSIGN_MAX_KEY_DATA_LEN sizeof(bcm_fwsign_ecp256_key_data_t)

/* BCM_FWSIGN_XTLV_KEY_INFO */
struct bcm_fwsign_key_info {
	bcm_fwsign_key_id_t	key_id;
	bcm_fwsign_key_use_t	key_use;
	bcm_fwsign_cs_id_t	cs_id;
	bcm_fwsign_key_compress_t compress;
	uint16			pad[2];
	uint16			key_data_len;	/* e.g. public key - x.y or just x */
	const uint8		*key_data;	/* [1..BCM_FWSIGN_MAX_KEY_DATA_LEN] */
};
typedef struct bcm_fwsign_key_info bcm_fwsign_key_info_t;

/* end tlv detail */

/*
 * Provide and specify information for uniquely identifying any individual chip
 */
struct bcm_fwsign_chip_info {
	bcm_fwsign_chip_id_t	id;	/* id # of all chips in this tape-out */
	bcm_fwsign_chip_lot_t	lot;	/* lot number in which this item was fabbed */
	bcm_fwsign_chip_wafer_t	wafer;	/* wafer number within the lot */
	bcm_fwsign_chip_x_t	x;	/* X coordinate of this chip within the wafer */
	bcm_fwsign_chip_y_t	y;	/* Y coordinate of this chip within the wafer */
};
typedef struct bcm_fwsign_chip_info bcm_fwsign_chip_info_t;

/* TODO bcm_fwenc_algo_t should be moved to firmware encryption headers */
enum bcm_fwenc_algo {
	BCM_FWENC_ALGO_NONE_SHA_256	= 0,	/* no encryption, fw tag is sha256 */
	BCM_FWENC_ALGO_GCM_256		= 1,	/* AES-GCM encryption, fw tag is GCM tag */
	BCM_FWENC_ALGO_CBC_256_SHA_256	= 2,	/* AES-CBC encryption, fw tag is sha256 */
};
typedef uint8 bcm_fwenc_algo_t;

/* TODO bcm_fwenc_hdr should be moved to firmware encryption headers */
struct bcm_fwenc_hdr {
	uint16			version;	/* Firmware encryption header version */
	uint16			len;		/* lenght of the header */
	bcm_fwenc_algo_t	algo;		/* firmware encryption algorithm */
	uint8			pad[3];
};
typedef struct bcm_fwenc_hdr bcm_fwenc_hdr_t;

struct bcm_fwsign_signature_info {
	bcm_fwsign_cs_id_t	cs_id;		/* signature cipher id */
	bcm_fwsign_key_id_t	key_id;		/* public key id */
	bcm_fwsign_key_use_t	key_use;	/* key use (dev, customer, etc) */
	uint8			pad[1];
};
typedef struct bcm_fwsign_signature_info bcm_fwsign_signature_info_t;

/* package which contains signature and metadata, */
/* signed from the beggining until signature xltv */
struct bcm_fwsign_package {
	bcm_fwsign_version_t		version;
	uint16				length;			/* length of package that follows length field */
	uint16				tlvs_off;		/* offset to xtlv from the beginning of the package */
	uint8				pad[2];
	bcm_fwsign_options_t		options;		/* signing options (personalization, demotion, etc.) */
	bcm_fwsign_chip_info_t		chip_info;		/* chip and wafer info */
	bcm_fwsign_signature_info_t	signature_info;		/* signature and key info */
	/* pubkey, fwtag, demotion, signature, etc. xtlvs */
	/* note: signature xtlv must be last */
	bcm_fwsign_tlv_t		tlvs[];
};
typedef struct bcm_fwsign_package bcm_fwsign_package_t;

#define BCM_FWSIGN_PACKAGE_HDR_LEN	OFFSETOF(bcm_fwsign_package_t, tlvs_off)

/* signature algorithm: see documentation */

/* Data in ROM including keys */
struct bcm_fwsign_rom_info {
	bcm_fwsign_version_t	version;
	uint16			length;
	bcm_fwsign_chip_id_t	chip_id;
	uint16			tlvp_off;	/* offset to tlps from start of version */
	uint8			pad[2];
	bcm_fwsign_const_tlp_t	*tlps;
};
typedef struct bcm_fwsign_rom_info bcm_fwsign_rom_info_t;

/* Bootloader states of operation - indicating progress */
enum bcm_fwsign_state {
	BCM_FWSIGN_STATE_START			= 1,	/* stack setup prior to start */
	BCM_FWSIGN_STATE_MAP_VALIDATE		= 2,	/* signature package validation */
	BCM_FWSIGN_STATE_SETUP_HEAP		= 3,	/* initialize heap */
	BCM_FWSIGN_STATE_OTP_READ		= 4,	/* read keys from OTP */
	BCM_FWSIGN_STATE_KEY_LOOKUP		= 5,	/* keys validation */
	BCM_FWSIGN_STATE_VERIFY_SIGNATURE	= 6,	/* signature verification */
	BCM_FWSIGN_STATE_VALIDATE_FIRMWARE	= 7,	/* firmware validation */
	BCM_FWSIGN_STATE_END			= 8	/* verification complete */
};
typedef uint16 bcm_fwsign_state_t;

typedef uint32 bcm_fwsign_counter_t;

/* verification status - populated on return  */
struct bcm_fwsign_verif_status {
	bcm_fwsign_status_t	status;
	bcm_fwsign_state_t	state;
	uint16			alloc_fail_size;	/* Size of the failled alloc request */
	bcm_fwsign_counter_t	alloc_bytes;		/* currently allocated */
	bcm_fwsign_counter_t	max_alloc_bytes;	/* max ever  allocated */
	bcm_fwsign_counter_t	total_alloc_bytes;	/* running count of allocated */
	bcm_fwsign_counter_t	total_freed_bytes;	/* running count of freed */
	bcm_fwsign_counter_t	num_allocs;		/* number of allocations */
	bcm_fwsign_counter_t	max_allocs;		/* max number of allocations */
	bcm_fwsign_counter_t	max_alloc_size;		/* max allocation size */
	bcm_fwsign_counter_t	alloc_failures;		/* allocation failure count */
	/* add other information as necessary */
};
typedef struct bcm_fwsign_verif_status bcm_fwsign_verif_status_t;

/* boot - does not return */
void bcm_sfw_boot(void *si, bcm_fwsign_mem_info_t *mi);

void* bcm_sfw_allocz(uint size);
void bcm_sfw_free(void *p);

#endif /* _BCM_FWSIGN_H_ */
