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
  UsbKeyboard.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "../Include/UsbIo.h"

typedef struct _USB_INTERFACE     USB_INTERFACE;
//
// Driver Consumed Protocol Prototypes
//
#include <Protocol/DevicePath.h>
#include <Protocol/StatusCode.h>

#define MAX_KEY_ALLOWED         0x20
#define USBKBD_REPEAT_DELAY     0x200
#define USBKBD_REPEAT_RATE      0x40
#define USBKBD_RECOVERY_DELAY   0x200
#define USBKBD_KBC_CMD_INTERVAL 0x10

#define CLASS_HID               3
#define SUBCLASS_BOOT           1
#define PROTOCOL_KEYBOARD       1

#define BOOT_PROTOCOL           0
#define REPORT_PROTOCOL         1

typedef struct {
    UINT8     Command;
    UINT8     Data;
} KBC_COMMAND;

#define  BUFFER_SIZE                8
#define  KBD_COMMAND_QUEUE_LENGTH   16
#define  KBD_BREAK_CODE_MARK        0x80
#define  KBD_MULT_KEY_MARK          0x80
#define  KEB_MULT_KEY_FILTER        0x7F
#define  KBD_MOUSE_DATA             0xD3
#define  KBD_KEYBOARD_DATA          0xD2
#define  AT_MULTI_KEY_MARK          0x80
#define  KBD_IN_EMPTY_COUNTER       3000
#define  KBD_DISABLE_AUX            0xA7
#define  KBD_ENABLE_AUX             0xA8
#define  KBD_CMD                    0x64
#define  KBD_DATA_PORT              0x60
#define  KBD_IBE                    0x02
#define  KBD_OBF                    0x01
#define  BDA_NUMBERLOCK             0x20
#define  BDA_LED_PTR                0x417

#define USB_KB_DEV_SIGNATURE  SIGNATURE_32 ('u', 'k', 'b', 'd')

typedef struct {
    UINTN                         Signature;
    EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
    EFI_USB_IO_PROTOCOL           *UsbIo;
    EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
    EFI_USB_ENDPOINT_DESCRIPTOR   IntEndpointDescriptor;

    UINT8                         NumLockOn;
    UINT8                         CapsOn;
    UINT8                         ScrollOn;
    UINT8                         LastKeyCodeArray[8];
    UINT8                         RepeatKey;
    KBC_COMMAND                   KbdCommandQueue[KBD_COMMAND_QUEUE_LENGTH];
    UINT8                         CmdQueueHead;
    UINT8                         CmdQueueTail;
    BOOLEAN                       CmdQueuePollingActive;
    UINT8                         RepeatDelayCount;
    BOOLEAN                       RepeatKeyPollingActive;
    UINT8                         SendCmdBlockCount;
} USB_KB_DEV;

#define MOD_CONTROL_L           0x01
#define MOD_CONTROL_R           0x10
#define MOD_SHIFT_L             0x02
#define MOD_SHIFT_R             0x20
#define MOD_ALT_L               0x04
#define MOD_ALT_R               0x40
#define MOD_WIN_L               0x08
#define MOD_WIN_R               0x80

typedef struct {
    UINT8 Mask;
    UINT8 Key;
    UINT8 ScanCode;
} KB_MODIFIER;

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#define USB_KEYCODE_MAX_MAKE      0x7E
#else
#define USB_KEYCODE_MAX_MAKE      0x64
#endif

#define USBKBD_VALID_KEYCODE(key) ((UINT8) (key) > 3)

typedef struct {
    UINT8 NumLock : 1;
    UINT8 CapsLock : 1;
    UINT8 ScrollLock : 1;
    UINT8 Resrvd : 5;
} LED_MAP;

typedef struct {
    UINT8 Reserved : 4;
    UINT8 ScrollLock : 1;
    UINT8 NumLock : 1;
    UINT8 CapsLock : 1;
    UINT8 Insert : 1;
} BDA_LED_MAP;

