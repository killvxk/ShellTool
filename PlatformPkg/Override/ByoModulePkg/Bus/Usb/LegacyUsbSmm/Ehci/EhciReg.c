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
  EhciReg.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "Ehci.h"
#include "../UsbLib/UsbUtility.h"
#include "Protocol/UsbPolicy.h"

UINT32
EhcReadCapRegister (
    IN  USB2_HC_DEV         *Ehc,
    IN  UINT32              Offset
)
/*++

Routine Description:

  Read  EHCI capability register

Arguments:

  Ehc     - The Ehc device
  Offset  - Capability register address

Returns:

  The register content read

--*/
{
    return SmmMmioRead32 (Ehc->UsbMemBase + Offset);
}

/**
  Read EHCI debug port register.

  @param  Ehc          The EHCI device.
  @param  Offset       Debug port register offset.

  @return The register content read.

**/
UINT32
EhcReadDbgRegister (
  IN  USB2_HC_DEV         *Ehc,
  IN  UINT32              Offset
  )
{
  UINT64    PciRegAddress;
  UINT32    MemoryBar;

  PciRegAddress = SMM_PCI_ADDRESS (
                    Ehc->PciAddress.Bus,
                    Ehc->PciAddress.Device,
                    Ehc->PciAddress.Function,
                    EHC_PCI_MEM_BASE_ADDRESS + 4 * Ehc->DebugPortBarNum
                  );

  SmmPciCfgRead (
    gSmst,
    EfiPciIoWidthUint32,
    (SMM_PCI_IO_ADDRESS *) &PciRegAddress,
    &MemoryBar
  );

  return SmmMmioRead32 (MemoryBar + Ehc->DebugPortOffset + Offset);
}

UINT32
EhcReadOpReg (
    IN  USB2_HC_DEV         *Ehc,
    IN  UINT32              Offset
)
/*++

Routine Description:

  Read  Ehc Operation register

Arguments:

  Ehc      - The EHCI device
  Offset   - The operation register offset

Returns:

  The register content read

--*/
{
    return SmmMmioRead32 (Ehc->UsbMemBase + Ehc->CapLen + Offset);
}

VOID
EhcWriteOpReg (
    IN USB2_HC_DEV          *Ehc,
    IN UINT32               Offset,
    IN UINT32               Data
)
/*++

Routine Description:

  Write  the data to the EHCI operation register

Arguments:

  Ehc      - The EHCI device
  Offset   - EHCI operation register offset
  Data     - The data to write

Returns:

  None

--*/
{
    SmmMmioWrite32 (Ehc->UsbMemBase + Ehc->CapLen + Offset, Data);
    return ;
}

VOID
EhcSetOpRegBit (
    IN USB2_HC_DEV          *Ehc,
    IN UINT32               Offset,
    IN UINT32               Bit
)
/*++

Routine Description:
  Set one bit of the operational register while keeping other bits

Arguments:
  Ehc     - The EHCI device
  Offset  - The offset of the operational register
  Bit     - The bit mask of the register to set

Returns:

  None

--*/
{
    UINT32                  Data;

    Data  = EhcReadOpReg (Ehc, Offset);
    Data |= Bit;
    EhcWriteOpReg (Ehc, Offset, Data);
}

VOID
EhcClearOpRegBit (
    IN USB2_HC_DEV          *Ehc,
    IN UINT32               Offset,
    IN UINT32               Bit
)
/*++

Routine Description:
  Clear one bit of the operational register while keeping other bits

Arguments:
  Ehc - The EHCI device
  Offset  - The offset of the operational register
  Bit     - The bit mask of the register to clear

Returns:

  None

--*/
{
    UINT32                  Data;

    Data  = EhcReadOpReg (Ehc, Offset);
    Data &= ~Bit;
    EhcWriteOpReg (Ehc, Offset, Data);
}

EFI_STATUS
EhcWaitOpRegBit (
    IN USB2_HC_DEV          *Ehc,
    IN UINT32               Offset,
    IN UINT32               Bit,
    IN BOOLEAN              WaitToSet,
    IN UINT32               Timeout
)
/*++

Routine Description:

  Wait the operation register's bit as specified by Bit
  to become set (or clear)

Arguments:

  Ehc         - The EHCI device
  Offset      - The offset of the operation register
  Bit         - The bit of the register to wait for
  WaitToSet   - Wait the bit to set or clear
  Timeout     - The time to wait before abort (in millisecond)

Returns:

  EFI_SUCCESS - The bit successfully changed by host controller
  EFI_TIMEOUT - The time out occurred

--*/
{
    UINT32                  Index;

    for (Index = 0; Index < Timeout / EHC_SYNC_POLL_INTERVAL + 1; Index++) {
        if (EHC_REG_BIT_IS_SET (Ehc, Offset, Bit) == WaitToSet) {
            return EFI_SUCCESS;
        }

        Stall (EHC_SYNC_POLL_INTERVAL);
    }

    return EFI_TIMEOUT;
}

