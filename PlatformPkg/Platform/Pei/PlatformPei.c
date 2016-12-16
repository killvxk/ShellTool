
//------------------------------------------------------------------------------
#include "PlatformPei.h"
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/AcpiWakeVectorPhase.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/MpServices.h>
#include <SetupVariable.h>
#include <ByoStatusCode.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PerformanceLib.h>
#include <Library/MtrrLib.h>
#include <PlatS3Record.h>



//------------------------------------------------------------------------------
EFI_STATUS
EFIAPI
EndOfPeiCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_STATUS
EFIAPI
AcpiWakeVectorCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_STATUS
EFIAPI
CpuMpPeiCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );  
  
EFI_STATUS 
PcieHotPlugInit (
  IN EFI_PEI_SERVICES  **PeiServices
	);

  
STATIC EFI_GUID gEfiDebugMaskPpiGuid     = EFI_DEBUG_MASK_PPI_GUID;
STATIC EFI_DEBUG_MASK_PPI gDebugMaskPpi  = {EFI_D_ERROR|EFI_D_INFO};

STATIC EFI_PEI_NOTIFY_DESCRIPTOR  gPpiNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiAcpiWakeVectorSignalPpiGuid,
    AcpiWakeVectorCallback  
  },  
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiEndOfPeiSignalPpiGuid,
    EndOfPeiCallback  
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiPeiMpServicesPpiGuid,
    CpuMpPeiCallback  
  },  
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    MemoryDiscoveredPpiNotifyCallback 
  }
};

STATIC EFI_PEI_PPI_DESCRIPTOR  gPpiInstallList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiDebugMaskPpiGuid,
    &gDebugMaskPpi
  }
};


EFI_STATUS  
GetAsiaPpi (
  EFI_ASIA_SB_PPI                  **SbPpi,
  EFI_ASIA_NB_PPI                  **NbPpi,
  EFI_ASIA_DRAM_PPI                **DramPpi,
  EFI_ASIA_CPU_PPI_PROTOCOL        **CpuPpi,
  ASIA_SB_LIB_PPI                  **SbLibPpi
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if(SbPpi!=NULL){
    Status = PeiServicesLocatePpi (
               &gAsiaSbPpiGuid,
               0,
               NULL,
               (VOID**)SbPpi
               );
    ASSERT_EFI_ERROR(Status);
  }  

  if(NbPpi!=NULL){
    Status = PeiServicesLocatePpi (
               &gAsiaNbPpiGuid,
               0,
               NULL,
               (VOID**)NbPpi
               );
    ASSERT_EFI_ERROR(Status);
  }  

  if(CpuPpi!=NULL){
    Status = PeiServicesLocatePpi (
               &gAsiaCpuPpiGuid,
               0,
               NULL,
               (VOID**)CpuPpi
               );
    ASSERT_EFI_ERROR(Status);
  }  

  if(DramPpi!=NULL){
    Status = PeiServicesLocatePpi (
               &gAsiaDramPpiGuid,
               0,
               NULL,
               (VOID**)DramPpi
               );
    ASSERT_EFI_ERROR(Status);
  }  
  
  if(SbLibPpi!=NULL){
    Status = PeiServicesLocatePpi (
               &gAsiaSbLibPpiGuid,
               0,
               NULL,
               (VOID**)SbLibPpi
               );
    ASSERT_EFI_ERROR(Status);  
  }
  
  return Status;
}



EFI_STATUS 
HandleXhciFwPei(
  PLATFORM_S3_RECORD *S3Record
  )
{
  //// TGR-2016062201. SOC not include XHCI controller
#if 0
  EFI_STATUS          Status;
  VOID                *McuFile;
  VOID                *HubFile;
  UINT32              Crc32;
  
	
  if(MmioRead16(XHCI_PCI_REG(0)) == 0xFFFF){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }  

  McuFile = (VOID*)(UINTN)S3Record->XhciMcuFw;
  HubFile = (VOID*)(UINTN)S3Record->XhciHubFw;

  LibCalcCrc32(McuFile, S3Record->XhciMcuFwSize, &Crc32);
  ASSERT(Crc32 == S3Record->XhciMcuFwCrc32);

  LibCalcCrc32(HubFile, S3Record->XhciHubFwSize, &Crc32);
  ASSERT(Crc32 == S3Record->XhciHubFwCrc32);

  Status = LoadXhciFw(HubFile, McuFile);
  ASSERT_EFI_ERROR(Status);
  
ProcExit:  
  return Status;
#else
  return EFI_SUCCESS;
#endif
}



