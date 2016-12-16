;
; This file contains an 'Intel Peripheral Driver' and is
; licensed for Intel CPUs and chipsets under the terms of your
; license agreement with Intel or your vendor.  This file may
; be modified by the user, subject to additional terms of the
; license agreement
;
;-------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
; This software and associated documentation (if any) is furnished
; under a license and may only be used or copied in accordance
; with the terms of the license. Except as permitted by such
; license, no part of this software or documentation may be
; reproduced, stored in a retrieval system, or transmitted in any
; form or by any means without the express written consent of
; Intel Corporation.

;
;
; Module Name:
;
;   MpFuncs.asm
;
; Abstract:
;
;   This is the assembly code for EM64T MP support
;
;-------------------------------------------------------------------------------


include  AsmInclude.inc
EXTERN  InitializeFloatingPointUnits:PROC
EXTERN  mBspSwitched:BYTE
EXTERN  mNewProcessorNumber:QWORD
EXTERN  mMonitorDataAddress:QWORD

;-------------------------------------------------------------------------------------

;-------------------------------------------------------------------------------------
;RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
;procedure serializes all the AP processors through an Init sequence. It must be
;noted that APs arrive here very raw...ie: real mode, no stack.
;ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
;IS IN MACHINE CODE.
;-------------------------------------------------------------------------------------
;RendezvousFunnelProc (&WakeUpBuffer,MemAddress);

text      SEGMENT

externdef ApProtectedModeThunk:near

RendezvousFunnelProc   PROC  PUBLIC
RendezvousFunnelProcStart::

; At this point CS = 0x(vv00) and ip= 0x0.

        db 66h,  08bh, 0e8h           ; mov        ebp, eax

        db 8ch,  0c8h                 ; mov        ax,  cs
        db 8eh,  0d8h                 ; mov        ds,  ax
        db 8eh,  0c0h                 ; mov        es,  ax
        db 8eh,  0d0h                 ; mov        ss,  ax
        db 33h,  0c0h                 ; xor        ax,  ax
        db 8eh,  0e0h                 ; mov        fs,  ax
        db 8eh,  0e8h                 ; mov        gs,  ax

; Get APIC ID
;YKN-20160728 -S {
;        db 66h,  0B8h
;        dd 00000000h                  ; mov        eax, 0
;        db 0Fh,  0A2h                 ; cpuid
;        db 66h, 3dh
;        dd 0000000Bh                  ; cmp        eax, 11
;        db 73h
;        db X2Apic - $ - 1             ; jnb        X2Apic
;}YKN-20160728 -E 

; Processor is not x2APIC capable, so get 8-bit APIC ID
        db 66h,  0B8h
        dd 00000001h                  ; mov        eax, 1
        db 0Fh,  0A2h                 ; cpuid
        db 66h,  0C1h, 0EBh, 18h      ; shr        ebx, 24
;YKN-20160728 -S {
;        db 0ebh
;        db CheckInitFlag - $ - 1      ; jmp CheckInitFlag

; Processor is x2APIC capable, so get 32-bit x2APIC ID
;X2Apic::
;        db 66h,  0B8h
;        dd 0000000Bh                  ; mov        eax, 0b
;        db 66h, 31h, 0c9h             ; xor        ecx, ecx
;        db 0Fh,  0A2h                 ; cpuid
;        db 66h, 89h, 0d3h             ; mov        ebx, edx

;CheckInitFlag::
;}YKN-20160728 -E 

; If it is the first time AP wakes up, just record AP's BIST
; Otherwise, switch to protected mode.

        db 0BEh
        dw InitFlagLocation           ; mov        si,  InitFlag
        db 66h,  83h, 3Ch, 00h        ; cmp        dword ptr [si], 0
        db 74h
        db flat32Start - $ - 1        ; jz         flat32Start

; Increase ApCount as processor number for index of BIST Info array

        db 66h, 0a1h                  ; mov        eax, [ApCountLocation]
        dw ApCountLocation
IncApCount::
        db 66h, 67h, 8dh, 48h, 01     ; lea        ecx, [eax+1]
        db 0f0h, 66h, 0fh, 0b1h, 0eh  ; lock       cmpxchg [ApCountLocation], ecx
        dw ApCountLocation
        db 75h
        db IncApCount - $ - 1         ; jnz        IncApCount

        db 66h, 0ffh, 0c0h            ; inc        eax                         ; AP processor number starts from 1

; Record BIST information
;
        db 66h, 67h, 8dh, 34h, 0c5h   ; lea esi, [BistBuffer + eax*8]
        dd BistBuffer

        db 66h, 89h, 1ch              ; mov        dword ptr [si], ebx         ; APIC ID
        db 66h,  89h,  6Ch,  04h      ; mov        dword ptr [si + 4], ebp     ; Store BIST value
