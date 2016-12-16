//
// This file contains an 'Intel Peripheral Driver' and is
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may
// be modified by the user, subject to additional terms of the
// license agreement
//
#-------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
# This software and associated documentation (if any) is furnished
# under a license and may only be used or copied in accordance
# with the terms of the license. Except as permitted by such
# license, no part of this software or documentation may be
# reproduced, stored in a retrieval system, or transmitted in any
# form or by any means without the express written consent of
# Intel Corporation.

#
#
# Module Name:
#
#   MpFuncs.S
#
# Abstract:
#
#   This is the assembly code for EM64T MP support
#
#-------------------------------------------------------------------------------


.equ                   VacantFlag,                0x0
.equ                   NotVacantFlag,             0xff
.equ                   StartupApSignal,           0x6E750000
.equ                   MonitorFilterSize,         0x10
.equ                   ApInHltLoop,               1
.equ                   ApInMwaitLoop,             2
.equ                   ApInRunLoop,               3


.equ                   LockLocation,              RendezvousFunnelProcEnd - RendezvousFunnelProcStart
.equ                   StackStartAddressLocation, RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x08
.equ                   StackSizeLocation,         RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x10
.equ                   CProcedureLocation,        RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x18
.equ                   GdtrLocation,              RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x20
.equ                   IdtrLocation,              RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x2A
.equ                   BufferStartLocation,       RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x34
.equ                   Cr3OffsetLocation,         RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x38
.equ                   InitFlagLocation,          RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x3C
.equ                   ApCountLocation,           RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x40
.equ                   DcuModeSelectFlagLocation, RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x44
.equ                   DcuModeLocation,           RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x45
.equ                   ApLoopModeLocation,        RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x48
.equ                   BistBuffer,                RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x4C

#-------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------
#RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
#procedure serializes all the AP processors through an Init sequence. It must be
#noted that APs arrive here very raw...ie: real mode, no stack.
#ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
#IS IN MACHINE CODE.
#-------------------------------------------------------------------------------------
#RendezvousFunnelProc (&WakeUpBuffer,MemAddress);

.text

ASM_GLOBAL ASM_PFX(RendezvousFunnelProc)
ASM_PFX(RendezvousFunnelProc):
RendezvousFunnelProcStart:

# At this point CS = 0x(vv00) and ip= 0x0.

        .byte 0x66,0x8b,0xe8          # mov        ebp, eax

        .byte 0x8c,0xc8               # mov        ax,  cs
        .byte 0x8e,0xd8               # mov        ds,  ax
        .byte 0x8e,0xc0               # mov        es,  ax
        .byte 0x8e,0xd0               # mov        ss,  ax
        .byte 0x33,0xc0               # xor        ax,  ax
        .byte 0x8e,0xe0               # mov        fs,  ax
        .byte 0x8e,0xe8               # mov        gs,  ax

# Get APIC ID
#
        .byte 0x66,0xB8
        .long 0x00000000              # mov        eax, 0
        .byte 0x0F,0xA2               # cpuid
        .byte 0x66,0x3d
        .long 0x0000000B              # cmp        eax, 0b
        .byte 0x73,0x0e               # jnb        X2Apic

# Processor is not x2APIC capable, so get 8-bit APIC ID
        .byte 0x66,0xB8               # mov        eax, 1
        .long 0x00000001              # cpuid
        .byte 0x0F,0xA2
        .byte 0x66,0xC1,0xEB,0x18     # shr        ebx, 24
        .byte 0xeb,0x0e               # jmp CheckInitFlag

# Processor is x2APIC capable, so get 32-bit x2APIC ID
X2Apic:
        .byte 0x66,0xB8
        .long 0x0000000B              # mov        eax, 0b
        .byte 0x66,0x31,0xc9          # xor        ecx, ecx
        .byte 0x0F,0xA2               # cpuid
        .byte 0x66,0x89,0xd3          # mov        ebx, edx

