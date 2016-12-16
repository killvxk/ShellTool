/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file
SMM MP service inplementation

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#include "PiSmmCpuDxeSmm.h"

//
// Slots for all MTRR( FIXED MTRR + VARIABLE MTRR + MTRR_LIB_IA32_MTRR_DEF_TYPE)
//
UINT64                                      gSmiMtrrs[MTRR_NUMBER_OF_FIXED_MTRR + 2 * MTRR_NUMBER_OF_VARIABLE_MTRR + 1];
UINT64                                      gPhyMask;
SMM_DISPATCHER_UC_DATA                      *mSmmUcData = NULL;



BOOLEAN IsSmiSourceActive();

VOID
EFIAPI
SetMtrrForSmram (
OUT MTRR_SETTINGS                *MtrrSetting
);

/**
  Performs an atomic compare exchange operation to get semaphore.
  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param      Sem        IN:  32-bit unsigned integer
                         OUT: original integer - 1
  @return     Orignal integer - 1

**/
UINT32
WaitForSemaphore (
  IN OUT  volatile UINT32           *Sem
  )
{
  UINT32                            Value;

  do {
    Value = *Sem;
  } while (Value == 0 ||
           InterlockedCompareExchange32 (
             (UINT32*)Sem,
             Value,
             Value - 1
             ) != Value);
  return Value - 1;
}


/**
  Performs an atomic compare exchange operation to release semaphore.
  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param      Sem        IN:  32-bit unsigned integer
                         OUT: original integer + 1
  @return     Orignal integer + 1

**/
UINT32
ReleaseSemaphore (
  IN OUT  volatile UINT32           *Sem
  )
{
  UINT32                            Value;

  do {
    Value = *Sem;
  } while (Value + 1 != 0 &&
           InterlockedCompareExchange32 (
             (UINT32*)Sem,
             Value,
             Value + 1
             ) != Value);
  return Value + 1;
}

/**
  Performs an atomic compare exchange operation to lock semaphore.
  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param      Sem        IN:  32-bit unsigned integer
                         OUT: -1
  @return     Orignal integer

**/
UINT32
LockdownSemaphore (
  IN OUT  volatile UINT32           *Sem
  )
{
  UINT32                            Value;

  do {
    Value = *Sem;
  } while (InterlockedCompareExchange32 (
             (UINT32*)Sem,
             Value, (UINT32)-1
             ) != Value);
  return Value;
}

/**
  Wait all APs to performs an atomic compare exchange operation to release semaphore.

  @param   NumberOfAPs      AP number

**/
VOID
WaitForAllAPs (
  IN      UINTN                     NumberOfAPs
  )
{
  UINTN                             BspIndex;

  BspIndex = gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu;
  while (NumberOfAPs-- > 0) {
    WaitForSemaphore (&mSmmUcData->CpuData[BspIndex].Run);
  }
}

/**
  Performs an atomic compare exchange operation to release semaphore
  for each AP.

**/
VOID
ReleaseAllAPs (
  VOID
  )
{
  UINTN                             Index;
  UINTN                             BspIndex;

  BspIndex = gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu;
  for (Index = gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; Index-- > 0;) {
    if (Index != BspIndex && mSmmUcData->CpuData[Index].Present) {
      ReleaseSemaphore (&mSmmUcData->CpuData[Index].Run);
    }
  }
}

/**
  Replace OS MTRR's with SMI MTRR's.

  @param    CpuIndex             Processor Index

**/
VOID
ReplaceOSMtrrs (
  IN      UINTN                     CpuIndex
  )
{
  PROCESSOR_SMM_DESCRIPTOR       *Psd;
  UINT64                         *SmiMtrrs;
  MTRR_SETTINGS                  *BiosMtrr;

  Psd = (PROCESSOR_SMM_DESCRIPTOR*)(gSmmCpuPrivate->SmBases[CpuIndex] + SMM_PSD_OFFSET);
  SmiMtrrs = (UINT64*)(UINTN)Psd->MtrrBaseMaskPtr;

  DisableSmrr ();

  //
  // Replace all MTRRs registers
  //
  BiosMtrr  = (MTRR_SETTINGS*)SmiMtrrs;
  MtrrSetAllMtrrs(BiosMtrr);
}

