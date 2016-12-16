
#include "Bds.h"
#include "Language.h"
#include "HwErrRecSupport.h"
#include <Guid/ByoGlobalVariable.h>
#include <Guid/SetupPassword.h>
#include <Protocol/ByoFormSetManager.h>
#include <SetupVariable.h>
#include <Library/BaseCryptLib.h>
#include <Library/LogoLib.h>
#include <Library/HobLib.h>
#include <SetupVariable.h>
#include <Protocol/BdsBootManagerProtocol.h>
#include <Library/PlatformCommLib.h>


#define SCREEN_PRINT_X_START                 40
#define SCREEN_PRINT_Y_START                 10
#define SCREEN_PRINT_Y_HEIGHT                20


BOOLEAN IsPlatformHotKeyPressed();

UINT8      gEraseCountLine = 0;
SETUP_DATA *gSetupData;

EFI_GRAPHICS_OUTPUT_BLT_PIXEL gEfiPixelColorsBlack  = {0x00, 0x00, 0x00, 0x00};
EFI_GRAPHICS_OUTPUT_BLT_PIXEL gEfiPixelColorsYellow = {0x00, 0xff, 0xff, 0x00};



//TODO: BdsDxe driver produces BDS ARCH protocol but also BDS UTILITY protocol to replace UefiBootManagerLib.
///
/// BDS arch protocol instance initial value.
///
EFI_BDS_ARCH_PROTOCOL  gBds = {
  BdsEntry
};


/**

  Install Boot Device Selection Protocol

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  BDS has finished initializing.
                        Return the dispatcher and recall BDS.Entry
  @retval  Other        Return status from AllocatePool() or gBS->InstallProtocolInterface

**/
EFI_STATUS
EFIAPI
BdsInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;
  //
  // Install protocol interface
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiBdsArchProtocolGuid, &gBds,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}




/**
  Generic function to update the EFI  variables.

  @param VariableName    The name of the variable to be updated.
  @param ProcessVariable The function pointer to update the variable.
                         NULL means to restore to the original value.
**/
VOID
BdsUpdateVariable (
  CHAR16               *VariableName,
  BDS_PROCESS_VARIABLE ProcessVariable,
  EFI_GUID             *VendorGuid
  )
{
  EFI_STATUS  Status;
  CHAR16      BackupVariableName[20];
  CHAR16      FlagVariableName[20];
  VOID        *Variable;
  VOID        *BackupVariable;
  VOID        *NewVariable;
  UINTN       VariableSize;
  UINTN       BackupVariableSize;
  UINTN       NewVariableSize;
  BOOLEAN     Flag;
  BOOLEAN     *FlagVariable;
  UINTN       FlagSize;

  ASSERT (StrLen (VariableName) <= 13);
  UnicodeSPrint (BackupVariableName, sizeof (BackupVariableName), L"%sBackup", VariableName);
  UnicodeSPrint (FlagVariableName, sizeof (FlagVariableName), L"%sModify", VariableName);

  Variable       = EfiBootManagerGetVariableAndSize (VariableName, &gEfiGlobalVariableGuid, &VariableSize);
  BackupVariable = EfiBootManagerGetVariableAndSize (BackupVariableName, &gEfiCallerIdGuid, &BackupVariableSize);
  FlagVariable   = EfiBootManagerGetVariableAndSize (FlagVariableName, &gEfiCallerIdGuid, &FlagSize);
  if ((ProcessVariable != NULL) && (FlagVariable == NULL)) {
    //
    // Current boot is a modified boot and last boot is a normal boot
    // Set flag to indicate it's a modified boot
    // BackupVariable <- Variable
    // Variable       <- ProcessVariable (Variable)
    //
    Flag   = TRUE;
    Status = gRT->SetVariable (
                    FlagVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof (Flag),
                    &Flag
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gRT->SetVariable (
                    BackupVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VariableSize,
                    Variable
                    );
    ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));


    NewVariable     = Variable;
    NewVariableSize = VariableSize;
    ProcessVariable (&NewVariable, &NewVariableSize);

    Status = gRT->SetVariable (
                    VariableName,
                    VendorGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    NewVariableSize,
                    NewVariable
                    );
    ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));

    if (NewVariable != NULL) {
      FreePool (NewVariable);
    }
  } else if ((ProcessVariable == NULL) && (FlagVariable != NULL)) {
    //
    // Current boot is a normal boot and last boot is a modified boot
    // Clear flag to indicate it's a normal boot
    // Variable       <- BackupVariable
    // BackupVariable <- NULL
    //
    Status = gRT->SetVariable (
                    FlagVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    0,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gRT->SetVariable (
                    VariableName,
                    VendorGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    BackupVariableSize,
                    BackupVariable
                    );
    ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));

    Status = gRT->SetVariable (
                    BackupVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    0,
                    NULL
                    );
    ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));
  }

  if (Variable != NULL) {
    FreePool (Variable);
  }

  if (BackupVariable != NULL) {
    FreePool (BackupVariable);
  }

  if (FlagVariable != NULL) {
    FreePool (FlagVariable);
  }
}


