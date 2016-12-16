/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  Ehci.c

Abstract:


Revision History:

Bug 2909:   Add some port 80 status codes into EDKII code.
TIME:       2011-09-23
$AUTHOR:    Liu Chunling
$REVIEWERS:
$SCOPE:     All Platforms
$TECHNICAL:
  1. Improve Port80 map table.
  2. Add Port80 status codes in the corresponding position to report status code.
  3. Change the seconed REPORT_STATUS_CODE_WITH_EXTENDED_DATA macro's parameter
     to EFI_SW_PC_INIT_END from EFI_SW_PC_INIT_BEGIN.
$END--------------------------------------------------------------------

Bug 2421:   Fixed the issue that the system hangs when do the hand-off
            of USB controllers from BIOS to OS.
TIME:       2011-07-14
$AUTHOR:    Chen Daolin
$REVIEWERS:
$SCOPE:     All Platforms Supporting USB 2.0/3.0
$TECHNICAL:
  Previously during UsbOwnershipHandoff() which hand-off the ownership of
  USB controller to OS, we unregisered USB SMI dispatcher to make it not
  invoked any more during OS runtime. But the current implementation of SMI
  core dispatcher in chipset reference code does't allow the handler to directly
  unregister itself when handler is executing. We removed the unregister to
  avoid this issue. It's OK considering USB controllers have been configurated
  to not generate SMI any more after hand-off. This will not bring any side
  effects.
$END--------------------------------------------------------------------

Bug 2421:   Fixed the issue that USB keyboard and mouse can't
            work properly after resume from S4
TIME:       2011-07-06
$AUTHOR:    Chen Daolin
$REVIEWERS:
$SCOPE:     All Platforms Supporting USB 2.0
$TECHNICAL:
  1. During the hand-off of EHCI ownership from BIOS to OS, BIOS needs
     to reset the EHCI controller to one stable state, so that OS can
     successfully initialize the controller. This is only necessary
     during S4 resume path, but to simplify the processing we will do
     this in both of S4/S4 boot path.
$END--------------------------------------------------------------------

**/

#include "UsbBus.h"
#include "../Uhci/Uhci.h"
#include "../Ehci/Ehci.h"
#include "../LegacyUsb.h"
#include "../UsbLib/UsbUtility.h"
#include "../UsbLib/SmmPciIo.h"
#include "../Ohci/Ohci.h"
#include <Protocol/SmmUsbDispatch2.h>
#include <Protocol/SmmGpiDispatch2.h>
#include <Protocol/SmmPeriodicTimerDispatch2.h>
#include <Protocol/UsbPolicy.h>
#include <library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>

EFI_GUID gEfiUsbIoProtocolGuid = EFI_USB_IO_PROTOCOL_GUID;
extern BOOLEAN                 mXhciOSRequestOwnership;
extern LIST_ENTRY              mXhcList;


EFI_STATUS
EFIAPI
UsbConnectXhciDriver (
    IN  EFI_PCI_FUNCTION_ADDRESS  *PciAddress,
    OUT EFI_USB2_HC_PROTOCOL      **Usb2HCInterface,
    OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
);

EFI_STATUS
EFIAPI
UsbDisconnectXhciDriver (
    IN EFI_USB2_HC_PROTOCOL     *Usb2HCInterface
);

VOID
XhcProcessAsyncIntOnlyRequests (
    EFI_USB2_HC_PROTOCOL            *This
);

LIST_ENTRY      mUsbBusList;
BOOLEAN         mUsbBiosOwned = FALSE;
BOOLEAN         mIfLegacyUsbStarted = FALSE;
USB_PCI_DEVICE  mUsbHcDevArray[USB_MAX_CONTROLLERS];
UINT8           mUsbHcDevCount = 0;
LIST_ENTRY      mLegacyUsbInfList;
BOOLEAN         mLegacyFreeSupport = FALSE;


EFI_STATUS
CreateUsbBusInstance (
    IN EFI_USB2_HC_PROTOCOL            *Usb2HCInterface,
    IN EFI_DEVICE_PATH_PROTOCOL        *ControllerDevicePath,
    IN OUT USB_BUS                     **pUsbBus
);

EFI_STATUS
EFIAPI
CleanupUsbBusInstance (
    IN USB_BUS     *Bus
);

