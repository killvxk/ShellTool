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
  UsbBus.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_USB_BUS_H_
#define _EFI_USB_BUS_H_

#include <PiSmm.h>
#include <IndustryStandard/pci22.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/LegacyUsbInf.h>
#include <Protocol/SmmBase2.h>

#include "../Include/Usb.h"
#include "../Include/UsbIo.h"
#include "../UsbLib/UsbMem.h"
#include "../Include/UsbHostController.h"
#include "../Usblib/SmmPciIo.h"

typedef struct _USB_DEVICE        USB_DEVICE;
typedef struct _USB_INTERFACE     USB_INTERFACE;
typedef struct _USB_BUS           USB_BUS;
typedef struct _USB_HUB_API       USB_HUB_API;

#include "UsbDesc.h"
#include "UsbHub.h"
#include "UsbEnumer.h"
#include "../UsbLib/UsbUtility.h"

#define   USB_DEBUG(a)              DEBUG(a)
#define   PCI_CLASS_OFFSET          0x08
#define   PCI_USB_UHCI_CLASS        0x0C030000
#define   PCI_USB_OHCI_CLASS        0x0C031000
#define   PCI_USB_EHCI_CLASS        0x0C032000
#define   PCI_USB_XHCI_CLASS        0x0C033000
#define   MAX_PCI_BUSS              40
#define   MAX_PCI_DEVICES           32
#define   MAX_PCI_FUNCTIONS         8
#define   MAX_LUN_COUNT             8

extern EFI_USB_IO_PROTOCOL          mUsbIoProtocol;

extern BOOLEAN        Childexist;

//
// Add for usb time tue
//
BOOLEAN         mUsbTimeTue;
BOOLEAN         mInternelHubExist;
UINTN           mEnumWaitPortStableStall;

typedef struct _USB_PCI_DEVICE {
    UINT8   Type;
    EFI_PCI_FUNCTION_ADDRESS PciAddress;
} USB_PCI_DEVICE;

enum {
    USB_MAX_LANG_ID           = 16,
    USB_MAX_INTERFACE         = 16,
    USB_MAX_DEVICES           = 128,
    USB_MAX_CONTROLLERS       = 20,

    USB_BUS_1_MILLISECOND     = 1000,

    //
    // Roothub and hub's polling interval, set by experience,
    // The unit of roothub is 100us, means 1s as interval, and
    // the unit of hub is 1ms, means 64ms as interval.
    //
    USB_ROOTHUB_POLL_INTERVAL = 1000 * 10000U,
    USB_HUB_POLL_INTERVAL     = 64,

    //
    // Wait for port stable to work, refers to specification
    // [USB20-9.1.2]
    //
    USB_WAIT_PORT_STATUS_STABLE_STALL    = 20 * USB_BUS_1_MILLISECOND,
    USB_WAIT_PORT_STABLE_STALL           = 100 * USB_BUS_1_MILLISECOND,
    USB_WAIT_TATTDB_STALL                = 100 * USB_BUS_1_MILLISECOND,

    //
    // Wait for port statue reg change, set by experience
    //
    USB_WAIT_PORT_STS_CHANGE_STALL = 5 * USB_BUS_1_MILLISECOND,

    //
    // Wait for set device address, refers to specification
    // [USB20-9.2.6.3, it says 2ms]
    //
    USB_SET_DEVICE_ADDRESS_STALL   = 20 * USB_BUS_1_MILLISECOND,

    //
    // Wait for retry max packet size, set by experience
    //
    USB_RETRY_MAX_PACK_SIZE_STALL  = 100 * USB_BUS_1_MILLISECOND,

    //
    // Wait for hub port power-on, refers to specification
    // [USB20-11.23.2]
    //
    USB_SET_PORT_POWER_STALL       = 2 * USB_BUS_1_MILLISECOND,

    //
    // Wait for port reset, refers to specification
    // [USB20-7.1.7.5, it says 10ms for hub and 50ms for
    // root hub]
    //
    USB_SET_PORT_RESET_STALL       = 30 * USB_BUS_1_MILLISECOND,
    USB_SET_ROOT_PORT_RESET_STALL  = 50 * USB_BUS_1_MILLISECOND,

    //
    // Wait for clear roothub port reset, set by experience
    //
    USB_CLR_ROOT_PORT_RESET_STALL  = 1 * USB_BUS_1_MILLISECOND,

    //
    // Wait for set roothub port enable, set by experience
    //
    USB_SET_ROOT_PORT_ENABLE_STALL = 20 * USB_BUS_1_MILLISECOND,