/**
  Function waits for a given event to fire, or for an optional timeout to expire.

  @param   Event              The event to wait for
  @param   Timeout            An optional timeout value in 100 ns units.

  @retval  EFI_SUCCESS      Event fired before Timeout expired.
  @retval  EFI_TIME_OUT     Timout expired before Event fired..

**/
EFI_STATUS
BdsWaitForSingleEvent (
  IN  EFI_EVENT                  Event,
  IN  UINT64                     Timeout
  )
{
  UINTN       Index;
  EFI_STATUS  Status;
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitList[2];

  
  if (Timeout != 0) {
    //
    // Create a timer event
    //
    Status = gBS->CreateEvent(EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      gBS->SetTimer (
             TimerEvent,
             TimerRelative,
             Timeout
             );
      WaitList[0] = Event;
      WaitList[1] = TimerEvent;
      Status = gBS->WaitForEvent(2, WaitList, &Index);
      ASSERT_EFI_ERROR(Status);
      gBS->CloseEvent (TimerEvent);
      if (Index == 1) {
        Status = EFI_TIMEOUT;
      }
    }
  } else {
    //
    // No timeout... just wait on the event
    //
    Status = gBS->WaitForEvent (1, &Event, &Index);
    ASSERT (!EFI_ERROR (Status));
    ASSERT (Index == 0);
  }

  return Status;
}

/**
  The function reads user inputs.

**/

VOID
BdsReadKeys (
  VOID
  )
{
  EFI_STATUS         Status;
  EFI_INPUT_KEY      Key;

  while (gST->ConIn != NULL) {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    
    if (EFI_ERROR (Status)) {
      //
      // No more keys.
      //
      break;
    }
  }

}

/**
  The function waits for the boot manager timeout expires or hotkey is pressed.

  It calls PlatformBootManagerWaitCallback each second.
**/
VOID
BdsWait (
  IN EFI_EVENT      HotkeyTriggered
  )
{
  EFI_STATUS            Status;
  UINT16                TimeoutRemain;
  UINT16                TotalTimeout;	


  TotalTimeout  = gSetupData->BootTimeout;
  TotalTimeout *= 10;
  TimeoutRemain = TotalTimeout;

  while (TimeoutRemain != 0) {
		
    PlatformBootManagerWaitCallback(TotalTimeout, TimeoutRemain);
    BdsReadKeys();
		if(IsPlatformHotKeyPressed()){
			break;
		}		
  
    // Can be removed after all keyboard drivers invoke callback in timer callback.
    Status = BdsWaitForSingleEvent(HotkeyTriggered, 1000000);
    if (!EFI_ERROR (Status)) {
      break;
    }

    //
    // 0xffff means waiting forever
    // BDS with no hotkey provided and 0xffff as timeout will "hang" in the loop
    //
    if (TimeoutRemain != 0xffff) {
      TimeoutRemain--;
    }
  }
}





VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16  Timeout,  
  UINT16  TimeoutRemain
  )
{
  CHAR16                        *TmpStr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;
  EFI_STATUS                    Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TxtInEx;
  EFI_KEY_DATA                  KeyData;
  BOOLEAN                       PausePressed;

    //
    // Show progress
    //
    SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
    SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
    SetMem (&Color,      sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);

    TmpStr = L"Start boot option";
    if (TmpStr != NULL) {
      ShowProgress (
        Foreground,
        Background,
        TmpStr,
        Color,
        (Timeout - TimeoutRemain) * 100 / Timeout,
        0
        );
    }

  //
  // Pause on PAUSE key
  //
  Status = gBS->HandleProtocol (gST->ConsoleInHandle, &gEfiSimpleTextInputExProtocolGuid, (VOID **) &TxtInEx);
  ASSERT_EFI_ERROR (Status);

  PausePressed = FALSE;

  while (TRUE) {
    Status = TxtInEx->ReadKeyStrokeEx (TxtInEx, &KeyData);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (KeyData.Key.ScanCode == SCAN_PAUSE) {
      PausePressed = TRUE;
      break;
    }
  }

  //
  // Loop until non-PAUSE key pressed
  //
  while (PausePressed) {
    Status = TxtInEx->ReadKeyStrokeEx (TxtInEx, &KeyData);
    if (!EFI_ERROR (Status)) {
      DEBUG ((
        EFI_D_INFO, "[PauseCallback] %x/%x %x/%x\n",
        KeyData.Key.ScanCode, KeyData.Key.UnicodeChar, 
        KeyData.KeyState.KeyShiftState, KeyData.KeyState.KeyToggleState
        ));
      PausePressed = (BOOLEAN) (KeyData.Key.ScanCode == SCAN_PAUSE);
    }
  }
}
 

VOID
PrintBootPrompt (
  VOID
  )
{
  UINT8  PosY = SCREEN_PRINT_Y_START;
 
  PrintXY (SCREEN_PRINT_X_START, PosY, NULL, NULL, L"Press [F2]     to enter setup and select boot options.\n");
  gEraseCountLine ++;		
  PosY += SCREEN_PRINT_Y_HEIGHT;
  PrintXY (SCREEN_PRINT_X_START, PosY, NULL, NULL, L"Press [F7]     to show boot menu options.\n");
  gEraseCountLine ++;
}





