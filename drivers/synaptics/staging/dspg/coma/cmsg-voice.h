/*
 * drivers/staging/dspg/coma/cmsg-voice.h - cordless messages / voice (RTP)
 *
 * Cordless messages are used to exchange commands and data between Linux and
 * cordless over cfifos. They consist of a type specifier, an optional parameter
 * block and optional additional payload.
 *
 * Copyright (C) 2009, 2012 DSPG Technologies GmbH
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

#ifndef CMSG_VOICE_H
#define CMSG_VOICE_H

enum cmsg_voice_types {
	CMSG_VOICE_REQUEST_GET_SESSION = 0,
	CMSG_VOICE_REPLY_GET_SESSION,
	CMSG_VOICE_REQUEST_SET_SESSION_FIFOS,
	CMSG_VOICE_REPLY_SET_SESSION_FIFOS,
	CMSG_VOICE_REQUEST_START_SESSION,
	CMSG_VOICE_REPLY_START_SESSION,
	CMSG_VOICE_REQUEST_STOP_SESSION,
	CMSG_VOICE_REPLY_STOP_SESSION,
	CMSG_VOICE_REQUEST_FREE_SESSION,
	CMSG_VOICE_REPLY_FREE_SESSION,
	CMSG_VOICE_REQUEST_SEND_DTMF,
	CMSG_VOICE_REPLY_SEND_DTMF,
	CMSG_VOICE_RECEIVE_DTMF,
	CMSG_VOICE_REQUEST_START_RTCP,
	CMSG_VOICE_REPLY_START_RTCP,
	CMSG_VOICE_REQUEST_STOP_RTCP,
	CMSG_VOICE_REPLY_STOP_RTCP,
	CMSG_VOICE_REQUEST_REPORT_RTCP,
	CMSG_VOICE_REPLY_REPORT_RTCP,
	CMSG_VOICE_REQUEST_UPDATE_SESSION,
	CMSG_VOICE_REPLY_UPDATE_SESSION,
	CMSG_VOICE_DATA,
	CMSG_VOICE_REQUEST_SEND_EVT,
	CMSG_VOICE_REPLY_SEND_EVT,
	CMSG_VOICE_REQUEST_UPDATE_RTCP,
	CMSG_VOICE_REPLY_UPDATE_RTCP,
	CMSG_VOICE_REQUEST_VOCODER_START_SESSION,
	CMSG_VOICE_REPLY_VOCODER_START_SESSION,
	CMSG_VOICE_REQUEST_VOCODER_STOP_SESSION,
	CMSG_VOICE_REPLY_VOCODER_STOP_SESSION,
};

typedef struct {
	unsigned int enc;
	unsigned int dec;
} rtp_session_fifos;

struct cmsg_voice_params {
	int session_id;
	int line_id;
	int result;
	void *vocoder_config;
	union {
		/* CMSG_REQUEST_SET_SESSION_FIFOS */
		rtp_session_fifos set_session_fifos;
		/* CMSG_REQUEST_START_SESSION */
		rtp_session_config start_session, update_session;
		/* CMSG_REQUEST_SEND_DTMF */
		rtp_dtmf_event send_dtmf;
		/* CMSG_REQUEST_SEND_DTMF */
		rtp_generic_event send_evt;
		/* CMSG_RECEIVE_DTMF */
		rtp_dtmf_event receive_dtmf;
		/* CMSG_REQUEST_START_RTCP */
		rtcp_session_config start_rtcp;
	} config;
};

#endif /* CMSG_VOICE_H */
