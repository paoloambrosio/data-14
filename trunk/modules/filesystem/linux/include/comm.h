#ifndef D14FLT_DEV_H_
#define D14FLT_DEV_H_

#include "../../include/internal.h"

/* d14flt device communication structures */

#define D14FLT_MAGIC 0x2d443431
#define D14FLT_DEV_NAME "d14flt"
#define D14FLT_DEV_VERSION 1
#define D14FLT_DEV_CONNECT  1
#define D14FLT_DEV_DEQUEUE 10

struct d14flt_cmd {
	int cmd;
	union {
		struct {
			int magic;
			int version;
		} connect;
		struct {
			int id;
		} dequeue;
	} args;
};

/* system dependent event structure */

typedef struct {
	__kernel_ino_t i_ino;        /* inode */
	__kernel_ino_t pi_ino;       /* parent inode */
	__kernel_mode_t i_mode;      /* file type and security */
	__kernel_uid_t i_uid;
	__kernel_gid_t i_gid;
	__kernel_dev_t i_rdev;       /* device information */
	unsigned int d_name_len;
	unsigned int s_name_len;
} m_create_t;

typedef struct {
	__kernel_ino_t i_ino;        /* inode */
	__kernel_ino_t pi_ino;       /* parent inode */
	unsigned int d_name_len;
} m_link_t;

typedef struct {
	__kernel_ino_t old_i_ino;    /* inode */
	__kernel_ino_t old_pi_ino;   /* old parent inode */
	__kernel_ino_t new_i_ino;    /* overwritten inode */
	__kernel_ino_t new_pi_ino;   /* new parent inode */
	unsigned int old_d_name_len; /* old name length */
	unsigned int new_d_name_len; /* new name length */
} m_rename_t;

typedef struct {
	__kernel_ino_t i_ino;        /* inode */
	__kernel_ino_t pi_ino;       /* parent inode */
	unsigned int d_name_len;     /* dentry name length */
} m_delete_t;

typedef struct {
	__kernel_ino_t i_ino;        /* inode */
	unsigned int ia_valid;       /* changed values */
	__kernel_mode_t i_mode;
	__kernel_uid_t i_uid;
	__kernel_gid_t i_gid;
} m_attr_t;

typedef struct {
	__kernel_ino_t i_ino;        /* inode */
	__kernel_loff_t i_size;
	__kernel_loff_t start;
	__kernel_loff_t end;
	__kernel_size_t bitmap_size;
} m_data_t;

struct d14usr_msg {
	__kernel_size_t m_size;
	int m_fd;                            /* -1 if no data */
	struct {
		int m_id;
		int m_type;
	} m_head;
	union {
		struct {
			m_create_t m_args;
			unsigned char names[1];
		} m_create;
		struct {
			m_link_t m_args;
			unsigned char name[1];
		} m_link;
		struct {
			m_rename_t m_args;
			unsigned char names[1];
		} m_rename;
		struct {
			m_delete_t m_args;
			unsigned char name[1];       /* dentry name buffer */
		} m_delete;
		struct {
			m_attr_t m_args;
		} m_attr;
		struct {
			m_data_t m_args;
			unsigned char bitmap[1];
		} m_data;
	} m_body;
};

#endif /*D14FLT_DEV_H_*/
