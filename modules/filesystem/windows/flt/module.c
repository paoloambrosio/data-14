/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Minifilter main functions
 */

#include "d14flt.h"

D14FLT_DATA FilterGlobals;

/*
 * DriverEntry
 * 
 * Initialization routine comon to all instances. It is called on driver load
 * and initializes all the needed structures. Then calls FltRegisterFilter and
 * FltStartFiltering to register within the filter mangager and start filtering.
 */

NTSTATUS DriverEntry (
	__in PDRIVER_OBJECT DriverObject,
	__in PUNICODE_STRING RegistryPath )
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(RegistryPath);

	FilterGlobals.DriverObject = DriverObject;

	D14MessageInit();

	status = D14FilterInit();
	if (!NT_SUCCESS(status)) {
		D14MessageExit();
	}

	D14CommInit();

	return status;
}

/*
 * DriverUnload
 */

NTSTATUS DriverUnload (
	FLT_FILTER_UNLOAD_FLAGS Flags )
{
	UNREFERENCED_PARAMETER(Flags);

	D14CommExit();
	D14FilterExit();
	D14MessageExit();

	return STATUS_SUCCESS;
}
