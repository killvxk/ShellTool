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
    EhciDebug.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "Ehci.h"

/**
  Dump the status byte in QTD/QH to a more friendly format.

  @param  State    The state in the QTD/QH.

**/
VOID
EhcDumpStatus (
    IN UINT32               State
)
{
    if (EHC_BIT_IS_SET (State, QTD_STAT_DO_PING)) {
        EHCI_DEBUG ((EFI_D_INFO, "  Do_Ping"));
    } else {
        EHCI_DEBUG ((EFI_D_INFO, "  Do_Out"));
    }

    if (EHC_BIT_IS_SET (State, QTD_STAT_DO_CS)) {
        EHCI_DEBUG ((EFI_D_INFO, "  Do_CS"));
    } else {
        EHCI_DEBUG ((EFI_D_INFO, "  Do_SS"));
    }

    if (EHC_BIT_IS_SET (State, QTD_STAT_TRANS_ERR)) {
        EHCI_DEBUG ((EFI_D_INFO, "  Transfer_Error"));
    }

    if (EHC_BIT_IS_SET (State, QTD_STAT_BABBLE_ERR)) {
        EHCI_DEBUG ((EFI_D_INFO, "  Babble_Error"));
    }

    if (EHC_BIT_IS_SET (State, QTD_STAT_BUFF_ERR)) {
        EHCI_DEBUG ((EFI_D_INFO, "  Buffer_Error"));
    }

    if (EHC_BIT_IS_SET (State, QTD_STAT_HALTED)) {
        EHCI_DEBUG ((EFI_D_INFO, "  Halted"));
    }

    if (EHC_BIT_IS_SET (State, QTD_STAT_ACTIVE)) {
        EHCI_DEBUG ((EFI_D_INFO, "  Active"));
    }

    EHCI_DEBUG ((EFI_D_INFO, "\n"));
}


