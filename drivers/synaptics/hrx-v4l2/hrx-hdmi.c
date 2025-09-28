// SPDX-License-Identifier: GPL-2.0
/* Copyright 2017 Synopsys Inc. */
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-hdmi.h"
#include "hrx-reg.h"

static unsigned char hdmi_infoframe_checksum(unsigned char *ptr, size_t size)
{
	unsigned char csum = 0;
	size_t i;

	/* compute checksum */
	for (i = 0; i < size; i++)
		csum += ptr[i];

	return (unsigned char)(256 - csum);
}

static const char *hdmi_infoframe_type_get_name(enum hdmi_infoframe_type type)
{
	if (type < 0x80 || type > 0x9f)
		return "Invalid";
	switch (type) {
	case HDMI_INFOFRAME_TYPE_VENDOR:
		return "Vendor";
	case HDMI_INFOFRAME_TYPE_AVI:
		return "Auxiliary Video Information (AVI)";
	case HDMI_INFOFRAME_TYPE_SPD:
		return "Source Product Description (SPD)";
	case HDMI_INFOFRAME_TYPE_AUDIO:
		return "Audio";
	case HDMI_INFOFRAME_TYPE_HDR:
		return "Dynamic Range and Mastering";
	}
	return "Reserved";
}

static const char *hdmi_colorspace_get_name(enum hdmi_colorspace colorspace)
{
	switch (colorspace) {
	case HDMI_COLORSPACE_RGB:
		return "RGB";
	case HDMI_COLORSPACE_YUV422:
		return "YCbCr 4:2:2";
	case HDMI_COLORSPACE_YUV444:
		return "YCbCr 4:4:4";
	case HDMI_COLORSPACE_YUV420:
		return "YCbCr 4:2:0";
	case HDMI_COLORSPACE_RESERVED4:
		return "Reserved (4)";
	case HDMI_COLORSPACE_RESERVED5:
		return "Reserved (5)";
	case HDMI_COLORSPACE_RESERVED6:
		return "Reserved (6)";
	case HDMI_COLORSPACE_IDO_DEFINED:
		return "IDO Defined";
	}
	return "Invalid";
}

static const char *hdmi_scan_mode_get_name(enum hdmi_scan_mode scan_mode)
{
	switch (scan_mode) {
	case HDMI_SCAN_MODE_NONE:
		return "No Data";
	case HDMI_SCAN_MODE_OVERSCAN:
		return "Overscan";
	case HDMI_SCAN_MODE_UNDERSCAN:
		return "Underscan";
	case HDMI_SCAN_MODE_RESERVED:
		return "Reserved";
	}
	return "Invalid";
}

static const char *hdmi_colorimetry_get_name(enum hdmi_colorimetry colorimetry)
{
	switch (colorimetry) {
	case HDMI_COLORIMETRY_NONE:
		return "No Data";
	case HDMI_COLORIMETRY_ITU_601:
		return "ITU601";
	case HDMI_COLORIMETRY_ITU_709:
		return "ITU709";
	case HDMI_COLORIMETRY_EXTENDED:
		return "Extended";
	}
	return "Invalid";
}

static const char *
hdmi_picture_aspect_get_name(enum hdmi_picture_aspect picture_aspect)
{
	switch (picture_aspect) {
	case HDMI_PICTURE_ASPECT_NONE:
		return "No Data";
	case HDMI_PICTURE_ASPECT_4_3:
		return "4:3";
	case HDMI_PICTURE_ASPECT_16_9:
		return "16:9";
	case HDMI_PICTURE_ASPECT_64_27:
		return "64:27";
	case HDMI_PICTURE_ASPECT_256_135:
		return "256:135";
	case HDMI_PICTURE_ASPECT_RESERVED:
		return "Reserved";
	}
	return "Invalid";
}

static const char *
hdmi_active_aspect_get_name(enum hdmi_active_aspect active_aspect)
{
	if (active_aspect < 0 || active_aspect > 0xf)
		return "Invalid";

	switch (active_aspect) {
	case HDMI_ACTIVE_ASPECT_16_9_TOP:
		return "16:9 Top";
	case HDMI_ACTIVE_ASPECT_14_9_TOP:
		return "14:9 Top";
	case HDMI_ACTIVE_ASPECT_16_9_CENTER:
		return "16:9 Center";
	case HDMI_ACTIVE_ASPECT_PICTURE:
		return "Same as Picture";
	case HDMI_ACTIVE_ASPECT_4_3:
		return "4:3";
	case HDMI_ACTIVE_ASPECT_16_9:
		return "16:9";
	case HDMI_ACTIVE_ASPECT_14_9:
		return "14:9";
	case HDMI_ACTIVE_ASPECT_4_3_SP_14_9:
		return "4:3 SP 14:9";
	case HDMI_ACTIVE_ASPECT_16_9_SP_14_9:
		return "16:9 SP 14:9";
	case HDMI_ACTIVE_ASPECT_16_9_SP_4_3:
		return "16:9 SP 4:3";
	}
	return "Reserved";
}

static const char *
hdmi_extended_colorimetry_get_name(enum hdmi_extended_colorimetry ext_col)
{
	switch (ext_col) {
	case HDMI_EXTENDED_COLORIMETRY_XV_YCC_601:
		return "xvYCC 601";
	case HDMI_EXTENDED_COLORIMETRY_XV_YCC_709:
		return "xvYCC 709";
	case HDMI_EXTENDED_COLORIMETRY_S_YCC_601:
		return "sYCC 601";
	case HDMI_EXTENDED_COLORIMETRY_ADOBE_YCC_601:
		return "Adobe YCC 601";
	case HDMI_EXTENDED_COLORIMETRY_ADOBE_RGB:
		return "Adobe RGB";
	case HDMI_EXTENDED_COLORIMETRY_BT2020_CONST_LUM:
		return "BT.2020 Constant Luminance";
	case HDMI_EXTENDED_COLORIMETRY_BT2020:
		return "BT.2020";
	case HDMI_EXTENDED_COLORIMETRY_RESERVED:
		return "Reserved";
	}
	return "Invalid";
}

