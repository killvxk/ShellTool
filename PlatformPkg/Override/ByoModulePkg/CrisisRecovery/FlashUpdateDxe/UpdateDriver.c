/*++

Module Name:

  UpdateDiver.c

Abstract:

  This file contains flash update functions when system is under
  recovery mode or flash update mode.

--*/
#include <Guid/CapsuleVendor.h>
#include "FlashUpdate.h"
#include <Guid/CapsuleRecord.h>
#include <Pi\PiHob.h>
#include <Library\UefiBootServicesTableLib.h>
#include <Library\HobLib.h>
#include <Library\DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Uefi\UefiSpec.h>
#include <Protocol\FirmwareVolumeBlock.h>
#include <Library\BaseLib.h>
#include <Library\MemoryAllocationLib.h>
#include <Uefi\UefiInternalFormRepresentation.h>
#include <Library\BaseMemoryLib.h>
#include <Guid\RecoveryDevice.h>
#include <Protocol\SimpleTextIn.h>
#include <Protocol\UpdateFlash.h>

CAPSULE_RECORD                   mCapsuleRecord;
UPDATE_FLASH_PROTOCOL            gUpdateFlash;
FIRMWARE_READ_TOP_SWAP_PROTOCOL* mTopSwapper   = NULL;

/**
  Check if there is a key stroke and return the key value if yes

  @param  Key                   a pointer to input key structure

  @return  EFI_NOT_FOUND        no key stroke at this time
  @return  EFI_SUCCESS          Read a key stroke

**/
EFI_STATUS
ReadKeyStroke (
  IN OUT EFI_INPUT_KEY      *Key
  )
{
  EFI_STATUS        Status;
  UINTN             Index;
  UINTN             HandleCount;
  EFI_HANDLE        *HandleBuffer;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL      *SimpleTextIn;
  EFI_TPL           OldTpl;

  //
  // The protocol "gEfiSimpleTextInProtocolGuid" about usb keyboard
  // will be updated in a period callback function becasue of this usb
  // keyboard plug in/out. Raise TPL to stop this update happen during
  // reading key stroke.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleTextInProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer);
  if (!EFI_ERROR(Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiSimpleTextInProtocolGuid,
                      &SimpleTextIn);
      if (!EFI_ERROR(Status)) {
        Status = SimpleTextIn->ReadKeyStroke (SimpleTextIn, Key);
        if (!EFI_ERROR(Status)) {
          break;
        }
      }
    }
  }
  gBS->FreePool (HandleBuffer);
  //
  // Retore to privous TPL and delay 1000us for other TPL function
  // to finish their works.
  //
  gBS->RestoreTPL (OldTpl);
  gBS->Stall (1000);

  if (EFI_ERROR(Status)) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;

}

/**
  Parse device guid then return a media type string for print() function

  @param  DeviceId              Indicate a media type

  @return  CHAR16*              a pointer to media type string

**/
CHAR16*
MediaType (
  IN EFI_GUID           *DeviceId
  )
{
  UINT16           *MediaType;

  MediaType = L"Unkonwn";

  if (CompareGuid (&gRecoveryOnDataCdGuid, DeviceId)) {
    MediaType = L"CDROM  ";
  } else if (CompareGuid (&gRecoveryOnFatFloppyDiskGuid, DeviceId)) {
    MediaType = L"Floppy ";
  } else if (CompareGuid (&gRecoveryOnFatIdeDiskGuid, DeviceId)) {
    MediaType = L"IdeDisk";
  } else if (CompareGuid (&gRecoveryOnFatUsbDiskGuid, DeviceId)) {
    MediaType = L"UsbDisk";
  }

  return MediaType;
}

/**
  when actived, function return a string.

  @param  Actived               boolean type parameter to indicate if it
                                is actived.

  @return  UINT8*               return string Yes or No.

**/
CHAR16*
IsActived (
  IN BOOLEAN        Actived
  )
{
  CHAR16        *String;

  String = L"No";

  if (Actived) {
    String = L"Yes";
  }

  return String;
}

