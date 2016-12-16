/*++

Copyright (c) 2006-2012, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Description:

  Provide boot option support for Application "BootMaint"

  Include file system navigation, system handle selection

  Boot option manipulation

**/

#include "BootMaint.h"
#include "BBSsupport.h"



/**
  Create a menu entry by given menu type.

  @param MenuType        The Menu type to be created.

  @retval NULL           If failed to create the menu.
  @return the new menu entry.

**/
BM_MENU_ENTRY *
BOpt_CreateMenuEntry (
  UINTN           MenuType
  )
{
  BM_MENU_ENTRY *MenuEntry;
  UINTN         ContextSize;

  //
  // Get context size according to menu type
  //
  switch (MenuType) {
  case BM_LOAD_CONTEXT_SELECT:
    ContextSize = sizeof (BM_LOAD_CONTEXT);
    break;

  case BM_LEGACY_DEV_CONTEXT_SELECT:
    ContextSize = sizeof (BM_LEGACY_DEVICE_CONTEXT);
    break;

  default:
    ContextSize = 0;
    break;
  }

  if (ContextSize == 0) {
    return NULL;
  }

  //
  // Create new menu entry
  //
  MenuEntry = AllocateZeroPool (sizeof (BM_MENU_ENTRY));
  if (MenuEntry == NULL) {
    return NULL;
  }

  MenuEntry->VariableContext = AllocateZeroPool (ContextSize);
  if (MenuEntry->VariableContext == NULL) {
    FreePool (MenuEntry);
    return NULL;
  }

  MenuEntry->Signature        = BM_MENU_ENTRY_SIGNATURE;
  MenuEntry->ContextSelection = MenuType;
  return MenuEntry;
}

/**
  Free up all resource allocated for a BM_MENU_ENTRY.

  @param MenuEntry   A pointer to BM_MENU_ENTRY.

**/
VOID
BOpt_DestroyMenuEntry (
  BM_MENU_ENTRY         *MenuEntry
  )
{
  BM_LEGACY_DEVICE_CONTEXT  *LegacyDevContext;

  //
  //  Select by the type in Menu entry for current context type
  //
  switch (MenuEntry->ContextSelection) {

  case BM_LEGACY_DEV_CONTEXT_SELECT:
    LegacyDevContext = (BM_LEGACY_DEVICE_CONTEXT *) MenuEntry->VariableContext;
    FreePool (LegacyDevContext);

  default:
    break;
  }

  FreePool (MenuEntry->DisplayString);
  if (MenuEntry->HelpString != NULL) {
    FreePool (MenuEntry->HelpString);
  }

  FreePool (MenuEntry);
}

/**
  Get the Menu Entry from the list in Menu Entry List.

  If MenuNumber is great or equal to the number of Menu
  Entry in the list, then ASSERT.

  @param MenuOption      The Menu Entry List to read the menu entry.
  @param MenuNumber      The index of Menu Entry.

  @return The Menu Entry.

**/
BM_MENU_ENTRY *
BOpt_GetMenuEntry (
  BM_MENU_OPTION      *MenuOption,
  UINTN               MenuNumber
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  UINTN           Index;
  LIST_ENTRY      *List;

  ASSERT (MenuNumber < MenuOption->MenuNumber);

  List = MenuOption->Head.ForwardLink;
  for (Index = 0; Index < MenuNumber; Index++) {
    List = List->ForwardLink;
  }

  NewMenuEntry = CR (List, BM_MENU_ENTRY, Link, BM_MENU_ENTRY_SIGNATURE);

  return NewMenuEntry;
}



/**
  Free resources allocated in Allocate Rountine.

  @param FreeMenu        Menu to be freed
**/
VOID
BOpt_FreeMenu (
  BM_MENU_OPTION        *FreeMenu
  )
{
  BM_MENU_ENTRY *MenuEntry;
  while (!IsListEmpty (&FreeMenu->Head)) {
    MenuEntry = CR (
                  FreeMenu->Head.ForwardLink,
                  BM_MENU_ENTRY,
                  Link,
                  BM_MENU_ENTRY_SIGNATURE
                  );
    RemoveEntryList (&MenuEntry->Link);
    BOpt_DestroyMenuEntry (MenuEntry);
  }
  FreeMenu->MenuNumber = 0;
}




