/*++

Copyright (c) 2006-2012, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Description:

  The functions for Boot Maintainence Main menu.

**/

#include "BootMaint.h"
#include <SetupVariable.h>


VOID
GroupMultipleLegacyBootOption4SameType (
  VOID
  );

EFI_DEVICE_PATH_PROTOCOL  EndDevicePath[] = {
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

HII_VENDOR_DEVICE_PATH  mBmmHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {165A028F-0BB2-4b5f-8747-77592E3F6499}
    //
    { 0x165a028f, 0xbb2, 0x4b5f, { 0x87, 0x47, 0x77, 0x59, 0x2e, 0x3f, 0x64, 0x99 } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

HII_VENDOR_DEVICE_PATH  mFeHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {91DB4238-B0C8-472e-BBCF-F3A6541010F4}
    //
    { 0x91db4238, 0xb0c8, 0x472e, { 0xbb, 0xcf, 0xf3, 0xa6, 0x54, 0x10, 0x10, 0xf4 } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

EFI_GUID mBootMaintGuid = BOOT_MAINT_FORMSET_GUID;
BMM_CALLBACK_DATA           *BmmCallbackInfo;

CHAR16  mBootMaintStorageName[]     = L"BmmData";


/**
  Init all memu.

  @param CallbackData    The BMM context data.

**/
VOID
InitAllMenu (
  IN  BMM_CALLBACK_DATA    *CallbackData
  );

/**
  Free up all Menu Option list.

**/
VOID
FreeAllMenu (
  VOID
  );

/**
  Create string tokens for a menu from its help strings and display strings

  @param CallbackData       The BMM context data.
  @param HiiHandle          Hii Handle of the package to be updated.
  @param MenuOption         The Menu whose string tokens need to be created

  @retval  EFI_SUCCESS      String tokens created successfully
  @retval  others           contain some errors
**/
EFI_STATUS
CreateMenuStringToken (
  IN BMM_CALLBACK_DATA                *CallbackData,
  IN EFI_HII_HANDLE                   HiiHandle,
  IN BM_MENU_OPTION                   *MenuOption
  )
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINTN         Index;

  for (Index = 0; Index < MenuOption->MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (MenuOption, Index);

    NewMenuEntry->DisplayStringToken = HiiSetString (
                                         HiiHandle,
                                         0,
                                         NewMenuEntry->DisplayString,
                                         NULL
                                         );

    if (NULL == NewMenuEntry->HelpString) {
      NewMenuEntry->HelpStringToken = NewMenuEntry->DisplayStringToken;
    } else {
      NewMenuEntry->HelpStringToken = HiiSetString (
                                        HiiHandle,
                                        0,
                                        NewMenuEntry->HelpString,
                                        NULL
                                        );
    }
  }

  return EFI_SUCCESS;
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
BootMaintExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS         Status;
  UINTN              BufferSize;
  BMM_CALLBACK_DATA  *Private;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;

  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &mBootMaintGuid, mBootMaintStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  Private = BMM_CALLBACK_DATA_FROM_THIS (This);
  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize = sizeof (BMM_FAKE_NV_DATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&mBootMaintGuid, mBootMaintStorageName, Private->BmmDriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) &Private->BmmFakeNvData,
                                BufferSize,
                                Results,
                                Progress
                                );
  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }
  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

EFI_STATUS
EFIAPI
BootMaintRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Configuration + StrLen (Configuration);
  return EFI_SUCCESS;
}
/**
  This function processes the results of changes in configuration.


  @param This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action             Specifies the type of action taken by the browser.
  @param QuestionId         A unique value which is sent to the original exporting driver
                            so that it can identify the type of data to expect.
  @param Type               The type of value for the question.
  @param Value              A pointer to the data being sent to the original exporting driver.
  @param ActionRequest      On return, points to the action requested by the callback function.

  @retval EFI_SUCCESS           The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR      The variable could not be saved.
  @retval EFI_UNSUPPORTED       The specified Action is not supported by the callback.
  @retval EFI_INVALID_PARAMETER The parameter of Value or ActionRequest is invalid.
**/
EFI_STATUS
EFIAPI
BootMaintCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL         *This,
  IN        EFI_BROWSER_ACTION                     Action,
  IN        EFI_QUESTION_ID                        QuestionId,
  IN        UINT8                                  Type,
  IN        EFI_IFR_TYPE_VALUE                     *Value,
  OUT       EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  BMM_CALLBACK_DATA *Private;
  BMM_FAKE_NV_DATA  *CurrentFakeNVMap;
  EFI_STATUS        Status;
  UINTN             OldValue;
  UINTN             NewValue;
  UINTN             Number;
  UINTN             Pos;
  UINTN             Bit;
  UINT16            NewValuePos;
  UINT16            Index3;
  UINT16            Index2;
  UINT16            Index;
  UINT8             *OldLegacyDev;
  UINT8             *NewLegacyDev;
  UINT8             *DisMap;
  EFI_FORM_ID       FormId;
  BOOLEAN           LegacyDevOrderChange = FALSE;
  
  
  Private        = BMM_CALLBACK_DATA_FROM_THIS (This);
  CurrentFakeNVMap = &Private->BmmFakeNvData;
  HiiGetBrowserData (&mBootMaintGuid, mBootMaintStorageName, sizeof (BMM_FAKE_NV_DATA), (UINT8 *) CurrentFakeNVMap);
  
  if ((Action == EFI_BROWSER_ACTION_FORM_OPEN) || (Action == EFI_BROWSER_ACTION_FORM_CLOSE)) {
    Status = EFI_SUCCESS;
    goto EXIT;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {
    return EFI_UNSUPPORTED;
  }

  DEBUG((EFI_D_INFO, "BMCB A:%d Q:%d T:%d\n", Action, QuestionId, Type));

  if ((Value == NULL) || (ActionRequest == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }


  OldValue       = 0;
  NewValue       = 0;
  Number         = 0;
  OldLegacyDev   = NULL;
  NewLegacyDev   = NULL;
  NewValuePos    = 0;
  DisMap         = NULL;
  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;


  UpdatePageId (Private, QuestionId);


  //
  // need to be subtituded.
  //
  // Update Select FD/HD/CD/NET/BEV Order Form
  //
  if (FORM_SET_FD_ORDER_ID == Private->BmmPreviousPageId ||
      FORM_SET_HD_ORDER_ID == Private->BmmPreviousPageId ||
      FORM_SET_CD_ORDER_ID == Private->BmmPreviousPageId ||
      FORM_SET_NET_ORDER_ID == Private->BmmPreviousPageId ||
      FORM_SET_BEV_ORDER_ID == Private->BmmPreviousPageId ||

      ((FORM_MAIN_ID == Private->BmmPreviousPageId ||
        FORM_BOOT_CHG_ID == Private->BmmPreviousPageId) &&
      (QuestionId >= LEGACY_FD_QUESTION_ID) &&
       (QuestionId < (LEGACY_BEV_QUESTION_ID + 100)) )
      ) {

      DisMap = Private->BmmOldFakeNVData.DisableMap;
      FormId = Private->BmmCurrentPageId;


    switch (FormId) {
    case FORM_SET_FD_ORDER_ID:
      Number        = (UINT16) LegacyFDMenu.MenuNumber;
      OldLegacyDev  = Private->BmmOldFakeNVData.LegacyFD;
      NewLegacyDev  = CurrentFakeNVMap->LegacyFD;
      break;

    case FORM_SET_HD_ORDER_ID:
      Number        = (UINT16) LegacyHDMenu.MenuNumber;
      OldLegacyDev  = Private->BmmOldFakeNVData.LegacyHD;
      NewLegacyDev  = CurrentFakeNVMap->LegacyHD;
      break;

    case FORM_SET_CD_ORDER_ID:
      Number        = (UINT16) LegacyCDMenu.MenuNumber;
      OldLegacyDev  = Private->BmmOldFakeNVData.LegacyCD;
      NewLegacyDev  = CurrentFakeNVMap->LegacyCD;
      break;

    case FORM_SET_NET_ORDER_ID:
      Number        = (UINT16) LegacyNETMenu.MenuNumber;
      OldLegacyDev  = Private->BmmOldFakeNVData.LegacyNET;
      NewLegacyDev  = CurrentFakeNVMap->LegacyNET;
      break;

    case FORM_SET_BEV_ORDER_ID:
      Number        = (UINT16) LegacyBEVMenu.MenuNumber;
      OldLegacyDev  = Private->BmmOldFakeNVData.LegacyBEV;
      NewLegacyDev  = CurrentFakeNVMap->LegacyBEV;
      break;

    default:
      break;
    }
    //
    //  First, find the different position
    //  if there is change, it should be only one
    //
    for (Index = 0; Index < Number; Index++) {
      if (OldLegacyDev[Index] != NewLegacyDev[Index]) {
        OldValue  = OldLegacyDev[Index];
        NewValue  = NewLegacyDev[Index];
        break;
      }
    }

      if (Index != Number) {
        //
        // there is change, now process
        //
        if (0xFF == NewValue) {
          //
          // This item will be disable
          // Just move the items behind this forward to overlap it
          //
          Pos = OldValue / 8;
          Bit = 7 - (OldValue % 8);
          DisMap[Pos] = (UINT8) (DisMap[Pos] | (UINT8) (1 << Bit));
          for (Index2 = Index; Index2 < Number - 1; Index2++) {
            NewLegacyDev[Index2] = NewLegacyDev[Index2 + 1];
          }

          NewLegacyDev[Index2] = 0xFF;
        } else {
          for (Index2 = 0; Index2 < Number; Index2++) {
            if (Index2 == Index) {
              continue;
            }

            if (OldLegacyDev[Index2] == NewValue) {
              //
              // If NewValue is in OldLegacyDev array
              // remember its old position
              //
              NewValuePos = Index2;
              break;
            }
          }

          if (Index2 != Number) {
            //
            // We will change current item to an existing item
            // (It's hard to describe here, please read code, it's like a cycle-moving)
            //
            for (Index2 = NewValuePos; Index2 != Index;) {
              if (NewValuePos < Index) {
                NewLegacyDev[Index2] = OldLegacyDev[Index2 + 1];
                Index2++;
              } else {
                NewLegacyDev[Index2] = OldLegacyDev[Index2 - 1];
                Index2--;
              }
            }
          } else {
            //
            // If NewValue is not in OldlegacyDev array, we are changing to a disabled item
            // so we should modify DisMap to reflect the change
            //
            Pos = NewValue / 8;
            Bit = 7 - (NewValue % 8);
            DisMap[Pos] = (UINT8) (DisMap[Pos] & (~ (UINT8) (1 << Bit)));
            if (0xFF != OldValue) {
              //
              // Because NewValue is a item that was disabled before
              // so after changing the OldValue should be disabled
              // actually we are doing a swap of enable-disable states of two items
              //
              Pos = OldValue / 8;
              Bit = 7 - (OldValue % 8);
              DisMap[Pos] = (UINT8) (DisMap[Pos] | (UINT8) (1 << Bit));
            }
          }
        }
        //
        // To prevent DISABLE appears in the middle of the list
        // we should perform a re-ordering
        //
        Index3 = Index;
        Index = 0;
        while (Index < Number) {
          if (0xFF != NewLegacyDev[Index]) {
            Index++;
            continue;
          }

          Index2 = Index;
          Index2++;
          while (Index2 < Number) {
            if (0xFF != NewLegacyDev[Index2]) {
              break;
            }

            Index2++;
          }

          if (Index2 < Number) {
            NewLegacyDev[Index]   = NewLegacyDev[Index2];
            NewLegacyDev[Index2]  = 0xFF;
          }

          Index++;
        }

        CopyMem (
          OldLegacyDev,
          NewLegacyDev,
          Number
          );
        LegacyDevOrderChange = TRUE;

        //
        // Return correct question value.
        //
        Value->u8 = NewLegacyDev[Index3];
      }
	  
      if(LegacyDevOrderChange){
        Status = ApplyChangeHandler (Private, CurrentFakeNVMap, FormId);
        if (EFI_ERROR (Status)) {
          return Status;
        }
        ByoCleanUpPage(FORM_BOOT_CHG_ID,Private);

        ByoUpdateOrderPage();
      }
    }



    switch (QuestionId) {

      case FORM_SET_FD_ORDER_ID:
      case FORM_SET_HD_ORDER_ID:
      case FORM_SET_CD_ORDER_ID:
      case FORM_SET_NET_ORDER_ID:
      case FORM_SET_BEV_ORDER_ID:
        ByoCleanUpPage (QuestionId, Private);
        UpdateSetLegacyDeviceOrderPage (QuestionId, Private);
        break;

      case OPTION_ORDER_QUESTION_ID:
        Status = Var_UpdateBootOrder(Private);
        break;

      default:
        break;
    }

  //
  // Pass changed uncommitted data back to Form Browser
  //
  Status = HiiSetBrowserData (&mBootMaintGuid, mBootMaintStorageName, sizeof (BMM_FAKE_NV_DATA), (UINT8 *) CurrentFakeNVMap, NULL);

EXIT:
  return Status;
}

/**
  Function handling request to apply changes for BMM pages.

  @param Private            Pointer to callback data buffer.
  @param CurrentFakeNVMap   Pointer to buffer holding data of various values used by BMM
  @param FormId             ID of the form which has sent the request to apply change.

  @retval  EFI_SUCCESS       Change successfully applied.
  @retval  Other             Error occurs while trying to apply changes.

**/
EFI_STATUS
ApplyChangeHandler (
  IN  BMM_CALLBACK_DATA               *Private,
  IN  BMM_FAKE_NV_DATA                *CurrentFakeNVMap,
  IN  EFI_FORM_ID                     FormId
  )
{
  EFI_STATUS          Status;

  Status = EFI_SUCCESS;

  switch (FormId) {
  case FORM_SET_FD_ORDER_ID:
  case FORM_SET_HD_ORDER_ID:
  case FORM_SET_CD_ORDER_ID:
  case FORM_SET_NET_ORDER_ID:
  case FORM_SET_BEV_ORDER_ID:
    Var_UpdateBBSOption (Private);
    break;

  case FORM_BOOT_CHG_ID:
    Status = Var_UpdateBootOrder (Private);
    break;

  default:
    break;
  }

  return Status;
}




/**
  Initialize the Boot Maintenance Utitliy.


  @retval  EFI_SUCCESS      utility ended successfully
  @retval  others           contain some errors

**/
EFI_STATUS
InitializeBM (
  VOID
  )
{
  EFI_LEGACY_BIOS_PROTOCOL    *LegacyBios;
  EFI_STATUS                  Status = EFI_SUCCESS;


  BmmCallbackInfo = AllocateZeroPool (sizeof (BMM_CALLBACK_DATA));
  if (BmmCallbackInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  BmmCallbackInfo->Root = AllocateZeroPool(sizeof(BM_MENU_ENTRY));
  if (BmmCallbackInfo->Root == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  BmmCallbackInfo->MenuEntry = (BM_MENU_ENTRY*)BmmCallbackInfo->Root;
  
  BmmCallbackInfo->Signature                     = BMM_CALLBACK_DATA_SIGNATURE;
  BmmCallbackInfo->BmmConfigAccess.ExtractConfig = BootMaintExtractConfig;
  BmmCallbackInfo->BmmConfigAccess.RouteConfig   = BootMaintRouteConfig;
  BmmCallbackInfo->BmmConfigAccess.Callback      = BootMaintCallback;
  BmmCallbackInfo->BmmSaveNotify.SaveValue       = BootConfigureSaveValue;
  BmmCallbackInfo->BmmSaveNotify.DiscardValue    = BootConfigureDiscardValue;
  BmmCallbackInfo->BmmSaveNotify.LoadDefault     = BootConfigureLoadDefault;
  BmmCallbackInfo->BmmSaveNotify.SaveUserDefault = BootConfigureSaveUserDefault;
  BmmCallbackInfo->BmmSaveNotify.LoadUserDefault = BootConfigureLoadUserDefault;
  BmmCallbackInfo->BmmSaveNotify.IsSetupDataChanged = IsBootOptionChanged;
  BmmCallbackInfo->BmmPreviousPageId             = FORM_MAIN_ID;
  BmmCallbackInfo->BmmCurrentPageId              = FORM_MAIN_ID;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &BmmCallbackInfo->BmmDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mBmmHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &BmmCallbackInfo->BmmConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ProcExit;    
  }

  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &BmmCallbackInfo->BmmSaveNotifyHandle,
                  &gSetupSaveNotifyProtocolGuid,
                  &BmmCallbackInfo->BmmSaveNotify,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ProcExit;   
  }

  BmmCallbackInfo->BmmHiiHandle = HiiAddPackages (
                                    &mBootMaintGuid,
                                    BmmCallbackInfo->BmmDriverHandle,
                                    BmBin,
                                    UiAppStrings,
                                    NULL
                                    );
  ASSERT (BmmCallbackInfo->BmmHiiHandle != NULL);


// Init OpCode Handle and Allocate space for creation of Buffer
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (mStartOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;   
  }

  mEndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (mEndOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit; 
  }

  mStartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (mStartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

  mEndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (mEndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  mEndLabel->Number       = LABEL_END;

  InitializeStringDepository ();

  InitAllMenu (BmmCallbackInfo);

  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &BootOptionMenu);

  RefreshUpdateData ();
  mStartLabel->Number = FORM_BOOT_LEGACY_DEVICE_ID;
  Status = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (!EFI_ERROR (Status)) {
    //
    // If LegacyBios Protocol is installed, add 3 tags about legacy boot option
    // in BootOption form: legacy FD/HD/CD/NET/BEV
    //
    if(LegacyFDMenu.MenuNumber) {
      HiiCreateGotoOpCode (
        mStartOpCodeHandle,
        FORM_SET_FD_ORDER_ID,
        STRING_TOKEN (STR_FORM_SET_FD_ORDER_TITLE),
        STRING_TOKEN (STR_FORM_SET_FD_ORDER_TITLE),
        EFI_IFR_FLAG_CALLBACK,
        FORM_SET_FD_ORDER_ID
        );
    }
    if(LegacyHDMenu.MenuNumber) {
      HiiCreateGotoOpCode (
        mStartOpCodeHandle,
        FORM_SET_HD_ORDER_ID,
        STRING_TOKEN (STR_FORM_SET_HD_ORDER_TITLE),
        STRING_TOKEN (STR_FORM_SET_HD_ORDER_TITLE),
        EFI_IFR_FLAG_CALLBACK,
        FORM_SET_HD_ORDER_ID
        );
    }
    if(LegacyCDMenu.MenuNumber) {
      HiiCreateGotoOpCode (
        mStartOpCodeHandle,
        FORM_SET_CD_ORDER_ID,
        STRING_TOKEN (STR_FORM_SET_CD_ORDER_TITLE),
        STRING_TOKEN (STR_FORM_SET_CD_ORDER_TITLE),
        EFI_IFR_FLAG_CALLBACK,
        FORM_SET_CD_ORDER_ID
        );
    }
    if(LegacyNETMenu.MenuNumber) {
      HiiCreateGotoOpCode (
        mStartOpCodeHandle,
        FORM_SET_NET_ORDER_ID,
        STRING_TOKEN (STR_FORM_SET_NET_ORDER_TITLE),
        STRING_TOKEN (STR_FORM_SET_NET_ORDER_TITLE),
        EFI_IFR_FLAG_CALLBACK,
        FORM_SET_NET_ORDER_ID
        );
    }
    if(LegacyBEVMenu.MenuNumber) {
      HiiCreateGotoOpCode (
        mStartOpCodeHandle,
        FORM_SET_BEV_ORDER_ID,
        STRING_TOKEN (STR_FORM_SET_BEV_ORDER_TITLE),
        STRING_TOKEN (STR_FORM_SET_BEV_ORDER_TITLE),
        EFI_IFR_FLAG_CALLBACK,
        FORM_SET_BEV_ORDER_ID
        );
    }
  }

  HiiUpdateForm (
    BmmCallbackInfo->BmmHiiHandle,
    &mBootMaintGuid,
    FORM_MAIN_ID,
    mStartOpCodeHandle, // Label FORM_BOOT_LEGACY_DEVICE_ID
    mEndOpCodeHandle    // LABEL_END
    );
  ByoCleanUpPage(FORM_BOOT_CHG_ID, BmmCallbackInfo);
  ByoUpdateOrderPage();


ProcExit:
  if(EFI_ERROR(Status)){
    if (BmmCallbackInfo != NULL) {
      if(BmmCallbackInfo->Root != NULL){
        FreePool(BmmCallbackInfo->Root);
      }
      FreePool(BmmCallbackInfo);
      BmmCallbackInfo = NULL;
    }  
  }  
  return Status;
}

/**
  Initialized all Menu Option List.

  @param CallbackData    The BMM context data.

**/
VOID
InitAllMenu (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  InitializeListHead (&BootOptionMenu.Head);
  BOpt_GetBootOptions (CallbackData);
  BOpt_GetLegacyOptions ();
}

/**
  Free up all Menu Option list.

**/
VOID
FreeAllMenu (
  VOID
  )
{
  BOpt_FreeMenu (&BootOptionMenu);
  BOpt_FreeLegacyOptions ();
}

/**
  Initialize all the string depositories.

**/
VOID
InitializeStringDepository (
  VOID
  )
{
  STRING_DEPOSITORY *StringDepository;
  
  StringDepository              = AllocateZeroPool (sizeof (STRING_DEPOSITORY) * STRING_DEPOSITORY_NUMBER);
  FirstStrDepository            = StringDepository;
  BootOptionStrDepository       = StringDepository++;
  BootOptionHelpStrDepository   = StringDepository++;
}

/**
  Fetch a usable string node from the string depository and return the string token.

  @param CallbackData       The BMM context data.
  @param StringDepository   The string repository.

  @retval  EFI_STRING_ID           String token.

**/
EFI_STRING_ID
GetStringTokenFromDepository (
  IN   BMM_CALLBACK_DATA     *CallbackData,
  IN   STRING_DEPOSITORY     *StringDepository
  )
{
  STRING_LIST_NODE  *CurrentListNode;
  STRING_LIST_NODE  *NextListNode;

  CurrentListNode = StringDepository->CurrentNode;

  if ((NULL != CurrentListNode) && (NULL != CurrentListNode->Next)) {
    //
    // Fetch one reclaimed node from the list.
    //
    NextListNode = StringDepository->CurrentNode->Next;
  } else {
    //
    // If there is no usable node in the list, update the list.
    //
    NextListNode = AllocateZeroPool (sizeof (STRING_LIST_NODE));
    ASSERT (NextListNode != NULL);
    NextListNode->StringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, L" ", NULL);
    ASSERT (NextListNode->StringToken != 0);

    StringDepository->TotalNodeNumber++;

    if (NULL == CurrentListNode) {
      StringDepository->ListHead = NextListNode;
    } else {
      CurrentListNode->Next = NextListNode;
    }
  }

  StringDepository->CurrentNode = NextListNode;

  return StringDepository->CurrentNode->StringToken;
}

/**
  Reclaim string depositories by moving the current node pointer to list head..

**/
VOID
ReclaimStringDepository (
  VOID
  )
{
  UINTN             DepositoryIndex;
  STRING_DEPOSITORY *StringDepository;

  StringDepository = FirstStrDepository;
  for (DepositoryIndex = 0; DepositoryIndex < STRING_DEPOSITORY_NUMBER; DepositoryIndex++) {
    StringDepository->CurrentNode = StringDepository->ListHead;
    StringDepository++;
  }
}

/**
  Release resource for all the string depositories.

**/
VOID
CleanUpStringDepository (
  VOID
  )
{
  UINTN             NodeIndex;
  UINTN             DepositoryIndex;
  STRING_LIST_NODE  *CurrentListNode;
  STRING_LIST_NODE  *NextListNode;
  STRING_DEPOSITORY *StringDepository;

  //
  // Release string list nodes.
  //
  StringDepository = FirstStrDepository;
  for (DepositoryIndex = 0; DepositoryIndex < STRING_DEPOSITORY_NUMBER; DepositoryIndex++) {
    CurrentListNode = StringDepository->ListHead;
    for (NodeIndex = 0; NodeIndex < StringDepository->TotalNodeNumber; NodeIndex++) {
      NextListNode = CurrentListNode->Next;
      FreePool (CurrentListNode);
      CurrentListNode = NextListNode;
    }

    StringDepository++;
  }
  //
  // Release string depository.
  //
  FreePool (FirstStrDepository);
}





VOID InitTempBootOrder()
{
  EFI_STATUS      Status;
  UINT16          *BootOrderList;
  UINTN           BootOrderListSize;
  UINT8           *VarData;
  UINTN           VarSize;
  CHAR16          VarName[100];
  CHAR16          TempVarName[100];
  UINTN           OptionSize;
  VOID            *BootOptionVar;
  UINT16          OrderIndex;

  BootOrderList     = NULL;
  BootOrderListSize = 0;
  VarData           = NULL;
  VarSize           = 0;
  BootOrderList = EfiBootManagerGetVariableAndSize (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    &BootOrderListSize
                    );
  if (BootOrderList == NULL) {
    DEBUG((EFI_D_ERROR, "BootOrder Not Present\n"));
  }
  Status = gRT->SetVariable (
                  L"BootOrderTemp",
                  &gByoGlobalVariableGuid,
                  TEMP_VAR_FLAG,
                  BootOrderListSize,
                  BootOrderList
                  );
  
  VarData = (UINT8 *) EfiBootManagerGetVariableAndSize (
                        VAR_LEGACY_DEV_ORDER,
                        &gEfiLegacyDevOrderVariableGuid,
                        &VarSize
                        );
  if (VarData == NULL) {
    DEBUG((EFI_D_ERROR, "Legacydevorder Not Present\n"));
  }
  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER_TEMP,
                  &gEfiLegacyDevOrderVariableGuid,
                  TEMP_VAR_FLAG,
                  VarSize,
                  VarData
                  );
  SafeFreePool (VarData);
  
  for(OrderIndex=0; OrderIndex < BootOrderListSize/sizeof(UINT16); OrderIndex++) {
    UnicodeSPrint (VarName, sizeof (VarName), L"Boot%04x", BootOrderList[OrderIndex]);
    BootOptionVar = EfiBootManagerGetVariableAndSize (
                      VarName,
                      &gEfiGlobalVariableGuid,
                      &OptionSize
                      );
    UnicodeSPrint (TempVarName, sizeof (TempVarName), L"BootTemp%04x", BootOrderList[OrderIndex]);
    Status = gRT->SetVariable (
                    TempVarName,
                    &gByoGlobalVariableGuid,
                    TEMP_VAR_FLAG,
                    OptionSize,
                    BootOptionVar
                    );
    SafeFreePool (BootOptionVar);
  }
  SafeFreePool (BootOrderList);
}






