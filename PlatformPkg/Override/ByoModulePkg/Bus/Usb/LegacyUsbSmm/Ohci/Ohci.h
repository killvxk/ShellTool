/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2009 Intel Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:
  Ohci.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _OHCI_H
#define _OHCI_H

#include <PiSmm.h>
#include <IndustryStandard/pci22.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../UsbLib/SmmPciIo.h"
#include "../Include/UsbHostController.h"
#include "../UsbVirtualKbc/UsbVirtualKbc.h"

#define OHCI_DEBUG(a) //DEBUG(a)   

typedef struct _USB_OHCI_DEV     USB_OHCI_DEV;

BOOLEAN              mOhciOSRequestOwnership;
LIST_ENTRY           mOhcList;


/******************************************************************************
    Schedule function
/*****************************************************************************/
#define OHC_SETUP_PACKET_ID     0x00
#define OHC_INPUT_PACKET_ID     0x02
#define OHC_OUTPUT_PACKET_ID    0x01
#define OHC_ERROR_PACKET_ID     0x03

/******************************************************************************
    Register function
/*****************************************************************************/
#define BIT(a)  (1 << (a))

enum {
    //
    // PCI Configuration Registers
    //
    OHC_PCI_CLASSC          = 0x09,
    OHC_PCI_CLASSC_PI       = 0x10,
    OHC_BAR_INDEX           = 0,
    OHC_PCI_MEM_BASE_ADDRESS = 0x10,     // Memory Bar register address
    OHC_PCI_MEM_BASE_MASK = 0x0ffffff00, // Memory address MASK

    //
    // OHCI Host Controller Operational Registers
    //
    OHC_HCREVISION_OFFSET             = 0x0,              // HC Revesion register
    OHC_HCCONTROL_OFFSET              = 0x04,             // HC Control register
    OHC_HCCOMMANDSTATUS_OFFSET        = 0x08,             // HC Command Status register
    OHC_HCINTERRUPTSTATUS_OFFSET      = 0x0C,             // HC Interrupt Status register
    OHC_HCINTERRUPTENABLE_OFFSET      = 0x10,             // HC Interrupt Enable register
    OHC_HCINTERRUPTDISABLE_OFFSET     = 0x14,             // HC Interrupt Disable register
    OHC_HCHCCA_OFFSET                 = 0x18,             // HC HCCA base address register
    OHC_HCPERIODCURRENTED_OFFSET      = 0x1C,             // HC Period current ED register
    OHC_HCCONTROLHEADED_OFFSET        = 0x20,             // HC Control head ED register
    OHC_HCCONTROLCURRENTED_OFFSET     = 0x24,             // HC Controlcurrent ED register
    OHC_HCBULKHEADED_OFFSET           = 0x28,             // HC bulk head ED register
    OHC_HCBULKCURRENTED_OFFSET        = 0x2C,             // HC bulk current ED register
    OHC_HCDONE_HEAD_OFFSET            = 0x30,             // HC Done head register
    OHC_HCFMINTERVAL_OFFSET           = 0x34,             // HC Hc FmInterval Register
    OHC_HCFMREMAINING_OFFSET          = 0x38,             // Hc FmRemaining Register
    OHC_HCFMNUMBER_OFFSET             = 0x3C,             // HC Fm number register
    OHC_HCPERIODICSTART_OFFSET        = 0x40,             // HC Periodic start register
    OHC_HCLSTHRESHOLD_OFFSET          = 0x44,             // HC LS threshold register
    OHC_HCRHDESCRIPTORA_OFFSET        = 0x48,             // HC RH descriptoeA register
    OHC_HCRHDESCRIPTORB_OFFSET        = 0x4C,             // HC RH descriptoeB register
    OHC_HCRHSTATUS_OFFSET             = 0x50,             // HC RH status register
    OHC_HCRHPORTSTATUSN_OFFSET        = 0x54,             // HC RH port statusN register

