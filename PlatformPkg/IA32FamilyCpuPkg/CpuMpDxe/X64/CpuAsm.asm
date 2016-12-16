;
; This file contains an 'Intel Peripheral Driver' and is      
; licensed for Intel CPUs and chipsets under the terms of your
; license agreement with Intel or your vendor.  This file may 
; be modified by the user, subject to additional terms of the 
; license agreement                                           
;
  page    ,132
  title   CPU ARCHITECTURAL DXE PROTOCOL ASSEMBLY HOOKS
;-------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
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
;   CpuAsm.asm
; 
; Abstract:
; 
;
;-------------------------------------------------------------------------------


text  SEGMENT

ApMachineCheckHandler    proc    
  ;
  ; Clear MCIP flag of IA32_MCG_STATUS register
  ;
  mov     ecx, 17ah
  rdmsr
  btr     eax, 2
  wrmsr

  iretq
  
ApMachineCheckHandler ENDP

PUBLIC  ApMachineCheckHandlerEnd

ApMachineCheckHandlerEnd LABEL BYTE

;------------------------------------------------------------------------------
;  VOID
;  C1eExceptionHandler (
;    VOID
;    )
;------------------------------------------------------------------------------
C1eExceptionHandler    proc    

  push    rbp
  mov     rbp, rsp

  pushfq
  cli

  ; Fix the return address on stack to skip offending
  ; code which caused the exception.
  mov    rax, qword ptr [rbp + 8] 
  add    rax, 2
  mov    qword ptr [rbp + 8], rax

  popfq
  pop    rbp
  
  add    rsp, 8
  
  iretq
  
C1eExceptionHandler ENDP

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalEnablePaging64 (
;   IN      UINT16                    Cs,
;   IN      UINT64                    EntryPoint,
;   IN      UINT64                    Context1,  OPTIONAL
;   IN      UINT64                    Context2,  OPTIONAL
;   IN      UINT64                    NewStack
;   );
;------------------------------------------------------------------------------
InternalEnablePaging64 PROC
    cli
    pop     rax                         ; skip the return address
    call    @Base
@Base:
    add     dword ptr [rsp], @F - @Base ; offset for far retf, seg is the 1st arg
    mov     rax, cr4
    or      al, (1 SHL 5)
    mov     cr4, rax                    ; enable PAE
    mov     ecx, 0c0000080h
    rdmsr
    or      ah, 1                       ; set LME
    wrmsr
    mov     rax, cr0
    bts     eax, 31
    mov     cr0, rax                    ; enable paging
    retf
@@:                                     ; long mode starts here
    mov     rbx, [esp]
    mov     rcx, [esp + 8]
    mov     rdx, [esp + 10h]
    mov     rsp, [esp + 18h]
    add     rsp, -20h
    call    rbx
    hlt                                 ; halt processor if EntryPoint() returned
InternalEnablePaging64 ENDP

;------------------------------------------------------------------------------
;  VOID
;  CpuOnlyResetResumeEntryWrapper (
;    VOID                 *Context1,
;    VOID                 *Context2
;    )
; NOTE: This function is written in Long mode instructions and build by 64-bit 
;       assembler, BUT running in 32-bit protected mode
; This function is the real entry point after CPU only reset. SEC will directly 
; jump into this function when SEC determine the reset is a CPU only reset        
;------------------------------------------------------------------------------

CpuOnlyResetResumeEntryWrapper  PROC 
  pop rax     ; discards return address (4 byte)
  pop rax     ; discards Context1       (4 byte)
  pop rax     ; discards Context2       (4 byte)
  ;
  ; here esp is pointer to the temporary GDT description: TemporaryGdtLimit and
  ; TemporaryGdtBase
  ; load the temporary gdtr
  
  lgdt 	fword ptr [rsp]
  
  ; discards temporary GDT description and now esp is pointing to TemporaryCR3       
  ; totally 8 byte
  pop rax    ;
  pop rax    ;
  
  pop rax      ; get value of temporaryCR3
  mov cr3, rax ; set the temporary page table address
  
  ;
  ; now ESP is point the reserve1, all parameter has already prepared in
  ; the stack
  ;
  jmp  InternalEnablePaging64
   
CpuOnlyResetResumeEntryWrapper ENDP

text  ENDS
END

