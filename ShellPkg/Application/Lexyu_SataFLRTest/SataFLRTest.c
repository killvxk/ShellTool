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

#define  MAX_PCI_BUSS      40
#define  SATA_CFG_IDE      0x01
#define  SATA_CFG_AHCI     0x06
#define  SATA_CFG_BOTH     0x08
#define  ZX_SATA_DEV       15

UINTN    ZX_SATA;
BOOLEAN  gIOCheck = FALSE;


typedef struct {
    char    *Signature;
    BOOLEAN  IOCheck;
    UINT8    Mode;
    UINT8    Reg;
    UINT8    BIT;
    UINT8    BITDefaultValue;
} REG_BIT; 

STATIC REG_BIT gRegBit[] = {
    {"Rx05[2]  ", FALSE, SATA_CFG_BOTH, 0x05, BIT2,                          0x0  },
    {"Rx07[5-4]", FALSE, SATA_CFG_BOTH, 0x07, BIT4|BIT5,                     0x0  },
    {"Rx0D[7-4]", FALSE, SATA_CFG_BOTH, 0x0D, BIT4|BIT5|BIT6|BIT7,           0x20 },
    {"Rx10[7-3]", TRUE , SATA_CFG_IDE,  0x10, BIT3|BIT4|BIT5|BIT6|BIT7,      0xF0 },
    {"Rx11[7-0]", TRUE , SATA_CFG_IDE,  0x11, 0xFF,                          0x01 },
    {"Rx14[7-2]", TRUE , SATA_CFG_IDE,  0x14, BIT2|BIT3|BIT4|BIT5|BIT6|BIT7, 0xF4 },
    {"Rx15[7-0]", TRUE , SATA_CFG_IDE,  0x15, 0xFF,                          0x03 },
    {"Rx18[7-3]", TRUE , SATA_CFG_IDE,  0x18, BIT3|BIT4|BIT5|BIT6|BIT7,      0x70 },
    {"Rx19[7-0]", TRUE , SATA_CFG_IDE,  0x19, 0xFF,                          0x01 },
    {"Rx18[7-6]", TRUE , SATA_CFG_AHCI, 0x18, BIT6|BIT7,                     0x0  },
    {"Rx19[7-0]", TRUE , SATA_CFG_AHCI, 0x19, 0xFF,                          0x0  },
    {"Rx1A[7-0]", TRUE , SATA_CFG_AHCI, 0x1A, 0xFF,                          0x0  },
    {"Rx1B[7-0]", TRUE , SATA_CFG_AHCI, 0x1B, 0xFF,                          0x0  },
    {"Rx1C[7-2]", TRUE , SATA_CFG_IDE,  0x1C, BIT2|BIT3|BIT4|BIT5|BIT6|BIT7, 0x74 },
    {"Rx1D[7-0]", TRUE , SATA_CFG_IDE,  0x1D, 0xFF,                          0x03 },
    {"Rx1C[7-4]", TRUE , SATA_CFG_AHCI, 0x1C, BIT4|BIT5|BIT6|BIT7,           0x0  },
    {"Rx1D[7-0]", TRUE , SATA_CFG_AHCI, 0x1D, 0xFF,                          0x0  },
    {"Rx1E[5-4]", TRUE , SATA_CFG_AHCI, 0x1E, 0xFF,                          0x0  },
    {"Rx1F[5-4]", TRUE , SATA_CFG_AHCI, 0x1F, 0xFF,                          0x0  },
    {"Rx20[7-4]", TRUE , SATA_CFG_IDE,  0x20, BIT4|BIT5|BIT6|BIT7,           0x0  },
    {"Rx21[7-0]", TRUE , SATA_CFG_IDE,  0x21, 0xFF,                          0xCC },   
    {"Rx20[7-5]", TRUE , SATA_CFG_AHCI, 0x20, BIT5|BIT6|BIT7,                0x0  },
    {"Rx21[7-0]", TRUE , SATA_CFG_AHCI, 0x21, 0xFF,                          0xCC },
    {"Rx25[7-5]", TRUE , SATA_CFG_AHCI, 0x25, BIT5|BIT6|BIT7,                0x0  },
    {"Rx26[7-0]", TRUE , SATA_CFG_AHCI, 0x26, 0xFF,                          0x0  },
    {"Rx27[7-0]", TRUE , SATA_CFG_AHCI, 0x27, 0xFF,                          0x0  },     
    {"RxA3[7-6]", FALSE, SATA_CFG_AHCI, 0xA3, BIT6|BIT7,                     0x0  },
    {"RxA4[7-0]", FALSE, SATA_CFG_AHCI, 0xA4, 0xFF,                          0x02 },
    {"RxA5[7-0]", FALSE, SATA_CFG_AHCI, 0xA5, 0xFF,                          0x0  },
    {"RxA6[7-0]", FALSE, SATA_CFG_AHCI, 0xA6, 0xFF,                          0x0  },
    {"RxA7[5-4]", FALSE, SATA_CFG_AHCI, 0xA7, 0xFF,                          0x0  },
    {"RxC4[1-0]", FALSE, SATA_CFG_BOTH, 0xC4, BIT0|BIT1,                     0x0  },
    {"RxC5[0]  ", FALSE, SATA_CFG_BOTH, 0xC5, BIT0,                          0x0  }, 
    {"RxE2[0]  ", FALSE, SATA_CFG_AHCI, 0xE2, BIT0,                          0x0  },
    {"RxE2[6-4]", FALSE, SATA_CFG_AHCI, 0xE2, BIT4|BIT5|BIT6,                0x0  },            
    {"RxE4[7-2]", FALSE, SATA_CFG_AHCI, 0xE4, BIT2|BIT3|BIT4|BIT5|BIT6|BIT7, 0x0  },
    {"RxE5[5-4]", FALSE, SATA_CFG_AHCI, 0xE5, 0xFF,                          0x0  },
    {"RxE6[5-4]", FALSE, SATA_CFG_AHCI, 0xE6, 0xFF,                          0x0  },
    {"RxE7[5-4]", FALSE, SATA_CFG_AHCI, 0xE7, 0xFF,                          0x0  },
    {"RxE8[5-4]", FALSE, SATA_CFG_AHCI, 0xE8, 0xFF,                          0x0  },
    {"RxE9[5-4]", FALSE, SATA_CFG_AHCI, 0xE9, 0xFF,                          0x0  },
    {"Rx04[2-0]", FALSE, SATA_CFG_BOTH, 0x04, BIT0|BIT1|BIT2,                0x0  },
};


