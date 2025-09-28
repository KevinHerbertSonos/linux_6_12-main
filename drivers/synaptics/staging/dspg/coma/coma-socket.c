/*
 * drivers/staging/dspg/coma/coma-socket.c - COMA socket family
 *
 *  Copyright (C) 2012 DSP Group Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/coma/coma_socket.h>
#include <linux/coma/coma.h>
#include <linux/coma/cfifo.h>
#include <net/sock.h>
#include <net/tcp_states.h>

/*
 * We must re-use the TCP socket states in sk_state. Deep down in the network
 * stack it is expected to use these states.
 */
enum coma_states {
	COMA_CONNECTED    = TCP_ESTABLISHED,
	COMA_DISCONNECTED = TCP_CLOSE
};

struct coma_sock_service {
	struct list_head node; /* list node for servies list */
	char name[16];
	int id;
	struct mutex lock;
	int users;
	struct list_head sockets;
};

struct coma_sock {
	struct sock sk; /* must be first member */
	struct coma_sock_service *service;
	struct list_head node;
};

static LIST_HEAD(services);
static DEFINE_MUTEX(services_mutex);

/*
 * All funtions below must obey the following locking order to avoid deadlocks:
 *
 * 1. socket lock
 * 2. services_mutex
 * 3. struct coma_sock_service.sockets_lock
 */

/*
 * service_remove - Service was closed by remote side
 *
 * This function will be called only if the service was terminated from the
 * other side or because the CSS was reloaded. It will always be called from
 * some other context.
 */
static void service_remove(void *arg)
{
	struct coma_sock_service *service = arg;
	struct coma_sock *coma_sock;

	/*
	 * Remove our service structure from the global list. Any new socket
	 * will have to do the coma_register() again.
	 */
	mutex_lock(&services_mutex);
	list_del(&service->node);

	/*
	 * Force close any remaining sockets if service was closed forcefully.
	 * If the service is closed gracefully the callback won't be called.
	 */
	mutex_lock(&service->lock);
	service->id = -1; /* mark as dead */
	list_for_each_entry(coma_sock, &service->sockets, node) {
		coma_sock->sk.sk_state = COMA_DISCONNECTED;
		coma_sock->sk.sk_err = ECONNRESET;
		coma_sock->sk.sk_error_report(&coma_sock->sk);
	}
	mutex_unlock(&service->lock);
	mutex_unlock(&services_mutex);
}

/*
 * service_process_message - Process message on a service
 *
 * Can be called anytime when the service is alive, even before coma_register()
 * returned. The core layer will call it from a workqueue and guarantees that
 * it will not be called after service_remove() or coma_deregister() was called
 * for the affected service.
 */
static void service_process_message(void *arg, struct cmsg *cmsg)
{
	int err, len = cmsg->payload_size;
	void *buf = cmsg_payload(cmsg);
	struct sk_buff *skb, *orig_skb;
	struct coma_sock_service *service = arg;
	struct list_head *node;

	if (unlikely(!len))
		return;

	orig_skb = alloc_skb(len, GFP_KERNEL);
	if (WARN_ON_ONCE(!orig_skb))
		return; /* FIXME: what to do? */

	memcpy(skb_put(orig_skb, len), buf, len);

	mutex_lock(&service->lock);
	list_for_each(node, &service->sockets) {
		struct coma_sock *coma_sock = container_of(node, struct coma_sock, node);

		skb = skb_clone(orig_skb, GFP_KERNEL);
		if (WARN_ON_ONCE(!skb)) {
			mutex_unlock(&service->lock);
			kfree_skb(orig_skb);
			return; /* FIXME: what to do? */
		}

		/* FIXME: use private queue if skb was dropped */
		err = sock_queue_rcv_skb(&coma_sock->sk, skb);
		if (WARN_ON_ONCE(err))
			kfree_skb(skb);
	}
	mutex_unlock(&service->lock);

	kfree_skb(orig_skb);
}

/*
 * service_connect - Connect to service and associate socket with it
 *
 * We will look in the list of already-connected services and just attach the
 * socket to it if we find the active service. Otherwise we try to register the
 * service on the CSS and put it into the global list if it succeeds.
 */
static int service_connect(struct sockaddr_coma *addr,
			   struct coma_sock *coma_sk)
{
	int ret = 0;
	struct coma_sock_service *service;

	mutex_lock(&services_mutex);

	/* already open? */
	list_for_each_entry(service, &services, node) {
		if (strncmp(service->name, addr->service, 16) == 0) {
			mutex_lock(&service->lock);
			service->users++;
			list_add_tail(&coma_sk->node, &service->sockets);
			goto done;
		}
	}

	service = kmalloc(sizeof(*service), GFP_KERNEL);
	if (!service) {
		mutex_unlock(&services_mutex);
		return -ENOMEM;
	}

