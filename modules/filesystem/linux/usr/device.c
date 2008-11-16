/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14usr.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

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
