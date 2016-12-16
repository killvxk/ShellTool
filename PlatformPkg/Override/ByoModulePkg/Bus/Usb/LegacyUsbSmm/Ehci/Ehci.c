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

#include "Ehci.h"

#include "../UsbLib/UsbUtility.h"
#include "../UsbLib/SmmPciIo.h"
#include "../UsbLib/UsbMem.h"
#include <Library/ReportStatusCodeLib.h>


//
// Two arrays used to translate the EHCI port state (change)
// to the UEFI protocol's port state (change).
//
static
USB_PORT_STATE_MAP  mUsbPortStateMap[] = {
    {PORTSC_CONN,     USB_PORT_STAT_CONNECTION},
    {PORTSC_ENABLED,  USB_PORT_STAT_ENABLE},
    {PORTSC_SUSPEND,  USB_PORT_STAT_SUSPEND},
    {PORTSC_OVERCUR,  USB_PORT_STAT_OVERCURRENT},
    {PORTSC_RESET,    USB_PORT_STAT_RESET},
    {PORTSC_POWER,    USB_PORT_STAT_POWER},
    {PORTSC_OWNER,    USB_PORT_STAT_OWNER}
};

static
USB_PORT_STATE_MAP  mUsbPortChangeMap[] = {
    {PORTSC_CONN_CHANGE,    USB_PORT_STAT_C_CONNECTION},
    {PORTSC_ENABLE_CHANGE,  USB_PORT_STAT_C_ENABLE},
    {PORTSC_OVERCUR_CHANGE, USB_PORT_STAT_C_OVERCURRENT}
};

VOID
EhcClearBiosOwnership (
    IN USB2_HC_DEV          *Ehc
);