VOID PlatformSettingForS3Resume()
{
// After CpuS3Init, Smi could be triggered(RestoreLockBox uses it). 
// So we formal enable SMI here.

  IoAnd16(PMIO_REG(PMIO_GBLCTRL_REG), (UINT16)~PMIO_GC_SMIEN);

  IoWrite16(PMIO_REG(PMIO_STS_REG), PMIO_STS_PWBTNOR|PMIO_STS_GBL|PMIO_STS_BM|PMIO_STS_TMROF);
  IoWrite16(PMIO_REG(PMIO_PM_EN), PMIO_PM_EN_PWRBTN);	
  IoWrite16(PMIO_REG(PMIO_GBLSTS_REG), 0xFFFF);		
  IoOr16(PMIO_REG(PMIO_GBLEN_REG), PMIO_GBLEN_TPAPO|PMIO_GBLEN_SWSMI|PMIO_GBLEN_SLPSMI);
  IoOr16(PMIO_REG(PMIO_PM1_CNT_REG), PMIO_PM1_CNT_SCI_EN);

  IoOr16(PMIO_REG(PMIO_GBLCTRL_REG), PMIO_GC_SMIACTIVE);	
  IoOr16(PMIO_REG(PMIO_GBLCTRL_REG), PMIO_GC_SMIEN);
}


EFI_STATUS
FindFvContainDxeCore (
  EFI_PEI_FV_HANDLE  *FvHandle
  )
{
  EFI_STATUS            Status;
  UINTN                 Instance;
  EFI_PEI_FV_HANDLE     VolumeHandle;
  EFI_PEI_FILE_HANDLE   FileHandle;
  
  Instance = 0;
  while (TRUE) {
    Status = PeiServicesFfsFindNextVolume(Instance, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
    FileHandle = NULL;
    Status = PeiServicesFfsFindNextFile(EFI_FV_FILETYPE_DXE_CORE, VolumeHandle, &FileHandle);
    if (!EFI_ERROR (Status)) {
      *FvHandle = VolumeHandle;
      return EFI_SUCCESS;
    }
    Instance++;
  }
  
  return EFI_NOT_FOUND;
}



VOID UpdateFvHobForRecovery()
{
  EFI_PEI_FV_HANDLE     FvHandle;
  EFI_STATUS            Status;	
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  UINT64                Length;
  BOOLEAN               FoundIt;
  
	Status = FindFvContainDxeCore(&FvHandle);
	ASSERT(!EFI_ERROR(Status));
  BaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)FvHandle;
  Length = ((EFI_FIRMWARE_VOLUME_HEADER*)(UINTN)FvHandle)->FvLength;

  FoundIt = FALSE;
  for (Hob.Raw = (UINT8*)GetHobList(); !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_FV) {
      continue;
    }
    if(Hob.FirmwareVolume->BaseAddress == BaseAddress && Hob.FirmwareVolume->Length == Length){
      FoundIt = TRUE;
      break;
    }
  }
  
  if(!FoundIt){
    DEBUG((EFI_D_INFO, "BuildFvHob(%lX,%lX)\n", BaseAddress, Length));  
    BuildFvHob(BaseAddress, Length);
  }
}




typedef struct {
  ASIA_CPU_FEATURE_INIT   CpuFeatureInit;
  ASIA_CPU_CONFIGURATION  *CpuFeature;
  MTRR_SETTINGS           *Mtrrs;
  UINT8                   FinishCount;
  BOOLEAN                 IsS3;
} AP_MISSION_ARG;

VOID
ApMission (  
  IN OUT VOID  *Buffer
  )
{
  AP_MISSION_ARG  *Arg;

  Arg = (AP_MISSION_ARG*)Buffer;
  Arg->CpuFeatureInit(Arg->CpuFeature);

  if(Arg->IsS3){
    MtrrSetAllMtrrs(Arg->Mtrrs);  
  }
  
  Arg->FinishCount++;
}