/**
  Start boot maintenance manager

  @retval EFI_SUCCESS If BMM is invoked successfully.
  @return Other value if BMM return unsuccessfully.

**/
EFI_STATUS
BdsStartBootMaint (
  VOID
  )
{
  EFI_STATUS  Status;

  DEBUG((EFI_D_INFO, __FUNCTION__"\n"));

  EfiBootManagerConnectAll();
  EfiBootManagerRefreshAllBootOption();
  GroupMultipleLegacyBootOption4SameType();
  
  InitTempBootOrder();

  Status = InitializeBM ();

  return Status;
}

/**
  Dispatch BMM formset and FileExplorer formset.


  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS If function complete successfully.
  @return Other value if the Setup Browser process BMM's pages and
           return unsuccessfully.

**/
EFI_STATUS
FormSetDispatcher (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  EFI_STATUS                 Status;
  EFI_BROWSER_ACTION_REQUEST ActionRequest;


  UpdatePageId (CallbackData, FORM_MAIN_ID);

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                           gFormBrowser2,
                           &CallbackData->BmmHiiHandle,
                           1,
                           &mBootMaintGuid,
                           0,
                           NULL,
                           &ActionRequest
                           );
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
  }

  ReclaimStringDepository ();

  return Status;
}



EFI_STATUS
BootConfigureSaveValue(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{ 
  EFI_STATUS           Status;
  UINT16               *BootOrderList;
  UINTN                BootOrderListSize;
  UINT16               *BootOrderTempList;
  UINTN                BootOrderTempListSize;
  UINT8                *VarData;
  UINTN                VarSize;
  UINT8                *VarDataTemp;
  UINTN                VarTempSize;
  CHAR16               VarName[100];
  CHAR16               TempVarName[100];
  UINTN                OptionSize;
  VOID                 *BootOptionVar;
  UINTN                TempOptionSize;
  VOID                 *TempBootOptionVar;
  UINT16               OrderIndex;
  BootOrderList          = NULL;
  BootOrderListSize      = 0;
  BootOrderTempList      = NULL;
  BootOrderTempListSize  = 0;
  VarData                = NULL;
  VarSize                = 0;
  VarDataTemp            = NULL;
  VarTempSize            = 0;
  BootOrderList = EfiBootManagerGetVariableAndSize (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    &BootOrderListSize
                    );
  BootOrderTempList = EfiBootManagerGetVariableAndSize (
                    L"BootOrderTemp",
                    &gByoGlobalVariableGuid,
                    &BootOrderTempListSize
                    );
  if((0 != CompareMem(BootOrderTempList, BootOrderList, BootOrderTempListSize)) ||
    (BootOrderListSize != BootOrderTempListSize)) {
    Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BootOrderTempListSize,
                  BootOrderTempList
                  );
  }
  VarData = (UINT8 *) EfiBootManagerGetVariableAndSize (
                        VAR_LEGACY_DEV_ORDER,
                        &gEfiLegacyDevOrderVariableGuid,
                        &VarSize
                        );
  VarDataTemp = (UINT8 *) EfiBootManagerGetVariableAndSize (
                        VAR_LEGACY_DEV_ORDER_TEMP,
                        &gEfiLegacyDevOrderVariableGuid,
                        &VarTempSize
                        );
  if((0 != CompareMem(VarDataTemp, VarData, VarTempSize)) ||
    (VarTempSize != VarSize)) {
    Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &gEfiLegacyDevOrderVariableGuid,
                  VAR_FLAG,
                  VarTempSize,
                  VarDataTemp
                  );
  }
  for(OrderIndex=0; OrderIndex < BootOrderTempListSize/sizeof(UINT16); OrderIndex++) {
    UnicodeSPrint (VarName, sizeof (VarName), L"Boot%04x", BootOrderTempList[OrderIndex]);
    BootOptionVar = EfiBootManagerGetVariableAndSize (
                      VarName,
                      &gEfiGlobalVariableGuid,
                      &OptionSize
                      );
    UnicodeSPrint (TempVarName, sizeof (TempVarName), L"BootTemp%04x", BootOrderTempList[OrderIndex]);
    TempBootOptionVar = EfiBootManagerGetVariableAndSize (
                      TempVarName,
                      &gByoGlobalVariableGuid,
                      &TempOptionSize
                      );
    if((0 != CompareMem(TempBootOptionVar, BootOptionVar, TempOptionSize)) ||
      (OptionSize != TempOptionSize)) {
      Status = gRT->SetVariable (
                      VarName,
                      &gEfiGlobalVariableGuid,
                      VAR_FLAG,
                      TempOptionSize,
                      TempBootOptionVar
                      );
    }    
    SafeFreePool (BootOptionVar);
    SafeFreePool (TempBootOptionVar);
  }
  SafeFreePool (BootOrderList);
  SafeFreePool (BootOrderTempList);
  SafeFreePool (VarData);
  SafeFreePool (VarDataTemp);
  return EFI_SUCCESS;
}

