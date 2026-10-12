#include "BpSetupMode.h"
#include "BpConfiguration.h"
#include "BpMemory.h"
#include "BpLoader.h"
#include "BpString.h"

EFI_STATUS BpSmPrimaryWait(
    _Out_ BOOLEAN *EscPressed
)
/*++
    Routine Description:
        Waits 2 seconds to let the user enter setup mode.

    Arguments:

        EscPressed:
            On return, indicates if ESC was pressed.

    Return Value:
        EFI status indicating success or failure.
--*/
{
    EFI_STATUS                      Result      = EFI_SUCCESS;
    EFI_BOOT_SERVICES               *BS         = g_AppContext.BootServices;
    SIMPLE_TEXT_OUTPUT_INTERFACE    *ConOut     = g_AppContext.SystemTable->ConOut;
    SIMPLE_INPUT_INTERFACE          *ConIn      = g_AppContext.SystemTable->ConIn;
    EFI_INPUT_KEY                   Key         = { 0 };
    UINT64                          Elapsed     = 0;

    if (EscPressed == NULL)
        return EFI_INVALID_PARAMETER;

    // Assume ESC was not pressed until proven otherwise
    *EscPressed = FALSE;

    if (ConOut == NULL || ConIn == NULL)
        return EFI_UNSUPPORTED;

    // Print the prompt
    Result = ConOut->OutputString(
        ConOut,
        L"Booting primary entry. Press ESC to enter setup mode...\r\n"
    );

    if (EFI_ERROR(Result))
        goto Done;

    // Discard any keystrokes that were buffered before the prompt was shown
    Result = ConIn->Reset(ConIn, FALSE);

    if (EFI_ERROR(Result))
        goto Done;

    // Poll the keyboard until the timeout expires or ESC is pressed
    while (Elapsed < BP_SM_WAIT_TIMEOUT_US)
    {
        Result = ConIn->ReadKeyStroke(ConIn, &Key);

        if (Result == EFI_SUCCESS)
        {
            if (Key.ScanCode == BP_SM_SCAN_ESC)
            {
                *EscPressed = TRUE;
                break;
            }

            // Any other key is ignored, keep waiting
            continue;
        }

        // EFI_NOT_READY just means that no key is pending
        if (Result != EFI_NOT_READY)
            goto Done;

        BS->Stall(BP_SM_POLL_INTERVAL_US);
        Elapsed += BP_SM_POLL_INTERVAL_US;
    }

    Result = EFI_SUCCESS;

Done:

    return Result;
}

VOID BpSmpPrint(
    _In_ CHAR16 *Text
)
/*++
    Routine Description:
        Prints a string to the console output.

    Arguments:

        Text:
            Pointer to the wide-character string to print.

    Return Value:
        None.
--*/
{
    SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut = g_AppContext.SystemTable->ConOut;

    // Output errors (e.g. unknown glyphs) are not worth aborting for
    ConOut->OutputString(ConOut, Text);
}

VOID BpSmpPrintUint32(
    _In_ UINT32 Value
)
/*++
    Routine Description:
        Prints a 32-bit unsigned integer to the console output.

    Arguments:

        Value:
            The integer value to print.

    Return Value:
        None.
--*/
{
    CHAR16  Buffer[11]  = { 0 };    // 10 digits + terminator
    UINT32  Position    = 10;

    // Fill the buffer from the back
    do
    {
        Buffer[--Position] = (CHAR16)(L'0' + (Value % 10));
        Value /= 10;
    } while (Value != 0);

    BpSmpPrint(&Buffer[Position]);
}

VOID BpSmpPrintHex64(
    _In_ UINT64 Value
)
/*++
    Routine Description:
        Prints a 64-bit integer in hexadecimal format to the console output.

    Arguments:

        Value:
            The integer value to print in hexadecimal.

    Return Value:
        None.
--*/
{
    CHAR16  Buffer[17]  = { 0 };    // 16 digits + terminator
    UINT32  Index       = 0;
    UINT32  Nibble      = 0;

    for (Index = 0; Index < 16; Index++)
    {
        Nibble = (UINT32)((Value >> ((15 - Index) * 4)) & 0xF);

        Buffer[Index] = (CHAR16)(Nibble < 10 ? L'0' + Nibble : L'A' + (Nibble - 10));
    }

    BpSmpPrint(L"0x");
    BpSmpPrint(Buffer);
}

