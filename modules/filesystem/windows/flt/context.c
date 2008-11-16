/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Context handling functions
 */

#include "d14flt.h"

NPAGED_LOOKASIDE_LIST DentryContextsPool;
FAST_MUTEX DentryContextsLock;
RTL_GENERIC_TABLE DentryContextsTable;

VOID D14ContextInit ()
{
	ExInitializeNPagedLookasideList(&(DentryContextsPool),
		NULL, NULL, 0,
		D14_DE_CTX_SIZE + sizeof(RTL_SPLAY_LINKS) + sizeof(LIST_ENTRY),
		D14_DE_CTX_TAG, 0);
	ExInitializeFastMutex(&(DentryContextsLock));
	RtlInitializeGenericTable(&(DentryContextsTable),
		(PRTL_GENERIC_COMPARE_ROUTINE)CompareDentryContexts,
		(PRTL_GENERIC_ALLOCATE_ROUTINE)AllocateDentryContext,
		(PRTL_GENERIC_FREE_ROUTINE)FreeDentryContext, NULL);
}

VOID D14ContextExit ()
{
	ExDeleteNPagedLookasideList(&(DentryContextsPool));
}

NTSTATUS D14TouchContexts (
	__in PFLT_CALLBACK_DATA Data )
{
	NTSTATUS status;
	PD14_SH_CTX shctx;

	if (BooleanFlagOn(Data->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN))
		return STATUS_BAD_FILE_TYPE;

	status = FltGetStreamHandleContext(Data->Iopb->TargetInstance,
		Data->Iopb->TargetFileObject, &shctx);
	/* context already defined: don't do anything, just release it */
	if (NT_SUCCESS(status))
		goto out_rel;
	/* error in context function: exit with error */
	if ((status != STATUS_NOT_FOUND) || FlagOn(Data->Iopb->TargetFileObject->Flags, FO_STREAM_FILE)) {
//		DEBUG("FltGetStreamHandleContext error 0x%08x", status);
		goto out;
	}
	/* context not defined yet: create it and reference dentry and stream ctx */
	status = D14AllocateStreamHandleContext(Data, &shctx);
	if (!NT_SUCCESS(status))
		goto out;
	status = D14SetStreamHandleContext(Data, &shctx);
	if (!NT_SUCCESS(status)) {
		DEBUG("D14SetStreamHandleContext ERROR 0x%08x", status);
	}
out_rel:
	FltReleaseContext(shctx);
out:
	return status;
}

NTSTATUS D14RenameDentryContext (
	__in PFLT_CALLBACK_DATA Data )
{
	NTSTATUS status;
	D14_DE_ITEM newDentryItem, *dentryItem, *tmp;
	PD14_DE_CTX dentryContext;
	BOOLEAN created = FALSE;
	PD14_SH_CTX shctx = NULL;

	status = FltGetStreamHandleContext(Data->Iopb->TargetInstance,
		Data->Iopb->TargetFileObject, &shctx);
	if(!NT_SUCCESS(status)) /* if the file is not being tracked (swap file) */
		return status;

	status = D14FillDentryItem(Data, &newDentryItem);
	if (!NT_SUCCESS(status))
		return status;

	ExAcquireFastMutex(&DentryContextsLock);

	dentryItem = RtlInsertElementGenericTable(
		&DentryContextsTable,
		&newDentryItem, sizeof(D14_DE_ITEM), &created);

	FATAL_IF(dentryItem == NULL, "RtlInsertElementGenericTable: FAILED\n");
	FATAL_IF(!created, "RtlInsertElementGenericTable: item created on rename");

	tmp = shctx->DentryContext->Item;
	shctx->DentryContext->Item = dentryItem;
	RtlDeleteElementGenericTable(&DentryContextsTable, tmp);

	ExReleaseFastMutex(&DentryContextsLock);

	FltReleaseContext(shctx);
	return status;
}

/*
 * D14AllocateXxxContext
 *
 * Returns a context ready to be attached to an object
 */

