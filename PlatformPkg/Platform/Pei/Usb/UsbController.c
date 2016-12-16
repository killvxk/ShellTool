/*++

Module Name:
  
  MultiUsbController.c

Abstract:

  Usb Controller PPI Init

--*/
#include "UsbController.h"
#include <PlatformDefinition.h>

//
// PPI interface function
//
STATIC
EFI_STATUS
EFIAPI
GetUsbController (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN  PEI_USB_CONTROLLER_PPI        *This,
  IN UINT8                          UsbControllerId,
  OUT UINTN                         *ControllerType,
  OUT UINTN                         *MemBaseAddress
  );

//
// Globals
//
STATIC PEI_USB_CONTROLLER_PPI  mUsbControllerPpi = { GetUsbController };

STATIC EFI_PEI_PPI_DESCRIPTOR   mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiUsbControllerPpiGuid,
  NULL
};

//
// PPI interface implementation
//
STATIC
EFI_STATUS
EFIAPI
GetUsbController (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  PEI_USB_CONTROLLER_PPI          *This,
  IN  UINT8                           UsbControllerId,
  OUT UINTN                           *ControllerType,
  OUT UINTN                           *MemBaseAddress
  )
{
  USB_CONTROLLER_INFO *PeiUsbControllerInfo;
  UINTN XhciIdStart,UhciIdStart,OhciIdStart;

  PeiUsbControllerInfo = USB_CONTROLLER_INFO_FROM_THIS(This);

  if (UsbControllerId >= PeiUsbControllerInfo->TotalUsbControllers) {
    return EFI_INVALID_PARAMETER;
  }

  XhciIdStart = PeiUsbControllerInfo->EhciControllersNum;
  UhciIdStart = XhciIdStart + PeiUsbControllerInfo->XhciControllersNum;
  OhciIdStart = UhciIdStart + PeiUsbControllerInfo->UhciControllersNum;
  
  PeiUsbControllerInfo = USB_CONTROLLER_INFO_FROM_THIS (This);

  if (UsbControllerId < XhciIdStart){
    *ControllerType = PEI_EHCI_CONTROLLER;
  } else if (UsbControllerId < UhciIdStart){
    *ControllerType = PEI_XHCI_CONTROLLER;
  } else if (UsbControllerId < OhciIdStart){
    *ControllerType = PEI_UHCI_CONTROLLER;
  } else {
    *ControllerType = PEI_OHCI_CONTROLLER;
  }

  *MemBaseAddress = PeiUsbControllerInfo->MemBase[UsbControllerId];

  return EFI_SUCCESS;
}

EFI_STATUS
InitUsbController (
  IN       EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_STATUS                Status;
  USB_CONTROLLER_INFO       *PeiUsbControllerInfo;
  UINTN                     UhciNum = SB_UHCI_COUNT;
  UINTN                     EhciNum = SB_EHCI_COUNT;
  UINTN                     XhciNum = SB_XHCI_COUNT;
	

  DEBUG ((EFI_D_INFO, "InitUsbController() Start\n"));

  PeiUsbControllerInfo = (USB_CONTROLLER_INFO *) AllocateZeroPool (sizeof (USB_CONTROLLER_INFO));
  if (PeiUsbControllerInfo == NULL) {
    DEBUG ((EFI_D_ERROR, "Failed to allocate memory for PeiEhciDev! \n"));
    return EFI_OUT_OF_RESOURCES;
  }

  if (XhciNum) {
    HandleXhciFwForRecovery ();
  }
  //
  // Allocate the UsbController ID in the sequence as 
  // EHC(0~(EhciNum-1))/XHC(EhciNum~(EhciNum+XhciNum-1))/UHC((EhciNum+XhciNum)~(EhciNum+XhciNum+UhciNum-1))/
  // OHC((EhciNum+XhciNum+UhciNum)~(EhciNum+XhciNum+UhciNum+OhciNum-1)) 
  // 
  PeiUsbControllerInfo->Signature           = PEI_USB_CONTROLLER_SIGNATURE;
  PeiUsbControllerInfo->UsbControllerPpi    = mUsbControllerPpi;
  PeiUsbControllerInfo->PpiList             = mPpiList;
  PeiUsbControllerInfo->PpiList.Ppi         = &PeiUsbControllerInfo->UsbControllerPpi;
  PeiUsbControllerInfo->EhciControllersNum  = EhciNum;
  PeiUsbControllerInfo->XhciControllersNum  = XhciNum;
  PeiUsbControllerInfo->UhciControllersNum  = UhciNum;
  PeiUsbControllerInfo->TotalUsbControllers = UhciNum + EhciNum + XhciNum;
  DEBUG ((EFI_D_INFO, "USB count:%x\n", PeiUsbControllerInfo->TotalUsbControllers));
	//
  // Init the Usb Controller 
  //
  InitUsbControl (PeiServices, PeiUsbControllerInfo);  
  //
  // Install USB Controller PPI
  //
  Status = PeiServicesInstallPpi (&PeiUsbControllerInfo->PpiList);

  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_INFO, "InitUsbController() End\n"));

  return Status;

}