VOID BpSmpPrintError(
    _In_ EFI_STATUS Status
)
{
    CHAR16 *Description = L"Command failed";

    switch (Status)
    {
    case EFI_INVALID_PARAMETER:
        Description = L"Invalid argument";
        break;

    case EFI_NOT_FOUND:
        Description = L"Not found";
        break;

    case EFI_OUT_OF_RESOURCES:
        Description = L"Out of resources";
        break;

    case EFI_SECURITY_VIOLATION:
        Description = L"Security violation";
        break;

    case EFI_NOT_READY:
        Description = L"Precondition not met (no primary entry set)";
        break;
    }

    BpSmpPrint(L"Error: ");
    BpSmpPrint(Description);
    BpSmpPrint(L" (status ");
    BpSmpPrintHex64(Status);
    BpSmpPrint(L")\r\n");
}

CHAR16 *BpSmpGetReasonText(
    _In_ UINT32 Reason
)
/*++
    Routine Description:
        Returns a descriptive message explaining why setup mode was entered.

    Arguments:

        Reason:
            The reason code indicating why setup mode is active.

    Return Value:
        Pointer to a static descriptive string.
--*/
{
    switch (Reason)
    {
    case SETUP_MODE_REASON_NO_CONFIGURATION:
        return L"No valid configuration was found. Add a boot entry with AddEntry and select it with SetPrimary.";

    case SETUP_MODE_REASON_PRIMARY_LOAD_FAILED:
        return L"The primary boot entry failed to load. Check it with ListEntries or select a different primary entry.";

    case SETUP_MODE_REASON_USER_REQUESTED:
        return L"Setup mode was requested by the user.";

    case SETUP_MODE_REASON_SECURE_BOOT_REQUIRED:
        return L"The boot entry requires Secure Boot, but Secure Boot is not enabled. "
            L"Enable Secure Boot in the firmware settings or use an entry that does not require it.";

    case SETUP_MODE_REASON_UNKNOWN:
    default:
        return L"Setup mode was entered for an unknown reason.";
    }
}


//
// Setup mode: console input
//

