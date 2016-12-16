/*++

Module Name:

  FlashUpdate.c

Abstract:

  This file contains flash update functions when system is under
  recovery mode or flash update mode.

--*/
#include "FlashUpdate.h"
#include <Protocol\NvMediaAccess.h>
#include <Pi\PiHob.h>
#include <Library\UefiBootServicesTableLib.h>
#include <Library\HobLib.h>
#include <Library\DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Uefi\UefiSpec.h>
#include <Protocol\Spi.h>
#include <Protocol\FirmwareVolumeBlock.h>
#include <Library\BaseLib.h>
#include <Library\MemoryAllocationLib.h>
#include <Uefi\UefiInternalFormRepresentation.h>
#include <Library\PcdLib.h>
#include <Library\BaseMemoryLib.h>
#include <Protocol\SimpleTextIn.h>
#include <Library/CacheMaintenanceLib.h>


typedef enum {
    SPI_WREN,             // Prefix Opcode 0: Write Enable
    SPI_EWSR,             // Prefix Opcode 1: Enable Write Status Register
} PREFIX_OPCODE_INDEX;

//
// Opcode Menu Index on the host SPI controller
//
typedef enum {
    SPI_READ_ID,        // Opcode 0: READ ID, Read cycle with address
    SPI_READ,           // Opcode 1: READ, Read cycle with address
    SPI_RDSR,           // Opcode 2: Read Status Register, No address
    SPI_WRDI,           // Opcode 3: Write Disable, No address
    SPI_SERASE,         // Opcode 4: Sector Erase (4KB), Write cycle with address
    SPI_BERASE,         // Opcode 5: Block Erase (32KB), Write cycle with address
    SPI_PROG,           // Opcode 6: Byte Program, Write cycle with address
    SPI_WRSR,           // Opcode 7: Write Status Register, No address
} SPI_OPCODE_INDEX;


EFI_STATUS
ReadKeyStroke (
  IN OUT EFI_INPUT_KEY      *Key
  );

/**
  Reset system according to a input attribute

  @param  Attribute             a value to determine what type reset
                                system will do

  @return  VOID

**/
VOID
ResetSystemByAttribute (
  UINT16        Attribute
  )
{
  EFI_STATUS        Status;
  UINT16            ResetType;
  EFI_INPUT_KEY     Key;
  UINTN             Index;

  ResetType = Attribute & POWER_MARK;
  switch (ResetType) {
  case SHUT_DOWN:
//  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_RED);	
//  Print (L"\rShutdown and Make Bios Configuration Jumper In Normal Mode.\n");
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);    
    Print (L"\rPress Any Key to Shutdown...\n");
    while (gST->ConIn != NULL) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);    	
      if (!EFI_ERROR (Status)) {      
        gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
      }
      gBS->Stall (1000 * 100);
    }    
    break;

  case RESET_COLD:
    for (Index = 10; Index > 0; Index --) {
      Status = ReadKeyStroke (&Key);
      if (!EFI_ERROR(Status)) {
        //
        // if ESC key, clean message and go to deadloop
        //
        if (Key.ScanCode == 0x0017) {
          Print (L"\r                                                  ");
          goto DeadLoop;
        }
        break;
      }
      Print (L"\rSystem will do reset in");
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_RED);
      Print (L" %d ", Index);
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);
      Print (L"seconds...");
      gBS->Stall (1000 * 1000);
    }
    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
    break;

  case RESET_WARM:
    for (Index = 10; Index > 0; Index --) {
      Status = ReadKeyStroke (&Key);
      if (!EFI_ERROR(Status)) {
        //
        // if ESC key, clean message and go to deadloop
        //
        if (Key.ScanCode == 0x0017) {
          Print (L"\r                                                  ");
          goto DeadLoop;
        }
        break;
      }
      Print (L"\rSystem will do reset in");
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_RED);
      Print (L" %d ", Index);
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);
      Print (L"seconds..");
      gBS->Stall (1000 * 1000);
    }
    gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
    break;
  default:
    break;
  }