#ifndef BIT0
#define BIT0           0x1
#endif
#ifndef BIT1
#define BIT1           0x2
#endif
#ifndef BIT2
#define BIT2           0x4
#endif
#ifndef BIT3
#define BIT3           0x8
#endif
#ifndef BIT4
#define BIT4          0x10
#endif
#ifndef BIT5
#define BIT5          0x20
#endif
#ifndef BIT6
#define BIT6          0x40
#endif
#ifndef BIT7
#define BIT7          0x80
#endif
#ifndef BIT8
#define BIT8         0x100
#endif
#ifndef BIT9
#define BIT9         0x200
#endif
#ifndef BIT10
#define BIT10        0x400
#endif
#ifndef BIT11
#define BIT11        0x800
#endif
#ifndef BIT12
#define BIT12       0x1000
#endif
#ifndef BIT13
#define BIT13       0x2000
#endif
#ifndef BIT14
#define BIT14       0x4000
#endif
#ifndef BIT15
#define BIT15       0x8000
#endif
#ifndef BIT16
#define BIT16      0x10000
#endif
#ifndef BIT17
#define BIT17      0x20000
#endif
#ifndef BIT18
#define BIT18      0x40000
#endif
#ifndef BIT19
#define BIT19      0x80000
#endif
#ifndef BIT20
#define BIT20     0x100000
#endif
#ifndef BIT21
#define BIT21     0x200000
#endif
#ifndef BIT22
#define BIT22     0x400000
#endif
#ifndef BIT23
#define BIT23     0x800000
#endif
#ifndef BIT24
#define BIT24    0x1000000
#endif
#ifndef BIT25
#define BIT25    0x2000000
#endif
#ifndef BIT26
#define BIT26    0x4000000
#endif
#ifndef BIT27
#define BIT27    0x8000000
#endif
#ifndef BIT28
#define BIT28   0x10000000
#endif
#ifndef BIT29
#define BIT29   0x20000000
#endif
#ifndef BIT30
#define BIT30   0x40000000
#endif
#ifndef BIT31
#define BIT31   0x80000000
#endif
#ifndef BIT32
#define BIT32          0x100000000
#endif
#ifndef BIT33
#define BIT33          0x200000000
#endif
#ifndef BIT34
#define BIT34          0x400000000
#endif
#ifndef BIT35
#define BIT35          0x800000000
#endif
#ifndef BIT36
#define BIT36         0x1000000000
#endif
#ifndef BIT37
#define BIT37         0x2000000000
#endif
#ifndef BIT38
#define BIT38         0x4000000000
#endif
#ifndef BIT39
#define BIT39         0x8000000000
#endif
#ifndef BIT40
#define BIT40        0x10000000000
#endif
#ifndef BIT41
#define BIT41        0x20000000000
#endif
#ifndef BIT42
#define BIT42        0x40000000000
#endif
#ifndef BIT43
#define BIT43        0x80000000000
#endif
#ifndef BIT44
#define BIT44       0x100000000000
#endif
#ifndef BIT45
#define BIT45       0x200000000000
#endif
#ifndef BIT46
#define BIT46       0x400000000000
#endif
#ifndef BIT47
#define BIT47       0x800000000000
#endif
#ifndef BIT48
#define BIT48      0x1000000000000
#endif
#ifndef BIT49
#define BIT49      0x2000000000000
#endif
#ifndef BIT50
#define BIT50      0x4000000000000
#endif
#ifndef BIT51
#define BIT51      0x8000000000000
#endif
#ifndef BIT52
#define BIT52     0x10000000000000
#endif
#ifndef BIT53
#define BIT53     0x20000000000000
#endif
#ifndef BIT54
#define BIT54     0x40000000000000
#endif
#ifndef BIT55
#define BIT55     0x80000000000000
#endif
#ifndef BIT56
#define BIT56    0x100000000000000
#endif
#ifndef BIT57
#define BIT57    0x200000000000000
#endif
#ifndef BIT58
#define BIT58    0x400000000000000
#endif
#ifndef BIT59
#define BIT59    0x800000000000000
#endif
#ifndef BIT60
#define BIT60   0x1000000000000000
#endif
#ifndef BIT61
#define BIT61   0x2000000000000000
#endif
#ifndef BIT62
#define BIT62   0x4000000000000000
#endif
#ifndef BIT63
#define BIT63   0x8000000000000000
#endif

#ifndef KILOBIT
#define KILOBIT           0x80
#endif
#ifndef MEGABIT
#define MEGABIT        0x20000
#endif

#ifndef KILOBYTE
#define KILOBYTE         0x400
#endif
#ifndef MEGABYTE
#define MEGABYTE      0x100000
#endif
#ifndef GIGABYTE
#define GIGABYTE    0x40000000
#endif
#ifndef TERABYTE
#define TERABYTE 0x10000000000
#endif

#ifndef __FAR
#define     __FAR
#endif

#define MK_FP(seg, offset) (void __FAR *)((((unsigned long)seg) << 16) \
    + (unsigned long)(unsigned)offset)

#pragma    pack(1)
typedef struct {
    UINT16    Floppy:1;
    UINT16    MathCoprocessor:1;
    UINT16    Ps2Mouse:1;
    UINT16    Config_Bit3:1;
    UINT16    InitialVideoMode:2;
    UINT16    NumberOfFloppyDiskDriver:2;
    UINT16    Config_Bit8:1;
    UINT16    SerialInstalled:3;
    UINT16    GamePort:1;
    UINT16    Config_Bit13:1;
    UINT16    ParallelInstalled:2;
} MCN_CFG;

