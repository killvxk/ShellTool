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

#include "Uhci.h"
#include "../UsbLib/UsbUtility.h"
#include "../UsbLib/SmmPciIo.h"
#include "../UsbLib/UsbMem.h"
#include <Library/ReportStatusCodeLib.h>

BOOLEAN  PollingEvent = FALSE;

UHCI_TD_SW *
UhciCreateDataTd (
    IN  USB_HC_DEV          *Uhc,
    IN  UINT8               DevAddr,
    IN  UINT8               Endpoint,
    IN  UINT8               *DataPtr,
    IN  UINTN               Len,
    IN  UINT8               PktId,
    IN  UINT8               Toggle,
    IN  BOOLEAN             IsLow
);

EFI_STATUS
EFIAPI
GetUhcInfo (
    IN EFI_PCI_FUNCTION_ADDRESS *PciAddress,
    IN VOID                     **SmmUhc,
    IN VOID                     **SmmUsbHc,
    IN VOID                     **SmmUsb2Hc
)
{
    LIST_ENTRY                  *Next;

    Next = mUhcList.BackLink;
    while (Next != &mUhcList) {
        USB_HC_DEV          *Hc;
        Hc = CR(Next, USB_HC_DEV, Link, USB_HC_DEV_SIGNATURE);
        if (Hc->PciAddress.Bus == PciAddress->Bus &&
                Hc->PciAddress.Device == PciAddress->Device &&
                Hc->PciAddress.Function == PciAddress->Function ) {

            *SmmUhc = (VOID *)Hc;
            *SmmUsbHc = (VOID *)(&Hc->UsbHc);
            *SmmUsb2Hc = (VOID *)(&Hc->Usb2Hc);

            return EFI_SUCCESS;
        }
        Next = Next->BackLink;
    }

    return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
Uhci2VirtualKbc (
    IN EFI_USB2_HC_PROTOCOL    *this,
    IN EFI_USB_VIRTUAL_FEATURE VirtualFeature,
    IN UINT8                   *State,
    IN BOOLEAN                 RWFlag
)
{

    USB_HC_DEV *Uhc;
    UINT16      Value;
    UINT16      Data;
    UINT64      PciAddress;

    Uhc = UHC_FROM_USB2_HC_PROTO (this);

    PciAddress = SMM_PCI_ADDRESS (
                     Uhc->PciAddress.Bus,
                     Uhc->PciAddress.Device,
                     Uhc->PciAddress.Function,
                     USB_EMULATION_OFFSET);

    SmmPciCfgRead (
        gSmst,
        SmmPciWidthUint16,
        (SMM_PCI_IO_ADDRESS *) &PciAddress,
        &Value
    );

    Data = (UINT16) Value;

    switch (VirtualFeature) {
    case EfiVirtualOn:
        Value &= USBLEGACY_SMIEN | USBLEGACY_A20PASSEN | USBLEGACY_SMIATENDPS
                 | USBLEGACY_USBPIRQEN;
        Value |= *State;
        break;

    case EfiVirtualOff:
        Value &= USBLEGACY_SMIEN | USBLEGACY_A20PASSEN | USBLEGACY_SMIATENDPS
                 | USBLEGACY_USBPIRQEN;
        break;
    case EfiVirtualStatus:
        Data = Data >> 8;
        Data &= *State;
        Data = Data << 8;
        *State = 0;

        if (Data == 0)
            goto  ON_EfiVirtualStatusEXIT;

        if ((Data & USBLEGACY_TRAPBY60R) == USBLEGACY_TRAPBY60R)
            *State = KBC_PORT60_READ;

        if ((Data & USBLEGACY_TRAPBY60W) == USBLEGACY_TRAPBY60W)
            *State = KBC_PORT60_WRITE;

        if ((Data & USBLEGACY_TRAPBY64R) == USBLEGACY_TRAPBY64R)
            *State = KBC_PORT64_READ;

        if ((Data & USBLEGACY_TRAPBY64W) == USBLEGACY_TRAPBY64W)
            *State = KBC_PORT64_WRITE;

ON_EfiVirtualStatusEXIT:
        break;

    case EfiVirtualClearStatus:
        Value &= USBLEGACY_SMIEN | USBLEGACY_A20PASSEN | USBLEGACY_SMIATENDPS
                 | USBLEGACY_USBPIRQEN | USBLEGACY_TRAPBY60R | USBLEGACY_TRAPBY60W
                 | USBLEGACY_TRAPBY64R | USBLEGACY_TRAPBY64W;
        break;

    default:
        break;
    }

    SmmPciCfgWrite(
        gSmst,
        SmmPciWidthUint16,
        (SMM_PCI_IO_ADDRESS *) &PciAddress,
        &Value
    );
    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
UhciReset (
    IN EFI_USB_HC_PROTOCOL     *This,
    IN UINT16                  Attributes
)
/*++

  Routine Description:
    Provides software reset for the USB host controller.

  Arguments:
    This      : A pointer to the EFI_USB_HC_PROTOCOL instance.
    Attributes: A bit mask of the reset operation to perform.

  Returns:
    EFI_SUCCESS           : The reset operation succeeded.
    EFI_INVALID_PARAMETER : Attributes is not valid.
    EFI_DEVICE_ERROR      : An error was encountered while attempting
                            to perform the reset operation.
--*/
{
    USB_HC_DEV          *Uhc;
    Uhc     = UHC_FROM_USB_HC_PROTO (This);

    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_IO_BUS_USB | EFI_IOB_PC_RESET));

    switch (Attributes) {
    case EFI_USB_HC_RESET_GLOBAL:
        //
        // Stop schedule and set the Global Reset bit in the command register
        //
        UhciStopHc (Uhc, UHC_GENERIC_TIMEOUT);
        UhciSetRegBit (Uhc, USBCMD_OFFSET, USBCMD_GRESET);

        //Notice: Remove for Reduce LegacyUsb Post time Stall (UHC_ROOT_PORT_RESET_STALL);
        Stall (10* UHC_1_MILLISECOND); //Notice: Add for Reduce LegacyUsb Post time

        //
        // Clear the Global Reset bit to zero.
        //
        UhciClearRegBit (Uhc, USBCMD_OFFSET, USBCMD_GRESET);

        Stall (UHC_ROOT_PORT_RECOVERY_STALL);
        break;

    case EFI_USB_HC_RESET_HOST_CONTROLLER:
        //
        // Stop schedule and set Host Controller Reset bit to 1
        //
        UhciStopHc (Uhc, UHC_GENERIC_TIMEOUT);
        UhciSetRegBit (Uhc, USBCMD_OFFSET, USBCMD_HCRESET);

        Stall (UHC_ROOT_PORT_RECOVERY_STALL);
        break;

    default:
        goto ON_INVAILD_PARAMETER;
    }

    //
    // Delete all old transactions on the USB bus, then
    // reinitialize the frame list
    //
    UhciFreeAllAsyncReq (Uhc);
    UhciDestoryFrameList (Uhc);
    UhciInitFrameList (Uhc);

    return EFI_SUCCESS;

ON_INVAILD_PARAMETER:

    return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
UhciGetState (
    IN  EFI_USB_HC_PROTOCOL     *This,
    OUT EFI_USB_HC_STATE        *State
)
/*++

  Routine Description:
    Retrieves current state of the USB host controller.

  Arguments:
    This    :  A pointer to the EFI_USB_HC_PROTOCOL instance.
    State   :  A pointer to the EFI_USB_HC_STATE data structure that
               indicates current state of the USB host controller.
  Returns:
    EFI_SUCCESS           : State was returned
    EFI_INVALID_PARAMETER : State is NULL.
    EFI_DEVICE_ERROR      : An error was encountered

--*/
{
    USB_HC_DEV          *Uhc;
    UINT16              UsbSts;
    UINT16              UsbCmd;

    if (State == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    Uhc     = UHC_FROM_USB_HC_PROTO (This);

    UsbCmd  = UhciReadReg (Uhc, USBCMD_OFFSET);
    UsbSts  = UhciReadReg (Uhc, USBSTS_OFFSET);

    if (UsbCmd & USBCMD_EGSM) {
        *State = EfiUsbHcStateSuspend;

    } else if ((UsbSts & USBSTS_HCH) != 0) {
        *State = EfiUsbHcStateHalt;

    } else {
        *State = EfiUsbHcStateOperational;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UhciSetState (
    IN EFI_USB_HC_PROTOCOL     *This,
    IN EFI_USB_HC_STATE        State
)
/*++

  Routine Description:
    Sets the USB host controller to a specific state.

  Arguments:
    This     : A pointer to the EFI_USB_HC_PROTOCOL instance.
    State    : Indicates the state of the host controller that will be set.

  Returns:
    EFI_SUCCESS           : The USB host controller was successfully set
    EFI_INVALID_PARAMETER : State is invalid.
    EFI_DEVICE_ERROR      : Failed to set the state specified

--*/
{
    EFI_USB_HC_STATE    CurState;
    USB_HC_DEV          *Uhc;
    EFI_STATUS          Status;
    UINT16              UsbCmd;

    Uhc     = UHC_FROM_USB_HC_PROTO (This);
    Status  = UhciGetState (This, &CurState);

    if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
    }

    if (CurState == State) {
        return EFI_SUCCESS;
    }

    Status  = EFI_SUCCESS;

    switch (State) {
    case EfiUsbHcStateHalt:
        Status = UhciStopHc (Uhc, UHC_GENERIC_TIMEOUT);
        break;

    case EfiUsbHcStateOperational:
        UsbCmd = UhciReadReg (Uhc, USBCMD_OFFSET);

        if (CurState == EfiUsbHcStateHalt) {
            //
            // Set Run/Stop bit to 1, also set the bandwidht reclamation
            // point to 64 bytes
            //
            UsbCmd |= USBCMD_RS | USBCMD_MAXP | USBCMD_CF;
            UhciWriteReg (Uhc, USBCMD_OFFSET, UsbCmd);

        } else if (CurState == EfiUsbHcStateSuspend) {
            //
            // If FGR(Force Global Resume) bit is 0, set it
            //
            if ((UsbCmd & USBCMD_FGR) == 0) {
                UsbCmd |= USBCMD_FGR;
                UhciWriteReg (Uhc, USBCMD_OFFSET, UsbCmd);
            }

            //
            // wait 20ms to let resume complete (20ms is specified by UHCI spec)
            //
            Stall (UHC_FORCE_GLOBAL_RESUME_STALL);

            //
            // Write FGR bit to 0 and EGSM(Enter Global Suspend Mode) bit to 0
            //
            UsbCmd &= ~USBCMD_FGR;
            UsbCmd &= ~USBCMD_EGSM;
            UsbCmd |= (USBCMD_RS|USBCMD_CF);
            UhciWriteReg (Uhc, USBCMD_OFFSET, UsbCmd);
        }

        break;

    case EfiUsbHcStateSuspend:
        Status = UhciSetState (This, EfiUsbHcStateHalt);

        if (EFI_ERROR (Status)) {
            Status = EFI_DEVICE_ERROR;
            goto ON_EXIT;
        }

        //
        // Set Enter Global Suspend Mode bit to 1.
        //
        UsbCmd = UhciReadReg (Uhc, USBCMD_OFFSET);
        UsbCmd |= USBCMD_EGSM;
        UhciWriteReg (Uhc, USBCMD_OFFSET, UsbCmd);
        break;

    default:
        Status = EFI_INVALID_PARAMETER;
        break;
    }

ON_EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
UhciGetRootHubPortNumber (
    IN  EFI_USB_HC_PROTOCOL     *This,
    OUT UINT8                   *PortNumber
)
/*++

  Routine Description:

    Retrieves the number of root hub ports.

  Arguments:

    This       : A pointer to the EFI_USB_HC_PROTOCOL instance.
    PortNumber : A pointer to the number of the root hub ports.

  Returns:

    EFI_SUCCESS           : The port number was retrieved successfully.
    EFI_INVALID_PARAMETER : PortNumber is NULL.
    EFI_DEVICE_ERROR      : An error was encountered

--*/
{
    USB_HC_DEV          *Uhc;
    UINT32              Offset;
    UINT16              PortSC;
    UINT32              Index;

    Uhc = UHC_FROM_USB_HC_PROTO (This);

    if (PortNumber == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    *PortNumber = 0;

    for (Index = 0; Index < USB_MAX_ROOTHUB_PORT; Index++) {
        Offset  = USBPORTSC_OFFSET + Index * 2;
        PortSC  = UhciReadReg (Uhc, Offset);

        //
        // Port status's bit 7 is reserved and always returns 1 if
        // the port number is valid. Intel's UHCI (in EHCI controller)
        // returns 0 in this bit if port number is invalid. Also, if
        // PciIo IoRead returns error, 0xFFFF is returned to caller.
        //
        if (((PortSC & 0x80) == 0) || (PortSC == 0xFFFF)) {
            break;
        }
        (*PortNumber)++;
    }

    Uhc->RootPorts = *PortNumber;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UhciGetRootHubPortStatus (
    IN  EFI_USB_HC_PROTOCOL     *This,
    IN  UINT8                   PortNumber,
    OUT EFI_USB_PORT_STATUS     *PortStatus
)
/*++

  Routine Description:

    Retrieves the current status of a USB root hub port.

  Arguments:

    This        : A pointer to the EFI_USB_HC_PROTOCOL.
    PortNumber  : Specifies the root hub port. This value is zero-based.
    PortStatus  : A pointer to the current port status bits and port status change bits.

  Returns:

    EFI_SUCCESS           : The port status was returned in PortStatus.
    EFI_INVALID_PARAMETER : PortNumber is invalid.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
    USB_HC_DEV          *Uhc;
    UINT32              Offset;
    UINT16              PortSC;

    Uhc = UHC_FROM_USB_HC_PROTO (This);

    if (PortStatus == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (PortNumber >= Uhc->RootPorts) {
        return EFI_INVALID_PARAMETER;
    }

    Offset                        = USBPORTSC_OFFSET + PortNumber * 2;
    PortStatus->PortStatus        = 0;
    PortStatus->PortChangeStatus  = 0;

    PortSC                        = UhciReadReg (Uhc, Offset);

    if (PortSC & USBPORTSC_CCS) {
        PortStatus->PortStatus |= USB_PORT_STAT_CONNECTION;
    }

    if (PortSC & USBPORTSC_PED) {
        PortStatus->PortStatus |= USB_PORT_STAT_ENABLE;
    }

    if (PortSC & USBPORTSC_SUSP) {
        PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
    }

    if (PortSC & USBPORTSC_PR) {
        PortStatus->PortStatus |= USB_PORT_STAT_RESET;
    }

    if (PortSC & USBPORTSC_LSDA) {
        PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
    }

    //
    // CHC will always return one in port owner bit
    //
    PortStatus->PortStatus |= USB_PORT_STAT_OWNER;

    if (PortSC & USBPORTSC_CSC) {
        PortStatus->PortChangeStatus |= USB_PORT_STAT_C_CONNECTION;
    }

    if (PortSC & USBPORTSC_PEDC) {
        PortStatus->PortChangeStatus |= USB_PORT_STAT_C_ENABLE;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UhciSetRootHubPortFeature (
    IN  EFI_USB_HC_PROTOCOL     *This,
    IN  UINT8                   PortNumber,
    IN  EFI_USB_PORT_FEATURE    PortFeature
)
/*++

  Routine Description:

    Sets a feature for the specified root hub port.

  Arguments:

    This        : A pointer to the EFI_USB_HC_PROTOCOL.
    PortNumber  : Specifies the root hub port whose feature
                  is requested to be set.
    PortFeature : Indicates the feature selector associated
                  with the feature set request.

  Returns:

    EFI_SUCCESS           : The feature was set for the port.
    EFI_INVALID_PARAMETER : PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
    USB_HC_DEV          *Uhc;
    UINT32              Offset;
    UINT16              PortSC;
    UINT16              Command;

    Uhc = UHC_FROM_USB_HC_PROTO (This);

    if (PortNumber >= Uhc->RootPorts) {
        return EFI_INVALID_PARAMETER;
    }

    Offset  = USBPORTSC_OFFSET + PortNumber * 2;

    PortSC  = UhciReadReg (Uhc, Offset);

    switch (PortFeature) {
    case EfiUsbPortSuspend:
        Command = UhciReadReg (Uhc, USBCMD_OFFSET);
        if (!(Command & USBCMD_EGSM)) {
            //
            // if global suspend is not active, can set port suspend
            //
            PortSC &= 0xfff5;
            PortSC |= USBPORTSC_SUSP;
        }
        break;

    case EfiUsbPortReset:
        PortSC &= 0xfff5;
        PortSC |= USBPORTSC_PR;
        break;

    case EfiUsbPortPower:
        //
        // No action
        //
        break;

    case EfiUsbPortEnable:
        PortSC &= 0xfff5;
        PortSC |= USBPORTSC_PED;
        break;

    default:
        return EFI_INVALID_PARAMETER;
    }

    UhciWriteReg (Uhc, Offset, PortSC);

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UhciClearRootHubPortFeature (
    IN  EFI_USB_HC_PROTOCOL     *This,
    IN  UINT8                   PortNumber,
    IN  EFI_USB_PORT_FEATURE    PortFeature
)
/*++

  Routine Description:

    Clears a feature for the specified root hub port.

  Arguments:

    This        : A pointer to the EFI_USB_HC_PROTOCOL instance.
    PortNumber  : Specifies the root hub port whose feature
                  is requested to be cleared.
    PortFeature : Indicates the feature selector associated with the
                  feature clear request.

  Returns:

    EFI_SUCCESS           : The feature was cleared for the port.
    EFI_INVALID_PARAMETER : PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
    USB_HC_DEV          *Uhc;
    UINT32              Offset;
    UINT16              PortSC;

    Uhc = UHC_FROM_USB_HC_PROTO (This);

    if (PortNumber >= Uhc->RootPorts) {
        return EFI_INVALID_PARAMETER;
    }

    Offset  = USBPORTSC_OFFSET + PortNumber * 2;

    PortSC  = UhciReadReg (Uhc, Offset);

    switch (PortFeature) {
    case EfiUsbPortEnable:
        PortSC &= 0xfff5;
        PortSC &= ~USBPORTSC_PED;
        break;

    case EfiUsbPortSuspend:
        //
        // Cause a resume on the specified port if in suspend mode.
        //
        PortSC &= 0xfff5;
        PortSC &= ~USBPORTSC_SUSP;
        break;

    case EfiUsbPortPower:
        //
        // No action
        //
        break;

    case EfiUsbPortReset:
        PortSC &= 0xfff5;
        PortSC &= ~USBPORTSC_PR;
        break;

    case EfiUsbPortConnectChange:
        PortSC &= 0xfff5;
        PortSC |= USBPORTSC_CSC;
        break;

    case EfiUsbPortEnableChange:
        PortSC &= 0xfff5;
        PortSC |= USBPORTSC_PEDC;
        break;

    case EfiUsbPortSuspendChange:
        //
        // Root hub does not support this
        //
        break;

    case EfiUsbPortOverCurrentChange:
        //
        // Root hub does not support this
        //
        break;

    case EfiUsbPortResetChange:
        //
        // Root hub does not support this
        //
        break;

    default:
        return EFI_INVALID_PARAMETER;
    }

    UhciWriteReg (Uhc, Offset, PortSC);


    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UhciControlTransfer (
    IN       EFI_USB_HC_PROTOCOL        *This,
    IN       UINT8                      DeviceAddress,
    IN       BOOLEAN                    IsSlowDevice,
    IN       UINT8                      MaximumPacketLength,
    IN       EFI_USB_DEVICE_REQUEST     *Request,
    IN       EFI_USB_DATA_DIRECTION     TransferDirection,
    IN OUT   VOID                       *Data,              OPTIONAL
    IN OUT   UINTN                      *DataLength,        OPTIONAL
    IN       UINTN                      TimeOut,
    OUT      UINT32                     *TransferResult
)
/*++

  Routine Description:

    Submits control transfer to a target USB device.

  Arguments:

    This                : A pointer to the EFI_USB_HC_PROTOCOL instance.
    DeviceAddress       : Usb device address
    IsSlowDevice        : Whether the device is of slow speed or full speed
    MaximumPacketLength : maximum packet size of the default control endpoint
    Request             : USB device request to send
    TransferDirection   : Specifies the data direction for the transfer.
    Data                : Data buffer to transmit from or receive into
    DataLength          : Number of bytes of the data
    TimeOut             : Maximum time, in microseconds
    TransferResult      : Return result in this

  Returns:
    EFI_SUCCESS           : Transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : Failed due to a lack of resources.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : Failed due to timeout.
    EFI_DEVICE_ERROR      : Failed due to host controller or device error.
--*/
{
    USB_HC_DEV              *Uhc;
    UHCI_TD_SW              *TDs;
    EFI_STATUS              Status;
    UHCI_QH_RESULT          QhResult;
    UINT8                   PktId;
    UINT8                   *DataPhy;

    Uhc         = UHC_FROM_USB_HC_PROTO (This);
    TDs         = NULL;
    DataPhy     = NULL;

    //
    // Parameters Checking
    //
    if (Request == NULL || TransferResult == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (IsSlowDevice && (MaximumPacketLength != 8)) {
        return EFI_INVALID_PARAMETER;
    }

    if ((MaximumPacketLength != 8) &&  (MaximumPacketLength != 16) &&
            (MaximumPacketLength != 32) && (MaximumPacketLength != 64)) {

        return EFI_INVALID_PARAMETER;
    }

    if ((TransferDirection != EfiUsbNoData) && (Data == NULL || DataLength == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;

    //
    // If errors exist that cause host controller halt,
    // clear status then return EFI_DEVICE_ERROR.
    //
    UhciAckAllInterrupt (Uhc);

    if (!UhciIsHcWorking (Uhc)) {
        return EFI_DEVICE_ERROR;
    }

    //
    // Map the Request and data for bus master access,
    // then create a list of TD for this transfer
    //
    Status = UhciMapUserData (Uhc, TransferDirection, Data, DataLength, &PktId, &DataPhy);

    if (EFI_ERROR (Status)) {
        goto ON_EXIT;
    }

    TDs = UhciCreateCtrlTds (
              Uhc,
              DeviceAddress,
              PktId,
              (UINT8 *)Request,
              DataPhy,
              *DataLength,
              MaximumPacketLength,
              IsSlowDevice
          );

    if (TDs == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
    }

    //
    // According to the speed of the end point, link
    // the TD to corrosponding queue head, then check
    // the execution result
    //
    UhciLinkTdToQh (Uhc->CtrlQh, TDs);
    Status = UhciExecuteTransfer (Uhc, Uhc->CtrlQh, TDs, TimeOut, IsSlowDevice, &QhResult);
    UhciUnlinkTdFromQh (Uhc->CtrlQh, TDs);

    *TransferResult = QhResult.Result;

    if (DataLength != NULL) {
        *DataLength = QhResult.Complete;
    }

    UhciDestoryTds (Uhc, TDs);

ON_EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
UhciBulkTransfer (
    IN       EFI_USB_HC_PROTOCOL     *This,
    IN       UINT8                   DeviceAddress,
    IN       UINT8                   EndPointAddress,
    IN       UINT8                   MaximumPacketLength,
    IN OUT   VOID                    *Data,
    IN OUT   UINTN                   *DataLength,
    IN OUT   UINT8                   *DataToggle,
    IN       UINTN                   TimeOut,
    OUT      UINT32                  *TransferResult
)
/*++

  Routine Description:

    Submits bulk transfer to a bulk endpoint of a USB device.

  Arguments:

    This                :A pointer to the EFI_USB_HC_PROTOCOL instance.
    DeviceAddress       : Usb device address
    EndPointAddress     : Endpoint number and endpoint direction
    MaximumPacketLength : Maximum packet size of the target endpoint
    Data                : Data buffer to transmit from or receive into
    DataLength          : Length of the data buffer
    DataToggle          : On input, data toggle to use, on output, the next toggle
    TimeOut             : Indicates the maximum time
    TransferResult      : Variable to receive the transfer result

  Returns:
    EFI_SUCCESS           : The bulk transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : Failed due to lack of resource.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : Failed due to timeout.
    EFI_DEVICE_ERROR      : Failed due to host controller or device error.

--*/
{
    EFI_USB_DATA_DIRECTION  Direction;
    USB_HC_DEV              *Uhc;
    UHCI_TD_SW              *TDs;
    UHCI_QH_SW              *BulkQh;
    UHCI_QH_RESULT          QhResult;
    EFI_STATUS              Status;
    UINT8                   PktId;
    UINT8                   *DataPhy;

    Uhc     = UHC_FROM_USB_HC_PROTO (This);
    DataPhy = NULL;

    if ((DataLength == NULL) || (Data == NULL) || (TransferResult == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    if (*DataLength == 0) {
        return EFI_INVALID_PARAMETER;
    }

    if ((*DataToggle != 1) && (*DataToggle != 0)) {
        return EFI_INVALID_PARAMETER;
    }

    if ((MaximumPacketLength != 8) && (MaximumPacketLength != 16) &&
            (MaximumPacketLength != 32) && (MaximumPacketLength != 64)) {
        return EFI_INVALID_PARAMETER;
    }

    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_OUT_OF_RESOURCES;

    //
    // If has errors that cause host controller halt,
    // then return EFI_DEVICE_ERROR directly.
    //
    UhciAckAllInterrupt (Uhc);

    if (!UhciIsHcWorking (Uhc)) {
        return EFI_DEVICE_ERROR;
    }

    //
    // Map the source data buffer for bus master access,
    // then create a list of TDs
    //
    if (EndPointAddress & 0x80) {
        Direction = EfiUsbDataIn;
    } else {
        Direction = EfiUsbDataOut;
    }

    Status = UhciMapUserData (Uhc, Direction, Data, DataLength, &PktId, &DataPhy);

    if (EFI_ERROR (Status)) {
        goto ON_EXIT;
    }

    Status = EFI_OUT_OF_RESOURCES;
    TDs    = UhciCreateBulkOrIntTds (
                 Uhc,
                 DeviceAddress,
                 EndPointAddress,
                 PktId,
                 DataPhy,
                 *DataLength,
                 DataToggle,
                 MaximumPacketLength,
                 FALSE,
                 FALSE
             );

    if (TDs == NULL) {
        goto ON_EXIT;
    }

    //
    // Link the TDs to bulk queue head. According to the platfore
    // defintion of UHCI_NO_BW_RECLAMATION, BulkQh is either configured
    // to do full speed bandwidth reclamation or not.
    //
    BulkQh = Uhc->BulkQh;

    UhciLinkTdToQh (BulkQh, TDs);
    Status = UhciExecuteTransfer (Uhc, BulkQh, TDs, TimeOut, FALSE, &QhResult);
    UhciUnlinkTdFromQh (BulkQh, TDs);


    *TransferResult = QhResult.Result;
    *DataToggle     = QhResult.NextToggle;
    *DataLength     = QhResult.Complete;

    UhciDestoryTds (Uhc, TDs);

ON_EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
UhciAsyncInterruptOnlyTransfer (
    IN     EFI_USB_HC_PROTOCOL                * This,
    IN     BOOLEAN                            IsNewTransfer,
    IN     UINTN                              PollingInterval,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
    IN     VOID                               *Context
)
/*++

  Routine Description:

    Submits an asynchronous interrupt transfer to an interrupt endpoint of a USB device.

  Arguments:

    This                : A pointer to the EFI_USB_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    EndPointAddress     : Endpoint number with direction
    IsSlowDevice        : Whether the target device is slow device or full-speed device.
    MaximumPacketLength : Maximum packet size of the target endpoint
    IsNewTransfer       : If TRUE, submit a new async interrupt transfer, otherwise
                          cancel an existed one
    DataToggle          : On input, the data toggle to use; On output, next data toggle
    PollingInterval     : Interrupt poll rate in milliseconds
    DataLength          : Length of data to receive
    CallBackFunction    : Function to call periodically
    Context             : User context

  Returns:

    EFI_SUCCESS           : Request is submitted or cancelled
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_OUT_OF_RESOURCES  : Failed due to a lack of resources.
    EFI_DEVICE_ERROR      : Failed to due to device error

--*/
{
    USB_HC_DEV          *Uhc;
    UHCI_QH_SW          *Qh;
    UHCI_TD_SW          *IntTds;
    EFI_STATUS          Status;
    UINT8               DataToggle;
    UINTN               AdjustedPollingInterval;

    Uhc        = UHC_FROM_USB_HC_PROTO (This);
    Qh         = NULL;
    IntTds     = NULL;
    DataToggle = 0;

    //
    // Delete Async interrupt transfer request
    //
    if (!IsNewTransfer) {
        Status = UhciRemoveAsyncReq (Uhc, 0, 0,
                                     CallBackFunction, Context, &DataToggle);
        return Status;
    }

    AdjustedPollingInterval = UhciConvertPollRate (PollingInterval);
    //
    // If has errors that cause host controller halt,
    // then return EFI_DEVICE_ERROR directly.
    //
    UhciAckAllInterrupt (Uhc);

    if (!UhciIsHcWorking (Uhc)) {
        return EFI_DEVICE_ERROR;
    }

    Qh = UhciCreateQh (Uhc, AdjustedPollingInterval);

    if (Qh == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto UNMAP_DATA;
    }

    IntTds = UhciCreateDataTd (
                 Uhc,
                 0,
                 0,
                 NULL,
                 0,
                 INPUT_PACKET_ID,
                 DataToggle,
                 TRUE
             );

    if (IntTds == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DESTORY_QH;
    }

    IntTds->TdHw.ErrorCount = 1;
    IntTds->TdHw.IntOnCpl = TRUE;

    UhciLinkTdToQh (Qh, IntTds);

    //
    // Save QH-TD structures to async Interrupt transfer list,
    // for monitor interrupt transfer execution routine use.
    //
    Status = UhciCreateAsyncReq (
                 Uhc,
                 Qh,
                 IntTds,
                 0,
                 0,
                 0,
                 AdjustedPollingInterval,
                 NULL,
                 CallBackFunction,
                 Context,
                 TRUE
             );

    if (EFI_ERROR (Status)) {
        goto DESTORY_QH;
    }

    UhciLinkQhToFrameList (Uhc->FrameBase, Qh);

    return EFI_SUCCESS;

DESTORY_QH:
    UsbFreePool (Qh, sizeof (UHCI_QH_SW));

UNMAP_DATA:
    return Status;
}

EFI_STATUS
EFIAPI
UhciAsyncInterruptTransfer (
    IN     EFI_USB_HC_PROTOCOL                * This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     BOOLEAN                            IsSlowDevice,
    IN     UINT8                              MaximumPacketLength,
    IN     BOOLEAN                            IsNewTransfer,
    IN OUT UINT8                              *DataToggle,
    IN     UINTN                              PollingInterval,    OPTIONAL
    IN     UINTN                              DataLength,         OPTIONAL
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,   OPTIONAL
    IN     VOID                               *Context OPTIONAL
)
/*++

  Routine Description:

    Submits an asynchronous interrupt transfer to an interrupt endpoint of a USB device.

  Arguments:

    This                : A pointer to the EFI_USB_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    EndPointAddress     : Endpoint number with direction
    IsSlowDevice        : Whether the target device is slow device or full-speed device.
    MaximumPacketLength : Maximum packet size of the target endpoint
    IsNewTransfer       : If TRUE, submit a new async interrupt transfer, otherwise
                          cancel an existed one
    DataToggle          : On input, the data toggle to use; On output, next data toggle
    PollingInterval     : Interrupt poll rate in milliseconds
    DataLength          : Length of data to receive
    CallBackFunction    : Function to call periodically
    Context             : User context

  Returns:

    EFI_SUCCESS           : Request is submitted or cancelled
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_OUT_OF_RESOURCES  : Failed due to a lack of resources.
    EFI_DEVICE_ERROR      : Failed to due to device error

--*/
{
    USB_HC_DEV          *Uhc;
    UHCI_QH_SW          *Qh;
    UHCI_TD_SW          *IntTds;
    EFI_STATUS          Status;
    UINT8               *DataPtr;
    UINT8               *DataPhy;
    UINT8               PktId;
    UINTN               AdjustedPollingInterval;

    Uhc       = UHC_FROM_USB_HC_PROTO (This);
    Qh        = NULL;
    IntTds    = NULL;
    DataPtr   = NULL;
    DataPhy   = NULL;

    if ((EndPointAddress & 0x80) == 0) {
        return EFI_INVALID_PARAMETER;
    }

    //
    // Delete Async interrupt transfer request
    //
    if (!IsNewTransfer) {
        Status = UhciRemoveAsyncReq (Uhc, DeviceAddress, EndPointAddress,
                                     CallBackFunction, Context, DataToggle);

        return Status;
    }

    if (PollingInterval < 1 || PollingInterval > 255) {
        return EFI_INVALID_PARAMETER;
    }

    if (DataLength == 0) {
        return EFI_INVALID_PARAMETER;
    }

    if ((*DataToggle != 1) && (*DataToggle != 0)) {
        return EFI_INVALID_PARAMETER;
    }

    AdjustedPollingInterval = UhciConvertPollRate (PollingInterval);
    //
    // If has errors that cause host controller halt,
    // then return EFI_DEVICE_ERROR directly.
    //
    UhciAckAllInterrupt (Uhc);

    if (!UhciIsHcWorking (Uhc)) {
        return EFI_DEVICE_ERROR;
    }

    //
    // Allocate and map source data buffer for bus master access.
    //
    DataPtr = UsbAllocatePool (DataLength);

    if (DataPtr == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    //
    // Map the user data then create a queue head and
    // list of TD for it.
    //
    Status = UhciMapUserData (
                 Uhc,
                 EfiUsbDataIn,
                 DataPtr,
                 &DataLength,
                 &PktId,
                 &DataPhy
             );

    if (EFI_ERROR (Status)) {
        goto FREE_DATA;
    }

    Qh = UhciCreateQh (Uhc, AdjustedPollingInterval);

    if (Qh == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto UNMAP_DATA;
    }

    IntTds = UhciCreateBulkOrIntTds (
                 Uhc,
                 DeviceAddress,
                 EndPointAddress,
                 PktId,
                 DataPhy,
                 DataLength,
                 DataToggle,
                 MaximumPacketLength,
                 IsSlowDevice,
                 TRUE
             );

    if (IntTds == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DESTORY_QH;
    }

    UhciLinkTdToQh (Qh, IntTds);

    //
    // Save QH-TD structures to async Interrupt transfer list,
    // for monitor interrupt transfer execution routine use.
    //
    Status = UhciCreateAsyncReq (
                 Uhc,
                 Qh,
                 IntTds,
                 DeviceAddress,
                 EndPointAddress,
                 DataLength,
                 AdjustedPollingInterval,
                 DataPtr,
                 CallBackFunction,
                 Context,
                 IsSlowDevice
             );

    if (EFI_ERROR (Status)) {
        goto DESTORY_QH;
    }

    UhciLinkQhToFrameList (Uhc->FrameBase, Qh);

    return EFI_SUCCESS;

DESTORY_QH:
    UsbFreePool (Qh, sizeof (UHCI_QH_SW));

UNMAP_DATA:

FREE_DATA:
    UsbFreePool (DataPtr, DataLength);
    return Status;
}

EFI_STATUS
EFIAPI
UhciSyncInterruptTransfer (
    IN       EFI_USB_HC_PROTOCOL     *This,
    IN       UINT8                   DeviceAddress,
    IN       UINT8                   EndPointAddress,
    IN       BOOLEAN                 IsSlowDevice,
    IN       UINT8                   MaximumPacketLength,
    IN OUT   VOID                    *Data,
    IN OUT   UINTN                   *DataLength,
    IN OUT   UINT8                   *DataToggle,
    IN       UINTN                   TimeOut,
    OUT      UINT32                  *TransferResult
)
/*++

  Routine Description:

    Submits synchronous interrupt transfer to an interrupt endpoint of a USB device.

  Arguments:

    This                : A pointer to the EFI_USB_HC_PROTOCOL instance.
    DeviceAddress       : Device address of the target USB device
    EndPointAddress     : Endpoint number and direction
    IsSlowDevice        : Whether the target device is of slow speed or full speed
    MaximumPacketLength : Maximum packet size of target endpoint
    Data                : Data to transmit or receive
    DataLength          : On input, data length to transmit or buffer size.
                          On output, the number of bytes transferred.
    DataToggle          : On input, data toggle to use; On output, next data toggle
    TimeOut             : Maximum time, in microseconds, transfer is allowed to complete.
    TransferResult      : Variable to receive transfer result

  Returns:
    EFI_SUCCESS           : Transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : Failed due to lack of resource.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : Failed due to timeout.
    EFI_DEVICE_ERROR      : Failed due to host controller or device error

--*/
{
    EFI_STATUS          Status;
    USB_HC_DEV          *Uhc;
    UHCI_TD_SW          *TDs;
    UHCI_QH_RESULT      QhResult;
    UINT8               *DataPhy;
    VOID                *DataMap;
    UINT8               PktId;

    Uhc     = UHC_FROM_USB_HC_PROTO (This);
    DataPhy = NULL;
    DataMap = NULL;
    TDs     = NULL;

    if ((DataLength == NULL) || (Data == NULL) || (TransferResult == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    if ((EndPointAddress & 0x80) == 0) {
        return EFI_INVALID_PARAMETER;
    }

    if ((*DataToggle != 1) && (*DataToggle != 0)) {
        return EFI_INVALID_PARAMETER;
    }

    if ((*DataLength == 0) || (MaximumPacketLength > 64)) {
        return EFI_INVALID_PARAMETER;
    }

    if (IsSlowDevice && (MaximumPacketLength > 8)) {
        return EFI_INVALID_PARAMETER;
    }

    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;


    UhciAckAllInterrupt (Uhc);

    if (!UhciIsHcWorking (Uhc)) {
        return Status;
    }

    //
    // Map the source data buffer for bus master access.
    // Create Tds list, then link it to the UHC's interrupt list
    //
    Status = UhciMapUserData (
                 Uhc,
                 EfiUsbDataIn,
                 Data,
                 DataLength,
                 &PktId,
                 &DataPhy
             );

    if (EFI_ERROR (Status)) {
        goto ON_EXIT;
    }

    TDs = UhciCreateBulkOrIntTds (
              Uhc,
              DeviceAddress,
              EndPointAddress,
              PktId,
              DataPhy,
              *DataLength,
              DataToggle,
              MaximumPacketLength,
              IsSlowDevice,
              FALSE
          );

    if (TDs == NULL) {

        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
    }


    UhciLinkTdToQh (Uhc->SyncIntQh, TDs);

    Status = UhciExecuteTransfer (Uhc, Uhc->SyncIntQh, TDs, TimeOut, IsSlowDevice, &QhResult);

    UhciUnlinkTdFromQh (Uhc->SyncIntQh, TDs);

    *TransferResult = QhResult.Result;
    *DataToggle     = QhResult.NextToggle;
    *DataLength     = QhResult.Complete;

    UhciDestoryTds (Uhc, TDs);

ON_EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
UhciIsochronousTransfer (
    IN       EFI_USB_HC_PROTOCOL     *This,
    IN       UINT8                   DeviceAddress,
    IN       UINT8                   EndPointAddress,
    IN       UINT8                   MaximumPacketLength,
    IN OUT   VOID                    *Data,
    IN       UINTN                   DataLength,
    OUT      UINT32                  *TransferResult
)
/*++

  Routine Description:

    Submits isochronous transfer to a target USB device.

  Arguments:

    This                : A pointer to the EFI_USB_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    EndPointAddress     : End point address withdirection
    MaximumPacketLength : Maximum packet size of the endpoint
    Data                : Data to transmit or receive
    DataLength          : Bytes of the data
    TransferResult      : Variable to receive the result

  Returns:

    EFI_UNSUPPORTED

--*/
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
UhciAsyncIsochronousTransfer (
    IN       EFI_USB_HC_PROTOCOL                 * This,
    IN       UINT8                               DeviceAddress,
    IN       UINT8                               EndPointAddress,
    IN       UINT8                               MaximumPacketLength,
    IN OUT   VOID                                *Data,
    IN       UINTN                               DataLength,
    IN       EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
    IN       VOID                                *Context OPTIONAL
)
/*++

  Routine Description:

    Submits Async isochronous transfer to a target USB device.

  Arguments:

    This                : A pointer to the EFI_USB_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    EndPointAddress     : End point address withdirection
    MaximumPacketLength : Maximum packet size of the endpoint
    Data                : Data to transmit or receive
    IsochronousCallBack : Function to call when the transfer completes
    Context             : User context

  Returns:

    EFI_UNSUPPORTED

--*/

{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
UhciClearSmi (
    IN       EFI_USB_HC_PROTOCOL                 * This
)
{
    USB_HC_DEV               *Uhc;
    UINTN                    Value;
    UINT64                   PciAddress;
    EFI_STATUS               Status;
    UINT16                   UsbStatus;

    Uhc = UHC_FROM_USB_HC_PROTO(This);

    //
    // Clear all the interrutp status bits in USB status regsiter. These bits
    // are Write-Clean
    //
    UsbStatus = UhciReadReg(Uhc, USBSTS_OFFSET);
    UhciWriteReg(Uhc, USBSTS_OFFSET, UsbStatus);

    //
    // Clear all the SMI status bits in legacy controller regsiter. These bits
    // are write-Clean
    PciAddress = SMM_PCI_ADDRESS (
                     Uhc->PciAddress.Bus,
                     Uhc->PciAddress.Device,
                     Uhc->PciAddress.Function,
                     USB_EMULATION_OFFSET);

    Status = SmmPciCfgRead (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Value
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }
    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Value
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
UhciRootHubPolling(
    IN VOID         *Data,
    IN UINTN        DataLength,
    IN VOID         *Context,
    IN UINT32       Status
)
{
  USB_HC_DEV                 *Uhc;
  EFI_USB2_HC_PROTOCOL       *Usb2Hc;
  LIST_ENTRY                 *Next;

  Next = mUhcList.BackLink;
  while (Next != &mUhcList) {
      Uhc = CR(Next, USB_HC_DEV, Link, USB_HC_DEV_SIGNATURE);
      Usb2Hc = &Uhc->Usb2Hc;
      Usb2Hc->BusEnumerationCallback (Usb2Hc->EnumerationContext);
      Next = Next->BackLink;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UhciDisableLegacySupport (
    IN       EFI_USB_HC_PROTOCOL                 * This
)
{
    USB_HC_DEV               *Uhc;
    UINTN                    Value;
    UINT64                   PciAddress;
    EFI_STATUS               Status;

    UHCI_DEBUG((EFI_D_INFO, "UhciDisableLegacySupport: called to disable legacy support\n"));

    Uhc = UHC_FROM_USB_HC_PROTO (This);

    //
    // Enable "A20Gate Pass-Through" and Disable "SMI On USB IRQ" in USB_LEGKEY
    //
    PciAddress = SMM_PCI_ADDRESS (
                     Uhc->PciAddress.Bus,
                     Uhc->PciAddress.Device,
                     Uhc->PciAddress.Function,
                     USB_EMULATION_OFFSET);
    Value = 0;

    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Value
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    //
    // Clear USBINTR to disable all of the interrupts
    //
    UhciWriteReg(Uhc, USBINTR_OFFSET, 0);
  /*
  if (PollingEvent) {
    //
    // Cancel the interrupt transfter to poll the status of root port
    //
    Status = UhciAsyncInterruptOnlyTransfer(
                 &Uhc->UsbHc,
                 FALSE,
                 1000,
                 UhciRootHubPolling,
                 Uhc);
    if (EFI_ERROR(Status)) {
        UHCI_DEBUG((EFI_D_INFO, "UhciDisableLegacySupport: Failed to cancel root hub polling transfer\n"));
    } else {
        UHCI_DEBUG((EFI_D_INFO, "UhciDisableLegacySupport: Succeed to cancel root hub polling transfer\n"));
    }
    PollingEvent = FALSE;
  }
  */
    return Status;
}

EFI_STATUS
EFIAPI
UhciEnableLegacySupport (
    IN       EFI_USB_HC_PROTOCOL                 * This
)
{
    USB_HC_DEV               *Uhc;
    UINTN                    Value;
    UINT64                   PciAddress;
    EFI_STATUS               Status;

    UHCI_DEBUG((EFI_D_INFO, "UhciEnableLegacySupport: called to enable legacy support\n"));
    Uhc = UHC_FROM_USB_HC_PROTO (This);

    //
    // Enable "A20Gate Pass-Through" and "SMI On USB IRQ" in USB_LEGKEY
    //
    PciAddress = SMM_PCI_ADDRESS (
                     Uhc->PciAddress.Bus,
                     Uhc->PciAddress.Device,
                     Uhc->PciAddress.Function,
                     USB_EMULATION_OFFSET);
    Value = USBLEGACY_SMIEN | USBLEGACY_A20PASSEN;
    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Value
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    //
    // Enable "IOC" in USBINTR
    //
    UhciWriteReg(Uhc, USBINTR_OFFSET, USBINTR_IOC);
  /*
  if (PollingEvent == FALSE) {
    //
    // Create one interrupt transfter to poll the status of root port
    //
    Status = UhciAsyncInterruptOnlyTransfer(
                 &Uhc->UsbHc,
                 TRUE,
                 1000,
                 UhciRootHubPolling,
                 Uhc);
    if (EFI_ERROR(Status)) {
        UHCI_DEBUG((EFI_D_INFO, "UhciEnableLegacySupport: Failed to create root hub polling transfer\n"));
    } else {
        UHCI_DEBUG((EFI_D_INFO, "UhciEnableLegacySupport: Succeed to create root hub polling transfer\n"));
    }
    PollingEvent = TRUE;
  }
  */
    return Status;
}

EFI_STATUS
EFIAPI
UhciLegacyCallback (
    IN       EFI_USB_HC_PROTOCOL                 * This
)
{
    USB_HC_DEV               *Uhc;
    UINT16                    UsbSts;
    UINT16                    UsbCmd;

    Uhc = UHC_FROM_USB_HC_PROTO (This);

    UsbSts = UhciReadReg (Uhc, USBSTS_OFFSET);
    UhciWriteReg (Uhc, USBSTS_OFFSET, UsbSts);
    UsbCmd = UhciReadReg (Uhc, USBCMD_OFFSET);

    if (UsbSts & (USBSTS_HSE)) {
        UHCI_DEBUG((EFI_D_INFO, "UhciSmiCallback: host system error occured \n"));
    }

    if (UsbSts & (USBSTS_HCPE)) {
        UHCI_DEBUG((EFI_D_INFO, "UhciSmiCallback: host controller process error occured\n"));
    }

    if (UsbSts & (USBSTS_USBINT)) {
        UhciProcessAsyncReqList (Uhc);
    }

    if (!(UsbCmd & (USBCMD_CF))) {
//        mUhciOSRequestOwnership = TRUE;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Uhci2Reset (
    IN EFI_USB2_HC_PROTOCOL   *This,
    IN UINT16                 Attributes
)
/*++

  Routine Description:

    Provides software reset for the USB host controller according to UEFI 2.0 spec.

  Arguments:

    This           - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    Attributes     - A bit mask of the reset operation to perform.
                       See below for a list of the supported bit mask values.
  Returns:

    EFI_SUCCESS           : The reset operation succeeded.
    EFI_INVALID_PARAMETER : Attributes is not valid.
    EFI_UNSUPPORTED       : This type of reset is not currently supported
    EFI_DEVICE_ERROR      : Other errors
--*/
{
    USB_HC_DEV          *UhciDev;

    UhciDev = UHC_FROM_USB2_HC_PROTO (This);

    if ((Attributes == EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG) ||
            (Attributes == EFI_USB_HC_RESET_HOST_WITH_DEBUG)) {
        return EFI_UNSUPPORTED;
    }

    return UhciReset (&UhciDev->UsbHc, Attributes);
}

EFI_STATUS
EFIAPI
Uhci2GetState (
    IN  EFI_USB2_HC_PROTOCOL   *This,
    OUT EFI_USB_HC_STATE       *State
)
/*++

  Routine Description:

    Retrieves current state of the USB host controller according to UEFI 2.0 spec.

  Arguments:

    This     - A pointer to the EFI_USB_HC_PROTOCOL instance.
    State    - Variable to receive current device state

  Returns:

    EFI_SUCCESS           : The state is returned
    EFI_INVALID_PARAMETER : State is not valid.
    EFI_DEVICE_ERROR      : Other errors

--*/
{
    USB_HC_DEV          *Uhc;

    Uhc = UHC_FROM_USB2_HC_PROTO (This);
    return UhciGetState (&Uhc->UsbHc, State);
}

EFI_STATUS
EFIAPI
Uhci2SetState (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN EFI_USB_HC_STATE        State
)
/*++

  Routine Description:

    Sets the USB host controller to a specific state according to UEFI 2.0 spec.

  Arguments:

    This     - A pointer to the EFI_USB_HC_PROTOCOL instance.
    State    - Indicates the state of the host controller that will be set.

  Returns:

    EFI_SUCCESS           : Host controller was successfully placed in the state
    EFI_INVALID_PARAMETER : State is invalid.
    EFI_DEVICE_ERROR      : Failed to set the state

--*/
{
    USB_HC_DEV          *Uhc;

    Uhc = UHC_FROM_USB2_HC_PROTO (This);
    return UhciSetState (&Uhc->UsbHc, State);
}

EFI_STATUS
EFIAPI
Uhci2GetCapability (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT UINT8                 *MaxSpeed,
    OUT UINT8                 *PortNumber,
    OUT UINT8                 *Is64BitCapable
)
/*++

  Routine Description:

    Retrieves capabilities of USB host controller according to UEFI 2.0 spec.

  Arguments:

    This           - A pointer to the EFI_USB2_HC_PROTOCOL instance
    MaxSpeed       - A pointer to the max speed USB host controller supports.
    PortNumber     - A pointer to the number of root hub ports.
    Is64BitCapable - A pointer to an integer to show whether USB host controller
                     supports 64-bit memory addressing.
  Returns:

    EFI_SUCCESS           : capabilities were retrieved successfully.
    EFI_INVALID_PARAMETER : MaxSpeed or PortNumber or Is64BitCapable is NULL.
    EFI_DEVICE_ERROR      : An error was encountered

--*/
{
    USB_HC_DEV          *Uhc;

    Uhc = UHC_FROM_USB2_HC_PROTO (This);

    if ((NULL == MaxSpeed) || (NULL == PortNumber) || (NULL == Is64BitCapable)) {
        return EFI_INVALID_PARAMETER;
    }

    *MaxSpeed       = EFI_USB_SPEED_FULL;
    *Is64BitCapable = (UINT8) FALSE;

    return UhciGetRootHubPortNumber (&Uhc->UsbHc, PortNumber);
}

EFI_STATUS
EFIAPI
Uhci2GetRootHubPortStatus (
    IN  EFI_USB2_HC_PROTOCOL   *This,
    IN  UINT8                  PortNumber,
    OUT EFI_USB_PORT_STATUS    *PortStatus
)
/*++

  Routine Description:

    Retrieves the current status of a USB root hub port according to UEFI 2.0 spec.

  Arguments:

    This        - A pointer to the EFI_USB2_HC_PROTOCOL.
    PortNumber  - The port to get status
    PortStatus  - A pointer to the current port status bits and
                  port status change bits.

  Returns:

    EFI_SUCCESS           : status of the USB root hub port was returned in PortStatus.
    EFI_INVALID_PARAMETER : PortNumber is invalid.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
    USB_HC_DEV          *Uhc;

    Uhc = UHC_FROM_USB2_HC_PROTO (This);

    return UhciGetRootHubPortStatus (&Uhc->UsbHc, PortNumber, PortStatus);
}

EFI_STATUS
EFIAPI
Uhci2SetRootHubPortFeature (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN UINT8                   PortNumber,
    IN EFI_USB_PORT_FEATURE    PortFeature
)
/*++

  Routine Description:

    Sets a feature for the specified root hub port according to UEFI 2.0 spec.

  Arguments:

    This        - A pointer to the EFI_USB2_HC_PROTOCOL.
    PortNumber  - Specifies the root hub port whose feature
                  is requested to be set.
    PortFeature - Indicates the feature selector associated
                  with the feature set request.

  Returns:

    EFI_SUCCESS           : PortFeature was set for the root port
    EFI_INVALID_PARAMETER : PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
    USB_HC_DEV          *Uhc;

    Uhc = UHC_FROM_USB2_HC_PROTO (This);

    return UhciSetRootHubPortFeature (&Uhc->UsbHc, PortNumber, PortFeature);
}

EFI_STATUS
EFIAPI
Uhci2ClearRootHubPortFeature (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN UINT8                   PortNumber,
    IN EFI_USB_PORT_FEATURE    PortFeature
)
/*++

  Routine Description:

    Clears a feature for the specified root hub port according to Uefi 2.0 spec.

  Arguments:

    This        - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    PortNumber  - Specifies the root hub port whose feature
                  is requested to be cleared.
    PortFeature - Indicates the feature selector associated with the
                  feature clear request.

  Returns:

    EFI_SUCCESS           : PortFeature was cleared for the USB root hub port
    EFI_INVALID_PARAMETER : PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
    USB_HC_DEV          *Uhc;

    Uhc = UHC_FROM_USB2_HC_PROTO (This);

    return UhciClearRootHubPortFeature (&Uhc->UsbHc, PortNumber, PortFeature);
}

EFI_STATUS
EFIAPI
Uhci2ControlTransfer (
    IN     EFI_USB2_HC_PROTOCOL                 *This,
    IN     UINT8                                DeviceAddress,
    IN     UINT8                                DeviceSpeed,
    IN     UINTN                                MaximumPacketLength,
    IN     EFI_USB_DEVICE_REQUEST               *Request,
    IN     EFI_USB_DATA_DIRECTION               TransferDirection,
    IN OUT VOID                                 *Data,
    IN OUT UINTN                                *DataLength,
    IN     UINTN                                TimeOut,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR   *Translator,
    OUT    UINT32                               *TransferResult
)
/*++

  Routine Description:

    Submits control transfer to a target USB device accroding to UEFI 2.0 spec..

  Arguments:

    This                : A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    DeviceSpeed         : Device speed
    MaximumPacketLength : Maximum packet size of the target endpoint
    Request             : USB device request to send
    TransferDirection   : Data direction of the Data stage in control transfer
    Data                : Data to transmit/receive in data stage
    DataLength          : Length of the data
    TimeOut             : Maximum time, in microseconds, for transfer to complete.
    TransferResult      : Variable to receive the transfer result

  Returns:

    EFI_SUCCESS           : The control transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : Failed due to lack of resource.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : Failed due to timeout.
    EFI_DEVICE_ERROR      : Failed due to host controller or device error.

--*/
{
    USB_HC_DEV          *Uhc;
    BOOLEAN             IsSlow;

    Uhc     = UHC_FROM_USB2_HC_PROTO (This);
    IsSlow  = (EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE;

    return UhciControlTransfer (
               &Uhc->UsbHc,
               DeviceAddress,
               IsSlow,
               (UINT8) MaximumPacketLength,
               Request,
               TransferDirection,
               Data,
               DataLength,
               TimeOut,
               TransferResult
           );
}

EFI_STATUS
EFIAPI
Uhci2BulkTransfer (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     UINT8                              DeviceSpeed,
    IN     UINTN                              MaximumPacketLength,
    IN     UINT8                              DataBuffersNumber,
    IN OUT VOID                               *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
    IN OUT UINTN                              *DataLength,
    IN OUT UINT8                              *DataToggle,
    IN     UINTN                              TimeOut,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    OUT    UINT32                             *TransferResult
)
/*++

  Routine Description:

    Submits bulk transfer to a bulk endpoint of a USB device

  Arguments:

    This                : A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    EndPointAddress     : Endpoint number and direction
    DeviceSpeed         : Device speed
    MaximumPacketLength : Maximum packet size of the target endpoint
    DataBuffersNumber   : Number of data buffers prepared for the transfer.
    Data                : Array of pointers to the buffers of data
    DataLength          : On input, size of the data buffer, On output,
                          actually transferred data size.
    DataToggle          : On input, data toggle to use; On output, next data toggle
    Translator          : A pointr to the transaction translator data.
    TimeOut             : Maximum time out, in microseconds
    TransferResult      : Variable to receive transfer result

  Returns:
    EFI_SUCCESS           : The bulk transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : Failed due to lack of resource.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : Failed due to timeout.
    EFI_DEVICE_ERROR      : Failed due to host controller or device error.

--*/
{
    USB_HC_DEV          *Uhc;

    Uhc = UHC_FROM_USB2_HC_PROTO (This);

    if (Data == NULL || DeviceSpeed == EFI_USB_SPEED_LOW) {
        return EFI_INVALID_PARAMETER;
    }

    //
    // For full-speed bulk transfers only the data pointed by Data[0] shall be used
    //
    return UhciBulkTransfer (
               &Uhc->UsbHc,
               DeviceAddress,
               EndPointAddress,
               (UINT8) MaximumPacketLength,
               *Data,
               DataLength,
               DataToggle,
               TimeOut,
               TransferResult
           );
}

EFI_STATUS
EFIAPI
Uhci2AsyncInterruptTransfer (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     UINT8                              DeviceSpeed,
    IN     UINTN                              MaximumPacketLength,
    IN     BOOLEAN                            IsNewTransfer,
    IN OUT UINT8                              *DataToggle,
    IN     UINTN                              PollingInterval,
    IN     UINTN                              DataLength,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
    IN     VOID                               *Context
)
/*++

  Routine Description:

    Submits an asynchronous interrupt transfer to an
    interrupt endpoint of a USB device according to UEFI 2.0 spec.

  Arguments:

    This                : A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    EndPointAddress     : Endpoint number and direction
    DeviceSpeed         : Device speed
    MaximumPacketLength : Maximum packet size of the target endpoint
    IsNewTransfer       : If TRUE, submit a new transfer, if FALSE cancel old transfer
    DataToggle          : On input, data toggle to use; On output, next data toggle
    PollingInterval     : Interrupt poll rate in milliseconds
    DataLength          : On input, size of the data buffer, On output,
                          actually transferred data size.
    Translator          : A pointr to the transaction translator data.
    CallBackFunction    : Function to call periodically
    Context             : User context

  Returns:

    EFI_SUCCESS           : Transfer was submitted
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_OUT_OF_RESOURCES  : Failed due to a lack of resources.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
    USB_HC_DEV          *Uhc;
    BOOLEAN             IsSlow;

    Uhc     = UHC_FROM_USB2_HC_PROTO (This);
    IsSlow  = (EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE;

    return UhciAsyncInterruptTransfer (
               &Uhc->UsbHc,
               DeviceAddress,
               EndPointAddress,
               IsSlow,
               (UINT8) MaximumPacketLength,
               IsNewTransfer,
               DataToggle,
               PollingInterval,
               DataLength,
               CallBackFunction,
               Context
           );
}

EFI_STATUS
EFIAPI
Uhci2AsyncInterruptOnlyTransfer (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     BOOLEAN                            IsNewTransfer,
    IN     UINTN                              PollingInterval,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
    IN     VOID                               *Context
)
/*++

  Routine Description:

    Submits an asynchronous interrupt transfer to an
    interrupt endpoint of a USB device according to UEFI 2.0 spec.

  Arguments:

    This                : A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    EndPointAddress     : Endpoint number and direction
    DeviceSpeed         : Device speed
    MaximumPacketLength : Maximum packet size of the target endpoint
    IsNewTransfer       : If TRUE, submit a new transfer, if FALSE cancel old transfer
    DataToggle          : On input, data toggle to use; On output, next data toggle
    PollingInterval     : Interrupt poll rate in milliseconds
    DataLength          : On input, size of the data buffer, On output,
                          actually transferred data size.
    Translator          : A pointr to the transaction translator data.
    CallBackFunction    : Function to call periodically
    Context             : User context

  Returns:

    EFI_SUCCESS           : Transfer was submitted
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_OUT_OF_RESOURCES  : Failed due to a lack of resources.
    EFI_DEVICE_ERROR      : Can't read register

--*/
{
    USB_HC_DEV          *Uhc;

    Uhc     = UHC_FROM_USB2_HC_PROTO (This);

    return UhciAsyncInterruptOnlyTransfer (
               &Uhc->UsbHc,
               IsNewTransfer,
               PollingInterval,
               CallBackFunction,
               Context
           );
}

EFI_STATUS
EFIAPI
Uhci2SyncInterruptTransfer (
    IN     EFI_USB2_HC_PROTOCOL                      *This,
    IN     UINT8                                     DeviceAddress,
    IN     UINT8                                     EndPointAddress,
    IN     UINT8                                     DeviceSpeed,
    IN     UINTN                                     MaximumPacketLength,
    IN OUT VOID                                      *Data,
    IN OUT UINTN                                     *DataLength,
    IN OUT UINT8                                     *DataToggle,
    IN     UINTN                                     TimeOut,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR        *Translator,
    OUT    UINT32                                    *TransferResult
)
/*++

  Routine Description:

    Submits synchronous interrupt transfer to an interrupt endpoint
    of a USB device according to UEFI 2.0 spec.

  Arguments:

    This                : A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    EndPointAddress     : Endpoint number and direction
    DeviceSpeed         : Device speed
    MaximumPacketLength : Maximum packet size of the target endpoint
    DataBuffersNumber   : Number of data buffers prepared for the transfer.
    Data                : Array of pointers to the buffers of data
    DataLength          : On input, size of the data buffer, On output,
                          actually transferred data size.
    DataToggle          : On input, data toggle to use; On output, next data toggle
    TimeOut             : Maximum time out, in microseconds
    Translator          : A pointr to the transaction translator data.
    TransferResult      : Variable to receive transfer result

  Returns:
    EFI_SUCCESS           : The transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  : Failed due to lack of resource.
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_TIMEOUT           : Failed due to timeout.
    EFI_DEVICE_ERROR      : Failed due to host controller or device error.

--*/
{
    USB_HC_DEV          *Uhc;
    BOOLEAN             IsSlow;

    if (DeviceSpeed == EFI_USB_SPEED_HIGH) {
        return EFI_INVALID_PARAMETER;
    }

    Uhc     = UHC_FROM_USB2_HC_PROTO (This);
    IsSlow  = (EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE;

    return UhciSyncInterruptTransfer (
               &Uhc->UsbHc,
               DeviceAddress,
               EndPointAddress,
               IsSlow,
               (UINT8) MaximumPacketLength,
               Data,
               DataLength,
               DataToggle,
               TimeOut,
               TransferResult
           );
}

EFI_STATUS
EFIAPI
Uhci2IsochronousTransfer (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     UINT8                              DeviceSpeed,
    IN     UINTN                              MaximumPacketLength,
    IN     UINT8                              DataBuffersNumber,
    IN OUT VOID                               *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN     UINTN                              DataLength,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    OUT    UINT32                             *TransferResult
)
/*++

  Routine Description:

    Submits isochronous transfer to a target USB device according to UEFI 2.0 spec.

  Arguments:

    This                : A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    EndPointAddress     : Endpoint number and direction
    DeviceSpeed         : Device speed
    MaximumPacketLength : Maximum packet size of the target endpoint
    DataBuffersNumber   : Number of data buffers prepared for the transfer.
    Data                : Array of pointers to the buffers of data
    DataLength          : On input, size of the data buffer, On output,
                          actually transferred data size.
    Translator          : A pointr to the transaction translator data.
    TransferResult      : Variable to receive transfer result

  Returns:

    EFI_UNSUPPORTED

--*/
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
Uhci2AsyncIsochronousTransfer (
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
/*++

  Routine Description:

    Submits Async isochronous transfer to a target USB device according to UEFI 2.0 spec.

  Arguments:

    This                : A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       : Target device address
    EndPointAddress     : Endpoint number and direction
    DeviceSpeed         : Device speed
    MaximumPacketLength : Maximum packet size of the target endpoint
    DataBuffersNumber   : Number of data buffers prepared for the transfer.
    Data                : Array of pointers to the buffers of data
    Translator          : A pointr to the transaction translator data.
    IsochronousCallBack : Function to call when the transfer complete
    Context             : Pass to the call back function as parameter

  Returns:

    EFI_UNSUPPORTED

--*/
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
Uhci2DisableLegacySupport (
    IN       EFI_USB2_HC_PROTOCOL                 * This
)
/*++

Routine Description:

  Disable legacy support of UHCI device

Arguments:

  This                : A pointer to the EFI_USB2_HC_PROTOCOL instance.

Returns:



--*/
{
    USB_HC_DEV          *Uhc;

    Uhc     = UHC_FROM_USB2_HC_PROTO (This);

    return UhciDisableLegacySupport (
               &Uhc->UsbHc
           );
}

EFI_STATUS
EFIAPI
Uhci2EnableLegacySupport (
    IN       EFI_USB2_HC_PROTOCOL                 * This
)
/*++

Routine Description:

  Enable legacy support of UHCI device

Arguments:

  This : A pointer to the EFI_USB2_HC_PROTOCOL instance.

Returns:



--*/
{
    USB_HC_DEV          *Uhc;

    Uhc     = UHC_FROM_USB2_HC_PROTO (This);

    return UhciEnableLegacySupport (
               &Uhc->UsbHc
           );
}

EFI_STATUS
EFIAPI
Uhci2LegacyCallback (
    IN       EFI_USB2_HC_PROTOCOL                 * This
)
/*++

Routine Description:


Arguments:


Returns:



--*/
{
    USB_HC_DEV          *Uhc;

    Uhc     = UHC_FROM_USB2_HC_PROTO (This);

    return UhciLegacyCallback (
               &Uhc->UsbHc
           );
}

USB_HC_DEV *
UhciAllocateDev (
    IN EFI_PCI_FUNCTION_ADDRESS   *PciAddress
)
/*++

Routine Description:

  Allocate and initialize the empty UHCI device

Arguments:

  PciIo - The PCIIO to use

Returns:

  Allocated UHCI device

--*/
{
    USB_HC_DEV  *Uhc;
    EFI_STATUS  Status;
    UINT64      PciRegAddress;
    FULL_USB_DEVICE_PATH ControllerPath = USB_DEVICE_PATH_TEMPLATE;

    Uhc = AllocateZeroPool (sizeof (USB_HC_DEV));
    if (Uhc == NULL)
        return NULL;

    //
    // This driver supports both USB_HC_PROTOCOL and USB2_HC_PROTOCOL.
    // USB_HC_PROTOCOL is for EFI 1.1 backward compability.
    //
    Uhc->Signature                        = USB_HC_DEV_SIGNATURE;
    Uhc->UsbHc.Reset                      = UhciReset;
    Uhc->UsbHc.GetState                   = UhciGetState;
    Uhc->UsbHc.SetState                   = UhciSetState;
    Uhc->UsbHc.ControlTransfer            = UhciControlTransfer;
    Uhc->UsbHc.BulkTransfer               = UhciBulkTransfer;
    Uhc->UsbHc.AsyncInterruptTransfer     = UhciAsyncInterruptTransfer;
    Uhc->UsbHc.SyncInterruptTransfer      = UhciSyncInterruptTransfer;
    Uhc->UsbHc.IsochronousTransfer        = UhciIsochronousTransfer;
    Uhc->UsbHc.AsyncIsochronousTransfer   = UhciAsyncIsochronousTransfer;
    Uhc->UsbHc.GetRootHubPortNumber       = UhciGetRootHubPortNumber;
    Uhc->UsbHc.GetRootHubPortStatus       = UhciGetRootHubPortStatus;
    Uhc->UsbHc.SetRootHubPortFeature      = UhciSetRootHubPortFeature;
    Uhc->UsbHc.ClearRootHubPortFeature    = UhciClearRootHubPortFeature;
    Uhc->UsbHc.MajorRevision              = 0x1;
    Uhc->UsbHc.MinorRevision              = 0x1;
    Uhc->UsbHc.AsyncInterruptOnlyTransfer = UhciAsyncInterruptOnlyTransfer;
    Uhc->UsbHc.EnableLegacySupport        = UhciEnableLegacySupport;
    Uhc->UsbHc.DisableLegacySupport       = UhciDisableLegacySupport;
    Uhc->UsbHc.LegacyCallback             = UhciLegacyCallback;

    Uhc->Usb2Hc.GetCapability             = Uhci2GetCapability;
    Uhc->Usb2Hc.Reset                     = Uhci2Reset;
    Uhc->Usb2Hc.GetState                  = Uhci2GetState;
    Uhc->Usb2Hc.SetState                  = Uhci2SetState;
    Uhc->Usb2Hc.ControlTransfer           = Uhci2ControlTransfer;
    Uhc->Usb2Hc.BulkTransfer              = Uhci2BulkTransfer;
    Uhc->Usb2Hc.AsyncInterruptTransfer    = Uhci2AsyncInterruptTransfer;
    Uhc->Usb2Hc.SyncInterruptTransfer     = Uhci2SyncInterruptTransfer;
    Uhc->Usb2Hc.IsochronousTransfer       = Uhci2IsochronousTransfer;
    Uhc->Usb2Hc.AsyncIsochronousTransfer  = Uhci2AsyncIsochronousTransfer;
    Uhc->Usb2Hc.GetRootHubPortStatus      = Uhci2GetRootHubPortStatus;
    Uhc->Usb2Hc.SetRootHubPortFeature     = Uhci2SetRootHubPortFeature;
    Uhc->Usb2Hc.ClearRootHubPortFeature   = Uhci2ClearRootHubPortFeature;
    Uhc->Usb2Hc.MajorRevision             = 0x1;
    Uhc->Usb2Hc.MinorRevision             = 0x1;

    Uhc->Usb2Hc.AsyncInterruptOnlyTransfer = Uhci2AsyncInterruptOnlyTransfer;
    Uhc->Usb2Hc.EnableLegacySupport       = Uhci2EnableLegacySupport;
    Uhc->Usb2Hc.DisableLegacySupport      = Uhci2DisableLegacySupport;
    Uhc->Usb2Hc.LegacyCallback            = Uhci2LegacyCallback;
    Uhc->Usb2Hc.GSetVirtualState          = Uhci2VirtualKbc;
    Uhc->PciAddress.Bus                   = PciAddress->Bus;
    Uhc->PciAddress.Device                = PciAddress->Device;
    Uhc->PciAddress.Function              = PciAddress->Function;

    PciRegAddress = SMM_PCI_ADDRESS (
                        Uhc->PciAddress.Bus,
                        Uhc->PciAddress.Device,
                        Uhc->PciAddress.Function,
                        USBBASE_OFFSET
                    );
    Status = SmmPciCfgRead (
                 gSmst,
                 SmmPciWidthUint32,
                 (SMM_PCI_IO_ADDRESS *) &PciRegAddress,
                 &Uhc->UsbIoBase
             );
    if (EFI_ERROR(Status)) {
        goto ON_ERROR;;
    }

    Uhc->UsbIoBase &= UCHI_IO_BASE_ADDRESS_MASK;
    Uhc->DevicePath = AllocateZeroPool (sizeof (ControllerPath));
    if (Uhc->DevicePath == NULL)
        goto ON_ERROR;

    ControllerPath.Pci.Device = (UINT8)Uhc->PciAddress.Device;
    ControllerPath.Pci.Function = (UINT8) Uhc->PciAddress.Function;
    CopyMem (
        Uhc->DevicePath,
        &ControllerPath,
        sizeof (ControllerPath));

    InitializeListHead (&Uhc->AsyncIntList);
    return Uhc;

ON_ERROR:
    FreePool (Uhc);
    return NULL;
}

VOID
UhciFreeDev (
    IN USB_HC_DEV           *Uhc
)
/*++

Routine Description:

  Free the UHCI device and release its associated resources

Arguments:

  Uhc - The UHCI device to release

Returns:

  None

--*/
{
    FreePool (Uhc);
}

VOID
UhciCleanDevUp (
    IN USB_HC_DEV          *Uhc
)
/*++
  Routine Description:

    Uninstall all Uhci Interface

  Arguments:

    Controller        - Controller handle
    This              - Protocol instance pointer.

  Returns:

    VOID

--*/
{
    UhciStopHc (Uhc, UHC_GENERIC_TIMEOUT);
    UhciFreeAllAsyncReq (Uhc);
    UhciDestoryFrameList (Uhc);

    UhciFreeDev (Uhc);
}

EFI_STATUS
EFIAPI
UsbConnectUhciDriver (
    IN EFI_PCI_FUNCTION_ADDRESS   *PciAddress,
    OUT EFI_USB2_HC_PROTOCOL      **Usb2HCInterface,
    OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
)
/*++

  Routine Description:

    Starting the Usb UHCI Driver

  Arguments:

    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:

    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.
    EFI_DEVICE_ERROR    - This driver cannot be started due to device Error
    EFI_OUT_OF_RESOURCES- Failed due to resource shortage

--*/
{
    EFI_STATUS          Status;
    USB_HC_DEV          *Uhc;

    Uhc = UhciAllocateDev (PciAddress);

    if (Uhc == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FREE_UHC;
    }

    Status = UhciEnableDevice (Uhc);
    if (EFI_ERROR (Status)) {
        goto FREE_UHC;
    }

    //
    // Stop schedule and set the Global Reset bit in the command register
    //
    UhciStopHc (Uhc, UHC_GENERIC_TIMEOUT);
    UhciSetRegBit (Uhc, USBCMD_OFFSET, USBCMD_GRESET);
    //Notice: Remove for Reduce LegacyUsb Post time  Stall (UHC_ROOT_PORT_RESET_STALL);
    Stall (10 * UHC_1_MILLISECOND);  //Notice: Add for Reduce LegacyUsb Post time

    //
    // Clear the Global Reset bit to zero.
    //
    UhciClearRegBit (Uhc, USBCMD_OFFSET, USBCMD_GRESET);
    Stall (UHC_ROOT_PORT_RECOVERY_STALL);

    //
    // Allocate and Init Host Controller's Frame List Entry
    //
    Status = UhciInitFrameList (Uhc);

    if (EFI_ERROR (Status)) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FREE_UHC;
    }

    //
    // Start the UHCI hardware, also set its reclamation point to 64 bytes
    //
    UhciWriteReg (Uhc, USBCMD_OFFSET, USBCMD_RS | USBCMD_MAXP | USBCMD_CF);

    *Usb2HCInterface = &Uhc->Usb2Hc;
    *DevicePath      = Uhc->DevicePath;
    InsertTailList (&mUhcList, &Uhc->Link);

    return EFI_SUCCESS;

FREE_UHC:
    UhciFreeDev (Uhc);
    return Status;
}

EFI_STATUS
EFIAPI
UsbDisconnectUhciDriver (
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

    EFI_SUCCESS
    others

--*/
{
    USB_HC_DEV          *Uhc;

    Uhc = UHC_FROM_USB2_HC_PROTO (Usb2HCInterface);
    UhciCleanDevUp (Uhc);
    return EFI_SUCCESS;
}