NTSTATUS D14AllocateStreamHandleContext (
	__in PFLT_CALLBACK_DATA Data,
	__deref_out PD14_SH_CTX *Context )
{
	NTSTATUS status;
	PD14_SH_CTX ctxCont;

	status = FltAllocateContext(FilterGlobals.FilterHandle,
			FLT_STREAMHANDLE_CONTEXT, D14_SH_CTX_SIZE, NonPagedPool, &ctxCont);
	if (!NT_SUCCESS(status))
		goto out;

	status = D14FillStreamHandleContext(Data, ctxCont);
	if (!NT_SUCCESS(status))
		goto out_release;

	*Context = ctxCont;
	goto out;

out_release:
	FltReleaseContext(ctxCont);
out:
	return status;
}

NTSTATUS D14AllocateStreamContext (
	__in PFLT_CALLBACK_DATA Data,
	__deref_out PD14_ST_CTX *Context )
{
	NTSTATUS status;
	PD14_ST_CTX ctxCont;

	status = FltAllocateContext(FilterGlobals.FilterHandle, FLT_STREAM_CONTEXT,
		D14_ST_CTX_SIZE, NonPagedPool, &ctxCont);
	if (!NT_SUCCESS(status))
		goto out;

	status = D14FillStreamContext(Data, ctxCont);
	if (!NT_SUCCESS(status))
		goto out_release;

	*Context = ctxCont;
	goto out;

out_release:
	FltReleaseContext(ctxCont);
out:
	return status;
}

/*
 * D14GetXxxContext
 */

NTSTATUS D14GetDentryContext(
	__in PFLT_CALLBACK_DATA Data,
	__deref_out PD14_DE_CTX *Context )
{
	NTSTATUS status;
	D14_DE_ITEM newDentryItem, *dentryItem;
	PD14_DE_CTX dentryContext;
	BOOLEAN created = FALSE;

	status = D14FillDentryItem(Data, &newDentryItem);
	if (!NT_SUCCESS(status))
		return status;

	ExAcquireFastMutex(&DentryContextsLock);

	dentryItem = RtlInsertElementGenericTable(
		&DentryContextsTable,
		&newDentryItem, sizeof(D14_DE_ITEM), &created);

	FATAL_IF(dentryItem == FALSE, "RtlInsertElementGenericTable FAILED");

	/* if created, initialize it */
	if (created) {
		dentryContext = ExAllocateFromNPagedLookasideList(&DentryContextsPool);

		if (dentryContext != NULL) {
			status = D14FillDentryContext(Data, dentryContext);
			if (NT_SUCCESS(status)) {
				dentryContext->Item = dentryItem;
				dentryItem->Context = dentryContext;
			} else {
				created = FALSE; /* so it will be cleaned on exit */
				ExFreeToNPagedLookasideList(&DentryContextsPool, dentryContext);
				RtlDeleteElementGenericTable(&DentryContextsTable, dentryItem); /*** ASSERT se failed ****************************/
			}
		} else {
			FATAL("CANNOT ALLOCATE CONTEXT!!!!");
		}
	}

	/* if the context is created correctly or it was already defined,
	 * increment the reference count */
	if (dentryItem->Context != NULL) {
		*Context = dentryItem->Context;
		dentryItem->Context->HandleCount++;
	}

	ExReleaseFastMutex(&DentryContextsLock);
	/* if it was not created, free the structure used for generic
	 * table comparison */
	if (!created)
		D14CleanupDentryItem(&newDentryItem, 0);
	return status;
}

