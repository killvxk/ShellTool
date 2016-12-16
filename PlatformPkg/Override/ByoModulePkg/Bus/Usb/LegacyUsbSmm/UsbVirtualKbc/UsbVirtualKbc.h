/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  UsbVirtualKbc.h

Abstract:


Revision History:

**/
#ifndef _USB_VIRTUAL_KBC_H_
#define _USB_VIRTUAL_KBC_H_

#include <Protocol/VirtualKbc.h>
#include <Protocol/UsbPolicy.h>
#include <Protocol/SmmCpu.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>

#include "../UsbLib/UsbLib.h"
#include "../UsbBus/UsbBus.h"
#include "../UsbKb/UsbKeyboard.h"
#include "../Ohci/Ohci.h"
#include "../Uhci/Uhci.h"
#include "../LegacyUsb.h"

//
//  Keyboard Controller Command Register
//
#define  KBC_CMD_RSVD           0x80
#define  KBC_CMD_XLAT           0x40
#define  KBC_CMD_DISMS          0x20
#define  KBC_CMD_DISKB          0x10
#define  KBC_CMD_OVR            0x08
#define  KBC_CMD_SYS            0x04
#define  KBC_CMD_INT2           0x02
#define  KBC_CMD_INT            0x01

//
//  Keyboard Controller Command
//
#define  KBC_READCMD_BYTE       0x20
#define  KBC_WRITCMD_BYTE       0x60
#define  KBC_GETVERS_NUMB       0xA1
#define  KBC_GETPASS_BYTE       0xA4
#define  KBC_SETPASS_BYTE       0xA5
#define  KBC_CHKPASS_BYTE       0xA6
#define  KBC_DISMOUSE_INF       0xA7
#define  KBC_ENAMOUSE_INF       0xA8
#define  KBC_MOUSEIF_TEST       0xA9
#define  KBC_CRLSELF_TEST       0xAA
#define  KBC_KEYBDIF_TEST       0xAB
#define  KBC_DISKEYBD_INF       0xAD
#define  KBC_ENAKEYBD_INF       0xAE
#define  KBC_READ_VERSION       0xAF
#define  KBC_READINP_PORT       0xC0
#define  KBC_READILS_PORT       0xC1
#define  KBC_READIMS_PORT       0xC2
#define  KBC_READOUT_PORT       0xD0
#define  KBC_WRITOUT_PORT       0xD1
#define  KBC_WRITKBB_BYTE       0xD2
#define  KBC_WRITMSB_BYTE       0xD3
#define  KBC_WRITMSD_BYTE       0xD4
#define  KBC_READTST_PORT       0xE0
#define  KBC_RESET_SYSTEM       0xFE

//
//  Keyboard Controller OutPut port definition
//
#define  KBC_OUTP_KBDATA        0x80
#define  KBC_OUTP_KBCLOCK       0x40
#define  KBC_OUTP_MSIBFIRQ      0x20
#define  KBC_OUTP_KBIBFIRQ      0x10
#define  KBC_OUTP_MSCLOCK       0x08
#define  KBC_OUTP_MSDATA        0x04
#define  KBC_OUTP_GATEA20       0x02
#define  KBC_OUTP_SYSRESET      0x01

//
//  Keyboard Controller Status Register
//
#define  KBC_STS_PERR           0x80
#define  KBC_STS_TO             0x40
#define  KBC_STS_MOBF           0x20
#define  KBC_STS_INH            0x10
#define  KBC_STS_A2             0x08
#define  KBC_STS_SYS            0x04
#define  KBC_STS_IBF            0x02
#define  KBC_STS_OBF            0x01
#define  KBC_STS_ERR0           0
#define  KBC_STS_ERR1           0xFF

//
//  KBC Command Respond
//
#define  KBC_ACK_STATUS         0xFA
#define  KBC_ACK1_STATUS        0xAA
#define  KBC_ACK2_STATUS        0x00
#define  KBC_RESET_STATUS       0xFE
#define  KBC_ECHO_STATUS        0xEE

//
//  MsMode Register
//
#define  MSMODE_RSVD           0x80
#define  MSMODE_REMOTE         0x40
#define  MSMODE_ENMS           0x20
#define  MSMODE_SCAL12         0x10
#define  MSMODE_RSVD0          0x08
#define  MSMODE_LBTN           0x04
#define  MSMODE_MBTN           0x02
#define  MSMODE_RBTN           0x01