CheckInitFlag:
# If it is the first time AP wakes up, just record AP's BIST
# Otherwise, switch to protected mode.

        .byte 0xBE
        .word InitFlagLocation        # mov        si,  InitFlag
        .byte 0x66,0x83,0x3C,0x0      # cmp        dword ptr [si], 0
        .byte 0x74
        .byte flat32Start - . - 1     # jz         flat32Start

# Increase ApCount as processor number for index of BIST Info array

        .byte 0x66,0xa1                  # mov        eax, [ApCountLocation]
        .word ApCountLocation
IncApCount:
        .byte 0x66,0x67,0x8d,0x48,0x01   # lea        ecx, [eax+1]
        .byte 0xf0,0x66,0x0f,0xb1,0x0e   # lock       cmpxchg [ApCountLocation], ecx
        .word ApCountLocation
        .byte 0x75,0xf2                  # jnz        IncApCount
        .byte 0x66, 0xff, 0xc0           # inc        eax                         ; AP processor number starts from 1
# Record BIST information
#
        .byte 0x66,0x67,0x8d,0x34,0xc5   # lea esi, [BistBuffer + eax*8]
        .long BistBuffer

        .byte 0x66,0x89,0x1c             # mov        dword ptr [si], ebx         ; APIC ID
        .byte 0x66,0x89,0x6C,0x04        # mov        dword ptr [si + 4], ebp     ; Store BIST value

#Check Dcu Mode select as early as possible
        .byte 0xbe
        .word DcuModeSelectFlagLocation  # mov si, DcuModeSelectFlagLocation
        .byte 0x80,0x3c,0x00             # cmp byte ptr [si], 0
        .byte 0x74,0x27                  # je SkipDcuModeSelect

        .byte 0x66,0xb9                  # mov ecx, 0ceh
        .long 0x0000000ce                #
        .byte 0x0f,0x32                  # rdmsr
        .byte 0x66,0x0f,0xba,0xe0,26     # bt eax, 26
        .byte 0x73,0x18                  # jnc SkipDcuModeSelect

        .byte 0xbe
        .word DcuModeLocation            # mov si, DcuModeLocation
        .byte 0x80,0x3c,0x00             # cmp byte ptr [si], 0
        .byte 0x74,0x10                  # je SkipDcuModeSelect

        .byte 0x66,0xb9                  # mov ecx, 031h
        .long 0x000000031                #
        .byte 0x0f,0x32                  # rdmsr
        .byte 0x66,0x0d                  # or eax, 1
        .long 0x000000001
        .byte 0x0f,0x30                  # wrmsr

SkipDcuModeSelect:
        cli
        hlt
        jmp .-2

# Switch to flat mode.

flat32Start:

        .byte 0xBE
        .word BufferStartLocation     # mov        si, BufferStartLocation
        .byte 0x66,0x8B,0x14          # mov        edx,dword ptr [si]          ; EDX is keeping the start address of wakeup buffer

        .byte 0xBE
        .word Cr3OffsetLocation       # mov        si, Cr3Location
        .byte 0x66,0x8B,0xC           # mov        ecx,dword ptr [si]          ; ECX is keeping the value of CR3

        .byte 0xBE
        .word GdtrLocation            # mov        si, GdtrLocation
        .byte 0x66                    # db         66h
        .byte 0x2E,0xF,0x1,0x14       # lgdt       fword ptr cs:[si]

        .byte 0xBE
        .word IdtrLocation            # mov        si, IdtrLocation
        .byte 0x66                    # db         66h
        .byte 0x2E,0xF,0x1,0x1C       # lidt       fword ptr cs:[si]

        .byte 0x33,0xC0               # xor        ax,  ax
        .byte 0x8E,0xD8               # mov        ds,  ax

        .byte 0xF,0x20,0xC0           # mov        eax, cr0                    ; Get control register 0
        .byte 0x66,0x83,0xC8,0x1      # or         eax, 000000001h             ; Set PE bit (bit #0)
        .byte 0xF,0x22,0xC0           # mov        cr0, eax

