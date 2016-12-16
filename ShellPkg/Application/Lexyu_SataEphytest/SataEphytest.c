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

#define  MAX_PCI_BUSS      40
#define  ZX_SATA_DEV       15


UINTN    ZX_SATA;
UINTN    MaxAddress = (0x12B/0x10 + 1);
UINTN    EphyIndex  = 0x68;
UINT8    EphyData0  = 0x64;
UINT8    EphyData1  = 0x65;
UINT8    EphyData2  = 0x66;
UINT8    EphyData3  = 0x67;
BOOLEAN  AttrCheck  = FALSE;



VOID
FunctionPcidump()
{
    UINT8        Index1;
    UINT8        Index2;
    UINT8        Data;

	DEBUG((EFI_D_ERROR, "[SATA PCI Space]:\n"));
    Print(L"[SATA PCI Space]:\n");
	DEBUG((EFI_D_ERROR, "     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"));
    Print(L"     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	DEBUG((EFI_D_ERROR, "---+--------------------------------------------------\n"));
    Print(L"---+------------------------------------------------\n");
	for(Index1=0;Index1<0x10;Index1++)
	{
		DEBUG((EFI_D_ERROR, "%02x | ",Index1*16));
        Print(L"%02x | ",Index1*16);
		for(Index2=0;Index2<0x10;Index2++)
		{
			Data = PciRead8(ZX_SATA|(Index1*16+Index2));
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
			DEBUG((EFI_D_ERROR, "------------------------------------------------------\n"));
            Print(L"----------------------------------------------------\n");
        }
	}
	DEBUG((EFI_D_ERROR, "\n")); 
    Print(L"\n"); 
}



UINT8
EFIAPI
SataScanPciBus()
{
    UINT8         PciBusNum;
    UINT16        VendorID;
    UINT16        DeviceID;

    for (PciBusNum = 1; PciBusNum < MAX_PCI_BUSS; PciBusNum++) 
    {
    	VendorID = PciRead16(PCI_LIB_ADDRESS(PciBusNum, ZX_SATA_DEV, 0, 0)|0x0);
        DeviceID = PciRead16(PCI_LIB_ADDRESS(PciBusNum, ZX_SATA_DEV, 0, 0)|0x2);
        if((VendorID==0x1106 || VendorID==0x1d17) && (DeviceID==0x9002 || DeviceID==0x9083))
        {
            return PciBusNum;
        }
    }
    return 0;
}



VOID
CheckBitAttr(
  IN UINT8    Data0,
  IN UINT8    Data1,
  IN UINT8    Data2
  )
{
    INT8       i;

    for(i=7; i>=0; i--)
    {
        if((((Data1>>i)&BIT0)==BIT0)&&(((Data2>>i)&BIT0)==0x0))
        {
	        Print(L"w");
	    }else if((((Data1>>i)&BIT0)==((Data0>>i)&BIT0))&&(((Data2>>i)&BIT0)==((Data0>>i)&BIT0))){
		    Print(L"r");
	    }else{
		    Print(L"?");
	    }
    }
    Print(L" ");
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
SataEphySettingDump()
{
    UINT16        Index1;
    UINT16        Index2;
    UINTN         Data;

	DEBUG((EFI_D_ERROR, "[SATA EPHY Setting]:\n"));
    Print(L"[SATA EPHY Setting]:\n");
	DEBUG((EFI_D_ERROR, "       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"));
    Print(L"       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	DEBUG((EFI_D_ERROR, "-----+--------------------------------------------------\n"));
    Print(L"-----+------------------------------------------------\n");
	for(Index1=0;Index1<=MaxAddress;Index1++)
	{
		DEBUG((EFI_D_ERROR, "%04x | ",Index1*16));
        Print(L"%04x | ",Index1*16);
		for(Index2=0;Index2<0x10;Index2+=4)
		{
			PciWrite16((ZX_SATA|EphyIndex),Index1*16+Index2);
			Data = PciRead8(ZX_SATA|EphyData0);
			DEBUG((EFI_D_ERROR, "%02x ",Data));
			Print(L"%02x ",Data);
			Data = PciRead8(ZX_SATA|EphyData1);
			DEBUG((EFI_D_ERROR, "%02x ",Data));
			Print(L"%02x ",Data);
			Data = PciRead8(ZX_SATA|EphyData2);
			DEBUG((EFI_D_ERROR, "%02x ",Data));
			Print(L"%02x ",Data);
			Data = PciRead8(ZX_SATA|EphyData3);
			DEBUG((EFI_D_ERROR, "%02x ",Data));
            Print(L"%02x ",Data);        
		}
		DEBUG((EFI_D_ERROR,"\n"));
        Print(L"\n");
		if((Index1%0x10)==0x0F)
        {
			DEBUG((EFI_D_ERROR, "--------------------------------------------------------\n"));
            Print(L"------------------------------------------------------\n");
        }
	}
	DEBUG((EFI_D_ERROR, "\n")); 
    Print(L"\n");

    return EFI_SUCCESS;
}


INTN
EFIAPI
SataEphySettingCheck(
  IN UINT16				Index1,
  IN UINT16             Index2
  )
{
	UINT8               Data0;
	UINT8               Data1;
	UINT8               Data2;

    PciWrite16((ZX_SATA|EphyIndex),Index1*16+Index2);
    
    Data0 = PciRead8(ZX_SATA|EphyData0);
    PciWrite16((ZX_SATA|EphyData0),0xFF);
    Data1 = PciRead8(ZX_SATA|EphyData0);
    PciWrite16((ZX_SATA|EphyData0),0x0);
    Data2 = PciRead8(ZX_SATA|EphyData0);
    CheckBitAttr(Data0, Data1, Data2);

    Data0 = PciRead8(ZX_SATA|EphyData1);
    PciWrite16((ZX_SATA|EphyData1),0xFF);
    Data1 = PciRead8(ZX_SATA|EphyData1);
    PciWrite16((ZX_SATA|EphyData1),0x0);
    Data2 = PciRead8(ZX_SATA|EphyData1);
    CheckBitAttr(Data0, Data1, Data2);

    Data0 = PciRead8(ZX_SATA|EphyData2);
    PciWrite16((ZX_SATA|EphyData2),0xFF);
    Data1 = PciRead8(ZX_SATA|EphyData2);
    PciWrite16((ZX_SATA|EphyData2),0x0);
    Data2 = PciRead8(ZX_SATA|EphyData2);
    CheckBitAttr(Data0, Data1, Data2);

    Data0 = PciRead8(ZX_SATA|EphyData3);
    PciWrite16((ZX_SATA|EphyData3),0xFF);
    Data1 = PciRead8(ZX_SATA|EphyData3);
    PciWrite16((ZX_SATA|EphyData3),0x0);
    Data2 = PciRead8(ZX_SATA|EphyData3);
    CheckBitAttr(Data0, Data1, Data2);

    return EFI_SUCCESS;
}



INTN
EFIAPI
SataEphySettingCheckAll()
{
    UINT16        Index1;
    UINT16        Index2;

	DEBUG((EFI_D_ERROR,"[SATA EPHY Attribute]:\n"));
    Print(L"[SATA EPHY Attribute]:\n");
	DEBUG((EFI_D_ERROR, "-----+------------------------------------------------------------------------------\n"));
    Print(L"-----+------------------------------------------------------------------------------\n");
	for(Index1=0;Index1<=MaxAddress;Index1++)
	{
		DEBUG((EFI_D_ERROR, "%04x | ",Index1*16));
        Print(L"%04x | ",Index1*16);
		for(Index2=0;Index2<0x8;Index2+=4)
		{
			SataEphySettingCheck(Index1, Index2);
            DEBUG((EFI_D_ERROR, " "));
            Print(L" ");
		}
		DEBUG((EFI_D_ERROR, "\n"));
        Print(L"\n");

		DEBUG((EFI_D_ERROR, "%04x | ",Index1*16));
        Print(L"%04x | ",Index1*16);
		for(Index2=0x8;Index2<0x10;Index2+=4)
		{
			SataEphySettingCheck(Index1, Index2);
            DEBUG((EFI_D_ERROR, " "));
            Print(L" ");
		}
		DEBUG((EFI_D_ERROR, "\n"));
        Print(L"\n");        
	}

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
  IN UINTN				Argc,
  IN CHAR16			  **Argv
  )
{
	UINT8               SataBusNum = 0;
    UINT8               ZXBus;
    UINT8               ZXDev;
    UINT8               ZXFunc;
    CHAR8               Parameter;

    if(Argc==1)
    {
        SataBusNum = SataScanPciBus();
        if (SataBusNum == 0) {
	        Print(L"[Lex]: NO ZX SATA CONTROLLER FOUND AT BUSX D15F0!\n");
	        return EFI_SUCCESS;
	    }
	  
	    Print(L"[Lex]: ZX SATA CONTROLLER FOUND AT BUS%d D15F0!\n",SataBusNum);
    	ZX_SATA=PCI_LIB_ADDRESS(SataBusNum,ZX_SATA_DEV,0,0);
    }else if(Argc==2)
    {
        UnicodeStrToAsciiStr(Argv[1],&Parameter);
	    if(AsciiStriCmp(&Parameter,"ATTR")==0)
	    {
		    AttrCheck = TRUE;
	    }else
	    {
		    MaxAddress = StrHexToUintn(Argv[1]);
	    }
    }else if(Argc==3)
    {
	    MaxAddress = StrHexToUintn(Argv[1]);
	    AttrCheck  = TRUE;
    }else if(Argc==4)
    {
        ZXBus      = (UINT8)StrHexToUintn(Argv[1]);
        ZXDev      = (UINT8)StrHexToUintn(Argv[2]);
        ZXFunc     = (UINT8)StrHexToUintn(Argv[3]);
	    ZX_SATA    = PCI_LIB_ADDRESS(ZXBus, ZXDev, ZXFunc, 0);
    }else if(Argc==5)
    {
        ZXBus      = (UINT8)StrHexToUintn(Argv[1]);
        ZXDev      = (UINT8)StrHexToUintn(Argv[2]);
        ZXFunc     = (UINT8)StrHexToUintn(Argv[3]);
	    ZX_SATA    = PCI_LIB_ADDRESS(ZXBus, ZXDev, ZXFunc, 0);

	    UnicodeStrToAsciiStr(Argv[4],&Parameter);
	    if(AsciiStriCmp(&Parameter,"ATTR")==0)
	    {
		    AttrCheck = TRUE;
	    }else
	    {
		    MaxAddress = StrHexToUintn(Argv[1]);
	    }
    }else if(Argc==6)
    {
        ZXBus      = (UINT8)StrHexToUintn(Argv[1]);
        ZXDev      = (UINT8)StrHexToUintn(Argv[2]);
        ZXFunc     = (UINT8)StrHexToUintn(Argv[3]);
	    ZX_SATA    = PCI_LIB_ADDRESS(ZXBus, ZXDev, ZXFunc, 0);
		MaxAddress = StrHexToUintn(Argv[1]);
		AttrCheck  = TRUE;
    }else
    {
        Print(L"[Usage]: PLEASE INPUT IN ORDER!\n");
        Print(L"SataEphytest.efi [BusNum DevNum FuncNum] [MaxAddress] [Attr]\n");
      	return EFI_UNSUPPORTED;
    }

    FunctionPcidump();
    SataEphySettingDump();
    if(AttrCheck)
    {
	    SataEphySettingCheckAll();
    }

  	Print(L"[Usage]: PLEASE INPUT IN ORDER!\n");
  	Print(L"SataEphytest.efi [BusNum DevNum FuncNum] [MaxAddress] [Attr]\n");
    return EFI_SUCCESS;
}
