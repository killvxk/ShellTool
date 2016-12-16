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

  EhciSched.h

Abstract:

  This file contains the definination for host controller schedule routines

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_EHCI_SCHED_H_
#define _EFI_EHCI_SCHED_H_

EFI_STATUS
EhcInitSched (
    IN USB2_HC_DEV          *Ehc
)
/*++

Routine Description:

  Initialize the schedule data structure such as frame list

Arguments:

  Ehc - The EHCI device to init schedule data for

Returns:

  EFI_OUT_OF_RESOURCES  - Failed to allocate resource to init schedule data
  EFI_SUCCESS           - The schedule data is initialized

--*/
;

VOID
EhcFreeSched (
    IN USB2_HC_DEV          *Ehc
)
/*++

Routine Description:

  Free the schedule data. It may be partially initialized.

Arguments:

  Ehc - The EHCI device

Returns:

  None

--*/
;

VOID
EhcLinkQhToAsync (
    IN USB2_HC_DEV          *Ehc,
    IN EHC_QH               *Qh
)
/*++

Routine Description:

  Link the queue head to the asynchronous schedule list.
  UEFI only supports one CTRL/BULK transfer at a time
  due to its interfaces. This simplifies the AsynList
  management: A reclamation header is always linked to
  the AsyncListAddr, the only active QH is appended to it.

Arguments:

  Ehc - The EHCI device
  Qh  - The queue head to link

Returns:

  None

--*/
;

VOID
EhcUnlinkQhFromAsync (
    IN USB2_HC_DEV          *Ehc,
    IN EHC_QH               *Qh
)
/*++

Routine Description:

  Unlink a queue head from the asynchronous schedule list.
  Need to synchronize with hardware

Arguments:

  Ehc - The EHCI device
  Qh  - The queue head to unlink

Returns:

  None

--*/
;

VOID
EhcLinkQhToPeriod (
    IN USB2_HC_DEV          *Ehc,
    IN EHC_QH               *Qh
)
/*++

Routine Description:

  Link a queue head for interrupt transfer to the periodic
  schedule frame list. This code is very much the same as
  that in UHCI.

Arguments:

  Ehc - The EHCI device
  Qh  - The queue head to link

Returns:

  None

--*/
;

VOID
EhcUnlinkQhFromPeriod (
    IN USB2_HC_DEV          *Ehc,
    IN EHC_QH               *Qh
)
/*++

Routine Description:

  Unlink an interrupt queue head from the periodic
  schedule frame list

Arguments:

  Ehc - The EHCI device
  Qh  - The queue head to unlink

Returns:

  None

--*/
;

EFI_STATUS
EhcExecTransfer (
    IN  USB2_HC_DEV         *Ehc,
    IN  URB                 *Urb,
    IN  UINTN               TimeOut
)
/*++

Routine Description:

  Execute the transfer by polling the URB. This is a synchronous operation.

Arguments:

  Ehc        - The EHCI device
  Urb        - The URB to execute
  TimeOut    - The time to wait before abort, in millisecond.

Returns:

  EFI_DEVICE_ERROR : The transfer failed due to transfer error
  EFI_TIMEOUT      : The transfer failed due to time out
  EFI_SUCCESS      : The transfer finished OK

--*/
;

EFI_STATUS
EhciDelAsyncIntTransfer (
    IN  USB2_HC_DEV         *Ehc,
    IN  UINT8               DevAddr,
    IN  UINT8               EpNum,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK CallBackFunction,
    IN  VOID                *Context,
    OUT UINT8               *DataToggle
)
/*++

Routine Description:

  Delete a single asynchronous interrupt transfer for
  the device and endpoint

Arguments:

  Ehc         - The EHCI device
  DevAddr     - The address of the target device
  EpNum       - The endpoint of the target
  DataToggle  - Return the next data toggle to use

Returns:

  EFI_SUCCESS   - An asynchronous transfer is removed
  EFI_NOT_FOUND - No transfer for the device is found

--*/
;

VOID
EhciDelAllAsyncIntTransfers (
    IN USB2_HC_DEV          *Ehc
)
/*++

Routine Description:

  Remove all the asynchronous interrutp transfers

Arguments:

  Ehc - The EHCI device

Returns:

  None

--*/
;


VOID
EFIAPI
EhcMoniteAsyncRequests (
    IN USB2_HC_DEV *            Ehc
)
/*++
Routine Description:

  Interrupt transfer periodic check handler

Arguments:
  Event    - Interrupt event
  Context  - Pointer to USB2_HC_DEV

Returns:

  None

--*/
;

enum {
    EHC_1_MICROSECOND            = 1,
    EHC_1_MILLISECOND            = 1000 * EHC_1_MICROSECOND,
    EHC_1_SECOND                 = 1000 * EHC_1_MILLISECOND,

    //
    // EHCI register operation timeout, set by experience
    //
    EHC_RESET_TIMEOUT            = 1 * EHC_1_SECOND,
    EHC_GENERIC_TIMEOUT          = 10 * EHC_1_MILLISECOND,

    //
    // Wait for roothub port power stable, refers to Spec[EHCI1.0-2.3.9]
    //
    EHC_ROOT_PORT_RECOVERY_STALL = 20 * EHC_1_MILLISECOND,

