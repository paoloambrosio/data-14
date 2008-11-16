/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Minifilter callbacks
 */

#include "d14flt.h"

/* create postoperation always called at IRQL = PASSIVE */
FLT_POSTOP_CALLBACK_STATUS D14PostCreate (
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags )
{
	/* not checking for write access anymore */
	if (NT_SUCCESS(Data->IoStatus.Status) &&
			FlagOff(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE)) 
		D14TouchContexts(Data);

	return FLT_POSTOP_FINISHED_PROCESSING;
}

/* called at IRQL <= APC */
FLT_POSTOP_CALLBACK_STATUS D14PostSetInformationSafe (
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags )
{
	NTSTATUS status;
	PD14_SH_CTX shctx;

	switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass) {
	case FileRenameInformation:
		D14RenameDentryContext(Data);
		break;
	case FileDispositionInformation:
		status = FltGetStreamHandleContext(Data->Iopb->TargetInstance,
			Data->Iopb->TargetFileObject, &shctx);
		if(!NT_SUCCESS(status))
			break;
		/* no locking necessary: it's just and assignment */
		if (shctx->DentryContext)
			shctx->DentryContext->Delete = ((FILE_DISPOSITION_INFORMATION *)
				Data->Iopb->Parameters.SetFileInformation.InfoBuffer)
				->DeleteFile;
		FltReleaseContext(shctx);
		break;
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}

/* called at IRQL <= APC */
FLT_PREOP_CALLBACK_STATUS D14PreStreamTouch (
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__out PVOID *CompletionContext )
{
	/* needed only for stream files because we don't see the create */
	if (FlagOn(Data->Iopb->TargetFileObject->Flags, FO_STREAM_FILE))
		D14TouchContexts(Data);

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

/* called at IRQL <= APC */
FLT_PREOP_CALLBACK_STATUS D14PreStreamTouchWithCallback (
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__out PVOID *CompletionContext )
{
	/* needed only for stream files because we don't see the create */
	if (FlagOn(Data->Iopb->TargetFileObject->Flags, FO_STREAM_FILE))
		D14TouchContexts(Data);

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

/*
 * Unsafe post operations (IRQL <= DPC)
 */

FLT_POSTOP_CALLBACK_STATUS D14PostSetInformation (
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags )
{
	FLT_POSTOP_CALLBACK_STATUS cbstatus;
	BOOLEAN boolstatus;

	if (!NT_SUCCESS(Data->IoStatus.Status))
		return FLT_POSTOP_FINISHED_PROCESSING;

	boolstatus = FltDoCompletionProcessingWhenSafe(Data, FltObjects,
			CompletionContext, Flags, D14PostSetInformationSafe, &cbstatus);
	FATAL_IF(!boolstatus, "Unable to execute post operation at safe IRQL");

	return cbstatus;
}

FLT_POSTOP_CALLBACK_STATUS D14PostQueryInformation (
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags )
{
	FLT_POSTOP_CALLBACK_STATUS cbstatus;
	BOOLEAN boolstatus;

	if (!NT_SUCCESS(Data->IoStatus.Status))
		return FLT_POSTOP_FINISHED_PROCESSING;

	boolstatus = FltDoCompletionProcessingWhenSafe(Data, FltObjects,
			CompletionContext, Flags, D14PostSetInformationSafe, &cbstatus);
	FATAL_IF(!boolstatus, "Unable to execute post operation at safe IRQL");

	return cbstatus;
}

FLT_PREOP_CALLBACK_STATUS D14PreCleanup (
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__out PVOID *CompletionContext )
{
	PFILE_OBJECT fo;
//	if (FltObjects->FileObject->SectionObjectPointer)
//		CcPurgeCacheSection(FltObjects->FileObject->SectionObjectPointer, NULL, 0, FALSE);
	if (CcIsFileCached(FltObjects->FileObject)) {
/*
		CcFlushCache(FltObjects->FileObject->SectionObjectPointer, NULL, 0, NULL);
		CcPurgeCacheSection(FltObjects->FileObject->SectionObjectPointer, NULL, 0, TRUE);
*/
/*
		fo = CcGetFileObjectFromSectionPtrs(FltObjects->FileObject->SectionObjectPointer);
		DbgPrint("D14PreCleanup: FileObject: 0x%p\n", FltObjects->FileObject);
		DbgPrint("              FileObject for cache: 0x%p\n", fo);
*/
		MmForceSectionClosed(FltObjects->FileObject->SectionObjectPointer, TRUE);
/*
		if (fo == FltObjects->FileObject) {
			if(CcUninitializeCacheMap(FltObjects->FileObject, NULL, NULL)) {
				DbgPrint("              CcUninitializeCacheMap succeded\n");
				//DbgPrint("              FileObject for cache after CcUninitializeCacheMap: 0x%p\n", fo);
			} else {
				DbgPrint("              CcUninitializeCacheMap failed\n");
			}
		}
*/
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

/*
 * Trapped callbacks 
 */

const FLT_OPERATION_REGISTRATION D14Callbacks[] = {

	{ IRP_MJ_CREATE,
		0,
		NULL,
		D14PostCreate, },

	{ IRP_MJ_SET_INFORMATION,
		0,
		D14PreStreamTouchWithCallback,
		D14PostSetInformation, },

	{ IRP_MJ_QUERY_INFORMATION,
		0,
		D14PreStreamTouch,
		NULL, },

	{ IRP_MJ_READ,
		0,
		D14PreStreamTouch,
		NULL, },

	{ IRP_MJ_WRITE,
		0,
		D14PreStreamTouch,
		NULL, },

	{ IRP_MJ_CLEANUP,
		0,
		D14PreCleanup,
		NULL, },

	{ IRP_MJ_CLOSE,
		0,
		NULL,
		NULL, },

	{ IRP_MJ_OPERATION_END }
};
