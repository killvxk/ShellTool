/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  UsbEnumer.c

Abstract:
  Implements related functions to enumerate USB devices

Revision History:
Bug 2241:   Fixed the build failure issue with legacy USB SMM driver
TIME:       2011-6-3
$AUTHOR:    Chen Daolin
$REVIEWERS:
$SCOPE:     All of the platforms having legacy USB support
$TECHNICAL: UsbEnumer.c is updated to use "CopyMem()" to replace the
            simple "=", when assign the content of one structure variable
            to another one. The latter way could cause complier to
            implicitly use "memcpy" to copy the content of structure,
            and then cause build failure.
$END--------------------------------------------------------------------

**/

#include "UsbBus.h"
#include "../UsbKb/UsbKeyboard.h"
#include "../UsbMassStorage/UsbMassimpl.h"
#include "../UsbMouse/UsbMouse.h"
#include "../UsbLib/UsbUtility.h"
#include <Library/ReportStatusCodeLib.h>
#include <Protocol/UsbPolicy.h>
#include "../Ehci/Ehci.h"  //For BUG

#define PROTOCOL_MOUSE          2
#define USB_MASS_STORE_CLASS    8

BOOLEAN        Childexist = FALSE;

/**
  Return the endpoint descriptor in this interface.

  @param  UsbIf                 The interface to search in.
  @param  EpAddr                The address of the endpoint to return.

  @return The endpoint descriptor or NULL.

**/
USB_ENDPOINT_DESC *
UsbGetEndpointDesc (
  IN USB_INTERFACE        *UsbIf,
  IN UINT8                EpAddr
  )
{
  USB_ENDPOINT_DESC       *EpDesc;
  UINT8                   Index;
  UINT8                   NumEndpoints;
  
  NumEndpoints = UsbIf->IfSetting->Desc.NumEndpoints;
  
  for (Index = 0; Index < NumEndpoints; Index++) {
    EpDesc = UsbIf->IfSetting->Endpoints[Index];

    if (EpDesc->Desc.EndpointAddress == EpAddr) {
      return EpDesc;
    }
  }

  return NULL;
}


/**
  Free the resource used by USB interface.

  @param  UsbIf                 The USB interface to free.

**/
VOID
UsbFreeInterface (
  IN USB_INTERFACE        *UsbIf
  )
{
    if (UsbIf->DevicePath != NULL) {
        FreePool (UsbIf->DevicePath);
    }

    FreePool (UsbIf);
}


