/** @file
  This is a test application that demonstrates how to use the C-style entry point
  for a shell application.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/ShellCEntryLib.h>
#include <Uefi/UefiSpec.h>
#include <AtaBusDxe/AtaBus.h>
#include <AtaAtapiPassThru/ComponentName.c>
#include <AtaAtapiPassThru/AtaAtapiPassThru.h>
#include <AtaAtapiPassThru/AtaAtapiPassThru.c>
#include <AtaAtapiPassThru/AhciMode.c>
#include <AtaAtapiPassThru/IdeMode.c>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/HiiLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <IndustryStandard/Atapi.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/PciIo.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/DriverDiagnostics2.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/PlatformDriverOverride.h>
#include <Protocol/BusSpecificDriverOverride.h>
#include <Protocol/PlatformToDriverConfiguration.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/DriverFamilyOverride.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/StorageSecurityCommand.h>
#include <Guid/GlobalVariable.h>
#include <Guid/ConsoleInDevice.h>
#include <Guid/ConsoleOutDevice.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/ShellLibHiiGuid.h>


//
// The maximum model name in ATA identify data
//
#define MAX_MODEL_NAME_LEN                40
#define MAX_FirmwareVer                   8
#define MAX_SerialNo                      20

//
// Length of temp string buffer to store value string.
//
#define _48_BIT_ADDRESS_FEATURE_SET_SUPPORTED   0x0400
#define ATAPI_DEVICE                            0x8000

#define SATA_CFG_IDE              0x01
#define SATA_CFG_AHCI             0x06
#define SATA_CFG_IDE_MAXPORT      0x04
#define SATA_CFG_AHCI_MAXPORT     0x04

#define PCI_LIB_ADDRESS(Bus,Device,Function,Offset)   \
  (((Offset) & 0xfff) | (((Function) & 0x07) << 12) | (((Device) & 0x1f) << 15) | (((Bus) & 0xff) << 20))


VOID
FunctionPcidump(
  IN UINTN       PciAddress
  )
{
    UINT8        Index1;
    UINT8        Index2;
    UINT8        Data;

	DEBUG((EFI_D_ERROR, "     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"));
    Print(L"     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	DEBUG((EFI_D_ERROR, "---+------------------------------------------------\n"));
    Print(L"---+------------------------------------------------\n");
	for(Index1=0;Index1<0x10;Index1++)
	{
		DEBUG((EFI_D_ERROR, "%02x | ",Index1*16));
        Print(L"%02x | ",Index1*16);
		for(Index2=0;Index2<0x10;Index2++)
		{
			Data = PciRead8(PciAddress|(Index1*16+Index2));
			DEBUG((EFI_D_ERROR, "%02x ",Data));
            Print(L"%02x ",Data);
			if(Index2==0x0F)
            {
				DEBUG((EFI_D_ERROR,"\n"));
                Print(L"\n");
            }
		}
		if((Index1%0x10)==0x0F)
        {
			DEBUG((EFI_D_ERROR, "--------------------------------------------------\n"));
            Print(L"----------------------------------------------------\n");
        }
	}
	DEBUG((EFI_D_ERROR, "\n")); 
    Print(L"\n"); 
}


EFI_STATUS
EFIAPI
PrintDevicePath( 
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
    EFI_STATUS                            Status;
    EFI_DEVICE_PATH_PROTOCOL             *DevPathNode;
    CHAR16                               *Textdevicepath =NULL;
    EFI_DEVICE_PATH_TO_TEXT_PROTOCOL     *Device2TextProtocol=NULL;

    Status=gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid,NULL,(VOID**)&Device2TextProtocol);
    ASSERT_EFI_ERROR (Status);
    
    DevPathNode=DevicePath;
    if(!IsDevicePathEnd(DevPathNode))
    {
      Textdevicepath=Device2TextProtocol->ConvertDevicePathToText(DevPathNode,TRUE,TRUE);
      Print(L"%s",Textdevicepath);
      Print(L"\n");
      if(Textdevicepath) gBS->FreePool(Textdevicepath);
    }

    DevPathNode=DevicePath;
    if(!IsDevicePathEnd(DevPathNode))
    {
      Print(L"(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
      DevPathNode=NextDevicePathNode(DevPathNode);
    }
    while(!IsDevicePathEnd(DevPathNode))
    {
      Print(L"/(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
      DevPathNode=NextDevicePathNode(DevPathNode);
    }
    Print(L"\n\n");

    return EFI_SUCCESS; 
}


VOID
PrintAtaFirmwareVer(
  IN EFI_IDENTIFY_DATA  *Buffer
  )
{
  UINT8      Index;
  CHAR16     FirmwareVer[MAX_FirmwareVer+1];
  CHAR8     *Source;

  Source      = Buffer->AtaData.ModelName;

  //
  // Swap the byte order in the original module name.
  //
  for (Index = 0; Index < MAX_FirmwareVer; Index += 2) {
    FirmwareVer[Index]      = Source[Index + 1];
    FirmwareVer[Index + 1]  = Source[Index];
  }
  FirmwareVer[MAX_FirmwareVer] = L'\0';
  
  Print (L"[FirmwareVer]            : %s\n",FirmwareVer);
}


VOID
PrintAtaSerialNo(
  IN EFI_IDENTIFY_DATA  *Buffer
  )
{
  UINT8      Index;
  CHAR16     SerialNo[MAX_FirmwareVer+1];
  CHAR8     *Source;

  Source      = Buffer->AtaData.ModelName;

  //
  // Swap the byte order in the original module name.
  //
  for (Index = 0; Index < MAX_SerialNo; Index += 2) {
    SerialNo[Index]      = Source[Index + 1];
    SerialNo[Index + 1]  = Source[Index];
  }
  SerialNo[MAX_SerialNo] = L'\0';
  
  Print (L"[SerialNo]               : %s\n",SerialNo);
}


VOID
PrintAtaModelName(
  IN EFI_IDENTIFY_DATA  *Buffer
  )
{
  UINT8      Index;
  CHAR16     ModelName[MAX_MODEL_NAME_LEN + 1];
  CHAR8     *Source;

  Source      = Buffer->AtaData.ModelName;

  //
  // Swap the byte order in the original module name.
  //
  for (Index = 0; Index < MAX_MODEL_NAME_LEN; Index += 2) {
    ModelName[Index]      = Source[Index + 1];
    ModelName[Index + 1]  = Source[Index];
  }
  ModelName[MAX_MODEL_NAME_LEN] = L'\0';
  
  Print (L"[MODEL]                  : %s\n",ModelName);
}


/**
  Prints ATA model name to ATA device structure.

  This function converts ATA device model name from ATA identify data
  to a string in ATA device structure. It needs to change the character
  order in the original model name string.

  @param  AtaDevice         The ATA child device involved for the operation.

**/
VOID
PrintAtaModelSize(
  IN EFI_IDENTIFY_DATA  *Buffer
  )
{
  	UINT64    NumSectors       = 0; 
	UINT64    DriveSizeInBytes = 0;
	UINT32    RemainderInBytes = 0;
	UINT32    DriveSizeInGB    = 0;
	UINTN     NumTenthsOfGB    = 0;

    // For HardDisk append the size. Otherwise display atapi
    if (!(Buffer->AtaData.config & ATAPI_DEVICE))
    {
        if (Buffer->AtaData.command_set_supported_83 & _48_BIT_ADDRESS_FEATURE_SET_SUPPORTED)
        { 
            NumSectors = (Buffer->AtaData.maximum_lba_for_48bit_addressing[0])|(((UINT64)Buffer->AtaData.maximum_lba_for_48bit_addressing[1])<<16)|(((UINT64)Buffer->AtaData.maximum_lba_for_48bit_addressing[2])<<32)|(((UINT64)Buffer->AtaData.maximum_lba_for_48bit_addressing[3])<<48); 
        }
        else
        {
            NumSectors = Buffer->AtaData.user_addressable_sectors_lo|(Buffer->AtaData.user_addressable_sectors_hi<<16); 
        }
        
        DriveSizeInBytes = MultU64x32(NumSectors, 512); 
    
        //DriveSizeInGB is DriveSizeInBytes / 1 GB (1 Decimal GB = 10^9 bytes)
        DriveSizeInGB = (UINT32) DivU64x32Remainder(DriveSizeInBytes, 1000000000, &RemainderInBytes);
        
        //Convert the Remainder, which is in bytes, to number of tenths of a Decimal GB.
        NumTenthsOfGB = RemainderInBytes / 100000000;
    
        Print (L"[SIZE]                   : %d.%d GB\n",DriveSizeInGB, NumTenthsOfGB);
    }
    else
    {
  	  Print (L"[SIZE]                   : ATAPI\n",DriveSizeInGB, NumTenthsOfGB);
    }
}


