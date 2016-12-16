/*++

Copyright (c) 2006-2012, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Description:

  Define some data used for Boot Maint

**/

#include "BootMaint.h"

VOID                *mStartOpCodeHandle = NULL;
VOID                *mEndOpCodeHandle = NULL;
EFI_IFR_GUID_LABEL  *mStartLabel = NULL;
EFI_IFR_GUID_LABEL  *mEndLabel = NULL;

STRING_DEPOSITORY   *FirstStrDepository;
STRING_DEPOSITORY   *BootOptionStrDepository;
STRING_DEPOSITORY   *BootOptionHelpStrDepository;

///
/// Boot Option from variable Menu
///
BM_MENU_OPTION      BootOptionMenu = {
  BM_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};

///
/// Legacy FD Info from LegacyBios.GetBbsInfo()
///
BM_MENU_OPTION      LegacyFDMenu = {
  BM_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};

///
/// Legacy HD Info from LegacyBios.GetBbsInfo()
///
BM_MENU_OPTION      LegacyHDMenu = {
  BM_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};

///
/// Legacy CD Info from LegacyBios.GetBbsInfo()
///
BM_MENU_OPTION      LegacyCDMenu = {
  BM_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};

///
/// Legacy NET Info from LegacyBios.GetBbsInfo()
///
BM_MENU_OPTION      LegacyNETMenu = {
  BM_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};

///
/// Legacy NET Info from LegacyBios.GetBbsInfo()
///
BM_MENU_OPTION      LegacyBEVMenu = {
  BM_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};



