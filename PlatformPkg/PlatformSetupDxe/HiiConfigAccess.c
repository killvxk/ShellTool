
#include "PlatformSetup.h"

#define GUID_CONFIG_STRING_TYPE 0x00
#define NAME_CONFIG_STRING_TYPE 0x01
#define PATH_CONFIG_STRING_TYPE 0x02

EFI_GUID tseNvramUpdateGuid          = TSE_NVRAM_UPDATE_GUID;

CHAR16* VarStoreVariableNameList[]  = {
  PLATFORM_SETUP_VARIABLE_NAME, 
  L"TSESetup", 
  SETUP_VOLATILE_VARIABLE_NAME, 
  L"PlatformLang",
  L"PlatformLangCodes", 
  NULL
  };


//
// Hii vendor device path template
//
HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePathTemplate = {
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8) (sizeof (HII_VENDOR_DEVICE_PATH_NODE)),
          (UINT8) ((sizeof (HII_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EFI_IFR_TIANO_GUID,
    },
    0,
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH
    }
  }
};

EFI_STATUS
InternalHiiGetBufferFromString (
  IN  EFI_STRING                 String,
  IN  UINT8                      Flag,
  OUT UINT8                      **Buffer
  );

BOOLEAN
IsVariableNameInSupportedList (
  IN CHAR16 *VariableName
  )
{
  UINTN Index;

  if (VariableName == NULL) {
    return FALSE;
  }
  
  for (Index = 0; VarStoreVariableNameList[Index] != NULL; Index ++) {
    if (StrCmp (VarStoreVariableNameList[Index], VariableName) == 0) {
      return TRUE;
    }
  }
  
  return FALSE;
}

