/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14flt.h"
#include <linux/sched.h>

static struct kmem_cache *d14flt_msg_cache = NULL;

spinlock_t d14flt_msg_lock = SPIN_LOCK_UNLOCKED;
LIST_HEAD(d14flt_msg_list);

DECLARE_WAIT_QUEUE_HEAD(d14flt_msg_waitq);

static atomic_t current_msg_id = ATOMIC_INIT(0);


struct d14flt_msg *d14flt_alloc_msg(void)
{
	struct d14flt_msg *msg;

	msg = kmem_cache_alloc(d14flt_msg_cache, GFP_KERNEL);
	if (!msg)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&msg->list);
	msg->file = NULL;

	return msg;
}

void d14flt_free_msg(struct d14flt_msg *msg)
{
	if (!msg)
		return;

	if (msg->file)
		fput(msg->file);

	if (msg->m_xargs && (msg->m_xargs > (void *)msg) &&
			(msg->m_xargs < (void *)(((char *)msg) + sizeof(msg))))
		kfree(msg->m_xargs);

	kmem_cache_free(d14flt_msg_cache, msg);
}

void d14flt_queue_msg(struct d14flt_msg *msg)
{
	/* assigns the progressive id with atomic_inc_return(atomic_t *) */
	spin_lock(&d14flt_msg_lock);
	msg->m_head.m_id = atomic_inc_return(&current_msg_id);
	list_add_tail(&msg->list, &d14flt_msg_list);
	spin_unlock(&d14flt_msg_lock);
	wake_up(&d14flt_msg_waitq);
}

void d14flt_dequeue_msg(struct d14flt_msg *msg)
{
	spin_lock(&d14flt_msg_lock);
	list_del(&msg->list);
	spin_unlock(&d14flt_msg_lock);
}

struct d14flt_msg *d14flt_first_msg(void) {
	struct d14flt_msg *msg;

	wait_event_interruptible(d14flt_msg_waitq, !list_empty(&d14flt_msg_list));

	spin_lock(&d14flt_msg_lock);
	if (list_empty(&d14flt_msg_list)) {
		spin_unlock(&d14flt_msg_lock);
		return NULL;
	}

	msg = list_entry(d14flt_msg_list.next, struct d14flt_msg, list);

	spin_unlock(&d14flt_msg_lock);

	return msg;
}

int d14flt_msg_init(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
	d14flt_msg_cache = kmem_cache_create("d14flt_msg_cache", 
			sizeof(struct d14flt_msg), 0, 
			SLAB_RECLAIM_ACCOUNT, NULL, NULL);
#else
	d14flt_msg_cache = kmem_cache_create("d14flt_msg_cache",
			sizeof(struct d14flt_msg), 0,
			SLAB_RECLAIM_ACCOUNT, NULL);
#endif
	if (!d14flt_msg_cache)
		return -ENOMEM;

	return 0;
}

void d14flt_msg_exit(void)
{
	kmem_cache_destroy(d14flt_msg_cache);
}