	mutex_init(&service->lock);
	mutex_lock(&service->lock);
	strcpy(service->name, addr->service);
	service->users = 1;
	INIT_LIST_HEAD(&service->sockets);
	INIT_LIST_HEAD(&service->node);
	list_add_tail(&coma_sk->node, &service->sockets);

	/* TODO: make cfifo size configurable */
	ret = coma_register(service->name, 20480, service,
		service_process_message, service_remove, NULL);
	if (ret < 0) {
		list_del(&coma_sk->node);
		mutex_unlock(&service->lock);
		mutex_destroy(&service->lock);
		kfree(service);
		mutex_unlock(&services_mutex);
		return ret;
	}

	service->id = ret;
	list_add_tail(&service->node, &services);

done:
	/* we're up and running */
	coma_sk->sk.sk_state = COMA_CONNECTED;
	coma_sk->service = service;
	mutex_unlock(&service->lock);
	mutex_unlock(&services_mutex);
	return 0;
}

/*
 * service_release - Detach socket from service
 *
 * Disassociates the socket @coma_sk from its service. If the socket was the
 * last one and the service was still alive we will deregister it from the CSS.
 * If service_remove() was already called we only have to free the service
 * structure if we were the last user.
 */
static void service_release(struct coma_sock *coma_sk)
{
	struct coma_sock_service *service = coma_sk->service;

	mutex_lock(&services_mutex);
	mutex_lock(&service->lock);

	/*
	 * Must de-register the service first to make sure that the process callback
	 * will not be called anymore.
	 * */
	service->users--;
	if (service->users == 0 && service->id >= 0)
		list_del(&service->node);

	list_del_init(&coma_sk->node);
	coma_sk->service = NULL;

	mutex_unlock(&service->lock);

	if (service->users == 0) {
		if (service->id >= 0)
			coma_deregister(service->id);
		mutex_destroy(&service->lock);
		kfree(service);
	}

	mutex_unlock(&services_mutex);
}


static int coma_sock_connect(struct socket *sock, struct sockaddr *uaddr,
			     int addr_len, int flags)
{
	struct sock *sk = sock->sk;
	struct coma_sock *coma_sk = container_of(sk, struct coma_sock, sk);
	int err;

	if (uaddr->sa_family != AF_COMA)
		return -EAFNOSUPPORT;

	if (addr_len != sizeof(struct sockaddr_coma))
		return -EINVAL;

	lock_sock(sk);

	switch (sock->state) {
	case SS_UNCONNECTED:
		BUG_ON(sk->sk_state != COMA_DISCONNECTED);
		break;
	case SS_CONNECTED:
		if (sk->sk_state != COMA_DISCONNECTED) {
			err = -EISCONN;
			goto out;
		} else {
			/* allow to reconnect when connection was interrupted */
			sock->state = SS_UNCONNECTED;
			service_release(coma_sk); /* release dead service */
		}
		break;
	case SS_DISCONNECTING:
	case SS_FREE:
	case SS_CONNECTING:
		BUG(); /*Should never happen */
		break;
	}

	/* Request the service and link us as one of the active sockets */
	err = service_connect((struct sockaddr_coma *)uaddr, coma_sk);
	if (!err)
		sock->state = SS_CONNECTED;

out:
	release_sock(sk);
	return err;
}

static int coma_sock_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct coma_sock *coma_sk = container_of(sk, struct coma_sock, sk);

	if (!sk)
		return 0;

	sock->sk = NULL;

	lock_sock(sk);

	sock_set_flag(sk, SOCK_DEAD);
	if (sock->state == SS_CONNECTED)
		service_release(coma_sk);
	coma_sk->sk.sk_socket->state = SS_UNCONNECTED;
	coma_sk->sk.sk_state = COMA_DISCONNECTED;
	wake_up_interruptible_poll(sk_sleep(sk), POLLERR|POLLHUP);

	sock_orphan(sk);
	release_sock(sk);
	sock_put(sk);

	return 0;
}

static unsigned int coma_sock_poll(struct file *file,
				   struct socket *sock, poll_table *wait)
{
	struct sock *sk = sock->sk;
	unsigned int mask = 0;

	sock_poll_wait(file, sock, wait);

	/* exceptional events? */
	if (sk->sk_err)
		mask |= POLLERR;

	/* readable? */
	if (!skb_queue_empty(&sk->sk_receive_queue))
		mask |= POLLIN | POLLRDNORM;

	/* writable? */
	/*
	 * FIXME: we are not using the transmit queue of the socket. Instead look
	 * in the CFIFO if there is enough room.
	 */
	if (sock_writeable(sk))
		mask |= POLLOUT | POLLWRNORM | POLLWRBAND;

	return mask;
}

