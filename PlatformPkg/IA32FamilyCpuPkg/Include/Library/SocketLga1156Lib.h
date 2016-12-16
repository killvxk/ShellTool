//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file
  Public include file for CPU definitions and CPU library functions that 
  apply to CPUs that fit into an LGA1156 spocket.
  
  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  SocketLga1156.h

**/

#ifndef _SOCKET_LGA_1156_H_
#define _SOCKET_LGA_1156_H_

#define EFI_CPUID_MONITOR_MWAIT_PARAMS               0x5

//
// Processor Family and Model information.
//
#define NEHALEM_FAMILY_ID             0x06

#define BLOOMFIELD_MODEL_ID           0x1A
#define LYNNFIELD_MODEL_ID            0x1E
#define CLARKDALE_MODEL_ID            0x25
#define TUNNELCREEK_MODEL_ID          0x26
#define CEDARTRIAL_MODEL_ID           0x36
#define NEHALEM_EX_MODEL_ID           0x2E
#define WESTMERE_EX_MODEL_ID          0x2F
#define SANDYBRIDGE_CLIENT_MODEL_ID   0x2A
#define SANDYBRIDGE_SERVER_MODEL_ID   0x2D

#define VALLEYVIEW_MODEL_ID           0x37
#define AVOTON_MODEL_ID               0x4D

#define IVYBRIDGE_CLIENT_MODEL_ID     0x3A
#define IVYBRIDGE_SERVER_MODEL_ID     0x3E

#define HASWELL_CLIENT_MODEL_ID       0x3C
#define HASWELL_SERVER_MODEL_ID       0x3F

#define IS_NEHALEM_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == BLOOMFIELD_MODEL_ID || \
    ModelId == LYNNFIELD_MODEL_ID || \
    ModelId == CLARKDALE_MODEL_ID \
    ) \
   )

#define IS_NEHALEM_SERVER_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == NEHALEM_EX_MODEL_ID || \
    ModelId == WESTMERE_EX_MODEL_ID \
    ) \
   )


#define IS_SANDYBRIDGE_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == SANDYBRIDGE_CLIENT_MODEL_ID || \
    ModelId == SANDYBRIDGE_SERVER_MODEL_ID \
    ) \
   )

#define IS_SANDYBRIDGE_SERVER_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == SANDYBRIDGE_SERVER_MODEL_ID \
    ) \
   )

 #define IS_SILVERMONT_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == VALLEYVIEW_MODEL_ID || \
    ModelId == AVOTON_MODEL_ID \
    ) \
   )

 #define IS_VALLEYVIEW_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == VALLEYVIEW_MODEL_ID \
    ) \
   )

 #define IS_AVOTON_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == AVOTON_MODEL_ID \
    ) \
   )

#define IS_IVYBRIDGE_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == IVYBRIDGE_CLIENT_MODEL_ID || \
    ModelId == IVYBRIDGE_SERVER_MODEL_ID \
    ) \
   )

#define IS_IVYBRIDGE_SERVER_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == IVYBRIDGE_SERVER_MODEL_ID \
    ) \
   )

#define IS_TUNNELCREEK_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == TUNNELCREEK_MODEL_ID \
    ) \
   )

#define IS_CEDARTRIAL_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == CEDARTRIAL_MODEL_ID \
    ) \
   )

#define IS_HASWELL_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == HASWELL_CLIENT_MODEL_ID || \
    ModelId == HASWELL_SERVER_MODEL_ID \
    ) \
   )

#define IS_HASWELL_SERVER_PROC(FamilyId, ModelId) \
  (FamilyId == NEHALEM_FAMILY_ID && \
   ( \
    ModelId == HASWELL_SERVER_MODEL_ID \
    ) \
   )

//
// Socket LGA1156 and LGA1366 based MSR definitions.
//
#define MSR_MONITOR_FILTER_SIZE             0x06
#define MSR_CHL_CONTROLS                    0x2D
#define MSR_PIC_MSG_CONTROL                 0x2E
#define MSR_DCU_MODE                        0x31
#define MSR_CORE_THREAD_COUNT               0x35
#define MSR_SOCKET_ID                       0x39
#define MSR_VLW_CONTROL                     0x4B
#define MSR_QUIESCE_CONTROL                 0x50
#define MSR_PLATFORM_INFO                   0xCE
#define MSR_PMG_CST_CONFIG_CONTROL          0xE2
#define MSR_PMG_IO_CAPTURE_BASE             0xE4
#define MSR_CPUID1_FEATURE_MASK             0x130
#define MSR_CPUID80000001_FEATURE_MASK      0x131
#define MSR_FEATURE_CONFIG                  0x13C
#define MSR_PCID_ENABLE_FEATURE             0x13D
#define MSR_MCG_CONTAIN                     0x178
#define MSR_FLEX_RATIO                      0x194
#define MSR_IA32_CLOCK_MODULATION           0x19A
#define MSR_TEMPERATURE_TARGET              0x1A2
#define MSR_MISC_FEATURE_CONTROL            0x1A4
#define MSR_MISC_PWR_MGMT                   0x1AA
#define MSR_TURBO_POWER_CURRENT_LIMIT       0x1AC
#define MSR_TURBO_RATIO_LIMIT               0x1AD
#define MSR_IA32_ENERGY_PERFORMANCE_BIAS    0x1B0
#define MSR_VLW_CAPABILITY                  0x1F0
#define MSR_FERR_CAPABILITY                 0x1F1
#define MSR_SMRR_PHYSBASE                   0x1F2
#define MSR_SMRR_PHYSMASK                   0x1F3
#define MSR_EMRR_PHYSBASE                   0x1F4
#define MSR_EMRR_PHYSMASK                   0x1F5
#define MSR_IA32_PLATFORM_DCA_CAP           0x1F8
#define MSR_IA32_CPU_DCA_CAP                0x1F9
#define MSR_IA32_DCA_0_CAP                  0x1FA
#define MSR_POWER_CTL                       0x1FC
#define MSR_NO_EVICT_MODE                   0x2E0
#define MSR_UNCORE_CR_MEMLOCK_COMMANDS      0x2E2
#define MSR_UNCORE_CR_UNCORE_MC_CFG_CONTROL 0x2E3
#define MSR_PCIEXBAR_MSR                    0x300
#define MSR_PKG_C3_RESIDENCY                0x3F8
#define MSR_PKG_C6_RESIDENCY                0x3F9
#define MSR_PKG_C7_RESIDENCY                0x3FA
#define MSR_CORE_C3_RESIDENCY               0x3FC
#define MSR_CORE_C6_RESIDENCY               0x3FD
#define MSR_CORE_C7_RESIDENCY               0x3FE

#define MSR_PACKAGE_POWER_LIMIT             0x610
#define MSR_PACKAGE_POWER_SKU_UNIT          0x606

#define MSR_IACORE_RATIOS                   0x66A
#define MSR_IACORE_VIDS                     0x66B
#define MSR_IACORE_TURBO_RATIOS             0x66C
#define MSR_IACORE_TURBO_VIDS               0x66D

#endif
