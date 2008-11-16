#ifndef D14FLT_H_
#define D14FLT_H_

#include <fltKernel.h>
#include "trace.h"
#include "comm.h"
#include "internal.h"

/*
 * Context management
 */

#define D14_MESSAGE_TAG 'Mess'
#define D14_MESSAGE_SIZE sizeof(D14_MESSAGE)

typedef struct _D14_MESSAGE {
	LIST_ENTRY List;
	MESSAGE_HEAD MessageHead;
	/* something else */
} D14_MESSAGE, *PD14_MESSAGE;

/* Macros */

#define D14_ST_CTX_TAG 'StCx'
#define D14_ST_CTX_SIZE sizeof(D14_ST_CTX)

#define D14_DE_CTX_TAG 'DeCx'
#define D14_DE_CTX_SIZE sizeof(D14_DE_CTX)

#define D14_SH_CTX_TAG 'SHCx'
#define D14_SH_CTX_SIZE sizeof(D14_SH_CTX)

/* Contexts */

typedef struct _D14_ST_CTX {
	LONG ReferenceCount;
	FAST_MUTEX Lock;
	LARGE_INTEGER IndexNumber; /* Stream id */
	PFILE_OBJECT FileObject;
} D14_ST_CTX, *PD14_ST_CTX;

struct _D14_DE_ITEM;

typedef struct _D14_DE_CTX {
	LONG HandleCount;
	FAST_MUTEX Lock;
	BOOLEAN Delete;
	struct _D14_DE_ITEM *Item;
} D14_DE_CTX, *PD14_DE_CTX;

typedef struct _D14_DE_ITEM {
	PFLT_FILE_NAME_INFORMATION FileName;
	USHORT FileNameHash;
	PD14_DE_CTX Context;
} D14_DE_ITEM, *PD14_DE_ITEM;

typedef struct _D14_SH_CTX {
	PD14_DE_CTX DentryContext;
	PD14_ST_CTX StreamContext;
	BOOLEAN DeleteOnClose;
} D14_SH_CTX, *PD14_SH_CTX;


NTSTATUS DriverUnload (
	FLT_FILTER_UNLOAD_FLAGS Flags );

NTSTATUS D14FilterInit ();
VOID D14FilterExit ();

VOID D14ContextInit ();
VOID D14ContextExit ();

VOID D14MessageInit ();
VOID D14MessageExit ();

NTSTATUS D14CommInit ();
VOID D14CommExit ();

/* Context functions */

NTSTATUS D14TouchContexts (
	__in PFLT_CALLBACK_DATA Data );

NTSTATUS D14RenameDentryContext (
	__in PFLT_CALLBACK_DATA Data );

NTSTATUS D14AllocateStreamHandleContext (
	__in PFLT_CALLBACK_DATA Data,
	__deref_out PD14_SH_CTX *Context );

NTSTATUS D14GetDentryContext(
	__in PFLT_CALLBACK_DATA Data,
	__deref_out PD14_DE_CTX *Context );

NTSTATUS D14GetStreamContext (
	__in PFLT_CALLBACK_DATA Data,
	__deref_out PD14_ST_CTX *Context );

NTSTATUS D14AllocateStreamContext (
	__in PFLT_CALLBACK_DATA Data,
	__deref_out PD14_ST_CTX *Context );

NTSTATUS D14SetStreamHandleContext(
	__in PFLT_CALLBACK_DATA Data,
	__in PD14_SH_CTX *newContext );

NTSTATUS D14SetStreamContext(
	__in PFLT_CALLBACK_DATA Data,
	__in PD14_ST_CTX *newContext );

VOID D14ReleaseDentryContext (
	__in PD14_DE_CTX Context );

VOID D14ReleaseStreamContext (
	__in PD14_ST_CTX Context );

NTSTATUS D14FillStreamHandleContext(
	__in PFLT_CALLBACK_DATA Data,
	__out PD14_SH_CTX ShContext );

NTSTATUS D14FillDentryItem(
	__in PFLT_CALLBACK_DATA Data,
	__out PD14_DE_ITEM DeItem );

NTSTATUS D14FillDentryContext(
	__in PFLT_CALLBACK_DATA Data,
	__out PD14_DE_CTX DeContext );

NTSTATUS D14FillStreamContext(
	__in PFLT_CALLBACK_DATA Data,
	__out PD14_ST_CTX StContext );

VOID D14ZeroHndDentryContext (
	__in PD14_DE_CTX DeContext );

VOID D14ZeroRefStreamContext (
	__in PD14_ST_CTX StContext );

VOID D14CleanupStreamHandleContext (
	__in PD14_SH_CTX ShContext,
	__in FLT_CONTEXT_TYPE ContextType );

VOID D14CleanupDentryItem (
	__in PD14_DE_ITEM DeItem,
	__in FLT_CONTEXT_TYPE ContextType );

VOID D14CleanupDentryContext (
	__in PD14_DE_CTX DeContext,
	__in FLT_CONTEXT_TYPE ContextType );

VOID D14CleanupStreamContext (
	__in PD14_ST_CTX StContext,
	__in FLT_CONTEXT_TYPE ContextType );

