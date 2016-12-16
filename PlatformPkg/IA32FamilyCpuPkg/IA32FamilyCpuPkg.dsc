#
# This file contains an 'Intel Peripheral Driver' and is      
# licensed for Intel CPUs and chipsets under the terms of your
# license agreement with Intel or your vendor.  This file may 
# be modified by the user, subject to additional terms of the 
# license agreement                                           
#
## @file
# Package description file for IA32FamilyCpuPkg.
#
# This DSC file is used for Package Level build.
#
# Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
#
#    This software and associated documentation (if any) is furnished
#    under a license and may only be used or copied in accordance
#    with the terms of the license. Except as permitted by such
#    license, no part of this software or documentation may be
#    reproduced, stored in a retrieval system, or transmitted in any
#    form or by any means without the express written consent of
#    Intel Corporation.
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = IA32FamilyCpuPkg
  PLATFORM_GUID                  = 191156A2-3366-4B08-9852-555E828B9575
  PLATFORM_VERSION               = 0.4
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/IA32FamilyCpuPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

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
[LibraryClasses]
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf  
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  CpuOnlyResetLib|IA32FamilyCpuPkg/Library/CpuOnlyResetLibNull/CpuOnlyResetLibNull.inf
  PlatformSecLib|IA32FamilyCpuPkg/Library/PlatformSecLibNull/PlatformSecLibNull.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  CpuConfigLib|IA32FamilyCpuPkg/Library/CpuConfigLib/CpuConfigLib.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  S3IoLib|MdePkg/Library/BaseS3IoLib/BaseS3IoLib.inf
  S3PciLib|MdePkg/Library/BaseS3PciLib/BaseS3PciLib.inf
  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  SmmLib|MdePkg/Library/SmmLibNull/SmmLibNull.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf

[LibraryClasses.common.PEIM]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  SmbusLib|MdePkg/Library/PeiSmbusLibSmbus2Ppi/PeiSmbusLibSmbus2Ppi.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf
  OemHookStatusCodeLib|ByoModulePkg/Library/ByoHookStatusCodeLib/Pei/ByoHookStatusCodePeiLib.inf

[LibraryClasses.common.DXE_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
  OemHookStatusCodeLib|ByoModulePkg/Library/ByoHookStatusCodeLib/Dxe/ByoHookStatusCodeDxeLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxSmmLib.inf
  OemHookStatusCodeLib|ByoModulePkg/Library/ByoHookStatusCodeLib/Smm/ByoHookStatusCodeSmmLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFeatureFlag]
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable|TRUE
  gEfiCpuTokenSpaceGuid.PcdCpuSmmEnableBspElection|FALSE
  gEfiCpuTokenSpaceGuid.PcdCpuSelectLfpAsBspFlag|TRUE

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x06
  gEfiCpuTokenSpaceGuid.PcdCpuMicrocodePatchAddress|0xfffba000
  gEfiCpuTokenSpaceGuid.PcdCpuMicrocodePatchRegionSize|0x2000
  gEfiCpuTokenSpaceGuid.PcdPlatformCpuMaxFsbFrequency|1066
  gEfiCpuTokenSpaceGuid.PcdPlatformCpuMaxCoreFrequency|3800

################################################################################
#
# Pcd Dynamic Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsDynamicDefault.common.DEFAULT]
  gEfiCpuTokenSpaceGuid.PcdCpuProcessorFeatureUserConfiguration|0xffffffff
  gEfiCpuTokenSpaceGuid.PcdCpuProcessorFeatureCapability|0
  gEfiCpuTokenSpaceGuid.PcdCpuProcessorFeatureSetting|0
  gEfiCpuTokenSpaceGuid.PcdCpuConfigContextBuffer|0
  gEfiCpuTokenSpaceGuid.PcdCpuCallbackSignal|0
  gEfiCpuTokenSpaceGuid.PcdPlatformCpuFrequencyLists|0
  gEfiCpuTokenSpaceGuid.PcdPlatformCpuSocketNames|0
  gEfiCpuTokenSpaceGuid.PcdPlatformCpuAssetTags|0
  gEfiCpuTokenSpaceGuid.PcdCpuPageTableAddress|0
  gEfiCpuTokenSpaceGuid.PcdCpuMtrrTableAddress|0
  gEfiCpuTokenSpaceGuid.PcdIsPowerOnReset|FALSE

###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################

[Components]
  IA32FamilyCpuPkg/Library/CpuConfigLib/CpuConfigLib.inf
  IA32FamilyCpuPkg/Library/CpuLocalApicTimerLib/CpuLocalApicTimerLib.inf
  IA32FamilyCpuPkg/Library/CpuOnlyResetLibNull/CpuOnlyResetLibNull.inf
  IA32FamilyCpuPkg/Library/PlatformSecLibNull/PlatformSecLibNull.inf
  IA32FamilyCpuPkg/SecCore/SecCore.inf
  IA32FamilyCpuPkg/SecCore/Vtf0SecCore.inf
  IA32FamilyCpuPkg/PiSmmCommunication/PiSmmCommunicationPei.inf
  IA32FamilyCpuPkg/PiSmmCommunication/PiSmmCommunicationSmm.inf
  IA32FamilyCpuPkg/CpuArchDxe/CpuArchDxe.inf
  IA32FamilyCpuPkg/CpuMpDxe/CpuMpDxe.inf
  IA32FamilyCpuPkg/CpuPei/CpuPei.inf
  IA32FamilyCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf
  

