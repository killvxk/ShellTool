/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2009 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:
  UchiDebug.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "Uhci.h"

/**
  Dump the content of QH structure.

  @param  QhSw    Pointer to software QH structure.

**/
VOID
UhciDumpQh (
    IN UHCI_QH_SW    *QhSw
)
{
    UHCI_DEBUG((EFI_D_INFO, "&QhSw @ 0x%p\n", QhSw));
    UHCI_DEBUG((EFI_D_INFO, "QhSw.NextQh    - 0x%p\n", QhSw->NextQh));
    UHCI_DEBUG((EFI_D_INFO, "QhSw.TDs       - 0x%p\n", QhSw->TDs));
    UHCI_DEBUG((EFI_D_INFO, "QhSw.QhHw:\n"));
    UHCI_DEBUG((EFI_D_INFO, " Horizon  Link - %x\n", QhSw->QhHw.HorizonLink));
    UHCI_DEBUG((EFI_D_INFO, " Vertical Link - %x\n\n", QhSw->QhHw.VerticalLink));
}


/**
  Dump the content of TD structure.

  @param  TdSw    Pointer to software TD structure.

**/
VOID
UhciDumpTds (
    IN UHCI_TD_SW           *TdSw
)
{
    UHCI_TD_SW              *CurTdSw;

    CurTdSw = TdSw;

    while (CurTdSw != NULL) {
        UHCI_DEBUG((EFI_D_INFO, "TdSw @ 0x%p\n",           CurTdSw));
        UHCI_DEBUG((EFI_D_INFO, "TdSw.NextTd   - 0x%p\n",  CurTdSw->NextTd));
        UHCI_DEBUG((EFI_D_INFO, "TdSw.DataLen  - %d\n",    CurTdSw->DataLen));
        UHCI_DEBUG((EFI_D_INFO, "TdSw.Data     - 0x%p\n",  CurTdSw->Data));
        UHCI_DEBUG((EFI_D_INFO, "TdHw:\n"));
        UHCI_DEBUG((EFI_D_INFO, " NextLink     - 0x%x\n",  CurTdSw->TdHw.NextLink));
        UHCI_DEBUG((EFI_D_INFO, " ActualLen    - %d\n",    CurTdSw->TdHw.ActualLen));
        UHCI_DEBUG((EFI_D_INFO, " Status       - 0x%x\n",  CurTdSw->TdHw.Status));
        UHCI_DEBUG((EFI_D_INFO, " IOC          - %d\n",    CurTdSw->TdHw.IntOnCpl));
        UHCI_DEBUG((EFI_D_INFO, " IsIsoCh      - %d\n",    CurTdSw->TdHw.IsIsoch));
        UHCI_DEBUG((EFI_D_INFO, " LowSpeed     - %d\n",    CurTdSw->TdHw.LowSpeed));
        UHCI_DEBUG((EFI_D_INFO, " ErrorCount   - %d\n",    CurTdSw->TdHw.ErrorCount));
        UHCI_DEBUG((EFI_D_INFO, " ShortPacket  - %d\n",    CurTdSw->TdHw.ShortPacket));
        UHCI_DEBUG((EFI_D_INFO, " PidCode      - 0x%x\n",  CurTdSw->TdHw.PidCode));
        UHCI_DEBUG((EFI_D_INFO, " DevAddr      - %d\n",    CurTdSw->TdHw.DeviceAddr));
        UHCI_DEBUG((EFI_D_INFO, " EndPoint     - %d\n",    CurTdSw->TdHw.EndPoint));
        UHCI_DEBUG((EFI_D_INFO, " DataToggle   - %d\n",    CurTdSw->TdHw.DataToggle));
        UHCI_DEBUG((EFI_D_INFO, " MaxPacketLen - %d\n",    CurTdSw->TdHw.MaxPacketLen));
        UHCI_DEBUG((EFI_D_INFO, " DataBuffer   - 0x%x\n\n",CurTdSw->TdHw.DataBuffer));

        CurTdSw = CurTdSw->NextTd;
    }
}