    OHC_HCCOMMANDSTATUS_HCR           = BIT(0),           // Host Controller Reset
    OHC_HCCOMMANDSTATUS_CLF           = BIT(1),           // Host Control list filled
    OHC_HCCOMMANDSTATUS_BLF           = BIT(2),           // Host Bulk list filled
    OHC_HCCONTROL_CBSR                = 0x3,              // Host Controller control bulk serice ratio
    OHC_HCCONTROL_PLE                 = BIT(2),           // Host Controller Period list enable
    OHC_HCCONTROL_CLE                 = BIT(4),           // Host Controller Control list enable
    OHC_HCCONTROL_BLE                 = BIT(5),           // Host Controller Bulk list enable
    OHC_HCCONTROL_HCFS_RESET          = 0x0,              // Host Controller functional operational reset state
    OHC_HCCONTROL_HCFS_OPERATIONAL    = 0x80,             // Host Controller functional operational operational state
    OHC_HCCONTROL_HCFS                = BIT(6) | BIT(7),  // Host Controller functional operational state
    OHC_HCCONTROL_IR                  = BIT(8),           // Host Controller Interrupt routing
    OHC_HCINTERRUPTENABLE_SO          = BIT(0),           // Interrupt generation due to Scheduling Overrun
    OHC_HCINTERRUPTENABLE_WDH         = BIT(1),           // Interrupt generation due to HcDoneHead Writeback
    OHC_HCINTERRUPTENABLE_SF          = BIT(2),           // Interrupt generation due to Start of Frame
    OHC_HCINTERRUPTENABLE_RD          = BIT(3),           // Interrupt generation due to Resume Detect
    OHC_HCINTERRUPTENABLE_UE          = BIT(4),           // Interrupt generation due to Unrecoverable Error
    OHC_HCINTERRUPTENABLE_FNO         = BIT(5),           // Interrupt generation due to Frame Number Overflow
    OHC_HCINTERRUPTENABLE_RHSC        = BIT(6),           // Interrupt generation due to Root Hub Status Change
    OHC_HCINTERRUPTENABLE_OC          = BIT(30),          // Interrupt generation due to Ownership Change
    OHC_HCINTERRUPTENABLE_MIE         = BIT(31),          // Master Interrupt enable
    OHC_HCFMINTERVAL_FIT              = BIT(31),          // Host Controller Frame Interval Toggle
    OHC_HCFMINTERVAL_FSMPS            = 0x27780000,       // Host Controller Largest Data Packet
    OHC_HCPERIODICSTART_PS            = 0x2A2F,           // Host Controller period start
    OHC_HCRHDESCRIPTORA_NDP           = 0x0F,             // Host Controller Number Downstream Ports
    OHC_HCRHDESCRIPTORA_PSM           = BIT(8),           // Host Controller PowerSwitchingMode
    OHC_HCRHDESCRIPTORA_NPS           = BIT(9),           // Host Controller NoPowerSwitching
    OHC_HCRHDESCRIPTORA_OCPM          = BIT(11),          // Host Controller OverCurrentProtectionMode
    OHC_HCRHDESCRIPTORA_NCP           = BIT(12),          // Host Controller NoOverCurrentProtection
    OHC_HCRHSTATUS_LPSC               = BIT(16),          // Host Controller SetGlobalPower
    OHC_HCRHPORTSTATUSN_CCS           = BIT(0),           // Port CurrentConnectStatus
    OHC_HCRHPORTSTATUSN_PES           = BIT(1),           // Port PortEnableStatus
    OHC_HCRHPORTSTATUSN_PSS           = BIT(2),           // Port PortSuspendStatus
    OHC_HCRHPORTSTATUSN_POCI          = BIT(3),           // Port PortOverCurrentIndicator
    OHC_HCRHPORTSTATUSN_PRS           = BIT(4),           // Port PortResetStatus
    OHC_HCRHPORTSTATUSN_PPS           = BIT(8),           // Port PortPowerStatus
    OHC_HCRHPORTSTATUSN_LSDA          = BIT(9),           // Port LowSpeedDeviceAttached
    OHC_HCRHPORTSTATUSN_CSC           = BIT(16),          // Port ConnectStatusChange
    OHC_HCRHPORTSTATUSN_PESC          = BIT(17),          // Port PortEnableStatusChange
    OHC_HCRHPORTSTATUSN_PSSC          = BIT(18),          // Port PortSuspendStatusChange
    OHC_HCRHPORTSTATUSN_OCIC          = BIT(19),          // Port PortOverCurrentIndicatorChange
    OHC_HCRHPORTSTATUSN_PRSC          = BIT(20),          // Port PortResetStatusChange
};

#define OHceControl             0x100
#define OHceC_EE                BIT (0)                 // EmulationEnable.
#define OHceC_EI                BIT (1)                 // EmulationInterrupt.
#define OHceC_CP                BIT (2)                 // CharacterPending.
#define OHceC_IRQEn             BIT (3)                 // IRQEn.
#define OHceC_EIRQEn            BIT (4)                 // ExternalIRQEn.
#define OHceC_GateA20S          BIT (5)                 // GateA20Sequence.
#define OHceC_IRQ1AT            BIT (6)                 // IRQ1Active.
#define OHceC_IRQ12AT           BIT (7)                 // IRQ12Active.
#define OHceC_A20State          BIT (8)                 // A20State.
#define OHceInput               0x104
#define OHceOutput              0x108
#define OHceStatus              0x10C
#define OHceS_OutPutFull        BIT (0)                 // OutPutFull.
#define OHceS_InPutFull         BIT (1)                 // InPutFull.
#define OHceS_Flag              BIT (2)                 // Flag.
#define OHceS_CmdData           BIT (3)                 // CmdData.
#define OHceS_IS                BIT (4)                 // Inhibit Switch.
#define OHceS_AOF               BIT (5)                 // AuxOutputFull.
#define OHceS_Timeout           BIT (6)                 // Time-out.
#define OHceS_Parity            BIT (7)                 // Parity.

//
// OHCI time out define
//
enum {
    OHC_1_MICROSECOND             = 1,
    OHC_1_MILLISECOND             = 1000 * OHC_1_MICROSECOND,
    OHC_1_SECOND                  = 1000 * OHC_1_MILLISECOND,

    //
    // OHCI register operation timeout, set by experience
    //
    OHC_PORT_RESET_RECOVERY_STALL = 8 * OHC_1_MILLISECOND,
    OHC_SYNC_POLL_INTERVAL        = 50 * OHC_1_MICROSECOND,