;YKN-20160728 -S {
;Check Dcu Mode select as early as possible
;        db 0beh
;        dw DcuModeSelectFlagLocation  ; mov si, DcuModeSelect
;        db 80h, 3ch, 00h              ; cmp byte ptr [si], 0
;        db 74h
;        db SkipDcuModeSelect - $ -1   ; je SkipDcuModeSelect

;        db 66h, 0b9h                  ; mov ecx, 0ceh
;        dd 0000000ceh                 ;
;        db 0fh, 32h                   ; rdmsr
;        db 66h, 0fh, 0bah, 0e0h, 26   ; bt eax, 26
;        db 73h
;        db SkipDcuModeSelect - $ - 1  ; jnc SkipDcuModeSelect

;        db 0beh
;        dw DcuModeLocation            ; mov si, DcuMode
;        db 80h, 3ch, 00h              ; cmp byte ptr [si], 0
;        db 74h
;        db SkipDcuModeSelect - $ -1   ; je SkipDcuModeSelect

;        db 66h, 0b9h                  ; mov ecx, 031h
;        dd 000000031h                 ;
;        db 0fh, 32h                   ; rdmsr
;        db 66h, 0dh                   ; or eax, 1
;        dd 000000001h
;        db 0fh, 30h                   ; wrmsr

;SkipDcuModeSelect:
;}YKN-20160728 -E

        cli
        hlt
        jmp $-2

; Switch to flat mode.

