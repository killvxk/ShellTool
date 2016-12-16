//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  ProcessorDef.h

Abstract:

  Definition for IA32 processor

**/

#ifndef _PROCESSOR_DEF_H_
#define _PROCESSOR_DEF_H_

#include <FrameworkDxe.h>
#include <Protocol/Cpu.h>

#pragma pack(1)

typedef struct {
  UINT16  OffsetLow;
  UINT16  SegmentSelector;
  UINT16  Attributes;
  UINT16  OffsetHigh;
} INTERRUPT_GATE_DESCRIPTOR;

#pragma pack()

typedef struct {
  VOID  *Start;
  UINTN Size;
  UINTN FixOffset;
} INTERRUPT_HANDLER_TEMPLATE_MAP;

typedef struct {
  UINT8 *RendezvousFunnelAddress;
  UINTN PModeEntryOffset;
  UINTN FlatJumpOffset;
  UINTN Size;
} MP_ASSEMBLY_ADDRESS_MAP;

/**
  Get starting address and size of the rendezvous entry for APs.
  Information for fixing a jump instruction in the code is also returned.

  @param AddressMap  Output buffer for address map information.
**/
VOID
EFIAPI
AsmGetAddressMap (
  OUT MP_ASSEMBLY_ADDRESS_MAP    *AddressMap
  );

#endif
