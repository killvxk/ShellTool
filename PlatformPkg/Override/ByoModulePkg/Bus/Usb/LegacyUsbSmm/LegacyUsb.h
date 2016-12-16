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

#ifndef _LEGACY_USB_H_
#define _LEGACY_USB_H_

#include <PiSmm.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmGpiDispatch2.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Framework/SmmCis.h>
#include <protocol/SmmBaseHelperReady.h>

#include "../Include/Usb.h"
#include "../Uhci/Uhci.h"
#include "../Ohci/Ohci.h"

#define LANGUAGE_CODE_ENGLISH2  "en-US"
#define LANGUAGE_CODE_ENGLISH   "eng"
#define USB_SOFT_SMI_VALUE      0xF0
#define USB_ASPI_SMI_VALUE      0x50

EFI_STATUS
EFIAPI
StartLegacyUsb(
    VOID
)
;

EFI_STATUS
EFIAPI
EndLegacyUsb(
    VOID
)
;

EFI_STATUS
EFIAPI
GetUhcInfo (
    IN EFI_PCI_FUNCTION_ADDRESS *PciAddress,
    IN VOID                     **SmmUhc,
    IN VOID                     **SmmUsbHc,
    IN VOID                     **SmmUsb2Hc
)
;

EFI_STATUS
EFIAPI
UhciReset (
    IN EFI_USB_HC_PROTOCOL     *This,
    IN UINT16                  Attributes
)
;

EFI_STATUS
EFIAPI
UhciGetState (
    IN  EFI_USB_HC_PROTOCOL     *This,
    OUT EFI_USB_HC_STATE        *State
)
;

EFI_STATUS
EFIAPI
UhciSetState (
    IN EFI_USB_HC_PROTOCOL     *This,
    IN EFI_USB_HC_STATE        State
)
;

EFI_STATUS
EFIAPI
UhciGetRootHubPortNumber (
    IN  EFI_USB_HC_PROTOCOL     *This,
    OUT UINT8                   *PortNumber
)
;

EFI_STATUS
EFIAPI
UhciGetRootHubPortStatus (
    IN  EFI_USB_HC_PROTOCOL     *This,
    IN  UINT8                   PortNumber,
    OUT EFI_USB_PORT_STATUS     *PortStatus
)
;

EFI_STATUS
EFIAPI
UhciSetRootHubPortFeature (
    IN  EFI_USB_HC_PROTOCOL     *This,
    IN  UINT8                   PortNumber,
    IN  EFI_USB_PORT_FEATURE    PortFeature
)
;

EFI_STATUS
EFIAPI
UhciClearRootHubPortFeature (
    IN  EFI_USB_HC_PROTOCOL     *This,
    IN  UINT8                   PortNumber,
    IN  EFI_USB_PORT_FEATURE    PortFeature
)
;

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
;

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
;

EFI_STATUS
EFIAPI
UhciAsyncInterruptOnlyTransfer (
    IN     EFI_USB_HC_PROTOCOL                * This,
    IN     BOOLEAN                            IsNewTransfer,
    IN     UINTN                              PollingInterval,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
    IN     VOID                               *Context
)
;

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
;

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
;

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
;

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
;

EFI_STATUS
EFIAPI
Uhci2Reset (
    IN EFI_USB2_HC_PROTOCOL   *This,
    IN UINT16                 Attributes
)
;

EFI_STATUS
EFIAPI
Uhci2GetState (
    IN  EFI_USB2_HC_PROTOCOL   *This,
    OUT EFI_USB_HC_STATE       *State
)
;

EFI_STATUS
EFIAPI
Uhci2SetState (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN EFI_USB_HC_STATE        State
)
;

EFI_STATUS
EFIAPI
Uhci2GetCapability (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT UINT8                 *MaxSpeed,
    OUT UINT8                 *PortNumber,
    OUT UINT8                 *Is64BitCapable
)
;

EFI_STATUS
EFIAPI
Uhci2GetRootHubPortStatus (
    IN  EFI_USB2_HC_PROTOCOL   *This,
    IN  UINT8                  PortNumber,
    OUT EFI_USB_PORT_STATUS    *PortStatus
)
;

EFI_STATUS
EFIAPI
Uhci2SetRootHubPortFeature (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN UINT8                   PortNumber,
    IN EFI_USB_PORT_FEATURE    PortFeature
)
;

EFI_STATUS
EFIAPI
Uhci2ClearRootHubPortFeature (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN UINT8                   PortNumber,
    IN EFI_USB_PORT_FEATURE    PortFeature
)
;

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
;

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
;

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
;