    OHC_FORCE_GLOBAL_RESUME_TIME  = 20 * OHC_1_MILLISECOND,
    OHC_ROOT_PORT_RESET_STALL     = 50 * OHC_1_MILLISECOND,
    OHC_ROOT_PORT_ENABLE_STALL    = 10 * OHC_1_MILLISECOND,
    OHC_HCRESET_STALL             = 50 * OHC_1_MILLISECOND,

    USB_OHCI_DEV_SIGNATURE        = SIGNATURE_32 ('o', 'h', 'c', 'i'),
};

//
// OHCI Completion Codes
//
enum {
    USBTD_NOERROR                = 0x00,    // TD has no error
    USBTD_CRCERROR               = 0x01,    // TD has CRC error
    USBTD_BITSTUFFING            = 0x02,    // TD has a bit stuffing violation error
    USBTD_DATATOGGLEMISMATCH     = 0x03,    // TD has data toggle PID mismatch error
    USBTD_STALL                  = 0x04,    // TD has stall error
    USBTD_DATAOVERRUN            = 0x08,    // TD has data over run error
    USBTD_UNDERRRUN              = 0x09,    // TD has data under run error
    USBTD_BUFFEROVERRUN          = 0x0C,    // TD has buffer over run error
    USBTD_BUFFERUNDERRUN         = 0x0D,    // TD has buffer under run error
    USBTD_NOTACCESSED            = 0x0E,    // TD is not accessable
};

/******************************************************************************
    Schedule defintions and function
/*****************************************************************************/
#pragma pack(1)

#define NumOfIntEDs 63

typedef struct _OHCI_ED_HW {
    UINT32  FunctionAdress : 7;
    UINT32  EndpointNumber : 4;
    UINT32  Direction : 2;
    UINT32  Speed : 1;
    UINT32  sKip : 1;
    UINT32  Format : 1;
    UINT32  MaximumPacketSize : 11;
    UINT32  MaximumPacketSizeRsvd : 5;
    UINT32  TDQueueTailPointerRsvd : 4;
    UINT32  TDQueueTailPointer : 28;
    UINT32  Halted : 1;
    UINT32  ToggleCarry : 1;
    UINT32  ToggleCarryRsvd : 2;
    UINT32  TDQueueHeadPointer : 28;
    UINT32  NextEDRsvd : 4;
    UINT32  NextED : 28;
} OHCI_ED_HW;

typedef struct _OHCI_ED_SW {
    OHCI_ED_HW  EdHw;
    UINT32      PreEDSoft;
    UINT32      NextEDSoft;
    UINT32      Rezvd1;
    UINT32      Rezvd2;
} OHCI_ED_SW;

typedef struct _OHCI_TD_HW {
    UINT32  Rsvd : 18;
    UINT32  BufferRounding : 1;
    UINT32  DirectionPID : 2;
    UINT32  DelayInterrupt : 3;
    UINT32  DataToggle : 2;
    UINT32  ErrorCount : 2;
    UINT32  ConditionCode : 4;
    UINT32  CurrentBufferPointer ;
    UINT32  NextTDRsvd : 4;
    UINT32  NextTD : 28;
    UINT32  BufferEnd;
} OHCI_TD_HW;

typedef struct _OHCI_TD_SW {
    OHCI_TD_HW  TdHw;
    UINT8       *Data;
    UINT32      NextTDSoft;
    UINT16      BufferLength;
    UINT16      Rezvd;
} OHCI_TD_SW;

typedef struct _HCCA_AREA {
    UINT32     HccaInterruptEDsPointers[32];
    UINT16     HccaFrameNumber;
    UINT16     HccaPad1;
    UINT32     HccaDoneHead;
    UINT8      HccaRsv[120];
} HCCA_AREA;
#pragma pack()

enum {
    OHCI_ASYNC_INT_SIGNATURE = SIGNATURE_32 ('o', 'h', 'c', 'a'),

};

//
// Structure to return the result of OHCI ED execution.
// Result is the final result of the ED's TD. NextToggle
// is the next data toggle to use. Complete is the actual
// length of data transferred.
//
typedef struct {
    UINT32                  Result;
    UINT8                   NextToggle;
    UINTN                   Complete;
} OHCI_ED_RESULT;

typedef struct _OHCI_ASYNC_REQUEST     OHCI_ASYNC_REQUEST;


//
// Structure used to manager the asynchronous interrupt transfers.
//
typedef struct _OHCI_ASYNC_REQUEST {
    UINTN                           Signature;
    LIST_ENTRY                      Link;

    //
    // Endpoint attributes
    //
    UINT8                           DevAddr;
    UINT8                           EndPoint;
    UINTN                           Interval;

    //
    // Data and OHC structures
    //
    OHCI_ED_SW                      *EdSw;
    OHCI_TD_SW                      *FirstTd;
    UINT8                           *Data;      // Allocated host memory, not mapped memory
    UINTN                           DataLen;
    UINTN                           RequestType;

    //
    // User callback and its context
    //
    EFI_ASYNC_USB_TRANSFER_CALLBACK Callback;
    VOID                            *Context;
} OHCI_ASYNC_REQUEST;

#define OHCI_ASYNC_INT_FROM_LINK(a) \
          CR (a, OHCI_ASYNC_REQUEST, Link, OHCI_ASYNC_INT_SIGNATURE)