EFI_STATUS
UsbScanPciBus(
    IN VOID
)
{
    UINT8         PciBusNum;
    UINT8         PciDeviceNum;
    UINT8         PciFunctionNum;
    UINT64        PciAddress;
    UINT32        PciClassCode;
    EFI_STATUS    Status;
    USB_CFG       *UsbCfg;

    GET_USB_CFG(UsbCfg);
	if(UsbCfg->LegacyUsbEnable==1)
			  {
			  return EFI_SUCCESS;
		  }

    mUsbHcDevCount = 0;
    for (PciBusNum = 0; PciBusNum < MAX_PCI_BUSS; PciBusNum++) {
        for (PciDeviceNum = 0; PciDeviceNum < MAX_PCI_DEVICES; PciDeviceNum++) {
            for (PciFunctionNum = 0; PciFunctionNum < MAX_PCI_FUNCTIONS; PciFunctionNum++) {
                //
                //  Add for check Pci Dev exist.
                //
                PciAddress = SMM_PCI_ADDRESS (PciBusNum, PciDeviceNum, PciFunctionNum, 0);
                Status = SmmPciCfgRead (
                             gSmst,
                             SmmPciWidthUint32,
                             (SMM_PCI_IO_ADDRESS *) &PciAddress,
                             &PciClassCode
                         );
                if (EFI_ERROR (Status) || (UINT16)PciClassCode == 0xFFFF) {
                    if (PciFunctionNum == 0) {
                        PciFunctionNum = MAX_PCI_FUNCTIONS;
                    }
                    continue;
                }

                PciAddress = SMM_PCI_ADDRESS (PciBusNum, PciDeviceNum, PciFunctionNum, PCI_CLASS_OFFSET);
                Status = SmmPciCfgRead (
                             gSmst,
                             SmmPciWidthUint32,
                             (SMM_PCI_IO_ADDRESS *) &PciAddress,
                             &PciClassCode
                         );
                if (EFI_ERROR(Status)) {
                    continue;
                }

                PciClassCode &= 0x0FFFFFF00;
                if ((PciClassCode == PCI_USB_UHCI_CLASS) || (PciClassCode == PCI_USB_OHCI_CLASS) ||
                        (PciClassCode == PCI_USB_EHCI_CLASS) || (PciClassCode == PCI_USB_XHCI_CLASS)) {

                    if ((UsbCfg->XhcSupport==0)&&(PciClassCode == PCI_USB_XHCI_CLASS)) {
                        continue; //skip xhci enumerate
                    }

                    mUsbHcDevArray[mUsbHcDevCount].PciAddress.Bus      = PciBusNum;
                    mUsbHcDevArray[mUsbHcDevCount].PciAddress.Device   = PciDeviceNum;
                    mUsbHcDevArray[mUsbHcDevCount].PciAddress.Function = PciFunctionNum;
                    mUsbHcDevArray[mUsbHcDevCount].Type                = USB_MAX_TYPE;

                    switch (PciClassCode ) {
                    case PCI_USB_UHCI_CLASS:
                        mUsbHcDevArray[mUsbHcDevCount].Type = USB_UHCI;
                        break;

                    case PCI_USB_OHCI_CLASS:
                        mUsbHcDevArray[mUsbHcDevCount].Type = USB_OHCI;
                        break;
                    case PCI_USB_EHCI_CLASS:
                        mUsbHcDevArray[mUsbHcDevCount].Type = USB_EHCI;
                        break;
                    case PCI_USB_XHCI_CLASS:
                        mUsbHcDevArray[mUsbHcDevCount].Type = USB_XHCI;
                        break;
                    default:
                        break;
                    }
                    if (++mUsbHcDevCount >= USB_MAX_CONTROLLERS)
                        return EFI_SUCCESS;
                }
            }
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbConnectAllHcs(
    IN VOID
)
{
    EFI_STATUS                Status;
    UINTN                     Index;
    UINTN                     Index1;
    EFI_USB2_HC_PROTOCOL      *Usb2Hc;
    USB_BUS                   *UsbBus;
    EFI_DEVICE_PATH_PROTOCOL  *ControllerPath;
    UINT8                     ControllerType;

    InitializeListHead (&mUsbBusList);
    InitializeListHead (&mOhcList);
    InitializeListHead (&mUhcList);
    InitializeListHead (&mEhcList);
    InitializeListHead (&mXhcList);
    InitializeListHead (&mLegacyUsbInfList);

    mUsbBiosOwned = TRUE;
    for (Index1 = mUsbHcDevCount; Index1 > 0; Index1 --) {
        Index = Index1 - 1;
        ControllerType =  mUsbHcDevArray[Index].Type;
        USB_DEBUG((EFI_D_ERROR, "mUsbHcDevArray[Index].Type = 0x%x\n", ControllerType));
        UsbBus = NULL;
        switch (ControllerType) {
        case USB_XHCI:
            Status = UsbConnectXhciDriver(&mUsbHcDevArray[Index].PciAddress, &Usb2Hc, &ControllerPath);
            if (EFI_ERROR(Status))
                continue;
            Status = CreateUsbBusInstance (Usb2Hc, ControllerPath, &UsbBus);
            if (EFI_ERROR (Status)) {
                UsbDisconnectXhciDriver (Usb2Hc);
                continue;
            }
            UsbBus->HostControllerType = ControllerType;
            InsertTailList (&mUsbBusList, &UsbBus->Link);
            break;

        case USB_EHCI:
            Status = UsbConnectEhciDriver(&mUsbHcDevArray[Index].PciAddress, &Usb2Hc, &ControllerPath);
            if (EFI_ERROR(Status))
                continue;
            Status = CreateUsbBusInstance (Usb2Hc, ControllerPath, &UsbBus);
            if (EFI_ERROR(Status)) {
                UsbDisconnectEhciDriver (Usb2Hc);
                continue;
            }
            UsbBus->HostControllerType = ControllerType;
            InsertTailList (&mUsbBusList, &UsbBus->Link);
            break;

        case USB_UHCI:
            Status = UsbConnectUhciDriver(&mUsbHcDevArray[Index].PciAddress, &Usb2Hc, &ControllerPath);
            if (EFI_ERROR(Status))
                continue;
            Status = CreateUsbBusInstance (Usb2Hc, ControllerPath, &UsbBus);
            if (EFI_ERROR(Status)) {
                UsbDisconnectUhciDriver (Usb2Hc);
                continue;
            }
            UsbBus->HostControllerType = ControllerType;
            InsertTailList (&mUsbBusList, &UsbBus->Link);
            break;

        case USB_OHCI:
            Status = UsbConnectOhciDriver(&mUsbHcDevArray[Index].PciAddress, &Usb2Hc, &ControllerPath);
            if (EFI_ERROR(Status))
                continue;
            Status = CreateUsbBusInstance (Usb2Hc, ControllerPath, &UsbBus);
            if (EFI_ERROR(Status)) {
                UsbDisconnectOhciDriver (Usb2Hc);
                continue;
            }
            UsbBus->HostControllerType = ControllerType;
            InsertTailList (&mUsbBusList, &UsbBus->Link);
            break;
        }
        if (UsbBus != NULL) {
          UsbRootHubEnumeration (UsbBus->Devices[0]->Interfaces[0]);
        }
    }

    return EFI_SUCCESS;
}

VOID
UsbOwnershipHandoff (
    IN  VOID
)
{
    USB_BUS                     *BusInstance;
    LIST_ENTRY                  *Link;
    EFI_USB2_HC_PROTOCOL        *Usb2Hc;
    UINT8                       ControllerType;

    USB_DEBUG((EFI_D_ERROR, "UsbOwnershipHandoff!\n"));
    REPORT_STATUS_CODE (
      EFI_PROGRESS_CODE, 
      (EFI_IO_BUS_USB | BYO_USB_HAND_OFF)
      );

    //
    // If no asynchronous interrupt transaction exists
    //
    if (IsListEmpty (&mUsbBusList)) {
        return;
    }

    UsbDisconnectVirtualKBCDriver();

    //
    // Remove the bus instances
    //
    Link = mUsbBusList.ForwardLink;
    while (Link != &mUsbBusList) {
        BusInstance = USB_BUS_FROM_LINK (Link);

        Usb2Hc = BusInstance->Usb2Hc;
        ControllerType = BusInstance->HostControllerType;
        Link = Link->ForwardLink;
        //
        // Based on some experiments on UHCI based platforms and Microsoft USB spec,
        // the hand-off of UHCI controller needs to be triggerred by setting the CF bit in
        // USBCM register. It seems be also OK if this hand-off never happen, as the OS
        // has completed this process by itself.  But the most important thing is that the
        // hand-off of UCHI controller MUST not be triggered by other type of controllers
        // (EHCI, OHCI and XHCI). Otherwise there will be problems during OS installation
        // boot when USB devices is connected.
        //
        if (!mUhciOSRequestOwnership && (ControllerType == USB_UHCI))
          continue;

        RemoveEntryList (&BusInstance->Link);
        CleanupUsbBusInstance (BusInstance);
        switch (ControllerType) {

        case USB_OHCI:
            Usb2Hc->DisableLegacySupport(Usb2Hc);
            UsbDisconnectOhciDriver (Usb2Hc);
            break;

        case USB_UHCI:
            Usb2Hc->DisableLegacySupport(Usb2Hc);
            UsbDisconnectUhciDriver (Usb2Hc);
            break;

        case USB_EHCI:
            Usb2Hc->DisableLegacySupport(Usb2Hc);
            UsbDisconnectEhciDriver (Usb2Hc);
            break;

        case USB_XHCI:
            Usb2Hc->DisableLegacySupport(Usb2Hc);
            UsbDisconnectXhciDriver (Usb2Hc);
            break;

        default:
            break;
        }
    }

    if (mUhciOSRequestOwnership || mOhciOSRequestOwnership) {
        InitializeListHead (&mUsbBusList);
        mOhciOSRequestOwnership = FALSE;
        mUhciOSRequestOwnership = FALSE;
        mUsbBiosOwned           = FALSE;
        mEhciOSRequestOwnership = FALSE;
        mXhciOSRequestOwnership = FALSE;
    } else if (mEhciOSRequestOwnership) {
        mEhciOSRequestOwnership = FALSE;
    }
    else if (mXhciOSRequestOwnership) {
        mXhciOSRequestOwnership = FALSE;
    }
    USB_DEBUG((EFI_D_ERROR, "UsbOwnershipHandoff end!\n"));
}

/**
  Compare two device pathes to check if they are exactly same.

  @param DevicePath1    A pointer to the first device path data structure.
  @param DevicePath2    A pointer to the second device path data structure.

  @retval TRUE    They are same.
  @retval FALSE   They are not same.

**/
BOOLEAN
CompareDevicePath (
    IN EFI_DEVICE_PATH_PROTOCOL *DevicePath1,
    IN EFI_DEVICE_PATH_PROTOCOL *DevicePath2
)
{
    UINTN Size1;
    UINTN Size2;

    Size1 = GetDevicePathSize (DevicePath1);
    Size2 = GetDevicePathSize (DevicePath2);

    if (Size1 != Size2) {
        return FALSE;
    }

    if (CompareMem (DevicePath1, DevicePath2, Size1) != 0) {
        return FALSE;
    }

    return TRUE;
}


EFI_STATUS
UsbSmiDispatcher(
    IN  EFI_HANDLE                                DispatchHandle,
    IN  CONST  VOID                               *Context,
    IN  OUT    VOID                               *CommBuffer,
    IN  OUT    UINTN                              *CommBufferSize
)
{
  USB_BUS                         *BusInstance;
  EFI_USB2_HC_PROTOCOL            *Usb2Hc;
  LIST_ENTRY                      *Link;


//DEBUG((EFI_D_INFO, "UsbSmi\n"));

  if (!mUsbBiosOwned)
    return EFI_SUCCESS;

  Link = mUsbBusList.ForwardLink;
  while (Link != &mUsbBusList) {
    BusInstance = USB_BUS_FROM_LINK(Link);
    Usb2Hc = BusInstance->Usb2Hc;
    Usb2Hc->LegacyCallback(Usb2Hc);
    Link = Link->ForwardLink;
  }

  if (mUhciOSRequestOwnership || mEhciOSRequestOwnership ||
      mOhciOSRequestOwnership || mXhciOSRequestOwnership) {
    UsbOwnershipHandoff();
  }

  return EFI_SUCCESS;
}


EFI_STATUS
UsbPeriodicTimerDispatch (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context         OPTIONAL,
  IN OUT VOID    *CommBuffer      OPTIONAL,
  IN OUT UINTN   *CommBufferSize  OPTIONAL
)
{
  USB_BUS                     *BusInstance;
  EFI_USB2_HC_PROTOCOL        *Usb2Hc;
  LIST_ENTRY                  *Link;
  UINT8                       ControllerType;
  static UINT8                EnumeCount = 0;

  if (!mUsbBiosOwned)
    return EFI_SUCCESS;

  EnumeCount++;

  // UsbVirtualKbcDispatcher (FALSE);

  Link = mUsbBusList.ForwardLink;
  while (Link != &mUsbBusList) {
    BusInstance = USB_BUS_FROM_LINK (Link);
    ControllerType = BusInstance->HostControllerType;
    Usb2Hc = BusInstance->Usb2Hc;
    if (ControllerType == USB_XHCI) {
      XhcProcessAsyncIntOnlyRequests(Usb2Hc);
    }
    Usb2Hc->LegacyCallback(Usb2Hc);
    if (EnumeCount == 15 && ControllerType == USB_UHCI) {
      Usb2Hc->BusEnumerationCallback (Usb2Hc->EnumerationContext);
    }
    Link = Link->ForwardLink;
  }

  if (EnumeCount == 15) {
    EnumeCount = 0;
  }

  if (mUhciOSRequestOwnership || mEhciOSRequestOwnership ||
      mOhciOSRequestOwnership || mXhciOSRequestOwnership) {
    UsbOwnershipHandoff();
  }

  // UsbVirtualKbcDispatcher (TRUE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbBusEnumerate(
    IN VOID
)
{
    USB_BUS                         *BusInstance;
    LIST_ENTRY                      *Link;
    USB_DEVICE                      *Device;
    USB_INTERFACE                   *HubIf;
    UINT8                           DeviceIndex;
    UINT8                           InterfaceIndex;
    EFI_USB2_HC_PROTOCOL            *Usb2Hc;
    EFI_STATUS                      Status;
  EFI_SMM_PERIODIC_TIMER_DISPATCH2_PROTOCOL *PeriodDispatch;
  EFI_SMM_PERIODIC_TIMER_REGISTER_CONTEXT  PeriodContext;
  EFI_HANDLE                               PeriodicTimerHandle;


    Link = mUsbBusList.ForwardLink;
    while (Link != &mUsbBusList) {
        BusInstance = USB_BUS_FROM_LINK (Link);

        for (DeviceIndex = 1; DeviceIndex < USB_MAX_DEVICES; DeviceIndex++) {
            Device = BusInstance->Devices[DeviceIndex];
            if (Device == NULL)
                continue;

            for (InterfaceIndex = 0;
                    InterfaceIndex < Device->NumOfInterface;
                    InterfaceIndex++) {

                HubIf = Device->Interfaces[InterfaceIndex];
                if ((HubIf == NULL) || HubIf->Type != UDT_HUB)
                    continue;
                HubIf->ChangeMap = AllocatePool (
                                       HubIf->NumOfPort / 8 + 1
                                   );
                if (HubIf->ChangeMap == NULL)
                    continue;
                SetMem (
                    HubIf->ChangeMap,
                    HubIf->NumOfPort / 8 + 1,
                    0xff);
                UsbHubEnumeration (HubIf);
            }
        }
        Link = Link->ForwardLink;
    }

  Link = mUsbBusList.ForwardLink;
  while (Link != &mUsbBusList) {
      BusInstance = USB_BUS_FROM_LINK (Link);
      Usb2Hc = BusInstance->Usb2Hc;
      Usb2Hc->EnumerationContext = BusInstance->Devices[0]->Interfaces[0];
      Usb2Hc->BusEnumerationCallback = (EFI_USB_BUS_ENUMERATION_CALLBACK)UsbRootHubEnumeration;
      Link = Link->ForwardLink;
  }

  Status = gSmst->SmmLocateProtocol(&gEfiSmmPeriodicTimerDispatch2ProtocolGuid, NULL, &PeriodDispatch);
  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }

  PeriodContext.Period          = 0;
  PeriodContext.SmiTickInterval = 16;
  Status = PeriodDispatch->Register (
                             PeriodDispatch,
                             UsbPeriodicTimerDispatch,
                             &PeriodContext,
                             &PeriodicTimerHandle
                             );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitUsbConfigFromPolicy(
    IN VOID
)
{
    EFI_STATUS                      Status;
    USB_CFG                         *UsbCfg;
    EFI_USB_POLICY_PROTOCOL         *UsbPolicy;
    //
    // Set USB Configuration Information
    //
    GET_USB_CFG(UsbCfg);

    UsbCfg->Configurated = 1;

    Status = gBS->LocateProtocol (&gUsbPolicyGuid, NULL, &UsbPolicy);
    if (!EFI_ERROR(Status)) {
        if (UsbPolicy->UsbMassStorageEmulationType >2)
            UsbCfg->USBHDDForceType = ATUO_TYPE;
        else
            UsbCfg->USBHDDForceType = UsbPolicy->UsbMassStorageEmulationType;
        UsbCfg->UsbDeviceInfor.UsbOperationMode  = UsbPolicy->UsbOperationMode;
        UsbCfg->UsbDeviceInfor.LegacyKBEnable    = UsbPolicy->LegacyKBEnable;
        UsbCfg->UsbDeviceInfor.LegacyMSEnable    = UsbPolicy->LegacyMSEnable;
        UsbCfg->UsbDeviceInfor.USBPeriodSupport  = UsbPolicy->USBPeriodSupport;
        UsbCfg->LegacyUsbEnable                  = UsbPolicy->LegacyUsbEnable;
        UsbCfg->XhcSupport                       = UsbPolicy->XhcSupport;

        mUsbTimeTue              = (UsbPolicy->UsbTimeTue > 0)? TRUE:FALSE;
        mInternelHubExist        = (UsbPolicy->InternelHubExist > 0)? TRUE:FALSE;
        mEnumWaitPortStableStall = (UINTN)(UsbPolicy->EnumWaitPortStableStall * USB_BUS_1_MILLISECOND);
        USB_DEBUG((EFI_D_ERROR, "Get Usb Plolicy  USB policy\n"));
    } else {
        UsbCfg->USBHDDForceType                  = ATUO_TYPE;
        UsbCfg->UsbDeviceInfor.UsbOperationMode  = HIGH_SPEED;
        UsbCfg->UsbDeviceInfor.LegacyKBEnable    = LEGACY_KB_EN;
        UsbCfg->UsbDeviceInfor.LegacyMSEnable    = LEGACY_MS_EN;
        UsbCfg->UsbDeviceInfor.USBPeriodSupport  = LEGACY_PERIOD_UN_SUPP;
        UsbCfg->LegacyUsbEnable                  = LEGACY_USB_EN;
        UsbCfg->XhcSupport                        = 0;
        mUsbTimeTue              = FALSE;
        mInternelHubExist        = TRUE;
        mEnumWaitPortStableStall = (UINTN)USB_WAIT_PORT_STABLE_STALL;
        USB_DEBUG((EFI_D_ERROR, "Defaule USB policy\n"));
    }
    UsbCfg->Codebase = ICBD_CODE_BASE;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
StartLegacyUsb(
    IN VOID
)
{
    EFI_STATUS Status;

    DEBUG((EFI_D_ERROR, __FUNCTION__"\n"));
    if (mIfLegacyUsbStarted) {
      return EFI_SUCCESS;
    }

    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_IO_BUS_USB | EFI_IOB_PC_INIT));

    InitUsbConfigFromPolicy();

    UsbScanPciBus ();

    Status = UsbInitMemryManger();
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "UsbInitMemryManger():%r\n", Status));
      return Status;
    }

    UsbConnectAllHcs();

    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_IO_BUS_USB | EFI_IOB_PC_DETECT));
    UsbBusEnumerate();

    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_IO_BUS_USB | EFI_IOB_PC_ENABLE));
    mIfLegacyUsbStarted = TRUE;

    return Status;
}

