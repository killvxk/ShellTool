# @file
#
# Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
# All rights reserved.This software and associated documentation (if any)
# is furnished under a license and may only be used or copied in
# accordance with the terms of the license. Except as permitted by such
# license, no part of this software or documentation may be reproduced,
# stored in a retrieval system, or transmitted in any form or by any
# means without the express written consent of Byosoft Corporation.
#
# File Name:
#   PlatformPkg.dsc
#
# Abstract:
#   Platform Configuration File
#
# Revision History:
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                       = PlatformPkg
  PLATFORM_GUID                       = 465B0A0B-7AC1-443b-8F67-7B8DEC145F90
  PLATFORM_VERSION                    = 0.1
  DSC_SPECIFICATION                   = 0x00010005
  OUTPUT_DIRECTORY                    = Build/$(PLATFORM_PACKAGE)
  SUPPORTED_ARCHITECTURES             = IA32|X64
  BUILD_TARGETS                       = DEBUG|RELEASE
  SKUID_IDENTIFIER                    = DEFAULT
  FLASH_DEFINITION                    = $(PLATFORM_PACKAGE)/PlatformX64Pkg.fdf

  DEFINE      PLATFORM_PACKAGE        = PlatformPkg
  DEFINE      ASIA_SOURCE             = AsiaPkg/Asia

!include PlatformAutoGen.dsc



#-------------------------------------------------------------------------------
# Set CSM_ENABLE value:
# TRUE  - CSM is always enabled
# FALSE - CSM is always disabled
# 0x2 - User can choose CSM enabled or disabled in BIOS setup
DEFINE CSM_ENABLE = 0x2
DEFINE PERFORMANCE_ENABLE = FALSE
DEFINE S3_ENABLE = TRUE
DEFINE ACPI_ENABLE = TRUE
DEFINE ACPI50_ENABLE = TRUE
DEFINE CAPSULE_ENABLE = FALSE
DEFINE BEEP_STATUS_CODE_ENABLE = TRUE
DEFINE POSTCODE_STATUS_CODE_ENABLE = TRUE
DEFINE MEMORY_TEST_ENABLE = FALSE
DEFINE TPM12_ENABLE = FALSE
DEFINE TPM20_ENABLE = FALSE
DEFINE TCM_ENABLE   = FALSE
DEFINE SECURE_BOOT_ENABLE = FALSE
DEFINE CRYPTO_ENABLE = TRUE
DEFINE RECOVERY_ENABLE = FALSE
DEFINE RECOVERY_CDROM_ENABLE = FALSE
DEFINE MTC_USE_CMOS = TRUE
DEFINE USE_PREBUILD_NETWORK_STACK = TRUE
DEFINE NVME_SUPPORT = FALSE
#
DEFINE ZX_SECRET_CODE = TRUE
#
#
# TARGET controls the compiler option to enable source level debug.
# DEBUG_BIOS_ENABLE flag enables DEBUG message and disable optimization.
#
# TARGET    DEBUG_BIOS_ENABLE    BiosImage
# DEBUG     TRUE                 Image supports easy source level debug, and have debug message.
# DEBUG     FALSE                Image supports source level debug, but no debug message.
# RELEASE   FALSE                Image without source level debug and debug message.
# RELEASE   TRUE                 Image without source level debug, but has debug message.
#
!if $(TARGET) == DEBUG
# Disable this flag when to debug image without debug message.
DEFINE DEBUG_BIOS_ENABLE = TRUE
!else
DEFINE DEBUG_BIOS_ENABLE = FALSE
!endif




DEFINE SECURE_KEY_PATH   = SecurityPkg/VariableAuthenticated/SecureKey
DEFINE XHCI_FW_PATH_FILE = AsiaPkg/FW/CND001-MCU_FW01060000.bin

!if $(DEBUG_BIOS_ENABLE) == TRUE
  DEFINE DEBUG_MESSAGE_ENABLE     = TRUE
  DEFINE OPTIMIZE_COMPILER_ENABLE = FALSE
!else
  DEFINE DEBUG_MESSAGE_ENABLE     = FALSE
  DEFINE OPTIMIZE_COMPILER_ENABLE = TRUE
!endif









#-------------------------------------------------------------------------------
# Set the global variables for EDK sytle module EDK_GLOBAL only takes effect to R8 INF.
  EDK_GLOBAL ASIA_SOURCE  = $(ASIA_SOURCE)
  EDK_GLOBAL ASIA_MBTYPE  = $(ASIA_MBTYPE)
  EDK_GLOBAL ASIA_SBTYPE  = $(ASIA_NBTYPE)
  EDK_GLOBAL ASIA_CPUTYPE = $(ASIA_CPUTYPE)
  EDK_GLOBAL UEFI_PREFIX  = Uefi
  EDK_GLOBAL PI_PERFIX    =

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
#                              Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################
[LibraryClasses.common]
  #
  # Entry point
  #
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf

  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|IntelFrameworkModulePkg/Library/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf

  #
  # Framework
  #
!if $(S3_ENABLE) == TRUE
  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf
!else
  S3BootScriptLib|MdePkg/Library/BaseS3BootScriptLibNull/BaseS3BootScriptLibNull.inf
!endif


  #
  # Generic Modules
  #
  GenericBdsLib|IntelFrameworkModulePkg/Library/GenericBdsLib/GenericBdsLib.inf
  PlatformBdsLib|IntelFrameworkModulePkg\Library\PlatformBdsLibNull\PlatformBdsLibNull.inf
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  TcpIoLib|MdeModulePkg/Library/DxeTcpIoLib/DxeTcpIoLib.inf
  DpcLib|MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf

  RecoveryLib|IntelFrameworkModulePkg/Library/PeiRecoveryLib/PeiRecoveryLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
!if $(CAPSULE_ENABLE) == TRUE
  CapsuleLib|IntelFrameworkModulePkg/Library/DxeCapsuleLib/DxeCapsuleLib.inf
!else
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
!endif
  LogoLib|$(PLATFORM_PACKAGE)/Library/LogoLib/LogoLib.inf
  UefiBootManagerLib|$(PLATFORM_PACKAGE)/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  LegacyBootManagerLib|$(PLATFORM_PACKAGE)/Library/LegacyBootManagerLib/LegacyBootManagerLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  SmmCorePlatformHookLib|MdeModulePkg/Library/SmmCorePlatformHookLibNull/SmmCorePlatformHookLibNull.inf
  IoApicLib|PcAtChipsetPkg/Library/BaseIoApicLib/BaseIoApicLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf

  #
  # CPU
  #
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf

  #
  # Platform
  #
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  TimerLib|$(PLATFORM_PACKAGE)/Library/AcpiTimerLib/AcpiTimerLib.inf
  PlatformSecLib|$(PLATFORM_PACKAGE)/Library/PlatformSecLib/PlatformSecLib.inf
  SmbusLib|$(PLATFORM_PACKAGE)/Library/SmbusLib/SmbusLib.inf
  SmmLib|$(PLATFORM_PACKAGE)/Library/SmmLib/SmmLib.inf
  BeepLib|$(PLATFORM_PACKAGE)/Library/BeepLib/BeepLib.inf
  PlatformCommLib|$(PLATFORM_PACKAGE)/Platform/Library/PlatformCommLib.inf
  ResetSystemLib|$(PLATFORM_PACKAGE)/Library/ResetSystemLib/ResetSystemLib.inf
  
  
  #
  # Misc
  #