EFI_STATUS
EFIAPI
GetEhcInfo (
    IN EFI_PCI_FUNCTION_ADDRESS *PciAddress,
    IN VOID                     **SmmUsb2HcDev,
    IN VOID                     **SmmUsb2Hc
)
{
    LIST_ENTRY            *Next;

    Next = mEhcList.BackLink;
    while (Next != &mEhcList) {
        USB2_HC_DEV          *Hc;
        Hc = CR(Next, USB2_HC_DEV, Link, USB2_HC_DEV_SIGNATURE);

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
EhcGetCapability (
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
    USB2_HC_DEV             *Ehc;

    if ((MaxSpeed == NULL) || (PortNumber == NULL) || (Is64BitCapable == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    Ehc             = EHC_FROM_THIS (This);

    *MaxSpeed       = EFI_USB_SPEED_HIGH;
    *PortNumber     = (UINT8) (Ehc->HcStructParams & HCSP_NPORTS);
    *Is64BitCapable = (UINT8) (Ehc->HcCapParams & HCCP_64BIT);

    EHCI_DEBUG ((EFI_D_INFO, "EhcGetCapability: %d ports, 64 bit %d\n", *PortNumber, *Is64BitCapable));

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EhcReset (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT16               Attributes
)
/*++

  Routine Description:
    Provides software reset for the USB host controller.

  Arguments:

    This        - This EFI_USB2_HC_PROTOCOL instance.
    Attributes  - A bit mask of the reset operation to perform.

  Returns:
    EFI_SUCCESS           : The reset operation succeeded.
    EFI_INVALID_PARAMETER : Attributes is not valid.
    EFI_UNSUPPOURTED      : The type of reset specified by Attributes is
                            not currently supported by the host controller.
    EFI_DEVICE_ERROR      : Host controller isn't halted to reset.

--*/
{
    USB2_HC_DEV             *Ehc;
    EFI_STATUS              Status;
    UINT32                  DbgCtrlStatus;
    
    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_IO_BUS_USB | EFI_IOB_PC_RESET));

    Ehc     = EHC_FROM_THIS (This);

    switch (Attributes) {
    case EFI_USB_HC_RESET_GLOBAL:
        //
        // Flow through, same behavior as Host Controller Reset
        //
    case EFI_USB_HC_RESET_HOST_CONTROLLER:
        //
        // Host Controller must be Halt when Reset it
        //
        if (Ehc->DebugPortNum != 0) {
          DbgCtrlStatus = EhcReadDbgRegister(Ehc, 0);
          if ((DbgCtrlStatus & (USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_OWNER)) == (USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_OWNER)) {
            Status = EFI_SUCCESS;
            goto ON_EXIT;
          }
        }

        if (!EhcIsHalt (Ehc)) {
            Status = EhcHaltHC (Ehc, EHC_GENERIC_TIMEOUT);

            if (EFI_ERROR (Status)) {
                Status = EFI_DEVICE_ERROR;
                goto ON_EXIT;
            }
        }

        //
        // Clean up the asynchronous transfers, currently only
        // interrupt supports asynchronous operation.
        //
        EhciDelAllAsyncIntTransfers (Ehc);
        EhcAckAllInterrupt (Ehc);
        EhcFreeSched (Ehc);

        Status = EhcResetHC (Ehc, EHC_RESET_TIMEOUT);

        if (EFI_ERROR (Status)) {
            goto ON_EXIT;
        }

        Status = EhcInitHC (Ehc);
        break;

    case EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG:
    case EFI_USB_HC_RESET_HOST_WITH_DEBUG:
        Status = EFI_UNSUPPORTED;
        break;

    default:
        Status = EFI_INVALID_PARAMETER;
    }

ON_EXIT:
    EHCI_DEBUG ((EFI_D_INFO, "EhcReset: exit status %r\n", Status));
    return Status;
}

EFI_STATUS
EFIAPI
EhcGetState (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT EFI_USB_HC_STATE      *State
)
/*++

  Routine Description:
    Retrieve the current state of the USB host controller.

  Arguments:
    This    - This EFI_USB2_HC_PROTOCOL instance.
    State   - Variable to return the current host controller state.

  Returns:
    EFI_SUCCESS           : Host controller state was returned in State.
    EFI_INVALID_PARAMETER : State is NULL.
    EFI_DEVICE_ERROR      : An error was encountered while attempting to
                            retrieve the host controller's current state.
--*/
{
    USB2_HC_DEV             *Ehc;

    if (State == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    Ehc     = EHC_FROM_THIS (This);

    if (EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT)) {
        *State = EfiUsbHcStateHalt;
    } else {
        *State = EfiUsbHcStateOperational;
    }

    EHCI_DEBUG ((EFI_D_INFO, "EhcGetState: current state %d\n", *State));
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EhcSetState (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN EFI_USB_HC_STATE     State
)
/*++

  Routine Description:

    Sets the USB host controller to a specific state.

  Arguments:

    This     - This EFI_USB2_HC_PROTOCOL instance.
    State    - The state of the host controller that will be set.

  Returns:

    EFI_SUCCESS           : The USB host controller was successfully placed
                            in the state specified by State.
    EFI_INVALID_PARAMETER : State is invalid.
    EFI_DEVICE_ERROR      : Failed to set the state due to device error.

--*/
{
    USB2_HC_DEV             *Ehc;
    EFI_STATUS              Status;
    EFI_USB_HC_STATE        CurState;

    Status = EhcGetState (This, &CurState);

    if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
    }

    if (CurState == State) {
        return EFI_SUCCESS;
    }

    Ehc     = EHC_FROM_THIS (This);

    switch (State) {
    case EfiUsbHcStateHalt:
        Status = EhcHaltHC (Ehc, EHC_GENERIC_TIMEOUT);
        break;

    case EfiUsbHcStateOperational:
        if (EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_SYS_ERROR)) {
            Status = EFI_DEVICE_ERROR;
            break;
        }

        //
        // Software must not write a one to this field unless the host controller
        // is in the Halted state. Doing so will yield undefined results.
        // refers to Spec[EHCI1.0-2.3.1]
        //
        if (!EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT)) {
            Status = EFI_DEVICE_ERROR;
            break;
        }

        Status = EhcRunHC (Ehc, EHC_GENERIC_TIMEOUT);
        break;

    case EfiUsbHcStateSuspend:
        Status = EFI_UNSUPPORTED;
        break;

    default:
        Status = EFI_INVALID_PARAMETER;
    }

    EHCI_DEBUG ((EFI_D_INFO, "EhcSetState: exit status %r\n", Status));
    return Status;
}

EFI_STATUS
EFIAPI
EhcGetRootHubPortStatus (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    OUT EFI_USB_PORT_STATUS   *PortStatus
)
/*++

  Routine Description:
    Retrieves the current status of a USB root hub port.

  Arguments:
    This        - This EFI_USB2_HC_PROTOCOL instance.
    PortNumber  - The root hub port to retrieve the state from.
                  This value is zero-based.
    PortStatus  - Variable to receive the port state

  Returns:
    EFI_SUCCESS           : The status of the USB root hub port specified
                            by PortNumber was returned in PortStatus.
    EFI_INVALID_PARAMETER : PortNumber is invalid.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
    USB2_HC_DEV             *Ehc;
    UINT32                  Offset;
    UINT32                  State;
    UINT32                  TotalPort;
    UINTN                   Index;
    UINTN                   MapSize;
    EFI_STATUS              Status;
    UINT32                  DbgCtrlStatus;

    if (PortStatus == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    Ehc       = EHC_FROM_THIS (This);
    Status    = EFI_SUCCESS;

    TotalPort = (Ehc->HcStructParams & HCSP_NPORTS);

    if (PortNumber >= TotalPort) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
    }

    Offset                        = (UINT32) (EHC_PORT_STAT_OFFSET + (4 * PortNumber));
    PortStatus->PortStatus        = 0;
    PortStatus->PortChangeStatus  = 0;

    if ((Ehc->DebugPortNum != 0) && (PortNumber == (Ehc->DebugPortNum - 1))) {
      DbgCtrlStatus = EhcReadDbgRegister(Ehc, 0);
      if ((DbgCtrlStatus & (USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_OWNER)) == (USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_OWNER)) {
        goto ON_EXIT;
      }
    }
    State                         = EhcReadOpReg (Ehc, Offset);

    //
    // Identify device speed. If in K state, it is low speed.
    // If the port is enabled after reset, the device is of
    // high speed. The USB bus driver should retrieve the actual
    // port speed after reset.
    //
    if (EHC_BIT_IS_SET (State, PORTSC_LINESTATE_K)) {
        PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;

    } else if (EHC_BIT_IS_SET (State, PORTSC_ENABLED)) {
        PortStatus->PortStatus |= USB_PORT_STAT_HIGH_SPEED;
    }

    //
    // Convert the EHCI port/port change state to UEFI status
    //
    MapSize = sizeof (mUsbPortStateMap) / sizeof (USB_PORT_STATE_MAP);

    for (Index = 0; Index < MapSize; Index++) {
        if (EHC_BIT_IS_SET (State, mUsbPortStateMap[Index].HwState)) {
            PortStatus->PortStatus |= mUsbPortStateMap[Index].UefiState;
        }
    }

    MapSize = sizeof (mUsbPortChangeMap) / sizeof (USB_PORT_STATE_MAP);

    for (Index = 0; Index < MapSize; Index++) {
        if (EHC_BIT_IS_SET (State, mUsbPortChangeMap[Index].HwState)) {
            PortStatus->PortChangeStatus |= mUsbPortChangeMap[Index].UefiState;
        }
    }

ON_EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
EhcSetRootHubPortFeature (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    IN  EFI_USB_PORT_FEATURE  PortFeature
)
/*++

  Routine Description:

    Sets a feature for the specified root hub port.

  Arguments:

    This        - This EFI_USB2_HC_PROTOCOL instance.
    PortNumber  - Root hub port to set.
    PortFeature - Feature to set

  Returns:

    EFI_SUCCESS           : The feature specified by PortFeature was set
    EFI_INVALID_PARAMETER : PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR      : Can't read register
--*/
{
    USB2_HC_DEV             *Ehc;
    UINT32                  Offset;
    UINT32                  State;
    UINT32                  TotalPort;
    EFI_STATUS              Status;

    Ehc       = EHC_FROM_THIS (This);
    Status    = EFI_SUCCESS;

    TotalPort = (Ehc->HcStructParams & HCSP_NPORTS);

    if (PortNumber >= TotalPort) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
    }

    Offset  = (UINT32) (EHC_PORT_STAT_OFFSET + (4 * PortNumber));
    State   = EhcReadOpReg (Ehc, Offset);

    //
    // Mask off the port status change bits, these bits are
    // write clean bit
    //
    State &= ~PORTSC_CHANGE_MASK;

    switch (PortFeature) {
    case EfiUsbPortEnable:
        //
        // Sofeware can't set this bit, Port can only be enable by
        // EHCI as a part of the reset and enable
        //
        State |= PORTSC_ENABLED;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    case EfiUsbPortSuspend:
        State |= PORTSC_SUSPEND;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    case EfiUsbPortReset:
        //
        // Make sure Host Controller not halt before reset it
        //
        if (EhcIsHalt (Ehc)) {
            Status = EhcRunHC (Ehc, EHC_GENERIC_TIMEOUT);

            if (EFI_ERROR (Status)) {
                EHCI_DEBUG ((EFI_D_INFO, "EhcSetRootHubPortFeature :failed to start HC - %r\n", Status));
                break;
            }
        }

        //
        // Set one to PortReset bit must also set zero to PortEnable bit
        //
        State |= PORTSC_RESET;
        State &= ~PORTSC_ENABLED;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    case EfiUsbPortPower:
        //
        // Not supported, ignore the operation
        //
        Status = EFI_SUCCESS;
        break;

    case EfiUsbPortOwner:
        State |= PORTSC_OWNER;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    default:
        Status = EFI_INVALID_PARAMETER;
    }

ON_EXIT:
    EHCI_DEBUG ((EFI_D_INFO, "EhcSetRootHubPortFeature: exit status %r\n", Status));

    return Status;
}