/**
  SMI handler for BSP.

  @param     CpuIndex         BSP processor Index

**/
VOID
BSPHandler (
  IN      UINTN                     CpuIndex
  )
{
  UINT64                            Timer;
  UINTN                             Index;
  MTRR_SETTINGS                     Mtrrs;
  UINTN                             ApCount;
  UINTN                             AsynCount;

  ASSERT (CpuIndex == mSmmUcData->BspIndex);

  //
  // Flag BSP's presence
  //
  mSmmUcData->InsideSmm = TRUE;

  //
  // Set running processor index
  //
  gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu = CpuIndex;

  //
  // Sync with APs 1st timeout
  //
  for (Timer = StartSyncTimer ();
       !IsSyncTimerTimeout (Timer) &&
       mSmmUcData->Counter < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
       ) {
    CpuPause ();
  }

  //
  // mSmmUcData->Counter < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus means we need 2nd round of timeout
  //
  if (mSmmUcData->Counter < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus) {
    //
    // Send SMI IPIs to bring outside processors in
    //
    for (Index = gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus ; Index-- > 0;) {
      if (!mSmmUcData->CpuData[Index].Present) {
        ASSERT (Index != CpuIndex);
        SendSmiIpi (gSmmCpuPrivate->ApicIds[Index]);
      }
    }

    //
    // Sync with APs 2nd timeout
    //
    for (Timer = StartSyncTimer ();
         !IsSyncTimerTimeout (Timer) &&
         mSmmUcData->Counter < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
         ) {
      CpuPause ();
    }
  }

  //
  // All CPUs are in sync and don't allow more CPU to run the code in SMM mode after this point
  //
  mSmmUcData->AllCpusInSync = TRUE;

  //
  // Lock the counter down and retrieve the number of APs
  //
  ApCount = LockdownSemaphore (&mSmmUcData->Counter) - 1;

  //
  // Wait for all APs to get ready for programming MTRRs
  //
  WaitForAllAPs (ApCount);

  if (NeedConfigureMtrrs()) {
    //
    // Signal all APs it's time for backup MTRRs
    //
    ReleaseAllAPs ();

    //
    // WaitForSemaphore() may wait for ever if an AP happens to enter SMM at
    // exactly this point. Please make sure PcdCpuSmmMaxSyncLoops has been set
    // to a large enough value to avoid this situation.
    // Note: For HT capable CPUs, threads within a core share the same set of MTRRs.
    // We do the backup first and then set MTRR to avoid race condition for threads
    // in the same core.
    //
    MtrrGetAllMtrrs(&Mtrrs);

    //
    // Wait for all APs to complete their MTRR saving
    //
    WaitForAllAPs (ApCount);

    //
    // Let all processors program SMM MTRRs together
    //
    ReleaseAllAPs ();

    //
    // WaitForSemaphore() may wait for ever if an AP happens to enter SMM at
    // exactly this point. Please make sure PcdCpuSmmMaxSyncLoops has been set
    // to a large enough value to avoid this situation.
    //
    ReplaceOSMtrrs (CpuIndex);

    //
    // Wait for all APs to complete their MTRR programming
    //
    WaitForAllAPs (ApCount);
  }

  //
  // The BUSY lock is initialized to Acquired state
  //
  InitializeSpinLock (&mSmmUcData->CpuData[CpuIndex].Busy);
  AcquireSpinLockOrFail (&mSmmUcData->CpuData[CpuIndex].Busy);

  //
  // Invoke SMM Foundation EntryPoint with the processor information context.
  //
  AsynCount = 0;
  while(AsynCount++ < 16){
    gSmmCpuPrivate->SmmCoreEntry(&gSmmCpuPrivate->SmmCoreEntryContext);
    ClearSmi();
    if(!IsSmiSourceActive()){
      break;
    }
  }
  ASSERT(AsynCount <= 16);	
  
  //
  // Notify all APs to exit
  //
  mSmmUcData->InsideSmm = FALSE;
  ReleaseAllAPs ();

  //
  // Wait for all APs to complete their pending tasks
  //
  WaitForAllAPs (ApCount);

  if (NeedConfigureMtrrs()) {
    //
    // Signal APs to restore MTRRs
    //
    ReleaseAllAPs ();

    //
    // Restore OS MTRRs
    //
    ReenableSmrr ();
    MtrrSetAllMtrrs(&Mtrrs);

    //
    // Wait for all APs to complete MTRR programming
    //
    WaitForAllAPs (ApCount);
  }

  //
  // Reset BspIndex to -1, meaning BSP has not been elected.
  //
  if (FeaturePcdGet (PcdCpuSmmEnableBspElection)) {
    mSmmUcData->BspIndex = (UINT32)-1;
  }

  mSmmUcData->Counter = 0;

  //
  // Notify all APs to exit SMM
  //
  mSmmUcData->AllCpusInSync = FALSE;
}

