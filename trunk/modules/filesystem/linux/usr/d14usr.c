/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 * NOTE: This code is for the sole testing of the kernel module. The
 *       final user mode daemon need to be rewritten from scratch.
 */

#include "d14usr.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stddef.h>
#include <sys/mman.h>

const struct d14flt_cmd d14flt_cmd_connect = {
		.cmd = D14FLT_DEV_CONNECT,
		.args.connect.magic = D14FLT_MAGIC,
		.args.connect.version = D14FLT_DEV_VERSION,
};

struct d14flt_con fltcon = { 0, 0 };

struct d14usr_msg *usr_buffer;
size_t usr_buffer_size;

D14FSMsg *msg_buffer;
size_t msg_buffer_size;

struct d14flt_cmd cmd_buffer_real;
struct d14flt_cmd *cmd_buffer = &cmd_buffer_real;
size_t cmd_buffer_size = sizeof(cmd_buffer_real);

int intr = 0;

void sighandler(int sig)
{
	intr = 1;
}

int main()
{
	size_t new_msg_buffer_size;
	struct sigaction sa;
	int rv;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGTERM);
	sigaddset(&sa.sa_mask, SIGINT);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);

	if (conv_init()) {
		printf("Locale conversion init failed\n");
		return 1;
	}

	rv = d14usr_opendev(&fltcon);
	if (rv) {
		printf("Open failed\n");
		return rv;
	}

	usr_buffer_size = 4096;
	usr_buffer = malloc(usr_buffer_size);

	while (!intr && usr_buffer) {
		rv = d14usr_read(&fltcon, usr_buffer, usr_buffer_size);
		if (rv == -1) {
			printf("Read failed\n");
			break;
		}
		if (rv == 0) {
			printf("Zero byte read\n");
			continue;
		}
		if (rv < usr_buffer->m_size+sizeof(usr_buffer->m_size)) {
			usr_buffer_size = usr_buffer->m_size+offsetof(struct d14usr_msg, m_head);
			printf("Enlarging buffer from %u to %u\n", usr_buffer->m_size, usr_buffer_size);
			free(usr_buffer);
			usr_buffer = malloc(usr_buffer_size);
			continue;
		}

		new_msg_buffer_size = msg_size(usr_buffer);
		if (new_msg_buffer_size == -1) {
			printf("Message size calculation failed, exiting\n");
			break;
		}
		if (new_msg_buffer_size > msg_buffer_size) {
			free(msg_buffer);
			msg_buffer = malloc(new_msg_buffer_size);
			if (!msg_buffer) {
				printf("Alloc failed, exiting\n");
				break;
			}
		}
		usr_to_msg(usr_buffer, msg_buffer);

		cmd_buffer->cmd = D14FLT_DEV_DEQUEUE;
		cmd_buffer->args.dequeue.id = usr_buffer->m_head.m_id;
		rv = d14usr_write(&fltcon, cmd_buffer, cmd_buffer_size);
		if (rv != cmd_buffer_size) {
			printf("Write failed, exiting\n");
			break;
		}
	}

	if (usr_buffer)
		free(usr_buffer);

	rv = d14usr_closedev(&fltcon);
	if (rv) {
		printf("Close failed\n");
		return rv;
	}

	conv_exit();

	return 0;
}
