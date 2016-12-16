/*++
Copyright (c) 2010 Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  CpuSetup.h

Abstract:

  Cpu Setup header file.
--*/

#ifndef __CPU_SETUP_H__
#define __CPU_SETUP_H__

#include <AsiaCpuProtocol.h>

//
// Defines for CPU types
//
#define CPU_TYPE_SANDYBRIDGE                    1
#define CPU_TYPE_HASWELL                        2

//
// Defines for CPU family
//
#define CPUID_FULL_FAMILY_MODEL_SANDYBRIDGE     0x000206A0  
#define CPUID_FULL_FAMILY_MODEL_HASWELL         0x000306C0  
#define CPUID_FULL_FAMILY_MODEL_SANDYBRIDGE_C0  0x000206A3  
#define CPUID_FULL_FAMILY_MODEL_SANDYBRIDGE_C1  0x000206A4  
#define CPUID_FULL_FAMILY_MODEL_SANDYBRIDGE_D0  0x000206A5  
#define CPUID_FULL_FAMILY_MODEL_SANDYBRIDGE_D1  0x000206A6  
#define CPUID_FULL_FAMILY_MODEL_SANDYBRIDGE_D2  0x000206A7  
#define CPUID_FULL_FAMILY_MODEL_HASWELL_X0      0x000306C0  


#define CPU_CACHE_TYPE_MASK                     0x1F
#define CPU_CACHE_LEVEL_MASK                    0x07
#define CPU_CACHE_ASSOCIATIVITY_MASK            0x03FF
#define CPU_CACHE_PARTITION_MASK                0x03FF
#define CPU_CACHE_LINE_SIZE_MASK                0x0FFF

#define MSR_EBC_HARD_POWERON                      0x2A

#define PRIVATE_INFO_NUM_OF_CPU_DATA            2
#define DELAY_CALCULATE_CPU_PERIOD              200
#define MSR_CORE_THREAD_COUNT                   0x35
#define CPU_EXTERNAL_CLOCK_FREQ                 0x64
#define MSR_FSB_FREQ                            0xCD
#define MSR_PLATFORM_INFO                       0xCE
#define MAX_NON_TURBO_RATIO_OFFSET              8
#define MAX_NON_TURBO_RATIO_MASK                0xff
#define MSR_PACKAGE_POWER_SKU_UNIT              0x606
#define MSR_TURBO_POWER_LIMIT                   0x610
#define MSR_PACKAGE_POWER_SKU                   0x614
#define PACKAGE_POWER_UNIT_MASK                 0x0f
#define PACKAGE_MAX_POWER_OFFSET                32
#define PACKAGE_TDP_POWER_MASK                  (0x7FFF)      // Bits 14:0
#define PACKAGE_MIN_POWER_MASK                  (0x7FFF0000)  // Bits 30:16
#define PACKAGE_MAX_POWER_MASK                  (0x7FFF)      // Bits 46:32
#define POWER_LIMIT_1_MASK                      (0x7FFF)      // Bits 14:0
#define POWER_LIMIT_2_MASK                      ((UINT64) 0x7FFF << 32) // Bits 46:32
#define POWER_LIMIT_2_OFFSET                    32
#define MAX_RATIO_LIMIT_4C_OFFSET               24
#define MAX_RATIO_LIMIT_4C_MASK                 (0xFF << MAX_RATIO_LIMIT_4C_OFFSET)
#define MAX_RATIO_LIMIT_3C_OFFSET               16
#define MAX_RATIO_LIMIT_3C_MASK                 (0xFF << MAX_RATIO_LIMIT_3C_OFFSET)
#define MAX_RATIO_LIMIT_2C_OFFSET                8
#define MAX_RATIO_LIMIT_2C_MASK                 (0xFF << MAX_RATIO_LIMIT_2C_OFFSET)
#define MAX_RATIO_LIMIT_1C_MASK                 (0xFF)
#define PACKAGE_MIN_POWER_OFFSET                 16
#define MSR_TURBO_RATIO_LIMIT                    0x1AD
#define MAX_EFFICIENCY_RATIO_OFFSET              40
#define MAX_EFFICIENCY_RATIO_MASK                0xff
#define MSR_IA32_MISC_ENABLE                     0x1A0
#define TURBO_MODE_DISABLE_BIT                   38
#define XE_CORE_RATIO_PROGRAMMABLE_BIT           28
#define XE_TDP_TDC_PROGRAMMABLE_BIT              29
#define MSR_FLEX_RATIO                           0x194
#define C1_SUB_STATES_MASK                       0x000f0
#define C3_SUB_STATES_MASK                       0x00f00
#define C6_SUB_STATES_MASK                       0x0f000
#define C7_SUB_STATES_MASK                       0xf0000
#define CPUID_FULL_FAMILY_MODEL                  0x0FFF0FF0
#define ENHANCED_CSTATE_SUPPORTED                0x2


