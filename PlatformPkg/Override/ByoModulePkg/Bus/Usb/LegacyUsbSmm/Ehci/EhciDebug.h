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
  EhciDebug.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EFI_EHCI_DEBUG_H_
#define _EFI_EHCI_DEBUG_H_

#include <Library/DebugLib.h>

#define EHCI_DEBUG(a)   //DEBUG(a)

/**
  Dump the fields of a QTD.

  @param  Qtd      The QTD to dump.
  @param  Msg      The message to print before the dump.

**/
VOID
EhcDumpQtd (
    IN EHC_QTD              *Qtd,
    IN UINT8                *Msg
);


/**
  Dump the queue head.

  @param  Qh       The queue head to dump.
  @param  Msg      The message to print before the dump.
  @param  DumpBuf  Whether to dump the memory buffer of the associated QTD.

**/
VOID
EhcDumpQh (
    IN EHC_QH               *Qh,
    IN UINT8                *Msg,
    IN BOOLEAN              DumpBuf
);

/**
  Dump the buffer in the form of hex.

  @param  Buf      The buffer to dump.
  @param  Len      The length of buffer.

**/
VOID
EhcDumpBuf (
    IN UINT8                *Buf,
    IN UINTN                Len
);


/**
  Dump the EHCI status registers.

  @param  Ehc    USB EHCI Host Controller instance

**/
VOID
EhcDumpRegs (
    IN  USB2_HC_DEV         *Ehc
);

#endif
