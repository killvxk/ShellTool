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

  CallBack.c

Abstract:

  Internal header of the Setup Component.
--*/
#include "PlatformSetup.h"

static
EFI_STATUS
FormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  );

VOID
EFIAPI
UpdateCallBack (
  SETUP_CALLBACK_INFO *pCallBackFound
  );
  
SETUP_CALLBACK MainCallbackProtocol = {
  {NULL, NULL, FormCallback}, MAIN_FORM_SET_CLASS,     0};

SETUP_CALLBACK AdvancedCallbackProtocol = {
  {NULL, NULL, FormCallback}, ADVANCED_FORM_SET_CLASS, 0};

SETUP_CALLBACK DevicesCallbackProtocol = {
  {NULL, NULL, FormCallback}, DEVICES_FORM_SET_CLASS,  0};

SETUP_CALLBACK SecurityCallbackProtocol = {
  {NULL, NULL, FormCallback}, SECURITY_FORM_SET_CLASS, 0};

SETUP_CALLBACK ExitCallbackProtocol = {
  {NULL, NULL, FormCallback}, EXIT_FORM_SET_CLASS,     0};

SETUP_CALLBACK PowerCallbackProtocol = {
  {NULL, NULL, FormCallback}, POWER_FORM_SET_CLASS,     0};

SETUP_CALLBACK_INFO gSetupCallbackInfo[] = {
  {NULL, &MainCallbackProtocol.Callback,     MAIN_FORM_SET_CLASS,     0, 0},
  {NULL, &AdvancedCallbackProtocol.Callback, ADVANCED_FORM_SET_CLASS, 0, 0},
  {NULL, &DevicesCallbackProtocol.Callback,  DEVICES_FORM_SET_CLASS,  0, 0},
  {NULL, &SecurityCallbackProtocol.Callback, SECURITY_FORM_SET_CLASS, 0, 0},
  {NULL, &ExitCallbackProtocol.Callback,     EXIT_FORM_SET_CLASS,     0, 0},
  {NULL, &PowerCallbackProtocol.Callback,     POWER_FORM_SET_CLASS,     0, 0},
};
UINT8 gSetupCallbackInfoNumber =   sizeof(gSetupCallbackInfo) / sizeof(SETUP_CALLBACK_INFO);

SETUP_ITEM_CALLBACK mSetupItemCallback[] = {
  {0,                       0, 0,                 NULL} 
};

SETUP_ITEM_CALLBACK_EX mSetupItemCallbackEx[] = { 
  {SECURITY_FORM_SET_CLASS, 0, KEY_USER_PASSWORD,               PasswordFormCallback},
  {SECURITY_FORM_SET_CLASS, 0, KEY_ADMIN_PASSWORD,              PasswordFormCallback},
  {SECURITY_FORM_SET_CLASS, 0, KEY_USER_ACCESS_LEVEL,           PasswordFormCallback},
  {EXIT_FORM_SET_CLASS,     0, KEY_SAVE_AND_EXIT_VALUE  ,       ExitFormCallback},
  {EXIT_FORM_SET_CLASS,     0, KEY_DISCARD_AND_EXIT_VALUE,      ExitFormCallback},
  {EXIT_FORM_SET_CLASS,     0, KEY_RESTORE_DEFAULTS_VALUE,      ExitFormCallback},
  {DEVICES_FORM_SET_CLASS,     0, KEY_VALUE_PCIERST,            DevicesFormCallback},
  {DEVICES_FORM_SET_CLASS,	   0, KEY_VALUE_PCIERP ,	        DevicesFormCallback},
  {DEVICES_FORM_SET_CLASS,	   0, KEY_VALUE_PCIE_PE0 ,	        DevicesFormCallback},
  {DEVICES_FORM_SET_CLASS,	   0, KEY_VALUE_PCIE_PE1 ,			DevicesFormCallback},
  {DEVICES_FORM_SET_CLASS,	   0, KEY_VALUE_PCIE_PE2 ,	        DevicesFormCallback},
  {DEVICES_FORM_SET_CLASS,	   0, KEY_VALUE_PCIE_PE3 ,	        DevicesFormCallback},
  {DEVICES_FORM_SET_CLASS,	   0, KEY_VALUE_PCIE_PEG0 ,	        DevicesFormCallback},
  {DEVICES_FORM_SET_CLASS,	   0, KEY_VALUE_PCIE_PEG1 ,	        DevicesFormCallback},
  {DEVICES_FORM_SET_CLASS,	   0, KEY_VALUE_PCIE_PEG2 ,	        DevicesFormCallback},
  {DEVICES_FORM_SET_CLASS,	   0, KEY_VALUE_PCIE_PEG3 ,	        DevicesFormCallback},
  {DEVICES_FORM_SET_CLASS,	   0, KEY_VALUE_PCIE_PEG,	        DevicesFormCallback},  
   {POWER_FORM_SET_CLASS,	     0, KEY_C4P_CONTROL ,	      PowerFormCallback},
 //{RESET_FORM_SET_CLASS,    0, KEY_RESET_SYSTEM,                ResetFormCallback},
  //{MAIN_FORM_SET_CLASS,     0, MAIN_PAGE_KEY_LANGUAGE,          SetLanguageCallback},
  {0,                       0, 0,                               NULL}
  };
  
EFI_STATUS 
FormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
)
{
  UINT8                  i;
  SETUP_ITEM_CALLBACK    *pItemCallback;
  SETUP_ITEM_CALLBACK_EX *pItemCallbackEx;
  SETUP_CALLBACK         *pCallback;
  EFI_STATUS Status = EFI_SUCCESS;

  pCallback = (SETUP_CALLBACK*) This;
  pCallback =  _CR(This, SETUP_CALLBACK, Callback);
  
  if(KeyValue == MAIN_PAGE_KEY_LANGUAGE && Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD) {
    SetLanguageCallback(This, Action, KeyValue, Type, Value, ActionRequest);
    return EFI_SUCCESS;
  }
  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    return EFI_UNSUPPORTED;
  }

  for (i = 0; i < gSetupCallbackInfoNumber; i++) {    
    if (gSetupCallbackInfo[i].Class    == pCallback->Class && 
        gSetupCallbackInfo[i].SubClass == pCallback->SubClass) {

      pItemCallback   = mSetupItemCallback;
      while (pItemCallback->UpdateItem != NULL) {
        if ((pItemCallback->Class    == pCallback->Class) &&  
            (pItemCallback->SubClass == pCallback->SubClass) &&
            (pItemCallback->Key      == KeyValue)) {
          pItemCallback->UpdateItem (
                           gSetupCallbackInfo[i].HiiHandle,
                           pItemCallback->Class, 
                           pItemCallback->SubClass,
                           KeyValue
                           );
        }
        pItemCallback++;
      }

      pItemCallbackEx = mSetupItemCallbackEx;
      while (pItemCallbackEx->UpdateItemEx != NULL) {
        if ((pItemCallbackEx->Class    == pCallback->Class) && 
            (pItemCallbackEx->SubClass == pCallback->SubClass) &&  
            (pItemCallbackEx->Key      == KeyValue)) {
          Status = pItemCallbackEx->UpdateItemEx (
                             This,
                             Action,
                             KeyValue,
                             Type,
                             Value,
                             ActionRequest
                             );
        }
        pItemCallbackEx++;
      }
    }
  }
  return Status;
}

/*
EFI_STATUS 
ResetFormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
)
{
  if (KeyValue == KEY_RESET_SYSTEM) {
    gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
  }

  return EFI_SUCCESS;
}
*/
