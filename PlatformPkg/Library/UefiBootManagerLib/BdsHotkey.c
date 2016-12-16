
#include "InternalBdsLib.h"
#include <Guid/ByoGlobalVariable.h>
//
// Lock for linked list
//
EFI_LOCK                     mHotkeyLock            = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);
LIST_ENTRY                   mHotkeyList            = INITIALIZE_LIST_HEAD_VARIABLE (mHotkeyList);
EFI_EVENT                    mHotkeyTriggered       = NULL;
BOOLEAN                      mHotkeyServiceStarted  = FALSE;
//
// Set OptionNumber as unassigned value to indicate the option isn't initialized
//
EFI_BOOT_MANAGER_LOAD_OPTION mHotkeyBootOption      = {LoadOptionNumberUnassigned};

EFI_BOOT_MANAGER_KEY_OPTION  *mContinueKeyOption    = NULL;
VOID                         *mTxtInExRegistration  = NULL;

VOID
EFIAPI
BdsEmptyFunction (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  ) {
}

UINTN
SizeOfKeyOption (
  EFI_BOOT_MANAGER_KEY_OPTION  *KeyOption
  )
{
  return OFFSET_OF (EFI_BOOT_MANAGER_KEY_OPTION, Keys)
    + KeyOption->KeyData.Options.InputKeyCount * sizeof (EFI_INPUT_KEY);
}

BOOLEAN
BitSet (
  IN UINT32   Value,
  IN UINT32   Bit
  )
{
  return (BOOLEAN) ((Value & Bit) != 0);
}

