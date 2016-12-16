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
DeviceFeatureSet(
    IN ATA_ATAPI_PASS_THRU_INSTANCE   *Instance, 
    IN UINTN                           Satacfg,
    IN EFI_PCI_IO_PROTOCOL            *PciIo,
    IN UINT8                           Port,
    IN UINTN                           Feature,
    IN UINTN                           Count
    )
{
    EFI_STATUS                         Status = EFI_UNSUPPORTED;
    UINT8                              SStatus;
    UINT32                             AHCISStatus;
	UINT32                             Offset;
    EFI_ATA_COMMAND_BLOCK              AtaCommandBlock;
    EFI_ATA_STATUS_BLOCK               AtaStatusBlock;

    ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
    ZeroMem (&AtaStatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));
    
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
    
	    AtaCommandBlock.AtaCommand     = ATA_CMD_SET_FEATURES;
	    AtaCommandBlock.AtaDeviceHead  = (UINT8) ((Port%2) << 0x4);
	    AtaCommandBlock.AtaFeatures    = (UINT8) Feature;
	    AtaCommandBlock.AtaSectorCount = (UINT8) Count;
	    
	    Status = AtaNonDataCommandIn (
			             PciIo,
			             &Instance->IdeRegisters[Port],
			             &AtaCommandBlock,
			             &AtaStatusBlock,
			             ATA_ATAPI_TIMEOUT,
			             NULL
			             );
    
    }
    else if(Satacfg==SATA_CFG_AHCI)
    {
        Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
        AHCISStatus = AhciReadReg (PciIo, Offset);
        if((AHCISStatus&(BIT0|BIT1))!=(BIT0|BIT1))
		{
			return EFI_UNSUPPORTED;
		}
		
	    AtaCommandBlock.AtaCommand      = ATA_CMD_SET_FEATURES;
	    AtaCommandBlock.AtaFeatures     = (UINT8) Feature;
	    AtaCommandBlock.AtaFeaturesExp  = (UINT8) (Feature >> 8);
	    AtaCommandBlock.AtaSectorCount  = (UINT8) Count;

	    Status = AhciNonDataTransfer (
			             PciIo,
			             &Instance->AhciRegisters,
			             Port,
			             0,
			             NULL,
			             0,
			             &AtaCommandBlock,
			             &AtaStatusBlock,
			             ATA_ATAPI_TIMEOUT,
			             NULL
			             );
    }

    if(!EFI_ERROR(Status))
    {
        Print(L"SET FEATURE SUCCESS!\n");
	    return EFI_SUCCESS;
	}else
	{
	    Print(L"SET FEATURE Fail!\n");
		return EFI_UNSUPPORTED;
	}
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
    EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
    ATA_ATAPI_PASS_THRU_INSTANCE          *Instance = NULL;
    EFI_ATA_PASS_THRU_PROTOCOL            *AtaPassThru;
    EFI_PCI_IO_PROTOCOL                   *PciIo=NULL;
    UINTN                                  ZXBus = 0;
    UINTN                                  ZXDev = 0;
    UINTN                                  ZXFunc= 0;
    UINTN                                  SATAPort;
    UINTN                                  SATAFeature;
    UINTN                                  SATACount;
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
    if(Argc==7)
    {
        ZXBus       = StrHexToUintn(Argv[1]);
        ZXDev       = StrHexToUintn(Argv[2]);
        ZXFunc      = StrHexToUintn(Argv[3]);
        SATAPort    = StrHexToUintn(Argv[4]);
        SATAFeature = StrHexToUintn(Argv[5]);
        SATACount   = StrHexToUintn(Argv[6]);
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
    }else{
	    Print(L"[Usage]:\n");
	    Print(L"SataSetfeature.efi Bus Dev Func Port Feature Count\n");
	    Print(L"\nFeatures(7:0):\n");
		Print(L"10h     : Enable use of Serial ATA feature\n");
		Print(L"90h     : Disable use of Serial ATA feature\n");
		Print(L"\nCount(7:0):\n");
		Print(L"01h     : Non-zero buffer offset in DMA Setup FIS\n");
		Print(L"02h     : DMA Setup FIS Auto-Activate optimization\n");
		Print(L"03h     : Device-initiated interface power state transitions\n");
		Print(L"04h     : Guaranteed In-Order Data Delivery\n");
		Print(L"05h     : Asynchronous Notification\n");
		Print(L"06h     : Software Settings Preservation\n");
		Print(L"07h     : Device Automatic Partial to Slumber transitions\n");
		Print(L"08h     : Enable Hardware Feature Control\n");
		Print(L"09h     : Enable Device Sleep\n");
		Print(L"0Ah     : Enable/Disable Hybrid Information\n");
		Print(L"0Bh..FFh: Reserved for future Serial ATA definition\n");
	    return EFI_UNSUPPORTED;
    }
    
    for(i=0; i<Count; i++)
    {
        Status=gBS->HandleProtocol(
                      HandleAtaatapipassthru[i],
                      &gEfiDevicePathProtocolGuid,
                      (VOID**) &DevicePath
                      );
      
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
			if((Bus!=ZXBus)||(Dev!=ZXDev)||(Func!=ZXFunc))
			{
	            continue;
	        }else
	        {
			    Found_Flag = TRUE;
				Status  = gBS->HandleProtocol (
	                             HandleAtaatapipassthru[i],
	                             &gEfiAtaPassThruProtocolGuid,
	                             (VOID **) &AtaPassThru
	                             );
	            ASSERT_EFI_ERROR (Status);
	            
	            Instance = ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS (AtaPassThru);        

	            Status = DeviceFeatureSet(Instance, Satacfg, PciIo,  (UINT8)SATAPort, SATAFeature, SATACount);
			}
        }

        if(Found_Flag)
        {
            break;
        }
    }
    
    if(Found_Flag ==FALSE)
    {
	    Print(L"[ERROR]: NO SATA Controller Handle Found!\n");
    }
    return EFI_SUCCESS;
}