static int coma_sock_sendmsg(struct socket *sock, struct msghdr *msg,
			     size_t len)
{
	struct sock *sk = sock->sk;
	struct coma_sock *coma_sk = container_of(sk, struct coma_sock, sk);
	struct cmsg *cmsg;
	int ret;

	ret = sock_error(sk);
	if (ret)
		return ret;

	if (unlikely(msg->msg_flags & MSG_OOB))
		return -EOPNOTSUPP;

	if (unlikely(msg->msg_namelen))
		return -EOPNOTSUPP;

	if (unlikely(msg->msg_iter.kvec->iov_base == NULL))
		return -EINVAL;

	mutex_lock(&coma_sk->service->lock);

	/* our socket state is protected by the service lock too */
	if (unlikely(sk->sk_state != COMA_CONNECTED)) {
		ret = -ECONNRESET;
		goto out;
	}

	cmsg = coma_cmsg_alloc(coma_sk->service->id, coma_sk->service->id, 0, len);
	if (IS_ERR(cmsg)) {
		ret = PTR_ERR(cmsg);
		goto out;
	}
	ret = copy_from_iter(cmsg_payload(cmsg), len, &msg->msg_iter);
	if (ret == len)
		ret = coma_cmsg_commit(coma_sk->service->id);
	else
		coma_cmsg_abort(coma_sk->service->id);

out:
	mutex_unlock(&coma_sk->service->lock);
	return (ret >= 0) ? len : ret;
}

static int coma_sock_recvmsg(struct socket *sock,
			     struct msghdr *m, size_t len, int flags)

{
	struct sock *sk = sock->sk;
	struct sk_buff *skb;
	int ret = 0;
	int copylen;

	if (unlikely(m->msg_flags & MSG_OOB))
		return -EOPNOTSUPP;

	skb = skb_recv_datagram(sk, flags, 0, &ret);
	if (!skb)
		return ret;

	copylen = skb->len;
	if (len < copylen) {
		m->msg_flags |= MSG_TRUNC;
		copylen = len;
	}

	ret = skb_copy_datagram_msg(skb, 0, m, copylen);
	if (ret)
		goto out;

	ret = skb->len;
out:
	skb_free_datagram(sk, skb);
	return ret;
}

static const struct proto_ops coma_seqpacket_ops = {
	.family = PF_COMA,
	.owner = THIS_MODULE,
	.release = coma_sock_release,
	.bind = sock_no_bind,
	.connect = coma_sock_connect,
	.socketpair = sock_no_socketpair,
	.accept = sock_no_accept,
	.getname = sock_no_getname,
	.poll = coma_sock_poll,
	.ioctl = sock_no_ioctl,
	.listen = sock_no_listen,
	.shutdown = sock_no_shutdown,
	.sendmsg = coma_sock_sendmsg,
	.recvmsg = coma_sock_recvmsg,
	.mmap = sock_no_mmap,
	.sendpage = sock_no_sendpage,
};

static void coma_socket_destruct(struct sock *sk)
{
	skb_queue_purge(&sk->sk_receive_queue);

	if (!sock_flag(sk, SOCK_DEAD)) {
		printk(KERN_ERR "Freeing alive netlink socket %p\n", sk);
		return;
	}

	WARN_ON(atomic_read(&sk->sk_rmem_alloc));
	WARN_ON(refcount_read(&sk->sk_wmem_alloc));
}

static int coma_socket_create(struct net *net, struct socket *sock, int protocol,
			      int kern)
{
	struct sock *sk = NULL;
	static struct proto prot = {
		.name = "PF_COMA",
		.owner = THIS_MODULE,
		.obj_size = sizeof(struct coma_sock),
	};

	/*
	 * The sock->type specifies the socket type to use.
	 * The CAIF socket is a packet stream in the sense
	 * that it is packet based. CAIF trusts the reliability
	 * of the link, no resending is implemented.
	 */
	if (sock->type == SOCK_SEQPACKET)
		sock->ops = &coma_seqpacket_ops;
	else
		return -ESOCKTNOSUPPORT;

	if (protocol != 0)
		return -EPROTONOSUPPORT;

	/*
	 * Set the socket state to unconnected.	 The socket state
	 * is really not used at all in the net/core or socket.c but the
	 * initialization makes sure that sock->state is not uninitialized.
	 */
	sk = sk_alloc(net, PF_COMA, GFP_KERNEL, &prot, kern);
	if (!sk)
		return -ENOMEM;

	sock_init_data(sock, sk);
	sk->sk_rcvbuf = 1UL << 30UL; /* 1GiB */
	sk->sk_protocol = (unsigned char) protocol;
	sk->sk_destruct = coma_socket_destruct;

	return 0;
}


static struct net_proto_family coma_family_ops = {
	.family = PF_COMA,
	.create = coma_socket_create,
	.owner = THIS_MODULE,
};

static int __init coma_sktinit_module(void)
{
	return sock_register(&coma_family_ops);
}

static void __exit coma_sktexit_module(void)
{
	sock_unregister(PF_COMA);
}
module_init(coma_sktinit_module);
module_exit(coma_sktexit_module);