/**
  Create an interface for the descriptor IfDesc. Each
  device's configuration can have several interfaces.

  @param  Device                The device has the interface descriptor.
  @param  IfDesc                The interface descriptor.

  @return The created USB interface for the descriptor, or NULL.

**/
USB_INTERFACE *
UsbCreateInterface (
  IN USB_DEVICE           *Device,
  IN USB_INTERFACE_DESC   *IfDesc
  )
{
    USB_DEVICE_PATH         UsbNode;
    USB_INTERFACE           *UsbIf;
    USB_INTERFACE           *HubIf;

    UsbIf = AllocateZeroPool(sizeof (USB_INTERFACE));
    if (UsbIf == NULL) {
        USB_DEBUG((EFI_D_ERROR, "UsbCreateInterface: failed to create device's interface\n"));
        return NULL;
    }

    UsbIf->Signature  = USB_INTERFACE_SIGNATURE;
    UsbIf->Device     = Device;
    UsbIf->IfDesc     = IfDesc;
    UsbIf->IfSetting  = IfDesc->Settings[IfDesc->ActiveIndex];
    CopyMem (&UsbIf->UsbIo, &mUsbIoProtocol, sizeof (mUsbIoProtocol));

    if (UsbIf->IfSetting->Desc.InterfaceClass == CLASS_HID &&
            UsbIf->IfSetting->Desc.InterfaceSubClass == SUBCLASS_BOOT &&
            UsbIf->IfSetting->Desc.InterfaceProtocol == PROTOCOL_KEYBOARD) {
        UsbIf->Type = UDT_KEYBOARD;
    }
    else if (UsbIf->IfSetting->Desc.InterfaceClass == CLASS_HID &&
             UsbIf->IfSetting->Desc.InterfaceSubClass == SUBCLASS_BOOT &&
             UsbIf->IfSetting->Desc.InterfaceProtocol == PROTOCOL_MOUSE) {
        UsbIf->Type = UDT_MOUSE;
    }
    else if ((UsbIf->IfSetting->Desc.InterfaceClass == USB_HUB_CLASS_CODE) &&
             (UsbIf->IfSetting->Desc.InterfaceSubClass == USB_HUB_SUBCLASS_CODE)) {
        UsbIf->Type = UDT_HUB;
    }
    else if ((UsbIf->IfSetting->Desc.InterfaceClass == USB_MASS_STORE_CLASS)) {
        UsbIf->Type = UDT_MASS_STORAGE;
    }

    //
    // Install protocols for USBIO and device path
    //
    UsbNode.Header.Type       = MESSAGING_DEVICE_PATH;
    UsbNode.Header.SubType    = MSG_USB_DP;
    UsbNode.ParentPortNumber  = Device->ParentPort;
    UsbNode.InterfaceNumber   = UsbIf->IfSetting->Desc.InterfaceNumber;

    SetDevicePathNodeLength (&UsbNode.Header, sizeof (UsbNode));

    HubIf = Device->ParentIf;
    ASSERT (HubIf != NULL);

    UsbIf->DevicePath = AppendDevicePathNode (HubIf->DevicePath, &UsbNode.Header);

    if (UsbIf->DevicePath == NULL) {
        USB_DEBUG((EFI_D_ERROR, "UsbCreateInterface: failed to create device path\n"));
        FreePool (UsbIf);
        return NULL;
    }

    return UsbIf;
}


/**
  Free the resource used by this USB device.

  @param  Device                The USB device to free.

**/
VOID
UsbFreeDevice (
  IN USB_DEVICE           *Device
  )
{
    if (Device->DevDesc != NULL) {
        UsbFreeDevDesc (Device->DevDesc);
    }

    FreePool (Device);
}


/**
  Create a device which is on the parent's ParentPort port.

  @param  ParentIf              The parent HUB interface.
  @param  ParentPort            The port on the HUB this device is connected to.

  @return Created USB device, Or NULL.

**/
USB_DEVICE *
UsbCreateDevice (
  IN USB_INTERFACE        *ParentIf,
  IN UINT8                ParentPort
  )
{
  USB_DEVICE              *Device;

  ASSERT (ParentIf != NULL);

  Device = AllocateZeroPool (sizeof (USB_DEVICE));

  if (Device == NULL) {
    return NULL;
  }

  Device->Bus         = ParentIf->Device->Bus;
  Device->MaxPacket0  = 8;
  Device->ParentAddr  = ParentIf->Device->Address;
  Device->ParentIf    = ParentIf;
  Device->ParentPort  = ParentPort;
  Device->Tier        = (UINT8)(ParentIf->Device->Tier + 1);
  return Device;
}