/**
  Dump the fields of a QTD.

  @param  Qtd      The QTD to dump.
  @param  Msg      The message to print before the dump.

**/
VOID
EhcDumpQtd (
    IN EHC_QTD              *Qtd,
    IN UINT8                *Msg
)
{
    QTD_HW                  *QtdHw;
    UINTN                   Index;

    if (Msg != NULL) {
        EHCI_DEBUG ((EFI_D_INFO, Msg));
    }

    EHCI_DEBUG ((EFI_D_INFO, "Queue TD @ 0x%p, data length %d\n", Qtd, (UINT32)Qtd->DataLen));

    QtdHw = &Qtd->QtdHw;

    EHCI_DEBUG ((EFI_D_INFO, "Next QTD     : %x\n", QtdHw->NextQtd));
    EHCI_DEBUG ((EFI_D_INFO, "AltNext QTD  : %x\n", QtdHw->AltNext));
    EHCI_DEBUG ((EFI_D_INFO, "Status       : %x\n", QtdHw->Status));
    EhcDumpStatus (QtdHw->Status);

    if (QtdHw->Pid == QTD_PID_SETUP) {
        EHCI_DEBUG ((EFI_D_INFO, "PID          : Setup\n"));

    } else if (QtdHw->Pid == QTD_PID_INPUT) {
        EHCI_DEBUG ((EFI_D_INFO, "PID          : IN\n"));

    } else if (QtdHw->Pid == QTD_PID_OUTPUT) {
        EHCI_DEBUG ((EFI_D_INFO, "PID          : OUT\n"));

    }

    EHCI_DEBUG ((EFI_D_INFO, "Error Count  : %d\n", QtdHw->ErrCnt));
    EHCI_DEBUG ((EFI_D_INFO, "Current Page : %d\n", QtdHw->CurPage));
    EHCI_DEBUG ((EFI_D_INFO, "IOC          : %d\n", QtdHw->IOC));
    EHCI_DEBUG ((EFI_D_INFO, "Total Bytes  : %d\n", QtdHw->TotalBytes));
    EHCI_DEBUG ((EFI_D_INFO, "Data Toggle  : %d\n", QtdHw->DataToggle));

    for (Index = 0; Index < 5; Index++) {
        EHCI_DEBUG ((EFI_D_INFO, "Page[%d]      : 0x%x\n", (UINT32)Index, QtdHw->Page[Index]));
    }
}


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
)
{
    EHC_QTD                 *Qtd;
    QH_HW                   *QhHw;
    LIST_ENTRY              *Entry;
    UINTN                   Index;

    if (Msg != NULL) {
        EHCI_DEBUG ((EFI_D_INFO, Msg));
    }

    EHCI_DEBUG ((EFI_D_INFO, "Queue head @ 0x%p, interval %ld, next qh %p\n",
            Qh, (UINT64)Qh->Interval, Qh->NextQh));

    QhHw = &Qh->QhHw;

    EHCI_DEBUG ((EFI_D_INFO, "Hoziontal link: %x\n", QhHw->HorizonLink));
    EHCI_DEBUG ((EFI_D_INFO, "Device address: %d\n", QhHw->DeviceAddr));
    EHCI_DEBUG ((EFI_D_INFO, "Inactive      : %d\n", QhHw->Inactive));
    EHCI_DEBUG ((EFI_D_INFO, "EP number     : %d\n", QhHw->EpNum));
    EHCI_DEBUG ((EFI_D_INFO, "EP speed      : %d\n", QhHw->EpSpeed));
    EHCI_DEBUG ((EFI_D_INFO, "DT control    : %d\n", QhHw->DtCtrl));
    EHCI_DEBUG ((EFI_D_INFO, "Reclaim head  : %d\n", QhHw->ReclaimHead));
    EHCI_DEBUG ((EFI_D_INFO, "Max packet len: %d\n", QhHw->MaxPacketLen));
    EHCI_DEBUG ((EFI_D_INFO, "Ctrl EP       : %d\n", QhHw->CtrlEp));
    EHCI_DEBUG ((EFI_D_INFO, "Nak reload    : %d\n", QhHw->NakReload));

    EHCI_DEBUG ((EFI_D_INFO, "SMask         : %x\n", QhHw->SMask));
    EHCI_DEBUG ((EFI_D_INFO, "CMask         : %x\n", QhHw->CMask));
    EHCI_DEBUG ((EFI_D_INFO, "Hub address   : %d\n", QhHw->HubAddr));
    EHCI_DEBUG ((EFI_D_INFO, "Hub port      : %d\n", QhHw->PortNum));
    EHCI_DEBUG ((EFI_D_INFO, "Multiplier    : %d\n", QhHw->Multiplier));

    EHCI_DEBUG ((EFI_D_INFO, "Cur QTD       : %x\n", QhHw->CurQtd));

    EHCI_DEBUG ((EFI_D_INFO, "Next QTD      : %x\n", QhHw->NextQtd));
    EHCI_DEBUG ((EFI_D_INFO, "AltNext QTD   : %x\n", QhHw->AltQtd));
    EHCI_DEBUG ((EFI_D_INFO, "Status        : %x\n", QhHw->Status));

    EhcDumpStatus (QhHw->Status);

    if (QhHw->Pid == QTD_PID_SETUP) {
        EHCI_DEBUG ((EFI_D_INFO, "PID           : Setup\n"));

    } else if (QhHw->Pid == QTD_PID_INPUT) {
        EHCI_DEBUG ((EFI_D_INFO, "PID           : IN\n"));

    } else if (QhHw->Pid == QTD_PID_OUTPUT) {
        EHCI_DEBUG ((EFI_D_INFO, "PID           : OUT\n"));
    }

    EHCI_DEBUG ((EFI_D_INFO, "Error Count   : %d\n", QhHw->ErrCnt));
    EHCI_DEBUG ((EFI_D_INFO, "Current Page  : %d\n", QhHw->CurPage));
    EHCI_DEBUG ((EFI_D_INFO, "IOC           : %d\n", QhHw->IOC));
    EHCI_DEBUG ((EFI_D_INFO, "Total Bytes   : %d\n", QhHw->TotalBytes));
    EHCI_DEBUG ((EFI_D_INFO, "Data Toggle   : %d\n", QhHw->DataToggle));

    for (Index = 0; Index < 5; Index++) {
        EHCI_DEBUG ((EFI_D_INFO, "Page[%d]       : 0x%x\n", Index, QhHw->Page[Index]));
    }

    EHCI_DEBUG ((EFI_D_INFO, "\n"));

    EFI_LIST_FOR_EACH (Entry, &Qh->Qtds) {
        Qtd = EFI_LIST_CONTAINER (Entry, EHC_QTD, QtdList);
        EhcDumpQtd (Qtd, NULL);

        if (DumpBuf && (Qtd->DataLen != 0)) {
            EhcDumpBuf (Qtd->Data, Qtd->DataLen);
        }
    }
}