/**
  SMI handler for AP.

  @param     CpuIndex         AP processor Index

**/
VOID
APHandler (
  IN      UINTN                     CpuIndex
  )
{
  UINT64                            Timer;
  UINTN                             BspIndex;
  MTRR_SETTINGS                     Mtrrs;

  BspIndex = mSmmUcData->BspIndex;
  ASSERT (CpuIndex != BspIndex);

  //
  // Timeout BSP if BSP election is disabled
  //
  if (!FeaturePcdGet (PcdCpuSmmEnableBspElection)) {
    for (Timer = StartSyncTimer ();
         !IsSyncTimerTimeout (Timer) &&
         !mSmmUcData->InsideSmm;
         ) {
      CpuPause ();
    }

    if (!mSmmUcData->InsideSmm) {
      //
      // BSP timeout in the first round, send SMI IPI to bring it in
      //
      SendSmiIpi (gSmmCpuPrivate->ApicIds[BspIndex]);

      //
      // Now clock BSP for the 2nd time
      //
      for (Timer = StartSyncTimer ();
           !IsSyncTimerTimeout (Timer) &&
           !mSmmUcData->InsideSmm;
           ) {
        CpuPause ();
      }

      if (!mSmmUcData->InsideSmm) {
        //
        // Give up since BSP is unable to enter SMM
        // and signal the completion of this AP
        WaitForSemaphore (&mSmmUcData->Counter);
        return;
      }
    }
  }

  //
  // Notify BSP of arrival at this point
  //
  ReleaseSemaphore (&mSmmUcData->CpuData[BspIndex].Run);

  if (NeedConfigureMtrrs()) {
    //
    // Wait for the signal from BSP to backup MTRRs
    //
    WaitForSemaphore (&mSmmUcData->CpuData[CpuIndex].Run);

    //
    // Backup OS MTRRs
    //
    MtrrGetAllMtrrs(&Mtrrs);

    //
    // Signal BSP the completion of this AP
    //
    ReleaseSemaphore (&mSmmUcData->CpuData[BspIndex].Run);

    //
    // Wait for BSP's signal to program MTRRs
    //
    WaitForSemaphore (&mSmmUcData->CpuData[CpuIndex].Run);

    //
    // Replace OS MTRRs with SMI MTRRs
    //
    ReplaceOSMtrrs (CpuIndex);

    //
    // Signal BSP the completion of this AP
    //
    ReleaseSemaphore (&mSmmUcData->CpuData[BspIndex].Run);
  }

  //
  // The BUSY lock is initialized to Released state
  //
  InitializeSpinLock (&mSmmUcData->CpuData[CpuIndex].Busy);

  while (TRUE) {
    //
    // Wait for something to happen
    //
    WaitForSemaphore (&mSmmUcData->CpuData[CpuIndex].Run);

    //
    // Check if BSP wants to exit SMM
    //
    if (!mSmmUcData->InsideSmm) {
      break;
    }

    //
    // BUSY should be acquired by SmmStartupThisAp()
    //
    ASSERT (
      !AcquireSpinLockOrFail (&mSmmUcData->CpuData[CpuIndex].Busy)
      );

    //
    // Invoke the scheduled procedure
    //
    (*mSmmUcData->CpuData[CpuIndex].Procedure) (
      (VOID*)mSmmUcData->CpuData[CpuIndex].Parameter
      );

    //
    // Release BUSY
    //
    ReleaseSpinLock (&mSmmUcData->CpuData[CpuIndex].Busy);
  }

  //
  // Notify BSP the readiness of this AP to exit SMM
  //
  ReleaseSemaphore (&mSmmUcData->CpuData[mSmmUcData->BspIndex].Run);

  if (NeedConfigureMtrrs()) {
    //
    // Wait for the signal from BSP to program MTRRs
    //
    WaitForSemaphore (&mSmmUcData->CpuData[CpuIndex].Run);

    //
    // Restore OS MTRRs
    //
    ReenableSmrr ();
    MtrrSetAllMtrrs(&Mtrrs);

    //
    // Signal BSP the completion of this AP
    //
    ReleaseSemaphore (&mSmmUcData->CpuData[BspIndex].Run);
  }
}