#define OHC_FROM_LINK(This)           CR(This, USB_OHCI_DEV, Link, USB_OHCI_DEV_SIGNATURE)
#define OHC_FROM_USB_HC_PROTO(This)   CR(This, USB_OHCI_DEV, UsbHc, USB_OHCI_DEV_SIGNATURE)
#define OHC_FROM_USB2_HC_PROTO(This)  CR(This, USB_OHCI_DEV, Usb2Hc, USB_OHCI_DEV_SIGNATURE)

//
// USB_OHCI_DEV support the OHCI hardware controller. It schedules
// the asynchronous interrupt transfer with the same method as
// EHCI: a reversed tree structure. For synchronous interrupt,
// control and bulk transfer, it uses two static endpoint descriptor to
// schedule them.
//
typedef struct _USB_OHCI_DEV {
    UINT32                    Signature;
    LIST_ENTRY                Link;
    EFI_USB_HC_PROTOCOL       UsbHc;
    EFI_USB2_HC_PROTOCOL      Usb2Hc;
    UINT32                    UsbMemBase;
    EFI_PCI_FUNCTION_ADDRESS  PciAddress;
    EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

    //
    // Schedule data structures
    //
    UINT32                    *HccaBase;
    OHCI_ED_SW                *HccaIntEDList;
    OHCI_ED_SW                *HccaCtrlEDHead;
    OHCI_ED_SW                *HccaBulkEDHead;

    //
    // Structures to maintain asynchronus interrupt transfers.
    //
    LIST_ENTRY                AsyncIntList;

    UINTN                     RootPorts;
} USB_OHCI_DEV;

/******************************************************************************
    Function defintions
/*****************************************************************************/
UINT32
OhciReadOpReg (
    IN USB_OHCI_DEV           *Ohc,
    IN UINT32                 Offset
)
/*++

Routine Description:

  Read a OHCI Operational register

Arguments:

  Ohc      - The OHCI device
  Offset   - Register offset

Returns:

  Content of register

--*/
;

VOID
OhciWriteOpReg (
    IN USB_OHCI_DEV           *Ohc,
    IN UINT32                 Offset,
    IN UINT32                 Data
)
/*++

Routine Description:

  Write data to OHCI register

Arguments:

  Ohc      - The OHCI device
  Offset   - Register offset to USB_BAR_INDEX
  Data     - Data to write

Returns:

  VOID

--*/
;

VOID
OhciSetOpRegBit (
    IN  USB_OHCI_DEV          *Ohc,
    IN UINT32                 Offset,
    IN UINT32                 Bit
)
/*++

Routine Description:

  Set a bit of the OHCI Register

Arguments:

  Ohc        - The OHCI device
  Offset     - Register offset to USB_BAR_INDEX
  Bit        - The bit to set

Returns:

  None

--*/
;

VOID
OhciClearOpRegBit (
    IN  USB_OHCI_DEV          *Ohc,
    IN UINT32                 Offset,
    IN UINT32                 Bit
)
/*++

Routine Description:

  Clear a bit of the OHCI Register

Arguments:

  Ohc        - The OHCI device
  Offset     - Register offset to USB_BAR_INDEX
  Bit        - The bit to clear
Returns:

  None

--*/
;

VOID
OhciFreeAllAsyncReq (
    IN USB_OHCI_DEV           *Ohc
)
/*++

Routine Description:

  Release all the asynchronous transfers on the lsit.

Arguments:

  Ohc   - The OHCI device

Returns:

  VOID

--*/
;

EFI_STATUS
OhcInitHc (
    IN USB_OHCI_DEV           *Ohc
)
/*++

Routine Description:

  Initialize the HC hardware.


Arguments:

  Ohc - The OHCI device

Returns:

  EFI_SUCCESS : The EHCI has come out of halt state
  EFI_TIMEOUT : Time out happened

--*/
;

VOID
OhciAckAllInterrupt (
    IN  USB_OHCI_DEV          *Ohc
)
/*++

Routine Description:

  Clear all the interrutp status bits, these bits
  are Write-Clean

Arguments:

  Ohc - The OHCI device

Returns:

  None

--*/
;

BOOLEAN
OhciIsHcWorking (
    IN USB_OHCI_DEV           *Ohc
)
/*++

Routine Description:

  Check whether the host controller operates well

Arguments:

  Ohc     - The U=OHCI device

Returns:

   TRUE  -  Host controller is working
   FALSE -  Host controller is halted or system error

--*/
;

EFI_STATUS
OhciMapUserData (
    IN  USB_OHCI_DEV              *Ohc,
    IN  EFI_USB_DATA_DIRECTION    Direction,
    IN  VOID                      *Data,
    IN  OUT UINTN                 *Len,
    OUT UINT8                     *PktId,
    OUT UINT8                     **MappedAddr
)
/*++

Routine Description:

  Map address of user data buffer

Arguments:

  Ohc        - The OHCI device
  Direction  - direction of the data transfer
  Data       - The user data buffer
  Len        - Length of the user data
  PktId      - Packet identificaion
  MappedAddr - mapped address to return
  Map        - identificaion of this mapping to return

Returns:

  EFI_SUCCESS      : Success
  EFI_DEVICE_ERROR : Fail to map the user data

--*/
;