/**
  Initialize the KeyData and Key[] in the EFI_BOOT_MANAGER_KEY_OPTION.
**/
EFI_STATUS
InitializeKeyFields (
  IN UINT32                       Modifier,
  IN VA_LIST                      Args,
  OUT EFI_BOOT_MANAGER_KEY_OPTION *KeyOption
  )
{
  EFI_INPUT_KEY                   *Key;

  if (KeyOption == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Key = NULL;
  while (KeyOption->KeyData.Options.InputKeyCount < sizeof (KeyOption->Keys) / sizeof (KeyOption->Keys[0])) {
    Key = VA_ARG (Args, EFI_INPUT_KEY *);
    if (Key == NULL) {
      break;
    }
    CopyMem (
      &KeyOption->Keys[KeyOption->KeyData.Options.InputKeyCount],
      Key,
      sizeof (EFI_INPUT_KEY)
      );
    KeyOption->KeyData.Options.InputKeyCount++;
  }

  if (Key != NULL) {
    //
    // Too many keys
    //
    return EFI_INVALID_PARAMETER;
  }
  
  if ((Modifier & ~(EFI_BOOT_MANAGER_SHIFT_PRESSED
                 | EFI_BOOT_MANAGER_CONTROL_PRESSED
                 | EFI_BOOT_MANAGER_ALT_PRESSED
                 | EFI_BOOT_MANAGER_LOGO_PRESSED
                 | EFI_BOOT_MANAGER_MENU_KEY_PRESSED
                 | EFI_BOOT_MANAGER_SYS_REQ_PRESSED
                 )) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (BitSet (Modifier, EFI_BOOT_MANAGER_SHIFT_PRESSED)) {
    KeyOption->KeyData.Options.ShiftPressed = 1;
  }
  if (BitSet (Modifier, EFI_BOOT_MANAGER_CONTROL_PRESSED)) {
    KeyOption->KeyData.Options.ControlPressed = 1;
  }
  if (BitSet (Modifier, EFI_BOOT_MANAGER_ALT_PRESSED)) {
    KeyOption->KeyData.Options.AltPressed = 1;
  }
  if (BitSet (Modifier, EFI_BOOT_MANAGER_LOGO_PRESSED)) {
    KeyOption->KeyData.Options.LogoPressed = 1;
  }
  if (BitSet (Modifier, EFI_BOOT_MANAGER_MENU_KEY_PRESSED)) {
    KeyOption->KeyData.Options.MenuPressed = 1;
  }
  if (BitSet (Modifier, EFI_BOOT_MANAGER_SYS_REQ_PRESSED)) {
    KeyOption->KeyData.Options.SysReqPressed = 1;
  }

  DEBUG((EFI_D_ERROR, "InitializeKeyFields, InputKeyCount is 0x%x.\n", KeyOption->KeyData.Options.InputKeyCount));
  return EFI_SUCCESS;
}

/**
  Try to boot the boot option triggered by hot key.
**/
VOID
EFIAPI
EfiBootManagerHotkeyBoot (
  VOID
  )
{
  if (mHotkeyBootOption.OptionNumber != LoadOptionNumberUnassigned) {
    EfiBootManagerBoot (&mHotkeyBootOption);
    EfiBootManagerFreeLoadOption (&mHotkeyBootOption);
    mHotkeyBootOption.OptionNumber = LoadOptionNumberUnassigned;
  }
}

/**
  This is the common notification function for HotKeys, it will be registered
  with SimpleTextInEx protocol interface - RegisterKeyNotify() of ConIn handle.

  @param KeyData         A pointer to a buffer that is filled in with the keystroke
                         information for the key that was pressed.

  @retval  EFI_SUCCESS   KeyData is successfully processed.
  @return  EFI_NOT_FOUND Fail to find boot option variable.
**/
EFI_STATUS
EFIAPI
HotkeyCallback (
  IN EFI_KEY_DATA     *KeyData
)
{
  LIST_ENTRY                    *Link;
  HOTKEY                        *Hotkey;
  CHAR16                        OptionName[sizeof ("Boot####")];
  EFI_STATUS                    Status;
  EFI_KEY_DATA                  *HotkeyData;


  if (mHotkeyBootOption.OptionNumber != LoadOptionNumberUnassigned) {
    //
    // Do not process sequential hotkey stroke until the current boot option returns
    //
    return EFI_SUCCESS;
  }

  DEBUG ((EFI_D_INFO, "[Bds]HotkeyCallback: %04x:%04x\n", KeyData->Key.ScanCode, KeyData->Key.UnicodeChar));

  EfiAcquireLock (&mHotkeyLock);
  for ( Link = GetFirstNode (&mHotkeyList)
      ; !IsNull (&mHotkeyList, Link)
      ; Link = GetNextNode (&mHotkeyList, Link)
      ) {
    Hotkey = HOTKEY_FROM_LINK (Link);

    //
    // Is this Key Stroke we are waiting for?
    //
    ASSERT (Hotkey->WaitingKey < (sizeof (Hotkey->KeyData) / sizeof (Hotkey->KeyData[0])));
    HotkeyData = &Hotkey->KeyData[Hotkey->WaitingKey];
    if ((KeyData->Key.ScanCode == HotkeyData->Key.ScanCode) &&
        (KeyData->Key.UnicodeChar == HotkeyData->Key.UnicodeChar) &&
        (((KeyData->KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) != 0) ? 
          (KeyData->KeyState.KeyShiftState == HotkeyData->KeyState.KeyShiftState) : TRUE
        )
       ) {

      //
      // Receive an expecting key stroke, transit to next waiting state
      //
      Hotkey->WaitingKey++;

      if (Hotkey->WaitingKey == Hotkey->CodeCount) {
        //
        // Reset to initial waiting state
        //
        Hotkey->WaitingKey = 0;
        //
        // Received the whole key stroke sequence
        //
        Status = gBS->SignalEvent (mHotkeyTriggered);
        ASSERT_EFI_ERROR (Status);

        if (!Hotkey->IsContinue) {
          //
          // Launch its BootOption
          //
          UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", Hotkey->BootOption);
          Status = EfiBootManagerVariableToLoadOption (OptionName, &mHotkeyBootOption);
          DEBUG ((EFI_D_INFO, "[Bds]Hotkey for %s pressed - %r\n", OptionName, Status));
          if (EFI_ERROR (Status)) {
            mHotkeyBootOption.OptionNumber = LoadOptionNumberUnassigned;
          }
        } else {
          DEBUG ((EFI_D_INFO, "[Bds]Continue key pressed!\n"));
        }

      }
    } else {
      //
      // Receive an unexpected key stroke, reset to initial waiting state
      //
      Hotkey->WaitingKey = 0;
    }

  }
  EfiReleaseLock (&mHotkeyLock);

  return EFI_SUCCESS;
}

EFI_STATUS
UnregisterHotkeyNotify (
  IN HOTKEY                             *Hotkey
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 KeyIndex;
  EFI_HANDLE                            *Handles;
  UINTN                                 HandleCount;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL     *TxtInEx;
  EFI_HANDLE                            NotifyHandle;

  gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiSimpleTextInputExProtocolGuid,
          NULL,
          &HandleCount,
          &Handles
          );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (Handles[Index], &gEfiSimpleTextInputExProtocolGuid, (VOID **) &TxtInEx);
    ASSERT_EFI_ERROR (Status);
    for (KeyIndex = 0; KeyIndex < Hotkey->CodeCount; KeyIndex++) {
      Status = TxtInEx->RegisterKeyNotify (
                          TxtInEx,
                          &Hotkey->KeyData[KeyIndex],
                          HotkeyCallback,
                          &NotifyHandle
                          );
      if (!EFI_ERROR (Status)) {
        Status = TxtInEx->UnregisterKeyNotify (TxtInEx, NotifyHandle);
        DEBUG ((EFI_D_INFO, "[Bds]UnregisterKeyNotify: %04x/%04x %r\n", Hotkey->KeyData[KeyIndex].Key.ScanCode, Hotkey->KeyData[KeyIndex].Key.UnicodeChar, Status));
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
RegisterHotkeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TxtInEx,
  IN HOTKEY                             *Hotkey
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Index;
  EFI_HANDLE                            NotifyHandle;

  for (Index = 0; Index < Hotkey->CodeCount; Index++) {
    Status = TxtInEx->RegisterKeyNotify (
                        TxtInEx,
                        &Hotkey->KeyData[Index],
                        HotkeyCallback,
                        &NotifyHandle
                        );
    DEBUG ((
      EFI_D_INFO,
      "[Bds]RegisterKeyNotify: %04x/%04x %08x/%02x %r\n",
      Hotkey->KeyData[Index].Key.ScanCode,
      Hotkey->KeyData[Index].Key.UnicodeChar,
      Hotkey->KeyData[Index].KeyState.KeyShiftState,
      Hotkey->KeyData[Index].KeyState.KeyToggleState,
      Status
      ));
    if (EFI_ERROR (Status)) {
      //
      // some of the hotkey registry failed
      // do not unregister all in case we have both CTRL-ALT-P and CTRL-ALT-P-R
      //
      break;
    }
  }

  return EFI_SUCCESS;
}

VOID
GenerateKeyShiftState (
  IN UINTN                             Depth,
  IN EFI_BOOT_MANAGER_KEY_OPTION       *KeyOption,
  IN UINT32                            KeyShiftState,
  IN UINT32                            *KeyShiftStates,
  IN UINTN                             *KeyShiftStateCount
  )
{
  switch (Depth) {
  case 0:
    if (KeyOption->KeyData.Options.ShiftPressed) {
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_RIGHT_SHIFT_PRESSED, KeyShiftStates, KeyShiftStateCount);
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_LEFT_SHIFT_PRESSED,  KeyShiftStates, KeyShiftStateCount);
    } else {
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState, KeyShiftStates, KeyShiftStateCount);
    }
    break;

  case 1:
    if (KeyOption->KeyData.Options.ControlPressed) {
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_RIGHT_CONTROL_PRESSED, KeyShiftStates, KeyShiftStateCount);
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_LEFT_CONTROL_PRESSED,  KeyShiftStates, KeyShiftStateCount);
    } else {
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState, KeyShiftStates, KeyShiftStateCount);
    }
    break;

  case 2:
    if (KeyOption->KeyData.Options.AltPressed) {
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_RIGHT_ALT_PRESSED, KeyShiftStates, KeyShiftStateCount);
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_LEFT_ALT_PRESSED,  KeyShiftStates, KeyShiftStateCount);
    } else {
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState, KeyShiftStates, KeyShiftStateCount);
    }
    break;
  case  3:
    if (KeyOption->KeyData.Options.LogoPressed) {
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_RIGHT_LOGO_PRESSED, KeyShiftStates, KeyShiftStateCount);
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_LEFT_LOGO_PRESSED,  KeyShiftStates, KeyShiftStateCount);
    } else {
      GenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState, KeyShiftStates, KeyShiftStateCount);
    }
    break;
  case 4:
    if (KeyOption->KeyData.Options.MenuPressed) {
      KeyShiftState |= EFI_MENU_KEY_PRESSED;
    }
    if (KeyOption->KeyData.Options.SysReqPressed) {
      KeyShiftState |= EFI_SYS_REQ_PRESSED;
    }
    KeyShiftStates[*KeyShiftStateCount] = KeyShiftState;
    (*KeyShiftStateCount)++;
    break;
  }
}

