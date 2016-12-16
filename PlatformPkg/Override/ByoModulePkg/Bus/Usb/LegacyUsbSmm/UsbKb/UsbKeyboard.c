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
  UsbKeyboard.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/


#include <Protocol/UsbPolicy.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

#include "UsbKeyboard.h"
#include "../UsbLib/hid.h"
#include "../UsbLib/UsbLib.h"
#include "../UsbBus/UsbBus.h"
#include "../UsbVirtualKbc/UsbVirtualKbc.h"

#define MASK_PIC_20     0x20
#define SLAVE_PIC_A0    0xA0
#define EOI_CMD         0x20

extern BOOLEAN         mLegacyFreeSupport;

UINT8  UsageTable[] = {
//   AT code    Usage   index#  Desciption
    0x00,      //       0       reserved
    0x00,      //       1       kbd error rollover
    0x00,      //       2       kbd POST fail
    0x00,      //       3       kbd ErrorUndefined
    0x1e,      //       4       'A'
    0x30,      //       5       'B'
    0x2e,      //       6       'C'
    0x20,      //       7       'D'
    0x12,      //       8       'E'
    0x21,      //       9       'F'
    0x22,      //       0A      'G'
    0x23,      //       0B      'H'
    0x17,      //       0C      'I'
    0x24,      //       0D      'J'
    0x25,      //       0E      'K'
    0x26,      //       0F      'L'
    0x32,      //       10      'M'
    0x31,      //       11      'N'
    0x18,      //       12      'O'
    0x19,      //       13      'P'
    0x10,      //       14      'Q'
    0x13,      //       15      'R'
    0x1f,      //       16      'S'
    0x14,      //       17      'T'
    0x16,      //       18      'U'
    0x2f,      //       19      'V'
    0x11,      //       1A      'W'
    0x2d,      //       1B      'X'
    0x15,      //       1C      'Y'
    0x2c,      //       1D      'Z'
    0x02,      //       1E      '1 !'
    0x03,      //       1F      '2 @'
    0x04,      //       20      '3 #'
    0x05,      //       21      '4 $'
    0x06,      //       22      '5 %'
    0x07,      //       23      '6 ^'
    0x08,      //       24      '7 &'
    0x09,      //       25      '8 *'
    0x0a,      //       26      '9 ('
    0x0b,      //       27      '0 )'
    0x1c,      //       28      'Enter'
    0x01,      //       29      'Esc'
    0x0e,      //       2A      'Backspace'
    0x0f,      //       2B      'Tab'
    0x39,      //       2C      'Space bar'
    0x0c,      //       2D      '- _'
    0x0d,      //       2E      '= +'
    0x1a,      //       2F      '[ {'
    0x1b,      //       30      '] }'
    0x2b,      //       31      '\ |'
    0x56,      //       32      Non-US '# ~' ******
    0x27,      //       33      '; :'
    0x28,      //       34      '' "'
    0x29,      //       35      ;` ~'
    0x33,      //       36      ', <'
    0x34,      //       37      '. >'
    0x35,      //       38      '/ ?'
    0x3a,      //       39      'CapsLock'
    0x3b,      //       3A      'F1'
    0x3c,      //       3B      'F2'
    0x3d,      //       3C      'F3'
    0x3e,      //       3D      'F4'
    0x3f,      //       3E      'F5'
    0x40,      //       3F      'F6'
    0x41,      //       40      'F7'
    0x42,      //       41      'F8'
    0x43,      //       42      'F9'
    0x44,      //       43      'F10'
    0x57,      //       44      'F11'
    0x58,      //       45      'F12'
    0x80,      //       46      'PrntScrn' = E0,2A,E0,37,E0,B7,E0,AA
    0x46,      //       47      'ScrollLock'
    0x81,      //       48      'Pause'= E1,1D,45,E1,9D,C5
    0x82,      //       49      'Insert'= E0,52,E0,D2
    0x83,      //       4A      'Home'= E0,47,E0,C7
    0x84,      //       4B      'PageUp'= E0,49,E0,C9
    0x85,      //       4C      'Delete Forward'= E0,53,E0,D3
    0x86,      //       4D      'End' = E0,4F,E0,CF
    0x87,      //       4E      'PageDown'= E0,51,E0,D1
    0x88,      //       4F      'Right Arrow'= E0,4D,E0.CD
    0x89,      //       50      'Left Arrow'= E0,4B,E0,CB
    0x8a,      //       51      'Down Arrow'= E0,50,E0,D0
    0x8b,      //       52      'Up Arrow' = E0,48,E0,C8
    0x45,      //       53      'NumLock'
    0x8c,      //       54      'keypad /'= E0,35,E0,B5
    0x37,      //       55      'keypad *'
    0x4a,      //       56      'keypad -'
    0x4e,      //       57      'keypad +'
    0x8d,      //       58      'keypad Enter'= E0,1C,E0,9C
    0x4f,      //       59      'keypad 1 End'
    0x50,      //       5A      'keypad 2 DownArrow'
    0x51,      //       5B      'keypad 3 PageDn'
    0x4b,      //       5C      'keypad 4 LeftArrow'
    0x4c,      //       5D      'keypad 5'
    0x4d,      //       5E      'keypad 6 RightArrow'
    0x47,      //       5F      'keypad 7 Home'
    0x48,      //       60      'keypad 8 UpArrow'
    0x49,      //       61      'keypad 9 PageUp'
    0x52,      //       62      'keypad 0 Insert'
    0x53,      //       63      'keypad . Del'
    0x2b,      //       64      '\ |'
    0x8e       //       65      'Application'= E0,5D,E0,DD
};

UINT8  MKeyTable[] = {
    0x37,                  // 0 'PrntScrn' = E0,2A,E0,37,
    0x1D,                  // 1  PAUSE
    0x52,                  // 2  Ins
    0x47,                  // 3  Home
    0x49,                  // 4 PageUp
    0x53,                  // 5 Del
    0x4f,                  // 6 End
    0x51,                  // 7 PageDown
    0x4d,                  // 8 Right Arrow
    0x4b,                  // 9 Left Arrow
    0x50,                  // a Down Arrow
    0x48,                  // b Up Arrow
    0x35,                  // c KeyPad /
    0x1c,                  // d KeyPad Enter
    0x5d                   // e Application
};

STATIC KB_MODIFIER  KB_Mod[8] = {
    { MOD_CONTROL_L,  0xe0, 0x1d }, // 11100000
    { MOD_CONTROL_R,  0xe4, 0x9d }, // 11100100
    { MOD_SHIFT_L,    0xe1, 0x2a }, // 11100001
    { MOD_SHIFT_R,    0xe5, 0x36 }, // 11100101
    { MOD_ALT_L,      0xe2, 0x38 }, // 11100010
    { MOD_ALT_R,      0xe6, 0xb8 }, // 11100110
    { MOD_WIN_L,      0xe3, 0xdb }, // 11100011
    { MOD_WIN_R,      0xe7, 0xdc }, // 11100111
};

