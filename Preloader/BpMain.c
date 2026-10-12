#include "BpMain.h"
#include "BpLoader.h"
#include "BpSetupMode.h"
#include "BpConfiguration.h"

// Define the global application context and variables
BP_APP_CONTEXT  g_AppContext                            = { 0 };
EFI_GUID        g_EfiDevicePathProtocolGuid             = EFI_DEVICE_PATH_PROTOCOL_GUID;
EFI_GUID        g_EfiDevicePathUtilitiesProtocolGuid    = EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID;
EFI_GUID        g_EfiLoadedImageProtocolGuid            = EFI_LOADED_IMAGE_PROTOCOL_GUID;

void BppInitializeContext(
    _In_    EFI_HANDLE          ImageHandle,
    _In_    EFI_SYSTEM_TABLE    *SystemTable,
    _Out_   PBP_APP_CONTEXT     Context
)
/*++
    Routine Description:
        Initializes the application context with EFI handles and system tables.

    Arguments:

        ImageHandle:
            EFI handle describing the current image.

        SystemTable:
            Pointer to the EFI system table.

        Context:
            Pointer to the application context structure to initialize.

    Return Value:
        None.
--*/
{
    EFI_GUID VendorGuid = BP_APP_VENDOR_GUID;

    Context->ImageHandle        = ImageHandle;
    Context->SystemTable        = SystemTable;
    Context->BootServices       = SystemTable->BootServices;
    Context->RuntimeServices    = SystemTable->RuntimeServices;
    Context->AppVendorGuid      = VendorGuid;
}

EFI_STATUS BpEntry(
    _In_ EFI_HANDLE         ImageHandle,
    _In_ EFI_SYSTEM_TABLE   *SystemTable
)
/*++
    Routine Description:
        Entry point for the boot preloader application.

    Arguments:

        ImageHandle:
            EFI handle describing the current image.

        SystemTable:
            Pointer to the EFI system table.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS              Result;
    BP_APP_CONFIGURATION    AppConfiguration    = { 0 };
    UINT32                  SetupModeReason     = SETUP_MODE_REASON_UNKNOWN;
    BOOLEAN                 EscPressed          = FALSE;

    // Initialize app context
    BppInitializeContext(ImageHandle, SystemTable, &g_AppContext);

    // Try to read the global configuration
    Result = BpCmReadAppConfiguration(&AppConfiguration);

    if (!EFI_ERROR(Result) && AppConfiguration.AutoLoad && AppConfiguration.PrimaryId != 0)
    {
        // Wait for boot delay
        Result = BpSmPrimaryWait(&EscPressed);

        if (EFI_ERROR(Result))
            goto Done;

        if (EscPressed == TRUE)
        {
            SetupModeReason = SETUP_MODE_REASON_USER_REQUESTED;
            goto Done;
        }

        // Load the primary application
        Result = BpLdrLoadEntryById(AppConfiguration.PrimaryId);

        if (Result == EFI_ACCESS_DENIED)
        {
            SetupModeReason = SETUP_MODE_REASON_SECURE_BOOT_REQUIRED;
        }

        if (EFI_ERROR(Result))
        {
            SetupModeReason = SETUP_MODE_REASON_PRIMARY_LOAD_FAILED;
        }
    }
    else
    {
        SetupModeReason = SETUP_MODE_REASON_NO_CONFIGURATION;
    }

Done:

    // Start setup mode
    BpSmEnterSetupMode(SetupModeReason);

    return Result;
}