/**
  Create 4G PageTable in SMRAM.

  @param          ExtraPages       Additional page numbers besides for 4G memory
  @return         PageTable Address

**/
UINT32
Gen4GPageTable (
  IN      UINTN                     ExtraPages
  )
{
  VOID    *PageTable;
  UINTN   Index;
  UINT64  *Pte;

  //
  // Allocate the page table
  //
  PageTable = AllocatePages (ExtraPages + 5);
  ASSERT (PageTable != NULL);

  PageTable = (VOID *)((UINTN)PageTable + EFI_PAGES_TO_SIZE (ExtraPages));
  Pte = (UINT64*)PageTable;

  //
  // Zero out all page table entries first
  //
  ZeroMem (Pte, EFI_PAGES_TO_SIZE (5));

  //
  // Set Page Directory Pointers
  //
  for (Index = 0; Index < 4; Index++) {
    Pte[Index] = (UINTN)PageTable + EFI_PAGE_SIZE * (Index + 1) + IA32_PG_P;
  }
  Pte += EFI_PAGE_SIZE / sizeof (*Pte);

  //
  // Fill in Page Directory Entries
  //
  for (Index = 0; Index < EFI_PAGE_SIZE * 4 / sizeof (*Pte); Index++) {
    Pte[Index] = (Index << 21) + IA32_PG_PS + IA32_PG_RW + IA32_PG_P;
  }

  return (UINT32)(UINTN)PageTable;
}

/**
  Set memory cache ability.

  @param    PageTable              PageTable Address
  @param    Address                Memory Address to change cache ability
  @param    Cacheability           Cache ability to set

**/
VOID
SetCacheability (
  IN      UINT64                    *PageTable,
  IN      UINTN                     Address,
  IN      UINT8                     Cacheability
  )
{
  UINTN   PTIndex;
  VOID    *NewPageTableAddress;
  UINT64  *NewPageTable;
  UINTN   Index;

  ASSERT ((Address & EFI_PAGE_MASK) == 0);

  if (sizeof (UINTN) == sizeof (UINT64)) {
    PTIndex = (UINTN)RShiftU64 (Address, 39) & 0x1ff;
    ASSERT (PageTable[PTIndex] & IA32_PG_P);
    PageTable = (UINT64*)(UINTN)(PageTable[PTIndex] & gPhyMask);
  }

  PTIndex = (UINTN)RShiftU64 (Address, 30) & 0x1ff;
  ASSERT (PageTable[PTIndex] & IA32_PG_P);
  PageTable = (UINT64*)(UINTN)(PageTable[PTIndex] & gPhyMask);

  //
  // A perfect implementation should check the original cacheability with the
  // one being set, and break a 2M page entry into pieces only when they
  // disagreed.
  //
  PTIndex = (UINTN)RShiftU64 (Address, 21) & 0x1ff;
  if ((PageTable[PTIndex] & IA32_PG_PS) != 0) {
    //
    // Allocate a page from SMRAM
    //
    NewPageTableAddress = AllocatePages (1);
    ASSERT (NewPageTableAddress != NULL);

    NewPageTable = (UINT64 *)NewPageTableAddress;

    for (Index = 0; Index < 0x200; Index++) {
      NewPageTable[Index] = PageTable[PTIndex];
      if ((NewPageTable[Index] & IA32_PG_PAT_2M) != 0) {
        NewPageTable[Index] &= ~IA32_PG_PAT_2M;
        NewPageTable[Index] |= IA32_PG_PAT_4K;
      }
      NewPageTable[Index] |= Index << EFI_PAGE_SHIFT;
    }

    PageTable[PTIndex] = ((UINTN)NewPageTableAddress & gPhyMask) | IA32_PG_P;
  }

  ASSERT (PageTable[PTIndex] & IA32_PG_P);
  PageTable = (UINT64*)(UINTN)(PageTable[PTIndex] & gPhyMask);

  PTIndex = (UINTN)RShiftU64 (Address, 12) & 0x1ff;
  ASSERT (PageTable[PTIndex] & IA32_PG_P);
  PageTable[PTIndex] &= ~(IA32_PG_PAT_4K | IA32_PG_CD | IA32_PG_WT);
  PageTable[PTIndex] |= Cacheability;
}