/**
  Clean the boot prompt.
**/
VOID
CleanBootPrompt (
  VOID
  )
{
  UINT8                         Row;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;
  UINT8                         PosY = SCREEN_PRINT_Y_START;

  for (Row = 0; Row < gEraseCountLine; Row ++) {
    PrintXY (SCREEN_PRINT_X_START, PosY, NULL, NULL, L"                                                          ");
    PosY += SCREEN_PRINT_Y_HEIGHT;			
  }
  //
  // Clean progress
  //
  SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
  SetMem (&Color,      sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  ShowProgress (
    Foreground,
    Background,
    L"                                                               ",
    Color,
    0,
    0
    );
}









BOOLEAN 
IsThisLegacyBootDevChanged (
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  );

VOID
BootAllBootOptions (
  IN EFI_BOOT_MANAGER_LOAD_OPTION    *BootOptions,
  IN UINTN                           BootOptionCount
  )
{
  UINTN                              Index;

  for (Index = 0; Index < BootOptionCount; Index++) {
    if ((BootOptions[Index].Attributes & LOAD_OPTION_ACTIVE) == 0) {
      continue;
    }

    if(IsThisLegacyBootDevChanged(&BootOptions[Index])){
      DEBUG((EFI_D_ERROR, "ThisLegacyBootOrderChanged, Need Refresh!\n"));		
      break;		
    }	
    EfiBootManagerBoot(&BootOptions[Index]);
  }
}

#if !defined(MDEPKG_NDEBUG)
STATIC VOID DumpBootOptions(VOID)
{
  EFI_BOOT_MANAGER_LOAD_OPTION    *Option;
  UINTN                           BootOptionCount;
  UINTN                           i;

  DEBUG ((EFI_D_INFO, "=============Dumping Boot Options==============\n"));
  
  Option = EfiBootManagerGetLoadOptions(&BootOptionCount, LoadOptionTypeBoot);
  for (i = 0; i < BootOptionCount; i++) {
    DEBUG((EFI_D_INFO, "Boot%04X: A:%08X %s\n", Option[i].OptionNumber, Option[i].Attributes, Option[i].Description));
  }
  EfiBootManagerFreeLoadOptions (Option, BootOptionCount);
  
  DEBUG ((EFI_D_INFO, "===================== END =====================\n"));
}
#endif

/**
  This function attempts to boot per the boot order specified by platform policy.

  If the boot via Boot#### returns with a status of EFI_SUCCESS the boot manager will stop 
  processing the BootOrder variable and present a boot manager menu to the user. If a boot via 
  Boot#### returns a status other than EFI_SUCCESS, the boot has failed and the next Boot####
  in the BootOrder variable will be tried until all possibilities are exhausted.
                                  -- Chapter 3.1.1 Boot Manager Programming, the 4th paragraph
**/
VOID
DefaultBootBehavior (
  VOID
  )
{
  UINTN                         BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;

  DEBUG((EFI_D_ERROR,"into DefaultBootBehavior() \n"));

  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
  BootAllBootOptions(BootOptions, BootOptionCount);
	
  EfiBootManagerFreeLoadOptions(BootOptions, BootOptionCount);
  DEBUG((EFI_D_ERROR,"bf EfiBootManagerConnectAll() \n"));
  EfiBootManagerConnectAll();
  EfiBootManagerRefreshAllBootOption();

#if !defined(MDEPKG_NDEBUG)
	DEBUG((EFI_D_ERROR,"This is the true and final boot option dump \n"));
	DumpBootOptions();
#endif

  BootOptions = EfiBootManagerGetLoadOptions(&BootOptionCount, LoadOptionTypeBoot);	
  BootAllBootOptions (BootOptions, BootOptionCount);
	
  Print(L"No bootable device!");
  CpuDeadLoop();	
}




/**
  The function will go through the driver option link list, load and start
  every driver the driver option device path point to.

  @param  BdsDriverLists        The header of the current driver option link list

**/
VOID
LoadDrivers (
  IN EFI_BOOT_MANAGER_LOAD_OPTION       *DriverOption,
  IN UINTN                              DriverOptionCount
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  EFI_HANDLE                ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo;
  BOOLEAN                   ReconnectAll;

  ReconnectAll = FALSE;

  //
  // Process the driver option
  //
  for (Index = 0; Index < DriverOptionCount; Index++) {
    //
    // If a load option is not marked as LOAD_OPTION_ACTIVE,
    // the boot manager will not automatically load the option.
    //
    if ((DriverOption[Index].Attributes & LOAD_OPTION_ACTIVE) == 0) {
      continue;
    }
    
    //
    // If a driver load option is marked as LOAD_OPTION_FORCE_RECONNECT,
    // then all of the EFI drivers in the system will be disconnected and
    // reconnected after the last driver load option is processed.
    //
    if ((DriverOption[Index].Attributes & LOAD_OPTION_FORCE_RECONNECT) != 0) {
      ReconnectAll = TRUE;
    }
    
    //
    // Make sure the driver path is connected.
    //
    EfiBootManagerConnectDevicePath (DriverOption[Index].FilePath, NULL);

    //
    // Load and start the image that Driver#### describes
    //
    Status = gBS->LoadImage (
                    FALSE,
                    gImageHandle,
                    DriverOption[Index].FilePath,
                    NULL,
                    0,
                    &ImageHandle
                    );

    if (!EFI_ERROR (Status)) {
      gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ImageInfo);

      //
      // Verify whether this image is a driver, if not,
      // exit it and continue to parse next load option
      //
      if (ImageInfo->ImageCodeType != EfiBootServicesCode && ImageInfo->ImageCodeType != EfiRuntimeServicesCode) {
        gBS->Exit (ImageHandle, EFI_INVALID_PARAMETER, 0, NULL);
        continue;
      }

      ImageInfo->LoadOptionsSize  = DriverOption[Index].OptionalDataSize;
      ImageInfo->LoadOptions      = DriverOption[Index].OptionalData;
      //
      // Before calling the image, enable the Watchdog Timer for
      // the 5 Minute period
      //
      gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);

      DriverOption[Index].Status = gBS->StartImage (ImageHandle, &DriverOption[Index].ExitDataSize, &DriverOption[Index].ExitData);
      DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Driver Return Status = %r\n", DriverOption[Index].Status));

      //
      // Clear the Watchdog Timer after the image returns
      //
      gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);
    }
  }
  
  //
  // Process the LOAD_OPTION_FORCE_RECONNECT driver option
  //
  if (ReconnectAll) {
    EfiBootManagerDisconnectAll ();
    EfiBootManagerConnectAll ();
  }

}




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



