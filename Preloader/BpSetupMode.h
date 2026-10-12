/*--

Copyright (c) chainloaded.  All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
PURPOSE.

    Module Name:
        BpSetupMode

    Abstract: 
        Implements setup mode and UI for the preloader application.

    Environment:
        Boot environment only.

--*/

#pragma once

#include "BpMain.h"

#define SETUP_MODE_REASON_NO_CONFIGURATION      1
#define SETUP_MODE_REASON_PRIMARY_LOAD_FAILED   2
#define SETUP_MODE_REASON_USER_REQUESTED        3
#define SETUP_MODE_REASON_UNKNOWN               4
#define SETUP_MODE_REASON_SECURE_BOOT_REQUIRED  5

#define BP_SM_SCAN_ESC              0x0017
#define BP_SM_WAIT_TIMEOUT_US       2000000     // 2 seconds
#define BP_SM_POLL_INTERVAL_US      50000       // 50 ms

#define BP_SM_LINE_MAX_CHARS        256
#define BP_SM_MAX_TOKENS            4   

typedef EFI_STATUS (*BP_SM_COMMAND_HANDLER)(
    _In_ CHAR16 **Arguments
);

typedef struct _BP_SM_COMMAND
{
    CHAR16                  *Name;
    UINT32                  ArgumentCount;
    CHAR16                  *Syntax;
    CHAR16                  *Description;
    BP_SM_COMMAND_HANDLER   Handler;
} BP_SM_COMMAND, *PBP_SM_COMMAND;

EFI_STATUS BpSmPrimaryWait(
    _Out_ BOOLEAN *EscPressed
);

VOID BpSmEnterSetupMode(
    _In_ UINT32 Reason
);