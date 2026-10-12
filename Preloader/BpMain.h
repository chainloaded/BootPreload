/*--

Copyright (c) chainloaded.  All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
PURPOSE.

    Module Name:
        BpMain

    Abstract: 
        Implements the main faculty for the preloader application.

    Environment:
        Boot environment only.

--*/

#pragma once

#include <efi.h>

#define BP_APP_VENDOR_GUID \
    { 0x7f2a3b4c, 0x9d1e, 0x48a5, { 0xbd, 0x32, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } }

typedef struct _BP_APP_CONTEXT
{
    EFI_HANDLE              ImageHandle;
    EFI_SYSTEM_TABLE        *SystemTable;
    EFI_BOOT_SERVICES       *BootServices;
    EFI_RUNTIME_SERVICES    *RuntimeServices;
    EFI_GUID                AppVendorGuid;
} BP_APP_CONTEXT, *PBP_APP_CONTEXT;

typedef struct _BP_APP_CONFIGURATION
{
    BOOLEAN AutoLoad;
    UINT32  PrimaryId;
} BP_APP_CONFIGURATION, *PBP_APP_CONFIGURATION;

extern BP_APP_CONTEXT       g_AppContext;
extern EFI_GUID             g_EfiDevicePathProtocolGuid;
extern EFI_GUID             g_EfiDevicePathUtilitiesProtocolGuid;
extern EFI_GUID             g_EfiLoadedImageProtocolGuid;