#include "BpLoader.h"
#include "BpConfiguration.h"
#include "BpString.h"
#include "BpMemory.h"

EFI_STATUS BpLdrpBuildFileDevicePath(
    _In_  EFI_HANDLE         DeviceHandle,
    _In_  CHAR16             *FilePath,
    _Out_ EFI_DEVICE_PATH    **OutPath
);

EFI_STATUS BpLdrpTransferExecution(
    _In_ PBP_LOAD_ENTRY Entry
);

EFI_STATUS BpLdrpSecureBootIsEnabled(
    _Out_ BOOLEAN *Enabled
);

EFI_STATUS BpLdrpBuildFileDevicePath(
    _In_  EFI_HANDLE         DeviceHandle,
    _In_  CHAR16            *FilePath,
    _Out_ EFI_DEVICE_PATH   **OutPath
);

EFI_STATUS BpLdrLoadEntryById(
    _In_  UINT64 EntryId
)
/*++
    Routine Description:
        Loads and executes a boot entry by its ID, checking secure boot requirements.

    Arguments:

        EntryId:
            The unique identifier of the boot entry to load.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS      Result;
    PBP_LOAD_ENTRY  LoadEntries         = NULL;
    BOOLEAN         SecureBootEnabled   = FALSE;
    UINT64          EntryCount          = 0;

    // Load the boot entries database
    Result = BpCmReadLoadEntries(&LoadEntries, &EntryCount);

    if (EFI_ERROR(Result))
        goto Done;

    // Find the entry with the specified ID
    for (UINT64 i = 0; i < EntryCount; i++)
    {
        if (LoadEntries[i].Id == EntryId)
        {
            // If secure boot is required, check if it's enabled
            if (LoadEntries[i].SecureBootRequired)
            {
                Result = BpLdrpSecureBootIsEnabled(&SecureBootEnabled);

                if (EFI_ERROR(Result))
                    goto Done;

                if (!SecureBootEnabled)
                {
                    Result = EFI_ACCESS_DENIED;
                    goto Done;
                }
            }

            // Load the entry
            Result = BpLdrpTransferExecution(&LoadEntries[i]);
            goto Done;
        }
    }

Done:

    if (LoadEntries)
        BpMmFreePool(&LoadEntries);

    return Result;
}

EFI_STATUS BpLdrpBuildFileDevicePath(
    _In_  EFI_HANDLE         DeviceHandle,
    _In_  CHAR16             *FilePath,
    _Out_ EFI_DEVICE_PATH    **OutPath
)
/*++
    Routine Description:
        Constructs a complete device path for a file on the EFI system partition.

    Arguments:

        DeviceHandle:
            The EFI device handle for the system partition.

        FilePath:
            The file path relative to the system partition root.

        OutPath:
            Pointer to receive the constructed device path.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS                          Result;
    EFI_BOOT_SERVICES                   *BootServices       = g_AppContext.BootServices;
    EFI_DEVICE_PATH                     *DevicePath         = NULL;
    EFI_DEVICE_PATH                     *EndPath            = NULL;
    EFI_DEVICE_PATH                     *AppendedPath       = NULL;
    FILEPATH_DEVICE_PATH                *FileDevicePath     = NULL;
    EFI_DEVICE_PATH_UTILITIES_PROTOCOL  *UtilsProtocol      = NULL;
    UINT32                              FilePathLen         = 0;
    UINT32                              AllocSize           = 0;

    // Get file path length in bytes (including null terminator)
    FilePathLen = (UINT32)((BpRtlStringLength(FilePath) + 1) * sizeof(CHAR16));

    // Calculate the total allocation size for the file device path node
    AllocSize = FilePathLen + SIZE_OF_FILEPATH_DEVICE_PATH + sizeof(EFI_DEVICE_PATH);

    // Get the device path from the device handle
    Result = BootServices->HandleProtocol(
        DeviceHandle,
        &g_EfiDevicePathProtocolGuid,
        (VOID **)&DevicePath
    );

    if (EFI_ERROR(Result))
        goto Done;

    // Locate the device path utilities protocol
    Result = BootServices->LocateProtocol(
        &g_EfiDevicePathUtilitiesProtocolGuid,
        NULL,
        (VOID **)&UtilsProtocol
    );

    if (EFI_ERROR(Result))
        goto Done;

    // Allocate memory for the file device path node
    Result = BpMmAllocatePool(
        AllocSize,
        (VOID **)&FileDevicePath
    );

    if (EFI_ERROR(Result))
        goto Done;

    // Initialize the file device path node
    FileDevicePath->Header.Type = MEDIA_DEVICE_PATH;
    FileDevicePath->Header.SubType = MEDIA_FILEPATH_DP;

    SetDevicePathNodeLength(
        &FileDevicePath->Header,
        FilePathLen + SIZE_OF_FILEPATH_DEVICE_PATH
    );

    // Copy the file path into the device path node
    BootServices->CopyMem(
        FileDevicePath->PathName,
        FilePath,
        FilePathLen
    );

    // Set the end device path node
    EndPath = NextDevicePathNode(&FileDevicePath->Header);
    SetDevicePathEndNode(EndPath);

    // Append the file device path to the device path
    AppendedPath = UtilsProtocol->AppendDevicePath(
        DevicePath, 
        (EFI_DEVICE_PATH *)FileDevicePath
    );

    if (AppendedPath)
    {
        Result      = EFI_SUCCESS;
        *OutPath    = AppendedPath;
    }
    else
    {
        Result = EFI_OUT_OF_RESOURCES;
    }

Done:

    if (FileDevicePath)
        BpMmFreePool(&FileDevicePath);

    return Result;
}

EFI_STATUS BpLdrpTransferExecution(
    _In_ PBP_LOAD_ENTRY Entry
)
/*++
    Routine Description:
        Loads an image from the ESP and starts its execution.

    Arguments:

        Entry:
            Pointer to the load entry structure describing the image to load.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS                  Result;
    EFI_HANDLE                  ImageHandle     = NULL;
    EFI_BOOT_SERVICES           *BT             = g_AppContext.BootServices;
    EFI_DEVICE_PATH             *DevicePath     = NULL;
    EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage    = NULL;
    // Sanitize filepath
    Entry->EspPath[255] = L'\0';

    // Get the device handle for the current image
    Result = BT->HandleProtocol(
        g_AppContext.ImageHandle,
        &g_EfiLoadedImageProtocolGuid,
        (VOID **)&LoadedImage
    );

    if (EFI_ERROR(Result))
        goto Done;

    // Build file device path
    Result = BpLdrpBuildFileDevicePath(
        LoadedImage->DeviceHandle,
        Entry->EspPath,
        &DevicePath
    );

    if (EFI_ERROR(Result))
        goto Done;

    // Load image from ESP
    Result = BT->LoadImage(
        FALSE,
        g_AppContext.ImageHandle,
        DevicePath,
        NULL,
        0,
        &ImageHandle
    );

    if (EFI_ERROR(Result))
        goto Done;

    // Start image
    Result = BT->StartImage(
        ImageHandle,
        NULL,
        NULL
    );

    // Unload image if it returns (should not happen)
    BT->UnloadImage(ImageHandle);

Done:

    if (DevicePath)
        BpMmFreePool(&DevicePath);

    return Result;
}

EFI_STATUS BpLdrpSecureBootIsEnabled(
    _Out_ BOOLEAN *Enabled
)
{
    EFI_STATUS              Result      = EFI_SUCCESS;
    EFI_RUNTIME_SERVICES    *RT         = g_AppContext.RuntimeServices;
    EFI_GUID                GlobalGuid  = EFI_GLOBAL_VARIABLE;
    UINT64                  DataSize    = sizeof(UINT8);
    UINT32                  Attributes  = 0;
    UINT8                   Value       = 0;

    if (Enabled == NULL)
        return EFI_INVALID_PARAMETER;

    // Assume disabled until proven otherwise
    *Enabled = FALSE;

    // Try to read the SecureBoot global variable
    Result = RT->GetVariable(
        L"SecureBoot",
        &GlobalGuid,
        &Attributes,
        &DataSize,
        &Value
    );

    // The variable does not exist on platforms without Secure Boot support
    if (Result == EFI_NOT_FOUND)
    {
        Result = EFI_SUCCESS;
        goto Done;
    }

    if (EFI_ERROR(Result))
        goto Done;

    // Check if the variable has the expected size and value
    if (DataSize != sizeof(UINT8) || Value > 1)
    {
        Result = EFI_SECURITY_VIOLATION;
        goto Done;
    }

    *Enabled = (Value == 1);

Done:

    return Result;
}