    //
    // Send general device request timeout, refers to
    // specification[USB20-11.24.1]
    //
    USB_GENERAL_DEVICE_REQUEST_TIMEOUT = 500,

    //
    // Send clear feature request timeout, set by experience
    //
    USB_CLEAR_FEATURE_REQUEST_TIMEOUT  = 10 * USB_BUS_1_MILLISECOND,

    USB_INTERFACE_SIGNATURE   = SIGNATURE_32 ('U', 'S', 'B', 'I'),
    USB_BUS_SIGNATURE         = SIGNATURE_32 ('U', 'S', 'B', 'B'),
};

#define USB_BIT(a)                  ((UINTN)(1 << (a)))
#define USB_BIT_IS_SET(Data, Bit)   ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define EFI_USB_BUS_PROTOCOL_GUID \
          {0x2B2F68CC, 0x0CD2, 0x44cf, 0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75}

#define USB_INTERFACE_FROM_USBIO(a) \
          CR(a, USB_INTERFACE, UsbIo, USB_INTERFACE_SIGNATURE)

#define USB_BUS_FROM_THIS(a) \
          CR(a, USB_BUS, BusId, USB_BUS_SIGNATURE)

//
// Used to locate USB_BUS
//
typedef struct _EFI_USB_BUS_PROTOCOL {
    UINT64                    Reserved;
} EFI_USB_BUS_PROTOCOL;

enum {
    UDT_UNKNOW   = 0,
    UDT_KEYBOARD = 1,
    UDT_MOUSE    = 2,
    UDT_HUB      = 3,
    UDT_MASS_STORAGE = 4,
};

//
//
//

typedef struct _ENDPOINT_RING {

    UINTN                     EPType;
    VOID                      *TransferRing;
    VOID                      *TransferRingEnqueue;
    VOID                      *TransferRingDequeue;
    UINT32                    TransferRingPCS;
} ENDPOINT_RING;

//
// The topology string used to present usb device location
//
typedef struct _USB_DEV_TOPOLOGY {
  //
  // The tier concatenation of down stream port.
  //
  UINT32 RouteString:20;
  //
  // The root port number of the chain.
  //
  UINT32 RootPortNum:8;
  //
  // The Tier the device reside.
  //
  UINT32 TierNum:4;
} USB_DEV_TOPOLOGY;

//
// USB Device's RouteChart
//
typedef union _USB_DEV_ROUTE {
  UINT32              Dword;
  USB_DEV_TOPOLOGY    Route;
} USB_DEV_ROUTE;


//
// Stands for the real USB device. Each device may
// has several seperately working interfaces.
//
typedef struct _USB_DEVICE {
    USB_BUS                   *Bus;

    //
    // Configuration information
    //
    UINT8                     Speed;
    UINT8                     Address;
    UINT32                    MaxPacket0;

    //
    // The device's descriptors and its configuration
    //
    USB_DEVICE_DESC           *DevDesc;
    USB_CONFIG_DESC           *ActiveConfig;

    UINT16                    LangId [USB_MAX_LANG_ID];
    UINT16                    TotalLangId;

    UINT8                     NumOfInterface;
    USB_INTERFACE             *Interfaces [USB_MAX_INTERFACE];

    //
    // Parent child relationship
    //
    EFI_USB2_HC_TRANSACTION_TRANSLATOR Translator;

    UINT8                     ParentAddr;
    USB_INTERFACE             *ParentIf;
    UINT8                     ParentPort;       // Start at 0
    USB_DEV_ROUTE             RouteChart;


    //
    // XHC related stuff
    //
    UINT8                     HCSpeed;
    UINT8                     SlotID;
    VOID                      *InputContxt;
    VOID                      *OutputDevContxt;

    ENDPOINT_RING             EPRing[30];
    UINT8                     Tier;
} USB_DEVICE;

//
// Stands for different functions of USB device
//
typedef struct _USB_INTERFACE {
    UINTN                     Signature;
    USB_DEVICE                *Device;
    USB_INTERFACE_DESC        *IfDesc;
    USB_INTERFACE_SETTING     *IfSetting;

    //
    // Handles and protocols
    //
    EFI_HANDLE                Handle;
    EFI_USB_IO_PROTOCOL       UsbIo;
    EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
    UINTN                     Type;

    //
    // Hub device special data
    //
    BOOLEAN                   IsHub;
    USB_HUB_API               *HubApi;
    UINT8                     NumOfPort;
    UINTN                     PollingInterval;

    //
    // Data used only by normal hub devices
    //
    USB_ENDPOINT_DESC         *HubEp;
    UINT8                     *ChangeMap;

    //
    // Data used only by root hub to hand over device to
    // companion UHCI driver if low/full speed devices are
    // connected to EHCI.
    //
    UINT8                     MaxSpeed;

    // Device Driver specifc data
    BOOLEAN                   IsManaged;
    UINT8                     LunCount;
    VOID                      *DriverContext[MAX_LUN_COUNT];
    VOID                      *LegacyUsbIfNode;
} USB_INTERFACE;

