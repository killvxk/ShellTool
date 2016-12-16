/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  CpuLocalApicTimerLib.c

Abstract:
  
Revision History:

Bug 3145:  Release initial verion bios for ChiefRiver/MahoBay platform base on
           Sugarbay Platform core drop.
TIME:       2011-11-25
$AUTHOR:    Liu Chunling
$REVIEWERS:
$SCOPE:     ChiefRiver/MahoBay Customer Refernce Board.
$TECHNICAL: 
  1. Change SnbClientX64Pkg to $(PLATFORM_PACKAGE) in INF file
     to fix failure to build other platform.
  2. Add the EDK debug libraries for IvyBridge Platform.
  3. Use UDK library IoLib in Xhci.c
  4. Add the Family ID and Model ID for IvyBridge processor.
$END--------------------------------------------------------------------------

**/
/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file
  Timer Library functions built upon local APIC on IA32/x64.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#include <Base.h>

#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SocketLga775Lib.h>
#include <Library/LocalApicLib.h>

//
// The following array is FSB frequencies defined in Pentinum 4 family
// and Xeon family CPUs, its value unit is HZ.
//
GLOBAL_REMOVE_IF_UNREFERENCED
CONST UINT32                          mPentinum4XeonFSBFrequencies[] = {
  100000000,
  133333333,
  200000000,
  166666667
};

//
// The following array is FSB frequencies defined in Core 2 Duo CPUs,
// , its value unit is HZ.
//
GLOBAL_REMOVE_IF_UNREFERENCED
CONST UINT32                          mCore2DuoFSBFrequencies[] = {
  266666667,
  133333333,
  200000000,
  166666667,
  333333333,
  100000000,
  400000000,
};


/**
  The function to get CPU intended FSB frequency.

  This function reads the type of CPU by CPUID and returns FSB frequecny,
  if CPU not supportted, then ASSERT().

  @retval CPU intended FSB frequency.

**/
UINT32
GetIntendFsbFrequency (
  VOID
  )
{
  UINT32              RegEax;
  UINT32              BasicFamilyId;
  UINT32              FamilyId;
  UINT32              ModelId;
  UINT32              ExtendedModelId;
  UINT32              FrequencyIndex;

  AsmCpuid (EFI_CPUID_VERSION_INFO, &RegEax, NULL, NULL, NULL);

  //
  // The Extended Family ID needs to be examined only when the Family ID is 0FH.
  //
  BasicFamilyId = BitFieldRead32 (RegEax, 8, 11);
  FamilyId      = BasicFamilyId;
  if (BasicFamilyId == 0x0F) {
    FamilyId += BitFieldRead32 (RegEax, 20, 27);
  }

  //
  // The Extended Model ID needs to be examined only when the Family ID is 06H or 0FH.
  //
  ModelId = BitFieldRead32 (RegEax, 4, 7);
  if (BasicFamilyId == 0x06 || BasicFamilyId == 0x0f) {
    ExtendedModelId = BitFieldRead32 (RegEax, 16, 19);
    ModelId        += ExtendedModelId << 4;
  }

  switch (FamilyId) {
  case 0x0F:
    //
    // It's Pentinum 4 Family CPUs.
    //
    switch (ModelId) {
    case 0x03:
    case 0x04:
    case 0x06:
      //
      // EFI_MSR_EBC_FREQUENCY_ID[18:16]
      //
      FrequencyIndex = (UINT32) BitFieldRead64 (AsmReadMsr64 (EFI_MSR_EBC_FREQUENCY_ID), 16, 18);
      return mPentinum4XeonFSBFrequencies[FrequencyIndex];
    }
    break;

  case 0x06:
    //
    // It's Core 2 Duo Family CPUs.
    //
    switch (ModelId) {
    case 0x0F:
    case 0x16:
    case 0x17:
    case 0x19:
      //
      // EFI_MSR_PSB_CLOCK_STATUS[2:0]
      //
      FrequencyIndex = (UINT32) BitFieldRead64 (AsmReadMsr64 (EFI_MSR_PSB_CLOCK_STATUS), 0, 2);
      return mCore2DuoFSBFrequencies[FrequencyIndex];
      
    case 0x1a:
    case 0x1e:
    case 0x1f:
    case 0x2e:
    case 0x25:
    case 0x2c:
    case 0x2f:
      //
      // Fixed 133MHz for Nehalem processors.
      //
      return 133333333;
    case 0x2a:
    case 0x2d:
      //
      // Fixed 100MHz for Sandy Bridge processors.
      //
    case 0x36:  
      //
      // Fixed 100MHz fpr CedarTrail processor.
      //
    case 0x3a: 
      //
      // Fixed 100MHz for Ivy Bridge processors.
      //   
    case 0x3c:
#ifdef ULT_FLAG
    case 0x45:
#endif // ULT_FLAG
      //
      // Fixed 100MHz for Haswell processors.
      //
      return 100000000;
    }
    break;
  }

  //
  // Always assert() for those unsupported CPUs
  //
  ASSERT (FALSE);

  return (UINT32) -1;
}


