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
  UchiReg.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "Uhci.h"
#include "../UsbLib/UsbUtility.h"

UINT16
UhciReadReg (
    IN USB_HC_DEV              *Uhc,
    IN UINT32                  Offset
)
/*++

Routine Description:

  Read a UHCI register

Arguments:

  Uhc      - The UHCI device
  Offset   - Register offset

Returns:

  Content of register

--*/
{

    return SmmIoRead16 (Uhc->UsbIoBase + Offset);

}

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
{
    SmmIoWrite16(Uhc->UsbIoBase + Offset, Data);
}

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
{
    UINT16  Data;

    Data = UhciReadReg (Uhc, Offset);
    Data |= Bit;
    UhciWriteReg (Uhc, Offset, Data);
}

VOID
UhciClearRegBit (
    IN  USB_HC_DEV             *Uhc,
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
{
    UINT16  Data;

    Data = UhciReadReg (Uhc, Offset);
    Data &= ~Bit;
    UhciWriteReg (Uhc, Offset, Data);
}

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
{
    UhciWriteReg (Uhc, USBSTS_OFFSET, 0x02);

    //
    // If current HC is halted, re-enable it. Host Controller Process Error
    // is a temporary error status.
    //
    if (!UhciIsHcWorking (Uhc)) {
        UHCI_DEBUG((EFI_D_ERROR, "UhciAckAllInterrupt: re-enable the UHCI from system error\n"));
        Uhc->UsbHc.SetState (&Uhc->UsbHc, EfiUsbHcStateOperational);
    }
}

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
{
    UINT64              PciAddress;
    UINT16              Command;
    EFI_STATUS          Status;

    //
    // PciAddress of command register
    //
    PciAddress = SMM_PCI_ADDRESS (
                     Uhc->PciAddress.Bus,
                     Uhc->PciAddress.Device,
                     Uhc->PciAddress.Function,
                     PCI_COMMAND_OFFSET
                 );
    Status = SmmPciCfgRead (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Command
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    Command |= (EFI_PCI_COMMAND_IO_SPACE |
                EFI_PCI_COMMAND_BUS_MASTER);

    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Command);
    return Status;
}

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
{
    UINT64              PciAddress;
    UINT16              Command;
    EFI_STATUS          Status;

    //
    // PciAddress of command register
    //
    PciAddress = SMM_PCI_ADDRESS (
                     Uhc->PciAddress.Bus,
                     Uhc->PciAddress.Device,
                     Uhc->PciAddress.Function,
                     PCI_COMMAND_OFFSET
                 );
    Status = SmmPciCfgRead (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Command
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    Command &= ~(EFI_PCI_COMMAND_IO_SPACE |
                 EFI_PCI_COMMAND_BUS_MASTER);

    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Command);

    return Status;
}

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
{
    UINT16                UsbSts;
    UINTN                 Index;

    UhciClearRegBit (Uhc, USBCMD_OFFSET, USBCMD_RS);
    UhciClearRegBit (Uhc, USBCMD_OFFSET, USBCMD_CF);

    //
    // ensure the HC is in halt status after send the stop command
    // Timeout is in us unit.
    //
    for (Index = 0; Index < (Timeout / 50) + 1; Index++) {
        UsbSts = UhciReadReg (Uhc, USBSTS_OFFSET);

        if ((UsbSts & USBSTS_HCH) == USBSTS_HCH) {
            return EFI_SUCCESS;
        }

        Stall (50);
    }

    return EFI_TIMEOUT;
}

BOOLEAN
UhciIsHcWorking (
    IN USB_HC_DEV         *Uhc
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
{
    UINT16                UsbSts;

    UsbSts = UhciReadReg (Uhc, USBSTS_OFFSET);

    if (UsbSts & (USBSTS_HCPE | USBSTS_HSE | USBSTS_HCH)) {
        UHCI_DEBUG((EFI_D_ERROR, "UhciIsHcWorking: current USB state is %x\n", UsbSts));
        return FALSE;
    }

    return TRUE;
}

VOID
UhciSetFrameListBaseAddr (
    IN USB_HC_DEV              *Uhc,
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
{
    UINT32                  Data;

    Data = (UINT32) ((UINTN) Addr & 0xFFFFF000);
    SmmIoWrite32 (Uhc->UsbIoBase + USB_FRAME_BASE_OFFSET, Data);
}
