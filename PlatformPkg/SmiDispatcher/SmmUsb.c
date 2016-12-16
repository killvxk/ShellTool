
#include "SmmHelpers.h"

SB_SMM_SOURCE_DESC mUSB_WAKE = {
  SB_SMM_SCI_EN_DEPENDENT,
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_GP_SMI_EN
      },
      2,
      14
    },
    NULL_BIT_DESC_INITIALIZER
  },
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_GP_STS
      },
      2,
      14
    }
  }
};

SB_SMM_SOURCE_DESC mUSB_LEGACY = {
  SB_SMM_NO_FLAGS,
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_GBLEN_REG
      },
      2,
      4
    },
    NULL_BIT_DESC_INITIALIZER
  },
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_GBLSTS_REG
      },
      2,
      4
    }
  }
};

typedef enum {
  SbUsbControllerUhci,	
  SbUsbControllerEhci,
  SbUsbControllerTypeMax
} SB_USB_CONTROLLER_TYPE;

typedef struct {
  UINT8                   Function;
  UINT8                   Device;
  SB_USB_CONTROLLER_TYPE  UsbConType;
} USB_CONTROLLER;

//// TGR-2016062201-S (SOC not include USB Controller)
/*
USB_CONTROLLER  mUsbControllersMap[] = {
  {
    UHCI_FUNC_NO,
    UHCI_DEV_NO,
    SbUsbControllerUhci
  },	
  {
    UHCI_FUNC1_NO,
    UHCI_DEV_NO,
    SbUsbControllerUhci
  },	
  {
    UHCI_FUNC2_NO,
    UHCI_DEV_NO,
    SbUsbControllerUhci
  },	
  {
    EHCI_FUNC_NO,
    EHCI_DEV_NO,
    SbUsbControllerEhci
  },
  {
    XHCI_FUNC_NO,
    XHCI_DEV_NO,
    SbUsbControllerEhci
  }  
};
*/
///// TGR-2016062201-E

EFI_STATUS
DevicePathToSupportedController (
  IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  OUT SB_USB_CONTROLLER_TYPE     *Controller
  )
/*++

Routine Description:

  Find the handle that best matches the input Device Path and return the USB controller type

Arguments:

  DevicePath              Pointer to the device Path table

Returns:

  EFI_UNSUPPORTED         Invalid device Path table or can't find any match USB device path
  PCH_USB_CONTROLLER_TYPE The USB controller type of the input device path

--*/
{
  EFI_STATUS                Status;
  EFI_HANDLE                DeviceHandle;
  ACPI_HID_DEVICE_PATH      *AcpiNode;
  PCI_DEVICE_PATH           *PciNode;
  EFI_DEVICE_PATH_PROTOCOL  *RemaingDevicePath;
  //UINT8                     UsbIndex;
  //
  // Find the handle that best matches the Device Path. If it is only a
  // partial match the remaining part of the device path is returned in
  // RemainingDevicePath.
  //
  RemaingDevicePath = DevicePath;
  Status = gBS->LocateDevicePath (
                  &gEfiPciRootBridgeIoProtocolGuid,
                  &DevicePath,
                  &DeviceHandle
                  );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_INFO, "(L%d)%r\n", __LINE__, Status));		
    return EFI_UNSUPPORTED;
  }

  DevicePath = RemaingDevicePath;

  //
  // Get first node: Acpi Node
  //
  AcpiNode = (ACPI_HID_DEVICE_PATH *) RemaingDevicePath;

  if (AcpiNode->Header.Type != ACPI_DEVICE_PATH ||
      AcpiNode->Header.SubType != ACPI_DP ||
      DevicePathNodeLength (&AcpiNode->Header) != sizeof (ACPI_HID_DEVICE_PATH) ||
      AcpiNode->HID != EISA_PNP_ID (0x0A03) ||
      AcpiNode->UID != 0
      ) {
    DEBUG((EFI_D_INFO, "(L%d)%r\n", __LINE__, EFI_UNSUPPORTED));  
    return EFI_UNSUPPORTED;
  } else {
    //
    // Get the next node: Pci Node
    //
    RemaingDevicePath = NextDevicePathNode (RemaingDevicePath);
    PciNode           = (PCI_DEVICE_PATH *) RemaingDevicePath;
    if (PciNode->Header.Type != HARDWARE_DEVICE_PATH ||
        PciNode->Header.SubType != HW_PCI_DP ||
        DevicePathNodeLength (&PciNode->Header) != sizeof (PCI_DEVICE_PATH)
        ) {
      DEBUG((EFI_D_INFO, "(L%d)%r\n", __LINE__, EFI_UNSUPPORTED));
      return EFI_UNSUPPORTED;
    }
    //// TGR-2016062201-S	
    /*
    for (UsbIndex = 0; UsbIndex < sizeof (mUsbControllersMap) / sizeof (USB_CONTROLLER); UsbIndex++) {
      if ((PciNode->Device == mUsbControllersMap[UsbIndex].Device) &&
          (PciNode->Function == mUsbControllersMap[UsbIndex].Function)) {
        *Controller = mUsbControllersMap[UsbIndex].UsbConType;
        return EFI_SUCCESS;
      }
    }
    */
    //// TGR-2016062201-E	
    DEBUG((EFI_D_INFO, "(L%d)%r\n", __LINE__, EFI_UNSUPPORTED));
    return EFI_UNSUPPORTED;
  }
}

VOID
MapUsbToSrcDesc (
  IN  SB_SMM_CONTEXT         *Context,
  OUT SB_SMM_SOURCE_DESC     *SrcDesc
  )
/*++

Routine Description:

  Maps a USB context to a source description.

Arguments:

  Context                 The context we need to map.  Type must be USB.
  SrcDesc                 The source description that corresponds to the given context.

Returns:

  None.
  
--*/
{
  SB_USB_CONTROLLER_TYPE  Controller;
  EFI_STATUS              Status;

  Status = DevicePathToSupportedController (Context->Usb.Device, &Controller);
  //
  // Either the device path passed in by the child is incorrect or
  // the ones stored here internally are incorrect.
  //
  ASSERT_EFI_ERROR (Status);

  switch (Context->Usb.Type) {
  case UsbLegacy:
    switch (Controller) {

    case SbUsbControllerEhci:
		case SbUsbControllerUhci:
      CopyMem ((VOID *) SrcDesc, (VOID *) (&mUSB_LEGACY), sizeof(SB_SMM_SOURCE_DESC));
      break;

    default:
      ASSERT (FALSE);
      break;
    }
    break;

  case UsbWake:
    switch (Controller) {
    case SbUsbControllerEhci:
		case SbUsbControllerUhci:
      CopyMem ((VOID *) SrcDesc, (VOID *) (&mUSB_WAKE), sizeof(SB_SMM_SOURCE_DESC));
      break;

    default:
      ASSERT (FALSE);
      break;
    }
    break;

  default:
    ASSERT (FALSE);
    break;
  }
}
