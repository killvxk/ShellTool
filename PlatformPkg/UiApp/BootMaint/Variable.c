/*++

Copyright (c) 2006-2012, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Description:

  Variable operation that will be used by bootmaint

**/

#include "BootMaint.h"


VOID
ByoGroupMultipleLegacyBootOption4SameType (
  VOID
  );

VOID
ByoOrderLegacyBootOption4SameType (
  UINT16                   *DevOrder,
  UINTN                    DevOrderCount,
  UINT16                   **EnBootOption,
  UINTN                    *EnBootOptionCount,
  UINT16                   **DisBootOption,
  UINTN                    *DisBootOptionCount
  );

/**
  Delete Boot Option that represent a Deleted state in BootOptionMenu.
  After deleting this boot option, call Var_ChangeBootOrder to
  make sure BootOrder is in valid state.

  @retval EFI_SUCCESS   If all boot load option EFI Variables corresponding to  
                        BM_LOAD_CONTEXT marked for deletion is deleted.
  @retval EFI_NOT_FOUND If can not find the boot option want to be deleted.
  @return Others        If failed to update the "BootOrder" variable after deletion. 

**/
EFI_STATUS
Var_DelBootOption (
  VOID
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  UINT16          BootString[10];
  EFI_STATUS      Status;
  UINTN           Index;
  UINTN           Index2;

  Status  = EFI_SUCCESS;
  Index2  = 0;
  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&BootOptionMenu, (Index - Index2));
    if (NULL == NewMenuEntry) {
      return EFI_NOT_FOUND;
    }

    NewLoadContext = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    if (!NewLoadContext->Deleted) {
      continue;
    }

    UnicodeSPrint (
      BootString,
      sizeof (BootString),
      L"Boot%04x",
      NewMenuEntry->OptionNumber
      );

    EfiLibDeleteVariable (BootString, &gEfiGlobalVariableGuid);
    Index2++;
    //
    // If current Load Option is the same as BootNext,
    // must delete BootNext in order to make sure
    // there will be no panic on next boot
    //
    if (NewLoadContext->IsBootNext) {
      EfiLibDeleteVariable (L"BootNext", &gEfiGlobalVariableGuid);
    }

    RemoveEntryList (&NewMenuEntry->Link);
    BOpt_DestroyMenuEntry (NewMenuEntry);
    NewMenuEntry = NULL;
  }

  BootOptionMenu.MenuNumber -= Index2;

  Status = Var_ChangeBootOrder ();
  return Status;
}

