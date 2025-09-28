/*
 * drivers/staging/dspg/coma/coma-tdm.c - cordless TDM kernel interface
 *
 *  Copyright (C) 2010 - 2016 DSP Group
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

#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/coma/coma.h>
#include <linux/coma/cfifo.h>
#include <linux/coma/coma-tdm.h>

#define SERVICE_NAME "tdm"
#define CFIFO_SIZE   1024

#define COMA_TDM_TIMEOUT (1 * HZ) /* 1 sec. */

struct tdm_msg_grant {
	uint32_t type;
	uint32_t id;
	uint32_t cookie;
	uint32_t channels;
	uint32_t sample_size;
	uint32_t rate;
} __attribute__ ((__packed__));

struct tdm_msg_revoke {
	uint32_t type;
	uint32_t id;
	uint32_t cookie;
} __attribute__ ((__packed__));


struct tdm_msg_ack {
	uint32_t type;
	uint32_t cookie;
} __attribute__ ((__packed__));

struct tdm_msg_nack {
	uint32_t type;
	uint32_t cookie;
	int32_t  reason;
} __attribute__ ((__packed__));

enum tdm_service_message_types {
	TDM_GRANT = 0,
	TDM_REVOKE = 1,
	TDM_ACK = 2,
	TDM_NACK = 3,
};

static DECLARE_COMPLETION(coma_tdm_reply);
static DEFINE_MUTEX(coma_tdm_mutex);

static int initialized = 0;
static int service_id;

static int coma_tdm_send_synced(void *buf, size_t size)
{
	int ret;

	mutex_lock(&coma_tdm_mutex);

	ret = coma_cmsg_send(service_id, service_id, NULL, 0,
	                          (unsigned char *)buf, size);
	if (ret < 0) {
		pr_err("%s: failed sending message (type=%u)\n",
		       SERVICE_NAME, *(uint32_t *)buf);
		mutex_unlock(&coma_tdm_mutex);
		return ret;
	}

	ret = wait_for_completion_timeout(&coma_tdm_reply, COMA_TDM_TIMEOUT);
	if (ret < 0)
		pr_err("%s: ack expected, but timed out\n", SERVICE_NAME);
	else
		ret = 0;

	mutex_unlock(&coma_tdm_mutex);
	return ret;
}

int coma_tdm_grant(unsigned int id, unsigned int rate, unsigned int channels,
		   unsigned int sample_size)
{
	int ret;
	struct tdm_msg_grant msg;

	if (!initialized) {
		pr_err("%s: granting but not initialized\n", SERVICE_NAME);
		return -EFAULT;
	}

	msg.type 	= TDM_GRANT;
	msg.id 		= id;
	msg.rate 	= rate;
	msg.channels 	= channels;
	msg.sample_size = sample_size;
	msg.cookie	= 0; /* future use */

	pr_debug("%s: granting tdm %u, rate %d, channels %d, sample_size %d\n",
		 SERVICE_NAME, id, rate, channels, sample_size);

	ret = coma_tdm_send_synced(&msg, sizeof(msg));
	if (ret < 0)
		pr_err("%s: granting tdm %u failed (%d)\n",
		       SERVICE_NAME, id, ret);
	return ret;
}
EXPORT_SYMBOL(coma_tdm_grant);

int coma_tdm_revoke(unsigned int id)
{
	int ret;
	struct tdm_msg_revoke msg;

	if (!initialized) {
		pr_err("%s: revoking but not initialized\n", SERVICE_NAME);
		return -EFAULT;
	}

	msg.type = TDM_REVOKE;
	msg.id = id;
	msg.cookie = 0; /* for future use */

	pr_debug("%s: revoking tdm %u\n", SERVICE_NAME, id);

	ret = coma_tdm_send_synced(&msg, sizeof(msg));
	if (ret < 0)
		pr_err("%s: revoking tdm %u failed (%d)\n",
		       SERVICE_NAME, id, ret);
	return ret;
}
EXPORT_SYMBOL(coma_tdm_revoke);

static void coma_tdm_process_message(void *arg, struct cmsg *cmsg)
{
	struct tdm_msg_ack *ack = cmsg_payload(cmsg);

	pr_debug("%s: cmsg->type %d, cmsg->payload_size %d, ack->type %d\n",
		 __func__, cmsg->type, cmsg->payload_size, ack->type);

	BUG_ON(cmsg->payload_size != sizeof(*ack) &&
	       cmsg->payload_size != sizeof(struct tdm_msg_nack));
	BUG_ON(((ack->type != TDM_ACK) && (ack->type != TDM_NACK)));

	/* do not continue in case of nack, this should be debugged */
	if (ack->type == TDM_NACK) {
		struct tdm_msg_nack *nack = cmsg_payload(cmsg);
		pr_err("%s: received nack, reason id %d \n", SERVICE_NAME,
		       nack->reason);
		BUG_ON(1);
	}
	complete(&coma_tdm_reply);
}

static void coma_tdm_remove(void *arg)
{
	pr_debug("%s()\n", __func__);
	initialized = 0;
}

void tdm_service_init(void)
{
	service_id = coma_register(SERVICE_NAME, CFIFO_SIZE, NULL,
	                           coma_tdm_process_message, coma_tdm_remove,
				   NULL);
	if (service_id < 0)
		pr_err("%s: error: cannot register service\n", SERVICE_NAME);
	else
		initialized = 1;

	mutex_init(&coma_tdm_mutex);
}
