/** @file
Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

Module Name:

  LegacyBda.c

Abstract:

  This code fills in BDA (0x400) and EBDA (pointed to by 0x4xx)
  information. There is support for doing initializeation before
  Legacy16 is loaded and before a legacy boot is attempted.

Revision History:

  Bug 1989:   Changed to use dynamic software SMI value instead of hard coding
  TIME:       2011-6-15
  $AUTHOR:    Peng Xianbing
  $REVIEWERS:
  $SCOPE:     Allocate 1k memory for int13 use.
  $TECHNICAL:
  $END-------------------------------------------------------------------------

  Bug 2204:   Created Byosoft specific package "ByoModulePkg" to
              include Byosoft specific functionality
  TIME:       2011-5-30
  $AUTHOR:    Chen Daolin
  $REVIEWERS:
  $SCOPE:     Sugar Bay and Tunnel Customer Refernce Board
  $TECHNICAL:
    1. ByoModulePkg is created to include the features as follows:
         USB (Legacy/Native/Crisis Recovery)
         SD (Legacy/Native)
         Security including HDD password, TCG/TPM, TCM and Pre-boot
           Authentication
         Setup Browser (Text/Graphic mode)
         Debugging
         Binary Image (CSM16/AHCI Option Rom)
    2. Both of DSC and DEC in Sugar Bay and Tunnel Creek are updated
       to refer to  the new location of drivers in ByoModulePkg.
    3. Reserved 630-640k for the usage of BIOS stack in int 13h handler.
       Previously this stack is allocated via EBDA. Sometimes this stack
       could be corrupted by OptionRom image's code.
  $END------------------------------------------------------------------
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LegacyBiosInterface.h"

/**
  Fill in the standard BDA and EBDA stuff before Legacy16 load

  @param  Private     Legacy BIOS Instance data

  @retval EFI_SUCCESS It should always work.

**/
EFI_STATUS
LegacyBiosInitBda (
  IN   LEGACY_BIOS_INSTANCE    *Private,
  OUT  EFI_PHYSICAL_ADDRESS    *LegacyMemTop                
  )
{
  BDA_STRUC             *Bda;
  UINT8                 *Ebda;
  UINT16                *Int13Stack;
  UINTN                 HoleSize;	
  EFI_STATUS            Status;	
  EFI_PHYSICAL_ADDRESS  PhyAddr;
  UINT32                Address;	
  UINT32                MemorySize;
  EFI_PHYSICAL_ADDRESS  MemoryAddress;
  UINT32                *ClearPtr;


// Allocate 0 - 4K for real mode interupt vectors and BDA.
  Status = AllocateLegacyMemory (
             AllocateAddress,
             0,
             1,
             &MemoryAddress
             );
  ASSERT_EFI_ERROR (Status);

  ClearPtr = (VOID *) ((UINTN) 0x0000);
  gBS->SetMem ((VOID *) ClearPtr, 0x400, INITIAL_VALUE_BELOW_1K);
  ZeroMem ((VOID *) ((UINTN)ClearPtr + 0x400), 0xC00);


  Bda = (BDA_STRUC *) ((UINTN) 0x400);

  // 640k-4k (0x9F000) to 640k   (0x9FFFF) is reserved for S3 resume (CpuMp)
  // 640k-5k (0x9ec00) to 640-4k (0x9EFFF) is IDE string
  // 640k-6k (0x9e800) to 640-5k (0x9eBFF) is int13 stack
  // 640k-7k (0x9e400) to 640-6k (0x9e7FF) is EBDA header	
  HoleSize = SIZE_1KB + SIZE_1KB + SIZE_1KB + PcdGet16(PcdCpuS3ApVectorMaxSize);
  Ebda     = (UINT8*)(UINTN)(0xA0000 - HoleSize);	
  PhyAddr  = ((UINTN)Ebda)&(~0xFFF);
  DEBUG((EFI_D_INFO, "EBDA:%X,%X PhyAddr:%lX\n", Ebda, HoleSize, PhyAddr));	

  Status = gBS->AllocatePages (
                  AllocateAddress,
                  EfiReservedMemoryType,
                  EFI_SIZE_TO_PAGES(HoleSize),
                  &PhyAddr
                  );
  ASSERT(!EFI_ERROR(Status));	
  DEBUG((EFI_D_ERROR,"mike_PhyAddr=%llx\n",PhyAddr));
  Address = 0xA0000 - PcdGet16(PcdCpuS3ApVectorMaxSize);
  // PCD : defined in IntelFrameworkModulePkg.dec
  PcdSet32(PcdCpuS3ApVectorAddress, Address);
  Address -= SIZE_1KB;	
  gIdeDevBbsNameStrAddress = Address;


// Allocate pages for OPROM usage
  MemorySize = PcdGet32 (PcdEbdaReservedMemorySize);
  DEBUG((EFI_D_ERROR,"mike_MemorySize=%x\n",MemorySize));
  ASSERT ((MemorySize & 0xFFF) == 0);
  MemoryAddress = PhyAddr - MemorySize;
  DEBUG((EFI_D_ERROR,"mike_MemoryAddress_bf=%llx\n",MemoryAddress));
  Status = gBS->AllocatePages (
                  AllocateAddress,
                  EfiReservedMemoryType,
                  EFI_SIZE_TO_PAGES(MemorySize),
                  &MemoryAddress
                  );
  ASSERT_EFI_ERROR (Status);
  DEBUG((EFI_D_ERROR,"mike_MemoryAddress_af=%llx\n",MemoryAddress));
  ZeroMem ((VOID*)(UINTN)MemoryAddress, MemorySize);

  if(LegacyMemTop!=NULL){
    *LegacyMemTop = MemoryAddress;
  }	

  ZeroMem (Bda, 0x100);
  ZeroMem (Ebda, 0x400);

  Bda->MemSize        = (UINT16)(((UINTN)Ebda)/SIZE_1KB);
  Bda->KeyHead        = 0x1e;
  Bda->KeyTail        = 0x1e;
  Bda->FloppyData     = 0x00;
  Bda->FloppyTimeout  = 0xff;

  Bda->KeyStart       = 0x001E;
  Bda->KeyEnd         = 0x003E;
  Bda->KeyboardStatus = 0x10;
  Bda->Ebda           = (UINT16)((UINTN)Ebda >> 4);

  //
  // Move LPT time out here and zero out LPT4 since some SCSI OPROMS
  // use this as scratch pad (LPT4 is Reserved)
  //
  Bda->Lpt1_2Timeout  = 0x1414;
  Bda->Lpt3_4Timeout  = 0x1400;

  *Ebda               = 0x01;
  Int13Stack          = (UINT16 *) ((UINTN) Ebda + 0x1ca); // Offset 0x1CA EBDA
  *Int13Stack         = Bda->Ebda + 0x40;                  // 0x40 skip EBDA segment

  return EFI_SUCCESS;
}