DeadLoop:
  CpuDeadLoop ();
}



/**
  This is a internal function to show ByoSoft copyrights ans warning messages

  @param BootMode         Indicate which mode system is under

  @retval EFI_SUCCESS     Show copyrights successfully

**/
EFI_STATUS
ShowCopyRightsAndWarning (
  IN EFI_BOOT_MODE      BootMode
)
{
  gST->ConOut->SetAttribute(gST->ConOut, EFI_BACKGROUND_BLACK | EFI_WHITE | EFI_BRIGHT);
  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->EnableCursor(gST->ConOut, FALSE);
  
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);

  Print(L"              **************************************************************\n");
  Print(L"              *                   Byosoft Flash Update                     *\n");
  Print(L"              *         Copyright(C) 2006-2015, Byosoft Co.,Ltd.           *\n");
  Print(L"              *                   All rights reserved                      *\n");
  Print(L"              **************************************************************\n");

  if (BootMode == BOOT_IN_RECOVERY_MODE) {
    Print(L"Warning: System is in Recovery Mode. Please don't shutdown system during erasing/programming flash!\n");
  } else if (BootMode == BOOT_ON_FLASH_UPDATE) {
    Print(L"Warning: System is in Flash Update Mode. Please don't shutdown system during erasing/programming flash!\n");
  }

  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_WHITE | EFI_BRIGHT);
  
  return EFI_SUCCESS;
}




EFI_STATUS  
UpdateSpiBlock (
  NV_MEDIA_ACCESS_PROTOCOL *NvAcc,
  UINT8                    *Src,
  UINT8                    *Target,
  UINT32                   BlockBase 
)
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINTN       BlockSize;
  
  DEBUG((EFI_D_INFO, __FUNCTION__"(%X,%X,%X)\n", (UINTN)Src, (UINTN)Target, BlockBase));  

  if(CompareMem(Src, Target, SIZE_64KB) == 0){
    DEBUG((EFI_D_INFO, "Equal\n"));
    goto ProcExit;
  }

  Status = NvAcc->Erase(NvAcc, BlockBase, SIZE_64KB, SPI_MEDIA_TYPE);
  if(EFI_ERROR(Status)){goto ProcExit;}
  Status = NvAcc->Write(NvAcc, BlockBase, Src, SIZE_64KB, SPI_MEDIA_TYPE);
  if(EFI_ERROR(Status)){goto ProcExit;}

  BlockSize = SIZE_64KB;  
  Status = NvAcc->Read(NvAcc, BlockBase, Target, &BlockSize, SPI_MEDIA_TYPE);
  if(EFI_ERROR(Status)){goto ProcExit;}
  
  if(CompareMem(Src, Target, SIZE_64KB)){
    DEBUG((EFI_D_INFO, "Verify Err\n"));     
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;    
  }
  
ProcExit:
  return Status;  
}  



EFI_STATUS
ReBaseFvPeims (
  IN  VOID    *FvBBImage,
  IN  UINT32  NewFvBase
  );

#define FVH_SIGNATURE     SIGNATURE_32('_', 'F', 'V', 'H')



