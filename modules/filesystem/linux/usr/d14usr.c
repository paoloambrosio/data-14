/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 * NOTE: This code is for the sole testing of the kernel module. The
 *       final user mode daemon need to be rewritten from scratch.
 */

#include "d14usr.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>

const struct d14flt_cmd d14flt_cmd_connect = {
		.cmd = D14FLT_DEV_CONNECT,
		.args.connect.magic = D14FLT_DEV_MAGIC,
		.args.connect.version = D14FLT_DEV_VERSION,
};

struct d14flt_con {
	int fd;
	int connected;
} fltcon = { 0, 0 };

struct d14usr_msg *msg_buffer;
size_t msg_buffer_size;

struct d14flt_cmd cmd_buffer_real;
struct d14flt_cmd *cmd_buffer = &cmd_buffer_real;
size_t cmd_buffer_size = sizeof(cmd_buffer_real);

int intr = 0;

void sighandler(int sig)
{
	intr = 1;
}

int d14usr_devconn(struct d14flt_con *fltcon) {
	int rv;

	rv = -1; /** try connecting **/

	if (rv == -1)
		return errno;

	fltcon->connected = 1;

	printf("Connected\n");

	return 0;
}

int d14usr_opendev(struct d14flt_con *fltcon)
{
	int fd;

	if (!fltcon)
		return EINVAL;

	fd = open("/dev/" D14FLT_DEV_NAME, O_RDWR);
	if (fd == -1)
		return errno;

	fltcon->fd = fd;
	fltcon->connected = 0;

	return 0;
}

int d14usr_closedev(struct d14flt_con *fltcon)
{
	if (!fltcon)
		return EINVAL;

	if (!close(fltcon->fd))
		return errno;

	fltcon->fd = 0;
	fltcon->connected = 0;

	return 0;
}

int d14usr_read(struct d14flt_con *fltcon, void *buffer, size_t size)
{
	int rv;

	if (!fltcon || !fltcon->fd)
		return EINVAL;

	if (!fltcon->connected) {
		rv = d14usr_devconn(fltcon);
		if (rv)
			return rv;
	}

	return read(fltcon->fd, buffer, size);
}

int d14usr_write(struct d14flt_con *fltcon, void *buffer, size_t size)
{
	int rv;

	if (!fltcon || !fltcon->fd)
		return EINVAL;

	if (!fltcon->connected) {
		rv = d14usr_devconn(fltcon);
		if (rv)
			return rv;
	}

	return write(fltcon->fd, buffer, size);
}