VOID SystemHalt()
{
  CHAR16  *Result = L"Fatal Error... System Halted.";

  CreatePopUp(EFI_RED | EFI_BACKGROUND_LIGHTGRAY, NULL, Result, NULL);
  gBS->RaiseTPL(TPL_HIGH_LEVEL);
  CpuDeadLoop();
}


EFI_STATUS
CheckSetupPassword (VOID ) 
{
  EFI_STATUS                        Status;
  EFI_BYO_FORMSET_MANAGER_PROTOCOL  *FormsetManager = NULL;
  CHAR16                            Password[PASSWORD_MAX_SIZE + 1];
  UINTN                             VarSize;
  UINTN                             Count;
  TSESETUP                          Tse;
  UINT8                             Sha256HashArray[32];
  UINT8                             AccessType;
  
   
  Status = gBS->LocateProtocol (
                  &gEfiByoFormsetManagerProtocolGuid,
                  NULL,
                  &FormsetManager
                  );
  ASSERT(!EFI_ERROR(Status));
  ASSERT(FormsetManager->CheckPassword!=NULL);	
  if (EFI_ERROR(Status)) {
    goto ProcExit;
  }

  VarSize = sizeof(TSESETUP);
  Status = gRT->GetVariable (
                  SETUP_PASSWORD_VARTSE_NAME,
                  &gSetupPasswordVariableTseGuid,
                  NULL,
                  &VarSize,
                  &Tse
                  );
  ASSERT(!EFI_ERROR(Status));

  if(!(Tse.UserPasswdPresent || Tse.AdminPasswdPresent)){
    goto ProcExit;
  }
    
  gST->ConOut->ClearScreen(gST->ConOut);
  Count = 0;
  AccessType = 0;
  while(1) {
    ZeroMem(&Password[0], sizeof(Password));
    if (Count == 0) {
      FormsetManager->CheckPassword(L"Please Input Your Password", &Password[0]);
    }else if (Count == 1) {
      FormsetManager->CheckPassword(L"Input Password 2 Times", &Password[0]);
    }else if (Count == 2) {
      FormsetManager->CheckPassword(L"Input Password 3 Times", &Password[0]);
    }
    
    if(Tse.AdminPasswdPresent){
      Sha256Hash(Password, StrSize(Password), Sha256HashArray, sizeof(Sha256HashArray));
      if(CompareMem(Tse.AdminPasswdHash, Sha256HashArray, sizeof(Sha256HashArray)) == 0){
        AccessType = 2;
        break;
      }
    }
    if(Tse.UserPasswdPresent){
      Sha256Hash(Password, StrSize(Password), Sha256HashArray, sizeof(Sha256HashArray));
      if(CompareMem(Tse.UserPasswdHash, Sha256HashArray, sizeof(Sha256HashArray)) == 0){
        AccessType = 1;
        if (Tse.UserAccessLevel == 2) {
          Status = EFI_ACCESS_DENIED;
          goto ProcExit;
        }
        break;        
      }
    }    

    if(AccessType == 0 && Count == 2){
      SystemHalt();
    } else {
      FormsetManager->CheckPassword(L"Press ENTER to Continue", NULL);
    }
    Count++;
  }
  
  gST->ConOut->ClearScreen (gST->ConOut);

ProcExit:  
  return Status;
}






