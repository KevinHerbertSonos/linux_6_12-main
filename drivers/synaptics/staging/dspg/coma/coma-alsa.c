/*
 * drivers/staging/dspg/coma/coma-alsa.c - COMA ALSA implementation
 *
 * Using interface to Linux kernel is implemented that
 * allows sending ALSA PCM messages
 *
 * Copyright (C) 2016 DSP Group
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
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/coma/coma.h>
#include <linux/coma/coma-alsa.h>
#include <sound/asound.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <sound/snd-dvf.h>

#define SERVICE_NAME              "alsa"
/*
 * 6 devices each two substreams
 * PCMHD and TRACE units have only one stream
 */
#define NR_CSS_ALSA_DEVICES       (8 * 2)
#define FIFO_SIZE                 (8000 * NR_CSS_ALSA_DEVICES)
#define COMA_ALSA_TIMEOUT         (3 * HZ) /* 3 sec. */

#define ALSA_PCM_STREAM_CAPTURE        (1 << 0)
#define ALSA_PCM_STREAM_PLAYBACK       (1 << 1)

struct alsa_msg_open {
	uint8_t type;
	uint8_t id;
	uint32_t cookie;
	uint32_t rate;
	uint32_t sample_size;
	uint32_t period_size;
	uint32_t channels;
	void *rx_buffer_addr;
	uint32_t rx_buffer_size;
	void *tx_buffer_addr;
	uint32_t tx_buffer_size;
} __attribute__ ((__packed__));

struct alsa_any_msg {
	uint8_t type;
};

struct alsa_msg_start {
	uint8_t type;
	uint8_t id;
	uint8_t stream;
	uint32_t cookie;
} __attribute__ ((__packed__));

struct alsa_msg_stop {
	uint8_t type;
	uint8_t id;
	uint8_t stream;
} __attribute__ ((__packed__));

struct alsa_msg_reset {
	uint8_t type;
	uint8_t id;
	uint8_t stream;
	uint32_t cookie;
} __attribute__ ((__packed__));

struct alsa_msg_close {
	uint8_t type;
	uint8_t id;
	uint8_t stream;
	uint32_t cookie;
} __attribute__ ((__packed__));

struct alsa_elapsed {
	uint8_t type;
	uint8_t id;
	uint8_t stream;
} __attribute__ ((__packed__));

struct alsa_msg_ack {
	uint8_t type;
	uint32_t cookie;
} __attribute__ ((__packed__));

struct alsa_msg_nack {
	uint8_t type;
	uint32_t cookie;
	int32_t  reason;
}__attribute__ ((__packed__));

enum alsa_service_message_types {
	ALSA_PCM_OPEN           = 0,
	ALSA_PCM_CLOSE          = 1,
	ALSA_PCM_START          = 2,
	ALSA_PCM_STOP           = 3,
	ALSA_PCM_RESET          = 4,
	ALSA_PCM_ELAPSED        = 5,
	ALSA_PCM_ACK            = 6,
	ALSA_PCM_NACK           = 7,
};

struct elapsed_service_t {
	void (*fn)(unsigned long int);
	unsigned int id;
	unsigned long int data;
};

static DECLARE_WAIT_QUEUE_HEAD(coma_alsa_reply);
static struct elapsed_service_t elapsed_service[NR_CSS_ALSA_DEVICES];
static int service_id;
static struct list_head to_wait;
static DEFINE_MUTEX(lock);
static int suspended = 0;

struct css_reply {
	int done;
	unsigned int cookie;
	int reason;
	struct list_head next;
};

static int
alsa_stream_to_offset(int stream)
{
	return (stream == SNDRV_PCM_STREAM_PLAYBACK? 0: 1);
}

static int
css_stream_playback_offset(void)
{
	return 0;
}

static int
css_stream_capture_offset(void)
{
	return 1;
}

void
coma_alsa_register_elapsed_cb(unsigned int id,
			      int stream,
			      void (*fn)(unsigned long int),
			      unsigned long int data)
{
	int offset = (id * 2) + alsa_stream_to_offset(stream);

	mutex_lock(&lock);
	elapsed_service[offset].fn = fn;
	elapsed_service[offset].id = id;
	elapsed_service[offset].data = data;
	mutex_unlock(&lock);
}
EXPORT_SYMBOL_GPL(coma_alsa_register_elapsed_cb);

