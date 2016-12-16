/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  ExitSetup.c

Abstract:
  Exit Setup Rountines

Revision History:

Bug 3211 - Add a setup item to control enable HDD frozen or not. 
TIME:       2011-12-09
$AUTHOR:    ZhangLin
$REVIEWERS:
$SCOPE:     Sugar Bay.
$TECHNICAL: 
  1. add a setup save notify protocol. NV Variable should be
     handled by module itself.
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
--*/

#include <PlatformSetup.h>
#include <Protocol/SetupSaveNotify.h>


 EFI_STATUS 
 SetupSaveNotify ( 
   SETUP_SAVE_NOTIFY_TYPE Type,
   BOOLEAN                *IsAllDataChanged  OPTIONAL
   )
 {
   EFI_STATUS                         Status;
 	 EFI_HANDLE                         *HandleBuffer;
 	 EFI_HANDLE                         Handle;
   UINTN                              HandleCount;
   UINTN                              Index;
   SETUP_SAVE_NOTIFY_PROTOCOL         *ptSetupSaveNotify;
   BOOLEAN                            IsDataChanged;
 
 	 HandleBuffer = NULL;
   DEBUG ((EFI_D_INFO, "SetupSaveNotify(%d)\n", Type));

   Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gSetupSaveNotifyProtocolGuid,
                   NULL,
                   &HandleCount,
                   &HandleBuffer
                   );
   if (EFI_ERROR(Status)){
     goto ProcExit; 
   }
 
   for (Index = 0; Index < HandleCount; Index++){
     Handle = HandleBuffer[Index];
     Status = gBS->HandleProtocol(
                     Handle,
                     &gSetupSaveNotifyProtocolGuid,
                     &ptSetupSaveNotify);
     ASSERT(!EFI_ERROR(Status));
     
     switch (Type){
     case SetupSaveNotifyTypeSaveValue:
       if (NULL != ptSetupSaveNotify->SaveValue) { 	
         Status = ptSetupSaveNotify->SaveValue (ptSetupSaveNotify);
       }
       break;
     case SetupSaveNotifyTypeDiscardVaule:
       if (NULL != ptSetupSaveNotify->DiscardValue) { 	
         Status = ptSetupSaveNotify->DiscardValue (ptSetupSaveNotify);
       }
       break;
     case SetupSaveNotifyTypeLoadDefault:
       if (NULL != ptSetupSaveNotify->LoadDefault) { 		 	
         Status = ptSetupSaveNotify->LoadDefault (ptSetupSaveNotify);
       }
       break;
     case SetupSaveNotifyTypeSaveUserDefault:
       if (NULL != ptSetupSaveNotify->SaveUserDefault) { 		 	
         Status = ptSetupSaveNotify->SaveUserDefault (ptSetupSaveNotify);
       }
       break;
     case SetupSaveNotifyTypeLoadUserDefault:
       if (NULL != ptSetupSaveNotify->LoadUserDefault) { 		 	
         Status = ptSetupSaveNotify->LoadUserDefault (ptSetupSaveNotify);
       }
       break;
     case SetupSaveNotifyTypeIsDataChanged:
       if (NULL != ptSetupSaveNotify->IsSetupDataChanged) { 		 	
         Status = ptSetupSaveNotify->IsSetupDataChanged (ptSetupSaveNotify, &IsDataChanged);
         if(IsAllDataChanged != NULL){
           *IsAllDataChanged |= IsDataChanged;
         }  
       }
       break;
     default:
       break;
     }
   }
 
 ProcExit:
   if (HandleBuffer != NULL){
     FreePool(HandleBuffer);
   }
   return Status;
 }

void
LoadStandardDefault (
  )
{
  EFI_STATUS Status;
  
  DEBUG((EFI_D_ERROR, "ExitSetup, LoadStandardDefault(),\n"));
  
  Status = mFormBrowserEx->ExecuteAction (BROWSER_ACTION_DEFAULT, EFI_HII_DEFAULT_CLASS_STANDARD);  
  if (!EFI_ERROR(Status)) {
    SetupSaveNotify(SetupSaveNotifyTypeLoadDefault, NULL);
  }
}