OHCI_TD_SW *
OhciCreateCtrlTds (
    IN USB_OHCI_DEV           *Ohc,
    IN UINT8                  DeviceAddr,
    IN UINT8                  DataPktId,
    IN UINT8                  *Request,
    IN UINT8                  *Data,
    IN UINTN                  DataLen,
    IN UINT8                  MaxPacket
)
/*++

Routine Description:

  Create Tds list for Control Transfer

Arguments:

  Ohc         - The OHCI device
  DeviceAddr  - The device address
  DataPktId   - Packet Identification of Data Tds
  Request     - A pointer to request structure buffer to transfer
  Data        - A pointer to user data buffer to transfer
  DataLen     - Length of user data to transfer
  MaxPacket   - Maximum packet size for control transfer

Returns:

  The Td list head for the control transfer

--*/
;

OHCI_ED_SW *
OhciCreateEd (
    IN  USB_OHCI_DEV          *Ohc,
    IN  UINT8                 DeviceAddress,
    IN  UINT8                 EndpointNumber,
    IN  BOOLEAN               IsSlowDevice,
    IN  BOOLEAN               Format,
    IN  UINT8                 MaxPacketLength,
    IN  UINT8                 Direction
)
/*++

Routine Description:

  Create an initialize a new queue head

Arguments:

  Ohc       - The OHCI device

Returns:

  The newly created ED

--*/
;

VOID
OhciLinkTdToEd (
    IN OHCI_ED_SW             *Ed,
    IN OHCI_TD_SW             *Td
)
/*++

Routine Description:

  Link the TD To ED

Arguments:

  Ed  - The Endpoint Descriptor for the TD to link to
  Td  - The TD to link

Returns:

  VOID

--*/
;

VOID
OhciLinkEdToEd (
    IN OHCI_ED_SW             *ED,
    IN OHCI_ED_SW             *NextED
)
/*++

  Routine Description:

    Set Next ED link of Endpoint Descriptor

  Arguments:

    ED       : A pointer of ED_STRUCT
    NextED   : Next Ed

  Returns:

    None

--*/
;

EFI_STATUS
OhciExecuteTransfer (
    IN  USB_OHCI_DEV          *Ohc,
    IN  OHCI_ED_SW            *Ed,
    IN  OHCI_TD_SW            *Td,
    IN  UINTN                 TimeOut,
    OUT OHCI_ED_RESULT        *EdResult
)
/*++

Routine Description:

  Check the result of the transfer

Arguments:

  Ohc            - The OHCI device
  Ed             - The endpoint descriptor of the transfer
  Td             - The first TDs of the transfer
  TimeOut        - TimeOut value in milliseconds
  EdResult       - The variable to return result

Returns:

  EFI_SUCCESS      - The transfer finished with success
  EFI_DEVICE_ERROR - Transfer failed

--*/
;


VOID
OhciUnlinkTdFromEd (
    IN OHCI_ED_SW             *Ed,
    IN OHCI_TD_SW             *Td
)
/*++

Routine Description:

  Unlink TD from the Ed

Arguments:

  Ed  - The endpoint descriptor to unlink from
  Td  - The TD to unlink

Returns:

  VOID

--*/
;

VOID
OhciDestoryTds (
    IN USB_OHCI_DEV           *Ohc,
    IN OHCI_TD_SW             *FirstTd
)
/*++
Routine Description:

  Delete a list of TDs

Arguments:

  Ohc       - The OHCI device
  FirstTd   - TD link list head

Returns:

  VOID

--*/
;

OHCI_TD_SW *
OhciCreateBulkOrIntTds (
    IN USB_OHCI_DEV           *Ohc,
    IN UINT8                  PktId,
    IN UINT8                  *Data,
    IN UINTN                  DataLen,
    IN OUT UINT8              *DataToggle,
    IN UINT8                  MaxPacket,
    IN BOOLEAN                IsIOCSet
)
/*++

Routine Description:

  Create Tds list for Bulk/Interrupt Transfer

Arguments:

  Ohc          - USB_OHCI_DEV
  PktId        - Packet Identification of Data Tds
  Data         - A pointer to user data buffer to transfer
  DataLen      - Length of user data to transfer
  DataToggle   - Data Toggle Pointer
  MaxPacket    - Maximum packet size for Bulk/Interrupt transfer
  IsIOCSet     - Interrupt delay count for this TD.

Returns:

  The Tds list head for the bulk transfer

--*/
;

EFI_STATUS
OhciRemoveAsyncReq (
    IN  USB_OHCI_DEV                      *Ohc,
    IN  UINT8                             DevAddr,
    IN  UINT8                             EndPoint,
    IN  EFI_ASYNC_USB_TRANSFER_CALLBACK   CallBackFunction,
    IN  VOID                              *Context,
    OUT UINT8                             *Toggle
)
/*++
Routine Description:

  Delete Async Interrupt ED and TDs

Arguments:

  Ohc         - The OHCI device
  DevAddr     - Device Address
  EndPoint    - EndPoint Address
  Toggle      - The next data toggle to use

Returns:

  EFI_SUCCESS            - The request is deleted
  EFI_INVALID_PARAMETER  - Paremeter is error
  EFI_NOT_FOUND          - The asynchronous isn't found

--*/
;

