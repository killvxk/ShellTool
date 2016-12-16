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

  SetupStrings.h

Abstract:

  Internal header of the Setup Component.
--*/

#ifndef __SETUP__STRINGS_H__
#define __SETUP__STRINGS_H__

typedef UINT16 STRING_REF;

#define LastLangCodes             L"PlatformLastLangCodes"
#define LastLang                  L"PlatformLastLang"
#define NativeLangCodes           L"PlatformLangCodes"
#define CompatLangCodes           L"LangCodes"
#define NativeLang                L"PlatformLang"
#define CompatLang                L"Lang"
#define DEFAULT_LANGUAGE_CODE     "en-us"

VOID 
InitString (
  EFI_HII_HANDLE HiiHandle,
  STRING_REF     StrRef, 
  CHAR16         *sFormat, ...
  );

VOID 
InitString2 (
  EFI_HII_HANDLE HiiHandle, 
  STRING_REF     StrRef, 
  CHAR8         *String
  );

VOID 
InitSetupStrings (
  EFI_HII_HANDLE HiiHandle,
  SETUP_CALLBACK_INFO *pCallBackFound
  );
  
EFI_STATUS
GetEfiVariable (
  IN     CHAR16    *sName, 
  IN     EFI_GUID  *pGuid,
  OUT    UINT32    *pAttributes OPTIONAL,
  IN OUT UINTN     *pDataSize, 
  OUT    VOID      **ppData
  );
  
VOID
InitLanguages (
  EFI_HII_HANDLE HiiHandle
  );
    
VOID 
SynchornizeLanguageVariables (
);  

VOID
UpdateLanguageSettingItem (
  EFI_HII_HANDLE HiiHandle
  );

EFI_STATUS
SetLanguageCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest  OPTIONAL
  );
  
  extern EFI_GUID gAsiaCpuCfgHobGuid;
#endif