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
  UsbMem.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/
#include <PiSmm.h>
#include "UsbMem.h"
#include <library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include "../UsbBus/UsbBus.h"


UINTN mUsbHcDebugLevel = DEBUG_INFO;
USB_MEM_POOL                  *mUsbMemPool = NULL;
EFI_PHYSICAL_ADDRESS          mUsbMemAddr = 0x0ffffffff;
extern UINT8                  mUsbHcDevCount;

extern EFI_STATUS
UsbGetHostControllerCount(
    OUT UINTN     *ControllerCount
);

STATIC
USB_MEM_BLOCK *
UsbAllocMemBlock (
    IN  UINTN               Pages
)
{
    USB_MEM_BLOCK         *Block;

    Block = AllocateZeroPool(sizeof (USB_MEM_BLOCK));
    if (Block == NULL)
        return NULL;

    //
    // each bit in the bit array represents USB_MEM_UNIT
    // bytes of memory in the memory block.
    //
    ASSERT (USB_MEM_UNIT * 8 <= EFI_PAGE_SIZE);

    Block->BufLen   = EFI_PAGES_TO_SIZE (Pages);
    Block->BitsLen  = Block->BufLen / (USB_MEM_UNIT * 8);
    Block->Bits = AllocateZeroPool(Block->BitsLen);
    if (Block->Bits == NULL) {
        FreePool (Block);
        return NULL;
    }

    Block->Buf = (UINT8 *)(UINTN)mUsbMemAddr;
    mUsbMemAddr += Pages * EFI_PAGE_SIZE;
    USB_DEBUG((EFI_D_INFO, "UsbHcAllocMemBlock: block %x created with buffer %x\n",
            Block, Block->Buf));

    return Block;
}

STATIC
VOID
UsbFreeMemBlock (
    IN USB_MEM_BLOCK      *Block
)
{
    ASSERT (Block != NULL);
    gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)Block->Buf, EFI_SIZE_TO_PAGES (Block->BufLen));
    FreePool (Block->Bits);
    FreePool (Block);
}

STATIC
VOID *
UsbAllocMemFromBlock (
    IN  USB_MEM_BLOCK      *Block,
    IN  UINTN               Units
)
{
    UINTN                   Byte;
    UINT8                   Bit;
    UINTN                   StartByte;
    UINT8                   StartBit;
    UINTN                   Available;
    UINTN                   Count;

    ASSERT ((Block != 0) && (Units != 0));

    StartByte  = 0;
    StartBit   = 0;
    Available  = 0;

    for (Byte = 0, Bit = 0; Byte < Block->BitsLen;) {
        //
        // If current bit is zero, the corresponding memory unit is
        // available, otherwise we need to restart our searching.
        // Available counts the consective number of zero bit.
        //
        if (!USB_HC_BIT_IS_SET (Block->Bits[Byte], Bit)) {
            Available++;

            if (Available >= Units) {
                break;
            }

            NEXT_BIT (Byte, Bit);

        } else {
            NEXT_BIT (Byte, Bit);

            Available  = 0;
            StartByte  = Byte;
            StartBit   = Bit;
        }
    }

    if (Available < Units) {
        USB_DEBUG((EFI_D_INFO, "UsbHcAllocMemBlock: USB memory exhausted!\n"));
        return NULL;
    }

    //
    // Mark the memory as allocated
    //
    Byte  = StartByte;
    Bit   = StartBit;

    for (Count = 0; Count < Units; Count++) {
        ASSERT (!USB_HC_BIT_IS_SET (Block->Bits[Byte], Bit));

        Block->Bits[Byte] |= USB_HC_BIT (Bit);
        NEXT_BIT (Byte, Bit);
    }

    return Block->Buf + (StartByte * 8 + StartBit) * USB_MEM_UNIT;
}

