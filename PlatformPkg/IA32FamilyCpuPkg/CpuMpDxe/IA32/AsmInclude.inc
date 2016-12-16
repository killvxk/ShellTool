;
; This file contains an 'Intel Peripheral Driver' and is      
; licensed for Intel CPUs and chipsets under the terms of your
; license agreement with Intel or your vendor.  This file may 
; be modified by the user, subject to additional terms of the 
; license agreement                                           
;
;******************************************************************************
;*
;*        Copyright (c) 2000 - 2011, Intel Corporation. All rights reserved.<BR>
;*
;*    This software and associated documenation (if any) is furnished
;*    under a license and may only be used or copied in accordance
;*    with the terms of the license. Except as permitted by such
;*    license, no part of this software or documentation may be
;*    reproduced, stored in a retrieval system, or transmitted in any
;*    form or by any means without the express written consent of
;*    Intel Corporation.
;*
;******************************************************************************

VacantFlag          Equ   00h
NotVacantFlag       Equ   0ffh
StartupApSignal     Equ   6E750000h
MonitorFilterSize   Equ   10h
ApInHltLoop         Equ   1
ApInMwaitLoop       Equ   2
ApInRunLoop         Equ   3

LockLocation        equ        (RendezvousFunnelProcEnd - RendezvousFunnelProcStart)
StackStart          equ        (LockLocation + 4h)
StackSize           equ        (LockLocation + 8h)
RendezvousProc      equ        (LockLocation + 0Ch)
GdtrProfile         equ        (LockLocation + 10h)
IdtrProfile         equ        (LockLocation + 16h)
BufferStart         equ        (LockLocation + 1Ch)
Cr3Location         equ        (LockLocation + 20h)
InitFlag            equ        (LockLocation + 24h)
ApCountLocation     equ        (LockLocation + 28h)
;DcuModeSelectFlag   equ        (LockLocation + 2Ch)
;DcuMode             equ        (LockLocation + 2Dh)
;ApLoopModeLocation  equ        (LockLocation + 30h)
;BistBuffer          equ        (LockLocation + 34h)
ApLoopModeLocation  equ        (LockLocation + 2Ch)
BistBuffer          equ        (LockLocation + 30h)

PAUSE32   MACRO
            DB      0F3h
            DB      090h
            ENDM

;-------------------------------------------------------------------------------
