/*
 *  drivers/cordless/dev-voice.c - voice character device
 *
 *  This driver registers one character device for each available voice session.
 *  It allows only one reader and one writer at the same time.
 *
 *  Copyright (C) 2007 NXP Semiconductors
 *  Copyright (C) 2008, 2009 DSPG Technologies GmbH
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

/* TODO:
 *  - if minor numbers dont start at zero, this code will fail
 *  - read/write several rtp packets at once
 */

#define DEBUG

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/uio.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/net.h>
#include <linux/file.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <net/inet_sock.h>
#include <linux/platform_device.h>
#include <linux/coma/coma.h>
#include <linux/coma/cfifo.h>
#include <asm/uaccess.h>
#include <asm/types.h>

#include "voice.h"
#include "cmsg-voice.h"
#include "coma-voice.h"

#define SERVICE_NAME  "voice"
#define DEFAULT_CODEC RTP_SESSION_PT_G722
#define FIFO_SIZE     (4096*5)
#define TIMEOUT 1000 /* Timeout to wait for a CSS response (in milliseconds) */

static dev_t voice_chrdev;
static struct cdev voice_cdev;
static struct class *voice_class;
static struct device *voice_dev[CONFIG_CORDLESS_NUM_VOICELINES];
static int voice_reg;
static int service_id;
const unsigned int voice_numlines = CONFIG_CORDLESS_NUM_VOICELINES;

static DECLARE_COMPLETION(voice_reply_compl);
static DEFINE_MUTEX(voice_mutex);

static struct cmsg_voice_params last_cmsg_voice_params;
#ifndef CONFIG_CORDLESS_USER_SPACE_FIFOS
static struct task_struct *kmode_thread = NULL;
#endif

static enum cmsg_voice_types eRequest = 0;

enum voice_session_status {
	SESSION_INVALID = 0,
	SESSION_INITIALIZED,
	SESSION_STARTED,
};

/**
 * struct voice_session - all the info needed to run one audio session
 *
 * @enc_fifo:  pointer to cfifo used for the encoder
 * @dec_fifo:  pointer to cfifo used for the decoder
 * @reader:    pointer to the reading process
 * @writer:    pointer to the writing process
 *
 */
struct voice_session {
	struct cfifo *enc_fifo;
	struct cfifo *dec_fifo;
	struct file *reader;
	struct file *writer;
	wait_queue_head_t enc_wq;
	enum voice_session_status status;
	int session_id;
	int flushed;
	struct socket *sock;
	struct sockaddr_storage remote_addr;
	struct socket *sock_rtcp;
	struct sockaddr_storage remote_addr_rtcp;
	struct msghdr txmsg;
	struct msghdr rxmsg;
	/* dtmf handling */
	rtp_dtmf_event dtmf_event;
	struct mutex dtmf_event_mutex;
	wait_queue_head_t dtmf_wq;
	enum rtcp_mode rtcpmode;
	int isSymmetricRTP;
	rtp_session_config config;
	struct timespec64 last_timestamp;
	unsigned long last_rtptime;
	struct mutex sock_read_write_mutex;
	int isStopRequested;           /*** This new flag triggers VOICE_IOCSTOP_SESSION is called before closing the voice -fd**/
	struct voice_css_error css_error;
	int num_packets_to_retransmit;
	void *vocoder_config;
};

static struct voice_session voice_sessions[CONFIG_CORDLESS_NUM_VOICELINES];

static short rx_pkt_count[CONFIG_CORDLESS_NUM_VOICELINES];
static short tx_pkt_count[CONFIG_CORDLESS_NUM_VOICELINES];

static int
voice_registered(void)
{
	return (voice_reg != 0);
}

static void
voice_generic_reply(struct cmsg_voice_params *params)
{
	last_cmsg_voice_params = *params;
	if( last_cmsg_voice_params.session_id >= 0 && last_cmsg_voice_params.session_id < CONFIG_CORDLESS_NUM_VOICELINES){
		struct voice_session *session = &voice_sessions[last_cmsg_voice_params.session_id];
		if(last_cmsg_voice_params.result != 0){
			session->css_error.error_num = last_cmsg_voice_params.result;
			last_cmsg_voice_params.result = -EFAULT;
		}else{
			session->css_error.error_num = 0;
		}
	}
	complete(&voice_reply_compl);
}

static int
coma_create_voice_message(enum cmsg_voice_types type,
			  struct cmsg_voice_params *params,
			  void *payload, unsigned int payload_size)
{
	eRequest = type;
	return coma_cmsg_send(service_id, (int)type,
			      (void *)params, sizeof(*params),
			      payload, payload_size);
}

/*
 * voice_session specific function
 */
static void
voice_session_reset(struct voice_session *session)
{
	cfifo_reset(session->enc_fifo);
	cfifo_reset(session->dec_fifo);
	session->reader = 0;
	session->writer = 0;
	session->status = SESSION_INVALID;
	session->sock = NULL;
	session->isSymmetricRTP = 0;

	rx_pkt_count[session->session_id] = 0;
	tx_pkt_count[session->session_id] = 0;
	session->isStopRequested = 0;
	session->css_error.error_num = 0;
}

static int
voice_request_get_session(int session_id)
{
	int ret;
	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;
	params.line_id = session_id;
	ret = coma_create_voice_message(CMSG_VOICE_REQUEST_GET_SESSION, &params,
					NULL, 0);

	return ret;
}

static int
voice_get_session(struct voice_session *session)
{
	int ret;

	mutex_lock(&voice_mutex);

	ret = voice_request_get_session(session->session_id);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}

static int
voice_request_set_session_fifos(int session_id, struct cfifo *enc, struct cfifo *dec)
{
	int ret;
	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;
	params.line_id = session_id;
	params.config.set_session_fifos.enc = (unsigned int)cfifo_phys(enc);
	params.config.set_session_fifos.dec = (unsigned int)cfifo_phys(dec);

	ret = coma_create_voice_message(CMSG_VOICE_REQUEST_SET_SESSION_FIFOS,
					&params, NULL, 0);

	return ret;
}

static int
voice_set_fifos(struct voice_session *session)
{
	int ret;

	mutex_lock(&voice_mutex);

	ret = voice_request_set_session_fifos(session->session_id, session->enc_fifo,
					   session->dec_fifo);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}

static int
voice_request_start_session(int session_id, rtp_session_config *config)
{
	int ret;
	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;
	params.line_id = config->voip_line_id;

	memcpy(&params.config.start_session,config,sizeof(rtp_session_config));

	ret = coma_create_voice_message(CMSG_VOICE_REQUEST_START_SESSION,
					&params, NULL, 0);
	return ret;
}

static int
voice_start_session(struct voice_session *session)
{
	int ret;

	mutex_lock(&voice_mutex);

	ret = voice_request_start_session(session->session_id, &session->config);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}


	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}

static int
voice_request_stop_session(int session_id,int line_id)
{
	int ret;
	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;
	params.line_id         = line_id;
	ret = coma_create_voice_message(CMSG_VOICE_REQUEST_STOP_SESSION, &params,
					NULL, 0);

	return ret;
}

