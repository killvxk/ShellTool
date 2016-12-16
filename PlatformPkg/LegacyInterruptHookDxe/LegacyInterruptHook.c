
#include <PiDxe.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/LegacyInterruptHandler.h>

VOID                      *mRegisteration;
EFI_LEGACY_BIOS_PROTOCOL  *mLegacyBios;

VOID
EFIAPI
NewLegacyInterruptHandler (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                            Status;
  UINTN                                 BufferSize;
  EFI_HANDLE                            Handle;
  EFI_LEGACY_INTERRUPT_HANDLER_PROTOCOL *LegacyInterrupt;
  INTERRUPT_HANDLER                     *NewHandler;
  LEGACY_VECTOR                         *Vector;
  

  while (TRUE) {
    BufferSize = sizeof (Handle);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    NULL,
                    mRegisteration,
                    &BufferSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      //
      // Exit Path of While Loop....
      //
      break;
    }

    Status = gBS->HandleProtocol (Handle, &gEfiLegacyInterruptHandlerProtocolGuid, (VOID **) &LegacyInterrupt);
    ASSERT_EFI_ERROR (Status);
    ASSERT (LegacyInterrupt->Number < 0x20);
    ASSERT (LegacyInterrupt->Handler != NULL);
    ASSERT (LegacyInterrupt->Length < 0x20000);

    Vector = (LEGACY_VECTOR *) (UINTN) (LegacyInterrupt->Number * sizeof (LEGACY_VECTOR));
    //
    // Save the original interrupt handler
    //
    LegacyInterrupt->Handler->NextSegment = Vector->Segment;
    LegacyInterrupt->Handler->NextOffset  = Vector->Offset;
    
    DEBUG ((EFI_D_ERROR, "GetLegacyRegion...\n"));
    Status = mLegacyBios->GetLegacyRegion (
                            mLegacyBios,
                            LegacyInterrupt->Length,
                            0,  // E or F segment
                            16,// 16-byte aligned
                            (VOID **) &NewHandler
                            );
    ASSERT_EFI_ERROR (Status);

    //
    // The Address should be 16-byte aligned
    //
    ASSERT (((UINTN) NewHandler & 0xF) == 0);
    
    DEBUG ((EFI_D_ERROR, "CopyLegacyRegion...\n"));
    Status = mLegacyBios->CopyLegacyRegion (
                            mLegacyBios,
                            LegacyInterrupt->Length,
                            NewHandler,
                            LegacyInterrupt->Handler
                            );
    ASSERT_EFI_ERROR (Status);


    //
    // Set the current interrupt handler
    //
    Vector->Segment = (UINT16) ((UINTN) &NewHandler->Code >> 4);
    Vector->Offset  = (UINT16) ((UINTN) &NewHandler->Code & 0xF);
    DEBUG ((EFI_D_ERROR, "Install Interrupt[%02x] - %04x:%04x l=%04x\n", 
           (UINTN) LegacyInterrupt->Number, (UINTN) Vector->Segment, (UINTN) Vector->Offset, LegacyInterrupt->Length
           ));
  }
}

/**
  The driver entry point for LegacyInterruptHook Driver.

  The function does the necessary initialization work.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI system table.

  @retval     EFI_SUCCESS       This funtion always return EFI_SUCCESS.
                                It will ASSERT on errors.

**/
EFI_STATUS
EFIAPI
InitializeLegacyInterruptHook (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS            Status;

  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &mLegacyBios
                  );
  ASSERT_EFI_ERROR (Status);
  
  EfiCreateProtocolNotifyEvent(
    &gEfiLegacyInterruptHandlerProtocolGuid,
    TPL_CALLBACK,
    NewLegacyInterruptHandler,
    NULL,
    &mRegisteration
    );
  
  return EFI_SUCCESS;
}