flat32Start::

        db 0BEh
        dw BufferStartLocation        ; mov        si, BufferStartLocation
        db 66h,  8Bh, 14h             ; mov        edx,dword ptr [si]          ; EDX is keeping the start address of wakeup buffer

        db 0BEh
        dw Cr3OffsetLocation          ; mov        si, Cr3Location
        db 66h,  8Bh, 0Ch             ; mov        ecx,dword ptr [si]          ; ECX is keeping the value of CR3

        db 0BEh
        dw GdtrLocation               ; mov        si, GdtrProfile
        db 66h                        ; db         66h
        db 2Eh,  0Fh, 01h, 14h        ; lgdt       fword ptr cs:[si]

        db 0BEh
        dw IdtrLocation               ; mov        si, IdtrProfile
        db 66h                        ; db         66h
        db 2Eh,  0Fh, 01h, 1Ch        ; lidt       fword ptr cs:[si]

        db 33h,  0C0h                 ; xor        ax,  ax
        db 8Eh,  0D8h                 ; mov        ds,  ax

        db 0Fh,  20h, 0C0h            ; mov        eax, cr0                    ; Get control register 0
        db 66h,  83h, 0C8h, 01h       ; or         eax, 000000001h             ; Set PE bit (bit #0)
        db 0Fh,  22h, 0C0h            ; mov        cr0, eax

FLAT32_JUMP::

        db 66h,  67h, 0EAh            ; far jump
        dd 0h                         ; 32-bit offset
        dw 20h                        ; 16-bit selector

NemInit::                             ; protected mode entry point

        db 66h,  0B8h, 18h,  00h      ; mov        ax,  18h
        db 66h,  8Eh,  0D8h           ; mov        ds,  ax
        db 66h,  8Eh,  0C0h           ; mov        es,  ax
        db 66h,  8Eh,  0E0h           ; mov        fs,  ax
        db 66h,  8Eh,  0E8h           ; mov        gs,  ax
        db 66h,  8Eh,  0D0h           ; mov        ss,  ax                     ; Flat mode setup.

        db 0Fh,  20h,  0E0h           ; mov        eax, cr4
        db 0Fh,  0BAh, 0E8h, 05h      ; bts        eax, 5
        db 0Fh,  22h,  0E0h           ; mov        cr4, eax

        db 0Fh,  22h,  0D9h           ; mov        cr3, ecx

        db 8Bh,  0F2h                 ; mov        esi, edx                    ; Save wakeup buffer address

        db 0B9h
        dd 0C0000080h                 ; mov        ecx, 0c0000080h             ; EFER MSR number.
        db 0Fh,  32h                  ; rdmsr                                  ; Read EFER.
        db 0Fh,  0BAh, 0E8h, 08h      ; bts        eax, 8                      ; Set LME=1.
        db 0Fh,  30h                  ; wrmsr                                  ; Write EFER.

        db 0Fh,  20h,  0C0h           ; mov        eax, cr0                    ; Read CR0.
        db 0Fh,  0BAh, 0E8h, 1Fh      ; bts        eax, 31                     ; Set PG=1.
        db 0Fh,  22h,  0C0h           ; mov        cr0, eax                    ; Write CR0.

LONG_JUMP::

        db 67h,  0EAh                 ; far jump
        dd 0h                         ; 32-bit offset
        dw 38h                        ; 16-bit selector

LongModeStart::

        mov         ax,  30h
        mov         ds,  ax
        mov         es,  ax
        mov         ss,  ax

        ;
        ; Get processor number for this AP
        ; Note that BSP may become an AP due to SwitchBsp()
        ;

        xor         ecx, ecx
        lea         edi, [esi + BistBuffer]

GetProcNumber::
        cmp         [edi], ebx                       ; APIC ID match?
        jz          Found
        add         edi, 8
        inc         ecx
        cmp         ecx, dword ptr [esi + ApCountLocation]
        jbe         GetProcNumber

Found::

        ;
        ; ProgramStack
        ;

        mov         edi, esi
        add         edi, StackSizeLocation
        mov         rax, qword ptr [edi]
        inc         rcx
        mul         rcx                               ; RAX = StackSize * (CpuNumber + 1)
        dec         rcx
        
        mov         edi, esi
        add         edi, StackStartAddressLocation
        mov         rbx, qword ptr [edi]
        add         rax, rbx                          ; RAX = StackStart + StackSize * (CpuNumber + 1)

        mov         rsp, rax
        sub         rsp, MonitorFilterSize            ; Reserved Monitor data space
        mov         rbp, rcx                          ; Save processor number in rbp

        ;
        ; Call assembly function to initialize FPU.
        ;
        mov         rax, InitializeFloatingPointUnits
        sub         rsp, 20h
        call        rax
        add         rsp, 20h

        ;
        ; Call C Function
        ;
        mov         edi, esi
        add         edi, CProcedureLocation
        add         esi, ApLoopModeLocation           ; esi = ApLoopMode address location

        xor         rbx, rbx                          
        mov         qword ptr [rsp + 0Ch], rbx        ; Clean ReadyToBoot Flag
        mov         bx, bp                            ; bx = the lowest 16bit of CpuNumber
        or          ebx, StartupApSignal              ; Construct AP run Singal
        
WakeUpThisAp::
        mov         rax, qword ptr [edi]

        test        rax, rax
        jz          CheckReadyToBoot

        mov         rcx, rbp                      ; rcx = CpuNumber 
        sub         rsp, 20h
        call        rax
        add         rsp, 20h

        ;
        ; Check if BSP was switched
        ;
        mov         rax, offset mBspSwitched
        cmp         byte ptr [rax], 0
        jz          CheckReadyToBoot

        mov         byte ptr [rax], 0                 ;clear BSP switch flag

        mov         rax, offset mNewProcessorNumber
        mov         rbp, [rax]                        ; rbp = new processor number

        mov         bx, bp                            ; bx = the lowest 16bit of CpuNumber

        ;
        ; Assign the dedicated AP stack for the new AP
        ;
        mov         rax, offset mMonitorDataAddress
        mov         rsp, [rax]
        
CheckReadyToBoot::
        mov         eax, dword ptr [esi]          ; eax = ApLoopMode for POST
        cmp         byte ptr [rsp + 0Ch], 1       ; Check ReadyToBoot flag?
        jnz         CheckWakeUpManner

        mov         rcx, rbp                      ; rcx = CpuNumber
        mov         rdx, rsp                      ; rdx = Monitor data address
        mov         r15, ApProtectedModeThunk     ; 

        sub         rsp, 20h                      
        call        r15                           ; Call thunk function to transfer to 32bit protected mode
        hlt
        jmp         $                             ; It will never reach here

CheckWakeUpManner::
        cli
        cmp         eax, ApInHltLoop
        jz          HltApLoop

        cmp         eax, ApInMwaitLoop
        jnz         CheckRunSignal

ApMwaitLoop::
        mov         rax, rsp                      ; Set Monitor Address
        xor         rcx, rcx                      ; rcx = 0
        xor         rdx, rdx                      ; rdx = 0
        DB          0fh, 1, 0c8h                  ; MONITOR

        mov         rax, qword ptr [rsp + 4]      ; Mwait Cx, Target C-State per rax[7:4]
        DB          0fh, 1, 0c9h                  ; MWAIT

CheckRunSignal::
        cmp         dword ptr [rsp], ebx          ; Check if run signal correct?
        jz          WakeUpThisAp                  ; Jmp to execute AP task
        pause
        jmp         CheckReadyToBoot              ; Unknown break, go checking run manner

ProtectedModeEntryPoint::
        DB          08bh, 44h, 24h, 04h           ; mov eax, [esp + 4] 
        test        eax, eax                      ; C-State Enable ?
        jz          HltApLoop                     ; Jump to halt loop 

ProtectedModeApMwaitLoop::
        mov         eax, esp                      ; Set Monitor Address
        xor         ecx, ecx
        xor         edx, edx
        DB          0fh, 1, 0c8h                  ; MONITOR

        DB          08bh, 44h, 24h, 08h           ; mov eax, [esp + 8] ; the deepest C-State value
        DB          0fh, 1, 0c9h                  ; MWAIT

        jmp         ProtectedModeApMwaitLoop

HltApLoop::
        cli
        hlt
        jmp         HltApLoop                     ; Jump to halt loop

RendezvousFunnelProc   ENDP
RendezvousFunnelProcEnd::


;-------------------------------------------------------------------------------------
;  AsmGetAddressMap (&AddressMap);
;-------------------------------------------------------------------------------------
; comments here for definition of address map
AsmGetAddressMap   PROC

        mov         rax, offset RendezvousFunnelProcStart
        mov         qword ptr [rcx], rax
        mov         qword ptr [rcx+8h], NemInit - RendezvousFunnelProcStart
        mov         qword ptr [rcx+10h], FLAT32_JUMP - RendezvousFunnelProcStart
        mov         qword ptr [rcx+18h], LongModeStart - RendezvousFunnelProcStart
        mov         qword ptr [rcx+20h], LONG_JUMP - RendezvousFunnelProcStart
        mov         qword ptr [rcx+28h], RendezvousFunnelProcEnd - RendezvousFunnelProcStart
        mov         qword ptr [rcx+30h], ProtectedModeEntryPoint - RendezvousFunnelProcStart

        ret

AsmGetAddressMap   ENDP

;-------------------------------------------------------------------------------------
;AsmExchangeRole procedure follows. This procedure executed by current BSP, that is
;about to become an AP. It switches it'stack with the current AP.
;AsmExchangeRole (IN   CPU_EXCHANGE_INFO    *MyInfo, IN   CPU_EXCHANGE_INFO    *OthersInfo);
;-------------------------------------------------------------------------------------
CPU_SWITCH_STATE_IDLE          equ        0
CPU_SWITCH_STATE_STORED        equ        1
CPU_SWITCH_STATE_LOADED        equ        2

AsmExchangeRole   PROC
        ; DO NOT call other functions in this function, since 2 CPU may use 1 stack
        ; at the same time. If 1 CPU try to call a functiosn, stack will be corrupted.

        push        rax
        push        rbx
        push        rcx
        push        rdx
        push        rsi
        push        rdi
        push        rbp
        push        r8
        push        r9
        push        r10
        push        r11
        push        r12
        push        r13
        push        r14
        push        r15

        mov         rax, cr0
        push        rax

        mov         rax, cr4
        push        rax

        ; rsi contains MyInfo pointer
        mov         rsi, rcx

        ; rdi contains OthersInfo pointer
        mov         rdi, rdx

        ;Store EFLAGS, GDTR and IDTR regiter to stack
        pushfq
        sgdt        fword ptr [rsi + 16]
        sidt        fword ptr [rsi + 26]

        ; Store the its StackPointer
        mov         qword ptr [rsi + 8], rsp

        ; update its switch state to STORED
        mov         byte ptr [rsi], CPU_SWITCH_STATE_STORED

WaitForOtherStored::
        ; wait until the other CPU finish storing its state
        cmp         byte ptr [rdi], CPU_SWITCH_STATE_STORED
        jz          OtherStored
        pause
        jmp         WaitForOtherStored

OtherStored::
        ; Since another CPU already stored its state, load them
        ; load GDTR value
        lgdt        fword ptr [rdi + 16]

        ; load IDTR value
        lidt        fword ptr [rdi + 26]

        ; load its future StackPointer
        mov         rsp, qword ptr [rdi + 8]

        ; update the other CPU's switch state to LOADED
        mov         byte ptr [rdi], CPU_SWITCH_STATE_LOADED

WaitForOtherLoaded::
        ; wait until the other CPU finish loading new state,
        ; otherwise the data in stack may corrupt
        cmp         byte ptr [rsi], CPU_SWITCH_STATE_LOADED
        jz          OtherLoaded
        pause
        jmp         WaitForOtherLoaded

OtherLoaded::
        ; since the other CPU already get the data it want, leave this procedure
        popfq

        pop         rax
        mov         cr4, rax

        pop         rax
        mov         cr0, rax

        pop         r15
        pop         r14
        pop         r13
        pop         r12
        pop         r11
        pop         r10
        pop         r9
        pop         r8
        pop         rbp
        pop         rdi
        pop         rsi
        pop         rdx
        pop         rcx
        pop         rbx
        pop         rax

        ret
AsmExchangeRole   ENDP

text    ENDS

END