//     +-------------------------------- Conversion Type
//     |  +----------------------------- Unshifted
//     |  |    +------------------------ Shifted
//     |  |    |    +------------------- Control Shift
//     |  |    |    |    +-------------- Alternate Shift
//     |  |    |    |    |
STATIC UINT8 ScanCodeConvertTBL[0x5B][5]={
    0,0xFF,0xFF,0xFF,0xFF,      // Error
    1,0x1B,0x1B,0x1B,0xF0,      //  Esc
    1, '1', '!',0xFF,0x78,      //   1
    1, '2', '@',0x00,0x79,      //   2
    1, '3', '#',0xFF,0x7A,      //   3
    1, '4', '$',0xFF,0x7B,      //   4
    1, '5', '%',0xFF,0x7C,      //   5
    1, '6', '^',0x1E,0x7D,      //   6
    1, '7', '&',0xFF,0x7E,      //   7
    1, '8', '*',0xFF,0x7F,      //   8
    1, '9', '(',0xFF,0x80,      //   9
    1, '0', ')',0xFF,0x81,      //   0
    1, '-', '_',0x1F,0x82,      //   -
    1, '=', '+',0xFF,0x83,      //   =
    1,0x08,0x08,0x7F,0xF0,      // Bcksp
    4,0x09,0x00,0x94,0xA5,      //  Tab
    1, 'q', 'Q',0x11,0x10,      //   q
    1, 'w', 'W',0x17,0x11,      //   w
    1, 'e', 'E',0x05,0x12,      //   e
    1, 'r', 'R',0x12,0x13,      //   r
    1, 't', 'T',0x14,0x14,      //   t
    1, 'y', 'Y',0x19,0x15,      //   y
    1, 'u', 'U',0x15,0x16,      //   u
    1, 'i', 'I',0x09,0x17,      //   i
    1, 'o', 'O',0x0F,0x18,      //   o
    1, 'p', 'P',0x10,0x19,      //   p
    1, '[', '{',0x1B,0xF0,      //   [
    1, ']', '}',0x1D,0xF0,      //   ]
    1,0x0D,0x0D,0x0A,0xF0,      //  CR
    0,0xFF,0xFF,0xFF,0xFF,      // Ctrl
    1, 'a', 'A',0x01,0x1E,      //   a
    1, 's', 'S',0x13,0x1F,      //   s
    1, 'd', 'D',0x04,0x20,      //   d
    1, 'f', 'F',0x06,0x21,      //   f
    1, 'g', 'G',0x07,0x22,      //   g
    1, 'h', 'H',0x08,0x23,      //   h
    1, 'j', 'J',0x0A,0x24,      //   j
    1, 'k', 'K',0x0B,0x25,      //   k
    1, 'l', 'L',0x0C,0x26,      //   l
    1, ';', ':',0xFF,0xF0,      //   ;
    1, '\'','"',0xFF,0xF0,      //   '
    1, '`', '~',0xFF,0xF0,      //   `
    0,0xFF,0xFF,0xFF,0xFF,      // LShft
    1, '\\', '|',0x1C,0xF0,     //   '\'
    1, 'z', 'Z',0x1A,0x2C,      //   z
    1, 'x', 'X',0x18,0x2D,      //   x
    1, 'c', 'C',0x03,0x2E,      //   c
    1, 'v', 'V',0x16,0x2F,      //   v
    1, 'b', 'B',0x02,0x30,      //   b
    1, 'n', 'N',0x0E,0x31,      //   n
    1, 'm', 'M',0x0D,0x32,      //   m
    1, ',', '<',0xFF,0xF0,      //   ,
    1, '.', '>',0xFF,0xF0,      //   .
    1, '/', '?',0xFF,0xF0,      //   /
    0,0xFF,0xFF,0xFF,0xFF,      //
    4, '*', '*',0x96,0xF0,      //   *
    0,0xFF,0xFF,0xFF,0xFF,      //  Alt
    5, ' ', ' ', ' ', ' ',      // Space
    0,0xFF,0xFF,0xFF,0xFF,      // CapLk
    2,0x3B,0x54,0x5E,0x68,      //  F1
    2,0x3C,0x55,0x5F,0x69,      //  F2
    2,0x3D,0x56,0x60,0x6A,      //  F3
    2,0x3E,0x57,0x61,0x6B,      //  F4
    2,0x3F,0x58,0x62,0x6C,      //  F5
    2,0x40,0x59,0x63,0x6D,      //  F6
    2,0x41,0x5A,0x64,0x6E,      //  F7
    2,0x42,0x5B,0x65,0x6F,      //  F8
    2,0x43,0x5C,0x66,0x70,      //  F9
    2,0x44,0x5D,0x67,0x71,      //  F10
    0,0xFF,0xFF,0xFF,0xFF,      // NumLk
    0,0xFF,0xFF,0xFF,0xFF,      // ScrLk
    3,0x00, '7',0x77,0x07,      //  7 Home
    3,0x00, '8',0x8D,0x08,      //  8  ^
    3,0x00, '9',0x84,0x09,      //  9 PgUp
    4, '-', '-',0x8E,0xF0,      //  -
    3,0x00, '4',0x73,0x04,      //  4  v
    3,0xF0, '5',0x8F,0x05,      //  5
    3,0x00, '6',0x74,0x06,      //  6  >
    4, '+', '+',0x90,0xF0,      //  +
    3,0x00, '1',0x75,0x01,      //  1 End
    3,0x00, '2',0x91,0x02,      //  2  <
    3,0x00, '3',0x76,0x03,      //  3 PgDn
    3,0x00, '0',0x92,0x00,      //  Ins
    3,0x00, '.',0x93,0xFF,      //  Del
    0,0xFF,0xFF,0xFF,0xFF,      // SysRq
    0,0xFF,0xFF,0xFF,0xFF,      //
    1,0x5C,0x7C,0xFF,0xFF,      // (102)
    2,0x85,0x87,0x89,0x8B,      //  F11
    2,0x86,0x88,0x8A,0x8C,      //  F12
    2,0x53,0x53,0x93,0xA3,      //  Del
    0,0xFF,0xFF,0xFF,0xFF,      // Reserved
};

STATIC
EFI_STATUS
UsbKbdQueueHandler (
    IN VOID         *Data,
    IN UINTN        DataLength,
    IN VOID         *Context,
    IN UINT32       Result
);

EFI_STATUS
EFIAPI
UsbKeyboardReset (
  IN  USB_KB_DEV          *UsbKeyboardDevice,
  IN  BOOLEAN             ExtendedVerification
  );

VOID
SendEOI (BOOLEAN bMaster)
{
  if (bMaster) {
    SmmIoWrite8(MASK_PIC_20, EOI_CMD);
  } else {
    SmmIoWrite8(SLAVE_PIC_A0, EOI_CMD);
    SmmIoWrite8(MASK_PIC_20, EOI_CMD);
  }
}

/*
 ______________________________________________

  Get the BIOS Data Area Segment
 ______________________________________________
*/
LPBDA_SEG
GetBda (VOID)
{
  LPBDA_SEG LpBda;

  LpBda = (LPBDA_SEG )0x400;
  return LpBda;
}

/*
 _________________________________________________________________________________

 Check scan code is Extended scan code?
 _________________________________________________________________________________
*/
BOOLEAN
CheckExtendedScanCode (LPBDA_SEG lpBda, UINT8 ScanCode)
{
  if (ScanCode == 0xE0) {
    lpBda->ExtKbdSts1 |= BIT1;
  } else if (ScanCode == 0xE1) {
    lpBda->ExtKbdSts1 |= BIT0;
  } else {
    return FALSE;
  }
  return TRUE;
}

/*
 _________________________________________________________________________________

 If scan code is extended scan code and conversion to extended Scan code,
 _________________________________________________________________________________
*/
UINT16
ExtendedScanCode (LPBDA_SEG lpBda, UINT8 ScanCode)
{
  if ((lpBda->ExtKbdSts1 & BIT0) == BIT0) {
    return 0xE100 + (UINT16) ScanCode;
  } else if ((lpBda->ExtKbdSts1 & BIT1) == BIT1) {
    return 0xE000 + (UINT16) ScanCode;
  }
  return (UINT16) ScanCode;
}

/*
 _________________________________________________________________________________

 Check response code.
 _________________________________________________________________________________
*/
BOOLEAN
CheckResponseCode (LPBDA_SEG lpBda, UINT8 ScanCode)
{
  if (ScanCode == 0xFE) {
    //
    // resend request
    //
    lpBda->ExtKbdSts2 |= BIT5;
  } else if (ScanCode == 0xFA) {
    //
    // ACk
    //
    lpBda->ExtKbdSts2 |= BIT4;
  } else {
    return FALSE;
  }
  return TRUE;
}

/*
 _________________________________________________________________________________

 Check Read ID command.
 _________________________________________________________________________________
*/
BOOLEAN
CheckReadIDCommand (LPBDA_SEG lpBda, UINT8 ScanCode)
{
  //
  // CSM32 Should pass the keyboard type to CSM16
  //
  return FALSE;
}

/*
 _________________________________________________________________________________

 Estimate key input and return number of inputing key.
 _________________________________________________________________________________
*/
UINT16
HasKeyInKeyBuffer (LPBDA_SEG lpBda)
{
  return ((lpBda->KeyTail - lpBda->KeyHead)/2);
}

/*
 _________________________________________________________________________________

 Insert key in key buffer.
 _________________________________________________________________________________
*/
VOID
InsertKey (LPBDA_SEG lpBda, UINT16 ScanCode)
{
  UINT16      KeyCount;
  UINT16      KeyHead = lpBda->KeyHead;
  UINT16      KeyCode;
  //
  // Modify the Key Tail
  //
  KeyCount = HasKeyInKeyBuffer (lpBda);
  if (KeyCount < ((lpBda->KeyEnd - lpBda->KeyStart)/2)) {
    if (ScanCode >= 0xE000) {
      KeyCode = ScanCode * 0x100;
    } else {
      KeyCode = ScanCode;
    }

    //
    // Calulate the tail position
    //
    if ((KeyHead + KeyCount * 2) < lpBda->KeyEnd) {
      //
      // Addjust Key Tail by plusing 2
      //
      lpBda->Bda1E_3D[KeyHead/2 + KeyCount - lpBda->KeyStart/2] = KeyCode;
      lpBda->KeyTail = KeyHead + (KeyCount + 1) * 2;
    } else {
      //
      // Overlapped, make tail not large than head
      //
      lpBda->Bda1E_3D[KeyHead/2 + KeyCount - lpBda->KeyEnd/2] = KeyCode;
      lpBda->KeyTail = KeyHead + (KeyCount + 1) * 2 - (lpBda->KeyEnd - lpBda->KeyStart);
    }
  }
}