EFI_STATUS
OhciCreateAsyncReq (
    IN USB_OHCI_DEV                       *Ohc,
    IN OHCI_ED_SW                         *Ed,
    IN OHCI_TD_SW                         *FirstTd,
    IN UINT8                              DevAddr,
    IN UINT8                              EndPoint,
    IN UINTN                              DataLen,
    IN UINTN                              Interval,
    IN UINT8                              *Data,
    IN EFI_ASYNC_USB_TRANSFER_CALLBACK    Callback,
    IN VOID                               *Context,
    IN BOOLEAN                            IsIntOnly
)
/*++

Routine Description:

  Create Async Request node, and Link to List

Arguments:

  Ohc         - The OHCI device
  Ed          - The endpoint descriptor of the transfer
  FirstTd     - First TD of the transfer
  DevAddr     - Device Address
  EndPoint    - EndPoint Address
  DataLen     - Data length
  Interval    - Polling Interval when inserted to frame list
  Mapping     - Mapping value
  Data        - Data buffer, unmapped
  Callback    - Callback after interrupt transfeer
  Context     - Callback Context passed as function parameter

Returns:

  EFI_SUCCESS            - An asynchronous transfer is created
  EFI_INVALID_PARAMETER  - Paremeter is error
  EFI_OUT_OF_RESOURCES   - Failed because of resource shortage.

--*/
;

VOID
OhciLinkEDToIntTree(
    IN USB_OHCI_DEV           *Ohc,
    IN OHCI_ED_SW             *Ed,
    IN UINTN                  PollingInterval
)
/*++

  Routine Description:

    Insert Endpoint Descriptor to interrupt list

  Arguments:

    HcDev         : The OHCI device
    UINT8         : PollingInterval
    ED         : A pointer of OHCI_ED_SW

  Returns:

    None

--*/
;

VOID
EFIAPI
OhciProcessAsyncReqList (
    IN USB_OHCI_DEV            *Ohc
)
/*++

  Routine Description:

    Interrupt transfer periodic check handler

  Arguments:

    ohc   : The OHCI device

  Returns:

    VOID

--*/
;

VOID
OhciUnlinkEdFromEd (
    IN OHCI_ED_SW           *Ed,
    IN OHCI_ED_SW           *NextEd
)
/*++

Routine Description:

  Unlink Ed from the Ed

Arguments:

  Ed  - The endpoint descriptor to unlink from
  NextEd  - The Ed to unlink

Returns:

  VOID

--*/
;

VOID
OhciTurnOffUsbEmulation (
    IN USB_OHCI_DEV           *Ohc
)
/*++

  Routine Description:

    Disable USB Emulation

  Arguments:

    Ohc     - The OHCI device

  Returns:

    VOID

--*/
;

EFI_STATUS
OhciEnableDevice (
    IN  USB_OHCI_DEV          *Ohc
)
/*++

Routine Description:

  Enable device by setting IO, memory and bus master bit
  in PCI command register

Arguments:

  Ohc        - The OHCI device

Returns:

   EFI_SUCCESS - The devcie is enabled
   EFI_TIMEOUT - Failed to enable the device

--*/
;

VOID
OhciFreeDev (
    IN USB_OHCI_DEV           *Ohc
)
/*++

Routine Description:

  Free the OHCI device and release its associated resources

Arguments:

  Ohc - The OHCI device to release

Returns:

  None

--*/
;


VOID
OhciUnlinkAsyncReq (
    IN USB_OHCI_DEV           *Ohc,
    IN OHCI_ASYNC_REQUEST     *AsyncReq
)
/*++
Routine Description:

  Unlink an asynchronous request's from OHC's asynchronus list.
  also remove the endpoint descriptor.

Arguments:

  Ohc         - The OHCI device
  AsyncReq    - The asynchronous request to free

Returns:

  None

--*/
;

EFI_STATUS
EFIAPI
UsbConnectOhciDriver (
    IN EFI_PCI_FUNCTION_ADDRESS   *PciAddress,
    OUT EFI_USB2_HC_PROTOCOL      **Usb2HCInterface,
    OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
)
/*++

  Routine Description:

    Starting the Usb OHCI Driver

  Arguments:

    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:

    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.
    EFI_DEVICE_ERROR    - This driver cannot be started due to device Error
    EFI_OUT_OF_RESOURCES- Failed due to resource shortage

--*/
;


EFI_STATUS
EFIAPI
UsbDisconnectOhciDriver (
    IN EFI_USB2_HC_PROTOCOL     *Usb2HCInterface
)
/*++

  Routine Description:

    Stop this driver on ControllerHandle. Support stoping any child handles
    created by this driver.

  Arguments:

    This              - Protocol instance pointer.
    Controller        - Handle of device to stop driver on
    NumberOfChildren  - Number of Children in the ChildHandleBuffer
    ChildHandleBuffer - List of handles for the children we need to stop.

  Returns:

    EFI_SUCCESS
    others

--*/
;


EFI_STATUS
EFIAPI
GetOhcInfo (
    IN EFI_PCI_FUNCTION_ADDRESS *PciAddress,
    IN VOID                     **SmmUhc,
    IN VOID                     **SmmUsbHc,
    IN VOID                     **SmmUsb2Hc
)
;

EFI_STATUS
EFIAPI
OhciReset (
    IN EFI_USB_HC_PROTOCOL     *This,
    IN UINT16                  Attributes
)
;

