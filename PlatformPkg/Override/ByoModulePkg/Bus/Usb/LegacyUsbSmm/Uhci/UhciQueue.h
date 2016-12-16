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
  UchiQueue.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_UHCI_QUEUE_H_
#define _EFI_UHCI_QUEUE_H_

// Macroes used to set various links in UHCI's driver.
// In this UHCI driver, QH's horizontal link always pointers to other QH,
// and its vertical link always pointers to TD. TD's next pointer always
// pointers to other sibling TD. Frame link always pointers to QH because
// ISO transfer isn't supported.
//
// We should use UINT32 to access these pointers to void race conditions
// with hardware.
//
#define QH_HLINK(Pointer, Terminate)  \
        (((UINT32) ((UINTN) (Pointer)) & 0xFFFFFFF0) | 0x02 | ((Terminate) ? 0x01 : 0))

#define QH_VLINK(Pointer, Terminate)  \
        (((UINT32) ((UINTN) (Pointer)) & 0xFFFFFFF0) | ((Terminate) ? 0x01 : 0))

#define TD_LINK(Pointer, VertFirst, Terminate) \
        (((UINT32) ((UINTN) (Pointer)) & 0xFFFFFFF0) | \
         ((VertFirst) ? 0x04 : 0) | ((Terminate) ? 0x01 : 0))

#define LINK_TERMINATED(Link) (((Link) & 0x01) != 0)

#define UHCI_ADDR(QhOrTd)     ((VOID *) (UINTN) ((QhOrTd) & 0xFFFFFFF0))

#pragma pack(1)
//
// Both links in QH has this internal structure:
//   Next pointer: 28, Reserved: 2, NextIsQh: 1, Terminate: 1
// This is the same as frame list entry.
//
typedef struct {
    UINT32              HorizonLink;
    UINT32              VerticalLink;
} UHCI_QH_HW;

//
// Next link in TD has this internal structure:
//   Next pointer: 28, Reserved: 1, Vertical First: 1, NextIsQh: 1, Terminate: 1
//
typedef struct {
    UINT32              NextLink;
    UINT32              ActualLen   : 11;
    UINT32              Reserved1   : 5;
    UINT32              Status      : 8;
    UINT32              IntOnCpl    : 1;
    UINT32              IsIsoch     : 1;
    UINT32              LowSpeed    : 1;
    UINT32              ErrorCount  : 2;
    UINT32              ShortPacket : 1;
    UINT32              Reserved2   : 2;
    UINT32              PidCode     : 8;
    UINT32              DeviceAddr  : 7;
    UINT32              EndPoint    : 4;
    UINT32              DataToggle  : 1;
    UINT32              Reserved3   : 1;
    UINT32              MaxPacketLen: 11;
    UINT32              DataBuffer;
} UHCI_TD_HW;
#pragma pack()

typedef struct _UHCI_TD_SW     UHCI_TD_SW;
typedef struct _UHCI_QH_SW     UHCI_QH_SW;


typedef struct _UHCI_QH_SW {
    UHCI_QH_HW        QhHw;
    UHCI_QH_SW        *NextQh;
    UHCI_TD_SW        *TDs;
    UINTN             Interval;
} UHCI_QH_SW;

typedef struct _UHCI_TD_SW {
    UHCI_TD_HW        TdHw;
    UHCI_TD_SW        *NextTd;
    UINT8             *Data;
    UINT16            DataLen;
} UHCI_TD_SW;

VOID
UhciLinkTdToQh (
    IN UHCI_QH_SW           *Qh,
    IN UHCI_TD_SW           *Td
)
/*++

Routine Description:

  Link the TD To QH

Arguments:

  Qh  - The queue head for the TD to link to
  Td  - The TD to link

Returns:

  VOID

--*/
;

VOID
UhciUnlinkTdFromQh (
    IN UHCI_QH_SW           *Qh,
    IN UHCI_TD_SW           *Td
)
/*++

Routine Description:

  Unlink TD from the QH

Arguments:

  Qh  - The queue head to unlink from
  Td  - The TD to unlink

Returns:

  VOID

--*/
;

EFI_STATUS
UhciMapUserData (
    IN  USB_HC_DEV              *Uhc,
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

  Uhc        - The UHCI device
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
;

VOID
UhciDestoryTds (
    IN USB_HC_DEV           *Uhc,
    IN UHCI_TD_SW           *FirstTd
)
/*++
Routine Description:

  Delete a list of TDs

Arguments:

  Uhc       - The UHCI device
  FirstTd   - TD link list head

Returns:

  VOID

--*/
;

UHCI_QH_SW *
UhciCreateQh (
    IN  USB_HC_DEV        *Uhc,
    IN  UINTN             Interval
)
/*++

Routine Description:

  Create an initialize a new queue head

Arguments:

  Uhc       - The UHCI device
  Interval  - The polling interval for the queue

Returns:

  The newly created queue header

--*/
;

UHCI_TD_SW *
UhciCreateCtrlTds (
    IN USB_HC_DEV           *Uhc,
    IN UINT8                DeviceAddr,
    IN UINT8                DataPktId,
    IN UINT8                *Request,
    IN UINT8                *Data,
    IN UINTN                DataLen,
    IN UINT8                MaxPacket,
    IN BOOLEAN              IsLow
)
/*++

Routine Description:

  Create Tds list for Control Transfer

Arguments:

  Uhc         - The UHCI device
  DeviceAddr  - The device address
  DataPktId   - Packet Identification of Data Tds
  Request     - A pointer to request structure buffer to transfer
  Data        - A pointer to user data buffer to transfer
  DataLen     - Length of user data to transfer
  MaxPacket   - Maximum packet size for control transfer
  IsLow       - Full speed or low speed

Returns:

  The Td list head for the control transfer

--*/
;

UHCI_TD_SW *
UhciCreateBulkOrIntTds (
    IN USB_HC_DEV           *Uhc,
    IN UINT8                DevAddr,
    IN UINT8                EndPoint,
    IN UINT8                PktId,
    IN UINT8                *Data,
    IN UINTN                DataLen,
    IN OUT UINT8            *DataToggle,
    IN UINT8                MaxPacket,
    IN BOOLEAN              IsLow,
    IN BOOLEAN              IsIOCSet
)
/*++

Routine Description:

  Create Tds list for Bulk/Interrupt Transfer

Arguments:

  Uhc          - USB_HC_DEV
  DevAddr      - Address of Device
  EndPoint     - Endpoint Number
  PktId        - Packet Identification of Data Tds
  Data         - A pointer to user data buffer to transfer
  DataLen      - Length of user data to transfer
  DataToggle   - Data Toggle Pointer
  MaxPacket    - Maximum packet size for Bulk/Interrupt transfer
  IsLow         - Is Low Speed Device
  IsIOCSet      - Is IOC Bit of data Tds to be set

Returns:

  The Tds list head for the bulk transfer

--*/
;

#endif