static int
voice_stop_session(struct voice_session *session)
{
	int ret;

	mutex_lock(&voice_mutex);

	ret = voice_request_stop_session(session->session_id,session->config.voip_line_id);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}

static int
voice_request_start_rtcp(int session_id, struct voice_kernelmode_rtcp *km_rtcp)
{
	int ret;

	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;
	memcpy(&params.config.start_rtcp, &km_rtcp->rtcpCfg,
	       sizeof(rtcp_session_config));
	ret = coma_create_voice_message(CMSG_VOICE_REQUEST_START_RTCP,
					&params, NULL, 0);

	return ret;
}

static int
voice_request_update_session(int session_id, rtp_session_config *mediaParams)
{
	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;
	params.line_id = mediaParams->voip_line_id;

	/* update configuration */
	memcpy(&params.config.update_session, mediaParams,
	       sizeof(rtp_session_config));

	return coma_create_voice_message(CMSG_VOICE_REQUEST_UPDATE_SESSION,
					 &params, NULL, 0);
}


static int
voice_request_update_rtcp(int session_id, rtcp_session_config *km_rtcpupdate)
{
	struct cmsg_voice_params params;

	if (!voice_registered())
	{
		return -EFAULT;
	}

	params.session_id = session_id;

	memcpy(&params.config.start_rtcp, km_rtcpupdate,
	       sizeof(rtcp_session_config));

	return  coma_create_voice_message(CMSG_VOICE_REQUEST_UPDATE_RTCP,
					&params, NULL, 0);
}

static int
voice_update_rtcp_session(struct voice_session *session, rtcp_session_config *km_rtcpupdate)
{
	int ret;

	mutex_lock(&voice_mutex);

	ret = voice_request_update_rtcp(session->session_id, km_rtcpupdate);
	if (ret < 0)
	{
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0)
	{
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}


static int
voice_start_rtcp(struct voice_session *session, struct voice_kernelmode_rtcp *km_rtcp)
{
	int ret;

	mutex_lock(&voice_mutex);

	ret = voice_request_start_rtcp(session->session_id, km_rtcp);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}

static int
voice_request_stop_rtcp(int session_id)
{
	int ret;
	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;

	ret = coma_create_voice_message(CMSG_VOICE_REQUEST_STOP_RTCP, &params,
					NULL, 0);

	return ret;
}

static int
voice_stop_rtcp(struct voice_session *session)
{
	int ret;

	mutex_lock(&voice_mutex);

	ret = voice_request_stop_rtcp(session->session_id);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}


	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}

static int
voice_request_free_session(int session_id)
{
	int ret;
	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;

	ret = coma_create_voice_message(CMSG_VOICE_REQUEST_FREE_SESSION, &params,
					NULL, 0);

	return ret;
}

static int
voice_free_session(struct voice_session *session)
{
	int ret;

	mutex_lock(&voice_mutex);

	ret = voice_request_free_session(session->session_id);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}

static int
voice_session_init(struct voice_session *session)
{
	int ret = 0, i;

	init_waitqueue_head(&session->enc_wq);

	for (i = 0; i < MAX_NUM_OF_CODEC; i++) {
		session->config.codec.rx_list[i].rx_pt = 0xff;
		memset(session->config.codec.rx_list[i].CodecStr, 0,
		       sizeof(session->config.codec.rx_list[i].CodecStr));
	}
	session->config.codec.rx_list[0].rx_pt = DEFAULT_CODEC;
	session->config.codec.rx_pt_event = 0xff;
	session->config.codec.tx_pt = DEFAULT_CODEC;
	session->config.codec.tx_pt_event = 0xff;
	session->config.codec.duration = DEFAULT_DURATION;
	session->config.codec.opts = 0;
	mutex_init(&session->dtmf_event_mutex);
	init_waitqueue_head(&session->dtmf_wq);

	ret = voice_get_session(session);
	if (ret != 0) {
		pr_err("voice_get_session failed %d\n", ret);
		return ret;
	}

	ret = voice_set_fifos(session);
	if (ret != 0) {
		pr_err("voice_set_fifos failed %d\n", ret);
		goto err_set_fifos;
	}


	session->status = SESSION_INITIALIZED;
	session->isStopRequested = 0;
	session->css_error.error_num = 0;
	session->num_packets_to_retransmit = 0;
	return 0;

err_set_fifos:
	voice_free_session(session);

	return ret;
}


#ifndef CONFIG_CORDLESS_USER_SPACE_FIFOS
static inline unsigned long timeval_to_rtp(const struct timespec64 *tv)
{
	return (tv->tv_sec * 8000) + (tv->tv_nsec / 125000);
}

/*
 * kernelmode
 */
static char kthread_buf[1492];

static void
voice_kmode_resend_packet(struct voice_session *session, struct msghdr *msg, struct kvec *vec, int to_send, unsigned char *enc_buf)
{
	struct rtp_packet_header *packet_header = NULL;
	int sent = 0;
	struct socket *sock = NULL;

	BUG_ON(to_send <= 0);

	packet_header = (struct rtp_packet_header *)enc_buf;

	/* If RTCP-MUX flag is enabled, Send RTCP Packet on the same RTP port*/
	if (session->config.rtcp_mux){
		sock = session->sock;
	}else{
		if(packet_header->packetType == CFIFO_RTP_PACKET){
			sock = session->sock;
		}else if (packet_header->packetType == CFIFO_RTCP_PACKET) {
			sock = session->sock_rtcp;
		}else{
			pr_err("%s: (session id %d) Unknown packet type %ld\n",
			__FUNCTION__,session->session_id,packet_header->packetType);
		}
	}
	if (sock)
		sent = kernel_sendmsg(sock, msg, vec, 1, to_send);

	if (sent < 0){
		pr_err("%s: sock_sendmsg() failed with error %d "
		       "(session %d) line %d\n", SERVICE_NAME, sent,session->session_id,__LINE__);
	}else if (sent != to_send){
		pr_err("%s: session %d failed to send bytes %d\n",
		        SERVICE_NAME, session->session_id, to_send - sent);
	}
}

