/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14flt.h"

int d14flt_ctl(struct rfs_ctl *ctl);
extern struct rfs_op_info d14flt_op_info[];

struct rfs_filter_info flt_info = {"d14flt", 2222, 0, d14flt_ctl};

rfs_filter d14flt_flt;

int d14flt_ctl(struct rfs_ctl *ctl)
{
	int rv = 0;

	switch (ctl->id) {
		case RFS_CTL_ACTIVATE:
			rv = rfs_activate_filter(d14flt_flt);
			break;

		case RFS_CTL_DEACTIVATE:
			rv = rfs_deactivate_filter(d14flt_flt);
			break;

		case RFS_CTL_SETPATH:
			rv = rfs_set_path(d14flt_flt, &ctl->data.path_info); 
			break;
	}

	return rv;
}

int d14flt_rfs_init(void)
{
	int rv;

	rv = d14flt_rfs_data_cache_init();
	if (rv) {
		d14flt_err("data cache init failed: %d", rv);
		return rv;
	}

	rv = rfs_register_filter(&d14flt_flt, &flt_info);
	if (rv) {
		d14flt_err("register filter failed: %d", rv);
		goto error_free;
	}

	rv = rfs_set_operations(d14flt_flt, d14flt_op_info); 
	if (rv) {
		d14flt_err("set operations failed: %d", rv);
		goto error_unreg;
	}

	rv = rfs_activate_filter(d14flt_flt);
	if (rv)
		goto error_unreg;

	return 0;

error_unreg:
	rfs_unregister_filter(d14flt_flt);
error_free:
	d14flt_rfs_data_cache_exit();

	return rv;
}

void d14flt_rfs_exit(void)
{
	int rv;

	rv = rfs_unregister_filter(d14flt_flt);
	if (rv)
		d14flt_err("rfs_unregister_filter failed: %d", rv);

	d14flt_rfs_data_cache_exit();
}

