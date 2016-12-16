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

  SetupPrivate.h

Abstract:

  Internal header of the Setup Component.
--*/

#ifndef _PLATFORM_SETUP_H_
#define _PLATFORM_SETUP_H_

#define EFI_FORWARD_DECLARATION(x)

#include <Protocol/Variable.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiString.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/SmbusHc.h>
#include <Protocol/AcpiSupport.h>
#include <Protocol/SerialIo.h>
#include <Protocol/Cpu.h>
#include <Protocol/MpService.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/ByoFormBrowserEx.h>
#include <Protocol/ByoFormSetManager.h>
#include <Protocol/SetupSaveNotify.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <library/DxeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PerformanceLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/DevicePathLib.h>
#include <Library/Pcdlib.h>

#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>
#include <Guid/HobList.h>
#include <IndustryStandard/Acpi.h>

#include <Library/BaseLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>

#include "VfrExtension.h"
#include "PlatformDefinition.h"
#include "SetupItemId.h"
#include "SetupCallback.h"
#include "SetupStrings.h"

#include <Guid/MdeModuleHii.h>


//
// Generic Definitions                            
//
#define BDS_ALL_DRIVERS_CONNECTED_PROTOCOL_GUID \
  { \
  0xdbc9fd21, 0xfad8, 0x45b0, 0x9e, 0x78, 0x27, 0x15, 0x88, 0x67, 0xcc, 0x93 \
  }

//
// TSE Notifications
//

#define TSE_AFTER_FIRST_BOOT_OPTION_GUID \
  { 0xC48D651C, 0x9D0E, 0x4ce7, 0xAD, 0x39, 0xED, 0xD1, 0xAB, 0x83, 0x6B, 0x30 }

#define TSE_NVRAM_UPDATE_GUID \
{ 0xd84beff0, 0x159a, 0x4b60, 0x9a, 0xb9, 0xac, 0x5c, 0x47, 0x4b, 0xd3, 0xb1 }

//
// GUIDed opcodes defined for Tiano
//
#define EFI_IFR_TIANO_GUID \
  { 0xf0b1735, 0x87a0, 0x4193, {0xb2, 0x66, 0x53, 0x8c, 0x38, 0xaf, 0x48, 0xce} }
  

#pragma pack(1)

typedef struct _FORM_SET_INFO {
  EFI_GUID    FormSetGuid;
  UINT8       *IfrPack;
  UINT16      Class;
  UINT16      SubClass;
  EFI_HANDLE  DriverHandle;
} FORM_SET_INFO;

typedef struct {
  VENDOR_DEVICE_PATH  VendorDevicePath;
  UINT32              Reserved;
  UINT64              UniqueId;
} HII_VENDOR_DEVICE_PATH_NODE;


#pragma pack()


extern EFI_GUID            MainFormSetGuid;
extern EFI_GUID            AdvancedFormSetGuid;
extern EFI_GUID            DevicesFormSetGuid;
extern EFI_GUID            PowerFormSetGuid;
extern EFI_GUID            BootFormSetGuid;
extern EFI_GUID            SecurityFormSetGuid;
extern EFI_GUID            ExitFormSetGuid;
extern EFI_GUID            ResetFormSetGuid;
extern EFI_GUID            gSetupEnterGuid;
extern BOOLEAN             gIsSetToDefault;
extern BOOLEAN             gIsSetupEntered;
extern CHAR16              *VarStoreVariableNameList[];
extern EFI_GUID            gMemInfoProtocolGuid;
extern EFI_GUID            gDxePlatformSaPolicyGuid;
extern SETUP_VOLATILE_DATA gSetupVolatileData;
extern FORM_SET_INFO       gSetupFormSets[];


//
// HII packages
//
extern UINT8               PlatformSetupDxeStrings[];
extern UINT8               MainBin[];
extern UINT8               AdvancedBin[];
extern UINT8               DevicesBin[];
extern UINT8               SecurityBin[];
extern UINT8               ExitBin[];
extern UINT8               PowerBin[];


extern EFI_BYO_FORM_BROWSER_EXTENSION_PROTOCOL *mFormBrowserEx;

typedef struct _HII_VENDOR_DEVICE_PATH{
  HII_VENDOR_DEVICE_PATH_NODE    Node;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

EFI_STATUS
PasswordFormCallback (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );


//
// CMOS test
//
BOOLEAN
IsCMOSBad (
  );
  
VOID
ClearCmos0E (
  );

VOID
EFIAPI
UpdateCallbackInfo (
  );

EFI_STATUS
CreateHiiDriverHandle (
  OUT EFI_HANDLE               *DriverHandle
  );

VOID
PlatformReset (
  VOID
  );


/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  );


#endif
                                                                                   