/*
 ____________________________________

 BIOS Data Location (mem 40:xx)
 ____________________________________
*/
typedef struct {
    UINT8 Status0;
    UINT8 Status1;
    UINT8 Status2;
    UINT8 Cylinder;
    UINT8 Head;
    UINT8 Sector;
    UINT8 Number;
} FDC_BLK;

typedef struct {
    UINT16      Com[4];
    UINT16      Lpt[3];
    UINT16      Ebda;
    MCN_CFG     MachineConfig;        // MachineConfig;
    UINT8       Bda12;
    UINT16      MemSize;
    UINT8       Bda15_16[0x02];
    UINT8       ShiftStatus;
    UINT8       ExtendedKbdStatus;
    UINT8       Bda19;
    UINT16      KeyHead;
    UINT16      KeyTail;
    UINT16      Bda1E_3D[0x10];
    UINT8       CalibrationStatus;
    UINT8       MotorStatus;
    UINT8       FloppyTimeout;
    UINT8       FppStatus;
    FDC_BLK     FDC_RESULT_BLK;
    UINT8       CRT_Mode;           // 40:49
    UINT16      CRT_Cols;           // 40:4A
    UINT16      CRT_Len;            // 40:4C
    UINT16      CRT_Start;          // 40:4E
    UINT16      Cursor_Posn[0x08];  // 40:50
    UINT16      Cursor_Mode;        // 40:60
    UINT8       Active_Page;        // 40:62
    UINT16      Addr_6845;          // 40:63
    UINT8       CRT_Mode_Set;       // 40:65
    UINT8       CRT_Palette;        // 40:66
    UINT16      ROM_MODULE_OFFSET;  // 40:67
    UINT16      ROM_MODULE_SEG;     // 40:69
    UINT8       LastInterrupt;
    union _TimeUnion
    {
        UINT32 dwTimerCount;
        struct _TimerCount
        {
            UINT16  LowTimerCount;
            UINT16  HighTimerCount;
        } TimerCount;
    } TimeUnion;

    UINT8       TimerOverFlag;         // Int1A
    UINT8       Bda71_73[0x03];
    UINT8       Int13Status;
    UINT8       NumberOfDrives;
    UINT8       Bda76_77[0x02];
    UINT16      Lpt1_2Timeout;
    UINT8       Lpt3_Timeout;
    UINT8       DosVds;
    UINT16      Com1_2Timeout;
    UINT16      Com3_4Timeout;
    UINT16      KeyStart;
    UINT16      KeyEnd;
    UINT8       CRT_Row;
    UINT8       Bda85_87[0x3];
    UINT8       DataXmit;
    UINT8       Bda89_8C[0x04];
    UINT8       HddErrStus;
    UINT8       HddIntStus;
    UINT8       FdcInfo;               // 0x8F
    UINT8       Fdc0Mediatype;         // 0x90
    UINT8       Fdc1Mediatype;         // 0x91
    UINT8       Fdc0OperationState;    // 0x92
    UINT8       Fdc1OperationState;    // 0x93
    UINT8       Fdc0CurrentTrack;      // 0x94
    UINT8       Fdc1CurrentTrack;      // 0x95
    UINT8       ExtKbdSts1;
    UINT8       ExtKbdSts2;
    UINT16      SegOfUserFlag;
    UINT16      OffOfUserFlag;
    UINT32      UserWaitCount;
    UINT8       UserWaitFlag;
    UINT8       BdaA1_D1[0x31];
    UINT8       INT13Para[0x0C];
    UINT8       BdaDE_FF[0x22];
    UINT8       PrintStatus;
} BDA_SEG;

typedef BDA_SEG  __FAR* LPBDA_SEG;

#pragma pack()


EFI_STATUS
InitUsbKeyboard (
    IN USB_KB_DEV   *UsbKeyboardDevice
);

EFI_STATUS
EFIAPI
KeyboardHandler (
    IN  VOID          *Data,
    IN  UINTN         DataLength,
    IN  VOID          *Context,
    IN  UINT32        Result
);

EFI_STATUS
EFIAPI
UsbKeyboardRecoveryHandler (
    IN  VOID          *Data,
    IN  UINTN         DataLength,
    IN  VOID          *Context,
    IN  UINT32        Result
);

EFI_STATUS
EFIAPI
UsbKeyboardRepeatHandler (
    IN  VOID          *Data,
    IN  UINTN         DataLength,
    IN  VOID          *Context,
    IN  UINT32        Result
);

EFI_STATUS
SetKeyLED (
    IN  USB_KB_DEV    *UsbKeyboardDevice
);

EFI_STATUS
EFIAPI
UsbConnectKeyboardDriver (
    IN USB_INTERFACE        *UsbIf
);

EFI_STATUS
EFIAPI
UsbDisconnectKeyboardDriver (
    IN USB_INTERFACE        *UsbIf
);

VOID
UsbSetLED (VOID);

#endif