EFI_STATUS
EFIAPI
CpuMpPeiCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                 Status;
  EFI_PEI_MP_SERVICES_PPI    *MpSvr;
  ASIA_CPU_CONFIGURATION     CpuFeatureData;
  ASIA_CPU_CONFIGURATION     *CpuFeature;  
  EFI_ASIA_CPU_PPI_PROTOCOL  *CpuPpi;
  EFI_ASIA_SB_PPI            *SbPpi;
  EFI_ASIA_NB_PPI            *NbPpi;
  ASIA_SB_CONFIGURATION      *SbCfg;
  ASIA_NB_CONFIGURATION      *NbCfg;  
  SETUP_DATA                 *SetupHob;
  AP_MISSION_ARG             MissionArg;
  EFI_BOOT_MODE              BootMode;
  PLATFORM_S3_RECORD         *S3Record; 
  //YKN-20160627 -S
  UINTN  NumberOfProcessors, NumberOfEnabledProcessors;
  EFI_PEI_HOB_POINTERS  GuidHob;
 //YKN-20160627 -E

  Status = PeiServicesGetBootMode(&BootMode);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesLocatePpi (
             &gEfiPeiMpServicesPpiGuid,
             0,
             NULL,
             (VOID**)&MpSvr
             );
  ASSERT_EFI_ERROR(Status);

  Status = GetAsiaPpi(&SbPpi, &NbPpi, NULL, &CpuPpi, NULL);
  ASSERT_EFI_ERROR(Status);
  SbCfg = (ASIA_SB_CONFIGURATION*)(SbPpi->SbCfg);
  NbCfg = (ASIA_NB_CONFIGURATION*)(NbPpi->NbCfg);
  SetupHob = (SETUP_DATA*)GetSetupDataHobData();
  S3Record = (PLATFORM_S3_RECORD*)GetS3RecordTable();

 //YKN-20160627  +S
 //Update ASIA CPU activeCpuNum
  GuidHob.Raw = GetFirstGuidHob(&gAsiaCpuCfgHobGuid);
  ASSERT(GuidHob.Raw!=NULL);
  CpuFeature = (ASIA_CPU_CONFIGURATION*)(GuidHob.Guid+1); 
  MpSvr->GetNumberOfProcessors(PeiServices, 
          MpSvr, 
          &NumberOfProcessors, 
          &NumberOfEnabledProcessors);
  CpuFeature->ActiveCpuNum = (UINT8)NumberOfEnabledProcessors;
#ifdef ZX_SECRET_CODE
  if(SetupHob->FixCpuRatio) {
  	// 1/2/3/4/5 = 4x/5x/6x/7x/8x
  	CpuFeature->FixedRatio = ((SetupHob->FixCpuRatio + 3) << 1);
  } else {
    // 0 = Max
    CpuFeature->FixedRatio = SetupHob->FixCpuRatio;
  }
  //Actually CpuFeature->KillAp has no effect, just sync code.
  CpuFeature->KillAp = SetupHob->KillAp;
#endif
  //YKN-20160627 +E

 //YKN-20160803 +s
 //NOTE: VRM type and phases are MB dependent. 
 //Temply set 2 indicating 2 phase PVID(EVB2)
 //Save the VrmSupport into ASIA_CPU_CFG_HOB for setup CPU sheet display.
  CpuFeature->VrmSupport         = 2;     // VRM-3
 //YKN-20160803 +e

  CpuFeature = &CpuFeatureData;
  ZeroMem(CpuFeature, sizeof(*CpuFeature));
  CpuPpi->GetCpuSupportFeature(CpuFeature);
#ifdef ZX_SECRET_CODE
  //MKE_20161027 Partial Reset_S
  CpuFeature->PartialResetEn = SetupHob->PartialResetEn;
  //MKE_20161027 Parital Reset_E
#endif
  CpuFeature->PLVL2IoBase        = PMIO_REG(PMIO_PLVL2_REG);