int main()
{
	int i;
	struct sigaction sa;
	int rv;
	char *type_str;
	char name[256];
	char *src;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGTERM);
	sigaddset(&sa.sa_mask, SIGINT);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);

	rv = d14usr_opendev(&fltcon);
	if (rv) {
		printf("Open failed\n");
		return rv;
	}

	msg_buffer_size = 4096;
	msg_buffer = malloc(msg_buffer_size);

	while (!intr && msg_buffer) {
		rv = d14usr_read(&fltcon, msg_buffer, msg_buffer_size);
		if (rv == -1) {
			printf("Read failed\n");
			break;
		}
		if (rv == 0) {
			printf("Zero byte read\n");
			continue;
		}
		if (rv < msg_buffer->m_size+sizeof(msg_buffer->m_size)) {
			msg_buffer_size = msg_buffer->m_size+offsetof(struct d14usr_msg, m_head);
			printf("Enlarging buffer from %u to %u\n", msg_buffer->m_size, msg_buffer_size);
			free(msg_buffer);
			msg_buffer = malloc(msg_buffer_size);
			continue;
		}

		printf("size: %lu, id: %d, type: %d ", (long unsigned int)msg_buffer->m_size, msg_buffer->m_head.m_id, msg_buffer->m_head.m_type);

		switch (msg_buffer->m_head.m_type) {
		case EVENT_TYPE_FS_CREATE:
			switch (msg_buffer->m_body.m_create.m_args.i_mode & 0xF000) {
			case 0xA000:
				type_str = "symlink";
				break;
			case 0x8000:
				type_str = "regular";
				break;
			case 0x6000:
				type_str = "block device";
				break;
			case 0x4000:
				type_str = "directory";
				break;
			case 0x2000:
				type_str = "character device";
				break;
			default:
				type_str = "unknown";
			}
			printf("**EVENT_TYPE_FS_CREATE**\n");
			printf("i_ino: %lu, pi_ino: %lu\n", msg_buffer->m_body.m_create.m_args.i_ino, msg_buffer->m_body.m_create.m_args.pi_ino);
			printf("i_rdev: %u, i_mode: %o, type: %s\n", (unsigned int)msg_buffer->m_body.m_create.m_args.i_rdev, msg_buffer->m_body.m_create.m_args.i_mode & 0x0FFF, type_str);
			printf("i_uid: %d, i_gid: %d\n", msg_buffer->m_body.m_create.m_args.i_uid, msg_buffer->m_body.m_create.m_args.i_gid);
			memcpy(name, msg_buffer->m_body.m_create.names, msg_buffer->m_body.m_create.m_args.d_name_len);
			name[msg_buffer->m_body.m_create.m_args.d_name_len]='\0';
			printf("name: %s\n", name);
			if (msg_buffer->m_body.m_create.m_args.s_name_len) {
				memcpy(name, &msg_buffer->m_body.m_create.names[msg_buffer->m_body.m_create.m_args.d_name_len], msg_buffer->m_body.m_create.m_args.s_name_len);
				name[msg_buffer->m_body.m_create.m_args.s_name_len]='\0';
				printf("symlink: %s\n", name);
			}
			break;
		case EVENT_TYPE_FS_LINK:
			printf("**EVENT_TYPE_FS_LINK**\n");
			printf("i_ino: %lu, pi_ino: %lu\n", msg_buffer->m_body.m_link.m_args.i_ino, msg_buffer->m_body.m_link.m_args.pi_ino);
			memcpy(name, msg_buffer->m_body.m_link.name, msg_buffer->m_body.m_link.m_args.d_name_len);
			name[msg_buffer->m_body.m_link.m_args.d_name_len]='\0';
			printf("name: %s\n", name);
			break;
		case EVENT_TYPE_FS_RENAME:
			printf("**EVENT_TYPE_FS_RENAME**\n");
			printf("old_i_ino: %lu, old_pi_ino: %lu\n", msg_buffer->m_body.m_rename.m_args.old_i_ino, msg_buffer->m_body.m_rename.m_args.old_pi_ino);
			printf("new_i_ino: %lu, new_pi_ino: %lu\n", msg_buffer->m_body.m_rename.m_args.new_i_ino, msg_buffer->m_body.m_rename.m_args.new_pi_ino);
			memcpy(name, msg_buffer->m_body.m_rename.names, msg_buffer->m_body.m_rename.m_args.old_d_name_len);
			name[msg_buffer->m_body.m_rename.m_args.old_d_name_len]='\0';
			printf("old_name: %s\n", name);
			memcpy(name, &msg_buffer->m_body.m_rename.names[msg_buffer->m_body.m_rename.m_args.old_d_name_len], msg_buffer->m_body.m_rename.m_args.new_d_name_len);
			name[msg_buffer->m_body.m_rename.m_args.new_d_name_len]='\0';
			printf("new_name: %s\n", name);
			break;
		case EVENT_TYPE_FS_DELETE:
			printf("**EVENT_TYPE_FS_DELETE**\n");
			printf("i_ino: %lu, pi_ino: %lu\n", msg_buffer->m_body.m_delete.m_args.i_ino, msg_buffer->m_body.m_delete.m_args.pi_ino);
			memcpy(name, msg_buffer->m_body.m_delete.name, msg_buffer->m_body.m_delete.m_args.d_name_len);
			name[msg_buffer->m_body.m_delete.m_args.d_name_len]='\0';
			printf("name: %s\n", name);
			break;
		case EVENT_TYPE_FS_ATTR:
			printf("**EVENT_TYPE_FS_ATTR**\n");
			printf("i_ino: %lu\n", msg_buffer->m_body.m_attr.m_args.i_ino);
			printf("i_mode: %o, i_uid: %d, i_gid: %d\n", msg_buffer->m_body.m_attr.m_args.i_mode & 0x0FFF, msg_buffer->m_body.m_attr.m_args.i_uid, msg_buffer->m_body.m_attr.m_args.i_gid);
			break;
		case EVENT_TYPE_FS_DATA:
			printf("**EVENT_TYPE_FS_DATA**\n");
			printf("i_ino: %lu\n", msg_buffer->m_body.m_data.m_args.i_ino);
			printf("range: %lld - %lld\n", msg_buffer->m_body.m_data.m_args.start, msg_buffer->m_body.m_data.m_args.end);
			printf("i_size: %lld, bitmap_size: %u\n", msg_buffer->m_body.m_data.m_args.i_size, msg_buffer->m_body.m_data.m_args.bitmap_size);
			for (i = 0; i < msg_buffer->m_body.m_data.m_args.bitmap_size; i++) {
				if (!(i % 32))
					printf("\n0x%08x: ", i);
				printf("%02x", msg_buffer->m_body.m_data.bitmap[i]);
			}
			printf("\n");
			printf("File descriptor: %d\n", msg_buffer->m_fd);
			if (msg_buffer->m_fd >= 0) {
				if ((src = mmap (0, msg_buffer->m_body.m_data.m_args.i_size, PROT_READ,
						MAP_SHARED, msg_buffer->m_fd, 0)) != (caddr_t) -1) {
					if (msg_buffer->m_body.m_data.m_args.start != -1) {
						printf("should copy range\n");
					} else {
						printf("should copy bitmap\n");
					}
					munmap(src, msg_buffer->m_body.m_data.m_args.i_size);
				} else {
					printf("mmap failed!\n");
				}
				close(msg_buffer->m_fd);
			}
			break;
		default:
			printf("Event type unknown\n");
		}

		cmd_buffer->cmd = D14FLT_DEV_DEQUEUE;
		cmd_buffer->args.dequeue.id = msg_buffer->m_head.m_id;
		rv = d14usr_write(&fltcon, cmd_buffer, cmd_buffer_size);
		if (rv != cmd_buffer_size) {
			printf("Write failed, exiting\n");
			break;
		}
	}

	if (msg_buffer)
		free(msg_buffer);

	rv = d14usr_closedev(&fltcon);
	if (rv) {
		printf("Close failed\n");
		return rv;
	}

	return 0;
}
