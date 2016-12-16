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
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/PciLib.h>
#include <Library/IoLib.h>


#define PCI_LIB_ADDRESS(Bus,Device,Function,Offset)   \
  (((Offset) & 0xfff) | (((Function) & 0x07) << 12) | (((Device) & 0x1f) << 15) | (((Bus) & 0xff) << 20))

#define MAX_PCI_BUSS              40
#define SATA_CFG_IDE              0x01
#define SATA_CFG_AHCI             0x06
#define ZX_SATA_DEV               15
#define SATA_CFG_IDE_MAXPORT      0x4

UINTN   ZX_SATA;


VOID
FunctionPcidump()
{
    UINT8        Index1;
    UINT8        Index2;
    UINT8        Data;

	DEBUG((EFI_D_ERROR, "     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"));
    Print(L"     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	DEBUG((EFI_D_ERROR, "---+-------------------------------------------------\n"));
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
			DEBUG((EFI_D_ERROR, "----------------------------------------------------\n"));
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


EFI_STATUS
EFIAPI
CheckRx5B()
{
    UINT8    Index;
    UINT8    Value;
    
    for(Index=0; Index<5; Index++)
    {
	    Value=PciRead8(ZX_SATA|0x5B);
	    DEBUG((EFI_D_ERROR, "Check Rx58 = 0x%x\n",Value));
        Print(L"Check Rx58 = 0x%x\n",Value);
	    if(Value==0x0)
	    {
	        return EFI_SUCCESS;
        }
    }
    return EFI_NOT_READY;
}



EFI_STATUS
EFIAPI
LoopbackTest(
    IN  UINT8      Port
    )
{
    EFI_STATUS     Status;
  
    DEBUG((EFI_D_ERROR, "Write Rx42 = 0x%x\n",Port));
    Print(L"Write Rx42 = 0x%x\n",Port);
    PciWrite8((ZX_SATA|0x42),Port);

    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x0\n"));
    Print(L"Write Rx58 = 0x0\n");
    PciWrite8((ZX_SATA|0x58),0x0);

    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x80\n"));
    Print(L"Write Rx58 = 0x80\n");
    PciWrite8((ZX_SATA|0x58),0x80);

    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x86\n"));
    Print(L"Write Rx58 = 0x87\n");
    PciWrite8((ZX_SATA|0x58),0x87);

    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x88\n"));
    Print(L"Write Rx58 = 0x88\n");
    PciWrite8((ZX_SATA|0x58),0x88);

    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x89\n"));
    Print(L"Write Rx58 = 0x89\n");
    PciWrite8((ZX_SATA|0x58),0x89);

    Status = CheckRx5B();
    if(EFI_ERROR(Status))
    {
	    DEBUG((EFI_D_ERROR, "[Port%d]: Loop Back Test Fail!\n",Port));
	    Print(L"[Port%d]: Loop Back Test Fail!\n",Port);
	    return EFI_NOT_READY;
    }

    //////////////////////////////////////////////////////////////////////////
    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x88\n"));
    Print(L"Write Rx58 = 0x88\n");
    PciWrite8((ZX_SATA|0x58),0x88);

    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x8E\n"));
    Print(L"Write Rx58 = 0x8E\n");
    PciWrite8((ZX_SATA|0x58),0x8E);

    Status = CheckRx5B();
    if(EFI_ERROR(Status))
    {
	    DEBUG((EFI_D_ERROR, "[Port%d]: Loop Back Test Fail!\n",Port));
	    Print(L"[Port%d]: Loop Back Test Fail!\n",Port);
	    return EFI_NOT_READY;
    }

    //////////////////////////////////////////////////////////////////////////
    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x88\n"));
    Print(L"Write Rx58 = 0x88\n");
    PciWrite8((ZX_SATA|0x58),0x88);

    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x8A\n"));
    Print(L"Write Rx58 = 0x8A\n");
    PciWrite8((ZX_SATA|0x58),0x8A);

    Status = CheckRx5B();
    if(EFI_ERROR(Status))
    {
	    DEBUG((EFI_D_ERROR, "[Port%d]: Loop Back Test Fail!\n",Port));
	    Print(L"[Port%d]: Loop Back Test Fail!\n",Port);
	    return EFI_NOT_READY;
    }

    //////////////////////////////////////////////////////////////////////////
    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x88\n"));
    Print(L"Write Rx58 = 0x88\n");
    PciWrite8((ZX_SATA|0x58),0x88);

    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x8B\n"));
    Print(L"Write Rx58 = 0x8B\n");
    PciWrite8((ZX_SATA|0x58),0x8B);

    Status = CheckRx5B();
    if(EFI_ERROR(Status))
    {
	    DEBUG((EFI_D_ERROR, "[Port%d]: Loop Back Test Fail!\n",Port));
	    Print(L"[Port%d]: Loop Back Test Fail!\n",Port);
	    return EFI_NOT_READY;
    }    

    //////////////////////////////////////////////////////////////////////////
    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x88\n"));
    Print(L"Write Rx58 = 0x88\n");
    PciWrite8((ZX_SATA|0x58),0x88);

    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x8C\n"));
    Print(L"Write Rx58 = 0x8C\n");
    PciWrite8((ZX_SATA|0x58),0x8C);

    Status = CheckRx5B();
    if(EFI_ERROR(Status))
    {
	    DEBUG((EFI_D_ERROR, "[Port%d]: Loop Back Test Fail!\n",Port));
	    Print(L"[Port%d]: Loop Back Test Fail!\n",Port);
	    return EFI_NOT_READY;
    }

    //////////////////////////////////////////////////////////////////////////
    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x88\n"));
    Print(L"Write Rx58 = 0x88\n");
    PciWrite8((ZX_SATA|0x58),0x88);

    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x8D\n"));
    Print(L"Write Rx58 = 0x8D\n");
    PciWrite8((ZX_SATA|0x58),0x8D);

    Status = CheckRx5B();
    if(EFI_ERROR(Status))
    {
	    DEBUG((EFI_D_ERROR, "[Port%d]: Loop Back Test Fail!\n",Port));
	    Print(L"[Port%d]: Loop Back Test Fail!\n",Port);
	    return EFI_NOT_READY;
    }

    //////////////////////////////////////////////////////////////////////////
    DEBUG((EFI_D_ERROR, "Write Rx58 = 0x88\n"));
    Print(L"Write Rx58 = 0x88\n");
    PciWrite8((ZX_SATA|0x58),0x88);

    Status = CheckRx5B();
    if(EFI_ERROR(Status))
    {
	    DEBUG((EFI_D_ERROR, "[Port%d]: Loop Back Test Fail!\n",Port));
	    Print(L"[Port%d]: Loop Back Test Fail!\n",Port);
	    return EFI_NOT_READY;
    }
    
    DEBUG((EFI_D_ERROR, "[Port%d]: Loop Back Test Success!\n",Port));
    Print(L"[Port%d]: Loop Back Test Success!\n",Port);
    
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
ShellAppMain (
  IN UINTN          Argc,
  IN CHAR16       **Argv
  )
{
    //EFI_STATUS    Status;
    UINT8           SataBusNum = 0;
    UINT8           Bus;
    UINT8           Dev;
    UINT8           Func;
    UINT8           Port;
    UINT16          VendorID;
    UINT16          DeviceID;
    UINT8           Satacfg;

    if(Argc==2)
    {       
	    SataBusNum = SataScanPciBus();
	    if (SataBusNum == 0) {
	        Print(L"[Lex]: NO ZX SATA CONTROLLER FOUND AT BUSX D15F0!\n");
	        return EFI_SUCCESS;
	    }
	  
	    Print(L"[Lex]: ZX SATA CONTROLLER FOUND AT BUS%d D15F0!\n",SataBusNum);
	    ZX_SATA = PCI_LIB_ADDRESS(SataBusNum, ZX_SATA_DEV, 0, 0);
	    Port = (UINT8)StrHexToUintn(Argv[1]);
    }else if(Argc==5)
    {
        Bus  = (UINT8)StrHexToUintn(Argv[1]);
        Dev  = (UINT8)StrHexToUintn(Argv[2]);
        Func = (UINT8)StrHexToUintn(Argv[3]);
        Port = (UINT8)StrHexToUintn(Argv[4]);
	    ZX_SATA = PCI_LIB_ADDRESS(Bus, Dev, Func, 0);
	    VendorID = PciRead16(ZX_SATA|0x0);
        DeviceID = PciRead16(ZX_SATA|0x2);
        if((VendorID==0x1106 || VendorID==0x1d17) && (DeviceID==0x9002 || DeviceID==0x9083))
        {
            Print(L"[Lex]: ZX SATA CONTROLLER AT BUS%d D%dF%d!\n", Bus, Dev, Func);
        }else
        {
	        Print(L"[Lex]: ZX SATA CONTROLLER NOT FOUND AT BUS%d D%dF%d!\n", Bus, Dev, Func);
	        return EFI_UNSUPPORTED;
        }
    }else{
	    Print(L"[Usage]:\n");
	    Print(L"\tSataLoopback.efi Port\n");
	    Print(L"\tSataLoopback.efi Bus Dev Func Port\n");
	    return EFI_UNSUPPORTED;
    }
    
    DEBUG((EFI_D_ERROR, "Dump SATA PCI Config Space:\n"));
    Print(L"Dump SATA PCI Config Space:\n");
    FunctionPcidump();

    
    //////////////////////////////////////////////////////////////////////////////
    Satacfg=PciRead8(ZX_SATA|0xA);
   
    if(Satacfg == SATA_CFG_AHCI)
	{
        DEBUG((EFI_D_ERROR, "[ERROR]: SATA IN AHCI Mode! Change To IDE Mode First!\n"));
        Print(L"[ERROR]: SATA IN AHCI Mode! Change To IDE Mode First!\n");
		PciWrite8((ZX_SATA|0x45),(PciRead8(ZX_SATA|0xA)|BIT7));
		PciWrite8((ZX_SATA|0xA),0x01);
		Satacfg=PciRead8(ZX_SATA|0xA);
		if(Satacfg == SATA_CFG_AHCI)
	    {
	        DEBUG((EFI_D_ERROR, "[ERROR]: Change To IDE Mode Fail!\n"));
	        Print(L"[ERROR]: Change To IDE Mode Fail!\n");
	        return EFI_UNSUPPORTED;
	    }else if(Satacfg == SATA_CFG_IDE)
	    {
	        DEBUG((EFI_D_ERROR, "[IDE Mode]:\n"));
            Print(L"[IDE Mode]:\n");
            LoopbackTest(Port);
            DEBUG((EFI_D_ERROR, "\n"));
            Print(L"\n");
	    }
	}else if(Satacfg == SATA_CFG_IDE)
	{
        DEBUG((EFI_D_ERROR, "[IDE Mode]:\n"));
        Print(L"[IDE Mode]:\n");
        LoopbackTest(Port);
        DEBUG((EFI_D_ERROR, "\n"));
        Print(L"\n");
    }

    return EFI_SUCCESS;
}