EFI_STATUS
EFIAPI
EndLegacyUsb(
    IN VOID
)
{
    USB_DEBUG((EFI_D_ERROR, "In EndLegacyUsb\n"));

    if (!mIfLegacyUsbStarted) {
        return EFI_SUCCESS;
    }
    mIfLegacyUsbStarted = FALSE;

    mUhciOSRequestOwnership = TRUE;
    mOhciOSRequestOwnership = TRUE;

    UsbOwnershipHandoff();

    USB_DEBUG((EFI_D_ERROR, "Out EndLegacyUsb\n"));
    return EFI_SUCCESS;
}

/**
  USB_IO function to execute a control transfer. This
  function will execute the USB transfer. If transfer
  successes, it will sync the internal state of USB bus
  with device state.

  @param  This                   The USB_IO instance
  @param  Request                The control transfer request
  @param  Direction              Direction for data stage
  @param  Timeout                The time to wait before timeout
  @param  Data                   The buffer holding the data
  @param  DataLength             Then length of the data
  @param  UsbStatus              USB result

  @retval EFI_INVALID_PARAMETER  The parameters are invalid
  @retval EFI_SUCCESS            The control transfer succeeded.
  @retval Others                 Failed to execute the transfer

**/
EFI_STATUS
EFIAPI
UsbIoControlTransfer (
  IN  EFI_USB_IO_PROTOCOL     *This,
  IN  EFI_USB_DEVICE_REQUEST  *Request,
  IN  EFI_USB_DATA_DIRECTION  Direction,
  IN  UINT32                  Timeout,
  IN  OUT VOID                *Data,      OPTIONAL
  IN  UINTN                   DataLength, OPTIONAL
  OUT UINT32                  *UsbStatus
  )
{
  USB_DEVICE              *Dev;
  USB_INTERFACE           *UsbIf;
  USB_ENDPOINT_DESC       *EpDesc;
  EFI_STATUS              Status;

  if (UsbStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbIf  = USB_INTERFACE_FROM_USBIO (This);
  Dev    = UsbIf->Device;

  Status = UsbHcControlTransfer (
             Dev->Bus,
             Dev->Address,
             Dev->Speed,
             Dev->MaxPacket0,
             Request,
             Direction,
             Data,
             &DataLength,
             (UINTN) Timeout,
             &Dev->Translator,
             UsbStatus
             );

  if (EFI_ERROR (Status) || (*UsbStatus != EFI_USB_NOERROR)) {
    //
    // Clear TT buffer when CTRL/BULK split transaction failes
    // Clear the TRANSLATOR TT buffer, not parent's buffer
    //
    if (Dev->Translator.TranslatorHubAddress != 0) {
      UsbHubCtrlClearTTBuffer (
        Dev->Bus->Devices[Dev->Translator.TranslatorHubAddress],
        Dev->Translator.TranslatorPortNumber,
        Dev->Address,
        0,
        USB_ENDPOINT_CONTROL
        );
    }

    goto ON_EXIT;
  }

  //
  // Some control transfer will change the device's internal
  // status, such as Set_Configuration and Set_Interface.
  // We must synchronize the bus driver's status with that in
  // device. We ignore the Set_Descriptor request because it's
  // hardly used by any device, especially in pre-boot environment
  //

  //
  // Reset the endpoint toggle when endpoint stall is cleared
  //
  if ((Request->Request     == USB_REQ_CLEAR_FEATURE) &&
      (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD,
                                                 USB_TARGET_ENDPOINT)) &&
      (Request->Value       == USB_FEATURE_ENDPOINT_HALT)) {

    EpDesc = UsbGetEndpointDesc (UsbIf, (UINT8) Request->Index);

    if (EpDesc != NULL) {
      EpDesc->Toggle = 0;
    }
  }

  //
  // Select a new configuration. This is a dangerous action. Upper driver
  // should stop use its current UsbIo after calling this driver. The old
  // UsbIo will be uninstalled and new UsbIo be installed. We can't use
  // ReinstallProtocol since interfaces in different configuration may be
  // completely irrelevant.
  //
  if ((Request->Request == USB_REQ_SET_CONFIG) &&
      (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD,
                                                 USB_TARGET_DEVICE))) {
    //
    // Don't re-create the USB interfaces if configuration isn't changed.
    //
    if ((Dev->ActiveConfig != NULL) &&
        (Request->Value == Dev->ActiveConfig->Desc.ConfigurationValue)) {

      goto ON_EXIT;
    }
    USB_DEBUG ((EFI_D_INFO, "UsbIoControlTransfer: configure changed!!! Do NOT use old UsbIo!!!\n"));

    if (Dev->ActiveConfig != NULL) {
      UsbRemoveConfig (Dev);
    }

    if (Request->Value != 0) {
      Status = UsbSelectConfig (Dev, (UINT8) Request->Value);
    }

    //
    // Exit now, Old USB_IO is invalid now
    //
    goto ON_EXIT;
  }

  //
  // A new alternative setting is selected for the interface.
  // No need to reinstall UsbIo in this case because only
  // underlying communication endpoints are changed. Functionality
  // should remains the same.
  //
  if ((Request->Request     == USB_REQ_SET_INTERFACE) &&
      (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD,
                                                 USB_TARGET_INTERFACE)) &&
      (Request->Index       == UsbIf->IfSetting->Desc.InterfaceNumber)) {

    Status = UsbSelectSetting (UsbIf->IfDesc, (UINT8) Request->Value);

    if (!EFI_ERROR (Status)) {
      UsbIf->IfSetting = UsbIf->IfDesc->Settings[UsbIf->IfDesc->ActiveIndex];
    }
  }