/**
  Schedule a procedure to run on the specified CPU.

  @param  Procedure                The address of the procedure to run
  @param  CpuIndex                 Target CPU Index
  @param  ProcArguments            The parameter to pass to the procedure

  @retval EFI_INVALID_PARAMETER    CpuNumber not valid
  @retval EFI_INVALID_PARAMETER    CpuNumber specifying BSP
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber did not enter SMM
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber is busy
  @retval EFI_SUCCESS              The procedure has been successfully scheduled

**/
EFI_STATUS
EFIAPI
SmmStartupThisAp (
  IN      EFI_AP_PROCEDURE          Procedure,
  IN      UINTN                     CpuIndex,
  IN OUT  VOID                      *ProcArguments OPTIONAL
  )
{
  if (CpuIndex >= gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus ||
      CpuIndex == gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu ||
      !mSmmUcData->CpuData[CpuIndex].Present ||
      !AcquireSpinLockOrFail (&mSmmUcData->CpuData[CpuIndex].Busy)) {
    return EFI_INVALID_PARAMETER;
  }

  mSmmUcData->CpuData[CpuIndex].Procedure = Procedure;
  mSmmUcData->CpuData[CpuIndex].Parameter = ProcArguments;
  ReleaseSemaphore (&mSmmUcData->CpuData[CpuIndex].Run);

  if (FeaturePcdGet (PcdCpuSmmBlockStartupThisAp)) {
    AcquireSpinLock (&mSmmUcData->CpuData[CpuIndex].Busy);
    ReleaseSpinLock (&mSmmUcData->CpuData[CpuIndex].Busy);
  }
  return EFI_SUCCESS;
}


/**
  C function for SMI entry, each processor comes here upon SMI trigger.

  @param    CpuIndex              Cpu Index

**/
VOID
EFIAPI
SmiRendezvous (
  IN      UINTN                     CpuIndex
  )
{
  SmmRendezvousEntry (CpuIndex);
  //
  // Signal presence of this processor
  //
  if (ReleaseSemaphore (&mSmmUcData->Counter) == 0) {
    //
    // BSP has already ended the synchronization, so QUIT!!!
    // Wait for BSP's signal to finish SMI
    //
    goto Exit;
  } else {
    //
    // Mark this processor's presence
    //
    mSmmUcData->CpuData[CpuIndex].Present = TRUE;
  }

  //
  // Enable exception handling
  //
  AsmWriteIdtr (&gcSmiIdtr);

  //
  // Enable XMM instructions & exceptions
  //
  AsmWriteCr4 (AsmReadCr4 () | 0x600);

  //
  // Elect BSP
  //
  if (FeaturePcdGet (PcdCpuSmmEnableBspElection)) {
    InterlockedCompareExchange32 (
      (UINT32*)&mSmmUcData->BspIndex,
      (UINT32)-1,
      (UINT32)CpuIndex
      );
  }

  //
  // "mSmmUcData->BspIndex == CpuIndex" means this is the BSP
  //
  if (mSmmUcData->BspIndex == CpuIndex) {
    //
    // Initialize Debug Agent to start source level debug in SMM code
    //
    InitializeDebugAgent (DEBUG_AGENT_INIT_ENTER_SMI, NULL, NULL);

    BSPHandler (CpuIndex);

    //
    // Stop source level debug in SMM code, the code below will not be
    // debugged.
    //
    InitializeDebugAgent (DEBUG_AGENT_INIT_EXIT_SMI, NULL, NULL);

  } else {
    APHandler (CpuIndex);
  }

  //
  // Reset states/semaphore for this processor
  //
  mSmmUcData->CpuData[CpuIndex].Present = FALSE;
  ASSERT (mSmmUcData->CpuData[CpuIndex].Run == 0);

Exit:
  //
  // Wait for BSP's signal to exit SMI
  //
  while (mSmmUcData->AllCpusInSync) {
    CpuPause ();
  }

  SmmRendezvousExit (CpuIndex);
}


/**
  Initialize un-cacheable data.

**/
VOID
EFIAPI
InitializeUcData (
  VOID
  )
{
  if (mSmmUcData != NULL) {
    ZeroMem (mSmmUcData, sizeof (*mSmmUcData));
    if (FeaturePcdGet (PcdCpuSmmEnableBspElection)) {
      //
      // Enable BSP election by setting BspIndex to -1
      //
      mSmmUcData->BspIndex = (UINT32)-1;
    }
  }
}