FLAT32_JUMP:

        .byte 0x66,0x67,0xEA          # far jump
        .long 0x0                     # 32-bit offset
        .word 0x20                    # 16-bit selector

NemInit:                              # protected mode entry point

        .byte 0x66,0xB8,0x18,0x0      # mov        ax,  18h
        .byte 0x66,0x8E,0xD8          # mov        ds,  ax
        .byte 0x66,0x8E,0xC0          # mov        es,  ax
        .byte 0x66,0x8E,0xE0          # mov        fs,  ax
        .byte 0x66,0x8E,0xE8          # mov        gs,  ax
        .byte 0x66,0x8E,0xD0          # mov        ss,  ax                     ; Flat mode setup.

        .byte 0xF,0x20,0xE0           # mov        eax, cr4
        .byte 0xF,0xBA,0xE8,0x5       # bts        eax, 5
        .byte 0xF,0x22,0xE0           # mov        cr4, eax

        .byte 0xF,0x22,0xD9           # mov        cr3, ecx

        .byte 0x8B,0xF2               # mov        esi, edx                    ; Save wakeup buffer address

        .byte 0xB9
        .long 0xC0000080              # mov        ecx, 0c0000080h             ; EFER MSR number.
        .byte 0xF,0x32                # rdmsr                                  ; Read EFER.
        .byte 0xF,0xBA,0xE8,0x8       # bts        eax, 8                      ; Set LME=1.
        .byte 0xF,0x30                # wrmsr                                  ; Write EFER.

        .byte 0xF,0x20,0xC0           # mov        eax, cr0                    ; Read CR0.
        .byte 0xF,0xBA,0xE8,0x1F      # bts        eax, 31                     ; Set PG=1.
        .byte 0xF,0x22,0xC0           # mov        cr0, eax                    ; Write CR0.

LONG_JUMP:

        .byte 0x67,0xEA               # far jump
        .long 0x0                     # 32-bit offset
        .word 0x38                    # 16-bit selector

LongModeStart:

        movw        $0x30,%ax
        .byte       0x66
        movw        %ax,%ds
        .byte       0x66
        movw        %ax,%es
        .byte       0x66
        movw        %ax,%ss

        #
        # Get processor number for this AP
        # Note that BSP may become an AP due to SwitchBsp()
        #
        xorl        %ecx, %ecx
        leal        BistBuffer(%esi), %edi

GetProcNumber:
        cmpl        %ebx, (%edi)                     # APIC ID match?
        jz          Found
        addl        $8, %edi
        incl        %ecx
        cmpl        ApCountLocation(%esi), %ecx
        jbe         GetProcNumber

Found:
        #
        # ProgramStack
        #

        movl        %esi, %edi
        addl        $StackSizeLocation, %edi
        movq        (%edi), %rax
        incq        %rcx
        mulq        %rcx                              # RAX = StackSize * (CpuNumber + 1)
        decq        %rcx
        
        movl        %esi, %edi
        addl        $StackStartAddressLocation, %edi
        movq        (%edi), %rbx
        addq        %rbx, %rax                        # RAX = StackStart + StackSize * (CpuNumber + 1)

        movq        %rax, %rsp
        subq        $MonitorFilterSize, %rsp          # Reserved Monitor data space
        movq        %rcx, %rbp                        # Save processor number in rbp

        #
        # Call assembly function to initialize FPU.
        #
        movabsq     $ASM_PFX(InitializeFloatingPointUnits), %rax
        subq        $0x20, %rsp
        call        *%rax
        addq        $0x20, %rsp

        #
        # Call C Function
        #
        movl        %esi,%edi
        addl        $CProcedureLocation, %edi
        addl        $ApLoopModeLocation, %esi         # esi = ApLoopMode address location

        xorq        %rbx, %rbx                          
        movq        %rbx, 0xC(%rsp)                   # Clean ReadyToBoot Flag
        movw        %bp, %bx                          # bx = the lowest 16bit of CpuNumber
        orl         $StartupApSignal, %ebx            # Construct AP run Singal
        
