/** @file

Copyright (c) 2006 - 2014, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  BeepLib.h

Abstract:
  Beep of platform.

Revision History:

**/
#ifndef _BEEP_PLATFORM_LIB_H_
#define _BEEP_PLATFORM_LIB_H_

/**
  Produces Beep.

  @param  Note         The note of beep.
  @param  Octave       The octave of beep.
  @param  Duration     The duration of beep in MicroSeconds.

  @retval None

**/

#define NOTE_DO    0
#define NOTE_RE    1
#define NOTE_MI    2
#define NOTE_FA    3
#define NOTE_SOL   4
#define NOTE_LA    5
#define NOTE_SI    6

VOID
Beep (
  UINT8            Note,
  UINT8            Octave,
  UINT32           Duration
  );

#endif