EFI_STATUS
EhciEnableDevice (
    IN USB2_HC_DEV          *Ehc
)
/*++

Routine Description:

  Enable device by setting IO, memory and bus master bit
  in PCI command register

Arguments:

  Ehc        - The UHCI device

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
                     Ehc->PciAddress.Bus,
                     Ehc->PciAddress.Device,
                     Ehc->PciAddress.Function,
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

    Command |= ( EFI_PCI_COMMAND_MEMORY_SPACE |
                 EFI_PCI_COMMAND_BUS_MASTER);

    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Command);
    return Status;
}

EFI_STATUS
EhciDisableDevice (
    IN USB2_HC_DEV          *Ehc
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
                     Ehc->PciAddress.Bus,
                     Ehc->PciAddress.Device,
                     Ehc->PciAddress.Function,
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
                 EFI_PCI_COMMAND_MEMORY_SPACE |
                 EFI_PCI_COMMAND_BUS_MASTER);

    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint16,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &Command);
    return Status;
}

VOID
EhcSetBiosOwnership (
    IN USB2_HC_DEV          *Ehc
)
/*++

Routine Description:

  Stop the legacy USB SMI support

Arguments:

  Ehc - The EHCI device.

Returns:

  None

--*/
{
    UINT32                    ExtendCap;
    UINTN                     SetBits, ClearBits;
    UINT64                    PciAddress;
    EFI_STATUS                Status;

    EHCI_DEBUG ((EFI_D_INFO, "EhcSetBiosOwnership: called to set BIOS ownership\n"));

    ExtendCap  = (Ehc->HcCapParams >> 8) & 0xFF;
    PciAddress = SMM_PCI_ADDRESS (
                     Ehc->PciAddress.Bus,
                     Ehc->PciAddress.Device,
                     Ehc->PciAddress.Function,
                     ExtendCap);
    SetBits    = USBLEGSP_BIOS_SEMAPHORE;
    ClearBits  = USBLEGSP_OS_SEMAPHORE;
    Status = SmmPciCfgModify (
                 gSmst,
                 SmmPciWidthUint32,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &SetBits,
                 &ClearBits
             );
    ASSERT_EFI_ERROR (Status);

    PciAddress += 4;
    SetBits = USBLEGCTLSTS_OWNERSHIP;
    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint32,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &SetBits
             );

    ASSERT_EFI_ERROR (Status);
}

VOID
EhcClearBiosOwnership (
    IN USB2_HC_DEV          *Ehc
)
/*++

Routine Description:

  Stop the legacy USB SMI support

Arguments:

  Ehc - The EHCI device.

Returns:

  None

--*/
{
    UINT32                    ExtendCap;
    UINTN                     SetBits, ClearBits;
    UINT64                    PciAddress;
    EFI_STATUS                Status;

    EHCI_DEBUG ((EFI_D_INFO, "EhcClearBiosOwnership: called to clear BIOS ownership\n"));

    ExtendCap  = (Ehc->HcCapParams >> 8) & 0xFF;
    PciAddress = SMM_PCI_ADDRESS (
                     Ehc->PciAddress.Bus,
                     Ehc->PciAddress.Device,
                     Ehc->PciAddress.Function,
                     ExtendCap);
    SetBits    = USBLEGSP_OS_SEMAPHORE;
    ClearBits  = USBLEGSP_BIOS_SEMAPHORE;
    Status = SmmPciCfgModify (
                 gSmst,
                 SmmPciWidthUint32,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &SetBits,
                 &ClearBits
             );
    ASSERT_EFI_ERROR (Status);

    PciAddress += 4;
    SetBits = USBLEGCTLSTS_OWNERSHIP;
    Status = SmmPciCfgWrite (
                 gSmst,
                 SmmPciWidthUint32,
                 (SMM_PCI_IO_ADDRESS *) &PciAddress,
                 &SetBits
             );

    ASSERT_EFI_ERROR (Status);
}

EFI_STATUS
EhcSetAndWaitDoorBell (
    IN  USB2_HC_DEV         *Ehc,
    IN  UINT32              Timeout
)
/*++

Routine Description:

  Set door bell and wait it to be ACKed by host controller.
  This function is used to synchronize with the hardware.

Arguments:

  Ehc     - The EHCI device
  Timeout - The time to wait before abort (in millisecond, ms)
Returns:

  EFI_SUCCESS : Synchronized with the hardware
  EFI_TIMEOUT : Time out happened while waiting door bell to set

--*/
{
    EFI_STATUS              Status;
    UINT32                  Data;

    EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_IAAD);

    Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_IAA, TRUE, Timeout);

    //
    // ACK the IAA bit in USBSTS register. Make sure other
    // interrupt bits are not ACKed. These bits are WC (Write Clean).
    //
    Data  = EhcReadOpReg (Ehc, EHC_USBSTS_OFFSET);
    Data &= ~USBSTS_INTACK_MASK;
    Data |= USBSTS_IAA;

    EhcWriteOpReg (Ehc, EHC_USBSTS_OFFSET, Data);

    return Status;
}