static void
voice_kmode_process_send(struct voice_session *session, struct msghdr *msg)
{
	struct kvec vec;
	unsigned char *enc_buf;
	int to_send = 0, sent = 0;
	struct rtp_packet_header *packet_header = NULL;
	struct timespec64 timestamp;
	struct sockaddr_in *chan_remote_addr_rtcp = NULL;
	struct sockaddr_in6 *chan_remote_addr6_rtcp = NULL;

	/* kernel mode is on */
	while ((to_send = cfifo_get(session->enc_fifo, (void **)&enc_buf)) > 0) {
		/* send data */
		packet_header = (struct rtp_packet_header *)enc_buf;
		to_send = to_send - sizeof(struct rtp_packet_header);
		vec.iov_base = enc_buf + sizeof(struct rtp_packet_header);
		vec.iov_len = to_send;

		/* send to appropriate destination according to RTP/RTCP flag */
		msg->msg_namelen = sizeof(session->remote_addr);
		/* If RTCP-MUX flag is enabled, Send RTCP Packet on the same RTP port*/
                if (session->config.rtcp_mux){
                        msg->msg_name = &session->remote_addr;
                        sent = kernel_sendmsg(session->sock, msg, &vec, 1, to_send);
                } else {
			 if(packet_header->packetType == CFIFO_RTP_PACKET){
				msg->msg_name = &session->remote_addr;
				sent = kernel_sendmsg(session->sock, msg, &vec, 1, to_send);
			}else if (packet_header->packetType == CFIFO_RTCP_PACKET) {
				if (session->sock_rtcp){
					if (session->remote_addr_rtcp.ss_family == AF_INET6) {
						chan_remote_addr6_rtcp =
							(struct sockaddr_in6 *)
							&session->remote_addr_rtcp;
						if ((chan_remote_addr6_rtcp->sin6_addr.s6_addr != 0)) {
							msg->msg_name = &session->remote_addr_rtcp;
							sent = kernel_sendmsg(session->sock_rtcp, msg, &vec, 1, to_send);
						} else {
							sent = to_send;
						}
					} else {
						chan_remote_addr_rtcp =
							(struct sockaddr_in *)
							&session->remote_addr_rtcp;
						if (chan_remote_addr_rtcp->sin_addr.s_addr != 0) {
							msg->msg_name = &session->remote_addr_rtcp;
							sent = kernel_sendmsg(session->sock_rtcp, msg, &vec, 1, to_send);
						} else {
							sent = to_send;
						}
					}
				}
			}else{
				pr_err("%s: (session id %d) Unknown packet type %ld\n",
						__FUNCTION__,session->session_id,packet_header->packetType);
			}
		}

		/* For measuring the Audio cut through delay, we are printing 10 packets time.*/
		if( tx_pkt_count[session->session_id] < 10 ){
			ktime_get_real_ts64(&timestamp);

			pr_debug("[audio-cut-through-delay] %s() session id '%d' seconds '%lld'  miliseconds '%ld' \n" ,
					__FUNCTION__ , session->session_id , timestamp.tv_sec , timestamp.tv_nsec/1000 );
			tx_pkt_count[session->session_id]++;
		}
		if (sent < 0){
			if(-ECONNREFUSED == sent){
				if(session->num_packets_to_retransmit > 0){
					voice_kmode_resend_packet(session,msg,&vec,to_send,enc_buf);
					session->num_packets_to_retransmit--;
				}
			}else{
				pr_err("%s: sock_sendmsg() failed with error %d "
			       "(session %d)\n", SERVICE_NAME, sent,session->session_id);
			}
		}else if (sent != to_send){
			pr_err("%s: session %d failed to send bytes %d\n",
			        SERVICE_NAME, session->session_id, to_send - sent);
		}

		cfifo_processed(session->enc_fifo);
	}
}

static int
voice_kmode_process_recv(struct voice_session *session, struct msghdr *msg)
{
	struct kvec vec;
	unsigned char *dec_buf;
	int recvd = 0;
	struct timespec64 timestamp;
	unsigned long rxtimeinms;
	struct rtp_packet_header *packet_header = NULL;
	unsigned char ctrl_buf[100];
	struct cmsghdr *cmsg;
	int ttl = 0;
	RTP_HDR *pRtpHdr = NULL;

	if (session->config.codec.opts & RTP_ENABLE_SYMMETRIC_RSP) {
		msg->msg_name = &session->remote_addr;
		msg->msg_namelen = sizeof(session->remote_addr);
	}
	msg->msg_control = ctrl_buf;
	msg->msg_controllen = sizeof(ctrl_buf);
	memset(&ctrl_buf, 0, sizeof(ctrl_buf));

	while (1) {
		vec.iov_base = kthread_buf;
		vec.iov_len = sizeof(kthread_buf);

		recvd = kernel_recvmsg(session->sock, msg, &vec, 1,
					sizeof(kthread_buf), MSG_DONTWAIT);
		if (recvd <= 0)
			break;
		ktime_get_real_ts64(&timestamp);

		/* For measuring the Audio cut through delay, we are printing 10 packets time.*/
		if( rx_pkt_count[session->session_id] < 10 ){
			pr_debug("[audio-cut-through-delay] %s() session id '%d' seconds '%lld'  miliseconds '%ld' \n" ,
						__FUNCTION__ , session->session_id , timestamp.tv_sec , timestamp.tv_nsec/1000000 );
			rx_pkt_count[session->session_id]++;
		}

		/* convert rx time into milliseconds and send to CSS */
		rxtimeinms = timestamp.tv_sec * 1000;
		rxtimeinms += timestamp.tv_nsec / 1000000;
		session->last_timestamp.tv_sec = timestamp.tv_sec;
		session->last_timestamp.tv_nsec = timestamp.tv_nsec;

		dec_buf = cfifo_request(session->dec_fifo, recvd +
					sizeof(struct rtp_packet_header));
		if (IS_ERR(dec_buf))
			return NF_DROP; /* drop the packet */

		cmsg = (struct cmsghdr *)ctrl_buf;
		msg->msg_control = ctrl_buf;
		msg->msg_controllen = sizeof(ctrl_buf);
		for (cmsg = CMSG_FIRSTHDR(msg); cmsg; cmsg = CMSG_NXTHDR(msg, cmsg)) {
			if (!CMSG_OK(msg, cmsg))
				break;

			if (cmsg->cmsg_level != SOL_IP)
				continue;

			if (cmsg->cmsg_type != IP_TTL)
				continue;

			ttl = *(int *)CMSG_DATA(cmsg);
		}

		packet_header = (struct rtp_packet_header *)dec_buf;
		packet_header->ttl = ttl;

		if (session->config.rtcp_mux) {
			pRtpHdr = (RTP_HDR *)kthread_buf;
			if (pRtpHdr->bMarkerPayload == RTCP_TYPE_SR
				|| pRtpHdr->bMarkerPayload == RTCP_TYPE_RR
					|| pRtpHdr->bMarkerPayload == RTCP_TYPE_XR)
				packet_header->packetType = CFIFO_RTCP_PACKET;
		} else {
			packet_header->packetType = CFIFO_RTP_PACKET;
		}

		session->last_rtptime = rxtimeinms;
		packet_header->receiptTime = session->last_rtptime;
		memcpy(dec_buf + sizeof(struct rtp_packet_header),
		       kthread_buf, recvd);
		cfifo_commit(session->dec_fifo);
	}

	/* Try to Recv RTCP packets if any */
	if (!session->sock_rtcp)
		return 0;

	if (session->config.codec.opts & RTP_ENABLE_SYMMETRIC_RSP) {
		msg->msg_name = &session->remote_addr_rtcp;
		msg->msg_namelen = sizeof(session->remote_addr_rtcp);
	}

	while (1) {
		vec.iov_base = kthread_buf;
		vec.iov_len = sizeof(kthread_buf);

		recvd = kernel_recvmsg(session->sock_rtcp, msg, &vec, 1,
					sizeof(kthread_buf), MSG_DONTWAIT);

		if (recvd <= 0)
			break;

		/* Add extra space to distinguish RTP and RTCP */
		dec_buf = cfifo_request(session->dec_fifo, recvd +
					sizeof(struct rtp_packet_header));
		if (IS_ERR(dec_buf))
			return NF_DROP; /* drop the packet */

		cmsg = (struct cmsghdr *)ctrl_buf;
		msg->msg_control = ctrl_buf;
		msg->msg_controllen = sizeof(ctrl_buf);
		for (cmsg = CMSG_FIRSTHDR(msg); cmsg; cmsg = CMSG_NXTHDR(msg, cmsg)) {
			if (!CMSG_OK(msg, cmsg))
				break;

			if (cmsg->cmsg_level != SOL_IP)
				continue;

			if (cmsg->cmsg_type != IP_TTL)
				continue;

			ttl = *(int *)CMSG_DATA(cmsg);
		}

		packet_header = (struct rtp_packet_header *)dec_buf;
		packet_header->packetType = CFIFO_RTCP_PACKET;
		packet_header->ttl = ttl;
		packet_header->receiptTime = 0;
		memcpy(dec_buf + sizeof(struct rtp_packet_header),
		       kthread_buf, recvd);
		cfifo_commit(session->dec_fifo);
	}

	return 0;
}