STATIC
VOID
UsbInsertMemBlockToPool (
    IN USB_MEM_BLOCK      *Head,
    IN USB_MEM_BLOCK      *Block
)
{
    ASSERT ((Head != NULL) && (Block != NULL));
    Block->Next = Head->Next;
    Head->Next  = Block;
}

STATIC
BOOLEAN
UsbIsMemBlockEmpty (
    IN USB_MEM_BLOCK     *Block
)
{
    UINTN                   Index;

    for (Index = 0; Index < Block->BitsLen; Index++) {
        if (Block->Bits[Index] != 0) {
            return FALSE;
        }
    }

    return TRUE;
}

STATIC
VOID
UsbUnlinkMemBlock (
    IN USB_MEM_BLOCK      *Head,
    IN USB_MEM_BLOCK      *BlockToUnlink
)
{
    USB_MEM_BLOCK         *Block;

    ASSERT ((Head != NULL) && (BlockToUnlink != NULL));

    for (Block = Head; Block != NULL; Block = Block->Next) {
        if (Block->Next == BlockToUnlink) {
            Block->Next         = BlockToUnlink->Next;
            BlockToUnlink->Next = NULL;
            break;
        }
    }
}

VOID *
UsbAllocatePage (
    IN  UINTN               Size
)
{
    USB_MEM_BLOCK           *Block;
    VOID                    *Mem;
    UINTN                   AllocSize;

    Mem       = NULL;
    Block     = mUsbMemPool->Head->Next;
    ASSERT (Block != NULL);
    AllocSize = Size * EFI_PAGE_SIZE;
    Mem = UsbAllocMemFromBlock (Block, AllocSize / USB_MEM_UNIT);
    if (Mem != NULL) {
        ZeroMem (Mem, AllocSize);
    }
    return Mem;
}

VOID
UsbFreePage (
    IN VOID                 *Mem,
    IN UINTN                Size
)
{
    USB_MEM_BLOCK           *Block;
    UINT8                   *ToFree;
    UINTN                   AllocSize;
    UINTN                   Byte;
    UINTN                   Bit;
    UINTN                   Count;

    Block     = mUsbMemPool->Head->Next;
    ASSERT (Block != NULL);
    AllocSize = Size * EFI_PAGE_SIZE;
    ToFree    = (UINT8 *) Mem;
    //
    // scan the memory block list for the memory block that
    // completely contains the memory to free.
    //
    if ((Block->Buf <= ToFree) && ((ToFree + AllocSize) <= (Block->Buf + Block->BufLen))) {
        //
        // compute the start byte and bit in the bit array
        //
        Byte  = ((ToFree - Block->Buf) / USB_MEM_UNIT) / 8;
        Bit   = ((ToFree - Block->Buf) / USB_MEM_UNIT) % 8;

        //
        // reset associated bits in bit arry
        //
        for (Count = 0; Count < (AllocSize / USB_MEM_UNIT); Count++) {
            ASSERT (USB_HC_BIT_IS_SET (Block->Bits[Byte], Bit));
            Block->Bits[Byte] ^= (UINT8) USB_HC_BIT (Bit);
            NEXT_BIT (Byte, Bit);
        }
    }
    return ;
}

VOID *
UsbAllocatePool (
    IN  UINTN               Size
)
{
    USB_MEM_BLOCK           *Block;
    VOID                    *Mem;
    UINTN                   AllocSize;

    Mem       = NULL;
    AllocSize = USB_MEM_ROUND (Size);
    Block     = mUsbMemPool->Head;
    ASSERT (Block != NULL);
    Mem = UsbAllocMemFromBlock (Block, AllocSize / USB_MEM_UNIT);
    if (Mem != NULL) {
        ZeroMem (Mem, Size);
    }
    return Mem;
}

