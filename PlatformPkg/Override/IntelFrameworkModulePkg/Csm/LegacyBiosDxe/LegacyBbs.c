/** @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

File Name:
  LegacyBbs.c

Abstract:


Revision History:

  Bug 1989:   Changed to use dynamic software SMI value instead of hard coding.
  TIME:       2011-6-15
  $AUTHOR:    Peng Xianbing
  $REVIEWERS:
  $SCOPE:     Define SwSmi value range build a PolicyData table for csm16 to 
              get SwSMI value.
  $TECHNICAL:
  $END-------------------------------------------------------------------------

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LegacyBiosInterface.h"
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Atapi.h>
#include <Library/PrintLib.h>
#include <PlatformDefinition.h>

// Give floppy 3 states
// FLOPPY_PRESENT_WITH_MEDIA  = Floppy controller present and media is inserted
// FLOPPY_NOT_PRESENT = No floppy controller present
// FLOPPY_PRESENT_NO_MEDIA = Floppy controller present but no media inserted
//
#define FLOPPY_NOT_PRESENT           0
#define FLOPPY_PRESENT_WITH_MEDIA    1
#define FLOPPY_PRESENT_NO_MEDIA      2

BBS_TABLE           *mBbsTable;
BOOLEAN             mBbsTableDoneFlag   = FALSE;
BOOLEAN             IsHaveMediaInFloppy = TRUE;

/**
  Checks the state of the floppy and if media is inserted.
  
  This routine checks the state of the floppy and if media is inserted.
  There are 3 cases:
  No floppy present         - Set BBS entry to ignore
  Floppy present & no media - Set BBS entry to lowest priority. We cannot
  set it to ignore since 16-bit CSM will
  indicate no floppy and thus drive A: is
  unusable. CSM-16 will not try floppy since
  lowest priority and thus not incur boot
  time penality.
  Floppy present & media    - Set BBS entry to some priority.

  @return  State of floppy media

**/
UINT8
HasMediaInFloppy (
  VOID
  )
{
  EFI_STATUS                            Status;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *HandleBuffer;
  UINTN                                 Index;
  EFI_ISA_IO_PROTOCOL                   *IsaIo;
  EFI_BLOCK_IO_PROTOCOL                 *BlkIo;

  HandleBuffer  = NULL;
  HandleCount   = 0;

  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiIsaIoProtocolGuid,
        NULL,
        &HandleCount,
        &HandleBuffer
        );

  //
  // If don't find any ISA/IO protocol assume no floppy. Need for floppy
  // free system
  //
  if (HandleCount == 0) {
    return FLOPPY_NOT_PRESENT;
  }

  ASSERT (HandleBuffer != NULL);

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiIsaIoProtocolGuid,
                    (VOID **) &IsaIo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (IsaIo->ResourceList->Device.HID != EISA_PNP_ID (0x604)) {
      continue;
    }
    //
    // Update blockio in case the floppy is inserted in during BdsTimeout
    //
    Status = gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (BlkIo->Media->MediaPresent) {
      FreePool (HandleBuffer);
      return FLOPPY_PRESENT_WITH_MEDIA;
    } else {
      FreePool (HandleBuffer);
      return FLOPPY_PRESENT_NO_MEDIA;
    }
  }

  FreePool (HandleBuffer);

  return FLOPPY_NOT_PRESENT;

}



STATIC
EFI_STATUS
GetHddDeviceModelNumber (
  IN ATA_IDENTIFY_DATA *IdentifyData,
  IN OUT CHAR8         *String
  )
{
  UINTN             Index;

  for (Index = 0; Index < 40; Index += 2) {
    String[Index]      = IdentifyData->ModelName[Index + 1];
    String[Index + 1]  = IdentifyData->ModelName[Index];
  }

  // try to remove more SPACEs at string tail.
  Index--;    // now point to last wchar.
  while (Index) {
    if (String[Index] == ' ') {
      Index--;
    } else {
      break;
    }
  }
  String[Index+1] = '\0';

  return EFI_SUCCESS;
}


