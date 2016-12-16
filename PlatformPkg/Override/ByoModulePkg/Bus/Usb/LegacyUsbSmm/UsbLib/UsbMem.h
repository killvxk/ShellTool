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
  UsbMem.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_USB_MEM_H_
#define _EFI_USB_MEM_H_

#include <PiSmm.h>
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/pci22.h>

#define MAX_USB_HOST_CONTROLLERS       10
#define MAX_USB_FRAME_LIST_LENGTH      4096
#define MAX_USB_FRAME_BUFFER_LENGTH    (MAX_USB_HOST_CONTROLLERS * MAX_USB_FRAME_LIST_LENGTH)

#define USB_HC_BIT(a)                  ((UINTN)(1 << (a)))

#define USB_HC_BIT_IS_SET(Data, Bit)   \
          ((BOOLEAN)(((Data) & USB_HC_BIT(Bit)) == USB_HC_BIT(Bit)))

#define USB_HC_HIGH_32BIT(Addr64)    \
          ((UINT32)(RShiftU64((UINTN)(Addr64), 32) & 0XFFFFFFFF))

typedef struct _USB_MEM_BLOCK     USB_MEM_BLOCK;

typedef struct _USB_MEM_BLOCK {
    UINT8                   *Bits;    // Bit array to record which unit is allocated
    UINTN                   BitsLen;
    UINT8                   *Buf;
    UINT8                   *BufHost;
    UINTN                   BufLen;   // Memory size in bytes
    VOID                    *Mapping;
    USB_MEM_BLOCK           *Next;
} USB_MEM_BLOCK;

//
// USBHC_MEM_POOL is used to manage the memory used by USB
// host controller. EHCI requires the control memory and transfer
// data to be on the same 4G memory.
//
typedef struct _USB_MEM_POOL {
    BOOLEAN                 Check4G;
    UINT32                  Which4G;
    USB_MEM_BLOCK           *Head;
} USB_MEM_POOL;

enum {
    USB_MEM_UNIT           = 64,     // Memory allocation unit, must be 2^n, n>4

    USB_MEM_UNIT_MASK      = USB_MEM_UNIT - 1,
    USB_MEM_DEFAULT_PAGES  = 64,
    XHC_RESERVED_PAGES     = 0x80,
};

#define USB_MEM_ROUND(Len)  (((Len) + USB_MEM_UNIT_MASK) & (~USB_MEM_UNIT_MASK))

//
// Advance the byte and bit to the next bit, adjust byte accordingly.
//
#define NEXT_BIT(Byte, Bit)   \
          do {                \
            (Bit)++;          \
            if ((Bit) > 7) {  \
              (Byte)++;       \
              (Bit) = 0;      \
            }                 \
          } while (0)

EFI_STATUS
UsbInitMemryManger(
);
/*++

Routine Description:

  Initialize the memory management pool for the host controller

Arguments:

  Pool    - The USB memory pool to initialize
  PciIo   - The PciIo that can be used to access the host controller
  Check4G - Whether the host controller requires allocated memory
            from one 4G address space.
  Which4G - The 4G memory area each memory allocated should be from

Returns:

  EFI_SUCCESS         : The memory pool is initialized
  EFI_OUT_OF_RESOURCE : Fail to init the memory pool

--*/
;

EFI_STATUS
UsbCleanupMemryManger (
)
/*++

Routine Description:

  Release the memory management pool

Arguments:

  Pool   - The USB memory pool to free

Returns:

  EFI_SUCCESS      : The memory pool is freed
  EFI_DEVICE_ERROR : Failed to free the memory pool

--*/
;

VOID *
UsbAllocatePool (
    IN  UINTN               Size
)
/*++

Routine Description:

  Allocate some memory from the host controller's memory pool
  which can be used to communicate with host controller.

Arguments:

  Pool      - The host controller's memory pool
  Size      - Size of the memory to allocate

Returns:

  The allocated memory or NULL

--*/
;

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
;

VOID *
UsbAllocatePage (
    IN  UINTN               Size
)
/*++

Routine Description:

  Allocate some memory from the host controller's memory pool
  which can be used to communicate with host controller.

Arguments:

  Pool      - The host controller's memory pool
  Size      - Size of the memory to allocate

Returns:

  The allocated memory or NULL

--*/
;

VOID
UsbFreePage (
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
;
#endif
