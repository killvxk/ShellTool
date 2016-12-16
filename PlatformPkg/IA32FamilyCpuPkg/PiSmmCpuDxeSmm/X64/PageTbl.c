/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file

Page Fault (#PF) handler for X64 processors

Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#include "PiSmmCpuDxeSmm.h"

#define PAGE_TABLE_PAGES            4
#define IA32_PG_PMNT                (1ull << 62)
LIST_ENTRY                          mPagePool = INITIALIZE_LIST_HEAD_VARIABLE (mPagePool);
SPIN_LOCK                           mPFLock;


/**
  Create PageTable for SMM use.

**/
UINT32
SmmInitPageTable (
  VOID
  )
{
  EFI_PHYSICAL_ADDRESS              Pages;
  UINT64                            *PTEntry;
  LIST_ENTRY                        *FreePage;
  UINTN                             Index;

  //
  // Initialize spin lock
  //
  InitializeSpinLock (&mPFLock);

  //
  // Generate PAE page table for the first 4GB memory space
  //
  Pages = Gen4GPageTable (PAGE_TABLE_PAGES + 1);

  //
  // Set IA32_PG_PMNT bit
  //
  PTEntry = (UINT64*)(UINTN)Pages;
  for (Index = 0; Index < 4; Index++) {
    PTEntry[Index] |= IA32_PG_PMNT;
  }

  //
  // Fill Page-Table-Level4 (PML4) entry
  //
  PTEntry = (UINT64*)(UINTN)(Pages - EFI_PAGES_TO_SIZE (PAGE_TABLE_PAGES + 1));
  *PTEntry = Pages + IA32_PG_P;
  ZeroMem (PTEntry + 1, EFI_PAGE_SIZE - sizeof (*PTEntry));

  //
  // Add remaining pages to page pool
  //
  FreePage = (LIST_ENTRY*)(PTEntry + EFI_PAGE_SIZE / sizeof (*PTEntry));
  while ((UINTN)FreePage < Pages) {
    InsertTailList (&mPagePool, FreePage);
    FreePage += EFI_PAGE_SIZE / sizeof (*FreePage);
  }

  //
  // Return the address of PML4 (to set CR3)
  //
  return (UINT32)(UINTN)PTEntry;
}


/**
  Reclaim free pages for PageFault handler.

**/
VOID
ReclaimPages (
  VOID
  )
{
  UINT64                            *PgTbl;
  UINT64                            *PgTbl2;
  UINTN                             Index, Index2;
  UINTN                             MinI, MinJ;
  UINT64                            MinAcc;
  UINT64                            Acc;

  MinAcc = (UINT64)-1;
  MinI = 0;
  MinJ = 0;
  PgTbl = (UINT64*)(UINTN)(AsmReadCr3 () & gPhyMask);
  for (Index = 0; Index < EFI_PAGE_SIZE / sizeof (*PgTbl); Index++) {
    if ((PgTbl[Index] & IA32_PG_P) == 0 || (PgTbl[Index] & IA32_PG_PMNT) != 0) {
      continue;
    }

    PgTbl2 = (UINT64*)(UINTN)(PgTbl[Index] & gPhyMask);
    for (Index2 = 0; Index2 < EFI_PAGE_SIZE / sizeof (*PgTbl2); Index2++) {
      if ((PgTbl2[Index2] & IA32_PG_P) == 0 || (PgTbl2[Index2] & IA32_PG_PMNT) != 0) {
        continue;
      }

      Acc = PgTbl2[Index2] & IA32_PG_AVL1;
      if ((PgTbl2[Index2] & IA32_PG_A) != 0) {
        PgTbl2[Index2] &= ~IA32_PG_A;
        Acc |= IA32_PG_AVL1 + 1;
      }
      if (Acc < MinAcc) {
        MinAcc = Acc;
        MinI = Index;
        MinJ = Index2;
      }
      PgTbl2[Index2] &= ~IA32_PG_AVL1;
      PgTbl2[Index2] |= RShiftU64 (Acc, 1) & IA32_PG_AVL1;
    }
  }

  ASSERT (MinAcc != (UINT64)-1);

  ASSERT (PgTbl[MinI] & IA32_PG_P);
  PgTbl2 = (UINT64*)(UINTN)(PgTbl[MinI] & gPhyMask);
  ASSERT (PgTbl2[MinJ] & IA32_PG_P);

  InsertTailList (&mPagePool, (LIST_ENTRY*)(UINTN)(PgTbl2[MinJ] & gPhyMask));
  PgTbl2[MinJ] = 0;

  Acc = BitFieldRead64 (PgTbl[MinI], 52, 60);
  if (Acc == 0) {
    InsertTailList (&mPagePool, (LIST_ENTRY*)(UINTN)(PgTbl[MinI] & gPhyMask));
    PgTbl[MinI] = 0;
  } else {
    PgTbl[MinI] = BitFieldWrite64 (PgTbl[MinI], 52, 60, Acc - 1);
  }
}

/**
  Allocate free Page for PageFault handler use.

**/
UINT64
AllocPage (
  VOID
  )
{
  UINT64                            RetVal;

  if (IsListEmpty (&mPagePool)) {
    ReclaimPages ();
  }

  RetVal = (UINT64)(UINTN)mPagePool.ForwardLink;
  RemoveEntryList (mPagePool.ForwardLink);
  ZeroMem ((VOID*)(UINTN)RetVal, EFI_PAGE_SIZE);
  return RetVal;
}

/**
  Page Fault handler for SMM use.

  @param   IntNumber    The number of the interrupt.
  @param   ErrorCode    The Error code.
**/
VOID
EFIAPI
SmiPFHandler (
  IN      UINTN                     IntNumber,
  IN      UINTN                     ErrorCode
  )
{
  UINT64                            *PageTable;
  UINT64                            PFAddress;
  UINTN                             StartBit;
  UINT64                            PTIndex;

  ASSERT (IntNumber == 0x0e);

  AcquireSpinLock (&mPFLock);

  PageTable = (UINT64*)(AsmReadCr3 () & gPhyMask);
  PFAddress = AsmReadCr2 ();

  for (StartBit = 39; StartBit > 21; StartBit -= 9) {
    PTIndex = BitFieldRead64 (PFAddress, StartBit, StartBit + 8);
    if ((PageTable[PTIndex] & IA32_PG_P) == 0) {
      PageTable[PTIndex] = AllocPage () | IA32_PG_RW | IA32_PG_P;
    } else {
      PageTable[PTIndex] =
        BitFieldWrite64 (
          PageTable[PTIndex], 52, 60,
          BitFieldRead64 (PageTable[PTIndex], 52, 60) + 1
          );
    }
    PageTable[PTIndex] |= IA32_PG_AVL1 | IA32_PG_A;
    PageTable = (UINT64*)(UINTN)(PageTable[PTIndex] & gPhyMask);
  }

  PTIndex = BitFieldRead64 (PFAddress, StartBit, StartBit + 8);
  PageTable[PTIndex] =
    (PFAddress & gPhyMask & ~((1ull << 21) - 1)) |
    IA32_PG_PS | IA32_PG_A | IA32_PG_RW | IA32_PG_P;

  ReleaseSpinLock (&mPFLock);
}