/**
  Add it to hot key database, register it to existing TxtInEx.
  new TxtInEx will be automatically registered with all the hot key in dababase
**/
EFI_STATUS
ProcessKeyOption (
  IN EFI_BOOT_MANAGER_KEY_OPTION       *KeyOption
  )
{
  EFI_STATUS                           Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL    *TxtInEx;
  EFI_HANDLE                           *Handles;
  UINTN                                HandleCount;
  UINTN                                HandleIndex;
  UINTN                                Index;
  HOTKEY                               *Hotkey;
  UINTN                                KeyIndex;
  //
  // 16 is enough to enumerate all the possible combination of LEFT_XXX and RIGHT_XXX
  //
  UINT32                               KeyShiftStates[16];
  UINTN                                KeyShiftStateCount;

  KeyShiftStateCount = 0;
  GenerateKeyShiftState (0, KeyOption, EFI_SHIFT_STATE_VALID, KeyShiftStates, &KeyShiftStateCount);
  
  ASSERT (KeyShiftStateCount <= sizeof (KeyShiftStates) / sizeof (KeyShiftStates[0]));

  EfiAcquireLock (&mHotkeyLock);

  for (Index = 0; Index < KeyShiftStateCount; Index++) {
    Hotkey = AllocateZeroPool (sizeof (HOTKEY));
    ASSERT (Hotkey != NULL);

    Hotkey->Signature  = HOTKEY_SIGNATURE;
    Hotkey->BootOption = KeyOption->BootOption;
    Hotkey->IsContinue = (BOOLEAN) (KeyOption == mContinueKeyOption);
    Hotkey->CodeCount  = (UINT8) KeyOption->KeyData.Options.InputKeyCount;

    for (KeyIndex = 0; KeyIndex < Hotkey->CodeCount; KeyIndex++) {
      CopyMem (&Hotkey->KeyData[KeyIndex].Key, &KeyOption->Keys[KeyIndex], sizeof (EFI_INPUT_KEY));
      Hotkey->KeyData[KeyIndex].KeyState.KeyShiftState = KeyShiftStates[Index];
    }
    InsertTailList (&mHotkeyList, &Hotkey->Link);

    gBS->LocateHandleBuffer (
            ByProtocol,
            &gEfiSimpleTextInputExProtocolGuid,
            NULL,
            &HandleCount,
            &Handles
            );

    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) { 
      Status = gBS->HandleProtocol (Handles[HandleIndex], &gEfiSimpleTextInputExProtocolGuid, (VOID **) &TxtInEx);
      ASSERT_EFI_ERROR (Status);
      RegisterHotkeyNotify (TxtInEx, Hotkey);
    }
  }

  EfiReleaseLock (&mHotkeyLock);
  
  return EFI_SUCCESS;
}