EFI_STATUS
EFIAPI
EhcClearRootHubPortFeature (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    IN  EFI_USB_PORT_FEATURE  PortFeature
)
/*++

  Routine Description:

    Clears a feature for the specified root hub port.

  Arguments:

    This        - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    PortNumber  - Specifies the root hub port whose feature
                  is requested to be cleared.
    PortFeature - Indicates the feature selector associated with the
                  feature clear request.

  Returns:

    EFI_SUCCESS           : The feature specified by PortFeature was cleared
                            for the USB root hub port specified by PortNumber.
    EFI_INVALID_PARAMETER : PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
    USB2_HC_DEV             *Ehc;
    UINT32                  Offset;
    UINT32                  State;
    UINT32                  TotalPort;
    EFI_STATUS              Status;

    Ehc       = EHC_FROM_THIS (This);
    Status    = EFI_SUCCESS;

    TotalPort = (Ehc->HcStructParams & HCSP_NPORTS);

    if (PortNumber >= TotalPort) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
    }

    Offset  = EHC_PORT_STAT_OFFSET + (4 * PortNumber);
    State   = EhcReadOpReg (Ehc, Offset);
    State &= ~PORTSC_CHANGE_MASK;

    switch (PortFeature) {
    case EfiUsbPortEnable:
        //
        // Clear PORT_ENABLE feature means disable port.
        //
        State &= ~PORTSC_ENABLED;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    case EfiUsbPortSuspend:
        //
        // A write of zero to this bit is ignored by the host
        // controller. The host controller will unconditionally
        // set this bit to a zero when:
        //   1. software sets the Forct Port Resume bit to a zero from a one.
        //   2. software sets the Port Reset bit to a one frome a zero.
        //
        State &= ~PORSTSC_RESUME;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    case EfiUsbPortReset:
        //
        // Clear PORT_RESET means clear the reset signal.
        //
        State &= ~PORTSC_RESET;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    case EfiUsbPortOwner:
        //
        // Clear port owner means this port owned by EHC
        //
        State &= ~PORTSC_OWNER;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    case EfiUsbPortConnectChange:
        //
        // Clear connect status change
        //
        State |= PORTSC_CONN_CHANGE;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    case EfiUsbPortEnableChange:
        //
        // Clear enable status change
        //
        State |= PORTSC_ENABLE_CHANGE;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    case EfiUsbPortOverCurrentChange:
        //
        // Clear PortOverCurrent change
        //
        State |= PORTSC_OVERCUR_CHANGE;
        EhcWriteOpReg (Ehc, Offset, State);
        break;

    case EfiUsbPortPower:
    case EfiUsbPortSuspendChange:
    case EfiUsbPortResetChange:
        //
        // Not supported or not related operation
        //
        break;

    default:
        Status = EFI_INVALID_PARAMETER;
        break;
    }

ON_EXIT:
    EHCI_DEBUG ((EFI_D_INFO, "EhcClearRootHubPortFeature: exit status %r\n", Status));
    return Status;
}

EFI_STATUS
EFIAPI
EhcControlTransfer (
    IN  EFI_USB2_HC_PROTOCOL                *This,
    IN  UINT8                               DeviceAddress,
    IN  UINT8                               DeviceSpeed,
    IN  UINTN                               MaximumPacketLength,
    IN  EFI_USB_DEVICE_REQUEST              *Request,
    IN  EFI_USB_DATA_DIRECTION              TransferDirection,
    IN  OUT VOID                            *Data,
    IN  OUT UINTN                           *DataLength,
    IN  UINTN                               TimeOut,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *TransferResult
)
/*++

  Routine Description:

    Submits control transfer to a target USB device.

  Arguments:

    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - The target device address
    DeviceSpeed         - Target device speed.
    MaximumPacketLength - Maximum packet size the default control transfer
                          endpoint is capable of sending or receiving.
    Request             - USB device request to send
    TransferDirection   - Specifies the data direction for the data stage
    Data                - Data buffer to be transmitted or received from USB device.
    DataLength          - The size (in bytes) of the data buffer
    TimeOut             - Indicates the maximum timeout, in millisecond,
    Translator          - Transaction translator to be used by this device.
    TransferResult      - Return the result of this control transfer.

  Returns:

    EFI_SUCCESS           : Transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : The transfer failed due to lack of resources.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : Transfer failed due to timeout.
    EFI_DEVICE_ERROR      : Transfer failed due to host controller or device error.

--*/
{
    USB2_HC_DEV             *Ehc;
    URB                     *Urb;
    UINT8                   Endpoint;
    EFI_STATUS              Status;

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
            (MaximumPacketLength != 32) && (MaximumPacketLength != 64)) {
        return EFI_INVALID_PARAMETER;
    }

    if ((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8)) {
        return EFI_INVALID_PARAMETER;
    }

    Ehc             = EHC_FROM_THIS (This);

    Status          = EFI_DEVICE_ERROR;
    *TransferResult = EFI_USB_ERR_SYSTEM;

    if (EhcIsHalt (Ehc) || EhcIsSysError (Ehc)) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcControlTransfer: HC halted at entrance\n"));

        EhcAckAllInterrupt (Ehc);
        goto ON_EXIT;
    }

    EhcAckAllInterrupt (Ehc);

    //
    // Create a new URB, insert it into the asynchronous
    // schedule list, then poll the execution status.
    //
    //
    // Encode the direction in address, although default control
    // endpoint is bidirectional. EhcCreateUrb expects this
    // combination of Ep addr and its direction.
    //
    Endpoint = 0 | ((TransferDirection == EfiUsbDataIn) ? 0x80 : 0);
    Urb = EhcCreateUrb (
              Ehc,
              DeviceAddress,
              Endpoint,
              DeviceSpeed,
              0,
              MaximumPacketLength,
              Translator,
              EHC_CTRL_TRANSFER,
              Request,
              Data,
              *DataLength,
              NULL,
              NULL,
              1
          );

    if (Urb == NULL) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcControlTransfer: failed to create URB"));

        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
    }

    EhcLinkQhToAsync (Ehc, Urb->Qh);
    Status = EhcExecTransfer (Ehc, Urb, TimeOut);
    EhcUnlinkQhFromAsync (Ehc, Urb->Qh);

    //
    // Get the status from URB. The result is updated in EhcCheckUrbResult
    // which is called by EhcExecTransfer
    //
    *TransferResult = Urb->Result;
    *DataLength     = Urb->Completed;

    if (*TransferResult == EFI_USB_NOERROR) {
        Status = EFI_SUCCESS;
    }

    EhcAckAllInterrupt (Ehc);
    EhcFreeUrb (Ehc, Urb);

