/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SmiVariable.h

Abstract:

 This file contains the Includes, Definitions, typedefs,
 Variable and External Declarations, Structure and
 function prototypes needed for the SmiVariable driver
--*/
//
// {1B838190-4625-4ead-ABC9-CD5E6AF18FE0}
//
#include <Base.h>

#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/LegacyInterruptHandler.h>
#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmCpu.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/DxeServicesLib.h>

#include <Uefi/UefiInternalFormRepresentation.h>

#include <Framework/SmmCis.h>
#include <Guid/SetupPassword.h>
#include <SetupVariable.h>

#define MAXVARNAME  128
#define MAXVARLIST  128
#define MAXVARPAES  4


#define EFI_HII_EXPORT_DATABASE_GUID { 0x1b838190, 0x4625, 0x4ead, 0xab, 0xc9, 0xcd, 0x5e, 0x6a, 0xf1, 0x8f, 0xe0 }

#define SW_SMI_VARIABLE_SERVICES	      0x71
#define SW_SMI_VARIABLE_SERVICES_INIT   0x70
                                        
#define GET_VARIABLE_SUBFUNCTION        0x01
#define GET_NEXT_VARIABLE_SUBFUNCTION   0x02
#define SET_VARIABLE_SUBFUNCTION        0x03
#define EXPORT_HII_DB_SUBFUNCTION       0x04
#define COPY_MEMORY_SUBFUNCTION         0x05
#define CHECK_BIOS_PASSWORD_SUBFUNCTION 0x06
#define REQUIRE_PASSWORD_ITEM             0x01
#define CHECK_BIOS_PASSWORD               0x02

//---------------------------------------------------------------------------
//
// Export HII DB information
//
EFI_GUID                              mEfiExportDatabaseGuid = EFI_HII_EXPORT_DATABASE_GUID;
CHAR16                                mEfiExportDatabaseName[] = L"HiiDB";
VOID                                  *mHiiDbAddress;

#pragma pack(push, 1)
typedef struct {
  EFI_GUID HiiProtocolGuid;
  UINT32   HiiDatabaseSize;
} EFI_HII_PACKAGE_EXPORT_HEADER;
#pragma pack(pop)


//---------------------------------------------------------------------------
// Name: ErrorCodes
//
// Description:	
// This table contains the list of error codes are supported by this function
//
// Fields: Value Name Description 
//---------------------------------------------------------------------------
//
// 00h NO_ERROR No error. Maps to EFI_SUCCESS
// 82h INVALID_VALUE One of the parameters has an invalid value. Maps to EFI_INVALID_PARAMETER.
// 85h BUFFER_TOO_SMALL The VariableNameSize is too small for the result. VariableNameSize has been updated with the size needed to complete the request. Maps to EFI_BUFFER_TOO_SMALL.
// 86h NOT_SUPPORTED Interface not supported. Maps to standard INT 0x15 error codes.
// 87h DEVICE_ERROR The variable could not be retrieved due to a hardware error. Maps to EFI_DEVICE_ERROR.
// 8Eh NOT_FOUND The variable was not found. Maps to EFI_NOT_FOUND.
//
//---------------------------------------------------------------------------

#define NO_ERROR         0x00
#define INVALID_VALUE    0x82
#define BUFFER_TOO_SMALL 0x85
#define NOT_SUPPORTED    0x86
#define DEVICE_ERROR     0x87
#define NOT_FOUND        0x8e

#define CARRY_FLAG       0x00000001

#if TEST_INT15h
#undef CARRY_FLAG
#include <Protocol/LegacyBios.h>
#pragma pack(1)
typedef struct _ASM_VARIABLE_BLOCK {
    EFI_GUID VendorGuid;
    UINT16 NameSegment;
    UINT16 NameOffset;
    UINT32 Attributes;
    UINT32 DataSize;
    UINT16 DataSegment;
    UINT16 DataOffset;
} ASM_VARIABLE_BLOCK;