/**
  Connect the USB interface with its driver. EFI USB bus will
  create a USB interface for each separate interface descriptor.

  @param  UsbIf             The interface to connect driver to.

  @return EFI_SUCCESS       Interface is managed by some driver.
  @return Others            Failed to locate a driver for this interface.

**/
EFI_STATUS
UsbConnectDriver (
  IN USB_INTERFACE        *UsbIf
  )
{
    EFI_STATUS              Status;
    USB_CFG                 *UsbCfg;

    Status = EFI_UNSUPPORTED;

    //
    // Get USB Configuration Information
    //
    GET_USB_CFG(UsbCfg);
    if (UsbIf->Device->HCSpeed == EFI_USB_SPEED_SUPER) {
        UsbCfg->UsbDeviceInfor.LegacyMSEnable = LEGACY_MS_EN;
    }
    //
    // Hub is maintained by the USB bus driver. Otherwise try to
    // connect drivers with this interface
    //
    switch (UsbIf->Type) {
    case UDT_HUB:
        USB_DEBUG((EFI_D_INFO, "UsbConnectDriver: Found a hub device\n"));
        Status = mUsbHubApi.Init (UsbIf);
        break;

    case UDT_KEYBOARD:
        if (UsbCfg->UsbDeviceInfor.LegacyKBEnable == LEGACY_KB_EN) {
           USB_DEBUG((EFI_D_INFO, "UsbConnectDriver: Connect keyboard driver\n"));
           Status = UsbConnectKeyboardDriver (UsbIf);
        } else {
            Status = EFI_NOT_READY;
        }
        break;

    case UDT_MOUSE:
        if (UsbCfg->UsbDeviceInfor.LegacyMSEnable== LEGACY_MS_EN) {
            USB_DEBUG((EFI_D_INFO, "UsbConnectDriver: found a mouse device\n"));
            Status = UsbConnectMouseDriver (UsbIf);
        } else {
            Status = EFI_NOT_READY;
        }
        break;

    case UDT_MASS_STORAGE:
        USB_DEBUG((EFI_D_INFO, "UsbConnectDriver: found a massstorage device\n"));
        Status = UsbConnectMassDriver (UsbIf);
        break;

    default:
        break;
    }

    USB_DEBUG((EFI_D_ERROR, "UsbConnectDriver: Status is %r\n", Status));
    UsbIf->IsManaged = (BOOLEAN)!EFI_ERROR (Status);

    return Status;
}


/**
  Select an alternate setting for the interface.
  Each interface can have several mutually exclusive
  settings. Only one setting is active. It will
  also reset its endpoints' toggle to zero.

  @param  IfDesc                The interface descriptor to set.
  @param  Alternate             The alternate setting number to locate.

  @retval EFI_NOT_FOUND         There is no setting with this alternate index.
  @retval EFI_SUCCESS           The interface is set to Alternate setting.

**/
EFI_STATUS
UsbSelectSetting (
  IN USB_INTERFACE_DESC   *IfDesc,
  IN UINT8                Alternate
  )
{
  USB_INTERFACE_SETTING   *Setting;
  UINT8                   Index;

  //
  // Locate the active alternate setting
  //
  Setting = NULL;

  for (Index = 0; Index < IfDesc->NumOfSetting; Index++) {
    ASSERT (Index < USB_MAX_INTERFACE_SETTING);
    Setting = IfDesc->Settings[Index];

    if (Setting->Desc.AlternateSetting == Alternate) {
      break;
    }
  }

  if (Index == IfDesc->NumOfSetting) {
    return EFI_NOT_FOUND;
  }

  IfDesc->ActiveIndex = Index;

  ASSERT (Setting != NULL);
  USB_DEBUG ((EFI_D_INFO, "UsbSelectSetting: setting %d selected for interface %d\n",
              Alternate, Setting->Desc.InterfaceNumber));

  //
  // Reset the endpoint toggle to zero
  //
  for (Index = 0; Index < Setting->Desc.NumEndpoints; Index++) {
    Setting->Endpoints[Index]->Toggle = 0;
  }

  return EFI_SUCCESS;
}