!if $(DEBUG_MESSAGE_ENABLE) == FALSE
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
!else
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
!endif
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf

!if $(CRYPTO_ENABLE) == TRUE
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
!endif

!if $(SECURE_BOOT_ENABLE) == TRUE
  PlatformSecureLib|SecurityPkg/Library/PlatformSecureLib/PlatformSecureLib.inf
!endif

!if $(TPM12_ENABLE) == TRUE
  TpmCommLib|SecurityPkg/Library/TpmCommLib/TpmCommLib.inf
!endif
!if $(TPM20_ENABLE) == TRUE
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  Tcg2PpVendorLib|SecurityPkg/Library/Tcg2PpVendorLibNull/Tcg2PpVendorLibNull.inf
!endif

  TcgPhysicalPresenceLib|SecurityPkg/Library/DxeTcgPhysicalPresenceNullLib/DxeTcgPhysicalPresenceLib.inf



[LibraryClasses.IA32]
  #
  # PEI phase common
  #
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf

!if $(CRYPTO_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
!endif
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf
  BiosIdLib|$(PLATFORM_PACKAGE)/Override/ByoModulePkg/Library/BiosIdLib/Pei/BiosIdPeiLib.inf
  MultiPlatSupportLib|$(PLATFORM_PACKAGE)/Library/PeiMultiPlatSupportLib/PeiMultiPlatSupportLib.inf


[LibraryClasses.IA32.SEC]
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  PlatformHookLib|$(PLATFORM_PACKAGE)/Library/PlatformHookLib/PlatformHookLib.inf
!else
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
!endif



[LibraryClasses.IA32.PEI_CORE]
!if $(PERFORMANCE_ENABLE) == TRUE
  PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  TimerLib|$(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/Library/CpuLocalApicTimerLib/CpuLocalApicTimerLib.inf
!endif

[LibraryClasses.X64]
  #
  # DXE phase common
  #
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
!if $(CRYPTO_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!endif
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
  SpiFlashLib|$(PLATFORM_PACKAGE)/Override/ByoNvMediaPkg/Library/ChipsetSpiFlashLib/SpiFlashLib.inf
  BiosIdLib|ByoModulePkg/Library/BiosIdLib/Dxe/BiosIdDxeLib.inf

[LibraryClasses.X64.PEIM]
!if $(DEBUG_MESSAGE_ENABLE) == FALSE
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!else
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
!endif
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

[LibraryClasses.X64.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
!if $(PERFORMANCE_ENABLE) == TRUE
  PerformanceLib|MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
  TimerLib|$(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/Library/CpuLocalApicTimerLib/CpuLocalApicTimerLib.inf
!endif

[LibraryClasses.X64.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxSmmLib.inf
  SetMemAttributeSmmLib|$(PLATFORM_PACKAGE)/Library/SetMemAttributeSmmLib/SetMemAttributeSmmLib.inf

!if $(CRYPTO_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
!endif

[LibraryClasses.X64.SMM_CORE]
  MemoryAllocationLib|MdeModulePkg/Library/PiSmmCoreMemoryAllocationLib/PiSmmCoreMemoryAllocationLib.inf
  SmmServicesTableLib|MdeModulePkg/Library/PiSmmCoreSmmServicesTableLib/PiSmmCoreSmmServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
!if $(CRYPTO_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
!endif

[LibraryClasses.X64.DXE_RUNTIME_DRIVER]
  ReportStatusCodeLib|MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf
!if $(CRYPTO_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
!endif

[LibraryClasses.X64.UEFI_DRIVER]
# PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

[LibraryClasses.X64.UEFI_APPLICATION]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|ShellPkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  SortLib|ShellPkg/Library/UefiSortLib/UefiSortLib.inf

################################################################################
#
# Library Section - list of all EDKFramework libraries
#
################################################################################
[Libraries.common]
  $(ASIA_SOURCE)/Foundation/Library/AsiaIoLib/AsiaIoLib.inf
  $(ASIA_SOURCE)/Foundation/Library/AsiaIoSaveLib/AsiaIoSaveLib.inf   
  $(ASIA_SOURCE)/Porting/Library/Cpu/$(ASIA_CPUTYPE)/AsiaCpuPortingLib.inf
#  $(ASIA_SOURCE)/PLATFORM/$(ASIA_MBTYPE)/AsiaPlatformLib.inf
  $(ASIA_SOURCE)/PLATFORM/$(ASIA_MBTYPE)/AsiaPlatformLib.inf
#  
  EdkCompatibilityPkg/Foundation/Guid/EdkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Cpu/Pentium/CpuIA32Lib/CpuIA32Lib.inf
  EdkCompatibilityPkg/Foundation/Library/CompilerStub/CompilerStubLib.inf
  EdkCompatibilityPkg/Foundation/Efi/Guid/EfiGuidLib.inf  
  EdkCompatibilityPkg/Foundation/Framework/Guid/EdkFrameworkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Library/EfiCommonLib/EfiCommonLib.inf  


  
  
[Libraries.IA32]
  $(ASIA_SOURCE)/Interface/PPI/AsiaPpiLib.inf
!ifdef ZX_DDR3  
!ifdef CHX001_HAPS
  $(ASIA_SOURCE)/Porting/Pei/Dram-ddr3/$(ASIA_NBTYPE)/AsiaDramPeimlib.inf
!else
  !ifdef CHX001_A0
    $(ASIA_SOURCE)/Porting/Pei/Dram-ddr3/$(ASIA_NBTYPE)/AsiaDramPeimlib.inf
  !endif
!endif  
!else
  ### DDR4 as default.
!ifdef CHX001_HAPS
  $(ASIA_SOURCE)/Porting/Pei/Dram-ddr4/$(ASIA_NBTYPE)/AsiaDramPeimlib.inf
!else
  !ifdef CHX001_A0
    $(ASIA_SOURCE)/Porting/Pei/Dram-ddr4/$(ASIA_NBTYPE)/AsiaDramPeimlib.inf
  !endif
!endif  
!endif  
#  $(ASIA_SOURCE)/Porting/Pei/Dram/$(ASIA_NBTYPE)/AsiaDramPeimlib.inf
#
  $(ASIA_SOURCE)/Porting/Pei/SB/$(ASIA_NBTYPE)/AsiaSbPeimLib.inf
  $(ASIA_SOURCE)/Porting/Pei/NB/$(ASIA_NBTYPE)/AsiaNbPeimLib.inf
  EdkCompatibilityPkg/Foundation/Ppi/EdkPpiLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Ppi/EdkFrameworkPpiLib.inf  
  EdkCompatibilityPkg/Foundation/Library/Pei/PeiLib/PeiLib.inf  
  EdkCompatibilityPkg/Foundation/Library/Pei/Hob/PeiHobLib.inf


[Libraries.X64]
  $(ASIA_SOURCE)/Interface/Protocol/AsiaProtocolLib.inf
  $(ASIA_SOURCE)/Porting/Dxe/NB/$(ASIA_NBTYPE)/AsiaNbDxePortingLib.inf
  $(ASIA_SOURCE)/Porting/Dxe/SB/$(ASIA_SBTYPE)/AsiaSbDxePortingLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Protocol/EdkFrameworkProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Protocol/EdkProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiDriverLib/EfiDriverLib.inf
  EdkCompatibilityPkg/Foundation/Library/RuntimeDxe/EfiRuntimeLib/EfiRuntimeLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiScriptLib/EfiScriptLib.inf
  EdkCompatibilityPkg/Foundation/Efi/Protocol/EfiProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Core/Dxe/ArchProtocol/ArchProtocolLib.inf  




################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFeatureFlag.common]
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdSkipReconnectFirstIdeHost|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetActiveForAhciCommand|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdScsiTestUnitReadyReadBufferPatch|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDetectIdeDeviceUseDiagCmd|TRUE
  gEfiByoModulePkgTokenSpaceGuid.PcdStatusCodeUseOem|FALSE
  gEfiByoModulePkgTokenSpaceGuid.PcdStatusCodePrintProgressCode|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPs2KeyboardRxAndDiscardPs2MouseData|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdUgaConsumeSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSkipSmmCommunicateWorkWhenS3Dxe|TRUE

!if $(CAPSULE_ENABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|TRUE
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|FALSE
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdFrameworkCompatibilitySupport|FALSE
  gEfiCpuTokenSpaceGuid.PcdCpuSmmEnableBspElection|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreImageLoaderSearchTeSectionFirst|FALSE
!if $(DEBUG_MESSAGE_ENABLE) == FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|FALSE
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|TRUE
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseMemory|FALSE
!if $(POSTCODE_STATUS_CODE_ENABLE) == TRUE
  gEfiByoModulePkgTokenSpaceGuid.PcdStatusCodeUsePostCode|TRUE
!else
  gEfiByoModulePkgTokenSpaceGuid.PcdStatusCodeUsePostCode|FALSE
!endif
!if $(BEEP_STATUS_CODE_ENABLE) == TRUE
  gEfiByoModulePkgTokenSpaceGuid.PcdStatusCodeUseBeep|TRUE
!else
  gEfiByoModulePkgTokenSpaceGuid.PcdStatusCodeUseBeep|FALSE
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableCollectStatistics|FALSE

[PcdsFeatureFlag.X64]
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiSkipPciCmdEnCheck|TRUE

[PcdsFixedAtBuild.common]
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xE0000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|L"Byosoft"  

!if $(SECURE_BOOT_ENABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x10000
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x4000
!endif
  gEfiCpuTokenSpaceGuid.PcdCpuMaxLogicalProcessorNumber|8
  gEfiCpuTokenSpaceGuid.PcdCpuSmmApSyncTimeout|1000
  gEfiMdeModulePkgTokenSpaceGuid.PcdHwErrStorageSize|0x00000800
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxHardwareErrorVariableSize|0x400
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPeimPerFv|40
  gEfiMdeModulePkgTokenSpaceGuid.PcdSrIovSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdAriSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPpiSupported|80
  
!if $(TARGET) == DEBUG
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
!else  
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|TRUE
!endif  

  gEfiCpuTokenSpaceGuid.PcdCpuSmmStackSize|0x8000
  gEfiCpuTokenSpaceGuid.PcdCpuApStackSize|0x8000

!if $(DEBUG_MESSAGE_ENABLE) == FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x03
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2F
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
!endif
!if $(PERFORMANCE_ENABLE) == TRUE
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0x1
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxPeiPerformanceLogEntries|30
!endif
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdEbdaReservedMemorySize|0x20000
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdHighPmmMemorySize|0x400000
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdBiosVideoCheckVbeEnable|TRUE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdBiosVideoCheckVgaEnable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdUsbSerialEhciBus|0x00
  gEfiMdeModulePkgTokenSpaceGuid.PcdUsbSerialEhciDev|0x10
  gEfiMdeModulePkgTokenSpaceGuid.PcdUsbSerialEhciFunc|0x07
  gEfiByoModulePkgTokenSpaceGuid.PcdRecoveryFindBiosIdFirstTryTopFv|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSwSmiCmdPort|0x082F
  
#  gPlatformModuleTokenSpaceGuid.PcdTemporaryRamBase|0xFEF00000
  gPlatformModuleTokenSpaceGuid.PcdTemporaryRamBase|0xFC000000
#  gPlatformModuleTokenSpaceGuid.PcdTemporaryRamSize|0x00004000 
  gPlatformModuleTokenSpaceGuid.PcdTemporaryRamSize|0x00008000 
  gEfiCpuTokenSpaceGuid.PcdPeiTemporaryRamStackSize|0x00001800  
  
  gEfiMdeModulePkgTokenSpaceGuid.PcdShadowPeimOnS3Boot|TRUE
  
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemId|"_BYO_ "
  
  gPerformancePkgTokenSpaceGuid.PcdPerfPkgAcpiIoPortBaseAddress|0x800
  gPlatformModuleTokenSpaceGuid.AcpiIoPortBaseAddress|0x800
  
[PcdsFixedAtBuild.X64]
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiCom1Enable|TRUE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiCom2Enable|FALSE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiPs2KeyboardEnable|TRUE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiPs2MouseEnable|FALSE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiFloppyAEnable|FALSE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiFloppyBEnable|FALSE
  gEfiNvMediaDeviceTokenSpaceGuid.PcdNvMediaDeviceNumbers|0x1
  gPcAtChipsetPkgTokenSpaceGuid.Pcd8259LegacyModeMask|0x0eB8

[PcdsPatchableInModule.common]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80080046
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdLegacyBiosCacheLegacyRegion|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareRevision|0x00010000
  gEfiByoModulePkgTokenSpaceGuid.PcdBiosFileExt|L"bin"

[PcdsDynamicHii.common.DEFAULT]
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|5 # Variable: L"Timeout"
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdHardwareErrorRecordLevel|L"HwErrRecSupport"|gEfiGlobalVariableGuid|0x0|1 # Variable: L"HwErrRecSupport"
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdBootState|L"BootState"|gEfiBootStateGuid|0x0|FALSE

[PcdsDynamicDefault.common.DEFAULT]
  gEfiMdeModulePkgTokenSpaceGuid.PcdS3BootScriptTablePrivateDataPtr|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|1024
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|768








#------------------------------------------------------------------------------
[Components.IA32]
  $(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/SecCore/SecCore.inf

  MdeModulePkg/Core/Pei/PeiMain.inf {
    <LibraryClasses>
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
  }

  MdeModulePkg/Universal/Pcd/Pei/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  
  UefiCpuPkg/CpuIoPei/CpuIoPei.inf  
  
  $(PLATFORM_PACKAGE)/Platform/EarlyPei/PlatformEarlyPei.inf {
    <LibraryClasses>
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
  }

  MdeModulePkg/Universal/ReportStatusCodeRouter/Pei/ReportStatusCodeRouterPei.inf
  ByoModulePkg/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf

  $(ASIA_SOURCE)/Foundation/Pei/Cpu/AsiaCpuPeim.inf
  $(ASIA_SOURCE)/Foundation/Pei/Dram/AsiaDramPeim.inf
  $(ASIA_SOURCE)/Foundation/Pei/NB/AsiaNbPeim.inf
  $(ASIA_SOURCE)/Foundation/Pei/SB/AsiaSbPeim.inf

  MdeModulePkg/Universal/FaultTolerantWritePei/FaultTolerantWritePei.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  SecurityPkg/VariableAuthenticated/Pei/VariablePei.inf
!else
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
!endif
  $(PLATFORM_PACKAGE)/Platform/Pei/PlatformPei.inf {
    <LibraryClasses>
!if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
    TimerLib|$(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/Library/CpuLocalApicTimerLib/CpuLocalApicTimerLib.inf
!endif  
  }

  $(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/CpuMpPei/CpuMpPei.inf  
  $(PLATFORM_PACKAGE)/Platform/Pei/SmmRebaseS3.inf
  $(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/PiSmmCommunication/PiSmmCommunicationPei.inf

  UefiCpuPkg/Universal/Acpi/S3Resume2Pei/S3Resume2Pei.inf
!if $(CAPSULE_ENABLE) == TRUE
  $(PLATFORM_PACKAGE)/Override/MdeModulePkg/Universal/CapsulePei/CapsulePei.inf
!endif
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf
  
!if $(RECOVERY_ENABLE) == TRUE
  $(PLATFORM_PACKAGE)/Platform/Pei/Usb/UsbController.inf
  # Pei Recovery Driver
  ByoModulePkg/Universal/Disk/FatPei/FatPei.inf
  ByoModulePkg/Bus/Ata/AtaBusPei/AtaBusPei.inf
  $(PLATFORM_PACKAGE)/Override/ByoModulePkg/CrisisRecovery/ModuleRecoveryPei/ModuleRecoveryPei.inf
  MdeModulePkg/Bus/Pci/EhciPei/EhciPei.inf
  MdeModulePkg/Bus/Usb/UsbBotPei/UsbBotPei.inf
  $(PLATFORM_PACKAGE)/Override/MdeModulePkg/Bus/Usb/UsbBusPei/UsbBusPei.inf
  $(PLATFORM_PACKAGE)/Override/ByoModulePkg/Bus/Pci/XhciPei/XhciPei.inf

  MdeModulePkg/Bus/Pci/UhciPei/UhciPei.inf
!if $(RECOVERY_CDROM_ENABLE) == TRUE
  MdeModulePkg/Bus/Pci/IdeBusPei/IdeBusPei.inf
  ByoModulePkg/Universal/Disk/CDExpressPei/CdExpressPei.inf
!endif
!endif

  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTablePei/FirmwarePerformancePei.inf {
    <LibraryClasses>
      TimerLib|$(PLATFORM_PACKAGE)/Override/PerformancePkg/Library/TscTimerLib/PeiTscTimerLib.inf     
  }

!if $(TCM_ENABLE) == TRUE
  ByoModulePkg/Security/Tcm/Pei/TcmPei.inf
!endif
!if $(TPM12_ENABLE) == TRUE
  SecurityPkg\Tcg\PhysicalPresencePei\PhysicalPresencePei.inf
  SecurityPkg\Tcg\TcgPei\TcgPei.inf
!endif
!if $(TPM20_ENABLE) == TRUE
  SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigPei.inf {
    <LibraryClasses>
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf
  }
  SecurityPkg/Tcg/Tcg2Pei/Tcg2Pei.inf {
    <LibraryClasses>
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterPei.inf
      NULL|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2InstanceLibDTpm.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
      HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterPei.inf
      Tcg2PhysicalPresenceLib|SecurityPkg/Library/PeiTcg2PhysicalPresenceLib/PeiTcg2PhysicalPresenceLib.inf  
  }
!endif




[Components.X64]

  AsiaPkg/MicroCode/MicrocodeUpdates.inf

  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|IntelFrameworkModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
#    <PcdsPatchableInModule>
#      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xFFFFFFFF
  }

  MdeModulePkg/Universal/Pcd/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/ReportStatusCodeRouter/Smm/ReportStatusCodeRouterSmm.inf
  ByoModulePkg/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf {
    <LibraryClasses>
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
  }
  ByoModulePkg/StatusCodeHandler/Smm/StatusCodeHandlerSmm.inf {
    <LibraryClasses>
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
  }
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
    <LibraryClasses>
!if $(SECURE_BOOT_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
!endif
!if $(TPM12_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeTpmMeasureBootLib/DxeTpmMeasureBootLib.inf
!endif
!if $(TPM20_ENABLE) == TRUE      
      NULL|SecurityPkg/Library/DxeTpm2MeasureBootLib/DxeTpm2MeasureBootLib.inf
!endif
  }

  $(ASIA_SOURCE)/Foundation/Dxe/Cpu/AsiaCpuDxe.inf    # gAsiaCpuProtocol
  $(ASIA_SOURCE)/Foundation/Dxe/NB/AsiaNbDxe.inf      # gAsiaNbProtocol
  $(ASIA_SOURCE)/Foundation/Dxe/SB/AsiaSbDxe.inf      # gAsiaSbProtocol

  AsiaPkg/Int15Callback/Int15Smm.inf
  AsiaPkg/Int15Callback/Int15Installer.inf
#  AsiaPkg/ErrorReport/CRBSmi.inf
  AsiaPkg/mcasmi/McaSmi.inf
  
  ByoModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  ByoModulePkg/Universal/SmbiosSmm/SmbiosSmm.inf

  PcAtChipsetPkg/8259InterruptControllerDxe/8259.inf
  
!if $(MTC_USE_CMOS) == TRUE   
  $(PLATFORM_PACKAGE)/Override/MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
!else  
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
!endif  
  
  $(PLATFORM_PACKAGE)/Override/MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  IntelFrameworkModulePkg/Universal/CpuIoDxe/CpuIoDxe.inf
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  $(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/CpuArchDxe/CpuArchDxe.inf
  $(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/CpuMpDxe/CpuMpDxe.inf

  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf  
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  PcAtChipsetPkg/8254TimerDxe/8254Timer.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  $(PLATFORM_PACKAGE)/Override/PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
!if $(S3_ENABLE) == TRUE
  MdeModulePkg/Universal/Acpi/S3SaveStateDxe/S3SaveStateDxe.inf
  EdkCompatibilityPkg/Compatibility/BootScriptSaveOnS3SaveStateThunk/BootScriptSaveOnS3SaveStateThunk.inf
  MdeModulePkg/Universal/LockBox/SmmLockBox/SmmLockBox.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  }
  IntelFrameworkModulePkg/Universal/Acpi/AcpiS3SaveDxe/AcpiS3SaveDxe.inf
  MdeModulePkg/Universal/Acpi/BootScriptExecutorDxe/BootScriptExecutorDxe.inf {   # NVS
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  }
!endif

  $(PLATFORM_PACKAGE)/BdsDxe/BdsDxe.inf {
    <LibraryClasses>
!if $(PERFORMANCE_ENABLE) == TRUE
      PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
      TimerLib|$(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/Library/CpuLocalApicTimerLib/CpuLocalApicTimerLib.inf
!endif
!if $(TPM12_ENABLE) == TRUE  
      TcgPhysicalPresenceLib|SecurityPkg/Library/DxeTcgPhysicalPresenceLib/DxeTcgPhysicalPresenceLib.inf
!endif  
!if $(TPM20_ENABLE) == TRUE 
      TcgPhysicalPresenceLib|SecurityPkg/Library/DxeTcg2PhysicalPresenceLib/DxeTcg2PhysicalPresenceLib.inf
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
!endif 
  }
  
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  $(PLATFORM_PACKAGE)/Override/MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf

  $(PLATFORM_PACKAGE)/Platform/EarlyDxe/PlatformEarlyDxe.inf {
    <LibraryClasses>
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
  }
  $(PLATFORM_PACKAGE)/Platform/Dxe/PlatformDxe.inf {
    <LibraryClasses>
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
!if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf 
    TimerLib|$(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/Library/CpuLocalApicTimerLib/CpuLocalApicTimerLib.inf
!endif  
  }
  $(PLATFORM_PACKAGE)/SmmControl2/SmmControl2.inf

  $(PLATFORM_PACKAGE)/Override/PcAtChipsetPkg/PciHostBridgeDxe/PciHostBridgeDxe.inf
  $(PLATFORM_PACKAGE)/Override/MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000040
  }
!if $(MEMORY_TEST_ENABLE) == TRUE
  MdeModulePkg/Universal/MemoryTest/GenericMemoryTestDxe/GenericMemoryTestDxe.inf
!else
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
!endif
  $(PLATFORM_PACKAGE)/Override/IntelFrameworkModulePkg/Csm/LegacyBiosDxe/LegacyBiosDxe.inf
  IntelFrameworkModulePkg/Csm/BiosThunk/BlockIoDxe/BlockIoDxe.inf
  IntelFrameworkModulePkg/Csm/BiosThunk/VideoDxe/VideoDxe.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  }
  $(PLATFORM_PACKAGE)/LegacyInterruptHookDxe/LegacyInterruptHook.inf

  UefiCpuPkg/CpuIo2Smm/CpuIo2Smm.inf
  MdeModulePkg/Core/PiSmmCore/PiSmmIpl.inf
  MdeModulePkg/Core/PiSmmCore/PiSmmCore.inf {
    <LibraryClasses>
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
  }
  $(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf {
    <LibraryClasses>
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
  }

  $(PLATFORM_PACKAGE)/SmiDispatcher/SmiDispatcher.inf
  $(PLATFORM_PACKAGE)/SmmPlatform/SmmPlatform.inf
  $(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/PiSmmCommunication/PiSmmCommunicationSmm.inf

#ATA & SCSI
  $(PLATFORM_PACKAGE)/Override/MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf

!if $(CAPSULE_ENABLE) == TRUE
  MdeModulePkg/Universal/CapsulePei/CapsuleX64.inf {
    <LibraryClasses>
!if $(DEBUG_MESSAGE_ENABLE) == TRUE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
  }
!endif

  #Byosoft smiflash
  $(PLATFORM_PACKAGE)/SmiFlash/SmiFlash.inf
  $(PLATFORM_PACKAGE)/SmiVariable/SmiVariableSmm.inf
  
#USB
  ByoModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  ByoModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  ByoModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  ByoModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
#  ByoModulePkg/Bus/Usb/LegacyUsbSmm/LegacyUsbSmm.inf
  $(PLATFORM_PACKAGE)/Override/ByoModulePkg/Bus/Usb/LegacyUsbSmm/LegacyUsbSmm.inf

#ISA Support
#HYL-2016101801  PcAtChipsetPkg/IsaAcpiDxe/IsaAcpi.inf
  $(PLATFORM_PACKAGE)/Override/PcAtChipsetPkg/IsaAcpiDxe/IsaAcpi.inf #HYL-2016101801
  IntelFrameworkModulePkg/Bus/Isa/IsaBusDxe/IsaBusDxe.inf
  $(PLATFORM_PACKAGE)/Override/IntelFrameworkModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2keyboardDxe.inf {
    <LibraryClasses>
!if $(PERFORMANCE_ENABLE) == TRUE
      PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf 
      TimerLib|$(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/Library/CpuLocalApicTimerLib/CpuLocalApicTimerLib.inf
!endif  
  }
  
#ACPI Support
  $(PLATFORM_PACKAGE)/AcpiTables/AcpiTables.inf {
    <BuildOptions>
!if $(TCM_ENABLE) == TRUE    
      *_*_*_ASLPP_FLAGS = /DTCM_ENABLE=1
!endif      
  }

  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  $(PLATFORM_PACKAGE)/AcpiPlatformDxe/AcpiPlatformDxe.inf
  
# SDIO
  ByoModulePkg/Bus/Pci/SdHostDxe/SdHostDxe.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  }
  ByoModulePkg/Bus/Sd/MediaDeviceDxe/MediaDeviceDxe.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  }
  ByoModulePkg/Bus/Sd/LegacySdSmm/SdLegacySmm.inf

# SPI
  $(PLATFORM_PACKAGE)/Spi/SpiSmm.inf
  $(PLATFORM_PACKAGE)/Spi/SpiDxe.inf
  ByoNvMediaPkg/PlatformAccess/Smm/PlatformAccess.inf
  ByoNvMediaPkg/NvMediaAccess/Smm/NvMediaAccess.inf
  ByoNvMediaPkg/NvMediaAccess/Dxe/NvMediaAccess.inf
  ByoNvMediaPkg/FlashDevice/SST25VF032B/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/SST25VF032B/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/SST25VF064C/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/SST25VF064C/Dxe/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/SST26VF064B/Smm/spiflashdevice.inf
  ByoNvMediaPkg/FlashDevice/SST26VF064B/Dxe/spiflashdevice.inf
  $(PLATFORM_PACKAGE)/Override/ByoNvMediaPkg/FvbService/Smm/FvbService.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  }
  
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmm.inf  
!if $(SECURE_BOOT_ENABLE) == TRUE
  SecurityPkg/VariableAuthenticated/RuntimeDxe/VariableSmmRuntimeDxe.inf
  SecurityPkg/VariableAuthenticated/RuntimeDxe/VariableSmm.inf
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
!else
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmm.inf
!endif  


  
  $(PLATFORM_PACKAGE)/Override/ByoModulePkg/CrisisRecovery/FlashUpdateDxe/FlashUpdateDxe.inf

  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  ByoModulePkg/Graphics/JpegDecoderDxe/JpegDecoder.inf
  ByoModulePkg/Setup/TextBrowserDxe/SetupBrowserDxe.inf
  $(PLATFORM_PACKAGE)/PlatformSetupDxe/PlatformSetupDxe.inf
  $(PLATFORM_PACKAGE)/UiApp/UiApp.inf
  $(PLATFORM_PACKAGE)/BootManagerMenuApp/BootManagerMenuApp.inf

  MdeModulePkg/Universal/Variable/EmuRuntimeDxe/EmuVariableRuntimeDxe.inf

  IntelFrameworkModulePkg/Bus/Isa/IsaSerialDxe/IsaSerialDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf



# Network
!if $(USE_PREBUILD_NETWORK_STACK) == FALSE
  MdeModulePkg/Universal/Network/SnpDxe/SnpDxe.inf
  MdeModulePkg/Universal/Network/DpcDxe/DpcDxe.inf
  MdeModulePkg/Universal/Network/MnpDxe/MnpDxe.inf
  MdeModulePkg/Universal/Network/ArpDxe/ArpDxe.inf
  MdeModulePkg/Universal/Network/Dhcp4Dxe/Dhcp4Dxe.inf
  MdeModulePkg/Universal/Network/Ip4ConfigDxe/Ip4ConfigDxe.inf
  MdeModulePkg/Universal/Network/Ip4Dxe/Ip4Dxe.inf
  MdeModulePkg/Universal/Network/Mtftp4Dxe/Mtftp4Dxe.inf
  MdeModulePkg/Universal/Network/Tcp4Dxe/Tcp4Dxe.inf
  MdeModulePkg/Universal/Network/Udp4Dxe/Udp4Dxe.inf
  MdeModulePkg/Universal/Network/VlanConfigDxe/VlanConfigDxe.inf
  NetworkPkg/Ip6Dxe/Ip6Dxe.inf
  NetworkPkg/Dhcp6Dxe/Dhcp6Dxe.inf
  NetworkPkg/IpSecDxe/IpSecDxe.inf
  NetworkPkg/TcpDxe/TcpDxe.inf
  NetworkPkg/Udp6Dxe/Udp6Dxe.inf
  NetworkPkg/Mtftp6Dxe/Mtftp6Dxe.inf
  NetworkPkg/UefiPxeBcDxe/UefiPxeBcDxe.inf
  NetworkPkg/IScsiDxe/IScsiDxe.inf
!endif



  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableDxe/FirmwarePerformanceDxe.inf {
    <LibraryClasses>
      TimerLib|$(PLATFORM_PACKAGE)/Override/PerformancePkg/Library/TscTimerLib/DxeTscTimerLib.inf
  }
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableSmm/FirmwarePerformanceSmm.inf {
    <LibraryClasses>
      TimerLib|$(PLATFORM_PACKAGE)/Override/PerformancePkg/Library/TscTimerLib/DxeTscTimerLib.inf
  }
  MdeModulePkg/Universal/Acpi/BootGraphicsResourceTableDxe/BootGraphicsResourceTableDxe.inf


!if $(TCM_ENABLE) == TRUE
  ByoModulePkg/Security/Tcm/Dxe/TcmDxe.inf
!if $(CSM_ENABLE) != FALSE
  ByoModulePkg/Security/Tcm/Smm/TcmSmm16.inf
  ByoModulePkg/Security/Tcm/Smm/TcmSmmInstallInt1A.inf
  ByoModulePkg/Security/Tcm/Smm/TcmSmm.inf  
!endif
# ByoModulePkg/Security/Tcm/Application/TcmApp/TcmApp.inf
!endif

!if $(PERFORMANCE_ENABLE) == TRUE
  PerformancePkg/Dp_App/Dp.inf {
    <LibraryClasses>
      ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
      FileHandleLib|ShellPkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
      TimerLib|$(PLATFORM_PACKAGE)/IA32FamilyCpuPkg/Library/CpuLocalApicTimerLib/CpuLocalApicTimerLib.inf
      PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  }
!endif

!if ($(TPM12_ENABLE) == TRUE) OR ($(TPM20_ENABLE) == TRUE)
  SecurityPkg/Tcg/MemoryOverwriteControl/TcgMor.inf  
!endif  

!if $(TPM12_ENABLE) == TRUE
  SecurityPkg/Tcg/TcgDxe/TcgDxe.inf{
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  SecurityPkg/Tcg/TcgConfigDxe/TcgConfigDxe.inf
  SecurityPkg/Tcg/TcgSmm/TcgSmm.inf
!if $(CSM_ENABLE) != FALSE
  ByoModulePkg/Security/Tpm/TcgServiceSmm/TcgSmm.inf
  ByoModulePkg/Security/Tpm/TcgServiceSmm/TcgSmm16.inf
  ByoModulePkg/Security/Tpm/TcgServiceSmm/TcgSmmInstallInt1A.inf
!endif  
!endif
!if $(TPM20_ENABLE) == TRUE
  SecurityPkg/Tcg/Tcg2Dxe/Tcg2Dxe.inf {
    <LibraryClasses>
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterDxe.inf
      HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterDxe.inf
      Tcg2PhysicalPresenceLib|SecurityPkg/Library/DxeTcg2PhysicalPresenceLib/DxeTcg2PhysicalPresenceLib.inf
      NULL|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2InstanceLibDTpm.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
  }
  SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigDxe.inf {
    <LibraryClasses>
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
      Tcg2PhysicalPresenceLib|SecurityPkg/Library/DxeTcg2PhysicalPresenceLib/DxeTcg2PhysicalPresenceLib.inf
  }
  
  SecurityPkg/Tcg/Tcg2Smm/Tcg2Smm.inf {
    <LibraryClasses>
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
      TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
      Tcg2PhysicalPresenceLib|SecurityPkg/Library/SmmTcg2PhysicalPresenceLib/SmmTcg2PhysicalPresenceLib.inf      
  }
  
!endif  

$(PLATFORM_PACKAGE)/MpTableDxe/MpTableDxe.inf
$(PLATFORM_PACKAGE)/Platform/BiosInfo/BiosInfo.inf
$(PLATFORM_PACKAGE)/Platform/TxtScreen/TxtScreen.inf
#$(PLATFORM_PACKAGE)/Apei/Apei.inf
#ServerCommonPkg\Ipmi\Generic\GenericIpmi.inf

!if $(NVME_SUPPORT) == TRUE
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf
!endif  


[BuildOptions]

!if $(CSM_ENABLE) == TRUE
  DEFINE DSC_CSM_BUILD_OPTIONS = /DCSM_ENABLE=1
!elseif $(CSM_ENABLE) == FALSE
  DEFINE DSC_CSM_BUILD_OPTIONS = /DCSM_ENABLE=0
!else
  DEFINE DSC_CSM_BUILD_OPTIONS = /DCSM_ENABLE=2
!endif

!if $(ZX_SECRET_CODE) == TRUE
DEFINE ZX_SECRET_COMMON_FEATURE_BUILD_OPTIONS = /DZX_SECRET_CODE
!else
DEFINE ZX_SECRET_COMMON_FEATURE_BUILD_OPTIONS =
!endif


#define DSC_COMMON_FEATURE_BUILD_OPTIONS = $(DSC_CSM_BUILD_OPTIONS) $(ZX_SECRET_COMMON_FEATURE_BUILD_OPTIONS)

!ifdef CHX001_HAPS  
DEFINE DSC_COMMON_FEATURE_BUILD_OPTIONS = $(DSC_CSM_BUILD_OPTIONS) $(ZX_SECRET_COMMON_FEATURE_BUILD_OPTIONS) /DCHX001_HAPS
!else
DEFINE DSC_COMMON_FEATURE_BUILD_OPTIONS = $(DSC_CSM_BUILD_OPTIONS) $(ZX_SECRET_COMMON_FEATURE_BUILD_OPTIONS)
!endif

!ifdef CHX001_A0
DEFINE DSC_COMMON_FEATURE_BUILD_OPTIONS = $(DSC_CSM_BUILD_OPTIONS) $(ZX_SECRET_COMMON_FEATURE_BUILD_OPTIONS) /DCHX001_A0
!else
DEFINE DSC_COMMON_FEATURE_BUILD_OPTIONS = $(DSC_CSM_BUILD_OPTIONS) $(ZX_SECRET_COMMON_FEATURE_BUILD_OPTIONS)
!endif

[BuildOptions.Common.EDK]

!if $(DEBUG_MESSAGE_ENABLE) == TRUE
  DEFINE DEBUG_BUILD_OPTIONS = /DEFI_DEBUG
!endif

!if $(OPTIMIZE_COMPILER_ENABLE) == FALSE
  DEFINE OPTIMIZE_DISABLE_OPTIONS = /Od /GL-
!endif

  DEFINE RC_EDK_BUILD_OPTIONS = /DGTL_AUTO_COMP_ENABLE /DCHIPSET_TPM_SUPPORT /DEFI_S3_RESUME

  DEFINE EDK_DSC_FEATURE_BUILD_OPTIONS = $(DSC_COMMON_FEATURE_BUILD_OPTIONS) $(DEBUG_BUILD_OPTIONS) $(OPTIMIZE_DISABLE_OPTIONS)
  DEFINE EDK_DSC_GLOBAL_BUILD_OPTIONS  = $(EDK_DSC_FEATURE_BUILD_OPTIONS) /DEFI_SPECIFICATION_VERSION=0x0002000A  /DPI_SPECIFICATION_VERSION=0x00010000  /DTIANO_RELEASE_VERSION=0x00080006 /DSUPPORT_DEPRECATED_PCI_CFG_PPI $(RC_EDK_BUILD_OPTIONS)

  !ifdef CHX001_HAPS
      DEFINE EDK_DSC_GLOBAL_BUILD_OPTIONS = $(EDK_DSC_FEATURE_BUILD_OPTIONS) /DEFI_SPECIFICATION_VERSION=0x0002000A  /DPI_SPECIFICATION_VERSION=0x00010000  /DTIANO_RELEASE_VERSION=0x00080006 /DSUPPORT_DEPRECATED_PCI_CFG_PPI $(RC_EDK_BUILD_OPTIONS) /DCHX001_HAPS
  !else
      DEFINE EDK_DSC_GLOBAL_BUILD_OPTIONS = $(EDK_DSC_FEATURE_BUILD_OPTIONS) /DEFI_SPECIFICATION_VERSION=0x0002000A  /DPI_SPECIFICATION_VERSION=0x00010000  /DTIANO_RELEASE_VERSION=0x00080006 /DSUPPORT_DEPRECATED_PCI_CFG_PPI $(RC_EDK_BUILD_OPTIONS)
  !endif


  *_*_IA32_ASM_FLAGS   = /DEFI32
  *_*_IA32_CC_FLAGS    = /DEFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_IA32_VFRPP_FLAGS = /DEFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_IA32_APP_FLAGS   = /DEFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_IA32_PP_FLAGS    = /DEFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_IA32_ASLPP_FLAGS =
  *_*_IA32_ASLCC_FLAGS =
  *_*_IA32_ASM16_FLAGS =

  *_*_X64_ASM_FLAGS    = /DEFIX64
  *_*_X64_CC_FLAGS     = /DEFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_X64_VFRPP_FLAGS  = /DEFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_X64_APP_FLAGS    = /DEFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_X64_PP_FLAGS     = /DEFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_X64_ASLCC_FLAGS  = /DEFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_X64_ASLPP_FLAGS  =
  *_*_X64_ASLCC_FLAGS  =
  *_*_X64_ASM16_FLAGS  =

  *_*_*_BUILD_FLAGS = -s






[BuildOptions.Common.EDKII]

!if $(ACPI_ENABLE) == TRUE
  DEFINE DSC_ACPI_BUILD_OPTIONS = /DACPI_SUPPORT=1
!else
  DEFINE DSC_ACPI_BUILD_OPTIONS =
!endif

!if $(DEBUG_MESSAGE_ENABLE) == TRUE
  DEFINE UDK_DEBUG_BUILD_OPTIONS =
!else
  DEFINE UDK_DEBUG_BUILD_OPTIONS = /DMDEPKG_NDEBUG
!endif

  DEFINE DSC_PLAT_TARGET_OPT1 =
  
!ifdef HX001EA1_10
  DEFINE DSC_PLAT_TARGET_OPT2 = /DHX001EA1_10
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT2 =
!endif 

!ifdef HX001EB1_10
  DEFINE DSC_PLAT_TARGET_OPT3 = /DHX001EB1_10
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT3 =
!endif 

!ifdef HX001EC1_04
  DEFINE DSC_PLAT_TARGET_OPT4 = /DHX001EC1_04
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT4 =
!endif 

!ifdef HX001EC1_05
  DEFINE DSC_PLAT_TARGET_OPT5 = /DHX001EC1_05
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT5 =
!endif 

!ifdef HX001EC1_08
  DEFINE DSC_PLAT_TARGET_OPT6 = /DHX001EC1_08
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT6 =
!endif 

!ifdef HX001EC1_09
  DEFINE DSC_PLAT_TARGET_OPT7 = /DHX001EC1_09
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT7 =
!endif 

!ifdef HX001EC1_12
  DEFINE DSC_PLAT_TARGET_OPT8 = /DHX001EC1_12
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT8 =
!endif 

!ifdef HX001EC1_13
  DEFINE DSC_PLAT_TARGET_OPT9 = /DHX001EC1_13
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT9 =
!endif 

!ifdef HX001ED0_03
  DEFINE DSC_PLAT_TARGET_OPT10 = /DHX001ED0_03
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_B
!else
  DEFINE DSC_PLAT_TARGET_OPT10 =
!endif 

!ifdef HX001EE0_03
  DEFINE DSC_PLAT_TARGET_OPT11 = /DHX001EE0_03
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_B
!else
  DEFINE DSC_PLAT_TARGET_OPT11 =
!endif 

!ifdef HX001EF0_06
  DEFINE DSC_PLAT_TARGET_OPT12 = /DHX001EF0_06
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_B
!else
  DEFINE DSC_PLAT_TARGET_OPT12 =
!endif 

!ifdef HX001EF0_10
  DEFINE DSC_PLAT_TARGET_OPT13 = /DHX001EF0_10
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_B
!else
  DEFINE DSC_PLAT_TARGET_OPT13 =
!endif 

!ifdef HX001EF0_14
  DEFINE DSC_PLAT_TARGET_OPT14 = /DHX001EF0_14
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_B
!else
  DEFINE DSC_PLAT_TARGET_OPT14 =
!endif 

!ifdef HX001EG0_09
  DEFINE DSC_PLAT_TARGET_OPT15 = /DHX001EG0_09
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT15 =
!endif 

!ifdef HX001EG0_10
  DEFINE DSC_PLAT_TARGET_OPT16 = /DHX001EG0_10
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT16 =
!endif 

!ifdef HX001EG0_11
  DEFINE DSC_PLAT_TARGET_OPT17 = /DHX001EG0_11
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT17 =
!endif 

!ifdef HX001EH0_09
  DEFINE DSC_PLAT_TARGET_OPT18 = /DHX001EH0_09
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT18 =
!endif 

!ifdef HX001EH0_10
  DEFINE DSC_PLAT_TARGET_OPT19 = /DHX001EH0_10
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT19 =
!endif 

!ifdef HX001EH0_11
  DEFINE DSC_PLAT_TARGET_OPT20 = /DHX001EH0_11
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT20 =
!endif 

!ifdef HX001EI0_01
  DEFINE DSC_PLAT_TARGET_OPT21 = /DHX001EI0_01
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT21 =
!endif 

!ifdef HX001EI0_02
  DEFINE DSC_PLAT_TARGET_OPT22 = /DHX001EI0_02
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT22 =
!endif 

!ifdef HX001EI0_03
  DEFINE DSC_PLAT_TARGET_OPT23 = /DHX001EI0_03
  DEFINE DSC_PLAT_TARGET_PKG_OPT = /DCHX001_PKG_A
!else
  DEFINE DSC_PLAT_TARGET_OPT23 =
!endif 

!ifdef ZX_DDR3
  DEFINE DSC_PLAT_TARGET_OPT24 = /DZX_DDR3
!else
!ifdef ZX_DDR4
  DEFINE DSC_PLAT_TARGET_OPT24 = /DZX_DDR4
!else
  DEFINE DSC_PLAT_TARGET_OPT24 =
!endif 
!endif 

  DEFINE EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS = $(DSC_COMMON_FEATURE_BUILD_OPTIONS) $(DSC_ACPI_BUILD_OPTIONS) $(UDK_DEBUG_BUILD_OPTIONS) $(PLATFORM_FEATURE_SUPPORT) $(DSC_PLAT_TARGET_OPT1) $(DSC_PLAT_TARGET_OPT2) $(DSC_PLAT_TARGET_OPT3) $(DSC_PLAT_TARGET_OPT4) $(DSC_PLAT_TARGET_OPT5) $(DSC_PLAT_TARGET_OPT6) $(DSC_PLAT_TARGET_OPT7) $(DSC_PLAT_TARGET_OPT8) $(DSC_PLAT_TARGET_OPT9) $(DSC_PLAT_TARGET_OPT10) $(DSC_PLAT_TARGET_OPT11) $(DSC_PLAT_TARGET_OPT12) $(DSC_PLAT_TARGET_OPT13) $(DSC_PLAT_TARGET_OPT14) $(DSC_PLAT_TARGET_OPT15) $(DSC_PLAT_TARGET_OPT16) $(DSC_PLAT_TARGET_OPT17) $(DSC_PLAT_TARGET_OPT18) $(DSC_PLAT_TARGET_OPT19) $(DSC_PLAT_TARGET_OPT20) $(DSC_PLAT_TARGET_OPT21) $(DSC_PLAT_TARGET_OPT22) $(DSC_PLAT_TARGET_OPT23) $(DSC_PLAT_TARGET_OPT24) $(DSC_PLAT_TARGET_PKG_OPT)


  *_*_IA32_ASM_FLAGS     = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_IA32_CC_FLAGS      = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_IA32_VFRPP_FLAGS   = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_IA32_APP_FLAGS     = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_IA32_PP_FLAGS      = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_IA32_ASLPP_FLAGS   = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_IA32_ASM16_FLAGS   = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)

  *_*_X64_ASM_FLAGS      = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_X64_CC_FLAGS       = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_X64_VFRPP_FLAGS    = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_X64_APP_FLAGS      = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_X64_PP_FLAGS       = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_X64_ASLPP_FLAGS    = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_X64_ASM16_FLAGS    = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)

  *_*_*_ASL_FLAGS = -oi


