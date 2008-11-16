/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/uaccess.h>
#include "d14flt.h"

const struct d14flt_cmd d14flt_cmd_connect = {
		.cmd = D14FLT_DEV_CONNECT,
		.args.connect.magic = D14FLT_MAGIC,
		.args.connect.version = D14FLT_DEV_VERSION,
};

struct file_operations d14flt_fops;

static struct class *d14flt_class;
static struct class_device *d14flt_class_device;
static dev_t d14flt_dev;

int d14flt_flt_to_usr(struct d14usr_msg __user *usr_msg, struct d14flt_msg *flt_msg, size_t buf_size)
{
	size_t cur_size, ret_size;

	/********* move this to an ioctl (I won't do more checking) ******************************************/
	if (flt_msg->file) {
		usr_msg->m_fd = get_unused_fd();
		if (usr_msg->m_fd >= 0)
			fd_install(usr_msg->m_fd, flt_msg->file);
	} else {
		usr_msg->m_fd = -1;
	}

	usr_msg->m_size = sizeof(flt_msg->m_head)+flt_msg->m_args_len+flt_msg->m_xargs_len;

	ret_size = offsetof(struct d14usr_msg, m_head);

	cur_size = sizeof(flt_msg->m_head);
	if (buf_size < cur_size)
		cur_size = buf_size;
	if (copy_to_user(&usr_msg->m_head, &flt_msg->m_head, cur_size))
			return -EFAULT;
	ret_size += cur_size;

	cur_size = flt_msg->m_args_len;
	if (buf_size < ret_size+cur_size)
		cur_size = buf_size-ret_size;
	if (copy_to_user(&usr_msg->m_body, &flt_msg->m_body, cur_size))
			return -EFAULT;
	ret_size += cur_size;

	if (!flt_msg->m_xargs || !flt_msg->m_xargs_len)
		return ret_size;

	cur_size = flt_msg->m_xargs_len;
	if (buf_size < ret_size+cur_size)
		cur_size = buf_size-ret_size;
	if (copy_to_user((char *)&usr_msg->m_body+flt_msg->m_args_len, flt_msg->m_xargs, cur_size))
			return -EFAULT;
	ret_size += cur_size;

	return ret_size;
}

int d14flt_do_connect(struct d14flt_cmd *cmd)
{
	return 0;
}

int d14flt_do_dequeue(struct d14flt_cmd *cmd)
{
	struct d14flt_msg *flt_msg;

	flt_msg = d14flt_first_msg();
	if (flt_msg == NULL)
			return -EINVAL;

	if (flt_msg->m_head.m_id != cmd->args.dequeue.id)
		return -EINVAL;

	d14flt_dequeue_msg(flt_msg);
	d14flt_free_msg(flt_msg);

	return 0;
}

static int d14flt_dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int d14flt_dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t d14flt_dev_read(struct file *file, char __user *buf,
		size_t size, loff_t *pos)
{
	struct d14flt_msg *flt_msg;

	if (!buf)
		return -EINVAL;

	/* buffer not large enough for basic informations */
	if (size < offsetof(struct d14usr_msg, m_head))
		return -EINVAL;

	flt_msg = d14flt_first_msg();
	if (flt_msg == NULL)
		return 0;

	return d14flt_flt_to_usr((struct d14usr_msg *)buf, flt_msg, size);
}

static ssize_t d14flt_dev_write(struct file *file, const char __user *buf,
		size_t size, loff_t *pos)
{
	int rv;
	struct d14flt_cmd tmp_cmd;

	if (!buf)
		return -EINVAL;

	if (size != sizeof(tmp_cmd))
		return -EINVAL;

	if (copy_from_user(&tmp_cmd, buf, size))
		return -EFAULT;

	switch (tmp_cmd.cmd) {
		case D14FLT_DEV_CONNECT:
			rv = d14flt_do_connect(&tmp_cmd);
			if (rv)
				return rv;
			break;

		case D14FLT_DEV_DEQUEUE:
			rv = d14flt_do_dequeue(&tmp_cmd);
			if (rv)
				return rv;
			break;

		default:
			return -EINVAL;
	}

	return size;
}

int d14flt_dev_init(void)
{
	int major;

	major = register_chrdev(0, D14FLT_DEV_NAME, &d14flt_fops);
	if (major < 0)
		return major;

	d14flt_dev = MKDEV(major, 0);

	d14flt_class = class_create(THIS_MODULE, D14FLT_DEV_NAME);
	if (IS_ERR(d14flt_class)) {
		unregister_chrdev(major, D14FLT_DEV_NAME);
		return PTR_ERR(d14flt_class);
	}

	d14flt_class_device = class_device_create(d14flt_class, NULL, d14flt_dev, NULL, D14FLT_DEV_NAME);
	if (IS_ERR(d14flt_class_device)) {
		class_destroy(d14flt_class);
		unregister_chrdev(major, D14FLT_DEV_NAME);
		return PTR_ERR(d14flt_class_device);
	}

	return 0;
}

void d14flt_dev_exit(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
	int err;
#endif

	class_device_destroy(d14flt_class, d14flt_dev);
	class_destroy(d14flt_class);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
	err = unregister_chrdev(MAJOR(d14flt_dev), D14FLT_DEV_NAME);
	if (err)
		d14flt_err("unregister_chrdev failed(%d)\n", err);
#else
	unregister_chrdev(MAJOR(d14flt_dev), D14FLT_DEV_NAME);	
#endif
}

struct file_operations d14flt_fops = {
	.owner = THIS_MODULE,
	.open = d14flt_dev_open,
	.release = d14flt_dev_release,
	.read = d14flt_dev_read,
	.write = d14flt_dev_write,
/*
	.poll = d14flt_dev_poll,
*/
};