static void
voice_kmode_process_recv_loop(struct voice_session *session, struct msghdr *msg)
{
	struct kvec vec;
	int recvd = 0;
	int sent = 0;

	if (session->config.codec.opts & RTP_ENABLE_SYMMETRIC_RSP) {
		msg->msg_name = &session->remote_addr;
		msg->msg_namelen = sizeof(session->remote_addr);
	}

	while (1) {
		vec.iov_base = kthread_buf;
		vec.iov_len = sizeof(kthread_buf);

		recvd = kernel_recvmsg(session->sock, msg, &vec, 1,
					sizeof(kthread_buf), MSG_DONTWAIT);

		if (recvd <= 0)
			break;

		if (recvd > 0) {
			vec.iov_base = kthread_buf;
			vec.iov_len = sizeof(kthread_buf);

			msg->msg_namelen = sizeof(session->remote_addr);
			msg->msg_name = &session->remote_addr;

			sent = kernel_sendmsg(session->sock, msg, &vec, 1, recvd);

			if (sent < 0)
				pr_err("%s: sock_sendmsg() failed with error %d "
				       "(session %d)\n", SERVICE_NAME, session->session_id, sent);
			else if (sent != recvd)
				pr_err("%s: failed to send %d bytes (session %d)\n",
				       SERVICE_NAME, session->session_id, recvd - sent);
		}
	}
}

static int
voice_kmode_thread(void *data)
{
	struct voice_session *session;
	struct task_struct *tsk = current;
	struct sched_param param = {.sched_priority = 1 };
	int i;

	sched_setscheduler(tsk, SCHED_FIFO, &param);

	while (!kthread_should_stop()) {
		for (i = 0; i < voice_numlines ; i++) {
			session = &voice_sessions[i];
			mutex_lock(&session->sock_read_write_mutex);
			if (session->status != SESSION_STARTED) {
				mutex_unlock(&session->sock_read_write_mutex);
				continue;
			}

			if (session->sock) {
				if ((session->config.media_loop_level == RTP_LOOP_IP_LEVEL)) {
					voice_kmode_process_recv_loop(session, &session->rxmsg);
					mutex_unlock(&session->sock_read_write_mutex);
					continue;
				}
				voice_kmode_process_send(session, &session->txmsg);
				if (voice_kmode_process_recv(session, &session->rxmsg) < 0)
					pr_warn("%s: FIFO full - dropping incoming RTP packet\n", SERVICE_NAME);
			} else {
				if (!cfifo_empty(session->enc_fifo))
					wake_up(&session->enc_wq);
			}
			mutex_unlock(&session->sock_read_write_mutex);
		}
		usleep_range(4000, 4500);
	}

	return 0;
}
#endif

static int
voice_kmode_start(struct voice_session *session, struct voice_kernelmode *km)
{
	int ret = 0;

	/* check if kernelmode is already active */
	if (session->sock) {
		pr_err("%s: start: kmode active\n", SERVICE_NAME);
		return -1;
	}

	/* set up kmode structure */
	session->remote_addr = km->remote_addr;
	session->sock = sockfd_lookup(km->sock_fd, &ret);
	session->sock_rtcp  = 0;
	memset(&session->txmsg, 0, sizeof(session->txmsg));
	memset(&session->rxmsg, 0, sizeof(session->rxmsg));

	if (!session->sock) {
		pr_err("%s: start: socket invalid\n", SERVICE_NAME);
		return -1;
	}
	{
		struct inet_sock *inet = inet_sk(session->sock->sk);
		inet->cmsg_flags |= IP_CMSG_TTL;
	}
	return ret;
}

static void
voice_kmode_stop(struct voice_session *session)
{
	if (session->sock) {
		sockfd_put(session->sock);
		session->sock = NULL;
	}

	if (session->sock_rtcp) {
		sockfd_put(session->sock_rtcp);
		session->sock_rtcp = NULL;
	}
}

static int
voice_kmode_start_rtcp(struct voice_session *session,
                       struct voice_kernelmode_rtcp *km_rtcp)
{
	int ret = 0;

	session->remote_addr_rtcp = km_rtcp->remote_addr;
	session->sock_rtcp = sockfd_lookup(km_rtcp->sock_fd, &ret);

	return ret;
}

static void
voice_kmode_stop_rtcp(struct voice_session *session)
{
	if (session->sock_rtcp) {
		sockfd_put(session->sock_rtcp);
		session->sock_rtcp = NULL;
	}
	/* notify cordless */
	voice_stop_rtcp(session);
}

static int
voice_update_session(struct voice_session *session)
{
	int ret;

	/* notify cordless */
	mutex_lock(&voice_mutex);

	ret = voice_request_update_session(session->session_id, &session->config);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;

	mutex_unlock(&voice_mutex);

	return ret;
}

/*
 * dtmf handling
 */
static void
voice_receive_dtmf(int session_id, rtp_dtmf_event *dtmf_event)
{
	struct voice_session *session = &voice_sessions[session_id];

	mutex_lock(&session->dtmf_event_mutex);
	memcpy(&session->dtmf_event, dtmf_event, sizeof(rtp_dtmf_event));
	mutex_unlock(&session->dtmf_event_mutex);
	wake_up(&session->dtmf_wq);
}

static int
voice_request_send_dtmf(int session_id, rtp_dtmf_event *dtmf_event)
{
	int ret;
	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;
	memcpy(&params.config.send_dtmf, dtmf_event, sizeof(rtp_dtmf_event));
	ret = coma_create_voice_message(CMSG_VOICE_REQUEST_SEND_DTMF, &params,
					NULL, 0);

	return ret;
}

