/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Hantro VPU codec driver
 *
 * Copyright 2018 Google LLC.
 *	Tomasz Figa <tfiga@chromium.org>
 *
 * A simple version from the kernel staging tree
 */

#ifndef HANTRO_H_
#define HANTRO_H_

#include <linux/videodev2.h>

#define H1_MIN_W		(144U)
#define H1_MIN_H		(96U)
#define H1_MAX_W		(1920U)
#define H1_MAX_H		(1088U)
#define MB_DIM			16
#define MB_WIDTH(w)		DIV_ROUND_UP(w, MB_DIM)
#define MB_HEIGHT(h)		DIV_ROUND_UP(h, MB_DIM)

/**
 * enum hantro_codec_mode - codec operating mode.
 * @HANTRO_MODE_NONE:  No operating mode. Used for RAW video formats.
 * @HANTRO_MODE_JPEG_ENC: JPEG encoder.
 * @HANTRO_MODE_H264_DEC: H264 decoder.
 * @HANTRO_MODE_MPEG2_DEC: MPEG-2 decoder.
 * @HANTRO_MODE_VP8_DEC: VP8 decoder.
 * @HANTRO_MODE_HEVC_DEC: HEVC decoder.
 * @HANTRO_MODE_VP9_DEC: VP9 decoder.
 */
enum hantro_codec_mode {
	HANTRO_MODE_NONE = -1,
	HANTRO_MODE_JPEG_ENC,
	HANTRO_MODE_H264_DEC,
	HANTRO_MODE_MPEG2_DEC,
	HANTRO_MODE_VP8_DEC,
	HANTRO_MODE_HEVC_DEC,
	HANTRO_MODE_VP9_DEC,
	HANTRO_MODE_H264_ENC,
	HANTRO_MODE_VP8_ENC,
};

/**
 * struct hantro_fmt - information about supported video formats.
 * @name:	Human readable name of the format.
 * @fourcc:	FourCC code of the format. See V4L2_PIX_FMT_*.
 * @codec_mode:	Codec mode related to this format. See
 *		enum hantro_codec_mode.
 * @header_size: Optional header size. Currently used by JPEG encoder.
 * @max_depth:	Maximum depth, for bitstream formats
 * @enc_fmt:	Format identifier for encoder registers.
 * @frmsize:	Supported range of frame sizes (only for bitstream formats).
 * @postprocessed: Indicates if this format needs the post-processor.
 */
struct hantro_fmt {
	char *name;
	u32 fourcc;
	enum hantro_codec_mode codec_mode;
	int header_size;
	int max_depth;
	u32 enc_fmt;
	struct v4l2_frmsize_stepwise frmsize;
	bool postprocessed;
	u32 version;
};

#endif