/**
  Show all bios images recorded in mCapsuleRecord and wait user select one
  of them to finish flash update.

  @return  UINT8                return which one is selected.

**/
UINT8
WaitForSelect(VOID)
{
  UINT8             Index;
  EFI_INPUT_KEY     Key;
  UINT8             ReturnNumber;
  UINT8             SelectNumber;
  BOOLEAN           LegalKey;
  BOOLEAN           EnterKeyPressed;

  if (mCapsuleRecord.CapsuleCount == 1) {
    return mCapsuleRecord.CapsuleCount;
  }

  Print (L"\rSystem is in Recovery Mode! Total %d BIOS images are found in attached medias\n", mCapsuleRecord.CapsuleCount);

  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);
  for (Index = 0; Index < mCapsuleRecord.CapsuleCount; Index ++) {
    Print (L"\r%d. %s   Media: %s   Actived: %s\n", Index +1,\
                                                  &mCapsuleRecord.CapsuleInfo[Index].BiosIdImage.BiosIdString,\
                                                  MediaType(&mCapsuleRecord.CapsuleInfo[Index].DeviceId),\
                                                  IsActived (mCapsuleRecord.CapsuleInfo[Index].Actived));
  }
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_WHITE);

  gST->ConOut->EnableCursor (gST->ConOut, TRUE);
  Print (L"\n");
  Print (L"\rPlease enter the number to select one for flash update:");
  //
  // wait legal key pressed
  //
  LegalKey = FALSE;
  EnterKeyPressed = FALSE;
  ReturnNumber = 1;
  ZeroMem (&Key, sizeof(Key));
  do {
    //
    // wait for key stroke
    //
    while (EFI_ERROR(ReadKeyStroke (&Key)));

    SelectNumber = (UINT8)Key.UnicodeChar - 0x30;
    if (SelectNumber > 0 && SelectNumber <= mCapsuleRecord.CapsuleCount && LegalKey == FALSE) {
      LegalKey = TRUE;
      Print (L"%c", Key.UnicodeChar);
      ReturnNumber = SelectNumber;
    } else if (Key.UnicodeChar == 0x0008 && LegalKey == TRUE) {
      Print (L"%c", Key.UnicodeChar);
      LegalKey = FALSE;
    } else if (Key.UnicodeChar == 0x000D && LegalKey == TRUE) {
      EnterKeyPressed = TRUE;
    }
  } while (!EnterKeyPressed);

  return ReturnNumber;
}