//
//  PS2 Keyboard Command Set
//
#define  KB_SET_LED             0xED
#define  KB_ECHO_RESP           0xEE
#define  KB_SET_SCANCODE        0xF0
#define  KB_READ_ID             0xF2
#define  KB_SET_TYPEMATIC       0xF3
#define  KB_ENABLE_SCANING      0xF4
#define  KB_DISABLE_SCANING     0xF5
#define  KB_SET_DEFAULT         0xF6
#define  KB_SETAKEY_TYPE        0xF7
#define  KB_SETAKEY_BREAK       0xF8
#define  KB_SETAKEY_MAKE        0xF9
#define  KB_SETAKEY_TMB         0xFA
#define  KB_SETKEY_TYPE         0xFB
#define  KB_SETKEY_BREAK        0xFC
#define  KB_SETKEY_MAKE         0xFD
#define  KB_RESEND_CMD          0xFE
#define  KB_RESET_CMD           0xFF

//
//  PS2 Mouse Command Set
//
#define  MS_SET_SCALING21       0xE6
#define  MS_SET_SCALING11       0xE7
#define  MS_SET_RESOLOTION      0xE8
#define  MS_READ_STATUS         0xE9
#define  MS_SET_STREAMMODE      0xEA
#define  MS_READ_DATA           0xEB
#define  MS_RESET_WRAPMODE      0xEC
#define  MS_SET_WRAPMODE        0xEE
#define  MS_SET_REMOTEMODE      0xF0
#define  MS_READ_ID             0xF2
#define  MS_SET_SAMPLERATE      0xF3
#define  MS_ENABLE_REPORT       0xF4
#define  MS_DISABLE_REPORT      0xF5
#define  MS_SET_DEFAULT         0xF6
#define  MS_RESEND_CMD          0xFE
#define  MS_RESET_CMD           0xFF
#define  MS_DEFAULT_SAMPLERATE  100
#define  MS_DEFAULT_RESOLUTION  2
#define  MS_DEFAULT_SCROLLING   0

#define  PORT_92                0x92
#define  PORT_92_FASTA20        0x02
#define  PORT_92_RESET          0x01

#define  KBC_CMD_PORT           0x64
#define  KBC_DATA_PORT          0x60

#define  KBC_INBUFFER_COUNTER         3000
#define  USB_VIRTUAL_KBC_INTERVAL     0x08
#define  BDA_LED                      0x0417

#define  KBC_NO_EMULATION       0x00
#define  KBC_PORT64_WRITE       0x08
#define  KBC_PORT64_READ        0x04
#define  KBC_PORT60_WRITE       0x02
#define  KBC_PORT60_READ        0x01
#define  KBC_KBMS_DATA          0x10
#define  KBC_DEBUG(a)  //USB_DEBUG(a)

#define GET_BOOT_CFG (BootCfg);\
 do {\
   UINT16                EBdaSeg;\
   EBdaSeg = *(UINT16 *)(UINTN)0x40E;\
   BootCfg  = (BOOT_CFG *)(UINTN)(((UINT32)EBdaSeg << 4) + 0x105);\
 } while(0);

typedef struct {
    UINT8  BootStart:1;
    UINT8  Reserved:7;
} BOOT_CFG;

typedef enum {
    Kbc,
    KbcKB,
    KbcMS
} KBC_DATA_TYPE;

typedef struct _USB_VIRTUAL_KBC_DATA {
    UINT8         VirtualKBC60Port;
    UINT8         VirtualKBC64Port;

    UINT8         MsMode;
    UINT8         MsArgNumber;
    UINT8         MsLastCmd;
    UINT8         MsSampleRate;
    UINT8         MsResolution;
    UINT8         MsScrolling     :2;
    UINT8         MsScrollingExt  :2;
    UINT8         MsScrollingID   :2;
    UINT8         MsScrollingCID  :2;

    UINT8         KbLastCmd;
    UINT8         KbArgNumber;
    UINT8         KbScanMode;
    UINT8         KbKeyType;
    UINT8         KbTypeRateDelay;

    UINT8         KBCCmdReg;
    UINT8         KBCLastCmd;
    UINT8         KBCOutPutPort;
    UINT8         KBCSendCmd;
    UINT8         KBCSendStatus;

    UINT8         KBCdataHead :4;
    UINT8         KBCdataTail :4;
    UINT8         KBdataHead  :4;
    UINT8         KBdataTail  :4;
    UINT8         MSdataHead  :4;
    UINT8         MSdataTail  :4;
    UINT8         KBCdata[16];
    UINT8         KBdata[16];
    UINT8         MSdata[16];
} USB_VIRTUAL_KBC_DATA;

