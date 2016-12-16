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
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/PciLib.h>
#include <Library/IoLib.h>


#define PCI_LIB_ADDRESS(Bus,Device,Function,Offset)   \
  (((Offset) & 0xfff) | (((Function) & 0x07) << 12) | (((Device) & 0x1f) << 15) | (((Bus) & 0xff) << 20))


#define CND003_SATA_DEV   15
#define CND003_SATA       PCI_LIB_ADDRESS(0xD, CND003_SATA_DEV, 0, 0) // D15F0
#define SATA_CFG_IDE      0x01
#define SATA_CFG_AHCI     0x06



EFI_STATUS
EFIAPI
FLRTest(
    IN       UINT8      Reg,
    IN       UINT8      BITX
    )
{
    UINT8     Tmp0;
    UINT8     Tmp1;
    UINT8     Tmp2;
    UINT8     Tmp3;

    PciWrite8((CND003_SATA|Reg),  (PciRead8(CND003_SATA|Reg)|BITX));
    Tmp0=PciRead8(CND003_SATA|Reg)&BITX;
	PciWrite8((CND003_SATA|0xB4), PciRead8(CND003_SATA|0xB4)|BIT0);
    gBS->Stall(100);
    Tmp1=PciRead8(CND003_SATA|Reg)&BITX;

	PciWrite8((CND003_SATA|Reg),  PciRead8(CND003_SATA|Reg)&(~BITX));
    Tmp2=PciRead8(CND003_SATA|Reg)&BITX;
	PciWrite8((CND003_SATA|0xB4), PciRead8(CND003_SATA|0xB4)|BIT0);
    gBS->Stall(100);
    Tmp3=PciRead8(CND003_SATA|Reg)&BITX;

    Print(L"Rx%02x: Tmp0/2/1/3=%02x-%02x-%02x-%02x\n",Reg,Tmp0,Tmp2,Tmp1,Tmp3);
    return EFI_SUCCESS;
}





/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/


EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE                  ImageHandle,
  IN EFI_SYSTEM_TABLE            *SystemTable
  )
{
  //EFI_STATUS                     Status;
  UINT8                          Mode;
  UINT8                          i;
  UINT8                          j;
  UINT8                          Tmp;


  Print(L"[lex]:Dump before Test\n");
  Print(L"|      "); 
  for(i=0;i<16;i+=4)
  {
      Print(L"|      %02x ~ %02x      ",i+3,i); 
  }
  Print(L"|\n"); 
  
  for(i=0;i<16;i++)
  {
      Print(L"|  %02x  ",i); 
      for(j=0;j<16;j++)
      {
          Tmp=PciRead8(CND003_SATA|(i*16+j));
          Print(L"| %02x ",Tmp); 
      }
      Print(L"|\n");
  }

//////////////////////////////////////////////////////////////////////////////////////
  Mode=PciRead8(CND003_SATA|0xA);

  //PciWrite8((CND003_SATA|0x04), 0xFF);
  FLRTest(0x04,BIT0);
  FLRTest(0x04,BIT1);
  FLRTest(0x04,BIT2);
  
  //PciWrite8((CND003_SATA|0x05), 0xFF); 
  FLRTest(0x05,BIT2);
  
  //PciWrite8((CND003_SATA|0x07), 0xFF);
  FLRTest(0x07,BIT4);
  FLRTest(0x07,BIT5);
  
  //PciWrite8((CND003_SATA|0x0D), 0xDF);
  FLRTest(0x0D,BIT4|BIT5|BIT6|BIT7);
  
  if(Mode==SATA_CFG_IDE)
  {
      //PciWrite8((CND003_SATA|0x10), 0x0E);
      FLRTest(0x10,BIT3|BIT4|BIT5|BIT6|BIT7);
            
      //PciWrite8((CND003_SATA|0x11), 0xFE);
      FLRTest(0x11,0xFF);
      
      //PciWrite8((CND003_SATA|0x14), 0x0A);
      FLRTest(0x14,BIT2|BIT3|BIT4|BIT5|BIT6|BIT7);
      
      //PciWrite8((CND003_SATA|0x15), 0xFC);
      FLRTest(0x15,0xFF);
      
      //PciWrite8((CND003_SATA|0x18), 0x8E);
      FLRTest(0x18,BIT3|BIT4|BIT5|BIT6|BIT7);
      
      //PciWrite8((CND003_SATA|0x19), 0xFE);
      FLRTest(0x19,0xFF);
      
      //PciWrite8((CND003_SATA|0x1C), 0x8A);
      FLRTest(0x1C,BIT2|BIT3|BIT4|BIT5|BIT6|BIT7);
      
      //PciWrite8((CND003_SATA|0x1D), 0xFC);
      FLRTest(0x1D,0xFF);

      //PciWrite8((CND003_SATA|0x20), 0xFE);
      FLRTest(0x20,BIT4|BIT5|BIT6|BIT7);
  
      //PciWrite8((CND003_SATA|0x21), 0x33);
      FLRTest(0x21,0xFF);   
  }
  else if(Mode==SATA_CFG_AHCI)
  {
      //PciWrite8((CND003_SATA|0x18), 0xFF);
      FLRTest(0x18,BIT6|BIT7);
      
      //PciWrite8((CND003_SATA|0x19), 0xFF);
      FLRTest(0x19,0xFF);
      
      //PciWrite8((CND003_SATA|0x1A), 0xFF);
      FLRTest(0x1A,0xFF);
      
      //PciWrite8((CND003_SATA|0x1B), 0xFF);
      FLRTest(0x1B,0xFF);
      
      //PciWrite8((CND003_SATA|0x1C), 0xFF);
      FLRTest(0x1C,BIT4|BIT5|BIT6|BIT7);
      
      //PciWrite8((CND003_SATA|0x1D), 0xFF);
      FLRTest(0x1D,0xFF);
      
      //PciWrite8((CND003_SATA|0x1E), 0xFF);
      FLRTest(0x1E,0xFF);
      
      //PciWrite8((CND003_SATA|0x1F), 0xFF);
      FLRTest(0x1F,0xFF);

      //PciWrite8((CND003_SATA|0x20), 0xFE);
      FLRTest(0x20,BIT5|BIT6|BIT7);
  
      //PciWrite8((CND003_SATA|0x21), 0x33);
      FLRTest(0x21,0xFF);
      
      //PciWrite8((CND003_SATA|0x25), 0xFF);
      FLRTest(0x25,BIT5|BIT6|BIT7);
      
      //PciWrite8((CND003_SATA|0x26), 0xFF);
      FLRTest(0x26,0xFF);
      
      //PciWrite8((CND003_SATA|0x27), 0xFF);
      FLRTest(0x27,0xFF);
      
      //PciWrite8((CND003_SATA|0xA3), 0xFF);
      FLRTest(0xA3,BIT6|BIT7);
      
      //PciWrite8((CND003_SATA|0xA4), 0xFD);
      FLRTest(0xA4,0xFF);
      
      //PciWrite8((CND003_SATA|0xA5), 0xFF);
	  FLRTest(0xA5,0xFF);
      
      //PciWrite8((CND003_SATA|0xA6), 0xFF);
      FLRTest(0xA6,0xFF);
      
      //PciWrite8((CND003_SATA|0xA7), 0x00);
      FLRTest(0xA7,0xFF);
      
      //PciWrite8((CND003_SATA|0xE2), 0xFB);
      FLRTest(0xE2,BIT0|BIT4|BIT5|BIT6);
            
      //PciWrite8((CND003_SATA|0xE4), 0xFF);
      FLRTest(0xE4,BIT2|BIT3|BIT4|BIT5|BIT6|BIT7);
      
      //PciWrite8((CND003_SATA|0xE5), 0xFF);
      FLRTest(0xE5,0xFF);
      
      //PciWrite8((CND003_SATA|0xE6), 0xFF);
      FLRTest(0xE6,0xFF);
      
      //PciWrite8((CND003_SATA|0xE7), 0xFF);
      FLRTest(0xE7,0xFF);
      
      //PciWrite8((CND003_SATA|0xE8), 0xFF);
      FLRTest(0xE8,0xFF);
      
      //PciWrite8((CND003_SATA|0xE9), 0xFF);
      FLRTest(0xE9,0xFF);
  }

  //PciWrite8((CND003_SATA|0xC4), 0xFF);
  FLRTest(0xC4,BIT0|BIT1);
  
  //PciWrite8((CND003_SATA|0xC5), 0xFF);
  FLRTest(0xC5,BIT0);
  
/////////////////////////////////////////////////////////////////////////////////////////////////////


#if 0
  Print(L"[lex]:Dump After Write\n");
  Print(L"|      "); 
  for(i=0;i<16;i+=4)
  {
      Print(L"|   %02x ~ %02x  ",i+3,i); 
  }
  Print(L"|\n"); 
  
  for(i=0;i<16;i++)
  {
      Print(L"|  %02x  ",i); 
      for(j=0;j<16;j+=4)
      {
          Tmp=PciRead8(CND003_SATA|(i*16+j));
          Print(L"|  %08x  ",Tmp); 
      }
      Print(L"|\n");
  }



  PciWrite8((CND003_SATA|0xB4), PciRead8(CND003_SATA|0xB4)|BIT0);


  Print(L"[lex]:Dump After Reset\n");
  Print(L"|      "); 
  for(i=0;i<16;i+=4)
  {
      Print(L"|   %02x ~ %02x  ",i+3,i); 
  }
  Print(L"|\n"); 
  
  for(i=0;i<16;i++)
  {
      Print(L"|  %02x  ",i); 
      for(j=0;j<16;j+=4)
      {
          Tmp=PciRead8(CND003_SATA|(i*16+j));
          Print(L"|  %08x  ",Tmp); 
      }
      Print(L"|\n");
  }
#endif
  
  return EFI_SUCCESS;
}