/**
  After any operation on Boot####, there will be a discrepancy in BootOrder.
  Since some are missing but in BootOrder, while some are present but are
  not reflected by BootOrder. Then a function rebuild BootOrder from
  scratch by content from BootOptionMenu is needed.


  

  @retval  EFI_SUCCESS  The boot order is updated successfully.
  @return               EFI_STATUS other than EFI_SUCCESS if failed to
                        Set the "BootOrder" EFI Variable.

**/
EFI_STATUS
Var_ChangeBootOrder (
  VOID
  )
{

  EFI_STATUS    Status;
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        *BootOrderList;
  UINT16        *BootOrderListPtr;
  UINTN         BootOrderListSize;
  UINTN         Index;

  BootOrderList     = NULL;
  BootOrderListSize = 0;

  //
  // First check whether BootOrder is present in current configuration
  //
  BootOrderList = EfiBootManagerGetVariableAndSize (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    &BootOrderListSize
                    );

  //
  // If exists, delete it to hold new BootOrder
  //
  if (BootOrderList != NULL) {
    EfiLibDeleteVariable (L"BootOrder", &gEfiGlobalVariableGuid);
    FreePool (BootOrderList);
    BootOrderList = NULL;
  }
  //
  // Maybe here should be some check method to ensure that
  // no new added boot options will be added
  // but the setup engine now will give only one callback
  // that is to say, user are granted only one chance to
  // decide whether the boot option will be added or not
  // there should be no indictor to show whether this
  // is a "new" boot option
  //
  BootOrderListSize = BootOptionMenu.MenuNumber;

  if (BootOrderListSize > 0) {
    BootOrderList = AllocateZeroPool (BootOrderListSize * sizeof (UINT16));
    ASSERT (BootOrderList != NULL);
    BootOrderListPtr = BootOrderList;

    //
    // Get all current used Boot#### from BootOptionMenu.
    // OptionNumber in each BM_LOAD_OPTION is really its
    // #### value.
    //
    for (Index = 0; Index < BootOrderListSize; Index++) {
      NewMenuEntry    = BOpt_GetMenuEntry (&BootOptionMenu, Index);
      *BootOrderList  = (UINT16) NewMenuEntry->OptionNumber;
      BootOrderList++;
    }

    BootOrderList = BootOrderListPtr;

    //
    // After building the BootOrderList, write it back
    //
    Status = gRT->SetVariable (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    BootOrderListSize * sizeof (UINT16),
                    BootOrderList
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  return EFI_SUCCESS;
}






/**
  This function update the "BootOrder" EFI Variable based on
  BMM Formset's NV map. It then refresh BootOptionMenu
  with the new "BootOrder" list.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS             The function complete successfully.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to complete the function.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateBootOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  EFI_STATUS  Status;
  UINT16      Index;
  UINT16      OrderIndex;
  UINT16      *BootOrder;
  UINTN       BootOrderSize;
  UINT16      OptionNumber;

  //
  // First check whether BootOrder is present in current configuration
  //
  BootOrder = EfiBootManagerGetVariableAndSize (
                L"BootOrderTemp",
                &gByoGlobalVariableGuid,
                &BootOrderSize
                );
  if (BootOrder == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ASSERT (BootOptionMenu.MenuNumber <= (sizeof (CallbackData->BmmFakeNvData.OptionOrder) / sizeof (CallbackData->BmmFakeNvData.OptionOrder[0])));

  //
  // OptionOrder is subset of BootOrder
  //
  for (OrderIndex = 0; (OrderIndex < BootOptionMenu.MenuNumber) && (CallbackData->BmmFakeNvData.OptionOrder[OrderIndex] != 0); OrderIndex++) {
    for (Index = OrderIndex; Index < BootOrderSize / sizeof (UINT16); Index++) {
      if ((BootOrder[Index] == (UINT16) (CallbackData->BmmFakeNvData.OptionOrder[OrderIndex] - 1)) && (OrderIndex != Index)) {
        OptionNumber = BootOrder[Index];
        CopyMem (&BootOrder[OrderIndex + 1], &BootOrder[OrderIndex], (Index - OrderIndex) * sizeof (UINT16));
        BootOrder[OrderIndex] = OptionNumber;
      }
    }
  }

  Status = gRT->SetVariable (
                  L"BootOrderTemp",
                  &gByoGlobalVariableGuid,
                  TEMP_VAR_FLAG,
                  BootOrderSize,
                  BootOrder
                  );
  FreePool (BootOrder);

  ByoGroupMultipleLegacyBootOption4SameType ();

  BOpt_FreeMenu (&BootOptionMenu);
  BOpt_GetBootOptions (CallbackData);

  return Status;

}




/**
  Update the legacy BBS boot option. L"LegacyDevOrder" and gEfiLegacyDevOrderVariableGuid EFI Variable
  is udpated with the new Legacy Boot order. The EFI Variable of "Boot####" and gEfiGlobalVariableGuid
  is also updated.

  @param CallbackData    The context data for BMM.

  @return EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         If L"LegacyDevOrder" and gEfiLegacyDevOrderVariableGuid EFI Variable can be found.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate memory resource
**/
EFI_STATUS
Var_UpdateBBSOption (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  UINTN                       Index;
  UINTN                       Index2;
  VOID                        *BootOptionVar;
  CHAR16                      VarName[100];
  UINTN                       OptionSize;
  EFI_STATUS                  Status;
  UINT32                      *Attribute;
  BM_MENU_OPTION              *OptionMenu;
  UINT8                       *LegacyDev;
  UINT8                       *VarData;
  UINTN                       VarSize;
  LEGACY_DEV_ORDER_ENTRY      *DevOrder;
  UINT8                       *OriginalPtr;
  UINT8                       *DisMap;
  UINTN                       Pos;
  UINTN                       Bit;
  UINT16                      *NewOrder;
  UINT16                      Tmp;
  UINT16                      *EnBootOption;
  UINTN                       EnBootOptionCount;
  UINT16                      *DisBootOption;
  UINTN                       DisBootOptionCount;

  DisMap              = NULL;
  NewOrder            = NULL;

  switch (CallbackData->BmmCurrentPageId) {
    case FORM_SET_FD_ORDER_ID:
      OptionMenu            = (BM_MENU_OPTION *) &LegacyFDMenu;
      LegacyDev             = CallbackData->BmmFakeNvData.LegacyFD;
      CallbackData->BbsType = BBS_FLOPPY;
      break;

    case FORM_SET_HD_ORDER_ID:
      OptionMenu            = (BM_MENU_OPTION *) &LegacyHDMenu;
      LegacyDev             = CallbackData->BmmFakeNvData.LegacyHD;
      CallbackData->BbsType = BBS_HARDDISK;
      break;

    case FORM_SET_CD_ORDER_ID:
      OptionMenu            = (BM_MENU_OPTION *) &LegacyCDMenu;
      LegacyDev             = CallbackData->BmmFakeNvData.LegacyCD;
      CallbackData->BbsType = BBS_CDROM;
      break;

    case FORM_SET_NET_ORDER_ID:
      OptionMenu            = (BM_MENU_OPTION *) &LegacyNETMenu;
      LegacyDev             = CallbackData->BmmFakeNvData.LegacyNET;
      CallbackData->BbsType = BBS_EMBED_NETWORK;
      break;

    default:
      ASSERT (FORM_SET_BEV_ORDER_ID == CallbackData->BmmCurrentPageId);
      OptionMenu            = (BM_MENU_OPTION *) &LegacyBEVMenu;
      LegacyDev             = CallbackData->BmmFakeNvData.LegacyBEV;
      CallbackData->BbsType = BBS_BEV_DEVICE;
      break;
  }

  DisMap  = CallbackData->BmmOldFakeNVData.DisableMap;
  Status  = EFI_SUCCESS;

  //
  // Update the Variable "LegacyDevOrder"
  //
  VarData = (UINT8 *) EfiBootManagerGetVariableAndSize (
                        VAR_LEGACY_DEV_ORDER_TEMP,
                        &gEfiLegacyDevOrderVariableGuid,
                        &VarSize
                        );

  if (VarData == NULL) {
    return EFI_NOT_FOUND;
  }

  OriginalPtr = VarData;
  DevOrder    = (LEGACY_DEV_ORDER_ENTRY *) VarData;

  while (VarData < OriginalPtr + VarSize) {
    if (DevOrder->BbsType == CallbackData->BbsType) {
      break;
    }

    VarData += sizeof (BBS_TYPE) + DevOrder->Length;
    DevOrder = (LEGACY_DEV_ORDER_ENTRY *) VarData;
  }

  if (VarData >= OriginalPtr + VarSize) {
    FreePool (OriginalPtr);
    return EFI_NOT_FOUND;
  }

  NewOrder = AllocateZeroPool (DevOrder->Length - sizeof (DevOrder->Length));
  if (NewOrder == NULL) {
    FreePool (OriginalPtr);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
    if (0xFF == LegacyDev[Index]) {
      break;
    }

    NewOrder[Index] = LegacyDev[Index];
    DEBUG((EFI_D_ERROR, "NewOrder[%d] is 0x%X\n", Index, NewOrder[Index]));
  }
  //
  // Only the enable/disable state of each boot device with same device type can be changed,
  // so we can count on the index information in DevOrder.
  // DisMap bit array is the only reliable source to check a device's en/dis state,
  // so we use DisMap to set en/dis state of each item in NewOrder array
  //
  for (Index2 = 0; Index2 < OptionMenu->MenuNumber; Index2++) {
    Tmp = (UINT16) (DevOrder->Data[Index2] & 0xFF);
    Pos = Tmp / 8;
    Bit = 7 - (Tmp % 8);
    if ((DisMap[Pos] & (1 << Bit)) != 0) {
      NewOrder[Index] = (UINT16) (0xFF00 | Tmp);
      Index++;
    }
  }

  CopyMem (
    DevOrder->Data,
    NewOrder,
    DevOrder->Length - sizeof (DevOrder->Length)
    );
  FreePool (NewOrder);

  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER_TEMP,
                  &gEfiLegacyDevOrderVariableGuid,
                  TEMP_VAR_FLAG,
                  VarSize,
                  OriginalPtr
                  );


  //
  // Update BootOrder and Boot####.Attribute
  //
  // 1. Re-order the Option Number in BootOrder according to Legacy Dev Order
  //
  ASSERT (OptionMenu->MenuNumber == DevOrder->Length / sizeof (UINT16) - 1);

  ByoOrderLegacyBootOption4SameType (
    DevOrder->Data,
    DevOrder->Length / sizeof (UINT16) - 1,
    &EnBootOption,
    &EnBootOptionCount,
    &DisBootOption,
    &DisBootOptionCount
    );

  //
  // 2. Deactivate the DisBootOption and activate the EnBootOption
  //
  for (Index = 0; Index < DisBootOptionCount; Index++) {
    UnicodeSPrint (VarName, sizeof (VarName), L"BootTemp%04x", DisBootOption[Index]);
    BootOptionVar = EfiBootManagerGetVariableAndSize (
                      VarName,
                      &gByoGlobalVariableGuid,
                      &OptionSize
                      );
    if (BootOptionVar != NULL) {
      Attribute   = (UINT32 *) BootOptionVar;
      *Attribute &= ~LOAD_OPTION_ACTIVE;

      Status = gRT->SetVariable (
                      VarName,
                      &gByoGlobalVariableGuid,
                      TEMP_VAR_FLAG,
                      OptionSize,
                      BootOptionVar
                      );

      FreePool (BootOptionVar);
    }
  }

  for (Index = 0; Index < EnBootOptionCount; Index++) {
    UnicodeSPrint (VarName, sizeof (VarName), L"BootTemp%04x", EnBootOption[Index]);
    BootOptionVar = EfiBootManagerGetVariableAndSize (
                      VarName,
                      &gByoGlobalVariableGuid,
                      &OptionSize
                      );
    if (BootOptionVar != NULL) {
      Attribute   = (UINT32 *) BootOptionVar;
      *Attribute |= LOAD_OPTION_ACTIVE;

      Status = gRT->SetVariable (
                      VarName,
                      &gByoGlobalVariableGuid,
                      TEMP_VAR_FLAG,
                      OptionSize,
                      BootOptionVar
                      );

      FreePool (BootOptionVar);
    }
  }
  BOpt_GetBootOptions (CallbackData);

  FreePool (OriginalPtr);
  FreePool (EnBootOption);
  FreePool (DisBootOption);

  return Status;
}


