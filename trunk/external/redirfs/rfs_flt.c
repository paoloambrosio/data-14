/*
 * RedirFS: Redirecting File System
 * Written by Frantisek Hrbata <frantisek.hrbata@redirfs.org>
 *
 * Copyright (C) 2008 Frantisek Hrbata
 * All rights reserved.
 *
 * This file is part of RedirFS.
 *
 * RedirFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RedirFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RedirFS. If not, see <http://www.gnu.org/licenses/>.
 */

#include "rfs.h"

static LIST_HEAD(rfs_flt_list);
DEFINE_MUTEX(rfs_flt_list_mutex);

struct rfs_flt *rfs_flt_alloc(struct redirfs_filter_info *flt_info)
{
	struct rfs_flt *rflt;
	char *name;
	int len;
	
	len = strlen(flt_info->name);
	name = kzalloc(len + 1, GFP_KERNEL);
	if (!name)
		return ERR_PTR(-ENOMEM);

	strncpy(name, flt_info->name, len);

	rflt = kzalloc(sizeof(struct rfs_flt), GFP_KERNEL);
	if (!rflt) {
		kfree(name);
		return ERR_PTR(-ENOMEM);
	}

	INIT_LIST_HEAD(&rflt->list);
	rflt->name = name;
	rflt->priority = flt_info->priority;
	rflt->owner = flt_info->owner;
	rflt->ops = flt_info->ops;
	spin_lock_init(&rflt->lock);
	try_module_get(rflt->owner);

	if (flt_info->active)
		atomic_set(&rflt->active, 1);
	else
		atomic_set(&rflt->active, 0);

	kobject_init(&rflt->kobj, &rfs_flt_ktype);

	return rflt;
}

struct rfs_flt *rfs_flt_get(struct rfs_flt *rflt)
{
	if (!rflt || IS_ERR(rflt))
		return NULL;

	kobject_get(&rflt->kobj);

	return rflt;
}

void rfs_flt_put(struct rfs_flt *rflt)
{
	if (!rflt || IS_ERR(rflt))
		return;

	kobject_put(&rflt->kobj);
}

void rfs_flt_release(struct kobject *kobj)
{
	struct rfs_flt *rflt = rfs_kobj_to_rflt(kobj);

	kfree(rflt->name);
	kfree(rflt);
}

static int rfs_flt_exist(const char *name, int priority)
{
	struct rfs_flt *rflt;

	list_for_each_entry(rflt, &rfs_flt_list, list) {
		if (rflt->priority == priority)
			return 1;

		if (!strcmp(rflt->name, name))
			return 1;
	}

	return 0;
}

redirfs_filter redirfs_register_filter(struct redirfs_filter_info *info)
{
	struct rfs_flt *rflt;
	int rv;

	if (!info)
		return ERR_PTR(-EINVAL);

	mutex_lock(&rfs_flt_list_mutex);

	if (rfs_flt_exist(info->name, info->priority)) {
		mutex_unlock(&rfs_flt_list_mutex);
		return ERR_PTR(-EEXIST);
	}

	rflt = rfs_flt_alloc(info);
	if (IS_ERR(rflt)) {
		mutex_unlock(&rfs_flt_list_mutex);
		return (redirfs_filter)rflt;
	}

	rv = rfs_flt_sysfs_init(rflt);
	if (rv) {
		rfs_flt_put(rflt);
		return ERR_PTR(rv);
	}

	list_add_tail(&rflt->list, &rfs_flt_list);
	rfs_flt_get(rflt);

	mutex_unlock(&rfs_flt_list_mutex);

	return (redirfs_filter)rflt;
}

int redirfs_unregister_filter(redirfs_filter filter)
{
	struct rfs_flt *rflt = (struct rfs_flt *)filter;

	if (!rflt)
		return -EINVAL;

	spin_lock(&rflt->lock);
	if (atomic_read(&rflt->kobj.kref.refcount) < 2) {
		spin_unlock(&rflt->lock);
		return 0;
	}

	if (atomic_read(&rflt->kobj.kref.refcount) != 2) {
		spin_unlock(&rflt->lock);
		return -EBUSY;
	}
	rfs_flt_put(rflt);
	spin_unlock(&rflt->lock);

	mutex_lock(&rfs_flt_list_mutex);
	list_del_init(&rflt->list);
	mutex_unlock(&rfs_flt_list_mutex);

	module_put(rflt->owner);

	return 0;
}

void redirfs_delete_filter(redirfs_filter filter)
{
	struct rfs_flt *rflt = (struct rfs_flt *)filter;

	BUG_ON(atomic_read(&rflt->kobj.kref.refcount) != 1);

	rfs_flt_sysfs_exit(rflt);
	rfs_flt_put(rflt);
}

static int rfs_flt_set_ops(struct rfs_flt *rflt)
{
	struct rfs_root *rroot;
	struct rfs_info *rinfo;
	int rv;

	list_for_each_entry(rroot, &rfs_root_list, list) {
		if (rfs_chain_find(rroot->rinfo->rchain, rflt) == -1)
			continue;

		rinfo = rfs_info_alloc(rroot, rroot->rinfo->rchain);
		if (IS_ERR(rinfo))
			return PTR_ERR(rinfo);

		rv = rfs_info_reset(rroot->dentry, rinfo);
		if (rv) {
			rfs_info_put(rinfo);
			return rv;
		}

		rfs_info_put(rroot->rinfo);
		rroot->rinfo = rinfo;
	}

	return 0;
}

int redirfs_set_operations(redirfs_filter filter, struct redirfs_op_info ops[])
{
	struct rfs_flt *rflt = (struct rfs_flt *)filter;
	int i = 0;
	int rv = 0;

	if (!rflt)
		return -EINVAL;

	while (ops[i].op_id != REDIRFS_OP_END) {
		rflt->cbs[ops[i].op_id].pre_cb = ops[i].pre_cb;
		rflt->cbs[ops[i].op_id].post_cb = ops[i].post_cb;
		i++;
	}

	mutex_lock(&rfs_path_mutex);
	rv = rfs_flt_set_ops(rflt);
	mutex_unlock(&rfs_path_mutex);

	return rv;
}

int redirfs_activate_filter(redirfs_filter filter)
{
	struct rfs_flt *rflt = (struct rfs_flt *)filter;

	if (!rflt || IS_ERR(rflt))
		return -EINVAL;

	atomic_set(&rflt->active, 1);

	return 0;
}

int redirfs_deactivate_filter(redirfs_filter filter)
{
	struct rfs_flt *rflt = (struct rfs_flt *)filter;

	if (!rflt || IS_ERR(rflt))
		return -EINVAL;

	atomic_set(&rflt->active, 0);

	return 0;
}

EXPORT_SYMBOL(redirfs_register_filter);
EXPORT_SYMBOL(redirfs_unregister_filter);
EXPORT_SYMBOL(redirfs_delete_filter);
EXPORT_SYMBOL(redirfs_set_operations);
EXPORT_SYMBOL(redirfs_activate_filter);
EXPORT_SYMBOL(redirfs_deactivate_filter);

