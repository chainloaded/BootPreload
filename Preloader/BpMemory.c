#include "BpMemory.h"

EFI_STATUS BpMmAllocatePool(
    _In_    UINT64      AllocationSize,
    _Out_   VOID        **Allocation
)
/*++
    Routine Description:
        Allocates and zeros a pool of memory of the specified size.

    Arguments:

        AllocationSize:
            Number of bytes to allocate.

        Allocation:
            Pointer to receive the allocated memory address.

    Return Value:
        EFI_STATUS indicating success or failure.
--*/
{
    EFI_STATUS          Result      = EFI_SUCCESS;
    EFI_BOOT_SERVICES   *BS         = g_AppContext.BootServices;

    Result = BS->AllocatePool(
        EfiBootServicesData, 
        AllocationSize,
        Allocation
    );

    // Zero out the allocated memory if allocation was successful
    if (!EFI_ERROR(Result))
        BS->SetMem(*Allocation, AllocationSize, 0);

    return Result;
}

void BpMmFreePool(
    _Inout_ VOID **Allocation
)
/*++
    Routine Description:
        Frees an allocated memory pool and nullifies the pointer.

    Arguments:

        Allocation:
            Pointer to the memory address to free.

    Return Value:
        None.
--*/
{
    EFI_BOOT_SERVICES *BS = g_AppContext.BootServices;

    if (Allocation != NULL)
    {
        if (*Allocation != NULL)
        {
            BS->FreePool(*Allocation);
            *Allocation = NULL;
        }
    }
}