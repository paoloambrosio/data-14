/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14usr.h"

int _cdecl main (
		__in int argc,
		__in_ecount(argc) char *argv[] )
{
	HANDLE port = INVALID_HANDLE_VALUE;
	HRESULT hResult = S_OK;
	DWORD bytesReturned;

	printf("Connecting to filter's port... ");

	hResult = FilterConnectCommunicationPort(D14_PORT_NAME,
			0, NULL, 0, NULL, &port);
	if (IS_ERROR(hResult)) {
		printf("ERROR\n");
		return 1;
	}
	printf("OK\n", port);

	getchar();

	command = CMD_CONNECT;
	hResult = FilterSendMessage(port, &command, sizeof(command),
			&message, sizeof(message), &bytesReturned);
	if (IS_ERROR(hResult))
		printf("Failed to send message (0x%08x)\n", hResult);
	else
		printf("Message sent\n");

	getchar();

	hResult = FilterGetMessage(port, &message.FilterMessageHeader,
			sizeof(message), NULL);
	if (IS_ERROR(hResult))
		printf("Failed to get message (0x%08x)\n", hResult);
	else
		printf("Got message %lu\n", message.FilterMessageBody.MessageHead.MessageID);

	getchar();

	hResult = FilterGetMessage(port, &message.FilterMessageHeader,
			sizeof(message), NULL);
	if (IS_ERROR(hResult))
		printf("Failed to get message (0x%08x)\n", hResult);
	else
		printf("Got message %lu\n", message.FilterMessageBody.MessageHead.MessageID);

	getchar();

	printf("Disconnecting from filter's port... ");
	if (CloseHandle(port))
		printf("OK\n");
	else
		printf("ERROR\n");

	return 0;
}