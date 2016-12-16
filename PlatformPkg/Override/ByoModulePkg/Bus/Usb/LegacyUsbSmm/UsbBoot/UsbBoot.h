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
  UsbBoot.h

Abstract:
  USB Module file.

Revision History:

  Bug 1989:   Changed to use dynamic software SMI value instead of hard coding.
  TIME:       2011-6-15
  $AUTHOR:    Peng Xianbing
  $REVIEWERS:
  $SCOPE:     Define SwSmi value range build a PolicyData table for csm16 to
              get SwSMI value.
  $TECHNICAL:
  $END-------------------------------------------------------------------------
--*/

#ifndef _USB_BOOT_H_
#define _USB_BOOT_H_

#include "UsbMassImpl.h"
#include "../UsbLib/UsbMem.h"

#define USBSyncLED_SMI                  0x8A
#define CSM16_SMMCALL_SMI_VALUE         0x89
#define USB_LEGACY_DIS                  0x87

#define CSM16_DISABLE_USB_DEVICE        0x00
#define CSM16_GET_USB_BOOT_DEVICE_INFO  0x01
#define CSM16_READ_USB_BOOT_DEVICE      0x02
#define CSM16_WRITE_USB_BOOT_DEVICE     0x03
#define CSM16_VERIFY_USB_BOOT_DEVICE    0x04

//
// UMS_INFO->UMS_INFO_ENTRY->PDT
//
#define USB_HDD                             0x00
#define USB_UFI_FDD                         0x01
#define USB_BULK_ZIPFDD                     0x02
#define USB_CDROM                           0x03
#define MaxDevType                          0x04

//
// UMS_INFO->Codebase
//
#define Tiano                               0x00
#define ICBO                                0x01
#define MAX_UMS_UNM                         0x08

#pragma pack(1)

typedef struct _CSM16_SMMCALL_COMM {
    UINT8  CallId;
    UINT32 StatusCode;
    UINT32 Arguments[1];
} CSM16_SMMCALL_COMM;

typedef struct _UMS_INFO_ENTRY {
    UINT8       PDT;
    UINT8       Bus;
    UINT8       Device;
    UINT8       Function;
    UINT32      BlockSize;
    UINT32      Head;
    UINT32      Sector;
    UINT32      Cylinder;
    UINT32      LastBlock[2];
    UINT8       ManufacturerDescAStr[0x20];
    UINT8       ProductDescAStr[0x20];
    UINT32      Handle[2];
    UINT8       USBHDDOriginalType;
    UINT8       FileType;
    UINT32      HiddenSector;
    UINT8       UsbDevParentAddr;
    UINT8       UsbDevParentPort;
    UINT8       Reserved[0x01];
} UMS_INFO_ENTRY;

typedef struct _UMS_INFO {
    UINT8             NumOfUms;
    UMS_INFO_ENTRY    UmsInfoEntry[MAX_UMS_UNM];
    UINT8             Reserved[0x16];
} UMS_INFO;

#pragma pack()

VOID
Csm16GetUsbBootDeviceInfo(
    VOID
)
;

UINT32
Csm16ReadUsbBootDevice (
    IN UINT64        Handle,
    IN EFI_LBA       Lba,
    IN UINT32        BufferSize,
    IN UINT32        BufferAddress
)
;

UINT32
Csm16WriteUsbBootDevice (
    IN UINT64        Handle,
    IN EFI_LBA       Lba,
    IN UINT32        BufferSize,
    IN UINT32        BufferAddress
)
;

UINT32
Csm16VerifyUsbBootDevice (
    IN UINT64         Handle,
    IN EFI_LBA        Lba,
    IN UINT32        VerifySize
)
;

EFI_STATUS
EFIAPI
UsbMassVerifyBlocks(
    IN EFI_BLOCK_IO_PROTOCOL    *This,
    IN UINT32                   MediaId,
    IN EFI_LBA                  Lba,
    IN UINTN                   VerSize
)
;

EFI_STATUS
EFIAPI
UsbMassWriteBlocks (
    IN EFI_BLOCK_IO_PROTOCOL    *This,
    IN UINT32                   MediaId,
    IN EFI_LBA                  Lba,
    IN UINTN                    BufferSize,
    OUT VOID                    *Buffer
)
;

EFI_STATUS
EFIAPI
UsbMassReadBlocks (
    IN EFI_BLOCK_IO_PROTOCOL    *This,
    IN UINT32                   MediaId,
    IN EFI_LBA                  Lba,
    IN UINTN                    BufferSize,
    OUT VOID                    *Buffer
)
;

#pragma warning ( disable : 4244 )
#pragma warning ( disable : 4305 )

#endif
