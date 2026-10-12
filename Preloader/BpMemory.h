/*--

Copyright (c) chainloaded.  All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
PURPOSE.

    Module Name:
        BpMemory

    Abstract: 
        Implements memory services for the preloader application.

    Environment:
        Boot environment only.

--*/

#pragma once

#include "BpMain.h"

EFI_STATUS BpMmAllocatePool(
    _In_    UINT64      AllocationSize,
    _Out_   VOID **Allocation
);

void BpMmFreePool(
    _Inout_ VOID **Allocation
);