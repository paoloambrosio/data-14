/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/mm.h>
#include <linux/radix-tree.h>
#include "d14flt.h"

enum rfs_retv d14flt_i_create(rfs_context context, struct rfs_args *args)
{
	d14flt_on_create(args->args.i_create.dir, args->args.i_create.dentry, NULL);
	return RFS_CONTINUE;
}

enum rfs_retv d14flt_i_symlink(rfs_context context, struct rfs_args *args)
{
	d14flt_on_create(args->args.i_symlink.dir, args->args.i_symlink.dentry,
			args->args.i_symlink.oldname);
	return RFS_CONTINUE;
}

enum rfs_retv d14flt_i_mknod(rfs_context context, struct rfs_args *args)
{
	d14flt_on_create(args->args.i_mknod.dir, args->args.i_mknod.dentry, NULL);
	return RFS_CONTINUE;
}

enum rfs_retv d14flt_i_mkdir(rfs_context context, struct rfs_args *args)
{
	d14flt_on_create(args->args.i_mkdir.dir, args->args.i_mkdir.dentry, NULL);
	return RFS_CONTINUE;
}

enum rfs_retv d14flt_i_link(rfs_context context, struct rfs_args *args)
{
	d14flt_on_link(args->args.i_link.dir, args->args.i_link.dentry);
	return RFS_CONTINUE;
}

enum rfs_retv d14flt_i_rename(rfs_context context, struct rfs_args *args)
{
	d14flt_on_rename(args->args.i_rename.old_dir, args->args.i_rename.old_dentry,
			args->args.i_rename.new_dir, args->args.i_rename.new_dentry);
	return RFS_CONTINUE;
}

enum rfs_retv d14flt_i_unlink(rfs_context context, struct rfs_args *args)
{
	d14flt_on_delete(args->args.i_unlink.dir, args->args.i_unlink.dentry);
	return RFS_CONTINUE;
}

enum rfs_retv d14flt_i_rmdir(rfs_context context, struct rfs_args *args)
{
	d14flt_on_delete(args->args.i_rmdir.dir, args->args.i_rmdir.dentry);
	return RFS_CONTINUE;
}

enum rfs_retv d14flt_i_setattr(rfs_context context, struct rfs_args *args)
{
	struct d14flt_msg *msg;
	struct d14flt_i_data *data;

	if (args->args.i_setattr.iattr->ia_valid & (ATTR_MODE | ATTR_UID | ATTR_GID)) {
		msg = d14flt_alloc_msg();

		msg->m_head.m_type = EVENT_TYPE_FS_ATTR;
		msg->m_body.m_attr.m_args.i_ino
				= args->args.i_setattr.dentry->d_inode->i_ino;
		msg->m_body.m_attr.m_args.i_mode
				= args->args.i_setattr.dentry->d_inode->i_mode;
		msg->m_body.m_attr.m_args.i_uid
				= args->args.i_setattr.dentry->d_inode->i_uid;
		msg->m_body.m_attr.m_args.i_gid
				= args->args.i_setattr.dentry->d_inode->i_gid;
		msg->m_args_len = sizeof(msg->m_body.m_attr.m_args);
		msg->m_xargs_len = 0;
		msg->m_xargs = NULL;

		d14flt_queue_msg(msg);
	}

	if (args->args.i_setattr.iattr->ia_valid & ATTR_SIZE) {
		data = d14flt_i_msg_get(args->args.i_setattr.dentry->d_inode, 1);
		if (!data)
			return RFS_CONTINUE;
		d14flt_i_msg_truncate(data->msg, args->args.i_setattr.dentry->d_inode);
		/* queue the message if there is no writer opened */
		if (atomic_read(&args->args.i_setattr.dentry->d_inode->i_writecount) == 1) {
			d14flt_i_msg_queue(data);
		} else {
			d14flt_i_msg_put(data);
		}
	}
	return RFS_CONTINUE;
}

enum rfs_retv d14flt_f_write(rfs_context context, struct rfs_args *args)
{
	struct d14flt_i_data *data;

	data = d14flt_i_msg_get(args->args.f_write.file->f_dentry->d_inode, 1);
	if (!data)
		return RFS_CONTINUE;

	/* we must use the actual written size */
	if (args->retv.rv_ssize)
		d14flt_i_msg_write(data->msg, args->args.f_write.file->f_dentry->d_inode,
			*args->args.f_write.pos-args->retv.rv_ssize,
			*args->args.f_write.pos);

