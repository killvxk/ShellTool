/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file
  Semaphore mechanism to indicate to the BSP that an AP has exited SMM
  after SMBASE relocation.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

**/

#include "PiSmmCpuDxeSmm.h"

UINTN   mSmmRelocationOriginalAddress;
BOOLEAN *mRebasedFlag;

/**
  Hook return address of SMM Save State so that semaphore code
  can be executed immediately after AP exits SMM to indicate to
  the BSP that an AP has exited SMM after SMBASE relocation.

  @param RebasedFlag  Pointer to a flag. AP sets it to be TRUE after exit of SMM.
  @param CpuState     Pointer to the SMM Save State Map.
**/
VOID
SemaphoreHook (
  IN BOOLEAN                       *RebasedFlag,
  IN SOCKET_LGA_775_SMM_CPU_STATE  *CpuState
  )
{
  mRebasedFlag = RebasedFlag;

  //
  // The offset of EIP/RIP is different depending on the SMMRevId
  //
  if (CpuState->x86.SMMRevId < SOCKET_LGA_775_SMM_MIN_REV_ID_x64) {
    mSmmRelocationOriginalAddress = (UINTN) CpuState->x86._EIP;
    CpuState->x86._EIP            = (UINT32) (UINTN) &SmmRelocationSemaphoreComplete;
  } else {
    mSmmRelocationOriginalAddress = (UINTN) CpuState->x64._RIP;
    CpuState->x64._RIP            = (UINT64) (UINTN) &SmmRelocationSemaphoreComplete;
  }

  if (CpuState->x86.AutoHALTRestart & BIT0) {
    //
    // Clear the auto HALT restart flag so the RSM instruction returns 
    //   program control to the instruction following the HLT instruction,
    //   actually returns to SmmRelocationSemaphoreComplete
    //
    CpuState->x86.AutoHALTRestart &= ~BIT0;
  }
}
          
