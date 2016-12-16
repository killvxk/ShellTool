//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file
  Public include file for CPU definitions and CPU library functions that 
  apply to CPUs that fit into an LGA775 spocket.
  
  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  CpuOnlyResetLib.h

**/

#ifndef __SOCKET_LGA_775_H__
#define __SOCKET_LGA_775_H__

//
// Definition for CPUID Index
//
#define EFI_CPUID_SIGNATURE                   0x0
#define EFI_CPUID_VERSION_INFO                0x1
#define EFI_CPUID_CACHE_INFO                  0x2
#define EFI_CPUID_SERIAL_NUMBER               0x3
#define EFI_CPUID_CACHE_PARAMS                0x4
#define EFI_CPUID_EXTENDED_TOPOLOGY           0xB
#define EFI_CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_INVALID 0x0
#define EFI_CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT     0x1
#define EFI_CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_CORE    0x2

#define EFI_CPUID_EXTENDED_FUNCTION           0x80000000
#define EFI_CPUID_EXTENDED_CPU_SIG            0x80000001
#define EFI_CPUID_BRAND_STRING1               0x80000002
#define EFI_CPUID_BRAND_STRING2               0x80000003
#define EFI_CPUID_BRAND_STRING3               0x80000004
#define EFI_CPUID_ADDRESS_SIZE                0x80000008

//
// Definition for MSR address
//
#define EFI_MSR_IA32_TIME_STAMP_COUNTER       0x10
#define EFI_MSR_IA32_PLATFORM_ID              0x17
#define EFI_MSR_IA32_APIC_BASE                0x1B
#define EFI_MSR_EBC_HARD_POWERON              0x2A
#define EFI_MSR_EBC_SOFT_POWERON              0x2B
#define EFI_MSR_EBC_FREQUENCY_ID              0x2C
#define MSR_IA32_FEATURE_CONTROL              0x3A
#define EFI_MSR_IA32_BIOS_UPDT_TRIG           0x79
#define EFI_MSR_IA32_BIOS_SIGN_ID             0x8B
#define EFI_MSR_PSB_CLOCK_STATUS              0xCD
#define MSR_EXT_CONFIG                        0xEE
#define EFI_IA32_MCG_CAP                      0x179
#define EFI_IA32_MCG_CTL                      0x17B

#define EFI_MSR_IA32_PERF_STS                 0x198
#define EFI_MSR_IA32_PERF_CTL                 0x199
#define EFI_MSR_IA32_CLOCK_MODULATION         0x19A
#define MSR_IA32_THERMAL_INTERRUPT            0x19B
#define EFI_MSR_IA32_THERM_STATUS             0x19C
#define EFI_MSR_GV_THERM                      0x19D
#define MSR_IA32_MISC_ENABLE                  0x1A0
#define MSR_PIC_SENS_CFG                      0x1AA

#define EFI_IA32_MC0_CTL                      0x400
#define EFI_IA32_MC0_STATUS                   0x401
#define MSR_PECI_CONTROL                      0x5A0

//
// Definition for Local APIC registers and related values
//
#define LOCAL_APIC_LVT_TIMER                  0x320
#define LOCAL_APIC_TIMER_INIT_COUNT           0x380
#define LOCAL_APIC_TIMER_COUNT                0x390
#define LOCAL_APIC_TIMER_DIVIDE               0x3E0


#define DELIVERY_MODE_FIXED                   0x0
#define DELIVERY_MODE_LOWEST_PRIORITY         0x1
#define DELIVERY_MODE_SMI                     0x2
#define DELIVERY_MODE_REMOTE_READ             0x3
#define DELIVERY_MODE_NMI                     0x4
#define DELIVERY_MODE_INIT                    0x5
#define DELIVERY_MODE_SIPI                    0x6




#define IA32_PG_P                   1u
#define IA32_PG_RW                  (1u << 1)
#define IA32_PG_USR                 (1u << 2)
#define IA32_PG_WT                  (1u << 3)
#define IA32_PG_CD                  (1u << 4)
#define IA32_PG_A                   (1u << 5)
#define IA32_PG_D                   (1u << 6)
#define IA32_PG_PS                  (1u << 7)
#define IA32_PG_G                   (1u << 8)
#define IA32_PG_AVL1                (7u << 9)
#define IA32_PG_PAT_2M              (1u << 12)
#define IA32_PG_PAT_4K              IA32_PG_PS
#define IA32_PG_AVL2                (0x7ffull << 52)