/*
_________________________________________________________________________________

 Deal with Special scan code.
 _________________________________________________________________________________
*/
BOOLEAN
IsSpecialScode (LPBDA_SEG lpBda, UINT16 *ScanCode)
{
  switch (*ScanCode) {
    case 0x1D:
      // Left Ctrl Press
      lpBda->ShiftStatus |= BIT2;
      lpBda->ExtendedKbdStatus |= BIT0;
      break;

    case 0x9D:
      // Left Ctrl Release
      lpBda->ExtendedKbdStatus &= ~BIT0;
      if ((lpBda->ExtKbdSts1 & BIT2) == 0) {
          lpBda->ShiftStatus &= ~BIT2;
      }
      break;

    case 0xE01D:
      // Right Ctrl Press
      lpBda->ShiftStatus |= BIT2;
      lpBda->ExtKbdSts1 |= BIT2;
      break;

    case 0xE09D:
      // Right Ctrl Release
      if ((lpBda->ExtendedKbdStatus & BIT0) == 0) {
          lpBda->ShiftStatus &= ~BIT2;
      }
      lpBda->ExtKbdSts1 &= ~BIT2;
      break;

    case 0x38:
      // Left Alt Press
      lpBda->ShiftStatus |= BIT3;
      lpBda->ExtendedKbdStatus |= BIT1;
      break;

    case 0xB8:
      // Left Alt Release
      lpBda->ExtendedKbdStatus &= ~BIT1;
      if ((lpBda->ExtKbdSts1 & BIT3) == 0) {
          lpBda->ShiftStatus &= ~BIT3;
      }
      break;

    case 0xE038:
      // Right Alt Press
      lpBda->ShiftStatus |= BIT3;
      lpBda->ExtKbdSts1 |= BIT3;
      break;

    case 0xE0B8:
      // Right Alt Release
      if ((lpBda->ExtendedKbdStatus & BIT1) == 0) {
          lpBda->ShiftStatus &= ~BIT3;
      }
      lpBda->ExtKbdSts1 &= ~BIT3;
      break;

    case 0x2A:
      // Left Shift Press
      lpBda->ShiftStatus |= BIT1;
      break;

    case 0xAA:
      lpBda->ShiftStatus &= ~BIT1;
      // Left Shift Release
      break;

    case 0x36:
      // Right Shift Press
      lpBda->ShiftStatus |= BIT0;
      break;

    case 0xB6:
      // Right Shift Release
      lpBda->ShiftStatus &= ~BIT0;
      break;

    case 0x3A:
      // Caps Press
      //
      // First Press
      //
      if ((lpBda->ShiftStatus & BIT6) == 0) {
          //
          // Caps Lock on
          //
          lpBda->ShiftStatus |= BIT6;
      } else {
          //
          // Caps Lock Off
          //
          lpBda->ShiftStatus &= ~BIT6;
      }
      break;

    case 0xBA:
      // Caps Release
      break;

    case 0x45:
      //
      // numlk Press, set up the led
      //
      //
      // First Press
      //
      if ((lpBda->ShiftStatus & BIT5) == 0) {
          //
          // numlk Lock on
          //
          lpBda->ShiftStatus |= BIT5;
      } else {
          //
          // numlk Lock Off
          //
          lpBda->ShiftStatus &= ~BIT5;
      }
      break;

    case 0xC5:
      // numlk Release
      //lpBda->ExtendedKbdStatus &= ~BIT5;
      break;

    case 0x46:
      // scrlk Press
      //
      // First Press
      //
      if ((lpBda->ShiftStatus & BIT4) == 0) {
        //
        // scrlk Lock on
        //
        lpBda->ShiftStatus |= BIT4;
      } else {
        //
        // scrlk Lock Off
        //
        lpBda->ShiftStatus &= ~BIT4;
      }
      break;

    case 0xC6:
      // scrlk Release
      break;

    case 0xE0C6:
      // ctrl + Brak
      *ScanCode = 0x1DC6;
      break;

    case 0x37:
      // Keypad *
      if ((lpBda->ExtKbdSts1 & BIT4) == 0) {
          if (lpBda->ShiftStatus & BIT2) {
              *ScanCode = 0xE037;
          }
      }
      return FALSE;

    case 0xE0:
      // Ext
      lpBda->ExtKbdSts1 |= BIT1;
      break;

    case 0xE1:
      // Ext
      lpBda->ExtKbdSts1 |= BIT0;
      break;

    case 0xFF:
    case 0xE0FF:
    case 0xE1FF:
      //
      // Unknow Key, Error Key
      break;

    case 0xE01C:
      if (lpBda->ShiftStatus & BIT3) {
          InsertKey (lpBda, 0xA600);
      } else if (lpBda->ShiftStatus & BIT2) {
          InsertKey (lpBda, 0xE00A);
      } else {
          InsertKey (lpBda, 0x1c0D);
      }
      //
      // Keypad Enter
      break;

    case 0xE035:
      //
      // Keypad
      if (lpBda->ShiftStatus & BIT3) {
          InsertKey (lpBda, 0xA400);
      } else if (lpBda->ShiftStatus & BIT2) {
          InsertKey (lpBda, 0x9500);
      } else {
          InsertKey (lpBda, 0x352F);
      }
      break;

    case 0x52:
      //
      // Keypad Ins Press
      //
      if ((lpBda->ExtendedKbdStatus & BIT5) == 0) {
        //
        // Ins Press
        //
        if ((lpBda->ExtendedKbdStatus & BIT7) == 0) {
          //
          // First Press
          //
          if ((lpBda->ShiftStatus & BIT7) == 0) {
            //
            // scrlk Lock on
            //
            lpBda->ShiftStatus |= BIT7;
          } else {
            //
            // scrlk Lock Off
            //
            lpBda->ShiftStatus &= ~BIT7;
          }
          lpBda->ExtendedKbdStatus |= BIT7;
        }
      } else {
        *ScanCode = 0x0B;
      }

      if (lpBda->ShiftStatus & BIT5) {
        InsertKey (lpBda, 0x5230);
      } else{
        InsertKey (lpBda, 0xe030);
      }
      break;

    case 0xD2:
      //
      // Keypad Ins Release
      if ((lpBda->ExtendedKbdStatus & BIT5) == 0) {
        lpBda->ExtendedKbdStatus &= ~BIT7;
      } else {
        *ScanCode = 0x8B;
      }
      break;

    case 0xE052:
      //
      // Ins Press
      if ((lpBda->ExtendedKbdStatus & BIT7) == 0) {
        //
        // First Press
        //
        if ((lpBda->ShiftStatus & BIT7) == 0) {
          //
          // scrlk Lock on
          //
          lpBda->ShiftStatus |= BIT7;
        } else {
          //
          // scrlk Lock Off
          //
          lpBda->ShiftStatus &= ~BIT7;
        }
        lpBda->ExtendedKbdStatus |= BIT7;
      }
    case 0xE0D2:
      //
      // Ins Release
      lpBda->ExtendedKbdStatus &= ~BIT7;
      break;

    case 0x53:

    case 0xe053:
      //
      // Del + Ctrl +Alt key press
      //
      if((lpBda->ShiftStatus & (BIT2 | BIT3)) == (BIT2 | BIT3)) {
        lpBda->Bda71_73[1] = 0x34;
        lpBda->Bda71_73[2] = 0x12;
        //
        // Send EOI
        //
        SendEOI (TRUE);
        //
        // Reset system
        //
        SmmIoWrite8(0xcf9,0x06);
        //EfiUsbHalt();
      } else {
        return FALSE;
      }

    default:
    return FALSE;
  }

  return TRUE;
}