//  CpuFeature->C5Control          = (UINT8)SbCfg->C5Control;

 //YKN-20160803 +s
 //NOTE: VRM type and phases are MB dependent. 
 //Temply set 2 indicating 2 phase PVID(EVB2)
  CpuFeature->VrmSupport         = 2;     // VRM-3
 //YKN-20160803 +e

  //YKN-20160627 +s
  //Tmp disable PMON to avoid mistakenly programming MSRs. CHX001 may need no PMON.
  CpuFeature->PmonEnable         = FALSE;
  
  //CpuFeature->ActiveCpuNum = 255;
  CpuFeature->ActiveCpuNum = (UINT8)NumberOfEnabledProcessors;
  CpuFeature->EpsEnable          = SetupHob->EpsEnable;
  CpuFeature->ProcessorCxeEnable = SetupHob->CxEEnable;
  CpuFeature->TPREchoEnable      = SetupHob->TPREcho;
  CpuFeature->ProcessorCState    = SetupHob->CpuCState;
  CpuFeature->C5Control          = SetupHob->C5Control;
  CpuFeature->C5L2sizingControl  = SetupHob->C5L2sizingControl;
  CpuFeature->CxIntFilterControl = SetupHob->CxIntFilterControl;
  CpuFeature->Msr3AEnable        = SetupHob->Msr3A;
  //YKN-20160627 +e
  CpuFeature->TscDeadlineModeEnable = SetupHob->TSC_Deadline_mode;
  CpuFeature->ExecuteDisableBit &= SetupHob->ExecuteDisable;
  /*
  #if 0
  CpuFeature->CxIntFilterControl = TRUE;
  CpuFeature->ProcessorCState    = SbCfg->ProcessorCState;
  CpuFeature->C5L2sizingControl  = TRUE;
  CpuFeature->EpsEnable          = SetupHob->EpsEnable;    // Enhanced Power Saver
  CpuFeature->VtEnable           = SetupHob->CpuVT;     // Virtualization Technology
  CpuFeature->ProcessorCxeEnable = SetupHob->CpuC1E;    // C1E
  CpuFeature->Msr3AEnable        = SetupHob->Msr3A; 
  CpuFeature->TPREchoEnable      = TRUE;
#else
  //Override default feature with SETUP settings
  CpuFeature->EpsEnable          = TRUE;
  CpuFeature->ProcessorCxeEnable = 0;
  CpuFeature->TPREchoEnable      = 1;
  CpuFeature->ProcessorCState = 0;
  CpuFeature->C5Control          = 0;
  CpuFeature->C5L2sizingControl  = 1;
  CpuFeature->CxIntFilterControl = 1;
  CpuFeature->Msr3AEnable        = 0;
  //YKN-20160413 +E
  #endif
  */
  DEBUG((DEBUG_INFO,  "FastString                     :%d\n", CpuFeature->FastString));   
  DEBUG((DEBUG_ERROR, "EpsEnable                      :%d\n", CpuFeature->EpsEnable));    
  DEBUG((DEBUG_INFO,  "FerrInterruptReporting         :%d\n", CpuFeature->FerrInterruptReporting));   
  DEBUG((DEBUG_INFO,  "BootWithMaxFrequence           :%d\n", CpuFeature->BootWithMaxFrequence));  
  DEBUG((DEBUG_INFO,  "NAPSNAPEnalbe                  :%d\n", CpuFeature->NAPSNAPEnalbe));   
  DEBUG((DEBUG_INFO,  "LimitCpuidMaximumValue         :%d\n", CpuFeature->LimitCpuidMaximumValue));  
  //DEBUG((DEBUG_INFO,  "L3CacheEnable                  :%d\n", CpuFeature->L3CacheEnable)); 
  DEBUG((DEBUG_ERROR, "ProcessorCxeEnable             :%d\n", CpuFeature->ProcessorCxeEnable)); 	
  //DEBUG((DEBUG_INFO,  "PeciEnable                     :%d\n", CpuFeature->PeciEnable));
  DEBUG((DEBUG_ERROR, "VtEnable                       :%d\n", CpuFeature->VtEnable));
  //DEBUG((DEBUG_INFO,  "LtEnable                       :%d\n", CpuFeature->LtEnable));    
  DEBUG((DEBUG_ERROR, "ExecuteDisableBit              :%d\n", CpuFeature->ExecuteDisableBit));
  DEBUG((DEBUG_INFO,  "MachineCheckEnable             :%d\n", CpuFeature->MachineCheckEnable));    
  //DEBUG((DEBUG_INFO,  "HardwarePrefetcherEnable       :%d\n", CpuFeature->HardwarePrefetcherEnable));
  //DEBUG((DEBUG_INFO,  "AdjacentCacheLinePrefetchEnable:%d\n", CpuFeature->AdjacentCacheLinePrefetchEnable));       
  DEBUG((DEBUG_INFO,  "MonitorMWaitEnable             :%d\n", CpuFeature->MonitorMWaitEnable));
  DEBUG((DEBUG_INFO,  "TPREchoEnable                  :%d\n", CpuFeature->TPREchoEnable));    
  //DEBUG((DEBUG_INFO,  "STOSEnable                     :%d\n", CpuFeature->STOSEnable));
  //DEBUG((DEBUG_INFO,  "Cmpxchg8bEnable                :%d\n", CpuFeature->Cmpxchg8bEnable));
  DEBUG((DEBUG_INFO,  "Msr3AEnable                    :%d\n", CpuFeature->Msr3AEnable)); 
  DEBUG((DEBUG_INFO,  "PLVL2IoBase                    :%X\n", CpuFeature->PLVL2IoBase));     
  DEBUG((DEBUG_INFO,  "PmonEnable                     :%d\n", CpuFeature->PmonEnable));
  DEBUG((DEBUG_INFO,  "VrmSupport                     :%d\n", CpuFeature->VrmSupport));     
  DEBUG((DEBUG_INFO,  "ActiveCpuNum                   :%d\n", CpuFeature->ActiveCpuNum)); 
  DEBUG((DEBUG_INFO,  "C5Control                      :%d\n", CpuFeature->C5Control));

  DEBUG((DEBUG_INFO,  "C5L2sizingControl              :%d\n", CpuFeature->C5L2sizingControl));
  DEBUG((DEBUG_INFO,  "CxIntFilterControl             :%d\n", CpuFeature->CxIntFilterControl));
  DEBUG((DEBUG_INFO,  "ProcessorCState             :%d\n", CpuFeature->ProcessorCState));
  DEBUG((DEBUG_INFO,  "TSCDeadLine                  :%d\n", CpuFeature->TscDeadlineModeEnable));