/**
  This procedure is used to update flash part and shutdown
  system by behavior parameter

  @param  BootMode              Indicate the system boot mode
  @param  Behavior              Indicate which fv part will be updated and
                                do which system reset type
  @param  FDImageBaseAddress    New BIOS image base address
  @param  FDImageLength         New BIOS image length

  @return  EFI_SUCCESS          Flash is updated successfully

**/
EFI_STATUS
FlashUpdate (
  EFI_BOOT_MODE           BootMode,
  UINT16                  Behavior,
  EFI_PHYSICAL_ADDRESS    FDImageBaseAddress,
  UINT64                  FDImageLength
  )
{
  NV_MEDIA_ACCESS_PROTOCOL *NvAcc;
  EFI_STATUS        Status;	
  UINT32            SpiAddress;
  UINT32            FlashSize;  
  UINT8             *OldBios = NULL;
  UINT8             *NewBios;
  UINTN             Index;
  UINTN             Count;
  UINT8             *Src;
  UINT8             *Target;
  UINT32            BiosBase;
  UINT32            BlockBase;
  UINT32            Offset;
  UINT32            NvramSize;
  UINT32            Signature;
  UINT8             *FvBBImageAlloc = NULL;
  UINT8             *FvBBImage;	
  BOOLEAN           FvBBIsBad;
  BOOLEAN           FvBBChanged;	
  UINTN             BlockSize;


  DEBUG((EFI_D_INFO, "BM:%X, BH:%X, Image(%lX,%lX)\n", BootMode, Behavior, FDImageBaseAddress, FDImageLength));
  
  Status = gBS->LocateProtocol (
             &gEfiNvMediaAccessProtocolGuid,
             NULL,
             (VOID**)&NvAcc
             );
  ASSERT(!EFI_ERROR(Status));
  if(EFI_ERROR(Status)){goto ProcExit;}

  ShowCopyRightsAndWarning (BootMode);



//-----------------------------------------------------------------------------
  FlashSize = PcdGet32(PcdFlashAreaSize);
  OldBios  = AllocatePool(FlashSize);
  if(OldBios == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }
  FvBBImageAlloc = AllocatePool(PcdGet32(PcdFlashFvRecoverySize) + 16);
  if(FvBBImageAlloc == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }
  FvBBImage = ALIGN_POINTER(FvBBImageAlloc, 16);

  NewBios  = (UINT8*)(UINTN)FDImageBaseAddress;
  BiosBase = PcdGet32(PcdFlashAreaBaseAddress);
  ASSERT(FlashSize == (UINT32)FDImageLength);
	
// Read Current Bios  
  for(SpiAddress=0; SpiAddress<FlashSize; SpiAddress+=SIZE_64KB){
    BlockSize = SIZE_64KB;
    Status = NvAcc->Read(NvAcc, BiosBase + SpiAddress, OldBios+SpiAddress, &BlockSize, SPI_MEDIA_TYPE);
	  if(EFI_ERROR(Status)){goto ProcExit;}
    Print(L"\rReading (%02d%%)", (SpiAddress+SIZE_64KB)*100/FlashSize); 
  }   

  FvBBIsBad   = FALSE;
  FvBBChanged = TRUE;
  Offset = PcdGet32(PcdFlashFvRecoveryBase) - BiosBase + 40;
  if(*(UINT32*)(OldBios+Offset) != FVH_SIGNATURE){		// FvBB is bad, so update it first
    FvBBIsBad = TRUE;
  } else {
    Offset = PcdGet32(PcdFlashFvRecoveryBase) - BiosBase;
    if(CompareMem(OldBios+Offset, NewBios+Offset, PcdGet32(PcdFlashFvRecoverySize)) == 0){
      FvBBChanged = FALSE;
    }			
  }	

// Clear FvMain.Hdr.Signature to show Updating.
  if(!FvBBIsBad){
    Signature = 0;
    Offset = PcdGet32(PcdFlashFvMainBase) - BiosBase + 40;
    NvAcc->Write(NvAcc, BiosBase + Offset, (UINT8*)&Signature, sizeof(Signature), SPI_MEDIA_TYPE);
	  if(EFI_ERROR(Status)){goto ProcExit;}
    *(UINT32*)(OldBios + Offset) = Signature;
  }		

// New.FvMain.Hdr.Sign = 0xFFFFFFFF
// New.FvBB.Hdr.Sign   = 0xFFFFFFFF
  Offset = PcdGet32(PcdFlashFvRecoveryBase) - BiosBase + 40;
  *(UINT32*)(NewBios+Offset) = 0xFFFFFFFF;		
  Offset = PcdGet32(PcdFlashFvMainBase) - BiosBase + 40;
  *(UINT32*)(NewBios+Offset) = 0xFFFFFFFF;	


// Handle FvBB
//
// FvBBBad   FvBBChanged      Copy
//---------------------------------------------
//    0          0           nothing    (boot from FvBB)
//    0          1             do       (boot from FvBB)     <---
//    1         N/A          nothing    (boot from FvMain)

  if(!FvBBIsBad && FvBBChanged){
    Print(L"\nPrepare FvBB"); 
    Offset = PcdGet32(PcdFlashFvRecoveryBase) - BiosBase;	
    CopyMem(FvBBImage, OldBios+Offset, PcdGet32(PcdFlashFvRecoverySize));
    Status = ReBaseFvPeims(FvBBImage, PcdGet32(PcdFlashFvMainBase));
    if(EFI_ERROR(Status)){goto ProcExit;}
    *(UINT32*)(FvBBImage+40) = 0xFFFFFFFF;	

  // Copy FvBB to FvMain	
    Count     = PcdGet32(PcdFlashFvRecoverySize)/SIZE_64KB;
    BlockBase = PcdGet32(PcdFlashFvMainBase);
    Src       = FvBBImage;
    Target    = OldBios + BlockBase - BiosBase;
    for(Index=0;Index<Count;Index++){
      Status = UpdateSpiBlock(NvAcc, Src, Target, BlockBase);
      if(EFI_ERROR(Status)){goto ProcExit;}		
      BlockBase += SIZE_64KB;
      Src       += SIZE_64KB;
      Target    += SIZE_64KB;    
      Print(L"\rPrepare FvBB (%02d%%)", (Index+1)*100/Count); 
    }

  // FvMain.Hdr.Sign = "_FVH"
    Offset = PcdGet32(PcdFlashFvMainBase) - BiosBase + 40;
    Signature = FVH_SIGNATURE;
    NvAcc->Write(NvAcc, BiosBase + Offset, (UINT8*)&Signature, sizeof(Signature), SPI_MEDIA_TYPE);
	  if(EFI_ERROR(Status)){goto ProcExit;}
    *(UINT32*)(OldBios + Offset) = Signature;

  // FvBB.Hdr.Sign = 0	
    Offset = PcdGet32(PcdFlashFvRecoveryBase) - BiosBase + 40;
    Signature = 0;
    Status = NvAcc->Write(NvAcc, BiosBase+Offset, (UINT8*)&Signature, sizeof(Signature), SPI_MEDIA_TYPE);    
	  if(EFI_ERROR(Status)){goto ProcExit;}
    *(UINT32*)(OldBios + Offset) = Signature;	
  }

  if(FvBBChanged){
    Print(L"\nUpdating FvBB"); 	
    Count     = PcdGet32(PcdFlashFvRecoverySize)/SIZE_64KB;  
    BlockBase = PcdGet32(PcdFlashFvRecoveryBase);
    Offset    = BlockBase - BiosBase;
    Src       = NewBios + Offset;
    Target    = OldBios + Offset;
    for(Index=0;Index<Count;Index++){
      Status = UpdateSpiBlock(NvAcc, Src, Target, BlockBase);
      if(EFI_ERROR(Status)){goto ProcExit;}			
      BlockBase += SIZE_64KB;
      Src       += SIZE_64KB;
      Target    += SIZE_64KB;
      Print(L"\rUpdating FvBB (%02d%%)", (Index+1)*100/Count); 
    }

// FvBB.Hdr.Sign = "_FVH", OK
    Offset = PcdGet32(PcdFlashFvRecoveryBase) - BiosBase + 40;
    Signature = FVH_SIGNATURE;
    Status = NvAcc->Write(NvAcc, BiosBase+Offset, (UINT8*)&Signature, sizeof(Signature), SPI_MEDIA_TYPE);    
	  if(EFI_ERROR(Status)){goto ProcExit;}
    *(UINT32*)(OldBios + Offset) = Signature;	
  }
	
// Handle FvMain
  Print(L"\nUpdating FvMain"); 
  Count     = PcdGet32(PcdFlashFvMainSize)/SIZE_64KB;
  BlockBase = PcdGet32(PcdFlashFvMainBase);
  Offset    = BlockBase - BiosBase;
  Src       = NewBios + Offset;
  Target    = OldBios + Offset;
  for(Index=0;Index<Count;Index++){
    Status = UpdateSpiBlock(NvAcc, Src, Target, BlockBase);
	  if(EFI_ERROR(Status)){goto ProcExit;}			
    BlockBase += SIZE_64KB;
    Src       += SIZE_64KB;
    Target    += SIZE_64KB;
    Print(L"\rUpdating FvMain (%02d%%)", (Index+1)*100/Count); 
  }  

// Handle FvMain2
  Print(L"\nUpdating FvMain2"); 
  Count     = PcdGet32(PcdFlashFvMain2Size)/SIZE_64KB;
  BlockBase = PcdGet32(PcdFlashFvMain2Base);
  Offset    = BlockBase - BiosBase;
  Src       = NewBios + Offset;
  Target    = OldBios + Offset;
  for(Index=0;Index<Count;Index++){
    Status = UpdateSpiBlock(NvAcc, Src, Target, BlockBase);
	  if(EFI_ERROR(Status)){goto ProcExit;}			
    BlockBase += SIZE_64KB;
    Src       += SIZE_64KB;
    Target    += SIZE_64KB;
    Print(L"\rUpdating FvMain2 (%02d%%)", (Index+1)*100/Count); 
  }  
  
// Handle NVRAM
  if(Behavior & NVSTORAGE_VARIABLE_UPDATE){
    Print(L"\nUpdating NVRAM"); 
    NvramSize = *(UINT32*)(((UINT8*)(UINTN)PcdGet32(PcdFlashNvStorageVariableBase))+32);
    Count     = NvramSize/SIZE_64KB;
    BlockBase = PcdGet32(PcdFlashNvStorageVariableBase);
    Offset    = BlockBase - BiosBase;
    Src       = NewBios + Offset;
    Target    = OldBios + Offset;
    for(Index=0;Index<Count;Index++){
      Status = UpdateSpiBlock(NvAcc, Src, Target, BlockBase);
      if(EFI_ERROR(Status)){goto ProcExit;}			
      BlockBase += SIZE_64KB;
      Src       += SIZE_64KB;
      Target    += SIZE_64KB;
      Print(L"\rUpdating NVRAM (%02d%%)", (Index+1)*100/Count); 
    } 
  }    

// Handle Microcode
  if(Behavior & MICROCODE_UPDATE){
    Print(L"\nUpdating MicroCode"); 
    Count     = PcdGet32(PcdFlashNvStorageMicrocodeSize)/SIZE_64KB;
    BlockBase = PcdGet32(PcdFlashNvStorageMicrocodeBase);
    Offset    = BlockBase - BiosBase;
    Src       = NewBios + Offset;
    Target    = OldBios + Offset;
    for(Index=0;Index<Count;Index++){
      Status = UpdateSpiBlock(NvAcc, Src, Target, BlockBase);
      if(EFI_ERROR(Status)){goto ProcExit;}			
      BlockBase += SIZE_64KB;
      Src       += SIZE_64KB;
      Target    += SIZE_64KB;
      Print(L"\rUpdating MicroCode (%02d%%)", (Index+1)*100/Count); 
    }
  }  

// FvMain.Hdr.Sign = "_FVH", OK
  Signature = FVH_SIGNATURE;
  Offset    = PcdGet32(PcdFlashFvMainBase) - BiosBase + 40;
  Status = NvAcc->Write(NvAcc, BiosBase+Offset, (UINT8*)&Signature, sizeof(Signature), SPI_MEDIA_TYPE);  
  *(UINT32*)(OldBios + Offset) = Signature;
	
ProcExit:
  if(OldBios!=NULL){FreePool(OldBios);}
  if(FvBBImageAlloc!=NULL){FreePool(FvBBImageAlloc);}	
  return Status;
}



