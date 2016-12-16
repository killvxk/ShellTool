/** @file

Copyright (c) 2006 - 2015, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SetMemAttributeSmmLib.h

Abstract:

Revision History:


**/

#ifndef __SET_MEM_ATTRIBUTE_LIB_H_
#define __SET_MEM_ATTRIBUTE_LIB_H_

VOID
EnableMemAccess(
  OUT UINT8     * FsegValue8,
  OUT UINT8     * EsegValue8
);

VOID
DisableMemAccess(
  IN UINT8        FsegValue8,
  IN UINT8        EsegValue8
);


#endif