/**
  Build the LegacyFDMenu LegacyHDMenu LegacyCDMenu according to LegacyBios.GetBbsInfo().

  @retval EFI_SUCCESS The function complete successfully.
  @retval EFI_OUT_OF_RESOURCES No enough memory to complete this function.

**/
EFI_STATUS
BOpt_GetLegacyOptions (
  VOID
  )
{
  BM_MENU_ENTRY                 *NewMenuEntry;
  BM_LEGACY_DEVICE_CONTEXT      *NewLegacyDevContext;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;
  UINT16                        Index;
  UINTN                         FDNum;
  UINTN                         HDNum;
  UINTN                         CDNum;
  UINTN                         NETNum;
  UINTN                         BEVNum;

  //
  // Initialize Bbs Table Context from BBS info data
  //
  InitializeListHead (&LegacyFDMenu.Head);
  InitializeListHead (&LegacyHDMenu.Head);
  InitializeListHead (&LegacyCDMenu.Head);
  InitializeListHead (&LegacyNETMenu.Head);
  InitializeListHead (&LegacyBEVMenu.Head);

  FDNum   = 0;
  HDNum   = 0;
  CDNum   = 0;
  NETNum  = 0;
  BEVNum  = 0;

  BootOption = ByoEfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
  for (Index = 0; Index < BootOptionCount; Index++) {
    if ((DevicePathType (BootOption[Index].FilePath) != BBS_DEVICE_PATH) ||
        (DevicePathSubType (BootOption[Index].FilePath) != BBS_BBS_DP)
       ) {
      continue;
    }
    ASSERT (BootOption[Index].OptionalDataSize == sizeof (LEGACY_BOOT_OPTION_BBS_DATA));
    NewMenuEntry = BOpt_CreateMenuEntry (BM_LEGACY_DEV_CONTEXT_SELECT);
    ASSERT (NewMenuEntry != NULL);

    NewLegacyDevContext              = (BM_LEGACY_DEVICE_CONTEXT *) NewMenuEntry->VariableContext;
    NewLegacyDevContext->BbsIndex    = ((LEGACY_BOOT_OPTION_BBS_DATA *) BootOption[Index].OptionalData)->BbsIndex;
    NewLegacyDevContext->Description = AllocateCopyPool (StrSize (BootOption[Index].Description), BootOption[Index].Description);
    ASSERT (NewLegacyDevContext->Description != NULL);

    NewMenuEntry->DisplayString = NewLegacyDevContext->Description;
    NewMenuEntry->HelpString    = NULL;

    switch (((BBS_BBS_DEVICE_PATH *) BootOption[Index].FilePath)->DeviceType) {
    case BBS_TYPE_FLOPPY:
      InsertTailList (&LegacyFDMenu.Head, &NewMenuEntry->Link);
      FDNum++;
      break;

    case BBS_TYPE_HARDDRIVE:
      InsertTailList (&LegacyHDMenu.Head, &NewMenuEntry->Link);
      HDNum++;
      break;

    case BBS_TYPE_CDROM:
      InsertTailList (&LegacyCDMenu.Head, &NewMenuEntry->Link);
      CDNum++;
      break;

    case BBS_TYPE_EMBEDDED_NETWORK:
      InsertTailList (&LegacyNETMenu.Head, &NewMenuEntry->Link);
      NETNum++;
      break;

    case BBS_TYPE_BEV:
      InsertTailList (&LegacyBEVMenu.Head, &NewMenuEntry->Link);
      BEVNum++;
      break;
    }
  }

  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);

  LegacyFDMenu.MenuNumber   = FDNum;
  LegacyHDMenu.MenuNumber   = HDNum;
  LegacyCDMenu.MenuNumber   = CDNum;
  LegacyNETMenu.MenuNumber  = NETNum;
  LegacyBEVMenu.MenuNumber  = BEVNum;
  return EFI_SUCCESS;
}

/**
  Free out resouce allocated from Legacy Boot Options.

**/
VOID
BOpt_FreeLegacyOptions (
  VOID
  )
{
  BOpt_FreeMenu (&LegacyFDMenu);
  BOpt_FreeMenu (&LegacyHDMenu);
  BOpt_FreeMenu (&LegacyCDMenu);
  BOpt_FreeMenu (&LegacyNETMenu);
  BOpt_FreeMenu (&LegacyBEVMenu);
}

