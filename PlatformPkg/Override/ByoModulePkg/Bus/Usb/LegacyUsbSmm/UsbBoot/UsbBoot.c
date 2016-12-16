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
  UsbBoot.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "UsbBoot.h"
#include "../Ehci/Ehci.h"
#include "../Uhci/Uhci.h"
#include "../Ohci/Ohci.h"
#include "../Xhci/Xhci.h"

UMS_INFO *mUmsInfo = (UMS_INFO *)(UINTN)0xEFC80;

extern LIST_ENTRY   mUsbMassStorageList;

VOID
StringU2A (
    OUT CHAR8         *AsciiStr,
    IN  CHAR16        *UnicodeStr,
    IN  INTN          AsciiStrLen
)
{
    INTN  Index;

    if (AsciiStrLen == 0) {
        AsciiStrLen = -1;
    }

    for (Index = 0; Index < AsciiStrLen; Index++) {
        *AsciiStr = (CHAR8) *(UnicodeStr++);

        if (*(AsciiStr++) == '\0') {
            return;
        }
    }

    *AsciiStr = '\0';
}

BOOLEAN
EFIAPI
IsDeviceInFddEmulationMode (
    IN EFI_BLOCK_IO_PROTOCOL     *BlockIo
)
/*++
Routine Description:
  Check the boot block to see if the disk is in FDD emulation mode.

Arguments:
  BlockIo - Block IO protocol.

Returns:
  BOOLEAN
--*/
{
    UINT8                       *Buffer;
    EFI_BLOCK_IO_MEDIA          *BlkMedia;
    BOOLEAN                     IsFddMode;
    EFI_STATUS                  Status;

    IsFddMode = FALSE;
    BlkMedia  = BlockIo->Media;

    Buffer = UsbAllocatePool (BlkMedia->BlockSize);
    if (Buffer) {
        Status = BlockIo->ReadBlocks (
                     BlockIo,
                     BlockIo->Media->MediaId,
                     0,
                     BlkMedia->BlockSize,
                     Buffer
                 );

        if (!EFI_ERROR (Status)) {
            if (*(UINT16*)((UINTN)Buffer + 0x1FE) == 0xAA55) {
                if ((*(UINT8*)((UINTN)Buffer + 0x26) == 0x29)
                        || (*(UINT8*)((UINTN)Buffer + 0x42) == 0x29)) {
                    IsFddMode = TRUE;
                }
            }

            UsbFreePool (Buffer, BlkMedia->BlockSize);
        }
    }

    return IsFddMode;
}

UINT8
EFIAPI
GetUsbDiskDeviceType (
    IN EFI_BLOCK_IO_PROTOCOL     *BlockIo
)
{
    USB_MASS_DEVICE               *UsbMass;
    EFI_USB_IO_PROTOCOL           *UsbIo;
    EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
    UINT8                         DeviceType;

    UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (BlockIo);
    UsbIo = UsbMass->UsbIo;

    UsbIo->UsbGetInterfaceDescriptor(UsbIo, &InterfaceDescriptor);

    if (InterfaceDescriptor.InterfaceSubClass == USB_MASS_STORE_UFI) {
        DeviceType = USB_UFI_FDD;
    } else if (InterfaceDescriptor.InterfaceSubClass == USB_MASS_STORE_8070I) {
        DeviceType = USB_BULK_ZIPFDD;
    } else if (UsbMass->OpticalStorage) {
        DeviceType = USB_CDROM;
    } else {
        if (IsDeviceInFddEmulationMode(BlockIo)) {
            DeviceType = USB_BULK_ZIPFDD;
        } else {
            DeviceType = USB_HDD;
        }
    }

    return DeviceType;
}

