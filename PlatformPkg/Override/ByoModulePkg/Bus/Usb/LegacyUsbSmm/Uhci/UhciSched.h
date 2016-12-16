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
  UchiSched.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_UHCI_SCHED_H_
#define _EFI_UHCI_SCHED_H_

enum {
    UHCI_ASYNC_INT_SIGNATURE = SIGNATURE_32 ('u', 'h', 'c', 'a'),

    //
    // The failure mask for USB transfer return status. If any of
    // these bit is set, the transfer failed. EFI_USB_ERR_NOEXECUTE
    // and EFI_USB_ERR_NAK are not considered as error condition:
    // the transfer is still going on.
    //
    USB_ERR_FAIL_MASK = EFI_USB_ERR_STALL   | EFI_USB_ERR_BUFFER |
    EFI_USB_ERR_BABBLE  | EFI_USB_ERR_CRC    |
    EFI_USB_ERR_TIMEOUT | EFI_USB_ERR_BITSTUFF |
    EFI_USB_ERR_SYSTEM,

};

//
// Structure to return the result of UHCI QH execution.
// Result is the final result of the QH's QTD. NextToggle
// is the next data toggle to use. Complete is the actual
// length of data transferred.
//
typedef struct {
    UINT32                  Result;
    UINT8                   NextToggle;
    UINTN                   Complete;
} UHCI_QH_RESULT;

typedef struct _UHCI_ASYNC_REQUEST     UHCI_ASYNC_REQUEST;

//
// Structure used to manager the asynchronous interrupt transfers.
//
typedef struct _UHCI_ASYNC_REQUEST {
    UINTN                           Signature;
    LIST_ENTRY                      Link;
    UHCI_ASYNC_REQUEST              *Recycle;

    //
    // Endpoint attributes
    //
    UINT8                           DevAddr;
    UINT8                           EndPoint;
    BOOLEAN                         IsLow;
    UINTN                           Interval;

    //
    // Data and UHC structures
    //
    UHCI_QH_SW                      *QhSw;
    UHCI_TD_SW                      *FirstTd;
    UINT8                           *Data;      // Allocated host memory, not mapped memory
    UINTN                           DataLen;

    //
    // User callback and its context
    //
    EFI_ASYNC_USB_TRANSFER_CALLBACK Callback;
    VOID                            *Context;
} UHCI_ASYNC_REQUEST;

#define UHCI_ASYNC_INT_FROM_LINK(a) \
          CR (a, UHCI_ASYNC_REQUEST, Link, UHCI_ASYNC_INT_SIGNATURE)

EFI_STATUS
UhciInitFrameList (
    IN USB_HC_DEV         *Uhc
)
/*++

Routine Description:

  Create Frame List Structure

Arguments:

  Uhc         - UHCI device

Returns:

  EFI_OUT_OF_RESOURCES - Can't allocate memory resources
  EFI_UNSUPPORTED      - Map memory fail
  EFI_SUCCESS          - Success

--*/
;

VOID
UhciDestoryFrameList (
    IN USB_HC_DEV           *Uhc
)
/*++

Routine Description:

  Destory FrameList buffer

Arguments:

  Uhc - The UHCI device

Returns:

  VOID

--*/
;

