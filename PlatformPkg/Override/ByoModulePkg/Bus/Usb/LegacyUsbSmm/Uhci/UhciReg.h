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
  UhciReg.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_UHCI_REG_H_
#define _EFI_UHCI_REG_H_

#define BIT(a)  (1 << (a))

enum {
    UHCI_FRAME_NUM        = 1024,

    //
    // Register offset and PCI related staff
    //
    CLASSC_OFFSET         = 0x09,
    USBBASE_OFFSET        = 0x20,
    UCHI_IO_BASE_ADDRESS_MASK  = 0x0ffe0,
    USB_BAR_INDEX         = 4,
    PCI_CLASSC_PI_UHCI    = 0x00,

    USBCMD_OFFSET         = 0,
    USBSTS_OFFSET         = 2,
    USBINTR_OFFSET        = 4,
    USBPORTSC_OFFSET      = 0x10,
    USB_FRAME_NO_OFFSET   = 6,
    USB_FRAME_BASE_OFFSET = 8,
    USB_EMULATION_OFFSET  = 0xC0,

    //
    // Packet IDs
    //
    SETUP_PACKET_ID       = 0x2D,
    INPUT_PACKET_ID       = 0x69,
    OUTPUT_PACKET_ID      = 0xE1,
    ERROR_PACKET_ID       = 0x55,

    //
    // USB port status and control bit definition.
    //
    USBPORTSC_CCS         = BIT(0),  // Current Connect Status
    USBPORTSC_CSC         = BIT(1),  // Connect Status Change
    USBPORTSC_PED         = BIT(2),  // Port Enable / Disable
    USBPORTSC_PEDC        = BIT(3),  // Port Enable / Disable Change
    USBPORTSC_LSL         = BIT(4),  // Line Status Low BIT
    USBPORTSC_LSH         = BIT(5),  // Line Status High BIT
    USBPORTSC_RD          = BIT(6),  // Resume Detect
    USBPORTSC_LSDA        = BIT(8),  // Low Speed Device Attached
    USBPORTSC_PR          = BIT(9),  // Port Reset
    USBPORTSC_SUSP        = BIT(12), // Suspend

    //
    // UHCI Spec said it must implement 2 ports each host at least,
    // and if more, check whether the bit7 of PORTSC is always 1.
    // So here assume the max of port number each host is 16.
    //
    USB_MAX_ROOTHUB_PORT  = 0x0F,   // Max number of root hub port

    //
    // USB Legacy Control register bit definitions
    //
    USBLEGACY_60REN       = BIT(0),  // SMI on Port 60h Read Enable
    USBLEGACY_60WEN       = BIT(1),  // SMI on Port 60h Write Enable
    USBLEGACY_64REN       = BIT(2),  // SMI on Port 64h Read Enable
    USBLEGACY_64WEN       = BIT(3),  // SMI on Port 64h Write Enable
    USBLEGACY_SMIEN       = BIT(4),  // SMI on USB IRQ Enable
    USBLEGACY_A20PASSEN   = BIT(5),  // A20Gate Pass Through Enable
    USBLEGACY_PSATTE      = BIT(6),  // Pass Through State
    USBLEGACY_SMIATENDPS  = BIT(7),  // SMI at End of A20 Pass-Through Enable
    USBLEGACY_TRAPBY60R   = BIT(8),  // SMI Caused by Port 60h Pead
    USBLEGACY_TRAPBY60W   = BIT(9),  // SMI Caused by Port 60h Write
    USBLEGACY_TRAPBY64R   = BIT(10), // SMI Caused by Port 64h Read
    USBLEGACY_TRAPBY64W   = BIT(11), // SMI Caused by Port 64h Write
    USBLEGACY_SMIBYUSB    = BIT(12), // SMI Caused by USB Interrupt
    USBLEGACY_USBPIRQEN   = BIT(13), // PCI Interrupt Enable
    USBLEGACY_RESERVED    = BIT(14), // Reserved
    USBLEGACY_SMIBYENDPS  = BIT(15), // SMI Caused by End of Pass-Through

    //
    // Command register bit definitions
    //
    USBCMD_RS             = BIT(0),  // Run/Stop
    USBCMD_HCRESET        = BIT(1),  // Host reset
    USBCMD_GRESET         = BIT(2),  // Global reset
    USBCMD_EGSM           = BIT(3),  // Global Suspend Mode
    USBCMD_FGR            = BIT(4),  // Force Global Resume
    USBCMD_SWDBG          = BIT(5),  // SW Debug mode
    USBCMD_CF             = BIT(6),  // Config Flag (sw only)
    USBCMD_MAXP           = BIT(7),  // Max Packet (0 = 32, 1 = 64)

    //
    // USB Status register bit definitions
    //
    USBSTS_USBINT         = BIT(0),  // Interrupt due to IOC
    USBSTS_ERROR          = BIT(1),  // Interrupt due to error
    USBSTS_RD             = BIT(2),  // Resume Detect
    USBSTS_HSE            = BIT(3),  // Host System Error
    USBSTS_HCPE           = BIT(4),  // Host Controller Process Error
    USBSTS_HCH            = BIT(5),  // HC Halted