VOID
GetUsbPciAddress(
    IN EFI_USB_IO_PROTOCOL *UsbIo,
    OUT EFI_PCI_FUNCTION_ADDRESS **Pfa
)
{

    USB_DEVICE                *Dev;
    USB_INTERFACE             *UsbIf;
    EFI_USB2_HC_PROTOCOL      *Usb2Hc;
    USB_HC_DEV                *Uhc;
    USB2_HC_DEV               *Ehc;
    USB_OHCI_DEV              *Ohc;
    USB_XHCI_INSTANCE         *Xhc;

    UsbIf  = USB_INTERFACE_FROM_USBIO (UsbIo);
    Dev    = UsbIf->Device;
    Usb2Hc = Dev->Bus->Usb2Hc;

    Uhc = _CR(Usb2Hc, USB_HC_DEV, Usb2Hc);
    if (Uhc->Signature == (UINTN)USB_HC_DEV_SIGNATURE) {
        *Pfa = &Uhc->PciAddress;
        return;
    }

    Ehc = _CR(Usb2Hc, USB2_HC_DEV, Usb2Hc);
    if (Ehc->Signature == (UINTN)USB2_HC_DEV_SIGNATURE) {
        *Pfa = &Ehc->PciAddress;
         return;
    }

    Ohc = _CR(Usb2Hc, USB_OHCI_DEV, Usb2Hc);
    if (Ohc->Signature == (UINTN)USB_OHCI_DEV_SIGNATURE) {
        *Pfa = &Ohc->PciAddress;
        return;
    }

    Xhc = _CR(Usb2Hc, USB_XHCI_INSTANCE, Usb2Hc);
    if (Xhc->Signature == (UINTN)XHCI_INSTANCE_SIG) {
        *Pfa = &Xhc->PciAddress;
    }

    return;
}

EFI_STATUS
CheckMassLunIllegle(
    IN EFI_BLOCK_IO_PROTOCOL     *BlockIo
)
{
  EFI_STATUS                 Status;
  VOID                       *Buffer;
  USB_MASS_DEVICE            *UsbMass;
  EFI_BLOCK_IO_MEDIA         *BlkMedia;

  BlkMedia  = BlockIo->Media;

  UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (BlockIo);

  Buffer = UsbAllocatePool (512);

  Status = UsbMassReadBlocks(BlockIo, BlkMedia->MediaId, 0, 0x200, Buffer);
  if (*(UINT16*)((UINTN)Buffer + 0x1FE) != 0xAA55) {
      Status = EFI_UNSUPPORTED;
  }
  UsbFreePool(Buffer,512);

  return  Status;
}



VOID
EliminateExtraSpaces (
  IN CHAR8                    *Str
  )
{
  UINTN                        Index;
  UINTN                        ActualIndex;
  
  for (Index = 0, ActualIndex = 0; Str[Index] != 0; Index++) {
    if (Str[Index] != ' ' || ((ActualIndex > 0) && (Str[ActualIndex - 1] != ' '))) {
      Str[ActualIndex++] = Str[Index];
    }
  }
  Str[ActualIndex] = 0;
}

VOID
Csm16GetUsbBootDeviceInfo(
  VOID
)
{
  USB_MASS_DEVICE               *UsbMass;
  UINTN                         Index;
  LIST_ENTRY                    *Link;
  EFI_PCI_FUNCTION_ADDRESS      *Pfa;
//UINT8                         StrIndex;
  USB_INTERFACE                 *UsbIf;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  USB_DEVICE                    *UsbDevice;
  EFI_STATUS                    Status;
  UINT8                         PDTType;
  UINT8                         DeviceType;
  UINT8                         *DescStr;                         
//BOOLEAN                       StrFound   = FALSE;
//UINT8                         StartIndex = 0;

  Index = 0;
  for (DeviceType = USB_HDD; DeviceType < MaxDevType; DeviceType ++) {
    Link  = mUsbMassStorageList.ForwardLink;
    while (Link != &mUsbMassStorageList && Index < 0x08) {
      UsbMass = _CR(Link ,USB_MASS_DEVICE, Link);
      UsbIo = UsbMass->UsbIo;
      UsbIf = USB_INTERFACE_FROM_USBIO (UsbIo);
      UsbDevice = UsbIf->Device;
      GetUsbPciAddress(UsbMass->UsbIo, &Pfa);
      PDTType = GetUsbDiskDeviceType (&UsbMass->BlockIo);

      //
      // Fix Installing OS will hang if there is a USB card reader without card.
      //
      if (UsbMass->BlockIo.Media->LastBlock == 0) {
        if (PDTType == USB_HDD) {
          Link = Link->ForwardLink;
          continue;
        }
      }

      //
      // Some Usb flash disk have MaxLun > 0, So we just report Lun =0 ,or it will hang when install XP.
      //
      if (PDTType == USB_HDD) {
        Status = CheckMassLunIllegle (&UsbMass->BlockIo);
        if (EFI_ERROR(Status)) {
          Link = Link->ForwardLink;
          continue;
        }
      }
      if (DeviceType == PDTType) {
        
        DescStr = mUmsInfo->UmsInfoEntry[Index].ProductDescAStr;
        CopyMem(DescStr, UsbMass->VendorID, sizeof(UsbMass->VendorID));
        DescStr += sizeof(UsbMass->VendorID);
        *(DescStr++) = ' ';
        CopyMem(DescStr, UsbMass->ProductID, sizeof(UsbMass->ProductID));
        DescStr += sizeof(UsbMass->ProductID);
        *DescStr = 0;
        DescStr = mUmsInfo->UmsInfoEntry[Index].ProductDescAStr;
        EliminateExtraSpaces(DescStr);

        mUmsInfo->UmsInfoEntry[Index].Bus          = Pfa->Bus;
        mUmsInfo->UmsInfoEntry[Index].Device       = Pfa->Device;
        mUmsInfo->UmsInfoEntry[Index].Function     = Pfa->Function;
        mUmsInfo->UmsInfoEntry[Index].PDT          = PDTType;

        mUmsInfo->UmsInfoEntry[Index].BlockSize    = UsbMass->BlockIo.Media->BlockSize;
        mUmsInfo->UmsInfoEntry[Index].LastBlock[0] = (UINT32)(UsbMass->BlockIo.Media->LastBlock & 0xffffffff);
        mUmsInfo->UmsInfoEntry[Index].LastBlock[1] = (UINT32)(RShiftU64(UsbMass->BlockIo.Media->LastBlock, 32) & 0xffffffff);

        mUmsInfo->UmsInfoEntry[Index].Handle[0]    = (UINT32)UsbMass & 0xffffffff;
        mUmsInfo->UmsInfoEntry[Index].Handle[1]    = (UINT32)(RShiftU64((UINT64)UsbMass, 32) & 0xffffffff);

        //
        //Format: [15:8]=Add ,[7:0]=Port.
        //
        mUmsInfo->UmsInfoEntry[Index].UsbDevParentAddr = UsbDevice->ParentAddr;
        mUmsInfo->UmsInfoEntry[Index].UsbDevParentPort = UsbDevice->ParentPort;

        Index++;
      }
      Link = Link->ForwardLink;
    }
  }
  mUmsInfo->NumOfUms = Index;
}