UINTN
UhciConvertPollRate (
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
;

VOID
UhciLinkQhToFrameList (
    UINT32                  *FrameBase,
    UHCI_QH_SW              *Qh
)
/*++

Routine Description:

  Link a queue head (for asynchronous interrupt transfer) to
  the frame list.

Arguments:

  FrameBase - The base of the frame list
  Qh        - The queue head to link into

Returns:

  None

--*/
;

VOID
UhciUnlinkQhFromFrameList (
    UINT32                *FrameBase,
    UHCI_QH_SW            *Qh
)
/*++

Routine Description:

  Unlink QH from the frame list is easier: find all
  the precedence node, and pointer there next to QhSw's
  next.

Arguments:

  FrameBase - The base address of the frame list
  Qh        - The queue head to unlink

Returns:

  None

--*/
;

EFI_STATUS
UhciExecuteTransfer (
    IN  USB_HC_DEV          *Uhc,
    IN  UHCI_QH_SW          *Qh,
    IN  UHCI_TD_SW          *Td,
    IN  UINTN               TimeOut,
    IN  BOOLEAN             IsLow,
    OUT UHCI_QH_RESULT      *QhResult
)
/*++

Routine Description:

  Check the result of the transfer

Arguments:

  Uhc            - The UHCI device
  Td             - The first TDs of the transfer
  TimeOut        - TimeOut value in milliseconds
  IsLow          - Is Low Speed Device
  QhResult       - The variable to return result

Returns:

  EFI_SUCCESS      - The transfer finished with success
  EFI_DEVICE_ERROR - Transfer failed

--*/
;

EFI_STATUS
UhciCreateAsyncReq (
    IN USB_HC_DEV                       *Uhc,
    IN UHCI_QH_SW                       *Qh,
    IN UHCI_TD_SW                       *FirstTd,
    IN UINT8                            DevAddr,
    IN UINT8                            EndPoint,
    IN UINTN                            DataLen,
    IN UINTN                            Interval,
    IN UINT8                            *Data,
    IN EFI_ASYNC_USB_TRANSFER_CALLBACK  Callback,
    IN VOID                             *Context,
    IN BOOLEAN                          IsLow
)
/*++

Routine Description:

  Create Async Request node, and Link to List

Arguments:

  Uhc         - The UHCI device
  Qh          - The queue head of the transfer
  FirstTd     - First TD of the transfer
  DevAddr     - Device Address
  EndPoint    - EndPoint Address
  Toggle      - Data Toggle
  DataLen     - Data length
  Interval    - Polling Interval when inserted to frame list
  Data        - Data buffer, unmapped
  Callback    - Callback after interrupt transfeer
  Context     - Callback Context passed as function parameter
  IsLow       - Is Low Speed

Returns:

  EFI_SUCCESS            - An asynchronous transfer is created
  EFI_INVALID_PARAMETER  - Paremeter is error
  EFI_OUT_OF_RESOURCES   - Failed because of resource shortage.

--*/
;

EFI_STATUS
UhciRemoveAsyncReq (
    IN  USB_HC_DEV          *Uhc,
    IN  UINT8               DevAddr,
    IN  UINT8               EndPoint,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK CallBackFunction,
    IN  VOID                *Context,
    OUT UINT8               *Toggle
)
/*++
Routine Description:

  Delete Async Interrupt QH and TDs

Arguments:

  Uhc         - The UHCI device
  DevAddr     - Device Address
  EndPoint    - EndPoint Address
  Toggle      - The next data toggle to use

Returns:

  EFI_SUCCESS            - The request is deleted
  EFI_INVALID_PARAMETER  - Paremeter is error
  EFI_NOT_FOUND         - The asynchronous isn't found

--*/
;

VOID
UhciFreeAllAsyncReq (
    IN USB_HC_DEV           *Uhc
)
/*++

Routine Description:

  Release all the asynchronous transfers on the lsit.

Arguments:

  Uhc   - The UHCI device

Returns:

  VOID

--*/
;

VOID
EFIAPI
UhciMonitorAsyncReqList (
    IN EFI_EVENT            Event,
    IN VOID                 *Context
)
/*++

  Routine Description:

    Interrupt transfer periodic check handler

  Arguments:

    Event   - The event of the time
    Context - Context of the event, pointer to USB_HC_DEV

  Returns:

    VOID

--*/
;

enum {
    UHC_1_MICROSECOND             = 1,
    UHC_1_MILLISECOND             = 1000 * UHC_1_MICROSECOND,
    UHC_1_SECOND                  = 1000 * UHC_1_MILLISECOND,

    //
    // UHCI register operation timeout, set by experience
    //
    UHC_GENERIC_TIMEOUT           = UHC_1_SECOND,

    //
    // Wait for force global resume(FGR) complete, refers to
    // specification[UHCI11-2.1.1]
    //
    UHC_FORCE_GLOBAL_RESUME_STALL = 20 * UHC_1_MILLISECOND,

    //
    // Wait for roothub port reset and recovery, reset stall
    // is set by experience, and recovery stall refers to
    // specification[UHCI11-2.1.1]
    //
    UHC_ROOT_PORT_RESET_STALL     = 50 * UHC_1_MILLISECOND,
    UHC_ROOT_PORT_RECOVERY_STALL  = 10 * UHC_1_MILLISECOND,

    //
    // Sync and Async transfer polling interval, set by experience,
    // and the unit of Async is 100us.
    //
    UHC_SYNC_POLL_INTERVAL        = 50 * UHC_1_MICROSECOND,
    UHC_ASYNC_POLL_INTERVAL       = 50 * 10000UL,

    USB_HC_DEV_SIGNATURE          = SIGNATURE_32 ('u', 'h', 'c', 'i'),
};

#pragma pack(1)
typedef struct {
    UINT8               PI;
    UINT8               SubClassCode;
    UINT8               BaseCode;
} USB_CLASSC;
#pragma pack()

#define UHC_FROM_LINK(This)           CR(This, USB_HC_DEV, Link, USB_HC_DEV_SIGNATURE)
#define UHC_FROM_USB_HC_PROTO(This)   CR(This, USB_HC_DEV, UsbHc, USB_HC_DEV_SIGNATURE)
#define UHC_FROM_USB2_HC_PROTO(This)  CR(This, USB_HC_DEV, Usb2Hc, USB_HC_DEV_SIGNATURE)

//
// USB_HC_DEV support the UHCI hardware controller. It schedules
// the asynchronous interrupt transfer with the same method as
// EHCI: a reversed tree structure. For synchronous interrupt,
// control and bulk transfer, it uses three static queue head to
// schedule them. SyncIntQh is for interrupt transfer. LsCtrlQh is
// for LOW speed control transfer, and FsCtrlBulkQh is for FULL
// speed control or bulk transfer. This is because FULL speed contrl
// or bulk transfer can reclaim the unused bandwidth. Some USB
// device requires this bandwidth reclamation capability.
//
typedef struct _USB_HC_DEV {
    UINT32                    Signature;
    LIST_ENTRY                Link;
    EFI_USB_HC_PROTOCOL       UsbHc;
    EFI_USB2_HC_PROTOCOL      Usb2Hc;
    UINT32                    UsbIoBase;
    EFI_PCI_FUNCTION_ADDRESS  PciAddress;
    EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

    //
    // Schedule data structures
    //
    UINT32                    *FrameBase;
    UHCI_QH_SW                *SyncIntQh;
    UHCI_QH_SW                *CtrlQh;
    UHCI_QH_SW                *BulkQh;

    //
    // Structures to maintain asynchronus interrupt transfers.
    // When asynchronous interrutp transfer is unlinked from
    // the frame list, the hardware may still hold a pointer
    // to it. To synchronize with hardware, its resoureces are
    // released in two steps using Recycle and RecycleWait.
    // Check the asynchronous interrupt management routines.
    //
    LIST_ENTRY                AsyncIntList;
    EFI_EVENT                 AsyncIntMonitor;
    UHCI_ASYNC_REQUEST        *Recycle;
    UHCI_ASYNC_REQUEST        *RecycleWait;

    UINTN                     RootPorts;
} USB_HC_DEV;

EFI_STATUS
EFIAPI
UsbConnectUhciDriver (
    IN EFI_PCI_FUNCTION_ADDRESS   *PciAddress,
    OUT EFI_USB2_HC_PROTOCOL      **Usb2HCInterface,
    OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
);

EFI_STATUS
EFIAPI
UsbDisconnectUhciDriver (
    IN EFI_USB2_HC_PROTOCOL     *Usb2HCInterface
);

#endif
