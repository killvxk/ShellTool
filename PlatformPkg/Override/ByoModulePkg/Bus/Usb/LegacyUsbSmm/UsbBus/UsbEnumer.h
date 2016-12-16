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
  UsbEnumer.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _USB_ENUMERATION_H_
#define _USB_ENUMERATION_H_

//
// Advance the byte and bit to the next bit, adjust byte accordingly.
//
#define USB_NEXT_BIT(Byte, Bit)   \
          do {                \
            (Bit)++;          \
            if ((Bit) > 7) {  \
              (Byte)++;       \
              (Bit) = 0;      \
            }                 \
          } while (0)


//
// Common interface used by usb bus enumeration process.
// This interface is defined to mask the difference between
// the root hub and normal hub. So, bus enumeration code
// can be shared by both root hub and normal hub
//
typedef
EFI_STATUS
(*USB_HUB_INIT) (
    IN USB_INTERFACE        *UsbIf
);

//
// Get the port status. This function is required to
// ACK the port change bits although it will return
// the port changes in PortState. Bus enumeration code
// doesn't need to ACK the port change bits.
//
typedef
EFI_STATUS
(*USB_HUB_GET_PORT_STATUS) (
    IN  USB_INTERFACE       *UsbIf,
    IN  UINT8               Port,
    OUT EFI_USB_PORT_STATUS *PortState
);

typedef
VOID
(*USB_HUB_CLEAR_PORT_CHANGE) (
    IN USB_INTERFACE        *HubIf,
    IN UINT8                Port
);

typedef
EFI_STATUS
(*USB_HUB_SET_PORT_FEATURE) (
    IN USB_INTERFACE        *UsbIf,
    IN UINT8                Port,
    IN EFI_USB_PORT_FEATURE Feature
);

typedef
EFI_STATUS
(*USB_HUB_CLEAR_PORT_FEATURE) (
    IN USB_INTERFACE        *UsbIf,
    IN UINT8                Port,
    IN EFI_USB_PORT_FEATURE Feature
);

typedef
EFI_STATUS
(*USB_HUB_RESET_PORT) (
    IN USB_INTERFACE        *UsbIf,
    IN UINT8                Port
);

typedef
EFI_STATUS
(*USB_HUB_RELEASE) (
    IN USB_INTERFACE        *UsbIf
);

typedef struct _USB_HUB_API {
    USB_HUB_INIT                Init;
    USB_HUB_GET_PORT_STATUS     GetPortStatus;
    USB_HUB_CLEAR_PORT_CHANGE   ClearPortChange;
    USB_HUB_SET_PORT_FEATURE    SetPortFeature;
    USB_HUB_CLEAR_PORT_FEATURE  ClearPortFeature;
    USB_HUB_RESET_PORT          ResetPort;
    USB_HUB_RELEASE             Release;
} USB_HUB_API;

USB_ENDPOINT_DESC*
UsbGetEndpointDesc (
    IN USB_INTERFACE        *UsbIf,
    IN UINT8                EpAddr
);


EFI_STATUS
UsbSelectSetting (
    IN USB_INTERFACE_DESC   *IfDesc,
    IN UINT8                Alternate
);

EFI_STATUS
UsbSelectConfig (
    IN USB_DEVICE           *Device,
    IN UINT8                ConfigIndex
);

VOID
UsbRemoveConfig (
    IN USB_DEVICE           *Device
);

STATIC
VOID
UsbDisconnectDriver (
    IN USB_INTERFACE        *UsbIf
);

EFI_STATUS
UsbRemoveDevice (
    IN USB_DEVICE           *Device
);

VOID
EFIAPI
UsbHubEnumeration (
    USB_INTERFACE           *HubIf
);

VOID
EFIAPI
UsbRootHubEnumeration (
    USB_INTERFACE           *RootHub
);
#endif