EFI_STATUS BpSmpReadLine(
    _Out_ CHAR16    *Buffer,
    _In_  UINT64    BufferChars
)
/*++
    Routine Description:
        Reads a line of input from the console, with backspace editing support.

    Arguments:

        Buffer:
            Pointer to receive the input line.

        BufferChars:
            Maximum number of characters that can be stored in the buffer.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS              Result      = EFI_SUCCESS;
    EFI_BOOT_SERVICES       *BS         = g_AppContext.BootServices;
    SIMPLE_INPUT_INTERFACE  *ConIn      = g_AppContext.SystemTable->ConIn;
    EFI_INPUT_KEY           Key         = { 0 };
    UINT64                  Length      = 0;
    UINTN                   EventIndex  = 0;
    CHAR16                  Echo[2]     = { 0, 0 };

    Buffer[0] = L'\0';

    for (;;)
    {
        // Sleep until a key is available
        Result = BS->WaitForEvent(1, &ConIn->WaitForKey, &EventIndex);

        if (EFI_ERROR(Result))
            goto Done;

        Result = ConIn->ReadKeyStroke(ConIn, &Key);

        // Spurious wake-up
        if (Result == EFI_NOT_READY)
            continue;

        if (EFI_ERROR(Result))
            goto Done;

        // Enter finishes the line
        if (Key.UnicodeChar == L'\r' || Key.UnicodeChar == L'\n')
        {
            BpSmpPrint(L"\r\n");
            break;
        }

        // Backspace (or DEL, which serial consoles send) removes the last character
        if (Key.UnicodeChar == L'\b' || Key.UnicodeChar == 0x7F)
        {
            if (Length > 0)
            {
                Length--;
                Buffer[Length] = L'\0';

                BpSmpPrint(L"\b \b");
            }

            continue;
        }

        // Ignore function keys (no unicode character) and other control characters
        if (Key.UnicodeChar < L' ')
            continue;

        // Keep room for the terminator
        if (Length + 1 >= BufferChars)
            continue;

        Buffer[Length++]    = Key.UnicodeChar;
        Buffer[Length]      = L'\0';

        Echo[0] = Key.UnicodeChar;
        BpSmpPrint(Echo);
    }

Done:

    return Result;
}

EFI_STATUS BpSmpTokenize(
    _Inout_ CHAR16  *Line,
    _Out_   CHAR16  **Tokens,
    _In_    UINT32  MaxTokens,
    _Out_   UINT32  *TokenCount
)
/*++
    Routine Description:
        Splits a string into whitespace-delimited tokens.

    Arguments:

        Line:
            Pointer to the input line (modified in place).

        Tokens:
            Pointer to receive the array of token pointers.

        MaxTokens:
            Maximum number of tokens that can be stored.

        TokenCount:
            Pointer to receive the number of tokens found.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS  Result  = EFI_SUCCESS;
    CHAR16      *Cursor = Line;

    *TokenCount = 0;

    while (*Cursor != L'\0')
    {
        // Skip separators
        if (*Cursor == L' ')
        {
            Cursor++;
            continue;
        }

        if (*TokenCount >= MaxTokens)
        {
            Result = EFI_INVALID_PARAMETER;
            goto Done;
        }

        if (*Cursor == L'"')
        {
            // Quoted token, may contain spaces
            Cursor++;
            Tokens[(*TokenCount)++] = Cursor;

            while (*Cursor != L'\0' && *Cursor != L'"')
                Cursor++;

            // Unterminated quote
            if (*Cursor == L'\0')
            {
                Result = EFI_INVALID_PARAMETER;
                goto Done;
            }
        }
        else
        {
            Tokens[(*TokenCount)++] = Cursor;

            while (*Cursor != L'\0' && *Cursor != L' ')
                Cursor++;
        }

        // Terminate the token in place
        if (*Cursor != L'\0')
        {
            *Cursor = L'\0';
            Cursor++;
        }
    }

Done:

    return Result;
}

EFI_STATUS BpSmpParseEntryId(
    _In_  CHAR16 *Text,
    _Out_ UINT32 *EntryId
)
/*++
    Routine Description:
        Parses a decimal string into a 32-bit unsigned integer entry ID.

    Arguments:

        Text:
            Pointer to the decimal string to parse.

        EntryId:
            Pointer to receive the parsed entry ID.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    UINT64 Value = 0;
    UINT64 Index = 0;

    if (Text[0] == L'\0')
        return EFI_INVALID_PARAMETER;

    for (Index = 0; Text[Index] != L'\0'; Index++)
    {
        if (Text[Index] < L'0' || Text[Index] > L'9')
            return EFI_INVALID_PARAMETER;

        Value = Value * 10 + (Text[Index] - L'0');

        // Reject values that do not fit into 32 bit
        if (Value > 0xFFFFFFFF)
            return EFI_INVALID_PARAMETER;
    }

    // ID 0 is reserved and never valid
    if (Value == 0)
        return EFI_INVALID_PARAMETER;

    *EntryId = (UINT32)Value;

    return EFI_SUCCESS;
}

EFI_STATUS BpSmpParseYesNo(
    _In_  CHAR16    *Text,
    _Out_ BOOLEAN   *Value
)
/*++
    Routine Description:
        Parses a yes/no string into a boolean value.

    Arguments:

        Text:
            Pointer to the string containing "y", "yes", "n", or "no".

        Value:
            Pointer to receive the boolean value.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS Result = EFI_SUCCESS;

    if (BpRtlStringEqualsInsensitive(Text, L"y") || BpRtlStringEqualsInsensitive(Text, L"yes"))
        *Value = TRUE;
    else if (BpRtlStringEqualsInsensitive(Text, L"n") || BpRtlStringEqualsInsensitive(Text, L"no"))
        *Value = FALSE;
    else
        Result = EFI_INVALID_PARAMETER;

    return Result;
}


//
// Setup mode: command handlers
//

EFI_STATUS BpSmpCmdExit(
    _In_ CHAR16 **Arguments
)
/*++
    Routine Description:
        Handles the exit command, which shuts down the system.

    Arguments:

        Arguments:
            Pointer to the array of command arguments (unused).

    Return Value:
        EFI_STATUS indicating success or device error if shutdown fails.
--*/
{
    EFI_RUNTIME_SERVICES *RT = g_AppContext.RuntimeServices;

    (VOID)Arguments;

    BpSmpPrint(L"Shutting down...\r\n");

    RT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

    // Only reached if the firmware failed to shut the device down
    return EFI_DEVICE_ERROR;
}