static const char *
hdmi_quantization_range_get_name(enum hdmi_quantization_range qrange)
{
	switch (qrange) {
	case HDMI_QUANTIZATION_RANGE_DEFAULT:
		return "Default";
	case HDMI_QUANTIZATION_RANGE_LIMITED:
		return "Limited";
	case HDMI_QUANTIZATION_RANGE_FULL:
		return "Full";
	case HDMI_QUANTIZATION_RANGE_RESERVED:
		return "Reserved";
	}
	return "Invalid";
}

static const char *hdmi_nups_get_name(enum hdmi_nups nups)
{
	switch (nups) {
	case HDMI_NUPS_UNKNOWN:
		return "Unknown Non-uniform Scaling";
	case HDMI_NUPS_HORIZONTAL:
		return "Horizontally Scaled";
	case HDMI_NUPS_VERTICAL:
		return "Vertically Scaled";
	case HDMI_NUPS_BOTH:
		return "Horizontally and Vertically Scaled";
	}
	return "Invalid";
}

static const char *
hdmi_ycc_quantization_range_get_name(enum hdmi_ycc_quantization_range qrange)
{
	switch (qrange) {
	case HDMI_YCC_QUANTIZATION_RANGE_LIMITED:
		return "Limited";
	case HDMI_YCC_QUANTIZATION_RANGE_FULL:
		return "Full";
	}
	return "Invalid";
}

static const char *
hdmi_content_type_get_name(enum hdmi_content_type content_type)
{
	switch (content_type) {
	case HDMI_CONTENT_TYPE_GRAPHICS:
		return "Graphics";
	case HDMI_CONTENT_TYPE_PHOTO:
		return "Photo";
	case HDMI_CONTENT_TYPE_CINEMA:
		return "Cinema";
	case HDMI_CONTENT_TYPE_GAME:
		return "Game";
	}
	return "Invalid";
}

static void hdmi_infoframe_log_header(struct hdmi_any_infoframe *frame)
{
	HRX_LOG(HRX_DRV_DEBUG, "HDMI infoframe: %s, version %u, length %u\n\n",
		hdmi_infoframe_type_get_name(frame->type),
		frame->version, frame->length);
}

static void hdmi_avi_infoframe_log(struct hdmi_avi_infoframe *frame)
{
	HRX_LOG(HRX_DRV_DEBUG, "Colorspace: %s\n", hdmi_colorspace_get_name(frame->colorspace));
	HRX_LOG(HRX_DRV_DEBUG, "Scan Mode: %s\n", hdmi_scan_mode_get_name(frame->scan_mode));
	HRX_LOG(HRX_DRV_DEBUG, "Colorimetry: %s\n", hdmi_colorimetry_get_name(frame->colorimetry));
	HRX_LOG(HRX_DRV_DEBUG, "Picture Aspect: %s\n", hdmi_picture_aspect_get_name(frame->picture_aspect));
	HRX_LOG(HRX_DRV_DEBUG, "Active Aspect: %s\n", hdmi_active_aspect_get_name(frame->active_aspect));
	HRX_LOG(HRX_DRV_DEBUG, "ITC: %s\n", frame->itc ? "IT Content" : "No Data");
	HRX_LOG(HRX_DRV_DEBUG, "Extended Colorimetry: %s\n", hdmi_extended_colorimetry_get_name(frame->extended_colorimetry));
	HRX_LOG(HRX_DRV_DEBUG, "Quantization Range: %s\n", hdmi_quantization_range_get_name(frame->quantization_range));
	HRX_LOG(HRX_DRV_DEBUG, "NUPS: %s\n", hdmi_nups_get_name(frame->nups));
	HRX_LOG(HRX_DRV_DEBUG, "Video Code: %u\n", frame->video_code);
	HRX_LOG(HRX_DRV_DEBUG, "YCC Quantization Range: %s\n", hdmi_ycc_quantization_range_get_name(frame->ycc_quantization_range));
	HRX_LOG(HRX_DRV_DEBUG, "HDMI Content Type: %s\n", hdmi_content_type_get_name(frame->content_type));
	HRX_LOG(HRX_DRV_DEBUG, "Pixel Repeat: %u\n", frame->pixel_repeat);
	HRX_LOG(HRX_DRV_DEBUG, "Bar: Top %u, Bottom %u, Left %u, Right %u\n",
			frame->top_bar, frame->bottom_bar,
			frame->left_bar, frame->right_bar);
}

static const char *hdmi_spd_sdi_get_name(enum hdmi_spd_sdi sdi)
{
	if (sdi < 0 || sdi > 0xff)
		return "Invalid";
	switch (sdi) {
	case HDMI_SPD_SDI_UNKNOWN:
		return "Unknown";
	case HDMI_SPD_SDI_DSTB:
		return "Digital STB";
	case HDMI_SPD_SDI_DVDP:
		return "DVD Player";
	case HDMI_SPD_SDI_DVHS:
		return "D-VHS";
	case HDMI_SPD_SDI_HDDVR:
		return "HDD Videorecorder";
	case HDMI_SPD_SDI_DVC:
		return "DVC";
	case HDMI_SPD_SDI_DSC:
		return "DSC";
	case HDMI_SPD_SDI_VCD:
		return "Video CD";
	case HDMI_SPD_SDI_GAME:
		return "Game";
	case HDMI_SPD_SDI_PC:
		return "PC General";
	case HDMI_SPD_SDI_BD:
		return "Blu-Ray Disc (BD)";
	case HDMI_SPD_SDI_SACD:
		return "Super Audio CD";
	case HDMI_SPD_SDI_HDDVD:
		return "HD DVD";
	case HDMI_SPD_SDI_PMP:
		return "PMP";
	}
	return "Reserved";
}

