/*--

Copyright (c) chainloaded.  All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
PURPOSE.

    Module Name:
        BpString

    Abstract: 
        Implements various string helpers for the application.

    Environment:
        Boot environment only.

--*/

#pragma once

#include "BpMain.h"

UINT64 BpRtlStringLength(
    _In_ CHAR16 *String
);

CHAR16 BpRtlCharToUpper(
    _In_ CHAR16 Character
);

BOOLEAN BpRtlStringEqualsInsensitive(
    _In_ CHAR16 *Left,
    _In_ CHAR16 *Right
);