VOID
UsbFreePool (
    IN VOID                 *Mem,
    IN UINTN                Size
)
/*++

Routine Description:

  Free the allocated memory back to the memory pool

Arguments:

  Pool    - The memory pool of the host controller
  Mem     - The memory to free
  Size    - The size of the memory to free

Returns:

  VOID

--*/
{
    USB_MEM_BLOCK           *Block;
    UINT8                   *ToFree;
    UINTN                   AllocSize;
    UINTN                   Byte;
    UINTN                   Bit;
    UINTN                   Count;

    Block     = mUsbMemPool->Head;
    ASSERT (Block != NULL);
    AllocSize = USB_MEM_ROUND (Size);
    ToFree    = (UINT8 *) Mem;
    //
    // scan the memory block list for the memory block that
    // completely contains the memory to free.
    //
    if ((Block->Buf <= ToFree) && ((ToFree + AllocSize) <= (Block->Buf + Block->BufLen))) {
        //
        // compute the start byte and bit in the bit array
        //
        Byte  = ((ToFree - Block->Buf) / USB_MEM_UNIT) / 8;
        Bit   = ((ToFree - Block->Buf) / USB_MEM_UNIT) % 8;

        //
        // reset associated bits in bit arry
        //
        for (Count = 0; Count < (AllocSize / USB_MEM_UNIT); Count++) {
            ASSERT (USB_HC_BIT_IS_SET (Block->Bits[Byte], Bit));
            Block->Bits[Byte] ^= (UINT8) USB_HC_BIT (Bit);
            NEXT_BIT (Byte, Bit);
        }
    }
    return ;
}

EFI_STATUS
UsbInitMemryManger(
)
{
    USB_MEM_BLOCK           *Block;
    EFI_STATUS              Status;
    UINTN                   Pages;

    if (mUsbHcDevCount == 0) {
        return EFI_DEVICE_ERROR;
    }

    Pages = mUsbHcDevCount + USB_MEM_DEFAULT_PAGES+XHC_RESERVED_PAGES;
    Status = gBS->AllocatePages (
                 AllocateMaxAddress,
                 EfiReservedMemoryType,
                 Pages,
                 &mUsbMemAddr
             );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    } else {
      DEBUG((EFI_D_ERROR, "UsbInitMemryManger.mUsbMemAddr:%lX,P:%X\n", mUsbMemAddr, Pages));
    }

    mUsbMemPool = AllocateZeroPool(sizeof(USB_MEM_POOL));
    if (mUsbMemPool == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FREE_2;
    }

    mUsbMemPool->Head    = UsbAllocMemBlock (USB_MEM_DEFAULT_PAGES);

    if (mUsbMemPool->Head == NULL) {
        goto FREE_2;
    }

    Block = UsbAllocMemBlock (mUsbHcDevCount+XHC_RESERVED_PAGES);
    if (Block == NULL) {
        UsbFreeMemBlock (mUsbMemPool->Head);
        Status = EFI_OUT_OF_RESOURCES;
        goto FREE_1;
    }

    UsbInsertMemBlockToPool(mUsbMemPool->Head, Block);
    return EFI_SUCCESS;

FREE_1:
    FreePool(mUsbMemPool);
    mUsbMemPool = NULL;
FREE_2:
    gBS->FreePages(mUsbMemAddr, Pages);
    return EFI_OUT_OF_RESOURCES;
}

EFI_STATUS
UsbCleanupMemryManger (
)
{
    USB_MEM_BLOCK *Block;

    ASSERT (mUsbMemPool != NULL);

    //
    // Unlink all the memory blocks from the pool, then free them.
    // UsbHcUnlinkMemBlock can't be used to unlink and free the
    // first block.
    //
    for (Block = mUsbMemPool->Head->Next; Block != NULL; Block = mUsbMemPool->Head->Next) {
        UsbUnlinkMemBlock (mUsbMemPool->Head, Block);
        UsbFreeMemBlock (Block);
    }

    UsbFreeMemBlock (mUsbMemPool->Head);
    FreePool (mUsbMemPool);
    return EFI_SUCCESS;
}