	d14flt_i_msg_put(data);

	return RFS_CONTINUE;
}

enum rfs_retv d14flt_f_mmap(rfs_context context, struct rfs_args *args)
{
	struct d14flt_i_data *data;

	if (!(args->args.f_mmap.file->f_mode & FMODE_WRITE))
			return RFS_CONTINUE;

	data = d14flt_i_msg_get(args->args.f_mmap.file->f_dentry->d_inode, 1);
	if (!data)
		return RFS_CONTINUE;

	atomic_set(&data->mmapped, 1);

	d14flt_i_msg_put(data);

	return RFS_CONTINUE;
}

enum rfs_retv d14flt_a_writepage(rfs_context context, struct rfs_args *args)
{
	struct d14flt_i_data *data;
	struct inode *inode = args->args.a_writepage.page->mapping->host;

	data = d14flt_i_msg_get(inode, 0);
	if (!data)
		return RFS_CONTINUE;

	if (!atomic_read(&data->mmapped)) {
		d14flt_i_msg_put(data);
		return RFS_CONTINUE;
	}

	d14flt_i_writepage(data->msg, args->args.a_writepage.page->index);
	if (!atomic_read(&inode->i_writecount) &&
				!radix_tree_tagged(&inode->i_mapping->page_tree, PAGECACHE_TAG_DIRTY)) {
		atomic_set(&data->mmapped, 0);
		d14flt_i_msg_queue(data);
	} else {
		d14flt_i_msg_put(data);
	}

	return RFS_CONTINUE;
}

enum rfs_retv d14flt_f_release(rfs_context context, struct rfs_args *args)
{
	struct d14flt_i_data *data;
	struct inode *inode = args->args.f_release.inode;
	struct file *file = args->args.f_release.file;

	if (!(file->f_mode & FMODE_WRITE))
		return RFS_CONTINUE;

	if (atomic_read(&inode->i_writecount) != 1)
		return RFS_CONTINUE;

	data = d14flt_i_msg_get(inode, 0);
	if (!data)
		return RFS_CONTINUE;

	/* update the final size before queuing the message
	 * (page writes cannot change inode or bitmap size) */
	data->msg->m_body.m_data.m_args.i_size = inode->i_size;
	data->msg->m_xargs_len = data->msg->m_body.m_data.m_args.bitmap_size;

	/* attach a file to the message if needed */
	if ((data->msg->m_body.m_data.m_args.start != -1) ||
			(data->msg->m_body.m_data.m_args.bitmap_size != 0))
		data->msg->file = dentry_open(file->f_dentry, file->f_vfsmnt, O_RDONLY);

	if (!atomic_read(&data->mmapped) ||
			!radix_tree_tagged(&inode->i_mapping->page_tree, PAGECACHE_TAG_DIRTY)) {
		d14flt_i_msg_queue(data);
	} else {
		d14flt_i_msg_put(data);
	}

	return RFS_CONTINUE;
}

struct rfs_op_info d14flt_op_info[] = {
		{RFS_DIR_IOP_CREATE, NULL, d14flt_i_create},
		{RFS_DIR_IOP_SYMLINK, NULL, d14flt_i_symlink},
		{RFS_DIR_IOP_MKNOD, NULL, d14flt_i_mknod},
		{RFS_DIR_IOP_MKDIR, NULL, d14flt_i_mkdir},
		{RFS_DIR_IOP_LINK, NULL, d14flt_i_link},
		{RFS_DIR_IOP_RENAME, NULL, d14flt_i_rename},
		{RFS_DIR_IOP_UNLINK, NULL, d14flt_i_unlink},
		{RFS_DIR_IOP_RMDIR, NULL, d14flt_i_rmdir},
		{RFS_REG_FOP_WRITE, NULL, d14flt_f_write},
		{RFS_REG_IOP_SETATTR, NULL, d14flt_i_setattr},
		{RFS_DIR_IOP_SETATTR, NULL, d14flt_i_setattr},
		{RFS_REG_FOP_RELEASE, NULL, d14flt_f_release},
		{RFS_REG_FOP_MMAP, NULL, d14flt_f_mmap},
		{RFS_REG_AOP_WRITEPAGE, NULL, d14flt_a_writepage},
		{RFS_OP_END, NULL, NULL}
};
