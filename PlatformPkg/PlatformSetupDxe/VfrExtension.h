/*++
Copyright (c) 2010 Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  VfrExtension.h

Abstract:

  OEM Specific Setup Variables and Structures
--*/

#ifndef __VFR_EXTENSION___H__
#define __VFR_EXTENSION___H__

#include "SetupVariable.h"
#include <Guid/SetupPassword.h>



//
// GUID for each of formset
//
#define MAIN_FORM_SET_GUID  \
  { \
    0x985eee91, 0xbcac, 0x4238, { 0x87, 0x78, 0x57, 0xef, 0xdc, 0x93, 0xf2, 0x4e } \
  }

#define ADVANCED_FORM_SET_GUID \
  { \
    0xe14f04fa, 0x8706, 0x4353, { 0x92, 0xf2, 0x9c, 0x24, 0x24, 0x74, 0x6f, 0x9f } \
  }

#define DEVICES_FORM_SET_GUID \
  { \
    0xadfe34c8, 0x9ae1, 0x4f8f, { 0xbe, 0x13, 0xcf, 0x96, 0xa2, 0xcb, 0x2c, 0x5b } \
  }

#define POWER_FORM_SET_GUID \
  { \
    0x5b5eb989, 0x4702, 0x47c5, { 0xbb, 0xe0, 0x4, 0xb9, 0x99, 0xf6, 0x2, 0x1e } \
  }

#define BOOT_FORM_SET_GUID \
  { \
    0x8b33ffe0, 0xd71c, 0x4f82, { 0x9c, 0xeb, 0xc9, 0x70, 0x58, 0xc1, 0x3f, 0x8e } \
  }

#define SECURITY_FORM_SET_GUID \
  { \
    0x981ceaee, 0x931c, 0x4a17, { 0xb9, 0xc8, 0x66, 0xc7, 0xbc, 0xfd, 0x77, 0xe1 } \
  }

#define EXIT_FORM_SET_GUID \
  { \
    0xa43b03dc, 0xc18a, 0x41b1, { 0x91, 0xc8, 0x3f, 0xf9, 0xaa, 0xa2, 0x57, 0x13 } \
  }

#define RESET_FORM_SET_GUID \
  { \
    0x46975fe8, 0x64f6, 0x4692, { 0x8d, 0x64, 0xd5, 0x6a, 0xa7, 0x52, 0xea, 0xa9 } \
  }

#define HDD_PASSWORD_CONFIG_GUID \
  { \
    0xd5fd1546, 0x22c5, 0x4c2e, { 0x96, 0x9f, 0x27, 0x3c, 0x0, 0x77, 0x10, 0x80 } \
  }

#define SECUREBOOT_CONFIG_FORM_SET_GUID \
  { \
    0x5daf50a5, 0xea81, 0x4de2, {0x8f, 0x9b, 0xca, 0xbd, 0xa9, 0xcf, 0x5c, 0x14} \
  }

#define CALLBACK_GUID \
  { 0x9CF0F18E, 0x7C7D, 0x49DE, { 0xB5, 0xAA, 0xBB, 0xBA, 0xD6, 0xB2, 0x10, 0x07 } }

#define BOOT_MANAGER_GUID \
  { 0xB4909CF3, 0x7B93, 0x4751, { 0x9B, 0xD8, 0x5B, 0xA8, 0x22, 0x0B, 0x9B, 0xB2 } }

#define BOOT_NOW_COUNT_GUID \
  { 0x052e6eb0, 0xf240, 0x42c5, { 0x83, 0x09, 0x45, 0x87, 0x45, 0x45, 0xc6, 0xb4 } }

#define EFI_SHELL_FILE_GUID \
  { 0xc57ad6b7, 0x0515, 0x40a8, { 0x9d, 0x21, 0x55, 0x16, 0x52, 0x85, 0x4e, 0x37 } }

#define EFI_GLOBAL_VARIABLE_GUID \
  { 0x8BE4DF61, 0x93CA, 0x11D2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } }

#define SYSTEM_ACCESS_GUID \
  { 0xE770BB69, 0xBCB4, 0x4D04, { 0x9E, 0x97, 0x23, 0xFF, 0x94, 0x56, 0xFE, 0xAC } }

#define IDE_SECURITY_CONFIG_GUID \
  { 0x3dd0de67, 0x02d7, 0x4129, { 0x91, 0x4a, 0x9f, 0x37, 0x7c, 0xc3, 0x4b, 0x0d } }