NTSTATUS D14GetStreamContext (
	__in PFLT_CALLBACK_DATA Data,
	__deref_out PD14_ST_CTX *Context )
{
	NTSTATUS status;
	PD14_ST_CTX stctx = NULL;

	status = FltGetStreamContext(Data->Iopb->TargetInstance,
		Data->Iopb->TargetFileObject, &stctx);
	/* context already defined: don't do anything, just release it */
	if (NT_SUCCESS(status)) {
		goto out;
	}
	/* error in context function: exit with error */
	if (status != STATUS_NOT_FOUND) {
		return status;
	}

	status = D14AllocateStreamContext(Data, &stctx);
	if (!NT_SUCCESS(status))
		return status;

	status = D14SetStreamContext(Data, &stctx);
	if (!NT_SUCCESS(status)) {
		FltReleaseContext(stctx);
		return status;
	}
out:
	ExAcquireFastMutex(&(stctx->Lock));
	stctx->ReferenceCount++;
	ExReleaseFastMutex(&(stctx->Lock));
	*Context = stctx;
	return status;
}

/*
 * D14SetXxxContext functions
 *
 * Attach a real context to the stream handle, dentry or stream. If exists,
 * returns the old context and releases the new one.
 */

NTSTATUS D14SetStreamHandleContext(
	__in PFLT_CALLBACK_DATA Data,
	__in PD14_SH_CTX *newContext )
{
	NTSTATUS status;
	PD14_SH_CTX oldContext;

	status = FltSetStreamHandleContext(Data->Iopb->TargetInstance,
		Data->Iopb->TargetFileObject, FLT_SET_CONTEXT_KEEP_IF_EXISTS,
		*newContext, &oldContext );
	if (NT_SUCCESS(status))
		return status;

	/* if set failed we must release the new context */
	FltReleaseContext(*newContext);

	/* if a context is already present, returns that one and succeds */
	if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED) {
		*newContext = oldContext;
		status = STATUS_SUCCESS;
	}
	return status;
}

NTSTATUS D14SetStreamContext(
	__in PFLT_CALLBACK_DATA Data,
	__in PD14_ST_CTX *newContext )
{
	NTSTATUS status;
	PD14_ST_CTX oldContext;

	status = FltSetStreamContext(Data->Iopb->TargetInstance,
		Data->Iopb->TargetFileObject, FLT_SET_CONTEXT_KEEP_IF_EXISTS,
		*newContext, &oldContext );
	if (NT_SUCCESS(status))
		return status;

	/* if set failed we must release the new context */
	FltReleaseContext(*newContext);

	/* if a context is already present, returns that one and succeds */
	if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED) {
		*newContext = oldContext;
		status = STATUS_SUCCESS;
	}
	return status;
}

/*
 * D14ReleaseXxxContext
 */

VOID D14ReleaseDentryContext (
	__in PD14_DE_CTX Context )
{
	/*
	 * If zero ref count, calls zero ref and cleanup
	 * When implemented cache management, only if Delete is set to TRUE
	 * should be deleted immediately.
	 */

	ExAcquireFastMutex(&DentryContextsLock);
	Context->HandleCount--;
	if (Context->HandleCount == 0) {
		D14ZeroHndDentryContext(Context);
		D14CleanupDentryContext(Context, 0);
		D14CleanupDentryItem(Context->Item, 0);
				RtlDeleteElementGenericTable(&DentryContextsTable, Context->Item); /*** ASSERT se failed ****************************/
		ExFreeToNPagedLookasideList(&DentryContextsPool, Context);
	}
/* we should not release the instance context before its lock! **************************************/
	ExReleaseFastMutex(&DentryContextsLock);
}

VOID D14ReleaseStreamContext (
	__in PD14_ST_CTX Context )
{
	ExAcquireFastMutex(&(Context->Lock));
	Context->ReferenceCount--;
	if (Context->ReferenceCount == 0)
		D14ZeroRefStreamContext(Context);
	ExReleaseFastMutex(&(Context->Lock));
	FltReleaseContext(Context);
}

/*
 * D14FillXxxContext functions
 *
 * Called whe the context is first created.
 */