//
// This structure is used for setup.
//
typedef struct _SETUP_CPU_FEATURES {
  UINT8   XDBitAvailable;
  UINT8   TurboModeAvailable;
  UINT8   XETdcTdpLimitAvailable;
  UINT8   XECoreRatioLimitAvailable;
  UINT8   HTAvailable;
  UINT8   MultiCoreAvailable;
  UINT8   VTAvailable;
  UINT8   TXTAvailable;
  UINT8   LimitCpuidAvailable;
  UINT8   CxAvailable;
  UINT8   C1Available;
  UINT8   C1EAvailable;
  UINT8   C3Available;
  UINT8   C6Available;
  UINT8   C7Available;
  UINT8   CxUndemotionAvailable;
  UINT8   EISTAvailable;
  UINT8   MultiSocketAvailable;
  UINT8   MultiSocketPopulated;
  UINT8   LocalX2ApicAvailable;
  UINT8   EnergyEfficientTurboAvailable;
  UINT8   XEorOCSupportAvailable;
  UINT8   ApicIdFlag;
} SETUP_CPU_FEATURES;


#ifndef VFRCOMPILE

#pragma pack(push, 1)

typedef struct _CPU_REV {
  UINT32  Cpuid;
  CHAR8   String[16];
} CPU_REV;

typedef struct _CACHE_DESCRIPTOR_INFO {
  UINT8   Desc;
  UINT8   Level;
  UINT8   Type;  // 0 - Data, 1 - Instruction, 3 - Unified
  UINT16  Size;
  UINT16  Associativity;
} CACHE_DESCRIPTOR_INFO;


typedef struct _CPU_INFO {
  UINT32  Version;
  UINT64  Features;
  CHAR8   *BrandString;
  BOOLEAN X64Supported;
  UINT8   NumSupportedCores;           // Total Number of Supported Cores in CPU Package. If Dual core, 2 cores.
  UINT8   NumSupportedThreadsPerCore;  // Number of Supported Threads per Core.
  UINT8   NumCores;                    // Number of Enabled or Active Cores. Ex.
  UINT8   NumHts;                      // Number of Hyper threading. This will be 0 or 2.
  UINT32  FsbFreq;                     // MT/s
  UINT32  IntendedFreq;                // MHz
  UINT32  ActualFreq;                  // MHz
  UINT32  Voltage;                     // Volt / 10000.
  UINT32  MicroCodeVers;
  CACHE_DESCRIPTOR_INFO *CacheInfo;
  UINT8   MaxCacheSupported;
  BOOLEAN Ht0;                         // Set if core is hyper-threadding logical CPU.
                                       // True, if not-hyperthreaded CPU.
} CPU_INFO;

typedef struct {
  UINT64  SmrrSupport:1;
  UINT64  Reserved:63;
} ADDITIONAL_FEATURE_FLAGS;

typedef struct _CPU_FEATURES {
  UINT32  FeatureEcx;
  UINT32  FeatureEdx;
  UINT32  ExtFeatureEax;
  UINT32  ExtFeatureEbx;
  UINT32  ExtFeatureEcx;
  UINT32  ExtFeatureEdx;
  ADDITIONAL_FEATURE_FLAGS Flags;
} CPU_FEATURES;

extern CPU_INFO CpuInfo;
extern SETUP_VOLATILE_DATA  gSetupVolatileData;
extern SETUP_CPU_FEATURES   SetupCpuFeatures;

VOID
InitCpuSetupInfo (
  );

VOID
InitCpuFeaturesInfo (
  );

#pragma pack(pop)
#endif
#endif
