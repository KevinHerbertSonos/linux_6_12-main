/*
 * drivers/staging/dspg/coma/cfifo.c - cordless FIFOs
 *
 * Cordless FIFOs store variable-length messages in a ring buffer. This FIFO
 * implementation is safe for parallel accesses of not more than one reader and
 * one writer. Messages are stored in a consecutive memory area, i.e., they
 * are not wrapped around at the end of the FIFO as in many other ring buffer
 * implementations.
 *
 * Copyright (C) 2007 NXP Semiconductors
 * Copyright (C) 2008 - 2012 DSP Group Inc.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/coma/cfifo.h>

#ifdef CONFIG_CFIFO_CHECKS
static void dump_cfifo(struct cfifo *cfifo, int line)
{
	printk(KERN_ERR "%s:%d: cfifo broken: in = %d; out = %d; lastin = %d;"
			" lastout = %d; processed = %d\n",
		__FILE__, line, cfifo->shm->in, cfifo->shm->out,
		cfifo->lastin, cfifo->lastout, cfifo->processed);
}

#define CFIFO_CHECK(cfifo, condition)	({	\
	int __broken = !!(condition);		\
	if (__broken)				\
		dump_cfifo(cfifo, __LINE__);	\
	__broken;				\
})
#else
#define CFIFO_CHECK(cfifo, condition)	({ if (condition) ; 0; })
#endif

static void *cfifo_buffer(struct cfifo *cfifo)
{
	return cfifo->shm->buffer;
}

/*
 * cfifo_request - request a message buffer from the cfifo
 * @cfifo: the cfifo to be used.
 * @len: the length of the message to be added.
 *
 * After this call, the returned message buffer can be filled by the
 * application. A call to cfifo_commit() makes the message visible to readers.
 * If there is not enough room in the FIFO or if some other error is
 * encountered an error-valued pointer is returned.
 *
 * Note that with only one concurrent reader and one concurrent writer, you
 * don't need extra locking to use these functions.
 */
void *cfifo_request(struct cfifo *cfifo, unsigned int len)
{
	/* padding to distinguish empty from full state and to reserve space
	   for wraparound marker (msg_len == 0) at the end of the cfifo */
	struct cfifo_shm *shm = cfifo->shm;
	unsigned int off = len + 2 * CFIFO_MSGHEADER_SIZE;
	unsigned int in, out;

	/* cfifo->shm->out has be sampled once to avoid race condition */
	out = READ_ONCE(shm->out);

	/* Sanity check for all 4 buffer indices */
	if (CFIFO_CHECK(cfifo, shm->in >= cfifo->size || out >= cfifo->size ||
			cfifo->lastin >= cfifo->size ||
			cfifo->lastout >= cfifo->size))
		return ERR_PTR(-EPIPE);

	if (((shm->in >= out) && (off <= (cfifo->size - shm->in))) ||
		/* free space at end of cfifo: reserve additional space for
		 * wraparound marker:
		 *    XXXXXXXX________0000
		 *    ^       ^       ^
		 *    out     in      wraparound marker
		 */
	    ((shm->in <  out) && (off <= (out - shm->in)))) {
		/* free space at beginning/middle/end of cfifo
		 *    XXXXXXXX________XXXXXXXX0000
		 *            ^       ^
		 *            in      out
		 */
		in = shm->in;
	} else if ((shm->in >= out) && (off <= out)) {
		/* free space at beginning of cfifo
		 *    ________XXXXXXXX____
		 *            ^       ^
		 *            out     in
		 */
		in = 0;

		/* signal cfifo_get() that next message starts at
		   beginning of cfifo */
		memset(cfifo_buffer(cfifo) + shm->in, 0, CFIFO_MSGHEADER_SIZE);
	} else {
		/* no space available */
		return ERR_PTR(-ENOMEM);
	}

	cfifo->lastin = CFIFO_PAD(in + CFIFO_MSGHEADER_SIZE + len);
	if (CFIFO_CHECK(cfifo, cfifo->lastin > (cfifo->size - CFIFO_MSGHEADER_SIZE)))
		return ERR_PTR(-EPIPE);

	memcpy(cfifo_buffer(cfifo) + in, &len, CFIFO_MSGHEADER_SIZE);

	return cfifo_buffer(cfifo) + in + CFIFO_MSGHEADER_SIZE;
}
EXPORT_SYMBOL(cfifo_request);