WakeUpThisAp:
        movq        (%edi), %rax

        testq       %rax, %rax
        jz          CheckReadyToBoot

        movq        %rbp, %rcx                    # rcx = CpuNumber 
        subq        $0x20, %rsp
        call        *%rax
        addq        $0x20, %rsp

        #
        # Check if BSP was switched
        #
        movabsq     $ASM_PFX(mBspSwitched), %rax
        cmpb        $0, (%rax)
        jz          CheckReadyToBoot

        movb        $0, (%rax)                    #clear BSP switch flag

        movabsq     $ASM_PFX(mNewProcessorNumber), %rax
        movq        (%rax), %rbp                  # rbp = new processor number

        movw        %bp, %bx                      # bx = the lowest 16bit of CpuNumber

        #
        # Assign the dedicated AP stack for the new AP
        #
        movabsq     $ASM_PFX(mMonitorDataAddress), %rax
        movq        (%rax), %rsp
        
CheckReadyToBoot:
        movl        (%esi), %eax                  # eax = ApLoopMode for POST
        cmpb        $1, 0xC(%rsp)                 # Check ReadyToBoot flag?
        jnz         CheckWakeUpManner

        movq        %rbp, %rcx                    # rcx = CpuNumber
        movq        %rsp, %rdx                    # rdx = Monitor data address
        movabsq     $ASM_PFX(ApProtectedModeThunk), %r15   # 

        subq        $0x20, %rsp                      
        call        *%r15                         # Call thunk function to transfer to 32bit protected mode
        hlt
        jmp         .                             # It will never reach here

CheckWakeUpManner:
        cli
        cmpl        $ApInHltLoop, %eax
        jz          HltApLoop

        cmpl        $ApInMwaitLoop, %eax
        jnz         CheckRunSignal

ApMwaitLoop:
        movq        %rsp, %rax                    # Set Monitor Address
        xorq        %rcx, %rcx                    # rcx = 0
        xorq        %rdx, %rdx                    # rdx = 0
        .byte       0x0f,0x1,0xc8                 # MONITOR

        movq        4(%rsp), %rax                 # Mwait Cx, Target C-State per rax[7:4]
        .byte       0x0f,0x1,0xc9                 # MWAIT

CheckRunSignal:
        cmpl        %ebx, (%rsp)                  # Check if run signal correct?
        jz          WakeUpThisAp                  # Jmp to execute AP task
        pause
        jmp         CheckReadyToBoot              # Unknown break, go checking run manner

ProtectedModeEntryPoint:
        .byte       0x8b,0x44,0x24,0x04           # mov eax, [esp + 4] 
        testl       %eax, %eax                    # C-State Enable ?
        jz          HltApLoop                     # Jump to halt loop 

ProtectedModeApMwaitLoop:
        movl        %esp, %eax                    # Set Monitor Address
        xorl        %ecx, %ecx
        xorl        %edx, %edx
        .byte       0x0f,0x1,0xc8                 # MONITOR

        .byte       0x8b,0x44,0x24,0x08           # mov eax, [esp + 8] ; the deepest C-State value
        .byte       0x0f,0x1,0xc9                 # MWAIT

        jmp         ProtectedModeApMwaitLoop

HltApLoop:
        cli
        hlt
        jmp         HltApLoop                     # Jump to halt loop

RendezvousFunnelProcEnd:


#-------------------------------------------------------------------------------------
#  AsmGetAddressMap (&AddressMap);
#-------------------------------------------------------------------------------------
# comments here for definition of address map
ASM_GLOBAL ASM_PFX(AsmGetAddressMap)
ASM_PFX(AsmGetAddressMap):

        movabsq      $RendezvousFunnelProcStart, %rax
        movq         %rax, (%rcx)
        movq         $(NemInit - RendezvousFunnelProcStart), 0x08(%rcx)
        movq         $(FLAT32_JUMP - RendezvousFunnelProcStart), 0x10(%rcx)
        movq         $(LongModeStart - RendezvousFunnelProcStart), 0x18(%rcx)
        movq         $(LONG_JUMP - RendezvousFunnelProcStart), 0x20(%rcx)
        movq         $(RendezvousFunnelProcEnd - RendezvousFunnelProcStart), 0x28(%rcx)
        movq         $(ProtectedModeEntryPoint - RendezvousFunnelProcStart), 0x30(%rcx)

        ret


#-------------------------------------------------------------------------------------
#AsmExchangeRole procedure follows. This procedure executed by current BSP, that is
#about to become an AP. It switches it'stack with the current AP.
#AsmExchangeRole (IN   CPU_EXCHANGE_INFO    *MyInfo, IN   CPU_EXCHANGE_INFO    *OthersInfo);
#-------------------------------------------------------------------------------------
.equ                           CPU_SWITCH_STATE_IDLE, 0
.equ                           CPU_SWITCH_STATE_STORED, 1
.equ                           CPU_SWITCH_STATE_LOADED, 2

ASM_GLOBAL ASM_PFX(AsmExchangeRole)
ASM_PFX(AsmExchangeRole):
        # DO NOT call other functions in this function, since 2 CPU may use 1 stack
        # at the same time. If 1 CPU try to call a functiosn, stack will be corrupted.

        pushq        %rax
        pushq        %rbx
        pushq        %rcx
        pushq        %rdx
        pushq        %rsi
        pushq        %rdi
        pushq        %rbp
        pushq        %r8
        pushq        %r9
        pushq        %r10
        pushq        %r11
        pushq        %r12
        pushq        %r13
        pushq        %r14
        pushq        %r15

        movq         %cr0, %rax
        pushq        %rax

        movq         %cr4, %rax
        pushq        %rax

        # rsi contains MyInfo pointer
        movq         %rcx, %rsi

        # rdi contains OthersInfo pointer
        movq         %rdx, %rdi

        #Store EFLAGS, GDTR and IDTR regiter to stack
        pushfq
        sgdt        16(%rsi)
        sidt        26(%rsi)

        # Store the its StackPointer
        movq        %rsp, 8(%rsi)

        # update its switch state to STORED
        movb       $CPU_SWITCH_STATE_STORED, (%rsi)

WaitForOtherStored:
        # wait until the other CPU finish storing its state
        cmpb       $CPU_SWITCH_STATE_STORED, (%rdi)
        jz         OtherStored
        pause
        jmp        WaitForOtherStored

OtherStored:
        # Since another CPU already stored its state, load them
        # load GDTR value
        lgdt        16(%rdi)

        # load IDTR value
        lidt        26(%rdi)

        # load its future StackPointer
        movq        8(%rdi), %rsp

        # update the other CPU's switch state to LOADED
        movb        $CPU_SWITCH_STATE_LOADED, (%rdi)

WaitForOtherLoaded:
        # wait until the other CPU finish loading new state,
        # otherwise the data in stack may corrupt
        cmpb       $CPU_SWITCH_STATE_LOADED, (%rsi)
        jz         OtherLoaded
        pause
        jmp         WaitForOtherLoaded

OtherLoaded:
        # since the other CPU already get the data it want, leave this procedure
        popfq

        popq         %rax
        movq         %rax, %cr4

        popq         %rax
        movq         %rax, %cr0

        popq         %r15
        popq         %r14
        popq         %r13
        popq         %r12
        popq         %r11
        popq         %r10
        popq         %r9
        popq         %r8
        popq         %rbp
        popq         %rdi
        popq         %rsi
        popq         %rdx
        popq         %rcx
        popq         %rbx
        popq         %rax

        ret