EFI_STATUS
BootConfigureDiscardValue(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
BootConfigureLoadDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
BootConfigureSaveUserDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
BootConfigureLoadUserDefault(
  IN SETUP_SAVE_NOTIFY_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS 
IsBootOptionChanged(
  IN  SETUP_SAVE_NOTIFY_PROTOCOL *This,
  OUT BOOLEAN                    *IsDataChanged
  )
{
  *IsDataChanged = FALSE;
  return EFI_SUCCESS;
}

VOID
SafeFreePool (
  IN VOID    *Buffer
  )
{
  if (Buffer != NULL) {
    gBS->FreePool (Buffer);
  }
}


VOID
ByoUpdateOrderPage (
  VOID
  )
{ 
  EFI_STATUS      Status;
  BM_MENU_ENTRY   *NewMenuEntry;
  UINT16          Index;
  UINT16          OptionOrderIndex;
  VOID            *OptionsOpCodeHandle;
  UINTN           DeviceType;
  BM_LOAD_CONTEXT *NewLoadContext;

  DeviceType                    = (UINTN) -1;
  RefreshUpdateData ();
  mStartLabel->Number = FORM_BOOT_CHG_ID;
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &BootOptionMenu);
  
  ZeroMem (BmmCallbackInfo->BmmFakeNvData.OptionOrder, sizeof (BmmCallbackInfo->BmmFakeNvData.OptionOrder));
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (
        Index = 0, OptionOrderIndex = 0;
        (
          (Index < BootOptionMenu.MenuNumber) &&
          (OptionOrderIndex <
            (
              sizeof (BmmCallbackInfo->BmmFakeNvData.OptionOrder) /
              sizeof (BmmCallbackInfo->BmmFakeNvData.OptionOrder[0])
            )
          )
        );
        Index++
      ) {
    NewMenuEntry   = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    NewLoadContext = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    if (NewLoadContext->IsLegacy) {
      if (((BBS_BBS_DEVICE_PATH *) NewLoadContext->FilePathList)->DeviceType != DeviceType) {
        DeviceType = ((BBS_BBS_DEVICE_PATH *) NewLoadContext->FilePathList)->DeviceType;
      } else {
        continue;
      }
    }
    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      NewMenuEntry->DisplayStringToken,
      0,
      EFI_IFR_TYPE_NUM_SIZE_32,
      (UINT32) (NewMenuEntry->OptionNumber + 1)
      );
    BmmCallbackInfo->BmmFakeNvData.OptionOrder[OptionOrderIndex++] = (UINT32) (NewMenuEntry->OptionNumber + 1);
    DEBUG((EFI_D_INFO, "OptionOrder[%d] is 0x%x.\n", OptionOrderIndex-1, (UINT32) (NewMenuEntry->OptionNumber + 1)));
  }
  if (BootOptionMenu.MenuNumber > 0) {
    HiiCreateOrderedListOpCode (                   
      mStartOpCodeHandle,                          // Container for dynamic created opcodes     
      (EFI_QUESTION_ID) OPTION_ORDER_QUESTION_ID,  // Question ID                               
      VARSTORE_ID_BOOT_MAINT,                      // VarStore ID                               
      OPTION_ORDER_VAR_OFFSET,                     // Offset in Buffer Storage                  
      STRING_TOKEN (STR_CHANGE_ORDER),             // Question prompt text                      
      STRING_TOKEN (STR_CHANGE_ORDER_HELP),        // Question help text                        
      EFI_IFR_FLAG_CALLBACK,                                           // Question flag                             
      0,                                           // Ordered list flag, e.g. EFI_IFR_UNIQUE_SET
      EFI_IFR_TYPE_NUM_SIZE_32,                    // Data type of Question value               
      100,                                         // Maximum container                         
      OptionsOpCodeHandle,                         // Option Opcode list                        
      NULL                                         // Default Opcode is NULL                    
      );
  }
  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  Status = HiiUpdateForm (
    BmmCallbackInfo->BmmHiiHandle,
    &mBootMaintGuid,
    FORM_MAIN_ID,
    mStartOpCodeHandle, // Label CallbackData->BmmCurrentPageId
    mEndOpCodeHandle    // LABEL_END
    );
  CopyMem (
    BmmCallbackInfo->BmmOldFakeNVData.OptionOrder,
    BmmCallbackInfo->BmmFakeNvData.OptionOrder,
    sizeof (BmmCallbackInfo->BmmOldFakeNVData.OptionOrder)
    );

}


