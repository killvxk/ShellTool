/** @file

Copyright (c) 2006 - 2014, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  BeepLib.c

Abstract:
  Beep of platform.

Revision History:

**/
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>

#define NOTE(x) ((119318200 + (x) / 2) / (x))

/**
  Switch on Beep.

  @param  Note         The note of beep.
  @param  Octave       The octave of beep.

  @retval None

**/
VOID
BeepOn (
  UINT8 Note,
  UINT8 Octave
  )
{
  UINT16  Frequency;

  //
  // beep tones
  //
  UINT16  tones[8] = {
    NOTE (26163),
    NOTE (29366),
    NOTE (32963),
    NOTE (34923),
    NOTE (39200),
    NOTE (44000),
    NOTE (49388),
    NOTE (26163 * 2)
  };

  Frequency = tones[(Note % 8)];

  if (Octave - 1 >= 0) {
    Frequency >>= Octave - 1;
  } else {
    Frequency <<= 1 - Octave;
  }
  //
  // set up channel 1 (used for delays)
  //
  IoWrite8 (0x43, 0x54);
  IoWrite8 (0x41, 0x12);
  //
  // set up channel 2 (used by speaker)
  //
  IoWrite8 (0x43, 0xb6);
  IoWrite8 (0x42, (UINT8) Frequency);
  IoWrite8 (0x42, (UINT8) (Frequency >> 8));
  //
  // turn the speaker on
  //
  IoWrite8 (0x61, IoRead8 (0x61) | 3);
}

/**
  Switch off Beep.

  @retval None

**/
VOID
BeepOff (
  VOID
  )
{
  IoWrite8 (0x61, IoRead8 (0x61) & 0xfc);
}

/**
  Produces Beep.

  @param  Note         The note of beep.
  @param  Octave       The octave of beep.
  @param  Duration     The duration of beep in MicroSeconds.

  @retval None

**/
VOID
Beep (
  UINT8            Note,
  UINT8            Octave,
  UINT32           Duration
  )
{
  BeepOn (Note, Octave);
  MicroSecondDelay (Duration);
  BeepOff ();
}
