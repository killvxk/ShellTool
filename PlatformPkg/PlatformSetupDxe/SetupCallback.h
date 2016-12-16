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

  SetupCallback.h

Abstract:

  Internal header of the Setup Component.
  
Revision History:

Bug 2733: Can not configured TPM option in BIOS TPM Setup menu .
TIME: 2011-8-16
$AUTHOR: Ken Zhu
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Add one new function InitTpmStrings() define.
$END--------------------------------------------------------------------
--*/

#ifndef _SETUP_CALLBACK_H_
#define _SETUP_CALLBACK_H_

#define _CR(Record, TYPE, Field)  ((TYPE *) ((CHAR8 *) (Record) - (CHAR8 *) &(((TYPE *) 0)->Field)))
//
//
//
typedef 
VOID 
(* UPDATE_ITEM) (
  EFI_HII_HANDLE HiiHandle, 
  UINT16         Class, 
  UINT16         SubClass, 
  UINT16         Key
  );

typedef struct _SETUP_ITEM_CALLBACK{
  UINT16      Class;
  UINT16      SubClass;
  UINT16      Key;
  UPDATE_ITEM UpdateItem;
} SETUP_ITEM_CALLBACK;  
  
 
typedef 
EFI_STATUS 
(* UPDATE_ITEM_EX) (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );

typedef struct SETUP_ITEM_CALLBACK_EX_{
  UINT16         Class;
  UINT16         SubClass;
  UINT16         Key;
  UPDATE_ITEM_EX UpdateItemEx;
} SETUP_ITEM_CALLBACK_EX;

EFI_STATUS
FwUpdOptionChg (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );

EFI_STATUS
MngStateOptionChg (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );

EFI_STATUS
MeStateChgEvent (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );

EFI_STATUS
IccMenuEntry (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );

EFI_STATUS
IccSubmenuEntry (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );

EFI_STATUS
IccAccept (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );

EFI_STATUS
IccFreqChange  (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );

EFI_STATUS
OcFormCallBack  (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );


EFI_STATUS
CpuFormCallBack (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );

EFI_STATUS
EFIAPI
ExitFormCallback(
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  );

EFI_STATUS
EFIAPI
DevicesFormCallback(
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  );


EFI_STATUS
EFIAPI
PowerFormCallback(
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  );
EFI_STATUS
ResetFormCallback (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This, 
  EFI_BROWSER_ACTION                   Action, 
  EFI_QUESTION_ID                      KeyValue, 
  UINT8                                Type, 
  EFI_IFR_TYPE_VALUE                   *Value, 
  EFI_BROWSER_ACTION_REQUEST           *ActionRequest 
  );

typedef struct _SETUP_CALLBACK_INFO {
  EFI_GUID                       *pGuid;
  EFI_HII_CONFIG_ACCESS_PROTOCOL *pFormCallback;
  UINT16                         Class;
  UINT16                         SubClass;
  EFI_HII_HANDLE                 HiiHandle;
} SETUP_CALLBACK_INFO;

typedef 
VOID
(*INIT_HII_PACK) (
  EFI_HII_HANDLE       HiiHandle,
  SETUP_CALLBACK_INFO  *pCallBackFound
);

//
//
//
typedef struct _SETUP_CALLBACK {
  EFI_HII_CONFIG_ACCESS_PROTOCOL Callback;
  UINT16                         Class;
  UINT16                         SubClass;
} SETUP_CALLBACK;


//
//
//
typedef 
VOID
(* INIT_STRINGS_FUNCTION) (
  EFI_HII_HANDLE HiiHandle, 
  UINT16         Class
  );
 

VOID 
InitPciBusStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );

VOID 
InitCpuStrings(
  EFI_HII_HANDLE    HiiHandle, 
  UINT16            Class
);
  
VOID 
InitAcpiStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID
InitCsmStrings  (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );

VOID  
InitTerminalStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID 
InitSioStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );

VOID
NatInitSioStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID 
InitSioStrings_WPCN381U (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
   
VOID
InitCPUStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID
InitNBStrings  (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );

VOID  
InitSBStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID  
InitHhmStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID
InitHhmMobileStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID    
InitIdeSecurityStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID    
InitMeInfo (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID    
InitICCStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID    
InitOverClockStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID    
InitPlatformInfoStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );
  
VOID  
InitBoardStrings (
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Class
  );

  
extern UINT8               gSetupCallbackInfoNumber;
extern SETUP_CALLBACK_INFO gSetupCallbackInfo[];
#endif