static int
voice_request_send_evt(int session_id, rtp_generic_event *evt)
{
	int ret;
	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;
	memcpy(&params.config.send_evt, evt, sizeof(rtp_generic_event));
	ret = coma_create_voice_message(CMSG_VOICE_REQUEST_SEND_EVT, &params,
					NULL, 0);

	return ret;
}

static int
voice_send_dtmf(struct voice_session *session, rtp_dtmf_event *new)
{
	int ret;

	mutex_lock(&voice_mutex);

	ret = voice_request_send_dtmf(session->session_id, new);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}

static int
voice_send_generic_evt(struct voice_session *session, rtp_generic_event *new)
{
	int ret;
	mutex_lock(&voice_mutex);

	ret = voice_request_send_evt(session->session_id, new);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(&voice_reply_compl,msecs_to_jiffies(TIMEOUT));
	if(ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}

static void
voice_validate_css_response(enum cmsg_voice_types eResponse,struct cmsg_voice_params *params)
{
	int ret = 0;
	switch(eResponse) {
		case CMSG_VOICE_REPLY_GET_SESSION :
			if(eRequest != CMSG_VOICE_REQUEST_GET_SESSION)
				ret = -1;
			break;
		case CMSG_VOICE_REPLY_SET_SESSION_FIFOS :
			if(eRequest != CMSG_VOICE_REQUEST_SET_SESSION_FIFOS)
				ret = -1;
			break;
		case CMSG_VOICE_REPLY_START_SESSION:
			if(eRequest != CMSG_VOICE_REQUEST_START_SESSION)
				ret = -1;
			break;

		case CMSG_VOICE_REPLY_VOCODER_START_SESSION:
			if (eRequest !=
				CMSG_VOICE_REQUEST_VOCODER_START_SESSION)
				ret = -1;
			break;
		case CMSG_VOICE_REPLY_STOP_SESSION:
			if(eRequest != CMSG_VOICE_REQUEST_STOP_SESSION)
				ret = -1;
			break;
		case CMSG_VOICE_REPLY_FREE_SESSION:
			if(eRequest != CMSG_VOICE_REQUEST_FREE_SESSION)
				ret = -1;
			break;
		case CMSG_VOICE_REPLY_START_RTCP:
			if(eRequest != CMSG_VOICE_REQUEST_START_RTCP)
				ret = -1;
			break;
		case CMSG_VOICE_REPLY_STOP_RTCP:
			if(eRequest != CMSG_VOICE_REQUEST_STOP_RTCP)
				ret = -1;
			break;
		case CMSG_VOICE_REPLY_SEND_DTMF:
			if(eRequest != CMSG_VOICE_REQUEST_SEND_DTMF)
				ret = -1;
			break;
		case CMSG_VOICE_REPLY_UPDATE_SESSION:
			if(eRequest != CMSG_VOICE_REQUEST_UPDATE_SESSION)
				ret = -1;
			break;
		case CMSG_VOICE_REPLY_SEND_EVT:
			if(eRequest != CMSG_VOICE_REQUEST_SEND_EVT)
				ret = -1;
			break;
		case CMSG_VOICE_REPLY_UPDATE_RTCP:
			if(eRequest != CMSG_VOICE_REQUEST_UPDATE_RTCP)
				ret = -1;
			break;
		default :
			ret = -1;
			break;
	}

	if( ret == -1){
		pr_err("Request sent to CSS : %d Response Received from CSS : %d\n",eRequest,eResponse);
	}


	return;
}

/*
 * coma handling
 */
static void
voice_process_message(void *arg, struct cmsg *cmsg)
{
	int ret = 0;
	struct cmsg_voice_params *params =
				(struct cmsg_voice_params *)cmsg_params(cmsg);

	switch (cmsg->type) {
	case CMSG_VOICE_REPLY_GET_SESSION:
	case CMSG_VOICE_REPLY_SET_SESSION_FIFOS:
	case CMSG_VOICE_REPLY_START_SESSION:
	case CMSG_VOICE_REPLY_VOCODER_START_SESSION:
	case CMSG_VOICE_REPLY_STOP_SESSION:
	case CMSG_VOICE_REPLY_FREE_SESSION:
	case CMSG_VOICE_REPLY_START_RTCP:
	case CMSG_VOICE_REPLY_STOP_RTCP:
	case CMSG_VOICE_REPLY_SEND_DTMF:
	case CMSG_VOICE_REPLY_UPDATE_SESSION:
	case CMSG_VOICE_REPLY_SEND_EVT:
	case CMSG_VOICE_REPLY_UPDATE_RTCP:
		voice_validate_css_response(cmsg->type,params);
		voice_generic_reply(params);
		break;

	case CMSG_VOICE_RECEIVE_DTMF:
		voice_receive_dtmf(params->session_id,
		                   &params->config.receive_dtmf);
		break;
	default:
		/* Handle RTP and RTCP Packets*/
		ret = -1;
		break;
	}

	return;
}

static int
voice_request_start_vocoder_session(int session_id, void *vocoder_config)
{
	int ret;
	struct cmsg_voice_params params;

	if (!voice_registered())
		return -EFAULT;

	params.session_id = session_id;
	params.line_id = session_id; //debatable
	params.vocoder_config = vocoder_config;

	ret = coma_create_voice_message(
			CMSG_VOICE_REQUEST_VOCODER_START_SESSION,
			&params, NULL, 0);
	return ret;
}

/*
 * start vocoder session
 */
static int
voice_start_vocoder_session(struct voice_session *session)
{
	int ret;

	mutex_lock(&voice_mutex);

	ret = voice_request_start_vocoder_session(
			session->session_id,
			session->vocoder_config);
	if (ret < 0) {
		mutex_unlock(&voice_mutex);
		return -EFAULT;
	}

	ret = wait_for_completion_timeout(
			&voice_reply_compl,
			msecs_to_jiffies(TIMEOUT));
	if (ret == 0) {
		mutex_unlock(&voice_mutex);
		return -ETIMEDOUT;
	}

	ret = last_cmsg_voice_params.result;
	mutex_unlock(&voice_mutex);

	return ret;
}

/*
 * character device functions
 */
static long
voice_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int size;
	int ret = 0;
	void __user *argp = (void __user *)arg;
	unsigned int minor = iminor(file->f_path.dentry->d_inode);
	struct voice_session *session = &voice_sessions[minor];

	if (_IOC_TYPE(cmd) != VOICE_IOC_MAGIC)
		return -EINVAL;

	if (_IOC_NR(cmd) > VOICE_IOC_MAXNR)
		return -EINVAL;

	size = _IOC_SIZE(cmd);

	if (!access_ok(argp, size))
		return -EFAULT;

	switch (cmd) {
	case VOICE_IOCSETCODEC:
	{
		mutex_lock(&session->sock_read_write_mutex);
		if (copy_from_user(&session->config, argp, sizeof(session->config))) {
			mutex_unlock(&session->sock_read_write_mutex);
			return -EFAULT;
		}
		/*
		 * TODO: switch codec in stack, this will also do
		 *       the sanity checking for us (result in reply)
		 */

		/* update our local info if codec switch went ok */
		if (session->status == SESSION_INVALID){
			mutex_unlock(&session->sock_read_write_mutex);
			return -EFAULT;
		}

		if (session->status == SESSION_STARTED){
			mutex_unlock(&session->sock_read_write_mutex);
			ret = voice_stop_session(session);
			if(ret < 0){
				return ret;
			}
			mutex_lock(&session->sock_read_write_mutex);
		}

		cfifo_reset(session->enc_fifo);

		/* If Symmetric Pkt count is configured as zero set to default value */
		if (session->config.SymmRTPTxPktCnt == 0)
			session->config.SymmRTPTxPktCnt = SYMMETRIC_TX_RTP_CNT;

		/* Release the lock while communicating to CSS */
		mutex_unlock(&session->sock_read_write_mutex);

		ret = voice_start_session(session);
		if (ret < 0)
			return ret;

		mutex_lock(&session->sock_read_write_mutex);
		session->last_timestamp.tv_sec = 0;
		session->last_timestamp.tv_nsec = 0;
		session->last_rtptime = 0;
		session->status = SESSION_STARTED;
		mutex_unlock(&session->sock_read_write_mutex);
		return ret;
	}
	case VOICE_IOCVOCODERSTART:
	{
		mutex_lock(&session->sock_read_write_mutex);

		session->vocoder_config = (void *)argp;
		/*
		 * TODO: switch codec in stack, this will also do
		 *		 the sanity checking for us (result in reply)
		 */
		if (session->status == SESSION_STARTED) {
			mutex_unlock(&session->sock_read_write_mutex);
			ret = voice_stop_session(session);
			if (ret < 0)
				return ret;
			mutex_lock(&session->sock_read_write_mutex);
		}
		cfifo_reset(session->enc_fifo);
		/* Release the lock while communicating to CSS */
		mutex_unlock(&session->sock_read_write_mutex);
		ret = voice_start_vocoder_session(session);
		if (ret < 0)
			return ret;
		mutex_lock(&session->sock_read_write_mutex);
		session->status = SESSION_STARTED;
		mutex_unlock(&session->sock_read_write_mutex);
		return ret;
	}
	case VOICE_IOCSENDDTMF:
	{
		rtp_dtmf_event *dtmf;

		dtmf = kmalloc(sizeof(rtp_dtmf_event), GFP_KERNEL);

		if (dtmf == NULL)
			return -ENOMEM;

		if (copy_from_user(dtmf, argp, sizeof(rtp_dtmf_event))) {
			kfree(dtmf);
			return -EFAULT;
		}
		ret = voice_send_dtmf(session, dtmf);

		kfree(dtmf);

		return ret;
	}

	case VOICE_IOCGENERIC_EVENT:
	{
		rtp_generic_event *event;

		event = kmalloc(sizeof(rtp_generic_event), GFP_KERNEL);

		if (event == NULL)
			return -ENOMEM;

		if (copy_from_user(event, argp, sizeof(rtp_generic_event))) {
			kfree(event);
			return -EFAULT;
		}

		ret = voice_send_generic_evt(session, event);

		kfree(event);

		return ret;

	}

	case VOICE_IOCGETDTMF:
	{
		return 0;
	}

	case VOICE_IOCFLUSH:
	{
		cfifo_reset(session->enc_fifo);
		return 0;
	}

	case VOICE_IOCKERNELMODE:
	{
		struct voice_kernelmode *km;
		/*
		 * if argp is set, start kernelmode, if its NULL,
		 * stop kernelmode
		 */
		if (argp) {
			km = kmalloc(sizeof(struct voice_kernelmode), GFP_KERNEL);

			if (km == NULL)
				return -ENOMEM;

			if (copy_from_user(km, argp, sizeof(struct voice_kernelmode))) {
				kfree(km);
				return -EFAULT;
			}

			/* Here communicating to Kernel Thread, not to CSS,
				dont release lock */
			mutex_lock(&session->sock_read_write_mutex);
			ret = voice_kmode_start(session, km);
			mutex_unlock(&session->sock_read_write_mutex);
			kfree(km);
		} else {
			mutex_lock(&session->sock_read_write_mutex);
			voice_kmode_stop(session);
			mutex_unlock(&session->sock_read_write_mutex);
		}
		return ret;
	}

	case VOICE_IOCRTCP:
	{
		struct voice_kernelmode_rtcp *km_rtcp;
		/* if argp is set, start rtcp, if its NULL, stop rtcp */
		if (argp) {

			km_rtcp = kmalloc(sizeof(struct voice_kernelmode_rtcp), GFP_KERNEL);

			if (km_rtcp == NULL)
				return -ENOMEM;

			if (copy_from_user(km_rtcp, argp, sizeof(struct voice_kernelmode_rtcp))) {
				kfree(km_rtcp);
				return -EFAULT;
			}

			/* if user mode, just inform CSS and return */
			if (km_rtcp->rtcpCfg.opts & RTCP_USER_MODE) {
				/* notify cordless */
				ret = voice_start_rtcp(session, km_rtcp);
				kfree(km_rtcp);
				return ret;
			}

			if (!session->config.rtcp_mux) {
				mutex_lock(&session->sock_read_write_mutex);
				ret = voice_kmode_start_rtcp(session, km_rtcp);
				mutex_unlock(&session->sock_read_write_mutex);
			}

			/* notify cordless */
			if (!ret)
				ret = voice_start_rtcp(session, km_rtcp);
			kfree(km_rtcp);

		} else {
			mutex_lock(&session->sock_read_write_mutex);
			voice_kmode_stop_rtcp(session);
			mutex_unlock(&session->sock_read_write_mutex);
		}

		return ret;
	}
	case VOICE_IOCRTCPUPDATE_SESSION:
	{
		rtcp_session_config *km_rtcpupdate;

		km_rtcpupdate = kmalloc(sizeof(rtcp_session_config), GFP_KERNEL);

		if (km_rtcpupdate == NULL)
			return -ENOMEM;

		if (copy_from_user(km_rtcpupdate, argp, sizeof(rtcp_session_config))) {
			kfree(km_rtcpupdate);
			return -EFAULT;
		}

		ret = voice_update_rtcp_session(session, km_rtcpupdate);
		kfree(km_rtcpupdate);
		return ret;
	}

	case VOICE_IOCUPDATE_SESSION:
	{
		mutex_lock(&session->sock_read_write_mutex);
		if (copy_from_user(&session->config, argp, sizeof(session->config))){
			mutex_unlock(&session->sock_read_write_mutex);
			return -EFAULT;
		}

		mutex_unlock(&session->sock_read_write_mutex);

		/* Talking to CSS, so releasing above lock */
		ret = voice_update_session(session);

		return ret;
	}

	case VOICE_IOCSTOP_SESSION:
	{
		if(session->status == SESSION_STARTED){
			mutex_lock(&session->sock_read_write_mutex);
			session->isStopRequested = 1;  /**Set this flag to indicate that for this voice sessionnel stop is requested **/
			session->num_packets_to_retransmit = NUM_PACKET_TO_RETRANSMIT;
			mutex_unlock(&session->sock_read_write_mutex);
			ret = voice_stop_session(session);
		}
		return ret;
	}

	case VOICE_IOCUPDATE_CHAN_PARAMS:
	{
		struct voice_kernelmode_ip *km;

		if (argp) {

			km = kmalloc(sizeof(struct voice_kernelmode_ip), GFP_KERNEL);

			if (km == NULL)
				return -ENOMEM;

			if (copy_from_user(km, argp, sizeof(struct voice_kernelmode_ip))) {
				kfree(km);
				return -EFAULT;
			}

			mutex_lock(&session->sock_read_write_mutex);
			if (km->rtp_sock_fd)
				session->remote_addr = km->rtp_remote_addr;

			if (km->rtcp_sock_fd)
				session->remote_addr_rtcp = km->rtcp_remote_addr;

			mutex_unlock(&session->sock_read_write_mutex);
			kfree(km);
		}
		return 0;
	}
	case VOICE_IOCGET_FIFO_INFO:
	{
		struct voice_fifo_info *info;

		info = kmalloc(sizeof(struct voice_fifo_info), GFP_KERNEL);

		if (info == NULL)
			return -ENOMEM;

		info->enc_offset = 0;
		info->enc_size = session->enc_fifo->alloc_size;
		info->dec_offset = 1 << PAGE_SHIFT;
		info->dec_size = session->dec_fifo->alloc_size;
		if (copy_to_user((void*)arg, info, sizeof(struct voice_fifo_info))) {
			kfree(info);
			return -EFAULT;
		}
		kfree(info);
		break;
	}

	case VOICE_IOCGET_LAST_CSS_ERROR:
	{
		if(copy_to_user((void*)arg, &session->css_error, sizeof(struct voice_css_error)))
			return -EFAULT;
		break;
	}

#ifndef CONFIG_CORDLESS_USER_SPACE_FIFOS
	case VOICE_IOCSETPRIORITY:
	{
		struct thread_priority_param param;

		if (copy_from_user(&param, argp, sizeof(param)))
			return -EFAULT;

		pr_info("changing kvoice driver thread policy: %d,priority: %d\n",
			param.policy, param.sched.sched_priority);
		sched_setscheduler(kmode_thread, param.policy, &param.sched);
		break;
	}
#endif

	default: /* redundant, as cmd was checked against MAXNR */
		return -EINVAL;
	}
	return 0;
}