/**
  Complete build of BBS TABLE.

  @param  Private                 Legacy BIOS Instance data
  @param  BbsTable                BBS Table passed to 16-bit code

  @retval EFI_SUCCESS             Removable media not present

**/
EFI_STATUS
LegacyBiosBuildBbs (
  IN  LEGACY_BIOS_INSTANCE      *Private,
  IN  BBS_TABLE                 *BbsTable
  )
{
  UINTN     BbsIndex;
  HDD_INFO  *HddInfo;
  UINTN     HddIndex;
  UINTN     Index;
  ATA_IDENTIFY_DATA   *IdentifyData;
  CHAR8               *SataNameDesc;
  UINTN               PortIndex;
  UINT32              AhciBar;
  UINTN               MaxPort;
  UINT8               HostIndex;
  BOOLEAN             BdfInit;  
  UINT8               Bus=0xFF, Dev=0xFF, Func=0xFF; 
  UINTN               StrIndex; 
  CHAR8               *AhciStr;
  CHAR8               *TempStr;
  UINTN               ThisPortIndex;
  CHAR8               ModelName[40+1];	

  //
  // First entry is floppy.
  // Next 2*MAX_IDE_CONTROLLER entries are for onboard IDE.
  // Next n entries are filled in after each ROM is dispatched.
  //   Entry filled in if follow BBS spec. See LegacyPci.c
  // Next entries are for non-BBS compliant ROMS. They are filled in by
  //   16-bit code during Legacy16UpdateBbs invocation. Final BootPriority
  //   occurs after that invocation.
  //
  // Floppy
  // Set default state.
  //
  IsHaveMediaInFloppy = HasMediaInFloppy ();
  if (IsHaveMediaInFloppy == FLOPPY_PRESENT_WITH_MEDIA) {
    BbsTable[0].BootPriority = BBS_UNPRIORITIZED_ENTRY;
  } else {
    if (IsHaveMediaInFloppy == FLOPPY_PRESENT_NO_MEDIA) {
      BbsTable[0].BootPriority = BBS_LOWEST_PRIORITY;
    } else {
      BbsTable[0].BootPriority = BBS_IGNORE_ENTRY;
    }
  }

  BbsTable[0].Bus                       = 0xff;
  BbsTable[0].Device                    = 0xff;
  BbsTable[0].Function                  = 0xff;
  BbsTable[0].DeviceType                = BBS_FLOPPY;
  BbsTable[0].Class                     = 01;
  BbsTable[0].SubClass                  = 02;
  BbsTable[0].StatusFlags.OldPosition   = 0;
  BbsTable[0].StatusFlags.Reserved1     = 0;
  BbsTable[0].StatusFlags.Enabled       = 0;
  BbsTable[0].StatusFlags.Failed        = 0;
  BbsTable[0].StatusFlags.MediaPresent  = 0;
  BbsTable[0].StatusFlags.Reserved2     = 0;

  //
  // Onboard HDD - Note Each HDD controller controls 2 drives
  //               Master & Slave
  //
  HddInfo = &Private->IntThunk->EfiToLegacy16BootTable.HddInfo[0];
  //
  // Get IDE Drive Info
  //
  LegacyBiosBuildIdeData (Private, &HddInfo, 0);

  for (HddIndex = 0; HddIndex < MAX_IDE_CONTROLLER; HddIndex++) {

    BbsIndex = HddIndex * 2 + 1;
    for (Index = 0; Index < 2; Index++) {

      BbsTable[BbsIndex + Index].Bus                      = HddInfo[HddIndex].Bus;
      BbsTable[BbsIndex + Index].Device                   = HddInfo[HddIndex].Device;
      BbsTable[BbsIndex + Index].Function                 = HddInfo[HddIndex].Function;
      BbsTable[BbsIndex + Index].Class                    = 1;
      BbsTable[BbsIndex + Index].SubClass                 = 1;
      BbsTable[BbsIndex + Index].StatusFlags.OldPosition  = 0;
      BbsTable[BbsIndex + Index].StatusFlags.Reserved1    = 0;
      BbsTable[BbsIndex + Index].StatusFlags.Enabled      = 0;
      BbsTable[BbsIndex + Index].StatusFlags.Failed       = 0;
      BbsTable[BbsIndex + Index].StatusFlags.MediaPresent = 0;
      BbsTable[BbsIndex + Index].StatusFlags.Reserved2    = 0;

// If no controller found or no device found set to ignore else set to unprioritized and set device type
      if (HddInfo[HddIndex].CommandBaseAddress == 0) {
        BbsTable[BbsIndex + Index].BootPriority = BBS_IGNORE_ENTRY;
        continue;        
      } 
      
      if (Index == 0) {            // master
        if ((HddInfo[HddIndex].Status & (HDD_MASTER_IDE | HDD_MASTER_ATAPI_CDROM | HDD_MASTER_ATAPI_ZIPDISK)) != 0) {
          BbsTable[BbsIndex + Index].BootPriority = BBS_UNPRIORITIZED_ENTRY;
          if ((HddInfo[HddIndex].Status & HDD_MASTER_IDE) != 0) {
            BbsTable[BbsIndex + Index].DeviceType = BBS_HARDDISK;
          } else if ((HddInfo[HddIndex].Status & HDD_MASTER_ATAPI_CDROM) != 0) {
            BbsTable[BbsIndex + Index].DeviceType = BBS_CDROM;
          } else {
            BbsTable[BbsIndex + Index].DeviceType = BBS_HARDDISK;         // for ZIPDISK
          }
        } else {
          BbsTable[BbsIndex + Index].BootPriority = BBS_IGNORE_ENTRY;
        }
      } else {
        if ((HddInfo[HddIndex].Status & (HDD_SLAVE_IDE | HDD_SLAVE_ATAPI_CDROM | HDD_SLAVE_ATAPI_ZIPDISK)) != 0) {
          BbsTable[BbsIndex + Index].BootPriority = BBS_UNPRIORITIZED_ENTRY;
          if ((HddInfo[HddIndex].Status & HDD_SLAVE_IDE) != 0) {
            BbsTable[BbsIndex + Index].DeviceType = BBS_HARDDISK;
          } else if ((HddInfo[HddIndex].Status & HDD_SLAVE_ATAPI_CDROM) != 0) {
            BbsTable[BbsIndex + Index].DeviceType = BBS_CDROM;
          } else {
            BbsTable[BbsIndex + Index].DeviceType = BBS_HARDDISK;        // for ZIPDISK
          }
        } else {
          BbsTable[BbsIndex + Index].BootPriority = BBS_IGNORE_ENTRY;
        }
      }

      if(HddInfo[HddIndex].Status & (HDD_MASTER_IDE | HDD_MASTER_ATAPI_CDROM | HDD_SLAVE_IDE | HDD_SLAVE_ATAPI_CDROM)){
        SataNameDesc = (CHAR8*)(UINTN)gIdeDevBbsNameStrAddress;  // 32 * 16 = 0.5K
        ASSERT((UINTN)SataNameDesc < 0xFFFFF);
        SataNameDesc = SataNameDesc + 32 * (BbsIndex + Index - 1);
        IdentifyData = (ATA_IDENTIFY_DATA *)(&(HddInfo[HddIndex].IdentifyDrive[Index]));
        GetHddDeviceModelNumber(IdentifyData, ModelName);
        AsciiSPrint(SataNameDesc, 32, "SATA %d: %a", BbsIndex + Index, ModelName);
        BbsTable[BbsIndex + Index].DescStringSegment = EFI_SEGMENT(SataNameDesc);
        BbsTable[BbsIndex + Index].DescStringOffset  = EFI_OFFSET(SataNameDesc); 
      }
    }
  }


// patch multi-AHCI port number issue.
  PortIndex = 0;
  HostIndex = 0;
  BdfInit   = FALSE;
  StrIndex  = 0;
  for (Index = 17; Index < 256; Index++) {
    if(BbsTable[Index].Bus != 0){
      continue;
    }
    if(BbsTable[Index].BootPriority == BBS_IGNORE_ENTRY){
      continue;
    }
    if(BbsTable[Index].Class != 0x01 || BbsTable[Index].SubClass != 0x06){
      continue;
    }
    if(!BdfInit){
      Bus = (UINT8)BbsTable[Index].Bus;
      Dev = (UINT8)BbsTable[Index].Device;
      Func = (UINT8)BbsTable[Index].Function;
      BdfInit = TRUE;
    }  
    if(Bus  != (UINT8)BbsTable[Index].Bus || 
       Dev  != (UINT8)BbsTable[Index].Device || 
       Func != (UINT8)BbsTable[Index].Function){
      AhciBar = MmioRead32(PCI_DEV_MMBASE(Bus, Dev, Func)+0x24);
      AhciBar &= 0xFFFFFFF0;
      MaxPort = (MmioRead32(AhciBar)&0x1F)+1;
      DEBUG((EFI_D_INFO, "(%d,%d,%d)Ports:%d\n", Bus, Dev, Func, MaxPort));
      HostIndex++;    
      PortIndex += MaxPort;
      Bus  = (UINT8)BbsTable[Index].Bus;
      Dev  = (UINT8)BbsTable[Index].Device;
      Func = (UINT8)BbsTable[Index].Function;      
    }   
    
    if(HostIndex){
      SataNameDesc = (CHAR8*)(UINTN)((BbsTable[Index].DescStringSegment << 4) + BbsTable[Index].DescStringOffset);
      if(SataNameDesc == NULL){
        continue;
      }
      if(StrIndex >= 16){
        break;
      }
      if(AsciiStrnCmp(SataNameDesc, "SATA ", 5)){
        continue;
      }

      DEBUG((EFI_D_INFO, "SataName:%a\n", SataNameDesc));
      AhciStr = AllocatePool(AsciiStrSize(SataNameDesc));
      AsciiStrCpy(AhciStr, SataNameDesc);
      SataNameDesc = AhciStr + 5;
      while(*SataNameDesc){
        if(*SataNameDesc >= '0' && *SataNameDesc <= '9'){
          SataNameDesc++;
        } else {
          break;
        }
      }  
      *SataNameDesc = 0;
      ThisPortIndex = AsciiStrDecimalToUintn(AhciStr+5);
      DEBUG((EFI_D_INFO, "ThisPortNo:%d, PortIndex:%d\n", ThisPortIndex, PortIndex));      
      SataNameDesc++;
      if(*SataNameDesc == ' '){SataNameDesc++;}
      TempStr = ((CHAR8*)(UINTN)gIdeDevBbsNameStrAddress) + 32 * 16 + StrIndex * 32;
      AsciiSPrint(TempStr, 
                  32, 
                  "SATA %d: %a", 
                  PortIndex + ThisPortIndex, 
                  SataNameDesc
                  );
      DEBUG((EFI_D_INFO, "%a\n", TempStr));
      BbsTable[Index].DescStringSegment = EFI_SEGMENT(TempStr);
      BbsTable[Index].DescStringOffset  = EFI_OFFSET(TempStr); 
      FreePool(AhciStr);
      StrIndex++;      
    }  
  }

  return EFI_SUCCESS;

}


