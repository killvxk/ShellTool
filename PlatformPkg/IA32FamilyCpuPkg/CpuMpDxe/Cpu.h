//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file

  Include file for CPU DXE Module

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  Cpu.h

**/

#ifndef _CPU_DXE_H_
#define _CPU_DXE_H_

#include "MpCommon.h"

#define PLATFORM_DESKTOP           0
#define PLATFORM_MOBILE            1
#define PLATFORM_SERVER            2

#define EFI_CPUID_CORE_TOPOLOGY    0x0B

//
// The definitions below follow the naming rules.
//   Definitions beginning with "B_" are bits within registers
//   Definitions beginning with "N_" are the bit position
//   Definitions with "_CPUID_" are CPUID bit fields
//   Definitions with "_MSR_" are MSR bit fields
//   Definitions with "N_*_START" are the bit start position
//   Definitions with "N_*_STOP" are the bit stop position
//   Definitions with "B_*_MASK" are the bit mask for the register values
//

//
// Bit definitions for CPUID EAX = 1
//
// ECX
#define N_CPUID_MONITOR_MWAIT_SUPPORT               3
#define N_CPUID_VMX_SUPPORT                         5
#define N_CPUID_SMX_SUPPORT                         6
#define N_CPUID_EIST_SUPPORT                        7
#define N_CPUID_TM2_SUPPORT                         8
#define N_CPUID_DCA_SUPPORT                         18
#define N_CPUID_X2APIC_SUPPORT                      21
#define N_CPUID_AESNI_SUPPORT                       25
// EDX
#define N_CPUID_MCE_SUPPORT                         7
#define N_CPUID_MCA_SUPPORT                         14
#define N_CPUID_TM_SUPPORT                          29
#define N_CPUID_PBE_SUPPORT                         31

//
// Bit definitions for CPUID EAX = 6
//
// EAX
#define N_CPUID_TURBO_MODE_AVAILABLE                1

//
// Bit definitions for CPUID EAX = 80000001h
//
// EDX
#define N_CPUID_XD_BIT_AVAILABLE                    20

//
// Bit definitions for MSR_IA32_APIC_BASE (ECX = 1Bh)
//
#define N_MSR_BSP_FLAG                              8
#define B_MSR_ENABLE_X2APIC_MODE                    BIT10
#define N_MSR_ENABLE_X2APIC_MODE                    10
#define N_MSR_APIC_GLOBAL_ENABLE                    11

//
// Bit definitions for MSR_EBC_SOFT_POWERON (ECX = 2Bh)
//
#define N_MSR_INITIATOR_MCERR_DISABLE               4
#define N_MSR_INTERNAL_MCERR_DISABLE                5
#define N_MSR_BINIT_DRIVER_DISABLE                  6

//
// Bit definitions for MSR_EBC_FREQUENCY_ID (ECX = 2Ch)
//
#define N_MSR_SCALABLE_BUS_SPEED_START              16
#define N_MSR_SCALABLE_BUS_SPEED_STOP               18

//
// Bit definitions for MSR_PIC_MSG_CONTROL (ECX = 2Eh)
//
#define N_MSR_APIC_TPR_UPD_MSG_DISABLE              10

//
// Bit definitions for MSR_IA32_FEATURE_CONTROL (ECX = 3Ah)
//
#define N_MSR_FEATURE_CONTROL_LOCK                  0
#define N_MSR_ENABLE_VMX_INSIDE_SMX                 1
#define N_MSR_ENABLE_VMX_OUTSIDE_SMX                2
#define N_MSR_SENTER_LOCAL_FUNC_ENABLE_START        8
#define N_MSR_SENTER_LOCAL_FUNC_ENABLE_STOP         14
#define N_MSR_SENTER_GLOBAL_ENABLE                  15

//
// Bit definitions for MSR_VLW_CONTROL (ECX = 4Bh)
//
#define N_MSR_A20M_DISABLE                          1

//
// Bit definitions for MSR_PLATFORM_INFO (ECX = 0CEh)
//
#define N_MSR_MAX_NON_TURBO_RATIO_START             8
#define N_MSR_MAX_NON_TURBO_RATIO_STOP              15
#define B_MSR_PROGRAMMABLE_RATIO_LIMIT_TURBO_MODE   BIT28
#define B_MSR_PROGRAMMABLE_TDP_LIMIT_TURBO_MODE     BIT29
#define N_MSR_MAX_EFFICIENCY_RATIO_START            40
#define N_MSR_MAX_EFFICIENCY_RATIO_STOP             47

//
// Bit definitions for MSR_PKG_CST_CONFIG_CONTROL (ECX = 0E2h)
//
#define B_MSR_PACKAGE_C_STATE_LIMIT_MASK            0x7
#define B_MSR_IO_MWAIT_REDIRECTION_ENABLE           BIT10
#define B_MSR_C3_STATE_AUTO_DEMOTION_ENABLE         BIT25
#define B_MSR_C1_STATE_AUTO_DEMOTION_ENABLE         BIT26

//
// Bit definitions for MSR_FEATURE_CONFIG (ECX = 13Ch)
//
#define N_MSR_FEATURE_CONFIG_LOCK                   0
#define B_MSR_FEATURE_CONFIG_LOCK                   BIT0
#define N_MSR_AESNI_DISABLE                         1

//
// Bit definitions for MSR_IA32_MCG_CAP (ECX = 179h)
//
#define N_MSR_MCG_COUNT_START                       0
#define N_MSR_MCG_COUNT_STOP                        7
#define N_MSR_MCG_CTL_P                             8