/**
  Dump the buffer in the form of hex.

  @param  Buf      The buffer to dump.
  @param  Len      The length of buffer.

**/
VOID
EhcDumpBuf (
    IN UINT8                *Buf,
    IN UINTN                Len
)
{
    UINTN                   Index;

    for (Index = 0; Index < Len; Index++) {
        if (Index % 16 == 0) {
            EHCI_DEBUG ((EFI_D_INFO,"\n"));
        }

        EHCI_DEBUG ((EFI_D_INFO, "%02x ", Buf[Index]));
    }

    EHCI_DEBUG ((EFI_D_INFO, "\n"));
}

/**
  Dump the EHCI status registers.

  @param  Ehc    USB EHCI Host Controller instance

**/
VOID
EhcDumpRegs (
    IN  USB2_HC_DEV         *Ehc
)
{
    UINT8   Index;

    EHCI_DEBUG ((EFI_D_INFO, "  EHC_CAPLENGTH_OFFSET   = 0x%08x\n", EhcReadCapRegister (Ehc, EHC_CAPLENGTH_OFFSET)));
    EHCI_DEBUG ((EFI_D_INFO, "  EHC_HCSPARAMS_OFFSET   = 0x%08x\n", EhcReadCapRegister (Ehc, EHC_HCSPARAMS_OFFSET)));
    EHCI_DEBUG ((EFI_D_INFO, "  EHC_HCCPARAMS_OFFSET   = 0x%08x\n", EhcReadCapRegister (Ehc, EHC_HCCPARAMS_OFFSET)));
    EHCI_DEBUG ((EFI_D_INFO, "  EHC_USBCMD_OFFSET      = 0x%08x\n", EhcReadOpReg (Ehc, EHC_USBCMD_OFFSET)));
    EHCI_DEBUG ((EFI_D_INFO, "  EHC_USBSTS_OFFSET      = 0x%08x\n", EhcReadOpReg (Ehc, EHC_USBSTS_OFFSET)));
    EHCI_DEBUG ((EFI_D_INFO, "  EHC_USBINTR_OFFSET     = 0x%08x\n", EhcReadOpReg (Ehc, EHC_USBINTR_OFFSET)));
    EHCI_DEBUG ((EFI_D_INFO, "  EHC_FRINDEX_OFFSET     = 0x%08x\n", EhcReadOpReg (Ehc, EHC_FRINDEX_OFFSET)));
    EHCI_DEBUG ((EFI_D_INFO, "  EHC_CTRLDSSEG_OFFSET   = 0x%08x\n", EhcReadOpReg (Ehc,  EHC_CTRLDSSEG_OFFSET)));
    EHCI_DEBUG ((EFI_D_INFO, "  EHC_FRAME_BASE_OFFSET  = 0x%08x\n", EhcReadOpReg (Ehc,  EHC_FRAME_BASE_OFFSET)));
    EHCI_DEBUG ((EFI_D_INFO, "  EHC_ASYNC_HEAD_OFFSET  = 0x%08x\n", EhcReadOpReg (Ehc, EHC_ASYNC_HEAD_OFFSET)));
    EHCI_DEBUG ((EFI_D_INFO, "  EHC_CONFIG_FLAG_OFFSET = 0x%08x\n", EhcReadOpReg (Ehc, EHC_CONFIG_FLAG_OFFSET)));
    for (Index = 0; Index < (UINT8) (Ehc->HcStructParams & HCSP_NPORTS); Index++) {
        EHCI_DEBUG ((EFI_D_INFO, "  EHC_PORT_STAT_OFFSET(%d)  = 0x%08x\n", Index, EhcReadOpReg (Ehc, EHC_PORT_STAT_OFFSET + (4 * Index))));
    }
}
