
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/SocketLga775Lib.h>
#include <Library/LocalApicLib.h>
#include <PlatformDefinition.h>
#include <SmmFeatures.h>

UINTN   mSmmRelocationOriginalAddress;
BOOLEAN *mRebasedFlag;

/**
  Seamphore operation for all processor relocate SMMBase.
**/
VOID
EFIAPI
SmmRelocationSemaphoreComplete (
  VOID
  );

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
          
