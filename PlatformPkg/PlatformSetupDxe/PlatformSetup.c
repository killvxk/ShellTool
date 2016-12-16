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

  Setup.c

Abstract:

  Setup functions.
--*/


#include "PlatformSetup.h"

extern EFI_GUID gEfiSetupEnterGuid;

//
// Global GUIDs
//
EFI_GUID              BdsAllDriversConnectedProtocolGuid = BDS_ALL_DRIVERS_CONNECTED_PROTOCOL_GUID;

BOOLEAN               gIsSetToDefault       = FALSE;
BOOLEAN               gIsSetupEntered       = FALSE;

EFI_GUID              MainFormSetGuid      = MAIN_FORM_SET_GUID;
EFI_GUID              AdvancedFormSetGuid  = ADVANCED_FORM_SET_GUID;
EFI_GUID              DevicesFormSetGuid   = DEVICES_FORM_SET_GUID;
EFI_GUID              SecurityFormSetGuid  = SECURITY_FORM_SET_GUID;
EFI_GUID              ExitFormSetGuid      = EXIT_FORM_SET_GUID;
EFI_GUID              PowerFormSetGuid    = POWER_FORM_SET_GUID;

FORM_SET_INFO         gSetupFormSets[]      = {
  {MAIN_FORM_SET_GUID,     MainBin,     MAIN_FORM_SET_CLASS,     0, NULL},
  {ADVANCED_FORM_SET_GUID, AdvancedBin, ADVANCED_FORM_SET_CLASS, 0, NULL},
  {DEVICES_FORM_SET_GUID,  DevicesBin,  DEVICES_FORM_SET_CLASS,  0, NULL},
  {SECURITY_FORM_SET_GUID, SecurityBin, SECURITY_FORM_SET_CLASS, 0, NULL},
  {EXIT_FORM_SET_GUID,     ExitBin,     EXIT_FORM_SET_CLASS,     0, NULL},
  {POWER_FORM_SET_GUID,    PowerBin,    POWER_FORM_SET_CLASS,    0, NULL},  
};

BOOLEAN              mIsResourcesLoaded = FALSE;

EFI_BYO_FORM_BROWSER_EXTENSION_PROTOCOL *mFormBrowserEx = NULL;



