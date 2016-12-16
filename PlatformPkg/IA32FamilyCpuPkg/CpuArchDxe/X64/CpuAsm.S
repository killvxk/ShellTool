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
#   CpuAsm.S
#
# Abstract:
#
#
#-------------------------------------------------------------------------------

.data

ExternalVectorTablePtr:  .quad  0

.text

ASM_GLOBAL ASM_PFX(InitializeExternalVectorTablePtr)
ASM_PFX(InitializeExternalVectorTablePtr):
        movq    %rcx, ExternalVectorTablePtr(%rip)
        ret
#
#
#
#------------------------------------------------------------------------------
#  Generic IDT Vector Handlers for the Host. They are all the same so they
#  will compress really well.
#
#  By knowing the return address for Vector 00 you can can calculate the
#  vector number by looking at the call CommonInterruptEntry return address.
#  (return address - AsmIdtVector00Base)/8 == IDT index
#
#------------------------------------------------------------------------------

.p2align 3

ASM_GLOBAL ASM_PFX(AsmIdtVector00)
ASM_PFX(AsmIdtVector00):
# The following segment repeats 256 times:
# No. 1
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 2
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 3
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 4
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 5
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 6
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 7
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 8
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 9
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 10
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 11
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 12
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 13
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 14
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 15
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 16
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 17
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 18
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 19
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 20
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 21
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 22
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 23
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 24
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 25
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 26
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 27
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 28
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 29
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 30
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 31
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 32
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 33
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 34
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 35
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 36
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 37
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 38
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 39
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 40
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 41
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 42
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 43
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 44
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 45
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 46
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 47
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 48
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 49
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 50
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 51
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 52
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 53
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 54
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 55
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 56
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 57
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 58
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 59
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 60
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 61
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 62
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 63
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 64
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 65
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 66
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 67
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 68
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 69
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 70
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 71
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 72
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 73
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 74
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 75
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 76
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 77
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 78
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 79
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 80
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 81
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 82
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 83
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 84
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 85
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 86
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 87
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 88
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 89
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 90
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 91
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 92
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 93
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 94
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 95
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 96
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 97
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 98
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 99
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 100
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 101
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 102
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 103
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 104
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 105
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 106
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 107
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 108
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 109
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 110
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 111
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 112
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 113
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 114
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 115
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 116
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 117
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 118
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 119
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 120
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 121
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 122
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 123
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 124
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 125
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 126
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 127
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 128
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 129
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 130
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 131
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 132
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 133
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 134
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 135
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 136
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 137
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 138
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 139
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 140
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 141
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 142
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 143
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 144
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 145
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 146
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 147
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 148
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 149
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 150
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 151
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 152
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 153
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 154
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 155
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 156
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 157
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 158
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 159
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 160
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 161
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 162
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 163
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 164
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 165
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 166
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 167
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 168
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 169
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 170
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 171
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 172
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 173
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 174
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 175
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 176
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 177
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 178
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 179
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 180
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 181
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 182
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 183
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 184
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 185
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 186
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 187
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 188
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 189
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 190
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 191
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 192
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 193
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 194
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 195
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 196
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 197
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 198
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 199
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 200
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 201
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 202
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 203
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 204
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 205
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 206
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 207
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 208
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 209
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 210
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 211
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 212
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 213
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 214
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 215
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 216
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 217
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 218
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 219
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 220
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 221
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 222
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 223
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 224
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 225
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 226
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 227
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 228
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 229
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 230
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 231
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 232
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 233
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 234
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 235
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 236
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 237
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 238
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 239
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 240
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 241
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 242
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 243
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 244
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 245
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 246
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 247
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 248
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 249
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 250
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 251
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 252
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 253
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 254
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 255
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop
# No. 256
        call  CommonInterruptEntry
        .word ( . - ASM_PFX(AsmIdtVector00) - 5 ) / 8
        nop


#---------------------------------------;
# CommonInterruptEntry                  ;
#---------------------------------------;
# The follow algorithm is used for the common interrupt routine.

#
# +---------------------+ <-- 16-byte aligned ensured by processor
# +    Old SS           +
# +---------------------+
# +    Old RSP          +
# +---------------------+
# +    RFlags           +
# +---------------------+
# +    CS               +
# +---------------------+
# +    RIP              +
# +---------------------+
# +    Error Code       +
# +---------------------+
# + RCX / Vector Number +
# +---------------------+
# +    RBP              +
# +---------------------+ <-- RBP, 16-byte aligned
#

CommonInterruptEntry:
  cli
  #
  # All interrupt handlers are invoked through interrupt gates, so
  # IF flag automatically cleared at the entry point
  #
  #
  # Calculate vector number
  #
  xchg    %rcx, (%rsp)
  movzwl  (%rcx), %ecx
  cmpl    $32,%ecx        # Intel reserved vector for exceptions?
  jae     NoErrorCode
  btl     %ecx, ASM_PFX(mErrorCodeFlag)
  jc      L1