static int
voice_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	unsigned int flags = file->f_flags & O_ACCMODE;
	unsigned int minor = iminor(inode);
	struct voice_session *session = &voice_sessions[minor];

	if (!voice_registered())
		return -1;

	/* only allow one reader */
	if (flags == O_RDONLY || flags == O_RDWR) {
		if (session->reader) {
			return -EBUSY;
		} else {
			session->reader = file;
			file->private_data = session;
		}
	}

	/* only allow one writer */
	if (flags == O_WRONLY || flags == O_RDWR) {
		if (session->writer) {
			return -EBUSY;
		} else {
			session->writer = file;
			file->private_data = session;
		}
	}

	if (session->status == SESSION_INVALID) {
		session->session_id = minor;
		ret = voice_session_init(session);
	}

	if (ret != 0) {
		session->reader = 0;
		session->writer = 0;
	}

	return ret;
}

static int
voice_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	unsigned int minor = iminor(inode);
	struct voice_session *session = &voice_sessions[minor];

	if (!voice_registered())
		return -1;

	/* Communication to CSS -  No need of sock read write lock
	     Communicate to Kernel Thread - sock read write lock needed*/

	mutex_lock(&session->sock_read_write_mutex);
	if (session->reader == file)
		session->reader = NULL;

	if (session->writer == file)
		session->writer = NULL;

	if (!session->reader && !session->writer) {
		if(session->status == SESSION_STARTED &&
			0 == session->isStopRequested){
			mutex_unlock(&session->sock_read_write_mutex);
			voice_stop_session(session);
			mutex_lock(&session->sock_read_write_mutex);
		}
		if (session->sock)
			voice_kmode_stop(session);
		mutex_unlock(&session->sock_read_write_mutex);
		voice_free_session(session);
		mutex_lock(&session->sock_read_write_mutex);
		voice_session_reset(session);
	}

	mutex_unlock(&session->sock_read_write_mutex);

	return ret;
}

