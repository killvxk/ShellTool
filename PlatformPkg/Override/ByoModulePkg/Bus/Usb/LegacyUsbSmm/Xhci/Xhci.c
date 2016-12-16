/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2009 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:
  Xhci.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "Xhci.h"
#include <IndustryStandard/pci22.h>
#include "../UsbLib/UsbUtility.h"
#include "../UsbLib/SmmPciIo.h"
#include <Library/IoLib.h>
#include <Library/ReportStatusCodeLib.h>

extern  LIST_ENTRY          mEhcList;
//
// Two arrays used to translate the XHCI port state (change)
// to the UEFI protocol's port state (change).
//
static
USB_PORT_STATE_MAP  mUsbPortStateMap[] = {
  {XHC_PORTSC_CCS,   USB_PORT_STAT_CONNECTION},
  {XHC_PORTSC_PED,   USB_PORT_STAT_ENABLE},
  {XHC_PORTSC_OCA,   USB_PORT_STAT_OVERCURRENT},
  {XHC_PORTSC_RESET, USB_PORT_STAT_RESET}
};

static
USB_PORT_STATE_MAP  mUsbPortChangeMap[] = {
  {XHC_PORTSC_CSC, USB_PORT_STAT_C_CONNECTION},
  {XHC_PORTSC_PEC, USB_PORT_STAT_C_ENABLE},
  {XHC_PORTSC_OCC, USB_PORT_STAT_C_OVERCURRENT},
  {XHC_PORTSC_PRC, USB_PORT_STAT_C_RESET}
};

USB_PORT_STATE_MAP  mUsbHubPortStateMap[] = {
  {XHC_HUB_PORTSC_CCS,   USB_PORT_STAT_CONNECTION},
  {XHC_HUB_PORTSC_PED,   USB_PORT_STAT_ENABLE},
  {XHC_HUB_PORTSC_OCA,   USB_PORT_STAT_OVERCURRENT},
  {XHC_HUB_PORTSC_RESET, USB_PORT_STAT_RESET}
};

USB_PORT_STATE_MAP  mUsbHubPortChangeMap[] = {
  {XHC_HUB_PORTSC_CSC, USB_PORT_STAT_C_CONNECTION},
  {XHC_HUB_PORTSC_PEC, USB_PORT_STAT_C_ENABLE},
  {XHC_HUB_PORTSC_OCC, USB_PORT_STAT_C_OVERCURRENT},
  {XHC_HUB_PORTSC_PRC, USB_PORT_STAT_C_RESET}
};

EFI_STATUS
EFIAPI
GetXhcInfo (
    IN EFI_PCI_FUNCTION_ADDRESS *PciAddress,
    IN VOID                     **SmmUsb2HcDev,
    IN VOID                     **SmmUsb2Hc
)
{
    LIST_ENTRY                  *Next;

    Next = mXhcList.BackLink;
    while (Next != &mXhcList) {
        USB_XHCI_INSTANCE          *Hc;
        Hc = CR(Next, USB_XHCI_INSTANCE, Link, XHCI_INSTANCE_SIG);
        if (Hc->PciAddress.Bus == PciAddress->Bus &&
                Hc->PciAddress.Device == PciAddress->Device &&
                Hc->PciAddress.Function == PciAddress->Function ) {

            *SmmUsb2HcDev = (VOID *)Hc;
            *SmmUsb2Hc = (VOID *)(&Hc->Usb2Hc);

            return EFI_SUCCESS;
        }
        Next = Next->BackLink;
    }

  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
XhcGetCapability (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT UINT8                 *MaxSpeed,
    OUT UINT8                 *PortNumber,
    OUT UINT8                 *Is64BitCapable
)
/*++

  Routine Description:
    Retrieves the capablility of root hub ports.

  Arguments:
    This            - This EFI_USB_HC_PROTOCOL instance.
    MaxSpeed        - Max speed supported by the controller
    PortNumber      - Number of the root hub ports.
    Is64BitCapable  - Whether the controller supports 64-bit memory addressing.

  Returns:
    EFI_SUCCESS           : host controller capability were retrieved successfully.
    EFI_INVALID_PARAMETER : Either of the three capability pointer is NULL

--*/
{
  USB_XHCI_INSTANCE  *Xhc;

  if ((MaxSpeed == NULL) || (PortNumber == NULL) || (Is64BitCapable == NULL)) {
    return EFI_INVALID_PARAMETER;
  }


  Xhc             = XHC_FROM_THIS (This);
  *MaxSpeed       = EFI_USB_SPEED_SUPER;
  *PortNumber     = (UINT8) (Xhc->HcSParams1.Data.MaxPorts);
  *Is64BitCapable = (UINT8) (Xhc->HcCParams.Data.Ac64);
  XHCI_DEBUG ((EFI_D_INFO, "XhcGetCapability: %d ports, 64 bit %d\n", *PortNumber, *Is64BitCapable));

  return EFI_SUCCESS;
}


/**
  Provides software reset for the USB host controller.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  Attributes            A bit mask of the reset operation to perform.

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.
  @retval EFI_UNSUPPOURTED      The type of reset specified by Attributes is
                                not currently supported by the host controller.
  @retval EFI_DEVICE_ERROR      Host controller isn't halted to reset.

**/
EFI_STATUS
EFIAPI
XhcReset (
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT16                Attributes
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  EFI_STATUS         Status;

  Xhc    = XHC_FROM_THIS (This);

    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_IO_BUS_USB | EFI_IOB_PC_RESET));
  switch (Attributes) {
  case EFI_USB_HC_RESET_GLOBAL:
  //
  // Flow through, same behavior as Host Controller Reset
  //
  case EFI_USB_HC_RESET_HOST_CONTROLLER:
    //
    // Host Controller must be Halt when Reset it
    //
    if (!XhcIsHalt (Xhc)) {
      Status = XhcHaltHC (Xhc, XHC_GENERIC_TIMEOUT);

      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto ON_EXIT;
      }
    }

    Status = XhcResetHC (Xhc, XHC_RESET_TIMEOUT);
    ASSERT (!(XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_CNR)));

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
    //
    // Clean up the asynchronous transfers, currently only
    // interrupt supports asynchronous operation.
    //
    XhciDelAllAsyncIntTransfers (Xhc);
    XhcFreeSched (Xhc);

    XhcInitSched (Xhc);
    break;

  case EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG:
  case EFI_USB_HC_RESET_HOST_WITH_DEBUG:
    Status = EFI_UNSUPPORTED;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  XHCI_DEBUG ((EFI_D_INFO, "XhcReset: status %r\n", Status));
  return Status;
}


/**
  Retrieve the current state of the USB host controller.

  @param  This                   This EFI_USB2_HC_PROTOCOL instance.
  @param  State                  Variable to return the current host controller
                                 state.

  @retval EFI_SUCCESS            Host controller state was returned in State.
  @retval EFI_INVALID_PARAMETER  State is NULL.
  @retval EFI_DEVICE_ERROR       An error was encountered while attempting to
                                 retrieve the host controller's current state.

**/
EFI_STATUS
EFIAPI
XhcGetState (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT EFI_USB_HC_STATE      *State
  )
{
  USB_XHCI_INSTANCE  *Xhc;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Xhc    = XHC_FROM_THIS (This);

  if (XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT)) {
    *State = EfiUsbHcStateHalt;
  } else {
    *State = EfiUsbHcStateOperational;
  }

  XHCI_DEBUG ((EFI_D_INFO, "XhcGetState: current state %d\n", *State));
  return EFI_SUCCESS;
}

