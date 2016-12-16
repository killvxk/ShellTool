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
  UsbUtility.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _USB_UTILITY_H
#define _USB_UTILITY_H


#include <PiSmm.h>
#include "../UsbBus/UsbBus.h"

#include <library/DevicePathLib.h>


#define USE_LITTLE_ENDIAN 1

#if USE_LITTLE_ENDIAN

#define ACPI_PATH                   { ACPI_DEVICE_PATH, ACPI_DP, (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8), EISA_PNP_ID(0x0A03), 0 }
#define PCI_USB_PATH(device, function)      { HARDWARE_DEVICE_PATH, HW_PCI_DP, (UINT8) (sizeof (PCI_DEVICE_PATH)), (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8), function, device}
#define END_PATH                    { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, END_DEVICE_PATH_LENGTH, 0x00 }

#else   // USE_LITTLE_ENDIAN

#define ACPI_PATH                   { ACPI_DEVICE_PATH, ACPI_DP, (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8), (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), EISA_PNP_ID(0x0A03), 0 }
#define PCI_USB_PATH(device, function)      { HARDWARE_DEVICE_PATH, HW_PCI_DP, (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8), (UINT8) (sizeof (PCI_DEVICE_PATH)), device, function}
#define END_PATH                    { END_DEVICE_PATH_TYPE | EFI_DP_TYPE_UNPACKED, END_ENTIRE_DEVICE_PATH_SUBTYPE, 0x00, END_DEVICE_PATH_LENGTH }

#endif  // USE_LITTLE_ENDIAN

#define USB_DEVICE_PATH_TEMPLATE  { ACPI_PATH, PCI_USB_PATH( 0, 0 ), END_PATH }

typedef struct _FULL_USB_DEVICE_PATH {
    ACPI_HID_DEVICE_PATH      Acpi;
    PCI_DEVICE_PATH           Pci;
    EFI_DEVICE_PATH_PROTOCOL  End;
} FULL_USB_DEVICE_PATH;

EFI_STATUS
UsbHcGetCapability (
    IN  USB_BUS             *UsbBus,
    OUT UINT8               *MaxSpeed,
    OUT UINT8               *NumOfPort,
    OUT UINT8               *Is64BitCapable
);

EFI_STATUS
UsbHcReset (
    IN USB_BUS              *UsbBus,
    IN UINT16               Attributes
);

EFI_STATUS
UsbHcGetState (
    IN  USB_BUS             *UsbBus,
    OUT EFI_USB_HC_STATE    *State
);

EFI_STATUS
UsbHcSetState (
    IN  USB_BUS             *UsbBus,
    IN EFI_USB_HC_STATE     State
);

EFI_STATUS
UsbHcGetRootHubPortStatus (
    IN  USB_BUS             *UsbBus,
    IN  UINT8               PortIndex,
    OUT EFI_USB_PORT_STATUS *PortStatus
);

EFI_STATUS
UsbHcSetRootHubPortFeature (
    IN USB_BUS              *UsbBus,
    IN UINT8                PortIndex,
    IN EFI_USB_PORT_FEATURE Feature
);

EFI_STATUS
UsbHcClearRootHubPortFeature (
    IN USB_BUS              *UsbBus,
    IN UINT8                PortIndex,
    IN EFI_USB_PORT_FEATURE Feature
);

EFI_STATUS
UsbHcControlTransfer (
    IN  USB_BUS                             *UsbBus,
    IN  UINT8                               DevAddr,
    IN  UINT8                               DevSpeed,
    IN  UINTN                               MaxPacket,
    IN  EFI_USB_DEVICE_REQUEST              *Request,
    IN  EFI_USB_DATA_DIRECTION              Direction,
    IN  OUT VOID                            *Data,
    IN  OUT UINTN                           *DataLength,
    IN  UINTN                               TimeOut,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *UsbResult
);

EFI_STATUS
UsbHcBulkTransfer (
    IN  USB_BUS                             *UsbBus,
    IN  UINT8                               DevAddr,
    IN  UINT8                               EpAddr,
    IN  UINT8                               DevSpeed,
    IN  UINTN                               MaxPacket,
    IN  UINT8                               BufferNum,
    IN  OUT VOID                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
    IN  OUT UINTN                           *DataLength,
    IN  OUT UINT8                           *DataToggle,
    IN  UINTN                               TimeOut,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *UsbResult
);

EFI_STATUS
UsbHcAsyncInterruptTransfer (
    IN  USB_BUS                             *UsbBus,
    IN  UINT8                               DevAddr,
    IN  UINT8                               EpAddr,
    IN  UINT8                               DevSpeed,
    IN  UINTN                               MaxPacket,
    IN  BOOLEAN                             IsNewTransfer,
    IN OUT UINT8                            *DataToggle,
    IN  UINTN                               PollingInterval,
    IN  UINTN                               DataLength,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     Callback,
    IN  VOID                                *Context OPTIONAL
);

EFI_STATUS
UsbHcAsyncInterruptOnlyTransfer (
    IN  USB_BUS                             *UsbBus,
    IN  BOOLEAN                             IsNewTransfer,
    IN  UINTN                               PollingInterval,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     Callback,
    IN  VOID                                *Context
);

EFI_STATUS
UsbHcSyncInterruptTransfer (
    IN  USB_BUS                             *UsbBus,
    IN  UINT8                               DevAddr,
    IN  UINT8                               EpAddr,
    IN  UINT8                               DevSpeed,
    IN  UINTN                               MaxPacket,
    IN OUT VOID                             *Data,
    IN OUT UINTN                            *DataLength,
    IN OUT UINT8                            *DataToggle,
    IN  UINTN                               TimeOut,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *UsbResult
);

EFI_STATUS
UsbHcIsochronousTransfer (
    IN  USB_BUS                             *UsbBus,
    IN  UINT8                               DevAddr,
    IN  UINT8                               EpAddr,
    IN  UINT8                               DevSpeed,
    IN  UINTN                               MaxPacket,
    IN  UINT8                               BufferNum,
    IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN  UINTN                               DataLength,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *UsbResult
);

EFI_STATUS
UsbHcAsyncIsochronousTransfer (
    IN  USB_BUS                             *UsbBus,
    IN  UINT8                               DevAddr,
    IN  UINT8                               EpAddr,
    IN  UINT8                               DevSpeed,
    IN  UINTN                               MaxPacket,
    IN  UINT8                               BufferNum,
    IN OUT VOID                             *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN  UINTN                               DataLength,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     Callback,
    IN  VOID                                *Context
);

EFI_STATUS
Stall (
    IN UINTN Microseconds
);

UINT8
SmmIoRead8 (
    IN  UINT64    Address
);

UINT16
SmmIoRead16 (
    IN  UINT64    Address
);

UINT32
SmmIoRead32 (
    IN  UINT64    Address
);

UINT8
SmmIoWrite8 (
    IN  UINT64    Address,
    IN  UINT8     Data
);

UINT16
SmmIoWrite16 (
    IN  UINT64    Address,
    IN  UINT16    Data
);

UINT32
SmmIoWrite32 (
    IN  UINT64    Address,
    IN  UINT32    Data
);

UINT32
SmmMmioWrite16 (
    IN  UINT64    Address,
    IN  UINT32    Data
);

UINT32
SmmMmioRead32 (
    IN  UINT64    Address
);

UINT32
SmmMmioWrite32 (
    IN  UINT64    Address,
    IN  UINT32    Data
);

UINT64
SmmMmioRead64 (
    IN  UINT64    Address
);

UINT64
SmmMmioWrite64 (
    IN  UINT64    Address,
    IN  UINT64    Data
);


#endif