EFI_STATUS
EFIAPI
Uhci2AsyncInterruptOnlyTransfer (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     BOOLEAN                            IsNewTransfer,
    IN     UINTN                              PollingInterval,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
    IN     VOID                               *Context
)
;

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
;

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
;

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
;

EFI_STATUS
EFIAPI
GetEhcInfo (
    IN EFI_PCI_FUNCTION_ADDRESS *PciAddress,
    IN VOID                     **SmmUsb2HcDev,
    IN VOID                     **SmmUsb2Hc
)
;

EFI_STATUS
EFIAPI
EhcGetCapability (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT UINT8                 *MaxSpeed,
    OUT UINT8                 *PortNumber,
    OUT UINT8                 *Is64BitCapable
)
;

EFI_STATUS
EFIAPI
EhcReset (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT16               Attributes
)
;
EFI_STATUS
EFIAPI
EhcGetState (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT EFI_USB_HC_STATE      *State
)
;

EFI_STATUS
EFIAPI
EhcSetState (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN EFI_USB_HC_STATE     State
)
;

EFI_STATUS
EFIAPI
EhcGetRootHubPortStatus (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    OUT EFI_USB_PORT_STATUS   *PortStatus
)
;

EFI_STATUS
EFIAPI
EhcSetRootHubPortFeature (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    IN  EFI_USB_PORT_FEATURE  PortFeature
)
;

EFI_STATUS
EFIAPI
EhcClearRootHubPortFeature (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    IN  EFI_USB_PORT_FEATURE  PortFeature
)
;

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
;

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
;

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
;

EFI_STATUS
EFIAPI
EhcAsyncInterruptOnlyTransfer (
    IN  EFI_USB2_HC_PROTOCOL                  * This,
    IN  BOOLEAN                               IsNewTransfer,
    IN  UINTN                                 PollingInterval,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
    IN  VOID                                  *Context
)
;

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
;

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
;

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
;

EFI_STATUS
EFIAPI
UsbIoControlTransfer (
    IN  EFI_USB_IO_PROTOCOL     *This,
    IN  EFI_USB_DEVICE_REQUEST  *Request,
    IN  EFI_USB_DATA_DIRECTION  Direction,
    IN  UINT32                  Timeout,
    IN  OUT VOID                *Data,      OPTIONAL
    IN  UINTN                   DataLength, OPTIONAL
    OUT UINT32                  *UsbStatus
)
;

EFI_STATUS
EFIAPI
UsbIoBulkTransfer (
    IN  EFI_USB_IO_PROTOCOL *This,
    IN  UINT8               Endpoint,
    IN  OUT VOID            *Data,
    IN  OUT UINTN           *DataLength,
    IN  UINTN               Timeout,
    OUT UINT32              *UsbStatus
)
;

EFI_STATUS
EFIAPI
UsbIoSyncInterruptTransfer (
    IN  EFI_USB_IO_PROTOCOL *This,
    IN  UINT8               Endpoint,
    IN  OUT VOID            *Data,
    IN  OUT UINTN           *DataLength,
    IN  UINTN               Timeout,
    OUT UINT32              *UsbStatus
)
;

EFI_STATUS
EFIAPI
UsbIoAsyncInterruptTransfer (
    IN EFI_USB_IO_PROTOCOL              *This,
    IN UINT8                            Endpoint,
    IN BOOLEAN                          IsNewTransfer,
    IN UINTN                            PollInterval,       OPTIONAL
    IN UINTN                            DataLength,         OPTIONAL
    IN EFI_ASYNC_USB_TRANSFER_CALLBACK  Callback,           OPTIONAL
    IN VOID                             *Context            OPTIONAL
)
;

EFI_STATUS
EFIAPI
UsbIoAsyncInterruptOnlyTransfer (
    IN EFI_USB_IO_PROTOCOL              *This,
    IN BOOLEAN                          IsNewTransfer,
    IN UINTN                            PollInterval,
    IN EFI_ASYNC_USB_TRANSFER_CALLBACK  Callback,
    IN VOID                             *Context
)
;

EFI_STATUS
EFIAPI
UsbIoIsochronousTransfer (
    IN  EFI_USB_IO_PROTOCOL *This,
    IN  UINT8               DeviceEndpoint,
    IN  OUT VOID            *Data,
    IN  UINTN               DataLength,
    OUT UINT32              *Status
)
;

