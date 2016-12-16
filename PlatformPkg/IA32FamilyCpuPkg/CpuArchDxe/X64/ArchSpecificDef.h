//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file

  Definition for EM64T processor

  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  ArchSpecificDef.h

**/

#ifndef _ARCH_SPECIFIC_DEF_H_
#define _ARCH_SPECIFIC_DEF_H_

#include <Protocol/Cpu.h>

#pragma pack(1)

typedef struct {
  UINT16    Offset15To0;
  UINT16    SegmentSelector;
  UINT16    Attributes;
  UINT16    Offset31To16;
  UINT32    Offset63To32;
  UINT32    Reserved;
} INTERRUPT_GATE_DESCRIPTOR;

#pragma pack()

#endif