typedef
EFI_STATUS
(*USB_VIRTUAL_KBC_ENABLE) (
    EFI_USB2_HC_PROTOCOL  *This,
    UINT8                 *SmiState
);

typedef
EFI_STATUS
(*USB_VIRTUAL_KBC_DISABLE) (
    EFI_USB2_HC_PROTOCOL  *This,
    UINT8                 *SmiState
);

typedef
EFI_STATUS
(*USB_VIRTUAL_KBC_CHECK) (
    EFI_USB2_HC_PROTOCOL       *This,
    OUT UINT8                  *SmiState
);

typedef
EFI_STATUS
(*USB_VIRTUAL_KBC_CLEAR) (
    EFI_USB2_HC_PROTOCOL       *This,
    IN UINT8                   *SmiState
);

typedef
EFI_STATUS
(*USB_VIRTUAL_KBC_CONTROL) (
    IN UINT8                   *Data,
    IN BOOLEAN                 RW
);

typedef
EFI_STATUS
(*USB_VIRTUAL_KBC_STATUS) (
    IN UINT8                   *Data,
    IN BOOLEAN                 RW
);

typedef
EFI_STATUS
(*USB_VIRTUAL_KBC_IRQ) (
    IN KBC_DATA_TYPE           Type
);

typedef
EFI_STATUS
(*USB_VIRTUAL_KBC_GETDATA) (
    IN UINT8                  *Data
);

typedef
EFI_STATUS
(*USB_VIRTUAL_KBC_SETDATA) (
    IN UINT8                  Data
);

typedef struct _USB_VIRTUAL_KBC_API {
    USB_VIRTUAL_KBC_ENABLE         VirtualEnable;
    USB_VIRTUAL_KBC_DISABLE        VirtualDisable;
    USB_VIRTUAL_KBC_CHECK          VirtualCheckStatus;
    USB_VIRTUAL_KBC_CLEAR          VirtualClearStatus;
    USB_VIRTUAL_KBC_CONTROL        VirtualControl;
    USB_VIRTUAL_KBC_STATUS         VirtualStatus;
    USB_VIRTUAL_KBC_IRQ            VirtualIRQ;
    USB_VIRTUAL_KBC_GETDATA        VirtualGetData;
    USB_VIRTUAL_KBC_SETDATA        VirtualSetData;
} USB_VIRTUAL_KBC_API;

typedef struct _USB_VIRTUAL_KBC_DEV {
    UINT32                    Signature;
    UINT8                     VirtualFlag;
    UINT8                     BusType;
    EFI_USB2_HC_PROTOCOL      *Usb2Hc;
    USB_VIRTUAL_KBC_API       *VirtualKBCApi;
    BOOLEAN                   IsKbcPresent;
    BOOLEAN                   IsKBPresent;
    BOOLEAN                   IsMSPresent;
    BOOLEAN                   IsTimerPresent;
    USB_VIRTUAL_KBC_DATA      VirtualKbcData;
} USB_VIRTUAL_KBC_DEV;

extern USB_VIRTUAL_KBC_API mUsbLPCAPI;
extern USB_VIRTUAL_KBC_API mUsbUhciAPI;
extern USB_VIRTUAL_KBC_API mUsbOhciAPI;

VOID
UsbVirtualKbcTimer (IN  BOOLEAN   IsNewTransfer);

UINT8
SmmKbcIoRead8(IN  UINT64    Address);

UINT8
SmmKbcIoWrite8 (
    IN  UINT64    Address,
    IN  UINT8     Data);

VOID
UsbDevSendData (
    IN  UINT8   Type,
    IN  UINT8   *Data
);

#endif