EFI_STATUS
CreateFvBootOption (
  EFI_GUID                     *FileGuid,
  CHAR16                       *Description,
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOption,
  BOOLEAN                      IsBootCategory,
  UINT8                        *OptionalData,    OPTIONAL
  UINT32                       OptionalDataSize       
  );

VOID 
PrintBootPrompt (
  VOID
  );




#define HOTKEY_BOOT_NONE              0
#define HOTKEY_BOOT_SETUP             1
#define HOTKEY_BOOT_MENU              2
#define HOTKEY_BOOT_PASS              3

typedef struct {
  EFI_INPUT_KEY   Key;
  UINTN           BootType;
  VOID            *RegHandle;
  EFI_GUID        *FileName;
  CHAR16          *KeyNameStr;	
} PLAT_HOT_KEY_CTX;

STATIC PLAT_HOT_KEY_CTX gPlatHotKeyCtx[] = {
  {
    {SCAN_F2, CHAR_NULL},
    HOTKEY_BOOT_SETUP,
    NULL,
    (EFI_GUID*)PcdGetPtr(PcdSetupFileGuid),
    L"F2"    
  },
  {
    {SCAN_F7, CHAR_NULL},
    HOTKEY_BOOT_MENU,
    NULL,
    (EFI_GUID*)PcdGetPtr(PcdBootMenuFileGuid),
    L"F7"    
  },
  {
    {SCAN_NULL, CHAR_CARRIAGE_RETURN},
    HOTKEY_BOOT_PASS,
    NULL,
    NULL,
    L"Enter"  
  },
};

STATIC UINTN gHotkeyBootType = HOTKEY_BOOT_NONE;

BOOLEAN IsPlatformHotKeyPressed()
{
  return (gHotkeyBootType != HOTKEY_BOOT_NONE);
}

STATIC EFI_STATUS KeyNotifyCallBack(EFI_KEY_DATA *KeyData)
{
  UINTN  Index;

  if(IsPlatformHotKeyPressed()){
    goto ProcExit;
  }		

  for(Index=0;Index<sizeof(gPlatHotKeyCtx)/sizeof(gPlatHotKeyCtx[0]);Index++){
    if(CompareMem(&KeyData->Key, &gPlatHotKeyCtx[Index].Key, sizeof(EFI_INPUT_KEY)) == 0){
      gHotkeyBootType = gPlatHotKeyCtx[Index].BootType;
/*      
      PrintXY(
        SCREEN_PRINT_X_START, 
        SCREEN_PRINT_Y_START + gEraseCountLine * SCREEN_PRINT_Y_HEIGHT, 
        &gEfiPixelColorsYellow, 
        &gEfiPixelColorsBlack, 
        L"Key [%s] Pressed, Boot process continue ...\n", 
        gPlatHotKeyCtx[Index].KeyNameStr
        );
        gEraseCountLine++;
*/			
      break;			
    }			
  }		

ProcExit:
  return EFI_SUCCESS;	
}



STATIC VOID RegisterPlatformHotKey()
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL	 *TextInEx;
  EFI_KEY_DATA                       KeyData;
  UINTN                              Index;

  Status = gBS->HandleProtocol(gST->ConsoleInHandle, &gEfiSimpleTextInputExProtocolGuid, (VOID**)&TextInEx);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }		

  ZeroMem(&KeyData, sizeof(KeyData));

  for(Index=0;Index<sizeof(gPlatHotKeyCtx)/sizeof(gPlatHotKeyCtx[0]);Index++){
    CopyMem(&KeyData, &gPlatHotKeyCtx[Index].Key, sizeof(EFI_INPUT_KEY));		
    Status = TextInEx->RegisterKeyNotify(TextInEx, &KeyData, KeyNotifyCallBack, &gPlatHotKeyCtx[Index].RegHandle);
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_ERROR, "HotKey[%d] Reg:%r\n", Index, Status));
    }  
  }		