EFI_STATUS
EFIAPI
DeviceFeatureIdentify(
    IN ATA_ATAPI_PASS_THRU_INSTANCE   *Instance, 
    IN UINTN                           Satacfg,
    IN EFI_PCI_IO_PROTOCOL            *PciIo,
    IN UINT8                           Port
    )
{
    EFI_STATUS                         Status = EFI_UNSUPPORTED;
    UINT8                              SStatus;
    UINT32                             AHCISStatus;
	EFI_IDENTIFY_DATA                  Buffer;
	UINT32                             Offset;

    
    if(Satacfg==SATA_CFG_IDE)
    {
	    Status = PciIo->Pci.Read (
	                      PciIo,
	                      EfiPciIoWidthUint8,
	                      0xA0+Port,
	                      0x1,
	                      &SStatus
	                      );
		if((SStatus&(BIT0|BIT1))!=(BIT0|BIT1))
		{
			return EFI_UNSUPPORTED;
		}
		
	    Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
    }
    else if(Satacfg==SATA_CFG_AHCI)
    {
        Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
        AHCISStatus = AhciReadReg (PciIo, Offset);
        if((AHCISStatus&(BIT0|BIT1))!=(BIT0|BIT1))
		{
			return EFI_UNSUPPORTED;
		}
		
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
    }

    if(!EFI_ERROR(Status))
    {
        Print (L"[Port%d]:\n", Port);
        PrintAtaModelName(&Buffer);    
        PrintAtaFirmwareVer(&Buffer);
        //PrintAtaSerialNo(&Buffer);
        PrintAtaModelSize(&Buffer); 
        
        //// SATA Speed
        if((Buffer.AtaData.serial_ata_capabilities&BIT3)==BIT3)
        {  
            Print (L"[SPEED]                  : Gen3, %d rpm\n", Buffer.AtaData.nominal_media_rotation_rate);
        }else if((Buffer.AtaData.serial_ata_capabilities&BIT2)==BIT2)
        {  
            Print (L"[SPEED]                  : Gen2, %d rpm\n", Buffer.AtaData.nominal_media_rotation_rate);
        }else if((Buffer.AtaData.serial_ata_capabilities&BIT3)==BIT3)
        {  
            Print (L"[SPEED]                  : Gen1, %d rpm\n", Buffer.AtaData.nominal_media_rotation_rate);
        }

        //// DMA Transfer Mode
        if ((Buffer.AtaData.field_validity&BIT2)==BIT2) {
	        if((Buffer.AtaData.multi_word_dma_mode&BIT2)==BIT2)
	        {  
	            Print (L"[DMA]                    : MDMA1 - MDMA2, ");
	        }else if((Buffer.AtaData.serial_ata_capabilities&BIT1)==BIT1)
	        {  
	            Print (L"[DMA]                    : MDMA1, ");
	        }

	        if((Buffer.AtaData.ultra_dma_mode&BIT6)==BIT6)
	        {  
	            Print (L"UMode0 - UDMA6\n");
	        }else if((Buffer.AtaData.ultra_dma_mode&BIT5)==BIT5)
	        {  
	            Print (L"UMode0 - UDMA5\n");
	        }else if((Buffer.AtaData.ultra_dma_mode&BIT4)==BIT4)
	        {  
	            Print (L"UMode0 - UDMA4\n");
	        }else if((Buffer.AtaData.ultra_dma_mode&BIT3)==BIT3)
	        {  
	            Print (L"UMode0 - UDMA3\n");
	        }else if((Buffer.AtaData.ultra_dma_mode&BIT2)==BIT2)
	        {  
	            Print (L"UMode0 - UMode2\n");
	        }else if((Buffer.AtaData.ultra_dma_mode&BIT1)==BIT1)
	        {  
	            Print (L"UMode0 - UMode1\n");
	        }else if((Buffer.AtaData.ultra_dma_mode&BIT0)==BIT0)
	        {  
	            Print (L"UMode0\n");
	        }
	    }
        
        //NCQ support
        if((Buffer.AtaData.serial_ata_capabilities&BIT8)==BIT8){
            Print (L"[NCQ]                    : YES\n");
        }else{
            Print (L"[NCQ]                    : NO\n");
        }  

        //HIPM support
        if((Buffer.AtaData.serial_ata_capabilities&BIT9)==BIT9){
            Print (L"[HIPM]                   : YES\n");
        }else{  
            Print (L"[HIPM]                   : NO\n");
        }

        //DIPM support
        if((Buffer.AtaData.serial_ata_features_supported&BIT3)==BIT3){
            Print (L"[DIPM]                   : YES, ");
        }else{
            Print (L"[DIPM]                   : NO,  ");
        }
        //DIPM enable
        if((Buffer.AtaData.serial_ata_features_enabled&BIT3)==BIT3){
            Print (L"ENABLE\n");
        }else{
            Print (L"DISABLE\n");
        }

        //Device Sleep support
        if((Buffer.AtaData.serial_ata_features_supported&BIT8)==BIT8){
            Print (L"[Device Sleep]           : YES, ");
        }else{
            Print (L"[Device Sleep]           : NO,  ");
        }
        //Device Sleep enable
        if((Buffer.AtaData.serial_ata_features_enabled&BIT8)==BIT8){
            Print (L"ENABLE\n");
        }else{
            Print (L"DISABLE\n");
        } 

        // S.M.A.R.T support
		if ((Buffer.AtaData.command_set_supported_82&BIT0)==BIT0){
			Print (L"[S.M.A.R.T]              : YES, ");
		}else{
			Print (L"[S.M.A.R.T]              : NO,  ");
		}
		// S.M.A.R.T enable
		if ((Buffer.AtaData.command_set_feature_enb_85&BIT0)==BIT0){
            Print (L"ENABLE\n");
        }else{
            Print (L"DISABLE\n");
        }

        // 48Bit LBA support
		if ((Buffer.AtaData.command_set_supported_83&BIT10)==BIT10){
			Print (L"[48Bit LBA]              : YES, ");
		}else{
			Print (L"[48Bit LBA]              : NO,  ");
		}
		// 48Bit LBA enable
		if ((Buffer.AtaData.command_set_feature_enb_86&BIT10)==BIT10){
            Print (L"ENABLE\n");
        }else{
            Print (L"DISABLE\n");
        }
        
        //Device Auto partial to slumber support
        if((Buffer.AtaData.serial_ata_capabilities&BIT14)==BIT14){
            Print (L"[Auto Partial to Slumber]: YES, ");
        }else{
            Print (L"[Auto Partial to Slumber]: NO,  ");
        }
        //Device Auto partial to slumber enable
        if((Buffer.AtaData.serial_ata_features_enabled&BIT7)==BIT7){
            Print (L"ENABLE\n");
        }else{
            Print (L"DISABLE\n");
        }
    }else{
        Print (L"[ERROR]                : Timeout!\n");
    }
    
    Print(L"\n");

    return EFI_SUCCESS;
}