/**
  Get all BBS info

  @param  This                    Protocol instance pointer.
  @param  HddCount                Number of HDD_INFO structures
  @param  HddInfo                 Onboard IDE controller information
  @param  BbsCount                Number of BBS_TABLE structures
  @param  BbsTable                List BBS entries

  @retval EFI_SUCCESS             Tables returned
  @retval EFI_NOT_FOUND           resource not found
  @retval EFI_DEVICE_ERROR        can not get BBS table

**/
EFI_STATUS
EFIAPI
LegacyBiosGetBbsInfo (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  OUT UINT16                          *HddCount,
  OUT HDD_INFO                        **HddInfo,
  OUT UINT16                          *BbsCount,
  OUT BBS_TABLE                       **BbsTable
  )
{
  LEGACY_BIOS_INSTANCE              *Private;
  EFI_IA32_REGISTER_SET             Regs;
  EFI_TO_COMPATIBILITY16_BOOT_TABLE *EfiToLegacy16BootTable;
//  HDD_INFO                          *LocalHddInfo;
//  IN BBS_TABLE                      *LocalBbsTable;
  UINTN                             NumHandles;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             Index;
  UINTN                             TempData;
  UINT32                            Granularity;

  HandleBuffer            = NULL;

  Private                 = LEGACY_BIOS_INSTANCE_FROM_THIS (This);
  EfiToLegacy16BootTable  = &Private->IntThunk->EfiToLegacy16BootTable;
//  LocalHddInfo            = EfiToLegacy16BootTable->HddInfo;
//  LocalBbsTable           = (BBS_TABLE*)(UINTN)EfiToLegacy16BootTable->BbsTable;

  if (!mBbsTableDoneFlag) {
    //
    // Register Legacy SMI Handler
    //
    Private->LegacyBiosPlatform->SmmInit (
                                   Private->LegacyBiosPlatform,
                                   EfiToLegacy16BootTable
                                   );
                        
    mBbsTable = Private->BbsTablePtr;

    //
    // Always enable disk controllers so 16-bit CSM code has valid information for all
    // drives.
    //
    //
    // Get PciRootBridgeIO protocol
    //
    gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiPciRootBridgeIoProtocolGuid,
          NULL,
          &NumHandles,
          &HandleBuffer
          );

    if (NumHandles == 0) {
      return EFI_NOT_FOUND;
    }

    mBbsTableDoneFlag = TRUE;
    for (Index = 0; Index < NumHandles; Index++) {
      //
      // Connect PciRootBridgeIO protocol handle with FALSE parameter to let
      // PCI bus driver enumerate all subsequent handles
      //
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, FALSE);

    }

    LegacyBiosBuildBbs (Private, mBbsTable);

    Private->LegacyRegion->UnLock (Private->LegacyRegion, 0xe0000, 0x20000, &Granularity);

    //
    // Call into Legacy16 code to add to BBS table for non BBS compliant OPROMs.
    //
    ZeroMem (&Regs, sizeof (EFI_IA32_REGISTER_SET));
    Regs.X.AX = Legacy16UpdateBbs;

    //
    // Pass in handoff data
    //
    TempData  = (UINTN) EfiToLegacy16BootTable;
    Regs.X.ES = EFI_SEGMENT ((UINT32) TempData);
    Regs.X.BX = EFI_OFFSET ((UINT32) TempData);

    DEBUG((EFI_D_INFO, "call %04X:%04X with %04X:%04X\n", \
                       Private->Legacy16CallSegment, Private->Legacy16CallOffset, \
                       Regs.X.ES, Regs.X.BX \
                       ));

    Private->LegacyBios.FarCall86 (
      This,
      Private->Legacy16CallSegment,
      Private->Legacy16CallOffset,
      &Regs,
      NULL,
      0
      );

    DEBUG((EFI_D_INFO, "AX:%X\n", Regs.X.AX));

    Private->Cpu->FlushDataCache (Private->Cpu, 0xE0000, 0x20000, EfiCpuFlushTypeWriteBackInvalidate);
    Private->LegacyRegion->Lock (Private->LegacyRegion, 0xe0000, 0x20000, &Granularity);

    if (Regs.X.AX != 0) {
      return EFI_DEVICE_ERROR;
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  *HddCount = MAX_IDE_CONTROLLER;
  *HddInfo  = EfiToLegacy16BootTable->HddInfo;
  *BbsTable = (BBS_TABLE*)(UINTN)EfiToLegacy16BootTable->BbsTable;
  *BbsCount = (UINT16) (sizeof (Private->IntThunk->BbsTable) / sizeof (BBS_TABLE));
  return EFI_SUCCESS;
}
