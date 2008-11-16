#ifndef D14USR_H_
#define D14USR_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
typedef __u32 __kernel_dev_t;

#include <sqlite3.h>

#include "comm.h"
#include "../../include/plugin.h"

struct d14flt_con {
	int fd;
	int connected;
};

/* device.c */
int d14usr_devconn(struct d14flt_con *fltcon);
int d14usr_opendev(struct d14flt_con *fltcon);
int d14usr_closedev(struct d14flt_con *fltcon);
int d14usr_read(struct d14flt_con *fltcon, void *buffer, size_t size);
int d14usr_write(struct d14flt_con *fltcon, void *buffer, size_t size);

/* convert.c */
int conv_init();
void conv_exit();
int usr_to_db(struct d14usr_msg *in);

#endif /*D14USR_H_*/