NTSTATUS D14FillStreamHandleContext(
	__in PFLT_CALLBACK_DATA Data,
	__out PD14_SH_CTX ShContext )
{
	NTSTATUS status = STATUS_SUCCESS;
	PD14_DE_CTX dectx;
	PD14_ST_CTX stctx;

	ShContext->DentryContext = NULL;
	ShContext->StreamContext = NULL;
	ShContext->DeleteOnClose = FALSE;

	/*********************************************** could be assinged directly from the getxxxcontext! */
	if (FlagOff(Data->Iopb->TargetFileObject->Flags, FO_STREAM_FILE)) {
		status = D14GetDentryContext(Data, &dectx);
		if (NT_SUCCESS(status))
			ShContext->DentryContext = dectx;
		else if (status != STATUS_BAD_FILE_TYPE)
			DEBUG("Dentry context error code: 0x%08x", status);
	}

	if (status != STATUS_BAD_FILE_TYPE) {
		status = D14GetStreamContext(Data, &stctx);
		if (NT_SUCCESS(status))
			ShContext->StreamContext = stctx;
		else
			DEBUG("Stream context error code: 0x%08x", status);
	}

	if ((ShContext->DentryContext != NULL) && (Data->Iopb->MajorFunction == IRP_MJ_CREATE))
		ShContext->DeleteOnClose = BooleanFlagOn(
			Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE);

	return status;
}

NTSTATUS D14FillDentryItem(
	__in PFLT_CALLBACK_DATA Data,
	__out PD14_DE_ITEM DeItem )
{
	NTSTATUS status = STATUS_SUCCESS;

	status = FltGetFileNameInformation(Data,
		FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
		&(DeItem->FileName));
	if (!NT_SUCCESS(status)) {
		DEBUG("FltGetFileNameInformation failed");
		return status;
	}

	status = FltParseFileNameInformation(DeItem->FileName);
	if (!NT_SUCCESS(status)) {
		DEBUG("FltParseFileNameInformation failed");
		goto out_release;
	}
	/************************************************************* needed? ***************/
	if (DeItem->FileName->FinalComponent.Length == 0) {
		status = STATUS_BAD_FILE_TYPE;
		goto out_release;
	}

	DeItem->FileNameHash = D14FullNameHash(DeItem->FileName->Name);

	DeItem->Context = NULL;

	goto out;
out_release:
	FltReleaseFileNameInformation(DeItem->FileName);
out:
	return status;
}

NTSTATUS D14FillDentryContext(
	__in PFLT_CALLBACK_DATA Data,
	__out PD14_DE_CTX DeContext )
{
	ExInitializeFastMutex(&(DeContext->Lock));
	DeContext->HandleCount = 0;
	DeContext->Delete = FALSE;

	return STATUS_SUCCESS;
}

NTSTATUS D14FillStreamContext(
	__in PFLT_CALLBACK_DATA Data,
	__out PD14_ST_CTX StContext )
{
	NTSTATUS status;
	FILE_INTERNAL_INFORMATION info;

	status = FltQueryInformationFile(Data->Iopb->TargetInstance,
		Data->Iopb->TargetFileObject, &info, sizeof(info),
		FileInternalInformation, NULL);
	if (!NT_SUCCESS(status))
		return status;

	StContext->IndexNumber = info.IndexNumber;
	StContext->ReferenceCount = 0;
	ExInitializeFastMutex(&(StContext->Lock));
/* TEST
	StContext->FileObject = IoCreateStreamFileObject(Data->Iopb->TargetFileObject, NULL);
	DbgPrint("New st context: New %s StreamFileObject: 0x%p, %u\n",
			CcIsFileCached(StContext->FileObject) ? "cached" : "not cached",
			StContext->FileObject, StContext->IndexNumber);
*/
	return status;
}

/*
 * D14ZeroRefXxxContext
 *
 * Callbacks called when the reference count reaches zero
 */

VOID D14ZeroHndDentryContext (
	__in PD14_DE_CTX DeContext )
{
	INFO("%s \"%wZ\" %s", __FUNCTION__, &(DeContext->Item->FileName->Name), DeContext->Delete ? "[DELETED]" : "");

	if (DeContext->Delete)
		OnDelete();
}