ProcExit:
  return;	
}



STATIC VOID InvokeTxtScreen()
{
  EFI_BOOT_MANAGER_LOAD_OPTION  BootOption;

  CreateFvBootOption(
    (EFI_GUID*)PcdGetPtr(PcdTxtScreenFileGuid), 
    L"", 
    &BootOption, 
    FALSE, 
    NULL, 
    0
    );
  EfiBootManagerBoot(&BootOption);
  EfiBootManagerFreeLoadOption(&BootOption);
}


STATIC VOID PlatformHotKeyBoot()
{
  EFI_BOOT_MANAGER_LOAD_OPTION  BootOption;
  UINTN                         Index;
	
  for(Index=0;Index<sizeof(gPlatHotKeyCtx)/sizeof(gPlatHotKeyCtx[0]);Index++){
    if(gHotkeyBootType == gPlatHotKeyCtx[Index].BootType && gPlatHotKeyCtx[Index].FileName!=NULL){
      CreateFvBootOption(gPlatHotKeyCtx[Index].FileName, L"", &BootOption, FALSE, NULL, 0);
      EfiBootManagerBoot(&BootOption);
      EfiBootManagerFreeLoadOption(&BootOption);
			break;
    }			
  }
}


STATIC VOID UnregisterPlatfromHotKey()
{
  UINTN                              Index;
  EFI_STATUS                         Status;	
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL	 *TextInEx;

  Status = gBS->HandleProtocol(gST->ConsoleInHandle, &gEfiSimpleTextInputExProtocolGuid, (VOID**)&TextInEx);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }		
	
  for(Index=0;Index<sizeof(gPlatHotKeyCtx)/sizeof(gPlatHotKeyCtx[0]);Index++){
    if(gPlatHotKeyCtx[Index].RegHandle != NULL){
  	  Status = TextInEx->UnregisterKeyNotify(TextInEx, gPlatHotKeyCtx[Index].RegHandle);
      if(EFI_ERROR(Status)){      
        DEBUG((EFI_D_ERROR, "HotKey[%d] UnReg:%r\n", Index, Status));
      }  
    }  
  }

ProcExit:
  return;
}