UINT32
Csm16ReadUsbBootDevice (
    IN UINT64         Handle,
    IN EFI_LBA        Lba,
    IN UINT32         BufferSize,
    IN UINT32         BufferAddress
)
{
    USB_MASS_DEVICE           *UsbMass;
    EFI_STATUS                Status;
    EFI_BLOCK_IO_PROTOCOL     *BlockIo;
    VOID                      *Buffer = (VOID *)BufferAddress;

    UsbMass = (USB_MASS_DEVICE *)Handle;

    if (UsbMass->Signature!=USB_MASS_SIGNATURE) {
        return  EFI_DEVICE_ERROR;
    }

    BlockIo = &UsbMass->BlockIo;

    Status = UsbMassReadBlocks(BlockIo, BlockIo->Media->MediaId, Lba, BufferSize, Buffer);

    return (UINT32)Status;
}

UINT32
Csm16WriteUsbBootDevice (
    IN UINT64         Handle,
    IN EFI_LBA        Lba,
    IN UINT32         BufferSize,
    IN UINT32         BufferAddress
)
{
    USB_MASS_DEVICE           *UsbMass;
    EFI_STATUS                Status;
    EFI_BLOCK_IO_PROTOCOL     *BlockIo;
    VOID                      *Buffer = (VOID *)BufferAddress;

    UsbMass = (USB_MASS_DEVICE *)Handle;
    if (UsbMass->Signature!=USB_MASS_SIGNATURE) {
        return  EFI_DEVICE_ERROR;
    }
    BlockIo = &UsbMass->BlockIo;

    Status = UsbMassWriteBlocks(BlockIo, BlockIo->Media->MediaId, Lba, BufferSize, Buffer);

    return (UINT32)Status;
}

UINT32
Csm16VerifyUsbBootDevice (
    IN UINT64         Handle,
    IN EFI_LBA        Lba,
    IN UINT32        VerifySize
)
{
    USB_MASS_DEVICE           *UsbMass;
    EFI_STATUS                Status;
    EFI_BLOCK_IO_PROTOCOL     *BlockIo;

    UsbMass = (USB_MASS_DEVICE *)Handle;
    if (UsbMass->Signature!=USB_MASS_SIGNATURE) {
        return  EFI_DEVICE_ERROR;
    }
    BlockIo = &UsbMass->BlockIo;
    Status = UsbMassVerifyBlocks(BlockIo, BlockIo->Media->MediaId, Lba, VerifySize);

    return (UINT32)Status;
}
