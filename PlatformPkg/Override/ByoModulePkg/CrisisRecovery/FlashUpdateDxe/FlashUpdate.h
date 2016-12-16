/*++

Module Name:

  Flash.h

Abstract:

  PEIM to provide the platform support functionality.

--*/
#ifndef __FLASH_UPDATE_H__
#define __FLASH_UPDATE_H__
#include <Protocol\TopSwap.h>
//
// power behavior after finish update
//
#define SHUT_DOWN                       0x0001
#define RESET_COLD                      0x0002
#define RESET_WARM                      0x0004
#define POWER_MARK                      0x0007

//
// flash update behavior
//
#define BOOTBLOCK_UPDATE                0x0100
#define NVSTORAGE_FTW_SPARE_UPDATE      0x0200
#define NVSTORAGE_VARIABLE_UPDATE       0x0400
#define NVSTORAGE_FTW_UPDATE            0x0800
#define MICROCODE_UPDATE                0x1000
#define FVMAIN_UPDATE                   0x2000
#define TEST_MENU_UPDATE                0x4000
#define UPDATE_MARK                     0xFF00

#define EFI_CAPSULE_VENDOR_GUID  \
{ 0x711C703F, 0xC285, 0x4B10, { 0xA3, 0xB0, 0x36, 0xEC, 0xBD, 0x3C, 0x8B, 0xE2 } }

#define EFI_CAPSULE_EXT_VARIABLE_NAME L"CapsuleUpdateExt"

typedef struct {
    UINT32 CRC32;
    UINT32 Flags;
} BYO_CAPSULE_EXTEND;

EFI_STATUS
FlashUpdate (
  EFI_BOOT_MODE           BootMode,
  UINT16                  Behavior,
  EFI_PHYSICAL_ADDRESS    FDImageBaseAddress,
  UINT64                  FDImageLength
  );

VOID
ResetSystemByAttribute (
  UINT16        Attribute
  );

EFI_STATUS
ReadFlashAndVerify (
  INT16                   UpdateBehavior,
  EFI_PHYSICAL_ADDRESS    FDImageBaseAddress
  );

//************************************************************************
//
// For flash tool(Shell/Dos/Windows) define
// BYO_CAPSULE_EXTEND.ExtFlags enumerates
//
#define UPDATE_DEFAULT              0           // Update all except FvRecovery
#define UPDATE_FORCE_RECOVERY       0x00000001
#define UPDATE_KEEP_VARIABLE        0x00000002


#define UPDATE_FV_SELECTOR_BIT      0x80000000  // if(Flags & UPDATE_FV_SELECTOR_BIT) ; Note : Default flash var FV
#define UPDATE_FV_SELECTOR_MASK     0x80000002
#define UPDATE_FV_BOOTBLOCK_SELECT  0x00000001
#define UPDATE_FV_MAIN_SELECT       0x00000010
#define UPDATE_FV_MICROCODE_SELECT  0x00000020

#define UPDATE_FV_SELECT(a)         (UPDATE_FV_SELECTOR_MASK | (a))


#endif
