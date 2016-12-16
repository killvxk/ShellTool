/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file

  Memory Operation Functions for x64 Architecture.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  ArchSpecific.c

**/

#include "Cpu.h"
#include "MemoryAttribute.h"

extern EFI_CPU_INTERRUPT_HANDLER   mExternalVectorTable[];

INTERRUPT_GATE_DESCRIPTOR   *mIdtTable = NULL;
INTERRUPT_GATE_DESCRIPTOR   *mOrigIdtEntry = NULL;
UINT16                      mOrigIdtEntryCount = 0;

UINT64   mValidMtrrAddressMask;
UINT64   mValidMtrrBitsMask;

/**
  Creates an IDT and a new GDT in RAM.

  This function creates an IDT and a new GDT in RAM.

**/
VOID
InitializeDescriptorTables (
  VOID
  )
{
  UINT16                         CodeSegment;
  INTERRUPT_GATE_DESCRIPTOR      *IdtEntry;
  UINT32                         Index;
  INTERRUPT_GATE_DESCRIPTOR      *IdtTable;
  IA32_DESCRIPTOR                LidtPseudoDescriptor;
  IA32_DESCRIPTOR                Idtr;

  //
  // Load Global Descriptor Table and update segment selectors
  //
  InitializeGdt ();

  //
  // Create Interrupt Descriptor Table
  //
  IdtTable = AllocateZeroPool (sizeof (INTERRUPT_GATE_DESCRIPTOR) * INTERRUPT_VECTOR_NUMBER);
  ASSERT (IdtTable != NULL);
  LidtPseudoDescriptor.Limit = (UINT16) (sizeof (INTERRUPT_GATE_DESCRIPTOR) * INTERRUPT_VECTOR_NUMBER - 1);
  LidtPseudoDescriptor.Base  = (UINTN) IdtTable;

  InitializeExternalVectorTablePtr (mExternalVectorTable);
  //
  // Use current CS as the segment selector of interrupt gate in IDT
  //
  CodeSegment = AsmReadCs ();

  //
  // Get original IDT address and size.
  //
  AsmReadIdtr ((IA32_DESCRIPTOR *) &Idtr);
  mOrigIdtEntryCount = (UINT16) ((Idtr.Limit + 1) / sizeof (INTERRUPT_GATE_DESCRIPTOR));
  ASSERT (mOrigIdtEntryCount <= INTERRUPT_VECTOR_NUMBER);
  mOrigIdtEntry = AllocateCopyPool (Idtr.Limit + 1, (VOID *) Idtr.Base);
  ASSERT (mOrigIdtEntry != NULL);

  //
  // Copy original IDT entry.
  //
  IdtEntry = IdtTable;
  CopyMem (IdtEntry, (VOID *) Idtr.Base, Idtr.Limit + 1);

  //
  // Save IDT table pointer
  //
  mIdtTable = IdtTable;

  for (Index = 0; Index < INTERRUPT_VECTOR_NUMBER; Index ++) {
    //
    // Update all IDT entries to use current CS value.
    //
    IdtEntry[Index].SegmentSelector = CodeSegment;
    if (Index < mOrigIdtEntryCount) {
      //
      // Skip original IDT entry.
      //
      continue;
    }
    //
    // Set the address of interrupt handler to the rest IDT entry.
    //
    SetInterruptDescriptorTableHandlerAddress (Index);
  }

  //
  // Load Interrupt Descriptor Table
  //
  AsmWriteIdtr ((IA32_DESCRIPTOR *) &LidtPseudoDescriptor);

  //
  // Initialize Exception Handlers
  //
  InitializeException (mOrigIdtEntryCount);

  return;
}

/**
  Set Interrupt Descriptor Table Handler Address.

  @param Index        The Index of the interrupt descriptor table handle.

**/
VOID
SetInterruptDescriptorTableHandlerAddress (
  IN UINTN       Index
  )
{
  UINTN          ExceptionHandle;

  //
  // Get the address of handler for entry
  //
  ExceptionHandle = ((UINTN) AsmIdtVector00) + (8 * Index);

  mIdtTable[Index].Offset15To0  = (UINT16)ExceptionHandle;
  mIdtTable[Index].Attributes   = INTERRUPT_GATE_ATTRIBUTE;
  mIdtTable[Index].Offset31To16 = (UINT16)(ExceptionHandle >> 16);
  mIdtTable[Index].Offset63To32 = (UINT32)(ExceptionHandle >> 32);
}

/**
  Restore original Interrupt Descriptor Table Handler Address.

  @param Index        The Index of the interrupt descriptor table handle.

**/
VOID
RestoreInterruptDescriptorTableHandlerAddress (
  IN UINTN       Index
  )
{
  if (Index < mOrigIdtEntryCount) {
    mIdtTable[Index].Offset15To0  = mOrigIdtEntry[Index].Offset15To0;
    mIdtTable[Index].Offset31To16 = mOrigIdtEntry[Index].Offset31To16;
    mIdtTable[Index].Offset63To32 = mOrigIdtEntry[Index].Offset63To32;
  }
}
