/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file
  The CPU specific programming for PiSmmCpuDxeSmm module, such as SMRR, EMRR, IED.
  Currently below CPUs are supported.

  0x00000F00 // Pentium4

  0x000006F0 // Conroe
  0x00010660 // Conroe-L
  0x00010670 // Wolfdale
  0x000106D0 // Dunnington
  0x000106C0 // Silverthorne
  0x00020660 // Tunnel Creek

  0x000106A0 // Nehalem
  0x000106E0 // Lynnfield
  0x000106F0 // Havendale
  0x000206E0 // Nehalem-EX
  0x00020650 // Clarkdale
  0x000206C0 // Westmere
  0x000206F0 // Westmere-EX
  0x000206A0 // SandyBridge
  0x000206D0 // SandyBridge-EP
  0x00030660 // Cedar Trial
  0x000306A0 // Ivybridge Client
  0x000306E0 // Ivybridge Server

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

**/

#include <Base.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PciLib.h>
#include <Library/LocalApicLib.h>
#include "PiSmmCpuDxeSmm.h"


extern volatile BOOLEAN  mSmrrEnabled[];

VOID
DisableSmrr (
  VOID
  )
{
}


VOID
ReenableSmrr (
  VOID
  )
{
}

VOID
ACpuInitSmrr (
  IN UINT32                SmrrBase,
  IN UINT32                SmrrSize,
  IN UINT8                 SmrrType
  )
{
  AsmWriteMsr64(EFI_MSR_SMRR_PHYS_BASE, SmrrBase | SmrrType);
  AsmWriteMsr64(EFI_MSR_SMRR_PHYS_MASK, (~(SmrrSize - 1) & EFI_MSR_SMRR_MASK)); // Valid bit will be set in ConfigSmrr() at first SMI
}




BOOLEAN
NeedConfigureMtrrs (
  VOID
  )
{
  return gFeatureContext.NeedUpdateMtrrs;
}

/**
  Processor specific hook point at each SMM entry.

  @param  CpuIndex    The index of the cpu which need to check.

**/

STATIC volatile UINT32 gIoCf8Value;
STATIC volatile UINT8  gIo70Value;
STATIC volatile UINT8  gIo72Value;
STATIC volatile UINT8  gIo74Value;

VOID
SmmRendezvousEntry (
  IN UINTN  CpuIndex
  )
{
  if(CpuIndex == mSmmUcData->BspIndex){
    gIoCf8Value = IoRead32(0xcf8);
    gIo70Value  = IoRead8(0x70);
    gIo72Value  = IoRead8(0x72);
    gIo74Value  = IoRead8(0x74);		
  }

  if (gFeatureContext.SmrrEnabled && !mSmrrEnabled[CpuIndex]) {
    AsmWriteMsr64(EFI_MSR_SMRR_PHYS_MASK, 
		         AsmReadMsr64(EFI_MSR_SMRR_PHYS_MASK) | EFI_MSR_SMRR_PHYS_MASK_VALID);
    mSmrrEnabled[CpuIndex] = TRUE;
    if(CpuIndex == mSmmUcData->BspIndex){
      DEBUG((EFI_D_INFO, "EnSmrr\n"));
    }  
  }  
}

/**
  Processor specific hook point at each SMM entry.

  @param  CpuIndex    The index of the cpu which need to check.
**/
VOID
SmmRendezvousExit (
  IN UINTN  CpuIndex
  )
{
  if(CpuIndex == mSmmUcData->BspIndex){
    IoWrite32(0xcf8, gIoCf8Value);
    IoWrite8(0x70, gIo70Value);
    IoWrite8(0x72, gIo72Value);
    IoWrite8(0x74, gIo74Value);	
  }
}


VOID
InitializeSmmMtrrManager (
  VOID
  )
{
// default
  gFeatureContext.NeedUpdateMtrrs = TRUE;
  gFeatureContext.SmrrEnabled     = FALSE;

  if(gFeatureContext.SmrrType != 0xFF){
    if(AsmReadMsr64(EFI_MSR_IA32_MTRR_CAP) & IA32_MTRR_SMRR_SUPPORT_BIT){
      gFeatureContext.SmrrEnabled     = TRUE;
      gFeatureContext.NeedUpdateMtrrs = FALSE;
    }  
  }
  
}



/**
  Initialize SMRR register in SMM Relocate.

  @param  SmrrBase           The base address SMRR.
  @param  SmrrSize           The size of SMRR.
  @param  IsBsp              If this processor treated as BSP.
**/
VOID
SmmInitFeatures (
  IN UINT32  SmrrBase,
  IN UINT32  SmrrSize,
  IN UINTN   CpuIndex
  )
{
  if(gFeatureContext.SmrrEnabled){
    ACpuInitSmrr(SmrrBase, SmrrSize, gFeatureContext.SmrrType);
    mSmrrEnabled[CpuIndex] = FALSE;
  }
}



