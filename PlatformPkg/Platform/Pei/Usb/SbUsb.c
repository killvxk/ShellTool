/*++

Module Name:
  
  SbUsb.c

Abstract:

  Ehci PPI Init

--*/
#include "UsbController.h"
#include "PlatformDefinition.h"

#define UHCI_IO_BASE                       0xF000
#define UHCI_IO_LENGTH                     0x20
#define EHCI_MEMORY_BASE                   0xFF000000
#define XHCI_MMIO_SPACE_START_ADDRESS      0xFF100000

STATIC
EFI_STATUS
EnableEhciController (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN USB_CONTROLLER_INFO      *PeiUsbControllerInfo,
  IN UINT8                    UsbControllerId
  )
{
  UINTN  BaseAddress;

  if (UsbControllerId >= PeiUsbControllerInfo->EhciControllersNum) {
    return EFI_INVALID_PARAMETER;
  }

  BaseAddress = PeiUsbControllerInfo->MemBase[UsbControllerId];
  //
  // Assign base address register
  //
  MmioWrite32 (EHCI_PCI_REG (SB_EHCI_MEM_BASE), BaseAddress);
  //
  // Enable EHCI register
  //
  MmioOr16 (
    EHCI_PCI_REG (SB_EHCI_COMMAND_REGISTER),
    (UINT16) (SB_EHCI_COMMAND_BME | SB_EHCI_COMMAND_MSE | BIT4 | BIT0)
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EnableXhciController (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN USB_CONTROLLER_INFO      *PeiUsbControllerInfo,
  IN UINT8                    UsbControllerId
  )
{
  UINTN BaseAddress;
  
  if ((UsbControllerId < PeiUsbControllerInfo->EhciControllersNum)||(UsbControllerId >= PeiUsbControllerInfo->EhciControllersNum +PeiUsbControllerInfo->XhciControllersNum)) {
    return EFI_INVALID_PARAMETER;
  }

  BaseAddress = PeiUsbControllerInfo->MemBase[UsbControllerId];
  //
  // Assign base address register
  //
  MmioWrite32 (XHCI_PCI_REG (SB_XHCI_MEM_BASE), BaseAddress);

  //
  // Enable XHCI register
  //
  MmioOr16 (
    XHCI_PCI_REG (SB_XHCI_COMMAND_REGISTER),
    (UINT16) (SB_XHCI_COMMAND_BME | SB_XHCI_COMMAND_MSE | BIT4 | BIT0)
    );
  //
  BaseAddress = MmioRead32(XHCI_PCI_REG (SB_XHCI_MEM_BASE)) & 0xFFFFFFF0;;
  if(BaseAddress != PeiUsbControllerInfo->MemBase[UsbControllerId]) {
    PeiUsbControllerInfo->MemBase[UsbControllerId] = 0;
    DEBUG ((EFI_D_ERROR, "specified Xhci devie is not present !\n"));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InitUsbControl (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  USB_CONTROLLER_INFO           *PeiUsbControllerInfo
  )
{
  EFI_STATUS            Status;
  UINTN                 XhciIdStart;
  UINTN                 UhciIdStart;
  UINTN                 Index;

  DEBUG ((EFI_D_INFO, "InitUsbControl() Start\n"));
  Status = EFI_SUCCESS;
  XhciIdStart = PeiUsbControllerInfo->EhciControllersNum;
  UhciIdStart = XhciIdStart + PeiUsbControllerInfo->XhciControllersNum;
  //
  // Assign resources and enable EHCI controllers
  //
  if (PeiUsbControllerInfo->EhciControllersNum) {
    PeiUsbControllerInfo->MemBase[0] = EHCI_MEMORY_BASE;
    Status = EnableEhciController (PeiServices, PeiUsbControllerInfo, (UINT8) 0);
    ASSERT_EFI_ERROR (Status);
  }
  if (PeiUsbControllerInfo->XhciControllersNum) {
    PeiUsbControllerInfo->MemBase[XhciIdStart] = XHCI_MMIO_SPACE_START_ADDRESS;
    Status = EnableXhciController (PeiServices, PeiUsbControllerInfo, (UINT8) XhciIdStart);
    ASSERT_EFI_ERROR (Status);
  }
  //
  // enable UHCI controllers
  //
  if (PeiUsbControllerInfo->UhciControllersNum) {
    for (Index = 0; Index < 5; Index ++) {
      PeiUsbControllerInfo->MemBase[UhciIdStart + Index] = UHCI_IO_BASE + UHCI_IO_LENGTH * Index;
      MmioWrite16 (UHCI_PCI_REG (Index, 0x0C), 0x4010);
      MmioOr8 (UHCI_PCI_REG (Index, 0x4B), BIT6 | BIT5 | BIT0); // must use BIT OR
      MmioOr8 (UHCI_PCI_REG (Index, 0xC0), BIT5);
      MmioWrite32 (UHCI_PCI_REG (Index, 0x20), UHCI_IO_BASE + UHCI_IO_LENGTH * Index);
      MmioOr16 (UHCI_PCI_REG (Index, 4), 5);
      MicroSecondDelay(20000);
    }
  }
  DEBUG ((EFI_D_INFO, "InitUsbControl() End\n"));

  return Status;

}