/**
  Select a new configuration for the device. Each
  device may support several configurations.

  @param  Device                The device to select configuration.
  @param  ConfigValue           The index of the configuration ( != 0).

  @retval EFI_NOT_FOUND         There is no configuration with the index.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource.
  @retval EFI_SUCCESS           The configuration is selected.

**/
EFI_STATUS
UsbSelectConfig (
  IN USB_DEVICE           *Device,
  IN UINT8                ConfigValue
  )
{
  USB_DEVICE_DESC         *DevDesc;
  USB_CONFIG_DESC         *ConfigDesc;
  USB_INTERFACE_DESC      *IfDesc;
  USB_INTERFACE           *UsbIf;
  EFI_STATUS              Status;
  UINT8                   Index;

  //
  // Locate the active config, then set the device's pointer
  //
  DevDesc     = Device->DevDesc;
  ConfigDesc  = NULL;

  for (Index = 0; Index < DevDesc->Desc.NumConfigurations; Index++) {
    ConfigDesc = DevDesc->Configs[Index];

    if (ConfigDesc->Desc.ConfigurationValue == ConfigValue) {
      break;
    }
  }

  if (Index == DevDesc->Desc.NumConfigurations) {
    return EFI_NOT_FOUND;
  }

  Device->ActiveConfig = ConfigDesc;

  USB_DEBUG ((EFI_D_INFO, "UsbSelectConfig: config %d selected for device %d\n",
              ConfigValue, Device->Address));

  //
  // Create interfaces for each USB interface descriptor.
  //
  for (Index = 0; Index < ConfigDesc->Desc.NumInterfaces; Index++) {
    //
    // First select the default interface setting, and reset
    // the endpoint toggles to zero for its endpoints.
    //
    IfDesc = ConfigDesc->Interfaces[Index];
    UsbSelectSetting (IfDesc, IfDesc->Settings[0]->Desc.AlternateSetting);

    //
    // Create a USB_INTERFACE and install USB_IO and other protocols
    //
    UsbIf = UsbCreateInterface (Device, ConfigDesc->Interfaces[Index]);

    if (UsbIf == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ASSERT (Index < USB_MAX_INTERFACE);
    Device->Interfaces[Index] = UsbIf;
    CreateLegacyUsbInfNode (UsbIf);
    //
    // Connect the device to drivers, if it failed, ignore
    // the error. Don't let the unsupported interfaces to block
    // the supported interfaces.
    //
    Status = UsbConnectDriver (UsbIf);

    if (EFI_ERROR (Status)) {
      USB_DEBUG ((EFI_D_ERROR, "UsbSelectConfig: failed to connect driver %r, ignored\n", Status));
    }
  }

  Device->NumOfInterface = Index;

  return EFI_SUCCESS;
}


/**
  Disconnect the USB interface with its driver.

  @param  UsbIf                 The interface to disconnect driver from.

**/
VOID
UsbDisconnectDriver (
  IN USB_INTERFACE        *UsbIf
  )
{
    //
    // Release the hub if it's a hub controller, otherwise
    // disconnect the driver if it is managed by other drivers.
    //
    if (!UsbIf->IsManaged)
        return;

    switch (UsbIf->Type) {
    case UDT_HUB:
        USB_DEBUG((EFI_D_INFO, "UsbDisconnectDriver: Remove a hub device\n"));
        UsbIf->HubApi->Release (UsbIf);
        break;

    case UDT_KEYBOARD:
        USB_DEBUG((EFI_D_INFO, "UsbDisconnectDriver: Disconnect keyboard device\n"));;
        UsbDisconnectKeyboardDriver (UsbIf);
        break;

    case UDT_MOUSE:
        USB_DEBUG((EFI_D_INFO, "UsbDisconnectDriver: Disconnect mouse device\n"));
        UsbDisconnectMouseDriver (UsbIf);
        break;

    case UDT_MASS_STORAGE:
        USB_DEBUG((EFI_D_INFO, "UsbDisconnectDriver: Disconnect mass storage driver\n"));
        UsbDisconnectMassDriver (UsbIf);
        break;

    default:
        break;
    }
}


/**
  Remove the current device configuration.

  @param  Device                The USB device to remove configuration from.

**/
VOID
UsbRemoveConfig (
  IN USB_DEVICE           *Device
  )
{
  USB_INTERFACE           *UsbIf;
  UINTN                   Index;

  //
  // Remove each interface of the device
  //
  for (Index = 0; Index < Device->NumOfInterface; Index++) {    
    ASSERT (Index < USB_MAX_INTERFACE);
    UsbIf = Device->Interfaces[Index];

    if (UsbIf == NULL) {
      continue;
    }

    UpdateLegacyUsbInfNode (UsbIf, EMPTY_INTERFACE);
    USB_DEBUG((EFI_D_INFO, "UsbDisconnectDriver\n"));
    UsbDisconnectDriver (UsbIf);
    UsbFreeInterface (UsbIf);
    Device->Interfaces[Index] = NULL;
  }

  Device->ActiveConfig    = NULL;
  Device->NumOfInterface  = 0;
}


/**
  Remove the device and all its children from the bus.

  @param  Device                The device to remove.

  @retval EFI_SUCCESS           The device is removed.

**/
EFI_STATUS
UsbRemoveDevice (
  IN USB_DEVICE           *Device
  )
{
  USB_BUS                 *Bus;
  USB_DEVICE              *Child;
  EFI_STATUS              Status;
  UINTN                   Index;

  Bus = Device->Bus;

  //
  // Remove all the devices on its downstream ports. Search from devices[1].
  // Devices[0] is the root hub.
  //
  for (Index = 1; Index < USB_MAX_DEVICES; Index++) {
    Child = Bus->Devices[Index];

    if ((Child == NULL) || (Child->ParentAddr != Device->Address)) {
      continue;
    }

    Status = UsbRemoveDevice (Child);

    if (EFI_ERROR (Status)) {
      USB_DEBUG ((EFI_D_ERROR, "UsbRemoveDevice: failed to remove child, ignore error\n"));
      Bus->Devices[Index] = NULL;
    }
  }

  UsbRemoveConfig (Device);

  USB_DEBUG (( EFI_D_INFO, "UsbRemoveDevice: device %d removed\n", Device->Address));

  Bus->Devices[Device->Address] = NULL;
  UsbFreeDevice (Device);

  return EFI_SUCCESS;
}


/**
  Find the child device on the hub's port.

  @param  HubIf                 The hub interface.
  @param  Port                  The port of the hub this child is connected to.

  @return The device on the hub's port, or NULL if there is none.

**/
USB_DEVICE *
UsbFindChild (
  IN USB_INTERFACE        *HubIf,
  IN UINT8                Port
  )
{
  USB_DEVICE              *Device;
  USB_BUS                 *Bus;
  UINTN                   Index;

  Bus = HubIf->Device->Bus;

  //
  // Start checking from device 1, device 0 is the root hub
  //
  for (Index = 1; Index < USB_MAX_DEVICES; Index++) {
    Device = Bus->Devices[Index];

    if ((Device != NULL) && (Device->ParentAddr == HubIf->Device->Address) &&
        (Device->ParentPort == Port)) {

      return Device;
    }
  }

  return NULL;
}


/**
  Enumerate and configure the new device on the port of this HUB interface.

  @param  HubIf                 The HUB that has the device connected.
  @param  Port                  The port index of the hub (started with zero).

  @retval EFI_SUCCESS           The device is enumerated (added or removed).
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the device.
  @retval Others                Failed to enumerate the device.

**/
EFI_STATUS
UsbEnumerateNewDev (
  IN USB_INTERFACE        *HubIf,
  IN UINT8                Port
  )
{
  USB_BUS                 *Bus;
  USB_HUB_API             *HubApi;
  USB_DEVICE              *Child;
  USB_DEVICE              *Parent;
  EFI_USB_PORT_STATUS     PortState;
  UINTN                   Address;
  UINT8                   Config;
  EFI_STATUS              Status;
  USB_BUS                *BusInstance;
  USB2_HC_DEV            *Ehc;
  UINT32                  UsbSts;
  UINT32                  UsbPortSts;

  Parent  = HubIf->Device;
  Bus     = Parent->Bus;
  HubApi  = HubIf->HubApi;  
  Address = USB_MAX_DEVICES;

  Stall (USB_WAIT_PORT_STABLE_STALL);
  
  //
  // Hub resets the device for at least 10 milliseconds.
  // Host learns device speed. If device is of low/full speed
  // and the hub is a EHCI root hub, ResetPort will release
  // the device to its companion UHCI and return an error.
  //
  Status = HubApi->ResetPort (HubIf, Port);
  
  if(Status == EFI_NOT_FOUND){ //For BUG
    BusInstance = HubIf->Device->Bus;
    Ehc =  EHC_FROM_THIS(BusInstance->Usb2Hc);
    UsbSts = EhcReadOpReg(Ehc, EHC_USBSTS_OFFSET);
    UsbPortSts = EhcReadOpReg(Ehc, EHC_PORT_STAT_OFFSET+Port*4);
    if((UsbPortSts & PORTSC_OWNER) && (UsbSts & USBSTS_PORT_CHANGE)){
      EhcWriteOpReg (Ehc, EHC_USBSTS_OFFSET, USBSTS_PORT_CHANGE);
    }
  }

  if (EFI_ERROR (Status)) {
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumerateNewDev: failed to reset port %d - %r\n", Port, Status));

    return Status;
  }

  USB_DEBUG (( EFI_D_INFO, "UsbEnumerateNewDev: hub port %d is reset\n", Port));

  Child = UsbCreateDevice (HubIf, Port);

  if (Child == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // OK, now identify the device speed. After reset, hub
  // fully knows the actual device speed.
  //
  Status = HubApi->GetPortStatus (HubIf, Port, &PortState);

  if (EFI_ERROR (Status)) {
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumerateNewDev: failed to get speed of port %d\n", Port));
    goto ON_ERROR;
  }

  if (!USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_CONNECTION)) {
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumerateNewDev: No device present at port %d\n", Port));
    goto ON_ERROR;
  } else if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_SUPER_SPEED)){
    Child->Speed      = EFI_USB_SPEED_SUPER;
    Child->MaxPacket0 = 512;
  } else if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_HIGH_SPEED)) {
    Child->Speed      = EFI_USB_SPEED_HIGH;
    Child->MaxPacket0 = 64;
  } else if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_LOW_SPEED)) {
    Child->Speed      = EFI_USB_SPEED_LOW;
    Child->MaxPacket0 = 8;
  } else {
    Child->Speed      = EFI_USB_SPEED_FULL;
    Child->MaxPacket0 = 8;
  }

  USB_DEBUG (( EFI_D_INFO, "UsbEnumerateNewDev: device is of %d speed\n", Child->Speed));

 
  if (((Child->Speed == EFI_USB_SPEED_LOW) || (Child->Speed == EFI_USB_SPEED_FULL)) &&
       (Parent->Speed == EFI_USB_SPEED_HIGH)) {
     //
     // If the child is a low or full speed device, it is necessary to
     // set the transaction translator. Port TT is 1-based.
     // This is quite simple:
     //  1. if parent is of high speed, then parent is our translator
     //  2. otherwise use parent's translator.
     //
     Child->Translator.TranslatorHubAddress  = Parent->Address;
     Child->Translator.TranslatorPortNumber  = (UINT8) (Port + 1);
   } else {
     Child->Translator = Parent->Translator;
   }
   USB_DEBUG (( EFI_D_INFO, "UsbEnumerateNewDev: device uses translator (%d, %d)\n",
            Child->Translator.TranslatorHubAddress,
            Child->Translator.TranslatorPortNumber));
 
  //
  // After port is reset, hub establishes a signal path between
  // the device and host (DEFALUT state). Device's registers are
  // reset, use default address 0 (host enumerates one device at
  // a time) , and ready to respond to control transfer at EP 0.
  //

  //
  // Host assigns an address to the device. Device completes the
  // status stage with default address, then switches to new address.
  // ADDRESS state. Address zero is reserved for root hub.
  //
  for (Address = 1; Address < USB_MAX_DEVICES; Address++) {
    if (Bus->Devices[Address] == NULL) {
      break;
    }
  }

  if (Address >= USB_MAX_DEVICES) {
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumerateNewDev: address pool is full for port %d\n", Port));

    Status = EFI_ACCESS_DENIED;
    goto ON_ERROR;
  }
  Stall (USB_SET_ROOT_PORT_RESET_STALL); // stall 50ms for some USB disks hot plug issue
  Status                = UsbSetAddress (Child, (UINT8)Address);
  Child->Address        = (UINT8)Address;
  Bus->Devices[Address] = Child;

  if (EFI_ERROR (Status)) {
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumerateNewDev: failed to set device address - %r\n", Status));
    goto ON_ERROR;
  }

  Stall (USB_SET_DEVICE_ADDRESS_STALL);

  USB_DEBUG ((EFI_D_INFO, "UsbEnumerateNewDev: device is now ADDRESSED at %d\n", Address));

  //
  // Host sends a Get_Descriptor request to learn the max packet
  // size of default pipe (only part of the device's descriptor).
  //
  Status = UsbGetMaxPacketSize0 (Child);

  if (EFI_ERROR (Status)) {
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumerateNewDev: failed to get max packet for EP 0 - %r\n", Status));
    goto ON_ERROR;
  }

  USB_DEBUG (( EFI_D_INFO, "UsbEnumerateNewDev: max packet size for EP 0 is %d\n", Child->MaxPacket0));

  //
  // Host learns about the device's abilities by requesting device's
  // entire descriptions.
  //
  Status = UsbBuildDescTable (Child);

  if (EFI_ERROR (Status)) {
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumerateNewDev: failed to build descriptor table - %r\n", Status));
    goto ON_ERROR;
  }

  //
  // Select a default configuration: UEFI must set the configuration
  // before the driver can connect to the device.
  //
  Config = Child->DevDesc->Configs[0]->Desc.ConfigurationValue;
  Status = UsbSetConfig (Child, Config);

  if (EFI_ERROR (Status)) {
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumerateNewDev: failed to set configure %d - %r\n", Config, Status));
    goto ON_ERROR;
  }

  USB_DEBUG (( EFI_D_INFO, "UsbEnumerateNewDev: device %d is now in CONFIGED state\n", Address));

  //
  // Host assigns and loads a device driver.
  //
  Status = UsbSelectConfig (Child, Config);

  if (EFI_ERROR (Status)) {
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumerateNewDev: failed to create interfaces - %r\n", Status));
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (Address != USB_MAX_DEVICES) {
    Bus->Devices[Address] = NULL;
  }

  if (Child != NULL) {
    UsbFreeDevice (Child);
  }

  return Status;
}


