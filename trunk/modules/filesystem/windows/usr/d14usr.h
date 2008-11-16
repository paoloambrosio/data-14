#ifndef D14USR_H_
#define D14USR_H_

#include <stdio.h>
#include <windows.h>
#include <fltUser.h>
#include "comm.h"
#include "plugin.h"

typedef struct _FILTER_MESSAGE {
    FILTER_MESSAGE_HEADER FilterMessageHeader;
    D14_USRMESSAGE FilterMessageBody;
} FILTER_MESSAGE, *PFILTER_MESSAGE;

FILTER_MESSAGE message;
ULONG command;

#endif /*D14USR_H_*/
