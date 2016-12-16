/** @file
  This is a simple shell application

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DevicePathFromText.h>
#include <AtaAtapiPassThru/ComponentName.c>
#include <AtaAtapiPassThru/AtaAtapiPassThru.h>
#include <AtaAtapiPassThru/AtaAtapiPassThru.c>
#include <AtaAtapiPassThru/AhciMode.c>
#include <AtaAtapiPassThru/IdeMode.c>


#define SATA_CFG_IDE                          0x01
#define SATA_CFG_AHCI                         0x06
#define SATA_CFG_IDE_MAXPORT                  0x04
#define SATA_CFG_AHCI_MAXPORT                 0x04
#define SATA_CFG_PARTIAL                      0x00
#define SATA_CFG_SLUMBER                      0x01

#define IDE_PORT_SELECTOR                     0x42
#define IDE_AUTO_PARTIAL_TO_SLUMBER_ENABLE    0x70
#define IDE_PHY_PARTIAL_SLUMBER_ENABLE        0x84
#define IDE_PHY_DRIVE_CONTROL                 0x8C
#define IDE_PORT0_SSTATUS                     0xA0
#define IDE_PORT0_SCONTROL                    0xA4
#define IDE_PORT_SERROR                       0xA8
#define AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR     0xFA

EFI_STATUS
EFIAPI
TimeDelay(
  IN UINT8   Count
  )
{
	UINT8  Tmp;
    
    for(Tmp=0; Tmp<Count; Tmp++){
    	MicroSecondDelay(2000);
    }
    
    return EFI_SUCCESS;
}



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


EFI_STATUS
EFIAPI
IDE_EnableDIPM (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                          Channel,
  IN     UINT8                          Device,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{ 
    EFI_STATUS                     Status;
    EFI_ATA_COMMAND_BLOCK          AtaCommandBlock;
    
    ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
    
    AtaCommandBlock.AtaCommand     = ATA_CMD_SET_FEATURES;
    AtaCommandBlock.AtaDeviceHead  = (UINT8)(Device << 0x4);
    AtaCommandBlock.AtaFeatures    = 0x10;
    AtaCommandBlock.AtaSectorCount = 0x03;
    
    //
    // Send SET FEATURE command (sub command 0x10) to initiating interface power management .
    //
    Status = AtaNonDataCommandIn (
             Instance->PciIo,
             &Instance->IdeRegisters[Channel],
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_ATAPI_TIMEOUT,
             NULL
             );
    
    return Status;
}

EFI_STATUS
EFIAPI
IDE_DisableDIMP (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                          Channel,
  IN     UINT8                          Device,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{
    EFI_STATUS                     Status;
    EFI_ATA_COMMAND_BLOCK          AtaCommandBlock;
    
    ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
    
    AtaCommandBlock.AtaCommand     = ATA_CMD_SET_FEATURES;
    AtaCommandBlock.AtaDeviceHead  = (UINT8)(Device << 0x4);
    AtaCommandBlock.AtaFeatures    = 0x90;
    AtaCommandBlock.AtaSectorCount = 0x03;
    
    //
    // Send SET FEATURE command (sub command 0x10) to initiating interface power management .
    //
    Status = AtaNonDataCommandIn (
             Instance->PciIo,
             &Instance->IdeRegisters[Channel],
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_ATAPI_TIMEOUT,
             NULL
             );
    
    return Status;
}

EFI_STATUS
EFIAPI
IDE_EnableAutoPTS (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                          Channel,
  IN     UINT8                          Device,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{ 
    EFI_STATUS                     Status;
    EFI_ATA_COMMAND_BLOCK          AtaCommandBlock;
    
    ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
    
    AtaCommandBlock.AtaCommand     = ATA_CMD_SET_FEATURES;
    AtaCommandBlock.AtaDeviceHead  = (UINT8)(Device << 0x4);
    AtaCommandBlock.AtaFeatures    = 0x10;
    AtaCommandBlock.AtaSectorCount = 0x07;
    
    //
    // Send SET FEATURE command (sub command 0x10) to initiating interface power management .
    //
    Status = AtaNonDataCommandIn (
             Instance->PciIo,
             &Instance->IdeRegisters[Channel],
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_ATAPI_TIMEOUT,
             NULL
             );
    
    return Status;
}


EFI_STATUS
EFIAPI
IDE_DisableAutoPTS (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                          Channel,
  IN     UINT8                          Device,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{
    EFI_STATUS                     Status;
    EFI_ATA_COMMAND_BLOCK          AtaCommandBlock;
    
    ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
    
    AtaCommandBlock.AtaCommand     = ATA_CMD_SET_FEATURES;
    AtaCommandBlock.AtaDeviceHead  = (UINT8)(Device << 0x4);
    AtaCommandBlock.AtaFeatures    = 0x90;
    AtaCommandBlock.AtaSectorCount = 0x07;
    
    //
    // Send SET FEATURE command (sub command 0x10) to initiating interface power management .
    //
    Status = AtaNonDataCommandIn (
             Instance->PciIo,
             &Instance->IdeRegisters[Channel],
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_ATAPI_TIMEOUT,
             NULL
             );
    
    return Status;
}

EFI_STATUS
EFIAPI
IDE_HIPM_Special(
  IN ATA_ATAPI_PASS_THRU_INSTANCE        *Instance, 
  IN UINT32                               PowerMode,
  IN EFI_PCI_IO_PROTOCOL                 *PciIo,
  IN UINT8                                Port
  )
{
	//EFI_IDE_REGISTERS          *IdeRegisters;
	EFI_STATUS                   Status;
	UINT8                        Data    =0;
	UINT32                       Data32;
	UINT32                       Delay;
    EFI_IDENTIFY_DATA            Buffer;
  
   
    Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT0_SSTATUS+Port,
                        0x1,
                        &Data
                        ); 
    
    if(Data==0x13||Data==0x23||Data==0x33)
    {
        Print(L"[Port%d HIPM Special]: ", Port);
           
        Print (L"Enter %a ", PowerMode?"Slumber":"Partial");
        
        //Step 1 Write A4/A5/A6/A7 8'h0
        Data = 0;
        Status = PciIo->Pci.Read(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
        if(PowerMode==SATA_CFG_PARTIAL){
           Data = ((Data|(BIT2|BIT3))&(~(BIT2)));
        }else if(PowerMode==SATA_CFG_SLUMBER){
           Data = ((Data|(BIT2|BIT3))&(~(BIT3)));
        }
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           );
   
        //Step 2 Check A0/A1/A2/A3 8'h13 8'h23 8'h33
        Status = PciIo->Pci.Read(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SSTATUS+Port,
                           0x1,
                           &Data
                           ); 
   
        if((Data!=0x13)&&(Data!=0x23)&&(Data!=0x33)){
            Print(L"[ERROR]: PHY not Ready!\n");
            return EFI_DEVICE_ERROR;
        }
   
        //Step 3 Write IDE_PORT_SELECTOR Select Port
        Data = Port;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT_SELECTOR,
                           0x1,
                           &Data
                           ); 
   
        //Step 4 Write A8 32'hffffffff
        Data32 = 0xffffffff;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint32,
                           IDE_PORT_SERROR,
                           0x1,
                           &Data32
                           );
   
        //Step 5 Write IDE_PHY_PARTIAL_SLUMBER_ENABLE to enable Power Management Timer
        Data = 0;
        Status = PciIo->Pci.Read(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PHY_PARTIAL_SLUMBER_ENABLE,
                           0x1,
                           &Data
                           );
        if(PowerMode==SATA_CFG_PARTIAL){
            Data = (Data&(~(BIT5|BIT6))|BIT5);
        }else if(PowerMode==SATA_CFG_SLUMBER){
            Data = (Data&(~(BIT5|BIT6))|BIT6);
        }
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PHY_PARTIAL_SLUMBER_ENABLE,
                           0x1,
                           &Data
                           );

        //Step 6 Check A1/A2/A3/A4 8'h09
        Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
        Delay = 100;
        do{      
            TimeDelay(20);
            //Step 7 Check A0/A1/A2/A3 8'h09
            Status = PciIo->Pci.Read(
                              PciIo,
                              EfiPciIoWidthUint8,
                              IDE_PORT0_SSTATUS+Port,
                              0x1,
                              &Data
                              );
             Data&=0x0F;
      
             //Step 8 Check IDE_PORT_SERROR 32'h00030000
             Status = PciIo->Pci.Read(
                              PciIo,
                              EfiPciIoWidthUint32,
                              IDE_PORT_SERROR,
                              0x1,
                              &Data32
                              );
  
             if(PowerMode==SATA_CFG_PARTIAL){
                 if(Data==0x05){
                     Print(L"Success, ");
                     break;
                 }
             }else if(PowerMode==SATA_CFG_SLUMBER){
                 if(Data==0x09){
                     Print(L"Success, ");
                     break;
                 }
             }
             
             Delay--;
        }while(Delay>0);
   
        if(Delay<=0)
        {
		    Print(L"Failed, ");
        }
   
        /////////////////////////////////////////////////////////////////////////////////
        Print(L"Exit %a ",PowerMode?"Slumber":"Partial");
   
        //Step 1 Write A4/A5/A6/A7 8'h6 Disable Slumber/Partial
        Data = 0;
        Status = PciIo->Pci.Read(
	                       PciIo,
	                       EfiPciIoWidthUint8,
	                       IDE_PORT0_SCONTROL+Port,
	                       0x1,
	                       &Data
                       ); 
        Data = Data | 0x0C;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
   
        //Step 2 Write IDE_PORT_SELECTOR Select Port
        Data = Port;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT_SELECTOR,
                           0x1,
                           &Data
                           ); 
   
        //Step 3 Write IDE_PHY_DRIVE_CONTROL  WakeUp 
        Data = 0;
        Status = PciIo->Pci.Read(
	                       PciIo,
	                       EfiPciIoWidthUint8,
	                       IDE_PHY_DRIVE_CONTROL,
	                       0x1,
	                       &Data
	                       ); 
        Data = Data | BIT0;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PHY_DRIVE_CONTROL,
                           0x1,
                           &Data
                           ); 
        Data = Data & 0xFE;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PHY_DRIVE_CONTROL,
                           0x1,
                           &Data
                           );
        
        //Step 4 Check A0/A1/A2/A3 8'h13 23h 33h
        Delay = 100;
        do{
	        Status = PciIo->Pci.Read(
							PciIo,
							EfiPciIoWidthUint8,
							IDE_PORT0_SSTATUS+Port,
							0x1,
							&Data
							);
			DEBUG((EFI_D_ERROR,"Read Rx%x=%x\n",IDE_PORT0_SSTATUS+Port,Data));
			TimeDelay(20);
			if(Data==0x13||Data==0x23||Data==0x33){
				Print(L"Success!\n");
				Data = 0;
				Status = PciIo->Pci.Read(
								PciIo,
								EfiPciIoWidthUint8,
								IDE_PHY_PARTIAL_SLUMBER_ENABLE,
								0x1,
								&Data
								); 
				Data |= (BIT5|BIT6);
				Status = PciIo->Pci.Write(
								PciIo,
								EfiPciIoWidthUint8,
								IDE_PHY_PARTIAL_SLUMBER_ENABLE,
								0x1,
								&Data
								); 
				break;
			}
			Delay--;
        }while(Delay>0);
        if(Delay<=0)
        {
             Print(L"Failed!\n");
             return EFI_DEVICE_ERROR;
        }
    }
    
    return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
IDE_HIPM_AUTOPTS(
  IN ATA_ATAPI_PASS_THRU_INSTANCE        *Instance, 
  IN EFI_PCI_IO_PROTOCOL                 *PciIo,
  IN UINT8                                Port
  )
{
	EFI_STATUS                   Status;
	UINT8                        Data    =0;
	UINT32                       Data32;
	INT32                        Delay;
    EFI_IDENTIFY_DATA            Buffer;

 
    Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT0_SSTATUS+Port,
                        0x1,
                        &Data
                        ); 
    if(Data==0x13||Data==0x23||Data==0x33)
    {
   
        Print(L"[Port%d HIPM AutoPTS]: ", Port);
   
        Print (L"Enter ");
        
        //Step 1 Write A4/A5/A6/A7 8'h0
        Data = 0;
        Status = PciIo->Pci.Read(
	                       PciIo,
	                       EfiPciIoWidthUint8,
	                       IDE_PORT0_SCONTROL+Port,
	                       0x1,
	                       &Data
	                       ); 
        Data = Data & (~(BIT2|BIT3));
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           );
   
        //Step 2 Check A0/A1/A2/A3 8'h13 8'h23 8'h33
        Status = PciIo->Pci.Read(
						PciIo,
						EfiPciIoWidthUint8,
						IDE_PORT0_SSTATUS+Port,
						0x1,
						&Data
						); 
   
        if((Data!=0x13)&&(Data!=0x23)&&(Data!=0x33)){
            Print(L"[ERROR]: PHY not Ready!\n");
            return EFI_DEVICE_ERROR;
        }
   
        //Step3 Enable Auto Partial to Slumber
        Data = Port;
        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT_SELECTOR,
                        0x1,
                        &Data
                        );
        Data = 0;
        Status = PciIo->Pci.Read(
				        PciIo,
				        EfiPciIoWidthUint8,
				        IDE_AUTO_PARTIAL_TO_SLUMBER_ENABLE,
				        0x1,
				        &Data
				        );
        Data = Data | BIT0;
        Status = PciIo->Pci.Write(
				        PciIo,
				        EfiPciIoWidthUint8,
				        IDE_AUTO_PARTIAL_TO_SLUMBER_ENABLE,
				        0x1,
				        &Data
				        );
				  
        //Step 4 Write A8 32'hffffffff
        Data32 = 0xffffffff;
        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint32,
                        IDE_PORT_SERROR,
                        0x1,
                        &Data32
                        );
 
        //Step 5 Write IDE_PHY_PARTIAL_SLUMBER_ENABLE to enable Power Management Timer, enable Partial but disable Slumber
        Data = 0;
        Status = PciIo->Pci.Read(
						PciIo,
						EfiPciIoWidthUint8,
						IDE_PHY_PARTIAL_SLUMBER_ENABLE,
						0x1,
						&Data
						);
        Data = ((Data&(~(BIT5|BIT6)))|BIT5);
        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PHY_PARTIAL_SLUMBER_ENABLE,
                        0x1,
                        &Data
                        );
     
        //Step 6 Check A1/A2/A3/A4 8'h09
        Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
        Delay = 100;
        do{
            TimeDelay(20);
            //Step 7 Check A0/A1/A2/A3 8'h09
            Status = PciIo->Pci.Read(
                            PciIo,
                            EfiPciIoWidthUint8,
                            IDE_PORT0_SSTATUS+Port,
                            0x1,
                            &Data
                            );
            Data&=0x0F;
     
            //Step 8 Check A8 32'h00030000
            Status = PciIo->Pci.Read(
                            PciIo,
                            EfiPciIoWidthUint32,
                            IDE_PORT_SERROR,
                            0x1,
                            &Data32
                            );
   
            if(Data==0x05){
                Print(L"Partial Success, Exit Partial ");
                break;
            }else if(Data==0x09){
                Print(L"Slumber Success, Exit Slumber ");
                break;
            }
            
            Delay--;
        }while(Delay>0);
   
        if(Delay<=0)
        {
           Print(L"Partial/Slumber Failed, Exit Partial/Slumber ");
        }
   
        /////////////////////////////////////////////////////////////////////////////////  
        //Step 1 Write A4/A5/A6/A7 8'h6 Disable Slumber/Partial
        Data = 0;
        Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT0_SCONTROL+Port,
                        0x1,
                        &Data
                        ); 
        Data = Data | 0x0C;
        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT0_SCONTROL+Port,
                        0x1,
                        &Data
                        ); 
   
        //Step 2 Write IDE_PORT_SELECTOR Select Port
        Data = Port;
        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT_SELECTOR,
                        0x1,
                        &Data
                        ); 
        
        Data = 0;
        Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_AUTO_PARTIAL_TO_SLUMBER_ENABLE,
                        0x1,
                        &Data
                        );
        Data = Data & 0xFE;
        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_AUTO_PARTIAL_TO_SLUMBER_ENABLE,
                        0x1,
                        &Data
                        );   
        
        //Step 3 Write IDE_PHY_DRIVE_CONTROL WakeUp 
        Data = 0;
        Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PHY_DRIVE_CONTROL,
                        0x1,
                        &Data
                        ); 
        Data = Data | BIT0;
        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PHY_DRIVE_CONTROL,
                        0x1,
                        &Data
                        ); 
        Data = Data & 0xFE;
        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PHY_DRIVE_CONTROL,
                        0x1,
                        &Data
                        );
        
        //Step 4 Check A0/A1/A2/A3 8'h13 23h 33h
        Delay = 100;
        do{
            TimeDelay(20);
            Status = PciIo->Pci.Read(
                            PciIo,
                            EfiPciIoWidthUint8,
                            IDE_PORT0_SSTATUS+Port,
                            0x1,
                            &Data
                            );
   
            if(Data==0x13||Data==0x23||Data==0x33){
	            Print(L"Success!\n");
                Data = 0;
				Status = PciIo->Pci.Read(
								PciIo,
								EfiPciIoWidthUint8,
								IDE_PHY_PARTIAL_SLUMBER_ENABLE,
								0x1,
								&Data
								); 
				Data |= (BIT5|BIT6);
                Status = PciIo->Pci.Write(
                                PciIo,
                                EfiPciIoWidthUint8,
                                IDE_PHY_PARTIAL_SLUMBER_ENABLE,
                                0x1,
                                &Data
                                ); 
                break;
            }
            Delay--;
        }while(Delay>0);
        if(Delay<=0)
        {
           Print(L"Failed!\n");
           return EFI_UNSUPPORTED;
        }
    }
    
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IDE_DIPM_Random(
  IN ATA_ATAPI_PASS_THRU_INSTANCE   *Instance, 
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN UINT8                           Port
  )
{
	//EFI_IDE_REGISTERS          *IdeRegisters;
	EFI_STATUS                   Status;
	UINT8                        Data;
	UINT32                       Data32;
	INT32                        Delay;
	EFI_IDENTIFY_DATA            Buffer;


	Data = 0;
    Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT0_SSTATUS+Port,
                        0x1,
                        &Data
                        ); 
    if(Data==0x13||Data==0x23||Data==0x33)
    {
        Print(L"[Port%d DIPM Random ]: ", Port);
         
        //Step 1 Check Device supports initiating interface power management or not
        gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
        Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
        if((Buffer.AtaData.serial_ata_features_supported&BIT3)!=BIT3)
        {  
            Print (L"Device NOT support DIPM!\n");
        	return EFI_UNSUPPORTED;
        }
   
        Print (L"Enter ");
        
        //Step 2 Write IDE_PORT_SELECTOR Select Port
        Data = (UINT8)Port;
        Status = PciIo->Pci.Write(
				        PciIo,
				        EfiPciIoWidthUint8,
				        IDE_PORT_SELECTOR,
				        0x1,
				        &Data
				        );   
        
        //Step 3 Write A8 32'hffffffff
        Data32 = 0xffffffff;
        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint32,
                        IDE_PORT_SERROR,
                        0x1,
                        &Data32
                        );
 
        //Step 4 Write A4/A5/A6/A7 8'h0
        Data = 0;
        Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT0_SCONTROL+Port,
                        0x1,
                        &Data
                        ); 
        Data = Data&(~(BIT2|BIT3));
        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT0_SCONTROL+Port,
                        0x1,
                        &Data
                        ); 
   
        if((Buffer.AtaData.serial_ata_features_enabled&BIT3)!=BIT3){
            Status = IDE_EnableDIPM(Instance, Port/2, Port%2, NULL);
            Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
            if((Buffer.AtaData.serial_ata_features_enabled&BIT3)!=BIT3){
	            return EFI_UNSUPPORTED;
            }
        }
   
        //Step 5 Check A1/A2/A3/A4 8'h09
        Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
        Delay = 100;
        do{
            TimeDelay(20);
            Status = PciIo->Pci.Read(
                            PciIo,
                            EfiPciIoWidthUint8,
                            IDE_PORT0_SSTATUS+Port,
                            0x1,
                            &Data
                            );
            Data&=0x0F;   
              
            //Step 6 Check A8 32'h00030000
            Status = PciIo->Pci.Read(
                            PciIo,
                            EfiPciIoWidthUint32,
                            IDE_PORT_SERROR,
                            0x1,
                            &Data32
                            );
   
            if(Data==0x05){
                Print(L"Partial Success, Exit Partial ");
                break;
            }else if(Data==0x09){
                Print(L"Slumber Success, Exit Slumber ");
                break;
            }
            
            Delay--;
        }while(Delay>0);
   
        if(Delay<=0)
        {
            Print(L"Partial/Slumber Failed, Exit Partial/Slumber ");
        }

   
        //Step 1 Write A4/A5/A6/A7 8'h6 to Disable Slumber/Partial
        Data = 0;
        Status = PciIo->Pci.Read(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
        Data |= (BIT2|BIT3);
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
        
        //Step 2 Write IDE_PORT_SELECTOR Select Port
        Data = Port;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT_SELECTOR,
                           0x1,
                           &Data
                           ); 
        
        //Step 3 Write IDE_PHY_DRIVE_CONTROL to WakeUp 
        Data = 0x1;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PHY_DRIVE_CONTROL,
                           0x1,
                           &Data
                           );    
        
        Data = 0x0;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PHY_DRIVE_CONTROL,
                           0x1,
                           &Data
                           ); 

   
        Delay = 100;
        do
        {
            TimeDelay(20);
            //Step 4 Check A1/A2/A3/A4 8'h13 23h 33h
            Status = PciIo->Pci.Read(
                             PciIo,
                             EfiPciIoWidthUint8,
                             IDE_PORT0_SSTATUS+Port,
                             0x1,
                             &Data
                             );
            if(Data==0x13||Data==0x23||Data==0x33)
            {
                Print(L"Success!\n");
				Status = IDE_DisableDIMP(Instance, Port/2, Port%2, NULL);
				Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
				if((Buffer.AtaData.serial_ata_features_enabled&BIT3)==BIT3)
				{
				    DEBUG((EFI_D_ERROR,"[ERROR]: Disable DIPM Failed!\n"));
				    continue;
				}
                break;
            }
            Delay--;
        }while(Delay>0);
        
        if(Delay<=0)
        {
           Print(L"Failed!\n");
           return EFI_UNSUPPORTED;
        }
    }
  
    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
IDE_DIPM_Special(
  IN ATA_ATAPI_PASS_THRU_INSTANCE   *Instance, 
  IN UINT32                          PowerMode,
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN UINT8                          Port
  )
{
	EFI_STATUS                   Status;
	UINT8                        Data;
	UINT32                       Data32;
	INT32                        Delay;
	EFI_IDENTIFY_DATA            Buffer;


	Data = 0;
    Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT0_SSTATUS+Port,
                        0x1,
                        &Data
                        ); 
    if(Data==0x13||Data==0x23||Data==0x33)
    {
        Print(L"[Port%d DIPM Special]: ", Port);
         
        //Step 1 Check Device supports initiating interface power management or not
        gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
        Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
        if((Buffer.AtaData.serial_ata_features_supported&BIT3)!=BIT3)
        {  
            Print (L"Device NOT support DIPM!\n");
        	return EFI_UNSUPPORTED;
        }
   
        Print (L"Enter %a ", PowerMode?"Slumber":"Partial");
        
        //Step 2 Write IDE_PORT_SELECTOR Select Port
        Data = (UINT8)Port;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT_SELECTOR,
                           0x1,
                           &Data
                           );   
        
        //Step 3 Write A8 32'hffffffff
        Data32 = 0xffffffff;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint32,
                           IDE_PORT_SERROR,
                           0x1,
                           &Data32
                           );
   
        //Step 4 Write A4/A5/A6/A7 8'h0
        Data = 0;
        Status = PciIo->Pci.Read(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
        if(PowerMode==SATA_CFG_PARTIAL){
            Data = ((Data|(BIT2|BIT3))&(~(BIT2)));
        }else if(PowerMode==SATA_CFG_SLUMBER){
            Data = ((Data|(BIT2|BIT3))&(~(BIT3)));
        }
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
   
        if((Buffer.AtaData.serial_ata_features_enabled&BIT3)!=BIT3){
            Status = IDE_EnableDIPM (Instance, Port/2, Port%2, NULL);
            Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
            if((Buffer.AtaData.serial_ata_features_enabled&BIT3)!=BIT3){
	            return EFI_UNSUPPORTED;
            }
        }
   
        //Step 5 Check A1/A2/A3/A4 8'h09
        Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
        Delay = 100;
        do{
            TimeDelay(20);
            Status = PciIo->Pci.Read(
                             PciIo,
                             EfiPciIoWidthUint8,
                             IDE_PORT0_SSTATUS+Port,
                             0x1,
                             &Data
                             );
            Data&=0x0F;   
              
            //Step 6 Check A8 32'h00030000
            Status = PciIo->Pci.Read(
                             PciIo,
                             EfiPciIoWidthUint32,
                             IDE_PORT_SERROR,
                             0x1,
                             &Data32
                             );
   
            if(PowerMode==SATA_CFG_PARTIAL){
                if(Data==0x05){
                    Print(L"Success, ");
                    break;
                }
            }else if(PowerMode==SATA_CFG_SLUMBER){
                if(Data==0x09){
                    Print(L"Success, ");
                    break;
                }
            }
            
            Delay--;
        }while(Delay>0);
   
        if(Delay<=0)
        {
           Print(L"Failed, ");
        }
   
   
        Print(L"Exit %a ",PowerMode?"Slumber":"Partial");
        //Step 1 Write A4/A5/A6/A7 8'h6 to Disable Slumber/Partial
        Data = 0;
        Status = PciIo->Pci.Read(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
        Data |= (BIT2|BIT3);
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
        
        //Step 2 Write IDE_PORT_SELECTOR Select Port
        Data = Port;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT_SELECTOR,
                           0x1,
                           &Data
                           ); 
        
        //Step 3 Write IDE_PHY_DRIVE_CONTROL to WakeUp 
        Data = 0x1;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PHY_DRIVE_CONTROL,
                           0x1,
                           &Data
                           );    
        
        Data = 0x0;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PHY_DRIVE_CONTROL,
                           0x1,
                           &Data
                           ); 
   
        Delay = 100;
        do
        {
            TimeDelay(20);
            //Step 4 Check A1/A2/A3/A4 8'h13 23h 33h
            Status = PciIo->Pci.Read(
                             PciIo,
                             EfiPciIoWidthUint8,
                             IDE_PORT0_SSTATUS+Port,
                             0x1,
                             &Data
                             );
                             
            if(Data==0x13||Data==0x23||Data==0x33)
            {
                Print(L"Success!\n");
				Status = IDE_DisableDIMP(Instance, Port/2, Port%2, NULL);
				Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
				if((Buffer.AtaData.serial_ata_features_enabled&BIT3)==BIT3)
				{
					DEBUG((EFI_D_ERROR,"Disable DIPM Failed!\n"));
					continue;
				}
                break;
            }
            Delay--;
        }while(Delay>0);
        
        if(Delay<=0)
        {
           Print(L"Failed!\n");
           return EFI_UNSUPPORTED;
        }
    }
  
    return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
IDE_DIPM_AUTOPTS(
  IN ATA_ATAPI_PASS_THRU_INSTANCE   *Instance,
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN UINT8                           Port
  )
{
	EFI_STATUS                   Status;
	UINT8                        Data;
	UINT32                       Data32;
	INT32                        Delay;
	EFI_IDENTIFY_DATA            Buffer;


	Data = 0;
    Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT0_SSTATUS+Port,
                        0x1,
                        &Data
                        ); 
    if(Data==0x13||Data==0x23||Data==0x33)
    {    
        Print(L"[Port%d DIMP AutoPTS]: ", Port);
        
        //Step 1 Check Device supports initiating interface power management or not
        Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
        if((Buffer.AtaData.serial_ata_features_supported&BIT3)!=BIT3)
        {  
            Print (L"Device NOT support DIPM!\n");
        	return EFI_UNSUPPORTED;
        }
	    if((Buffer.AtaData.serial_ata_capabilities&BIT14)!=BIT14){  
            Print (L"Device NOT support Device Auto Partial to Slumber!\n");
        	return EFI_UNSUPPORTED;
        }
        Print (L"Enter ");
        
        //Step 2 Write IDE_PORT_SELECTOR Select Port
        Data = Port;
        Status = PciIo->Pci.Write(
                          PciIo,
                          EfiPciIoWidthUint8,
                          IDE_PORT_SELECTOR,
                          0x1,
                          &Data
                          );
   
        //Step 3 Write A8 32'hffffffff
        Data32 = 0xffffffff;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint32,
                           IDE_PORT_SERROR,
                           0x1,
                           &Data32
                           );
   
        //Step 4 Write A4/A5/A6/A7 8'h0
        Status = PciIo->Pci.Read(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
        Data = Data&(~(BIT2|BIT3));
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
   
        if((Buffer.AtaData.serial_ata_features_enabled&BIT3)!=BIT3)
        {  
            Status = IDE_EnableDIPM (Instance, Port/2, Port%2, NULL);
            Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
	        if((Buffer.AtaData.serial_ata_features_enabled&BIT3)==BIT3){
	            if((Buffer.AtaData.serial_ata_features_enabled&BIT7)!=BIT7){
		            Status = IDE_EnableAutoPTS (Instance, Port/2, Port%2, NULL);
		            Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
		            if((Buffer.AtaData.serial_ata_features_enabled&BIT7)!=BIT7){
		            	Print (L"[ERROR]: Enable Device AutoPTS Failed!\n");
		                return EFI_UNSUPPORTED;
		            }
                }
	        }else{
				Print (L"[ERROR]: Enable DIPM Failed!\n");
        	    return EFI_UNSUPPORTED;
            }
        }
   
        //Step 5 Check A1/A2/A3/A4 8'h09
        Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
        Delay = 100;
        do{
            TimeDelay(20);
            Status = PciIo->Pci.Read(
                             PciIo,
                             EfiPciIoWidthUint8,
                             IDE_PORT0_SSTATUS+Port,
                             0x1,
                             &Data
                             );
            Data&=0x0F;   
              
            //Step 6 Check A8 32'h00030000
            Status = PciIo->Pci.Read(
                             PciIo,
                             EfiPciIoWidthUint32,
                             IDE_PORT_SERROR,
                             0x1,
                             &Data32
                             );
   
            if(Data==0x05){
                Print(L"Partial Success, Exit Partial ");
                break;
            }else if(Data==0x09){
                Print(L"Slumber Success, Exit Slumber ");
                break;
            }
            
            Delay--;
        }while(Delay>0);
   
        if(Delay<=0)
        {
           Print(L"Partial/Slumber Failed, Exit Partial/Slumber ");
        }

        //Step 1 Write A4/A5/A6/A7 8'h6 Disable Slumber/Partial
        Data = 0;
        Status = PciIo->Pci.Read(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
        Data|= (BIT2|BIT3);
   
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT0_SCONTROL+Port,
                           0x1,
                           &Data
                           ); 
        
        //Step 2 Write IDE_PORT_SELECTOR Select Port
        Data = Port;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PORT_SELECTOR,
                           0x1,
                           &Data
                           ); 

        //Step 3 Write IDE_PHY_DRIVE_CONTROL to WakeUp 
        Data = 0x1;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PHY_DRIVE_CONTROL,
                           0x1,
                           &Data
                           ); 
        
        Data = 0x0;
        Status = PciIo->Pci.Write(
                           PciIo,
                           EfiPciIoWidthUint8,
                           IDE_PHY_DRIVE_CONTROL,
                           0x1,
                           &Data
                           );   
        
        Delay = 100;
        do
        {
            TimeDelay(20);
            //Step 4 Check A1/A2/A3/A4 8'h13 23h 33h
            Status = PciIo->Pci.Read(
                             PciIo,
                             EfiPciIoWidthUint8,
                             IDE_PORT0_SSTATUS+Port,
                             0x1,
                             &Data
                             );
            
            if(Data==0x13||Data==0x23||Data==0x33)
            {
                Print(L"Success!\n");
                Status = IDE_DisableAutoPTS(Instance, Port/2, Port%2, NULL);
                Status = IDE_DisableDIMP(Instance, Port/2, Port%2, NULL);
                Status = AtaIdentify (Instance, Port/2, Port%2, &Buffer, NULL);
                if((Buffer.AtaData.serial_ata_features_enabled&BIT7)!=BIT7){  
					if((Buffer.AtaData.serial_ata_features_enabled&BIT3)==BIT3){
						DEBUG((EFI_D_ERROR, "Disable DIPM Failed!\n"));
						continue;
					}
                }else{
					DEBUG((EFI_D_ERROR, "Disable Device AutoPTS Failed!\n"));
					continue;
                }
                break;
            }
            Delay--;
        }while(Delay>0);
        
        if(Delay<=0)
        {
           Print(L"Failed!\n");
           return EFI_UNSUPPORTED;
        }
    }
    
    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
AHCI_EnableDIMP (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                          Port,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{ 
    EFI_STATUS                     Status;
    EFI_ATA_COMMAND_BLOCK          AtaCommandBlock;
    
    ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
    
    AtaCommandBlock.AtaCommand     = ATA_CMD_SET_FEATURES;
    AtaCommandBlock.AtaFeatures    = 0x10;
    AtaCommandBlock.AtaSectorCount = 0x03;
    
    Status = AhciNonDataTransfer (
                     Instance->PciIo,
                     &Instance->AhciRegisters,
                     Port,
                     0,
                     NULL,
                     0,
                     &AtaCommandBlock,
                     AtaStatusBlock,
                     ATA_ATAPI_TIMEOUT,
                     NULL
                     );
    return Status;
}


EFI_STATUS
EFIAPI
AHCI_DisableDIMP (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                          Port,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{ 
    EFI_STATUS                     Status;
    EFI_ATA_COMMAND_BLOCK          AtaCommandBlock;
    
    ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
    
    AtaCommandBlock.AtaCommand     = ATA_CMD_SET_FEATURES;
    AtaCommandBlock.AtaFeatures    = 0x90;
    AtaCommandBlock.AtaSectorCount = 0x03;
    
    Status = AhciNonDataTransfer (
                     Instance->PciIo,
                     &Instance->AhciRegisters,
                     Port,
                     0,
                     NULL,
                     0,
                     &AtaCommandBlock,
                     AtaStatusBlock,
                     ATA_ATAPI_TIMEOUT,
                     NULL
                     );
    return Status;
}


EFI_STATUS
EFIAPI
AHCI_EnableAutoPTS(
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                          Port,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{ 
    EFI_STATUS                     Status;
    EFI_ATA_COMMAND_BLOCK          AtaCommandBlock;
    
    ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
    
    AtaCommandBlock.AtaCommand     = ATA_CMD_SET_FEATURES;
    AtaCommandBlock.AtaFeatures    = 0x10;
    AtaCommandBlock.AtaSectorCount = 0x07;
    
    Status = AhciNonDataTransfer (
                     Instance->PciIo,
                     &Instance->AhciRegisters,
                     Port,
                     0,
                     NULL,
                     0,
                     &AtaCommandBlock,
                     AtaStatusBlock,
                     ATA_ATAPI_TIMEOUT,
                     NULL
                     );

    
    return Status;
}


EFI_STATUS
EFIAPI
AHCI_DisableAutoPTS (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                          Port,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{ 
    EFI_STATUS                     Status;
    EFI_ATA_COMMAND_BLOCK          AtaCommandBlock;
    
    ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
    
    AtaCommandBlock.AtaCommand     = ATA_CMD_SET_FEATURES;
    AtaCommandBlock.AtaFeatures    = 0x90;
    AtaCommandBlock.AtaSectorCount = 0x07;
    
    Status = AhciNonDataTransfer (
                     Instance->PciIo,
                     &Instance->AhciRegisters,
                     Port,
                     0,
                     NULL,
                     0,
                     &AtaCommandBlock,
                     AtaStatusBlock,
                     ATA_ATAPI_TIMEOUT,
                     NULL
                     );
    return Status;
}


EFI_STATUS
EFIAPI
AHCI_HIPM_Special(
  IN ATA_ATAPI_PASS_THRU_INSTANCE      *Instance,
  IN UINT32                             PowerMode,
  IN EFI_PCI_IO_PROTOCOL*               PciIo,
  IN UINT8                              Port
  )
{
	EFI_STATUS                       Status;
	UINT32                           Offset;
	UINT8                            Data;
	UINT32                           Data32;
	EFI_IDENTIFY_DATA                Buffer;
	UINT32                           Capability;
	UINT32                           Delay;
	

    //Step1 Check CAP.SSC/PSC is 'h1
	Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
	if(PowerMode==SATA_CFG_PARTIAL)
	{
		if((Capability&BIT13)!=BIT13)
		{
	        Status = PciIo->Pci.Write(
                       PciIo,
                       EfiPciIoWidthUint8,
                       IDE_PORT_SELECTOR,
                       0x1,
                       &Port
                       );
                       
	        Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
                        0x1,
                        &Data
                        ); 
		    Data |= BIT5;
 	        Status = PciIo->Pci.Write(
			            PciIo,
			            EfiPciIoWidthUint8,
			            AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
			            0x1,
			            &Data
			            );
			Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
			if((Capability&BIT13)!=BIT13){
			    Print(L"[ERROR]: HBA NOT Support Transition to Partial State!\n"); 
			    return EFI_UNSUPPORTED;
			}
		}
    }else if(PowerMode==SATA_CFG_SLUMBER)
    {
    	if((Capability&BIT14)!=BIT14)
		{
		    Status = PciIo->Pci.Write(
                       PciIo,
                       EfiPciIoWidthUint8,
                       IDE_PORT_SELECTOR,
                       0x1,
                       &Port
                       );
                       
	        Status = PciIo->Pci.Read(
                       PciIo,
                       EfiPciIoWidthUint8,
                       AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
                       0x1,
                       &Data
                       ); 
		    Data |= BIT6;
 	        Status = PciIo->Pci.Write(
			            PciIo,
			            EfiPciIoWidthUint8,
			            AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
			            0x1,
			            &Data
			            );
			Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
			if((Capability&BIT14)!=BIT14){
			    Print(L"[ERROR]: HBA NOT Support Transition to Slumber State!\n"); 
			    return EFI_UNSUPPORTED;
			}
		}
    }

    //Step2 Check PxSSTS.DET and PxSSTS.IPM
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    Data32 = AhciReadReg(PciIo,Offset);
    if((Data32&(BIT0|BIT1))!=(BIT0|BIT1))
    {
        return EFI_UNSUPPORTED;
    }

    if((Data32&(BIT8|BIT9|BIT10|BIT11))!=BIT8)
    {
        Print(L"[ERROR]: Port[%d] not in active state\n", Port);
        return EFI_UNSUPPORTED;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    Print(L"[Port%d HIPM Special]: Enter %a ",Port,PowerMode?"Slumber":"Partial");
    //Step3 Program PxCMD.ASP 1'b1 PxCMD.ALPE 1'b1
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
    Data32 = AhciReadReg(PciIo,Offset);
	if(PowerMode==SATA_CFG_PARTIAL)
	{
	    Data32 = Data32|EFI_AHCI_PORT_CMD_ALPE&(~EFI_AHCI_PORT_CMD_ASP);
    }else if(PowerMode==SATA_CFG_SLUMBER)
    {
        Data32 = Data32|(EFI_AHCI_PORT_CMD_ALPE|EFI_AHCI_PORT_CMD_ASP);
    }
    AhciWriteReg(PciIo, Offset, Data32);

    
    //Step4 Program PxSCTL.IPM
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
	if(PowerMode==SATA_CFG_PARTIAL)
	{
        AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)&(~(BIT8|BIT9|BIT10|BIT11))|BIT9);

    }else if(PowerMode==SATA_CFG_SLUMBER)
    {
        AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)&(~(BIT8|BIT9|BIT10|BIT11))|BIT8);
    }

    //Step5 Write PxSERR 32'hffffffff
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
    Data32 = 0xffffffff;
    AhciWriteReg(PciIo,Offset,Data32);


    //Step6 Issue identify command to make sata idle
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		  Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		  return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }


    //Setp7 Check PxSSTS.IPM is 'h6 and PxSERR is 32'h30000
    Delay = 100;
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    do{ 
        TimeDelay(20);
	    Data32 = AhciReadReg(PciIo,Offset);
	    if(PowerMode==SATA_CFG_PARTIAL){
		    if((Data32&(BIT8|BIT9|BIT10|BIT11))==BIT9)
		    {
		        Print(L"Success, ");
		        break;
		    }
	    }else if(PowerMode==SATA_CFG_SLUMBER){
	        if((Data32&(BIT8|BIT9|BIT10|BIT11))==(BIT9|BIT10))
		    {
		        Print(L"Success, ");
		        break;
		    }
	    }
        Delay--;
    }while(Delay>0);

    if(Delay<=0)
    {
       Print(L"Failed, ");
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    Print(L"Exit %a ", PowerMode?"Slumber":"Partial");
    //Setp1 Clear Aggressive related registers
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
    AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)&(~EFI_AHCI_PORT_CMD_ALPE));
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
    AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)|(BIT8|BIT9));


    //Step2 Issue identify command to make sata idle
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		  Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		  return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }

    
    //Setp3 Check PxSSTS.IPM is 'h1 and PxSERR is 32'h30000
    Delay = 100;
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    do{  
	    TimeDelay(20);
	    Data32 = AhciReadReg(PciIo,Offset);
	    if((Data32&(BIT8|BIT9|BIT10|BIT11))==BIT8)
	    {
	        Print(L"Success!\n");
	        return EFI_SUCCESS;
	    }
        Delay--;
    }while(Delay>0);
    
    Print(L"Failed!\n");
    return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
AHCI_HIPM_AUTOPTS(
  IN ATA_ATAPI_PASS_THRU_INSTANCE      *Instance,
  IN EFI_PCI_IO_PROTOCOL*               PciIo,
  IN UINT8                              Port
  )
{
	EFI_STATUS                       Status;
	UINT32                           Offset;
	UINT8                            Data;
	UINT32                           Data32;
	EFI_IDENTIFY_DATA                Buffer;
	UINT32                           Capability;
	UINT32                           Capability2;
	UINT32                           Delay;
	
	
    //Step1 Check CAP.SSC/PSC is 'h1
	Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
	if((Capability&BIT13)!=BIT13)
	{
        Status = PciIo->Pci.Write(
                   PciIo,
                   EfiPciIoWidthUint8,
                   IDE_PORT_SELECTOR,
                   0x1,
                   &Port
                   );
                   
        Status = PciIo->Pci.Read(
			        PciIo,
			        EfiPciIoWidthUint8,
			        AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
			        0x1,
			        &Data
			        ); 
	    Data |= BIT5;
        Status = PciIo->Pci.Write(
		            PciIo,
		            EfiPciIoWidthUint8,
		            AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
		            0x1,
		            &Data
		            );
		Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
		if((Capability&BIT13)!=BIT13){
		    Print(L"[ERROR]: HBA NOT Support Transition to Partial State!\n"); 
		    return EFI_UNSUPPORTED;
		}
	}
	
	if((Capability&BIT14)!=BIT14)
	{
	    Status = PciIo->Pci.Write(
                   PciIo,
                   EfiPciIoWidthUint8,
                   IDE_PORT_SELECTOR,
                   0x1,
                   &Port
                   );
                   
        Status = PciIo->Pci.Read(
                   PciIo,
                   EfiPciIoWidthUint8,
                   AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
                   0x1,
                   &Data
                   ); 
	    Data |= BIT6;
        Status = PciIo->Pci.Write(
	            PciIo,
	            EfiPciIoWidthUint8,
	            AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
	            0x1,
	            &Data
	            );
		Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
		if((Capability&BIT14)!=BIT14){
		    Print(L"[ERROR]: HBA NOT Support Transition to Slumber State!\n"); 
		    return EFI_UNSUPPORTED;
		}
    }
    

    //Step2 Check PxSSTS.DET and PxSSTS.IPM
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    Data32 = AhciReadReg(PciIo,Offset);
    if((Data32&(BIT0|BIT1))!=(BIT0|BIT1))
    {
        return EFI_UNSUPPORTED;
    }

    if((Data32&(BIT8|BIT9|BIT10|BIT11))!=BIT8)
    {
        Print(L"[ERROR]: Port[%d] not in active state\n", Port);
        return EFI_UNSUPPORTED;
    }

    //Step3 Check CAP.APST and enable PxCMD.APSTE
    Capability2 = AhciReadReg (PciIo, 0x24);
	if((Capability2&BIT2)!=BIT2)
	{
		Print(L"[ERROR]: HBA NOT Support Auto Partial to Slumber!\n"); 
		return EFI_UNSUPPORTED;	
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////
    Print(L"[Port%d HIPM AutoPTS]: Enter ", Port);
    //Step3 Program PxCMD.ASP 1'b1 PxCMD.ALPE 1'b1
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
    Data32 = AhciReadReg(PciIo,Offset);
	Data32 = Data32|(EFI_AHCI_PORT_CMD_ALPE|BIT23)&(~EFI_AHCI_PORT_CMD_ASP);
    AhciWriteReg(PciIo, Offset, Data32);

    
    //Step4 Program PxSCTL.IPM
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
	AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)&(~(BIT8|BIT9|BIT10|BIT11)));


    //Step5 Write PxSERR 32'hffffffff
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
    Data32 = 0xffffffff;
    AhciWriteReg(PciIo,Offset,Data32);


    //Step6 Issue identify command to make sata idle
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		  Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		  return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }


    //Setp7 Check PxSSTS.IPM is 'h6 and PxSERR is 32'h30000
    Delay = 100;
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    do{ 
        TimeDelay(20);
	    Data32 = AhciReadReg(PciIo,Offset);
	    if((Data32&(BIT8|BIT9|BIT10|BIT11))==BIT9)
	    {
	        Print(L"Partial Success, Exit Partial ");
	        break;
	    }else if((Data32&(BIT8|BIT9|BIT10|BIT11))==(BIT9|BIT10))
	    {
	        Print(L"Slumber Success, Exit Slumber ");
	        break;
	    }
        Delay--;
    }while(Delay>0);

    if(Delay<=0)
    {
       Print(L"Partial/Slumber Failed, Exit Partial/Slumber ");
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    //Setp1 Clear Aggressive related registers
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
    AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)&(~EFI_AHCI_PORT_CMD_ALPE));
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
    AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)|(BIT8|BIT9));


    //Step2 Issue identify command to make sata idle
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		  Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		  return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }

    
    //Setp3 Check PxSSTS.IPM is 'h1 and PxSERR is 32'h30000
    Delay = 100;
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    do{  
	    TimeDelay(20);
	    Data32 = AhciReadReg(PciIo,Offset);
	    if((Data32&(BIT8|BIT9|BIT10|BIT11))==BIT8)
	    {
	        Print(L"Success!\n");
	        return EFI_SUCCESS;
	    }
        Delay--;
    }while(Delay>0);
    
    Print(L"Failed!\n");
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
AHCI_DIPM_Random(
  IN ATA_ATAPI_PASS_THRU_INSTANCE      *Instance,
  IN EFI_PCI_IO_PROTOCOL               *PciIo,
  IN UINT8                              Port
  )
{
	EFI_STATUS                       Status;
	UINT32                           Offset;
	UINT8                            Data;
	UINT32                           Data32;
	UINT32                           Capability;
	EFI_IDENTIFY_DATA                Buffer;
	UINTN                            Delay;
	BOOLEAN                          HABPartial_Flag = TRUE;
    BOOLEAN                          HABSlumber_Flag = TRUE;
	
    //Step1 Check CAP.SSC/PSC is 'h1
	Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
	if((Capability&BIT13)!=BIT13)
	{
        Status = PciIo->Pci.Write(
                    PciIo,
                    EfiPciIoWidthUint8,
                    IDE_PORT_SELECTOR,
                    0x1,
                    &Port
                    );
                   
        Status = PciIo->Pci.Read(
			        PciIo,
			        EfiPciIoWidthUint8,
			        AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
			        0x1,
			        &Data
			        ); 
	    Data |= BIT5;
        Status = PciIo->Pci.Write(
			        PciIo,
			        EfiPciIoWidthUint8,
			        AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
			        0x1,
			        &Data
			        );
		Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
		if((Capability&BIT13)!=BIT13){
			HABPartial_Flag = FALSE;
		    Print(L"[ERROR]: HBA NOT Support Transition to Partial State!\n"); 
		}
	}

	if((Capability&BIT14)!=BIT14)
	{
	    Status = PciIo->Pci.Write(
                    PciIo,
                    EfiPciIoWidthUint8,
                    IDE_PORT_SELECTOR,
                    0x1,
                    &Port
                    );
                   
        Status = PciIo->Pci.Read(
                    PciIo,
                    EfiPciIoWidthUint8,
                    AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
                    0x1,
                    &Data
                    ); 
	    Data |= BIT6;
        Status = PciIo->Pci.Write(
		            PciIo,
		            EfiPciIoWidthUint8,
		            AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
		            0x1,
		            &Data
		            );
		Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
		if((Capability&BIT14)!=BIT14){
			HABSlumber_Flag = FALSE;
		    Print(L"[ERROR]: HBA NOT Support Transition to Slumber State!\n"); 
		}
	}
    if(HABPartial_Flag==FALSE &&  HABSlumber_Flag==FALSE)
    {
	    return EFI_UNSUPPORTED;
    }


    //Step2 Check PxSSTS.DET and PxSSTS.IPM
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    Data32 = AhciReadReg(PciIo,Offset);
    if((Data32&(BIT0|BIT1))!=(BIT0|BIT1))
    {
        return EFI_UNSUPPORTED;
    }

    if((Data32&(BIT8|BIT9|BIT10|BIT11))!=BIT8)
    {
        Print(L"[ERROR]: Port[%d] not in active state\n", Port);
        return EFI_UNSUPPORTED;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    Print (L"[Port%d DIPM Random ]: ",Port);
    //Step3 Check Device supports initiating interface power management or not
    gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		  Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		  return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }
    if((Buffer.AtaData.serial_ata_features_supported&BIT3)!=BIT3)
    {  
        Print (L"Device NOT support DIPM!\n");
        return EFI_UNSUPPORTED;
    }

    //Step5 Program PxSCTL.IPM
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
    AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)&(~(BIT8|BIT9|BIT10|BIT11)));

    
    //Step6 Write PxSERR 32'hffffffff
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
    Data32 = 0xffffffff;
    AhciWriteReg(PciIo,Offset,Data32);


    if((Buffer.AtaData.serial_ata_features_enabled&BIT3)!=BIT3){
        Status = AHCI_EnableDIMP (Instance, Port, NULL);
        gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
	    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
	    Data32 = AhciReadReg (PciIo, Offset);
	    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
	    {
			Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
			if (EFI_ERROR (Status)) 
			{
			  Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
			  return EFI_UNSUPPORTED;
			}
	    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
	    {
	        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
	        if (EFI_ERROR (Status)) 
	        {
	            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
	            return EFI_UNSUPPORTED;
	        }
	    }
	    
        if((Buffer.AtaData.serial_ata_features_enabled&BIT3)!=BIT3){
	        Print (L"DIPM Enable Fail!\n");
            return EFI_UNSUPPORTED;
        }
    }

    //Step7 Issue identify command to make sata idle
    gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		    Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		    return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }


    //Setp8 Check PxSSTS.IPM is 'h6 PxSERR is 32'h30000
    Delay = 100;
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    do{  
        TimeDelay(20);
	    Data32 = AhciReadReg(PciIo,Offset);
	    if((Data32&(BIT8|BIT9|BIT10|BIT11))==BIT9)
	    {
	        Print(L"Enter Partial Success, Exit Partial ");
	        break;
	    }else if((Data32&(BIT8|BIT9|BIT10|BIT11))==(BIT9|BIT10))
	    {
	        Print(L"Enter Slumber Success, Exit Slumber ");
	        break;
	    }
        Delay--;
    }while(Delay>0);

    if(Delay<=0)
    {
	    Print(L"Partial/Slumber Failed, Exit Partial/Slumber ");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //Setp1 Clear Aggressive related registers
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
    AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)|(BIT8|BIT9));


    //Step2 Issue identify command to make sata idle
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		  Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		  return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }

    
    //Setp3 Check PxSSTS.IPM is 'h1 and PxSERR is 32'h30000
    Delay = 100;
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    do{  
	    TimeDelay(20);
	    Data32 = AhciReadReg(PciIo,Offset);
	    if((Data32&(BIT8|BIT9|BIT10|BIT11))==BIT8)
	    {
	        Print(L"Success!\n");
	        return EFI_SUCCESS;
	    }
	    Delay--;
    }while(Delay>0);

    Print(L"Failed!\n");
    return EFI_UNSUPPORTED;
}



EFI_STATUS
EFIAPI
AHCI_DIPM_Special(
  IN ATA_ATAPI_PASS_THRU_INSTANCE      *Instance,
  IN UINT32                             PowerMode,
  IN EFI_PCI_IO_PROTOCOL               *PciIo,
  IN UINT8                              Port
  )
{
	EFI_STATUS                       Status;
	UINT32                           Offset;
	UINT8                            Data;
	UINT32                           Data32;
	UINT32                           Capability;
	EFI_IDENTIFY_DATA                Buffer;
	UINTN                            Delay;

	
    //Step1 Check CAP.SSC/PSC is 'h1
	Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
	if(PowerMode==SATA_CFG_PARTIAL)
	{
		if((Capability&BIT13)!=BIT13)
		{
	        Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT_SELECTOR,
                        0x1,
                        &Port
                        );
                       
	        Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
                        0x1,
                        &Data
                        ); 
		    Data |= BIT5;
 	        Status = PciIo->Pci.Write(
			            PciIo,
			            EfiPciIoWidthUint8,
			            AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
			            0x1,
			            &Data
			            );
			Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
			if((Capability&BIT13)!=BIT13){
			    Print(L"[ERROR]: HBA NOT Support Transition to Partial State!\n"); 
			    return EFI_UNSUPPORTED;
			}
		}
    }else if(PowerMode==SATA_CFG_SLUMBER)
    {
    	if((Capability&BIT14)!=BIT14)
		{
		    Status = PciIo->Pci.Write(
                        PciIo,
                        EfiPciIoWidthUint8,
                        IDE_PORT_SELECTOR,
                        0x1,
                        &Port
                        );
                       
	        Status = PciIo->Pci.Read(
                        PciIo,
                        EfiPciIoWidthUint8,
                        AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
                        0x1,
                        &Data
                        ); 
		    Data |= BIT6;
 	        Status = PciIo->Pci.Write(
			            PciIo,
			            EfiPciIoWidthUint8,
			            AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
			            0x1,
			            &Data
			            );
			Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
			if((Capability&BIT14)!=BIT14){
			    Print(L"[ERROR]: HBA NOT Support Transition to Slumber State!\n"); 
			    return EFI_UNSUPPORTED;
			}
		}
    }


    //Step2 Check PxSSTS.DET and PxSSTS.IPM
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    Data32 = AhciReadReg(PciIo,Offset);
    if((Data32&(BIT0|BIT1))!=(BIT0|BIT1))
    {
        return EFI_UNSUPPORTED;
    }

    if((Data32&(BIT8|BIT9|BIT10|BIT11))!=BIT8)
    {
        Print(L"[ERROR]: Port[%d] not in active state\n", Port);
        return EFI_UNSUPPORTED;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //Step3 Check Device supports initiating interface power management or not
    Print (L"[Port%d DIPM Special]: ", Port);
    gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		    Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		    return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }
   
    if((Buffer.AtaData.serial_ata_features_supported&BIT3)!=BIT3)
    {  
        Print (L"Device NOT support DIPM!\n");
        return EFI_UNSUPPORTED;
    }


    //Step5 Program PxSCTL.IPM
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
	if(PowerMode==SATA_CFG_PARTIAL)
	{
        AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)&(~(BIT8|BIT9|BIT10|BIT11))|BIT9);

    }else if(PowerMode==SATA_CFG_SLUMBER)
    {
        AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)&(~(BIT8|BIT9|BIT10|BIT11))|BIT8);
    }  

    
    //Step6 Write PxSERR 32'hffffffff
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
    Data32 = 0xffffffff;
    AhciWriteReg(PciIo,Offset,Data32);

    Print (L"Enter %a ", PowerMode?"Slumber":"Partial");
    if((Buffer.AtaData.serial_ata_features_enabled&BIT3)!=BIT3){
        Status = AHCI_EnableDIMP (Instance, Port, NULL);
        gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
	    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
	    Data32 = AhciReadReg (PciIo, Offset);
	    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
	    {
			Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
			if (EFI_ERROR (Status)) 
			{
			    Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
			    return EFI_UNSUPPORTED;
			}
	    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
	    {
	        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
	        if (EFI_ERROR (Status)) 
	        {
	            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
	            return EFI_UNSUPPORTED;
	        }
	    }
        if((Buffer.AtaData.serial_ata_features_enabled&BIT3)!=BIT3){
	        Print (L"DIPM Enable Fail!\n");
            return EFI_UNSUPPORTED;
        }
    }

    //Step7 Issue identify command to make sata idle
    gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		    Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		    return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }


    //Setp8 Check PxSSTS.IPM is 'h6 PxSERR is 32'h30000
    Delay = 100;
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    do{  
        TimeDelay(20);
	    Data32 = AhciReadReg(PciIo,Offset);
	    if(PowerMode==SATA_CFG_PARTIAL){
		    if((Data32&(BIT8|BIT9|BIT10|BIT11))==BIT9)
		    {
		        Print(L"Success, ");
		        break;
		    }
	    }else if(PowerMode==SATA_CFG_SLUMBER){
	        if((Data32&(BIT8|BIT9|BIT10|BIT11))==(BIT9|BIT10))
		    {
		        Print(L"Success, ");
		        break;
		    }
	    }
        Delay--;
    }while(Delay>0);

    if(Delay<=0)
    {
       Print(L"Failed, ");
    }


    Print(L"Exit %a ", PowerMode?"Slumber":"Partial");
    //Setp1 Clear Aggressive related registers
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
    AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)|(BIT8|BIT9));


    //Step2 Issue identify command to make sata idle
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		    Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		    return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }


    //Setp3 Check PxSSTS.IPM is 'h1 and PxSERR is 32'h30000
    Delay = 100;
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    do{  
	    TimeDelay(20);
	    Data32 = AhciReadReg(PciIo,Offset);
	    if((Data32&(BIT8|BIT9|BIT10|BIT11))==BIT8)
	    {
	        Print(L"Success!\n");
	        return EFI_SUCCESS;
	    }
	    Delay--;
    }while(Delay>0);

    Print(L"Failed!\n");
    return EFI_UNSUPPORTED;
}



EFI_STATUS
EFIAPI
AHCI_DIPM_AUTOPTS(
  IN ATA_ATAPI_PASS_THRU_INSTANCE      *Instance,
  IN EFI_PCI_IO_PROTOCOL               *PciIo,
  IN UINT8                              Port
  )
{
	EFI_STATUS                       Status;
	UINT32                           Offset;
	UINT8                            Data;
	UINT32                           Data32;
	UINT32                           Capability;
	EFI_IDENTIFY_DATA                Buffer;
	UINTN                            Delay;

	
    //Step1 Check CAP.SSC/PSC is 'h1
	Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
	if((Capability&BIT13)!=BIT13)
	{
        Status = PciIo->Pci.Write(
                    PciIo,
                    EfiPciIoWidthUint8,
                    IDE_PORT_SELECTOR,
                    0x1,
                    &Port
                    );
                   
        Status = PciIo->Pci.Read(
                    PciIo,
                    EfiPciIoWidthUint8,
                    AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
                    0x1,
                    &Data
                    ); 
	    Data |= BIT5;
        Status = PciIo->Pci.Write(
	 	            PciIo,
	 	            EfiPciIoWidthUint8,
	 	            AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
	 	            0x1,
	 	            &Data
	 	            );
		Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
		if((Capability&BIT13)!=BIT13){
		    Print(L"[ERROR]: HBA NOT Support Transition to Partial State!\n"); 
		    return EFI_UNSUPPORTED;
		}
	}
	
	if((Capability&BIT14)!=BIT14)
	{
	    Status = PciIo->Pci.Write(
                    PciIo,
                    EfiPciIoWidthUint8,
                    IDE_PORT_SELECTOR,
                    0x1,
                    &Port
                    );
                   
        Status = PciIo->Pci.Read(
                    PciIo,
                    EfiPciIoWidthUint8,
                    AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
                    0x1,
                    &Data
                    ); 
	    Data |= BIT6;
        Status = PciIo->Pci.Write(
		            PciIo,
		            EfiPciIoWidthUint8,
		            AHCI_CAP_PARTIAL_SLUMBER_BACKDOOR,
		            0x1,
		            &Data
		            );
		Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
		if((Capability&BIT14)!=BIT14){
		    Print(L"[ERROR]: HBA NOT Support Transition to Slumber State!\n"); 
		    return EFI_UNSUPPORTED;
		}
    }


    //Step2 Check PxSSTS.DET and PxSSTS.IPM
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    Data32 = AhciReadReg(PciIo,Offset);
    if((Data32&(BIT0|BIT1))!=(BIT0|BIT1))
    {
        return EFI_UNSUPPORTED;
    }

    if((Data32&(BIT8|BIT9|BIT10|BIT11))!=BIT8)
    {
        Print(L"[ERROR]: Port[%d] not in active state\n", Port);
        return EFI_UNSUPPORTED;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    Print (L"[Port%d DIPM AutoPTS]: ", Port);
    //Step3 Check Device supports initiating interface power management or not
    gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		    Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		    return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }
    if((Buffer.AtaData.serial_ata_features_supported&BIT3)!=BIT3)
    {  
        Print (L"Device NOT support DIPM!\n");
        return EFI_UNSUPPORTED;
    }
    if((Buffer.AtaData.serial_ata_capabilities&BIT14)!=BIT14){  
        Print (L"Device NOT support Device Auto Partial to Slumber!\n");
        return EFI_UNSUPPORTED;
    }


    //Step5 Program PxSCTL.IPM
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
    AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)&(~(BIT8|BIT9|BIT10|BIT11)));


    
    //Step6 Write PxSERR 32'hffffffff
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
    Data32 = 0xffffffff;
    AhciWriteReg(PciIo,Offset,Data32);


    if((Buffer.AtaData.serial_ata_features_enabled&BIT3)!=BIT3){
        Status = AHCI_EnableDIMP (Instance, Port, NULL);
        gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
	    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
	    Data32 = AhciReadReg (PciIo, Offset);
	    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
	    {
			Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
			if (EFI_ERROR (Status)) 
			{
			    Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
			    return EFI_UNSUPPORTED;
			}
	    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
	    {
	        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
	        if (EFI_ERROR (Status)) 
	        {
	            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
	            return EFI_UNSUPPORTED;
	        }
	    }
        if((Buffer.AtaData.serial_ata_features_enabled&BIT3)==BIT3)
        {
            Status = AHCI_EnableAutoPTS(Instance, Port, NULL);
            gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
		    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
		    Data32 = AhciReadReg (PciIo, Offset);
		    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
		    {
				Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
				if (EFI_ERROR (Status)) 
				{
				    Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
				    return EFI_UNSUPPORTED;
				}
		    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
		    {
		        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		        if (EFI_ERROR (Status)) 
		        {
		            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
		            return EFI_UNSUPPORTED;
		        }
		    }
            if((Buffer.AtaData.serial_ata_features_enabled&BIT7)!=BIT7){
            	Print (L"Auto Partial to Slumber Enable Failed!\n");
                return EFI_UNSUPPORTED;
            }
        }else
        {
	        Print (L"DIPM Enable Fail!\n");
            return EFI_UNSUPPORTED;
        }
    }

    //Step7 Issue identify command to make sata idle
    gBS->SetMem(&Buffer,sizeof(EFI_IDENTIFY_DATA),0x0);
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		    Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		    return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }


    //Setp8 Check PxSSTS.IPM is 'h6 PxSERR is 32'h30000
    Delay = 100;
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    do{  
        TimeDelay(20);
	    Data32 = AhciReadReg(PciIo,Offset);
	    if((Data32&(BIT8|BIT9|BIT10|BIT11))==BIT9)
	    {
	        Print(L"Enter Partial Success, Exit Partial ");
	        break;
	    }else if((Data32&(BIT8|BIT9|BIT10|BIT11))==(BIT9|BIT10))
	    {
	        Print(L"Enter Slumber Success, Exit Slumber ");
	        break;
	    }
        Delay--;
    }while(Delay>0);

    if(Delay<=0)
    {
       Print(L"Partial/Slumber Failed, Exit Partial/Slumber ");
    }

    //Setp1 Clear Aggressive related registers
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
    AhciWriteReg(PciIo,Offset,AhciReadReg(PciIo,Offset)|(BIT8|BIT9));


    //Step2 Issue identify command to make sata idle
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
    Data32 = AhciReadReg (PciIo, Offset);
    if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) 
    {
		Status = AhciIdentifyPacket (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
		if (EFI_ERROR (Status)) 
		{
		    Print(L"AhciIdentifyPacket Port%d ERROR_Status=%d!\n",Port,Status);
		    return EFI_UNSUPPORTED;
		}
    } else if ((Data32 & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) 
    {
        Status = AhciIdentify (PciIo, &Instance->AhciRegisters, Port, 0, &Buffer);
        if (EFI_ERROR (Status)) 
        {
            Print(L"AhciIdentify Port%d ERROR_Status=%d!\n",Port,Status);
            return EFI_UNSUPPORTED;
        }
    }

    //Setp3 Check PxSSTS.IPM is 'h1
    Delay = 100;
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
    do{  
	    TimeDelay(20);
	    Data32 = AhciReadReg(PciIo,Offset);
	    if((Data32&(BIT8|BIT9|BIT10|BIT11))==BIT8)
	    {
	        Print(L"Success!\n");
	        return EFI_SUCCESS;
	    }
	    Delay--;
    }while(Delay>0);

    Print(L"Failed!\n");
    return EFI_UNSUPPORTED;
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
    EFI_STATUS                     Status;
    EFI_HANDLE                    *HandleAtaPassBuffer;
    UINTN                          NumberOfAtaPassHandles;
    EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru;
    ATA_ATAPI_PASS_THRU_INSTANCE  *Instance  = NULL;
    EFI_PCI_IO_PROTOCOL           *PciIo     = NULL;
    EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
    UINT8                          i = 0;
    UINT8                          j = 0;
    UINT8                          ZXBus = 0;
    UINT8                          ZXDev = 0;
    UINT8                          ZXFunc= 0;
    UINTN                          ZX_SATA;
    UINTN                          Segment;
    UINTN                          Bus; 
    UINTN                          Dev; 
    UINTN                          Func;
    UINT8                          Satacfg;
    BOOLEAN                        Found_Flag = FALSE;
    
  
    ////////////Find AtaPassThruProtocol////////////////////////////
    Status = gBS->LocateHandleBuffer(
                     ByProtocol,
                     &gEfiAtaPassThruProtocolGuid,
                     NULL,
                     &NumberOfAtaPassHandles,
                     &HandleAtaPassBuffer
                     );
    if (EFI_ERROR(Status)){
		Print(L"[ERROR]: Locate EfiAtaPassThruProtocol Failed!\n");
		return EFI_UNSUPPORTED;
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
	    Print(L"SataPMTest.efi\n");
	    Print(L"SataPMTest.efi Bus Dev Func\n");
	    return EFI_UNSUPPORTED;
    }

   
    for(i=0; i<NumberOfAtaPassHandles; i++)
    {
	    Status=gBS->HandleProtocol(
	                      HandleAtaPassBuffer[i],
	                      &gEfiDevicePathProtocolGuid,
	                      (VOID**) &DevicePath
	                      );
        ASSERT_EFI_ERROR (Status);
	        
        Status = gBS->HandleProtocol(
                      HandleAtaPassBuffer[i],
                      &gEfiPciIoProtocolGuid,
                      (VOID**) &PciIo
                      );
        ASSERT_EFI_ERROR (Status);
          
        Status = PciIo->GetLocation(
                      PciIo,
                      &Segment,
                      &Bus,
                      &Dev,
                      &Func
                      );
        ASSERT_EFI_ERROR (Status);

        if(Argc==4)
        {
            if((Bus!=ZXBus)||(Dev!=ZXDev)||(Func!=ZXFunc))
            {
                continue;
            }else
            {
                Found_Flag = TRUE;
            }
        }

        Print(L"[CONTROLLER]: Bus%d D%dF%d, ", Bus, Dev, Func);
        
        Status = gBS->HandleProtocol(
                            HandleAtaPassBuffer[i],
                            &gEfiAtaPassThruProtocolGuid,
                            (VOID**) &AtaPassThru
                            );
        ASSERT_EFI_ERROR (Status);
         
        Instance = ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS (AtaPassThru);
  
        Status = PciIo->Pci.Read(
                      PciIo,
                      EfiPciIoWidthUint8,
                      0x0A,
                      0x1,
                      &Satacfg
                      );
        ASSERT_EFI_ERROR (Status);
        
        if(Satacfg == SATA_CFG_IDE)
        {
            Print(L"IN IDE Mode\n");
            PrintDevicePath(DevicePath);
            for(j=0; j<SATA_CFG_IDE_MAXPORT; j++)
            {   
                Status = IDE_HIPM_Special(Instance, SATA_CFG_PARTIAL, PciIo, j);
                TimeDelay(20);
                Status = IDE_HIPM_Special(Instance, SATA_CFG_SLUMBER, PciIo, j);
            	TimeDelay(20);
            	Status = IDE_HIPM_AUTOPTS(Instance, PciIo, j);
                TimeDelay(20);

                Status = IDE_DIPM_Random(Instance, PciIo, j);
                TimeDelay(20);
                Status = IDE_DIPM_Special(Instance, SATA_CFG_PARTIAL, PciIo, j);
                TimeDelay(20);
                Status = IDE_DIPM_Special(Instance, SATA_CFG_SLUMBER, PciIo, j);
                TimeDelay(20);
                Status = IDE_DIPM_AUTOPTS(Instance, PciIo, j);
                TimeDelay(20);
            }     
        }else if(Satacfg == SATA_CFG_AHCI)
        {
            Print(L"IN AHCI Mode\n");
            PrintDevicePath(DevicePath);
            for(j=0; j<SATA_CFG_AHCI_MAXPORT; j++)
            { 
                Status = AHCI_HIPM_Special(Instance,SATA_CFG_PARTIAL,PciIo, j);
                TimeDelay(20);
                Status = AHCI_HIPM_Special(Instance,SATA_CFG_SLUMBER,PciIo, j);
                TimeDelay(20);
   				Status = AHCI_HIPM_AUTOPTS(Instance, PciIo, j);
   				TimeDelay(20);
     
                Status = AHCI_DIPM_Random(Instance, PciIo, j);
                TimeDelay(20);
                Status = AHCI_DIPM_Special(Instance,SATA_CFG_PARTIAL,PciIo, j);
                TimeDelay(20);
                Status = AHCI_DIPM_Special(Instance,SATA_CFG_SLUMBER,PciIo, j);
                TimeDelay(20);
                Status = AHCI_DIPM_AUTOPTS(Instance, PciIo, j);
                TimeDelay(20);
            }
        }
        Print(L"\n");

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