    //
    // Sync and Async transfer polling interval, set by experience,
    // and the unit of Async is 100us, means 50ms as interval.
    //
    EHC_SYNC_POLL_INTERVAL       = 20 * EHC_1_MICROSECOND,
    EHC_ASYNC_POLL_INTERVAL      = 50 * 10000U,

    USB2_HC_DEV_SIGNATURE        = SIGNATURE_32 ('e', 'h', 'c', 'i'),
};


///
///  _CR - returns a pointer to the structure
///      from one of it's elements.
///
#define _CR(Record, TYPE, Field)  ((TYPE *) ((CHAR8 *) (Record) - (CHAR8 *) &(((TYPE *) 0)->Field)))

//
//Iterate through the doule linked list. NOT delete safe
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

//
//Iterate through the doule linked list. This is delete-safe.
//Don't touch NextEntry
//
#define EFI_LIST_FOR_EACH_SAFE(Entry, NextEntry, ListHead)            \
  for(Entry = (ListHead)->ForwardLink, NextEntry = Entry->ForwardLink;\
      Entry != (ListHead); Entry = NextEntry, NextEntry = Entry->ForwardLink)

#define EFI_LIST_CONTAINER(Entry, Type, Field) _CR(Entry, Type, Field)

#define EHC_LOW_32BIT(Addr64)     ((UINT32)(((UINTN)(Addr64)) & 0XFFFFFFFF))
#define EHC_HIGH_32BIT(Addr64)    ((UINT32)(RShiftU64((UINTN)(Addr64), 32) & 0XFFFFFFFF))
#define EHC_BIT_IS_SET(Data, Bit) ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define EHC_REG_BIT_IS_SET(Ehc, Offset, Bit) \
          (EHC_BIT_IS_SET(EhcReadOpReg ((Ehc), (Offset)), (Bit)))

#define EHC_FROM_LINK(a)              CR (a, USB2_HC_DEV, Link, USB2_HC_DEV_SIGNATURE)
#define EHC_FROM_THIS(a)              CR(a, USB2_HC_DEV, Usb2Hc, USB2_HC_DEV_SIGNATURE)

typedef struct _USB2_HC_DEV {
    UINTN                     Signature;
    LIST_ENTRY            Link;
    EFI_USB2_HC_PROTOCOL      Usb2Hc;

    UINT32                    UsbMemBase;
    EFI_PCI_FUNCTION_ADDRESS  PciAddress;
    EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

    //
    // Schedule data shared between asynchronous and periodic
    // transfers:
    // ShortReadStop, as its name indicates, is used to terminate
    // the short read except the control transfer. EHCI follows
    // the alternative next QTD point when a short read happens.
    // For control transfer, even the short read happens, try the
    // status stage.
    //
    EHC_QTD                  *ShortReadStop;
    EFI_EVENT                 PollTimer;

    //
    // Asynchronous(bulk and control) transfer schedule data:
    // ReclaimHead is used as the head of the asynchronous transfer
    // list. It acts as the reclamation header.
    //
    EHC_QH                   *ReclaimHead;

    //
    // Peroidic (interrupt) transfer schedule data:
    //
    VOID                      *PeriodFrame;     // Mapped as common buffer
    VOID                      *PeriodFrameHost;
    VOID                      *PeriodFrameMap;

    EHC_QH                    *PeriodOne;
    LIST_ENTRY            AsyncIntTransfers;

    //
    // EHCI configuration data
    //
    UINT32                    HcStructParams; // Cache of HC structure parameter, EHC_HCSPARAMS_OFFSET
    UINT32                    HcCapParams;    // Cache of HC capability parameter, HCCPARAMS
    UINT32                    CapLen;         // Capability length
    UINT32                    High32bitAddr;

    //
    // Misc
    //
    //EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
    
    //
    // EHCI debug port info
    //
    UINT16                    DebugPortOffset; // The offset of debug port mmio register
    UINT8                     DebugPortBarNum; // The bar number of debug port mmio register
    UINT8                     DebugPortNum;    // The port number of usb debug port
} USB2_HC_DEV;

EFI_STATUS
EFIAPI
UsbConnectEhciDriver (
    IN  EFI_PCI_FUNCTION_ADDRESS  *PciAddress,
    OUT EFI_USB2_HC_PROTOCOL      **Usb2HCInterface,
    OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
);

EFI_STATUS
EFIAPI
UsbDisconnectEhciDriver (
    IN EFI_USB2_HC_PROTOCOL     *Usb2HCInterface
);

EFI_STATUS
EFIAPI
EhcEnableLegacySupport (
    IN       EFI_USB2_HC_PROTOCOL                 * This
);

EFI_STATUS
EFIAPI
EhcDisableLegacySupport (
    IN       EFI_USB2_HC_PROTOCOL                 * This
);

EFI_STATUS
EFIAPI
EhcLegacyCallback(
    IN       EFI_USB2_HC_PROTOCOL                 * This
);

VOID
EhcSetBiosOwnership (
    IN USB2_HC_DEV          *Ehc
);

VOID
EhcReleaseBiosOwnership (
    IN USB2_HC_DEV          *Ehc
);

#endif