EFI_STATUS
EFIAPI
OhciGetState (
    IN  EFI_USB_HC_PROTOCOL     *This,
    OUT EFI_USB_HC_STATE        *State
)
;

EFI_STATUS
EFIAPI
OhciSetState (
    IN EFI_USB_HC_PROTOCOL     *This,
    IN EFI_USB_HC_STATE        State
)
;

EFI_STATUS
EFIAPI
OhciGetRootHubPortNumber (
    IN  EFI_USB_HC_PROTOCOL     *This,
    OUT UINT8                   *PortNumber
)
;

EFI_STATUS
EFIAPI
OhciGetRootHubPortStatus (
    IN  EFI_USB_HC_PROTOCOL     *This,
    IN  UINT8                   PortNumber,
    OUT EFI_USB_PORT_STATUS     *PortStatus
)
;

EFI_STATUS
EFIAPI
OhciSetRootHubPortFeature (
    IN  EFI_USB_HC_PROTOCOL     *This,
    IN  UINT8                   PortNumber,
    IN  EFI_USB_PORT_FEATURE    PortFeature
)
;

EFI_STATUS
EFIAPI
OhciClearRootHubPortFeature (
    IN  EFI_USB_HC_PROTOCOL     *This,
    IN  UINT8                   PortNumber,
    IN  EFI_USB_PORT_FEATURE    PortFeature
)
;

EFI_STATUS
EFIAPI
OhciControlTransfer (
    IN       EFI_USB_HC_PROTOCOL        *This,
    IN       UINT8                      DeviceAddress,
    IN       BOOLEAN                    IsSlowDevice,
    IN       UINT8                      MaximumPacketLength,
    IN       EFI_USB_DEVICE_REQUEST     *Request,
    IN       EFI_USB_DATA_DIRECTION     TransferDirection,
    IN OUT   VOID                       *Data,              OPTIONAL
    IN OUT   UINTN                      *DataLength,        OPTIONAL
    IN       UINTN                      TimeOut,
    OUT      UINT32                     *TransferResult
)
;

EFI_STATUS
EFIAPI
OhciBulkTransfer (
    IN       EFI_USB_HC_PROTOCOL     *This,
    IN       UINT8                   DeviceAddress,
    IN       UINT8                   EndPointAddress,
    IN       UINT8                   MaximumPacketLength,
    IN OUT   VOID                    *Data,
    IN OUT   UINTN                   *DataLength,
    IN OUT   UINT8                   *DataToggle,
    IN       UINTN                   TimeOut,
    OUT      UINT32                  *TransferResult
)
;

EFI_STATUS
EFIAPI
OhciAsyncInterruptOnlyTransfer (
    IN     EFI_USB_HC_PROTOCOL                * This,
    IN     BOOLEAN                            IsNewTransfer,
    IN     UINTN                              PollingInterval,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
    IN     VOID                               *Context
)
;

EFI_STATUS
EFIAPI
OhciAsyncInterruptTransfer (
    IN     EFI_USB_HC_PROTOCOL                * This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     BOOLEAN                            IsSlowDevice,
    IN     UINT8                              MaximumPacketLength,
    IN     BOOLEAN                            IsNewTransfer,
    IN OUT UINT8                              *DataToggle,
    IN     UINTN                              PollingInterval,    OPTIONAL
    IN     UINTN                              DataLength,         OPTIONAL
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,   OPTIONAL
    IN     VOID                               *Context OPTIONAL
)
;

EFI_STATUS
EFIAPI
OhciSyncInterruptTransfer (
    IN       EFI_USB_HC_PROTOCOL     *This,
    IN       UINT8                   DeviceAddress,
    IN       UINT8                   EndPointAddress,
    IN       BOOLEAN                 IsSlowDevice,
    IN       UINT8                   MaximumPacketLength,
    IN OUT   VOID                    *Data,
    IN OUT   UINTN                   *DataLength,
    IN OUT   UINT8                   *DataToggle,
    IN       UINTN                   TimeOut,
    OUT      UINT32                  *TransferResult
)
;

EFI_STATUS
EFIAPI
OhciIsochronousTransfer (
    IN       EFI_USB_HC_PROTOCOL     *This,
    IN       UINT8                   DeviceAddress,
    IN       UINT8                   EndPointAddress,
    IN       UINT8                   MaximumPacketLength,
    IN OUT   VOID                    *Data,
    IN       UINTN                   DataLength,
    OUT      UINT32                  *TransferResult
)
;

EFI_STATUS
EFIAPI
OhciAsyncIsochronousTransfer (
    IN       EFI_USB_HC_PROTOCOL                 * This,
    IN       UINT8                               DeviceAddress,
    IN       UINT8                               EndPointAddress,
    IN       UINT8                               MaximumPacketLength,
    IN OUT   VOID                                *Data,
    IN       UINTN                               DataLength,
    IN       EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
    IN       VOID                                *Context OPTIONAL
)
;

EFI_STATUS
EFIAPI
Ohci2Reset (
    IN EFI_USB2_HC_PROTOCOL   *This,
    IN UINT16                 Attributes
)
;

EFI_STATUS
EFIAPI
Ohci2GetState (
    IN  EFI_USB2_HC_PROTOCOL   *This,
    OUT EFI_USB_HC_STATE       *State
)
;

