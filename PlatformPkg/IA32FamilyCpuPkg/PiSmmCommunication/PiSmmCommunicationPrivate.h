//
// This file contains an 'Intel Peripheral Driver' and is
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may
// be modified by the user, subject to additional terms of the
// license agreement
//
/** @file

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:
  
   PiSmmCommunicationPrivate.h

Abstract:

  PiSmmCommunication private data structure

**/

#ifndef _SMM_COMMUNICATION_PRIVATE_H_
#define _SMM_COMMUNICATION_PRIVATE_H_

#pragma pack(push, 1)

#define SMM_COMMUNICATION_SIGNATURE SIGNATURE_32 ('S','M','M','C')

typedef struct {
  UINT32                   Signature;
  UINT32                   SwSmiNumber;
  EFI_PHYSICAL_ADDRESS     AcpiTableAddress;
} EFI_SMM_COMMUNICATION_CONTEXT;

#pragma pack(pop)

#endif