#ifdef ZX_SECRET_CODE
  DEBUG((DEBUG_INFO,  "KillAp                         :%d\n", CpuFeature->KillAp));
  DEBUG((DEBUG_INFO,  "FixedRatio                     :%d\n", CpuFeature->FixedRatio));
  DEBUG((DEBUG_INFO,  "PartialReset                   :%d\n",CpuFeature->PartialResetEn));
#endif
  CpuPpi->CpuFeatureInit(CpuFeature);

  MissionArg.CpuFeatureInit = CpuPpi->CpuFeatureInit;
  MissionArg.CpuFeature     = CpuFeature;
  MissionArg.FinishCount    = 0;
  MissionArg.IsS3           = (BOOLEAN)(BootMode == BOOT_ON_S3_RESUME);
  if(MissionArg.IsS3){
    MissionArg.Mtrrs = &S3Record->MtrrTable;
  }  
  
  Status = MpSvr->StartupAllAPs(PeiServices, MpSvr, ApMission, TRUE, 1000000, &MissionArg);

  DEBUG((EFI_D_INFO, "StartupAllAPs(%r) %d\n", Status, MissionArg.FinishCount));

  return Status;
}


EFI_STATUS
EFIAPI
AcpiWakeVectorCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  SetCacheMtrrAtS3PeiEnd(PeiServices);

  return EFI_SUCCESS;  
}



EFI_STATUS
EFIAPI
EndOfPeiCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_BOOT_MODE               BootMode;
  EFI_STATUS                  Status;
  PLATFORM_MEMORY_INFO        *MemInfo; 
  PLATFORM_S3_RECORD          *S3Record;
  EFI_ACPI_RAM_DATA           *AcpiRam;
  

  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));

  Status = PeiServicesGetBootMode(&BootMode);
  ASSERT_EFI_ERROR(Status); 

  MemInfo = (PLATFORM_MEMORY_INFO*)GetPlatformMemInfo(); 

  if(BootMode == BOOT_ON_S3_RESUME){

//    DEBUG((EFI_D_INFO, "IGD_SSID  :%08X\n", MmioRead32(IGD_PCI_REG(PCI_SSID_REG))));
//    DEBUG((EFI_D_INFO, "IGDAC_SSID:%08X\n", MmioRead32(IGDAC_PCI_REG(PCI_SSID_REG))));

    S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
    AcpiRam  = (EFI_ACPI_RAM_DATA*)(EFI_ACPI_RAM_DATA*)GetAcpiRam();

    if(PcdGet8(PcdAcpiWakeupSrc) == WAK_TYPE_RTC){
      AcpiRam->IsRtcWake = TRUE;
    } else {
      AcpiRam->IsRtcWake = FALSE;
    }
    /// if S3 Resume, will reload XHCI FW.
    HandleXhciFwPei(S3Record);
    
    Status = AzaliaLoadVerbTable(gOemVerbTable, gOemVerbTableSize);
    ASSERT_EFI_ERROR(Status);    
    
    Status = KbcCmdReadData(0xAA, NULL);
    
    PlatformSettingForS3Resume();

    IssueS3PeiEndSwSmi(PeiServices);
    
  } else {           // not S3
  
    if(BootMode == BOOT_IN_RECOVERY_MODE) {
      UpdateFvHobForRecovery();
    }

    PERF_START(NULL, "CACHE2", NULL, 0);    
    SetCacheMtrrAtNormalPeiEnd(PeiServices, MemInfo);
    PERF_END  (NULL, "CACHE2", NULL, 0);    
  }


#ifndef MDEPKG_NDEBUG	
  EndOfPeiDebug(BootMode);
#endif

  return EFI_SUCCESS;
}


UINT16 FindTolum(IN ASIA_NB_CONFIGURATION *NbCfg);

UINT16
GetTolum (
  IN  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi,
  IN  ASIA_NB_CONFIGURATION            *NbCfg  
  )
{
  PLAT_NV_INFO  NvInfo;
  UINTN         VarSize;
  EFI_STATUS    Status;	
	
  VarSize = sizeof(PLAT_NV_INFO);
  Status  = Var2Ppi->GetVariable (
                      Var2Ppi,
                      NVINFO_TOLUM_VAR_NAME,
                      &gEfiPlatformNvInfoGuid,
                      NULL,
                      &VarSize,
                      &NvInfo
                      );
  if(EFI_ERROR(Status)){
    NvInfo.Tolum = FindTolum(NbCfg);
  }
  if(NvInfo.Tolum > 0xE00){NvInfo.Tolum = 0xE00;}
	
  return NvInfo.Tolum;
}



