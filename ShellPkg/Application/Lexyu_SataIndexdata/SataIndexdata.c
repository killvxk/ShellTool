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
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PciLib.h>
#include <Library/IoLib.h>
#include <Library/BaseLib.h>


#include <Protocol/AtaPassThru.h>
#include <Protocol/PciIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DevicePathFromText.h>

#define PCI_LIB_ADDRESS(Bus,Device,Function,Offset)   \
  (((Offset) & 0xfff) | (((Function) & 0x07) << 12) | (((Device) & 0x1f) << 15) | (((Bus) & 0xff) << 20))


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
	    Print(L"DevicePath: (%d,%d)",DevPathNode->Type,DevPathNode->SubType);
	    DevPathNode=NextDevicePathNode(DevPathNode);
    }
    while(!IsDevicePathEnd(DevPathNode))
    {
	    Print(L"/(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
	    DevPathNode=NextDevicePathNode(DevPathNode);
    }
    Print(L"\n");
    


    if(!IsDevicePathEnd(DevPathNode))
    {
	    Textdevicepath=Device2TextProtocol->ConvertDevicePathToText(DevPathNode,TRUE,TRUE);
	    Print(L"%s",Textdevicepath);
	    Print(L"\n\n");
	    if(Textdevicepath) gBS->FreePool(Textdevicepath);
    }

    return EFI_SUCCESS;
    
}



INTN
EFIAPI
CheckIndexDataPair(
  IN UINT32				INDEX,
  IN UINT32             DATA,
  IN UINT32             AHCIBase,
  IN UINT32             OFFSET
  )
{
	EFI_STATUS          Status;
	UINT32              Tmp01;
    UINT32              Tmp02;
    UINT32              Tmp03;
    UINT32              Tmp04;
    UINT32              Tmp05;
    UINT32              Tmp06;

    Tmp01=MmioRead32(AHCIBase+OFFSET);
    IoWrite32(INDEX,OFFSET);
    Tmp02=IoRead32(DATA);

    IoWrite32(DATA,0xffffffff);
    Tmp03=MmioRead32(AHCIBase+OFFSET);
    Tmp04=IoRead32(DATA);

    IoWrite32(DATA,0x0);
    Tmp05=MmioRead32(AHCIBase+OFFSET);
    Tmp06=IoRead32(DATA);

    if(Tmp01==Tmp02 && Tmp03==Tmp04 && Tmp05==Tmp06)
    {
    	Status=EFI_SUCCESS;
    }else
    {
        Status=EFI_LOAD_ERROR;
    }

    Print(L"[%08x]: 0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/0x%08x, %a\n",AHCIBase+OFFSET,Tmp01,Tmp02,Tmp03,Tmp04,Tmp05,Tmp06,Status?"NO":"YES");
    return EFI_SUCCESS;
}



INTN
EFIAPI
CheckOneSatacontroller(
  IN UINTN						BusNum,
  IN UINTN						DevNum,
  IN UINTN						FuncNum
  )
{
  	//EFI_STATUS          	   Status;
    UINT8                      Capability = 0;
    UINT32                     SATACap    = 0;
	UINT32                     Value32;
    UINT8                      Value8;
	BOOLEAN                    FoundIDP = FALSE;

    UINT8                      BARX   = 0;
    UINT32                     OFFSET = 0; 
    UINT32                     INDEX  = 0;
    UINT32                     DATA   = 0;
    UINT32                     AHCIBase;
    UINT8                      i;
    UINT8                      j;

    UINT32                     PortImplementBitMap;
    UINT8                      MaxPortNumber;


    Print(L"Location: (Bus,Dev,Func)=(0x%02x,0x%02x,0x%02x)\n",BusNum,DevNum,FuncNum);
    Value8 = PciRead8(PCI_LIB_ADDRESS(BusNum,DevNum,FuncNum,0x34)); 
    
    Capability = Value8;
    while(Capability!=0x00)
    {
        Value32 = PciRead32(PCI_LIB_ADDRESS(BusNum,DevNum,FuncNum,Capability));
	    if((Value32&0xff)==0x12)
        {
	    	FoundIDP = TRUE;
            SATACap = PciRead32(PCI_LIB_ADDRESS(BusNum,DevNum,FuncNum,Capability+0x4));
	      	goto Here;
	    }
	    Capability = (UINT8)(Value32>>8);
    }
    
Here:
    if(FoundIDP==FALSE)
    {
  		Print(L"Sorry, Index-Data Pair Capability Not Found!\n");
		goto Exit;
  	}

    switch(SATACap&0xf)
    {
    	case 0x4:
            BARX=0x10;
            break;
            
        case 0x5:
            BARX=0x14;
            break;
            
        case 0x6:
            BARX=0x18;
            break;
            
        case 0x7:
            BARX=0x1C;
            break;
            
        case 0x8:
            BARX=0x20;
            break;
            
        case 0x9:
            BARX=0x24;
            break;
            
        case 0xf:
            BARX=0x0;
            break;

        default:
            BARX=0x0;
            break;
    }

    if(((SATACap&0xffffff)>>4)<= 0x3fff)
    {
    	OFFSET=((SATACap&0xffffff)>>4)*4;
    }

    if(BARX!=0 && OFFSET!=0)
    {
    	INDEX = (PciRead32(PCI_LIB_ADDRESS(BusNum,DevNum,FuncNum,BARX))&0x0000fff0) + OFFSET;
    	DATA  = INDEX+0x4;

	    AHCIBase = PciRead32(PCI_LIB_ADDRESS(BusNum,DevNum,FuncNum,0x24));

        //
        // Get the highest bit of implemented ports which decides how many bytes are allocated for recived FIS.
        //
        PortImplementBitMap  = MmioRead32(AHCIBase+0x000C);
        MaxPortNumber        = (UINT8)(UINTN)(HighBitSet32(PortImplementBitMap) + 1);
        if (MaxPortNumber == 0) {
          return EFI_DEVICE_ERROR;
        }

        Print(L"[Checking GHC]\n");
	    for(i=0;i<11;i++)
	    {
	    	CheckIndexDataPair(INDEX,DATA,AHCIBase,i*0x4);
	    }

        if(MaxPortNumber>1)
        {
        	Print(L"[Checking Port 0-%d]\n",MaxPortNumber-1);
        }else if(MaxPortNumber==1)
        {
        	Print(L"[Checking Port 0]\n");
        }
        for(i=0;i<MaxPortNumber;i++)
        {
		    for(j=0;j<19;j++)
		    {
		    	CheckIndexDataPair(INDEX,DATA,AHCIBase,0x100+0x80*i+j*0x4);
		    }
        }
       
	}

    
Exit:
	return EFI_SUCCESS;
}