EFI_STATUS
EFIAPI
Ohci2SetState (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN EFI_USB_HC_STATE        State
)
;

EFI_STATUS
EFIAPI
Ohci2GetCapability (
    IN  EFI_USB2_HC_PROTOCOL  *This,
    OUT UINT8                 *MaxSpeed,
    OUT UINT8                 *PortNumber,
    OUT UINT8                 *Is64BitCapable
)
;

EFI_STATUS
EFIAPI
Ohci2GetRootHubPortStatus (
    IN  EFI_USB2_HC_PROTOCOL   *This,
    IN  UINT8                  PortNumber,
    OUT EFI_USB_PORT_STATUS    *PortStatus
)
;

EFI_STATUS
EFIAPI
Ohci2SetRootHubPortFeature (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN UINT8                   PortNumber,
    IN EFI_USB_PORT_FEATURE    PortFeature
)
;

EFI_STATUS
EFIAPI
Ohci2ClearRootHubPortFeature (
    IN EFI_USB2_HC_PROTOCOL    *This,
    IN UINT8                   PortNumber,
    IN EFI_USB_PORT_FEATURE    PortFeature
)
;

EFI_STATUS
EFIAPI
Ohci2ControlTransfer (
    IN     EFI_USB2_HC_PROTOCOL                 *This,
    IN     UINT8                                DeviceAddress,
    IN     UINT8                                DeviceSpeed,
    IN     UINTN                                MaximumPacketLength,
    IN     EFI_USB_DEVICE_REQUEST               *Request,
    IN     EFI_USB_DATA_DIRECTION               TransferDirection,
    IN OUT VOID                                 *Data,
    IN OUT UINTN                                *DataLength,
    IN     UINTN                                TimeOut,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR   *Translator,
    OUT    UINT32                               *TransferResult
)
;

EFI_STATUS
EFIAPI
Ohci2BulkTransfer (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     UINT8                              DeviceSpeed,
    IN     UINTN                              MaximumPacketLength,
    IN     UINT8                              DataBuffersNumber,
    IN OUT VOID                               *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
    IN OUT UINTN                              *DataLength,
    IN OUT UINT8                              *DataToggle,
    IN     UINTN                              TimeOut,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    OUT    UINT32                             *TransferResult
)
;

EFI_STATUS
EFIAPI
Ohci2AsyncInterruptTransfer (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     UINT8                              DeviceSpeed,
    IN     UINTN                              MaximumPacketLength,
    IN     BOOLEAN                            IsNewTransfer,
    IN OUT UINT8                              *DataToggle,
    IN     UINTN                              PollingInterval,
    IN     UINTN                              DataLength,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
    IN     VOID                               *Context
)
;

EFI_STATUS
EFIAPI
Ohci2AsyncInterruptOnlyTransfer (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     BOOLEAN                            IsNewTransfer,
    IN     UINTN                              PollingInterval,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
    IN     VOID                               *Context
)
;

EFI_STATUS
EFIAPI
Ohci2SyncInterruptTransfer (
    IN     EFI_USB2_HC_PROTOCOL                      *This,
    IN     UINT8                                     DeviceAddress,
    IN     UINT8                                     EndPointAddress,
    IN     UINT8                                     DeviceSpeed,
    IN     UINTN                                     MaximumPacketLength,
    IN OUT VOID                                      *Data,
    IN OUT UINTN                                     *DataLength,
    IN OUT UINT8                                     *DataToggle,
    IN     UINTN                                     TimeOut,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR        *Translator,
    OUT    UINT32                                    *TransferResult
)
;

EFI_STATUS
EFIAPI
Ohci2IsochronousTransfer (
    IN     EFI_USB2_HC_PROTOCOL               *This,
    IN     UINT8                              DeviceAddress,
    IN     UINT8                              EndPointAddress,
    IN     UINT8                              DeviceSpeed,
    IN     UINTN                              MaximumPacketLength,
    IN     UINT8                              DataBuffersNumber,
    IN OUT VOID                               *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN     UINTN                              DataLength,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
    OUT    UINT32                             *TransferResult
)
;

EFI_STATUS
EFIAPI
Ohci2AsyncIsochronousTransfer (
    IN     EFI_USB2_HC_PROTOCOL                *This,
    IN     UINT8                               DeviceAddress,
    IN     UINT8                               EndPointAddress,
    IN     UINT8                               DeviceSpeed,
    IN     UINTN                               MaximumPacketLength,
    IN     UINT8                               DataBuffersNumber,
    IN OUT VOID                                *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
    IN     UINTN                               DataLength,
    IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    IN     EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
    IN     VOID                                *Context
)
;

VOID
OhciDumpEd (
    IN OHCI_ED_SW         *EdSw
)
/*++

Routine Description:

  Dump the content of QH structure

Arguments:

  EdSw  - Pointer to software ED structure

Returns:

  None

--*/
;

VOID
OhciDumpTds (
    IN OHCI_TD_SW           *TdSw
)
/*++

Routine Description:

  Dump the content of TD structure.

Arguments:

  TdSw  - Pointer to software TD structure

Returns:

  None

--*/
;

BOOLEAN
IsLegacySupport (
  EFI_USB2_HC_PROTOCOL *This
);

#endif