STATIC VOID ClearAcpiStatus()
{
  /// register synch with CHX001 old code base
  IoWrite16(PMIO_REG(PMIO_PM_EN), 0);
  IoWrite16(PMIO_REG(PMIO_GP_SCI_EN), 0);	  
  IoWrite16(PMIO_REG(PMIO_GP_SMI_EN), 0);
  IoWrite16(PMIO_REG(PMIO_GPI_SCIEN_REG), 0);  
  IoWrite32(PMIO_REG(PMIO_PAD_EN_REG), 0);
  IoWrite32(PMIO_REG(PMIO_PAD_STS_REG), IoRead32(PMIO_REG(PMIO_PAD_STS_REG)));
}



////
#include "vdump_asiacfg.c"
////
EFI_STATUS
EFIAPI
PlatformPeiEntry (
  IN       EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_STATUS                       Status;
  EFI_ASIA_SB_PPI                  *SbPpi;
  ASIA_SB_LIB_PPI                  *SbLibPpi;  
  EFI_ASIA_NB_PPI                  *NbPpi;
  EFI_ASIA_DRAM_PPI                *DramPpi;
  EFI_ASIA_CPU_PPI_PROTOCOL        *CpuPpi;
  EFI_PEI_SERVICES                 **PeiSrv;          // EDK does not use "CONST"
  ASIA_SB_CONFIGURATION            *SbCfg;
  ASIA_NB_CONFIGURATION            *NbCfg;
  ASIA_DRAM_CONFIGURATION          *DramCfg;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi;
  EFI_BOOT_MODE                    BootMode;
  ASIA_DRAM_INFO                   AsiaDramInfo;
  ASIA_DRAM_INFO                   *DramInfo;	
  UINT32                           MemSize;	
  PLATFORM_MEMORY_INFO             *MemInfo; 
  UINTN                            Index;
  UINT32                           RegEax;
  UINT32                           BiosUpPeiMem;
  PLATFORM_S3_RECORD               *S3Record;

  ////
  DEBUG((DEBUG_ERROR,"%a : into PlatformPeiEntry() \n",__FILE__));
  ////
  //MIKE_Debug_S  Dump CPU FMS
  AsmCpuid(0x1, &RegEax, NULL, NULL, NULL);
  DEBUG((DEBUG_ERROR,"CPU_FMS=%x\n",RegEax));
  if((AsmReadMsr64(0x1205)&0xFF)==0x01){
  	DEBUG((EFI_D_ERROR,"\n\n\n BSP Microcode update ok\n\n"));
  }
  //MIKE_Debug_E

  PeiSrv = (EFI_PEI_SERVICES**)PeiServices;
  GetAsiaPpi(&SbPpi, &NbPpi, &DramPpi, &CpuPpi, &SbLibPpi);
  SbCfg   = (ASIA_SB_CONFIGURATION*)(SbPpi->SbCfg);
  NbCfg   = (ASIA_NB_CONFIGURATION*)(NbPpi->NbCfg);
  DramCfg = (ASIA_DRAM_CONFIGURATION*)(DramPpi->DramCfg);


#ifndef MDEPKG_NDEBUG
  PlatformPeiEntryDebug(CpuPpi);
#endif



//--------------------------------- Before RC ----------------------------------
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID**)&Var2Ppi
             );
  ASSERT_EFI_ERROR(Status);

// VISA needs to know BootMode.  
  Status = BootModeInit(PeiServices, Var2Ppi, &BootMode);
  ASSERT_EFI_ERROR(Status);

  PERF_START (NULL,"ASIACFG", NULL, 0);	
  Status = UpdateAsiaConfig(BootMode, SbCfg, NbCfg, DramCfg, Var2Ppi);
  PERF_END   (NULL,"ASIACFG", NULL, 0);	
  ASSERT_EFI_ERROR(Status);    

  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PEI_CAR_CPU_INIT);
  
  Status = PeiServicesInstallPpi(&gPpiInstallList[0]);
  ASSERT_EFI_ERROR (Status);  
  Status = PeiServicesNotifyPpi(&gPpiNotifyList[0]);
  ASSERT_EFI_ERROR(Status);

  ClearAcpiStatus();





//------------------------------------ RC --------------------------------------
// Patch BootMode for NbPei.CheckWarmReboot()
  if(BootMode == BOOT_ON_FLASH_UPDATE){
    Status = (*PeiServices)->SetBootMode(PeiServices, BOOT_ON_S3_RESUME);
    ASSERT_EFI_ERROR(Status);    
  }  

#if PCAL6416A_PCIE_HOTPLUG_SUPPORT
  //PcieHotPlugInit(PeiSrv);
