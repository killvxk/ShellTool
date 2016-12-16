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
  UsbLib.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _USB_LIB_H
#define _USB_LIB_H

#include <PiSmm.h>

#include "../Include/UsbIo.h"

//
// Standard device request and request type
// By [Spec-USB20/Chapter-9.4]
//
#define USB_DEV_GET_STATUS                  0x00
#define USB_DEV_GET_STATUS_REQ_TYPE_D       0x80 // Receiver : Device
#define USB_DEV_GET_STATUS_REQ_TYPE_I       0x81 // Receiver : Interface
#define USB_DEV_GET_STATUS_REQ_TYPE_E       0x82 // Receiver : Endpoint

#define USB_DEV_CLEAR_FEATURE               0x01
#define USB_DEV_CLEAR_FEATURE_REQ_TYPE_D    0x00 // Receiver : Device
#define USB_DEV_CLEAR_FEATURE_REQ_TYPE_I    0x01 // Receiver : Interface
#define USB_DEV_CLEAR_FEATURE_REQ_TYPE_E    0x02 // Receiver : Endpoint

#define USB_DEV_SET_FEATURE                 0x03
#define USB_DEV_SET_FEATURE_REQ_TYPE_D      0x00 // Receiver : Device
#define USB_DEV_SET_FEATURE_REQ_TYPE_I      0x01 // Receiver : Interface
#define USB_DEV_SET_FEATURE_REQ_TYPE_E      0x02 // Receiver : Endpoint

#define USB_DEV_SET_ADDRESS                 0x05
#define USB_DEV_SET_ADDRESS_REQ_TYPE        0x00

#define USB_DEV_GET_DESCRIPTOR              0x06
#define USB_DEV_GET_DESCRIPTOR_REQ_TYPE     0x80

#define USB_DEV_SET_DESCRIPTOR              0x07
#define USB_DEV_SET_DESCRIPTOR_REQ_TYPE     0x00

#define USB_DEV_GET_CONFIGURATION           0x08
#define USB_DEV_GET_CONFIGURATION_REQ_TYPE  0x80

#define USB_DEV_SET_CONFIGURATION           0x09
#define USB_DEV_SET_CONFIGURATION_REQ_TYPE  0x00

#define USB_DEV_GET_INTERFACE               0x0A
#define USB_DEV_GET_INTERFACE_REQ_TYPE      0x81

#define USB_DEV_SET_INTERFACE               0x0B
#define USB_DEV_SET_INTERFACE_REQ_TYPE      0x01

#define USB_DEV_SYNCH_FRAME                 0x0C
#define USB_DEV_SYNCH_FRAME_REQ_TYPE        0x82

#define TIMEOUT_VALUE 3 * 1000

EFI_STATUS
UsbGetDescriptor (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  Value,
    IN  UINT16                  Index,
    IN  UINT16                  DescriptorLength,
    OUT VOID                    *Descriptor,
    OUT UINT32                  *Status
);

EFI_STATUS
UsbSetDescriptor (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  Value,
    IN  UINT16                  Index,
    IN  UINT16                  DescriptorLength,
    IN  VOID                    *Descriptor,
    OUT UINT32                  *Status
);

EFI_STATUS
UsbGetInterface (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  Index,
    OUT UINT8                   *AltSetting,
    OUT UINT32                  *Status
);

EFI_STATUS
UsbSetInterface (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  InterfaceNo,
    IN  UINT16                  AltSetting,
    OUT UINT32                  *Status
);

EFI_STATUS
UsbGetConfiguration (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    OUT UINT8                   *ConfigValue,
    OUT UINT32                  *Status
);

EFI_STATUS
UsbSetConfiguration (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  Value,
    OUT UINT32                  *Status
);

EFI_STATUS
UsbSetFeature (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINTN       Recipient,
    IN  UINT16                  Value,
    IN  UINT16                  Target,
    OUT UINT32                  *Status
);

EFI_STATUS
UsbClearFeature (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINTN       Recipient,
    IN  UINT16                  Value,
    IN  UINT16                  Target,
    OUT UINT32                  *Status
);

EFI_STATUS
UsbGetStatus (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINTN       Recipient,
    IN  UINT16                  Target,
    OUT UINT16                  *DevStatus,
    OUT UINT32                  *Status
);

EFI_STATUS
UsbGetHubDescriptor (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT16                  Value,
    IN  UINT16                  Index,
    IN  UINT16                  DescriptorLength,
    OUT VOID                    *Descriptor,
    OUT UINT32                  *Status
);

//
// Clear endpoint stall
//
EFI_STATUS
UsbClearEndpointHalt (
    IN  EFI_USB_IO_PROTOCOL     *UsbIo,
    IN  UINT8                   EndpointNo,
    OUT UINT32                  *Status
);

#endif
