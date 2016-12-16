/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  PasswordFormCallback.c

Abstract:
  PasswordFormCallback Setup Rountines

Revision History:


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
#include <Guid/SetupPassword.h>
#include <Library/BaseCryptLib.h>

EFI_STATUS
EFIAPI
Sha256Hash (
  IN  CONST VOID        *Data,
  IN        UINTN       DataLen,
  OUT       UINT8       *Digest,
  IN        UINTN       DigestSize  
  )
{
  VOID     *Sha256Ctx;
  UINTN    CtxSize;

  ASSERT(Data != NULL && DataLen != 0 && Digest != NULL && DigestSize >= 32);

  CtxSize = Sha256GetContextSize();
  Sha256Ctx = AllocatePool(CtxSize);
  ASSERT(Sha256Ctx != NULL);

  Sha256Init (Sha256Ctx);
  Sha256Update (Sha256Ctx, Data, DataLen);
  Sha256Final (Sha256Ctx, Digest);

  FreePool(Sha256Ctx);

  return EFI_SUCCESS;
}


EFI_GUID  gTseSetupGuid = TSESETUP_GUID;
TSESETUP  gTse;
TSESETUP  gTseTemp;


VOID SystemHalt()
{
  CHAR16  *Result = L"Fatal Error... System Halted.";

  CreatePopUp(EFI_RED | EFI_BACKGROUND_LIGHTGRAY, NULL, Result, NULL);
  gBS->RaiseTPL(TPL_HIGH_LEVEL);
  CpuDeadLoop();
}