#endif

  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PEI_CAR_SB_INIT);
  PERF_START(NULL,"SIBM", NULL, 0);
  Status  = SbPpi->PreMemoryInit(PeiSrv, SbPpi);
   PERF_END (NULL,"SIBM", NULL, 0);	
  ASSERT_EFI_ERROR(Status);

 
  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PEI_CAR_NB_INIT);
  PERF_START(NULL,"NIBM", NULL, 0);	
  Status  = NbPpi->PreMemoryInit(PeiSrv, NbPpi);
  PERF_END  (NULL,"NIBM", NULL, 0);	
  ASSERT_EFI_ERROR(Status);
  
  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PEI_MEMORY_INIT);  
  if(BootMode == BOOT_ON_S3_RESUME || BootMode == BOOT_ON_FLASH_UPDATE){
    Status  = DramPpi->DramS3Init(PeiSrv, DramPpi);
  } else {
    DramCfg->PCIMemoryStart = GetTolum(Var2Ppi, NbCfg); // del it for SOC FPGA. 2016-06-17
	//DEBUG((EFI_D_ERROR,"mike_GetTolum_e\n"));
	// DramCfg->PCIMemoryStart = 0x800;
    PERF_START (NULL,"MRC", NULL, 0);			
	DEBUG((EFI_D_ERROR,"Begin Call DramNormalInit\n"));
    Status = DramPpi->DramNormalInit(PeiSrv, DramPpi);
    PERF_END   (NULL,"MRC", NULL, 0);		
    if(Status == EFI_NOT_FOUND){
      while(1){
        REPORT_STATUS_CODE(EFI_ERROR_CODE, PEI_MEMORY_NOT_DETECTED);
        MicroSecondDelay(2000000);
      }		
    }
  }
  ASSERT_EFI_ERROR(Status);

  if(BootMode == BOOT_ON_FLASH_UPDATE){
    Status = (*PeiServices)->SetBootMode(PeiServices, BOOT_ON_FLASH_UPDATE);
    ASSERT_EFI_ERROR(Status);   
  }  





//-------------------------------- AFTER RC ------------------------------------
  ClearAcpiStatus();   // some status cannot be cleared before, clear it again.

  MmioAnd8(LPC_PCI_REG(LPC_ROM_RANGE_REG), (UINT8)~0x0F);   // LPC FF000000h ~ FF7FFFFFh not selected, same SOC IRS.
  IoAnd8(PMIO_REG(PMIO_EXTSMI_EN_REG), (UINT8)~GP3_2ND_TIMEOUT_REBOOT);

  DramInfo = &AsiaDramInfo;
  //ASSERT(sizeof(ASIA_DRAM_INFO) <= 256);
  Status = DramPpi->DramGetInfo(PeiSrv, DramPpi, DramInfo);
  ASSERT_EFI_ERROR(Status);

  DEBUG((EFI_D_INFO, "sizeof(PLATFORM_MEMORY_INFO):%d\n", sizeof(PLATFORM_MEMORY_INFO)));
  DEBUG((EFI_D_INFO, "sizeof(PLATFORM_S3_RECORD)  :%d\n", sizeof(PLATFORM_S3_RECORD)));
  
  MemInfo = BuildGuidHob(&gEfiPlatformMemInfoGuid, sizeof(PLATFORM_MEMORY_INFO));
  ASSERT(MemInfo != NULL);
  ZeroMem(MemInfo, sizeof(PLATFORM_MEMORY_INFO));

  MemInfo->Tolum       = (UINT32)DramInfo->PciStartAddress<<20;      // TOLUM
  MemInfo->VgaBufSize  = (UINT32)DramInfo->UMASize<<20;
  //ECS20161103 patch uma enable issue   -S
  //MemInfo->VgaBufAddr  = MemInfo->Tolum - MemInfo->VgaBufSize;
  MemInfo->TSegSize    = (UINT32)DramInfo->SmmTSegSize<<20;
  //MemInfo->TSegAddr    = MemInfo->VgaBufAddr - MemInfo->TSegSize;
  MemInfo->TSegAddr    = MemInfo->Tolum - MemInfo->TSegSize;
  //ECS20161103 patch uma enable issue   -E
  MemInfo->S3DataRecord = MemInfo->TSegAddr - S3_DATA_RECORD_SIZE;
  MemInfo->S3MemoryAddr = MemInfo->S3DataRecord - S3_PEI_MEMORY_SIZE;
  MemInfo->LowMemSize   = MemInfo->S3MemoryAddr;	

  //LGE20160619 report AC Timing  for SETUP -S
  MemInfo->DramCL = DramInfo->DramCL;
  MemInfo->DramTras = DramInfo->DramTras;
  MemInfo->DramTrcd = DramInfo->DramTrcd;
  MemInfo->DramTrp = DramInfo->DramTrp;
  //LGE20160619 report AC Timing  for SETUP-E

  
  MemInfo->PhyAddrBits = 36;
  AsmCpuid(0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    MemInfo->PhyAddrBits = (UINT8)RegEax;
  }

  DEBUG((DEBUG_ERROR,"MemInfo->PhyAddrBits = %x \n",MemInfo->PhyAddrBits));
  MemInfo->Pci64Base = LShiftU64(1, MemInfo->PhyAddrBits) - PCI64_MMIO_SIZE;
  MemInfo->Pci64Size = PCI64_MMIO_SIZE;

  DEBUG((DEBUG_INFO,  "Fake(%X,%X)\n", DramInfo->FakeBegin, DramInfo->FakeLength));	
  DEBUG((DEBUG_ERROR, "LowMemSize:%X, TSegAddr:%X\n", MemInfo->LowMemSize, MemInfo->TSegAddr));
  DEBUG((DEBUG_ERROR, "VgaBufAddr:%X,L:%X\n", MemInfo->VgaBufAddr, MemInfo->VgaBufSize));
  DEBUG((DEBUG_ERROR, "S3PeiMem(%X,%X)\n", MemInfo->S3MemoryAddr, S3_PEI_MEMORY_SIZE));

  ASSERT(sizeof(PLATFORM_S3_RECORD) <= S3_DATA_RECORD_SIZE);
  ///
  DEBUG((DEBUG_ERROR,"bf Set PcdS3RecordAddr = %x \n",MemInfo->S3DataRecord));
  PcdSet32(PcdS3RecordAddr, MemInfo->S3DataRecord);
  DEBUG((DEBUG_ERROR,"MemInfo->S3DataRecord = %x \n",MemInfo->S3DataRecord));

