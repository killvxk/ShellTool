

#include "SpiCommon.h"
#include <Guid/EventGroup.h>


EFI_STATUS
EFIAPI
InitializeSpiDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT32      LpcMmioBar;
  UINT32      SpiBar;
  EFI_EVENT   VirtualAddressChangeEvent;  

  LpcMmioBar = (MmioRead32(LPC_PCI_REG(LPC_MMIO_BAR_REG))>>4)<<12;
  SpiBar     = MmioRead32(LpcMmioBar) & ~0xFF;
  gSpiInstance.SpiBar = SpiBar;
  
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gSpiInstance.Handle,
                  &gEfiSpiProtocolGuid,
                  &gSpiInstance.SpiProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SpiVirtualddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &VirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);  
  
  return Status;
}