/**
  Process the events on the port.

  @param  HubIf                 The HUB that has the device connected.
  @param  Port                  The port index of the hub (started with zero).

  @retval EFI_SUCCESS           The device is enumerated (added or removed).
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the device.
  @retval Others                Failed to enumerate the device.

**/
EFI_STATUS
UsbEnumeratePort (
  IN USB_INTERFACE        *HubIf,
  IN UINT8                Port
  )
{
  USB_HUB_API             *HubApi;
  USB_DEVICE              *Child;
  EFI_USB_PORT_STATUS     PortState;
  EFI_STATUS              Status;

  Child   = NULL;
  HubApi  = HubIf->HubApi;

  //
  // Host learns of the new device by polling the hub for port changes.
  //
  Childexist = FALSE;
  Child = UsbFindChild (HubIf, Port);
  if (Child != NULL) {
    Childexist = TRUE;
  }
  Status = HubApi->GetPortStatus (HubIf, Port, &PortState);
  Childexist = FALSE;

  if (EFI_ERROR (Status)) {
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumeratePort: failed to get state of port %d\n", Port));
    return Status;
  }

  //
  // Only handle connection/enable/overcurrent/reset change.
  // Usb super speed hub may report other changes, such as warm reset change. Ignore them.
  //
  if ((PortState.PortChangeStatus & (USB_PORT_STAT_C_CONNECTION | USB_PORT_STAT_C_ENABLE | USB_PORT_STAT_C_OVERCURRENT | USB_PORT_STAT_C_RESET)) == 0) {
    return EFI_SUCCESS;
  }

  USB_DEBUG (( EFI_D_INFO, "UsbEnumeratePort: port %d state - %02x, change - %02x on %p\n",
              Port, PortState.PortStatus, PortState.PortChangeStatus, HubIf));

  //
  // This driver only process two kinds of events now: over current and
  // connect/disconnect. Other three events are: ENABLE, SUSPEND, RESET.
  // ENABLE/RESET is used to reset port. SUSPEND isn't supported.
  //
  
  if (USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_OVERCURRENT)) {     

    if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_OVERCURRENT)) {
      //
      // Case1:
      //   Both OverCurrent and OverCurrentChange set, means over current occurs, 
      //   which probably is caused by short circuit. It has to wait system hardware
      //   to perform recovery.
      //
      USB_DEBUG (( EFI_D_ERROR, "UsbEnumeratePort: Critical Over Current\n", Port));
      return EFI_DEVICE_ERROR;
      
    } 
    //
    // Case2:
    //   Only OverCurrentChange set, means system has been recoveried from 
    //   over current. As a result, all ports are nearly power-off, so
    //   it's necessary to detach and enumerate all ports again. 
    //
    USB_DEBUG (( EFI_D_ERROR, "UsbEnumeratePort: 2.0 device Recovery Over Current\n", Port)); 
  }

  if (USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_ENABLE)) {  
    //
    // Case3:
    //   1.1 roothub port reg doesn't reflect over-current state, while its counterpart
    //   on 2.0 roothub does. When over-current has influence on 1.1 device, the port 
    //   would be disabled, so it's also necessary to detach and enumerate again.
    //
    USB_DEBUG (( EFI_D_ERROR, "UsbEnumeratePort: 1.1 device Recovery Over Current\n", Port));
  }
  
  if (USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_CONNECTION)) {
    //
    // Case4:
    //   Device connected or disconnected normally. 
    //
    USB_DEBUG ((EFI_D_ERROR, "UsbEnumeratePort: Device Connect/Disconnect Normally\n", Port));
  }

  // 
  // Following as the above cases, it's safety to remove and create again.
  //
  //Child = UsbFindChild (HubIf, Port);
  
  if (Child != NULL) {
    USB_DEBUG (( EFI_D_INFO, "UsbEnumeratePort: device at port %d removed from root hub %p\n", Port, HubIf));
    UsbRemoveDevice (Child);
  }
  
  if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_CONNECTION)) {
    //
    // Now, new device connected, enumerate and configure the device 
    //
    USB_DEBUG (( EFI_D_INFO, "UsbEnumeratePort: new device connected at port %d\n", Port));
    Status = UsbEnumerateNewDev (HubIf, Port);
  
  } else {
    USB_DEBUG (( EFI_D_INFO, "UsbEnumeratePort: device disconnected event on port %d\n", Port));
  }
  
  HubApi->ClearPortChange (HubIf, Port);
  return Status;
}