#define IA32_CPUID_SS               0x08000000

#define IA32_MTRRCAP                0xfe

#define SMM_DEFAULT_SMBASE          0x30000
#define SMM_HANDLER_OFFSET          0x8000
#define SMM_PSD_OFFSET              0xfb00
#define SMM_CPU_STATE_OFFSET        0xfc00

#define SMM_CPU_INTERVAL            SIZE_2KB

#pragma pack (1)

typedef struct {
  UINT64                            Signature;              // Offset 0x00
  UINT16                            Reserved1;              // Offset 0x08
  UINT16                            Reserved2;              // Offset 0x0A
  UINT16                            Reserved3;              // Offset 0x0C
  UINT16                            SmmCs;                  // Offset 0x0E
  UINT16                            SmmDs;                  // Offset 0x10
  UINT16                            SmmSs;                  // Offset 0x12
  UINT16                            SmmOtherSegment;        // Offset 0x14
  UINT16                            Reserved4;              // Offset 0x16
  UINT64                            Reserved5;              // Offset 0x18
  UINT64                            Reserved6;              // Offset 0x20
  UINT64                            Reserved7;              // Offset 0x28
  UINT64                            SmmGdtPtr;              // Offset 0x30
  UINT32                            SmmGdtSize;             // Offset 0x38
  UINT32                            Reserved8;              // Offset 0x3C
  UINT64                            Reserved9;              // Offset 0x40
  UINT64                            Reserved10;             // Offset 0x48
  UINT16                            Reserved11;             // Offset 0x50
  UINT16                            Reserved12;             // Offset 0x52
  UINT32                            Reserved13;             // Offset 0x54
  UINT64                            MtrrBaseMaskPtr;        // Offset 0x58
} PROCESSOR_SMM_DESCRIPTOR;

///
///
///
#define SOCKET_LGA_775_SMM_MIN_REV_ID_IOMISC  0x30004
#define SOCKET_LGA_775_SMM_MIN_REV_ID_x64     0x30006

///
/// SMM Save State IOMisc related defines
///
//
// I/O Length BitMap
//
#define  SMM_IO_LENGTH_BYTE             0x01
#define  SMM_IO_LENGTH_WORD             0x02
#define  SMM_IO_LENGTH_DWORD            0x04

//
// I/O Instruction Type BitMap
//
#define  SMM_IO_TYPE_IN_IMMEDIATE       0x9
#define  SMM_IO_TYPE_IN_DX              0x1
#define  SMM_IO_TYPE_OUT_IMMEDIATE      0x8
#define  SMM_IO_TYPE_OUT_DX             0x0
#define  SMM_IO_TYPE_INS                0x3
#define  SMM_IO_TYPE_OUTS               0x2
#define  SMM_IO_TYPE_REP_INS            0x7 
#define  SMM_IO_TYPE_REP_OUTS           0x6

typedef union {
  struct {
    UINT32  SmiFlag:1;
    UINT32  Length:3;
    UINT32  Type:4;
    UINT32  Reserved1:8;
    UINT32  Port:16;
  } Bits;
  UINT32  Uint32;
} SOCKET_LGA_775_SMM_CPU_STATE_IOMISC;

/// 
/// CPU save-state strcuture for IA32.
///
typedef struct {
  UINT8   Reserved[0x200];  // fc00h
  UINT8   Reserved1[0xf8];  // fe00h
  UINT32  SMBASE;           // fef8h
  UINT32  SMMRevId;         // fefch
  UINT16  IORestart;        // ff00h
  UINT16  AutoHALTRestart;  // ff02h
  UINT32  IEDBASE;          // ff04h
  UINT8   Reserved2[0x98];  // ff08h
  UINT32  IOMemAddr;        // ffa0h
  UINT32  IOMisc;           // ffa4h
  UINT32  _ES;
  UINT32  _CS;
  UINT32  _SS;
  UINT32  _DS;
  UINT32  _FS;
  UINT32  _GS;
  UINT32  Reserved3;
  UINT32  _TR;
  UINT32  _DR7;
  UINT32  _DR6;
  UINT32  _EAX;
  UINT32  _ECX;
  UINT32  _EDX;
  UINT32  _EBX;
  UINT32  _ESP;
  UINT32  _EBP;
  UINT32  _ESI;
  UINT32  _EDI;
  UINT32  _EIP;
  UINT32  _EFLAGS;
  UINT32  _CR3;
  UINT32  _CR0;
} SOCKET_LGA_775_SMM_CPU_STATE32;