EFI_STATUS
EFIAPI
ExtractGuidNameFromConfigHdr (
  IN CONST EFI_STRING  ConfigHdr,
  OUT      EFI_GUID    *Guid,
  OUT      CHAR16      **Name,
  OUT      UINTN       *Offset
  )
{
  EFI_STATUS     Status;
  CHAR16         *StrPtr;
  UINTN          Length;
  CHAR16         *StrName;
  UINT8          *Buffer;

  *Offset = 0;
  //
  // <ConfigHdr> ::=
  // GUID=<HexCh>32&NAME=<Char>NameStrLen&PATH=<HexChar>DevicePathStrLen <NULL>
  // | 5  |   32   |  6  |  NameStrLen*4 |  6  |    DevicePathStrLen    | 1 |
  //
  if (StrLen (ConfigHdr) <= (5 + 32 + 6)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // check syntax for guid
  //
  StrPtr = ConfigHdr + 5 + 32;
  if (*StrPtr != L'&') {
    return EFI_INVALID_PARAMETER;
  }
  *StrPtr = L'\0';

  //
  // extract guid
  //
  Status = InternalHiiGetBufferFromString (
             ConfigHdr + 5, 
             GUID_CONFIG_STRING_TYPE, 
             (UINT8 **) &Buffer
             );
  *StrPtr = L'&';
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "HII Config Access: Guid extraction failed\n"));
    *Offset = 5;
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (Guid, Buffer, sizeof (EFI_GUID));
  FreePool (Buffer);
  

  //
  // check syntax for name
  //
  StrPtr = ConfigHdr + 5 + 32 + 6;
  while (*StrPtr != L'\0' && *StrPtr != L'&') {
    StrPtr++;
  }
  if (*StrPtr != L'&') {
    *Offset = 5 + 32 + 6;
    return EFI_INVALID_PARAMETER;
  }

  //
  // extract name
  //
  *StrPtr = L'\0';
  Length  = StrSize (ConfigHdr + 5 + 32 + 6);
  StrName = AllocatePool (Length);
  if (StrName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Status = InternalHiiGetBufferFromString (
             ConfigHdr + 5 + 32 + 6,
             NAME_CONFIG_STRING_TYPE, 
             (UINT8 **) &Buffer
             );

  
  *StrPtr = L'&';
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "HII Config Access: Name extraction failed\n"));
    *Offset = 5 + 32 + 6;
    return EFI_INVALID_PARAMETER;
  }

  StrCpy (StrName, (CONST CHAR16 *)Buffer);
  FreePool (Buffer);
    
  if (!IsVariableNameInSupportedList (StrName)) {
    return EFI_NOT_FOUND;
  }
  *Name = StrName;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiAccessExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN CONST EFI_STRING                     Request,
  OUT EFI_STRING                          *Progress,
  OUT EFI_STRING                          *Results
  )
{
  EFI_STATUS                      Status;
  UINTN                           Length;
  EFI_GUID                        Guid;
  CHAR16                          *Name;
  UINT8                           *Var;
  UINTN                           Offset;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_STRING                      StringPtr;
  EFI_STRING                      ConfigRequestHdr;
  EFI_STRING                      ConfigRequest;
  BOOLEAN                         AllocatedRequest;

                     
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ConfigRequest = Request;
  *Progress = Request;
  AllocatedRequest = FALSE;
  
  if (ConfigRequest == NULL) {
    HiiHandle = _CR (This, SETUP_CALLBACK_INFO, pFormCallback)->HiiHandle;
    Status = gHiiDatabase->GetPackageListHandle (gHiiDatabase, HiiHandle, &DriverHandle);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
    
    CopyMem (&Guid, &gPlatformSetupVariableGuid, sizeof (gPlatformSetupVariableGuid));
    if (gIsSetupEntered && !gIsSetToDefault) {
      Name = AllocateCopyPool (
               StrSize (SETUP_TEMP_VARIABLE_NAME), 
               SETUP_TEMP_VARIABLE_NAME
               ); 
    } else {
      Name = AllocateCopyPool (
               StrSize (PLATFORM_SETUP_VARIABLE_NAME), 
               PLATFORM_SETUP_VARIABLE_NAME
               ); 
    } 
       
    //
    // Construct <ConfigHdr> : "GUID=...&NAME=...&PATH=..."
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&Guid, Name, DriverHandle);
    ASSERT (ConfigRequestHdr != NULL);
    if (ConfigRequestHdr == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    Length = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocatePool(Length);
    ASSERT (ConfigRequest != NULL);
    if (ConfigRequest == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    UnicodeSPrint (
      ConfigRequest, 
      Length, 
      L"%s&OFFSET=0&WIDTH=%016LX", 
      ConfigRequestHdr, 
      sizeof (SETUP_DATA)
      );
    FreePool (ConfigRequestHdr);
    AllocatedRequest = TRUE;
  } else {
  
    Status = ExtractGuidNameFromConfigHdr (ConfigRequest, &Guid, &Name, &Offset);
    if (EFI_ERROR (Status)) {
      *Progress = ConfigRequest + Offset;
      return Status;
    }

    //
    // Check whether Request includes Request Element.
    //
    if (StrStr (ConfigRequest, L"OFFSET") == NULL) {
      //
      // Check Request Element does exist in Reques String
      //
      StringPtr = StrStr (ConfigRequest, L"PATH");
      if (StringPtr == NULL) {
        return EFI_INVALID_PARAMETER;
      }
      if (StrStr (StringPtr, L"&") == NULL) {
        
        Length = (StrLen (ConfigRequest) + 32 + 1) * sizeof (CHAR16);
        ConfigRequest = AllocatePool (Length);

        //
        // Construct <ConfigHdr> : "GUID=...&NAME=...&PATH=..."
        //
        UnicodeSPrint (
          ConfigRequest, 
          Length, 
          L"%s&OFFSET=0&WIDTH=%016LX", 
          Request, 
          sizeof (SETUP_DATA)
          );

        AllocatedRequest = TRUE;
      }
    }
  }

  //
  // get variable by name and guid from nvram
  //
  Length  = 0;
  Var     = NULL;
  if ((StrCmp (Name, PLATFORM_SETUP_VARIABLE_NAME) == 0) && 
       gIsSetupEntered && 
       !gIsSetToDefault) {
    FreePool (Name);
    Name = AllocateCopyPool (
             StrSize (SETUP_TEMP_VARIABLE_NAME), 
             SETUP_TEMP_VARIABLE_NAME
             ); 
  }
  
  Status  = GetEfiVariable (Name, &Guid, NULL, &Length, &Var);
  if (EFI_ERROR (Status)) {
    if (AllocatedRequest) {
      FreePool (ConfigRequest);
    }
    *Progress = Request;
    FreePool (Name);
    return Status;
  }

  Status = gHiiConfigRouting->BlockToConfig (gHiiConfigRouting, ConfigRequest, Var, Length, Results, Progress);
  FreePool (Name);
  FreePool (Var);
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    //
    // Set Progress string to the original request string.
    //
    if (Request == NULL) {
      *Progress = NULL;
    } else {
      *Progress = Request + StrLen (Request);
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
HiiAccessRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN  CONST EFI_STRING                     Configuration,
  OUT       EFI_STRING                     *Progress
  )
{
  EFI_STATUS                      Status;
  UINTN                           Length;
  UINTN                           VarSize;
  UINTN                           Offset;
  EFI_GUID                        Guid;
  CHAR16                          *Name;
  UINT8                           *Var;
  EFI_HANDLE                      Handle;
  BOOLEAN                         IsTempVariableWrite;
  UINT32                          Attributes;

  if (Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Configuration;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ExtractGuidNameFromConfigHdr (Configuration, &Guid, &Name, &Offset);
  if (EFI_ERROR (Status)) {
    *Progress = Configuration + Offset;
    return Status;
  }

  //
  // get variable by name and guid from nvram
  //
  Var     = NULL;
  Length  = 0;
  IsTempVariableWrite = FALSE;
  if ((StrCmp (Name, PLATFORM_SETUP_VARIABLE_NAME) == 0) && 
       gIsSetupEntered && 
       !gIsSetToDefault) {
    FreePool (Name);
    Name = AllocateCopyPool (
             StrSize (SETUP_TEMP_VARIABLE_NAME), 
             SETUP_TEMP_VARIABLE_NAME
             ); 
    IsTempVariableWrite = TRUE;
  }

  Status = GetEfiVariable (Name, &Guid, NULL, &Length, &Var);
  if (EFI_ERROR (Status)) {
    //
    // requested variable not found
    //
    Var = NULL;
    Length = 0;
    Status  = gHiiConfigRouting->ConfigToBlock (gHiiConfigRouting, Configuration, Var, &Length, Progress);
    if (Status == EFI_INVALID_PARAMETER) {
      Var = AllocatePool (Length);
      if (Var == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      return EFI_NOT_FOUND;
    }
  }

  VarSize = Length;
  Status  = gHiiConfigRouting->ConfigToBlock (gHiiConfigRouting, Configuration, Var, &Length, Progress);
  while (Status == EFI_DEVICE_ERROR) {
    //
    // block not large enough
    //
    if (Var != NULL) {
      FreePool (Var);
    }

    VarSize = Length;
    Var  = AllocatePool (Length);
    if (Var == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status  = gHiiConfigRouting->ConfigToBlock (gHiiConfigRouting, Configuration, Var, &Length, Progress);
  }

  if (EFI_ERROR (Status)) {
    FreePool (Var);
    return Status;
  }

  if (!IsTempVariableWrite) 
    Attributes = EFI_VARIABLE_NON_VOLATILE       | 
                 EFI_VARIABLE_BOOTSERVICE_ACCESS | 
                 EFI_VARIABLE_RUNTIME_ACCESS;
  else
    Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | 
                 EFI_VARIABLE_RUNTIME_ACCESS;
  gRT->SetVariable (
         Name,
         &Guid,
         Attributes,
         VarSize,
         Var
         );
  
  FreePool (Name);
  FreePool (Var);


  if (!gIsSetToDefault) {
    //
    // Trigger tseNvramUpdateGuid event when save value on UI.
    // The code is in Greens Glacier SetupBrowser driver but as we are using Tiano SetupBrowser driver, move
    // the code here.
    //
    Handle = NULL;
    Status = gBS->InstallProtocolInterface (
                    &Handle,
                    &tseNvramUpdateGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  
    Status = gBS->UninstallProtocolInterface (
                    Handle,
                    &tseNvramUpdateGuid,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiAccessFormCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN  EFI_BROWSER_ACTION                   Action,
  IN  EFI_QUESTION_ID                      QuestionId,
  IN  UINT8                                Type,
  IN  EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  )
{
  return EFI_UNSUPPORTED;
}

EFI_HII_CONFIG_ACCESS_PROTOCOL DefaultConfigAccess = {
  HiiAccessExtractConfig,
  HiiAccessRouteConfig,
  HiiAccessFormCallback
};

VOID
EFIAPI
UpdateCallbackInfo (
  SETUP_CALLBACK_INFO *pCallbackInfo
  )
{
  if (pCallbackInfo->pFormCallback != NULL) {
    if (pCallbackInfo->pFormCallback->ExtractConfig == NULL) {
      pCallbackInfo->pFormCallback->ExtractConfig = HiiAccessExtractConfig;
    }
    if (pCallbackInfo->pFormCallback->RouteConfig == NULL) {
      pCallbackInfo->pFormCallback->RouteConfig = HiiAccessRouteConfig;
    }
    if (pCallbackInfo->pFormCallback->Callback == NULL) {
      pCallbackInfo->pFormCallback->Callback = HiiAccessFormCallback;
    }
  } else {
    pCallbackInfo->pFormCallback = &DefaultConfigAccess;
  }
}


EFI_STATUS
CreateHiiDriverHandle (
  OUT EFI_HANDLE               *DriverHandle
  )
/*++

Routine Description:
  The HII driver handle passed in for HiiDatabase.NewPackageList() requires
  that there should be DevicePath Protocol installed on it.
  This routine create a virtual Driver Handle by installing a vendor device
  path on it, so as to use it to invoke HiiDatabase.NewPackageList().

Arguments:
  DriverHandle         - Handle to be returned

Returns:
  EFI_SUCCESS          - Handle destroy success.
  EFI_OUT_OF_RESOURCES - Not enough memory.

--*/
{
  EFI_STATUS                   Status;
  HII_VENDOR_DEVICE_PATH_NODE  *VendorDevicePath;

  VendorDevicePath = AllocateCopyPool (sizeof (HII_VENDOR_DEVICE_PATH), &mHiiVendorDevicePathTemplate);
  if (VendorDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Use memory address as unique ID to distinguish from different device paths
  //
  VendorDevicePath->UniqueId = (UINT64) ((UINTN) VendorDevicePath);

  *DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  VendorDevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
