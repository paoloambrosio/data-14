#ifndef FSPLUGIN_H_
#define FSPLUGIN_H_

#include "../../../core/include/message.h"
#include "internal.h"

/* used by the dispatcher messages only */

#define MSG_TYPE_FS        1

/* extra type */

typedef union {
	uint32_t value;
	struct {
		uint16_t endianness;
		uint16_t size;
	} buffer;
} d14_acl_t;

/*
 * We need to have only the attributes that are present on file creation.
 * Reparse point tags are set after file creation, but ACLs are immediate.
 */
typedef struct _D14FSCreate {
	uint64_t id;            /* file id: signed on windows, unsigned on linux */
	uint64_t parent_id;

	uint16_t file_type;     /* regular, directory, device, ... */
	uint16_t message_flags; /* acl type (inline/buffered) */

	uint16_t attr;          /* windows file attributes */

	uint16_t name_size;     /* filename length: in utf-8 can be more than 255 */
	uint16_t typedata_size; /* type-dependent data length: symlnk, devid, ... */

	d14_acl_t owner;        /* linux i_uid */
	d14_acl_t group;        /* linux i_gid */
	d14_acl_t security;     /* linux i_mode & 0x7fff */

	uint8_t buffer[1];      /* name + type_data + owner + group + acl */
} D14FSCreate;

typedef struct _D14FSLink {
	uint64_t id;
	uint64_t parent_id;
	uint16_t name_size;
	
	uint8_t buffer[1];
} D14FSLink;

typedef struct _D14FSRename {
	uint64_t old_id;
	uint64_t old_parent_id;
	uint64_t new_id;        /* overwritten file */ 
	uint64_t new_parent_id;
	uint16_t old_name_size; /* utf-8 bytes */
	uint16_t new_name_size; /* utf-8 bytes */

	uint8_t buffer[1];      /* old_name + new_name */
} D14FSRename;

typedef struct _D14FSDelete {
	uint64_t id;
	uint64_t parent_id;
	uint16_t name_size;     /* utf-8 bytes */

	uint8_t buffer[1];      /* name */
} D14FSDelete;

/* Here go all file attributes subject to change */
typedef struct _D14FSAttributes {
	uint64_t id;

/*	uint32_t attr;          * file attributes (windows only, no file type) */

/*	uint16_t mask;          * changed attributes */
/*	uint16_t message_flags; * acl type (inline/buffered) */

	d14_acl_t owner;        /* linux i_uid */
	d14_acl_t group;        /* linux i_gid */
	d14_acl_t security;     /* linux i_mode & 0x7fff */

/*
 * reparse_tag, reparse_guid, reparse_data
 */

/*	uint8_t buffer[1];      * what present in the mask */
} D14FSAttributes;

typedef struct _D14FSData {
	uint64_t id;

	int64_t eof;            /* linux i_size */
	int64_t start_offset;
	int64_t end_offset;
	int64_t bitmap_size;

	uint8_t buffer[1];      /* bitmap */
} D14FSData;

typedef struct _D14FSMsg {
	D14MsgHead head;
	union {
		D14FSCreate create;
		D14FSLink link;
		D14FSRename rename;
		D14FSDelete delete;
		D14FSAttributes attributes;
		D14FSData data;
	} body;
} D14FSMsg;

#endif /*FSPLUGIN_H_*/