VOID D14ZeroRefStreamContext (
	__in PD14_ST_CTX StContext )
{
	INFO("%s %u", __FUNCTION__, StContext->IndexNumber);
}

/*
 * D14CleanupXxxContext
 *
 * Cleanup routines, different from zeroed reference count routines
 */

VOID D14CleanupStreamHandleContext (
	__in PD14_SH_CTX ShContext,
	__in FLT_CONTEXT_TYPE ContextType )
{
/*
	if (ShContext->StreamContext)
		INFO("%s %u", __FUNCTION__, ShContext->StreamContext->IndexNumber);
*/
	if (ShContext->DentryContext != NULL) {
		if (ShContext->DeleteOnClose == TRUE)
			ShContext->DentryContext->Delete = TRUE;
		D14ReleaseDentryContext(ShContext->DentryContext);
	}
	if (ShContext->StreamContext != NULL)
		D14ReleaseStreamContext(ShContext->StreamContext);
}

VOID D14CleanupDentryItem (
	__in PD14_DE_ITEM DeItem,
	__in FLT_CONTEXT_TYPE ContextType )
{
	FltReleaseFileNameInformation(DeItem->FileName);
}

VOID D14CleanupDentryContext (
	__in PD14_DE_CTX DeContext,
	__in FLT_CONTEXT_TYPE ContextType )
{
	INFO("%s \"%wZ\"", __FUNCTION__, &(DeContext->Item->FileName->Name))
}

VOID D14CleanupStreamContext (
	__in PD14_ST_CTX StContext,
	__in FLT_CONTEXT_TYPE ContextType )
{
	INFO("%s %u", __FUNCTION__, StContext->IndexNumber);

/* TEST
	ObDereferenceObject(StContext->FileObject);
*/
}

/*
 * Dentry Contexts GenericTable functions
 */

RTL_GENERIC_COMPARE_RESULTS CompareDentryContexts (
	struct _RTL_GENERIC_TABLE *Table,
	PD14_DE_ITEM FirstDentry,
	PD14_DE_ITEM SecondDentry )
{
	LONG result = FirstDentry->FileNameHash - SecondDentry->FileNameHash;

	/* if same hash we do full string comparison */
	if (result == 0)
		result = RtlCompareUnicodeString(&(FirstDentry->FileName->Name),
			&(SecondDentry->FileName->Name), FALSE);

	if (result < 0)
		return GenericLessThan;
	if (result > 0)
		return GenericGreaterThan;
	return GenericEqual;
}

PVOID AllocateDentryContext (
	struct _RTL_GENERIC_TABLE *Table,
	CLONG ByteSize )
{
	return ExAllocatePoolWithTag(NonPagedPool, ByteSize, D14_DE_CTX_TAG);
}

VOID FreeDentryContext (
	struct _RTL_GENERIC_TABLE *Table,
	PVOID Buffer )
{
	ExFreePoolWithTag(Buffer, D14_DE_CTX_TAG);
}

/*
 * Context utility functions
 */

USHORT D14FullNameHash (
	UNICODE_STRING Name )
{
	WCHAR hash;
	PWSTR ptr;

	for (ptr = Name.Buffer, hash = 0;
		ptr < (PWSTR)Add2Ptr(Name.Buffer, Name.Length); ptr++) {
		hash += *ptr;
	}
	return (USHORT) hash;
}

/*
 * Context registration structure
 */

const FLT_CONTEXT_REGISTRATION D14Contexts[] = {

	{ FLT_STREAM_CONTEXT,
		0,
		D14CleanupStreamContext,
		D14_ST_CTX_SIZE,
		D14_ST_CTX_TAG },

	{ FLT_STREAMHANDLE_CONTEXT,
		0,
		D14CleanupStreamHandleContext,
		D14_SH_CTX_SIZE,
		D14_SH_CTX_TAG },

	{ FLT_CONTEXT_END }
};