EFI_STATUS
EFIAPI
PasswordFormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  )
{
  EFI_STATUS         Status = EFI_SUCCESS;  
  UINTN              VarSize;
  UINTN              i;
  static INTN        StateMathine = 0;  
  CHAR16             *Password = NULL;
  EFI_HII_HANDLE     HiiHandle = NULL;
  static INTN        ADMIPassWdCount = 0;
  static INTN        UserPassWdCount = 0;
  UINT8              Sha256HashArray[32]; 
  BOOLEAN            Rc;	
  UINT32             Attributes;	


  DEBUG((EFI_D_INFO, "A:%X K:%X T:%X\n", Action, KeyValue, Type));
  
  if (Action != EFI_BROWSER_ACTION_CHANGED && Action != EFI_BROWSER_ACTION_CHANGING) {
    return EFI_SUCCESS;
  }
  if (Action == EFI_BROWSER_ACTION_CHANGING && KeyValue == KEY_USER_ACCESS_LEVEL) {
    return EFI_SUCCESS;
  }
	
  if ((Type == EFI_IFR_TYPE_STRING) && (Value->string == 0)) {
		DEBUG((EFI_D_INFO, "StateMathine -> 0\n"));
    StateMathine = 0;
    return EFI_INVALID_PARAMETER;
  }

  for (i=0;i<gSetupCallbackInfoNumber;i++) {
    if (gSetupCallbackInfo[i].Class == SECURITY_FORM_SET_CLASS) {
      HiiHandle = gSetupCallbackInfo[i].HiiHandle;
      break;
    }
  }  
  if (HiiHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }
 
  VarSize = sizeof(TSESETUP);
  Status = gRT->GetVariable (
                  SETUP_PASSWORD_VARTSE_NAME,
                  &gTseSetupGuid,
                  &Attributes,
                  &VarSize,
                  &gTse
                  );
  ASSERT(!EFI_ERROR(Status));     
  if (EFI_ERROR(Status)) {    
    return Status;
  }
  
  switch (KeyValue) {
    case KEY_USER_ACCESS_LEVEL:
      Rc = HiiGetBrowserData (
             &gTseSetupGuid, 
             SETUP_PASSWORD_VARTSE_NAME,
             sizeof(gTseTemp), 
             (UINT8*)&gTseTemp
             );	
      DEBUG((EFI_D_INFO, "HiiGetBrowserData:%d, AccLvl:%d\n", Rc, gTseTemp.UserAccessLevel));
      gTse.UserAccessLevel = gTseTemp.UserAccessLevel;
      break;				
		
    case KEY_ADMIN_PASSWORD:
      Password = HiiGetString(HiiHandle, Value->string, NULL);		
      if (gTse.AdminPasswdPresent) {
        if (StateMathine == 0) {       
          if (Password[0] == CHAR_NULL) { 
            Status = EFI_INVALID_PARAMETER;     
          } else {
            Status = EFI_SUCCESS;   
          }
        } else if (StateMathine == 1){
          Sha256Hash(Password, StrSize(Password), Sha256HashArray, sizeof(Sha256HashArray));
          if(CompareMem(gTse.AdminPasswdHash, Sha256HashArray, sizeof(Sha256HashArray)) == 0){
            ADMIPassWdCount = 0;
            Status = EFI_SUCCESS;
          } else {
            ADMIPassWdCount++;
            if (ADMIPassWdCount == 3){
              SystemHalt();
            }
            Status = EFI_NOT_READY;   
          }
        } else if (StateMathine == 2){   //Set new password.
          StateMathine = -1;
          if(Password[0] == 0){  // clear.
            ZeroMem(gTse.AdminPasswdHash, sizeof(gTse.AdminPasswdHash));
            gTse.AdminPasswdPresent = 0;
          } else {
            Sha256Hash(Password, StrSize(Password), Sha256HashArray, sizeof(Sha256HashArray)); 
            CopyMem(gTse.AdminPasswdHash, Sha256HashArray, sizeof(Sha256HashArray));
            gTse.AdminPasswdPresent = 1;
          }
        }
      } else {                         // admin password not present
        if (StateMathine == 0) {
          if (Password[0] == CHAR_NULL) {  
            Status = EFI_SUCCESS;          
          } else {
            Status = EFI_INVALID_PARAMETER;   
          }
        } else if  (StateMathine == 1){
          StateMathine = -1;
          if (Password[0] != 0) {
            Sha256Hash(Password, StrSize(Password), Sha256HashArray, sizeof(Sha256HashArray)); 
            CopyMem(gTse.AdminPasswdHash, Sha256HashArray, sizeof(Sha256HashArray));
            gTse.AdminPasswdPresent = 1;
            Status = EFI_SUCCESS;
          } else {                              // clear
            ZeroMem(gTse.AdminPasswdHash, sizeof(gTse.AdminPasswdHash));
            gTse.AdminPasswdPresent = 0;
          }
        } 
      } 
      StateMathine++;
      break;
    
    case KEY_USER_PASSWORD:
      Password = HiiGetString(HiiHandle, Value->string, NULL);			
      if (gTse.UserPasswdPresent) {
        if (StateMathine == 0) {       
          if (Password[0] == CHAR_NULL) { 
            Status = EFI_INVALID_PARAMETER;     
          } else {
            Status = EFI_SUCCESS;   
          }
        } else if (StateMathine == 1){
          Sha256Hash(Password, StrSize(Password), Sha256HashArray, sizeof(Sha256HashArray));
          if(CompareMem(gTse.UserPasswdHash, Sha256HashArray, sizeof(Sha256HashArray)) == 0){
            ADMIPassWdCount = 0;
            Status = EFI_SUCCESS;
          } else {
            ADMIPassWdCount++;
            if (ADMIPassWdCount == 3){
              SystemHalt();
            }
            Status = EFI_NOT_READY;   
          }
        } else if (StateMathine == 2){   //Set new password.
          StateMathine = -1;
          if(Password[0] == 0){  // clear.
            ZeroMem(gTse.UserPasswdHash, sizeof(gTse.UserPasswdHash));
            gTse.UserPasswdPresent = 0;
          } else {
            Sha256Hash(Password, StrSize(Password), Sha256HashArray, sizeof(Sha256HashArray)); 
            CopyMem(gTse.UserPasswdHash, Sha256HashArray, sizeof(Sha256HashArray));
            gTse.UserPasswdPresent = 1;
          }
        }
      } else {
        if (StateMathine == 0) {
          if (Password[0] == CHAR_NULL) {  
            Status = EFI_SUCCESS;          
          } else {
            Status = EFI_INVALID_PARAMETER;   
          }
        } else if  (StateMathine == 1){
          StateMathine = -1;
          if (Password[0] != 0) {
            Sha256Hash(Password, StrSize(Password), Sha256HashArray, sizeof(Sha256HashArray)); 
            CopyMem(gTse.UserPasswdHash, Sha256HashArray, sizeof(Sha256HashArray));
            gTse.UserPasswdPresent = 1;
            Status = EFI_SUCCESS;
          } else {
            ZeroMem(gTse.UserPasswdHash, sizeof(gTse.UserPasswdHash));
            gTse.UserPasswdPresent = 0;
          }
        } 
      } 
      StateMathine++;
      break;    
    
    default:
      break;
    
  }

  HiiSetBrowserData (
            &gTseSetupGuid, 
            SETUP_PASSWORD_VARTSE_NAME, 
            sizeof(TSESETUP), 
            (UINT8*)&gTse, 
            NULL
            );

  gRT->SetVariable (
         SETUP_PASSWORD_VARTSE_NAME,
         &gTseSetupGuid,
         Attributes,
         sizeof(TSESETUP),
         &gTse
         );
  if(Password != NULL){
    ZeroMem(Password, StrSize(Password));		
    FreePool(Password);
  }
  DEBUG((EFI_D_INFO, __FUNCTION__" %r M:%d\n", Status, StateMathine));	
  return Status;
}


