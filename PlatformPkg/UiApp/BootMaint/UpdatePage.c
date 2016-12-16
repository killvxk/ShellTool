/*++

Copyright (c) 2006-2012, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Description:

Dynamically update the pages.

**/

#include "BootMaint.h"

/**
  Refresh the global UpdateData structure.

**/
VOID
RefreshUpdateData (
  VOID
  )
{
  //
  // Free current updated date
  //  
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  //
  // Create new OpCode Handle
  //
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mStartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (mStartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

}

/**
  Add a "Go back to main page" tag in front of the form when there are no
  "Apply changes" and "Discard changes" tags in the end of the form.
 
  @param CallbackData    The BMM context data.

**/
VOID
UpdatePageStart (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  RefreshUpdateData ();
  mStartLabel->Number = CallbackData->BmmCurrentPageId;
}

/**
  Create the "Apply changes" and "Discard changes" tags. And
  ensure user can return to the main page.

  @param CallbackData    The BMM context data.

**/
VOID
UpdatePageEnd (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{

  HiiUpdateForm (
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    CallbackData->BmmCurrentPageId,
    mStartOpCodeHandle, // Label CallbackData->BmmCurrentPageId
    mEndOpCodeHandle    // LABEL_END
    );
}








/**
  Update the page's NV Map if user has changed the order
  a list. This list can be Boot Order or Driver Order.

  @param UpdatePageId    The form ID to be updated.
  @param OptionMenu      The new list.
  @param CallbackData    The BMM context data.

**/
VOID
UpdateOrderPage (
  IN UINT16                           UpdatePageId,
  IN BM_MENU_OPTION                   *OptionMenu,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  UINT16          Index;
  UINT16          OptionOrderIndex;
  VOID            *OptionsOpCodeHandle;
  UINTN           DeviceType;
  BM_LOAD_CONTEXT *NewLoadContext;

  DeviceType                    = (UINTN) -1;

  UpdatePageStart (CallbackData);

  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, OptionMenu);

  ZeroMem (CallbackData->BmmFakeNvData.OptionOrder, sizeof (CallbackData->BmmFakeNvData.OptionOrder));

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);
  
  for (
        Index = 0, OptionOrderIndex = 0;
        (
          (Index < OptionMenu->MenuNumber) &&
          (OptionOrderIndex <
            (
              sizeof (CallbackData->BmmFakeNvData.OptionOrder) /
              sizeof (CallbackData->BmmFakeNvData.OptionOrder[0])
            )
          )
        );
        Index++
      ) {
    NewMenuEntry   = BOpt_GetMenuEntry (OptionMenu, Index);
    NewLoadContext = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;

    if (!NewLoadContext->IsActive) {
      continue;
    }
    if (NewLoadContext->IsLegacy) {
      if (((BBS_BBS_DEVICE_PATH *) NewLoadContext->FilePathList)->DeviceType != DeviceType) {
        DeviceType = ((BBS_BBS_DEVICE_PATH *) NewLoadContext->FilePathList)->DeviceType;
      } else {
        //
        // Only show one legacy boot option for the same device type
        // assuming the boot options are grouped by the device type
        //
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
    CallbackData->BmmFakeNvData.OptionOrder[OptionOrderIndex++] = (UINT32) (NewMenuEntry->OptionNumber + 1);
  }

  if (OptionMenu->MenuNumber > 0) {
    HiiCreateOrderedListOpCode (                   
      mStartOpCodeHandle,                          // Container for dynamic created opcodes     
      (EFI_QUESTION_ID) OPTION_ORDER_QUESTION_ID,  // Question ID                               
      VARSTORE_ID_BOOT_MAINT,                      // VarStore ID                               
      OPTION_ORDER_VAR_OFFSET,                     // Offset in Buffer Storage                  
      STRING_TOKEN (STR_CHANGE_ORDER),             // Question prompt text                      
      STRING_TOKEN (STR_CHANGE_ORDER),             // Question help text                        
      0,                                           // Question flag                             
      0,                                           // Ordered list flag, e.g. EFI_IFR_UNIQUE_SET
      EFI_IFR_TYPE_NUM_SIZE_32,                    // Data type of Question value               
      100,                                         // Maximum container                         
      OptionsOpCodeHandle,                         // Option Opcode list                        
      NULL                                         // Default Opcode is NULL                    
      );
  }

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  UpdatePageEnd (CallbackData);

  CopyMem (
    CallbackData->BmmOldFakeNVData.OptionOrder,
    CallbackData->BmmFakeNvData.OptionOrder,
    sizeof (CallbackData->BmmOldFakeNVData.OptionOrder)
    );
}





/**
  Get the index number (#### in Boot####) for the boot option pointed to a BBS legacy device type
  specified by DeviceType.

  @param DeviceType      The legacy device type. It can be floppy, network, harddisk, cdrom,
                         etc.
  @param OptionIndex     Returns the index number (#### in Boot####).
  @param OptionSize      Return the size of the Boot### variable.

**/
VOID *
GetLegacyBootOptionVar (
  IN  UINTN                            DeviceType,
  OUT UINTN                            *OptionIndex,
  OUT UINTN                            *OptionSize
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  VOID                      *OptionBuffer;
  UINTN                     OrderSize;
  UINTN                     Index;
  UINT16                    *OrderBuffer;
  CHAR16                    StrTemp[100];
  UINT16                    FilePathSize;
  UINT8                     *Ptr;
  UINT8                     *OptionalData;

  //
  // Get Boot Option number from the size of BootOrder
  //
  OrderBuffer = EfiBootManagerGetVariableAndSize (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  &OrderSize
                  );
                  
  if (OrderBuffer == NULL) {
    return NULL;
  }
  
  for (Index = 0; Index < OrderSize / sizeof (UINT16); Index++) {
    UnicodeSPrint (StrTemp, 100, L"Boot%04x", OrderBuffer[Index]);
    OptionBuffer = EfiBootManagerGetVariableAndSize (
                    StrTemp,
                    &gEfiGlobalVariableGuid,
                    OptionSize
                    );
    if (NULL == OptionBuffer) {
      continue;
    }

    Ptr       = (UINT8 *) OptionBuffer;
    Ptr += sizeof (UINT32);

    FilePathSize = *(UINT16 *) Ptr;
    Ptr += sizeof (UINT16);

    Ptr += StrSize ((CHAR16 *) Ptr);

    //
    // Now Ptr point to Device Path
    //
    DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
    Ptr += FilePathSize;

    //
    // Now Ptr point to Optional Data
    //
    OptionalData = Ptr;

    if ((DeviceType == ((BBS_TABLE *) OptionalData)->DeviceType) &&
        (BBS_DEVICE_PATH == DevicePath->Type) &&
        (BBS_BBS_DP == DevicePath->SubType)
        ) {
      *OptionIndex = OrderBuffer[Index];
      FreePool (OrderBuffer);
      return OptionBuffer;
    } else {
      FreePool (OptionBuffer);
    }
  }

  FreePool (OrderBuffer);
  return NULL;
}

/**
  Create a dynamic page so that Legacy Device boot order
  can be set for specified device type.

  @param UpdatePageId    The form ID. It also spefies the legacy device type.
  @param CallbackData    The BMM context data.


**/
VOID
UpdateSetLegacyDeviceOrderPage (
  IN UINT16                           UpdatePageId,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  LEGACY_DEV_ORDER_ENTRY      *DevOrder;
  BM_MENU_OPTION              *OptionMenu;
  BM_MENU_ENTRY               *NewMenuEntry;
  EFI_STRING_ID               StrRef;
  EFI_STRING_ID               StrRefHelp;
  BBS_TYPE                    BbsType;
  UINTN                       VarSize;
  UINTN                       Pos;
  UINTN                       Bit;
  UINT16                      Index;
  UINT16                      Key;
  CHAR16                      String[100];
  CHAR16                      *TypeStr;
  CHAR16                      *TypeStrHelp;
  UINT16                      VarDevOrder;
  UINT8                       *VarData;
  UINT8                       *LegacyOrder;
  UINT8                       *OldData;
  UINT8                       *DisMap;
  VOID                        *OptionsOpCodeHandle;

  OptionMenu = NULL;
  Key = 0;
  StrRef = 0;
  StrRefHelp = 0;
  TypeStr = NULL;
  TypeStrHelp = NULL;
  BbsType = BBS_FLOPPY;
  LegacyOrder = NULL;
  OldData = NULL;
  DisMap = NULL;

  UpdatePageStart (CallbackData);

  DisMap = ZeroMem (CallbackData->BmmOldFakeNVData.DisableMap, sizeof (CallbackData->BmmOldFakeNVData.DisableMap));

  //
  // Create oneof option list
  //
  switch (UpdatePageId) {
  case FORM_SET_FD_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyFDMenu;
    Key         = (UINT16) LEGACY_FD_QUESTION_ID;
    TypeStr     = STR_FLOPPY;
    TypeStrHelp = STR_FLOPPY_HELP;
    BbsType     = BBS_FLOPPY;
    LegacyOrder = CallbackData->BmmFakeNvData.LegacyFD;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyFD;
    break;

  case FORM_SET_HD_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyHDMenu;
    Key         = (UINT16) LEGACY_HD_QUESTION_ID;
    TypeStr     = STR_HARDDISK;
    TypeStrHelp = STR_HARDDISK_HELP;
    BbsType     = BBS_HARDDISK;
    LegacyOrder = CallbackData->BmmFakeNvData.LegacyHD;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyHD;
    break;

  case FORM_SET_CD_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyCDMenu;
    Key         = (UINT16) LEGACY_CD_QUESTION_ID;
    TypeStr     = STR_CDROM;
    TypeStrHelp = STR_CDROM_HELP;
    BbsType     = BBS_CDROM;
    LegacyOrder = CallbackData->BmmFakeNvData.LegacyCD;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyCD;
    break;

  case FORM_SET_NET_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyNETMenu;
    Key         = (UINT16) LEGACY_NET_QUESTION_ID;
    TypeStr     = STR_NET;
    TypeStrHelp = STR_NET_HELP;
    BbsType     = BBS_EMBED_NETWORK;
    LegacyOrder = CallbackData->BmmFakeNvData.LegacyNET;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyNET;
    break;

  case FORM_SET_BEV_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyBEVMenu;
    Key         = (UINT16) LEGACY_BEV_QUESTION_ID;
    TypeStr     = STR_BEV;
    TypeStrHelp = STR_BEV_HELP;
    BbsType     = BBS_BEV_DEVICE;
    LegacyOrder = CallbackData->BmmFakeNvData.LegacyBEV;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyBEV;
    break;

  default:
    DEBUG ((EFI_D_ERROR, "Invalid command ID for updating page!\n"));
    return;
  }

  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, OptionMenu);

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
    NewMenuEntry                = BOpt_GetMenuEntry (OptionMenu, Index);
    //
    // Create OneOf for each legacy device
    //
    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      NewMenuEntry->DisplayStringToken,
      0,
      EFI_IFR_TYPE_NUM_SIZE_8,
      (UINT8) ((BM_LEGACY_DEVICE_CONTEXT *) NewMenuEntry->VariableContext)->BbsIndex
      );
  }

  //
  // Create OneOf for item "Disabled"
  //
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_DISABLE_LEGACY_DEVICE),
    0,
    EFI_IFR_TYPE_NUM_SIZE_8,
    0xFF
    );

  //
  // Get Device Order from variable
  //
  VarData = EfiBootManagerGetVariableAndSize (
              VAR_LEGACY_DEV_ORDER_TEMP,
              &gEfiLegacyDevOrderVariableGuid,
              &VarSize
              );

  if (NULL != VarData) {
    DevOrder    = (LEGACY_DEV_ORDER_ENTRY *) VarData;
    while (VarData < VarData + VarSize) {
      if (DevOrder->BbsType == BbsType) {
        break;
      }

      VarData += sizeof (BBS_TYPE);
      VarData += *(UINT16 *) VarData;
      DevOrder = (LEGACY_DEV_ORDER_ENTRY *) VarData;
    }
    //
    // Create oneof tag here for FD/HD/CD #1 #2
    //
    for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
      //
      // Create the string for oneof tag
      //
      UnicodeSPrint (String, sizeof (String), TypeStr, Index);
      StrRef = HiiSetString (CallbackData->BmmHiiHandle, 0, String, NULL);

      UnicodeSPrint (String, sizeof (String), TypeStrHelp, Index);
      StrRefHelp = HiiSetString (CallbackData->BmmHiiHandle, 0, String, NULL);

      HiiCreateOneOfOpCode (
        mStartOpCodeHandle,
        (EFI_QUESTION_ID) (Key + Index),
        VARSTORE_ID_BOOT_MAINT,
        (UINT16) (Key + Index - CONFIG_OPTION_OFFSET),
        StrRef,
        StrRefHelp,
        EFI_IFR_FLAG_CALLBACK,
        EFI_IFR_NUMERIC_SIZE_1,
        OptionsOpCodeHandle,
        NULL
        );

      VarDevOrder = *(UINT16 *) ((UINT8 *) DevOrder + sizeof (BBS_TYPE) + sizeof (UINT16) + Index * sizeof (UINT16));

      if (0xFF00 == (VarDevOrder & 0xFF00)) {
        LegacyOrder[Index]  = 0xFF;
        Pos                 = (VarDevOrder & 0xFF) / 8;
        Bit                 = 7 - ((VarDevOrder & 0xFF) % 8);
        DisMap[Pos] = (UINT8) (DisMap[Pos] | (UINT8) (1 << Bit));
      } else {
        LegacyOrder[Index] = (UINT8) (VarDevOrder & 0xFF);
      }
    }
  }

  CopyMem (OldData, LegacyOrder, 100);

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  UpdatePageEnd (CallbackData);
}

/**
  Dispatch the display to the next page based on NewPageId.

  @param Private         The BMM context data.
  @param NewPageId       The original page ID.

**/
VOID
UpdatePageId (
  BMM_CALLBACK_DATA              *Private,
  UINT16                         NewPageId
  )
{
  if ((NewPageId > 0) && (NewPageId < MAXIMUM_FORM_ID)) {
    Private->BmmPreviousPageId  = Private->BmmCurrentPageId;
    Private->BmmCurrentPageId   = NewPageId;
  }
}