// {613F0874-22DD-4f33-8A3A-7F7A96F9E3AA}
#define EM_FORMSET_GUID \
  { \
    0x613f0874, 0x22dd, 0x4f33, 0x8a, 0x3a, 0x7f, 0x7a, 0x96, 0xf9, 0xe3, 0xaa \
  }
 
//
// Class/Sub-Class GUID Definiton
//
#define MAIN_FORM_SET_CLASS             0x01
#define ADVANCED_FORM_SET_CLASS         0x02
#define DEVICES_FORM_SET_CLASS          0x03
#define BOOT_FORM_SET_CLASS             0x04
#define SECURITY_FORM_SET_CLASS         0x05
#define EXIT_FORM_SET_CLASS             0x06
#define POWER_FORM_SET_CLASS            0x08

#define NON_FRONT_PAGE_SUBCLASS         0x00
#define FRONT_PAGE_SUBCLASS             0x02

#define TSESETUP_KEY                    0xF013


#pragma pack()


#endif
#define SETUP_DATA_VARSTORE \
  efivarstore SETUP_DATA, varid = 1, attribute = 0x7, name  = Setup, guid  = PLATFORM_SETUP_VARIABLE_GUID;  

                  
#define LANGUAGE_VARSTORE   \
  varstore LANGUAGE, varid = LANGUAGE_KEY_ID, name  = Lang, guid  = EFI_GLOBAL_VARIABLE_GUID;
    
#define LANGUAGE_CODES_VARSTORE \
  varstore LANGUAGE_CODES, varid = LANGUAGE_CODES_KEY_ID, name = LangCodes, guid = EFI_GLOBAL_VARIABLE_GUID;
    
#define SYSTEM_ACCESS_VARSTORE  \
  varstore SYSTEM_ACCESS, varid = SYSTEM_ACCESS_KEY_ID, name = SystemAccess, guid = SYSTEM_ACCESS_GUID;
  
#define TSESETUP_VARSTORE    \
  varstore TSESETUP, varid = TSESETUP_KEY, name = TSESetup, guid = TSESETUP_GUID;
  
#define BOOT_MANAGER_VARSTORE   \
  varstore BOOT_MANAGER, varid = BOOT_MANAGER_KEY_ID, name = BootManager, guid = BOOT_MANAGER_GUID;
    
#define TIMEOUT_VARSTORE        \
  varstore TIMEOUT, varid = BOOT_TIMEOUT_KEY_ID, name = Timeout, guid = EFI_GLOBAL_VARIABLE_GUID;
    
#define BOOT_ORDER_VARSTORE     \
  varstore BOOT_ORDER, varid = BOOT_ORDER_KEY_ID, name = BootOrder, guid = EFI_GLOBAL_VARIABLE_GUID;
    
#define NEW_BOOT_OPTION_VARSTORE \
  varstore NEW_BOOT_OPTION, varid = NEW_BOOT_OPTION_KEY_ID, name = AddBootOption, guid = ADD_BOOT_OPTION_GUID; 
    
#define DEL_BOOT_OPTION_VARSTORE \
  varstore DEL_BOOT_OPTION, varid = DEL_BOOT_OPTION_KEY_ID, name = DelBootOption, guid = DEL_BOOT_OPTION_GUID;
    
#define LEGACY_DEV_INFO_VARSTORE \
  varstore LEGACY_DEV_INFO, varid = LEGACY_DEV_KEY_ID, name = LegacyDev, guid = LEGACY_DEV_ORDER_GUID;
    
#define LEGACY_GROUP_INFO_VARSTORE \
  varstore LEGACY_GROUP_INFO, varid = LEGACY_GROUP_KEY_ID, name  = LegacyGroup, guid = LEGACY_DEV_ORDER_GUID;
    
#define LEGACY_DEV_ORDER_VARSTORE\
  varstore LEGACY_DEV_ORDER, varid = LEGACY_ORDER_KEY_ID, name = LegacyDevOrder, guid = LEGACY_DEV_ORDER_GUID;
    
#define CALLBACK_VARSTORE  \
  varstore CALLBACK, varid = CALLBACK_KEY_ID, name = Callback, guid = CALLBACK_GUID;
    
#define BOOT_NOW_COUNT_VARSTORE \
  varstore BOOT_NOW_COUNT, varid = BOOT_NOW_COUNT_KEY_ID, name = BootNowCount, guid = BOOT_NOW_COUNT_GUID;
    
#define EFI_SHELL_VARSTORE \
  varstore EFI_SHELL, varid = EFI_SHELL_KEY_ID, name = Shell, guid = EFI_SHELL_FILE_GUID;