/*
 _________________________________________________________________________________

 Deal Normal Scan code.
 _________________________________________________________________________________
*/
BOOLEAN
IsNormalCode (LPBDA_SEG lpBda,UINT16 ScanCode)
{
  UINT8     ScanCodeCheck[5];
  UINT16    ExtendScanCode = ScanCode;

  if (ScanCode >= 0x100) {
    //
    // Not deal with extended scan code.
    //
    return FALSE;
  } else if (ScanCode > 0x58) {
    //
    // Not deal with Scan code over 0x58
    //
    return FALSE;
  }

  //
  // Deal  with Ctrl + Alt + Del key press
  //
  if (((ScanCode == 0x53) | (ScanCode == 0xe053)) && ((lpBda->ShiftStatus & (BIT2 | BIT3)) == (BIT2 | BIT3))) {
    //
    // reset system
    //
    SmmIoWrite8 (0xCF9, 0x6);
  }

  //
  // Convert Scan Code
  //
  ScanCodeCheck[0] = ScanCodeConvertTBL[ExtendScanCode][0];
  ScanCodeCheck[1] = ScanCodeConvertTBL[ExtendScanCode][1];
  ScanCodeCheck[2] = ScanCodeConvertTBL[ExtendScanCode][2];
  ScanCodeCheck[3] = ScanCodeConvertTBL[ExtendScanCode][3];
  ScanCodeCheck[4] = ScanCodeConvertTBL[ExtendScanCode][4];

  //
  // We have the code, check it
  //
  if (ScanCodeCheck[0] == 1) {
      //
      // Check the Ctrl and Shift Status
      //
      // High 8 bit in ExtendScanCode is scan code
      ExtendScanCode = ExtendScanCode * 0x100;
      //
      // Get ASC Code
      //
      if (lpBda->ShiftStatus & BIT3) {
      //
      // with Alt key press
      //
      } else if (lpBda->ShiftStatus & BIT2) {
        ExtendScanCode = ExtendScanCode +  (UINT16) (ScanCodeCheck[3]);
      } else if (lpBda->ShiftStatus & 0x3) {
        //
        // With Shift key press
        //
        if (ScanCodeCheck[2] >= 'A' && ScanCodeCheck[2] <= 'Z') {
            //
            // Check Cap Lock
            //
            if (lpBda->ShiftStatus & BIT6) {
                ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[1]);
            } else {
                ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[2]);
            }
        } else {
                ExtendScanCode = ExtendScanCode +(UINT16) ScanCodeCheck[2];
        }
      } else {
        if (ScanCodeCheck[2] >= 'A' && ScanCodeCheck[2] <= 'Z') {
            //
            // Check Cap Lock
            //
            if (lpBda->ShiftStatus & BIT6) {
                ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[2]);
            } else {
                ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[1]);
            }
        } else {
                ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[1]);
        }
      }
  } else if (ScanCodeCheck[0] == 2) {
    //
    // No Char
    //
    ExtendScanCode = 0xE000;
    if (lpBda->ShiftStatus & BIT3) {
      //
      // With Alt key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[4]);
    } else if (lpBda->ShiftStatus & BIT2) {
      //
      // With Control key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[3]);
    } else if (lpBda->ShiftStatus & (BIT1 | BIT0)) {
      //
      // With Shift key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[2]);
    } else {
      //
      // Not With any functional press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[1]);
    }
  } else if (ScanCodeCheck[0] == 3) {
    //
    // Deal with Conversion Type three key
    //
    if (lpBda->ShiftStatus & BIT5) {
      //
      // Num key open
      //
      ExtendScanCode = ExtendScanCode * 0x100;
      if (lpBda->ShiftStatus & BIT3) {
        //
        // With Alt key press
        //
        ExtendScanCode = 0xE000;
        ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[4]);
      } else if (lpBda->ShiftStatus & BIT2) {
        //
        // With Control key press
        //
        ExtendScanCode = 0xE000;
        ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[3]);
      } else if (lpBda->ShiftStatus & (BIT1 | BIT0)) {
        //
        // With Shift key press
        //
        ExtendScanCode = 0xE000;
        ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[1]);
      } else {
        //
        // Not with any functional key press
        //
        ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[2]);
      }
    } else {
      ExtendScanCode = 0xFF;
    }
  } else if (ScanCodeCheck[0] == 4) {
    //
    // Deal with Conversion Type four key
    //
    ExtendScanCode = ScanCode * 0x100;
    if (lpBda->ShiftStatus & BIT3) {
      //
      // With Alt key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[4]);
    } else if (lpBda->ShiftStatus & BIT2) {
      //
      // With Control key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[3]);
    } else if (lpBda->ShiftStatus & (BIT1 | BIT0)) {
      //
      // With Shift key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[2]);
    } else {
      //
      // not with any functional key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[1]);
    }
  } else if (ScanCodeCheck[0] == 5) {
    //
    // Deal with Conversion Type five key
    //
    ExtendScanCode = ScanCode * 0x100;
    if (lpBda->ShiftStatus & BIT3) {
      //
      // With Alt key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[4]);
    } else if (lpBda->ShiftStatus & BIT2) {
      //
      // With Control key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[3]);
    } else if (lpBda->ShiftStatus & (BIT1 | BIT0)) {
      //
      // With Shift key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[2]);
    } else {
      //
      // Not with any functional key press
      //
      ExtendScanCode = ExtendScanCode + (UINT16) (ScanCodeCheck[1]);
    }
  } else if (ScanCodeCheck[0] == 0) {
    //
    // This scan code will be dropped
    //
    ExtendScanCode = 0xFF;
  }

  if ((ExtendScanCode & 0xFF) != 0xFF) {
    //
    // Insert key in BDA key buffer.
    //
    InsertKey (lpBda, ExtendScanCode);
  }

  return TRUE;
}
/*
 _________________________________________________________________________________

 Deal with Extended special scan code.
 _________________________________________________________________________________
*/
BOOLEAN
IsExtendCode (LPBDA_SEG lpBda,UINT16 ScanCode)
{
  UINT16      Extend;
  UINT16      Scan;
  Extend = ScanCode & 0xFF00;
  Scan   = ScanCode & 0xFF;

  if (Extend == 0xE000) {
    if ((Scan & BIT7) == 0) {
      switch (Scan) {
        case 0x47:
          //
          // Gray Home key Press
          //
          if (lpBda->ShiftStatus & BIT2) {
            //
            // With Control key press
            //
            Extend += 0x77;
          } else if (lpBda->ShiftStatus & BIT3) {
            //
            // With Alt key press
            //
            Extend += 0x97;
          } else {
            Extend = Extend+ Scan;
          }
          break;

        case 0x48:
          //
          // Gray Up key press
          //
          if (lpBda->ShiftStatus & BIT2) {
            //
            // With Control key press
            //
            Extend += 0x8D;
          } else if (lpBda->ShiftStatus & BIT3) {
            //
            // With Alt key press
            //
            Extend += 0x98;
          } else {
            Extend = Extend+ Scan;
          }
          break;

        case 0x49:
          //
          // Gray PgUp key press
          //
          if (lpBda->ShiftStatus & BIT2) {
            //
            // With Control key press
            //
            Extend += 0x84;
          } else if (lpBda->ShiftStatus & BIT3) {
            //
            // With Alt key press
            //
            Extend += 0x99;
          } else {
             Extend = Extend+ Scan;
          }
          break;

        case 0x4B:
          //
          // Gray Left key press
          //
          if (lpBda->ShiftStatus & BIT2) {
            //
            // With Control key press
            //
            Extend += 0x73;
          } else if (lpBda->ShiftStatus & BIT3) {
            //
            // With Alt key press
            //
            Extend += 0x9B;
          } else {
            Extend = Extend+ Scan;
          }
          break;;

        case 0x4D:
          //
          // Gray Right key press
          //
          if (lpBda->ShiftStatus & BIT2) {
            //
            // With Control key press
            //
            Extend += 0x74;
          } else if (lpBda->ShiftStatus & BIT3) {
            //
            // With Alt key press
            //
            Extend += 0x9D;
          } else {
            Extend = Extend+ Scan;
          }
          break;

        case 0x4F:
          //
          // Gray End key press
          //
          if (lpBda->ShiftStatus & BIT2) {
            //
            // With Control key press
            //
            Extend += 0x75;
          } else if (lpBda->ShiftStatus & BIT3) {
            //
            // With Alt key press
            //
            Extend += 0x9F;
          } else {
            Extend = Extend+ Scan;
          }
          break;

        case 0x50:
          //
          // Gray Down key press
          //
          if (lpBda->ShiftStatus & BIT2) {
            //
            // With Control key press
            //
            Extend += 0x91;
          } else if (lpBda->ShiftStatus & BIT3) {
            //
            // With Alt key press
            //
            Extend += 0xA0;
          } else {
            Extend = Extend+ Scan;
          }
          break;

        case 0x51:
          //
          // Gray PgDn key press
          //
          if (lpBda->ShiftStatus & BIT2) {
            //
            // With Control key press
            //
            Extend += 0x76;
          } else if (lpBda->ShiftStatus & BIT3) {
            //
            // With Alt key press
            //
            Extend += 0x0A1;
          } else {
            Extend = Extend+ Scan;
          }
          break;

        case 0x52:
          //
          // Gray Ins key press
          //
          if (lpBda->ShiftStatus & BIT2) {
            //
            // With Control key press
            //
            Extend += 0x92;
          } else if (lpBda->ShiftStatus & BIT3) {
            //
            // With Alt key press
            //
            Extend += 0xA2;
          } else {
            Extend = Extend+ Scan;
          }
          break;;

        case 0x53:
          //
          // Gray Del key press
          //
          if (lpBda->ShiftStatus & BIT2) {
            //
            // With Control key press
            //
            Extend += 0x93;
          } else if (lpBda->ShiftStatus & BIT3) {
            //
            // With Alt key press
            //
            Extend += 0xA3;
          } else {
            Extend = Extend+ Scan;
          }
          break;

        default:
          return FALSE;
      }

      //
      // Insert key in BDA key buffer.
      //
      InsertKey (lpBda, Extend);
    }

    return TRUE;
  }

  return FALSE;
}

/*
 _________________________________________________________________________________

 Deal with scan code.
 _________________________________________________________________________________
*/
VOID
TranslateKey (UINT8 ScanCode)
{
  UINT16      ExtendScanCode;
  LPBDA_SEG   lpBda;

  lpBda = GetBda ();
  //
  // Convert to extended scan code On BDA flag base.
  //
  ExtendScanCode = ExtendedScanCode (lpBda, ScanCode);

  //
  // Clear extended flag, if neccessary
  //
  if (ExtendScanCode > 0x100)
  {
    lpBda->ExtKbdSts1 &= ~(BIT0 | BIT1);
  }

  /*Legacy Free V1.0 do not need to deal with the response code and Keyboard ID Command
  //
  // Deal with response code
  //
  if (CheckResponseCode (lpBda, ScanCode)) {
    return;
  }

  //
  // Check keyboard ID
  //
  if (CheckReadIDCommand (lpBda, ScanCode)) {
    return;
  }
  */

  //
  // Deal with special Scan code
  //
  if (IsSpecialScode (lpBda,&ExtendScanCode)) {
    return;
  }

  //
  // Deal with normal Scan code
  //
  if (IsNormalCode (lpBda, ExtendScanCode)) {
    return;
  }

  //
  // Deal some special extended scan code
  //
  IsExtendCode (lpBda, ExtendScanCode);
}