/**
  Callback function for SimpleTextInEx protocol install events

  @param Event           the event that is signaled.
  @param Context         not used here.

**/
VOID
EFIAPI
TxtInExCallback (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS                         Status;
  UINTN                              BufferSize;
  EFI_HANDLE                         Handle;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TxtInEx;
  LIST_ENTRY                         *Link;
  
  while (TRUE) {
    BufferSize = sizeof (EFI_HANDLE);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    NULL,
                    mTxtInExRegistration,
                    &BufferSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      //
      // If no more notification events exist
      //
      return ;
    }

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiSimpleTextInputExProtocolGuid,
                    (VOID **) &TxtInEx
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Register the hot key notification for the existing items in the list
    //
    EfiAcquireLock (&mHotkeyLock);
    for (Link = GetFirstNode (&mHotkeyList); !IsNull (&mHotkeyList, Link); Link = GetNextNode (&mHotkeyList, Link)) {
      RegisterHotkeyNotify (TxtInEx, HOTKEY_FROM_LINK (Link));
    }
    EfiReleaseLock (&mHotkeyLock);
  }

}

EFI_STATUS
EFIAPI
EfiBootManagerFreeKeyOptions (
  IN EFI_BOOT_MANAGER_KEY_OPTION    *KeyOptions,
  IN UINTN                          KeyOptionCount
  )
{
  if (KeyOptions != NULL) {
    FreePool (KeyOptions);
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

EFI_STATUS
EFIAPI
EfiBootManagerRegisterContinueKeyOption (
  IN UINT32           Modifier,
  ...
  )
{
  EFI_STATUS                   Status;
  EFI_BOOT_MANAGER_KEY_OPTION  KeyOption;
  VA_LIST                      Args;
  
  if (mContinueKeyOption != NULL) {
    return EFI_ALREADY_STARTED;
  }

  ZeroMem (&KeyOption, sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
  VA_START (Args, Modifier);
  Status = InitializeKeyFields (Modifier, Args, &KeyOption);
  VA_END (Args);

  if (!EFI_ERROR (Status)) {
    mContinueKeyOption = AllocateCopyPool (sizeof (EFI_BOOT_MANAGER_KEY_OPTION), &KeyOption);
    ASSERT (mContinueKeyOption != NULL);
    if (mHotkeyServiceStarted) {
      ProcessKeyOption (mContinueKeyOption);
    }
  }

  return Status;
}

/**
  Stop the hotkey processing.
**/
VOID
EFIAPI
StopHotkeyService (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  LIST_ENTRY            *Link;
  HOTKEY                *Hotkey;

  DEBUG ((EFI_D_INFO, "[Bds]Stop Hotkey Service!\n"));
  gBS->CloseEvent (Event);

  EfiAcquireLock (&mHotkeyLock);
  for (Link = GetFirstNode (&mHotkeyList); !IsNull (&mHotkeyList, Link); ) {
    Hotkey = HOTKEY_FROM_LINK (Link);
    UnregisterHotkeyNotify (Hotkey);
    Link   = RemoveEntryList (Link);
    FreePool (Hotkey);
  }
  EfiReleaseLock (&mHotkeyLock);
}

/**

  Process all the "Key####" variables, associate Hotkeys with corresponding Boot Options.

  @retval  EFI_SUCCESS    Hotkey services successfully initialized.
**/
EFI_STATUS
EFIAPI
EfiBootManagerStartHotkeyService (
  IN EFI_EVENT                 *HotkeyTriggered
  )
{
  EFI_STATUS                   Status;
  UINT32                       BootOptionSupport;
  EFI_BOOT_MANAGER_KEY_OPTION  *KeyOptions;
  UINTN                        KeyOptionCount;
  UINTN                        Index;
  EFI_EVENT                    Event;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_CALLBACK,
                  BdsEmptyFunction,
                  NULL,
                  &mHotkeyTriggered
                  );
  ASSERT_EFI_ERROR (Status);

  if (HotkeyTriggered != NULL) {
    *HotkeyTriggered = mHotkeyTriggered;
  }
  //
  // Export our capability - EFI_BOOT_OPTION_SUPPORT_KEY and EFI_BOOT_OPTION_SUPPORT_APP
  // with maximum number of key presses of 3
  //
  BootOptionSupport = EFI_BOOT_OPTION_SUPPORT_KEY | EFI_BOOT_OPTION_SUPPORT_APP;
  SET_BOOT_OPTION_SUPPORT_KEY_COUNT (BootOptionSupport, 3);
  Status = gRT->SetVariable (
                  L"BootOptionSupport",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (UINT32),
                  &BootOptionSupport
                  );
  ASSERT_EFI_ERROR (Status);

  KeyOptions = EfiBootManagerGetKeyOptions (&KeyOptionCount);
  
  for (Index = 0; Index < KeyOptionCount; Index ++) {
    ProcessKeyOption (&KeyOptions[Index]);
  }
  EfiBootManagerFreeKeyOptions (KeyOptions, KeyOptionCount);
  if (mContinueKeyOption != NULL) {
    ProcessKeyOption (mContinueKeyOption);
  }

  EfiCreateProtocolNotifyEvent (
    &gEfiSimpleTextInputExProtocolGuid,
    TPL_CALLBACK,
    TxtInExCallback,
    NULL,
    &mTxtInExRegistration
    );

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             StopHotkeyService,
             NULL,
             &Event
             );
  ASSERT_EFI_ERROR (Status);


  mHotkeyServiceStarted = TRUE;
  return Status;
}

BOOLEAN
EFIAPI
IsKeyOptionValid (
  IN EFI_BOOT_MANAGER_KEY_OPTION     *KeyOption
)
{
  UINT16   OptionName[sizeof (L"Boot####")];
  UINT8    *BootOption;
  UINTN    BootOptionSize;
  UINT32   Crc;

  //
  // Check whether corresponding Boot Option exist
  //
  UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", KeyOption->BootOption);
  
  BootOption = EfiBootManagerGetVariableAndSize (
                    OptionName,
                    &gEfiGlobalVariableGuid,
                    &BootOptionSize
                    );

  if (OptionName == NULL) {
    return FALSE;
  }

  //
  // Check CRC for Boot Option
  //
  gBS->CalculateCrc32 (BootOption, BootOptionSize, &Crc);
  FreePool (BootOption);

  return (BOOLEAN) (KeyOption->BootOptionCrc == Crc);
}

EFI_BOOT_MANAGER_KEY_OPTION *
EFIAPI
EfiBootManagerGetKeyOptions (
  OUT UINTN     *Count
  )
{
  UINTN                       Index;
  UINTN                       OptionIndex;
  CHAR16                      KeyOptionName[sizeof (L"Key####")];
  UINT16                      *KeyOrder;
  UINTN                       KeyOrderSize;
  EFI_BOOT_MANAGER_KEY_OPTION *KeyOptions;
  EFI_BOOT_MANAGER_KEY_OPTION *KeyOption;

  if (Count == NULL) {
    return NULL;
  }

  *Count   = 0;
  KeyOrder = EfiBootManagerGetVariableAndSize (
               L"KeyOrder",
               &gByoGlobalVariableGuid,
               &KeyOrderSize
               );

  if (KeyOrder == NULL) {
    return NULL;
  }
  
  *Count      = KeyOrderSize / sizeof (UINT16);
  KeyOptions  = AllocatePool (*Count * sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
  OptionIndex = 0;
  
  for (Index = 0; Index < *Count; Index++) {
    UnicodeSPrint (KeyOptionName, sizeof (KeyOptionName), L"Key%04x", KeyOrder[Index]);

    KeyOption = GetEfiGlobalVariable (KeyOptionName);
    if ((KeyOption != NULL) && IsKeyOptionValid (KeyOption)) {
      CopyMem (&KeyOptions[OptionIndex], KeyOption, SizeOfKeyOption (KeyOption));
      KeyOptions[OptionIndex].OptionNumber = KeyOrder[Index];
      OptionIndex++;
    } else {
      DeleteOptionVariable (L"KeyOrder", KeyOrder[Index], &gByoGlobalVariableGuid);
    }
    
    if (KeyOption != NULL) {
      FreePool (KeyOption);
    }
  }

  //
  // *Count may less then OptionIndex which means we allocated more buffer than needed
  // It's fine because FreePool can free all the memory we allocated.
  //
  *Count = OptionIndex;

  return KeyOptions;
}

VOID
UpdateBootOption (
  IN EFI_BOOT_MANAGER_KEY_OPTION    KeyOption
)
{
  LIST_ENTRY                    *Link;
  HOTKEY                        *Hotkey;

  EfiAcquireLock (&mHotkeyLock);
  for ( Link = GetFirstNode (&mHotkeyList)
      ; !IsNull (&mHotkeyList, Link)
      ; Link = GetNextNode (&mHotkeyList, Link)
      ) {
    Hotkey = HOTKEY_FROM_LINK (Link);
    if (KeyOption.Keys[0].ScanCode == Hotkey->KeyData[0].Key.ScanCode &&
        KeyOption.Keys[0].UnicodeChar == Hotkey->KeyData[0].Key.UnicodeChar) {
      Hotkey->BootOption = KeyOption.BootOption;
      break;
    }
  }
  EfiReleaseLock (&mHotkeyLock);
  return;
}

/**
  Add the key option.
  It adds the key option variable and the key option takes affect immediately.

  @param AddedOption      Return the added key option.
  @param BootOptionNumber The boot option number for the key option.
  @param Modifier         Key shift state.
  @param ...              Parameter list of pointer of EFI_INPUT_KEY.

  @retval EFI_SUCCESS         The key option is added.
  @retval EFI_ALREADY_STARTED The hot key is already used by certain key option.
**/
EFI_STATUS
EFIAPI
EfiBootManagerAddKeyOptionVariable (
  OUT EFI_BOOT_MANAGER_KEY_OPTION *AddedOption,   OPTIONAL
  IN UINT16                       BootOptionNumber,
  IN UINT32                       Modifier,
  ...
  )
{
  EFI_STATUS                     Status;
  VA_LIST                        Args;
  VOID                           *BootOption;
  UINTN                          BootOptionSize;
  CHAR16                         BootOptionName[sizeof (L"Boot####")];
  EFI_BOOT_MANAGER_KEY_OPTION    KeyOption;
  EFI_BOOT_MANAGER_KEY_OPTION    *KeyOptions;
  UINTN                          KeyOptionCount;
  UINTN                          Index;
  UINT16                         KeyOptionNumber;
  CHAR16                         KeyOptionName[sizeof (L"Key####")];

  UnicodeSPrint (BootOptionName, sizeof (BootOptionName), L"Boot%04x", BootOptionNumber);
  BootOption = EfiBootManagerGetVariableAndSize (
                 BootOptionName,
                 &gEfiGlobalVariableGuid,
                 &BootOptionSize
                 );

  if (BootOption == NULL) {
    return EFI_NOT_FOUND;
  }

  ZeroMem (&KeyOption, sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
  KeyOption.BootOption = BootOptionNumber;
  Status = gBS->CalculateCrc32 (BootOption, BootOptionSize, &KeyOption.BootOptionCrc);
  ASSERT_EFI_ERROR (Status);
  FreePool (BootOption);

  VA_START (Args, Modifier);
  Status = InitializeKeyFields (Modifier, Args, &KeyOption);
  VA_END (Args);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if the hot key sequence was defined already
  //
  KeyOptions = EfiBootManagerGetKeyOptions (&KeyOptionCount);
  for (Index = 0; Index < KeyOptionCount; Index++) {
    if ((KeyOptions[Index].KeyData.PackedValue == KeyOption.KeyData.PackedValue) &&
      (CompareMem (KeyOptions[Index].Keys, KeyOption.Keys, KeyOption.KeyData.Options.InputKeyCount * sizeof (EFI_INPUT_KEY)) == 0)) {
      break;
    }
  }
  if (Index < KeyOptionCount && KeyOption.BootOption == KeyOptions[Index].BootOption) {
    Status =  EFI_ALREADY_STARTED;
    goto Exit;
  }
  if (Index >= KeyOptionCount) {
    Status = GetFreeOptionNumber (L"KeyOrder", &KeyOptionNumber, &gByoGlobalVariableGuid);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  } else {
    //
    // Update Exist Hotkey with new BootOption
    //
    UpdateBootOption (KeyOption);
    Status =  EFI_SUCCESS;
    goto Exit;
  }

  UnicodeSPrint (KeyOptionName, sizeof (KeyOptionName), L"Key%04x", KeyOptionNumber);

  Status = gRT->SetVariable (
                  KeyOptionName,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  SizeOfKeyOption (&KeyOption),
                  &KeyOption
                  );
  if (!EFI_ERROR (Status)) {
    Status = AddOptionNumberToOrderVariable (L"KeyOrder", KeyOptionNumber, (UINTN) -1, &gByoGlobalVariableGuid);
  }
  
  if (!EFI_ERROR (Status)) {
    //
    // Return the Key Option in case needed by caller
    //
    if (AddedOption != NULL) {
      CopyMem (AddedOption, &KeyOption, sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
    }

    //
    // Register the newly added hot key
    // Calling this function before EfiBootManagerStartHotkeyService doesn't
    // need to call ProcessKeyOption
    //
    if (mHotkeyServiceStarted) {
      ProcessKeyOption (&KeyOption);
    }
  }

  Exit:
    EfiBootManagerFreeKeyOptions (KeyOptions, KeyOptionCount);

  return Status;
}

/**
  Delete the Key Option variable and unregister the hot key
**/
EFI_STATUS
EFIAPI
EfiBootManagerDeleteKeyOptionVariable (
  IN EFI_BOOT_MANAGER_KEY_OPTION *DeletedOption, OPTIONAL
  IN UINT32                      Modifier,
  ...
  )
{
  EFI_STATUS                     Status;
  UINTN                          Index;
  VA_LIST                        Args;
  EFI_BOOT_MANAGER_KEY_OPTION    KeyOption;
  EFI_BOOT_MANAGER_KEY_OPTION    *KeyOptions;
  UINTN                          KeyOptionCount;
  LIST_ENTRY                     *Link;
  HOTKEY                         *Hotkey;
  UINT32                         ShiftState;
  BOOLEAN                        Match;

  ZeroMem (&KeyOption, sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
  VA_START (Args, Modifier);
  Status = InitializeKeyFields (Modifier, Args, &KeyOption);
  VA_END (Args);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  EfiAcquireLock (&mHotkeyLock);
  //
  // Delete the key option from active hot key list
  // Could have multiple entries when modifier isn't 0 because we map the ShiftPressed to RIGHT_SHIFT and RIGHT_SHIFT
  //
  for (Link = GetFirstNode (&mHotkeyList); !IsNull (&mHotkeyList, Link); ) {
    Hotkey = HOTKEY_FROM_LINK (Link);
    Match  = (BOOLEAN) (Hotkey->CodeCount == KeyOption.KeyData.Options.InputKeyCount);

    for (Index = 0; Match && (Index < Hotkey->CodeCount); Index++) {
      ShiftState = Hotkey->KeyData[Index].KeyState.KeyShiftState;
      if (
        (BitSet (ShiftState, EFI_RIGHT_SHIFT_PRESSED | EFI_LEFT_SHIFT_PRESSED) != KeyOption.KeyData.Options.ShiftPressed) ||
        (BitSet (ShiftState, EFI_RIGHT_CONTROL_PRESSED | EFI_LEFT_CONTROL_PRESSED) != KeyOption.KeyData.Options.ControlPressed) ||
        (BitSet (ShiftState, EFI_RIGHT_ALT_PRESSED | EFI_LEFT_ALT_PRESSED) != KeyOption.KeyData.Options.AltPressed) ||
        (BitSet (ShiftState, EFI_RIGHT_LOGO_PRESSED | EFI_LEFT_LOGO_PRESSED) != KeyOption.KeyData.Options.LogoPressed) ||
        (BitSet (ShiftState, EFI_MENU_KEY_PRESSED) != KeyOption.KeyData.Options.MenuPressed) ||
        (BitSet (ShiftState, EFI_SYS_REQ_PRESSED) != KeyOption.KeyData.Options.SysReqPressed) ||
        (CompareMem (&Hotkey->KeyData[Index].Key, &KeyOption.Keys[Index], sizeof (EFI_INPUT_KEY)) != 0)
        ) {
        //
        // Break when any field doesn't match
        //
        Match = FALSE;
        break;
      }
    }

    if (Match) {
      Link = RemoveEntryList (Link);
      FreePool (Hotkey);
    } else {
      Link = GetNextNode (&mHotkeyList, Link);
    }
  }

  //
  // Delete the key option from the variable
  //
  Status     = EFI_NOT_FOUND;
  KeyOptions = EfiBootManagerGetKeyOptions (&KeyOptionCount);
  for (Index = 0; Index < KeyOptionCount; Index++) {
    if ((KeyOptions[Index].KeyData.PackedValue == KeyOption.KeyData.PackedValue) &&
        (CompareMem (
           KeyOptions[Index].Keys, KeyOption.Keys,
           KeyOption.KeyData.Options.InputKeyCount * sizeof (EFI_INPUT_KEY)) == 0)
       ) {
      Status = DeleteOptionVariable (L"KeyOrder", KeyOptions[Index].OptionNumber, &gByoGlobalVariableGuid);
      //
      // Return the deleted key option in case needed by caller
      //
      if (DeletedOption != NULL) {
        CopyMem (DeletedOption, &KeyOptions[Index], sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
      }
      break;
    }
  }
  EfiBootManagerFreeKeyOptions (KeyOptions, KeyOptionCount);

  EfiReleaseLock (&mHotkeyLock);

  return Status;
}