VOID
EhcAckAllInterrupt (
    IN  USB2_HC_DEV         *Ehc
)
/*++

Routine Description:

  Clear all the interrutp status bits, these bits
  are Write-Clean

Arguments:

  Ehc - The EHCI device

Returns:

  None

--*/
{
    EhcWriteOpReg (Ehc, EHC_USBSTS_OFFSET, 0x0002);
}

EFI_STATUS
EhcEnablePeriodSchd (
    IN USB2_HC_DEV          *Ehc,
    IN UINT32               Timeout
)
/*++

Routine Description:

  Enable the periodic schedule then wait EHC to
  actually enable it.

Arguments:

  Ehc     - The EHCI device
  Timeout - The time to wait before abort (in millisecond, ms)
Returns:

  EFI_SUCCESS : The periodical schedule is enabled
  EFI_TIMEOUT : Time out happened while enabling periodic schedule

--*/
{
    EFI_STATUS              Status;

    EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_ENABLE_PERIOD);

    Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_PERIOD_ENABLED, TRUE, Timeout);
    return Status;
}

EFI_STATUS
EhcDisablePeriodSchd (
    IN USB2_HC_DEV          *Ehc,
    IN UINT32               Timeout
)
/*++

Routine Description:

  Disable periodic schedule

Arguments:

  Ehc     - The EHCI device
  Timeout - Time to wait before abort (in millisecond, ms)

Returns:

  EFI_SUCCESS      : Periodic schedule is disabled.
  EFI_DEVICE_ERROR : Fail to disable periodic schedule

--*/
{
    EFI_STATUS              Status;

    EhcClearOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_ENABLE_PERIOD);

    Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_PERIOD_ENABLED, FALSE, Timeout);
    return Status;
}

EFI_STATUS
EhcEnableAsyncSchd (
    IN USB2_HC_DEV          *Ehc,
    IN UINT32               Timeout
)
/*++

Routine Description:

  Enable asynchrounous schedule

Arguments:

  Ehc     - The EHCI device
  Timeout - Time to wait before abort

Returns:

  EFI_SUCCESS : The EHCI asynchronous schedule is enabled
  Others      : Failed to enable the asynchronous scheudle

--*/
{
    EFI_STATUS              Status;

    EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_ENABLE_ASYNC);

    Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_ASYNC_ENABLED, TRUE, Timeout);
    return Status;
}

EFI_STATUS
EhcDisableAsyncSchd (
    IN USB2_HC_DEV          *Ehc,
    IN UINT32               Timeout
)
/*++

Routine Description:

  Disable asynchrounous schedule

Arguments:

  Ehc     - The EHCI device
  Timeout - Time to wait before abort (in millisecond, ms)

Returns:

  EFI_SUCCESS : The asynchronous schedule is disabled
  Others      : Failed to disable the asynchronous schedule

--*/
{
    EFI_STATUS  Status;

    EhcClearOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_ENABLE_ASYNC);

    Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_ASYNC_ENABLED, FALSE, Timeout);
    return Status;
}


BOOLEAN
EhcIsHalt (
    IN USB2_HC_DEV          *Ehc
)
/*++

Routine Description:

  Whether Ehc is halted

Arguments:

  Ehc - The EHCI device

Returns:

  TRUE  : The controller is halted
  FALSE : It isn't halted

--*/
{
    return EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT);
}

BOOLEAN
EhcIsSysError (
    IN USB2_HC_DEV          *Ehc
)
/*++

Routine Description:

  Whether system error occurred

Arguments:

  Ehc - The EHCI device

Returns:

  TRUE  : System error happened
  FALSE : No system error

--*/
{
    return EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_SYS_ERROR);
}

EFI_STATUS
EhcResetHC (
    IN USB2_HC_DEV          *Ehc,
    IN UINT32               Timeout
)
/*++

Routine Description:

  Reset the host controller

Arguments:

  Ehc     - The EHCI device
  Timeout - Time to wait before abort (in millisecond, ms)

Returns:

  EFI_SUCCESS : The host controller is reset
  Others      : Failed to reset the host

--*/
{
    EFI_STATUS              Status;

    //
    // Host can only be reset when it is halt. If not so, halt it
    //
    if (!EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT)) {
        Status = EhcHaltHC (Ehc, Timeout);

        if (EFI_ERROR (Status)) {
            return Status;
        }
    }

    EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RESET);
    Status = EhcWaitOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RESET, FALSE, Timeout);
    return Status;
}

