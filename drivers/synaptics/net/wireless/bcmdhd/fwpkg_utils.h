/*
 * Firmware package defines
 *
 * $ Copyright Open Broadcom $
 *
 *
 * <<Broadcom-WL-IPTag/Dual:>>
 */

#ifndef _fwpkg_utils_h_
#define _fwpkg_utils_h_

enum {
	FWPKG_TAG_FW	= 1,
	FWPKG_TAG_SIG	= 2,
	FWPKG_TAG_INFO	= 3,
	FWPKG_TAG_LAST
};
#define NBR_OF_FWPKG_UNITS	(FWPKG_TAG_LAST-1)

/* internal firmware package header */
typedef struct fwpkg_hdr {
	uint32 options;
	uint32 version;		/* fw package version */
	uint32 length;		/* length of data after the length field */
	uint32 magic_word0;	/* hardcoded value */
	uint32 magic_word1;	/* hardcoded value */
} fwpkg_hdr_t;

/* internal firmware package unit header */
typedef struct fwpkg_unit
{
	uint32	offset;		/* offset to the data in the file */
	uint32	size;		/* the data size */
	uint32  type;
} fwpkg_unit_t;

#define FWPKG_SINGLE_FLG	1U
#define FWPKG_COMBND_FLG	2U

typedef struct fwpkg_info
{
	fwpkg_unit_t units[NBR_OF_FWPKG_UNITS];
	uint32 file_size;
	uint8 status;
	uint8 pad[3];
} fwpkg_info_t;

#define IS_FWPKG_SINGLE(fwpkg)	\
	((fwpkg->status == FWPKG_SINGLE_FLG) ? TRUE : FALSE)
#define IS_FWPKG_COMBND(fwpkg)	\
	((fwpkg->status == FWPKG_COMBND_FLG) ? TRUE : FALSE)

#ifdef BCMDRIVER
#define FWPKG_FILE	void
#else
#define FWPKG_FILE	FILE
#endif /* BCMDRIVER */

int fwpkg_init(fwpkg_info_t *fwpkg, char *fname);
int fwpkg_open_firmware_img(fwpkg_info_t *fwpkg, char *fname, FWPKG_FILE **fp);
int fwpkg_open_signature_img(fwpkg_info_t *fwpkg, char *fname, FWPKG_FILE **fp);
void fwpkg_deinit(fwpkg_info_t *fwpkg);
uint32 fwpkg_get_firmware_img_size(fwpkg_info_t *fwpkg);
uint32 fwpkg_get_signature_img_size(fwpkg_info_t *fwpkg);
uint32 fwpkg_get_unit_offset(fwpkg_info_t *fwpkg, uint32 unit_type);

#endif /* _fwpkg_utils_h_ */