ON_EXIT:

    if (EFI_ERROR (Status)) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcControlTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
    }

    return Status;
}

EFI_STATUS
EFIAPI
EhcBulkTransfer (
    IN  EFI_USB2_HC_PROTOCOL                *This,
    IN  UINT8                               DeviceAddress,
    IN  UINT8                               EndPointAddress,
    IN  UINT8                               DeviceSpeed,
    IN  UINTN                               MaximumPacketLength,
    IN  UINT8                               DataBuffersNumber,
    IN  OUT VOID                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
    IN  OUT UINTN                           *DataLength,
    IN  OUT UINT8                           *DataToggle,
    IN  UINTN                               TimeOut,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *TransferResult
)
/*++

  Routine Description:

    Submits bulk transfer to a bulk endpoint of a USB device.

  Arguments:

    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Target device address
    EndPointAddress     - Endpoint number and its direction in bit 7. .
    DeviceSpeed         - Device speed, Low speed device doesn't support
                          bulk transfer.
    MaximumPacketLength - Maximum packet size the endpoint is capable of
                          sending or receiving.
    DataBuffersNumber   - Number of data buffers prepared for the transfer.
    Data                - Array of pointers to the buffers of data to transmit
                          from or receive into.
    DataLength          - The lenght of the data buffer
    DataToggle          - On input, the initial data toggle for the transfer;
                          On output, it is updated to to next data toggle to use
                          of the subsequent bulk transfer.
    Translator          - A pointr to the transaction translator data.
    TimeOut             - Indicates the maximum time, in millisecond, which the
                          transfer is allowed to complete.
    TransferResult      - A pointer to the detailed result information of the
                          bulk transfer.

  Returns:

    EFI_SUCCESS           : The transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : The transfer failed due to lack of resource.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : The transfer failed due to timeout.
    EFI_DEVICE_ERROR      : The transfer failed due to host controller error.

--*/
{
    USB2_HC_DEV             *Ehc;
    URB                     *Urb;
    EFI_STATUS              Status;

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
            ((EFI_USB_SPEED_HIGH == DeviceSpeed) && (MaximumPacketLength > 512))) {
        return EFI_INVALID_PARAMETER;
    }

    Ehc             = EHC_FROM_THIS (This);

    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;

    if (EhcIsHalt (Ehc) || EhcIsSysError (Ehc)) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcBulkTransfer: HC is halted\n"));

        EhcAckAllInterrupt (Ehc);
        goto ON_EXIT;
    }

    EhcAckAllInterrupt (Ehc);

    //
    // Create a new URB, insert it into the asynchronous
    // schedule list, then poll the execution status.
    //
    Urb = EhcCreateUrb (
              Ehc,
              DeviceAddress,
              EndPointAddress,
              DeviceSpeed,
              *DataToggle,
              MaximumPacketLength,
              Translator,
              EHC_BULK_TRANSFER,
              NULL,
              Data[0],
              *DataLength,
              NULL,
              NULL,
              1
          );

    if (Urb == NULL) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcBulkTransfer: failed to create URB\n"));

        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
    }

    EhcLinkQhToAsync (Ehc, Urb->Qh);
    Status = EhcExecTransfer (Ehc, Urb, TimeOut);
    EhcUnlinkQhFromAsync (Ehc, Urb->Qh);

    *TransferResult = Urb->Result;
    *DataLength     = Urb->Completed;
    *DataToggle     = Urb->DataToggle;

    if (*TransferResult == EFI_USB_NOERROR) {
        Status = EFI_SUCCESS;
    }

    EhcAckAllInterrupt (Ehc);
    EhcFreeUrb (Ehc, Urb);