static void hdmi_spd_infoframe_log(struct hdmi_spd_infoframe *frame)
{
	char buf[17];

	memset(buf, 0, sizeof(buf));

	strncpy(buf, frame->vendor, 8);
	HRX_LOG(HRX_DRV_DEBUG, "Vendor: %s\n", buf);
	strncpy(buf, frame->product, 16);
	HRX_LOG(HRX_DRV_DEBUG, "Product: %s\n", buf);
	HRX_LOG(HRX_DRV_DEBUG, "Source Device Information: %s (0x%x)\n",
		hdmi_spd_sdi_get_name(frame->sdi), frame->sdi);
}

static const char *
hdmi_audio_coding_type_get_name(enum hdmi_audio_coding_type coding_type)
{
	switch (coding_type) {
	case HDMI_AUDIO_CODING_TYPE_STREAM:
		return "Refer to Stream Header";
	case HDMI_AUDIO_CODING_TYPE_PCM:
		return "PCM";
	case HDMI_AUDIO_CODING_TYPE_AC3:
		return "AC-3";
	case HDMI_AUDIO_CODING_TYPE_MPEG1:
		return "MPEG1";
	case HDMI_AUDIO_CODING_TYPE_MP3:
		return "MP3";
	case HDMI_AUDIO_CODING_TYPE_MPEG2:
		return "MPEG2";
	case HDMI_AUDIO_CODING_TYPE_AAC_LC:
		return "AAC";
	case HDMI_AUDIO_CODING_TYPE_DTS:
		return "DTS";
	case HDMI_AUDIO_CODING_TYPE_ATRAC:
		return "ATRAC";
	case HDMI_AUDIO_CODING_TYPE_DSD:
		return "One Bit Audio";
	case HDMI_AUDIO_CODING_TYPE_EAC3:
		return "Dolby Digital +";
	case HDMI_AUDIO_CODING_TYPE_DTS_HD:
		return "DTS-HD";
	case HDMI_AUDIO_CODING_TYPE_MLP:
		return "MAT (MLP)";
	case HDMI_AUDIO_CODING_TYPE_DST:
		return "DST";
	case HDMI_AUDIO_CODING_TYPE_WMA_PRO:
		return "WMA PRO";
	case HDMI_AUDIO_CODING_TYPE_CXT:
		return "Refer to CXT";
	}
	return "Invalid";
}

static const char *
hdmi_audio_sample_size_get_name(enum hdmi_audio_sample_size sample_size)
{
	switch (sample_size) {
	case HDMI_AUDIO_SAMPLE_SIZE_STREAM:
		return "Refer to Stream Header";
	case HDMI_AUDIO_SAMPLE_SIZE_16:
		return "16 bit";
	case HDMI_AUDIO_SAMPLE_SIZE_20:
		return "20 bit";
	case HDMI_AUDIO_SAMPLE_SIZE_24:
		return "24 bit";
	}
	return "Invalid";
}

static const char *
hdmi_audio_sample_frequency_get_name(enum hdmi_audio_sample_frequency freq)
{
	switch (freq) {
	case HDMI_AUDIO_SAMPLE_FREQUENCY_STREAM:
		return "Refer to Stream Header";
	case HDMI_AUDIO_SAMPLE_FREQUENCY_32000:
		return "32 kHz";
	case HDMI_AUDIO_SAMPLE_FREQUENCY_44100:
		return "44.1 kHz (CD)";
	case HDMI_AUDIO_SAMPLE_FREQUENCY_48000:
		return "48 kHz";
	case HDMI_AUDIO_SAMPLE_FREQUENCY_88200:
		return "88.2 kHz";
	case HDMI_AUDIO_SAMPLE_FREQUENCY_96000:
		return "96 kHz";
	case HDMI_AUDIO_SAMPLE_FREQUENCY_176400:
		return "176.4 kHz";
	case HDMI_AUDIO_SAMPLE_FREQUENCY_192000:
		return "192 kHz";
	}
	return "Invalid";
}

static const char *
hdmi_audio_coding_type_ext_get_name(enum hdmi_audio_coding_type_ext ctx)
{
	if (ctx < 0 || ctx > 0x1f)
		return "Invalid";

	switch (ctx) {
	case HDMI_AUDIO_CODING_TYPE_EXT_CT:
		return "Refer to CT";
	case HDMI_AUDIO_CODING_TYPE_EXT_HE_AAC:
		return "HE AAC";
	case HDMI_AUDIO_CODING_TYPE_EXT_HE_AAC_V2:
		return "HE AAC v2";
	case HDMI_AUDIO_CODING_TYPE_EXT_MPEG_SURROUND:
		return "MPEG SURROUND";
	case HDMI_AUDIO_CODING_TYPE_EXT_MPEG4_HE_AAC:
		return "MPEG-4 HE AAC";
	case HDMI_AUDIO_CODING_TYPE_EXT_MPEG4_HE_AAC_V2:
		return "MPEG-4 HE AAC v2";
	case HDMI_AUDIO_CODING_TYPE_EXT_MPEG4_AAC_LC:
		return "MPEG-4 AAC LC";
	case HDMI_AUDIO_CODING_TYPE_EXT_DRA:
		return "DRA";
	case HDMI_AUDIO_CODING_TYPE_EXT_MPEG4_HE_AAC_SURROUND:
		return "MPEG-4 HE AAC + MPEG Surround";
	case HDMI_AUDIO_CODING_TYPE_EXT_MPEG4_AAC_LC_SURROUND:
		return "MPEG-4 AAC LC + MPEG Surround";
	}
	return "Reserved";
}