#ifndef CONFIG_CORDLESS_USER_SPACE_FIFOS
static ssize_t
voice_read(struct file *file, char __user *buf, size_t count_want,
           loff_t *f_pos)
{
	struct voice_session *session = (struct voice_session *)file->private_data;
	unsigned char *enc_buf;
	size_t not_copied;
	ssize_t to_copy;
	ssize_t ret;

	if (!voice_registered())
		return -1;

	if (session->sock)
		return -EBUSY;

	if (wait_event_interruptible(session->enc_wq,
				     (cfifo_empty(session->enc_fifo) == 0)))
		return -ERESTARTSYS;

	ret = cfifo_get(session->enc_fifo, (void **)&enc_buf);
	if (ret < 0)
		return ret;

	/* copy one rtp packet at a time */
	to_copy = ret;
	/* Need to check if we are returning more than what has been asked ??? */
	if(count_want < to_copy){
		pr_err("%s: coma-voice: Got more data of '%d' than requested, copying only requested '%d' \n" , SERVICE_NAME , to_copy , count_want);
		to_copy = count_want;
	}
	not_copied = copy_to_user(buf, enc_buf, to_copy);
	cfifo_processed(session->enc_fifo);

	return (to_copy - not_copied);
}

static ssize_t
voice_write(struct file *file, const char __user *buf, size_t count,
            loff_t *f_pos)
{
	int ret = 0;
	struct voice_session *session = (struct voice_session *)file->private_data;
	char *fifo_buf;

	if (!voice_registered())
		return -1;

	if (session->sock)
		return -EBUSY;
	fifo_buf = cfifo_request(session->dec_fifo, count);
	if (IS_ERR(fifo_buf))
		return -ENOMEM;

	ret = copy_from_user(fifo_buf, buf, count);
	if (ret != 0)
		return -1;

	cfifo_commit(session->dec_fifo);

	return count;
}

static unsigned int
voice_poll(struct file *file, poll_table * event_list)
{
	struct voice_session *session = (struct voice_session *)file->private_data;
	unsigned int mask = POLLOUT | POLLWRNORM; /* we always allow writing */

	if (!cfifo_empty(session->enc_fifo))
		mask |= POLLIN | POLLRDNORM;

	poll_wait(file, &session->enc_wq, event_list);

	return mask;
}
#else
static const struct vm_operations_struct mmap_mem_ops = {
};