NoErrorCode:
  #
  # Push a dummy error code on the stack
  # to maintain coherent stack map
  #
  pushq    (%rsp)
  movq     $0, 8(%rsp)
L1:
  pushq    %rbp
  movq    %rsp, %rbp

  #
  # Since here the stack pointer is 16-byte aligned, so
  # EFI_FX_SAVE_STATE_X64 of EFI_SYSTEM_CONTEXT_x64
  # is 16-byte aligned
  #

## UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
## UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
  pushq %r15
  pushq %r14
  pushq %r13
  pushq %r12
  pushq %r11
  pushq %r10
  pushq %r9
  pushq %r8
  pushq %rax
  pushq 8(%rbp)
  pushq %rdx
  pushq %rbx
  pushq 48(%rbp)
  pushq (%rbp)
  pushq %rsi
  pushq %rdi

## UINT64  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
  movzwq   56(%rbp), %rax
  pushq    %rax
  movzwq   32(%rbp), %rax
  pushq    %rax
  movq     %ds, %rax
  pushq    %rax
  movq     %es, %rax
  pushq    %rax
  movq     %fs, %rax
  pushq    %rax
  movq     %gs, %rax
  pushq    %rax

   movq     %rcx, 8(%rbp)

## UINT64  Rip;
   pushq    24(%rbp)

## UINT64  Gdtr[2], Idtr[2];
   subq     $16, %rsp
   sidt    (%rsp)
   subq     $16, %rsp
   sgdt    (%rsp)

## UINT64  Ldtr, Tr;
   xorq     %rax, %rax
  strw    %ax
  pushq    %rax
  sldtw   %ax
  pushq    %rax

## UINT64  RFlags;
   pushq    40(%rbp)

## UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
   movq    %cr8, %rax
  pushq    %rax
   movq    %cr4, %rax
   orq     $0x208, %rax
   movq    %rax, %cr4
  pushq    %rax
   movq    %cr3, %rax
  pushq    %rax
   movq    %cr2, %rax
  pushq    %rax
   xorq    %rax, %rax
  pushq    %rax
   movq    %cr0, %rax
  pushq    %rax

## UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
   movq    %dr7, %rax
  pushq    %rax
   movq    %dr6, %rax
  pushq    %rax
   movq    %dr3, %rax
  pushq    %rax
   movq    %dr2, %rax
  pushq    %rax
   movq    %dr1, %rax
  pushq    %rax
   movq    %dr0, %rax
  pushq    %rax

## FX_SAVE_STATE_X64 FxSaveState;

   subq    $512, %rsp
   movq    %rsp, %rdi
   .byte   0x0f, 0xae, 0b00000111

## UEFI calling convention for x64 requires that Direction flag in EFLAGs is clear
   cld

## UINT32  ExceptionData;
   pushq   16(%rbp)

## call into exception handler
   movq    8(%rbp), %rcx
   movq    ExternalVectorTablePtr(%rip), %rax
   movq    (%rax, %rcx, 8), %rax
   orq     %rax, %rax

  je    nonNullValue #

## Prepare parameter and call
   movq    %rsp, %rdx
  #
  # Per X64 calling convention, allocate maximum parameter stack space
  # and make sure RSP is 16-byte aligned
  #
   subq    $(4 * 8 + 8), %rsp
   call    *%rax
   addq    $(4 * 8 + 8), %rsp

nonNullValue:
  cli
## UINT64  ExceptionData;
   addq    $8, %rsp

## FX_SAVE_STATE_X64 FxSaveState;

   movq    %rsp, %rsi
   .byte   0x0f, 0xae, 0b00001110
   addq    $512, %rsp

## UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
## Skip restoration of DRx registers to support in-circuit emualators
## or debuggers set breakpoint in interrupt/exception context
   addq     $48, %rsp

## UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
   popq     %rax
   movq     %rax, %cr0
   addq     $8, %rsp
   popq     %rax
   movq     %rax, %cr2
   popq     %rax
   movq     %rax, %cr3
   popq     %rax
   movq     %rax, %cr4
   popq     %rax
   movq     %rax, %cr8

## UINT64  RFlags;
   popq    40(%rbp)

## UINT64  Ldtr, Tr;
## UINT64  Gdtr[2], Idtr[2];
## Best not let anyone mess with these particular registers...
   addq    $48, %rsp

## UINT64  Rip;
   popq    24(%rbp)

