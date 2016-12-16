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
  UsbDesc.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _USB_DESCRIPTOR_H_
#define _USB_DESCRIPTOR_H_

enum {
    USB_MAX_INTERFACE_SETTING  = 8,
};

//
// The RequestType in EFI_USB_DEVICE_REQUEST is composed of
// three fields: One bit direction, 2 bit type, and 5 bit
// target.
//
#define USB_REQUEST_TYPE(Dir, Type, Target) \
          ((UINT8)((((Dir) == EfiUsbDataIn ? 0x01 : 0) << 7) | (Type) | (Target)))

//
// A common header for usb standard descriptor.
// Each stand descriptor has a length and type.
//
#pragma pack(1)
typedef struct {
    UINT8                   Len;
    UINT8                   Type;
} USB_DESC_HEAD;
#pragma pack()


//
// Each USB device has a device descriptor. Each device may
// have several configures. Each configure contains several
// interfaces. Each interface may have several settings. Each
// setting has several endpoints.
//
// EFI_USB_..._DESCRIPTOR must be the first member of the
// structure.
//
typedef struct {
    EFI_USB_ENDPOINT_DESCRIPTOR   Desc;
    UINT8                         Toggle;
} USB_ENDPOINT_DESC;

typedef struct {
    EFI_USB_INTERFACE_DESCRIPTOR  Desc;
    USB_ENDPOINT_DESC             **Endpoints;
} USB_INTERFACE_SETTING;

//
// An interface may have several settings. Use a
// fixed max number of settings to simplify code.
// It should sufice in most environments.
//
typedef struct {
    USB_INTERFACE_SETTING*        Settings[USB_MAX_INTERFACE_SETTING];
    UINTN                         NumOfSetting;
    UINT8                         ActiveIndex;  // Index of active setting
} USB_INTERFACE_DESC;

typedef struct {
    EFI_USB_CONFIG_DESCRIPTOR     Desc;
    USB_INTERFACE_DESC            **Interfaces;
} USB_CONFIG_DESC;

typedef struct {
    EFI_USB_DEVICE_DESCRIPTOR     Desc;
    USB_CONFIG_DESC               **Configs;
} USB_DEVICE_DESC;

EFI_STATUS
UsbCtrlRequest (
    IN USB_DEVICE             *UsbDev,
    IN EFI_USB_DATA_DIRECTION Direction,
    IN UINTN                  Type,
    IN UINTN                  Target,
    IN UINTN                  Request,
    IN UINT16                 Value,
    IN UINT16                 Index,
    IN OUT VOID               *Buf,
    IN UINTN                  Length
);

EFI_STATUS
UsbGetMaxPacketSize0 (
    IN USB_DEVICE           *UsbDev
);

VOID
UsbFreeDevDesc (
    IN USB_DEVICE_DESC      *DevDesc
);

EFI_USB_STRING_DESCRIPTOR*
UsbGetOneString (
    IN     USB_DEVICE       *UsbDev,
    IN     UINT8            StringIndex,
    IN     UINT16           LangId
);

EFI_STATUS
UsbBuildDescTable (
    IN USB_DEVICE           *UsbDev
);

EFI_STATUS
UsbSetAddress (
    IN USB_DEVICE           *UsbDev,
    IN UINT8                Address
);

EFI_STATUS
UsbSetConfig (
    IN USB_DEVICE           *UsbDev,
    IN UINT8                ConfigIndex
);

EFI_STATUS
UsbIoClearFeature (
    IN  EFI_USB_IO_PROTOCOL *UsbIo,
    IN  UINTN               Target,
    IN  UINT16              Feature,
    IN  UINT16              Index
);
#endif