EFI_STATUS BpSmpCmdAddEntry(
    _In_ CHAR16 **Arguments
)
/*++
    Routine Description:
        Handles the AddEntry command, which adds a new boot entry to the configuration.

    Arguments:

        Arguments:
            Pointer to the array of command arguments (path and optional secure boot flag).

    Return Value:
        EFI_STATUS indicating success or relevant error.
--*/
{
    EFI_STATUS  Result              = EFI_SUCCESS;
    BOOLEAN     SecureBootRequired  = FALSE;
    UINT32      EntryId             = 0;

    Result = BpSmpParseYesNo(Arguments[2], &SecureBootRequired);

    if (EFI_ERROR(Result))
        goto Done;

    Result = BpCmAddLoadEntry(Arguments[0], Arguments[1], SecureBootRequired, &EntryId);

    if (EFI_ERROR(Result))
        goto Done;

    BpSmpPrint(L"Created boot entry with ID ");
    BpSmpPrintUint32(EntryId);
    BpSmpPrint(L".\r\n");

Done:

    return Result;
}

EFI_STATUS BpSmpCmdListEntries(
    _In_ CHAR16 **Arguments
)
/*++
    Routine Description:
        Handles the ListEntries command, which displays all configured boot entries.

    Arguments:

        Arguments:
            Pointer to the array of command arguments (unused).

    Return Value:
        EFI_STATUS indicating success or relevant error.
--*/
{
    EFI_STATUS              Result  = EFI_SUCCESS;
    PBP_LOAD_ENTRY          Entries = NULL;
    UINT64                  Count   = 0;
    UINT64                  Index   = 0;
    BP_APP_CONFIGURATION    Configuration;

    (VOID)Arguments;

    Result = BpCmGetAppConfiguration(&Configuration);

    if (EFI_ERROR(Result))
        goto Done;

    BpSmpPrint(L"Auto load: ");
    BpSmpPrint(Configuration.AutoLoad ? L"enabled\r\n" : L"disabled\r\n");

    Result = BpCmReadLoadEntries(&Entries, &Count);

    // No database means no entries
    if (Result == EFI_NOT_FOUND)
    {
        BpSmpPrint(L"No boot entries installed.\r\n");
        Result = EFI_SUCCESS;
        goto Done;
    }

    if (EFI_ERROR(Result))
        goto Done;

    for (Index = 0; Index < Count; Index++)
    {
        // Never trust the terminators of data read from NVRAM
        Entries[Index].FriendlyName[BP_ARRAY_COUNT(Entries[Index].FriendlyName) - 1] = L'\0';
        Entries[Index].EspPath[BP_ARRAY_COUNT(Entries[Index].EspPath) - 1] = L'\0';

        BpSmpPrint(L"  [");
        BpSmpPrintUint32(Entries[Index].Id);
        BpSmpPrint(L"] ");
        BpSmpPrint(Entries[Index].FriendlyName);

        if (Entries[Index].Id == Configuration.PrimaryId)
            BpSmpPrint(L" (primary)");

        BpSmpPrint(L"\r\n      Path: ");
        BpSmpPrint(Entries[Index].EspPath);
        BpSmpPrint(L"\r\n      Secure Boot required: ");
        BpSmpPrint(Entries[Index].SecureBootRequired ? L"yes\r\n" : L"no\r\n");
    }

Done:

    if (Entries != NULL)
        BpMmFreePool((VOID **)&Entries);

    return Result;
}

EFI_STATUS BpSmpCmdRemoveEntry(
    _In_ CHAR16 **Arguments
)
/*++
    Routine Description:
        Handles the RemoveEntry command, which removes a boot entry from the configuration.

    Arguments:

        Arguments:
            Pointer to the array of command arguments (entry ID).

    Return Value:
        EFI_STATUS indicating success or relevant error.
--*/
{
    EFI_STATUS  Result  = EFI_SUCCESS;
    UINT32      EntryId = 0;

    Result = BpSmpParseEntryId(Arguments[0], &EntryId);

    if (EFI_ERROR(Result))
        goto Done;

    Result = BpCmRemoveLoadEntry(EntryId);

    if (EFI_ERROR(Result))
        goto Done;

    BpSmpPrint(L"Removed boot entry ");
    BpSmpPrintUint32(EntryId);
    BpSmpPrint(L". If it was the primary entry, auto load has been disabled.\r\n");

Done:

    return Result;
}

