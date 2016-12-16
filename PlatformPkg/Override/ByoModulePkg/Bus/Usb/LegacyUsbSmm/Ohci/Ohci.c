/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2009 Intel Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:
  Ohci.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "Ohci.h"
#include "../UsbLib/UsbUtility.h"
#include "../UsbLib/SmmPciIo.h"
#include "../UsbLib/UsbMem.h"
#include <Library/ReportStatusCodeLib.h>

EFI_STATUS
EFIAPI
GetOhcInfo (
    IN EFI_PCI_FUNCTION_ADDRESS *PciAddress,
    IN VOID                     **SmmOhc,
    IN VOID                     **SmmUsbHc,
    IN VOID                     **SmmUsb2Hc
)
{
    LIST_ENTRY                 *Next;

    Next = mOhcList.BackLink;
    while (Next != &mOhcList) {
        USB_OHCI_DEV          *Hc;
        Hc = CR(Next, USB_OHCI_DEV, Link, USB_OHCI_DEV_SIGNATURE);
        if (Hc->PciAddress.Bus == PciAddress->Bus &&
                Hc->PciAddress.Device == PciAddress->Device &&
                Hc->PciAddress.Function == PciAddress->Function ) {

            *SmmOhc = (VOID *)Hc;
            *SmmUsbHc = (VOID *)(&Hc->UsbHc);
            *SmmUsb2Hc = (VOID *)(&Hc->Usb2Hc);

            return EFI_SUCCESS;
        }
        Next = Next->BackLink;
    }

    return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
OhciReset (
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
    USB_OHCI_DEV        *Ohc;
    UINTN               WaitPeriod;
    UINTN               Index;
    EFI_STATUS          Status;
    UINT32              Data;
    Ohc     = OHC_FROM_USB_HC_PROTO (This);

    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_IO_BUS_USB | EFI_IOB_PC_RESET));

    switch (Attributes) {
    case EFI_USB_HC_RESET_GLOBAL:
        //
        // Set the Global Reset bit in the control register
        //
        OhciClearOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS);
        OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS_RESET);
        Stall (OHC_ROOT_PORT_RESET_STALL);
        break;

    case EFI_USB_HC_RESET_HOST_CONTROLLER:
        //
        // Set the Reset bit in the command status register
        //
        OhciSetOpRegBit (Ohc, OHC_HCCOMMANDSTATUS_OFFSET, OHC_HCCOMMANDSTATUS_HCR);
        WaitPeriod = OHC_1_MILLISECOND;

        for (Index = 0; Index < OHC_PORT_RESET_RECOVERY_STALL / WaitPeriod; Index++) {
            Data = OhciReadOpReg(Ohc, OHC_HCCOMMANDSTATUS_OFFSET);
            if (!(Data & OHC_HCCOMMANDSTATUS_HCR))
                break;
            Stall (OHC_ROOT_PORT_ENABLE_STALL);
        }


        if (Index ==  (OHC_PORT_RESET_RECOVERY_STALL / WaitPeriod))
            return EFI_DEVICE_ERROR;

        break;

    default:
        goto ON_INVAILD_PARAMETER;
    }

    //
    // Delete all old transactions on the USB bus, then
    // reinitialize the HCCA
    //
    OhciFreeAllAsyncReq (Ohc);

    Status = OhcInitHc(Ohc);
    if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        return Status;
    }

    return EFI_SUCCESS;

ON_INVAILD_PARAMETER:
    return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
OhciGetState (
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
    USB_OHCI_DEV        *Ohc;
    UINT32              UsbSts;

    if (State == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    Ohc     = OHC_FROM_USB_HC_PROTO (This);
    UsbSts  = OhciReadOpReg(Ohc, OHC_HCCONTROL_OFFSET);
    UsbSts &= OHC_HCCONTROL_HCFS;

    if (UsbSts == OHC_HCCONTROL_HCFS) {
        *State = EfiUsbHcStateSuspend;
    } else if (UsbSts == OHC_HCCONTROL_HCFS_OPERATIONAL) {
        *State = EfiUsbHcStateOperational;
    } else if (UsbSts == 0) {
        *State = EfiUsbHcStateHalt;
    } else {
        *State = EfiUsbHcStateHalt;
    }

    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
OhciSetState (
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
    USB_OHCI_DEV        *Ohc;
    EFI_STATUS          Status;

    Ohc     = OHC_FROM_USB_HC_PROTO (This);
    Status  = OhciGetState (This, &CurState);

    if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
    }

    if (CurState == State) {
        return EFI_SUCCESS;
    }

    Status  = EFI_SUCCESS;

    switch (State) {
    case EfiUsbHcStateHalt:
        //
        // Set the Global Reset bit in the control register
        //
        OhciClearOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS);
        OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS_RESET);
        Stall (OHC_PORT_RESET_RECOVERY_STALL);
        break;

    case EfiUsbHcStateOperational:
        //
        // Set the functional state to operational
        //
        OhciClearOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS);
        OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS_OPERATIONAL);
        Stall (OHC_PORT_RESET_RECOVERY_STALL);
        break;

    case EfiUsbHcStateSuspend:
        //
        // Set the functional state to suspend
        //
        OhciClearOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS);
        OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS);
        Stall (OHC_PORT_RESET_RECOVERY_STALL);
        break;

    default:
        Status = EFI_INVALID_PARAMETER;
        break;
    }

    return Status;
}