EFI_STATUS
EFIAPI
ProcessFlash (
  EFI_BOOT_MODE     BootMode
  )
{
  EFI_STATUS                    Status;
  EFI_PEI_HOB_POINTERS          HobList;
  EFI_CAPSULE_HEADER            *CapsuleHob;
  EFI_PHYSICAL_ADDRESS          FDImageBaseAddress;
  UINT64                        FDImageLength;
  UINT8                         SelectNum;
  UINT16                        UpdateBehavior;
  BOOLEAN                       bCapExt;
  UINT32                        CRC32;
  UINTN                         Delay;
  UINTN                         RewriteTimes = 2;
  EFI_HOB_GUID_TYPE             *GuidHob;
  BYO_CAPSULE_EXTEND            *CapExt;	
	

  bCapExt = FALSE;
  UpdateBehavior = 0;

  if (gST->ConOut != NULL) {
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK| EFI_WHITE);
    gST->ConOut->ClearScreen (gST->ConOut);
    gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  }

  if (BootMode == BOOT_ON_FLASH_UPDATE) {
    CapsuleHob = NULL;
    HobList.Raw = (UINT8*)GetHobList();
    while (!END_OF_HOB_LIST (HobList)) {
      if (HobList.Header->HobType == EFI_HOB_TYPE_UEFI_CAPSULE) {
        CapsuleHob = (EFI_CAPSULE_HEADER *)(UINTN)HobList.Capsule->BaseAddress;
      }
      HobList.Raw = GET_NEXT_HOB (HobList);
    }
    if (CapsuleHob == NULL) {
      DEBUG ((EFI_D_ERROR, "Error! Flash update mode not find Capsule Hob\n"));
      return EFI_NOT_FOUND;
    }

    FDImageBaseAddress = (EFI_PHYSICAL_ADDRESS)CapsuleHob + CapsuleHob->HeaderSize;
    FDImageLength = CapsuleHob->CapsuleImageSize - CapsuleHob->HeaderSize;
    ASSERT(FDImageBaseAddress != 0 && FDImageLength != 0);

    GuidHob = GetFirstGuidHob(&gEfiCapsuleVendorGuid);
    CapExt  = (BYO_CAPSULE_EXTEND*)GET_GUID_HOB_DATA(GuidHob);
    Status = gBS->CalculateCrc32 ((UINT8*)(UINTN)FDImageBaseAddress, (UINTN)FDImageLength, &CRC32);
    if(EFI_ERROR(Status) || CRC32 != CapExt->CRC32) {
      return EFI_NOT_FOUND;
    }

    //
    // set update behavior
    //
    UpdateBehavior = UpdateBehavior | RESET_COLD;
    DEBUG((EFI_D_INFO, "CapExt->Flags:%X ROM(%lX,%lX)\n", CapExt->Flags, FDImageBaseAddress, FDImageLength));

    if(CapExt->Flags & UPDATE_FV_SELECTOR_BIT) {
      UpdateBehavior = UpdateBehavior | NVSTORAGE_VARIABLE_UPDATE;
      if (CapExt->Flags & UPDATE_FV_MAIN_SELECT) {
        UpdateBehavior |= FVMAIN_UPDATE;
      }
      if (CapExt->Flags & UPDATE_FV_MICROCODE_SELECT) {
        UpdateBehavior |= MICROCODE_UPDATE;
      }
    } else {
      UpdateBehavior = UpdateBehavior | TEST_MENU_UPDATE | NVSTORAGE_VARIABLE_UPDATE | \
                       NVSTORAGE_FTW_UPDATE | MICROCODE_UPDATE | FVMAIN_UPDATE | NVSTORAGE_FTW_SPARE_UPDATE;

      if (CapExt->Flags & UPDATE_FORCE_RECOVERY) {
        UpdateBehavior |= BOOTBLOCK_UPDATE;
      }
    }

    if (CapExt->Flags & UPDATE_KEEP_VARIABLE) {
      UpdateBehavior &= ~NVSTORAGE_VARIABLE_UPDATE;
    }
  } else if (BootMode == BOOT_IN_RECOVERY_MODE) {
    HobList.Raw = (UINT8*)GetHobList();

    while (!END_OF_HOB_LIST (HobList)) {
      if (HobList.Header->HobType == EFI_HOB_TYPE_GUID_EXTENSION) {
        if (CompareGuid (&HobList.Guid->Name, &gRecoveryCapsuleRecordGuid)) {
          CopyMem (&mCapsuleRecord, (UINT8 *)HobList.Guid + sizeof(EFI_HOB_GUID_TYPE), sizeof(CAPSULE_RECORD));
        }
      }
      HobList.Raw = GET_NEXT_HOB (HobList);
    }

    SelectNum = WaitForSelect();
    FDImageBaseAddress = mCapsuleRecord.CapsuleInfo[SelectNum - 1].BaseAddress;
    FDImageLength = mCapsuleRecord.CapsuleInfo[SelectNum - 1].Length;
    UpdateBehavior = UpdateBehavior | SHUT_DOWN;
    UpdateBehavior = UpdateBehavior | TEST_MENU_UPDATE | NVSTORAGE_VARIABLE_UPDATE | \
                     NVSTORAGE_FTW_UPDATE | MICROCODE_UPDATE | NVSTORAGE_FTW_SPARE_UPDATE | \
                     FVMAIN_UPDATE | BOOTBLOCK_UPDATE;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  do {
    Status = FlashUpdate (BootMode, UpdateBehavior, FDImageBaseAddress, FDImageLength);
    if (EFI_ERROR (Status)) {
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_RED);
      for (Delay = 3; Delay > 0; Delay--) {
        Print(L"\rFlash Verify Error! System will rewrite flash in %d seconds...", Delay);
        gBS->Stall (1000 * 1000);
      }
    } else {
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);
      Print(L"\rFlash is updated successfully!\n");
    }
    RewriteTimes --;
  } while (EFI_ERROR (Status) && RewriteTimes != 0);

  if (EFI_ERROR (Status)) {
    Print(L"\rFlash verfiy still error! Can't rewrite flash successfully!\n");
    CpuDeadLoop();
  }

  ResetSystemByAttribute (UpdateBehavior);
  return Status;
}

/**
  Entry point of this module.

  @param  ImageHandle           A handle for the image that is
                                initializing this driver.
  @param  SystemTable           A pointer to the EFI system table.

  @return  EFI_SUCCESS          Creat notify event successfully.

**/
EFI_STATUS
EFIAPI
UpdateDriverEntry (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
)
{
  EFI_STATUS        Status;
  EFI_HANDLE        NewHandle;

  gUpdateFlash.ProcessFlash = ProcessFlash;

  NewHandle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &NewHandle,
                  &gUpdateFlashProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gUpdateFlash
                  );

  return   EFI_SUCCESS;
}