EFI_STATUS
EhcHaltHC (
    IN USB2_HC_DEV         *Ehc,
    IN UINT32              Timeout
)
/*++

Routine Description:

  Halt the host controller

Arguments:

  Ehc     - The EHCI device
  Timeout - Time to wait before abort

Returns:

  EFI_SUCCESS : The EHCI is halt
  EFI_TIMEOUT : Failed to halt the controller before Timeout

--*/
{
    EFI_STATUS              Status;

    EhcClearOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RUN);
    Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT, TRUE, Timeout);
    return Status;
}

EFI_STATUS
EhcRunHC (
    IN USB2_HC_DEV          *Ehc,
    IN UINT32               Timeout
)
/*++

Routine Description:

  Set the EHCI to run

Arguments:

  Ehc     - The EHCI device
  Timeout - Time to wait before abort

Returns:

  EFI_SUCCESS : The EHCI is running
  Others      : Failed to set the EHCI to run

--*/
{
    EFI_STATUS              Status;

    EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RUN);
    Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT, FALSE, Timeout);
    return Status;
}

EFI_STATUS
EhcInitHC (
    IN USB2_HC_DEV          *Ehc
)
/*++

Routine Description:

  Initialize the HC hardware.
  EHCI spec lists the five things to do to initialize the hardware
  1. Program CTRLDSSEGMENT
  2. Set USBINTR to enable interrupts
  3. Set periodic list base
  4. Set USBCMD, interrupt threshold, frame list size etc
  5. Write 1 to CONFIGFLAG to route all ports to EHCI

Arguments:

  Ehc - The EHCI device

Returns:

  EFI_SUCCESS : The EHCI has come out of halt state
  EFI_TIMEOUT : Time out happened

--*/
{
    EFI_STATUS              Status;
    USB_CFG                 *UsbCfg;
    UINT8                   NumPort = 0;
    UINT8                   Index   = 0;

    //ASSERT (EhcIsHalt (Ehc));

    //
    // Allocate the periodic frame and associated memeory
    // management facilities if not already done.
    //
    if (Ehc->PeriodFrame != NULL) {
        EhcFreeSched (Ehc);
    }

    Status = EhcInitSched (Ehc);

    if (EFI_ERROR (Status)) {
        return Status;
    }
    //
    // 1. Program the CTRLDSSEGMENT register with the high 32 bit addr
    //
    EhcWriteOpReg (Ehc, EHC_CTRLDSSEG_OFFSET, Ehc->High32bitAddr);

    //
    // 2. Clear USBINTR to disable all the interrupt. UEFI works by polling
    //
    EhcWriteOpReg (Ehc, EHC_USBINTR_OFFSET, 0);

    //
    // 3. Program periodic frame list, already done in EhcInitSched
    // 4. Start the Host Controller
    //
    EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RUN);

    //
    // Get USB Configuration Information
    //
    GET_USB_CFG(UsbCfg);

    //
    // 5. Set all ports routing to EHC
    //
    if (UsbCfg->UsbDeviceInfor.UsbOperationMode == HIGH_SPEED) {
        EhcSetOpRegBit (Ehc, EHC_CONFIG_FLAG_OFFSET, CONFIGFLAG_ROUTE_EHC);
    }

    //
    // Do port Power according the Port Power switch ,Ehci spec 2.2, HCSPARAMS bit4
    //
    if ((Ehc->HcStructParams &0x00000010) == 0x00000010) {
        NumPort = (UINT8)Ehc->HcStructParams & 0x0f;
        for ( Index =0 ; Index< NumPort ; Index++) {
            EhcSetOpRegBit (Ehc, EHC_PORT_STAT_OFFSET+Index*4, PORTSC_POWER);
            Stall (EHC_ROOT_PORT_RECOVERY_STALL);
        }
    }

    //
    // Wait roothub port power stable
    // This seems uncessary??? cause this is for setting Port Power Bit
    //
    Stall (EHC_ROOT_PORT_RECOVERY_STALL);

    Status = EhcEnablePeriodSchd (Ehc, EHC_GENERIC_TIMEOUT);

    if (EFI_ERROR (Status)) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcInitHC: failed to enable period schedule\n"));
        return Status;
    }

    Status = EhcEnableAsyncSchd (Ehc, EHC_GENERIC_TIMEOUT);

    if (EFI_ERROR (Status)) {
        EHCI_DEBUG ((EFI_D_ERROR, "EhcInitHC: failed to enable async schedule\n"));
        return EFI_SUCCESS;
    }

    return EFI_SUCCESS;
}
