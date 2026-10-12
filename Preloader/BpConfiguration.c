#include "BpConfiguration.h"
#include "BpMemory.h"
#include "BpString.h"

EFI_STATUS BpCmReadLoadEntries(
    _Out_ PBP_LOAD_ENTRY    *LoadEntries,
    _Out_ UINT64            *EntryCount
)
/*++
    Routine Description:
        Reads the boot entry database from UEFI variables.

    Arguments:

        LoadEntries:
            Pointer to receive the allocated boot entry array.

        EntryCount:
            Pointer to receive the number of entries in the array.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS              Result      = EFI_SUCCESS;
    EFI_RUNTIME_SERVICES    *RT         = g_AppContext.RuntimeServices;
    UINT32                  Attributes  = 0;
    UINT64                  DataSize    = 0;

    // Try to get boot entry database variable
    Result = RT->GetVariable(
        L"BootEntryDatabase",
        &g_AppContext.AppVendorGuid,
        &Attributes,
        &DataSize,
        NULL
    );

    if (EFI_ERROR(Result) && Result != EFI_BUFFER_TOO_SMALL)
        goto Done;

    // Check if the variable has runtime access attribute
    if (Attributes & EFI_VARIABLE_RUNTIME_ACCESS || 
        DataSize % sizeof(BP_LOAD_ENTRY) != 0 || 
        DataSize == 0)
    {
        // Delete the invalid variable
        RT->SetVariable(
            L"BootEntryDatabase",
            &g_AppContext.AppVendorGuid,
            Attributes,
            0,
            NULL
        );

        Result = EFI_SECURITY_VIOLATION;
        goto Done;
    }

    // Allocate memory for the load entries
    Result = BpMmAllocatePool(DataSize, (VOID **)LoadEntries);

    if (EFI_ERROR(Result))
        goto Done;

    // Read the load entries into memory
    Result = RT->GetVariable(
        L"BootEntryDatabase",
        &g_AppContext.AppVendorGuid,
        &Attributes,
        &DataSize,
        *LoadEntries
    );

    if (EFI_ERROR(Result))
    {
        BpMmFreePool(LoadEntries);
    }
    else
    {
        // Set the entry count based on the data size and size of BP_LOAD_ENTRY
        *EntryCount = (DataSize / sizeof(BP_LOAD_ENTRY));
    }

Done:

    return Result;
}

EFI_STATUS BpCmReadAppConfiguration(
    _Out_ PBP_APP_CONFIGURATION Configuration
)
/*++
    Routine Description:
        Reads the application configuration from UEFI variables.

    Arguments:

        Configuration:
            Pointer to receive the application configuration.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS              Result      = EFI_SUCCESS;
    EFI_RUNTIME_SERVICES    *RT         = g_AppContext.RuntimeServices;
    UINT32                  Attributes  = 0;
    UINT64                  DataSize    = sizeof(BP_APP_CONFIGURATION);

    // Try to read the configuration into memory
    Result = RT->GetVariable(
        L"AppConfiguration",
        &g_AppContext.AppVendorGuid,
        &Attributes,
        &DataSize,
        Configuration
    );

    if (EFI_ERROR(Result))
        goto Done;

    // Check if the variable has runtime access attribute
    if (Attributes & EFI_VARIABLE_RUNTIME_ACCESS || DataSize != sizeof(BP_APP_CONFIGURATION))
    {
        // Delete the invalid variable
        RT->SetVariable(
            L"AppConfiguration",
            &g_AppContext.AppVendorGuid,
            Attributes,
            0,
            NULL
        );

        Result = EFI_SECURITY_VIOLATION;
        goto Done;
    }

Done:

    return Result;
}

EFI_STATUS BpCmpWriteVariable(
    _In_     CHAR16 *Name,
    _In_     UINT64 DataSize,
    _In_opt_ VOID   *Data
)
/*++
    Routine Description:
        Writes or deletes a UEFI variable owned by the application.

    Arguments:

        Name:
            The name of the variable to write.

        DataSize:
            The size of the data, or zero to delete the variable.

        Data:
            Pointer to the data to write, or NULL if deleting.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS              Result      = EFI_SUCCESS;
    EFI_RUNTIME_SERVICES    *RT         = g_AppContext.RuntimeServices;

    // A DataSize of zero deletes the variable
    Result = RT->SetVariable(
        Name,
        &g_AppContext.AppVendorGuid,
        BP_CM_VARIABLE_ATTRIBUTES,
        DataSize,
        Data
    );

    // Deleting a variable that does not exist is not an error
    if (DataSize == 0 && Result == EFI_NOT_FOUND)
        Result = EFI_SUCCESS;

    return Result;
}

EFI_STATUS BpCmpWriteLoadEntries(
    _In_opt_ PBP_LOAD_ENTRY LoadEntries,
    _In_     UINT64         EntryCount
)
/*++
    Routine Description:
        Writes the boot entry database to UEFI variables.

    Arguments:

        LoadEntries:
            Pointer to the boot entry array, or NULL to delete.

        EntryCount:
            The number of entries in the array.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    return BpCmpWriteVariable(
        L"BootEntryDatabase",
        EntryCount * sizeof(BP_LOAD_ENTRY),
        LoadEntries
    );
}

EFI_STATUS BpCmpWriteAppConfiguration(
    _In_ PBP_APP_CONFIGURATION Configuration
)
/*++
    Routine Description:
        Writes the application configuration to UEFI variables.

    Arguments:

        Configuration:
            Pointer to the application configuration to write.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    return BpCmpWriteVariable(
        L"AppConfiguration",
        sizeof(BP_APP_CONFIGURATION),
        Configuration
    );
}

EFI_STATUS BpCmpCheckLoadEntryExists(
    _In_ UINT32 EntryId
)
/*++
    Routine Description:
        Checks whether a load entry with the specified ID exists.

    Arguments:

        EntryId:
            The unique identifier of the entry to check.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS      Result  = EFI_SUCCESS;
    PBP_LOAD_ENTRY  Entries = NULL;
    UINT64          Count   = 0;
    UINT64          Index   = 0;

    Result = BpCmReadLoadEntries(&Entries, &Count);

    if (EFI_ERROR(Result))
        goto Done;

    Result = EFI_NOT_FOUND;

    for (Index = 0; Index < Count; Index++)
    {
        if (Entries[Index].Id == EntryId)
        {
            Result = EFI_SUCCESS;
            break;
        }
    }

Done:

    if (Entries != NULL)
        BpMmFreePool((VOID **)&Entries);

    return Result;
}

EFI_STATUS BpCmGetAppConfiguration(
    _Out_ PBP_APP_CONFIGURATION Configuration
)
/*++
    Routine Description:
        Gets the application configuration with default values if not stored.

    Arguments:

        Configuration:
            Pointer to receive the application configuration.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS          Result  = EFI_SUCCESS;
    EFI_BOOT_SERVICES   *BS     = g_AppContext.BootServices;

    // Start from zeroed defaults
    BS->SetMem(Configuration, sizeof(BP_APP_CONFIGURATION), 0);

    Result = BpCmReadAppConfiguration(Configuration);

    // No configuration stored yet, keep the defaults
    if (Result == EFI_NOT_FOUND)
        Result = EFI_SUCCESS;

    return Result;
}

EFI_STATUS BpCmAddLoadEntry(
    _In_  CHAR16    *FriendlyName,
    _In_  CHAR16    *EspPath,
    _In_  BOOLEAN   SecureBootRequired,
    _Out_ UINT32    *EntryId
)
/*++
    Routine Description:
        Adds a new boot entry to the database and returns its ID.

    Arguments:

        FriendlyName:
            A user-friendly name for the new entry.

        EspPath:
            The file path relative to the ESP for the image to boot.

        SecureBootRequired:
            Whether this entry requires secure boot to be enabled.

        EntryId:
            Pointer to receive the unique identifier of the new entry.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS          Result      = EFI_SUCCESS;
    EFI_BOOT_SERVICES   *BS         = g_AppContext.BootServices;
    PBP_LOAD_ENTRY      OldEntries  = NULL;
    PBP_LOAD_ENTRY      NewEntries  = NULL;
    UINT64              OldCount    = 0;
    UINT64              Index       = 0;
    UINT64              NameLength  = 0;
    UINT64              PathLength  = 0;
    UINT32              NewId       = 1;
    BP_LOAD_ENTRY       NewEntry;

    if (FriendlyName == NULL || EspPath == NULL || EntryId == NULL)
        return EFI_INVALID_PARAMETER;

    NameLength = BpRtlStringLength(FriendlyName);
    PathLength = BpRtlStringLength(EspPath);

    // Both strings must be non-empty and fit including the terminator
    if (NameLength == 0 || NameLength >= BP_ARRAY_COUNT(NewEntry.FriendlyName) ||
        PathLength == 0 || PathLength >= BP_ARRAY_COUNT(NewEntry.EspPath) ||
        EspPath[0] != L'\\')
    {
        return EFI_INVALID_PARAMETER;
    }

    // Read the current database
    Result = BpCmReadLoadEntries(&OldEntries, &OldCount);

    // No database yet, the first entry gets the lowest possible ID
    if (Result == EFI_NOT_FOUND)
    {
        OldEntries  = NULL;
        OldCount    = 0;
        Result      = EFI_SUCCESS;
    }

    if (EFI_ERROR(Result))
        goto Done;

    // The new ID is one above the highest existing ID, so it is unique and never 0
    for (Index = 0; Index < OldCount; Index++)
    {
        if (OldEntries[Index].Id < NewId)
            continue;

        if (OldEntries[Index].Id == 0xFFFFFFFF)
        {
            Result = EFI_OUT_OF_RESOURCES;
            goto Done;
        }

        NewId = OldEntries[Index].Id + 1;
    }

    // Build the new entry
    BS->SetMem(&NewEntry, sizeof(BP_LOAD_ENTRY), 0);

    NewEntry.Id                 = NewId;
    NewEntry.SecureBootRequired = SecureBootRequired ? TRUE : FALSE;

    BS->CopyMem(NewEntry.FriendlyName, FriendlyName, NameLength * sizeof(CHAR16));
    BS->CopyMem(NewEntry.EspPath, EspPath, PathLength * sizeof(CHAR16));

    // Allocate the new database
    Result = BpMmAllocatePool((OldCount + 1) * sizeof(BP_LOAD_ENTRY), (VOID **)&NewEntries);

    if (EFI_ERROR(Result))
        goto Done;

    if (OldCount != 0)
        BS->CopyMem(NewEntries, OldEntries, OldCount * sizeof(BP_LOAD_ENTRY));

    BS->CopyMem(&NewEntries[OldCount], &NewEntry, sizeof(BP_LOAD_ENTRY));

    // Replace the database in one atomic variable write
    Result = BpCmpWriteLoadEntries(NewEntries, OldCount + 1);

    if (EFI_ERROR(Result))
        goto Done;

    *EntryId = NewId;

Done:

    if (NewEntries != NULL)
        BpMmFreePool((VOID **)&NewEntries);

    if (OldEntries != NULL)
        BpMmFreePool((VOID **)&OldEntries);

    return Result;
}

EFI_STATUS BpCmRemoveLoadEntry(
    _In_ UINT32 EntryId
)
/*++
    Routine Description:
        Removes a boot entry from the database by its ID.

    Arguments:

        EntryId:
            The unique identifier of the entry to remove.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS              Result          = EFI_SUCCESS;
    EFI_BOOT_SERVICES       *BS             = g_AppContext.BootServices;
    PBP_LOAD_ENTRY          Entries         = NULL;
    UINT64                  Count           = 0;
    UINT64                  Index           = 0;
    BP_APP_CONFIGURATION    Configuration   = { 0 };

    if (EntryId == 0)
        return EFI_INVALID_PARAMETER;

    Result = BpCmReadLoadEntries(&Entries, &Count);

    if (EFI_ERROR(Result))
        goto Done;

    // Find the entry
    for (Index = 0; Index < Count; Index++)
    {
        if (Entries[Index].Id == EntryId)
            break;
    }

    if (Index == Count)
    {
        Result = EFI_NOT_FOUND;
        goto Done;
    }

    // Unset the primary entry first
    Result = BpCmGetAppConfiguration(&Configuration);

    if (EFI_ERROR(Result))
        goto Done;

    if (Configuration.PrimaryId == EntryId)
    {
        Configuration.PrimaryId = 0;
        Configuration.AutoLoad  = FALSE;

        Result = BpCmpWriteAppConfiguration(&Configuration);

        if (EFI_ERROR(Result))
            goto Done;
    }

    // Close the gap left by the removed entry
    if (Index + 1 < Count)
    {
        BS->CopyMem(
            &Entries[Index],
            &Entries[Index + 1],
            (Count - Index - 1) * sizeof(BP_LOAD_ENTRY)
        );
    }

    Result = BpCmpWriteLoadEntries(Entries, Count - 1);

Done:

    if (Entries != NULL)
        BpMmFreePool((VOID **)&Entries);

    return Result;
}

EFI_STATUS BpCmSetPrimaryEntry(
    _In_ UINT32 EntryId
)
/*++
    Routine Description:
        Sets the primary boot entry by ID.

    Arguments:

        EntryId:
            The unique identifier of the entry to set as primary.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS              Result          = EFI_SUCCESS;
    BP_APP_CONFIGURATION    Configuration   = { 0 };

    if (EntryId == 0)
        return EFI_INVALID_PARAMETER;

    // The entry has to exist
    Result = BpCmpCheckLoadEntryExists(EntryId);

    if (EFI_ERROR(Result))
        goto Done;

    Result = BpCmGetAppConfiguration(&Configuration);

    if (EFI_ERROR(Result))
        goto Done;

    Configuration.PrimaryId = EntryId;
    Result = BpCmpWriteAppConfiguration(&Configuration);

Done:

    return Result;
}

EFI_STATUS BpCmSetAutoLoad(
    _In_ BOOLEAN AutoLoad
)
/*++
    Routine Description:
        Enables or disables automatic loading of the primary boot entry.

    Arguments:

        AutoLoad:
            TRUE to enable auto-load, FALSE to disable.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS              Result  = EFI_SUCCESS;
    BP_APP_CONFIGURATION    Configuration;

    Result = BpCmGetAppConfiguration(&Configuration);

    if (EFI_ERROR(Result))
        goto Done;

    // Auto load is only meaningful if there is a primary entry to load
    if (AutoLoad && Configuration.PrimaryId == 0)
    {
        Result = EFI_NOT_READY;
        goto Done;
    }

    Configuration.AutoLoad = AutoLoad ? TRUE : FALSE;
    Result = BpCmpWriteAppConfiguration(&Configuration);

Done:

    return Result;
}