static void hdmi_audio_infoframe_log(struct hdmi_audio_infoframe *frame)
{
	if (frame->channels)
		HRX_LOG(HRX_DRV_DEBUG, "Channels: %u\n", frame->channels - 1);
	else
		HRX_LOG(HRX_DRV_DEBUG, "Channels: Refer to stream header\n");
	HRX_LOG(HRX_DRV_DEBUG, "Coding Type: %s\n", hdmi_audio_coding_type_get_name(frame->coding_type));
	HRX_LOG(HRX_DRV_DEBUG, "Sample Size: %s\n", hdmi_audio_sample_size_get_name(frame->sample_size));
	HRX_LOG(HRX_DRV_DEBUG, "Sample Frequency: %s\n", hdmi_audio_sample_frequency_get_name(frame->sample_frequency));
	HRX_LOG(HRX_DRV_DEBUG, "Coding Type Ext: %s\n", hdmi_audio_coding_type_ext_get_name(frame->coding_type_ext));
	HRX_LOG(HRX_DRV_DEBUG, "Channel Allocation: 0x%x\n", frame->channel_allocation);
	HRX_LOG(HRX_DRV_DEBUG, "Level Shift Value: %u dB\n", frame->level_shift_value);
	HRX_LOG(HRX_DRV_DEBUG, "Downmix Inhibit: %s\n", frame->downmix_inhibit ? "Yes" : "No");
}

static const char *
hdmi_3d_structure_get_name(enum hdmi_3d_structure s3d_struct)
{
	if (s3d_struct < 0 || s3d_struct > 0xf)
		return "Invalid";

	switch (s3d_struct) {
	case HDMI_3D_STRUCTURE_FRAME_PACKING:
		return "Frame Packing";
	case HDMI_3D_STRUCTURE_FIELD_ALTERNATIVE:
		return "Field Alternative";
	case HDMI_3D_STRUCTURE_LINE_ALTERNATIVE:
		return "Line Alternative";
	case HDMI_3D_STRUCTURE_SIDE_BY_SIDE_FULL:
		return "Side-by-side (Full)";
	case HDMI_3D_STRUCTURE_L_DEPTH:
		return "L + Depth";
	case HDMI_3D_STRUCTURE_L_DEPTH_GFX_GFX_DEPTH:
		return "L + Depth + Graphics + Graphics-depth";
	case HDMI_3D_STRUCTURE_TOP_AND_BOTTOM:
		return "Top-and-Bottom";
	case HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF:
		return "Side-by-side (Half)";
	default:
		break;
	}
	return "Reserved";
}

static void
hdmi_vendor_any_infoframe_log(union hdmi_vendor_any_infoframe *frame)
{
	struct hdmi_vendor_infoframe *hvf = &frame->hdmi;

	if (frame->any.oui != HDMI_IEEE_OUI) {
		HRX_LOG(HRX_DRV_DEBUG, "Not a HDMI vendor infoframe oui= 0x%x\n", frame->any.oui);
		return;
	}
	if (hvf->vic == 0 && hvf->s3d_struct == HDMI_3D_STRUCTURE_INVALID) {
		HRX_LOG(HRX_DRV_DEBUG, "Empty Frame oui= 0x%x\n", frame->any.oui);
		return;
	}

	if (hvf->vic)
		HRX_LOG(HRX_DRV_DEBUG, "HDMI VIC: %u\n", hvf->vic);
	if (hvf->s3d_struct != HDMI_3D_STRUCTURE_INVALID) {
		HRX_LOG(HRX_DRV_DEBUG, "3D Structure: %s\n", hdmi_3d_structure_get_name(hvf->s3d_struct));
		if (hvf->s3d_struct >= HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF)
			HRX_LOG(HRX_DRV_DEBUG, "3D Extension Data: %d\n", hvf->s3d_ext_data);
	}
}

static const char *hdmi_hdr_eotf_get_name(enum hdmi_hdr_eotf eotf)
{
	switch (eotf) {
	case HDMI_HDR_EOTF_SDR:
		return "Traditional gamma - SDR";
	case HDMI_HDR_EOTF_HDR:
		return "Traditional gamma - HDR";
	case HDMI_HDR_EOTF_SMPTE:
		return "SMPTE ST 2084";
	case HDMI_HDR_EOTF_HLG:
		return "Hybrid Log-Gamma";
	default:
		break;
	}
	return "Invalid";
}

static const char *
hdmi_hdr_metadata_type_get_name(enum hdmi_hdr_metadata_type type)
{
	switch (type) {
	case HDMI_HDR_METADATA_TYPE1:
		return "Type 1";
	default:
		break;
	}
	return "Invalid";
}

static void hdmi_hdr_infoframe_log(struct hdmi_hdr_infoframe *frame)
{
	int i;

	HRX_LOG(HRX_DRV_DEBUG, "EOTF: %s\n", hdmi_hdr_eotf_get_name(frame->eotf));
	HRX_LOG(HRX_DRV_DEBUG, "Static Metadata Descriptor ID: %s\n",
			hdmi_hdr_metadata_type_get_name(frame->metadata_type));

	HRX_LOG(HRX_DRV_DEBUG, "Display Primaries:\n");
	for (i = 0; i < 3; i++) {
		HRX_LOG(HRX_DRV_DEBUG, "\tx[%d]: %u\n", i, frame->display_prim_x[i]);
		HRX_LOG(HRX_DRV_DEBUG, "\ty[%d]: %u\n", i, frame->display_prim_y[i]);
	}

	HRX_LOG(HRX_DRV_DEBUG, "White Point: x %u, y %u\n",
			frame->white_point_x, frame->white_point_y);
	HRX_LOG(HRX_DRV_DEBUG, "Max Display Mastering Luminance: %u\n", frame->max_dml);
	HRX_LOG(HRX_DRV_DEBUG, "Min Display Mastering Luminance: %u\n", frame->min_dml);
	HRX_LOG(HRX_DRV_DEBUG, "Maximum Content Light Level: %u\n", frame->max_cll);
	HRX_LOG(HRX_DRV_DEBUG, "Maximum Frame-Average Light Level: %u\n", frame->max_fll);
}

static void hdmi_infoframe_log(union hdmi_infoframe *frame)
{
	hdmi_infoframe_log_header((struct hdmi_any_infoframe *)frame);
	switch (frame->any.type) {
	case HDMI_INFOFRAME_TYPE_AVI:
		hdmi_avi_infoframe_log(&frame->avi);
		break;
	case HDMI_INFOFRAME_TYPE_SPD:
		hdmi_spd_infoframe_log(&frame->spd);
		break;
	case HDMI_INFOFRAME_TYPE_AUDIO:
		hdmi_audio_infoframe_log(&frame->audio);
		break;
	case HDMI_INFOFRAME_TYPE_VENDOR:
		hdmi_vendor_any_infoframe_log(&frame->vendor);
		break;
	case HDMI_INFOFRAME_TYPE_HDR:
		hdmi_hdr_infoframe_log(&frame->hdr);
		break;
	}
}