//
// Bit definitions for MSR_IA32_CLOCK_MODULATION (ECX = 19Ah)
//
#define N_MSR_CLOCK_MODULATION_DUTY_CYCLE_START     0
#define N_MSR_CLOCK_MODULATION_DUTY_CYCLE_STOP      3
#define B_MSR_CLOCK_MODULATION_DUTY_CYCLE_MASK      0xF
#define N_MSR_CLOCK_MODULATION_ENABLE               4

//
// Bit definitions for MSR_IA32_MISC_ENABLE (ECX = 1A0h)
//
#define N_MSR_FAST_STRINGS_ENABLE                   0
#define N_MSR_AUTOMATIC_TCC_ENABLE                  3
#define N_MSR_HW_PREFETCHER_DISABLE                 9
#define N_MSR_FERR_MULTIPLEXING_ENABLE              10
#define N_MSR_TM2_ENABLE                            13
#define N_MSR_EIST_ENABLE                           16
#define N_MSR_ENABLE_MONITOR_FSM                    18
#define N_MSR_ADJACENT_CACHE_LINE_PREFETCH_DISABLE  19
#define N_MSR_EIST_SELECT_LOCK                      20
#define N_MSR_LIMIT_CPUID_MAXVAL                    22
#define N_MSR_XD_BIT_DISABLE                        34
#define N_MSR_DCU_PREFETCHER_DISABLE                37
#define N_MSR_TURBO_MODE_DISABLE                    38
#define N_MSR_IP_PREFETCHER_DISABLE                 39

//
// Bit definitions for MSR_MISC_FEATURE_CONTROL (ECX = 1A4h)
//
#define N_MSR_MLC_STREAMER_PREFETCHER_DISABLE       0
#define N_MSR_MLC_SPATIAL_PREFETCHER_DISABLE        1
#define N_MSR_DCU_STREAMER_PREFETCHER_DISABLE       2
#define N_MSR_DCU_IP_PREFETCHER_DISABLE             3
#define N_MSR_THREE_STRIKE_COUNTER_DISABLE          11

//
// Bit definitions for MSR_MISC_PWR_MGMT (ECX = 1AAh)
//
#define N_MSR_EIST_HW_COORDINATION_DISABLE          0
#define N_MSR_LOCK_TM_INTERRUPT                     22

//
// Bit definitions for MSR_TURBO_POWER_CURRENT_LIMIT (ECX = 1ACh)
//
#define N_MSR_TDP_LIMIT_START                       0
#define N_MSR_TDP_LIMIT_STOP                        14
#define N_MSR_TDC_LIMIT_START                       16
#define N_MSR_TDC_LIMIT_STOP                        30

//
// Bit definitions for MSR_TURBO_RATIO_LIMIT (ECX = 1ADh)
//
#define N_MSR_MAX_RATIO_LIMIT_1C_START              0
#define N_MSR_MAX_RATIO_LIMIT_1C_STOP               7

//
// Bit definitions for MSR_IA32_ENERGY_PERFORMANCE_BIAS (ECX = 1B0h)
//
#define N_MSR_POWER_POLICY_PREFERENCE_START         0
#define N_MSR_POWER_POLICY_PREFERENCE_STOP          3

//
// Bit definitions for MSR_IA32_PLATFORM_DCA_CAP (ECX = 1F8h)
//
#define N_MSR_DCA_TYPE0_ENABLE                      0

//
// Bit definitions for MSR_IA32_CPU_DCA_CAP (ECX = 1F9h)
//
#define B_MSR_DCA_TYPE0_SUPPORTED                   BIT0

//
// Bit definitions for MSR_IA32_DCA_0_CAP (ECX = 1FAh)
//
#define N_MSR_SW_BLOCK                              24

//
// Bit definitions for MSR_POWER_CTL (ECX = 1FCh)
//
#define N_MSR_BI_DIRECTIONAL_PROCHOT_ENABLE         0
#define N_MSR_C1E_ENABLE                            1
#define N_MSR_IA32_ENERGY_PERF_BIAS_ACCESS_ENABLE   18

//
// Bit definitions for MSR_PACKAGE_POWER_SKU_UNIT (ECX = 606h)
//
#define N_MSR_POWER_UNIT_START                      0
#define N_MSR_POWER_UNIT_STOP                       3

//
// Bit definitions for MSR_PACKAGE_POWER_LIMIT (ECX = 610h)
//
#define N_MSR_POWER_LIMIT_1_START                   0
#define N_MSR_POWER_LIMIT_1_STOP                    14

extern EFI_PHYSICAL_ADDRESS        mStartupVector;

/**
  Collects basic processor data for calling processor.

  This function collects basic processor data for calling processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
CollectBasicProcessorData (
  IN UINTN  ProcessorNumber
  );

/**
  Early MP Initialization.
  
  This function does early MP initialization, including MTRR sync and first time microcode load.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
EarlyMpInit (
  IN UINTN  ProcessorNumber
  );

/**
  Wakes up APs for the first time to count their number and collect BIST data.

  This function wakes up APs for the first time to count their number and collect BIST data.

**/
VOID
WakeupAPAndCollectBist (
  VOID
  );

/**
  Configures all logical processors with three-phase architecture.
  
  This function configures all logical processors with three-phase architecture.

**/
VOID
ProcessorConfigurationuration (
  VOID
  );





/**
  Label of start of AP machine check handler.

  This is just a label of start of AP machine check handler.

**/
VOID
EFIAPI
ApMachineCheckHandler (
  VOID
  );

/**
  Label of end of AP machine check handler.

  This is just a label of end of AP machine check handler.

**/
VOID
EFIAPI
ApMachineCheckHandlerEnd (
  VOID
  );

#endif
