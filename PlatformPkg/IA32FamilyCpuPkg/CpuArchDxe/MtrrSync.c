/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file

  Code for MTRR synchronzation.

  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  MtrrSync.c

**/

#include "Cpu.h"

MTRR_SETTINGS   *mMtrrTable;

/**
  Initialize memory region for MTRR data.
  
  This function allocates ACPI NVS memory for MTRR data.
  Each time MTRRs are written, this memory region will be updated accordingly.

**/
VOID
InitializeMtrrData (
  VOID
  )
{
  //
  // Allocate memory for fixed MTRRs, variable MTRRs and MTRR_DEF_TYPE
  //
  mMtrrTable = AllocatePool(sizeof(MTRR_SETTINGS));
  PcdSet64 (PcdCpuMtrrTableAddress, (UINT64) (UINTN) mMtrrTable);
}

/**
  Synchronzies up the MTRR values with BSP for calling processor.

  This function synchronzies up the MTRR values with BSP for calling processor.
**/
VOID
EFIAPI
LoadMtrrData (
  VOID    *Buffer
  )
{
  MtrrSetAllMtrrs (mMtrrTable);
}

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.
  
  @return Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN   Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}