VOID
FunctionPcidump()
{
    UINT8        Index1;
    UINT8        Index2;
    UINT8        Data;

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


EFI_STATUS
EFIAPI
FLRTest(
  IN       UINT8      Index
  )
{
    UINT8     Tmp0;
    UINT8     Tmp1;
    UINT8     Tmp2;
    UINT8     Tmp3;
    
    PciWrite8((ZX_SATA|gRegBit[Index].Reg),(PciRead8(ZX_SATA|gRegBit[Index].Reg)|gRegBit[Index].BIT));
    Tmp0=PciRead8(ZX_SATA|gRegBit[Index].Reg)&gRegBit[Index].BIT;
	PciWrite8((ZX_SATA|0xB4),(PciRead8(ZX_SATA|0xB4)|BIT0));
    gBS->Stall(100);
    Tmp1=PciRead8(ZX_SATA|gRegBit[Index].Reg)&gRegBit[Index].BIT;

	PciWrite8((ZX_SATA|gRegBit[Index].Reg),(PciRead8(ZX_SATA|gRegBit[Index].Reg)&(~gRegBit[Index].BIT)));
    Tmp2=PciRead8(ZX_SATA|gRegBit[Index].Reg)&gRegBit[Index].BIT;
	PciWrite8((ZX_SATA|0xB4),(PciRead8(ZX_SATA|0xB4)|BIT0));
    gBS->Stall(100);
    Tmp3=PciRead8(ZX_SATA|gRegBit[Index].Reg)&gRegBit[Index].BIT;

    DEBUG((EFI_D_ERROR,"%a: [Write All 1, %02x-%02x], [Write All 0, %02x-%02x], ",gRegBit[Index].Signature,Tmp0,Tmp1,Tmp2,Tmp3));
    Print(L"%a: [Write All 1, %02x-%02x], [Write All 0, %02x-%02x], ",gRegBit[Index].Signature,Tmp0,Tmp1,Tmp2,Tmp3);
    if((Tmp1==gRegBit[Index].BITDefaultValue)&&(Tmp3==gRegBit[Index].BITDefaultValue))
    {
        DEBUG((EFI_D_ERROR, "Success!\n"));
	    Print(L"Success!\n");
    }else
    {
	    DEBUG((EFI_D_ERROR, "Fail!\n"));
	    Print(L"Fail!\n");
    }
    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
FLRTestAll()
{
    UINT8        Satacfg;
    UINT8        Count;
    UINT8        Index;
    
    
    Satacfg=PciRead8(ZX_SATA|0xA);
   
	if(Satacfg == SATA_CFG_IDE)
	{
        DEBUG((EFI_D_ERROR, "[IDE Mode]:\n"));
        Print(L"[IDE Mode]:\n");		
	}else if(Satacfg == SATA_CFG_AHCI)
	{
        DEBUG((EFI_D_ERROR, "[AHCI Mode]:\n"));
        Print(L"[AHCI Mode]:\n");
	}

    PciWrite8((ZX_SATA|0x4),(PciRead8(ZX_SATA|0x4)&(~(BIT0|BIT1|BIT2))));
    
    Count = sizeof(gRegBit)/sizeof(gRegBit[0]);
    for(Index=0; Index<Count; Index++)
    {
	    if((gIOCheck==FALSE)&&(gRegBit[Index].IOCheck==FALSE))
	    {
	        if((gRegBit[Index].Mode == SATA_CFG_BOTH) || (gRegBit[Index].Mode == Satacfg))
	        {
	            FLRTest(Index);
	        }
        }else if((gIOCheck==TRUE)&&(gRegBit[Index].IOCheck==TRUE))
        {
	        if((gRegBit[Index].Mode == SATA_CFG_BOTH) || (gRegBit[Index].Mode == Satacfg))
	        {
	            FLRTest(Index);
	        }
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
    CHAR8           Tmp;
    UINT8           Satacfg;


    if(Argc==1 || Argc==2)
    {       
	    SataBusNum = SataScanPciBus();
	    if (SataBusNum == 0) {
	        Print(L"[Lex]: NO ZX SATA CONTROLLER FOUND AT BUSX D15F0!\n");
	        return EFI_SUCCESS;
	    }
	  
	    Print(L"[Lex]: ZX SATA CONTROLLER FOUND AT BUS%d D15F0!\n",SataBusNum);
	    ZX_SATA = PCI_LIB_ADDRESS(SataBusNum, ZX_SATA_DEV, 0, 0);
		if(Argc==2)
	    {
			UnicodeStrToAsciiStr(Argv[1],&Tmp);
	        if(AsciiStriCmp(&Tmp,"IO")==0)
	        {
	            gIOCheck = TRUE;
	        }
	    }
    }else if(Argc==4 || Argc==5)
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
        if(Argc==5)
	    {
			UnicodeStrToAsciiStr(Argv[1],&Tmp);
	        if(AsciiStriCmp(&Tmp,"IO")==0)
	        {
	            gIOCheck = TRUE;
	        }
	    }
    }else{
	    Print(L"[Usage]:\n");
	    Print(L"\tSataFLRTest.efi IO\n");
	    Print(L"\tSataFLRTest.efi Bus Dev Func IO\n");
	    return EFI_UNSUPPORTED;
    }
    
    DEBUG((EFI_D_ERROR, "Dump SATA PCI Config Space:\n"));
    Print(L"Dump SATA PCI Config Space:\n");
    FunctionPcidump();

	
    ////////////////////////////////////////////////////////////////////////////////////////////////
	FLRTestAll();

	//Change to another mode
	DEBUG((EFI_D_ERROR,"\n"));
	Print(L"\n");
	Satacfg=PciRead8(ZX_SATA|0xA);
	PciWrite8((ZX_SATA|0x45),(PciRead8(ZX_SATA|0xA)|BIT7));
	if(Satacfg == SATA_CFG_IDE)
	{
		PciWrite8((ZX_SATA|0xA),0x06);
	}else if(Satacfg == SATA_CFG_AHCI)
	{
		PciWrite8((ZX_SATA|0xA),0x01);
	}
	FLRTestAll();
  
    return EFI_SUCCESS;
}