ON_EXIT:

    if (EFI_ERROR (Status)) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcBulkTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
    }

    return Status;
}

EFI_STATUS
EFIAPI
EhcAsyncInterruptTransfer (
    IN  EFI_USB2_HC_PROTOCOL                  * This,
    IN  UINT8                                 DeviceAddress,
    IN  UINT8                                 EndPointAddress,
    IN  UINT8                                 DeviceSpeed,
    IN  UINTN                                 MaximumPacketLength,
    IN  BOOLEAN                               IsNewTransfer,
    IN  OUT UINT8                             *DataToggle,
    IN  UINTN                                 PollingInterval,
    IN  UINTN                                 DataLength,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    * Translator,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
    IN  VOID                                  *Context OPTIONAL
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
    USB2_HC_DEV             *Ehc;
    URB                     *Urb;
    EFI_STATUS              Status;
    UINT8                   *Data;

    //
    // Validate parameters
    //
    if (!EHCI_IS_DATAIN (EndPointAddress)) {
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

    Ehc     = EHC_FROM_THIS (This);

    //
    // Delete Async interrupt transfer request. DataToggle will return
    // the next data toggle to use.
    //
    if (!IsNewTransfer) {
        Status = EhciDelAsyncIntTransfer (Ehc, DeviceAddress, EndPointAddress,
                                          CallBackFunction, Context, DataToggle);

        EHCI_DEBUG ((EFI_D_INFO, "EhcAsyncInterruptTransfer: remove old transfer - %r\n", Status));
        goto ON_EXIT;
    }

    Status = EFI_SUCCESS;

    if (EhcIsHalt (Ehc) || EhcIsSysError (Ehc)) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcAsyncInterruptTransfer: HC is halt\n"));
        EhcAckAllInterrupt (Ehc);

        Status = EFI_DEVICE_ERROR;
        goto ON_EXIT;
    }

    EhcAckAllInterrupt (Ehc);

    //
    // Allocate and map source data buffer for bus master access.
    //
    Data = UsbAllocatePool (DataLength);

    if (Data == NULL) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcAsyncInterruptTransfer: failed to allocate buffer\n"));

        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
    }

    Urb = EhcCreateUrb (
              Ehc,
              DeviceAddress,
              EndPointAddress,
              DeviceSpeed,
              *DataToggle,
              MaximumPacketLength,
              Translator,
              EHC_INT_TRANSFER_ASYNC,
              NULL,
              Data,
              DataLength,
              CallBackFunction,
              Context,
              PollingInterval
          );

    if (Urb == NULL) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcAsyncInterruptTransfer: failed to create URB\n"));

        UsbFreePool (Data, DataLength);
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
    }

    //
    // New asynchronous transfer must inserted to the head.
    // Check the comments in EhcMoniteAsyncRequests
    //
    EhcLinkQhToPeriod (Ehc, Urb->Qh);
    InsertHeadList (&Ehc->AsyncIntTransfers, &Urb->UrbList);

ON_EXIT:

    return Status;
}

EFI_STATUS
EFIAPI
EhcAsyncInterruptOnlyTransfer (
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
    USB2_HC_DEV             *Ehc;
    URB                     *Urb;
    EFI_STATUS              Status;
    UINT8                   DataToggle;

    DataToggle = 0;

    //
    // Validate parameters
    //
    Ehc     = EHC_FROM_THIS (This);

    //
    // Delete Async interrupt transfer request. DataToggle will return
    // the next data toggle to use.
    //
    if (!IsNewTransfer) {
        Status = EhciDelAsyncIntTransfer (
                     Ehc,
                     EHC_INTERRUPT_ONLY_DEVICE_ADDRESS,
                     EHC_INTERRUPT_ONLY_ENDPOINT_ADDRESS,
                     CallBackFunction,
                     Context,
                     &DataToggle);

        goto ON_EXIT;
    }

    Status = EFI_SUCCESS;

    if (EhcIsHalt (Ehc) || EhcIsSysError (Ehc)) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcAsyncInterruptOnlyTransfer: HC is halt\n"));
        EhcAckAllInterrupt (Ehc);

        Status = EFI_DEVICE_ERROR;
        goto ON_EXIT;
    }

    EhcAckAllInterrupt (Ehc);

    Urb = EhcCreateUrb (
              Ehc,
              EHC_INTERRUPT_ONLY_DEVICE_ADDRESS,
              EHC_INTERRUPT_ONLY_ENDPOINT_ADDRESS,
              EFI_USB_SPEED_HIGH,
              0,
              0,
              NULL,
              EHC_INT_ONLY_TRANSFER_ASYNC,
              NULL,
              NULL,
              0,
              CallBackFunction,
              Context,
              PollingInterval
          );

    if (Urb == NULL) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcAsyncInterruptOnlyTransfer: failed to create URB\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
    }

    //
    // New asynchronous transfer must inserted to the head.
    // Check the comments in EhcMoniteAsyncRequests
    //
    EhcLinkQhToPeriod (Ehc, Urb->Qh);
    InsertHeadList (&Ehc->AsyncIntTransfers, &Urb->UrbList);

