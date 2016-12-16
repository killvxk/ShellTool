//
// This file contains 'Framework Code' and is licensed as such 
// under the terms of your license agreement with Intel or your
// vendor.  This file may not be modified, except as allowed by
// additional terms of your license agreement.                 
//
/**@file
Constants and declarations that are common accross PEI and DXE.
  
Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#ifndef _LEGACY_DEV_ORDER_H_
#define _LEGACY_DEV_ORDER_H_

///
/// Name and Guid of a NV Variable which stores the information about the
/// FD/HD/CD/NET/BEV order
///
#define EFI_LEGACY_DEV_ORDER_VARIABLE_GUID \
  { \
  0xa56074db, 0x65fe, 0x45f7, {0xbd, 0x21, 0x2d, 0x2b, 0xdd, 0x8e, 0x96, 0x52} \
  }

typedef UINT8 BBS_TYPE;

#pragma pack(1)
typedef struct {
  BBS_TYPE  BbsType;
  ///
  /// Length = sizeof (UINT16) + sizeof (Data)
  ///
  UINT16    Length;
  UINT16    Data[1];
} LEGACY_DEV_ORDER_ENTRY;
#pragma pack()

#define VAR_LEGACY_DEV_ORDER L"LegacyDevOrder"

extern EFI_GUID gEfiLegacyDevOrderVariableGuid;

#endif