static void hdmi_avi_infoframe_init(struct hdmi_avi_infoframe *frame)
{
	memset(frame, 0, sizeof(*frame));

	frame->type = HDMI_INFOFRAME_TYPE_AVI;
	frame->version = 2;
	frame->length = HDMI_AVI_INFOFRAME_SIZE;
}

/**
 * hdmi_avi_infoframe_unpack() - unpack binary buffer to a HDMI AVI infoframe
 * @buffer: source buffer
 * @frame: HDMI AVI infoframe
 *
 * Unpacks the information contained in binary @buffer into a structured
 * @frame of the HDMI Auxiliary Video (AVI) information frame.
 * Also verifies the checksum as required by section 5.3.5 of the HDMI 1.4
 * specification.
 *
 * Returns 0 on success or a negative error code on failure.
 */
static int hdmi_avi_infoframe_unpack(struct hdmi_avi_infoframe *frame,
					 void *buffer)
{
	unsigned char *ptr = buffer;

	if (hdmi_infoframe_checksum(buffer, HDMI_INFOFRAME_SIZE(AVI)) != 0)
		return -1;

	hdmi_avi_infoframe_init(frame);

	ptr += HDMI_INFOFRAME_HEADER_SIZE;

	frame->colorspace = (ptr[0] >> 5) & 0x3;
	if (ptr[0] & 0x10)
		frame->active_aspect = ptr[1] & 0xf;
	if (ptr[0] & 0x8) {
		frame->top_bar = (ptr[5] << 8) + ptr[6];
		frame->bottom_bar = (ptr[7] << 8) + ptr[8];
	}
	if (ptr[0] & 0x4) {
		frame->left_bar = (ptr[9] << 8) + ptr[10];
		frame->right_bar = (ptr[11] << 8) + ptr[12];
	}
	frame->scan_mode = ptr[0] & 0x3;

	frame->colorimetry = (ptr[1] >> 6) & 0x3;
	frame->picture_aspect = (ptr[1] >> 4) & 0x3;
	frame->active_aspect = ptr[1] & 0xf;

	frame->itc = ptr[2] & 0x80 ? 1 : 0;
	frame->extended_colorimetry = (ptr[2] >> 4) & 0x7;
	frame->quantization_range = (ptr[2] >> 2) & 0x3;
	frame->nups = ptr[2] & 0x3;

	frame->video_code = ptr[3] & 0x7f;
	frame->ycc_quantization_range = (ptr[4] >> 6) & 0x3;
	frame->content_type = (ptr[4] >> 4) & 0x3;

	frame->pixel_repeat = ptr[4] & 0xf;

	return 0;
}

static void hdmi_spd_infoframe_init(struct hdmi_spd_infoframe *frame,
		const char *vendor, const char *product)
{
	memset(frame, 0, sizeof(*frame));

	frame->type = HDMI_INFOFRAME_TYPE_SPD;
	frame->version = 1;
	frame->length = HDMI_SPD_INFOFRAME_SIZE;

	strncpy(frame->vendor, vendor, sizeof(frame->vendor) - 1);
	strncpy(frame->product, product, sizeof(frame->product) - 1);
}

/**
 * hdmi_spd_infoframe_unpack() - unpack binary buffer to a HDMI SPD infoframe
 * @buffer: source buffer
 * @frame: HDMI SPD infoframe
 *
 * Unpacks the information contained in binary @buffer into a structured
 * @frame of the HDMI Source Product Description (SPD) information frame.
 * Also verifies the checksum as required by section 5.3.5 of the HDMI 1.4
 * specification.
 *
 * Returns 0 on success or a negative error code on failure.
 */
static int hdmi_spd_infoframe_unpack(struct hdmi_spd_infoframe *frame,
					 void *buffer)
{
	unsigned char *ptr = buffer;

	if (ptr[0] != HDMI_INFOFRAME_TYPE_SPD ||
		ptr[1] != 1 ||
		ptr[2] != HDMI_SPD_INFOFRAME_SIZE) {
		return -1;
	}

	if (hdmi_infoframe_checksum(buffer, HDMI_INFOFRAME_SIZE(SPD)) != 0)
		return -1;

	ptr += HDMI_INFOFRAME_HEADER_SIZE;

	hdmi_spd_infoframe_init(frame, (char *)ptr, (char *)ptr + 8);
	frame->sdi = ptr[24];

	return 0;
}

static void hdmi_audio_infoframe_init(struct hdmi_audio_infoframe *frame)
{
	memset(frame, 0, sizeof(*frame));

	frame->type = HDMI_INFOFRAME_TYPE_AUDIO;
	frame->version = 1;
	frame->length = HDMI_AUDIO_INFOFRAME_SIZE;
}

/**
 * hdmi_audio_infoframe_unpack() - unpack binary buffer to a HDMI AUDIO infoframe
 * @buffer: source buffer
 * @frame: HDMI Audio infoframe
 *
 * Unpacks the information contained in binary @buffer into a structured
 * @frame of the HDMI Audio information frame.
 * Also verifies the checksum as required by section 5.3.5 of the HDMI 1.4
 * specification.
 *
 * Returns 0 on success or a negative error code on failure.
 */
