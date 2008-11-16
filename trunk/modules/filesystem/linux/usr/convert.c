/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14usr.h"
#include <arpa/inet.h>
#include <string.h>
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>

/* just for test */
#include <stdio.h>

iconv_t iconv_data;

char *dbname="/tmp/data-14/local.db";
char *dbdatadir="/tmp/data-14/data/";
char data_pathn[PATH_MAX];
char *data_filen;
sqlite3 *db;

/*
uint64_t ntohll(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
	return n;
#else
	return (((uint64_t)ntohl(n)) << 32) + ntohl(n >> 32);
#endif
}

uint64_t htonll(uint64_t n)
{
#if __BYTE_ORDER == __BIG_ENDIAN
	return n;
#else
	return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
#endif
}
*/

int conv_init() {
	int rv;

	iconv_data = iconv_open("UTF-8", nl_langinfo(CODESET));
	if (iconv_data == (iconv_t)(-1))
		return -1;
	return 0;

	if (sqlite3_open(dbname, &db) != SQLITE_OK)
		return -1;
/*
	sqlite3_exec(dbcon->db,
			"PRAGMA synchronous = OFF;"
			, NULL, NULL, NULL);
*/
	strcpy(data_pathn, dbdatadir);
	data_filen = data_pathn + strlen(data_pathn);

	return 0;
}

void conv_exit() {
	iconv_close(iconv_data);
}

/*
size_t msg_size(struct d14usr_msg *in) {
	size_t size = 0;
	size_t name_utf8 = 0;

	switch (in->m_head.m_type) {
	case MSG_TYPE_FS_CREATE:
		size = sizeof(D14FSCreate);
		if (iconv(iconv_data, &in->m_body.m_create.names,
				&in->m_body.m_create.m_args.d_name_len,
				NULL, &name_utf8 == (size_t)(-1))
		size += 
		if(S_ISLNK(in->m_body.m_create.m_args.i_mode))
			;
		else if (S_ISBLK(in->m_body.m_create.m_args.i_mode) ||
				S_ISCHR(in->m_body.m_create.m_args.i_mode))
			;
		break;
	case MSG_TYPE_FS_LINK:
		break;
	}

	return size;
}
*/

int usr_to_db(struct d14usr_msg *in)
{
	char *type_str;
	int i;
	char name[256];

	printf("size: %lu, id: %d, type: %d ", (long unsigned int)in->m_size, in->m_head.m_id, in->m_head.m_type);

	switch (in->m_head.m_type) {
	case MSG_TYPE_FS_CREATE:
		switch (in->m_body.m_create.m_args.i_mode & 0xF000) {
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
		printf("**MSG_TYPE_FS_CREATE**\n");
		printf("i_ino: %lu, pi_ino: %lu\n", in->m_body.m_create.m_args.i_ino, in->m_body.m_create.m_args.pi_ino);
		printf("i_rdev: %u, i_mode: %o, type: %s\n", (unsigned int)in->m_body.m_create.m_args.i_rdev, in->m_body.m_create.m_args.i_mode & 0x0FFF, type_str);
		printf("i_uid: %d, i_gid: %d\n", in->m_body.m_create.m_args.i_uid, in->m_body.m_create.m_args.i_gid);
		memcpy(name, in->m_body.m_create.names, in->m_body.m_create.m_args.d_name_len);
		name[in->m_body.m_create.m_args.d_name_len]='\0';
		printf("name: %s\n", name);
		if (in->m_body.m_create.m_args.s_name_len) {
			memcpy(name, &in->m_body.m_create.names[in->m_body.m_create.m_args.d_name_len], in->m_body.m_create.m_args.s_name_len);
			name[in->m_body.m_create.m_args.s_name_len]='\0';
			printf("symlink: %s\n", name);
		}
		break;
	case MSG_TYPE_FS_LINK:
		printf("**MSG_TYPE_FS_LINK**\n");
		printf("i_ino: %lu, pi_ino: %lu\n", in->m_body.m_link.m_args.i_ino, in->m_body.m_link.m_args.pi_ino);
		memcpy(name, in->m_body.m_link.name, in->m_body.m_link.m_args.d_name_len);
		name[in->m_body.m_link.m_args.d_name_len]='\0';
		printf("name: %s\n", name);
		break;
	case MSG_TYPE_FS_RENAME:
		printf("**MSG_TYPE_FS_RENAME**\n");
		printf("old_i_ino: %lu, old_pi_ino: %lu\n", in->m_body.m_rename.m_args.old_i_ino, in->m_body.m_rename.m_args.old_pi_ino);
		printf("new_i_ino: %lu, new_pi_ino: %lu\n", in->m_body.m_rename.m_args.new_i_ino, in->m_body.m_rename.m_args.new_pi_ino);
		memcpy(name, in->m_body.m_rename.names, in->m_body.m_rename.m_args.old_d_name_len);
		name[in->m_body.m_rename.m_args.old_d_name_len]='\0';
		printf("old_name: %s\n", name);
		memcpy(name, &in->m_body.m_rename.names[in->m_body.m_rename.m_args.old_d_name_len], in->m_body.m_rename.m_args.new_d_name_len);
		name[in->m_body.m_rename.m_args.new_d_name_len]='\0';
		printf("new_name: %s\n", name);
		break;
	case MSG_TYPE_FS_DELETE:
		printf("**MSG_TYPE_FS_DELETE**\n");
		printf("i_ino: %lu, pi_ino: %lu\n", in->m_body.m_delete.m_args.i_ino, in->m_body.m_delete.m_args.pi_ino);
		memcpy(name, in->m_body.m_delete.name, in->m_body.m_delete.m_args.d_name_len);
		name[in->m_body.m_delete.m_args.d_name_len]='\0';
		printf("name: %s\n", name);
		break;
	case MSG_TYPE_FS_ATTR:
		printf("**MSG_TYPE_FS_ATTR**\n");
		printf("i_ino: %lu\n", in->m_body.m_attr.m_args.i_ino);
		printf("i_mode: %o, i_uid: %d, i_gid: %d\n", in->m_body.m_attr.m_args.i_mode & 0x0FFF, in->m_body.m_attr.m_args.i_uid, in->m_body.m_attr.m_args.i_gid);
		break;
	case MSG_TYPE_FS_DATA:
		printf("**MSG_TYPE_FS_DATA**\n");
		printf("i_ino: %lu\n", in->m_body.m_data.m_args.i_ino);
		printf("range: %lld - %lld\n", in->m_body.m_data.m_args.start, in->m_body.m_data.m_args.end);
		printf("i_size: %lld, bitmap_size: %u\n", in->m_body.m_data.m_args.i_size, in->m_body.m_data.m_args.bitmap_size);
		for (i = 0; i < in->m_body.m_data.m_args.bitmap_size; i++) {
			if (!(i % 32))
				printf("\n0x%08x: ", i);
			printf("%02x", in->m_body.m_data.bitmap[i]);
		}
		printf("\n");
		printf("File descriptor: %d\n", in->m_fd);
/*
		if (in->m_fd >= 0) {
			if ((src = mmap (0, in->m_body.m_data.m_args.i_size, PROT_READ,
					MAP_SHARED, in->m_fd, 0)) != (caddr_t) -1) {
				if (in->m_body.m_data.m_args.start != -1) {
					printf("should copy range\n");
				} else {
					printf("should copy bitmap\n");
				}
				munmap(src, in->m_body.m_data.m_args.i_size);
			} else {
				printf("mmap failed!\n");
			}
			close(in->m_fd);
		}
*/
		break;
	default:
		printf("Event type unknown\n");
	}
	return 0;
}
