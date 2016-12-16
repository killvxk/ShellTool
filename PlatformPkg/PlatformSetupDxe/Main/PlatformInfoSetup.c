/*++
Copyright (c) 2010 Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.
--*/
//
//
// Name:  PlatformSetup.c
//
// Description:  Platform Setup Rountines
//
//
//**********************************************************************
#include "PlatformSetup.h"
#include <AutoBuildTime.h>
#include <BiosVersion.h>


VOID
InitPlatformInfoStrings (
  EFI_HII_HANDLE HiiHandle,
  UINT16         Class
  )
{
  if (Class == MAIN_FORM_SET_CLASS) {
    DEBUG ((EFI_D_INFO, "<InitPlatformInfoStrings>"));
    InitString (
      HiiHandle,
      STRING_TOKEN(STR_BIOS_DATE_VALUE),
      L"%a",
      TKN_BUILD_TIME_STR
      );
    InitString (
      HiiHandle,
      STRING_TOKEN(STR_BIOS_REVISION_LEVEL_VALUE),
      L"%a",
      TKN2STR(TKN_BIOSID_NAME)
      );      
      
      
  } 

}

