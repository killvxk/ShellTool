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
  UchiDebug.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_UHCI_DEBUG_H_
#define _EFI_UHCI_DEBUG_H_

#define UHCI_DEBUG(a)  //DEBUG(a)
/**
  Dump the content of QH structure.

  @param  QhSw    Pointer to software QH structure.

  @return None.

**/
VOID
UhciDumpQh (
    IN UHCI_QH_SW         *QhSw
);
/**
  Dump the content of TD structure.

  @param  TdSw    Pointer to software TD structure.

  @return None.

**/
VOID
UhciDumpTds (
    IN UHCI_TD_SW           *TdSw
);

#endif