/**
  Sets the USB host controller to a specific state.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  State                 The state of the host controller that will be set.

  @retval EFI_SUCCESS           The USB host controller was successfully placed
                                in the state specified by State.
  @retval EFI_INVALID_PARAMETER State is invalid.
  @retval EFI_DEVICE_ERROR      Failed to set the state due to device error.

**/
EFI_STATUS
EFIAPI
XhcSetState (
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN EFI_USB_HC_STATE      State
  )
{
  USB_XHCI_INSTANCE   *Xhc;
  EFI_STATUS          Status;
  EFI_USB_HC_STATE    CurState;

  Status = XhcGetState (This, &CurState);

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (CurState == State) {
    return EFI_SUCCESS;
  }

  Xhc    = XHC_FROM_THIS (This);

  switch (State) {
  case EfiUsbHcStateHalt:
    Status = XhcHaltHC (Xhc, XHC_GENERIC_TIMEOUT);
    break;

  case EfiUsbHcStateOperational:
    if (XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HSE)) {
      Status = EFI_DEVICE_ERROR;
      break;
    }

    //
    // Software must not write a one to this field unless the host controller
    // is in the Halted state. Doing so will yield undefined results.
    // refers to Spec[XHCI1.0-2.3.1]
    //
    if (!XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT)) {
      Status = EFI_DEVICE_ERROR;
      break;
    }

    Status = XhcRunHC (Xhc, XHC_GENERIC_TIMEOUT);
    break;

  case EfiUsbHcStateSuspend:
    Status = EFI_UNSUPPORTED;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

  XHCI_DEBUG ((EFI_D_INFO, "XhcSetState: status %r\n", Status));
    return Status;
}