void
coma_alsa_deregister_elapsed_cb(unsigned int id, int stream)
{
	int offset = (id * 2) + alsa_stream_to_offset(stream);

	mutex_lock(&lock);
	elapsed_service[offset].fn = NULL;
	elapsed_service[offset].id = 0;
	elapsed_service[offset].data = 0;
	mutex_unlock(&lock);
}
EXPORT_SYMBOL_GPL(coma_alsa_deregister_elapsed_cb);

static int
coma_alsa_send_synced(void *buf, size_t size, struct css_reply *r)
{
	int ret;

	mutex_lock(&lock);
	list_add(&r->next, &to_wait);
	mutex_unlock(&lock);

	ret = coma_cmsg_send(service_id,
			     0,
			     NULL,
			     0,
			     (unsigned char *)buf,
			     size);
	if (ret < 0) {
		mutex_lock(&lock);
		list_del(&r->next);
		mutex_unlock(&lock);
		return ret;
	}

	ret = wait_event_timeout(coma_alsa_reply,
				 r->done != 0,
				 COMA_ALSA_TIMEOUT);
	if (ret == 0){
		mutex_lock(&lock);
		list_del(&r->next);
		mutex_unlock(&lock);
		ret = -ETIMEDOUT;
		goto out;
	}
	ret = 0;
out:
	return ret;
}

static int
coma_alsa_send_unsynced(void *buf, size_t size)
{
	return coma_cmsg_send(service_id,
			      0,
			      NULL,
			      0,
			      (unsigned char *)buf,
			      size);
}

int
coma_alsa_open(uint8_t id,
	       uint32_t rate,
	       uint32_t sample_size,
	       uint32_t period_size,
	       uint32_t channels,
	       void *rx_buffer_addr,
	       uint32_t rx_buffer_size,
	       void *tx_buffer_addr,
	       uint32_t tx_buffer_size)
{
	int ret;
	struct alsa_msg_open msg;
	struct css_reply cr;

	if (service_id < 0)
		return -EFAULT;

	memset((void *)&cr, 0, sizeof(cr));

	msg.type = ALSA_PCM_OPEN;
	msg.id = id;
	msg.rate = rate;
	msg.sample_size = sample_size;
	msg.period_size = period_size;
	msg.channels = channels;
	msg.rx_buffer_addr = rx_buffer_addr; /* for capture */
	msg.rx_buffer_size = rx_buffer_size;
	msg.tx_buffer_addr = tx_buffer_addr; /* for playback */
	msg.tx_buffer_size = tx_buffer_size;
	msg.cookie = (uint32_t)&cr;

	cr.cookie = msg.cookie;

	ret = coma_alsa_send_synced(&msg, sizeof(msg), &cr);
	if (ret < 0)
		goto out;

	ret = cr.reason;
out:
	return ret;
}
EXPORT_SYMBOL_GPL(coma_alsa_open);

static int
alsa_stream_to_css_stream(int stream)
{
	return (stream == SNDRV_PCM_STREAM_PLAYBACK? ALSA_PCM_STREAM_PLAYBACK: ALSA_PCM_STREAM_CAPTURE);
}

static int
coma_alsa_send_and_wait(int type, unsigned int id, int stream)
{
	int ret;
	struct alsa_msg_close msg;
	struct css_reply cr;

	if (service_id < 0)
		return -EFAULT;

	memset((void *)&cr, 0, sizeof(cr));

	msg.type = (uint8_t)type;
	msg.id = id;
	msg.stream = alsa_stream_to_css_stream(stream);
	msg.cookie = (uint32_t)&cr;

	cr.cookie = msg.cookie;

	ret = coma_alsa_send_synced(&msg, sizeof(msg), &cr);
	if (ret < 0)
		goto out;

	ret = cr.reason;
out:
	return ret;
}

int
coma_alsa_close(unsigned int id, int stream)
{
	return coma_alsa_send_and_wait(ALSA_PCM_CLOSE, id, stream);
}
EXPORT_SYMBOL_GPL(coma_alsa_close);

int
coma_alsa_start(unsigned int id, int stream)
{
	struct alsa_msg_start msg;

	if (service_id < 0)
		return -EFAULT;

	msg.type = ALSA_PCM_START;
	msg.id = id;
	msg.stream = alsa_stream_to_css_stream(stream);

	return coma_alsa_send_unsynced(&msg, sizeof(msg));
}
EXPORT_SYMBOL_GPL(coma_alsa_start);

