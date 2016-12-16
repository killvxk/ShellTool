

#include "SpiCommon.h"


EFI_STATUS
EFIAPI
InitializeSpiSmm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT32      LpcMmioBar;
  UINT32      SpiBar;
  
  LpcMmioBar = (MmioRead32(LPC_PCI_REG(LPC_MMIO_BAR_REG))>>4)<<12;
  SpiBar     = MmioRead32(LpcMmioBar) & ~0xFF;
  gSpiInstance.SpiBar = SpiBar;
  
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gSpiInstance.Handle,
                  &gEfiSmmSpiProtocolGuid,
                  &gSpiInstance.SpiProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  return Status;
}



