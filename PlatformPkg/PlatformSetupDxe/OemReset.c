/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  OemReset.c

Abstract: 
  do platform reset.

Revision History:

Bug 2956 - Issue a full reset when hdd password installed or removed at setup exit.
TIME: 2011-10-01
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Hdd password set need a power cycle to be valid.
     Note: when password has been installed, if SATA mode changed, 
     full reset is also need.
$END--------------------------------------------------------------------

**/

/*++
Copyright (c) 2009 - 2010 Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  OemReset.c

--*/

#include "PlatformSetup.h"


VOID
PlatformReset (
  VOID
  )
{
  EFI_STATUS         Status;
  UINTN              VariableSize;
  EFI_HOB_GUID_TYPE  *GuidHob;
  SETUP_DATA         *SetupHob;
  STATIC SETUP_DATA  SetupData;


  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));

  VariableSize = sizeof(SETUP_DATA);
  Status = gRT->GetVariable (
                  PLATFORM_SETUP_VARIABLE_NAME,
                  &gPlatformSetupVariableGuid,
                  NULL,
                  &VariableSize,
                  &SetupData
                  );
  ASSERT(!EFI_ERROR(Status));
  if (EFI_ERROR (Status)) {
    goto DoColdReset;
  }

  GuidHob = GetFirstGuidHob(&gPlatformSetupVariableGuid);
  ASSERT(GuidHob!=NULL);	
  SetupHob = (SETUP_DATA*)GET_GUID_HOB_DATA(GuidHob);

  if (CompareMem(&SetupData, SetupHob, sizeof(SETUP_DATA))!=0) {
    DEBUG((EFI_D_INFO, "SetupData changed!\n"));
    PcdSetBool(PcdBootState, TRUE);
  }

DoColdReset:
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  CpuDeadLoop();
}