VOID
ByoCleanUpPage (
  IN UINT16                           LabelId,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  RefreshUpdateData();
  mStartLabel->Number = LabelId;
  HiiUpdateForm (
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    FORM_MAIN_ID,
    mStartOpCodeHandle, // Label LabelId
    mEndOpCodeHandle    // LABEL_END
    );
}


/**
  Remvoe the intalled BootMaint and FileExplorer HiiPackages.

**/
VOID
ByoFreeBMPackage (
  VOID
  )
{
  BMM_CALLBACK_DATA           *TempBmmCallbackInfo;

  TempBmmCallbackInfo = BmmCallbackInfo;
  CleanUpStringDepository ();
  FreeAllMenu ();

  //
  // Remove our IFR data from HII database
  //
  HiiRemovePackages (TempBmmCallbackInfo->BmmHiiHandle);

  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  if (mEndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mEndOpCodeHandle);
  }

  if (TempBmmCallbackInfo->BmmSaveNotifyHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           TempBmmCallbackInfo->BmmSaveNotifyHandle,
           &gSetupSaveNotifyProtocolGuid,
           &BmmCallbackInfo->BmmSaveNotify,
           NULL
           );
  }

  if (TempBmmCallbackInfo->BmmDriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           TempBmmCallbackInfo->BmmDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mBmmHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &TempBmmCallbackInfo->BmmConfigAccess,
           NULL
           );
  }

  FreePool (TempBmmCallbackInfo->Root);
  FreePool (TempBmmCallbackInfo);

  BmmCallbackInfo = NULL; 
  
  return;  
}