EFI_STATUS
EFIAPI
UsbIoAsyncIsochronousTransfer (
    IN EFI_USB_IO_PROTOCOL              *This,
    IN UINT8                            DeviceEndpoint,
    IN OUT VOID                         *Data,
    IN UINTN                            DataLength,
    IN EFI_ASYNC_USB_TRANSFER_CALLBACK  IsochronousCallBack,
    IN VOID                             *Context              OPTIONAL
)
;

EFI_STATUS
EFIAPI
UsbIoGetDeviceDescriptor (
    IN  EFI_USB_IO_PROTOCOL       *This,
    OUT EFI_USB_DEVICE_DESCRIPTOR *Descriptor
)
;

EFI_STATUS
EFIAPI
UsbIoGetActiveConfigDescriptor (
    IN  EFI_USB_IO_PROTOCOL       *This,
    OUT EFI_USB_CONFIG_DESCRIPTOR *Descriptor
)
;

EFI_STATUS
EFIAPI
UsbIoGetInterfaceDescriptor (
    IN  EFI_USB_IO_PROTOCOL           *This,
    OUT EFI_USB_INTERFACE_DESCRIPTOR  *Descriptor
)
;


EFI_STATUS
EFIAPI
UsbIoGetEndpointDescriptor (
    IN  EFI_USB_IO_PROTOCOL         *This,
    IN  UINT8                       Index,
    OUT EFI_USB_ENDPOINT_DESCRIPTOR *Descriptor
)
;

EFI_STATUS
EFIAPI
UsbIoGetSupportedLanguages (
    IN  EFI_USB_IO_PROTOCOL *This,
    OUT UINT16              **LangIDTable,
    OUT UINT16              *TableSize
)
;

EFI_STATUS
EFIAPI
UsbIoGetStringDescriptor (
    IN  EFI_USB_IO_PROTOCOL   *This,
    IN  UINT16                LangID,
    IN  UINT8                 StringIndex,
    OUT CHAR16                **String
)
;

EFI_STATUS
EFIAPI
UsbIoPortReset (
    IN EFI_USB_IO_PROTOCOL  *This
)
;

EFI_STATUS
EFIAPI
ScanUsbBus (
    IN LIST_ENTRY    *NativeUsbIfList,
    IN UINTN          NativeUsbIfSize
)
;



EFI_STATUS
EFIAPI
GetXhcInfo (
    IN EFI_PCI_FUNCTION_ADDRESS *PciAddress,
    IN VOID                     **SmmUsb2HcDev,
    IN VOID                     **SmmUsb2Hc
)
;

EFI_STATUS
EFIAPI
XhcGetCapability (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT UINT8                 *MaxSpeed,
    OUT UINT8                 *PortNumber,
    OUT UINT8                 *Is64BitCapable
)
;

EFI_STATUS
EFIAPI
XhcReset (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT16               Attributes
)
;
EFI_STATUS
EFIAPI
XhcGetState (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT EFI_USB_HC_STATE      *State
)
;

EFI_STATUS
EFIAPI
XhcSetState (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN EFI_USB_HC_STATE     State
)
;

EFI_STATUS
EFIAPI
XhcGetRootHubPortStatus (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    OUT EFI_USB_PORT_STATUS   *PortStatus
)
;

EFI_STATUS
EFIAPI
XhcSetRootHubPortFeature (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    IN  EFI_USB_PORT_FEATURE  PortFeature
)
;

EFI_STATUS
EFIAPI
XhcClearRootHubPortFeature (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    IN  UINT8                 PortNumber,
    IN  EFI_USB_PORT_FEATURE  PortFeature
)
;

EFI_STATUS
EFIAPI
XhcControlTransfer (
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
;

EFI_STATUS
EFIAPI
XhcBulkTransfer (
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
;

EFI_STATUS
EFIAPI
XhcAsyncInterruptTransfer (
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
;

EFI_STATUS
EFIAPI
XhcAsyncInterruptOnlyTransfer (
    IN  EFI_USB2_HC_PROTOCOL                  * This,
    IN  BOOLEAN                               IsNewTransfer,
    IN  UINTN                                 PollingInterval,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
    IN  VOID                                  *Context
)
;

EFI_STATUS
EFIAPI
XhcSyncInterruptTransfer (
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
;

EFI_STATUS
EFIAPI
XhcIsochronousTransfer (
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
;

EFI_STATUS
EFIAPI
XhcAsyncIsochronousTransfer (
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
;
#endif
