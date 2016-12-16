/*++
Copyright (c) 2010 Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  CpuSetup.c

Abstract:

  CPU Setup Routines.
  
Revision History:

--*/

#include <PlatformSetup.h>
#include <AsiaCpuProtocol.h>


STATIC ACPU_MICROCODE_UPDATE_HEADER  gDummyCpuMicroCodeHdr;

VOID 
InitCpuStrings(
  EFI_HII_HANDLE    HiiHandle, 
  UINT16            Class
)
/*++

Routine Description:

  This function initializes the Acpi related setup option values

Arguments:

  HiiHandle - HiiHandle Handle to HII database
  Class     - Indicates the setup class

Returns:

  VOID

--*/
{
  UINT32                        Cpuid;
  UINT32                        McRev;
  ACPU_MICROCODE_UPDATE_HEADER  *CpuMc; 
  UINT32                        McAddress;	
  
  DEBUG((EFI_D_ERROR, "<InitCpuStrings>"));

  if (Class == ADVANCED_FORM_SET_CLASS) {

    AsmWriteMsr64(0x8B, 0);
    AsmCpuid(1, &Cpuid, NULL, NULL, NULL);
    McRev = (UINT32)RShiftU64(AsmReadMsr64(0x8B), 32);

    McAddress = PcdGet32(PcdCpuMicroCodeAddress);
    if(McAddress){		
      CpuMc = ((ACPU_MICROCODE_UPDATE_HEADER*)(UINTN)McAddress)-1;
    } else {
      ZeroMem(&gDummyCpuMicroCodeHdr, sizeof(gDummyCpuMicroCodeHdr));
      CpuMc = &gDummyCpuMicroCodeHdr;
    }    
    
    InitString (
      HiiHandle,
      STRING_TOKEN(STR_CPUID_VALUE), 
      L"%X", 
      Cpuid
      );
    InitString (
      HiiHandle,
      STRING_TOKEN(STR_CPU_MICROCODE_REV_VALUE), 
      L"%08X(%02d/%02d/%04d)", 
      McRev,
      CpuMc->Month,
      CpuMc->Day,
      CpuMc->Year
      );      
  }
}