static int
voice_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct voice_session *session = (struct voice_session *)file->private_data;
	struct cfifo *cfif;
	size_t size = vma->vm_end - vma->vm_start;

	switch (vma->vm_pgoff) {
	case 0:
		cfif = session->enc_fifo;
		break;
	case 1:
		cfif = session->dec_fifo;
		break;
	default:
		return -EFAULT;
	}

	if (size != cfif->alloc_size)
		return -EIO;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_ops = &mmap_mem_ops;

	if (remap_pfn_range(vma,
			    vma->vm_start,
			    cfif->phys >> PAGE_SHIFT,
			    cfif->alloc_size,
			    vma->vm_page_prot)) {
		return -EAGAIN;
	}

	return 0;
}
#endif

static struct file_operations voice_fops = {
	.owner   = THIS_MODULE,
	.unlocked_ioctl   = voice_ioctl,
	.open    = voice_open,
	.release = voice_release,
#ifndef CONFIG_CORDLESS_USER_SPACE_FIFOS
	.read    = voice_read,
	.write   = voice_write,
	.poll    = voice_poll,
#else
	.mmap    = voice_mmap,
#endif
};

static void
voice_release_all(void)
{
	int i;
	struct voice_session *session;

	if (voice_registered()) {
		for (i = 0; i < CONFIG_CORDLESS_NUM_VOICELINES; i++) {
			session = &voice_sessions[i];

			mutex_lock(&session->sock_read_write_mutex);
			if (session->sock)
				voice_kmode_stop(session);

			if (session->status == SESSION_STARTED) {
				voice_stop_session(session);
				voice_free_session(session);
			}
			voice_session_reset(session);
			mutex_unlock(&session->sock_read_write_mutex);
		}
	}
}

static void
voice_remove(void *arg)
{
	int i;

#ifndef CONFIG_CORDLESS_USER_SPACE_FIFOS
	if (kmode_thread) {
		kthread_stop(kmode_thread);
		kmode_thread = NULL;
	}
#endif

	if (voice_reg) {
		voice_reg = 0;

		voice_release_all();
		for (i = 0; i < CONFIG_CORDLESS_NUM_VOICELINES; i++) {
			device_unregister(voice_dev[i]);
		}
		class_destroy(voice_class);
		cdev_del(&voice_cdev);
		unregister_chrdev_region(voice_chrdev, voice_numlines);
	}

	for (i = 0; i < CONFIG_CORDLESS_NUM_VOICELINES; i++) {
		if (voice_sessions[i].enc_fifo) {
			cfifo_unref(voice_sessions[i].enc_fifo);
			voice_sessions[i].enc_fifo = NULL;
		}
		if (voice_sessions[i].dec_fifo) {
			cfifo_unref(voice_sessions[i].dec_fifo);
			voice_sessions[i].dec_fifo = NULL;
		}
	}
}

static int
voice_suspend(void)
{
	int i;

	for (i = 0; i < CONFIG_CORDLESS_NUM_VOICELINES; i++) {
		if (voice_sessions[i].status != SESSION_INVALID)
			return -EINVAL;
	}

	return 0;
}

int
voice_init(struct device *dev)
{
	int i, ret = 0;

	ret = alloc_chrdev_region(&voice_chrdev, 0, voice_numlines, SERVICE_NAME);
	if (ret) {
		pr_err("%s: coma-voice: alloc of chrdev returned %d\n",
		       SERVICE_NAME, ret);
		goto out;
	}

	cdev_init(&voice_cdev, &voice_fops);

	voice_cdev.owner = THIS_MODULE;
	voice_cdev.ops   = &voice_fops;

	ret = cdev_add(&voice_cdev, voice_chrdev, voice_numlines);
	if (ret) {
		pr_err("%s: error %d adding character device", SERVICE_NAME, ret);
		goto err_unreg_chrdev_reg;
	}

	/* Create a sysfs class */
	voice_class = class_create(THIS_MODULE, "voice");
	if (IS_ERR(voice_class)) {
		ret = PTR_ERR(voice_class);
		pr_err("%s: error %d creating the voice class", SERVICE_NAME, ret);
		goto err_del_cdev;
	}

	for (i = 0; i < CONFIG_CORDLESS_NUM_VOICELINES; i++) {
		voice_sessions[i].enc_fifo = cfifo_alloc(dev, FIFO_SIZE, NULL, 0);
		voice_sessions[i].dec_fifo = cfifo_alloc(dev, FIFO_SIZE, NULL, 0);
		if (!voice_sessions[i].enc_fifo || !voice_sessions[i].dec_fifo) {
			ret = -ENOMEM;
			pr_err("%s: out of memory\n", SERVICE_NAME);
			goto err_cfifo_free;
		}
		mutex_init(&voice_sessions[i].sock_read_write_mutex);
		voice_dev[i] = device_create(voice_class, &platform_bus,
					     MKDEV(MAJOR(voice_chrdev), i),
					     NULL, "voice%d", i);
		if (IS_ERR(voice_dev[i])) {
			pr_err("%s: could not create device\n", SERVICE_NAME);
			goto err_cfifo_free;
		}
	}

#ifndef CONFIG_CORDLESS_USER_SPACE_FIFOS
	/* create sender thread */
	kmode_thread = kthread_run(voice_kmode_thread, NULL, "kvoice");
	if (IS_ERR(kmode_thread)) {
		kmode_thread = NULL;
		pr_err("%s: could not start kernel thread\n", SERVICE_NAME);
		goto err_cfifo_free;
	}
#endif

	ret = coma_register(SERVICE_NAME, FIFO_SIZE, NULL,
			    voice_process_message, voice_remove, voice_suspend);
	if (ret < 0) {
		pr_err("%s: registration failed: %d\n", SERVICE_NAME, ret);
		goto err_kthread_stop;
	}

	service_id = ret;
	voice_reg = 1;

	pr_info("%s: character device initialized (major=%d)\n",
	       SERVICE_NAME, MAJOR(voice_chrdev));

	ret = 0;
	goto out;

err_kthread_stop:
#ifndef CONFIG_CORDLESS_USER_SPACE_FIFOS
	kthread_stop(kmode_thread);
#endif
err_cfifo_free:
	for (i = 0; i < CONFIG_CORDLESS_NUM_VOICELINES; i++) {
		if (voice_sessions[i].enc_fifo) {
			cfifo_unref(voice_sessions[i].enc_fifo);
			voice_sessions[i].enc_fifo = NULL;
		}
		if (voice_sessions[i].dec_fifo) {
			cfifo_unref(voice_sessions[i].dec_fifo);
			voice_sessions[i].dec_fifo = NULL;
		}
		if (voice_dev[i]) {
			device_unregister(voice_dev[i]);
			voice_dev[i] = NULL;
		}
	}
	class_destroy(voice_class);

err_del_cdev:
	cdev_del(&voice_cdev);

err_unreg_chrdev_reg:
	unregister_chrdev_region(voice_chrdev, voice_numlines);
out:
	return ret;
}

EXPORT_SYMBOL(voice_init);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_LICENSE("GPL");
