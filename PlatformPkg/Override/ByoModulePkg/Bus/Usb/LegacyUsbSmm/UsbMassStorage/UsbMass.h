/** @file
  Definition of USB Mass Storage Class and its value, USB Mass Transport Protocol, 
  and other common definitions.

Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_USBMASS_H_
#define _EFI_USBMASS_H_

#include <Protocol/SmmCpu.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../UsbLib/UsbLib.h"
#include "../Include/UsbIo.h"
#include "../Include/UsbBlockIo.h"

#define USB_IS_IN_ENDPOINT(EndPointAddr)      (((EndPointAddr) & 0x80) == 0x80)
#define USB_IS_OUT_ENDPOINT(EndPointAddr)     (((EndPointAddr) & 0x80) == 0)
#define USB_IS_BULK_ENDPOINT(Attribute)       (((Attribute) & 0x03) == 0x02)
#define USB_IS_INTERRUPT_ENDPOINT(Attribute)  (((Attribute) & 0x03) == 0x03)
#define USB_IS_ERROR(Result, Error)           (((Result) & (Error)) != 0)


enum {
    //
    // Usb mass storage class code
    //
    USB_MASS_STORE_CLASS    = 0x08,

    //
    // Usb mass storage subclass code, specify the command set used.
    //
    USB_MASS_STORE_RBC      = 0x01, // Reduced Block Commands
    USB_MASS_STORE_8020I    = 0x02, // SFF-8020i, typically a CD/DVD device
    USB_MASS_STORE_QIC      = 0x03, // Typically a tape device
    USB_MASS_STORE_UFI      = 0x04, // Typically a floppy disk driver device
    USB_MASS_STORE_8070I    = 0x05, // SFF-8070i, typically a floppy disk driver device.
    USB_MASS_STORE_SCSI     = 0x06, // SCSI transparent command set

    //
    // Usb mass storage protocol code, specify the transport protocol
    //
    USB_MASS_STORE_CBI0     = 0x00, // CBI protocol with command completion interrupt
    USB_MASS_STORE_CBI1     = 0x01, // CBI protocol without command completion interrupt
    USB_MASS_STORE_BOT      = 0x50, // Bulk-Only Transport

    USB_MASS_1_MILLISECOND  = 1000,
    USB_MASS_1_SECOND       = 1000 * USB_MASS_1_MILLISECOND,

    USB_MASS_CMD_SUCCESS    = 0,
    USB_MASS_CMD_FAIL,
    USB_MASS_CMD_PERSISTENT,
};
/**
  Initializes USB transport protocol.

  This function initializes the USB mass storage class transport protocol.
  It will save its context in the Context if Context isn't NULL.

  @param  UsbIo                 The USB I/O Protocol instance
  @param  Context               The buffer to save the context to

  @retval EFI_SUCCESS           The device is successfully initialized.
  @retval EFI_UNSUPPORTED       The transport protocol doesn't support the device.
  @retval Other                 The USB transport initialization fails.

**/
typedef
EFI_STATUS
(*USB_MASS_INIT_TRANSPORT) (
    IN  EFI_USB_IO_PROTOCOL     *Usb,
    OUT VOID                    **Context    OPTIONAL
);

/**
  Execute USB mass storage command through the transport protocol.

  @param  Context               The USB Transport Protocol.
  @param  Cmd                   The command to transfer to device
  @param  CmdLen                The length of the command
  @param  DataDir               The direction of data transfer
  @param  Data                  The buffer to hold the data
  @param  DataLen               The length of the buffer
  @param  Lun                   Should be 0, this field for bot only
  @param  Timeout               The time to wait
  @param  CmdStatus             The result of the command execution

  @retval EFI_SUCCESS           The command is executed successfully.
  @retval Other                 Failed to execute the command

**/
typedef
EFI_STATUS
(*USB_MASS_EXEC_COMMAND) (
    IN  VOID                    *Context,
    IN  VOID                    *Cmd,
    IN  UINT8                   CmdLen,
    IN  EFI_USB_DATA_DIRECTION  DataDir,
    IN  VOID                    *Data,
    IN  UINT32                  DataLen,
    IN  UINT8                   Lun,
    IN  UINT32                  Timeout,
    OUT UINT32                  *CmdStatus
);

/**
  Reset the USB mass storage device by Transport protocol.

  @param  Context               The USB Transport Protocol
  @param  ExtendedVerification  The flag controlling the rule of reset.
                                Not used here.

  @retval EFI_SUCCESS           The device is reset.
  @retval Others                Failed to reset the device.

**/
typedef
EFI_STATUS
(*USB_MASS_RESET) (
    IN  VOID                    *Context,
    IN  BOOLEAN                 ExtendedVerification
);

/**
  Get the max LUN (Logical Unit Number) of USB mass storage device.

  @param  Context          The context of the transport protocol.
  @param  MaxLun           Return pointer to the max number of LUN. (e.g. MaxLun=1 means LUN0 and
                           LUN1 in all.)

  @retval EFI_SUCCESS      Max LUN is got successfully.
  @retval Others           Fail to execute this request.

**/
typedef
EFI_STATUS
(*USB_MASS_GET_MAX_LUN) (
    IN  VOID                    *Context,
    IN  UINT8                   *MaxLun
);

/**
  Clean up the transport protocol's resource.

  @param  Context               The instance of transport protocol.

  @retval EFI_SUCCESS           The resource is cleaned up.

**/
typedef
EFI_STATUS
(*USB_MASS_FINI) (
    IN  VOID                    *Context
);

//
// This structure contains information necessary to select the
// proper transport protocol. The mass storage class defines
// two transport protocols. One is the CBI, and the other is BOT.
// CBI is being obseleted. The design is made modular by this
// structure so that the CBI protocol can be easily removed when
// it is no longer necessary.
//
typedef struct {
    UINT8                   Protocol;
    USB_MASS_INIT_TRANSPORT Init;        // Initialize the mass storage transport protocol
    USB_MASS_EXEC_COMMAND   ExecCommand; // Transport command to the device then get result
    USB_MASS_RESET          Reset;       // Reset the device
    USB_MASS_GET_MAX_LUN    GetMaxLun;   // Get max lun, only for bot;
    USB_MASS_FINI           Fini;        // Clean up the resources.
} USB_MASS_TRANSPORT;


EFI_STATUS
UsbClearEndpointStall (
    IN EFI_USB_IO_PROTOCOL      *UsbIo,
    IN UINT8                    EndpointAddress
);

extern UINTN mUsbMscInfo;
extern UINTN mUsbMscError;
#endif
