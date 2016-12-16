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
  Xhci.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _XHCI_H_
#define _XHCI_H_

#include <Protocol/PciIo.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include "../Usblib/SmmPciIo.h"
#include "../Include/UsbHostController.h"
#include "../UsbBus/UsbBus.h"
#include "../UsbLib/UsbMem.h"

#define XHCI_DEBUG(a)   DEBUG(a)

#define MaxUsbDevContext    50

#define BIT(a)  (1 << (a))

typedef struct _USB_XHCI_INSTANCE    USB_XHCI_INSTANCE;
typedef struct _USB_DEV_CONTEXT      USB_DEV_CONTEXT;

#include "XhciReg.h"
#include "XhciSched.h"

BOOLEAN                 mXhciOSRequestOwnership;
LIST_ENTRY              mXhcList;

//
// Convert millisecond to microsecond.
//
#define XHC_1_MILLISECOND            (1000)
//
// XHC generic timeout experience values.
// The unit is microsecond, setting it as 10ms.
//
#define XHC_GENERIC_TIMEOUT          (10 * 1000)
//
// XHC reset timeout experience values.
// The unit is microsecond, setting it as 1s.
//
#define XHC_RESET_TIMEOUT            (1000 * 1000)
//
// XHC delay experience value for polling operation.
// The unit is microsecond, set it as 1ms.
//
#define XHC_POLL_DELAY               (1000)
//
// XHC async transfer timer interval, set by experience.
// The unit is 100us, takes 50ms as interval.
//
#define XHC_ASYNC_TIMER_INTERVAL     EFI_TIMER_PERIOD_MILLISECONDS(50)
#define CMD_RING_TRB_NUMBER          0x100
#define TR_RING_TRB_NUMBER           0x100
#define ERST_NUMBER                  0x01
#define EVENT_RING_TRB_NUMBER        0x200

#define CMD_INTER                    0
#define CTRL_INTER                   1
#define BULK_INTER                   2
#define INT_INTER                    3
#define INT_INTER_ASYNC              4

//
// Iterate through the doule linked list. This is delete-safe.
// Don't touch NextEntry
//
#ifndef _EFI_EHCI_SCHED_H_
#define EFI_LIST_FOR_EACH_SAFE(Entry, NextEntry, ListHead) \
  for (Entry = (ListHead)->ForwardLink, NextEntry = Entry->ForwardLink;\
      Entry != (ListHead); Entry = NextEntry, NextEntry = Entry->ForwardLink)

#define EFI_LIST_CONTAINER(Entry, Type, Field) BASE_CR(Entry, Type, Field)
#endif
#define XHC_LOW_32BIT(Addr64)          ((UINT32)(((UINTN)(Addr64)) & 0xFFFFFFFF))
#define XHC_HIGH_32BIT(Addr64)         ((UINT32)(RShiftU64((UINT64)(UINTN)(Addr64), 32) & 0xFFFFFFFF))
#define XHC_BIT_IS_SET(Data, Bit)      ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define XHC_REG_BIT_IS_SET(Xhc, Offset, Bit) \
          (XHC_BIT_IS_SET(XhcReadOpReg ((Xhc), (Offset)), (Bit)))

#define XHCI_IS_DATAIN(EndpointAddr)   XHC_BIT_IS_SET((EndpointAddr), 0x80)

#define XHCI_INSTANCE_SIG              SIGNATURE_32 ('x', 'h', 'c', 'i')
#define XHC_FROM_THIS(a)               CR(a, USB_XHCI_INSTANCE, Usb2Hc, XHCI_INSTANCE_SIG)

#define USB_DESC_TYPE_HUB              0x29
#define USB_DESC_TYPE_HUB_SUPER_SPEED  0x2a

//
// The RequestType in EFI_USB_DEVICE_REQUEST is composed of
// three fields: One bit direction, 2 bit type, and 5 bit
// target.
//
#define USB_REQUEST_TYPE(Dir, Type, Target) \
          ((UINT8)((((Dir) == EfiUsbDataIn ? 0x01 : 0) << 7) | (Type) | (Target)))