VOID
InstallHiiResources (
  )
{
  UINT8                i;
  EFI_STATUS           Status;
  EFI_HANDLE           DriverHandle;

  for (i = 0; i < gSetupCallbackInfoNumber; i++) {
    //
    // Install Config Access protocol
    //
    Status = CreateHiiDriverHandle (&DriverHandle);
    ASSERT_EFI_ERROR (Status);

    gSetupFormSets[i].DriverHandle = DriverHandle;
    UpdateCallbackInfo (&gSetupCallbackInfo[i]);

    Status = gBS->InstallProtocolInterface (
                    &DriverHandle,
                    &gEfiHiiConfigAccessProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    gSetupCallbackInfo[i].pFormCallback
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Publish our HII data
    //
    gSetupCallbackInfo[i].HiiHandle = HiiAddPackages (
                                       &gSetupFormSets[i].FormSetGuid,
                                       DriverHandle,
                                       PlatformSetupDxeStrings,
                                       gSetupFormSets[i].IfrPack,
                                       NULL
                                       );
    ASSERT (gSetupCallbackInfo[i].HiiHandle != NULL);
  }
}

VOID
BackupSetupData (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  UINTN      VariableSize;
  SETUP_DATA BackupSetupData;
  EFI_STATUS Status;
  VOID       *Interface;
  UINT8      FormIndex;

  Status = gBS->LocateProtocol (
                  &gEfiSetupEnterGuid,
                  NULL,
                  &Interface
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  if (Event != NULL)
    gBS->CloseEvent (Event);

  //
  //Initialize the setup strings, such as the sata devices.
  //
  for (FormIndex = 0; FormIndex < gSetupCallbackInfoNumber; FormIndex++) {
    InitSetupStrings (gSetupCallbackInfo[FormIndex].HiiHandle, &gSetupCallbackInfo[FormIndex]);
  }

  VariableSize = sizeof (SETUP_DATA);
  Status = gRT->GetVariable (
                  PLATFORM_SETUP_VARIABLE_NAME,
                  &gPlatformSetupVariableGuid,
                  NULL,
                  &VariableSize,
                  &BackupSetupData
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gRT->SetVariable (
                    SETUP_TEMP_VARIABLE_NAME,
                    &gPlatformSetupVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    VariableSize,
                    &BackupSetupData
                    );
  ASSERT_EFI_ERROR (Status);

  gIsSetupEntered = TRUE;

  //
  // Set gIsSetToDefault to false, otherwise setup will don't use the vriable L"SetupTemporary".
  //
  gIsSetToDefault = FALSE;
}



EFI_STATUS 
PlatformLoadDefault (  
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  SETUP_DATA    *SetupData;

  SetupData = AllocatePool(sizeof(SETUP_DATA));
  ASSERT(SetupData!=NULL);
  if(SetupData == NULL){return EFI_OUT_OF_RESOURCES;}
 
  HiiGetBrowserData(&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8*)SetupData);
  SetupData->Csm         = 1;
  SetupData->UefiPxeIPV4 = 0;
  SetupData->UefiPxeIPV6 = 0;		
  HiiSetBrowserData(&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8*)SetupData, NULL); 
  return Status;
}



STATIC SETUP_SAVE_NOTIFY_PROTOCOL  gSetupSaveNotify; 

VOID
SetupProtocolEventCallback (
  IN EFI_EVENT Event,
  IN VOID     *Context
  )
{
  UINT8                         i;
  EFI_STRING                    ConfigRequest;
  EFI_STATUS                    Status;
  VOID                          *pSetupRegistration;
  VOID                          *Interface;
  BOOLEAN                       ActionFlag;
  EFI_HANDLE                    DriverHandle;


  DEBUG((EFI_D_INFO, __FUNCTION__"\n"));

  //
  // Add more check to locate protocol after got event, because
  // the library will signal this event immediately once it is register
  // just in case it is already installed.
  //
  if (!gIsSetToDefault) {
    Status = gBS->LocateProtocol (
                    &BdsAllDriversConnectedProtocolGuid,
                    NULL,
                    &Interface
                    );
    if (EFI_ERROR (Status)) {
      Status = gBS->LocateProtocol (
                    &gEfiSetupEnterGuid,
                    NULL,
                    &Interface
                    );
      if (EFI_ERROR (Status)) {
       return ;
      }
    }
  }

  PERF_START (NULL, "SetupProtocolEventCallback", NULL, 0);

  if (Event != NULL)gBS->CloseEvent(Event);

  if (mIsResourcesLoaded) {
    PERF_END (NULL, "SetupProtocolEventCallback", NULL, 0);
    return;
  }

  mIsResourcesLoaded = TRUE;
  InstallHiiResources ();
  InitLanguages (gSetupCallbackInfo[0].HiiHandle);

  for (i = 0; i < gSetupCallbackInfoNumber; i++) {
    if (gIsSetToDefault) {
      ConfigRequest = HiiConstructConfigHdr (
                        &gPlatformSetupVariableGuid,
                        PLATFORM_SETUP_VARIABLE_NAME,
                        gSetupFormSets[i].DriverHandle
                        );
      ASSERT (ConfigRequest != NULL);
      ActionFlag = HiiValidateSettings (ConfigRequest);
      ASSERT (ActionFlag);
      FreePool (ConfigRequest);
    }
  }

  UpdateLanguageSettingItem(gSetupCallbackInfo[0].HiiHandle);

  EfiCreateProtocolNotifyEvent(
    &gEfiSetupEnterGuid,
    TPL_CALLBACK,
    BackupSetupData,
    NULL,
    &pSetupRegistration
    );

  PERF_END (NULL, "SetupCallback", NULL, 0);

  ZeroMem(&gSetupSaveNotify, sizeof(gSetupSaveNotify));
  gSetupSaveNotify.LoadDefault = PlatformLoadDefault;
  DriverHandle = NULL;	
  Status = gBS->InstallProtocolInterface (
                  &DriverHandle,
                  &gSetupSaveNotifyProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gSetupSaveNotify
                  );   

}



EFI_STATUS
PlatformSetupEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS       Status;
  VOID             *Registration;
  EFI_BOOT_MODE    BootMode;
  BOOLEAN          IsLanguagVariableExist;
  VOID             *pVariableBuff;


  BootMode = GetBootModeHob();

  Status = gBS->LocateProtocol(&gEfiByoFormBrowserExProtocolGuid, NULL, &mFormBrowserEx);
  ASSERT_EFI_ERROR (Status);
  mFormBrowserEx->PlatformReset = PlatformReset;

  IsLanguagVariableExist = FALSE;
  pVariableBuff = GetVariable(LastLangCodes, &gPlatformSetupVariableGuid);
  if (pVariableBuff != NULL) {
    FreePool (pVariableBuff);
    IsLanguagVariableExist = TRUE;
  }
  DEBUG((EFI_D_INFO, "%s Present:%d, BootMode:%d\n", LastLangCodes, IsLanguagVariableExist, BootMode));


  if (!IsLanguagVariableExist && BootMode != BOOT_IN_RECOVERY_MODE) {
    gIsSetToDefault = TRUE;
    SetupProtocolEventCallback (NULL, NULL);
    gIsSetToDefault = FALSE;
  } else {
    EfiCreateProtocolNotifyEvent (
      &gEfiSetupEnterGuid,
      TPL_CALLBACK,
      SetupProtocolEventCallback,
      NULL,
      &Registration
    );
  }

  if (BootMode != BOOT_IN_RECOVERY_MODE) {
    SynchornizeLanguageVariables();
  }

  return EFI_SUCCESS;
}