/**
  Internal function to return the frequency of the local APIC timer.
  @return The frequency of the timer in Hz.
**/
UINT32
InternalX86GetTimerFrequency (
  VOID
  )
{
  UINT32 Freq;
  UINTN  DivideValue;

  Freq = GetIntendFsbFrequency ();
  GetApicTimerState (&DivideValue, NULL, NULL);
  return Freq / ((UINT32) DivideValue);
}

/**
  Internal function to read the current tick counter of local APIC.

  Internal function to read the current tick counter of local APIC.

  @return The tick counter read.

**/
INT32
InternalX86GetTimerTick (
  VOID
  )
{
  return GetApicTimerCurrentCount ();
}

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  Delay     A period of time to delay in ticks.

**/
VOID
InternalX86Delay (
  IN      UINT32                    Delay
  )
{
  INT32                             Ticks;

  //
  // The target timer count is calculated here
  //
  Ticks = InternalX86GetTimerTick () - Delay;

  //
  // Wait until time out
  // Delay > 2^31 could not be handled by this function
  // Timer wrap-arounds are handled correctly by this function
  //
  while (InternalX86GetTimerTick () - Ticks >= 0);
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  InternalX86Delay (
    (UINT32)DivU64x32 (
              MultU64x64 (
                InternalX86GetTimerFrequency (),
                MicroSeconds
                ),
              1000000u
              )
    );
  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return NanoSeconds

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN                     NanoSeconds
  )
{
  InternalX86Delay (
    (UINT32)DivU64x32 (
              MultU64x64 (
                InternalX86GetTimerFrequency (),
                NanoSeconds
                ),
              1000000000u
              )
    );
  return NanoSeconds;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  Retrieves the current value of a 64-bit free running performance counter. The
  counter can either count up by 1 or count down by 1. If the physical
  performance counter counts by a larger increment, then the counter values
  must be translated. The properties of the counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return (UINT64)(UINT32)InternalX86GetTimerTick ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  If StartValue is not NULL, then the value that the performance counter starts
  with immediately after is it rolls over is returned in StartValue. If
  EndValue is not NULL, then the value that the performance counter end with
  immediately before it rolls over is returned in EndValue. The 64-bit
  frequency of the performance counter in Hz is always returned. If StartValue
  is less than EndValue, then the performance counter counts up. If StartValue
  is greater than EndValue, then the performance counter counts down. For
  example, a 64-bit free running counter that counts up would have a StartValue
  of 0 and an EndValue of 0xFFFFFFFFFFFFFFFF. A 24-bit free running counter
  that counts down would have a StartValue of 0xFFFFFF and an EndValue of 0.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64                    *StartValue,  OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = GetApicTimerInitCount ();
  }

  if (EndValue != NULL) {
    *EndValue = 0;
  }

  return (UINT64) InternalX86GetTimerFrequency ();
}

/**
  Converts elapsed ticks of performance counter to time in nanoseconds.

  This function converts the elapsed ticks of running performance counter to
  time value in unit of nanoseconds.

  @param  Ticks     The number of elapsed ticks of running performance counter.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN      UINT64                     Ticks
  )
{
  UINT64  Frequency;
  UINT64  NanoSeconds;
  UINT64  Remainder;
  INTN    Shift;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x64Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder = RShiftU64 (Remainder, (UINTN) Shift);
  Frequency = RShiftU64 (Frequency, (UINTN) Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}