ON_EXIT:

    return Status;
}

EFI_STATUS
EFIAPI
EhcSyncInterruptTransfer (
    IN  EFI_USB2_HC_PROTOCOL                *This,
    IN  UINT8                               DeviceAddress,
    IN  UINT8                               EndPointAddress,
    IN  UINT8                               DeviceSpeed,
    IN  UINTN                               MaximumPacketLength,
    IN  OUT VOID                            *Data,
    IN  OUT UINTN                           *DataLength,
    IN  OUT UINT8                           *DataToggle,
    IN  UINTN                               TimeOut,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *TransferResult
)
/*++

  Routine Description:

    Submits synchronous interrupt transfer to an interrupt endpoint
    of a USB device.

  Arguments:

    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Target device address
    EndPointAddress     - Endpoint number and its direction encoded in bit 7
    DeviceSpeed         - Indicates device speed.
    MaximumPacketLength - Maximum packet size the target endpoint is capable
                          of sending or receiving.
    Data                - Buffer of data that will be transmitted to
                          USB device or received from USB device.
    DataLength          - On input, the size, in bytes, of the data buffer;
                          On output, the number of bytes transferred.
    DataToggle          - On input, the initial data toggle to use;
                          on output, it is updated to indicate the next data toggle
    TimeOut             - Maximum time, in second, to complete
    Translator          - Transaction translator to use.
    TransferResult      - Variable to receive the transfer result

  Returns:

    EFI_SUCCESS           : The transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : The transfer failed due to lack of resource.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : The transfer failed due to timeout.
    EFI_DEVICE_ERROR      : The failed due to host controller or device error

--*/
{
    USB2_HC_DEV             *Ehc;
    URB                     *Urb;
    EFI_STATUS              Status;

    //
    // Validates parameters
    //
    if ((DataLength == NULL) || (*DataLength == 0) ||
            (Data == NULL) || (TransferResult == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    if (!EHCI_IS_DATAIN (EndPointAddress)) {
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

    Ehc             = EHC_FROM_THIS (This);

    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;

    if (EhcIsHalt (Ehc) || EhcIsSysError (Ehc)) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcSyncInterruptTransfer: HC is halt\n"));

        EhcAckAllInterrupt (Ehc);
        goto ON_EXIT;
    }

    EhcAckAllInterrupt (Ehc);

    Urb = EhcCreateUrb (
              Ehc,
              DeviceAddress,
              EndPointAddress,
              DeviceSpeed,
              *DataToggle,
              MaximumPacketLength,
              Translator,
              EHC_INT_TRANSFER_SYNC,
              NULL,
              Data,
              *DataLength,
              NULL,
              NULL,
              1
          );

    if (Urb == NULL) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcSyncInterruptTransfer: failed to create URB\n"));

        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
    }

    EhcLinkQhToPeriod (Ehc, Urb->Qh);
    Status = EhcExecTransfer (Ehc, Urb, TimeOut);
    EhcUnlinkQhFromPeriod (Ehc, Urb->Qh);

    *TransferResult = Urb->Result;
    *DataLength     = Urb->Completed;
    *DataToggle     = Urb->DataToggle;

    if (*TransferResult == EFI_USB_NOERROR) {
        Status = EFI_SUCCESS;
    }
    EhcFreeUrb (Ehc, Urb);

ON_EXIT:
    if (EFI_ERROR (Status)) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcSyncInterruptTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
    }

    return Status;
}

EFI_STATUS
EFIAPI
EhcIsochronousTransfer (
    IN  EFI_USB2_HC_PROTOCOL                *This,
    IN  UINT8                               DeviceAddress,
    IN  UINT8                               EndPointAddress,
    IN  UINT8                               DeviceSpeed,
    IN  UINTN                               MaximumPacketLength,
    IN  UINT8                               DataBuffersNumber,
    IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN  UINTN                               DataLength,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *TransferResult
)
/*++

  Routine Description:

    Submits isochronous transfer to a target USB device.

  Arguments:

    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Target device address
    EndPointAddress     - End point address with its direction
    DeviceSpeed         - Device speed, Low speed device doesn't support this type.
    MaximumPacketLength - Maximum packet size that the endpoint is capable of
                          sending or receiving.
    DataBuffersNumber   - Number of data buffers prepared for the transfer.
    Data                - Array of pointers to the buffers of data that will be
                          transmitted to USB device or received from USB device.
    DataLength          - The size, in bytes, of the data buffer
    Translator          - Transaction translator to use.
    TransferResult      - Variable to receive the transfer result

  Returns:

    EFI_UNSUPPORTED : Isochronous transfer is unsupported.

--*/
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
EhcAsyncIsochronousTransfer (
    IN  EFI_USB2_HC_PROTOCOL                *This,
    IN  UINT8                               DeviceAddress,
    IN  UINT8                               EndPointAddress,
    IN  UINT8                               DeviceSpeed,
    IN  UINTN                               MaximumPacketLength,
    IN  UINT8                               DataBuffersNumber,
    IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN  UINTN                               DataLength,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
    IN  VOID                                *Context
)
/*++

  Routine Description:

    Submits Async isochronous transfer to a target USB device.

  Arguments:
    This                - This EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Target device address
    EndPointAddress     - End point address with its direction
    DeviceSpeed         - Device speed, Low speed device doesn't support this type.
    MaximumPacketLength - Maximum packet size that the endpoint is capable of
                          sending or receiving.
    DataBuffersNumber   - Number of data buffers prepared for the transfer.
    Data                - Array of pointers to the buffers of data that will be
                          transmitted to USB device or received from USB device.
    DataLength          - The size, in bytes, of the data buffer
    Translator          - Transaction translator to use.
    IsochronousCallBack - Function to be called when the transfer complete
    Context             - Context passed to the call back function as parameter

  Returns:

    EFI_UNSUPPORTED : Isochronous transfer isn't supported

--*/
{
    return EFI_UNSUPPORTED;
}

