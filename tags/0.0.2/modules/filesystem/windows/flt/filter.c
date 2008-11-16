/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14flt.h"

NTSTATUS D14FilterInit ()
{
	NTSTATUS status;

	D14ContextInit();

	status = FltRegisterFilter(FilterGlobals.DriverObject, &D14Registration,
			&FilterGlobals.FilterHandle);
	if (!NT_SUCCESS(status)) {
		D14ContextExit();
		return status;
	}

	status = FltStartFiltering(FilterGlobals.FilterHandle);	
	if (!NT_SUCCESS(status)) {
		FltUnregisterFilter(FilterGlobals.FilterHandle);
		D14ContextExit();
	}

	return status;
}

VOID D14FilterExit ()
{
	FltUnregisterFilter(FilterGlobals.FilterHandle);
	D14ContextExit();
}

/*
 * D14QueryTeardown
 * 
 * PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK implementation. Does nothing but
 * successfully. It is needed because the default behaviour is to fail.
 */

NTSTATUS D14QueryTeardown (
	IN PCFLT_RELATED_OBJECTS FltObjects,
	IN FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags )
{
	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( Flags );
	return STATUS_SUCCESS;
}

/*
 * Filter registration structure
 */

const FLT_REGISTRATION D14Registration = {
	sizeof(FLT_REGISTRATION), // Size
	FLT_REGISTRATION_VERSION, // Version
	0,                        // Flags

	D14Contexts,               // FLT_CONTEXT_REGISTRATION
	D14Callbacks,              // FLT_OPERATION_REGISTRATION
    
	DriverUnload,                 // PFLT_FILTER_UNLOAD_CALLBACK

	NULL,                     // PFLT_INSTANCE_SETUP_CALLBACK
	D14QueryTeardown,          // PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK
	NULL,                     // PFLT_INSTANCE_TEARDOWN_CALLBACK (begin)
	NULL,                     // PFLT_INSTANCE_TEARDOWN_CALLBACK (end)

	NULL,                     // PFLT_GENERATE_FILE_NAME
	NULL,                     // PFLT_NORMALIZE_NAME_COMPONENT
	NULL                      // PFLT_NORMALIZE_CONTEXT_CLEANUP
};
