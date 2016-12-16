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
  CSMSETUP.C

Abstract:
  CSM Setup related functions
--*/

#include "PlatformSetup.h"


//----------------------------------------------------------------------------
// Procedure: InitCsmStrings
//
// Description: This function is eLink'ed with the chain executed right before
//        the Setup.
//
//----------------------------------------------------------------------------
//

VOID InitCsmStrings(EFI_HII_HANDLE HiiHandle, UINT16 Class)
{
  //example: InitString(HiiHandle, STRING_TOKEN(STR_USB_MODULE_VERSION_VALUE), L"%d", 25);
}