RTL_GENERIC_COMPARE_RESULTS CompareDentryContexts (
	struct _RTL_GENERIC_TABLE *Table,
	PD14_DE_ITEM FirstDentry,
	PD14_DE_ITEM SecondDentry );

PVOID AllocateDentryContext (
	struct _RTL_GENERIC_TABLE *Table,
	CLONG ByteSize );

VOID FreeDentryContext (
	struct _RTL_GENERIC_TABLE *Table,
	PVOID Buffer );

USHORT D14FullNameHash (
	UNICODE_STRING Name );

/* Message queue functions */

PD14_MESSAGE D14AllocMessage ( VOID );
VOID D14FreeMessage ( PD14_MESSAGE Message );
VOID D14QueueMessage ( PD14_MESSAGE Message );
VOID D14DequeueMessage ( PD14_MESSAGE Message );
PD14_MESSAGE D14FirstMessage ( VOID );

/* File system events */

VOID OnDelete ( VOID );

/*
 * Other definitions
 */

#define D14_OPERATIONS_POOL_TAG 'OpPo'

/* FlagOn, SetFlag, ClearFlag already defined */

#ifndef FlagOff
#define FlagOff(_F,_SF)   (((_F) & (_SF)) == 0)
#endif

/*
 * Global structures
 */

typedef struct _D14FLT_DATA {
	PFLT_FILTER FilterHandle;
	PDRIVER_OBJECT DriverObject;
	PFLT_PORT ServerPort;
	PFLT_PORT ClientPort;
	BOOLEAN StopWorkItem;
	PFLT_GENERIC_WORKITEM CommWorkItem;
} D14FLT_DATA, *PD14FLT_DATA;

extern D14FLT_DATA FilterGlobals;

/*
 * Global variables
 */

extern const FLT_CONTEXT_REGISTRATION D14Contexts[];
extern const FLT_OPERATION_REGISTRATION D14Callbacks[];
extern const FLT_REGISTRATION D14Registration;





typedef struct _D14_OP_HEADER {
	UINT16 MessageLength;
	UINT8  MajorOperation;
	UINT8  MinorOperation;
	UINT32 Reserved;
} D14_OP_HEADER, *PD14_OP_HEADER;

typedef union _D14_OP_PARAMS {
	struct {
		UINT16 NameLength;
		WCHAR NameBuffer[1];
	} Create;
	struct {
		UINT16 NameLength;
		WCHAR NameBuffer[1];
	} Delete;
	struct {
		UINT16 OldNameOffset;
		UINT16 OldNameLength;
		UINT16 NewNameOffset;
		UINT16 NewNameLength;
		WCHAR NameBuffer[2];
	} Rename;
	struct {
		UINT64 Length;
	} Length;
	struct {
		UINT64 Offset;
		UINT64 Length;
	} Write;
	struct {
		UINT64 Timestamp;
	} End;
} D14_OP_PARAMS, *PD14_OP_PARAMS;

typedef struct _D14_OPERATION {
	D14_OP_HEADER Header;
	D14_OP_PARAMS Param;
} D14_OPERATION, *PD14_OPERATION;

/* Values for Operation field */

#define D14_OP_CREATE         0x01
#define D14_OP_CREATE_FILE         0x01
#define D14_OP_CREATE_DIR          0x02
#define D14_OP_DELETE         0x02
#define D14_OP_LENGTH         0x03
#define D14_OP_LENGTH_EOF          0x01
#define D14_OP_LENGTH_ALLOC        0x02
#define D14_OP_LENGTH_VALID        0x03
#define D14_OP_RENAME         0x04
#define D14_OP_WRITE          0x05
#define D14_OP_END            0xFF
#define D14_OP_END_CLOSE           0x01
#define D14_OP_END_SNAPSHOT        0X02
#define D14_OP_END_UPGRADE         0X03
#define D14_OP_END_FAILURE         0X04

/* Macro */

#define D14OperationSize(TYPE, FILE_LENGTH) \
	(sizeof(D14_OP_HEADER) + sizeof(D14_OPERATION) + \
	FILE_LENGTH)

#define D14AllocOperation(_OP_PTR, _MAYOR, _MINOR, _TYPE, _FILE_LENGTH) \
	_OP_PTR = ExAllocatePoolWithTag(NonPagedPool, \
		(sizeof(D14_OP_HEADER) + sizeof(_OP_PTR##->Param.##_TYPE) + \
		_FILE_LENGTH), D14_OPERATIONS_POOL_TAG); \
	if (_OP_PTR) { \
		_OP_PTR->Header.MessageLength = (sizeof(D14_OP_HEADER) + \
		sizeof(_OP_PTR##->Param.##_TYPE) + _FILE_LENGTH); \
		_OP_PTR->Header.MajorOperation = _MAYOR; \
		_OP_PTR->Header.MinorOperation = _MINOR; \
		_OP_PTR->Header.Reserved = 0; \
	}

#define D14FreeOperation(_OP_PTR) ExFreePool(_OP_PTR)

#endif /*D14FLT_H_*/