// +----+ 64G
// |----| PCI64
// |    |
// +----+ 4G
// |____| PCI
// |    |       <-- TOLUM
// |____| FB
// |____| TSEG
// |____| S3 Data Record (4K)
// |    | S3 Memory
// |----| <-- valid memory top
// |PMM |                    
// |----| 
// |    |
// |    | Normal
// |____|
// |    | 1M
// +----+

  S3Record = (PLATFORM_S3_RECORD*)(UINTN)(MemInfo->S3DataRecord);
  if(BootMode != BOOT_ON_S3_RESUME){          // normal boot
    MemSize = 0;
    for(Index=0;Index<ASIA_MAX_RANKS;Index++){
      MemSize += DramInfo->RankInfo[Index].RankSize;
    }
    MemInfo->PhyMemSize = LShiftU64(MemSize, 20);  // MB --> B

    S3Record->Signature = PLAT_S3_RECORD_SIGNATURE;
    ZeroMem(&S3Record->AcpiRam, sizeof(S3Record->AcpiRam));
    S3Record->AcpiRam.Signature = ACPI_RAM_DATA_SIGNATURE;
    S3Record->AcpiRam.FlashSize = PcdGet32(PcdFlashAreaSize);
	//ECS20161104 Boot Windows issue   -S
    //S3Record->AcpiRam.PciBase   = MemInfo->VgaBufAddr;
    //S3Record->AcpiRam.PciLength = PCI_MMIO_TOP_ADDRESS - MemInfo->VgaBufAddr;
    S3Record->AcpiRam.PciBase   = MemInfo->Tolum;
    S3Record->AcpiRam.PciLength = PCI_MMIO_TOP_ADDRESS - MemInfo->Tolum;	
	//ECS20161104 Boot Windows  issue   -E
    S3Record->AcpiRam.AcpiWakeState = 4;   

    S3Record->S3Sleep = 0;
  }
  S3Record->DebugDataIndex = 0;

 
  if(BootMode == BOOT_ON_S3_RESUME){
    PeiServicesInstallPeiMemory(MemInfo->S3MemoryAddr, S3_PEI_MEMORY_SIZE);
    
  } else if (BootMode == BOOT_ON_FLASH_UPDATE){
    HandleCapsuleBeforeMemInstall(PeiServices, MemInfo, &BiosUpPeiMem);
    PeiServicesInstallPeiMemory(BiosUpPeiMem, PEI_BU_MEMORY_SIZE);
    
  } else if(BootMode == BOOT_IN_RECOVERY_MODE){
    PeiServicesInstallPeiMemory(PEI_MEMORY_BASE, MemInfo->LowMemSize/2);
    
  } else {
    DEBUG((EFI_D_ERROR,"PlatformPei.c : bf InstallPeiMemory \n"));
    PeiServicesInstallPeiMemory(PEI_MEMORY_BASE, PEI_MEMORY_SIZE); // 1MB, 64MB
  }

  return Status;
}


