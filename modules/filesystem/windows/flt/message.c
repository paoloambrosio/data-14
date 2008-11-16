/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Message management and queuing
 */

#include "d14flt.h"

LIST_ENTRY MessageList;
KSPIN_LOCK MessageLock;
NPAGED_LOOKASIDE_LIST MessageCache;
KEVENT MessageEvent;
LONG CurrentMessageID;

PD14_MESSAGE D14AllocMessage ( VOID )
{
	PD14_MESSAGE Message;

	Message = ExAllocateFromNPagedLookasideList(&MessageCache);

	if (Message) {
		InitializeListHead(&Message->List);
		Message->MessageHead.MessageID = 0;
		/************************************************** Initialization  of other fields ********/
	}

	return Message;
}

VOID D14FreeMessage ( PD14_MESSAGE Message )
{
	if (!Message)
		return;

	/************************************************** Fields deinitialization ********/

	ExFreeToNPagedLookasideList(&MessageCache, Message);
}

VOID D14QueueMessage ( PD14_MESSAGE Message )
{
	KIRQL OldIrql;

	KeAcquireSpinLock(&MessageLock, &OldIrql);
	Message->MessageHead.MessageID = InterlockedIncrement(&CurrentMessageID);
	InsertTailList(&MessageList, &Message->List);
	KeReleaseSpinLock(&MessageLock, OldIrql);

	KeSetEvent(&MessageEvent, 0, FALSE);
}

VOID D14DequeueMessage ( PD14_MESSAGE Message )
{
	KIRQL OldIrql;

	KeAcquireSpinLock(&MessageLock, &OldIrql);
	RemoveEntryList(&Message->List);
	KeReleaseSpinLock(&MessageLock, OldIrql);
}

PD14_MESSAGE D14FirstMessage ( VOID )
{
	NTSTATUS status;
	PD14_MESSAGE Message;
	KIRQL OldIrql;
/*
	if (IsListEmpty(&MessageList)) {
		status = KeWaitForSingleObject(&MessageEvent, UserRequest, UserMode, TRUE, NULL);
		if (!NT_SUCCESS(status))
			return NULL;
	}
*/
	KeAcquireSpinLock(&MessageLock, &OldIrql);
	if (IsListEmpty(&MessageList)) {
		KeReleaseSpinLock(&MessageLock, OldIrql);
		return NULL;
	}

	Message = CONTAINING_RECORD(MessageList.Flink, D14_MESSAGE, List);
	KeReleaseSpinLock(&MessageLock, OldIrql);

	return Message;
}

VOID D14MessageInit ()
{
	KeInitializeSpinLock(&MessageLock);
	InitializeListHead(&MessageList);
	KeInitializeEvent(&MessageEvent, SynchronizationEvent, FALSE);
	ExInitializeNPagedLookasideList(&MessageCache,
		NULL, NULL, 0, D14_MESSAGE_SIZE, D14_MESSAGE_TAG, 0);
	CurrentMessageID = 0;
}

VOID D14MessageExit ()
{
	ExDeleteNPagedLookasideList(&MessageCache);
}
