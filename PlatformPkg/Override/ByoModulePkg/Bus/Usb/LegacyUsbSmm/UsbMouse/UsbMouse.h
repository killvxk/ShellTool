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
  UsbMouse.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _USB_MOUSE_H
#define _USB_MOUSE_H

#include "../Include/usb.h"
#include "../Include/UsbIo.h"

typedef struct _USB_INTERFACE     USB_INTERFACE;



#define CLASS_HID               3
#define SUBCLASS_BOOT           1
#define PROTOCOL_MOUSE          2

#define BOOT_PROTOCOL           0
#define REPORT_PROTOCOL         1

#define USB_MOUSE_DEV_SIGNATURE SIGNATURE_32 ('u', 'm', 'o', 'u')

typedef struct {
    BOOLEAN ButtonDetected;
    UINT8   ButtonMinIndex;
    UINT8   ButtonMaxIndex;
    UINT8   Reserved;
} PRIVATE_DATA;

#define  KBC_CMD                0x64
#define  KBC_DATA               0x60
#define  KBC_IBE                0x02
#define  KBC_OBF                0x01
#define  KBC_READ_CMD           0x20
#define  KBC_WRITE_CMD          0x60
#define  KBC_CMD_INT1           0x01
#define  KBC_CMD_INT12          0x02
#define  KBC_KB_INTERFACE       0x10
#define  KBC_MS_INTERFACE       0x20

#define  PIC_MASTER             0x20
#define  PIC_SLAVE              0xA0
#define  PIC_INISR_CMD          0x0B
#define  PIC_IRQ12_IN_SERVICE   0x10
#define  KBC_DISABLE_MOUSE      0xA7
#define  KBC_ENABLE_MOUSE       0xA8
#define  KBC_SEND_MOUSE_DATA    0xD3
#define  PS2_MOUSE_XYSIGN       0x80
#define  PS2_MOUSE_YSIGN        0x20
#define  PS2_MOUSE_XSIGN        0x10
#define  PS2_MOUSE_DATA         0x08
#define  PS2_MOUSE_MBTN         0x04
#define  PS2_MOUSE_RBTN         0x02
#define  PS2_MOUSE_LBTN         0x01
#define  PS2_MOUSE_ALLBTN       0x7
#define  PS2_MOUSE_QUEUE        0x2
#define  PS2_QUEUE_MASK         PS2_MOUSE_QUEUE-1
#define  MS_IN_EMPTY_COUNTER    3000

#define  USB_MS_BTN_DATA        0x00
#define  USB_MS_X_DATA          0x01
#define  USB_MS_Y_DATA          0x02
#define  USBMS_KBC_CMD_INTERVAL 0x08

#define  USBMS_POLLING_INTERVAL 0x20
#define  BDA_HARDWARE           0x410
#define  BDA_HARDWARE_BIT_MOUSE 0x04

typedef  struct {
    UINT8     Cmd;
    UINT8     Data;
} MS_QUEUE;

typedef struct {
    MS_QUEUE      Queue[8];
    UINT8         QueueIndex;
    UINT8         Kbcstatus;
} MS_CTRL_DATA;

typedef struct {
    UINTN                         Signature;
    MS_CTRL_DATA                  MsCtrlData;
    EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
    EFI_EVENT                     DelayedRecoveryEvent;
    EFI_USB_IO_PROTOCOL           *UsbIo;
    EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
    EFI_USB_ENDPOINT_DESCRIPTOR   IntEndpointDescriptor;
    UINT8                         NumberOfButtons;
    INT32                         XLogicMax;
    INT32                         XLogicMin;
    INT32                         YLogicMax;
    INT32                         YLogicMin;
    BOOLEAN                       StateChanged;
    PRIVATE_DATA                  PrivateData;
    BOOLEAN                       QueuePollingActive;
} USB_MOUSE_DEV;

#define USB_MOUSE_DEV_FROM_CONTROL_DATA(This) \
    CR(This, USB_MOUSE_DEV, MsCtrlData, USB_MOUSE_DEV_SIGNATURE)

EFI_STATUS
EFIAPI
UsbConnectMouseDriver (
    IN USB_INTERFACE        *UsbIf
);

EFI_STATUS
EFIAPI
UsbDisconnectMouseDriver (
    IN USB_INTERFACE        *UsbIf
);

//
// HID Item general structure
//
typedef struct _hid_item {
    UINT16  Format;
    UINT8   Size;
    UINT8   Type;
    UINT8   Tag;
    union {
        UINT8   U8;
        UINT16  U16;
        UINT32  U32;
        INT8    I8;
        INT16   I16;
        INT32   I32;
        UINT8   *LongData;
    } Data;
} HID_ITEM;

typedef struct {
    UINT16  UsagePage;
    INT32   LogicMin;
    INT32   LogicMax;
    INT32   PhysicalMin;
    INT32   PhysicalMax;
    UINT16  UnitExp;
    UINT16  UINT;
    UINT16  ReportId;
    UINT16  ReportSize;
    UINT16  ReportCount;
} HID_GLOBAL;

typedef struct {
    UINT16  Usage[16];  /* usage array */
    UINT16  UsageIndex;
    UINT16  UsageMin;
} HID_LOCAL;

typedef struct {
    UINT16  Type;
    UINT16  Usage;
} HID_COLLECTION;

typedef struct {
    HID_GLOBAL      Global;
    HID_GLOBAL      GlobalStack[8];
    UINT32          GlobalStackPtr;
    HID_LOCAL       Local;
    HID_COLLECTION  CollectionStack[8];
    UINT32          CollectionStackPtr;
} HID_PARSER;

EFI_STATUS
ParseMouseReportDescriptor (
    IN  USB_MOUSE_DEV   *UsbMouse,
    IN  UINT8           *ReportDescriptor,
    IN  UINTN           ReportSize
);

#endif
