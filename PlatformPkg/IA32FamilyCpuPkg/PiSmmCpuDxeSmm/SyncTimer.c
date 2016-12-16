/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file

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

UINT64   mTimeoutTicker = 0;
//
//  Number of counts in a roll-over cycle of the performance counter.
//
UINT64   mCycle = 0;
//
// Flag to indicate the performance counter is count-up or count-down.
//
BOOLEAN  mCountDown;

/**
  Initialize Timer for Smm AP Sync.
  
**/
VOID
InitializeSmmTimer (
  VOID
  )
{
  UINT64  TimerFrequency;
  UINT64  Start;
  UINT64  End;

  TimerFrequency = GetPerformanceCounterProperties (&Start, &End);
  mTimeoutTicker = DivU64x32 (
                     MultU64x64(TimerFrequency, PcdGet64 (PcdCpuSmmApSyncTimeout)),
                     1000 * 1000
                     ); 
  if (End < Start) {
    mCountDown = TRUE;
    mCycle = Start - End;
  } else {
    mCountDown = FALSE;
    mCycle = End - Start;
  }
}

/**
  Start Timer for Smm AP Sync.
  
**/
UINT64
EFIAPI
StartSyncTimer (
  VOID
  )
{
  return GetPerformanceCounter ();
}


/**
  Check if the Smm AP Sync timer is timeout.
  
  @param Timer  The start timer from the begin.

**/
BOOLEAN
EFIAPI
IsSyncTimerTimeout (
  IN      UINT64                    Timer
  )
{
  UINT64  CurrentTimer;
  UINT64  Delta;

  CurrentTimer = GetPerformanceCounter ();

  if (mCountDown) {
    //
    // The performance counter counts down.  Check for roll over condition.
    //
    if (CurrentTimer < Timer) {
      Delta = Timer - CurrentTimer;
    } else {
      //
      // Handle one roll-over. 
      //
      Delta = mCycle - (CurrentTimer - Timer);
    }
  } else {
    //
    // The performance counter counts up.  Check for roll over condition.
    //
    if (CurrentTimer > Timer) {
      Delta = CurrentTimer - Timer;
    } else {
      //
      // Handle one roll-over. 
      //
      Delta = mCycle - (Timer - CurrentTimer);
    }
  }
 
  return (BOOLEAN) (Delta >= mTimeoutTicker);
}
