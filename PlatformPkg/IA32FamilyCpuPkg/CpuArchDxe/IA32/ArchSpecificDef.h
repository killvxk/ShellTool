//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  ProcessorDef.h

Abstract:

  Definition for IA32 processor

**/

#ifndef _PROCESSOR_DEF_H_
#define _PROCESSOR_DEF_H_

#include <Protocol/Cpu.h>

#pragma pack(1)

typedef struct {
  UINT16  OffsetLow;
  UINT16  SegmentSelector;
  UINT16  Attributes;
  UINT16  OffsetHigh;
} INTERRUPT_GATE_DESCRIPTOR;

#pragma pack()

typedef struct {
  VOID  *Start;
  UINTN Size;
  UINTN FixOffset;
} INTERRUPT_HANDLER_TEMPLATE_MAP;

/**
  Return address map of interrupt handler template so that C code can generate
  interrupt handlers, and dynamically do address fix.

  @param AddressMap  Pointer to a buffer where the address map is returned.
**/
VOID
EFIAPI
GetTemplateAddressMap (
  OUT INTERRUPT_HANDLER_TEMPLATE_MAP *AddressMap
  );

/**
  Creates an IDT table starting at IdtTablPtr. It has IdtLimit/8 entries.
  Table is initialized to intxx where xx is from 00 to number of entries or
  100h, whichever is smaller. After table has been initialized the LIDT
  instruction is invoked.
 
  TableStart is the pointer to the callback table and is not used by 
  InitializedIdt but by commonEntry. CommonEntry handles all interrupts,
  does the context save and calls the callback entry, if non-NULL.
  It is the responsibility of the callback routine to do hardware EOIs.

  @param TableStart     Pointer to interrupt callback table.
  @param IdtTablePtr    Pointer to IDT table.
  @param IdtTableLimit  IDT Table limit (number of interrupt entries * 8).
**/
VOID
EFIAPI
InitializeIdt (
  IN EFI_CPU_INTERRUPT_HANDLER      *TableStart,
  IN UINTN                          *IdtTablePtr,
  IN UINT16                         IdtTableLimit
  );

#endif