/**
  Retrieves the current status of a USB root hub port.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            The root hub port to retrieve the state from.
                                This value is zero-based.
  @param  PortStatus            Variable to receive the port state.

  @retval EFI_SUCCESS           The status of the USB root hub port specified.
                                by PortNumber was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
XhcGetRootHubPortStatus (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  IN  UINT8                 PortNumber,
  OUT EFI_USB_PORT_STATUS   *PortStatus
  )
{
  USB_XHCI_INSTANCE       *Xhc;
    UINT32                  Offset;
    UINT32                  State;
    UINT32                  TotalPort;
    UINTN                   Index;
    UINTN                   MapSize;
    EFI_STATUS              Status;
  USB_DEV_ROUTE           ParentRouteChart;

  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Xhc       = XHC_FROM_THIS (This);
  Status    = EFI_SUCCESS;

  TotalPort = Xhc->HcSParams1.Data.MaxPorts;

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset                       = (UINT32) (XHC_PORTSC_OFFSET + (0x10 * PortNumber));
  PortStatus->PortStatus       = 0;
  PortStatus->PortChangeStatus = 0;

  State = XhcReadOpReg (Xhc, Offset);

  //
  // According to XHCI 1.0 spec, bit 10~13 of the root port status register identifies the speed of the attached device.
  //
  switch ((State & XHC_PORTSC_PS) >> 10) {
  case 2:
    PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
    break;

  case 3:
    PortStatus->PortStatus |= USB_PORT_STAT_HIGH_SPEED;
    break;

  case 4:
    PortStatus->PortStatus |= USB_PORT_STAT_SUPER_SPEED;
    break;

  default:
    break;
  }

  //
  // Convert the XHCI port/port change state to UEFI status
  //
  MapSize = sizeof (mUsbPortStateMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (XHC_BIT_IS_SET (State, mUsbPortStateMap[Index].HwState)) {
      PortStatus->PortStatus = (UINT16) (PortStatus->PortStatus | mUsbPortStateMap[Index].UefiState);
    }
  }
  //
  // Bit5~8 reflects its current link state.
  //
  if ((State & XHC_PORTSC_PLS) >> 5 == 3) {
    PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
  }

  MapSize = sizeof (mUsbPortChangeMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (XHC_BIT_IS_SET (State, mUsbPortChangeMap[Index].HwState)) {
      PortStatus->PortChangeStatus = (UINT16) (PortStatus->PortChangeStatus | mUsbPortChangeMap[Index].UefiState);
    }
  }

  //
  // Poll the root port status register to enable/disable corresponding device slot if there is a device attached/detached.
  // For those devices behind hub, we get its attach/detach event by hooking Get_Port_Status request at control transfer for those hub.
  //
  ParentRouteChart.Dword = 0;
  XhcPollPortStatusChange (Xhc, ParentRouteChart, PortNumber, PortStatus);

ON_EXIT:
  return Status;
}


/**
  Sets a feature for the specified root hub port.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            Root hub port to set.
  @param  PortFeature           Feature to set.

  @retval EFI_SUCCESS           The feature specified by PortFeature was set.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
XhcSetRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  )
{
  USB_XHCI_INSTANCE       *Xhc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  UINT8                   SlotId;
  USB_DEV_ROUTE           RouteChart;
  EFI_STATUS              Status;


  Xhc    = XHC_FROM_THIS (This);
  Status = EFI_SUCCESS;

  TotalPort = (Xhc->HcSParams1.Data.MaxPorts);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset = (UINT32) (XHC_PORTSC_OFFSET + (0x10 * PortNumber));
  State  = XhcReadOpReg (Xhc, Offset);

  //
  // Mask off the port status change bits, these bits are
  // write clean bit
  //
  State &= ~ (BIT1 | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22 | BIT23);

  switch (PortFeature) {
  case EfiUsbPortEnable:
    //
    // Ports may only be enabled by the xHC. Software cannot enable a port by writing a '1' to this flag.
    // A port may be disabled by software writing a '1' to this flag.
    //
    Status = EFI_SUCCESS;
    break;

  case EfiUsbPortSuspend:
    State |= XHC_PORTSC_LWS;
    XhcWriteOpReg (Xhc, Offset, State);
    State &= ~XHC_PORTSC_PLS;
    State |= (3 << 5) ;
    XhcWriteOpReg (Xhc, Offset, State);
    break;

  case EfiUsbPortReset:
    XHCI_DEBUG ((EFI_D_INFO, "XhcUsbPortReset!\n"));
    //
    // Make sure Host Controller not halt before reset it
    //
    if (XhcIsHalt (Xhc)) {
      Status = XhcRunHC (Xhc, XHC_GENERIC_TIMEOUT);

      if (EFI_ERROR (Status)) {
        XHCI_DEBUG ((EFI_D_INFO, "XhcSetRootHubPortFeature :failed to start HC - %r\n", Status));
        break;
      }
    }

    RouteChart.Route.RouteString = 0;
    RouteChart.Route.RootPortNum = PortNumber + 1;
    RouteChart.Route.TierNum     = 1;
    //
    // If the port reset operation happens after the usb super speed device is enabled,
    // The subsequent configuration, such as getting device descriptor, will fail.
    // So here a workaround is introduced to skip the reset operation if the device is enabled.
    //
    SlotId = XhcRouteStringToSlotId (Xhc, RouteChart);
    if (SlotId == 0) {
      //
      // 4.3.1 Resetting a Root Hub Port
      // 1) Write the PORTSC register with the Port Reset (PR) bit set to '1'.
      //
      State |= XHC_PORTSC_RESET;
      XhcWriteOpReg (Xhc, Offset, State);
      XhcWaitOpRegBit(Xhc, Offset, XHC_PORTSC_PRC, TRUE, XHC_GENERIC_TIMEOUT);
    }
    break;

  case EfiUsbPortPower:
    //
    // Not supported, ignore the operation
    //
    Status = EFI_SUCCESS;
    break;

  case EfiUsbPortOwner:
    //
    // XHCI root hub port don't has the owner bit, ignore the operation
    //
    Status = EFI_SUCCESS;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  XHCI_DEBUG ((EFI_D_INFO, "XhcSetRootHubPortFeature: status %r\n", Status));
  return Status;
}


/**
  Clears a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            Specifies the root hub port whose feature is
                                requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was cleared
                                for the USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
XhcClearRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  )
{
  USB_XHCI_INSTANCE       *Xhc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  EFI_STATUS              Status;


  Xhc       = XHC_FROM_THIS (This);
  Status    = EFI_SUCCESS;

  TotalPort = (Xhc->HcSParams1.Data.MaxPorts);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset = XHC_PORTSC_OFFSET + (0x10 * PortNumber);

  //
  // Mask off the port status change bits, these bits are
  // write clean bit
  //
  State  = XhcReadOpReg (Xhc, Offset);
  State &= ~ (BIT1 | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22 | BIT23);

  switch (PortFeature) {
  case EfiUsbPortEnable:
    //
    // Ports may only be enabled by the xHC. Software cannot enable a port by writing a '1' to this flag.
    // A port may be disabled by software writing a '1' to this flag.
    //
    State |= XHC_PORTSC_PED;
    State &= ~XHC_PORTSC_RESET;
    XhcWriteOpReg (Xhc, Offset, State);
    break;

  case EfiUsbPortSuspend:
    State |= XHC_PORTSC_LWS;
    XhcWriteOpReg (Xhc, Offset, State);
    State &= ~XHC_PORTSC_PLS;
    XhcWriteOpReg (Xhc, Offset, State);
    break;

  case EfiUsbPortReset:
    //
    // PORTSC_RESET BIT(4) bit is RW1S attribute, which means Write-1-to-set status:
    // Register bits indicate status when read, a clear bit may be set by
    // writing a '1'. Writing a '0' to RW1S bits has no effect.
    //
    break;

  case EfiUsbPortOwner:
    //
    // XHCI root hub port don't has the owner bit, ignore the operation
    //
    break;

  case EfiUsbPortConnectChange:
    //
    // Clear connect status change
    //
    State |= XHC_PORTSC_CSC | XHC_PORTSC_PLC | XHC_PORTSC_WRC | XHC_PORTSC_CEC; //Add XHC_PORTSC_PLC and XHC_PORTSC_WRC For BUG
    XhcWriteOpReg (Xhc, Offset, State);
    break;

  case EfiUsbPortEnableChange:
    //
    // Clear enable status change
    //
    State |= XHC_PORTSC_PEC;
    XhcWriteOpReg (Xhc, Offset, State);
    break;

  case EfiUsbPortOverCurrentChange:
    //
    // Clear PortOverCurrent change
    //
    State |= XHC_PORTSC_OCC;
    XhcWriteOpReg (Xhc, Offset, State);
    break;

  case EfiUsbPortResetChange:
    //
    // Clear Port Reset change
    //
    State |= XHC_PORTSC_PRC;
    XhcWriteOpReg (Xhc, Offset, State);
    break;

  case EfiUsbPortPower:
  case EfiUsbPortSuspendChange:
    //
    // Not supported or not related operation
    //
    break;

    default:
        Status = EFI_INVALID_PARAMETER;
        break;
    }

ON_EXIT:
  XHCI_DEBUG ((EFI_D_INFO, "XhcClearRootHubPortFeature: status %r\n", Status));
  return Status;
}


/**
  Submits control transfer to a target USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         The target device address.
  @param  DeviceSpeed           Target device speed.
  @param  MaximumPacketLength   Maximum packet size the default control transfer
                                endpoint is capable of sending or receiving.
  @param  Request               USB device request to send.
  @param  TransferDirection     Specifies the data direction for the data stage
  @param  Data                  Data buffer to be transmitted or received from USB
                                device.
  @param  DataLength            The size (in bytes) of the data buffer.
  @param  Timeout               Indicates the maximum timeout, in millisecond.
  @param  Translator            Transaction translator to be used by this device.
  @param  TransferResult        Return the result of this control transfer.

  @retval EFI_SUCCESS           Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
XhcControlTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST              *Request,
  IN     EFI_USB_DATA_DIRECTION              TransferDirection,
  IN OUT VOID                                *Data,
  IN OUT UINTN                               *DataLength,
  IN     UINTN                               Timeout,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  USB_XHCI_INSTANCE       *Xhc;
  URB                     *Urb;
  UINT8                   Endpoint;
  UINT8                   Index;
  UINT8                   DescriptorType;
  UINT8                   SlotId;
  UINT8                   TTT;
  UINT8                   MTT;
  UINT32                  MaxPacket0;
  EFI_USB_HUB_DESCRIPTOR  *HubDesc;
  EFI_STATUS              Status;
  EFI_STATUS              RecoveryStatus;
  UINTN                   MapSize;
  EFI_USB_PORT_STATUS     PortStatus;
  UINT32                  State;

  //
  // Validate parameters
  //
  if ((Request == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbDataIn) &&
      (TransferDirection != EfiUsbDataOut) &&
      (TransferDirection != EfiUsbNoData)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection == EfiUsbNoData) &&
      ((Data != NULL) || (*DataLength != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbNoData) &&
     ((Data == NULL) || (*DataLength == 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MaximumPacketLength != 8)  && (MaximumPacketLength != 16) &&
      (MaximumPacketLength != 32) && (MaximumPacketLength != 64) &&
      (MaximumPacketLength != 512)
      ) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_SUPER) && (MaximumPacketLength != 512)) {
    return EFI_INVALID_PARAMETER;
  }

  Xhc             = XHC_FROM_THIS (This);

  Status          = EFI_DEVICE_ERROR;
  *TransferResult = EFI_USB_ERR_SYSTEM;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcControlTransfer: HC halted at entrance\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = XhcBusDevAddrToSlotId (Xhc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  //
  // Hook the Set_Address request from UsbBus.
  // According to XHCI 1.0 spec, the Set_Address request is replaced by XHCI's Address_Device cmd.
  //
  if ((Request->Request     == USB_REQ_SET_ADDRESS) &&
      (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE))) {
    //
    // Reset the BusDevAddr field of all disabled entries in UsbDevContext array firstly.
    // This way is used to clean the history to avoid using wrong device address by XhcAsyncInterruptTransfer().
    //
    for (Index = 0; Index < MaxUsbDevContext; Index++) {
      if (!Xhc->UsbDevContext[Index + 1].Enabled &&
          (Xhc->UsbDevContext[Index + 1].SlotId == 0) &&
          (Xhc->UsbDevContext[Index + 1].BusDevAddr == (UINT8)Request->Value)) {
        Xhc->UsbDevContext[Index + 1].BusDevAddr = 0;
      }
    }
    //
    // The actual device address has been assigned by XHCI during initializing the device slot.
    // So we just need establish the mapping relationship between the device address requested from UsbBus
    // and the actual device address assigned by XHCI. The the following invocations through EFI_USB2_HC_PROTOCOL interface
    // can find out the actual device address by it.
    //
    Xhc->UsbDevContext[SlotId].BusDevAddr = (UINT8)Request->Value;
    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }
  
  //
  // If the port reset operation happens after the usb super speed device is enabled,
  // The subsequent configuration, such as getting device descriptor, will fail.
  // So here a workaround is introduced to skip the reset operation if the device is enabled.
  //
  if ((Request->Request     == USB_REQ_SET_FEATURE) &&
      (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_CLASS, USB_TARGET_OTHER)) &&
      (Request->Value       == EfiUsbPortReset)) {
    if (DeviceSpeed == EFI_USB_SPEED_SUPER) {
      Status = EFI_SUCCESS;
      goto ON_EXIT;
    }
  }

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  // Note that we encode the direction in address although default control
  // endpoint is bidirectional. XhcCreateUrb expects this
  // combination of Ep addr and its direction.
  //
  Endpoint = (UINT8) (0 | ((TransferDirection == EfiUsbDataIn) ? 0x80 : 0));
  Urb = XhcCreateUrb (
          Xhc,
          DeviceAddress,
          Endpoint,
          DeviceSpeed,
          MaximumPacketLength,
          XHC_CTRL_TRANSFER,
          Request,
          Data,
          *DataLength,
          NULL,
          NULL
          );

  if (Urb == NULL) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcControlTransfer: failed to create URB"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = XhcExecTransfer (Xhc, FALSE, Urb, Timeout);

  //
  // Get the status from URB. The result is updated in XhcCheckUrbResult
  // which is called by XhcExecTransfer
  //
  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  } else if (*TransferResult == EFI_USB_ERR_STALL) {
    RecoveryStatus = XhcRecoverHaltedEndpoint(Xhc, Urb);
    ASSERT_EFI_ERROR (RecoveryStatus);
    Status = EFI_DEVICE_ERROR;
    goto FREE_URB;
  } else {
    goto FREE_URB;
  }

  //
  // Hook Get_Descriptor request from UsbBus as we need evaluate context and configure endpoint.
  // Hook Get_Status request form UsbBus as we need trace device attach/detach event happened at hub.
  // Hook Set_Config request from UsbBus as we need configure device endpoint.
  //
  if ((Request->Request     == USB_REQ_GET_DESCRIPTOR) &&
      ((Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE)) || 
      ((Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_CLASS, USB_TARGET_DEVICE))))) {
    DescriptorType = (UINT8)(Request->Value >> 8);
    if ((DescriptorType == USB_DESC_TYPE_DEVICE) && (*DataLength == sizeof (EFI_USB_DEVICE_DESCRIPTOR))) {
        ASSERT (Data != NULL);
        //
        // Store a copy of device scriptor as hub device need this info to configure endpoint.
        //
        CopyMem (&Xhc->UsbDevContext[SlotId].DevDesc, Data, *DataLength);
        if (Xhc->UsbDevContext[SlotId].DevDesc.BcdUSB == 0x0300) {
          //
          // If it's a usb3.0 device, then its max packet size is a 2^n.
          //
          MaxPacket0 = 1 << Xhc->UsbDevContext[SlotId].DevDesc.MaxPacketSize0;
        } else {
          MaxPacket0 = Xhc->UsbDevContext[SlotId].DevDesc.MaxPacketSize0;
        }
        Xhc->UsbDevContext[SlotId].ConfDesc = AllocateZeroPool (Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations * sizeof (EFI_USB_CONFIG_DESCRIPTOR *));
        if (Xhc->HcCParams.Data.Csz == 0) {
          Status = XhcEvaluateContext (Xhc, SlotId, MaxPacket0);
        } else {
          Status = XhcEvaluateContext64 (Xhc, SlotId, MaxPacket0);
        }
        ASSERT_EFI_ERROR (Status);
    } else if (DescriptorType == USB_DESC_TYPE_CONFIG) {
      ASSERT (Data != NULL);
      if (*DataLength == ((UINT16 *)Data)[1]) {
        //
        // Get configuration value from request, Store the configuration descriptor for Configure_Endpoint cmd.
        //
        Index = (UINT8)Request->Value;
        ASSERT (Index < Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations);
        Xhc->UsbDevContext[SlotId].ConfDesc[Index] = AllocateZeroPool(*DataLength);
        CopyMem (Xhc->UsbDevContext[SlotId].ConfDesc[Index], Data, *DataLength);
      }
    } else if (((DescriptorType == USB_DESC_TYPE_HUB) ||
               (DescriptorType == USB_DESC_TYPE_HUB_SUPER_SPEED)) && (*DataLength > 2)) {
      ASSERT (Data != NULL);
      HubDesc = (EFI_USB_HUB_DESCRIPTOR *)Data;
      ASSERT (HubDesc->NumPorts <= 15);
      //
      // The bit 5,6 of HubCharacter field of Hub Descriptor is TTT.
      //
      TTT = (UINT8)((HubDesc->HubCharacter & (BIT5 | BIT6)) >> 5);
      if (Xhc->UsbDevContext[SlotId].DevDesc.DeviceProtocol == 2) {
        //
        // Don't support multi-TT feature for super speed hub now.
        //
        MTT = 0;
        XHCI_DEBUG ((EFI_D_ERROR, "XHCI: Don't support multi-TT feature for Hub now. (force to disable MTT)\n"));
      } else {
        MTT = 0;
      }

      if (Xhc->HcCParams.Data.Csz == 0) {
        Status = XhcConfigHubContext (Xhc, SlotId, HubDesc->NumPorts, TTT, MTT);
      } else {
        Status = XhcConfigHubContext64 (Xhc, SlotId, HubDesc->NumPorts, TTT, MTT);
      }
      ASSERT_EFI_ERROR (Status);
    }
  } else if ((Request->Request     == USB_REQ_SET_CONFIG) &&
             (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE))) {
    //
    // Hook Set_Config request from UsbBus as we need configure device endpoint.
    //
    for (Index = 0; Index < Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations; Index++) {
      if (Xhc->UsbDevContext[SlotId].ConfDesc[Index]->ConfigurationValue == (UINT8)Request->Value) {
        if (Xhc->HcCParams.Data.Csz == 0) {
          Status = XhcSetConfigCmd (Xhc, SlotId, DeviceSpeed, Xhc->UsbDevContext[SlotId].ConfDesc[Index]);
        } else {
          Status = XhcSetConfigCmd64 (Xhc, SlotId, DeviceSpeed, Xhc->UsbDevContext[SlotId].ConfDesc[Index]);
        }
        ASSERT_EFI_ERROR (Status);
        break;
      }
    }
  } else if ((Request->Request     == USB_REQ_GET_STATUS) &&
             (Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_CLASS, USB_TARGET_OTHER))) {
    ASSERT (Data != NULL);
    //
    // Hook Get_Status request from UsbBus to keep track of the port status change.
    //
    State                       = *(UINT32 *)Data;
    PortStatus.PortStatus       = 0;
    PortStatus.PortChangeStatus = 0;

    if (DeviceSpeed == EFI_USB_SPEED_SUPER) {
      //
      // For super speed hub, its bit10~12 presents the attached device speed.
      //
      if ((State & XHC_PORTSC_PS) >> 10 == 0) {
        PortStatus.PortStatus |= USB_PORT_STAT_SUPER_SPEED;
      }
    } else if (DeviceSpeed == EFI_USB_SPEED_HIGH) {
      //
      // For high speed hub, its bit9~10 presents the attached device speed.
      //
      if (XHC_BIT_IS_SET (State, BIT9)) {
        PortStatus.PortStatus |= USB_PORT_STAT_LOW_SPEED;
      } else if (XHC_BIT_IS_SET (State, BIT10)) {
        PortStatus.PortStatus |= USB_PORT_STAT_HIGH_SPEED;
      }
    } else {
      ASSERT (0);
    }

    //
    // Convert the XHCI port/port change state to UEFI status
    //
    MapSize = sizeof (mUsbHubPortStateMap) / sizeof (USB_PORT_STATE_MAP);
    for (Index = 0; Index < MapSize; Index++) {
      if (XHC_BIT_IS_SET (State, mUsbHubPortStateMap[Index].HwState)) {
        PortStatus.PortStatus = (UINT16) (PortStatus.PortStatus | mUsbHubPortStateMap[Index].UefiState);
      }
    }

    MapSize = sizeof (mUsbHubPortChangeMap) / sizeof (USB_PORT_STATE_MAP);
    for (Index = 0; Index < MapSize; Index++) {
      if (XHC_BIT_IS_SET (State, mUsbHubPortChangeMap[Index].HwState)) {
        PortStatus.PortChangeStatus = (UINT16) (PortStatus.PortChangeStatus | mUsbHubPortChangeMap[Index].UefiState);
      }
    }

    XhcPollPortStatusChange (Xhc, Xhc->UsbDevContext[SlotId].RouteString, (UINT8)Request->Index, &PortStatus);

    *(UINT32 *)Data = *(UINT32*)&PortStatus;
  }

FREE_URB:
  FreePool (Urb);

ON_EXIT:

  if (EFI_ERROR (Status)) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcControlTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }

  return Status;
}


/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction in bit 7.
  @param  DeviceSpeed           Device speed, Low speed device doesn't support bulk
                                transfer.
  @param  MaximumPacketLength   Maximum packet size the endpoint is capable of
                                sending or receiving.
  @param  DataBuffersNumber     Number of data buffers prepared for the transfer.
  @param  Data                  Array of pointers to the buffers of data to transmit
                                from or receive into.
  @param  DataLength            The lenght of the data buffer.
  @param  DataToggle            On input, the initial data toggle for the transfer;
                                On output, it is updated to to next data toggle to
                                use of the subsequent bulk transfer.
  @param  Timeout               Indicates the maximum time, in millisecond, which
                                the transfer is allowed to complete.
  @param  Translator            A pointr to the transaction translator data.
  @param  TransferResult        A pointer to the detailed result information of the
                                bulk transfer.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
XhcBulkTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN OUT UINTN                               *DataLength,
  IN OUT UINT8                               *DataToggle,
  IN     UINTN                               Timeout,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  USB_XHCI_INSTANCE       *Xhc;
  URB                     *Urb;
  UINT8                   SlotId;
  EFI_STATUS              Status;
  EFI_STATUS              RecoveryStatus;

  //
  // Validate the parameters
  //
  if ((DataLength == NULL) || (*DataLength == 0) ||
      (Data == NULL) || (Data[0] == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 0) && (*DataToggle != 1)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) ||
      ((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
      ((EFI_USB_SPEED_HIGH == DeviceSpeed) && (MaximumPacketLength > 512)) ||
      ((EFI_USB_SPEED_SUPER == DeviceSpeed) && (MaximumPacketLength > 1024))) {
    return EFI_INVALID_PARAMETER;
  }

  Xhc             = XHC_FROM_THIS (This);

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcBulkTransfer: HC is halted\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = XhcBusDevAddrToSlotId (Xhc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  Urb = XhcCreateUrb (
          Xhc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          MaximumPacketLength,
          XHC_BULK_TRANSFER,
          NULL,
          Data[0],
          *DataLength,
          NULL,
          NULL
          );

  if (Urb == NULL) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcBulkTransfer: failed to create URB\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = XhcExecTransfer (Xhc, FALSE, Urb, Timeout);

  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  } else if (*TransferResult == EFI_USB_ERR_STALL) {
    RecoveryStatus = XhcRecoverHaltedEndpoint(Xhc, Urb);
    ASSERT_EFI_ERROR (RecoveryStatus);
    Status = EFI_DEVICE_ERROR;
  }

  FreePool (Urb);

ON_EXIT:

  if (EFI_ERROR (Status)) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcBulkTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }

  return Status;
}

EFI_STATUS
EFIAPI
XhcAsyncInterruptOnlyTransfer (
    IN  EFI_USB2_HC_PROTOCOL                  * This,
    IN  BOOLEAN                               IsNewTransfer,
    IN  UINTN                                 PollingInterval,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
    IN  VOID                                  *Context
)
/*++

  Routine Description:

    Submits an asynchronous interrupt transfer to an
    interrupt endpoint of a USB device.

  Arguments:
    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Target device address
    EndPointAddress     - Endpoint number and its direction encoded in bit 7
    DeviceSpeed         - Indicates device speed.
    MaximumPacketLength - Maximum packet size the target endpoint is capable
    IsNewTransfer       - If TRUE, to submit an new asynchronous interrupt transfer
                          If FALSE, to remove the specified asynchronous interrupt
    DataToggle          - On input, the initial data toggle to use;
                          on output, it is updated to indicate the next data toggle
    PollingInterval     - The he interval, in milliseconds, that the transfer is polled.
    DataLength          - The length of data to receive at the rate specified by
                          PollingInterval.
    Translator          - Transaction translator to use.
    CallBackFunction    - Function to call at the rate specified by PollingInterval
    Context             - Context to CallBackFunction.

  Returns:

    EFI_SUCCESS           : The request has been successfully submitted or canceled.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_OUT_OF_RESOURCES  : The request failed due to a lack of resources.
    EFI_DEVICE_ERROR      : The transfer failed due to host controller error.

--*/
{
      USB_XHCI_INSTANCE       *Xhc;
      URB                     *Urb;
      EFI_STATUS               Status;

      //
      // Validate parameters
      //
     if (CallBackFunction == NULL) {
        XHCI_DEBUG ((EFI_D_ERROR, "XhcAsyncInterruptOnlyTransfer: CallBackFunction is NULL\n"));
        return EFI_INVALID_PARAMETER;
     }

     if (IsNewTransfer) {
       if ((PollingInterval > 255) || (PollingInterval < 1)) {
         return EFI_INVALID_PARAMETER;
        }
     }

      Xhc    = XHC_FROM_THIS (This);
 
      //
      // Delete Async interrupt transfer request.
      //
     if (!IsNewTransfer) {
        Status = XhciDelAsyncIntOnlyTransfer (Xhc, 0xFF, 0xFF, 0xFF, XHC_INT_ONLY_TRANSFER_ASYNC,CallBackFunction,Context);
        //XHCI_DEBUG ((EFI_D_INFO, "XhcAsyncInterruptOnlyTransfer: remove old transfer.Status = %r\n",Status));
        goto ON_EXIT;
      }

      Status = EFI_SUCCESS;

      if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
        XHCI_DEBUG ((EFI_D_ERROR, "XhcAsyncInterruptOnlyTransfer: HC is halt\n"));
        Status = EFI_DEVICE_ERROR;
      }

      Urb = XhcCreateAsyncInterruptOnlyUrb (
              Xhc,
              0xFF,
              0xFF,
              0xFF,
              XHC_INT_ONLY_TRANSFER_ASYNC,
              CallBackFunction,
              Context
              );

     if (Urb == NULL) {
        XHCI_DEBUG ((EFI_D_ERROR, "XhcAsyncInterruptOnlyTransfer: failed to create URB\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }
      if(PollingInterval<0x10) PollingInterval = 0x10;
      Urb->DelayRecord = (UINT32)(PollingInterval/0x10);
      Urb->DelayCount = Urb->DelayRecord;

      InsertHeadList (&Xhc->AsyncIntOnlyTransfers, &Urb->UrbList);

     ON_EXIT:
 
     return Status;

}


/**
  Submits an asynchronous interrupt transfer to an
  interrupt endpoint of a USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction encoded in bit 7
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Maximum packet size the target endpoint is capable
  @param  IsNewTransfer         If TRUE, to submit an new asynchronous interrupt
                                transfer If FALSE, to remove the specified
                                asynchronous interrupt.
  @param  DataToggle            On input, the initial data toggle to use; on output,
                                it is updated to indicate the next data toggle.
  @param  PollingInterval       The he interval, in milliseconds, that the transfer
                                is polled.
  @param  DataLength            The length of data to receive at the rate specified
                                by  PollingInterval.
  @param  Translator            Transaction translator to use.
  @param  CallBackFunction      Function to call at the rate specified by
                                PollingInterval.
  @param  Context               Context to CallBackFunction.

  @retval EFI_SUCCESS           The request has been successfully submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request failed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
XhcAsyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     BOOLEAN                             IsNewTransfer,
  IN OUT UINT8                               *DataToggle,
  IN     UINTN                               PollingInterval,
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK     CallBackFunction,
  IN     VOID                                *Context OPTIONAL
  )
{
  USB_XHCI_INSTANCE       *Xhc;
  URB                     *Urb;
  EFI_STATUS              Status;
  UINT8                   SlotId;
  UINT8                   Index;
  UINT8                   *Data;

  //
  // Validate parameters
  //
  if (!XHCI_IS_DATAIN (EndPointAddress)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsNewTransfer) {
    if (DataLength == 0) {
      return EFI_INVALID_PARAMETER;
    }

    if ((*DataToggle != 1) && (*DataToggle != 0)) {
      return EFI_INVALID_PARAMETER;
    }

    if ((PollingInterval > 255) || (PollingInterval < 1)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Xhc    = XHC_FROM_THIS (This);

  //
  // Delete Async interrupt transfer request.
  //
  if (!IsNewTransfer) {
    //
    // The delete request may happen after device is detached.
    //
    for (Index = 0; Index < MaxUsbDevContext; Index++) {
      if (Xhc->UsbDevContext[Index + 1].BusDevAddr == DeviceAddress) {
        break;
      }
    }

    if (Index == MaxUsbDevContext) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    Status = XhciDelAsyncIntTransfer (Xhc, DeviceAddress, EndPointAddress);
    XHCI_DEBUG ((EFI_D_INFO, "XhcAsyncInterruptTransfer: remove old transfer for addr %d, Status = %r\n", DeviceAddress, Status));
    goto ON_EXIT;
  }

  Status = EFI_SUCCESS;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcAsyncInterruptTransfer: HC is halt\n"));
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = XhcBusDevAddrToSlotId (Xhc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  Data = UsbAllocatePool (DataLength);

  if (Data == NULL) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcAsyncInterruptTransfer: failed to allocate buffer\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Urb = XhcCreateUrb (
          Xhc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          MaximumPacketLength,
          XHC_INT_TRANSFER_ASYNC,
          NULL,
          Data,
          DataLength,
          CallBackFunction,
          Context
          );

  if (Urb == NULL) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcAsyncInterruptTransfer: failed to create URB\n"));
    UsbFreePool (Data, DataLength);
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  InsertHeadList (&Xhc->AsyncIntTransfers, &Urb->UrbList);
  //
  // Ring the doorbell
  //
  Status = RingIntTransferDoorBell (Xhc, Urb);

ON_EXIT:

  return Status;
}


/**
  Submits synchronous interrupt transfer to an interrupt endpoint
  of a USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction encoded in bit 7
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Maximum packet size the target endpoint is capable
                                of sending or receiving.
  @param  Data                  Buffer of data that will be transmitted to  USB
                                device or received from USB device.
  @param  DataLength            On input, the size, in bytes, of the data buffer; On
                                output, the number of bytes transferred.
  @param  DataToggle            On input, the initial data toggle to use; on output,
                                it is updated to indicate the next data toggle.
  @param  Timeout               Maximum time, in second, to complete.
  @param  Translator            Transaction translator to use.
  @param  TransferResult        Variable to receive the transfer result.

  @return EFI_SUCCESS           The transfer was completed successfully.
  @return EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @return EFI_INVALID_PARAMETER Some parameters are invalid.
  @return EFI_TIMEOUT           The transfer failed due to timeout.
  @return EFI_DEVICE_ERROR      The failed due to host controller or device error

**/
EFI_STATUS
EFIAPI
XhcSyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN OUT VOID                                *Data,
  IN OUT UINTN                               *DataLength,
  IN OUT UINT8                               *DataToggle,
  IN     UINTN                               Timeout,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  USB_XHCI_INSTANCE       *Xhc;
  URB                     *Urb;
  UINT8                   SlotId;
  EFI_STATUS              Status;
  EFI_STATUS              RecoveryStatus;

  //
  // Validates parameters
  //
  if ((DataLength == NULL) || (*DataLength == 0) ||
      (Data == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!XHCI_IS_DATAIN (EndPointAddress)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 1) && (*DataToggle != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8))  ||
      ((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
      ((DeviceSpeed == EFI_USB_SPEED_HIGH) && (MaximumPacketLength > 3072))) {
    return EFI_INVALID_PARAMETER;
  }


  Xhc     = XHC_FROM_THIS (This);

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    XHCI_DEBUG ((EFI_D_ERROR, "EhcSyncInterruptTransfer: HC is halt\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = XhcBusDevAddrToSlotId (Xhc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  Urb = XhcCreateUrb (
          Xhc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          MaximumPacketLength,
          XHC_INT_TRANSFER_SYNC,
          NULL,
          Data,
          *DataLength,
          NULL,
          NULL
          );

  if (Urb == NULL) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcSyncInterruptTransfer: failed to create URB\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = XhcExecTransfer (Xhc, FALSE, Urb, Timeout);

  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  } else if (*TransferResult == EFI_USB_ERR_STALL) {
    RecoveryStatus = XhcRecoverHaltedEndpoint(Xhc, Urb);
    ASSERT_EFI_ERROR (RecoveryStatus);
    Status = EFI_DEVICE_ERROR;
  }

  FreePool (Urb);

ON_EXIT:
  if (EFI_ERROR (Status)) {
    XHCI_DEBUG ((EFI_D_ERROR, "XhcSyncInterruptTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }

  return Status;
}


/**
  Submits isochronous transfer to a target USB device.

  @param  This                 This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress        Target device address.
  @param  EndPointAddress      End point address with its direction.
  @param  DeviceSpeed          Device speed, Low speed device doesn't support this
                               type.
  @param  MaximumPacketLength  Maximum packet size that the endpoint is capable of
                               sending or receiving.
  @param  DataBuffersNumber    Number of data buffers prepared for the transfer.
  @param  Data                 Array of pointers to the buffers of data that will
                               be transmitted to USB device or received from USB
                               device.
  @param  DataLength           The size, in bytes, of the data buffer.
  @param  Translator           Transaction translator to use.
  @param  TransferResult       Variable to receive the transfer result.

  @return EFI_UNSUPPORTED      Isochronous transfer is unsupported.

**/
EFI_STATUS
EFIAPI
XhcIsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Submits Async isochronous transfer to a target USB device.

  @param  This                 This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress        Target device address.
  @param  EndPointAddress      End point address with its direction.
  @param  DeviceSpeed          Device speed, Low speed device doesn't support this
                               type.
  @param  MaximumPacketLength  Maximum packet size that the endpoint is capable of
                               sending or receiving.
  @param  DataBuffersNumber    Number of data buffers prepared for the transfer.
  @param  Data                 Array of pointers to the buffers of data that will
                               be transmitted to USB device or received from USB
                               device.
  @param  DataLength           The size, in bytes, of the data buffer.
  @param  Translator           Transaction translator to use.
  @param  IsochronousCallBack  Function to be called when the transfer complete.
  @param  Context              Context passed to the call back function as
                               parameter.

  @return EFI_UNSUPPORTED      Isochronous transfer isn't supported.

**/
EFI_STATUS
EFIAPI
XhcAsyncIsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN     VOID                                *Context
  )
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
XhcDisableLegacySupport (
  IN  EFI_USB2_HC_PROTOCOL  *This
)
{
  USB_XHCI_INSTANCE        *Xhc;
  UINT32                   Buffer;

  Xhc =  XHC_FROM_THIS (This);

  //
  // Clear USBINTR to disable all the interrupt.
  //
  XhcClearOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_INTE);
  XhcClearRuntimeRegBit (Xhc, XHC_IMAN_OFFSET, XHC_IMAN_IE);
  //
  // clear status
  //
  Buffer = 0;
  XhcWriteExtCapReg (Xhc, Xhc->UsbLegSupOffset + 4, Buffer);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
XhcEnableLegacySupport (
  IN EFI_USB2_HC_PROTOCOL  *This
)
{
  USB_XHCI_INSTANCE        *Xhc;
  UINT32                   Buffer;

  Xhc =  XHC_FROM_THIS(This);

  //
  // Enable USBINTR to enable all the interrupt.
  //

  XhcSetRuntimeRegBit (Xhc, XHC_IMAN_OFFSET, XHC_IMAN_IE);
  XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_INTE|XHC_USBCMD_HSEE);

  //
  // 2. Enable the IE bit of  the IMAN register in the IR[xx]
  //
  //may be implemente at  the EventRing Allocate point

  //
  // 3.Pending How to ensure the port change event can generate SMI
  //

  //
  // 4.Enablet the SMI generation (Port change may be one source of the IOC caused by port change event )
  //
  Buffer = USBLEGCTLSTS_OWNERSHIP_ENABLE |
           USBLEGCTLSTS_IOC_ENABLE ;
  XhcWriteExtCapReg (Xhc, Xhc->UsbLegSupOffset + 4, Buffer);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
XhcLegacyCallback(
    IN       EFI_USB2_HC_PROTOCOL                 * This
)
{
    USB_XHCI_INSTANCE        *Xhc;
    UINT32                   UsbSts;
    UINT32                   LegacyCap;
    UINT32                   LegacyCtlSts;

    Xhc =  XHC_FROM_THIS(This);

    //
    //xHC process Hot Plug
    //
    UsbSts = XhcReadOpReg(Xhc, XHC_USBSTS_OFFSET);
    //Clear the status ,may be risk when primary SMI exist. here only care the PortChangeDetect!
    XhcWriteOpReg (Xhc, XHC_USBSTS_OFFSET, UsbSts);
    if ((UsbSts & USBSTS_PORT_CHANGE) &&
        (This->BusEnumerationCallback != NULL) &&
        (This->EnumerationContext != NULL)) {
        This->BusEnumerationCallback (This->EnumerationContext);
    }
    //
    //xHC process AsynIntEvent
    //
    XhcProcessAsyncRequests (Xhc);
    //
    //XHCI process the ownership handoff
    //
    LegacyCap = XhcReadExtCapReg (Xhc, Xhc->UsbLegSupOffset);
    LegacyCtlSts = XhcReadExtCapReg (Xhc, Xhc->UsbLegSupOffset + 4);;
    if (!(LegacyCap & USBLEGSP_BIOS_SEMAPHORE)) {
      // mXhciOSRequestOwnership = TRUE;
    }

    if (LegacyCtlSts & USBLEGCTLSTS_OWNERSHIP) {
      //mXhciOSRequestOwnership = TRUE;//ALJ20161028,for reboot issue ,remove this
    }

    //
    // clear Legacy usb  support status
    //
    XhcWriteExtCapReg (Xhc, Xhc->UsbLegSupOffset + 4, LegacyCtlSts);

    return EFI_SUCCESS;
}

USB_XHCI_INSTANCE*
XhcCreateUsb3Hc (
    IN EFI_PCI_FUNCTION_ADDRESS   *PciAddress
)
/*++
  Routine Description:
    Create and initialize a USB3_HC_DEV

  Arguments:
    PciIo    - The PciIo on this device

  Returns:
    The allocated and initialized USB3_HC_DEV structure
    if created, otherwise NULL.

--*/
{
  USB_XHCI_INSTANCE       *Xhc;
  EFI_STATUS              Status;
  UINT64                  PciRegAddress;
  UINT32                  PageSize;
  UINT16                  ExtCapReg;
  FULL_USB_DEVICE_PATH    DevicePath = USB_DEVICE_PATH_TEMPLATE;


  Xhc = AllocateZeroPool (sizeof (USB_XHCI_INSTANCE));
  if (Xhc == NULL) {
    return NULL;
  }

  //
  // Initialize private data structure
  //
  Xhc->Signature             = XHCI_INSTANCE_SIG;
  Xhc->Usb2Hc.GetCapability             = XhcGetCapability;
  Xhc->Usb2Hc.Reset                     = XhcReset;
  Xhc->Usb2Hc.GetState                  = XhcGetState;
  Xhc->Usb2Hc.SetState                  = XhcSetState;
  Xhc->Usb2Hc.ControlTransfer           = XhcControlTransfer;
  Xhc->Usb2Hc.BulkTransfer              = XhcBulkTransfer;
  Xhc->Usb2Hc.AsyncInterruptTransfer    = XhcAsyncInterruptTransfer;
  Xhc->Usb2Hc.SyncInterruptTransfer     = XhcSyncInterruptTransfer;
  Xhc->Usb2Hc.IsochronousTransfer       = XhcIsochronousTransfer;
  Xhc->Usb2Hc.AsyncIsochronousTransfer  = XhcAsyncIsochronousTransfer;
  Xhc->Usb2Hc.GetRootHubPortStatus      = XhcGetRootHubPortStatus;
  Xhc->Usb2Hc.SetRootHubPortFeature     = XhcSetRootHubPortFeature;
  Xhc->Usb2Hc.ClearRootHubPortFeature   = XhcClearRootHubPortFeature;
  Xhc->Usb2Hc.MajorRevision             = 0x3;
  Xhc->Usb2Hc.MinorRevision             = 0x0;

  Xhc->Usb2Hc.AsyncInterruptOnlyTransfer = XhcAsyncInterruptOnlyTransfer;
  Xhc->Usb2Hc.EnableLegacySupport       = XhcEnableLegacySupport;
  Xhc->Usb2Hc.DisableLegacySupport      = XhcDisableLegacySupport;
  Xhc->Usb2Hc.LegacyCallback            = XhcLegacyCallback;

  InitializeListHead (&Xhc->AsyncIntTransfers);
  InitializeListHead (&Xhc->AsyncIntOnlyTransfers);
  Xhc->PciAddress.Bus                   = PciAddress->Bus;
  Xhc->PciAddress.Device                = PciAddress->Device;
  Xhc->PciAddress.Function              = PciAddress->Function;

  Status = XhciEnableDevice (Xhc);
  if (EFI_ERROR(Status)) {
      goto ON_ERROR;
  }

  PciRegAddress = SMM_PCI_ADDRESS (
                      Xhc->PciAddress.Bus,
                      Xhc->PciAddress.Device,
                      Xhc->PciAddress.Function,
                      XHC_PCI_MEM_BASE_ADDRESS
                  );
  Status = SmmPciCfgRead (
               gSmst,
               SmmPciWidthUint32,
               (SMM_PCI_IO_ADDRESS *) &PciRegAddress,
               &Xhc->UsbMemBase
           );
  if (EFI_ERROR(Status)) {
      goto ON_ERROR;
  }
  Xhc->UsbMemBase &= XHC_PCI_MEM_BASE_MASK;

  //
  // Be caution that the Offset passed to XhcReadCapRegister() should be Dword align
  //
  Xhc->CapLength    = XhcReadCapReg8 (Xhc, XHC_CAPLENGTH_OFFSET);
  Xhc->HcSParams1.Dword = XhcReadCapReg (Xhc, XHC_HCSPARAMS1_OFFSET);
  Xhc->HcSParams2.Dword = XhcReadCapReg (Xhc, XHC_HCSPARAMS2_OFFSET);
  Xhc->HcCParams.Dword  = XhcReadCapReg (Xhc, XHC_HCCPARAMS_OFFSET);
  Xhc->DBOff        = XhcReadCapReg (Xhc, XHC_DBOFF_OFFSET);
  Xhc->RTSOff       = XhcReadCapReg (Xhc, XHC_RTSOFF_OFFSET);

  //
  // This PageSize field defines the page size supported by the xHC implementation.
  // This xHC supports a page size of 2^(n+12) if bit n is Set. For example,
  // if bit 0 is Set, the xHC supports 4k byte page sizes.
  //
  PageSize      = XhcReadOpReg(Xhc, XHC_PAGESIZE_OFFSET) & XHC_PAGESIZE_MASK;
  Xhc->PageSize = 1 << (HighBitSet32(PageSize) + 12);

  ExtCapReg            = (UINT16) (Xhc->HcCParams.Data.ExtCapReg);
  Xhc->ExtCapRegBase   = ExtCapReg << 2;

  Xhc->UsbLegSupOffset = XhcGetLegSupCapAddr(Xhc);
  Xhc->DevicePath = AllocateZeroPool (sizeof (DevicePath));
  if (Xhc->DevicePath == NULL)
    goto ON_ERROR;

  DevicePath.Pci.Device   = (UINT8) Xhc->PciAddress.Device;
  DevicePath.Pci.Function = (UINT8) Xhc->PciAddress.Function;
  CopyMem (
      Xhc->DevicePath,
      &DevicePath,
      sizeof (DevicePath));

  return Xhc;

ON_ERROR:
  FreePool (Xhc);
  return NULL;
}

EFI_STATUS
EFIAPI
UsbConnectXhciDriver (
    IN EFI_PCI_FUNCTION_ADDRESS   *PciAddress,
    OUT EFI_USB2_HC_PROTOCOL      **Usb2HCInterface,
    OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
)
/*++
  Routine Description:
    Starting the Usb XHCI Driver

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS          : supports this device.
    EFI_UNSUPPORTED      : do not support this device.
    EFI_DEVICE_ERROR     : cannot be started due to device Error
    EFI_OUT_OF_RESOURCES : cannot allocate resources
--*/
{
  USB_XHCI_INSTANCE       *Xhc;

  //
  // Create then Init  USB2_HC_PROTOCOL
  //
  Xhc = XhcCreateUsb3Hc (PciAddress);

  if (Xhc == NULL) {
      XHCI_DEBUG((EFI_D_ERROR, "CreateXhciController: failed to create USB3_HC\n"));
      return EFI_OUT_OF_RESOURCES;
  }

  //
  //  chapter4.2  step2 After Chip Hardware Reset wait until the Controller Not Ready (CNR) flag in the USBSTS is ¡®0¡¯ before
  //  writing any xHC Operational or Runtime registers. HW RESET READY!
  //
  //ASSERT (!(XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHCSTS_CNR)));
  if((XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHCSTS_CNR)))
  	 return EFI_UNSUPPORTED;

  XhcResetHC (Xhc, XHC_RESET_TIMEOUT);
  ASSERT (XhcIsHalt (Xhc));
  XhcSetBiosOwnership (Xhc);
  //
  // After Chip Hardware Reset wait until the Controller Not Ready (CNR) flag
  // in the USBSTS is '0' before writing any xHC Operational or Runtime registers.
  //
  ASSERT (!(XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_CNR)));

  //
  // Initialize the schedule
  //
  XhcInitSched (Xhc);

  //
  // Start the Host Controller
  //
  XhcRunHC(Xhc, XHC_GENERIC_TIMEOUT);

  //
  //return the Usb2Hc protocol for bus layer use
  //
  *Usb2HCInterface = &Xhc->Usb2Hc;
  *DevicePath      = Xhc->DevicePath;
  //
  //Record the controller in the mXhcList
  //
  InsertTailList (&mXhcList, &Xhc->Link);
  return EFI_SUCCESS;

}

EFI_STATUS
EFIAPI
UsbDisconnectXhciDriver (
    IN EFI_USB2_HC_PROTOCOL     *Usb2HCInterface
)
/*++

  Routine Description:

    Stop this driver on ControllerHandle. Support stoping any child handles
    created by this driver.

  Arguments:

    This              - Protocol instance pointer.
    Controller        - Handle of device to stop driver on
    NumberOfChildren  - Number of Children in the ChildHandleBuffer
    ChildHandleBuffer - List of handles for the children we need to stop.

  Returns:

    EFI_SUCCESS         Success
    EFI_DEVICE_ERROR    Fail
--*/
{
  EFI_STATUS            Status;
  USB_XHCI_INSTANCE     *Xhc;

  Xhc = XHC_FROM_THIS (Usb2HCInterface);

  // Halt XHC
  Status = XhcHaltHC (Xhc, XHC_GENERIC_TIMEOUT);
  if (EFI_ERROR (Status)) {
    ASSERT(0);
  }

  XhciDelAllAsyncIntTransfers (Xhc);
  XhcFreeSched (Xhc);

  XhcClearBiosOwnership (Xhc);
  FreePool (Xhc);

  return  EFI_SUCCESS;
}

