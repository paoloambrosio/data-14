/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14flt.h"

/**
 * Called on file (regular, directory, ...) creation
 * @param dir Parent inode
 * @param dentry Created file dentry
 * @param oldname Symlink target name (NULL otherwise)
 */

void d14flt_on_create(struct inode *dir, struct dentry *dentry, const char *oldname)
{
	struct d14flt_msg *msg;

	msg = d14flt_alloc_msg();

	msg->m_head.m_type = EVENT_TYPE_FS_CREATE;
	msg->m_body.m_create.m_args.i_ino = dentry->d_inode->i_ino;
	msg->m_body.m_create.m_args.pi_ino = dir->i_ino;
	msg->m_body.m_create.m_args.i_rdev = dentry->d_inode->i_rdev;
	msg->m_body.m_create.m_args.i_mode = dentry->d_inode->i_mode;
	msg->m_body.m_create.m_args.i_uid = dentry->d_inode->i_uid;
	msg->m_body.m_create.m_args.i_gid = dentry->d_inode->i_gid;
	msg->m_args_len = offsetof(struct d14flt_msg, m_body.m_create.inline_names) -
		offsetof(struct d14flt_msg, m_body);
	msg->m_body.m_create.m_args.d_name_len = dentry->d_name.len;
	if (oldname)
		msg->m_body.m_create.m_args.s_name_len = strlen(oldname);
	else
		msg->m_body.m_create.m_args.s_name_len = 0;
	msg->m_xargs_len = msg->m_body.m_create.m_args.d_name_len+msg->m_body.m_create.m_args.s_name_len;
	if (msg->m_xargs_len > sizeof(struct d14flt_msg) - offsetof(struct d14flt_msg, m_body.m_create.inline_names))
		msg->m_xargs = kmalloc(msg->m_xargs_len, GFP_KERNEL);
	else
		msg->m_xargs = msg->m_body.m_create.inline_names;
	memcpy(msg->m_xargs, dentry->d_name.name, msg->m_body.m_create.m_args.d_name_len);
	if (oldname)
		memcpy((char *)msg->m_xargs+msg->m_body.m_create.m_args.d_name_len, oldname, msg->m_body.m_create.m_args.s_name_len);

	d14flt_queue_msg(msg);
}

/**
 * Called on hard link creation
 * @param dir Parent inode
 * @param dentry Hard link dentry
 */

void d14flt_on_link(struct inode *dir, struct dentry *dentry)
{
	struct d14flt_msg *msg;

	msg = d14flt_alloc_msg();

	msg->m_head.m_type = EVENT_TYPE_FS_LINK;
	msg->m_body.m_link.m_args.i_ino = dentry->d_inode->i_ino;
	msg->m_body.m_link.m_args.pi_ino = dir->i_ino;
	msg->m_args_len = offsetof(struct d14flt_msg, m_body.m_link.inline_name) -
		offsetof(struct d14flt_msg, m_body);
	msg->m_body.m_link.m_args.d_name_len = dentry->d_name.len;
	msg->m_xargs_len = msg->m_body.m_link.m_args.d_name_len;
	if (msg->m_xargs_len > sizeof(struct d14flt_msg) - offsetof(struct d14flt_msg, m_body.m_link.inline_name))
		msg->m_xargs = kmalloc(msg->m_xargs_len, GFP_KERNEL);
	else
		msg->m_xargs = msg->m_body.m_link.inline_name;
	memcpy(msg->m_xargs, dentry->d_name.name, msg->m_body.m_link.m_args.d_name_len);

	d14flt_queue_msg(msg);
}

/**
 * Called on file rename
 * @param old_dir Old parent directory inode
 * @param old_dentry Old file dentry
 * @param new_dir New parent directory inode
 * @param new_dentry New file dentry
 */

void d14flt_on_rename(struct inode *old_dir, struct dentry *old_dentry,
		struct inode *new_dir, struct dentry *new_dentry)
{
	struct d14flt_msg *msg;

	msg = d14flt_alloc_msg();

	msg->m_head.m_type = EVENT_TYPE_FS_RENAME;
	msg->m_body.m_rename.m_args.old_i_ino = old_dentry->d_inode ? old_dentry->d_inode->i_ino : 0;
	msg->m_body.m_rename.m_args.old_pi_ino = old_dir->i_ino;
	msg->m_body.m_rename.m_args.new_i_ino = new_dentry->d_inode ? new_dentry->d_inode->i_ino : 0;
	msg->m_body.m_rename.m_args.new_pi_ino = new_dir->i_ino;
	msg->m_args_len = offsetof(struct d14flt_msg, m_body.m_rename.inline_names) -
		offsetof(struct d14flt_msg, m_body);
	msg->m_body.m_rename.m_args.old_d_name_len = old_dentry->d_name.len;
	msg->m_body.m_rename.m_args.new_d_name_len = new_dentry->d_name.len;
	msg->m_xargs_len = msg->m_body.m_rename.m_args.old_d_name_len+msg->m_body.m_rename.m_args.new_d_name_len;
	if (msg->m_xargs_len > sizeof(struct d14flt_msg) - offsetof(struct d14flt_msg, m_body.m_rename.inline_names))
		msg->m_xargs = kmalloc(msg->m_xargs_len, GFP_KERNEL);
	else
		msg->m_xargs = msg->m_body.m_rename.inline_names;
	memcpy(msg->m_xargs, old_dentry->d_name.name, msg->m_body.m_rename.m_args.old_d_name_len);
	memcpy((char *)msg->m_xargs+msg->m_body.m_rename.m_args.old_d_name_len, new_dentry->d_name.name, msg->m_body.m_rename.m_args.new_d_name_len);

	d14flt_queue_msg(msg);
}

/**
 * Called on file deletion (regular, directory, ...)
 * @param dir Parent inode
 * @param dentry Removed file
 */

void d14flt_on_delete(struct inode *dir, struct dentry *dentry)
{
	struct d14flt_msg *msg;

	msg = d14flt_alloc_msg();

	msg->m_head.m_type = EVENT_TYPE_FS_DELETE;
	msg->m_body.m_delete.m_args.i_ino = dentry->d_inode->i_ino;
	msg->m_body.m_delete.m_args.pi_ino = dir->i_ino;
	msg->m_args_len = offsetof(struct d14flt_msg, m_body.m_delete.inline_name) -
		offsetof(struct d14flt_msg, m_body);
	msg->m_body.m_delete.m_args.d_name_len = dentry->d_name.len;
	msg->m_xargs_len = msg->m_body.m_delete.m_args.d_name_len;
	if (msg->m_xargs_len > sizeof(struct d14flt_msg) - offsetof(struct d14flt_msg, m_body.m_delete.inline_name))
		msg->m_xargs = kmalloc(msg->m_xargs_len, GFP_KERNEL);
	else
		msg->m_xargs = msg->m_body.m_delete.inline_name;
	memcpy(msg->m_xargs, dentry->d_name.name, msg->m_body.m_delete.m_args.d_name_len);

	d14flt_queue_msg(msg);
}
