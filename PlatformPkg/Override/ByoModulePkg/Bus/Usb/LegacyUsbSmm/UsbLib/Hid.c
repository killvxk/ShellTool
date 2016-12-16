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
  Hid.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "hid.h"
#include "UsbMem.h"

//
// Function to get HID descriptor
//
EFI_STATUS
UsbGetHidDescriptor (
    IN  EFI_USB_IO_PROTOCOL        *UsbIo,
    IN  UINT8                      InterfaceNum,
    OUT EFI_USB_HID_DESCRIPTOR     *HidDescriptor
)
/*++

  Routine Description:
    Get Hid Descriptor

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    InterfaceNum      -   Hid interface number
    HidDescriptor     -   Caller allocated buffer to store Usb hid descriptor
                          if successfully returned.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
    UINT32                  Status;
    EFI_STATUS              Result;
    EFI_USB_DEVICE_REQUEST  *Request;
    UINT8                   *UsbBuf;

    Request = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (Request == NULL)
        return EFI_OUT_OF_RESOURCES;

    UsbBuf = UsbAllocatePool (sizeof(EFI_USB_HID_DESCRIPTOR));
    if (UsbBuf == NULL) {
        UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
        return EFI_OUT_OF_RESOURCES;
    }

    Request->RequestType = 0x81;
    Request->Request     = 0x06;
    Request->Value       = (UINT16) (0x21 << 8);
    Request->Index       = InterfaceNum;
    Request->Length      = sizeof (EFI_USB_HID_DESCRIPTOR);

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 Request,
                 EfiUsbDataIn,
                 TIMEOUT_VALUE,
                 UsbBuf,
                 sizeof (EFI_USB_HID_DESCRIPTOR),
                 &Status
             );
    CopyMem (HidDescriptor, UsbBuf, sizeof(EFI_USB_HID_DESCRIPTOR));
    UsbFreePool (UsbBuf, sizeof(EFI_USB_HID_DESCRIPTOR));
    UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

//
// Function to get Report Class descriptor
//
EFI_STATUS
UsbGetReportDescriptor (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT8                   InterfaceNum,
    IN  UINT16                  DescriptorSize,
    OUT UINT8                   *DescriptorBuffer
)
/*++

  Routine Description:
    get Report Class descriptor

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL.
    InterfaceNum      -   Report interface number.
    DescriptorSize    -   Length of DescriptorBuffer.
    DescriptorBuffer  -   Caller allocated buffer to store Usb report descriptor
                          if successfully returned.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
    UINT32                  Status;
    EFI_STATUS              Result;
    EFI_USB_DEVICE_REQUEST  *Request;
    UINT8                   *UsbBuf;

    Request = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (Request == NULL)
        return EFI_OUT_OF_RESOURCES;

    if ((DescriptorSize != 0) && (DescriptorBuffer != NULL)) {
        UsbBuf = UsbAllocatePool (DescriptorSize);
        if (UsbBuf == NULL) {
            UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
            return EFI_OUT_OF_RESOURCES;
        }
    } else {
        UsbBuf = DescriptorBuffer;
    }

    //
    // Fill Device request packet
    //
    Request->RequestType = 0x81;
    Request->Request     = 0x06;
    Request->Value       = (UINT16) (0x22 << 8);
    Request->Index       = InterfaceNum;
    Request->Length      = DescriptorSize;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 Request,
                 EfiUsbDataIn,
                 TIMEOUT_VALUE,
                 UsbBuf,
                 DescriptorSize,
                 &Status
             );

    if ((DescriptorSize != 0) && (DescriptorBuffer != NULL)) {
        if (!EFI_ERROR(Result))
            CopyMem (DescriptorBuffer, UsbBuf, DescriptorSize);
        UsbFreePool (UsbBuf, DescriptorSize);
    }

    UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

//
// Following are HID class request
//
EFI_STATUS
UsbGetProtocolRequest (
    IN EFI_USB_IO_PROTOCOL     *UsbIo,
    IN UINT8                   Interface,
    IN UINT8                   *Protocol
)
/*++

  Routine Description:
    Get Hid Protocol Request

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to get protocol
    Protocol          -   Protocol value returned.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
    UINT32                  Status;
    EFI_STATUS              Result;
    EFI_USB_DEVICE_REQUEST  *Request;
    UINT8                   *UsbBuf;

    Request = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (Request == NULL)
        return EFI_OUT_OF_RESOURCES;

    UsbBuf = UsbAllocatePool (1);
    if (UsbBuf == NULL) {
        UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
        return EFI_OUT_OF_RESOURCES;
    }
    //
    // Fill Device request packet
    //
    Request->RequestType = 0xa1;
    //
    // 10100001b;
    //
    Request->Request = EFI_USB_GET_PROTOCOL_REQUEST;
    Request->Value   = 0;
    Request->Index   = Interface;
    Request->Length  = 1;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 Request,
                 EfiUsbDataIn,
                 TIMEOUT_VALUE,
                 UsbBuf,
                 sizeof (UINT8),
                 &Status
             );
    *Protocol = *UsbBuf;
    UsbFreePool (UsbBuf, 1);
    UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

EFI_STATUS
UsbSetProtocolRequest (
    IN EFI_USB_IO_PROTOCOL     *UsbIo,
    IN UINT8                   Interface,
    IN UINT8                   Protocol
)
/*++

  Routine Description:
    Set Hid Protocol Request

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to set protocol
    Protocol          -   Protocol value the caller wants to set.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
    UINT32                  Status;
    EFI_STATUS              Result;
    EFI_USB_DEVICE_REQUEST  *Request;

    Request = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (Request == NULL)
        return EFI_OUT_OF_RESOURCES;

    //
    // Fill Device request packet
    //
    Request->RequestType = 0x21;
    //
    // 00100001b;
    //
    Request->Request = EFI_USB_SET_PROTOCOL_REQUEST;
    Request->Value   = Protocol;
    Request->Index   = Interface;
    Request->Length  = 0;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 Request,
                 EfiUsbNoData,
                 TIMEOUT_VALUE,
                 NULL,
                 0,
                 &Status
             );
    UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

EFI_STATUS
UsbSetIdleRequest (
    IN EFI_USB_IO_PROTOCOL     *UsbIo,
    IN UINT8                   Interface,
    IN UINT8                   ReportId,
    IN UINT8                   Duration
)
/*++

  Routine Description:
    Set Idel request.

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to set.
    ReportId          -   Which report the caller wants to set.
    Duration          -   Idle rate the caller wants to set.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
    UINT32                  Status;
    EFI_STATUS              Result;
    EFI_USB_DEVICE_REQUEST  *Request;

    Request = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (Request == NULL)
        return EFI_OUT_OF_RESOURCES;


    //
    // Fill Device request packet
    //
    Request->RequestType = 0x21;
    //
    // 00100001b;
    //
    Request->Request = EFI_USB_SET_IDLE_REQUEST;
    Request->Value   = (UINT16) ((Duration << 8) | ReportId);
    Request->Index   = Interface;
    Request->Length  = 0;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 Request,
                 EfiUsbNoData,
                 TIMEOUT_VALUE,
                 NULL,
                 0,
                 &Status
             );
    UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));

    return Result;
}

EFI_STATUS
UsbGetIdleRequest (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT8                   Interface,
    IN  UINT8                   ReportId,
    OUT UINT8                   *Duration
)
/*++

  Routine Description:
    Get Idel request.

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to get.
    ReportId          -   Which report the caller wants to get.
    Duration          -   Idle rate the caller wants to get.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
    UINT32                  Status;
    EFI_STATUS              Result;
    EFI_USB_DEVICE_REQUEST  *Request;
    UINT8                   *UsbBuf;

    Request = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (Request == NULL)
        return EFI_OUT_OF_RESOURCES;

    UsbBuf = UsbAllocatePool (1);
    if (UsbBuf == NULL) {
        UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
        return EFI_OUT_OF_RESOURCES;
    }

    //
    // Fill Device request packet
    //
    Request->RequestType = 0xa1;
    //
    // 10100001b;
    //
    Request->Request = EFI_USB_GET_IDLE_REQUEST;
    Request->Value   = ReportId;
    Request->Index   = Interface;
    Request->Length  = 1;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 Request,
                 EfiUsbDataIn,
                 TIMEOUT_VALUE,
                 UsbBuf,
                 1,
                 &Status
             );
    *Duration = *UsbBuf;
    UsbFreePool (UsbBuf, 1);
    UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

EFI_STATUS
UsbSetReportRequest (
    IN EFI_USB_IO_PROTOCOL     *UsbIo,
    IN UINT8                   Interface,
    IN UINT8                   ReportId,
    IN UINT8                   ReportType,
    IN UINT16                  ReportLen,
    IN UINT8                   *Report
)
/*++

  Routine Description:
    Hid Set Report request.

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to set.
    ReportId          -   Which report the caller wants to set.
    ReportType        -   Type of report.
    ReportLen         -   Length of report descriptor.
    Report            -   Report Descriptor buffer.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
    UINT32                  Status;
    EFI_STATUS              Result;
    EFI_USB_DEVICE_REQUEST  *Request;
    UINT8                   *UsbBuf;

    Request = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (Request == NULL)
        return EFI_OUT_OF_RESOURCES;

    if ((ReportLen != 0) && (Report != NULL)) {
        UsbBuf = UsbAllocatePool (ReportLen);
        if (UsbBuf == NULL) {
            UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
            return EFI_OUT_OF_RESOURCES;
        }
        CopyMem (UsbBuf, Report, ReportLen);
    }
    else
        UsbBuf = Report;

    //
    // Fill Device request packet
    //
    Request->RequestType = 0x21;
    //
    // 00100001b;
    //
    Request->Request = EFI_USB_SET_REPORT_REQUEST;
    Request->Value   = (UINT16) ((ReportType << 8) | ReportId);
    Request->Index   = Interface;
    Request->Length  = ReportLen;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 Request,
                 EfiUsbDataOut,
                 TIMEOUT_VALUE,
                 UsbBuf,
                 ReportLen,
                 &Status
             );
    if ((ReportLen != 0) && (Report != NULL)) {
        UsbFreePool (UsbBuf, ReportLen);
    }

    UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}

EFI_STATUS
UsbGetReportRequest (
    IN EFI_USB_IO_PROTOCOL     *UsbIo,
    IN UINT8                   Interface,
    IN UINT8                   ReportId,
    IN UINT8                   ReportType,
    IN UINT16                  ReportLen,
    IN UINT8                   *Report
)
/*++

  Routine Description:
    Hid Set Report request.

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to set.
    ReportId          -   Which report the caller wants to set.
    ReportType        -   Type of report.
    ReportLen         -   Length of report descriptor.
    Report            -   Caller allocated buffer to store Report Descriptor.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
    UINT32                  Status;
    EFI_STATUS              Result;
    EFI_USB_DEVICE_REQUEST  *Request;
    UINT8                   *UsbBuf;

    Request = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (Request == NULL)
        return EFI_OUT_OF_RESOURCES;

    if ((ReportLen != 0) && (Report != NULL)) {
        UsbBuf = UsbAllocatePool (ReportLen);
        if (UsbBuf == NULL) {
            UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
            return EFI_OUT_OF_RESOURCES;
        }
    }
    else
        UsbBuf = Report;

    //
    // Fill Device request packet
    //
    Request->RequestType = 0xa1;
    //
    // 10100001b;
    //
    Request->Request = EFI_USB_GET_REPORT_REQUEST;
    Request->Value   = (UINT16) ((ReportType << 8) | ReportId);
    Request->Index   = Interface;
    Request->Length  = ReportLen;

    Result = UsbIo->UsbControlTransfer (
                 UsbIo,
                 Request,
                 EfiUsbDataIn,
                 TIMEOUT_VALUE,
                 UsbBuf,
                 ReportLen,
                 &Status
             );

    if ((ReportLen != 0) && (Report != NULL)) {
        if (!EFI_ERROR(Result))
            CopyMem (Report, UsbBuf, ReportLen);
        UsbFreePool (UsbBuf, ReportLen);
    }

    UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
    return Result;
}