/**

  Build the BootOptionMenu according to BootOrder Variable.
  This Routine will access the Boot#### to get EFI_LOAD_OPTION.

  @param CallbackData The BMM context data.

  @return EFI_NOT_FOUND Fail to find "BootOrder" variable.
  @return EFI_SUCESS    Success build boot option menu.

**/
EFI_STATUS
BOpt_GetBootOptions (
  IN  BMM_CALLBACK_DATA         *CallbackData
  )
{
  UINTN                     Index;
  UINT16                    BootString[20];
  UINT8                     *LoadOptionFromVar;
  UINT8                     *LoadOption;
  UINTN                     BootOptionSize;
  BOOLEAN                   BootNextFlag;
  UINT16                    *BootOrderList;
  UINTN                     BootOrderListSize;
  UINT16                    *BootNext;
  UINTN                     BootNextSize;
  BM_MENU_ENTRY             *NewMenuEntry;
  BM_LOAD_CONTEXT           *NewLoadContext;
  UINT8                     *LoadOptionPtr;
  UINTN                     StringSize;
  UINTN                     OptionalDataSize;
  UINT8                     *LoadOptionEnd;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     MenuCount;
  UINT8                     *Ptr;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;  
  UINTN                         OptionIndex;
  
  MenuCount         = 0;
  BootOrderListSize = 0;
  BootNextSize      = 0;
  BootOrderList     = NULL;
  BootNext          = NULL;
  LoadOptionFromVar = NULL;

  
  BOpt_FreeMenu (&BootOptionMenu);
  InitializeListHead (&BootOptionMenu.Head);

  //
  // Get the BootOrder from the Var
  //
  BootOrderList = EfiBootManagerGetVariableAndSize (
                    L"BootOrderTemp",
                    &gByoGlobalVariableGuid,
                    &BootOrderListSize
                    );
  if (BootOrderList == NULL) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Get the BootNext from the Var
  //
  BootNext = EfiBootManagerGetVariableAndSize (
              L"BootNext",
              &gEfiGlobalVariableGuid,
              &BootNextSize
              );

  if (BootNext != NULL) {
    if (BootNextSize != sizeof (UINT16)) {
      FreePool (BootNext);
      BootNext = NULL;
    }
  }
  BootOption = ByoEfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
    
  for (Index = 0; Index < BootOrderListSize / sizeof (UINT16); Index++) {
    OptionIndex = (BootNext == NULL) ? Index : Index + 1;
    //
    // Don't display the hidden/inactive boot option
    //
    if (((BootOption[OptionIndex].Attributes & LOAD_OPTION_HIDDEN) != 0) || ((BootOption[OptionIndex].Attributes & LOAD_OPTION_ACTIVE) == 0)) {
      continue;
    }
    UnicodeSPrint (BootString, sizeof (BootString), L"BootTemp%04x", BootOrderList[Index]);
    
    //
    //  Get all loadoptions from the VAR
    //
    LoadOptionFromVar = EfiBootManagerGetVariableAndSize (
                          BootString,
                          &gByoGlobalVariableGuid,
                          &BootOptionSize
                          );
    if (LoadOptionFromVar == NULL) {
      continue;
    }

    LoadOption = AllocateZeroPool (BootOptionSize);
    if (LoadOption == NULL) {
      continue;
    }

    CopyMem (LoadOption, LoadOptionFromVar, BootOptionSize);
    FreePool (LoadOptionFromVar);
     
    if (BootNext != NULL) {
      BootNextFlag = (BOOLEAN) (*BootNext == BootOrderList[Index]);
    } else {
      BootNextFlag = FALSE;
    }

    NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
    ASSERT (NULL != NewMenuEntry);

    NewLoadContext                      = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;

    LoadOptionPtr                       = LoadOption;
    LoadOptionEnd                       = LoadOption + BootOptionSize;

    NewMenuEntry->OptionNumber          = BootOrderList[Index];
    DEBUG((EFI_D_INFO, "NewMenuEntry->OptionNumber is 0x%x.\n", NewMenuEntry->OptionNumber ));
    NewLoadContext->LoadOptionModified  = FALSE;
    NewLoadContext->Deleted             = FALSE;
    NewLoadContext->IsBootNext          = BootNextFlag;

    //
    // Is a Legacy Device?
    //
    Ptr = (UINT8 *) LoadOption;

    //
    // Attribute = *(UINT32 *)Ptr;
    //
    Ptr += sizeof (UINT32);
    
    //
    // FilePathSize = *(UINT16 *)Ptr;
    //
    Ptr += sizeof (UINT16);

    //
    // Description = (CHAR16 *)Ptr;
    //
    Ptr += StrSize ((CHAR16 *) Ptr);

    //
    // Now Ptr point to Device Path
    //
    DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
    if ((BBS_DEVICE_PATH == DevicePath->Type) && (BBS_BBS_DP == DevicePath->SubType)) {
      NewLoadContext->IsLegacy = TRUE;
    } else {
      NewLoadContext->IsLegacy = FALSE;
    }
    //
    // LoadOption is a pointer type of UINT8
    // for easy use with following LOAD_OPTION
    // embedded in this struct
    //
    NewLoadContext->LoadOption      = LoadOption;
    NewLoadContext->LoadOptionSize  = BootOptionSize;

    NewLoadContext->Attributes      = *(UINT32 *) LoadOptionPtr;
    NewLoadContext->IsActive        = (BOOLEAN) (NewLoadContext->Attributes & LOAD_OPTION_ACTIVE);

    NewLoadContext->ForceReconnect  = (BOOLEAN) (NewLoadContext->Attributes & LOAD_OPTION_FORCE_RECONNECT);

    LoadOptionPtr += sizeof (UINT32);

    NewLoadContext->FilePathListLength = *(UINT16 *) LoadOptionPtr;
    LoadOptionPtr += sizeof (UINT16);
    
    StringSize = StrSize((UINT16*)LoadOptionPtr);

    NewLoadContext->Description = AllocateZeroPool (StrSize((UINT16*)LoadOptionPtr));
    ASSERT (NewLoadContext->Description != NULL);
    StrCpy (NewLoadContext->Description, (UINT16*)LoadOptionPtr);
    
    ASSERT (NewLoadContext->Description != NULL);
    NewMenuEntry->DisplayString = NewLoadContext->Description;
        DEBUG((EFI_D_INFO, "NewMenuEntry->DisplayString is %s.\n", NewMenuEntry->DisplayString));

    LoadOptionPtr += StringSize;

    NewLoadContext->FilePathList = AllocateZeroPool (NewLoadContext->FilePathListLength);
    ASSERT (NewLoadContext->FilePathList != NULL);
    CopyMem (
      NewLoadContext->FilePathList,
      (EFI_DEVICE_PATH_PROTOCOL *) LoadOptionPtr,
      NewLoadContext->FilePathListLength
      );

    NewMenuEntry->HelpString = UiDevicePathToStr (NewLoadContext->FilePathList);
    NewMenuEntry->DisplayStringToken = GetStringTokenFromDepository (
                                        CallbackData,
                                        BootOptionStrDepository
                                        );
    NewMenuEntry->HelpStringToken = GetStringTokenFromDepository (
                                      CallbackData,
                                      BootOptionHelpStrDepository
                                      );
    LoadOptionPtr += NewLoadContext->FilePathListLength;

    if (LoadOptionPtr < LoadOptionEnd) {
      OptionalDataSize = BootOptionSize -
        sizeof (UINT32) -
        sizeof (UINT16) -
        StringSize -
        NewLoadContext->FilePathListLength;

      NewLoadContext->OptionalData = AllocateZeroPool (OptionalDataSize);
      ASSERT (NewLoadContext->OptionalData != NULL);
      CopyMem (
        NewLoadContext->OptionalData,
        LoadOptionPtr,
        OptionalDataSize
        );

      NewLoadContext->OptionalDataSize = OptionalDataSize;
    }

    InsertTailList (&BootOptionMenu.Head, &NewMenuEntry->Link);
    MenuCount++;
  }
  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);

  if (BootNext != NULL) {
    FreePool (BootNext);
  }
  if (BootOrderList != NULL) {
    FreePool (BootOrderList);
  }
  BootOptionMenu.MenuNumber = MenuCount;
  return EFI_SUCCESS;
}