## UINT64  Gs, Fs, Es, Ds, Cs, Ss;
   popq     %rax
  # mov     gs, rax ; not for gs
   popq     %rax
  # mov     fs, rax ; not for fs
  # (X64 will not use fs and gs, so we do not restore it)
   popq     %rax
  movw    %rax, %es
   popq     %rax
  movw    %rax, %ds
   popq     32(%rbp)
   popq     56(%rbp)

## UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
## UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
   popq     %rdi
   popq     %rsi
   addq     $8, %rsp
   popq     48(%rbp)
   popq     %rbx
   popq     %rdx
   popq     %rcx
   popq     %rax
   popq     %r8
   popq     %r9
   popq     %r10
   popq     %r11
   popq     %r12
   popq     %r13
   popq     %r14
   popq     %r15

   movq     %rbp, %rsp
   popq     %rbp
   addq     $16,  %rsp
  iretq


.equ          CODE_SELECTOR, 0x38
.equ          DATA_SELECTOR, 0x30

ASM_GLOBAL ASM_PFX(InitializeGdt)
ASM_PFX(InitializeGdt):

  movabsq $Gdtr, %rdx

  movw    %cs,%ax             # Get the selector data from our code image
  movw    %ax,%ds
  lgdt    (%rdx)

SelectorRld:
  movw    $DATA_SELECTOR, %ax # Update the Base for the new selectors, too
  movw    %ax,%ds
  movw    %ax,%es
  movw    %ax,%fs
  movw    %ax,%gs
  movw    %ax,%ss

  ret

#-----------------------------------------------------------------------------;
# data
#-----------------------------------------------------------------------------;

        .p2align 4

 Gdtr:   .word GDT_END - GDT_BASE - 1
         .quad GDT_BASE

#-----------------------------------------------------------------------------;
#   global descriptor table (GDT)
#-----------------------------------------------------------------------------;

        .p2align 4

GDT_BASE:
# null descriptor
#.equ                NULL_SEL, $-GDT_BASE # Selector [0]
    .word 0 # limit 15:0
    .word 0 # base  15:0
    .byte 0 # base  23:16
    .byte 0 #
    .byte 0 # type & limit 19:16
    .byte 0 # base  31:24

# linear data segment descriptor
#.equ            LINEAR_SEL, $-GDT_BASE # Selector [0x8]
    .word 0xffff # limit 15:0
    .word 0     # base  15:0
    .byte 0     # base  23:16
    .byte 0x92  #present, ring 0, data, expand-up writable
    .byte 0xcf  # type & limit 19:16
    .byte 0     # base  31:24

# linear code segment descriptor
#.equ            LINEAR_CODE_SEL, $-GDT_BASE # Selector [0x10]
    .word 0xffff    # limit 15:0
    .word 0        # base  15:0
    .byte 0        # base  23:16
    .byte 0x9a     # present, ring 0, code, expand-up writable
    .byte 0xcf     # type & limit 19:16
    .byte 0        # base  31:24

# system data segment descriptor
#.equ            SYS_DATA_SEL, $-GDT_BASE # Selector [0x18]
    .word 0xffff   # limit 15:0
    .word 0        # base  15:0
    .byte 0        # base  23:16
    .byte 0x92    # type & limit 19:16
    .byte 0xcf
    .byte 0        # base  31:24

# system code segment descriptor
#.equ            SYS_CODE_SEL, $-GDT_BASE # Selector [0x20]
    .word 0xffff   # limit 15:0
    .word 0    # base  15:0
    .byte 0    # base  23:16
    .byte 0x9a # type & limit 19:16
    .byte 0xcf
    .byte 0    # base  31:24

# spare segment descriptor             ; Selector [0x28]
#.equ        SPARE3_SEL, $-GDT_BASE
    .word 0 # limit 15:0
    .word 0 # base  15:0
    .byte 0 # base  23:16
    .byte 0 # type & limit 19:16
    .byte 0 # base  31:24
    .byte 0

# 64bit data segment descriptor
#.equ                  LONG_MODE_DATA_SEL, $-GDT_BASE # Selector [0x30]
    .word 0xffff # limit 15:0
    .word 0    # base  15:0
    .byte 0    # base  23:16
    .byte 0x92 # type & limit 19:16
    .byte 0xcf
    .byte 0    # base  31:24

# 64bit code segment descriptor
#.equ              LONG_MODE_CODE, $-GDT_BASE # Selector [0x38]
   .word 0xffff # limit 15:0
   .word 0    # base  15:0
   .byte 0    # base  23:16
   .byte 0x9a # type & limit 19:16
   .byte 0xaf
   .byte 0    # base  31:24

# spare segment descriptor
#.equ        SPARE5_SEL, $-GDT_BASE       # Selector [0x40]
    .word 0 # limit 15:0
    .word 0 # base  15:0
    .byte 0 # base  23:16
    .byte 0 # type & limit 19:16
    .byte 0 # base  31:24
    .byte 0

GDT_END:

