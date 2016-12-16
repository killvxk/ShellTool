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

  EhciReg.h

Abstract:

  This file contains the definination for host controller register operation routines

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------

  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_EHCI_REG_H_
#define _EFI_EHCI_REG_H_

enum {
    //
    // Capability register offset
    //
    EHC_CAPLENGTH_OFFSET    = 0,    // Capability register length offset
    EHC_HCSPARAMS_OFFSET    = 0x04, // Structural Parameters 04-07h
    EHC_HCCPARAMS_OFFSET    = 0x08, // Capability parameters offset

    //
    // Capability register bit definition
    //
    HCSP_NPORTS             = 0x0F, // Number of root hub port
    HCCP_64BIT              = 0x01, // 64-bit addressing capability

    //
    // Operational register offset
    //
    EHC_USBCMD_OFFSET       = 0x0,  // USB command register offset
    EHC_USBSTS_OFFSET       = 0x04, // Statue register offset
    EHC_USBINTR_OFFSET      = 0x08, // USB interrutp offset
    EHC_FRINDEX_OFFSET      = 0x0C, // Frame index offset
    EHC_CTRLDSSEG_OFFSET    = 0x10, // Control data structure segment offset
    EHC_FRAME_BASE_OFFSET   = 0x14, // Frame list base address offset
    EHC_ASYNC_HEAD_OFFSET   = 0x18, // Next asynchronous list address offset
    EHC_CONFIG_FLAG_OFFSET  = 0x40, // Configure flag register offset
    EHC_PORT_STAT_OFFSET    = 0x44, // Port status/control offset

    EHC_FRAME_LEN           = 1024,

    //
    // Register bit definition
    //
    CONFIGFLAG_ROUTE_EHC    = 0x01, // Route port to EHC

    USBLEGSP_BIOS_SEMAPHORE = BIT(16), // HC BIOS Owned Semaphore
    USBLEGSP_OS_SEMAPHORE   = BIT(24), // HC OS Owned Semaphore

    USBLEGCTLSTS_IOC_ENABLE       = BIT(0), // SMI on USB Complete Enable
    USBLEGCTLSTS_ERROR_ENABLE     = BIT(1), // SMI on USB Error Enable
    USBLEGCTLSTS_PORT_CHANGE_ENABLE      = BIT(2), // SMI on Port Change Enable
    USBLEGCTLSTS_FRAME_LIST_ROLLOVER_ENABLE     = BIT(3), // SMI on Frame List Rollover Enable
    USBLEGCTLSTS_HOST_ERROR_ENABLE       = BIT(4), // SMI on Host System Error Enable
    USBLEGCTLSTS_ASYNC_ADVANCE_ENABLE    = BIT(5), // SMI on Async Advance Enable
    USBLEGCTLSTS_OWNERSHIP_ENABLE        = BIT(13), // SMI on OS Ownership Enable
    USBLEGCTLSTS_PCI_COMMAND_ENABLE      = BIT(14), // SMI on PCI Command Enable
    USBLEGCTLSTS_BAR_ENABLE              = BIT(15), // SMI on BAR Enable
    USBLEGCTLSTS_IOC              = BIT(16), // SMI on USB Complete
    USBLEGCTLSTS_ERROR            = BIT(17), // SMI on USB Error
    USBLEGCTLSTS_PORT_CHANGE      = BIT(18), // SMI on Port Change
    USBLEGCTLSTS_FRAME_LIST_ROLLOVER     = BIT(19), // SMI on Frame List Rollover
    USBLEGCTLSTS_HOST_ERROR       = BIT(20), // SMI on Host System Error
    USBLEGCTLSTS_ASYNC_ADVANCE    = BIT(21), // SMI on Async Advance
    USBLEGCTLSTS_OWNERSHIP        = BIT(29), // SMI on OS Ownership
    USBLEGCTLSTS_PCI_COMMAND      = BIT(30), // SMI on PCI Command
    USBLEGCTLSTS_BAR              = BIT(31), // SMI on BAR

    USBCMD_RUN              = 0x01,   // Run/stop
    USBCMD_RESET            = 0x02,   // Start the host controller reset
    USBCMD_ENABLE_PERIOD    = 0x10,   // Enable periodic schedule
    USBCMD_ENABLE_ASYNC     = 0x20,   // Enable asynchronous schedule
    USBCMD_IAAD             = 0x40,   // Interrupt on async advance doorbell

    USBSTS_IOC              = 0x01,
    USBSTS_USBERRINT        = 0x02,
    USBSTS_PORT_CHANGE      = 0x04,
    USBSTS_FARME_LIST_ROLLVER      = 0x08,
    USBSTS_IAA              = 0x20,   // Interrupt on async advance
    USBSTS_PERIOD_ENABLED   = 0x4000, // Periodic schedule status
    USBSTS_ASYNC_ENABLED    = 0x8000, // Asynchronous schedule status
    USBSTS_HALT             = 0x1000, // Host controller halted
    USBSTS_SYS_ERROR        = 0x10,   // Host system error
    USBSTS_INTACK_MASK      = 0x003F, // Mask for the interrupt ACK, the WC
    // (write clean) bits in USBSTS register

    USBINTR_IOCEN           = 0x01,   // USB Interrupt Enable
    USBINTR_ERROR           = 0x02,   // USB Error Interrupt Enable
    USBINTR_PORT_CHANGE     = 0x04,   // Port Change Interrupt Enable
    USBINTR_FAME_LIST_ROLLOVER   = 0x08,   // Frame List Rollover Enable
    USBINTR_HOST_ERROR      = 0x10,   // Host System Error Enable
    USBINTR_ASYNC_ADVANCE   = 0x20,   // Interrupt on Async Advance Enable
    PORTSC_CONN             = 0x01,   // Current Connect Status
    PORTSC_CONN_CHANGE      = 0x02,   // Connect Status Change
    PORTSC_ENABLED          = 0x04,   // Port Enable / Disable
    PORTSC_ENABLE_CHANGE    = 0x08,   // Port Enable / Disable Change
    PORTSC_OVERCUR          = 0x10,   // Over current Active
    PORTSC_OVERCUR_CHANGE   = 0x20,   // Over current Change
    PORSTSC_RESUME          = 0x40,   // Force Port Resume
    PORTSC_SUSPEND          = 0x80,   // Port Suspend State
    PORTSC_RESET            = 0x100,  // Port Reset
    PORTSC_LINESTATE_K      = 0x400,  // Line Status K-state
    PORTSC_LINESTATE_J      = 0x800,  // Line Status J-state
    PORTSC_POWER            = 0x1000, // Port Power
    PORTSC_OWNER            = 0x2000, // Port Owner
    PORTSC_CHANGE_MASK      = 0x2A,   // Mask of the port change bits,
    // they are WC (write clean)
    //
    // PCI Configuration Registers
    //
    EHC_PCI_CLASSC          = 0x09,
    EHC_PCI_CLASSC_PI       = 0x20,
    EHC_BAR_INDEX           = 0, /* how many bytes away from USB_BASE to 0x10 */
    EHC_PCI_MEM_BASE_ADDRESS = 0x10,    // Memory Bar register address
    EHC_PCI_MEM_BASE_MASK = 0x0ffffff00 // Memory address MASK
};

//
// Debug port capability id
//
#define EHC_DEBUG_PORT_CAP_ID   0x0A


#define EHC_LINK_TERMINATED(Link) (((Link) & 0x01) != 0)

#define EHC_ADDR(High, QhHw32)   \
        ((VOID *) (UINTN) (LShiftU64 ((High), 32) | ((QhHw32) & 0xFFFFFFF0)))

#define EHCI_IS_DATAIN(EndpointAddr) EHC_BIT_IS_SET((EndpointAddr), 0x80)

//
// Structure to map the hardware port states to the
// UEFI's port states.
//
typedef struct {
    UINT32                  HwState;
    UINT16                  UefiState;
} USB_PORT_STATE_MAP;
/*Ivan?
//
// Ehci Data and Ctrl Structures
//
#pragma pack(1)
typedef struct {
  UINT8                   PI;
  UINT8                   SubClassCode;
  UINT8                   BaseCode;
} USB_CLASSC;
#pragma pack()
*/

/**
  Read EHCI debug port register.

  @param  Ehc          The EHCI device.
  @param  Offset       Debug port register address.

  @return The register content read.
  @retval If err, return 0xffff.

**/
UINT32
EhcReadDbgRegister (
  IN  USB2_HC_DEV         *Ehc,
  IN  UINT32              Offset
  );

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
;

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
;

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
;
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
;


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
;


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
;

VOID
EhcClearLegacySupport (
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
;

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
;

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
;

EFI_STATUS
EhcSetAndWaitDoorBell (
    IN  USB2_HC_DEV         *Ehc,
    IN  UINT32               Timeout
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
;

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
;

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
;

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
;

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
;

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
;

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
;

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
;

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
;

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
;

#endif