// ----------------------------------------------------------------------------
// Procedure: ExitFormCallBackFunction
//
// Description: This function validates the Flex Ratio setup value
//
// Input:
//       EFI_FORM_CALLBACK_PROTOCOL   *This
//       UINT16                       KeyValue
//       EFI_IFR_DATA_ARRAY           *Data,
//       EFI_HII_CALLBACK_PACKET      **Packet
//
// Output:
//        EFI_SUCCESS
//
// ----------------------------------------------------------------------------
//
EFI_STATUS
EFIAPI
ExitFormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  )
{
  EFI_STATUS    Status;
//SETUP_DATA    CurrentSetupData;

  if (Action != EFI_BROWSER_ACTION_CHANGING) {
    //
    // Do nothing for UEFI OPEN/CLOSE Action
    //
    return EFI_SUCCESS;
  }
  Status = EFI_SUCCESS;

  switch (KeyValue) {

  case KEY_SAVE_AND_EXIT_VALUE:
    Status = mFormBrowserEx->ExecuteAction (BROWSER_ACTION_SUBMIT | BROWSER_ACTION_EXIT, 0);
    if (!EFI_ERROR(Status)) {
      SetupSaveNotify (SetupSaveNotifyTypeSaveValue, NULL);
      mFormBrowserEx->ExecuteAction (BROWSER_ACTION_EXIT, 0);
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
      PlatformReset ();
    }
    break;

  case KEY_DISCARD_AND_EXIT_VALUE:
    Status = mFormBrowserEx->ExecuteAction (BROWSER_ACTION_DISCARD | BROWSER_ACTION_EXIT, 0);
    if (!EFI_ERROR(Status)) {
      SetupSaveNotify (SetupSaveNotifyTypeDiscardVaule, NULL);  
      mFormBrowserEx->ExecuteAction (BROWSER_ACTION_EXIT, 0);
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
      PlatformReset ();		
    }
    break;
        
  case KEY_RESTORE_DEFAULTS_VALUE:
    LoadStandardDefault ();
    break;    

/*    
  case KEY_OS_OPTIMIZED_DEFAULTS_VALUE:
    HiiGetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *) &CurrentSetupData);
    DEBUG((EFI_D_ERROR,"ExitFormCallback: Value->u8=%d, OsOptimizedDefaultConfig=%d, OsOptimizedDefaultCheck=%d\n",Value->u8,CurrentSetupData.OsOptimizedDefaultConfig,CurrentSetupData.OsOptimizedDefaultCheck));
    if (Value->u8 != CurrentSetupData.OsOptimizedDefaultCheck) {
      switch (Value->u8) {
        case 1:
          Status = mFormBrowserEx->ExecuteAction (BROWSER_ACTION_CONFIG_OS_OPTIMIZED_DEFAULTS | BROWSER_ACTION_ENABLE_OS_OPTIMIZED_DEFAULTS, 0);
          if (!EFI_ERROR(Status)) {
            //
            // When OS optimized defaults enabled, CSM should be disabled and PXE IPV4/PXE IPV6/Secure Boot should be enabled.	
            //
            CurrentSetupData.Csm = 0;
            CurrentSetupData.PxeIPV4  = 1;
            CurrentSetupData.PxeIPV6  = 1;
            CurrentSetupData.SecureBootConfig = 1;
            CurrentSetupData.OsOptimizedDefaultCheck = Value->u8;
          } else {
            Value->u8 = CurrentSetupData.OsOptimizedDefaultCheck;
          }
          break;

        case 0:
          Status = mFormBrowserEx->ExecuteAction (BROWSER_ACTION_CONFIG_OS_OPTIMIZED_DEFAULTS | BROWSER_ACTION_DISABLE_OS_OPTIMIZED_DEFAULTS, 0);
          if (!EFI_ERROR(Status)) {
            //
            // When OS optimized defaults disabled, CSM should be enabled and PXE IPV4/PXE IPV6/Secure Boot should be disabled and hidden.	
            //
            CurrentSetupData.Csm = 1;
            CurrentSetupData.PxeIPV4  = 0;
            CurrentSetupData.PxeIPV6  = 0;
            CurrentSetupData.SecureBootConfig = 0;
            CurrentSetupData.OsOptimizedDefaultCheck = Value->u8;
          } else {
            Value->u8 = CurrentSetupData.OsOptimizedDefaultCheck;
          }
          break;
      }
      HiiSetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *)&CurrentSetupData, NULL);
    }
    break;
*/    
  default:
    break;
    
  }

  return EFI_SUCCESS;
}