EFI_STATUS
EFIAPI
OhciGetRootHubPortNumber (
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
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB_HC_PROTO (This);

    if (PortNumber == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    *PortNumber = 0;

    *PortNumber  = (UINT8)(OhciReadOpReg(Ohc, OHC_HCRHDESCRIPTORA_OFFSET) & OHC_HCRHDESCRIPTORA_NDP);

    Ohc->RootPorts = *PortNumber;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OhciGetRootHubPortStatus (
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
    USB_OHCI_DEV        *Ohc;
    UINT32              PortSC;

    Ohc = OHC_FROM_USB_HC_PROTO (This);

    if (PortStatus == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (PortNumber >= Ohc->RootPorts) {
        return EFI_INVALID_PARAMETER;
    }

    PortStatus->PortStatus        = 0;
    PortStatus->PortChangeStatus  = 0;

    PortSC = OhciReadOpReg(Ohc, OHC_HCRHPORTSTATUSN_OFFSET + PortNumber * 4);

    if (PortSC & OHC_HCRHPORTSTATUSN_CCS) {
        PortStatus->PortStatus |= USB_PORT_STAT_CONNECTION;
    }

    if (PortSC & OHC_HCRHPORTSTATUSN_PES) {
        PortStatus->PortStatus |= USB_PORT_STAT_ENABLE;
    }

    if (PortSC & OHC_HCRHPORTSTATUSN_PSS) {
        PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
    }

    if (PortSC & OHC_HCRHPORTSTATUSN_POCI) {
        PortStatus->PortStatus |= USB_PORT_STAT_OVERCURRENT;
    }

    if (PortSC & OHC_HCRHPORTSTATUSN_PRS) {
        PortStatus->PortStatus |= USB_PORT_STAT_RESET;
    }

    if (PortSC & OHC_HCRHPORTSTATUSN_PPS) {
        PortStatus->PortStatus |= USB_PORT_STAT_POWER;
    }

    if (PortSC & OHC_HCRHPORTSTATUSN_LSDA) {
        PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
    }

    //
    // CHC will always return one in port owner bit
    //
    PortStatus->PortStatus |= USB_PORT_STAT_OWNER;

    if (PortSC & OHC_HCRHPORTSTATUSN_CSC) {
        PortStatus->PortChangeStatus |= USB_PORT_STAT_C_CONNECTION;
    }

    if (PortSC & OHC_HCRHPORTSTATUSN_PESC) {
        PortStatus->PortChangeStatus |= USB_PORT_STAT_C_ENABLE;
    }

    if (PortSC & OHC_HCRHPORTSTATUSN_PSSC) {
        PortStatus->PortChangeStatus |= USB_PORT_STAT_C_SUSPEND;
    }

    if (PortSC & OHC_HCRHPORTSTATUSN_OCIC) {
        PortStatus->PortChangeStatus |= USB_PORT_STAT_C_OVERCURRENT;
    }

    if (PortSC & OHC_HCRHPORTSTATUSN_PRSC) {
        PortStatus->PortChangeStatus |= USB_PORT_STAT_C_RESET;
    }
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OhciSetRootHubPortFeature (
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
    USB_OHCI_DEV        *Ohc;
    UINT32              PortSC;

    Ohc = OHC_FROM_USB_HC_PROTO (This);

    if (PortNumber >= Ohc->RootPorts) {
        return EFI_INVALID_PARAMETER;
    }

    PortSC = OhciReadOpReg(Ohc, OHC_HCRHPORTSTATUSN_OFFSET + PortNumber * 4);

    switch (PortFeature) {
    case EfiUsbPortSuspend:
        if (PortSC & OHC_HCRHPORTSTATUSN_CCS)
            PortSC = OHC_HCRHPORTSTATUSN_PSS;
        break;

    case EfiUsbPortReset:
        if (PortSC & OHC_HCRHPORTSTATUSN_CCS)
            PortSC = OHC_HCRHPORTSTATUSN_PRS;
        break;

    case EfiUsbPortPower:
        PortSC = OHC_HCRHPORTSTATUSN_PPS;
        break;

    case EfiUsbPortEnable:
        if (PortSC & OHC_HCRHPORTSTATUSN_CCS)
            PortSC = OHC_HCRHPORTSTATUSN_PES;
        break;

    case EfiUsbPortOwner:
        return EFI_SUCCESS;

    default:
        return EFI_INVALID_PARAMETER;
    }

    OhciWriteOpReg(Ohc, OHC_HCRHPORTSTATUSN_OFFSET + PortNumber * 4, PortSC);
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OhciClearRootHubPortFeature (
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
    USB_OHCI_DEV        *Ohc;
    UINT32              PortSC;

    Ohc = OHC_FROM_USB_HC_PROTO (This);

    if (PortNumber >= Ohc->RootPorts) {
        return EFI_INVALID_PARAMETER;
    }

    PortSC = OhciReadOpReg(Ohc, OHC_HCRHPORTSTATUSN_OFFSET + PortNumber * 4);

    switch (PortFeature) {
    case EfiUsbPortEnable:
        PortSC = OHC_HCRHPORTSTATUSN_CCS;
        break;

    case EfiUsbPortSuspend:
        //
        // Cause a resume on the specified port if in suspend mode.
        //
        PortSC = OHC_HCRHPORTSTATUSN_POCI;
        break;

    case EfiUsbPortPower:
        //
        // No action
        //
        return EFI_SUCCESS;

    case EfiUsbPortReset:
        PortSC = OHC_HCRHPORTSTATUSN_PRSC;
        break;

    case EfiUsbPortConnectChange:
        PortSC = OHC_HCRHPORTSTATUSN_CSC;
        break;

    case EfiUsbPortEnableChange:
        PortSC = OHC_HCRHPORTSTATUSN_PESC;
        break;

    case EfiUsbPortSuspendChange:
        //
        // Root hub does not support this
        //
        PortSC = OHC_HCRHPORTSTATUSN_PSSC;
        break;

    case EfiUsbPortOverCurrentChange:
        //
        // Root hub does not support this
        //
        PortSC = OHC_HCRHPORTSTATUSN_OCIC;
        break;

    case EfiUsbPortResetChange:
        //
        // Root hub does not support this
        //
        PortSC = OHC_HCRHPORTSTATUSN_PRSC;
        break;

    default:
        return EFI_INVALID_PARAMETER;
    }

    OhciWriteOpReg(Ohc, OHC_HCRHPORTSTATUSN_OFFSET + PortNumber * 4, PortSC);
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OhciControlTransfer (
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
    USB_OHCI_DEV            *Ohc;
    OHCI_TD_SW              *TDs;
    OHCI_ED_SW              *Ed;
    EFI_STATUS              Status;
    OHCI_ED_RESULT          EdResult;
    UINT8                   PktId;
    UINT8                   *DataPhy;

    Ohc         = OHC_FROM_USB_HC_PROTO (This);
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

    if ((TransferDirection != EfiUsbNoData) && (DataLength == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;

    //
    // If errors exist that cause host controller halt,
    // clear status then return EFI_DEVICE_ERROR.
    //
    OhciAckAllInterrupt (Ohc);

    if (!OhciIsHcWorking (Ohc)) {
        return EFI_DEVICE_ERROR;
    }

    //
    // Map the Request and data for bus master access,
    // then create a list of TD for this transfer
    //
    Status = OhciMapUserData (Ohc, TransferDirection, Data, DataLength, &PktId, &DataPhy);

    if (EFI_ERROR (Status)) {
        goto ON_EXIT;
    }

    TDs = OhciCreateCtrlTds (
              Ohc,
              DeviceAddress,
              PktId,
              (UINT8 *)Request,
              DataPhy,
              *DataLength,
              MaximumPacketLength
          );

    if (TDs == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
    }

    //
    // According to the speed of the end point, link
    // the TD to corrosponding ED, then check
    // the execution result
    //
    Ed = OhciCreateEd (Ohc, DeviceAddress, 0, IsSlowDevice, 0, MaximumPacketLength, 3);
    if (Ed == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        OhciDestoryTds (Ohc, TDs);
        goto ON_EXIT;
    }

    //
    // Disable the control and bulk list
    //

    OhciClearOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_CLE | OHC_HCCONTROL_BLE);
    OhciLinkTdToEd (Ed, TDs);
    OhciLinkEdToEd (Ohc->HccaCtrlEDHead, Ed);

    //
    // Enable the control list and control list filled
    //
    OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_CLE);
    OhciSetOpRegBit(Ohc, OHC_HCCOMMANDSTATUS_OFFSET, OHC_HCCOMMANDSTATUS_CLF);

    Status = OhciExecuteTransfer (Ohc, Ohc->HccaCtrlEDHead, TDs, TimeOut, &EdResult);
    OhciUnlinkTdFromEd (Ed, TDs);
    Ohc->HccaCtrlEDHead->EdHw.NextED = 0;

    *TransferResult = EdResult.Result;

    if (DataLength != NULL) {
        *DataLength = EdResult.Complete;
    }

    OhciDestoryTds (Ohc, TDs);

ON_EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
OhciBulkTransfer (
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
    USB_OHCI_DEV            *Ohc;
    OHCI_TD_SW              *TDs;
    OHCI_ED_SW              *Ed;
    OHCI_ED_RESULT          EdResult;
    EFI_STATUS              Status;
    UINT8                   PktId;
    UINT8                   *DataPhy;

    Ohc     = OHC_FROM_USB_HC_PROTO (This);
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
    OhciAckAllInterrupt (Ohc);

    if (!OhciIsHcWorking (Ohc)) {
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

    Status = OhciMapUserData (Ohc, Direction, Data, DataLength, &PktId, &DataPhy);

    if (EFI_ERROR (Status)) {
        goto ON_EXIT;
    }

    Status = EFI_OUT_OF_RESOURCES;
    TDs    = OhciCreateBulkOrIntTds (
                 Ohc,
                 PktId,
                 DataPhy,
                 *DataLength,
                 DataToggle,
                 MaximumPacketLength,
                 FALSE
             );

    if (TDs == NULL) {
        goto ON_EXIT;
    }

    //
    // According to the speed of the end point, link
    // the TD to corrosponding ED, then check
    // the execution result
    //
    Ed = OhciCreateEd (Ohc, DeviceAddress, EndPointAddress, FALSE, 0, MaximumPacketLength, 3);
    if (Ed == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        OhciDestoryTds (Ohc, TDs);
        goto ON_EXIT;
    }

    //
    // Disable the period and bulk list
    //
    OhciClearOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_PLE | OHC_HCCONTROL_BLE);

    OhciLinkTdToEd (Ed, TDs);
    OhciLinkEdToEd (Ohc->HccaBulkEDHead, Ed);

    //
    // Enable the bulk/period list and bulk list filled
    //
    OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_BLE | OHC_HCCONTROL_PLE) ;
    OhciSetOpRegBit(Ohc, OHC_HCCOMMANDSTATUS_OFFSET, OHC_HCCOMMANDSTATUS_BLF);
    Status = OhciExecuteTransfer (Ohc, Ohc->HccaBulkEDHead, TDs, TimeOut, &EdResult);
    OhciUnlinkTdFromEd (Ed, TDs);
    Ohc->HccaBulkEDHead->EdHw.NextED = 0;

    *TransferResult = EdResult.Result;
    *DataLength     = EdResult.Complete;

    OhciDestoryTds (Ohc, TDs);

ON_EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
OhciAsyncInterruptOnlyTransfer (
    IN     EFI_USB_HC_PROTOCOL                *This,
    IN     BOOLEAN                            IsNewTransfer,
    IN     UINTN                              PollingInterval,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
    IN     VOID                               *Context
)
/*++

  Routine Description:

    Submits an asynchronous interrupt transfer to an interrupt endpoint of a USB device.

  Arguments:

    This                      : A pointer to the EFI_USB_HC_PROTOCOL instance.
    IsNewTransfer       : If TRUE, submit a new async interrupt transfer, otherwise
                          cancel an existed one
    PollingInterval       : Interrupt poll rate in milliseconds
    CallBackFunction    : Function to call periodically
    Context                 : User context

  Returns:

    EFI_SUCCESS           : Request is submitted or cancelled
    EFI_INVALID_PARAMETER : Some parameters are invalid.
    EFI_OUT_OF_RESOURCES  : Failed due to a lack of resources.
    EFI_DEVICE_ERROR      : Failed to due to device error

--*/
{

    USB_OHCI_DEV        *Ohc;
    OHCI_ED_SW          *Ed;
    OHCI_TD_SW          *IntTds;
    EFI_STATUS          Status;
    UINT8               DataToggle;

    Ohc        = OHC_FROM_USB_HC_PROTO (This);
    Ed         = NULL;
    IntTds     = NULL;
    DataToggle = 0;

    //
    // Delete Async interrupt transfer request
    //
    if (!IsNewTransfer) {
        Status = OhciRemoveAsyncReq (Ohc, 0, 0,
                                     CallBackFunction, Context, &DataToggle);
        return Status;
    }

    //
    // If has errors that cause host controller halt,
    // then return EFI_DEVICE_ERROR directly.
    //
    OhciAckAllInterrupt (Ohc);

    if (!OhciIsHcWorking (Ohc)) {
        return EFI_DEVICE_ERROR;
    }

    //
    // According to the speed of the end point, link
    // the TD to corrosponding ED, then check
    // the execution result
    //
    Ed = OhciCreateEd (Ohc, 0, 0, 1, 0, 8, 3);
    if (Ed == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto UNMAP_DATA;
    }

    IntTds = OhciCreateBulkOrIntTds (
                 Ohc,
                 OHC_INPUT_PACKET_ID,
                 NULL,
                 0,
                 0,
                 0,
                 TRUE
             );

    if (IntTds == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DESTORY_ED;
    }

    OhciLinkTdToEd (Ed, IntTds);
    //
    // Save ED-TD structures to async Interrupt transfer list,
    // for monitor interrupt transfer execution routine use.
    //
    Status = OhciCreateAsyncReq (
                 Ohc,
                 Ed,
                 IntTds,
                 0,
                 0,
                 0,
                 PollingInterval,
                 NULL,
                 CallBackFunction,
                 Context,
                 TRUE
             );

    if (EFI_ERROR (Status)) {
        OhciDestoryTds (Ohc, IntTds);
        goto DESTORY_ED;
    }

    OhciLinkEDToIntTree(Ohc, Ed, PollingInterval);
    Ed->EdHw.sKip = 0;
    ((OHCI_ED_SW*)(Ed->PreEDSoft))->EdHw.sKip = 0;
    OhciSetOpRegBit (Ohc, OHC_HCINTERRUPTSTATUS_OFFSET, (UINT32)0x0400007F);
    OhciSetOpRegBit (Ohc, OHC_HCINTERRUPTENABLE_OFFSET, (UINT32)OHC_HCINTERRUPTENABLE_MIE);

    return EFI_SUCCESS;

DESTORY_ED:
    UsbFreePool (Ed, sizeof (OHCI_ED_SW));

UNMAP_DATA:
    return Status;

}

EFI_STATUS
EFIAPI
OhciAsyncInterruptTransfer (
    IN     EFI_USB_HC_PROTOCOL                *This,
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
    USB_OHCI_DEV        *Ohc;
    OHCI_ED_SW          *Ed;
    OHCI_TD_SW          *IntTds;
    EFI_STATUS          Status;
    UINT8               *DataPtr;
    UINT8               *DataPhy;
    UINT8               PktId;

    Ohc       = OHC_FROM_USB_HC_PROTO (This);
    Ed        = NULL;
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
        Status = OhciRemoveAsyncReq (Ohc, DeviceAddress, EndPointAddress,
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

    //
    // If has errors that cause host controller halt,
    // then return EFI_DEVICE_ERROR directly.
    //
    OhciAckAllInterrupt (Ohc);

    if (!OhciIsHcWorking (Ohc)) {
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
    // Map the user data then create a endpoint descriptor and
    // list of TD for it.
    //
    Status = OhciMapUserData (
                 Ohc,
                 EfiUsbDataIn,
                 DataPtr,
                 &DataLength,
                 &PktId,
                 &DataPhy
             );

    if (EFI_ERROR (Status)) {
        goto FREE_DATA;
    }

    //
    // According to the speed of the end point, link
    // the TD to corrosponding ED, then check
    // the execution result
    //
    Ed = OhciCreateEd (Ohc, DeviceAddress, EndPointAddress, IsSlowDevice, 0, MaximumPacketLength, 3);
    if (Ed == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto UNMAP_DATA;
    }

    IntTds = OhciCreateBulkOrIntTds (
                 Ohc,
                 PktId,
                 DataPhy,
                 DataLength,
                 DataToggle,
                 MaximumPacketLength,
                 TRUE
             );

    if (IntTds == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DESTORY_ED;
    }

    OhciLinkTdToEd (Ed, IntTds);

    //
    // Save ED-TD structures to async Interrupt transfer list,
    // for monitor interrupt transfer execution routine use.
    //
    Status = OhciCreateAsyncReq (
                 Ohc,
                 Ed,
                 IntTds,
                 DeviceAddress,
                 EndPointAddress,
                 DataLength,
                 PollingInterval,
                 DataPtr,
                 CallBackFunction,
                 Context,
                 FALSE
             );

    if (EFI_ERROR (Status)) {
        OhciDestoryTds (Ohc, IntTds);
        goto DESTORY_ED;
    }

    OhciLinkEDToIntTree(Ohc, Ed, PollingInterval);
    Ed->EdHw.sKip = 0;
    ((OHCI_ED_SW*)(Ed->PreEDSoft))->EdHw.sKip = 0;
    OhciSetOpRegBit (Ohc, OHC_HCINTERRUPTSTATUS_OFFSET, (UINT32)0x0400007F);
    OhciSetOpRegBit (Ohc, OHC_HCINTERRUPTENABLE_OFFSET, (UINT32)OHC_HCINTERRUPTENABLE_MIE);
    return EFI_SUCCESS;

DESTORY_ED:
    UsbFreePool (Ed, sizeof (OHCI_ED_SW));

UNMAP_DATA:

FREE_DATA:
    UsbFreePool (DataPtr, DataLength);
    return Status;
}

EFI_STATUS
EFIAPI
OhciSyncInterruptTransfer (
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
    USB_OHCI_DEV        *Ohc;
    OHCI_TD_SW          *TDs;
    OHCI_ED_SW          *Ed;
    OHCI_ED_RESULT      EdResult;
    UINT8               *DataPhy;
    VOID                *DataMap;
    UINT8               PktId;

    Ohc     = OHC_FROM_USB_HC_PROTO (This);
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


    OhciAckAllInterrupt (Ohc);

    if (!OhciIsHcWorking (Ohc)) {
        return Status;
    }

    //
    // Map the source data buffer for bus master access.
    // Create Tds list, then link it to the OHC's interrupt list
    //
    Status = OhciMapUserData (
                 Ohc,
                 EfiUsbDataIn,
                 Data,
                 DataLength,
                 &PktId,
                 &DataPhy
             );

    if (EFI_ERROR (Status)) {
        goto ON_EXIT;
    }

    TDs = OhciCreateBulkOrIntTds (
              Ohc,
              PktId,
              DataPhy,
              *DataLength,
              DataToggle,
              MaximumPacketLength,
              FALSE
          );

    if (TDs == NULL) {

        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
    }

    //
    // According to the speed of the end point, link
    // the TD to corrosponding ED, then check
    // the execution result
    //
    Ed = OhciCreateEd (Ohc, DeviceAddress, EndPointAddress, IsSlowDevice, 0, MaximumPacketLength, 3);
    if (Ed == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        OhciDestoryTds (Ohc, TDs);
        goto ON_EXIT;
    }

    OhciLinkTdToEd (Ed, TDs);
    OhciLinkEDToIntTree(Ohc, Ed, 1);
    Ed->EdHw.sKip = 0;
    ((OHCI_ED_SW*)(Ed->PreEDSoft))->EdHw.sKip = 0;
    Status = OhciExecuteTransfer (Ohc, (OHCI_ED_SW*)Ed->PreEDSoft, TDs, TimeOut, &EdResult);

    OhciUnlinkEdFromEd ((OHCI_ED_SW*)Ed->PreEDSoft, Ed);
    OhciUnlinkTdFromEd (Ed, TDs);

    *TransferResult = EdResult.Result;
    *DataToggle     = EdResult.NextToggle;
    *DataLength     = EdResult.Complete;

    OhciDestoryTds (Ohc, TDs);

ON_EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
OhciIsochronousTransfer (
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
OhciAsyncIsochronousTransfer (
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
OhciEnableLegacySupport (
    IN       EFI_USB_HC_PROTOCOL                 * This
)
/*++

Routine Description:

  Enable legacy support for OHCI

Arguments:

  This -  A pointer to the EFI_USB_HC_PROTOCOL instance.

Returns:

  EFI_SUCCESS: Enable legacy support successfully.

--*/
{
    USB_OHCI_DEV             *Ohc;
    EFI_STATUS               Status;

    OHCI_DEBUG ((EFI_D_INFO, "OhciEnableLegacySupport: called to enable legacy support\n"));
    Ohc     = OHC_FROM_USB_HC_PROTO (This);

    //
    // Set the interrupt routing to SMI
    //
    OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_IR);

    //
    // Enable "IOC" in Interupt enable register
    //
    OhciSetOpRegBit(Ohc, OHC_HCINTERRUPTENABLE_OFFSET, OHC_HCINTERRUPTENABLE_WDH);

    Status = EFI_SUCCESS;

    return Status;
}

EFI_STATUS
EFIAPI
OhciDisableLegacySupport (
    IN       EFI_USB_HC_PROTOCOL                 * This
)
/*++

Routine Description:

  Dsiable legacy support for OHCI

Arguments:

  This -  A pointer to the EFI_USB_HC_PROTOCOL instance.

Returns:

  EFI_SUCCESS: Enable legacy support successfully.

--*/
{
    USB_OHCI_DEV             *Ohc;
    EFI_STATUS               Status;

    Ohc     = OHC_FROM_USB_HC_PROTO (This);

    //
    // Clear the interrupt routing to SMI
    //
    OhciClearOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_IR);

    //
    // Disable "IOC" in Interupt disable register
    //
    OhciClearOpRegBit(Ohc, OHC_HCINTERRUPTDISABLE_OFFSET, OHC_HCINTERRUPTENABLE_WDH);

    Status = EFI_SUCCESS;

    return Status;
}

EFI_STATUS
EFIAPI
OhciLegacyCallback (
    IN       EFI_USB_HC_PROTOCOL                 * This
)
/*++

Routine Description:

  OHCI SMI callback handler.

Arguments:

  This -  A pointer to the EFI_USB_HC_PROTOCOL instance.

Returns:

  EFI_SUCCESS: Finish the SMI callback successfully.

--*/
{
    USB_OHCI_DEV             *Ohc;
    UINT32                   IntStatus;

    Ohc = OHC_FROM_USB_HC_PROTO (This);

    IntStatus = OhciReadOpReg(Ohc, OHC_HCINTERRUPTSTATUS_OFFSET);

    //
    // Clear all the interrupt status register bits
    //
    OhciSetOpRegBit (Ohc, OHC_HCINTERRUPTSTATUS_OFFSET, 0x0400007F);
    if ((IntStatus & OHC_HCINTERRUPTENABLE_RHSC) &&
            (Ohc->Usb2Hc.BusEnumerationCallback != NULL) &&
            (Ohc->Usb2Hc.EnumerationContext != NULL)) {
        Ohc->Usb2Hc.BusEnumerationCallback (Ohc->Usb2Hc.EnumerationContext);
    }

    if (IntStatus & (OHC_HCINTERRUPTENABLE_WDH))
        OhciProcessAsyncReqList (Ohc);

    if (IntStatus & OHC_HCINTERRUPTENABLE_OC) {
        mOhciOSRequestOwnership = TRUE;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Ohci2Reset (
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
    USB_OHCI_DEV        *OhciDev;

    OhciDev = OHC_FROM_USB2_HC_PROTO (This);

    if ((Attributes == EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG) ||
            (Attributes == EFI_USB_HC_RESET_HOST_WITH_DEBUG)) {
        return EFI_UNSUPPORTED;
    }

    return OhciReset (&OhciDev->UsbHc, Attributes);
}


EFI_STATUS
EFIAPI
Ohci2GetState (
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
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB2_HC_PROTO (This);
    return OhciGetState (&Ohc->UsbHc, State);
}


EFI_STATUS
EFIAPI
Ohci2SetState (
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
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB2_HC_PROTO (This);
    return OhciSetState (&Ohc->UsbHc, State);
}


EFI_STATUS
EFIAPI
Ohci2GetCapability (
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
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB2_HC_PROTO (This);

    if ((NULL == MaxSpeed) || (NULL == PortNumber) || (NULL == Is64BitCapable)) {
        return EFI_INVALID_PARAMETER;
    }

    *MaxSpeed       = EFI_USB_SPEED_FULL;
    *Is64BitCapable = (UINT8) FALSE;

    return OhciGetRootHubPortNumber (&Ohc->UsbHc, PortNumber);
}


EFI_STATUS
EFIAPI
Ohci2GetRootHubPortStatus (
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
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB2_HC_PROTO (This);

    return OhciGetRootHubPortStatus (&Ohc->UsbHc, PortNumber, PortStatus);
}


EFI_STATUS
EFIAPI
Ohci2SetRootHubPortFeature (
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
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB2_HC_PROTO (This);

    return OhciSetRootHubPortFeature (&Ohc->UsbHc, PortNumber, PortFeature);
}


EFI_STATUS
EFIAPI
Ohci2ClearRootHubPortFeature (
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
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB2_HC_PROTO (This);

    return OhciClearRootHubPortFeature (&Ohc->UsbHc, PortNumber, PortFeature);
}


EFI_STATUS
EFIAPI
Ohci2ControlTransfer (
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
    USB_OHCI_DEV        *Ohc;
    BOOLEAN             IsSlow;

    Ohc     = OHC_FROM_USB2_HC_PROTO (This);
    IsSlow  = (EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE;

    return OhciControlTransfer (
               &Ohc->UsbHc,
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
Ohci2BulkTransfer (
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
    USB_OHCI_DEV        *Ohc;
    Ohc = OHC_FROM_USB2_HC_PROTO (This);

    if (Data == NULL || DeviceSpeed == EFI_USB_SPEED_LOW) {
        return EFI_INVALID_PARAMETER;
    }

    //
    // For full-speed bulk transfers only the data pointed by Data[0] shall be used
    //
    return OhciBulkTransfer (
               &Ohc->UsbHc,
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
Ohci2AsyncInterruptTransfer (
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
    USB_OHCI_DEV        *Ohc;
    BOOLEAN             IsSlow;

    Ohc     = OHC_FROM_USB2_HC_PROTO (This);
    IsSlow  = (EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE;

    return OhciAsyncInterruptTransfer (
               &Ohc->UsbHc,
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
Ohci2AsyncInterruptOnlyTransfer (
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
    USB_OHCI_DEV        *Ohc;

    Ohc     = OHC_FROM_USB2_HC_PROTO (This);

    return OhciAsyncInterruptOnlyTransfer (
               &Ohc->UsbHc,
               IsNewTransfer,
               PollingInterval,
               CallBackFunction,
               Context
           );
}


EFI_STATUS
EFIAPI
Ohci2SyncInterruptTransfer (
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
    USB_OHCI_DEV        *Ohc;
    BOOLEAN             IsSlow;

    if (DeviceSpeed == EFI_USB_SPEED_HIGH) {
        return EFI_INVALID_PARAMETER;
    }

    Ohc     = OHC_FROM_USB2_HC_PROTO (This);
    IsSlow  = (EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE;

    return OhciSyncInterruptTransfer (
               &Ohc->UsbHc,
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
Ohci2IsochronousTransfer (
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
Ohci2AsyncIsochronousTransfer (
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

STATIC
EFI_STATUS
EFIAPI
Ohci2VirtualKbc (
    IN  EFI_USB2_HC_PROTOCOL      *This,
    IN  EFI_USB_VIRTUAL_FEATURE   VirtualFeature,
    IN  UINT8                     *State,
    IN  BOOLEAN                   RWFlag
)
{

    USB_OHCI_DEV              *Ohc;
    UINT32                    Data;

    Ohc = OHC_FROM_USB2_HC_PROTO (This);
    Data = 0;

    switch (VirtualFeature) {
    case EfiVirtualOn:
        Data = OhciReadOpReg (Ohc, (UINT32)OHceControl);
        //
        // IRQEn of Ohci will be set always when Virtual on.
        //

        Data |= OHceC_EE | OHceC_EIRQEn | OHceC_IRQEn;
        Data &= ~(OHceC_IRQ1AT | OHceC_IRQ12AT);

        OhciWriteOpReg (Ohc, (UINT32)OHceControl, Data);
        break;

    case EfiVirtualOff:
        Data = OhciReadOpReg (Ohc, (UINT32)OHceControl);

        Data &= ~OHceC_EE;
        Data &= ~(OHceC_IRQ1AT | OHceC_IRQ12AT);

        OhciWriteOpReg (Ohc, (UINT32)OHceControl, Data);
        break;

    case EfiVirtualStatus:
        Data = OhciReadOpReg (Ohc, (UINT32)OHceControl);
        if ((Data & OHceC_EI) != OHceC_EI) {
            *State = KBC_NO_EMULATION;
            break;
        }

        Data = OhciReadOpReg (Ohc, (UINT32)OHceStatus);

        if ((Data & OHceS_InPutFull) == OHceS_InPutFull)
            *State = KBC_PORT60_WRITE;

        if ((Data & OHceS_CmdData) == OHceS_CmdData)
            *State = KBC_PORT64_WRITE;

        Data |= OHceS_Flag|OHceS_IS;
        Data &= ~(OHceS_InPutFull|OHceS_CmdData);
        OhciWriteOpReg (Ohc, (UINT32)OHceStatus, Data);

        Data = OhciReadOpReg (Ohc, (UINT32)OHceControl);
        if ((Data & OHceC_IRQ1AT) == OHceC_IRQ1AT)
            *State = KBC_KBMS_DATA;
        if ((Data & OHceC_IRQ12AT) == OHceC_IRQ12AT)
            *State = KBC_KBMS_DATA;
        OhciWriteOpReg (Ohc, (UINT32)OHceControl, Data);

        break;

    case EfiVirtualClearStatus:
        break;

    case EfiVirtualInputData:
        Data = OhciReadOpReg (Ohc, (UINT32)OHceInput);
        *State = (UINT8) Data;
        break;

    case EfiVirtualOuputData:
        if (RWFlag) {
            Data = (UINT32) *State;
            OhciWriteOpReg (Ohc, (UINT32)OHceOutput, Data);
        } else {
            Data = OhciReadOpReg (Ohc, (UINT32)OHceOutput);
            *State = (UINT8) Data;
        }
        break;

    case EfiVirtualControlReg:

        if (RWFlag) {
            Data = (UINT32) *State;
            OhciWriteOpReg (Ohc, (UINT32)OHceControl, Data);
        } else {
            Data = OhciReadOpReg (Ohc, (UINT32)OHceControl);
            *State = (UINT8) Data;
        }
        break;

    case EfiVirtualStatusReg:
        if (RWFlag) {
            Data = (UINT32) *State;
            OhciWriteOpReg (Ohc, (UINT32)OHceStatus, Data);
        } else {
            Data = OhciReadOpReg (Ohc, (UINT32)OHceStatus);
            *State = (UINT8) Data;
        }
        break;

    default:
        break;
    }
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Ohci2DisableLegacySupport (
    IN       EFI_USB2_HC_PROTOCOL                 * This
)
/*++

Routine Description:

  Disable legacy support of OhcI device

Arguments:

  This                : A pointer to the EFI_USB2_HC_PROTOCOL instance.

Returns:



--*/
{
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB2_HC_PROTO (This);

    return OhciDisableLegacySupport (
               &Ohc->UsbHc
           );
}


EFI_STATUS
EFIAPI
Ohci2EnableLegacySupport (
    IN       EFI_USB2_HC_PROTOCOL                 * This
)
/*++

Routine Description:

  Enable legacy support of OhcI device

Arguments:

  This : A pointer to the EFI_USB2_HC_PROTOCOL instance.

Returns:



--*/
{
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB2_HC_PROTO (This);

    return OhciEnableLegacySupport (
               &Ohc->UsbHc
           );
}


EFI_STATUS
EFIAPI
Ohci2LegacyCallback (
    IN       EFI_USB2_HC_PROTOCOL                 * This
)
/*++

Routine Description:


Arguments:


Returns:



--*/
{
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB2_HC_PROTO (This);

    return OhciLegacyCallback (
               &Ohc->UsbHc
           );
}

USB_OHCI_DEV *
OhciAllocateDev (
    IN EFI_PCI_FUNCTION_ADDRESS   *PciAddress
)
/*++

Routine Description:

  Allocate and initialize the empty OHCI device

Arguments:

  PciIo - The PCIIO to use

Returns:

  Allocated OHCI device

--*/
{
    USB_OHCI_DEV          *Ohc;
    EFI_STATUS            Status;
    UINT64                PciRegAddress;
    FULL_USB_DEVICE_PATH  ControllerPath = USB_DEVICE_PATH_TEMPLATE;

    Ohc = AllocateZeroPool (sizeof (USB_OHCI_DEV));
    if (Ohc == NULL)
        return NULL;

    //
    // This driver supports both USB_HC_PROTOCOL and USB2_HC_PROTOCOL.
    // USB_HC_PROTOCOL is for EFI 1.1 backward compability.
    //
    Ohc->Signature                        = USB_OHCI_DEV_SIGNATURE;
    Ohc->UsbHc.Reset                      = OhciReset;
    Ohc->UsbHc.GetState                   = OhciGetState;
    Ohc->UsbHc.SetState                   = OhciSetState;
    Ohc->UsbHc.ControlTransfer            = OhciControlTransfer;
    Ohc->UsbHc.BulkTransfer               = OhciBulkTransfer;
    Ohc->UsbHc.AsyncInterruptTransfer     = OhciAsyncInterruptTransfer;
    Ohc->UsbHc.SyncInterruptTransfer      = OhciSyncInterruptTransfer;
    Ohc->UsbHc.IsochronousTransfer        = OhciIsochronousTransfer;
    Ohc->UsbHc.AsyncIsochronousTransfer   = OhciAsyncIsochronousTransfer;
    Ohc->UsbHc.GetRootHubPortNumber       = OhciGetRootHubPortNumber;
    Ohc->UsbHc.GetRootHubPortStatus       = OhciGetRootHubPortStatus;
    Ohc->UsbHc.SetRootHubPortFeature      = OhciSetRootHubPortFeature;
    Ohc->UsbHc.ClearRootHubPortFeature    = OhciClearRootHubPortFeature;
    Ohc->UsbHc.MajorRevision              = 0x1;
    Ohc->UsbHc.MinorRevision              = 0x1;
    Ohc->UsbHc.AsyncInterruptOnlyTransfer = OhciAsyncInterruptOnlyTransfer;
    Ohc->UsbHc.EnableLegacySupport        = OhciEnableLegacySupport;
    Ohc->UsbHc.DisableLegacySupport       = OhciDisableLegacySupport;
    Ohc->UsbHc.LegacyCallback             = OhciLegacyCallback;

    Ohc->Usb2Hc.GetCapability             = Ohci2GetCapability;
    Ohc->Usb2Hc.Reset                     = Ohci2Reset;
    Ohc->Usb2Hc.GetState                  = Ohci2GetState;
    Ohc->Usb2Hc.SetState                  = Ohci2SetState;
    Ohc->Usb2Hc.ControlTransfer           = Ohci2ControlTransfer;
    Ohc->Usb2Hc.BulkTransfer              = Ohci2BulkTransfer;
    Ohc->Usb2Hc.AsyncInterruptTransfer    = Ohci2AsyncInterruptTransfer;
    Ohc->Usb2Hc.SyncInterruptTransfer     = Ohci2SyncInterruptTransfer;
    Ohc->Usb2Hc.IsochronousTransfer       = Ohci2IsochronousTransfer;
    Ohc->Usb2Hc.AsyncIsochronousTransfer  = Ohci2AsyncIsochronousTransfer;
    Ohc->Usb2Hc.GetRootHubPortStatus      = Ohci2GetRootHubPortStatus;
    Ohc->Usb2Hc.SetRootHubPortFeature     = Ohci2SetRootHubPortFeature;
    Ohc->Usb2Hc.ClearRootHubPortFeature   = Ohci2ClearRootHubPortFeature;
    Ohc->Usb2Hc.MajorRevision             = 0x1;
    Ohc->Usb2Hc.MinorRevision             = 0x1;

    Ohc->Usb2Hc.GSetVirtualState          = Ohci2VirtualKbc;
    Ohc->Usb2Hc.AsyncInterruptOnlyTransfer = Ohci2AsyncInterruptOnlyTransfer;
    Ohc->Usb2Hc.EnableLegacySupport       = Ohci2EnableLegacySupport;
    Ohc->Usb2Hc.DisableLegacySupport      = Ohci2DisableLegacySupport;
    Ohc->Usb2Hc.LegacyCallback            = Ohci2LegacyCallback;
    Ohc->PciAddress.Bus                   = PciAddress->Bus;
    Ohc->PciAddress.Device                = PciAddress->Device;
    Ohc->PciAddress.Function              = PciAddress->Function;

    PciRegAddress = SMM_PCI_ADDRESS (
                        Ohc->PciAddress.Bus,
                        Ohc->PciAddress.Device,
                        Ohc->PciAddress.Function,
                        OHC_PCI_MEM_BASE_ADDRESS
                    );
    Status = SmmPciCfgRead (
                 gSmst,
                 SmmPciWidthUint32,
                 (SMM_PCI_IO_ADDRESS *) &PciRegAddress,
                 &Ohc->UsbMemBase
             );
    if (EFI_ERROR(Status)) {
        goto ON_ERROR;;
    }

    Ohc->UsbMemBase &= OHC_PCI_MEM_BASE_MASK;
    Ohc->DevicePath = AllocateZeroPool (sizeof (ControllerPath));
    if (Ohc->DevicePath == NULL)
        goto ON_ERROR;

    ControllerPath.Pci.Device = (UINT8)Ohc->PciAddress.Device;
    ControllerPath.Pci.Function = (UINT8) Ohc->PciAddress.Function;
    CopyMem (
        Ohc->DevicePath,
        &ControllerPath,
        sizeof (ControllerPath));

    InitializeListHead (&Ohc->AsyncIntList);
    return Ohc;

ON_ERROR:
    FreePool (Ohc);
    return NULL;
}


EFI_STATUS
EFIAPI
UsbConnectOhciDriver (
    IN EFI_PCI_FUNCTION_ADDRESS   *PciAddress,
    OUT EFI_USB2_HC_PROTOCOL      **Usb2HCInterface,
    OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
)
/*++

  Routine Description:

    Starting the Usb OHCI Driver

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
    USB_OHCI_DEV        *Ohc;
    UINTN               WaitPeriod;
    UINTN               Index;
    UINT32              Data;

    Ohc = OhciAllocateDev (PciAddress);

    if (Ohc == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
    }

    Status = OhciEnableDevice (Ohc);
    if (EFI_ERROR (Status)) {
        goto FREE_OHC;
    }

    //
    // Set the Global Reset bit in the command status register
    //
    OhciSetOpRegBit (Ohc, OHC_HCCOMMANDSTATUS_OFFSET, OHC_HCCOMMANDSTATUS_HCR);
    WaitPeriod = OHC_1_MILLISECOND;

    for (Index = 0; Index < OHC_PORT_RESET_RECOVERY_STALL / WaitPeriod; Index++) {
        Data = OhciReadOpReg(Ohc, OHC_HCCOMMANDSTATUS_OFFSET);
        if (!(Data & OHC_HCCOMMANDSTATUS_HCR))
            break;
        Stall (OHC_ROOT_PORT_ENABLE_STALL);
    }

    if (Index == (OHC_PORT_RESET_RECOVERY_STALL / WaitPeriod)) {
        Status = EFI_DEVICE_ERROR;
        goto FREE_OHC;
    }

    //
    // OHCI Host Controller Operational Registers and HCCA
    //
    Status = OhcInitHc(Ohc);

    if (EFI_ERROR (Status)) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FREE_OHC;
    }

    *Usb2HCInterface = &Ohc->Usb2Hc;
    *DevicePath      = Ohc->DevicePath;
    InsertTailList (&mOhcList, &Ohc->Link);

    return EFI_SUCCESS;

FREE_OHC:
    OhciFreeDev (Ohc);
    return Status;
}

VOID
OhciFreeDev (
    IN USB_OHCI_DEV         *Ohc
)
/*++

Routine Description:

  Free the OHCI device and release its associated resources

Arguments:

  Ohc - The OHCI device to release

Returns:

  None

--*/
{
    UINTN                 Pages;
    UINTN                 Bytes;

    //
    // Allocate the OHCI HCCA structure
    //
    Bytes = EFI_PAGE_SIZE;
    Pages = EFI_SIZE_TO_PAGES (Bytes);

    if (Ohc->HccaBase) {
        UsbFreePage(Ohc->HccaBase,Pages);
    }

    FreePool (Ohc);
}

VOID
OhciCleanDevUp (
    IN USB_OHCI_DEV        *Ohc
)
/*++
  Routine Description:

    Uninstall all Ohci Interface

  Arguments:

    Controller        - Controller handle
    This              - Protocol instance pointer.

  Returns:

    VOID

--*/
{

    //
    // Set the Global Reset bit in the control register
    //
    OhciClearOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS);
    OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS_RESET);
    Stall (OHC_PORT_RESET_RECOVERY_STALL);
    //
    // Clear the interrupt routing to SMI
    //
    OhciClearOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_IR);
    //
    // Clear HcRhDescriptorA Register
    //
    OhciClearOpRegBit(Ohc, OHC_HCRHDESCRIPTORA_OFFSET, OHC_HCRHDESCRIPTORA_PSM |
                      OHC_HCRHDESCRIPTORA_NPS| OHC_HCRHDESCRIPTORA_OCPM | OHC_HCRHDESCRIPTORA_NCP);

    OhciFreeAllAsyncReq (Ohc);
    OhciFreeDev (Ohc);
}

EFI_STATUS
EFIAPI
UsbDisconnectOhciDriver (
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
    USB_OHCI_DEV        *Ohc;

    Ohc = OHC_FROM_USB2_HC_PROTO (Usb2HCInterface);
    OhciCleanDevUp (Ohc);
    return EFI_SUCCESS;
}

/********************************************************************************
  Schedule function
********************************************************************************/
EFI_STATUS
OhciMapUserData (
    IN  USB_OHCI_DEV            *Ohc,
    IN  EFI_USB_DATA_DIRECTION  Direction,
    IN  VOID                    *Data,
    IN  OUT UINTN               *Len,
    OUT UINT8                   *PktId,
    OUT UINT8                   **MappedAddr
)
/*++

Routine Description:

  Map address of user data buffer

Arguments:

  Ohc        - The OHCI device
  Direction  - direction of the data transfer
  Data       - The user data buffer
  Len        - Length of the user data
  PktId      - Packet identificaion
  MappedAddr - mapped address to return
  Map        - identificaion of this mapping to return

Returns:

  EFI_SUCCESS      : Success
  EFI_DEVICE_ERROR : Fail to map the user data

--*/
{
    EFI_STATUS            Status;

    Status = EFI_SUCCESS;

    switch (Direction) {
    case EfiUsbDataIn:
        //
        // BusMasterWrite means cpu read
        //
        *PktId = OHC_INPUT_PACKET_ID;
        *MappedAddr = Data;
        break;

    case EfiUsbDataOut:
        *PktId = OHC_OUTPUT_PACKET_ID;
        *MappedAddr = Data;
        break;

    case EfiUsbNoData:
        if ((Len != NULL) && (*Len != 0)) {
            Status = EFI_INVALID_PARAMETER;
            goto EXIT;
        }

        *PktId      = OHC_OUTPUT_PACKET_ID;
        *Len        = 0;
        *MappedAddr = NULL;
        break;

    default:
        Status      = EFI_INVALID_PARAMETER;
    }

EXIT:
    return Status;
}

OHCI_ED_SW *
OhciCreateEd (
    IN  USB_OHCI_DEV      *Ohc,
    IN  UINT8             DeviceAddress,
    IN  UINT8             EndpointNumber,
    IN  BOOLEAN           IsSlowDevice,
    IN  BOOLEAN           Format,
    IN  UINT8             MaxPacketLength,
    IN  UINT8             Direction
)
/*++

Routine Description:

  Create an initialize a new queue head

Arguments:

  Ohc       - The OHCI device

Returns:

  The newly created ED

--*/
{
    OHCI_ED_SW            *Ed;

    Ed = UsbAllocatePool (sizeof (OHCI_ED_SW));

    if (Ed == NULL) {
        return NULL;
    }

    Ed->EdHw.FunctionAdress     = DeviceAddress & 0x7f;
    Ed->EdHw.EndpointNumber     = EndpointNumber ;
    Ed->EdHw.Direction          = Direction;
    Ed->EdHw.Speed              = IsSlowDevice;
    Ed->EdHw.sKip               = 0;
    Ed->EdHw.Format             = Format;
    Ed->EdHw.MaximumPacketSize  = MaxPacketLength ;
    Ed->EdHw.NextED             = 0;
    return Ed;
}

VOID
OhciAppendTd (
    IN OHCI_TD_SW     *PrevTd,
    IN OHCI_TD_SW     *ThisTd
)
/*++

Routine Description:

  Append a new TD To the previous TD

Arguments:

  PrevTd  - Previous OHCI_TD_SW to be linked to
  ThisTd  - TD to link

Returns:

  VOID

--*/
{
    ASSERT ((PrevTd != NULL) && (ThisTd != NULL));

    PrevTd->TdHw.NextTD = (((UINT32)(UINTN)ThisTd) >> 4);
    PrevTd->NextTDSoft  = (UINT32)(UINTN)ThisTd;
}

VOID
OhciDestoryTds (
    IN USB_OHCI_DEV         *Ohc,
    IN OHCI_TD_SW           *FirstTd
)
/*++
Routine Description:

  Delete a list of TDs

Arguments:

  Ohc       - The OHCI device
  FirstTd   - TD link list head

Returns:

  VOID

--*/
{
    OHCI_TD_SW            *NextTd;
    OHCI_TD_SW            *ThisTd;

    NextTd = FirstTd;

    while (NextTd != NULL) {
        ThisTd  = NextTd;
        NextTd  = (OHCI_TD_SW*)ThisTd->NextTDSoft;
        UsbFreePool (ThisTd, sizeof (OHCI_TD_SW));
    }
}

OHCI_TD_SW *
OhciCreateTd (
    IN  USB_OHCI_DEV        *Ohc
)
/*++

Routine Description:

  Create and intialize a TD

Arguments:

  Ohc   - The OHCI device

Returns:

  The newly allocated and initialized TD

--*/
{
    OHCI_TD_SW              *Td;

    Td     = UsbAllocatePool (sizeof (OHCI_TD_SW));
    if (Td == NULL) {
        return NULL;
    }

    Td->TdHw.NextTD   = 0;
    Td->Data          = NULL;
    Td->NextTDSoft    = 0;
    Td->BufferLength   = 0;

    return Td;
}


VOID
OhciLinkTdToEd (
    IN OHCI_ED_SW           *Ed,
    IN OHCI_TD_SW           *Td
)
/*++

Routine Description:

  Link the TD To ED

Arguments:

  Ed  - The Endpoint Descriptor for the TD to link to
  Td  - The TD to link

Returns:

  VOID

--*/
{
    OHCI_TD_SW    *TdTail;

    ASSERT ((Ed != NULL) && (Td != NULL));

    Ed->EdHw.TDQueueHeadPointer = ((UINT32)(UINTN)Td) >> 4;

    TdTail = Td;
    while (TdTail)
        TdTail = (OHCI_TD_SW*)(((UINT32)TdTail->TdHw.NextTD) << 4);

    Ed->EdHw.TDQueueTailPointer = ((UINT32)(UINTN)TdTail) >> 4;

    Ed->EdHw.sKip = 0;

}

VOID
OhciLinkEdToEd (
    IN OHCI_ED_SW     *ED,
    IN OHCI_ED_SW     *NextED
)
/*++

  Routine Description:

    Set Next ED link of Endpoint Descriptor

  Arguments:

    ED       : A pointer of ED_STRUCT
    NextED   : Next Ed

  Returns:

    None

--*/
{
    ED->EdHw.NextED = ((UINT32)(UINTN)NextED) >> 4;
}

VOID
OhciUnlinkTdFromEd (
    IN OHCI_ED_SW           *Ed,
    IN OHCI_TD_SW           *Td
)
/*++

Routine Description:

  Unlink TD from the Ed

Arguments:

  Ed  - The endpoint descriptor to unlink from
  Td  - The TD to unlink

Returns:

  VOID

--*/
{
    ASSERT ((Ed != NULL) && (Td != NULL));

    UsbFreePool (Ed, sizeof (OHCI_ED_SW));
}

VOID
OhciUnlinkEdFromEd (
    IN OHCI_ED_SW           *Ed,
    IN OHCI_ED_SW           *NextEd
)
/*++

Routine Description:

  Unlink Ed from the Ed

Arguments:

  Ed  - The endpoint descriptor to unlink from
  NextEd  - The Ed to unlink

Returns:

  VOID

--*/
{
    ASSERT ((Ed != NULL) && (NextEd != NULL));
    Ed->EdHw.NextED = NextEd->NextEDSoft >> 4;
}


OHCI_TD_SW *
OhciCreateSetupTd (
    IN  USB_OHCI_DEV      *Ohc,
    IN  UINT8               *Request
)
/*++

Routine Description:

  Create and initialize a TD for Setup Stage of a control transfer

Arguments:

  Ohc         - The OHCI device
  Request     - Device request

Returns:

  The created setup Td Pointer

--*/
{
    OHCI_TD_SW              *Td;

    Td = OhciCreateTd (Ohc);

    if (Td == NULL) {
        return NULL;
    }

    Td->TdHw.BufferRounding       = 1;
    Td->TdHw.DirectionPID         = OHC_SETUP_PACKET_ID;
    Td->TdHw.DelayInterrupt       = 7;
    Td->TdHw.DataToggle           = 0;
    Td->TdHw.CurrentBufferPointer = (UINT32)(UINTN)Request;
    Td->TdHw.NextTD               = 0;
    Td->NextTDSoft                = 0;
    Td->TdHw.BufferEnd            = (UINT32)(UINTN)Request + sizeof (EFI_USB_DEVICE_REQUEST) - 1;

    return Td;
}

OHCI_TD_SW *
OhciCreateDataTd (
    IN  USB_OHCI_DEV        *Ohc,
    IN  UINT8               *DataPtr,
    IN  UINTN               Len,
    IN  UINT8               PktId,
    IN  UINT8               Toggle
)
/*++

Routine Description:

  Create a TD for data

Arguments:

  Ohc         - The OHCI device
  DevAddr     - Device address
  Endpoint    - Endpoint number
  DataPtr     - Data buffer
  Len         - Data length
  PktId       - Packet ID
  Toggle      - Data toggle value

Returns:

  Data Td pointer if success, otherwise NUL

--*/
{
    OHCI_TD_SW  *Td;

    Td  = OhciCreateTd (Ohc);

    if (Td == NULL) {
        return NULL;
    }

    Td->TdHw.BufferRounding       = 0;
    Td->TdHw.DirectionPID         = PktId;
    Td->TdHw.DelayInterrupt       = 7;
    if (Toggle)
        Td->TdHw.DataToggle         = Toggle + 2;
    else
        Td->TdHw.DataToggle         = 0;
    Td->TdHw.CurrentBufferPointer = (UINT32)(UINTN)DataPtr;
    Td->TdHw.NextTD               = 0;
    Td->NextTDSoft                = 0;
    Td->TdHw.BufferEnd            = (UINT32)(UINTN)(DataPtr + Len - 1);
    Td->Data                      = DataPtr;
    Td->BufferLength              = (UINT16)Len;

    if (!Len) {
        Td->TdHw.BufferEnd = 0;
    }

    return Td;
}

OHCI_TD_SW *
OhciCreateStatusTd (
    IN  USB_OHCI_DEV        *Ohc,
    IN  UINT8               PktId
)
/*++

Routine Description:

  Create TD for the Status Stage of control transfer

Arguments:

  Ohc         - The OHCI device
  PktId       - Packet ID

Returns:

  Status Td Pointer

--*/
{
    OHCI_TD_SW  *Td;

    Td  = OhciCreateTd (Ohc);

    if (Td == NULL) {
        return NULL;
    }

    Td->TdHw.BufferRounding       = 1;
    Td->TdHw.DirectionPID         = PktId;
    Td->TdHw.DelayInterrupt       = 7;
    Td->TdHw.DataToggle           = 3;
    Td->TdHw.CurrentBufferPointer = 0;
    Td->TdHw.NextTD               = 0;
    Td->NextTDSoft                = 0;
    Td->TdHw.BufferEnd            = 0;
    Td->Data                      = NULL;
    Td->BufferLength              = 0;

    return Td;
}

OHCI_TD_SW *
OhciCreateCtrlTds (
    IN USB_OHCI_DEV         *Ohc,
    IN UINT8                DeviceAddr,
    IN UINT8                DataPktId,
    IN UINT8                *Request,
    IN UINT8                *Data,
    IN UINTN                DataLen,
    IN UINT8                MaxPacket
)
/*++

Routine Description:

  Create Tds list for Control Transfer

Arguments:

  Ohc         - The OHCI device
  DeviceAddr  - The device address
  DataPktId   - Packet Identification of Data Tds
  Request     - A pointer to request structure buffer to transfer
  Data        - A pointer to user data buffer to transfer
  DataLen     - Length of user data to transfer
  MaxPacket   - Maximum packet size for control transfer

Returns:

  The Td list head for the control transfer

--*/
{
    OHCI_TD_SW                *SetupTd;
    OHCI_TD_SW                *FirstDataTd;
    OHCI_TD_SW                *DataTd;
    OHCI_TD_SW                *PrevDataTd;
    OHCI_TD_SW                *StatusTd;
    UINT8                     DataToggle;
    UINT8                     StatusPktId;
    UINTN                     ThisTdLen;

    DataTd      = NULL;
    SetupTd     = NULL;
    FirstDataTd = NULL;
    PrevDataTd  = NULL;
    StatusTd    = NULL;

    //
    // Create setup packets for the transfer
    //
    SetupTd = OhciCreateSetupTd (Ohc, Request);

    if (SetupTd == NULL) {
        return NULL;
    }

    //
    // Create data packets for the transfer
    //
    DataToggle = 1;

    while (DataLen > 0) {
        //
        // PktSize is the data load size in each Td.
        //
        ThisTdLen = (DataLen > MaxPacket ? MaxPacket : DataLen);

        DataTd = OhciCreateDataTd (
                     Ohc,
                     Data,
                     ThisTdLen,
                     DataPktId,
                     DataToggle
                 );

        if (DataTd == NULL) {
            goto FREE_TD;
        }

        if (FirstDataTd == NULL) {
            FirstDataTd = DataTd;
            FirstDataTd->TdHw.NextTD = 0;
        } else {
            OhciAppendTd (PrevDataTd, DataTd);
        }

        DataToggle ^= 1;
        PrevDataTd = DataTd;
        Data += ThisTdLen;
        DataLen -= ThisTdLen;
    }

    //
    // Status packet is on the opposite direction to data packets
    //
    if (OHC_OUTPUT_PACKET_ID == DataPktId) {
        StatusPktId = OHC_INPUT_PACKET_ID;
    } else {
        StatusPktId = OHC_OUTPUT_PACKET_ID;
    }

    StatusTd = OhciCreateStatusTd (Ohc, StatusPktId);

    if (StatusTd == NULL) {
        goto FREE_TD;
    }

    //
    // Link setup Td -> data Tds -> status Td together
    //
    if (FirstDataTd != NULL) {
        OhciAppendTd (SetupTd, FirstDataTd);
        OhciAppendTd (PrevDataTd, StatusTd);
    } else {
        OhciAppendTd (SetupTd, StatusTd);
    }

    return SetupTd;

FREE_TD:
    if (SetupTd != NULL) {
        OhciDestoryTds (Ohc, SetupTd);
    }

    if (FirstDataTd != NULL) {
        OhciDestoryTds (Ohc, FirstDataTd);
    }

    if (StatusTd != NULL) {
        OhciDestoryTds (Ohc, StatusTd);
    }

    return NULL;
}

OHCI_TD_SW *
OhciCreateBulkOrIntTds (
    IN USB_OHCI_DEV         *Ohc,
    IN UINT8                PktId,
    IN UINT8                *Data,
    IN UINTN                DataLen,
    IN OUT UINT8            *DataToggle,
    IN UINT8                MaxPacket,
    IN BOOLEAN              IsIOCSet
)
/*++

Routine Description:

  Create Tds list for Bulk/Interrupt Transfer

Arguments:

  Ohc          - USB_OHCI_DEV
  PktId        - Packet Identification of Data Tds
  Data         - A pointer to user data buffer to transfer
  DataLen      - Length of user data to transfer
  DataToggle   - Data Toggle Pointer
  MaxPacket    - Maximum packet size for Bulk/Interrupt transfer
  IsIOCSet     - Interrupt delay count for this TD.

Returns:

  The Tds list head for the bulk transfer

--*/
{
    OHCI_TD_SW              *DataTd;
    OHCI_TD_SW              *FirstDataTd;
    OHCI_TD_SW              *PrevDataTd;
    UINTN                   ThisTdLen;

    DataTd      = NULL;
    FirstDataTd = NULL;
    PrevDataTd  = NULL;

    //
    // Create data packets for the transfer
    //
    while (DataLen > 0) {
        //
        // PktSize is the data load size that each Td.
        //
        ThisTdLen = DataLen;

        if (DataLen > MaxPacket) {
            ThisTdLen = MaxPacket;
        }

        DataTd = OhciCreateDataTd (
                     Ohc,
                     Data,
                     ThisTdLen,
                     PktId,
                     *DataToggle
                 );

        if (DataTd == NULL) {
            goto FREE_TD;
        }

        if (IsIOCSet) {
            DataTd->TdHw.DelayInterrupt =  0;
            DataTd->TdHw.BufferRounding = 1;
        }

        if (FirstDataTd == NULL) {
            FirstDataTd = DataTd;
            FirstDataTd->TdHw.NextTD = 0;
        } else {
            OhciAppendTd (PrevDataTd, DataTd);
        }

        *DataToggle ^= 1;
        PrevDataTd   = DataTd;
        Data        += ThisTdLen;
        DataLen     -= ThisTdLen;
    }

    //
    // For interrupt only transfer data Td
    //
    if ((Data == NULL) && (DataLen == 0)) {
        DataTd = OhciCreateDataTd (
                     Ohc,
                     NULL,
                     0,
                     PktId,
                     0
                 );

        if (DataTd == NULL) {
            goto FREE_TD;
        }

        DataTd->TdHw.DelayInterrupt =  0;
        DataTd->TdHw.BufferRounding = 1;
        DataTd->TdHw.ConditionCode  = USBTD_NOTACCESSED;
        DataTd->TdHw.ErrorCount     = 2;
        DataTd->TdHw.BufferEnd      = 0;
        FirstDataTd = DataTd;
        PrevDataTd  = DataTd;
    }

    return FirstDataTd;

FREE_TD:
    if (FirstDataTd != NULL) {
        OhciDestoryTds (Ohc, FirstDataTd);
    }

    return NULL;
}

BOOLEAN
OhciCheckTdStatus (
    IN  USB_OHCI_DEV        *Ohc,
    IN  OHCI_ED_SW          *Ed,
    IN  OHCI_TD_SW          *Td,
    OUT OHCI_ED_RESULT      *EdResult
)
/*++

Routine Description:

  Check TDs Results

Arguments:

  Ohc         - This OHCI device
  Ed          - OHCI_ED_SW to check
  Td          - OHCI_TD_SW to check
  EdResult    - Return the result of this TD list

Returns:

  Whether the TD's result is finialized.

--*/
{
    UINTN                   Len;
    UINT8                   State;
    OHCI_TD_SW              *pTd;
    OHCI_ED_SW              *pEd;
    BOOLEAN                 Finished;

    Finished             = TRUE;

    //
    // Initialize the data toggle to that of the first
    // TD. The next toggle to use is either:
    // 1. first TD's toggle if no TD is executed OK
    // 2. the next toggle of last executed-OK TD
    //
    EdResult->Result     = EFI_USB_NOERROR;
    EdResult->NextToggle = (UINT8)Td->TdHw.DataToggle;
    EdResult->Complete   = 0;

    pTd = Td;
    pEd = (OHCI_ED_SW*)((UINT32)Ed->EdHw.NextED << 4);

    while (pTd != NULL) {
        State = (UINT8)pTd->TdHw.ConditionCode;

        if (State == USBTD_NOTACCESSED ) {
            Finished = FALSE;
            goto ON_EXIT;
        }

        if (State == USBTD_CRCERROR) {
            EdResult->Result |= EFI_USB_ERR_CRC;
            Finished = TRUE;
            goto ON_EXIT;
        }

        if (State == USBTD_BITSTUFFING) {
            EdResult->Result |= EFI_USB_ERR_BITSTUFF;
            Finished = TRUE;
            goto ON_EXIT;
        }

        if (State == USBTD_STALL) {
            EdResult->Result |= EFI_USB_ERR_STALL;
            Finished = TRUE;
            goto ON_EXIT;
        }

        if (State == USBTD_BUFFEROVERRUN) {
            EdResult->Result |= EFI_USB_ERR_BUFFER;
            Finished = TRUE;
            goto ON_EXIT;
        }

        if (State == USBTD_BUFFERUNDERRUN) {
            EdResult->Result |= EFI_USB_ERR_BUFFER;
            Finished = TRUE;
            goto ON_EXIT;
        }

        if ((State & 0x05) == 0x05) {
            Finished = TRUE;
            goto ON_EXIT;
        }

        if (State & 0x0F) {
            EdResult->Result |= EFI_USB_ERR_SYSTEM;
            Finished = TRUE;
            goto ON_EXIT;
        }

        if (pEd->EdHw.TDQueueHeadPointer != pEd->EdHw.TDQueueTailPointer) {
            //
            // The TD is still active, no need to check further.
            //
            EdResult->Result |= EFI_USB_ERR_NOTEXECUTE;
            Finished = FALSE;
            goto ON_EXIT;
        }

        //
        // Update the next data toggle, it is always the
        // next to the last known-good TD's data toggle if
        // any TD is executed OK
        //
        EdResult->NextToggle = ((UINT8)pTd->TdHw.DataToggle & 0x1);

        //
        // This TD is finished OK or met short packet read. Update the
        // transfer length if it isn't a SETUP.
        //
        Len = pTd->BufferLength;

        if (pTd->TdHw.DirectionPID != OHC_SETUP_PACKET_ID) {
            EdResult->Complete += Len;
        }

        pTd = (OHCI_TD_SW*)((UINT32)pTd->NextTDSoft);
    }

ON_EXIT:
    //
    // Check whether HC is halted. Don't move this up. It must be
    // called after data toggle is successfully updated.
    //
    if (!OhciIsHcWorking (Ohc)) {
        EdResult->Result |= EFI_USB_ERR_SYSTEM;
        Finished  = TRUE;
    }

    OhciAckAllInterrupt (Ohc);
    return Finished;
}

EFI_STATUS
OhciExecuteTransfer (
    IN  USB_OHCI_DEV        *Ohc,
    IN  OHCI_ED_SW          *Ed,
    IN  OHCI_TD_SW          *Td,
    IN  UINTN               TimeOut,
    OUT OHCI_ED_RESULT      *EdResult
)
/*++

Routine Description:

  Check the result of the transfer

Arguments:

  Ohc            - The OHCI device
  Ed             - The endpoint descriptor of the transfer
  Td             - The first TDs of the transfer
  TimeOut        - TimeOut value in milliseconds
  EdResult       - The variable to return result

Returns:

  EFI_SUCCESS      - The transfer finished with success
  EFI_DEVICE_ERROR - Transfer failed

--*/
{
    UINTN                   Index;
    UINTN                   Delay;
    BOOLEAN                 Finished;
    EFI_STATUS              Status;
    BOOLEAN                 InfiniteLoop;

    Finished = FALSE;
    Status   = EFI_SUCCESS;
    Delay    = (TimeOut * OHC_1_MILLISECOND / OHC_SYNC_POLL_INTERVAL) + 1;
    InfiniteLoop = FALSE;

    //
    // According to UEFI spec section 16.2.4, If Timeout is 0, then the caller
    // must wait for the function to be completed until EFI_SUCCESS or EFI_DEVICE_ERROR
    // is returned.
    //
    if (TimeOut == 0) {
        InfiniteLoop = TRUE;
    }

    for (Index = 0; InfiniteLoop || (Index < Delay); Index++) {
        Finished = OhciCheckTdStatus (Ohc, Ed, Td, EdResult);

        //
        // Transfer is OK or some error occured
        //
        if (Finished) {
            break;
        }

        Stall (OHC_SYNC_POLL_INTERVAL);
    }

    if (!Finished) {
        OHCI_DEBUG (( EFI_D_ERROR, "OhciExecuteTransfer: execution not finished for %dms\n", TimeOut));
        OhciDumpEd (Ed);
        OhciDumpTds (Td);
        Status = EFI_TIMEOUT;
    } else if (EdResult->Result != EFI_USB_NOERROR) {
        OHCI_DEBUG (( EFI_D_ERROR, "OhciExecuteTransfer: execution failed with result %x\n", EdResult->Result));
        OhciDumpEd (Ed);
        OhciDumpTds (Td);
        Status = EFI_DEVICE_ERROR;
    }

    return Status;
}

UINTN
OhciConvertPollRate (
    IN  UINTN               Interval
)
/*++

Routine Description:

  Convert the poll rate to the maxium 2^n that is smaller
  than Interval

Arguments:

  Interval  - The poll rate to convert

Returns:

  The converted poll rate

--*/
{
    UINTN                   BitCount;
    UINTN                   Index;

    ASSERT (Interval != 0);

    //
    // Find the index (1 based) of the highest non-zero bit
    //
    BitCount = 0;

    while (Interval != 0) {
        Interval >>= 1;
        BitCount++;
    }

    Interval = 1;

    for (Index = 0; Index < BitCount - 1; Index++)
        Interval = Interval << 1;

    return Interval;

}

VOID
OhciLinkEDToIntTree(
    IN USB_OHCI_DEV         *Ohc,
    IN OHCI_ED_SW           *Ed,
    IN UINTN                PollingInterval
)
/*++

  Routine Description:

    Insert Endpoint Descriptor to interrupt list

  Arguments:

    HcDev         : The OHCI device
    UINT8         : PollingInterval
    ED         : A pointer of OHCI_ED_SW

  Returns:

    None

--*/
{
    UINT8 InterruptEdIndex;

    if (PollingInterval == 1) {
        InterruptEdIndex = 0;
    } else if (PollingInterval >= 32) {
        InterruptEdIndex = 32;
    } else {
        InterruptEdIndex = (UINT8)OhciConvertPollRate(PollingInterval);
    }

    Ed->EdHw.NextED = Ohc->HccaIntEDList[InterruptEdIndex].EdHw.NextED;
    Ed->NextEDSoft = Ohc->HccaIntEDList[InterruptEdIndex].EdHw.NextED << 4;
    Ed->PreEDSoft = (UINT32)(UINTN)&Ohc->HccaIntEDList[InterruptEdIndex];
    Ohc->HccaIntEDList[InterruptEdIndex].EdHw.NextED = ((UINT32)(UINTN)Ed) >> 4;

}

VOID
OhciFreeAllAsyncReq (
    IN USB_OHCI_DEV         *Ohc
)
/*++

Routine Description:

  Release all the asynchronous transfers on the lsit.

Arguments:

  Ohc   - The OHCI device

Returns:

  VOID

--*/
{
    LIST_ENTRY              *Head;
    OHCI_ASYNC_REQUEST      *AsyncReq;

    Head = &(Ohc->AsyncIntList);

    if (IsListEmpty (Head)) {
        return;
    }

    while (!IsListEmpty (Head)) {
        AsyncReq  = OHCI_ASYNC_INT_FROM_LINK (Head->ForwardLink);
        OhciUnlinkAsyncReq (Ohc, AsyncReq);
    }
}

EFI_STATUS
OhciCreateAsyncReq (
    IN USB_OHCI_DEV                     *Ohc,
    IN OHCI_ED_SW                       *Ed,
    IN OHCI_TD_SW                       *FirstTd,
    IN UINT8                            DevAddr,
    IN UINT8                            EndPoint,
    IN UINTN                            DataLen,
    IN UINTN                            Interval,
    IN UINT8                            *Data,
    IN EFI_ASYNC_USB_TRANSFER_CALLBACK  Callback,
    IN VOID                             *Context,
    IN BOOLEAN                          IsIntOnly
)
/*++

Routine Description:

  Create Async Request node, and Link to List

Arguments:

  Ohc         - The OHCI device
  Ed          - The endpoint descriptor of the transfer
  FirstTd     - First TD of the transfer
  DevAddr     - Device Address
  EndPoint    - EndPoint Address
  DataLen     - Data length
  Interval    - Polling Interval when inserted to frame list
  Mapping     - Mapping value
  Data        - Data buffer, unmapped
  Callback    - Callback after interrupt transfeer
  Context     - Callback Context passed as function parameter

Returns:

  EFI_SUCCESS            - An asynchronous transfer is created
  EFI_INVALID_PARAMETER  - Paremeter is error
  EFI_OUT_OF_RESOURCES   - Failed because of resource shortage.

--*/
{
    OHCI_ASYNC_REQUEST      *AsyncReq;

    AsyncReq = AllocateZeroPool(sizeof (OHCI_ASYNC_REQUEST));
    if (AsyncReq == NULL)
        return EFI_OUT_OF_RESOURCES;

    //
    // Fill Request field. Data is allocated host memory, not mapped
    //
    AsyncReq->Signature   = OHCI_ASYNC_INT_SIGNATURE;
    AsyncReq->DevAddr     = DevAddr;
    AsyncReq->EndPoint    = EndPoint;
    AsyncReq->DataLen     = DataLen;
    AsyncReq->Interval    = Interval;
    AsyncReq->Data        = Data;
    AsyncReq->Callback    = Callback;
    AsyncReq->Context     = Context;
    AsyncReq->EdSw        = Ed;
    AsyncReq->FirstTd     = FirstTd;

    if (IsIntOnly) {
        AsyncReq->RequestType =1;
    } else {
        AsyncReq->RequestType =0;
    }

    //
    // Insert the new interrupt transfer to the head of the list.
    // The interrupt transfer's monitor function scans the whole
    // list from head to tail. The new interrupt transfer MUST be
    // added to the head of the list.
    //
    InsertHeadList (&(Ohc->AsyncIntList), &(AsyncReq->Link));

    return EFI_SUCCESS;
}

VOID
OhciFreeAsyncReq (
    IN USB_OHCI_DEV         *Ohc,
    IN OHCI_ASYNC_REQUEST   *AsyncReq
)
/*++
Routine Description:

  Free an asynchronous request's resource such as memory

Arguments:

  Ohc         - The OHCI device
  AsyncReq    - The asynchronous request to free

Returns:

  None

--*/

{
    ASSERT ((Ohc != NULL) && (AsyncReq != NULL));
    OhciDestoryTds (Ohc, AsyncReq->FirstTd);
    UsbFreePool (AsyncReq->EdSw, sizeof (OHCI_ED_SW));

    if (AsyncReq->Data != NULL) {
        UsbFreePool (AsyncReq->Data, AsyncReq->DataLen);
    }

    FreePool (AsyncReq);
}

VOID
OhciUnlinkAsyncReq (
    IN USB_OHCI_DEV         *Ohc,
    IN OHCI_ASYNC_REQUEST   *AsyncReq
)
/*++
Routine Description:

  Unlink an asynchronous request's from OHC's asynchronus list.
  also remove the endpoint descriptor.

Arguments:

  Ohc         - The OHCI device
  AsyncReq    - The asynchronous request to free

Returns:

  None

--*/

{
    ASSERT ((Ohc != NULL) && (AsyncReq != NULL));

    RemoveEntryList (&(AsyncReq->Link));
    OhciUnlinkEdFromEd ((OHCI_ED_SW*)AsyncReq->EdSw->PreEDSoft, AsyncReq->EdSw);
    OhciFreeAsyncReq (Ohc, AsyncReq);
}

EFI_STATUS
OhciRemoveAsyncReq (
    IN  USB_OHCI_DEV                    *Ohc,
    IN  UINT8                           DevAddr,
    IN  UINT8                           EndPoint,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK CallBackFunction,
    IN  VOID                            *Context,
    OUT UINT8                           *Toggle
)
/*++
Routine Description:

  Delete Async Interrupt ED and TDs

Arguments:

  Ohc         - The OHCI device
  DevAddr     - Device Address
  EndPoint    - EndPoint Address
  Toggle      - The next data toggle to use

Returns:

  EFI_SUCCESS            - The request is deleted
  EFI_INVALID_PARAMETER  - Paremeter is error
  EFI_NOT_FOUND          - The asynchronous isn't found

--*/
{
    EFI_STATUS          Status;
    OHCI_ASYNC_REQUEST  *AsyncReq;
    OHCI_ED_RESULT      EdResult;
    LIST_ENTRY          *Link;
    BOOLEAN             Found;

    Status = EFI_SUCCESS;

    //
    // If no asynchronous interrupt transaction exists
    //
    if (IsListEmpty (&(Ohc->AsyncIntList))) {
        return EFI_SUCCESS;
    }

    //
    // Find the asynchronous transfer to this device/endpoint pair
    //
    Found = FALSE;
    Link  = Ohc->AsyncIntList.ForwardLink;

    do {
        AsyncReq  = OHCI_ASYNC_INT_FROM_LINK (Link);
        Link      = Link->ForwardLink;
        if ((AsyncReq->DevAddr == DevAddr) && (AsyncReq->EndPoint == EndPoint)) {
            if ((DevAddr == 0) && (EndPoint == 0 )) {
                //
                // This is a async interrupt timing request, we need to continue
                // identifying them by callback function and context.
                //
                if ((AsyncReq->Callback == CallBackFunction) && (AsyncReq->Context == Context)) {
                    Found = TRUE;
                    break;
                }
            } else {
                Found = TRUE;
                break;
            }
        }

    } while (Link != &(Ohc->AsyncIntList));

    if (!Found) {
        return EFI_NOT_FOUND;
    }

    //
    // Check the result of the async transfer then update it
    // to get the next data toggle to use.
    //
    OhciCheckTdStatus (Ohc, AsyncReq->EdSw, AsyncReq->FirstTd, &EdResult);
    *Toggle = EdResult.NextToggle;

    //
    // Release the request now
    //
    OhciUnlinkAsyncReq (Ohc, AsyncReq);
    return Status;
}

VOID
OhciUpdateAsyncReq (
    IN OHCI_ASYNC_REQUEST  *AsyncReq,
    IN UINT32              Result,
    IN UINT32              NextToggle
)
/*++

Routine Description:

  Update Async Request, ED and TDs

Arguments:

  AsyncReq  - The OHCI asynchronous transfer to update
  Result    - Transfer reslut
  ErrTdPos  - Error TD Position

Returns:

  VOID

--*/
{
    OHCI_ED_SW              *Ed;
    OHCI_TD_SW              *FirstTd;
    OHCI_TD_SW              *Td;

    Ed          = AsyncReq->EdSw;
    FirstTd     = AsyncReq->FirstTd;

    Ed->EdHw.sKip = 1;
    if (Result == EFI_USB_NOERROR) {
        //
        // The last transfer succeeds. Then we need to update
        // the Ed and Td for next round of transfer.
        // 1. Update the TD's data toggle
        // 2. Activate all the TDs
        // 3. Link the TD to the endpoint descriptor again since during
        //    execution.
        //

        for (Td = FirstTd; Td != NULL; Td = (OHCI_TD_SW*)Td->NextTDSoft) {
            Td->TdHw.NextTD = (UINT32)Td->NextTDSoft >> 4;
            if (NextToggle) {
                Td->TdHw.DataToggle = 3;
            } else {
                Td->TdHw.DataToggle = 0;
            }
            NextToggle ^= 1;
            Td->TdHw.CurrentBufferPointer = (UINT32)(UINTN)Td->Data;
        }

        if (Ed->EdHw.FunctionAdress == 0) {
            Td = FirstTd;
            Td->TdHw.ConditionCode = USBTD_NOTACCESSED;
            Td->TdHw.ErrorCount    = 2;
        }

        if (AsyncReq->RequestType) {
            if (Ed->EdHw.Halted) {
                Ed->EdHw.Halted=0;
            }
        }

        OhciLinkTdToEd (Ed, FirstTd);
        return;
    }
}

VOID
EFIAPI
OhciProcessAsyncReqList (
    IN USB_OHCI_DEV          *Ohc
)
/*++

  Routine Description:

    Interrupt transfer periodic check handler

  Arguments:

    ohc   : The OHCI device

  Returns:

    VOID

--*/
{
    OHCI_ASYNC_REQUEST      *AsyncReq;
    LIST_ENTRY              *Link;
    VOID                    *Data;
    BOOLEAN                 Finished;
    OHCI_ED_RESULT          EdResult;

    if (IsListEmpty (&(Ohc->AsyncIntList))) {
        return ;
    }

    //
    // This loop must be delete safe
    //
    Link = Ohc->AsyncIntList.ForwardLink;

    do {
        AsyncReq  = OHCI_ASYNC_INT_FROM_LINK (Link);
        Link      = Link->ForwardLink;

        Finished = OhciCheckTdStatus (Ohc, AsyncReq->EdSw, AsyncReq->FirstTd, &EdResult);
        if (!Finished) {
            continue;
        }

        //
        // Copy the data to temporary buffer if there are some
        // data transferred. We may have zero-length packet
        //
        Data = NULL;

        if (EdResult.Complete != 0) {
            Data = AllocateCopyPool (
                       EdResult.Complete,
                       AsyncReq->FirstTd->Data);
            if (Data == NULL) {
                OhciUpdateAsyncReq (AsyncReq, EdResult.Result, EdResult.NextToggle);
                continue;
            }
        }

        OhciUpdateAsyncReq (AsyncReq, EdResult.Result, EdResult.NextToggle);

        //
        // Now, either transfer is SUCCESS or met errors since
        // we have skipped to next transfer earlier if current
        // transfer is still active.
        //
        if (EdResult.Result == EFI_USB_NOERROR) {
            AsyncReq->Callback (Data, EdResult.Complete, AsyncReq->Context, EdResult.Result);
        } else {
            //
            // Leave error recovery to its related device driver.
            // A common case of the error recovery is to re-submit
            // the interrupt transfer. When an interrupt transfer
            // is re-submitted, its position in the linked list is
            // changed. It is inserted to the head of the linked
            // list, while this function scans the whole list from
            // head to tail. Thus, the re-submitted interrupt transfer's
            // callback function will not be called again in this round.
            //
            AsyncReq->Callback (NULL, 0, AsyncReq->Context, EdResult.Result);
        }
        if (Data != NULL) {
            FreePool(Data);
        }
    } while (Link != &(Ohc->AsyncIntList));
}

/********************************************************************************
   Regsiter function
********************************************************************************/
UINT32
OhciReadOpReg (
    IN USB_OHCI_DEV              *Ohc,
    IN UINT32                    Offset
)
/*++

Routine Description:

  Read a OHCI Operational register

Arguments:

  Ohc      - The OHCI device
  Offset   - Register offset

Returns:

  Content of register

--*/
{
    return SmmMmioRead32 (Ohc->UsbMemBase + Offset);
}

VOID
OhciWriteOpReg (
    IN USB_OHCI_DEV            *Ohc,
    IN UINT32                  Offset,
    IN UINT32                  Data
)
/*++

Routine Description:

  Write data to OHCI register

Arguments:

  Ohc      - The OHCI device
  Offset   - Register offset to USB_BAR_INDEX
  Data     - Data to write

Returns:

  VOID

--*/
{
    SmmMmioWrite32 (Ohc->UsbMemBase + Offset, Data);
}

VOID
OhciSetOpRegBit (
    IN  USB_OHCI_DEV           *Ohc,
    IN UINT32                  Offset,
    IN UINT32                  Bit
)
/*++

Routine Description:

  Set a bit of the OHCI Register

Arguments:

  Ohc        - The OHCI device
  Offset     - Register offset to USB_BAR_INDEX
  Bit        - The bit to set

Returns:

  None

--*/
{
    UINT32  Data;

    Data = OhciReadOpReg (Ohc, Offset);
    Data |= Bit;
    OhciWriteOpReg (Ohc, Offset, Data);
}

VOID
OhciClearOpRegBit (
    IN  USB_OHCI_DEV           *Ohc,
    IN UINT32                  Offset,
    IN UINT32                  Bit
)
/*++

Routine Description:

  Clear a bit of the OHCI Register

Arguments:

  Ohc        - The OHCI device
  Offset     - Register offset to USB_BAR_INDEX
  Bit        - The bit to clear
Returns:

  None

--*/
{
    UINT32  Data;

    Data = OhciReadOpReg (Ohc, Offset);
    Data &= ~Bit;
    OhciWriteOpReg (Ohc, Offset, Data);
}

VOID
OhciAckAllInterrupt (
    IN  USB_OHCI_DEV          *Ohc
)
/*++

Routine Description:

  Clear all the interrutp status bits, these bits
  are Write-Clean

Arguments:

  Ohc - The OHCI device

Returns:

  None

--*/
{
    if (!OhciIsHcWorking (Ohc)) {
        Ohc->UsbHc.SetState (&Ohc->UsbHc, EfiUsbHcStateOperational);
    }
}

EFI_STATUS
OhciEnableDevice (
    IN  USB_OHCI_DEV          *Ohc
)
/*++

Routine Description:

  Enable device by setting IO, memory and bus master bit
  in PCI command register

Arguments:

  Ohc        - The OHCI device

Returns:

   EFI_SUCCESS - The devcie is enabled
   EFI_TIMEOUT - Failed to enable the device

--*/
{
    UINT64              PciAddress;
    UINT16              Command;
    EFI_STATUS          Status;

    //
    // PciAddress of command register
    //
    PciAddress = SMM_PCI_ADDRESS (
                     Ohc->PciAddress.Bus,
                     Ohc->PciAddress.Device,
                     Ohc->PciAddress.Function,
                     PCI_COMMAND_OFFSET
                 );
    Status = SmmPciCfgRead (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Command
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    Command |= (EFI_PCI_COMMAND_IO_SPACE |
                EFI_PCI_COMMAND_MEMORY_SPACE |
                EFI_PCI_COMMAND_BUS_MASTER);

    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Command);
    return Status;
}

EFI_STATUS
OhciDisableDevice (
    IN  USB_OHCI_DEV          *Ohc
)
/*++

Routine Description:

  Disable device by clearing IO, memory and bus master bit
  in PCI command register

Arguments:

  Ohc        - The OHCI device

Returns:

   EFI_SUCCESS - The devcie is disabled
   EFI_TIMEOUT - Failed to disable the device

--*/
{
    UINT64              PciAddress;
    UINT16              Command;
    EFI_STATUS          Status;

    //
    // PciAddress of command register
    //
    PciAddress = SMM_PCI_ADDRESS (
                     Ohc->PciAddress.Bus,
                     Ohc->PciAddress.Device,
                     Ohc->PciAddress.Function,
                     PCI_COMMAND_OFFSET
                 );
    Status = SmmPciCfgRead (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Command
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    Command &= ~(EFI_PCI_COMMAND_IO_SPACE |
                 EFI_PCI_COMMAND_MEMORY_SPACE |
                 EFI_PCI_COMMAND_BUS_MASTER);

    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Command);

    return Status;
}

BOOLEAN
OhciIsHcWorking (
    IN USB_OHCI_DEV         *Ohc
)
/*++

Routine Description:

  Check whether the host controller operates well

Arguments:

  Ohc     - The U=OHCI device

Returns:

   TRUE  -  Host controller is working
   FALSE -  Host controller is halted or system error

--*/
{
    UINT32                UsbSts;

    UsbSts = OhciReadOpReg (Ohc, OHC_HCCONTROL_OFFSET);

    if ((UsbSts & OHC_HCCONTROL_HCFS) != OHC_HCCONTROL_HCFS_OPERATIONAL) {
        return FALSE;
    }

    return TRUE;
}

/********************************************************************************
// Scheudle function
********************************************************************************/
VOID
InitEDTree(
    IN OHCI_ED_SW      *IntEDTree
)
/*++

Routine Description:

  Initialize the OHCI Interrupt ED Tree.


Arguments:

  IntEDTree - The OHCI_ED Tree.

Returns:

  None.

--*/
{
    UINTN        Index;
    OHCI_ED_SW   *CurrentEDAddr;

    CurrentEDAddr = IntEDTree;
    for (Index  = 0; Index < 31; Index++) {
        IntEDTree++;
        IntEDTree->EdHw.NextED = ((UINT32)(UINTN)CurrentEDAddr) >> 4;
        IntEDTree->EdHw.sKip = 1;
        IntEDTree++;
        IntEDTree->EdHw.NextED = ((UINT32)(UINTN)CurrentEDAddr) >> 4;
        IntEDTree->EdHw.sKip = 1;
        CurrentEDAddr++;
    }
}

VOID
InitInterruptEDsPointers(
    IN USB_OHCI_DEV   *Ohc,
    IN HCCA_AREA      *HccaArea
)
/*++

Routine Description:

  Initialize the OHCI HCCA Int table.


Arguments:

  HccaArea - The HCCA_AREA struct.

Returns:

  None.

--*/
{
    UINTN       Index;
    OHCI_ED_SW  *pED;
    UINT8       IntListTable[32] = {
        0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0C, 0x1C,
        0x02, 0x12, 0x0A, 0x1A, 0x06, 0x16, 0x0E, 0x1E,
        0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0D, 0x1D,
        0x03, 0x13, 0x0B, 0x1B, 0x07, 0x17, 0x0F, 0x1F
    };

    pED = Ohc->HccaIntEDList + 31;
    for (Index = 0; Index < 32; Index++) {
        HccaArea->HccaInterruptEDsPointers[IntListTable[Index]] = (UINT32)(UINTN)pED;
        pED++;
    }
}

EFI_STATUS
OhcInitHc (
    IN USB_OHCI_DEV        *Ohc
)
/*++

Routine Description:

  Initialize the HC hardware.

Arguments:

  Ohc - The OHCI device

Returns:

  EFI_SUCCESS : The EHCI has come out of halt state
  EFI_TIMEOUT : Time out happened

--*/
{
    UINTN                 Pages;
    UINTN                 Bytes;
    UINT32                Data;

    //
    // Allocate the OHCI HCCA structure
    //

    Bytes = EFI_PAGE_SIZE;
    Pages = EFI_SIZE_TO_PAGES (Bytes);
    if (!Ohc->HccaBase) {
        Ohc->HccaBase = UsbAllocatePage (Pages);
        if (Ohc->HccaBase == NULL) {
            return EFI_OUT_OF_RESOURCES;
        }
    }

    Ohc->HccaIntEDList  = (OHCI_ED_SW*)((UINTN)Ohc->HccaBase + sizeof(HCCA_AREA));
    Ohc->HccaCtrlEDHead = (OHCI_ED_SW*)((UINTN)Ohc->HccaBase + sizeof(HCCA_AREA) + sizeof(OHCI_ED_SW) * NumOfIntEDs);
    Ohc->HccaBulkEDHead = (OHCI_ED_SW*)((UINTN)Ohc->HccaBase + sizeof(HCCA_AREA) + sizeof(OHCI_ED_SW) * NumOfIntEDs + sizeof(OHCI_ED_SW));

    Ohc->HccaIntEDList->EdHw.sKip = 1;
    Ohc->HccaCtrlEDHead->EdHw.sKip = 1;
    Ohc->HccaBulkEDHead->EdHw.sKip = 1;

    InitEDTree(Ohc->HccaIntEDList);
    InitInterruptEDsPointers(Ohc, (HCCA_AREA*)Ohc->HccaBase);

    //
    // Set the HCCA base address
    //
    OhciWriteOpReg (Ohc, OHC_HCHCCA_OFFSET, (UINT32)(UINTN)Ohc->HccaBase);

    //
    // Set the control head ED register
    //
    OhciWriteOpReg(Ohc, OHC_HCCONTROLHEADED_OFFSET, (UINT32)(UINTN)Ohc->HccaCtrlEDHead);

    //
    // Set the bulk head ED register
    //
    OhciWriteOpReg(Ohc, OHC_HCBULKHEADED_OFFSET, (UINT32)(UINTN)Ohc->HccaBulkEDHead);

    //
    // Set the control bulk service ratio
    //
    OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET,  BIT(1) | BIT(0));

    //
    // Set the interrupt routing to SMI
    //
    OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_IR);

    //
    // Set the interrupt enable register
    //
    Data = (UINT32) (OHC_HCINTERRUPTENABLE_WDH | OHC_HCINTERRUPTENABLE_RD | OHC_HCINTERRUPTENABLE_UE |
                     OHC_HCINTERRUPTENABLE_RHSC |OHC_HCINTERRUPTENABLE_OC | OHC_HCINTERRUPTENABLE_MIE);
    OhciWriteOpReg(Ohc, OHC_HCINTERRUPTENABLE_OFFSET, Data);

    //
    // Enable Period , bulk and Control list
    //
    OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_PLE | OHC_HCCONTROL_CLE | OHC_HCCONTROL_BLE);

    //
    // Set the functional state to operational
    //
    OhciClearOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS);
    OhciSetOpRegBit(Ohc, OHC_HCCONTROL_OFFSET, OHC_HCCONTROL_HCFS_OPERATIONAL);

    //
    // Set the Period start
    //
    OhciSetOpRegBit(Ohc, OHC_HCPERIODICSTART_OFFSET, OHC_HCPERIODICSTART_PS);

    //
    // Set the Fm Interval
    //
    OhciSetOpRegBit(Ohc, OHC_HCFMINTERVAL_OFFSET, (UINT32)(OHC_HCFMINTERVAL_FIT | OHC_HCFMINTERVAL_FSMPS));

    //
    // Set RhDescriptorA
    //
    OhciClearOpRegBit(Ohc, OHC_HCRHDESCRIPTORA_OFFSET, OHC_HCRHDESCRIPTORA_PSM);
    OhciSetOpRegBit(Ohc, OHC_HCRHDESCRIPTORA_OFFSET, OHC_HCRHDESCRIPTORA_NPS);

    //
    // Set HcRhStatus
    //
    OhciSetOpRegBit(Ohc, OHC_HCRHSTATUS_OFFSET, OHC_HCRHSTATUS_LPSC);

    return EFI_SUCCESS;
}


VOID
OhciDumpEd (
    IN OHCI_ED_SW    *EdSw
)
/*++

Routine Description:

  Dump the content of ED structure

Arguments:

  EdSw  - Pointer to software ED structure

Returns:

  None

--*/
{
    OHCI_DEBUG (( EFI_D_INFO, "&EdSw @ 0x%x\n", EdSw));
    OHCI_DEBUG (( EFI_D_INFO, "EdSw.NextEd    - 0x%x\n", EdSw->NextEDSoft));
    OHCI_DEBUG (( EFI_D_INFO, "EdSw.PreEd     - 0x%x\n", EdSw->PreEDSoft));
    OHCI_DEBUG (( EFI_D_INFO, "EdSw.EdHw:\n"));
    OHCI_DEBUG (( EFI_D_INFO, " FunctionAdress      - %x\n",   EdSw->EdHw.FunctionAdress));
    OHCI_DEBUG (( EFI_D_INFO, " EndpointNumber      - %x\n",   EdSw->EdHw.EndpointNumber));;
    OHCI_DEBUG (( EFI_D_INFO, " Direction           - %x\n",   EdSw->EdHw.Direction));
    OHCI_DEBUG (( EFI_D_INFO, " Speed               - %x\n",   EdSw->EdHw.Speed));
    OHCI_DEBUG (( EFI_D_INFO, " sKip                - %x\n",   EdSw->EdHw.sKip));
    OHCI_DEBUG (( EFI_D_INFO, " Format              - %x\n",   EdSw->EdHw.Format));
    OHCI_DEBUG (( EFI_D_INFO, " MaximumPacketSize   - %x\n",   EdSw->EdHw.MaximumPacketSize));
    OHCI_DEBUG (( EFI_D_INFO, " Halted              - %x\n",   EdSw->EdHw.Halted));
    OHCI_DEBUG (( EFI_D_INFO, " ToggleCarry         - %x\n",   EdSw->EdHw.ToggleCarry));
    OHCI_DEBUG (( EFI_D_INFO, " NextED              - %x\n",   EdSw->EdHw.NextED << 4));
    OHCI_DEBUG (( EFI_D_INFO, " TDs Head Pointer - %x\n",   EdSw->EdHw.TDQueueHeadPointer << 4));
    OHCI_DEBUG (( EFI_D_INFO, " TDs Tail Pointor - %x\n\n", EdSw->EdHw.TDQueueTailPointer << 4));
}

VOID
OhciDumpTds (
    IN OHCI_TD_SW           *TdSw
)
/*++

Routine Description:

  Dump the content of TD structure.

Arguments:

  TdSw  - Pointer to software TD structure
  IsCur - Whether dump the whole list, or only dump the current TD

Returns:

  None

--*/
{
    OHCI_TD_SW              *CurTdSw;

    CurTdSw = TdSw;
    while (CurTdSw != NULL) {
        OHCI_DEBUG (( EFI_D_INFO, "TdSw @ 0x%x\n",           CurTdSw));
        OHCI_DEBUG (( EFI_D_INFO, "TdSw.NextTd            - 0x%x\n",  CurTdSw->NextTDSoft));
        OHCI_DEBUG (( EFI_D_INFO, "TdSw.DataLen           - %d\n",    CurTdSw->BufferLength));
        OHCI_DEBUG (( EFI_D_INFO, "TdSw.Data              - 0x%x\n",  CurTdSw->Data));
        OHCI_DEBUG (( EFI_D_INFO, "TdHw:\n"));
        OHCI_DEBUG (( EFI_D_INFO, " NextLink              - 0x%x\n",  CurTdSw->TdHw.NextTD << 4));
        OHCI_DEBUG (( EFI_D_INFO, " BufferRounding        - %d\n",    CurTdSw->TdHw.BufferRounding));
        OHCI_DEBUG (( EFI_D_INFO, " DirectionPID          - 0x%x\n",  CurTdSw->TdHw.DirectionPID));
        OHCI_DEBUG (( EFI_D_INFO, " DelayInterrupt        - 0x%x\n",  CurTdSw->TdHw.DelayInterrupt));
        OHCI_DEBUG (( EFI_D_INFO, " DataToggle            - 0x%x\n",  CurTdSw->TdHw.DataToggle));
        OHCI_DEBUG (( EFI_D_INFO, " ErrorCount            - %d\n",    CurTdSw->TdHw.ErrorCount));
        OHCI_DEBUG (( EFI_D_INFO, " ConditionCode         - 0x%x\n",  CurTdSw->TdHw.ConditionCode));
        OHCI_DEBUG (( EFI_D_INFO, " CurrentBufferPointer  - 0x%x\n",  CurTdSw->TdHw.CurrentBufferPointer));
        OHCI_DEBUG (( EFI_D_INFO, " BufferEnd             - 0x%x\n",  CurTdSw->TdHw.BufferEnd));
        CurTdSw = (OHCI_TD_SW*)CurTdSw->NextTDSoft;
    }
}

BOOLEAN
IsLegacySupport (
  EFI_USB2_HC_PROTOCOL *This
) {
  USB_OHCI_DEV        *OhciDev;
  UINT32              HcRevision;

  OhciDev       = OHC_FROM_USB2_HC_PROTO (This);
  HcRevision    = OhciReadOpReg (OhciDev, OHC_HCREVISION_OFFSET);
  if ((HcRevision & BIT(8)) != BIT(8)) {
    return FALSE;
  }

  return TRUE;
}