    USBTD_ACTIVE          = BIT(7),  // TD is still active
    USBTD_STALLED         = BIT(6),  // TD is stalled
    USBTD_BUFFERR         = BIT(5),  // Buffer underflow or overflow
    USBTD_BABBLE          = BIT(4),  // Babble condition
    USBTD_NAK             = BIT(3),  // NAK is received
    USBTD_CRC             = BIT(2),  // CRC/Time out error
    USBTD_BITSTUFF        = BIT(1),  // Bit stuff error

    //
    // Interrupt Enable register bit definitions
    //
    USBINTR_TIMEOUT       = BIT(0),  // Timeout/CRC Interrupt Enable
    USBINTR_RESUME        = BIT(1),  // Resume Interrupt Enable
    USBINTR_IOC           = BIT(2),  // Interrupt On Complete Enable
    USBINTR_SP            = BIT(3),  // Short Packet Interrupt Enable
};

UINT16
UhciReadReg (
    IN  USB_HC_DEV             *Uhc,
    IN UINT32                  Offset
)
/*++

Routine Description:

  Read a UHCI register

Arguments:

  Uhc      - The UHCI device
  Offset   - Register offset to USB_BAR_INDEX

Returns:

  Content of register

--*/
;

VOID
UhciWriteReg (
    IN USB_HC_DEV              *Uhc,
    IN UINT32                  Offset,
    IN UINT16                  Data
)
/*++

Routine Description:

  Write data to UHCI register

Arguments:

  Uhc      - The UHCI device
  Offset   - Register offset to USB_BAR_INDEX
  Data     - Data to write

Returns:

  VOID

--*/
;

VOID
UhciSetRegBit (
    IN  USB_HC_DEV             *Uhc,
    IN UINT32                  Offset,
    IN UINT16                  Bit
)
/*++

Routine Description:

  Set a bit of the UHCI Register

Arguments:

  Uhc        - The UHCI device
  Offset     - Register offset to USB_BAR_INDEX
  Bit        - The bit to set

Returns:

  None

--*/
;

VOID
UhciClearRegBit (
    IN USB_HC_DEV             *Uhc,
    IN UINT32                  Offset,
    IN UINT16                  Bit
)
/*++

Routine Description:

  Clear a bit of the UHCI Register

Arguments:

  Uhc        - The UHCI device
  Offset     - Register offset to USB_BAR_INDEX
  Bit        - The bit to clear
Returns:

  None

--*/
;

EFI_STATUS
UhciEnableDevice (
    IN  USB_HC_DEV          *Uhc
)
/*++

Routine Description:

  Enable device by setting IO, memory and bus master bit
  in PCI command register

Arguments:

  Uhc        - The UHCI device

Returns:

   EFI_SUCCESS - The devcie is enabled
   EFI_TIMEOUT - Failed to enable the device

--*/
;

EFI_STATUS
UhciDisableDevice (
    IN  USB_HC_DEV          *Uhc
)
/*++

Routine Description:

  Disable device by clearing IO, memory and bus master bit
  in PCI command register

Arguments:

  Uhc        - The UHCI device

Returns:

   EFI_SUCCESS - The devcie is disabled
   EFI_TIMEOUT - Failed to disable the device

--*/
;

VOID
EFIAPI
UhciProcessAsyncReqList (
    IN USB_HC_DEV            *Uhc
)
;

VOID
UhciAckAllInterrupt (
    IN  USB_HC_DEV          *Uhc
)
/*++

Routine Description:

  Clear all the interrutp status bits, these bits
  are Write-Clean

Arguments:

  Uhc - The UHCI device

Returns:

  None

--*/
;

EFI_STATUS
UhciStopHc (
    IN USB_HC_DEV         *Uhc,
    IN UINTN              Timeout
)
/*++

Routine Description:

  Stop the host controller

Arguments:

  Uhc     - The UHCI device
  Timeout - Max time allowed

Returns:

  EFI_SUCCESS - The host controller is stopped
  EFI_TIMEOUT - Failed to stop the host controller

--*/
;

BOOLEAN
UhciIsHcWorking (
    IN  USB_HC_DEV             *Uhc
)
/*++

Routine Description:

  Check whether the host controller operates well

Arguments:

  Uhc     - The UHCI device

Returns:

   TRUE  -  Host controller is working
   FALSE -  Host controller is halted or system error

--*/
;

VOID
UhciSetFrameListBaseAddr (
    IN  USB_HC_DEV             *Uhc,
    IN VOID                    *Addr
)
/*++

Routine Description:

  Set the UHCI frame list base address. It can't use
  UhciWriteReg which access memory in UINT16.

Arguments:

  Uhc     - The UHCI device
  Addr    - Address to set

Returns:

  VOID

--*/
;

VOID
UhciTurnOffUsbEmulation (
    IN  USB_HC_DEV             *Uhc
)
/*++

  Routine Description:

    Disable USB Emulation

  Arguments:

    Uhc     - The UHCI device

  Returns:

    VOID

--*/
;
#endif