EFI_STATUS
SendSwIRQ1(
VOID
)
/*++

 Routine Description:

    Send IRQ1.

 Arguments:

 Returns:
    None

--*/
{
  //
  // Step 1: clear tco IRQ1 bit
  //
  SmmIoWrite8(0x470,0x00);

  //
  // Step 2: Set TCO IRQ1 bit
  //
  SmmIoWrite8(0x470,0x01);

  return EFI_SUCCESS;
}

VOID
Int09ServiceSimulator(
    IN UINT8 Scan
)
{
  TranslateKey (Scan);
  //SendSwIRQ1();
}

VOID
KbdSendCmdData (
    IN  USB_KB_DEV       *UsbKeyboardDevice,
    IN  UINT8             Cmd,
    IN  UINT8             Data
)
/*++

  Routine Description:

  Send command and data into keyboard controller queue.

  Arguments:
    KbdCtrl - instance of keyboard controller data.
    Cmd - the command to send.
    Data - the data to send.

  Returns:

  None

--*/
{
    UINT8                   KbdTail, KbdNewTail;
    EFI_STATUS              Status;
    EFI_USB_IO_PROTOCOL     *UsbIo;

    KbdTail = UsbKeyboardDevice->CmdQueueTail;
    KbdNewTail = ((KbdTail + 1) % KBD_COMMAND_QUEUE_LENGTH);
    if (KbdNewTail == UsbKeyboardDevice->CmdQueueHead)
        return;

    //
    // Place a Key to Queue, update Tail
    //
    UsbKeyboardDevice->KbdCommandQueue[KbdTail].Data = Data;
    UsbKeyboardDevice->KbdCommandQueue[KbdTail].Command = Cmd;
    UsbKeyboardDevice->CmdQueueTail = KbdNewTail;
    UsbIo = UsbKeyboardDevice->UsbIo;

    if (!UsbKeyboardDevice->CmdQueuePollingActive) {
        Status = UsbIo->UsbAsyncInterruptOnlyTransfer (
                     UsbIo,
                     TRUE,
                     USBKBD_KBC_CMD_INTERVAL,
                     UsbKbdQueueHandler,
                     UsbKeyboardDevice
                 );
        if (!EFI_ERROR(Status))
            UsbKeyboardDevice->CmdQueuePollingActive = TRUE;
    }
}

BOOLEAN
UsbCheckBdaLed (
    IN  UINT8   WhichLed
)
{
    UINT8               *BdaPtr;

    BdaPtr = (UINT8 *)(UINTN)BDA_LED_PTR;
    if ((*BdaPtr & WhichLed) == WhichLed)
        return  TRUE;
    return  FALSE;
}

VOID
DoMultiKeyBreak (
    IN  USB_KB_DEV       *UsbKeyboardDevice,
    IN  UINT8             MKey
)
/*++

  Routine Description:
    do_multi_key_Break

  Arguments:
    Kbd:
    Uhci:
    MKey:

  Returns:

--*/
{
    UINT8   Code;

    MKey &= KEB_MULT_KEY_FILTER;
    Code = MKeyTable[MKey];

    Code |= KBD_BREAK_CODE_MARK;
    if (MKey == 1) {
        //
        // Pause
        //
        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            0xE1);

        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            Code);

        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            0xC5);

    } else if (MKey < 12) {

        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            0xE0);

        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            Code);
        if (MKey == 0) {

            KbdSendCmdData (
                UsbKeyboardDevice,
                KBD_KEYBOARD_DATA,
                0xE0);

            KbdSendCmdData (
                UsbKeyboardDevice,
                KBD_KEYBOARD_DATA,
                0xAA);
        }
        if (UsbCheckBdaLed(BDA_NUMBERLOCK)) {

            KbdSendCmdData (
                UsbKeyboardDevice,
                KBD_KEYBOARD_DATA,
                0xE0);

            KbdSendCmdData (
                UsbKeyboardDevice,
                KBD_KEYBOARD_DATA,
                0xAA);
        }
    } else {

        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            0xE0);

        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            Code);
    }
}

VOID
Send8042Break (
    IN  USB_KB_DEV       *UsbKeyboardDevice,
    IN  UINT8            InKey
)
{
    UINT8      ATCode;
    ATCode = UsageTable[InKey];

    if ((ATCode & 0x80) == 0x80) {

        DoMultiKeyBreak (
            UsbKeyboardDevice,
            ATCode);

    } else {

        ATCode |= 0x80;
        KbdSendCmdData (
            UsbKeyboardDevice,
            0xD2,
            ATCode);
    }
}

VOID
DoMultiKeyMake (
    IN  USB_KB_DEV       *UsbKeyboardDevice,
    IN  UINT8            MKey
)
/*++

  Routine Description:

  Arguments:
    Kbd:
    Uhci:
    MKey:

  Returns:

--*/
{
    MKey &= 0x7F;

    if (MKey == 1) {
        //
        // Pause
        //
        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            0xE1);

        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            MKeyTable[MKey]);

        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            0x45);

    } else if (MKey < 12) {
        if (MKey == 0) {

            KbdSendCmdData (
                UsbKeyboardDevice,
                KBD_KEYBOARD_DATA,
                0xE0);

            KbdSendCmdData (
                UsbKeyboardDevice,
                KBD_KEYBOARD_DATA,
                0xFF);
        }

        if (UsbCheckBdaLed (BDA_NUMBERLOCK)) {

            KbdSendCmdData (
                UsbKeyboardDevice,
                KBD_KEYBOARD_DATA,
                0xE0);

            KbdSendCmdData (
                UsbKeyboardDevice,
                KBD_KEYBOARD_DATA,
                0xFF);
        }

        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            0xE0);

        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            MKeyTable[MKey]);

    } else {
        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            0xE0);

        KbdSendCmdData(
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            MKeyTable[MKey]);
    }
}

VOID
Send8042Make (
    IN  USB_KB_DEV       *UsbKeyboardDevice,
    IN  UINT8            InKey
)
/*++

  Routine Description:

  Arguments:
    Kbd:
    Uhci:
    InKey:

  Returns:

--*/
{
    UINT8      ATCode;

    ATCode = UsageTable[InKey];

    if ((ATCode & AT_MULTI_KEY_MARK) == AT_MULTI_KEY_MARK) {
        DoMultiKeyMake (
            UsbKeyboardDevice,
            ATCode);
    } else {
        KbdSendCmdData (
            UsbKeyboardDevice,
            KBD_KEYBOARD_DATA,
            ATCode);
    }
}

BOOLEAN
WaitIBFEmpty (
)
/*++

  Routine Description:

  Arguments:

  Returns:
    BOOLEAN

--*/
{
  UINT8    KbdCmd;
  UINT16   TimeOut;

  for (TimeOut = 0; TimeOut < KBD_IN_EMPTY_COUNTER; TimeOut++) {
    KbdCmd = SmmKbcIoRead8 (KBD_CMD);
    if ((KbdCmd & KBD_IBE) == 0)
      return  TRUE;
  }
  return  FALSE;
}

BOOLEAN
SendKbdQueue (
    IN  USB_KB_DEV       *UsbKeyboardDevice
)
/*++

  Routine Description:

  Arguments:

  Returns:

--*/
{
  UINT8     Cmd;
  UINT8     Data;
  UINT8     Head;

  Head = UsbKeyboardDevice->CmdQueueHead;

  Cmd = UsbKeyboardDevice->KbdCommandQueue[Head].Command;
  if (!mLegacyFreeSupport) {
    SmmKbcIoWrite8 (KBD_CMD, Cmd);
    if ((Cmd != KBD_DISABLE_AUX) && (Cmd != KBD_ENABLE_AUX)) {
      if (!WaitIBFEmpty()){
          return FALSE;
      }
      Cmd = SmmKbcIoRead8 (KBD_CMD);
      if ((Cmd & KBD_OBF) == KBD_OBF) {
        return FALSE;
      }

      Data = UsbKeyboardDevice->KbdCommandQueue[Head].Data;
      SmmKbcIoWrite8(KBD_DATA_PORT, Data);
    }
  } else {
    if ((Cmd != KBD_DISABLE_AUX) && (Cmd != KBD_ENABLE_AUX)) {
      Data = UsbKeyboardDevice->KbdCommandQueue[Head].Data;
      Int09ServiceSimulator(Data);
    }
  }

  UsbKeyboardDevice->CmdQueueHead = ((Head + 1) % KBD_COMMAND_QUEUE_LENGTH);
  return  TRUE;
}