int
coma_alsa_stop(unsigned int id, int stream)
{
	struct alsa_msg_stop msg;

	if (service_id < 0)
		return -EFAULT;

	msg.type = ALSA_PCM_STOP;
	msg.id = id;
	msg.stream = alsa_stream_to_css_stream(stream);

	return coma_alsa_send_unsynced(&msg, sizeof(msg));
}
EXPORT_SYMBOL_GPL(coma_alsa_stop);

int
coma_alsa_reset(unsigned int id, int stream)
{
	return coma_alsa_send_and_wait(ALSA_PCM_RESET, id, stream);
}
EXPORT_SYMBOL_GPL(coma_alsa_reset);

static int
css_stream_is_playback(int stream)
{
	return (stream & ALSA_PCM_STREAM_PLAYBACK? 1: 0);
}

static int
css_stream_is_capture(int stream)
{
	return (stream & ALSA_PCM_STREAM_CAPTURE? 1: 0);
}

static void
coma_alsa_process_message(void *arg, struct cmsg *cmsg)
{
	struct alsa_any_msg *result = (struct alsa_any_msg *)cmsg->body;
	struct list_head *p, *q;
	int reason = 0;

	switch (result->type) {
	case ALSA_PCM_NACK:
	{
		struct alsa_msg_nack *reply = (struct alsa_msg_nack *)cmsg->body;
		reason = reply->reason;
	}
	fallthrough;
	case ALSA_PCM_ACK:
	{
		struct alsa_msg_ack *reply = (struct alsa_msg_ack *)cmsg->body;
		struct css_reply *r;
		mutex_lock(&lock);
		list_for_each_safe(p, q, &to_wait) {
			r = list_entry(p, struct css_reply, next);
			if (r->cookie == reply->cookie) {
				r->reason = reason;
				list_del(p);
				r->done = 1;
				wake_up(&coma_alsa_reply);
			}
		}
		mutex_unlock(&lock);
		break;
	}
	case ALSA_PCM_ELAPSED:
	{
		struct alsa_elapsed *elapsed = (struct alsa_elapsed *)cmsg->body;
		int id = elapsed->id * 2;
		int offset;
		void (*pcb)(unsigned long int) = NULL;
		unsigned long int pdata = 0;
		void (*ccb)(unsigned long int) = NULL;
		unsigned long int cdata = 0;

		mutex_lock(&lock);
		offset = id + css_stream_playback_offset();
		if (css_stream_is_playback(elapsed->stream)) {
			pcb = elapsed_service[offset].fn;
			pdata = elapsed_service[offset].data;
		}
		offset = id + css_stream_capture_offset();
		if (css_stream_is_capture(elapsed->stream)) {
			ccb = elapsed_service[offset].fn;
			cdata = elapsed_service[offset].data;
		}
		mutex_unlock(&lock);
		if (pcb)
			pcb(pdata);
		if (ccb)
			ccb(cdata);
		break;
	}
	default:
		BUG_ON(1);
		break;
	}
}

static void
coma_alsa_remove(void *arg)
{
	if (service_id < 0)
		return;

	service_id = -1;

	if (!suspended) {
#ifdef CONFIG_DEBUG_FS
		/* XXX temporary workaround */
		msleep(200);
#endif
		dvf_snd_exit();
		dvf_snd_cmbs_exit();
		dvf_snd_fxs_exit();
		dvf_snd_bt_exit();
		board_dbmdx_snd_exit();
	}
}

static int coma_alsa_suspend(void)
{
	suspended = 1;

	return 0;
}

void
alsa_service_init(void)
{
	INIT_LIST_HEAD(&to_wait);

	service_id = coma_register(SERVICE_NAME,
				   FIFO_SIZE,
				   NULL,
				   coma_alsa_process_message,
				   coma_alsa_remove, coma_alsa_suspend);
	if (service_id < 0) {
		pr_err("%s: error: cannot register service\n", SERVICE_NAME);
		return;
	}

	if (!suspended) {
		dvf_snd_init();
		dvf_snd_cmbs_init();
		dvf_snd_fxs_init();
		dvf_snd_bt_init();
		board_dbmdx_snd_init();
	}
	suspended = 0;
}