EFI_STATUS BpSmpCmdSetPrimary(
    _In_ CHAR16 **Arguments
)
/*++
    Routine Description:
        Handles the SetPrimary command, which sets the primary boot entry.

    Arguments:

        Arguments:
            Pointer to the array of command arguments (entry ID).

    Return Value:
        EFI_STATUS indicating success or relevant error.
--*/
{
    EFI_STATUS  Result  = EFI_SUCCESS;
    UINT32      EntryId = 0;

    Result = BpSmpParseEntryId(Arguments[0], &EntryId);

    if (EFI_ERROR(Result))
        goto Done;

    Result = BpCmSetPrimaryEntry(EntryId);

    if (EFI_ERROR(Result))
        goto Done;

    BpSmpPrint(L"Boot entry ");
    BpSmpPrintUint32(EntryId);
    BpSmpPrint(L" is now the primary entry.\r\n");

Done:

    return Result;
}

EFI_STATUS BpSmpCmdEnableAutoLoad(
    _In_ CHAR16 **Arguments
)
/*++
    Routine Description:
        Handles the EnableAutoLoad command, which enables or disables automatic boot entry loading.

    Arguments:

        Arguments:
            Pointer to the array of command arguments (yes/no flag).

    Return Value:
        EFI_STATUS indicating success or relevant error.
--*/
{
    EFI_STATUS  Result      = EFI_SUCCESS;
    BOOLEAN     AutoLoad    = FALSE;

    Result = BpSmpParseYesNo(Arguments[0], &AutoLoad);

    if (EFI_ERROR(Result))
        goto Done;

    Result = BpCmSetAutoLoad(AutoLoad);

    if (EFI_ERROR(Result))
        goto Done;

    BpSmpPrint(AutoLoad ? L"Auto load enabled.\r\n" : L"Auto load disabled.\r\n");

Done:

    return Result;
}

EFI_STATUS BpSmpCmdLoadEntry(
    _In_ CHAR16 **Arguments
)
/*++
    Routine Description:
        Handles the LoadEntry command, which loads and executes a boot entry.

    Arguments:

        Arguments:
            Pointer to the array of command arguments (entry ID).

    Return Value:
        EFI_STATUS indicating success or relevant error (does not return on successful load).
--*/
{
    EFI_STATUS  Result  = EFI_SUCCESS;
    UINT32      EntryId = 0;

    Result = BpSmpParseEntryId(Arguments[0], &EntryId);

    if (EFI_ERROR(Result))
        goto Done;

    BpSmpPrint(L"Booting entry ");
    BpSmpPrintUint32(EntryId);
    BpSmpPrint(L"...\r\n");

    // All verification (Secure Boot etc.) is done by the loader
    Result = BpLdrLoadEntryById(EntryId);

Done:

    return Result;
}

// Define valid commands
static BP_SM_COMMAND g_BpSmCommands[] =
{
    {
        L"Exit", 0,
        L"Exit",
        L"Shuts the device down",
        BpSmpCmdExit
    },
    {
        L"AddEntry", 3,
        L"AddEntry [FriendlyName] [Path] [Secure Boot Required (y/n)]",
        L"Creates a new boot entry (use \"quotes\" for names with spaces)",
        BpSmpCmdAddEntry
    },
    {
        L"ListEntries", 0,
        L"ListEntries",
        L"Lists all installed boot entries with their ID",
        BpSmpCmdListEntries
    },
    {
        L"RemoveEntry", 1,
        L"RemoveEntry [ID]",
        L"Deletes an installed boot entry",
        BpSmpCmdRemoveEntry
    },
    {
        L"SetPrimary", 1,
        L"SetPrimary [ID]",
        L"Sets an installed boot entry as primary",
        BpSmpCmdSetPrimary
    },
    {
        L"EnableAutoLoad", 1,
        L"EnableAutoLoad [(y/n)]",
        L"Enables or disables auto load",
        BpSmpCmdEnableAutoLoad
    },
    {
        L"LoadEntry", 1,
        L"LoadEntry [ID]",
        L"Boots an entry by ID",
        BpSmpCmdLoadEntry
    }
};