STATIC
EFI_STATUS
UsbKbdQueueHandler (
    IN VOID         *Data,
    IN UINTN        DataLength,
    IN VOID         *Context,
    IN UINT32       Result
)
{
    USB_KB_DEV            *UsbKeyboardDevice;
    UINT8                 KbdCmd = 0;
    EFI_USB_IO_PROTOCOL   *UsbIo;
    EFI_STATUS            Status;
    BOOLEAN               KbcBusy = FALSE;

    UsbKeyboardDevice = (USB_KB_DEV *)Context;
    UsbIo = UsbKeyboardDevice->UsbIo;

    if (Result != EFI_USB_NOERROR) {
        UsbIo->UsbAsyncInterruptOnlyTransfer (
            UsbIo,
            FALSE,
            USBKBD_KBC_CMD_INTERVAL,
            UsbKbdQueueHandler,
            UsbKeyboardDevice
        );
    }

  if (UsbKeyboardDevice->CmdQueueTail != UsbKeyboardDevice->CmdQueueHead) {
    if (!mLegacyFreeSupport) {
      KbdCmd = SmmKbcIoRead8 (KBD_CMD);
      if (((KbdCmd & KBD_IBE) == 0) && ((KbdCmd & KBD_OBF) == 0)) {
        SendKbdQueue (UsbKeyboardDevice);
      } else {
        KbcBusy = TRUE; 
      }
    } else {
      SendKbdQueue (UsbKeyboardDevice);
    }
  }
  if (KbcBusy) {
    UsbKeyboardDevice->SendCmdBlockCount++;
  }

  //
  // Analyzes the Result and performs corresponding action.
  //
  if (UsbKeyboardDevice->CmdQueueTail != UsbKeyboardDevice->CmdQueueHead) {
      UsbKeyboardDevice->CmdQueuePollingActive = TRUE;
      if (Result != EFI_USB_NOERROR) {
          Status = UsbIo->UsbAsyncInterruptOnlyTransfer (
                       UsbIo,
                       TRUE,
                       USBKBD_KBC_CMD_INTERVAL,
                       UsbKbdQueueHandler,
                       UsbKeyboardDevice
                   );
          if (EFI_ERROR(Status)) {
              UsbKeyboardDevice->CmdQueuePollingActive = FALSE;
          }
      } else {
          if (KbcBusy && (UsbKeyboardDevice->SendCmdBlockCount > 1)) {
            //
            //  Remove date on Data port,when KBC buzy.
            //
            if (!mLegacyFreeSupport) {
              KbdCmd = SmmKbcIoRead8 (KBD_DATA_PORT);
              USB_DEBUG ((EFI_D_ERROR, "usb remove data\n"));
            }
          }
      }
  } else {
      UsbKeyboardDevice->CmdQueuePollingActive = FALSE;
      if (Result == EFI_USB_NOERROR) {
          Status = UsbIo->UsbAsyncInterruptOnlyTransfer (
                       UsbIo,
                       FALSE,
                       USBKBD_KBC_CMD_INTERVAL,
                       UsbKbdQueueHandler,
                       UsbKeyboardDevice
                   );
          if (EFI_ERROR(Status)) {
              UsbKeyboardDevice->CmdQueuePollingActive = TRUE;
          }
          UsbKeyboardDevice->SendCmdBlockCount = 0;
      }
  }
  
  return EFI_SUCCESS;
}

EFI_STATUS
InitUsbKeyboard (
    IN USB_KB_DEV   *UsbKeyboardDevice
)
/*++

  Routine Description:
    Initialize USB Keyboard device and all private data structures.

  Arguments:
    UsbKeyboardDevice    The USB_KB_DEV instance.

  Returns:
    EFI_SUCCESS      - Success
    EFI_DEVICE_ERROR - Hardware Error
--*/
{
    UINT8               ConfigValue;
    UINT8               Protocol;
    UINT8               ReportId;
    UINT8               Duration;
    EFI_STATUS          Status;
    UINT32              TransferResult;
    EFI_USB_IO_PROTOCOL *UsbIo;

    UsbIo = UsbKeyboardDevice->UsbIo;
  //
  // Use the config out of the descriptor
  // Assumed the first config is the correct one and this is not always the case
  //
  Status = UsbGetConfiguration (
             UsbKeyboardDevice->UsbIo,
             &ConfigValue,
             &TransferResult
             );
  if (EFI_ERROR (Status)) {
    //
    // default configurations
    //
    ConfigValue = 0x01;

    //
    // Uses default configuration to configure the USB Keyboard device.
    //
    Status = UsbSetConfiguration (
                 UsbKeyboardDevice->UsbIo,
                 (UINT16) ConfigValue,
                 &TransferResult
             );
    if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
    }
  }

    UsbGetProtocolRequest (
        UsbKeyboardDevice->UsbIo,
        UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
        &Protocol
    );
    //
    // Sets boot protocol for the USB Keyboard.
    // This driver only supports boot protocol.
    // !!BugBug: How about the device that does not support boot protocol?
    //
    if (Protocol != BOOT_PROTOCOL) {
        UsbSetProtocolRequest (
            UsbKeyboardDevice->UsbIo,
            UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
            BOOT_PROTOCOL
        );
    }
    //
    // the duration is indefinite, so the endpoint will inhibit reporting forever,
    // and only reporting when a change is detected in the report data.
    //

    //
    // idle value for all report ID
    //
    ReportId = 0;

    //
    // idle forever until there is a key pressed and released.
    //
    Duration = 0;
    UsbSetIdleRequest (
        UsbKeyboardDevice->UsbIo,
        UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
        ReportId,
        Duration
    );

    UsbKeyboardDevice->NumLockOn  = 0;
    UsbKeyboardDevice->CapsOn     = 0;
    UsbKeyboardDevice->ScrollOn   = 0;

    //
    // Sync the initial state of lights
    //
    SetKeyLED (UsbKeyboardDevice);

    ZeroMem (UsbKeyboardDevice->LastKeyCodeArray, sizeof (UINT8) * 8);
    ZeroMem (UsbKeyboardDevice->KbdCommandQueue, sizeof (UsbKeyboardDevice->KbdCommandQueue));
    UsbKeyboardDevice->CmdQueueHead = 0;
    UsbKeyboardDevice->CmdQueueTail = 0;
    UsbKeyboardDevice->RepeatDelayCount = 0;
    UsbKeyboardDevice->RepeatKey = 0;
    UsbKeyboardDevice->RepeatKeyPollingActive = FALSE;
    UsbKeyboardDevice->CmdQueuePollingActive = FALSE;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