/*
 * cfifo_commit - commit the previously requested message to the cfifo
 * @cfifo: the cfifo to be used.
 *
 * The message has already been copied to the cfifo after a
 * call to cfifo_request().
 *
 * On success, zero is returned, otherwise some negative error value.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
int cfifo_commit(struct cfifo *cfifo)
{
	if (CFIFO_CHECK(cfifo, cfifo->lastin > (cfifo->size - CFIFO_MSGHEADER_SIZE)))
		return -EPIPE;

	wmb();
	cfifo->shm->in = cfifo->lastin;

	return 0;
}
EXPORT_SYMBOL(cfifo_commit);

/**
 * cfifo_get - get one message from the cfifo
 * @cfifo: the cfifo to be used.
 * @buf: the pointer to the message.
 *
 * This function returns the address (in @buf) and length
 * of the next message in the cfifo (if any).
 *
 * On success, the size of the message is returned. If the cfifo is currently
 * empty, the function returns 0, otherwise some negative error value.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
int cfifo_get(struct cfifo *cfifo, void **buf)
{
	unsigned int in, out;
	unsigned int msg_len;

	if (CFIFO_CHECK(cfifo, !cfifo->processed))
		return -EINPROGRESS;

	/* cfifo->in has be sampled once to avoid race condition */
	in = READ_ONCE(cfifo->shm->in);
	out = cfifo->shm->out;

	/* Sanity check for all 4 buffer indices */
	if (CFIFO_CHECK(cfifo, in >= cfifo->size || out >= cfifo->size ||
			cfifo->lastin >= cfifo->size ||
			cfifo->lastout >= cfifo->size))
		return -EPIPE;

	if (in == out)
		return 0;

	/* this should not happen due to padding! */
	if (CFIFO_CHECK(cfifo, (cfifo->size - out) < CFIFO_MSGHEADER_SIZE))
		return -EPIPE;

	/*
	 * Zero length means cfifo->out is really at the beginning of the
	 * cfifo.
	 */
	msg_len = *(unsigned int *)(cfifo_buffer(cfifo) + out);
	if (msg_len == 0) {
		if (CFIFO_CHECK(cfifo, out == 0))
			return -EPIPE;
		/* this should not happen -> wrap around, but no message */
		if (CFIFO_CHECK(cfifo, in == 0))
			return -EPIPE;

		out = 0;
		msg_len = *(unsigned int *)cfifo_buffer(cfifo);

		/* this should not happen -> invalid message length */
		if (CFIFO_CHECK(cfifo, (msg_len == 0) ||
				(msg_len > cfifo->size - CFIFO_MSGHEADER_SIZE)))
			return -EPIPE;
	}

	out += CFIFO_MSGHEADER_SIZE;

	/* not enough space for message? */
	if (CFIFO_CHECK(cfifo, (in - out) < msg_len))
		return -EPIPE;

	*buf = cfifo_buffer(cfifo) + out;
	out += msg_len;

	/*
	 * Compute new out pointer, but do not yet increase it (this is done by
	 * cfifo_processed()).
	 */
	cfifo->lastout = CFIFO_PAD(out);
	cfifo->processed = 0;

	return msg_len;
}
EXPORT_SYMBOL(cfifo_get);

void cfifo_processed(struct cfifo *cfifo)
{
	CFIFO_CHECK(cfifo, cfifo->processed);

	/* now it is save to update cfifo->shm->out */
	mb();
	cfifo->shm->out = cfifo->lastout;
	cfifo->processed = 1;
}
EXPORT_SYMBOL(cfifo_processed);

/**
 * cfifo_reset - removes the entire cfifo contents
 * @cfifo: the cfifo to be emptied.
 */
void cfifo_reset(struct cfifo *cfifo)
{
	cfifo->shm->in = cfifo->shm->out = cfifo->lastout = cfifo->lastin = 0;
	cfifo->processed = 1;
	memset(cfifo_buffer(cfifo), 0, cfifo->size);
	mb();
}
EXPORT_SYMBOL(cfifo_reset);

/**
 * cfifo_empty - checks if cfifo is empty
 * @cfifo: the fifo to check
 *
 * returns 0 if not empty, any other value if empty
 */
int cfifo_empty(struct cfifo *cfifo)
{
	return (cfifo->shm->in == cfifo->shm->out);
}
EXPORT_SYMBOL(cfifo_empty);

/**
 * cfifo_alloc - allocate a CFIFO in consistent memory
 * @buffer_size: size of shared memory buffer
 *
 * Returns the pointer to the allocated cfifo structure or NULL if out of
 * memory.
 */
struct cfifo *cfifo_alloc(struct device *dev, unsigned int buffer_size, void *buffer, dma_addr_t phys)
{
	struct cfifo *cfifo;

	cfifo = kmalloc(sizeof(*cfifo), GFP_KERNEL);
	if (!cfifo)
		return NULL;

	cfifo->alloc_size = CFIFO_PAD(buffer_size);
	cfifo->size = cfifo->alloc_size - sizeof(struct cfifo_shm);
	cfifo->dev = dev;

	if (buffer && phys) {
		cfifo->shm = buffer;
		cfifo->phys = phys;
		cfifo->allocated = 0;
	} else {
		cfifo->shm = dma_alloc_coherent(dev, cfifo->alloc_size,
						&cfifo->phys, GFP_KERNEL);
		if (!cfifo->shm) {
			kfree(cfifo);
			return NULL;
		}
		cfifo->allocated = 1;
	}

	atomic_set(&cfifo->refs, 1);
	cfifo->shm->magic = CFIFO_MAGIC;
	cfifo->shm->size = cfifo->size;
	cfifo_reset(cfifo);

	return cfifo;
}
EXPORT_SYMBOL(cfifo_alloc);

void cfifo_ref(struct cfifo *cfifo)
{
	atomic_inc(&cfifo->refs);
}
EXPORT_SYMBOL(cfifo_ref);

void cfifo_unref(struct cfifo *cfifo)
{
	if (atomic_dec_and_test(&cfifo->refs)) {
		if (cfifo->allocated)
			dma_free_coherent(cfifo->dev, cfifo->alloc_size,
					  cfifo->shm, cfifo->phys);
		kfree(cfifo);
	}
}
EXPORT_SYMBOL(cfifo_unref);