VOID BpSmpPrintCommands(
    VOID
)
/*++
    Routine Description:
        Prints the list of available setup mode commands and their descriptions.

    Arguments:
        None.

    Return Value:
        None.
--*/
{
    UINT64 Index = 0;

    BpSmpPrint(L"Available commands:\r\n");

    for (Index = 0; Index < BP_ARRAY_COUNT(g_BpSmCommands); Index++)
    {
        BpSmpPrint(L"  ");
        BpSmpPrint(g_BpSmCommands[Index].Syntax);
        BpSmpPrint(L"\r\n      ");
        BpSmpPrint(g_BpSmCommands[Index].Description);
        BpSmpPrint(L"\r\n");
    }
}

EFI_STATUS BpSmpExecuteCommand(
    _Inout_ CHAR16 *Line
)
/*++
    Routine Description:
        Parses a command line and executes the corresponding command handler.

    Arguments:

        Line:
            Pointer to the command line string (modified in place during tokenization).

    Return Value:
        EFI_STATUS indicating success or relevant error.
--*/
{
    EFI_STATUS      Result                      = EFI_SUCCESS;
    PBP_SM_COMMAND  Command                     = NULL;
    CHAR16          *Tokens[BP_SM_MAX_TOKENS]   = { NULL };
    UINT32          TokenCount                  = 0;
    UINT64          Index                       = 0;

    Result = BpSmpTokenize(Line, Tokens, (UINT32)BP_ARRAY_COUNT(Tokens), &TokenCount);

    if (EFI_ERROR(Result))
    {
        BpSmpPrint(L"Invalid command line (too many arguments or unterminated quote).\r\n");
        goto Done;
    }

    // Empty line
    if (TokenCount == 0)
        goto Done;

    for (Index = 0; Index < BP_ARRAY_COUNT(g_BpSmCommands); Index++)
    {
        Command = &g_BpSmCommands[Index];

        if (!BpRtlStringEqualsInsensitive(Tokens[0], Command->Name))
            continue;

        if (TokenCount - 1 != Command->ArgumentCount)
        {
            BpSmpPrint(L"Usage: ");
            BpSmpPrint(Command->Syntax);
            BpSmpPrint(L"\r\n");

            Result = EFI_INVALID_PARAMETER;
            goto Done;
        }

        Result = Command->Handler(&Tokens[1]);

        if (EFI_ERROR(Result))
            BpSmpPrintError(Result);

        goto Done;
    }

    BpSmpPrint(L"Unknown command: ");
    BpSmpPrint(Tokens[0]);
    BpSmpPrint(L"\r\n");

    Result = EFI_NOT_FOUND;

Done:

    return Result;
}

VOID BpSmEnterSetupMode(
    _In_ UINT32 Reason
)
/*++
    Routine Description:
        Enters interactive setup mode, displaying the reason and command prompt.

    Arguments:

        Reason:
            An identifier indicating why setup mode was entered.

    Return Value:
        None.
--*/
{
    EFI_STATUS              Result                      = EFI_SUCCESS;
    EFI_BOOT_SERVICES       *BS                         = g_AppContext.BootServices;
    EFI_RUNTIME_SERVICES    *RT                         = g_AppContext.RuntimeServices;
    CHAR16                  Line[BP_SM_LINE_MAX_CHARS];

    BS->SetMem(Line, sizeof(Line), 0);

    // Header
    BpSmpPrint(L"\r\n");
    BpSmpPrint(L"============================================================\r\n");
    BpSmpPrint(L" BOOTPRELOADER SETUP MODE\r\n");
    BpSmpPrint(L"============================================================\r\n\r\n");

    // Why are we here?
    BpSmpPrint(L"Reason: ");
    BpSmpPrint(BpSmpGetReasonText(Reason));
    BpSmpPrint(L"\r\n\r\n");

    BpSmpPrintCommands();

    for (;;)
    {
        BpSmpPrint(L"\r\nSetup> ");

        Result = BpSmpReadLine(Line, BP_SM_LINE_MAX_CHARS);

        if (EFI_ERROR(Result))
        {
            // Without a working keyboard setup mode is unusable, fail closed
            RT->ResetSystem(EfiResetShutdown, Result, 0, NULL);
            BS->Stall(1000000);
            continue;
        }

        // Errors are reported to the user by the command dispatcher
        BpSmpExecuteCommand(Line);
    }
}