/**

  Get the Option Number that has not been allocated for use.

  @param Type  The type of Option.

  @return The available Option Number.

**/
UINT16
BOpt_GetOptionNumber (
  CHAR16        *Type
  )
{
  UINT16        *OrderList;
  UINTN         OrderListSize;
  UINTN         Index;
  CHAR16        StrTemp[20];
  UINT16        *OptionBuffer;
  UINT16        OptionNumber;
  UINTN         OptionSize;

  OrderListSize = 0;
  OrderList     = NULL;
  OptionNumber  = 0;
  Index         = 0;

  UnicodeSPrint (StrTemp, sizeof (StrTemp), L"%sOrder", Type);

  OrderList = EfiBootManagerGetVariableAndSize (
                          StrTemp,
                          &gEfiGlobalVariableGuid,
                          &OrderListSize
                          );

  for (OptionNumber = 0; ; OptionNumber++) {
    if (OrderList != NULL) {
      for (Index = 0; Index < OrderListSize / sizeof (UINT16); Index++) {
        if (OptionNumber == OrderList[Index]) {
          break;
        }
      }
    }

    if (Index < OrderListSize / sizeof (UINT16)) {
      //
      // The OptionNumber occurs in the OrderList, continue to use next one
      //
      continue;
    }
    UnicodeSPrint (StrTemp, sizeof (StrTemp), L"%s%04x", Type, (UINTN) OptionNumber);
    DEBUG((EFI_D_ERROR,"Option = %s\n", StrTemp));
    OptionBuffer = EfiBootManagerGetVariableAndSize (
                       StrTemp,
                       &gEfiGlobalVariableGuid,
                       &OptionSize
                       );
    if (NULL == OptionBuffer) {
      //
      // The Boot[OptionNumber] / Driver[OptionNumber] NOT occurs, we found it
      //
      break;
    }
  }

  return OptionNumber;
}

/**

  Get the Option Number for Boot#### that does not used.

  @return The available Option Number.

**/
UINT16
BOpt_GetBootOptionNumber (
  VOID
  )
{
  return BOpt_GetOptionNumber (L"Boot");
}


