/*++
  This file contains an 'Intel Peripheral Driver' and is
  licensed for Intel CPUs and chipsets under the terms of your
  license agreement with Intel or your vendor.  This file may
  be modified by the user, subject to additional terms of the
  license agreement
--*/
/** @file
  Code for Data Collection phase.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  DataCollection.c

**/

#include "Cpu.h"

BOOLEAN    HtCapable  = FALSE;
BOOLEAN    CmpCapable = FALSE;

/**
  Collects Local APIC data of the processor.

  This function collects Local APIC base, verion, and APIC ID of the processor.

  @param  ProcessorNumber    Handle number of specified logical processor

**/
VOID
CollectApicData (
  UINTN    ProcessorNumber
  )
{
  CPU_MISC_DATA          *CpuMiscData;
  CpuMiscData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber].CpuMiscData;
  CpuMiscData->ApicBase    = PcdGet32 (PcdCpuLocalApicBaseAddress);
  CpuMiscData->ApicVersion = GetApicVersion () & 0xff;
  CpuMiscData->ApicID      = GetApicId ();
}



/**
  Collects physical location of the processor.

  This function collects physical location of the processor.

  @param  ProcessorNumber    Handle number of specified logical processor

**/
VOID
CollectProcessorLocation (
  UINTN    ProcessorNumber
  )
{
  CPU_COLLECTED_DATA   *CpuCollectedData;
  UINT32               DefaultApicID;
  UINT8                ThreadBitMaskWidth;
  UINT8                CoreBitMaskWidth;
  UINT32               BitMask;
  UINT32               MaxCoresInPackage;
  UINT32               MaxLogicalProcessorPerCore;
  UINT32               MaxLogicalProcessorInPackage;
  EFI_CPUID_REGISTER   CpuidRegisters;

  DefaultApicID    = GetInitialApicId ();
  CpuCollectedData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber];

  //
  // Calculate number of logical processors and cores in the package.
  //
  AsmCpuid (EFI_CPUID_VERSION_INFO, NULL, &CpuidRegisters.RegEbx, NULL, NULL);
  MaxLogicalProcessorInPackage = (UINT8) ((CpuidRegisters.RegEbx >> 16) & 0xff);
  AsmCpuid (EFI_CPUID_SIGNATURE, &CpuidRegisters.RegEax, NULL, NULL, NULL);

  if (CpuidRegisters.RegEax >= 4) {
    AsmCpuidEx (4, 0, &CpuidRegisters.RegEax, NULL, NULL, NULL);
    MaxCoresInPackage = (((CpuidRegisters.RegEax) >> 26) & 0x3f) + 1;
  }
  else {
    MaxCoresInPackage = 1;
  }
  MaxLogicalProcessorPerCore = MaxLogicalProcessorInPackage / MaxCoresInPackage;

  //
  // Check CMP and HT capabilities
  //
  if (ProcessorNumber == mCpuConfigConextBuffer.BspNumber) {
    if (MaxCoresInPackage > 1) {
      CmpCapable = TRUE;
    }
    if (MaxLogicalProcessorPerCore > 1) {
      HtCapable = TRUE;
    }
  }
  //
  // Calculate thread number
  //
  ThreadBitMaskWidth                         = (UINT8) (HighBitSet32 (MaxLogicalProcessorPerCore - 1) + 1);
  BitMask                                    = (1 << ThreadBitMaskWidth) - 1;
  CpuCollectedData->ProcessorLocation.Thread  = DefaultApicID & BitMask;

  //
  // Calculate core number
  //
  CoreBitMaskWidth                           = (UINT8) (HighBitSet32 (MaxCoresInPackage - 1) + 1);
  BitMask                                    = ((1 << ThreadBitMaskWidth) - 1) ^ ((1 << (ThreadBitMaskWidth + CoreBitMaskWidth)) - 1);
  CpuCollectedData->ProcessorLocation.Core    = (DefaultApicID & BitMask) >> ThreadBitMaskWidth;

  //
  // Calculate package number
  //
  BitMask                                    = ~((1 << (ThreadBitMaskWidth + CoreBitMaskWidth)) - 1);
  CpuCollectedData->ProcessorLocation.Package = (DefaultApicID & BitMask) >> (ThreadBitMaskWidth + CoreBitMaskWidth);
}




/**
  Get the location of specified processor.
  
  This function gets the location of specified processor, including
  package number, core number within package, thread number within core.

  @param  ProcessorNumber Handle number of specified logical processor.
  @param  PackageNumber   Pointer to the output package number.
  @param  CoreNumber      Pointer to the output core number.
  @param  ThreadNumber    Pointer to the output thread number.

**/
VOID
EFIAPI
GetProcessorLocation (
  IN    UINTN     ProcessorNumber,
  OUT   UINT32    *PackageNumber   OPTIONAL,
  OUT   UINT32    *CoreNumber      OPTIONAL,
  OUT   UINT32    *ThreadNumber    OPTIONAL
)
{
  CPU_COLLECTED_DATA  *CpuCollectedData;

  CpuCollectedData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber];

  if (PackageNumber != NULL) {
    *PackageNumber = CpuCollectedData->ProcessorLocation.Package;
  }
  if (CoreNumber != NULL) {
    *CoreNumber    = CpuCollectedData->ProcessorLocation.Core;
  }
  if (ThreadNumber != NULL) {
    *ThreadNumber  = CpuCollectedData->ProcessorLocation.Thread;
  }
}


/**
  Collects processor data for calling processor.

  This function collects processor data for calling processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
CollectBasicProcessorData (
  IN UINTN  ProcessorNumber
  )
{
  CollectApicData(ProcessorNumber);
  CollectProcessorLocation(ProcessorNumber);
}


