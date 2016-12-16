//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file
  The CPU specific programming for PiSmmCpuDxeSmm module.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

**/

#ifndef __SMM_FEATURES_H__
#define __SMM_FEATURES_H__


#define EFI_CPUID_VERSION_INFO                 0x1

#define EFI_MSR_IA32_MTRR_CAP                  0xFE
#define  IA32_MTRR_SMRR_SUPPORT_BIT            BIT11

#define  EFI_MSR_SMRR_PHYS_MASK_VALID          BIT11
#define  EFI_MSR_SMRR_MASK                     0xFFFFF000

#define EFI_MSR_SMRR_PHYS_BASE         0x1F2
#define EFI_MSR_SMRR_PHYS_MASK         0x1F3

#define CACHE_WRITE_THROUGH                    4
#define CACHE_WRITE_BACK                       6
#define SMM_DEFAULT_SMBASE                     0x30000




typedef struct {
  BOOLEAN     SmrrEnabled;
  BOOLEAN     NeedUpdateMtrrs;
  UINT8       SmrrType;
} CPU_SMM_FEATURE_CONTEXT;


/**
  Disable SMRR register when SmmInit set SMM MTRRs.
**/
VOID
DisableSmrr (
  VOID
  );

/**
  Enable SMRR register when SmmInit restore non SMM MTRRs.
**/
VOID
ReenableSmrr (
  VOID
  );

/**
  Return if it is needed to configure MTRR to set TSEG cacheability.

  @retval  TRUE  - we need configure MTRR
  @retval  FALSE - we do not need configure MTRR
**/
BOOLEAN
NeedConfigureMtrrs (
  VOID
  );

/**
  Processor specific hook point at each SMM entry.

  @param  CpuIndex    The index of the cpu which need to check.
**/
VOID
SmmRendezvousEntry (
  IN UINTN  CpuIndex
  );

/**
  Processor specific hook point at each SMM exit.

  @param  CpuIndex    The index of the cpu which need to check.
**/
VOID
SmmRendezvousExit (
  IN UINTN  CpuIndex
  );

/**
  Initialize SMRR context in SMM Init.
**/
VOID
InitializeSmmMtrrManager (
  VOID
  );

/**
  Initialize SMRR/EMRR/IED register in SMM Relocate.

  @param  SmrrBase           The base address SMRR.
  @param  SmrrSize           The size of SMRR.
  @param  IsBsp              If this processor treated as BSP.
**/
VOID
SmmInitFeatures (
  IN UINT32  SmrrBase,
  IN UINT32  SmrrSize,
  IN UINTN   CpuIndex
  );

#endif