static int hdmi_audio_infoframe_unpack(struct hdmi_audio_infoframe *frame,
					   void *buffer)
{
	unsigned char *ptr = buffer;

	if (ptr[0] != HDMI_INFOFRAME_TYPE_AUDIO ||
		ptr[1] != 1 ||
		ptr[2] != HDMI_AUDIO_INFOFRAME_SIZE) {
		return -1;
	}

	if (hdmi_infoframe_checksum(buffer, HDMI_INFOFRAME_SIZE(AUDIO)) != 0)
		return -1;

	hdmi_audio_infoframe_init(frame);

	ptr += HDMI_INFOFRAME_HEADER_SIZE;

	frame->channels = ptr[0] & 0x7;
	frame->coding_type = (ptr[0] >> 4) & 0xf;
	frame->sample_size = ptr[1] & 0x3;
	frame->sample_frequency = (ptr[1] >> 2) & 0x7;
	frame->coding_type_ext = ptr[2] & 0x1f;
	frame->channel_allocation = ptr[3];
	frame->level_shift_value = (ptr[4] >> 3) & 0xf;
	frame->downmix_inhibit = ptr[4] & 0x80 ? 1 : 0;

	return 0;
}

static void hdmi_vendor_infoframe_init(struct hdmi_vendor_infoframe *frame)
{
	memset(frame, 0, sizeof(*frame));

	frame->type = HDMI_INFOFRAME_TYPE_VENDOR;
	frame->version = 1;
	frame->oui = HDMI_IEEE_OUI;
	frame->s3d_struct = HDMI_3D_STRUCTURE_INVALID;
	frame->vic = 0;
}

/**
 * hdmi_vendor_infoframe_unpack() - unpack binary buffer to a HDMI vendor infoframe
 * @buffer: source buffer
 * @frame: HDMI Vendor infoframe
 *
 * Unpacks the information contained in binary @buffer into a structured
 * @frame of the HDMI Vendor information frame.
 * Also verifies the checksum as required by section 5.3.5 of the HDMI 1.4
 * specification.
 *
 * Returns 0 on success or a negative error code on failure.
 */
static int
hdmi_vendor_any_infoframe_unpack(union hdmi_vendor_any_infoframe *frame,
				 void *buffer)
{
	unsigned char *ptr = buffer;
	size_t length;
	unsigned char hdmi_video_format;
	struct hdmi_vendor_infoframe *hvf = &frame->hdmi;

	length = ptr[2];

	if (hdmi_infoframe_checksum(buffer,
					HDMI_INFOFRAME_HEADER_SIZE + length) != 0)
		return -1;

	ptr += HDMI_INFOFRAME_HEADER_SIZE;

	hdmi_video_format = ptr[3] >> 5;

	hdmi_vendor_infoframe_init(hvf);

	hvf->length = length;
	hvf->oui = (ptr[2]<<16 | ptr[1]<<8 | ptr[0]);

	if (hvf->oui == HDMI_IEEE_OUI) {
		if (hdmi_video_format == 0x1) {
			hvf->vic = ptr[4];
		} else if (hdmi_video_format == 0x2) {
			hvf->s3d_struct = ptr[4] >> 4;
			if (hvf->s3d_struct >= HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF) {
				if (length == 6)
					hvf->s3d_ext_data = ptr[5] >> 4;
				else
					return -1;
			}
		}
	} else if (hvf->oui == HDMI_DV_IEEE_OUI) {
		hvf->DoViSignal = (ptr[3] & 0x02)>>1;
		hvf->LowLatency = ptr[3] & 0x01;
		HRX_LOG(HRX_DRV_DEBUG, "DoViSignal = %d LowLatency = %d\n",
			hvf->DoViSignal, hvf->LowLatency);
	}

	return 0;
}

static int
hdmi_hdr_infoframe_unpack(struct hdmi_hdr_infoframe *frame, void *buffer)
{
	unsigned char *ptr = buffer;
	unsigned char len = ptr[2];

	memset(frame, 0, sizeof(*frame));

	frame->type = HDMI_INFOFRAME_TYPE_HDR;
	frame->version = ptr[1];
	frame->length = ptr[2];

	if (len < 2)
		return 0;

	frame->eotf = ptr[4] & 0x7;
	frame->metadata_type = ptr[5] & 0x7;

	if (len < 26)
		return 0;

	frame->display_prim_x[0] = (ptr[7] << 8) | ptr[6];
	frame->display_prim_y[0] = (ptr[9] << 8) | ptr[8];
	frame->display_prim_x[1] = (ptr[11] << 8) | ptr[10];
	frame->display_prim_y[1] = (ptr[13] << 8) | ptr[12];
	frame->display_prim_x[2] = (ptr[15] << 8) | ptr[14];
	frame->display_prim_y[2] = (ptr[17] << 8) | ptr[16];

	frame->white_point_x = (ptr[19] << 8) | ptr[18];
	frame->white_point_y = (ptr[21] << 8) | ptr[20];

	frame->max_dml = (ptr[23] << 8) | ptr[22];
	frame->min_dml = (ptr[25] << 8) | ptr[24];

	frame->max_cll = (ptr[27] << 8) | ptr[26];
	frame->max_fll = (ptr[29] << 8) | ptr[28];

	return 0;
}

/**
 * hdmi_infoframe_unpack() - unpack binary buffer to a HDMI infoframe
 * @buffer: source buffer
 * @frame: HDMI infoframe
 *
 * Unpacks the information contained in binary buffer @buffer into a structured
 * @frame of a HDMI infoframe.
 * Also verifies the checksum as required by section 5.3.5 of the HDMI 1.4
 * specification.
 *
 * Returns 0 on success or a negative error code on failure.
 */