STATIC 
EFI_STATUS 
EfiBootManagerGetLoadOptions2 (
  OUT EFI_BOOT_MANAGER_LOAD_OPTION   **Option,
  OUT UINTN                          *OptionCount
)
{
  *Option = EfiBootManagerGetLoadOptions(OptionCount, LoadOptionTypeBoot);
  if(*Option == NULL || *OptionCount == 0){
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EfiBootManagerCreateSetupBootOption (
  OUT EFI_BOOT_MANAGER_LOAD_OPTION   *SetupOption
)
{
  EFI_STATUS  Status;

  if(SetupOption == NULL){
    return EFI_INVALID_PARAMETER;
  }  

  Status = CreateFvBootOption (
             (EFI_GUID*)PcdGetPtr(PcdSetupFileGuid),
             L"Enter Setup",
             SetupOption,
             FALSE,             // IsBootCategory
             NULL,              // OptionalData
             0                  // OptionalDataSize
             );
  
  return Status;
}


EFI_BDS_BOOT_MANAGER_PROTOCOL gBdsBootManagerProtocol = {
  EfiBootManagerConnectAll,
  EfiBootManagerRefreshAllBootOption,
  EfiBootManagerGetLoadOptions2,
  EfiBootManagerFreeLoadOptions,
  CreateFvBootOption,
  EfiBootManagerCreateSetupBootOption,
  EfiBootManagerBoot
};  


/**

  Service routine for BdsInstance->Entry(). Devices are connected, the
  consoles are initialized, and the boot options are tried.
  @param This             Protocol Instance structure.

**/
VOID
EFIAPI
BdsEntry (
  IN EFI_BDS_ARCH_PROTOCOL  *This
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION    *DriverOption;
  UINTN                           DriverOptionCount;
  CHAR16                          *FirmwareVendor;
  EFI_EVENT                       HotkeyTriggered;
  EFI_STATUS                      Status;
  EFI_BOOT_MODE                   BootMode;


  PERF_END (NULL, "DXE", NULL, 0);
  PERF_START (NULL, "BDS", NULL, 0);

  DEBUG ((EFI_D_INFO, __FUNCTION__"\n"));

	BootMode = GetBootModeHob();
  gSetupData = (SETUP_DATA*)GetSetupDataHobData();

  //
  // Fill in FirmwareVendor and FirmwareRevision from PCDs
  //
  FirmwareVendor = (CHAR16 *) PcdGetPtr (PcdFirmwareVendor);
  gST->FirmwareVendor = AllocateRuntimeCopyPool (StrSize (FirmwareVendor), FirmwareVendor);
  ASSERT (gST->FirmwareVendor != NULL);
  gST->FirmwareRevision = PcdGet32 (PcdFirmwareRevision);

  //
  // Fixup Tasble CRC after we updated Firmware Vendor and Revision
  //
  gST->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 ((VOID *) gST, sizeof (EFI_SYSTEM_TABLE), &gST->Hdr.CRC32);

  InitializeHwErrRecSupport ();

  InitializeLanguage (TRUE);

  EfiBootManagerStartHotkeyService (&HotkeyTriggered);
  
  PERF_START (NULL, "BeforeConsole", "BDS", 0);
  PlatformBootManagerBeforeConsole();
  PERF_END   (NULL, "BeforeConsole", "BDS", 0);

  DriverOption = EfiBootManagerGetLoadOptions (&DriverOptionCount, LoadOptionTypeDriver);
  LoadDrivers (DriverOption, DriverOptionCount);
  EfiBootManagerFreeLoadOptions (DriverOption, DriverOptionCount);
  
  PERF_START (NULL, "ConnectConsoles", "BDS", 0);
  EfiBootManagerConnectAllDefaultConsoles();
  PERF_END   (NULL, "ConnectConsoles", "BDS", 0);




// ----------------------------- HotKey -----------------------------
  if(BootMode != BOOT_ON_FLASH_UPDATE && BootMode != BOOT_IN_RECOVERY_MODE){
    RegisterPlatformHotKey();
    PrintBootPrompt();
    BdsReadKeys();

    PERF_START (NULL, "BdsWait", "BDS", 0);
    BdsWait(HotkeyTriggered);
    PERF_END   (NULL, "BdsWait", "BDS", 0);

    CleanBootPrompt();
    UnregisterPlatfromHotKey();
  }


// -------------------------- afterConsole --------------------------
  PERF_START (NULL, "AfterConsole", "BDS", 0);
  PlatformBootManagerAfterConsole();
  PERF_END   (NULL, "AfterConsole", "BDS", 0);

  //
  // Clear/Restore Key####/Timeout per platform request
  // For Timeout, variable change is enough to control the following behavior
  // For Key####, variable change is not enough so we still need to check the *_ENABLE_HOTKEY later
  //
  BdsUpdateVariable (L"KeyOrder", NULL, &gByoGlobalVariableGuid);

#if !defined(MDEPKG_NDEBUG)
  DEBUG((EFI_D_ERROR,"This may be not true Boot Option, Please ref another boot option dump \n"));
  DumpBootOptions();
#endif

//------------------------------------------------------------
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gImageHandle,
                  &gEfiBootManagerProtocolGuid, &gBdsBootManagerProtocol,
                  NULL
                  );



//InvokeTxtScreen();

  CheckSetupPassword();
  
  EfiBootManagerHotkeyBoot();
  PlatformHotKeyBoot();			
  
  DEBUG((EFI_D_ERROR,"bf DefaultBootBehavior() \n"));
  DefaultBootBehavior();
  CpuDeadLoop();	
}