ON_EXIT:
  return Status;
}


/**
  Execute a bulk transfer to the device endpoint.

  @param  This                   The USB IO instance.
  @param  Endpoint               The device endpoint.
  @param  Data                   The data to transfer.
  @param  DataLength             The length of the data to transfer.
  @param  Timeout                Time to wait before timeout.
  @param  UsbStatus              The result of USB transfer.

  @retval EFI_SUCCESS            The bulk transfer is OK.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval Others                 Failed to execute transfer, reason returned in
                                 UsbStatus.

**/
EFI_STATUS
EFIAPI
UsbIoBulkTransfer (
  IN  EFI_USB_IO_PROTOCOL *This,
  IN  UINT8               Endpoint,
  IN  OUT VOID            *Data,
  IN  OUT UINTN           *DataLength,
  IN  UINTN               Timeout,
  OUT UINT32              *UsbStatus
  )
{
  USB_DEVICE              *Dev;
  USB_INTERFACE           *UsbIf;
  USB_ENDPOINT_DESC       *EpDesc;
  UINT8                   BufNum;
  UINT8                   Toggle;
  EFI_STATUS              Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((USB_ENDPOINT_ADDR (Endpoint) == 0) || (USB_ENDPOINT_ADDR(Endpoint) > 15) ||
      (UsbStatus == NULL)) {

    return EFI_INVALID_PARAMETER;
  }


  UsbIf   = USB_INTERFACE_FROM_USBIO (This);
  Dev     = UsbIf->Device;

  EpDesc  = UsbGetEndpointDesc (UsbIf, Endpoint);

  if ((EpDesc == NULL) || (USB_ENDPOINT_TYPE (&EpDesc->Desc) != USB_ENDPOINT_BULK)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  BufNum  = 1;
  Toggle  = EpDesc->Toggle;
  Status  = UsbHcBulkTransfer (
              Dev->Bus,
              Dev->Address,
              Endpoint,
              Dev->Speed,
              EpDesc->Desc.MaxPacketSize,
              BufNum,
              &Data,
              DataLength,
              &Toggle,
              Timeout,
              &Dev->Translator,
              UsbStatus
              );

  EpDesc->Toggle = Toggle;

  if (EFI_ERROR (Status) || (*UsbStatus != EFI_USB_NOERROR)) {
    //
    // Clear TT buffer when CTRL/BULK split transaction failes.
    // Clear the TRANSLATOR TT buffer, not parent's buffer
    //
    if (Dev->Translator.TranslatorHubAddress != 0) {
      UsbHubCtrlClearTTBuffer (
        Dev->Bus->Devices[Dev->Translator.TranslatorHubAddress],
        Dev->Translator.TranslatorPortNumber,
        Dev->Address,
        0,
        USB_ENDPOINT_BULK
        );
    }
  }

ON_EXIT:
  return Status;
}


/**
  Execute a synchronous interrupt transfer.

  @param  This                   The USB IO instance.
  @param  Endpoint               The device endpoint.
  @param  Data                   The data to transfer.
  @param  DataLength             The length of the data to transfer.
  @param  Timeout                Time to wait before timeout.
  @param  UsbStatus              The result of USB transfer.

  @retval EFI_SUCCESS            The synchronous interrupt transfer is OK.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval Others                 Failed to execute transfer, reason returned in
                                 UsbStatus.

**/
EFI_STATUS
EFIAPI
UsbIoSyncInterruptTransfer (
  IN  EFI_USB_IO_PROTOCOL *This,
  IN  UINT8               Endpoint,
  IN  OUT VOID            *Data,
  IN  OUT UINTN           *DataLength,
  IN  UINTN               Timeout,
  OUT UINT32              *UsbStatus
  )
{
  USB_DEVICE              *Dev;
  USB_INTERFACE           *UsbIf;
  USB_ENDPOINT_DESC       *EpDesc;
  UINT8                   Toggle;
  EFI_STATUS              Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((USB_ENDPOINT_ADDR (Endpoint) == 0) || (USB_ENDPOINT_ADDR(Endpoint) > 15) ||
      (UsbStatus == NULL)) {

    return EFI_INVALID_PARAMETER;
  }

  UsbIf   = USB_INTERFACE_FROM_USBIO (This);
  Dev     = UsbIf->Device;

  EpDesc  = UsbGetEndpointDesc (UsbIf, Endpoint);

  if ((EpDesc == NULL) || (USB_ENDPOINT_TYPE (&EpDesc->Desc) != USB_ENDPOINT_INTERRUPT)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Toggle = EpDesc->Toggle;
  Status = UsbHcSyncInterruptTransfer (
             Dev->Bus,
             Dev->Address,
             Endpoint,
             Dev->Speed,
             EpDesc->Desc.MaxPacketSize,
             Data,
             DataLength,
             &Toggle,
             Timeout,
             &Dev->Translator,
             UsbStatus
             );

  EpDesc->Toggle = Toggle;

ON_EXIT:
  return Status;
}


/**
  Queue a new asynchronous interrupt transfer, or remove the old
  request if (IsNewTransfer == FALSE).

  @param  This                   The USB_IO instance.
  @param  Endpoint               The device endpoint.
  @param  IsNewTransfer          Whether this is a new request, if it's old, remove
                                 the request.
  @param  PollInterval           The interval to poll the transfer result, (in ms).
  @param  DataLength             The length of perodic data transfer.
  @param  Callback               The function to call periodicaly when transfer is
                                 ready.
  @param  Context                The context to the callback.

  @retval EFI_SUCCESS            New transfer is queued or old request is removed.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval Others                 Failed to queue the new request or remove the old
                                 request.

**/
EFI_STATUS
EFIAPI
UsbIoAsyncInterruptTransfer (
  IN EFI_USB_IO_PROTOCOL              *This,
  IN UINT8                            Endpoint,
  IN BOOLEAN                          IsNewTransfer,
  IN UINTN                            PollInterval,       OPTIONAL
  IN UINTN                            DataLength,         OPTIONAL
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK  Callback,           OPTIONAL
  IN VOID                             *Context            OPTIONAL
  )
{
  USB_DEVICE              *Dev;
  USB_INTERFACE           *UsbIf;
  USB_ENDPOINT_DESC       *EpDesc;
  UINT8                   Toggle;
  EFI_STATUS              Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((USB_ENDPOINT_ADDR (Endpoint) == 0) || (USB_ENDPOINT_ADDR (Endpoint) > 15)) {
    return EFI_INVALID_PARAMETER;
  }

  UsbIf   = USB_INTERFACE_FROM_USBIO (This);
  Dev     = UsbIf->Device;

  EpDesc  = UsbGetEndpointDesc (UsbIf, Endpoint);

  if ((EpDesc == NULL) || (USB_ENDPOINT_TYPE (&EpDesc->Desc) != USB_ENDPOINT_INTERRUPT)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Toggle  = EpDesc->Toggle;
  Status  = UsbHcAsyncInterruptTransfer (
              Dev->Bus,
              Dev->Address,
              Endpoint,
              Dev->Speed,
              EpDesc->Desc.MaxPacketSize,
              IsNewTransfer,
              &Toggle,
              PollInterval,
              DataLength,
              &Dev->Translator,
              Callback,
              Context
              );

  EpDesc->Toggle = Toggle;

ON_EXIT:
  return Status;
}

EFI_STATUS
EFIAPI
UsbIoAsyncInterruptOnlyTransfer (
    IN EFI_USB_IO_PROTOCOL              *This,
    IN BOOLEAN                          IsNewTransfer,
    IN UINTN                            PollInterval,
    IN EFI_ASYNC_USB_TRANSFER_CALLBACK  Callback,
    IN VOID                             *Context
)
/*++

Routine Description:

  Queue a new asynchronous interrupt transfer, or remove the old
  request if (IsNewTransfer == FALSE)

Arguments:

  This          - The USB_IO instance
  Endpoint      - The device endpoint
  IsNewTransfer - Whether this is a new request, if it's old, remove the request
  PollInterval  - The interval to poll the transfer result, (in ms)
  DataLength    - The length of perodic data transfer
  Callback      - The function to call periodicaly when transfer is ready
  Context       - The context to the callback

Returns:

  EFI_SUCCESS           - New transfer is queued or old request is removed
  EFI_INVALID_PARAMETER - Some parameters are invalid
  Others                - Failed to queue the new request or remove the old request

--*/
{
    USB_INTERFACE           *UsbIf;
    EFI_STATUS              Status;

    if (This == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    UsbIf   = USB_INTERFACE_FROM_USBIO (This);
    Status  = UsbHcAsyncInterruptOnlyTransfer (
                  UsbIf->Device->Bus,
                  IsNewTransfer,
                  PollInterval,
                  Callback,
                  Context
              );
    return Status;
}

/**
  Execute a synchronous isochronous transfer.

  @param  This                   The USB IO instance.
  @param  DeviceEndpoint         The device endpoint.
  @param  Data                   The data to transfer.
  @param  DataLength             The length of the data to transfer.
  @param  UsbStatus              The result of USB transfer.

  @retval EFI_UNSUPPORTED        Currently isochronous transfer isn't supported.

**/
EFI_STATUS
EFIAPI
UsbIoIsochronousTransfer (
  IN  EFI_USB_IO_PROTOCOL *This,
  IN  UINT8               DeviceEndpoint,
  IN  OUT VOID            *Data,
  IN  UINTN               DataLength,
  OUT UINT32              *Status
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Queue an asynchronous isochronous transfer.

  @param  This                   The USB_IO instance.
  @param  DeviceEndpoint         The device endpoint.
  @param  Data                   The data to transfer.
  @param  DataLength             The length of perodic data transfer.
  @param  IsochronousCallBack    The function to call periodicaly when transfer is
                                 ready.
  @param  Context                The context to the callback.

  @retval EFI_UNSUPPORTED        Currently isochronous transfer isn't supported.

**/
EFI_STATUS
EFIAPI
UsbIoAsyncIsochronousTransfer (
  IN EFI_USB_IO_PROTOCOL              *This,
  IN UINT8                            DeviceEndpoint,
  IN OUT VOID                         *Data,
  IN UINTN                            DataLength,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK  IsochronousCallBack,
  IN VOID                             *Context              OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Retrieve the device descriptor of the device.

  @param  This                   The USB IO instance.
  @param  Descriptor             The variable to receive the device descriptor.

  @retval EFI_SUCCESS            The device descriptor is returned.
  @retval EFI_INVALID_PARAMETER  The parameter is invalid.

**/
EFI_STATUS
EFIAPI
UsbIoGetDeviceDescriptor (
  IN  EFI_USB_IO_PROTOCOL       *This,
  OUT EFI_USB_DEVICE_DESCRIPTOR *Descriptor
  )
{
  USB_DEVICE              *Dev;
  USB_INTERFACE           *UsbIf;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Descriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  UsbIf  = USB_INTERFACE_FROM_USBIO (This);
  Dev    = UsbIf->Device;

  CopyMem (Descriptor, &Dev->DevDesc->Desc, sizeof (EFI_USB_DEVICE_DESCRIPTOR));

  return EFI_SUCCESS;
}


/**
  Return the configuration descriptor of the current active configuration.

  @param  This                   The USB IO instance.
  @param  Descriptor             The USB configuration descriptor.

  @retval EFI_SUCCESS            The active configuration descriptor is returned.
  @retval EFI_INVALID_PARAMETER  Some parameter is invalid.
  @retval EFI_NOT_FOUND          Currently no active configuration is selected.

**/
EFI_STATUS
EFIAPI
UsbIoGetActiveConfigDescriptor (
  IN  EFI_USB_IO_PROTOCOL       *This,
  OUT EFI_USB_CONFIG_DESCRIPTOR *Descriptor
  )
{
  USB_DEVICE              *Dev;
  USB_INTERFACE           *UsbIf;
  EFI_STATUS              Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Descriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  UsbIf  = USB_INTERFACE_FROM_USBIO (This);
  Dev    = UsbIf->Device;

  if (Dev->ActiveConfig == NULL) {
    Status = EFI_NOT_FOUND;
    goto ON_EXIT;
  }

  CopyMem (Descriptor, &(Dev->ActiveConfig->Desc), sizeof (EFI_USB_CONFIG_DESCRIPTOR));

ON_EXIT:
  return Status;
}


/**
  Retrieve the active interface setting descriptor for this USB IO instance.

  @param  This                   The USB IO instance.
  @param  Descriptor             The variable to receive active interface setting.

  @retval EFI_SUCCESS            The active interface setting is returned.
  @retval EFI_INVALID_PARAMETER  Some parameter is invalid.

**/
EFI_STATUS
EFIAPI
UsbIoGetInterfaceDescriptor (
  IN  EFI_USB_IO_PROTOCOL           *This,
  OUT EFI_USB_INTERFACE_DESCRIPTOR  *Descriptor
  )
{
  USB_INTERFACE           *UsbIf;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Descriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbIf  = USB_INTERFACE_FROM_USBIO (This);
  CopyMem (Descriptor, &(UsbIf->IfSetting->Desc), sizeof (EFI_USB_INTERFACE_DESCRIPTOR));

  return EFI_SUCCESS;
}


/**
  Retrieve the endpoint descriptor from this interface setting.

  @param  This                   The USB IO instance.
  @param  Index                  The index (start from zero) of the endpoint to
                                 retrieve.
  @param  Descriptor             The variable to receive the descriptor.

  @retval EFI_SUCCESS            The endpoint descriptor is returned.
  @retval EFI_INVALID_PARAMETER  Some parameter is invalid.

**/
EFI_STATUS
EFIAPI
UsbIoGetEndpointDescriptor (
  IN  EFI_USB_IO_PROTOCOL         *This,
  IN  UINT8                       Index,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR *Descriptor
  )
{
  USB_INTERFACE           *UsbIf;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbIf  = USB_INTERFACE_FROM_USBIO (This);

  if ((Descriptor == NULL) || (Index > 15)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Index >= UsbIf->IfSetting->Desc.NumEndpoints) {
    return EFI_NOT_FOUND;
  }

  CopyMem (
    Descriptor,
    &(UsbIf->IfSetting->Endpoints[Index]->Desc),
    sizeof (EFI_USB_ENDPOINT_DESCRIPTOR)
    );

  return EFI_SUCCESS;
}


/**
  Retrieve the supported language ID table from the device.

  @param  This                   The USB IO instance.
  @param  LangIDTable            The table to return the language IDs.
  @param  TableSize              The size, in bytes, of the table LangIDTable.

  @retval EFI_SUCCESS            The language ID is return.

**/
EFI_STATUS
EFIAPI
UsbIoGetSupportedLanguages (
  IN  EFI_USB_IO_PROTOCOL *This,
  OUT UINT16              **LangIDTable,
  OUT UINT16              *TableSize
  )
{
  USB_DEVICE              *Dev;
  USB_INTERFACE           *UsbIf;
  UINT8                   *Buf;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbIf         = USB_INTERFACE_FROM_USBIO (This);
  Dev           = UsbIf->Device;

  *LangIDTable  = Dev->LangId;
  *TableSize    = (UINT16) (Dev->TotalLangId * sizeof (UINT16));

  Buf = (UINT8*)UsbAllocatePool (Dev->TotalLangId * 2);
  if (Buf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Buf, Dev->LangId, Dev->TotalLangId * 2);

  *LangIDTable  = (UINT16*) Buf;

  return EFI_SUCCESS;
}


/**
  Retrieve an indexed string in the language of LangID.

  @param  This                   The USB IO instance.
  @param  LangID                 The language ID of the string to retrieve.
  @param  StringIndex            The index of the string.
  @param  String                 The variable to receive the string.

  @retval EFI_SUCCESS            The string is returned.
  @retval EFI_NOT_FOUND          No such string existed.

**/
EFI_STATUS
EFIAPI
UsbIoGetStringDescriptor (
  IN  EFI_USB_IO_PROTOCOL   *This,
  IN  UINT16                LangID,
  IN  UINT8                 StringIndex,
  OUT CHAR16                **String
  )
{
  USB_DEVICE                *Dev;
  USB_INTERFACE             *UsbIf;
  EFI_USB_STRING_DESCRIPTOR *StrDesc;
  UINT8                     *Buf;
  UINT8                     Index;
  EFI_STATUS                Status;
  UINT8                     StrData;
  UINT8                     *Str;
  UINTN                     tempIndex;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((StringIndex == 0) || (LangID == 0)) {
    return EFI_NOT_FOUND;
  }

  UsbIf  = USB_INTERFACE_FROM_USBIO (This);
  Dev    = UsbIf->Device;

  //
  // Check whether language ID is supported
  //
  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < Dev->TotalLangId; Index++) {
    ASSERT (Index < USB_MAX_LANG_ID);
    if (Dev->LangId[Index] == LangID) {
      break;
    }
  }

  if (Index == Dev->TotalLangId) {
    goto ON_EXIT;
  }

  //
  // Retrieve the string descriptor then allocate a buffer
  // to hold the string itself.
  //
  StrDesc = UsbGetOneString (Dev, StringIndex, LangID);

  if (StrDesc == NULL) {
    goto ON_EXIT;
  }

  if (StrDesc->Length <= 2) {
    goto FREE_STR;
  }

  // Buf = (UINT8*)UsbAllocatePool (StrDesc->Length);
  Status = gBS->AllocatePool (EfiBootServicesData, StrDesc->Length, &Buf);
  if (Buf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FREE_STR;
  }

  tempIndex = (StrDesc->Length - 2) / 2;
  StrData   = 0;

  //
  // Fix for some USB flash Disk manufacture string's format is not UINT16.
  //
  if (*StrDesc->String & 0xFF00) {
    Str = (UINT8*)StrDesc->String;
    for (Index = 0; Index < tempIndex; Index++) {
      StrData = *(Str + Index);
      if (StrData != '\0') {
        *(Buf + Index * 2)     = StrData;
        *(Buf + Index * 2 + 1) = 0;
      } else {
        break;
      }
    }
  } else {
    CopyMem ((VOID*)Buf, StrDesc->String, StrDesc->Length - 2);
  }

  *String = (CHAR16 *) Buf;
  Status  = EFI_SUCCESS;

FREE_STR:
    FreePool (StrDesc);

ON_EXIT:
  return Status;
}


/**
  Reset the device, then if that succeeds, reconfigure the
  device with its address and current active configuration.

  @param  This                   The USB IO instance.

  @retval EFI_SUCCESS            The device is reset and configured.
  @retval Others                 Failed to reset the device.

**/
EFI_STATUS
EFIAPI
UsbIoPortReset (
  IN EFI_USB_IO_PROTOCOL  *This
  )
{
  USB_INTERFACE           *UsbIf;
  USB_INTERFACE           *HubIf;
  USB_DEVICE              *Dev;
  EFI_STATUS              Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbIf  = USB_INTERFACE_FROM_USBIO (This);
  Dev    = UsbIf->Device;

  if (UsbIf->IsHub) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  HubIf  = Dev->ParentIf;
  Status = HubIf->HubApi->ResetPort (HubIf, Dev->ParentPort);

  if (EFI_ERROR (Status)) {
    USB_DEBUG (( EFI_D_ERROR, "UsbIoPortReset: failed to reset hub port %d@hub  %d, %r \n",
                Dev->ParentPort, Dev->ParentAddr, Status));

    goto ON_EXIT;
  }

  //
  // Reset the device to its current address. The device now has an address
  // of ZERO after port reset, so need to set Dev->Address to the device again for
  // host to communicate with it.
  //
  Status  = UsbSetAddress (Dev, Dev->Address);

  Stall (USB_SET_DEVICE_ADDRESS_STALL);
  
  if (EFI_ERROR (Status)) {
    //
    // It may fail due to device disconnection or other reasons.
    //
    USB_DEBUG (( EFI_D_ERROR, "UsbIoPortReset: failed to set address for device %d - %r\n",
                Dev->Address, Status));

    goto ON_EXIT;
  }

  USB_DEBUG (( EFI_D_INFO, "UsbIoPortReset: device is now ADDRESSED at %d\n", Dev->Address));

  //
  // Reset the current active configure, after this device
  // is in CONFIGURED state.
  //
  if (Dev->ActiveConfig != NULL) {
    Status = UsbSetConfig (Dev, Dev->ActiveConfig->Desc.ConfigurationValue);

    if (EFI_ERROR (Status)) {
      USB_DEBUG (( EFI_D_ERROR, "UsbIoPortReset: failed to set configure for device %d - %r\n",
                  Dev->Address, Status));
    }
  }

ON_EXIT:
  return Status;
}

EFI_STATUS
CreateUsbBusInstance (
    IN EFI_USB2_HC_PROTOCOL            *Usb2HCInterface,
    IN EFI_DEVICE_PATH_PROTOCOL        *ControllerDevicePath,
    IN OUT USB_BUS                     **pUsbBus
)
/*++

Routine Description:

  Install Usb Bus Protocol on host controller, and start the Usb bus

Arguments:

  This                - The USB bus driver binding instance
  Controller          - The controller to check
  RemainingDevicePath - The remaining device patch

Returns:

  EFI_SUCCESS           - The controller is controlled by the usb bus
  EFI_ALREADY_STARTED   - The controller is already controlled by the usb bus
  EFI_OUT_OF_RESOURCES  - Failed to allocate resources

--*/
{
  USB_BUS                 *UsbBus;
  USB_DEVICE              *RootHub;
  USB_INTERFACE           *RootIf;
  EFI_STATUS              Status;

    //
    // Allocate USB_BUS_CONTROLLER_DEVICE structure
    //
  UsbBus = AllocateZeroPool (sizeof (USB_BUS));
  if (UsbBus == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UsbBus->Signature   = USB_BUS_SIGNATURE;
  UsbBus->Usb2Hc      = Usb2HCInterface;
  UsbBus->DevicePath  = ControllerDevicePath;
  UsbHcReset (UsbBus, EFI_USB_HC_RESET_GLOBAL);
  UsbHcSetState (UsbBus, EfiUsbHcStateOperational);


  //
  // Initial the wanted child device path list, and add first RemainingDevicePath
  //
  InitializeListHead (&UsbBus->WantedUsbIoDPList);
  Status = UsbBusAddWantedUsbIoDP (&UsbBus->BusId, NULL);
  ASSERT (!EFI_ERROR (Status));
  //
  // Create a fake usb device for root hub
  //
  RootHub = AllocateZeroPool (sizeof (USB_DEVICE));

  if (RootHub == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto UNINSTALL_USBBUS;
  }

  RootIf = AllocateZeroPool (sizeof (USB_INTERFACE));

  if (RootIf == NULL) {
    FreePool (RootHub);
    Status = EFI_OUT_OF_RESOURCES;
    goto FREE_ROOTHUB;
  }

  RootHub->Bus            = UsbBus;
  RootHub->NumOfInterface = 1;
  RootHub->Interfaces[0]  = RootIf;
  RootHub->Tier           = 0;
  RootIf->Signature       = USB_INTERFACE_SIGNATURE;
  RootIf->Device          = RootHub;
  RootIf->DevicePath      = UsbBus->DevicePath;

  Status                  = mUsbRootHubApi.Init (RootIf);

  if (EFI_ERROR (Status)) {
    USB_DEBUG ((EFI_D_ERROR, "UsbBusStart: Failed to init root hub %r\n", Status));
    goto FREE_ROOTHUB;
  }

  UsbBus->Devices[0] = RootHub;
  *pUsbBus = UsbBus;

  USB_DEBUG ((EFI_D_INFO, "UsbBusStart: usb bus started on, root hub %p\n", RootIf));
  return EFI_SUCCESS;

FREE_ROOTHUB:
  if (RootIf != NULL) {
    FreePool (RootIf);
  }
  if (RootHub != NULL) {
    FreePool (RootHub);
  }

UNINSTALL_USBBUS:
  FreePool (UsbBus);

  USB_DEBUG ((EFI_D_ERROR, "UsbBusStart: Failed to start bus driver %r\n", Status));
  return Status;
}

EFI_USB_IO_PROTOCOL mUsbIoProtocol = {
    UsbIoControlTransfer,
    UsbIoBulkTransfer,
    UsbIoAsyncInterruptTransfer,
    UsbIoSyncInterruptTransfer,
    UsbIoIsochronousTransfer,
    UsbIoAsyncIsochronousTransfer,
    UsbIoGetDeviceDescriptor,
    UsbIoGetActiveConfigDescriptor,
    UsbIoGetInterfaceDescriptor,
    UsbIoGetEndpointDescriptor,
    UsbIoGetStringDescriptor,
    UsbIoGetSupportedLanguages,
    UsbIoPortReset,
    UsbIoAsyncInterruptOnlyTransfer
};

EFI_STATUS
EFIAPI
CleanupUsbBusInstance (
    IN USB_BUS     *Bus
)
/*++

Routine Description:

  Stop handle the controller by this USB bus driver

Arguments:

  This              - The USB bus driver binding protocol
  Controller        - The controller to release
  NumberOfChildren  - The child of USB bus that opened controller BY_CHILD
  ChildHandleBuffer - The array of child handle

Returns:

  EFI_SUCCESS       - The controller or children are stopped
  EFI_DEVICE_ERROR  - Failed to stop the driver

--*/
{
    USB_DEVICE            *RootHub;
    USB_INTERFACE         *RootIf;
    UINTN                 Index;
    EFI_STATUS            Status;

    Status  = EFI_SUCCESS;

    USB_DEBUG((EFI_D_INFO, "UsbBusStop: usb bus stopped on %x\n", Bus));

    //
    // Stop the root hub, then free all the devices
    //
    UsbHcSetState (Bus, EfiUsbHcStateHalt);

    RootHub = Bus->Devices[0];
    RootIf  = RootHub->Interfaces[0];

    mUsbRootHubApi.Release (RootIf);

    for (Index = 1; Index < USB_MAX_DEVICES; Index++) {
        if (Bus->Devices[Index] != NULL) {
            UsbRemoveDevice (Bus->Devices[Index]);
        }
    }

    FreePool   (RootIf);
    FreePool   (RootHub);
    Status = UsbBusFreeUsbDPList (&Bus->WantedUsbIoDPList);
    ASSERT (!EFI_ERROR (Status));
    FreePool (Bus);

    return Status;
}

EFI_STATUS
EFIAPI
ScanUsbBus (
    IN LIST_ENTRY    *NativeUsbIfList,
    IN UINTN          NativeUsbIfSize
)
{
    EFI_STATUS        Status;
    LIST_ENTRY        *Node;
    NATIVE_USBIF_HDR  *NativeUsbIf;
    LEGACY_USBIF_NODE *LegacyUsbIfNode;
    UINTN              Size;

    Node = mLegacyUsbInfList.ForwardLink;

    while (Node != &mLegacyUsbInfList) {
        LegacyUsbIfNode = _CR(Node, LEGACY_USBIF_NODE, List);
        Node = Node->ForwardLink;

        if (LegacyUsbIfNode->UsbIfState == NEW_INTERFACE) {
            Status = gBS->AllocatePool (
                         EfiReservedMemoryType,
                         NativeUsbIfSize,
                         &NativeUsbIf
                     );
            if (EFI_ERROR (Status)) {
              return EFI_SUCCESS;
            } else {
              DEBUG((EFI_D_ERROR, "ScanUsbBus.NativeUsbIf:%lX,S:%X\n", (UINT64)(UINTN)NativeUsbIf, NativeUsbIfSize));
            }
            NativeUsbIf->LegacyUsbIo = (VOID *)&LegacyUsbIfNode->LegacyUsbIf->UsbIo;
            NativeUsbIf->UsbIfState = NEW_INTERFACE;
            Size = GetDevicePathSize (LegacyUsbIfNode->LegacyUsbIf->DevicePath);
            if (Size != 0) {
                Status = gBS->AllocatePool (EfiBootServicesData, Size, &NativeUsbIf->DevicePath);
                if (!EFI_ERROR(Status)) {
                    CopyMem (
                        NativeUsbIf->DevicePath,
                        LegacyUsbIfNode->LegacyUsbIf->DevicePath,
                        Size);
                }
            }
            InsertTailList (NativeUsbIfList, &NativeUsbIf->List);

            LegacyUsbIfNode->NativeUsbIf = (VOID *)NativeUsbIf;
            LegacyUsbIfNode->UsbIfState = OLD_INTERFACE;
        }
    }

    return EFI_SUCCESS;
}

VOID
CreateLegacyUsbInfNode (
    USB_INTERFACE *UsbIf
)
{
    LEGACY_USBIF_NODE     *LegacyUsbIfNode;

    if (UsbIf->Type == UDT_HUB)
        return;

    LegacyUsbIfNode = AllocatePool (sizeof(LEGACY_USBIF_NODE));
    if (LegacyUsbIfNode == NULL) {
        UsbIf->LegacyUsbIfNode = NULL;
        return;
    }
    LegacyUsbIfNode->LegacyUsbIf = UsbIf;
    UsbIf->LegacyUsbIfNode = LegacyUsbIfNode;

    LegacyUsbIfNode->NativeUsbIf = NULL;
    LegacyUsbIfNode->UsbIfState  = NEW_INTERFACE;
    InsertTailList (&mLegacyUsbInfList, &LegacyUsbIfNode->List);
}

VOID
UpdateLegacyUsbInfNode (
    USB_INTERFACE *UsbIf,
    UINTN State
)
{
    LEGACY_USBIF_NODE     *LegacyUsbIfNode;
    NATIVE_USBIF_HDR      *NativeUsbIf;

    if (UsbIf->Type == UDT_HUB)
        return;

    LegacyUsbIfNode = UsbIf->LegacyUsbIfNode;
    if (LegacyUsbIfNode != NULL) {
        LegacyUsbIfNode->UsbIfState = State;
        if (State == EMPTY_INTERFACE) {
            NativeUsbIf = LegacyUsbIfNode->NativeUsbIf;
            if (NativeUsbIf != NULL) {
                NativeUsbIf->UsbIfState  = EMPTY_INTERFACE;
                NativeUsbIf->LegacyUsbIo = NULL;
            }
            RemoveEntryList (&LegacyUsbIfNode->List);
            FreePool (LegacyUsbIfNode);
        }
    }
}