static int hdmi_infoframe_unpack(union hdmi_infoframe *frame, void *buffer)
{
	int ret;
	unsigned char *ptr = buffer;

	switch (ptr[0]) {
	case HDMI_INFOFRAME_TYPE_AVI:
		ret = hdmi_avi_infoframe_unpack(&frame->avi, buffer);
		break;
	case HDMI_INFOFRAME_TYPE_SPD:
		ret = hdmi_spd_infoframe_unpack(&frame->spd, buffer);
		break;
	case HDMI_INFOFRAME_TYPE_AUDIO:
		ret = hdmi_audio_infoframe_unpack(&frame->audio, buffer);
		break;
	case HDMI_INFOFRAME_TYPE_VENDOR:
		ret = hdmi_vendor_any_infoframe_unpack(&frame->vendor, buffer);
		break;
	case HDMI_INFOFRAME_TYPE_HDR:
		ret = hdmi_hdr_infoframe_unpack(&frame->hdr, buffer);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int hrx_hdr_infoframe_unpack(struct hrx_hdr_infoframe *frame,
		void *buffer)
{
	u8 *ptr = buffer;
	u8 len = ptr[2];

	/* Save RAW data */
	memcpy(frame->raw_data, ptr, 35);

	frame->version = ptr[1];
	frame->length = ptr[2];

	if (len < 2)
		return 0;

	frame->eotf = ptr[4] & 0x7;
	frame->metadata_type = ptr[5] & 0x7;

	if (len < 26)
		return 0;

	frame->display_prim_x[0] = (ptr[7] << 8) | ptr[6];
	frame->display_prim_y[0] = (ptr[9] << 8) | ptr[8];
	frame->display_prim_x[1] = (ptr[11] << 8) | ptr[10];
	frame->display_prim_y[1] = (ptr[13] << 8) | ptr[12];
	frame->display_prim_x[2] = (ptr[15] << 8) | ptr[14];
	frame->display_prim_y[2] = (ptr[17] << 8) | ptr[16];

	frame->white_point_x = (ptr[19] << 8) | ptr[18];
	frame->white_point_y = (ptr[21] << 8) | ptr[20];

	frame->max_dml = (ptr[23] << 8) | ptr[22];
	frame->min_dml = (ptr[25] << 8) | ptr[24];

	frame->max_cll = (ptr[27] << 8) | ptr[26];
	frame->max_fll = (ptr[29] << 8) | ptr[28];

	return 0;
}

static u8 hrx_infoframe_checksum(u8 *ptr, size_t size)
{
	u8 csum = 0;
	size_t i;

	for (i = 0; i < size; i++)
		csum += ptr[i];
	return (u8)(256 - csum);
}

static void hrx_get_raw_infoframe(struct syna_hrx_v4l2_dev *hrx_dev,
		struct hrx_infoframe_cfg *fcfg)
{
	u32 pb, ph = hrx_reg_read(hrx_dev, fcfg->header_addr);
	struct hrx_infoframe *frame = fcfg->frame;
	u8 packet_hlen = sizeof(fcfg->header);
	u8 packet_len = (ph >> 8) & 0xff;
	u8 packet[35];
	int i, j, pos = 0;

	/* HRX_LOG(HRX_DRV_DEBUG, "[%s]: packet_header=0x%x\n", fcfg->desc, ph); */

	if (!ph) /* Fail silently if there is no packet */
		return;
	if ((packet_len + packet_hlen + 1) > sizeof(packet)) {
		HRX_LOG(HRX_DRV_ERROR, "%s: invalid length\n", __func__);
		goto out;
	}

	memcpy(packet, fcfg->header, packet_hlen);
	packet[2] = packet_len; /* Replace fake header size by real header */
	pos += packet_hlen;

	for (i = 0; i < fcfg->payload_len; i++) {
		pb = hrx_reg_read(hrx_dev, fcfg->payload_addr + 4 * i);
		for (j = 0; j < 4; j++) {
			if (pos >= (packet_len + packet_hlen + 1))
				break;
			packet[pos++] = (pb >> (8 * j)) & 0xff;
		}
	}

	packet[3] = 0;
	packet[3] = hrx_infoframe_checksum(packet, packet_len +
			packet_hlen + 1);

	switch (fcfg->type) {
	case HDMI_INFOFRAME_TYPE_GENERAL:
		if (hdmi_infoframe_unpack(&frame->infoframe.general, packet)) {
			HRX_LOG(HRX_DRV_ERROR, "[%s]: failed to unpack\n", fcfg->desc);
			goto out;
		}

		break;
	case HDMI_INFOFRAME_TYPE_HDRIF:
		if (hrx_hdr_infoframe_unpack(&frame->infoframe.hdr, packet)) {
			HRX_LOG(HRX_DRV_ERROR, "[%s]: failed to unpack\n", fcfg->desc);
			goto out;
		}

		break;
	default:
		HRX_LOG(HRX_DRV_ERROR, "[%s]: invalid infoframe type %d\n", fcfg->desc, fcfg->type);
		goto out;
	}

	frame->valid = true;
	frame->type = fcfg->type;
	return;
out:
	return;
}

static const char *hrx_hdr_get_eotf(u8 eotf)
{
	switch (eotf) {
	case 0x0:
		return "Traditional gamma - SDR";
	case 0x1:
		return "Traditional gamma - HDR";
	case 0x2:
		return "SMPTE ST 2084";
	case 0x3:
		return "Hybrid Log-Gamma";
	default:
		return "Reserved";
	}
}

static const char *hrx_hdr_get_metadata_type(u8 type)
{
	switch (type) {
	case 0x0:
		return "Type 1";
	default:
		return "Reserved";
	}
}

static void hrx_hdr_infoframe_log(struct hrx_hdr_infoframe *frame)
{
	int i;

	HRX_LOG(HRX_DRV_DEBUG, "HDMI infoframe: %s, version %u, length %u\n",
			"Dynamic Range and Mastering",
			frame->version, frame->length);
	HRX_LOG(HRX_DRV_DEBUG, "    eotf: %s\n", hrx_hdr_get_eotf(frame->eotf));
	HRX_LOG(HRX_DRV_DEBUG, "    static metadata descriptor id: %s\n",
			hrx_hdr_get_metadata_type(frame->metadata_type));

	HRX_LOG(HRX_DRV_DEBUG, "    display primaries:\n");
	for (i = 0; i < 3; i++) {
		HRX_LOG(HRX_DRV_DEBUG, "        x[%d]: %u\n", i, frame->display_prim_x[i]);
		HRX_LOG(HRX_DRV_DEBUG, "        y[%d]: %u\n", i, frame->display_prim_y[i]);
	}

	HRX_LOG(HRX_DRV_DEBUG, "    white point: x %u, y %u\n",
			frame->white_point_x, frame->white_point_y);
	HRX_LOG(HRX_DRV_DEBUG, "    max display mastering luminance: %u\n", frame->max_dml);
	HRX_LOG(HRX_DRV_DEBUG, "    min display mastering luminance: %u\n", frame->min_dml);
	HRX_LOG(HRX_DRV_DEBUG, "    maximum content light level: %u\n", frame->max_cll);
	HRX_LOG(HRX_DRV_DEBUG, "    maximum frame-average light level: %u\n", frame->max_fll);
}

static void hrx_infoframe_log(const char *name, struct hrx_infoframe *frame)
{
	HRX_LOG(HRX_DRV_DEBUG, "--- Infoframe: %s%s\n", name,
			frame->valid ? " ---" : ": Not available ---");
	if (!frame->valid)
		return;

	switch (frame->type) {
	case HDMI_INFOFRAME_TYPE_GENERAL:
		hdmi_infoframe_log(&frame->infoframe.general);
		break;
	case HDMI_INFOFRAME_TYPE_HDRIF:
		hrx_hdr_infoframe_log(&frame->infoframe.hdr);
		break;
	default:
		return;
	}
}

void hrx_get_infoframes(struct syna_hrx_v4l2_dev *hrx_dev)
{
	struct hrx_infoframe_cfg ifs[] = {
		{
			.desc = "AVI",
			.type = HDMI_INFOFRAME_TYPE_GENERAL,
			.header = {0x82, 2, 13},
			.header_addr = HDMI_PKTDEC_AVIIF_PH,
			.payload_addr = HDMI_PKTDEC_AVIIF_PB(0),
			.payload_len = HDMI_PKTDEC_AVIIF_PBLEN,
			.frame = &hrx_dev->aviif,
			.frame_size = sizeof(hrx_dev->aviif),
		}, {
			.desc = "SPD",
			.type = HDMI_INFOFRAME_TYPE_GENERAL,
			.header = {0x83, 1, 25},
			.header_addr = HDMI_PKTDEC_SRCPDIF_PH,
			.payload_addr = HDMI_PKTDEC_SRCPDIF_PB(0),
			.payload_len = HDMI_PKTDEC_SRCPDIF_PBLEN,
			.frame = &hrx_dev->spdif,
			.frame_size = sizeof(hrx_dev->spdif),
		}, {
			.desc = "Audio",
			.type = HDMI_INFOFRAME_TYPE_GENERAL,
			.header = {0x84, 1, 10},
			.header_addr = HDMI_PKTDEC_AUDIF_PH,
			.payload_addr = HDMI_PKTDEC_AUDIF_PB(0),
			.payload_len = HDMI_PKTDEC_AUDIF_PBLEN,
			.frame = &hrx_dev->audioif,
			.frame_size = sizeof(hrx_dev->audioif),
		}, {
			.desc = "Vendor Specific",
			.type = HDMI_INFOFRAME_TYPE_GENERAL,
			.header = {0x81, 1, 0},
			.header_addr = HDMI_PKTDEC_VSIF_PH,
			.payload_addr = HDMI_PKTDEC_VSIF_PB(0),
			.payload_len = HDMI_PKTDEC_VSIF_PBLEN,
			.frame = &hrx_dev->vsif,
			.frame_size = sizeof(hrx_dev->vsif),
		}, {
			.desc = "HDR",
			.type = HDMI_INFOFRAME_TYPE_HDRIF,
			.header = {0x87, 1, 0},
			.header_addr = HDMI_PKTDEC_DRMIF_PH,
			.payload_addr = HDMI_PKTDEC_DRMIF_PB(0),
			.payload_len = HDMI_PKTDEC_DRMIF_PBLEN,
			.frame = &hrx_dev->hdrif,
			.frame_size = sizeof(hrx_dev->hdrif),
		},
	};
	union hdmi_vendor_any_infoframe *vendor;
	struct hdmi_avi_infoframe *avi;
	int i;

	for (i = 0; i < ARRAY_SIZE(ifs); i++) {
		memset(ifs[i].frame, 0, ifs[i].frame_size);
		hrx_get_raw_infoframe(hrx_dev, &ifs[i]);
	}


	vendor = &hrx_dev->vsif.infoframe.general.vendor;
	avi = &hrx_dev->aviif.infoframe.general.avi;
	hrx_infoframe_log("AVI", &hrx_dev->aviif);
	hrx_infoframe_log("VSIF", &hrx_dev->vsif);
	hrx_infoframe_log("SPD",  &hrx_dev->spdif);
	hrx_infoframe_log("AUDIO", &hrx_dev->audioif);
	hrx_infoframe_log("HDR",  &hrx_dev->hdrif);
	if ((vendor->any.length == 0x18) && (vendor->any.oui == HDMI_IEEE_OUI) &&
		(avi->colorspace == HDMI_COLORSPACE_RGB) &&
		(avi->extended_colorimetry == HDMI_EXTENDED_COLORIMETRY_BT2020) &&
		(avi->quantization_range == HDMI_QUANTIZATION_RANGE_FULL)) {
		vendor->hdmi.DoViSignal = 1;
		vendor->hdmi.LowLatency = 0;
		HRX_LOG(HRX_DRV_DEBUG, "Tunnel mode DoViSignal = %d ", vendor->hdmi.DoViSignal);
	}

	/*
	 * Update current VIC: When transmitting any extended video format
	 * indicated through use of the HDMI_VIC field in the HDMI Vendor
	 * Specific InfoFrame or any other format which is not described in
	 * the above cases, an HDMI Source shall set the AVI InfoFrame VIC
	 * field to zero.
	 */
	if (vendor->hdmi.vic && !avi->video_code) {
		hrx_dev->current_vic = vendor->hdmi.vic;
		hrx_dev->current_vic_is_4k = true;
	} else {
		hrx_dev->current_vic = avi->video_code;
		hrx_dev->current_vic_is_4k = false;
	}
	HRX_LOG(HRX_DRV_DEBUG, "current_vic = %d, is_current_vic_4k = %d\n",
		hrx_dev->current_vic, hrx_dev->current_vic_is_4k);
}
