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

#define MAX_PCI_BUSS      40
#define SATA_CFG_IDE      0x01
#define SATA_CFG_AHCI     0x06
#define ZX_SATA_DEV       15


UINTN   ZX_SATA;

typedef struct {
    char    *Signature;
    UINT32   MmioBit;
    UINT8    PciReg;
    UINT8    PciBit;
} AHCI_BACKDOOR_BIT; 

STATIC AHCI_BACKDOOR_BIT gAhciBackdoorBit[] = {
    {"CAP[4-0]  , RxFB[3-0]", BIT0|BIT1|BIT2|BIT3|BIT4,    0xFB, BIT0|BIT1|BIT2|BIT3 },  //Special Bit
    {"CAP[7-5]  , RxFA[4-2]", BIT5|BIT6|BIT7,              0xFA, BIT2|BIT3|BIT4      },
    {"CAP[12-8] , RxFA[1-0]", BIT8|BIT9|BIT10|BIT11|BIT12, 0xFA, BIT0|BIT1           },         //Special Bit
    {"CAP[14-13], RxFA[6-5]", BIT13|BIT14,                 0xFA, BIT5|BIT6           },
    {"CAP[16]   , RxFB[7]  ", BIT16,                       0xFA, BIT7                },
    {"CAP[18-17], RxF9[1-0]", BIT17|BIT18,                 0xF9, BIT0|BIT1           },
    {"CAP[26-24], RxF9[4-2]", BIT24|BIT25|BIT26,           0xF9, BIT2|BIT3|BIT4      },
    {"CAP[28],    RxDB[5]  ", BIT28,                       0xDB, BIT5                },
    {"CAP[31-29], RxF9[7-5]", BIT29|BIT30|BIT31,           0xF9, BIT5|BIT6|BIT7      },
};

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
BackdoorTestSlot(
    IN  UINT32    AhciBase,
    IN  UINT32    Index
    )
{
    UINT32     Tmp0;
    UINT32     Tmp1;
    UINT32     Tmp2;
    UINT32     Tmp3;
    
    PciWrite8((ZX_SATA|gAhciBackdoorBit[Index].PciReg),((PciRead8(ZX_SATA|gAhciBackdoorBit[Index].PciReg))&(~gAhciBackdoorBit[Index].PciBit)));  //one port, need change if not begin with bit0
    gBS->Stall(100);
    Tmp0=MmioRead32(AhciBase|0x0)&(BIT8|BIT9|BIT9|BIT10|BIT11|BIT12);

    PciWrite8((ZX_SATA|gAhciBackdoorBit[Index].PciReg),((PciRead8(ZX_SATA|gAhciBackdoorBit[Index].PciReg))&(~gAhciBackdoorBit[Index].PciBit)|(BIT0)));  //two port, need change if not begin with bit0
    gBS->Stall(100);
    Tmp1=MmioRead32(AhciBase|0x0)&(BIT8|BIT9|BIT9|BIT10|BIT11|BIT12);

    PciWrite8((ZX_SATA|gAhciBackdoorBit[Index].PciReg),((PciRead8(ZX_SATA|gAhciBackdoorBit[Index].PciReg))&(~gAhciBackdoorBit[Index].PciBit)|(BIT1)));  //three port, need change if not begin with bit0
    gBS->Stall(100);
    Tmp2=MmioRead32(AhciBase|0x0)&(BIT8|BIT9|BIT9|BIT10|BIT11|BIT12);

    PciWrite8((ZX_SATA|gAhciBackdoorBit[Index].PciReg),((PciRead8(ZX_SATA|gAhciBackdoorBit[Index].PciReg))&(~gAhciBackdoorBit[Index].PciBit)|(BIT0|BIT1)));  //four port, need change if not begin with bit0
    gBS->Stall(100);
    Tmp3=MmioRead32(AhciBase|0x0)&(BIT8|BIT9|BIT9|BIT10|BIT11|BIT12);

    if((Tmp0==0x0)&&(Tmp1==(BIT8|BIT9|BIT10))&&(Tmp2==(BIT8|BIT9|BIT10|BIT11))&&(Tmp3==(BIT8|BIT9|BIT10|BIT11|BIT12)))
    {
	    Print(L"%a: Success!\n", gAhciBackdoorBit[Index].Signature);
	}else
	{
		Print(L"%a: Fail!\n", gAhciBackdoorBit[Index].Signature);
	}
	
    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
BackdoorTestPort(
    IN  UINT32    AhciBase,
    IN  UINT32    Index
    )
{
    UINT32     Tmp0;
    UINT32     Tmp1;
    UINT32     Tmp2;
    UINT32     Tmp3;

    PciWrite8((ZX_SATA|gAhciBackdoorBit[Index].PciReg),((PciRead8(ZX_SATA|gAhciBackdoorBit[Index].PciReg))&(~gAhciBackdoorBit[Index].PciBit)));  //one port, need change if not begin with bit0
    gBS->Stall(100);
    Tmp0=MmioRead32(AhciBase|0x0)&(BIT0|BIT1|BIT2|BIT3|BIT4);

    PciWrite8((ZX_SATA|gAhciBackdoorBit[Index].PciReg),((PciRead8(ZX_SATA|gAhciBackdoorBit[Index].PciReg))&(~gAhciBackdoorBit[Index].PciBit)|(BIT0)));  //two port, need change if not begin with bit0
    gBS->Stall(100);
    Tmp1=MmioRead32(AhciBase|0x0)&(BIT0|BIT1|BIT2|BIT3|BIT4);

    PciWrite8((ZX_SATA|gAhciBackdoorBit[Index].PciReg),((PciRead8(ZX_SATA|gAhciBackdoorBit[Index].PciReg))&(~gAhciBackdoorBit[Index].PciBit)|(BIT1)));  //three port, need change if not begin with bit0
    gBS->Stall(100);
    Tmp2=MmioRead32(AhciBase|0x0)&(BIT0|BIT1|BIT2|BIT3|BIT4);

    PciWrite8((ZX_SATA|gAhciBackdoorBit[Index].PciReg),((PciRead8(ZX_SATA|gAhciBackdoorBit[Index].PciReg))&(~gAhciBackdoorBit[Index].PciBit)|(BIT0|BIT1)));  //four port, need change if not begin with bit0
    gBS->Stall(100);
    Tmp3=MmioRead32(AhciBase|0x0)&(BIT0|BIT1|BIT2|BIT3|BIT4);
 
    if((Tmp0 == 0x0)&&(Tmp1==BIT0)&&(Tmp2==BIT1)&&(Tmp3==(BIT0|BIT1)))
    {
	    Print(L"%a: Success!\n", gAhciBackdoorBit[Index].Signature);
	}else
	{
		Print(L"%a: Fail!\n", gAhciBackdoorBit[Index].Signature);
	}
	
    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
BackdoorTest(
    IN  UINT32    AhciBase,
    IN  UINT32    Index
    )
{
    UINT32     Tmp0;
    UINT32     Tmp1;

    PciWrite8((ZX_SATA|gAhciBackdoorBit[Index].PciReg),(PciRead8(ZX_SATA|gAhciBackdoorBit[Index].PciReg)&(~gAhciBackdoorBit[Index].PciBit)));
    gBS->Stall(100);
    Tmp0=MmioRead32(AhciBase|0x0)&gAhciBackdoorBit[Index].MmioBit;
    
    PciWrite8((ZX_SATA|gAhciBackdoorBit[Index].PciReg),(PciRead8(ZX_SATA|gAhciBackdoorBit[Index].PciReg)|gAhciBackdoorBit[Index].PciBit));
    gBS->Stall(100);
    Tmp1=MmioRead32(AhciBase|0x0)&gAhciBackdoorBit[Index].MmioBit;

    if((Tmp0==0)&&(Tmp1==gAhciBackdoorBit[Index].MmioBit))
    {
	    Print(L"%a: Success!\n", gAhciBackdoorBit[Index].Signature);
	}else
	{
		Print(L"%a: Fail!\n", gAhciBackdoorBit[Index].Signature);
	}
	
    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
BackdoorTestAll()
{
    UINT32        AhciBase;
    UINT32        Count;
    UINT32        Index;

    AhciBase = PciRead32(ZX_SATA|0x24);
    
    Count = sizeof(gAhciBackdoorBit)/sizeof(gAhciBackdoorBit[0]);
    for(Index=0; Index<Count; Index++)
    {
        if((gAhciBackdoorBit[Index].MmioBit & BIT0)==BIT0)
        {
	        BackdoorTestPort(AhciBase, Index);
	    }else if((gAhciBackdoorBit[Index].MmioBit & BIT8)==BIT8)
	    {
		    BackdoorTestSlot(AhciBase, Index);
		}else{
	        BackdoorTest(AhciBase, Index);
	    }
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
    UINT16          VendorID;
    UINT16          DeviceID;
    UINT8           Satacfg;

    if(Argc==1)
    {       
	    SataBusNum = SataScanPciBus();
	    if (SataBusNum == 0) 
	    {
	        Print(L"[Lex]: NO ZX SATA CONTROLLER FOUND AT BUSX D15F0!\n");
	        return EFI_SUCCESS;
	    }
	  
	    Print(L"[Lex]: ZX SATA CONTROLLER FOUND AT BUS%d D15F0!\n",SataBusNum);
	    ZX_SATA = PCI_LIB_ADDRESS(SataBusNum, ZX_SATA_DEV, 0, 0);
    }else if(Argc==4)
    {
        Bus  = (UINT8)StrHexToUintn(Argv[1]);
        Dev  = (UINT8)StrHexToUintn(Argv[2]);
        Func = (UINT8)StrHexToUintn(Argv[3]);
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
	    Print(L"\tSataBackdoor.efi\n");
	    Print(L"\tSataBackdoor.efi Bus Dev Func\n");
	    return EFI_UNSUPPORTED;
    }
    
    DEBUG((EFI_D_ERROR, "Dump SATA PCI Config Space:\n"));
    Print(L"Dump SATA PCI Config Space:\n");
    FunctionPcidump();

      
    //////////////////////////////////////////////////////////////////////////////
    Satacfg=PciRead8(ZX_SATA|0xA);
    
    if(Satacfg == SATA_CFG_IDE)
    {
        DEBUG((EFI_D_ERROR, "[ERROR]: SATA IN IDE Mode! Change To AHCI Mode First!\n"));
        Print(L"[ERROR]: SATA IN IDE Mode! Change To AHCI Mode First!\n");
        return EFI_UNSUPPORTED;
    }else if(Satacfg == SATA_CFG_AHCI)
    {
        DEBUG((EFI_D_ERROR, "[AHCI Mode]:\n"));
        Print(L"[AHCI Mode]:\n");
        BackdoorTestAll();
        DEBUG((EFI_D_ERROR, "\n"));
    }
    return EFI_SUCCESS;
}
