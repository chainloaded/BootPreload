/*--

Copyright (c) chainloaded.  All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
PURPOSE.

    Module Name:
        BpLoader

    Abstract: 
        Implements the preloader for loading and executing boot applications.

    Environment:
        Boot environment only.

--*/

#pragma once

#include "BpMain.h"

EFI_STATUS BpLdrLoadEntryById(
    _In_ UINT64 EntryId
);