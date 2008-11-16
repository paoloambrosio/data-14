/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14flt.h"

extern KEVENT MessageEvent;

VOID D14CommWorker(
		__in PFLT_GENERIC_WORKITEM FltWorkItem,
		__in PFLT_FILTER Filter,
	    __in PVOID Context )
{
	NTSTATUS status;
	D14_USRMESSAGE uMessage;
	PD14_MESSAGE kMessage;

	UNREFERENCED_PARAMETER(FltWorkItem);
	UNREFERENCED_PARAMETER(Filter);
	UNREFERENCED_PARAMETER(Context);

	DEBUG("Worker started");

	do {
		kMessage = D14FirstMessage();
		if (!kMessage) {
			DEBUG("WAITING FOR MESSAGE");
			status = KeWaitForSingleObject(&MessageEvent, Executive, KernelMode, TRUE, NULL);
			DEBUG("GOT MESSAGE");
		} else {
			DEBUG("SENDING MESSAGE");
			uMessage.MessageHead.MessageID = kMessage->MessageHead.MessageID;
			status = FltSendMessage(FilterGlobals.FilterHandle,
					&FilterGlobals.ClientPort, &uMessage, sizeof(uMessage),
					NULL, NULL, NULL);
			if (NT_SUCCESS(status)) {
				DEBUG("MESSAGE %d SENT", uMessage.MessageHead.MessageID);
				D14DequeueMessage(kMessage);
			} else {
				DEBUG("MESSAGE NOT SENT");
			}
		}
	} while (!FilterGlobals.StopWorkItem && NT_SUCCESS(status));

	DEBUG("Worker stopped");
}

NTSTATUS D14CommConnect(
		__in PFLT_PORT ClientPort,
		__in PVOID ServerPortCookie,
		__in_bcount(SizeOfContext) PVOID ConnectionContext,
		__in ULONG SizeOfContext,
		__deref_out_opt PVOID *ConnectionCookie )
{
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);

    FilterGlobals.CommWorkItem = NULL;
    FilterGlobals.ClientPort = ClientPort;

    FilterGlobals.StopWorkItem = FALSE;

    return STATUS_SUCCESS;
}

VOID D14CommDisconnect (
		__in_opt PVOID ConnectionCookie )
{
	UNREFERENCED_PARAMETER(ConnectionCookie);

	if (FilterGlobals.CommWorkItem)
		FltFreeGenericWorkItem(FilterGlobals.CommWorkItem);

	if (FilterGlobals.ClientPort)
		FltCloseClientPort(FilterGlobals.FilterHandle, &FilterGlobals.ClientPort);
}

NTSTATUS D14CommMessage (
		__in PVOID ConnectionCookie,
		__in_bcount_opt(InputBufferSize) PVOID InputBuffer,
		__in ULONG InputBufferSize,
		__out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
		__in ULONG OutputBufferSize,
		__out PULONG ReturnOutputBufferLength )
{
	NTSTATUS status;
	ULONG command;
	PD14_USRMESSAGE message;

	if ((InputBuffer == NULL) || (InputBufferSize < sizeof(command)))
		return STATUS_INVALID_PARAMETER;

	try {
		command = *((PULONG) InputBuffer);
	} except( EXCEPTION_EXECUTE_HANDLER ) {
		return GetExceptionCode();
	}

	switch (command) {
	case CMD_CONNECT: /* of course this is not the real connect command! */
		if ((OutputBuffer == NULL) || (OutputBufferSize == 0)) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		if (FilterGlobals.CommWorkItem) {
			status = STATUS_SUCCESS;
			break;
		}
		FilterGlobals.CommWorkItem = FltAllocateGenericWorkItem();
		if (!FilterGlobals.CommWorkItem) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		status = FltQueueGenericWorkItem(FilterGlobals.CommWorkItem,
				FilterGlobals.FilterHandle, D14CommWorker,
				DelayedWorkQueue, NULL);
		if (!NT_SUCCESS(status)) {
			FltFreeGenericWorkItem(FilterGlobals.CommWorkItem);
			FilterGlobals.CommWorkItem = NULL;
		}
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
	}

	return status;
}

NTSTATUS D14CommInit ()
{
	PSECURITY_DESCRIPTOR sd;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uniString;
	NTSTATUS status;

	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (!NT_SUCCESS(status))
		return status;

	RtlInitUnicodeString(&uniString, D14_PORT_NAME);

	InitializeObjectAttributes(&oa, &uniString,
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd);

	status = FltCreateCommunicationPort(FilterGlobals.FilterHandle,
			&FilterGlobals.ServerPort, &oa, NULL, D14CommConnect,
			D14CommDisconnect, D14CommMessage, 1);

	FltFreeSecurityDescriptor(sd);

	return status;
}

VOID D14CommExit ()
{
	FltCloseCommunicationPort(FilterGlobals.ServerPort);
	if (FilterGlobals.ClientPort)
		FltCloseClientPort(FilterGlobals.FilterHandle, &FilterGlobals.ClientPort);

	FilterGlobals.StopWorkItem = TRUE;
	KeSetEvent(&MessageEvent, 0, FALSE);
}
