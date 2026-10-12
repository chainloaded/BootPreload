/*--

Copyright (c) chainloaded.  All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
PURPOSE.

    Module Name:
        BpConfiguration

    Abstract: 
        Implements configuration management for the preloader application.

    Environment:
        Boot environment only.

--*/

#pragma once

#include "BpMain.h"

#define BP_CM_VARIABLE_ATTRIBUTES           (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)
#define BP_ARRAY_COUNT(Array)               (sizeof(Array) / sizeof((Array)[0]))

typedef struct _BP_LOAD_ENTRY
{
    UINT32 Id;                  // Unique identifier for the load entry
    CHAR16 FriendlyName[32];    // User-friendly name for the load entry
    CHAR16 EspPath[256];        // Path to the ESP file for this entry
    BOOLEAN SecureBootRequired; // Indicates if Secure Boot is required for this entry
 } BP_LOAD_ENTRY, *PBP_LOAD_ENTRY;

EFI_STATUS BpCmReadAppConfiguration(
    _Out_ PBP_APP_CONFIGURATION Configuration
);

EFI_STATUS BpCmReadLoadEntries(
    _Out_ PBP_LOAD_ENTRY    *LoadEntries,
    _Out_ UINT64            *EntryCount
);

EFI_STATUS BpCmGetAppConfiguration(
    _Out_ PBP_APP_CONFIGURATION Configuration
);

EFI_STATUS BpCmAddLoadEntry(
    _In_  CHAR16    *FriendlyName,
    _In_  CHAR16    *EspPath,
    _In_  BOOLEAN   SecureBootRequired,
    _Out_ UINT32    *EntryId
);

EFI_STATUS BpCmRemoveLoadEntry(
    _In_ UINT32 EntryId
);

EFI_STATUS BpCmSetPrimaryEntry(
    _In_ UINT32 EntryId
);

EFI_STATUS BpCmSetAutoLoad(
    _In_ BOOLEAN AutoLoad
);