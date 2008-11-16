/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14flt.h"

VOID OnDelete ( VOID )
{
	PD14_MESSAGE kMessage;

	kMessage = D14AllocMessage(); 
	if (kMessage) {
		D14QueueMessage(kMessage);
	} else {
		DEBUG("Message alloc failed");
	}
}