/**
  Enumerate all the changed hub ports.

  @param  Event                 The event that is triggered.
  @param  Context               The context to the event.

**/
VOID
EFIAPI
UsbHubEnumeration (
  IN  USB_INTERFACE           *HubIf
  )
{

  UINT8                   Byte;
  UINT8                   Bit;
  UINT8                   Index;

  if (HubIf->ChangeMap == NULL) {
    return ;
  }

  //
  // HUB starts its port index with 1.
  //
  Byte  = 0;
  Bit   = 1;

  for (Index = 0; Index < HubIf->NumOfPort; Index++) {
    if (USB_BIT_IS_SET (HubIf->ChangeMap[Byte], USB_BIT (Bit))) {
      UsbEnumeratePort (HubIf, Index);
    }

    USB_NEXT_BIT (Byte, Bit);
  }

  UsbHubAckHubStatus (HubIf->Device);

  FreePool (HubIf->ChangeMap);
  HubIf->ChangeMap = NULL;
  return ;
}


/**
  Enumerate all the changed hub ports.

  @param  Event                 The event that is triggered.
  @param  Context               The context to the event.

**/
VOID
EFIAPI
UsbRootHubEnumeration (
  IN  USB_INTERFACE           *RootHub
)
{
  UINT8                   Index;


  for (Index = 0; Index < RootHub->NumOfPort; Index++) {
    UsbEnumeratePort (RootHub, Index);
  }
}