KeyboardHandler (
    IN  VOID          *Data,
    IN  UINTN         DataLength,
    IN  VOID          *Context,
    IN  UINT32        Result
)
/*++

  Routine Description:
    Handler function for USB Keyboard's asynchronous interrupt transfer.

  Arguments:
    Data       A pointer to a buffer that is filled with key data which is
               retrieved via asynchronous interrupt transfer.
    DataLength Indicates the size of the data buffer.
    Context    Pointing to USB_KB_DEV instance.
    Result     Indicates the result of the asynchronous interrupt transfer.

  Returns:
    EFI_SUCCESS      - Success
    EFI_DEVICE_ERROR - Hardware Error
--*/
{
    USB_KB_DEV          *UsbKeyboardDevice;
    EFI_USB_IO_PROTOCOL *UsbIo;
    UINT8               *CurKeyCodeBuffer;
    UINT8               *OldKeyCodeBuffer;
    UINT8               CurModifierMap;
    UINT8               OldModifierMap;
    UINT8               Index;
    UINT8               Index2;
    BOOLEAN             Down;
    EFI_STATUS          Status;
    BOOLEAN             KeyRelease;
    BOOLEAN             KeyPress;
    UINT8               NewRepeatKey;
    UINT32              UsbStatus;
    UINT8               *DataPtr;
    UINT8               ScanCode;

    ASSERT (Context);

    NewRepeatKey      = 0;
    DataPtr           = (UINT8 *) Data;
    UsbKeyboardDevice = (USB_KB_DEV *) Context;
    UsbIo             = UsbKeyboardDevice->UsbIo;

    //
    // Analyzes the Result and performs corresponding action.
    //
    if (Result != EFI_USB_NOERROR) {
        //
        // stop the repeat key generation if any
        //
        UsbKeyboardDevice->RepeatKey = 0;
        UsbKeyboardDevice->RepeatDelayCount = 0;
        UsbKeyboardDevice->RepeatKeyPollingActive = FALSE;
        UsbIo->UsbAsyncInterruptOnlyTransfer (
            UsbIo,
            FALSE,
            USBKBD_REPEAT_RATE,
            UsbKeyboardRepeatHandler,
            UsbKeyboardDevice
        );

        if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
            UsbClearEndpointHalt (
                UsbIo,
                UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
                &UsbStatus
            );
        }

        //
        // Delete & Submit this interrupt again
        //
        Status = UsbIo->UsbAsyncInterruptTransfer (
                     UsbIo,
                     UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
                     FALSE,
                     0,
                     0,
                     NULL,
                     NULL
                 );
        if (!EFI_ERROR(Status)) {
            UsbIo->UsbAsyncInterruptOnlyTransfer (
                UsbIo,
                TRUE,
                USBKBD_RECOVERY_DELAY,
                UsbKeyboardRecoveryHandler,
                UsbKeyboardDevice
            );
        }
        return EFI_DEVICE_ERROR;
    }

    if (DataLength == 0 || Data == NULL) {
        return EFI_SUCCESS;
    }

    CurKeyCodeBuffer  = (UINT8 *) Data;
    OldKeyCodeBuffer  = UsbKeyboardDevice->LastKeyCodeArray;

    //
    // checks for new key stroke.
    // if no new key got, return immediately.
    //
    for (Index = 0; Index < 8; Index++) {
        if (OldKeyCodeBuffer[Index] != CurKeyCodeBuffer[Index]) {
            break;
        }
    }
    if (Index == 8) {
        return EFI_SUCCESS;
    }

    //
    // Parse the modifier key
    //
    CurModifierMap  = CurKeyCodeBuffer[0];
    OldModifierMap  = OldKeyCodeBuffer[0];
    //
    // handle modifier key's pressing or releasing situation.
    //
    for (Index = 0; Index < 8; Index++) {
        if ((CurModifierMap & KB_Mod[Index].Mask) != (OldModifierMap & KB_Mod[Index].Mask)) {
            //
            // if current modifier key is up, then
            // CurModifierMap & KB_Mod[Index].Mask = 0;
            // otherwize it is a non-zero value.
            // Inserts the pressed modifier key into key buffer.
            //
            Down = (UINT8) (CurModifierMap & KB_Mod[Index].Mask);
            ScanCode = KB_Mod[Index].ScanCode;
            if (Down) {
                if ((ScanCode & KBD_MULT_KEY_MARK) != 0) {
                    KbdSendCmdData (
                        UsbKeyboardDevice,
                        KBD_KEYBOARD_DATA,
                        0xE0);
                    ScanCode &= KEB_MULT_KEY_FILTER;
                }
                KbdSendCmdData (
                    UsbKeyboardDevice,
                    KBD_KEYBOARD_DATA,
                    ScanCode);
            } else {
                if ((ScanCode & KBD_MULT_KEY_MARK) != 0) {
                    KbdSendCmdData (
                        UsbKeyboardDevice,
                        KBD_KEYBOARD_DATA,
                        0xE0);
                } else
                    ScanCode |= KBD_MULT_KEY_MARK;
                KbdSendCmdData (
                    UsbKeyboardDevice,
                    KBD_KEYBOARD_DATA,
                    ScanCode);
            }
        }
    }

    //
    // handle normal key's releasing situation
    //
    KeyRelease = FALSE;
    for (Index = 2; Index < 8; Index++) {

        if (!USBKBD_VALID_KEYCODE (OldKeyCodeBuffer[Index])) {
            continue;
        }

        KeyRelease = TRUE;
        for (Index2 = 2; Index2 < 8; Index2++) {
            if (!USBKBD_VALID_KEYCODE (CurKeyCodeBuffer[Index2])) {
                continue;
            }

            if (OldKeyCodeBuffer[Index] == CurKeyCodeBuffer[Index2]) {
                KeyRelease = FALSE;
                break;
            }
        }

        if (KeyRelease) {
            //
            // the original reapeat key is released.
            //
            if (OldKeyCodeBuffer[Index] == UsbKeyboardDevice->RepeatKey) {
                UsbKeyboardDevice->RepeatKey = 0;
            }

            Send8042Break (
                UsbKeyboardDevice,
                OldKeyCodeBuffer[Index]);
        }
    }

    //
    // original repeat key is released, cancel the repeat timer
    //
    if (UsbKeyboardDevice->RepeatKey == 0) {
        UsbIo->UsbAsyncInterruptOnlyTransfer (
            UsbIo,
            FALSE,
            USBKBD_REPEAT_RATE,
            UsbKeyboardRepeatHandler,
            UsbKeyboardDevice
        );
        UsbKeyboardDevice->RepeatDelayCount = 0;
        UsbKeyboardDevice->RepeatKeyPollingActive = FALSE;
    }

    //
    // handle normal key's pressing situation
    //
    KeyPress = FALSE;
    for (Index = 2; Index < 8; Index++) {

        if (!USBKBD_VALID_KEYCODE (CurKeyCodeBuffer[Index])) {
            continue;
        }

        KeyPress = TRUE;
        for (Index2 = 2; Index2 < 8; Index2++) {

            if (!USBKBD_VALID_KEYCODE (OldKeyCodeBuffer[Index2])) {
                continue;
            }

            if (CurKeyCodeBuffer[Index] == OldKeyCodeBuffer[Index2]) {
                KeyPress = FALSE;
                break;
            }
        }

        if (KeyPress) {
            //
            // NumLock pressed or CapsLock pressed
            //
            if (CurKeyCodeBuffer[Index] == 0x53 || CurKeyCodeBuffer[Index] == 0x39) {
                UsbKeyboardDevice->RepeatKey = 0;
            } else {
                NewRepeatKey = CurKeyCodeBuffer[Index];
                //
                // do not repeat the original repeated key
                //
                UsbKeyboardDevice->RepeatKey = 0;
            }

            Send8042Make (
                UsbKeyboardDevice,
                CurKeyCodeBuffer[Index]);
        }
    }

    // SetKeyLED (UsbKeyboardDevice);
    // UsbSetLED ();
    //
    // Update LastKeycodeArray[] buffer in the
    // Usb Keyboard Device data structure.
    //
    for (Index = 0; Index < 8; Index++) {
        UsbKeyboardDevice->LastKeyCodeArray[Index] = CurKeyCodeBuffer[Index];
    }

    //
    // If have new key pressed, update the RepeatKey value, and set the
    // timer to repeate delay timer
    //
    if (NewRepeatKey != 0) {
        //
        // sets trigger time to "Repeat Delay Time",
        // to trigger the repeat timer when the key is hold long
        // enough time.
        //
        UsbKeyboardDevice->RepeatDelayCount = 0;
        UsbKeyboardDevice->RepeatKey = NewRepeatKey;
        if (!UsbKeyboardDevice->RepeatKeyPollingActive) {

            UsbIo->UsbAsyncInterruptOnlyTransfer (
                UsbIo,
                TRUE,
                USBKBD_REPEAT_RATE,
                UsbKeyboardRepeatHandler,
                UsbKeyboardDevice
            );
            UsbKeyboardDevice->RepeatKeyPollingActive = TRUE;
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS
SetKeyLED (
    IN  USB_KB_DEV    *UsbKeyboardDevice
)
/*++

  Routine Description:
    Sets USB Keyboard LED state.

  Arguments:
    UsbKeyboardDevice - The USB_KB_DEV instance.

  Returns:
    EFI_SUCCESS - Success
--*/
{
    LED_MAP        Led;
    UINT8          ReportId;
    BDA_LED_MAP    *BdaLed;

    BdaLed = (BDA_LED_MAP *)((UINT8 *)(UINTN)BDA_LED_PTR);

    //
    // Set each field in Led map.
    //
    UsbKeyboardDevice->NumLockOn = BdaLed->NumLock;
    UsbKeyboardDevice->CapsOn    = BdaLed->CapsLock;
    UsbKeyboardDevice->ScrollOn  = BdaLed->ScrollLock;

    //
    // Set each field in Led map.
    //
    Led.NumLock    = (UINT8) UsbKeyboardDevice->NumLockOn;
    Led.CapsLock   = (UINT8) UsbKeyboardDevice->CapsOn;
    Led.ScrollLock = (UINT8) UsbKeyboardDevice->ScrollOn;
    Led.Resrvd     = 0;

    //
    // call Set Report Request to lighten the LED.
    //
    ReportId       = 0;

    UsbSetReportRequest (
        UsbKeyboardDevice->UsbIo,
        UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
        ReportId,
        HID_OUTPUT_REPORT,
        1,
        (CHAR8 *) &Led
    );
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbKeyboardRepeatHandler (
    IN  VOID          *Data,
    IN  UINTN         DataLength,
    IN  VOID          *Context,
    IN  UINT32        Result
)
/*++

  Routine Description:
    Timer handler for Repeat Key timer.

  Arguments:
    Event   - The Repeat Key event.
    Context - Points to the USB_KB_DEV instance.

  Returns:

--*/
{
    USB_KB_DEV          *UsbKeyboardDevice;
    EFI_USB_IO_PROTOCOL *UsbIo;

    UsbKeyboardDevice = (USB_KB_DEV *) Context;
    UsbIo             = UsbKeyboardDevice->UsbIo;

    if (Result != EFI_USB_NOERROR) {
        UsbIo->UsbAsyncInterruptOnlyTransfer (
            UsbIo,
            FALSE,
            USBKBD_REPEAT_RATE,
            UsbKeyboardRepeatHandler,
            UsbKeyboardDevice
        );
        UsbIo->UsbAsyncInterruptOnlyTransfer (
            UsbIo,
            TRUE,
            USBKBD_REPEAT_RATE,
            UsbKeyboardRepeatHandler,
            UsbKeyboardDevice
        );
    }

    //
    // Do nothing when there is no repeat key.
    //
    if (UsbKeyboardDevice->RepeatKey != 0) {
        if (UsbKeyboardDevice->RepeatDelayCount < (USBKBD_REPEAT_DELAY / USBKBD_REPEAT_RATE)) {
            UsbKeyboardDevice->RepeatDelayCount++;
            return EFI_SUCCESS;
        }

        //
        // Inserts one Repeat key into keyboard buffer,
        //
        Send8042Make (
            UsbKeyboardDevice,
            UsbKeyboardDevice->RepeatKey);
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbKeyboardRecoveryHandler (
    IN  VOID          *Data,
    IN  UINTN         DataLength,
    IN  VOID          *Context,
    IN  UINT32        Result
)
/*++

  Routine Description:
    Timer handler for Delayed Recovery timer.

  Arguments:
    Event   -  The Delayed Recovery event.
    Context -  Points to the USB_KB_DEV instance.

  Returns:

--*/
{
    USB_KB_DEV          *UsbKeyboardDevice;
    EFI_USB_IO_PROTOCOL *UsbIo;
    UINT8               PacketSize;

    UsbKeyboardDevice = (USB_KB_DEV *) Context;

    UsbIo             = UsbKeyboardDevice->UsbIo;

    PacketSize        = (UINT8) (UsbKeyboardDevice->IntEndpointDescriptor.MaxPacketSize);

    UsbIo->UsbAsyncInterruptTransfer (
        UsbIo,
        UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
        TRUE,
        UsbKeyboardDevice->IntEndpointDescriptor.Interval,
        PacketSize,
        KeyboardHandler,
        UsbKeyboardDevice
    );

    UsbIo->UsbAsyncInterruptOnlyTransfer (
        UsbIo,
        FALSE,
        USBKBD_RECOVERY_DELAY,
        UsbKeyboardRecoveryHandler,
        UsbKeyboardDevice
    );
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SynchronizeKeyboardLed (
    IN  VOID          *Data,
    IN  UINTN         DataLength,
    IN  VOID          *Context,
    IN  UINT32        Result
)
/*++

  Routine Description:
    Synchronize the Keyboard Led.

  Arguments:
    Data
    DataLength
    Context
    Result

  Returns:
    EFI_SUCCESS      - Success
    EFI_DEVICE_ERROR - Hardware Error
--*/
{
    USB_KB_DEV          *UsbKeyboardDevice;
    EFI_USB_IO_PROTOCOL *UsbIo;

    UsbKeyboardDevice = (USB_KB_DEV *) Context;
    UsbIo             = UsbKeyboardDevice->UsbIo;

    //
    // Sync the Led
    //
    SetKeyLED (UsbKeyboardDevice);

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
USBKeyboardReset (
    IN  USB_KB_DEV          *UsbKeyboardDevice,
    IN  BOOLEAN             ExtendedVerification
)
/*++

  Routine Description:
    Implements EFI_SIMPLE_TEXT_IN_PROTOCOL.Reset() function.

  Arguments:
    This      The EFI_SIMPLE_TEXT_IN_PROTOCOL instance.
    ExtendedVerification
              Indicates that the driver may perform a more exhaustive
              verification operation of the device during reset.

  Returns:
    EFI_SUCCESS      - Success
    EFI_DEVICE_ERROR - Hardware Error
--*/
{
    EFI_STATUS          Status;
    EFI_USB_IO_PROTOCOL *UsbIo;

    UsbIo             = UsbKeyboardDevice->UsbIo;

    //
    // Non Exhaustive reset:
    // only reset private data structures.
    //
    if (!ExtendedVerification) {

        return EFI_SUCCESS;
    }

    //
    // Exhaustive reset
    //
    Status = InitUsbKeyboard (UsbKeyboardDevice);
    if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbConnectKeyboardDriver (
    IN USB_INTERFACE        *UsbIf
)
/*++

  Routine Description:
    Start.

  Arguments:
    UsbIf       - EFI_DRIVER_BINDING_PROTOCOL
    Controller - Controller handle
    RemainingDevicePath - EFI_DEVICE_PATH_PROTOCOL
  Returns:
    EFI_SUCCESS          - Success
    EFI_OUT_OF_RESOURCES - Can't allocate memory
    EFI_UNSUPPORTED      - The Start routine fail
--*/
{
    EFI_STATUS                    Status;
    EFI_USB_IO_PROTOCOL           *UsbIo;
    USB_KB_DEV                    *UsbKeyboardDevice;
    UINT8                         EndpointNumber;
    EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
    UINT8                         Index;
    UINT8                         EndpointAddr;
    UINT8                         PollingInterval;
    UINT8                         PacketSize;
    BOOLEAN                       Found;
    USB_CFG                       *UsbCfg;

    UsbKeyboardDevice = NULL;
    Found             = FALSE;

    UsbIo = &UsbIf->UsbIo;
    UsbKeyboardDevice = AllocateZeroPool(sizeof (USB_KB_DEV));
    if (UsbKeyboardDevice == NULL)
        return EFI_OUT_OF_RESOURCES;

    //
    // Initialize UsbKeyboardDevice
    //
    UsbKeyboardDevice->UsbIo = UsbIo;
    UsbIf->DriverContext[UsbIf->LunCount] = UsbKeyboardDevice;
    UsbIf->LunCount++;

    //
    // Get interface & endpoint descriptor
    //
    UsbIo->UsbGetInterfaceDescriptor (
        UsbIo,
        &UsbKeyboardDevice->InterfaceDescriptor
    );

    EndpointNumber = UsbKeyboardDevice->InterfaceDescriptor.NumEndpoints;

    for (Index = 0; Index < EndpointNumber; Index++) {
        UsbIo->UsbGetEndpointDescriptor (
            UsbIo,
            Index,
            &EndpointDescriptor
        );
        EndpointAddr = EndpointDescriptor.EndpointAddress;
        if (((EndpointDescriptor.Attributes & 0x03) == 0x03) && (EndpointAddr & 0x80) == 0x80) {
            //
            // We only care interrupt endpoint here
            //
            UsbKeyboardDevice->IntEndpointDescriptor  = EndpointDescriptor;
            Found = TRUE;
            break;
        }
    }

    if (!Found) {
        //
        // No interrupt endpoint found, then return unsupported.
        //
        FreePool (UsbKeyboardDevice);
        return EFI_UNSUPPORTED;
    }

    UsbKeyboardDevice->Signature = USB_KB_DEV_SIGNATURE;

    //
    // Set USB Configuration Information
    //
    GET_USB_CFG(UsbCfg);
    UsbCfg->UsbDeviceInfor.HasUSBKeyboard = 1;

    //
    // Reset USB Keyboard Device
    //
    Status = USBKeyboardReset (
                 UsbKeyboardDevice,
                 TRUE
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    //
    // submit async interrupt transfer
    //
    EndpointAddr    = UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress;
    PollingInterval = UsbKeyboardDevice->IntEndpointDescriptor.Interval;
    PacketSize      = (UINT8) (UsbKeyboardDevice->IntEndpointDescriptor.MaxPacketSize);
	if (PollingInterval<20)
		  PollingInterval= 20;

    Status = UsbIo->UsbAsyncInterruptTransfer (
                 UsbIo,
                 EndpointAddr,
                 TRUE,
                 PollingInterval,
                 PacketSize,
                 KeyboardHandler,
                 UsbKeyboardDevice
             );

    if (EFI_ERROR (Status)) {
        FreePool (UsbKeyboardDevice);
        return Status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbDisconnectKeyboardDriver (
    IN USB_INTERFACE        *UsbIf
)
/*++

  Routine Description:
    Stop.

  Arguments:
    This              - EFI_DRIVER_BINDING_PROTOCOL
    Controller        - Controller handle
    NumberOfChildren  - Child handle number
    ChildHandleBuffer - Child handle buffer
  Returns:
    EFI_SUCCESS       - Success
    EFI_UNSUPPORTED   - Can't support
--*/
{
    USB_KB_DEV                  *UsbKeyboardDevice;
    EFI_USB_IO_PROTOCOL         *UsbIo;

    UsbKeyboardDevice = (USB_KB_DEV *) (UsbIf->DriverContext[0]);
    UsbIo = UsbKeyboardDevice->UsbIo;

    //
    // Uninstall the Asyn Interrupt Transfer from this device
    // will disable the key data input from this device
    //

    //
    // Destroy asynchronous interrupt transfer
    //
    UsbIo->UsbAsyncInterruptTransfer (
        UsbIo,
        UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
        FALSE,
        UsbKeyboardDevice->IntEndpointDescriptor.Interval,
        0,
        NULL,
        NULL
    );

    UsbIo->UsbAsyncInterruptOnlyTransfer (
        UsbIo,
        FALSE,
        USBKBD_REPEAT_RATE,
        UsbKeyboardRepeatHandler,
        UsbKeyboardDevice
    );

    UsbIo->UsbAsyncInterruptOnlyTransfer (
        UsbIo,
        FALSE,
        USBKBD_KBC_CMD_INTERVAL,
        UsbKbdQueueHandler,
        UsbKeyboardDevice
    );

    UsbIo->UsbAsyncInterruptOnlyTransfer (
        UsbIo,
        FALSE,
        USBKBD_RECOVERY_DELAY,
        UsbKeyboardRecoveryHandler,
        UsbKeyboardDevice
    );

    FreePool (UsbKeyboardDevice);

    return EFI_SUCCESS;
}
