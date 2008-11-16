/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include "d14flt.h"

char *d14flt_privdir = ".d14";

static int __init d14flt_init(void)
{
	int rv;

	rv = d14flt_msg_init();
	if (rv)
		return rv;

	rv = d14flt_rfs_init();
	if (rv)
		goto err_cache;

	rv = d14flt_dev_init();
	if (rv)
		goto err_rfs;

	return 0;

err_rfs:
	d14flt_rfs_exit();
err_cache:
	d14flt_msg_exit();

	return rv;
}

static void __exit d14flt_exit(void)
{
	d14flt_dev_exit();
	d14flt_rfs_exit();
	d14flt_msg_exit();
}

module_init(d14flt_init);
module_exit(d14flt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paolo Ambrosio <blues@data-14.org>");
MODULE_DESCRIPTION("Data-14 Filesystem Filter");
