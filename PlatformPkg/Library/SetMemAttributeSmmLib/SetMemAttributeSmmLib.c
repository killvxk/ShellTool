/** @file

Copyright (c) 2006 - 2015, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SetMemAttributeSmmLib.c

Abstract:


Revision History:

**/

#include <PlatformDefinition.h>

/*
(0,0,3,82)  0(dis)  1(W) 2(R) 3(RW)
  [7:6] - EC000 - EFFFF
  [5:4] - E8000 - EBFFF
  [3:2] - E4000 - E7FFF
  [1:0] - E0000 - E3FFF

(0,0,3,83)[5:4] F0000 - FFFFF


(0,11,7,63)[7:6] - E0000 - EFFFF
(0,11,7,63)[5:4] - F0000 - FFFFF

*/

VOID
EnableMemAccess(
  OUT UINT8     *FsegValue8,
  OUT UINT8     *EsegValue8
  )
{
  *FsegValue8 = MmioRead8(HIF_PCI_REG(PAGE_F_SHADOW_MH_SD_REG)); // D0F2_RxA3
  MmioOr8(HIF_PCI_REG(PAGE_F_SHADOW_MH_SD_REG), MEM_F0000_64K_RW_EN);

  *EsegValue8 = MmioRead8(HIF_PCI_REG (PAGE_E_SHADOW_CTRL_REG));
  //MmioOr8(HIF_PCI_REG(PAGE_E_SHADOW_CTRL_REG),(UINT8)(UINTN)(MEM_E0000_4K_RW_EN|MEM_E4000_4K_RW_EN|MEM_E8000_4K_RW_EN|MEM_EC000_4K_RW_EN));
  MmioOr8(HIF_PCI_REG(PAGE_E_SHADOW_CTRL_REG),0xFF);
}

VOID
DisableMemAccess(
  IN UINT8        FsegValue8,
  IN UINT8        EsegValue8
  )
{
  MmioWrite8(HIF_PCI_REG (PAGE_F_SHADOW_MH_SD_REG), FsegValue8);
  MmioWrite8(HIF_PCI_REG (PAGE_E_SHADOW_CTRL_REG), EsegValue8);
}