#define NEW_INTERFACE       0
#define OLD_INTERFACE       1
#define EMPTY_INTERFACE     2

typedef struct _NATIVE_USBIF_HDR {
    UINTN                     Signature;
    EFI_HANDLE                Handle;
    EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
    UINTN                     UsbIfState;
    VOID                      *LegacyUsbIo;
    LIST_ENTRY                List;
} NATIVE_USBIF_HDR;

typedef struct _LEGACY_USBIF_NODE {
    USB_INTERFACE  *LegacyUsbIf;
    UINTN          UsbIfState;
    VOID           *NativeUsbIf;
    LIST_ENTRY     List;
} LEGACY_USBIF_NODE;

#define USB_BUS_FROM_LINK(a)  CR (a, USB_BUS, Link, USB_BUS_SIGNATURE)

enum {
    USB_UHCI = 0,
    USB_OHCI,
    USB_EHCI,
    USB_XHCI,
    USB_MAX_TYPE
};

//
// Stands for the current USB Bus
//
typedef struct _USB_BUS {
    UINTN                     Signature;
    LIST_ENTRY                Link;
    EFI_USB_BUS_PROTOCOL      BusId;

    //
    // Managed USB host controller
    //
    EFI_HANDLE                HostHandle;
    UINT8                     HostControllerType;
    EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
    EFI_USB2_HC_PROTOCOL      *Usb2Hc;
    EFI_USB_HC_PROTOCOL       *UsbHc;
    EFI_HANDLE                DispatchHandle;

    //
    // An array of device that is on the bus. Devices[0] is
    // for root hub. Device with address i is at Devices[i].
    //
    USB_DEVICE                *Devices[USB_MAX_DEVICES];

    //
    // USB Bus driver need to control the recursive connect policy of the bus, only those wanted
    // usb child device will be recursively connected.
    //
    // WantedUsbIoDPList tracks the Usb child devices which user want to recursivly fully connecte,
    // every wanted child device is stored in a item of the WantedUsbIoDPList, whose structrure is
    // DEVICE_PATH_LIST_ITEM
    //
    LIST_ENTRY             WantedUsbIoDPList;

} USB_BUS;

#define USB_US_LAND_ID   0x0409

#define DEVICE_PATH_LIST_ITEM_SIGNATURE     SIGNATURE_32('d','p','l','i')
typedef struct _DEVICE_PATH_LIST_ITEM {
    UINTN                         Signature;
    LIST_ENTRY                    Link;
    EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
} DEVICE_PATH_LIST_ITEM;

typedef struct {
    USB_CLASS_DEVICE_PATH         UsbClass;
    EFI_DEVICE_PATH_PROTOCOL      End;
} USB_CLASS_FORMAT_DEVICE_PATH;

EFI_STATUS
EFIAPI
UsbBusFreeUsbDPList (
    IN     LIST_ENTRY                                 *UsbIoDPList
);

EFI_STATUS
EFIAPI
UsbBusAddWantedUsbIoDP (
    IN EFI_USB_BUS_PROTOCOL         *UsbBusId,
    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
);

BOOLEAN
EFIAPI
UsbBusIsWantedUsbIO (
    IN USB_BUS                 *Bus,
    IN USB_INTERFACE           *UsbIf
);

EFI_STATUS
EFIAPI
UsbBusRecursivelyConnectWantedUsbIo (
    IN EFI_USB_BUS_PROTOCOL         *UsbBusId
);

VOID
CreateLegacyUsbInfNode(USB_INTERFACE *UsbIf);

VOID
UpdateLegacyUsbInfNode(USB_INTERFACE *UsbIf, UINTN State);

EFI_STATUS
EFIAPI
EndLegacyUsb(
    VOID
);

EFI_STATUS
UsbSmiDispatcher(
    IN  EFI_HANDLE                                DispatchHandle,
    IN  CONST  VOID                               *Context,
    IN  OUT    VOID                               *CommBuffer,
    IN  OUT    UINTN                              *CommBufferSize
);

EFI_STATUS
UsbConnectVirtualKbc (VOID);

EFI_STATUS
UsbDisconnectVirtualKBCDriver (VOID);

VOID
UsbVirtualKbcDispatcher (IN BOOLEAN Flag);

#endif
