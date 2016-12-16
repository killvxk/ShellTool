/*++
Copyright (c) 2011 Byosoft Corporation. All rights reserved.

Module Name:

  SmiFlash.h

Abstract:

 This file contains the Includes, Definitions, typedefs,
 Variable and External Declarations, Structure and
 function prototypes needed for the SmiFlash driver
--*/

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/SetMemAttributeSmmLib.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/NvMediaAccess.h>
#include <Protocol/Smbios.h>
//#include <Library/BiosIdLib.h>
#include <Guid/SmBios.h>
#include <IndustryStandard/SmBios.h>

//#include <openssl/ossl_typ.h>
//#include <openssl/evp.h>
//#include <openssl/x509.h>
//#include <Library/BaseCryptLib.h>

#define SW_SMI_FLASH_SERVICES	                0xec

#define GET_FLASH_INFO_SUBFUNCTION            0x01
#define GET_FD_AREA_SUBFUNCTION               0x02
#define READ_FLASH_BLOCK_SUBFUNCTION          0x03
#define COMPARE_BIOS_ID_SUBFUNCTION           0x04
#define UPDATE_SMBIOS_DATA                    0x05
#define BIOS_VERIFY                           0X06
#define MAX_BLOCK_SIZE                        0x10000

#define FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE        PcdGet32 (PcdFlashNvStorageSmbiosBase)
#define FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_SIZE        PcdGet32 (PcdFlashNvStorageSmbiosSize)
#define SMBIOS_BUFFER_SIZE        4096
#define END_HANDLE                0xFFFF
#define SMBIOS_REC_SIGNATURE      0x55AA
#define MAX_STRING_LENGTH         30

#pragma pack(1)
typedef struct {
    UINT16 Signature;
    UINT16 RecordLen;
} SMBIOS_REC_HEADER;

typedef struct {
    UINT8   Type;
    UINT8   Length;
    UINT16  Handle;
} SMBIOS_HEADER;

typedef struct {
  UINT8 Command;
  UINT8 FieldOffset;
  UINT32 ChangeMask;
  UINT32 ChangeValue;
  UINT16 DataLength;
  SMBIOS_HEADER StructureHeader;
  UINT8 StructureData[1];
} PNP_52_DATA_BUFFER;

typedef struct {
  UINT8               SubFun;
  PNP_52_DATA_BUFFER  Parameter;
} UPDATE_SMBIOS_PARAMETER;

#define EFI_PNP_52_SIGNATURE  SIGNATURE_32('_','p','n','p')
typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Link;
  SMBIOS_REC_HEADER   header;
  PNP_52_DATA_BUFFER  *pRecord;
} PNP_52_RECORD;

///
/// Identifies the structure-setting operation to be performed.
///
typedef enum {
  ByteChanged,
  WordChanged,
  DoubleWordChanged,
  AddChanged,
  DeleteChanged,
  StringChanged,
  BlockChanged,
  Reseved
} FUNC52_CMD;

typedef enum {
    UPDATE_UUID = 1,
    UPDATE_SERIAL_NUMBER,
    UPDATE_ASSET_TAG,
    UPDATE_MODEL_NUMBER,
    UPDATE_BRAND_ID,
    UPDATE_LOCK_STATUS,
    UPDATE_BASE_BOARD_SERIAL_NUMBER,
    UPDATE_BASE_BOARD_ASSET_TAG
} UPDATE_SMBIOS_TYPE;

typedef enum {
    FD_AREA_TYPE_FD,                            // FD_AREA_INFO.Offset return 0
    FD_AREA_TYPE_FVMAIN_COMPACT,
    FD_AREA_TYPE_NVSTORAGE_VARIABLE,
    FD_AREA_TYPE_NVSTORAGE_MICROCODE,
    FD_AREA_TYPE_FVRECOVERY
} FD_AREA_TYPE;

typedef struct {
  UINT32        Type;                         // IN, Type of flash area
  UINT32        Offset;                       // OUT, Offset from FD base address or base address for FD_AREA_TYPE_FD
  UINT32        Size;                         // OUT, Byte size of flash area
} FD_AREA_INFO;

typedef struct {
    UINT16 BlockSize;
    UINT16 BlockCount;
} FLASH_BLOCK_INFO;

typedef struct {
    UINT32               Offset;
    UINT32               BufferSize;
    EFI_PHYSICAL_ADDRESS Buffer;
} BLOCK_READ_PARAMETER;

typedef struct {
	EFI_PHYSICAL_ADDRESS Buffer;
	UINT32               Size;
} FLASH_BUFFER;

typedef enum {
  VERIFY_INIT = 1,
  VERIFY_UPDATE,
  VERIFY_FINAL
} BIOS_VERIFY_SUBFUN;

typedef struct {
  UINT8                SubFun;
  EFI_PHYSICAL_ADDRESS Buffer;
  UINT32               Size;
} BIOS_VERIFY_PARAMETER;

/*
EFI_CAPSULE_HEADER
  CapsuleGuid         // 16
  HeaderSize          // 4
  Flags               // 4
  CapsuleImageSize    // 4
-------------------------------------------------------------  
  (+) PubkeySize      // 4            +28
  (+) SignSize        // 4            +32
  (+) RangeArraySize  // 4            +36
  
  (+) Pubkey          //              +40
  (+) Sign            //              +40+PubkeySize            align 4
  (+) Range[]         //              +40+PubkeySize+SignSize   align 4
-------------------------------------------------------------  
FD                    // 16 align
*/
typedef struct {
  EFI_CAPSULE_HEADER   Header;
  UINT32               PubkeySize;
  UINT32               SignSize;
  UINT32               RangeArraySize;
  UINT8                Data[1];
} BIOS_VERIFY_CAPSULE_BUFFER;

#pragma pack()
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

#define CARRY_FLAG       0x00000001

extern EFI_GUID gSignCapsuleHeaderGuild;