typedef struct _MEM_POOL {
  CHAR16             Name[4];
  UINT32             Data;
  ASM_VARIABLE_BLOCK Block;
} MEM_POOL;
#pragma pack()

#endif
//---------------------------------------------------------------------------

#pragma pack(1)

//---------------------------------------------------------------------------
// Name: VARIABLE_BLOCK
//
// Description: Description of the VARIABLE_BLOCK variables
//
// Fields: Name Type Description
//---------------------------------------------------------------------------
//
// VendorGuid EFI_GUID A unique identifier for the vendor.
// VariableName UINT32 32-bit physical address of the null-terminated Unicode string that is the name of the vendor's variable. The caller must ensure natural alignment.
// VariableNameSize UINT32 On input, the size in bytes of the Variable Name buffer. Updated with the actual size needed if the input size was too small.
// Attributes UINT32 The attributes bitmask for the variable. This information is always returned to the caller. The corresponding EFI function defined this as an optional parameter. The attributes are defined in the EFI specification.
// DataSize UINT32 On input, the size in bytes of the return Data buffer. On output the size of data returned in Data.
// Data UINT32 32-bit physical address of the buffer for returning the value of the variable. No specific alignment requirements exist.
//
//---------------------------------------------------------------------------

typedef struct _VARIABLE_BLOCK {
    EFI_GUID VendorGuid;
    UINT32   VariableName;
    UINT32   Attributes;
    UINT32   DataSize;
    UINT32   Data;
} VARIABLE_BLOCK;


//---------------------------------------------------------------------------
// Name: GET_NEXT_VARIABLE_BLOCK
//
// Description: Description of the GET_NEXT_VARIABLE_BLOCK variables
//
// Fields: Name Type Description
//---------------------------------------------------------------------------
//
// VendorGuid EFI_GUID A unique identifier for the vendor.
// VariableName UINT32 32-bit physical address of the null-terminated Unicode string that is the name of the vendor's variable. The caller must ensure natural alignment.
// VariableNameSize UINT32 On input, the size in bytes of the Variable Name buffer. Updated with the actual size needed if the input size was too small.
//
//---------------------------------------------------------------------------

typedef struct _GET_NEXT_VARIABLE_BLOCK {
    EFI_GUID VendorGuid;
    UINT32   VariableName;
    UINT32   VariableNameSize;
} GET_NEXT_VARIABLE_BLOCK;

//---------------------------------------------------------------------------
// Name: COPY_MEMORY_BLOCK
//
// Description: Parameter block used in COPY_MEMORY_SUBFUNCTION
//
// Fields: 
//
//  UINT32  Source       - 32-bit physical address of the source
//  UINT32  Destination  - 32-bit physical address of the destination
//  UINT32  Size         - Number of bytes to copy
//---------------------------------------------------------------------------

typedef struct _COPY_MEMORY_BLOCK {
  UINT32  Source;
  UINT32  Destination;
  UINT32  Size;
} COPY_MEMORY_BLOCK;

typedef struct _CHECK_BIOS_PASSWORD_BLOCK {
  UINT8   SubFun;
  UINT8   RequireAdminPsw;
  CHAR16  Password[1];
} CHECK_BIOS_PASSWORD_BLOCK;

typedef struct _REG_BLOCK {
    UINT32 EAX;
    UINT32 EBX;
    UINT32 ECX;
    UINT32 EDX;
    UINT32 ESI;
    UINT32 EDI;
    UINT32 EFLAGS;
    UINT16 ES;
    UINT16 CS;
    UINT16 SS;
    UINT16 DS;
    UINT16 FS;
    UINT16 GS;
    UINT32 EBP;
    UINT32 ESP;
} REG_BLOCK;

typedef struct {
  UINT8                    OpCode;
  CHAR8                    *OpCodeStr;
} IFR_OPCODE_STRING_STRUCT;

#pragma pack()
