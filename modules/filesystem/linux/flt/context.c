/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14flt.h"

struct kmem_cache *d14flt_i_data_cache;

void d14flt_i_data_fill(struct d14flt_i_data *data)
{
	atomic_set(&data->mmapped, 0);
	init_MUTEX(&data->msg_lock);
	data->msg = NULL;
}

void d14flt_i_data_free(struct rfs_priv_data *rfs_data)
{
	struct d14flt_i_data *data = rfs_to_d14flt_i_data(rfs_data);

	if (data->msg) {
		d14flt_free_msg(data->msg);
		d14flt_debug("modify message discarded");
	}

	kmem_cache_free(d14flt_i_data_cache, data);
}

struct d14flt_i_data *d14flt_i_data_alloc(void)
{
	struct d14flt_i_data *data;
	int rv;

	data = kmem_cache_alloc(d14flt_i_data_cache, GFP_KERNEL);
	if (!data)
		return NULL;

	rv = rfs_init_data(&data->rfs_data, d14flt_flt, d14flt_i_data_free);
	if (rv) {
		 kmem_cache_free(d14flt_i_data_cache, data);
		 return NULL;
	}

	d14flt_i_data_fill(data);

	return data;
}

void d14flt_i_data_put(struct d14flt_i_data *data)
{
	rfs_put_data(&data->rfs_data);
}

struct d14flt_i_data *d14flt_i_data_get(struct inode *inode, int create)
{
	struct d14flt_i_data *data;
	struct rfs_priv_data *rfs_data;
	int rv;

	rv = rfs_get_data_inode(d14flt_flt, inode, &rfs_data);
	if (!rv)
		return rfs_to_d14flt_i_data(rfs_data);

	if ((rv != -ENODATA) || (!create))
		return NULL;

	data = d14flt_i_data_alloc();
	if (IS_ERR(data))
		return NULL;

	rv = rfs_attach_data_inode(d14flt_flt, inode, &data->rfs_data, &rfs_data);
	if (rv) {
		if (rv == -EEXIST) {
			d14flt_i_data_put(data);
			data = rfs_to_d14flt_i_data(rfs_data);
		} else {
			d14flt_i_data_put(data);
			return NULL;
		}
	}

	return data;
}

void d14flt_i_msg_put(struct d14flt_i_data *data)
{
	up(&data->msg_lock);
	d14flt_i_data_put(data);
}

void d14flt_i_msg_queue(struct d14flt_i_data *data)
{
	d14flt_queue_msg(data->msg);
	d14flt_i_data_fill(data);
	d14flt_i_data_put(data);
}

struct d14flt_i_data *d14flt_i_msg_get(struct inode *inode, int create)
{
	struct d14flt_i_data *data;

	data = d14flt_i_data_get(inode, create);
	if (!data)
		return NULL;

	/***** what happens if the lock cannot be held???? ************/
	if(down_interruptible(&data->msg_lock)) {
		d14flt_i_data_put(data);
		return NULL;
	}

	if (!data->msg && create) {
		data->msg = d14flt_alloc_msg();
		data->msg->m_head.m_type = EVENT_TYPE_FS_DATA;
		data->msg->m_args_len = offsetof(struct d14flt_msg, m_body.m_data.inline_bitmap) -
			offsetof(struct d14flt_msg, m_body);
		data->msg->m_xargs = data->msg->m_body.m_data.inline_bitmap;
		data->msg->m_body.m_data.m_args.i_ino = inode->i_ino;
		data->msg->m_body.m_data.m_args.i_size = inode->i_size; /* not necessary */
		data->msg->m_body.m_data.m_args.start = -1;
		data->msg->m_body.m_data.m_args.end = -1;
		data->msg->m_body.m_data.m_args.bitmap_size = 0;
	}
	if (!data->msg) {
		up(&data->msg_lock);
		d14flt_i_data_put(data);
		return NULL;
	}

	return data;
}

/*
 * Initialization functions
 */

int d14flt_rfs_data_cache_init(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
	d14flt_i_data_cache = kmem_cache_create("d14flt_i_data_cache",
				sizeof(struct d14flt_i_data), 0, SLAB_RECLAIM_ACCOUNT,
				NULL, NULL);
#else
	d14flt_i_data_cache = kmem_cache_create("d14flt_i_data_cache",
				sizeof(struct d14flt_i_data), 0, SLAB_RECLAIM_ACCOUNT,
				NULL);
#endif
	if (!d14flt_i_data_cache)
		return -ENOMEM;

	return 0;
}

void d14flt_rfs_data_cache_exit(void)
{
	kmem_cache_destroy(d14flt_i_data_cache);
}
