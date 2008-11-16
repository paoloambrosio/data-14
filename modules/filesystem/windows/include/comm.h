#ifndef COMM_H_
#define COMM_H_

#define D14_PORT_NAME L"\\D14FltPort"
#define CMD_CONNECT 1

typedef struct _MESSAGE_HEAD {
	LONG MessageID;
} MESSAGE_HEAD;

typedef struct _MESSAGE_CREATE {
	LARGE_INTEGER FileIndex;
	LARGE_INTEGER ParentIndex;
	ULONG FileAttributes;      /* FILE_ATTRIBUTE_DIRECTORY for file type */

	/*
	 * We need to keep this short because it determines the size of
	 * all the messages in memory
	 */
	USHORT OwnerSize;
	USHORT GroupSize;
	USHORT SecuritySize;       /* DACL */

	USHORT NameSize;
} MESSAGE_CREATE;

typedef struct _MESSAGE_LINK {
	LARGE_INTEGER FileIndex;
	LARGE_INTEGER ParentIndex;

	USHORT NameLength;
} MESSAGE_LINK;

typedef struct _MESSAGE_RENAME {
	LARGE_INTEGER FileIndex;
	LARGE_INTEGER OldParentIndex;
	LARGE_INTEGER NewParentIndex;

	USHORT OldNameLength;
	USHORT NewNameLength;
} MESSAGE_RENAME;

typedef struct _MESSAGE_DELETE {
	LARGE_INTEGER FileIndex;
	LARGE_INTEGER ParentIndex;

	USHORT NameLength;
} MESSAGE_DELETE;

typedef struct _MESSAGE_ATTRIBUTES {
	LARGE_INTEGER FileIndex;

	ULONG ChangesMask;         /* what changed */

	ULONG FileAttributes;

	USHORT OwnerSize;
	USHORT GroupSize;
	USHORT SecuritySize;       /* DACL */
} MESSAGE_ATTRIBUTES;

typedef struct _MESSAGE_DATA {
	LARGE_INTEGER FileIndex;

	LARGE_INTEGER EndOfFile;

	LARGE_INTEGER StartOffset;
	LARGE_INTEGER EndOffset;

	LARGE_INTEGER BitmapSize;
} MESSAGE_DATA;

/*
 * The buffer is external to let the kernel use a small inline buffer or
 * allocate an external buffer for big data
 */

typedef struct _D14_USRMESSAGE {
	MESSAGE_HEAD MessageHead;
	union {
		struct {
			MESSAGE_CREATE Args;
			UCHAR Buffer[1]; /* file name, owner, group, acl */
		} Create;
		struct {
			MESSAGE_LINK Args;
			UCHAR Buffer[1]; /* file name */
		} Link;
		struct {
			MESSAGE_RENAME Args;
			UCHAR Buffer[1]; /* file names */
		} Rename;
		struct {
			MESSAGE_DELETE Args;
			UCHAR Buffer[1]; /* file name */
		} Delete;
		struct {
			MESSAGE_ATTRIBUTES Args;
			UCHAR Buffer[1]; /* owner, group, acl */
		} Attributes;
		struct {
			MESSAGE_DATA Args;
			UCHAR Buffer[1]; /* bitmap */
		} Data;
	} MessageBody;
} D14_USRMESSAGE, *PD14_USRMESSAGE;

#endif /*COMM_H_*/