//
// Xhci Data and Ctrl Structures
//
#ifndef _EFI_UHCI_SCHED_H_
#pragma pack(1)
typedef struct {
  UINT8                     PI;
  UINT8                     SubClassCode;
  UINT8                     BaseCode;
} USB_CLASSC;
#pragma pack()
#endif
struct _USB_DEV_CONTEXT {
  //
  // Whether this entry in UsbDevContext array is used or not.
  //
  BOOLEAN                   Enabled;
  //
  // The slot id assigned to the new device through XHCI's Enable_Slot cmd.
  //
  UINT8                     SlotId;
  //
  // The route string presented an attached usb device.
  //
  USB_DEV_ROUTE             RouteString;
  //
  // The route string of parent device if it exists. Otherwise it's zero.
  //
  USB_DEV_ROUTE             ParentRouteString;
  //
  // The actual device address assigned by XHCI through Address_Device command.
  //
  UINT8                     XhciDevAddr;
  //
  // The requested device address from UsbBus driver through Set_Address standard usb request.
  // As XHCI spec replaces this request with Address_Device command, we have to record the
  // requested device address and establish a mapping relationship with the actual device address.
  // Then UsbBus driver just need to be aware of the requested device address to access usb device
  // through EFI_USB2_HC_PROTOCOL. Xhci driver would be responsible for translating it to actual
  // device address and access the actual device.
  //
  UINT8                     BusDevAddr;
  //
  // The pointer to the input device context.
  //
  VOID                      *InputContext;
  //
  // The pointer to the output device context.
  //
  VOID                      *OutputContext;
  //
  // The transfer queue for every endpoint.
  //
  VOID                      *EndpointTransferRing[31];
  //
  // The device descriptor which is stored to support XHCI's Evaluate_Context cmd.
  //
  EFI_USB_DEVICE_DESCRIPTOR DevDesc;
  //
  // As a usb device may include multiple configuration descriptors, we dynamically allocate an array
  // to store them.
  // Note that every configuration descriptor stored here includes those lower level descriptors,
  // such as Interface descriptor, Endpoint descriptor, and so on.
  // These information is used to support XHCI's Config_Endpoint cmd.
  //
  EFI_USB_CONFIG_DESCRIPTOR **ConfDesc;
};

struct _USB_XHCI_INSTANCE {
  UINT32                    Signature;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  LIST_ENTRY                AsyncIntTransfers;
  LIST_ENTRY                AsyncIntOnlyTransfers;
  LIST_ENTRY                Link;

  UINT8                     CapLength;    ///< Capability Register Length
  XHC_HCSPARAMS1            HcSParams1;   ///< Structural Parameters 1
  XHC_HCSPARAMS2            HcSParams2;   ///< Structural Parameters 2
  XHC_HCCPARAMS             HcCParams;    ///< Capability Parameters
  UINT32                    DBOff;        ///< Doorbell Offset
  UINT32                    RTSOff;       ///< Runtime Register Space Offset
  UINT16                    MaxInterrupt;
  UINT32                    PageSize;
  UINT64                    *ScratchBuf;
  UINT32                    MaxScratchpadBufs;
  UINT32                    ExtCapRegBase;
  UINT32                    UsbLegSupOffset;
  UINT64                    *DCBAA;
  UINT32                    MaxSlotsEn;
  //
  // Cmd Transfer Ring
  //
  TRANSFER_RING             CmdRing;
  //
  // EventRing
  //
  EVENT_RING                EventRing;
  //
  // Store device contexts managed by XHCI instance
  // The array supports up to 255 devices, entry 0 is reserved and should not be used.
  //
  USB_DEV_CONTEXT           UsbDevContext[MaxUsbDevContext];
  UINT32                    UsbMemBase;
  EFI_PCI_FUNCTION_ADDRESS  PciAddress;
};

#endif