/**
  UEFI application entry point which has an interface similar to a
  standard C main function.

  The ShellCEntryLib library instance wrappers the actual UEFI application
  entry point and calls this ShellAppMain function.

  @param[in] Argc     The number of items in Argv.
  @param[in] Argv     Array of pointers to strings.

  @retval  0               The application exited normally.
  @retval  Other           An error occurred.

**/
INTN
EFIAPI
ShellAppMain(
  IN UINTN        Argc,
  IN CHAR16     **Argv
  )
{
    EFI_STATUS                             Status;
    UINTN                                  Count;
    EFI_HANDLE                            *HandleAtaatapipassthru;
    UINT8                                  i = 0;
    UINT8                                  j = 0;
    EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
    ATA_ATAPI_PASS_THRU_INSTANCE          *Instance = NULL;
    EFI_ATA_PASS_THRU_PROTOCOL            *AtaPassThru;
    EFI_PCI_IO_PROTOCOL                   *PciIo=NULL;
    UINT8                                  ZXBus = 0;
    UINT8                                  ZXDev = 0;
    UINT8                                  ZXFunc= 0;
    UINTN                                  ZX_SATA;
    UINTN                                  Segment;
    UINTN                                  Bus;
    UINTN                                  Dev;
    UINTN                                  Func;
    UINT8                                  Satacfg;
    BOOLEAN                                Found_Flag = FALSE;


    /////////// Get the handle of SATA controller /////////////////////
    Status=gBS->LocateHandleBuffer(
	                    ByProtocol,
	                    &gEfiAtaPassThruProtocolGuid,
	                    NULL,
	                    &Count,
	                    &HandleAtaatapipassthru
	                    );
    if (EFI_ERROR (Status))
    {
        Print(L"[ERROR]: Locate Handle With PassThrough Protocol Error!\n");
    }

    /////////////////////////////////////////////////////////////////////////////////
    if(Argc==4)
    {
        ZXBus   = (UINT8)StrHexToUintn(Argv[1]);
        ZXDev   = (UINT8)StrHexToUintn(Argv[2]);
        ZXFunc  = (UINT8)StrHexToUintn(Argv[3]);
	    ZX_SATA = PCI_LIB_ADDRESS(ZXBus, ZXDev, ZXFunc, 0);
	    Satacfg = PciRead8(ZX_SATA|0x0A);
        if(Satacfg!=SATA_CFG_IDE && Satacfg!=SATA_CFG_AHCI)
        {
	        Print(L"[ERROR]: NO SATA CONTROLLER FOUND At Bus%d D%dF%d!\n", ZXBus, ZXDev, ZXFunc);
	        return EFI_UNSUPPORTED;
        }
        DEBUG((EFI_D_ERROR, "Dump SATA PCI Config Space:\n"));
	    Print(L"Dump SATA PCI Config Space:\n");
	    FunctionPcidump(ZX_SATA);
    }else if(Argc!=1){
	    Print(L"[Usage]:\n");
	    Print(L"SataIdentify.efi\n");
	    Print(L"SataIdentify.efi Bus Dev Func\n");
	    return EFI_UNSUPPORTED;
    }
    
    for(i=0; i<Count; i++)
    {
        Status=gBS->HandleProtocol(
                      HandleAtaatapipassthru[i],
                      &gEfiDevicePathProtocolGuid,
                      (VOID**) &DevicePath
                      );
        ASSERT_EFI_ERROR (Status);
        
        Status = gBS->HandleProtocol (
                       HandleAtaatapipassthru[i],
                       &gEfiPciIoProtocolGuid,
                       (VOID**) &PciIo
                       );
        ASSERT_EFI_ERROR (Status);

        Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint8,
                          0x0A,
                          0x1,
                          &Satacfg
                          );
        ASSERT_EFI_ERROR (Status); 
        
        if(Satacfg==SATA_CFG_IDE || Satacfg==SATA_CFG_AHCI)
        {
            Status = PciIo->GetLocation (
                              PciIo,
                              &Segment,
                              &Bus,
                              &Dev,
                              &Func
                              );
			if(Argc==4)
			{
				if((Bus!=ZXBus)||(Dev!=ZXDev)||(Func!=ZXFunc))
				{
		            continue;
		        }else
		        {
				    Found_Flag = TRUE;
				}
			}else
			{
				Print(L"[CONTROLLER]: Bus%d D%dF%d, ", Bus, Dev, Func);
			}
			
            Status  = gBS->HandleProtocol (
                             HandleAtaatapipassthru[i],
                             &gEfiAtaPassThruProtocolGuid,
                             (VOID **) &AtaPassThru
                             );
            ASSERT_EFI_ERROR (Status);
            
            Instance = ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS (AtaPassThru);        
          
            if(Satacfg==SATA_CFG_IDE)
            {
                Print(L"IN IDE MODE!\n");
				PrintDevicePath(DevicePath);
                for(j=0; j<SATA_CFG_IDE_MAXPORT ;j++)
                {
                    Status = DeviceFeatureIdentify(Instance, Satacfg, PciIo, j);
                }
            }
            else if(Satacfg==SATA_CFG_AHCI)
            {
                Print(L"IN AHCI MODE!\n");
				PrintDevicePath(DevicePath);
                for(j=0; j<SATA_CFG_AHCI_MAXPORT ;j++)
                {
                    Status = DeviceFeatureIdentify(Instance, Satacfg, PciIo, j);
                }
            }
        }

        if(Found_Flag)
        {
            break;
        }
    }
    if(Argc==4 && Found_Flag ==FALSE)
    {
	    Print(L"[ERROR]: NO SATA Controller Handle Found!\n");
    }
    return EFI_SUCCESS;
}