/**
  Get the usb debug port related information.

  @param  Ehc                The EHCI device.

  @retval RETURN_SUCCESS     Get debug port number, bar and offset successfully.
  @retval Others             The usb host controller does not supported usb debug port capability.

**/
EFI_STATUS
EhcGetUsbDebugPortInfo (
  IN  USB2_HC_DEV     *Ehc
 )
{
  UINT16              PciStatus;
  UINT8               CapabilityPtr;
  UINT8               CapabilityId;
  UINT16              DebugPort;
  EFI_STATUS          Status;
  UINT64              PciRegAddress;

  //
  // Detect if the EHCI host controller support Capaility Pointer.
  //
  PciRegAddress = SMM_PCI_ADDRESS (
                      Ehc->PciAddress.Bus,
                      Ehc->PciAddress.Device,
                      Ehc->PciAddress.Function,
                      PCI_PRIMARY_STATUS_OFFSET
                  );
  Status = SmmPciCfgRead (
               gSmst,
               EfiPciIoWidthUint16,
               (SMM_PCI_IO_ADDRESS *) &PciRegAddress,
               &PciStatus
           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((PciStatus & EFI_PCI_STATUS_CAPABILITY) == 0) {
    //
    // The Pci Device Doesn't Support Capability Pointer.
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Get Pointer To Capability List
  //
  PciRegAddress = SMM_PCI_ADDRESS (
                      Ehc->PciAddress.Bus,
                      Ehc->PciAddress.Device,
                      Ehc->PciAddress.Function,
                      PCI_CAPBILITY_POINTER_OFFSET
                  );
  Status = SmmPciCfgRead (
               gSmst,
               EfiPciIoWidthUint8,
               (SMM_PCI_IO_ADDRESS *) &PciRegAddress,
               &CapabilityPtr
           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find Capability ID 0xA, Which Is For Debug Port
  //
  while (CapabilityPtr != 0) {
    PciRegAddress = SMM_PCI_ADDRESS (
                        Ehc->PciAddress.Bus,
                        Ehc->PciAddress.Device,
                        Ehc->PciAddress.Function,
                        CapabilityPtr
                    );
    Status = SmmPciCfgRead (
                 gSmst,
                 EfiPciIoWidthUint8,
                 (SMM_PCI_IO_ADDRESS *) &PciRegAddress,
                 &CapabilityId
             );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (CapabilityId == EHC_DEBUG_PORT_CAP_ID) {
      break;
    }
    PciRegAddress = SMM_PCI_ADDRESS (
                        Ehc->PciAddress.Bus,
                        Ehc->PciAddress.Device,
                        Ehc->PciAddress.Function,
                        CapabilityPtr + 1
                    );
    Status = SmmPciCfgRead (
                 gSmst,
                 EfiPciIoWidthUint8,
                 (SMM_PCI_IO_ADDRESS *) &PciRegAddress,
                 &CapabilityPtr
             );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // No Debug Port Capability Found
  //
  if (CapabilityPtr == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Get The Base Address Of Debug Port Register In Debug Port Capability Register
  //
  PciRegAddress = SMM_PCI_ADDRESS (
                      Ehc->PciAddress.Bus,
                      Ehc->PciAddress.Device,
                      Ehc->PciAddress.Function,
                      CapabilityPtr + 2
                  );
  Status = SmmPciCfgRead (
               gSmst,
               EfiPciIoWidthUint16,
               (SMM_PCI_IO_ADDRESS *) &PciRegAddress,
               &DebugPort
           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Ehc->DebugPortOffset = DebugPort & 0x1FFF;
  Ehc->DebugPortBarNum = (UINT8)((DebugPort >> 13) - 1);
  Ehc->DebugPortNum    = (UINT8)((Ehc->HcStructParams & 0x00F00000) >> 20);

  return EFI_SUCCESS;
}

USB2_HC_DEV *
EhcCreateUsb2Hc (
    IN EFI_PCI_FUNCTION_ADDRESS   *PciAddress
)
/*++
  Routine Description:
    Create and initialize a USB2_HC_DEV

  Arguments:
    PciIo    - The PciIo on this device

  Returns:
    The allocated and initialized USB2_HC_DEV structure
    if created, otherwise NULL.

--*/
{
    USB2_HC_DEV             *Ehc;
    EFI_STATUS              Status;
    UINT64                  PciRegAddress;
    FULL_USB_DEVICE_PATH    DevicePath = USB_DEVICE_PATH_TEMPLATE;

    Ehc = AllocateZeroPool (sizeof (USB2_HC_DEV));
    if (Ehc == NULL)
        return NULL;

    //
    // Init EFI_USB2_HC_PROTOCOL interface and private data structure
    //
    Ehc->Signature                        = USB2_HC_DEV_SIGNATURE;

    Ehc->Usb2Hc.GetCapability             = EhcGetCapability;
    Ehc->Usb2Hc.Reset                     = EhcReset;
    Ehc->Usb2Hc.GetState                  = EhcGetState;
    Ehc->Usb2Hc.SetState                  = EhcSetState;
    Ehc->Usb2Hc.ControlTransfer           = EhcControlTransfer;
    Ehc->Usb2Hc.BulkTransfer              = EhcBulkTransfer;
    Ehc->Usb2Hc.AsyncInterruptTransfer    = EhcAsyncInterruptTransfer;
    Ehc->Usb2Hc.SyncInterruptTransfer     = EhcSyncInterruptTransfer;
    Ehc->Usb2Hc.IsochronousTransfer       = EhcIsochronousTransfer;
    Ehc->Usb2Hc.AsyncIsochronousTransfer  = EhcAsyncIsochronousTransfer;
    Ehc->Usb2Hc.GetRootHubPortStatus      = EhcGetRootHubPortStatus;
    Ehc->Usb2Hc.SetRootHubPortFeature     = EhcSetRootHubPortFeature;
    Ehc->Usb2Hc.ClearRootHubPortFeature   = EhcClearRootHubPortFeature;
    Ehc->Usb2Hc.MajorRevision             = 0x2;
    Ehc->Usb2Hc.MinorRevision             = 0x0;

    Ehc->Usb2Hc.AsyncInterruptOnlyTransfer = EhcAsyncInterruptOnlyTransfer;
    Ehc->Usb2Hc.EnableLegacySupport       = EhcEnableLegacySupport;
    Ehc->Usb2Hc.DisableLegacySupport      = EhcDisableLegacySupport;
    Ehc->Usb2Hc.LegacyCallback            = EhcLegacyCallback;

    InitializeListHead (&Ehc->AsyncIntTransfers);
    Ehc->PciAddress.Bus                   = PciAddress->Bus;
    Ehc->PciAddress.Device                = PciAddress->Device;
    Ehc->PciAddress.Function              = PciAddress->Function;

    Status = EhciEnableDevice (Ehc);
    if (EFI_ERROR(Status))
    {
        goto ON_ERROR;
    }

    PciRegAddress = SMM_PCI_ADDRESS (
                        Ehc->PciAddress.Bus,
                        Ehc->PciAddress.Device,
                        Ehc->PciAddress.Function,
                        EHC_PCI_MEM_BASE_ADDRESS
                    );
    Status = SmmPciCfgRead (
                 gSmst,
                 SmmPciWidthUint32,
                 (SMM_PCI_IO_ADDRESS *) &PciRegAddress,
                 &Ehc->UsbMemBase
             );
    if (EFI_ERROR(Status)) {
        goto ON_ERROR;
    }
    Ehc->UsbMemBase &= EHC_PCI_MEM_BASE_MASK;

    Ehc->HcStructParams = EhcReadCapRegister (Ehc, EHC_HCSPARAMS_OFFSET);
    Ehc->HcCapParams    = EhcReadCapRegister (Ehc, EHC_HCCPARAMS_OFFSET);
    Ehc->CapLen         = EhcReadCapRegister (Ehc, EHC_CAPLENGTH_OFFSET) & 0x0FF;

    EHCI_DEBUG ((EFI_D_INFO, "EhcCreateUsb2Hc: capability length %d\n", Ehc->CapLen));

    EhcGetUsbDebugPortInfo (Ehc);

    Ehc->DevicePath = AllocateZeroPool (sizeof (DevicePath));
    if (Ehc->DevicePath == NULL)
        goto ON_ERROR;

    DevicePath.Pci.Device   = (UINT8) Ehc->PciAddress.Device;
    DevicePath.Pci.Function = (UINT8) Ehc->PciAddress.Function;
    CopyMem (
        Ehc->DevicePath,
        &DevicePath,
        sizeof (DevicePath));

    return Ehc;

ON_ERROR:
    FreePool (Ehc);
    return NULL;
}

EFI_STATUS
EFIAPI
UsbConnectEhciDriver (
    IN EFI_PCI_FUNCTION_ADDRESS   *PciAddress,
    OUT EFI_USB2_HC_PROTOCOL      **Usb2HCInterface,
    OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
)
/*++
  Routine Description:
    Starting the Usb EHCI Driver

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
    EFI_STATUS              Status;
    USB2_HC_DEV             *Ehc;
    UINT32                  State;
    //
    // Create then install USB2_HC_PROTOCOL
    //
    Ehc = EhcCreateUsb2Hc (PciAddress);

    if (Ehc == NULL) {
        EHCI_DEBUG ((EFI_D_ERROR, "CreateEhciController: failed to create USB2_HC\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    EhcSetBiosOwnership (Ehc);


    if (Ehc->DebugPortNum != 0) {
      State = EhcReadDbgRegister(Ehc, 0);
      if ((State & (USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_OWNER)) != (USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_OWNER)) {
        EhcResetHC (Ehc, EHC_RESET_TIMEOUT);
      }
    }
    Status = EhcInitHC (Ehc);

    if (EFI_ERROR (Status)) {
        EHCI_DEBUG ((EFI_D_ERROR, "CreateEhciController: failed to init host controller\n"));
        goto FREE_POOL;
    }

    *Usb2HCInterface = &Ehc->Usb2Hc;
    *DevicePath      = Ehc->DevicePath;

    InsertTailList (&mEhcList, &Ehc->Link);
    return EFI_SUCCESS;

FREE_POOL:
    EhcFreeSched (Ehc);
    FreePool (Ehc);

    return Status;
}

EFI_STATUS
EFIAPI
UsbDisconnectEhciDriver (
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
    USB2_HC_DEV           *Ehc;

    Ehc = EHC_FROM_THIS (Usb2HCInterface);

    EhcClearBiosOwnership (Ehc);
    EhcHaltHC (Ehc, EHC_GENERIC_TIMEOUT);

    //
    // Clean up the asynchronous transfers, currently only
    // interrupt supports asynchronous operation.
    //
    EhciDelAllAsyncIntTransfers (Ehc);
    EhcAckAllInterrupt (Ehc);
    EhcFreeSched (Ehc);
    EhcResetHC (Ehc, EHC_RESET_TIMEOUT);

    RemoveEntryList (&Ehc->Link);
    FreePool (Ehc);

    return  EFI_SUCCESS;
}