/**
  Initialize global data for MP synchronization.

  @param ImageHandle  File Image Handle.
  @param Stacks       Base address of SMI stack buffer for all processors.

**/
VOID
InitializeMpServiceData (
  IN EFI_HANDLE  ImageHandle,
  IN VOID        *Stacks
  )
{
  UINTN                     Index;
  MTRR_SETTINGS             *Mtrr;
  PROCESSOR_SMM_DESCRIPTOR  *Psd;
  UINTN                     GdtTssTableSize;
  UINT8                     *GdtTssTables;

  //
  // Initialize physical address mask
  // NOTE: Physical memory above virtual address limit is not supported !!!
  //
  AsmCpuid (0x80000008, (UINT32*)&Index, NULL, NULL, NULL);
  gPhyMask = LShiftU64 (1, (UINT8)Index) - 1;
  gPhyMask &= (1ull << 48) - EFI_PAGE_SIZE;
  gSmiCr3 = SmmInitPageTable ();
  gSmiStack = (UINTN)Stacks + PcdGet32 (PcdCpuSmmStackSize) - sizeof(UINTN);


// The Smi Handler of CPU[i] and PSD of CPU[i+x] are in the same SMM_CPU_INTERVAL,
// and they cannot overlap.
  ASSERT (gcSmiHandlerSize + 0x10000 - SMM_PSD_OFFSET < SMM_CPU_INTERVAL);
  ASSERT (SMM_HANDLER_OFFSET % SMM_CPU_INTERVAL == 0);

  GdtTssTables    = NULL;
  GdtTssTableSize = 0;
  //
  // For X64 SMM, we allocate separate GDT/TSS for each CPUs to avoid TSS load contention
  // on each SMI entry. For IA32 SMM, we just use template GDT for all CPUs directly.
  //
  if (EFI_IMAGE_MACHINE_TYPE_SUPPORTED(EFI_IMAGE_MACHINE_X64)) {
    GdtTssTableSize = (gcSmiGdtr.Limit + 1 + TSS_SIZE + 7) & ~7; // 8 bytes aligned
    GdtTssTables = (UINT8*)AllocatePages (EFI_SIZE_TO_PAGES (GdtTssTableSize * gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus));
    for (Index = 0; Index < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; Index++) {
      CopyMem (GdtTssTables + GdtTssTableSize * Index, (VOID*)(UINTN)gcSmiGdtr.Base, gcSmiGdtr.Limit + 1 + TSS_SIZE);
    }
  }

  for (Index = 0; Index < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; Index++) {
    *(UINTN*)gSmiStack = Index;

    CopyMem (
      (VOID*)(UINTN)(gSmmCpuPrivate->SmBases[Index] + SMM_HANDLER_OFFSET),
      (VOID*)gcSmiHandlerTemplate,
      gcSmiHandlerSize
      );

    Psd = (PROCESSOR_SMM_DESCRIPTOR*)(VOID*)(UINTN)(gSmmCpuPrivate->SmBases[Index] + SMM_PSD_OFFSET);
    CopyMem (Psd, &gcPsd, sizeof (gcPsd));
    //
    // For X64 SMM, set GDT to the copy allocated above.
    //
    if (EFI_IMAGE_MACHINE_TYPE_SUPPORTED(EFI_IMAGE_MACHINE_X64)) {
      Psd->SmmGdtPtr = (UINT64)(UINTN)(GdtTssTables + GdtTssTableSize * Index);
    }

    gSmiStack += PcdGet32 (PcdCpuSmmStackSize);
  }

  //
  // Initialize mSmmUcData
  //
  mSmmUcData = (SMM_DISPATCHER_UC_DATA*) AllocatePages (EFI_SIZE_TO_PAGES (sizeof (*mSmmUcData)));
  ASSERT (mSmmUcData != NULL);
  InitializeUcData ();

// mSmmUcData should resides in un-cached RAM
  SetCacheability ((UINT64*)(UINTN)gSmiCr3, (UINTN)mSmmUcData, IA32_PG_CD);

  //
  // Record current MTRR settings
  //
  ZeroMem(gSmiMtrrs, sizeof (gSmiMtrrs));
  Mtrr =  (MTRR_SETTINGS*)gSmiMtrrs;
  MtrrGetAllMtrrs (Mtrr);

}

/**

  Register the SMM Foundation entry point.

  @param          This              Pointer to EFI_SMM_CONFIGURATION_PROTOCOL instance
  @param          SmmEntryPoint     SMM Foundation EntryPoint

  @retval         EFI_SUCCESS       Successfully to register SMM foundation entry point

**/
EFI_STATUS
EFIAPI
RegisterSmmEntry (
  IN CONST EFI_SMM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                   SmmEntryPoint
  )
{
  //
  // Record SMM Foundation EntryPoint, later invoke it on SMI entry vector.
  //
  gSmmCpuPrivate->SmmCoreEntry = SmmEntryPoint;
  return EFI_SUCCESS;
}
