//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file

  Include file for MTRR synchronzation.

  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  MtrrSync.h

**/

#ifndef _EFI_MTRR_SYNC_H_
#define _EFI_MTRR_SYNC_H_

#include "Cpu.h"

extern MTRR_SETTINGS   *mMtrrTable;

/**
  Initialize memory region for MTRR data.
  
  This function allocates ACPI NVS memory for MTRR data, and fills the region
  with current MTRR data. Each time MTRRs are written, this memory region
  will be updated accordingly.

**/
VOID
InitializeMtrrData (
  VOID
  );

/**
  Synchronzies up the MTRR values with BSP for calling processor.

  This function synchronzies up the MTRR values with BSP for calling processor.
**/
VOID
EFIAPI
LoadMtrrData (
  VOID    *Buffer
  );

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.
  
  @return Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN   Size
  );
#endif