/// 
/// CPU save-state strcuture for X64.
///
typedef struct {
  UINT8   Reserved1[0x1d0];  // fc00h
  UINT32  GdtBaseHiDword;    // fdd0h
  UINT32  LdtBaseHiDword;    // fdd4h
  UINT32  IdtBaseHiDword;    // fdd8h
  UINT8   Reserved2[0xc];    // fddch
  UINT64  IO_EIP;            // fde8h
  UINT8   Reserved3[0x50];   // fdf0h
  UINT32  _CR4;              // fe40h
  UINT8   Reserved4[0x48];   // fe44h
  UINT32  GdtBaseLoDword;    // fe8ch
  UINT32  GdtLimit;          // fe90h
  UINT32  IdtBaseLoDword;    // fe94h
  UINT32  IdtLimit;          // fe98h
  UINT32  LdtBaseLoDword;    // fe9ch
  UINT32  LdtLimit;          // fea0h
  UINT32  LdtInfo;           // fea4h
  UINT8   Reserved5[0x50];   // fea8h
  UINT32  SMBASE;            // fef8h
  UINT32  SMMRevId;          // fefch
  UINT16  IORestart;         // ff00h
  UINT16  AutoHALTRestart;   // ff02h
  UINT32  IEDBASE;           // ff04h
  UINT8   Reserved6[0x14];   // ff08h
  UINT64  _R15;              // ff1ch
  UINT64  _R14;
  UINT64  _R13;
  UINT64  _R12;
  UINT64  _R11;
  UINT64  _R10;
  UINT64  _R9;
  UINT64  _R8;
  UINT64  _RAX;              // ff5ch
  UINT64  _RCX;
  UINT64  _RDX;
  UINT64  _RBX;
  UINT64  _RSP;
  UINT64  _RBP;
  UINT64  _RSI;
  UINT64  _RDI;
  UINT64  IOMemAddr;         // ff9ch
  UINT32  IOMisc;            // ffa4h
  UINT32  _ES;               // ffa8h
  UINT32  _CS;
  UINT32  _SS;
  UINT32  _DS;
  UINT32  _FS;
  UINT32  _GS;
  UINT32  _LDTR;             // ffc0h
  UINT32  _TR;
  UINT64  _DR7;              // ffc8h
  UINT64  _DR6;
  UINT64  _RIP;              // ffd8h
  UINT64  IA32_EFER;         // ffe0h
  UINT64  _RFLAGS;           // ffe8h
  UINT64  _CR3;              // fff0h
  UINT64  _CR0;              // fff8h
} SOCKET_LGA_775_SMM_CPU_STATE64;

///
/// Union of CPU save-state strcutures for IA32 and X64.
///
typedef union  {
  SOCKET_LGA_775_SMM_CPU_STATE32  x86;
  SOCKET_LGA_775_SMM_CPU_STATE64  x64;
} SOCKET_LGA_775_SMM_CPU_STATE;

//
// Definition for IA32 microcode format
//
typedef struct {
  UINT32  HeaderVersion;
  UINT32  UpdateRevision;
  UINT32  Date;
  UINT32  ProcessorId;
  UINT32  Checksum;
  UINT32  LoaderRevision;
  UINT32  ProcessorFlags;
  UINT32  DataSize;
  UINT32  TotalSize;
  UINT8   Reserved[12];
} EFI_CPU_MICROCODE_HEADER;

typedef struct {
  UINT32  ExtendedSignatureCount;
  UINT32  ExtendedTableChecksum;
  UINT8   Reserved[12];
} EFI_CPU_MICROCODE_EXTENDED_TABLE_HEADER;

typedef struct {
  UINT32  ProcessorSignature;
  UINT32  ProcessorFlag;
  UINT32  ProcessorChecksum;
} EFI_CPU_MICROCODE_EXTENDED_TABLE;

#pragma pack ()

#endif   