INTN
EFIAPI
CheckAllSatacontroller()
{
  	EFI_STATUS       				Status;
    UINT8                           i;
	EFI_HANDLE                     *BufferofAtaPassHandles;
  	UINTN                          	NumberOfAtaPassHandles;
  	EFI_PCI_IO_PROTOCOL            *PciIo     = NULL;
    UINT8                           ClassCode = 0;
    EFI_DEVICE_PATH_PROTOCOL       *DevicePath;

    UINTN                           SegNum;
    UINTN                           BusNum;
    UINTN                           DevNum;
    UINTN                           FuncNum;


    ////////////Find AtaPassThruProtocol////////////////////////////
    Status = gBS->LocateHandleBuffer (
                     ByProtocol,
                     &gEfiAtaPassThruProtocolGuid,
                     NULL,
                     &NumberOfAtaPassHandles,
                     &BufferofAtaPassHandles
                     );
    if (EFI_ERROR (Status))
    {
	    Print(L"Sorry, NO Active SATA Controller Founded!\n");
	    return Status;
    }
    Print(L"[Number of SATA Controller Actived = %d]\n\n",NumberOfAtaPassHandles);
  
    
    for(i=0;i<NumberOfAtaPassHandles;i++)
    {
        Print(L"Start Test SATA Controller %02d:\n",i+1);

        Status = gBS->HandleProtocol (
                      BufferofAtaPassHandles[i],
                      &gEfiPciIoProtocolGuid,
                      (VOID**) &PciIo
                      );
  
        
        Status = PciIo->Pci.Read (
                      PciIo,
                      EfiPciIoWidthUint8,
                      0x0A,
                      0x1,
                      &ClassCode
                      );
  
        
        if(ClassCode==0x01)
        {
	        Print(L"SATA Controller in IDE MODE!\n");
            continue;
	      
        }
        else if(ClassCode==0x06)
        {       
            Status=gBS->HandleProtocol(
                          BufferofAtaPassHandles[i],
                          &gEfiDevicePathProtocolGuid,
                          (VOID**) &DevicePath
                          );
            PrintDevicePath(DevicePath);
            
            Status = PciIo->GetLocation (
                      	PciIo,
                      	&SegNum,
                      	&BusNum,
                      	&DevNum,
                      	&FuncNum
                      	);
            Status=CheckOneSatacontroller(BusNum,DevNum,FuncNum);
        }
        Print(L"\n");
  
    }


  	return Status;
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
  IN UINTN				Argc,
  IN CHAR16			  **Argv
  )
{
	EFI_STATUS          Status;

    if(Argc==1)
    {
    	Status=CheckAllSatacontroller();
    }else if(Argc==4)
    {
    	Status=CheckOneSatacontroller(StrDecimalToUintn(Argv[1]),StrDecimalToUintn(Argv[2]),StrDecimalToUintn(Argv[3]));
    }else
    {
      	Print(L"WRONG PARAMETERS!\n");
      	Print(L"INPUT: SATAIndexdata BusNum DevNum FuncNum\n");
      	Status=EFI_UNSUPPORTED;
    }
  
    return Status;
}
