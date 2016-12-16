
#include "PlatformPei.h"
#include <Library/MultiPlatSupportLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>
#include <SetupVariable.h>
#include <RtcDef.h>



VOID PatchSataIdeDevIdForLinux (VOID);
VOID UpdateLpcSsid(VOID);


VOID SetAfterPowerLoss(UINT8 AfterPowerLoss)
{
  if(AfterPowerLoss == AFTER_POWER_LOSS_PREVIOUS || 
     AfterPowerLoss == AFTER_POWER_LOSS_ON){          // Last or On
    SetCmosVRT(FALSE);
  } else if(AfterPowerLoss == AFTER_POWER_LOSS_OFF){  // Off
    SetCmosVRT(TRUE);  
  }
}


VOID 
HandleRtcWake(
  IN EFI_BOOT_MODE          BootMode,
  IN SETUP_DATA             *SetupData
  )
{
  UINT8           Cmos0B;
  RTC_REGISTER_B  RegisterB;  
  EFI_STATUS      Status;

  if(BootMode == BOOT_ON_S3_RESUME || BootMode == BOOT_ON_S4_RESUME){		                    // Ignore Sleep Wake.
    goto ProcExit;
  }
  if(!(IoRead16(PMIO_REG(PMIO_STS_REG)) & IoRead16(PMIO_REG(PMIO_PM_EN)) & PMIO_STS_RTC)){  // Only Handle RTC wake.
    goto ProcExit;
  }
  if(SetupData->WakeOnRTC != RTC_WAKE_VAL_SINGLE_EVENT){                                    // Only Handle Single Event.
    goto ProcExit;
  }    

  Status = RtcWaitToUpdate();
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }   
  RegisterB.Data = CmosRead(RTC_ADDRESS_REGISTER_B);
  Cmos0B = RegisterB.Data;
  RegisterB.Bits.Set = 1;   // Updates inhibited
  RegisterB.Bits.Aie = 0;  
  RegisterB.Bits.Mil = 1;		// 24 hours
  RegisterB.Bits.Dm  = 0;		// BCD Format
  CmosWrite(RTC_ADDRESS_REGISTER_B, RegisterB.Data);

  if(CmosRead(RTC_ADDRESS_SECONDS_ALARM) == DecimalToBcd8(SetupData->RTCWakeupTime.Second) &&
     CmosRead(RTC_ADDRESS_MINUTES_ALARM) == DecimalToBcd8(SetupData->RTCWakeupTime.Minute) &&
     CmosRead(RTC_ADDRESS_HOURS_ALARM)   == DecimalToBcd8(SetupData->RTCWakeupTime.Hour)   &&
     CmosRead(RTC_ADDRESS_DATE_ALARM)    == DecimalToBcd8(SetupData->RTCWakeupDate.Day)    &&
     CmosRead(RTC_ADDRESS_MONTH_ALARM)   == DecimalToBcd8(SetupData->RTCWakeupDate.Month)  &&
     CmosRead(RTC_ADDRESS_YEAR)          <  DecimalToBcd8((UINT8)(SetupData->RTCWakeupDate.Year-2000))){
    
    DEBUG((EFI_D_ERROR, "RTC Relay\n"));
    
    CmosRead(RTC_ADDRESS_REGISTER_C);  // Read 0xC to clear pending RTC interrupts  	
    RegisterB.Bits.Aie = 1;
    RegisterB.Bits.Set = 0;
    CmosWrite(RTC_ADDRESS_REGISTER_B, RegisterB.Data);
    
    IoWrite16(PMIO_REG(PMIO_STS_REG), PMIO_STS_RTC);
    IoOr16(PMIO_REG(PMIO_PM_EN), PMIO_PM_EN_RTC);
    
    DEBUG((EFI_D_ERROR, "\n"));    
    SystemSoftOff();   
  }

  CmosWrite(RTC_ADDRESS_REGISTER_B, Cmos0B);

ProcExit:
  return;  
}







EFI_STATUS
EFIAPI
UpdateAsiaConfig (
  IN     EFI_BOOT_MODE                    BootMode,
  IN OUT ASIA_SB_CONFIGURATION            *SbCfg,
  IN OUT ASIA_NB_CONFIGURATION            *NbCfg,
  IN OUT ASIA_DRAM_CONFIGURATION          *DramCfg,
  IN     EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi
  )
{
  EFI_STATUS             Status;
  SETUP_DATA             *SetupData;
  SETUP_DATA             *VarHobSetupData;	
  UINTN                  Size;
  BOOLEAN                IsCmosBad;
  //UINT32                 RegEax, RegEbx, RegEcx, RegEdx;
	

  SetupData = BuildGuidHob(&gPlatformSetupVariableGuid, sizeof(SETUP_DATA));
  ASSERT(SetupData!=NULL);
  
// CMOS 0x0E is a normal storage. If RTC battery is LOST, this value will change to 0xFF.
// So we use this feature to judge RTC power loss or not.
// When setup variable has been sync from HOB, set it as ZERO.
  IsCmosBad = (CmosRead(0x0E) & BIT7)?TRUE:FALSE;
  DEBUG((EFI_D_INFO, "IsCmosBad:%d\n", IsCmosBad));

  if(IsCmosBad || BootMode == BOOT_IN_RECOVERY_MODE || BootMode == BOOT_ON_FLASH_UPDATE){
   goto LoadSetupDefault;
  }	 
  
  Size   = sizeof(SETUP_DATA);  
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      PLATFORM_SETUP_VARIABLE_NAME,
                      &gPlatformSetupVariableGuid,
                      NULL,
                      &Size,
                      SetupData
                      );
  if(!EFI_ERROR(Status)){
    goto GotSetupVariable;		
  } else {
    DEBUG((EFI_D_INFO, "GetSetupVar:%r\n", Status));
  }	
	
LoadSetupDefault:
  Status = CreateDefaultVariableHob(EFI_HII_DEFAULT_CLASS_STANDARD, 0, (VOID**)&VarHobSetupData);
  ASSERT_EFI_ERROR(Status);
  
// FCE tool has some limitation for gathering default value, so do override here.
  VarHobSetupData->RTCWakeupDate.Year   = 2016;
  VarHobSetupData->RTCWakeupDate.Month  = 1;
  VarHobSetupData->RTCWakeupDate.Day    = 1;
  VarHobSetupData->RTCWakeupTime.Hour   = 0;
  VarHobSetupData->RTCWakeupTime.Minute = 0;
  VarHobSetupData->RTCWakeupTime.Second = 0;
  if(VarHobSetupData->Csm){
    VarHobSetupData->UefiPxeIPV4 = 0;
    VarHobSetupData->UefiPxeIPV6 = 0;
  }
  
  Size   = sizeof(SETUP_DATA);  
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      PLATFORM_SETUP_VARIABLE_NAME,
                      &gPlatformSetupVariableGuid,
                      NULL,
                      &Size,
                      SetupData
                      );
  ASSERT_EFI_ERROR(Status);
  ASSERT(SetupData->RTCWakeupDate.Year == 2016);		


//------------------------------------------------------------------------------
GotSetupVariable:

  DEBUG((EFI_D_ERROR, "SetupData->Csm = %x \n",SetupData->Csm));
  //chipset sb
  SbCfg->ApplicationModeSelection= SetupData->ApplicationModeSelection;
  ///
  SbCfg->SBSPEValue= SetupData->SBSPEValue;
  SbCfg->VARTSPEValue= SetupData->VARTSPEValue;
  SbCfg->ESPISPEValue   = SetupData->ESPISPEValue;	  
  SbCfg->SDIOSPEValue= SetupData->SDIOSPEValue;
  SbCfg->BusCtrlSPEValue= SetupData->BusCtrlSPEValue;
  SbCfg->PMUSPEValue= SetupData->PMUSPEValue;
  SbCfg->PCCASPEValue= SetupData->PCCASPEValue;
  SbCfg->HDACSPEValue= SetupData->HDACSPEValue;   
  SbCfg->SPISPEValue=SetupData->SPISPEValue;
 // SbCfg->P2PBSPEValue = 2; 
  
  SbCfg->SbApicID	= 9;
  //SbCfg->AHCIGen=SetupData->AHCIGen;  

  //UART
  if(PcdGetBool(PcdBiosDebugUsePciUart) == TRUE) {  
    /// default should come here (PlatformPkg.dec)	
    SbCfg->OnChipUartMode= SetupData->OnChipUartMode;
    SbCfg->UartModuleSelection= SetupData->UartModuleSelection;
    SbCfg->Uart0Enable= SetupData->Uart0Enable;
    SbCfg->Uart0IoBaseSelection= SetupData->Uart0IoBaseSelection;
    SbCfg->Uart0IRQSelection= SetupData->Uart0IRQSelection;
	
    SbCfg->Uart1Enable= SetupData->Uart1Enable;
    SbCfg->Uart1IoBaseSelection= SetupData->Uart1IoBaseSelection;
    SbCfg->Uart1IRQSelection= SetupData->Uart1IRQSelection;
	
    SbCfg->Uart2Enable= SetupData->Uart2Enable;
    SbCfg->Uart2IoBaseSelection= SetupData->Uart2IoBaseSelection;
    SbCfg->Uart2IRQSelection= SetupData->Uart2IRQSelection;
	
    SbCfg->Uart3Enable= SetupData->Uart3Enable;
    SbCfg->Uart3IoBaseSelection= SetupData->Uart3IoBaseSelection;
    SbCfg->Uart3IRQSelection= SetupData->Uart3IRQSelection;   

  } else {
    SbCfg->OnChipUartMode = ON_CHIP_UART_MODE_DISABLED;
  }
  
  SbCfg->ESPI = SetupData->ESPI;
  SbCfg->SPIBus0ClockSelect= SetupData->SPIBus0ClockSelect;
  SbCfg->PmioBar = PcdGet16(AcpiIoPortBaseAddress);
  SbCfg->MobileCenterControl= SetupData->MobileCenterControl;
  SbCfg->ACAdapterControl= SetupData->ACAdapterControl;
  SbCfg->MsiSupport= SetupData->MsiSupport;
  ///
  SbCfg->SBDynamicClkControl  = SetupData->SBDynamicClkControl;  
  SbCfg->GoNonSnoopPath       = SetupData->GoNonSnoopPath; 

  SbCfg->PowerCtrleBMC       = SetupData->PowerCtrleBMC; 
  SbCfg->eBMCSettleTime       = SetupData->eBMCSettleTime; 
  SbCfg->KBDCLegacySelControl       = SetupData->KBDCLegacySelControl; 
  SbCfg->INTCLegacySelControl       = SetupData->INTCLegacySelControl; 
  SbCfg->DMACLegacySelControl      = SetupData->DMACLegacySelControl; 
  SbCfg->TMRCLegacySelControl       = SetupData->TMRCLegacySelControl; 
  
  //PMU_ACPI
  SbCfg->ProcessorCState= SetupData->CpuCState;
  SbCfg->ConditionalC4= SetupData->ConditionalC4;
  
  //AsmCpuid(1, &RegEax, &RegEbx, &RegEcx, &RegEdx);
  //if CNQ001 or new processor, then support C5.
  //if ((((RegEax & 0xFFFFF) >= 0x6FD) && ((RegEax & 0xFF000) == 0x0)) || ((RegEax & 0xFFFFF) >= 0x10690))
  //{
		SbCfg->C5Control = SetupData->C5Control;
  //}else
  //{
  //	SbCfg->C5Control = 0;
  //}

   SbCfg->VidSelect = SetupData->VID_SEL;
  
  ////
  SbCfg->C4BusMasterIdleTimer= SetupData->C4BusMasterIdleTimer;
  SbCfg->C3BusMasterIdleTimer= SetupData->C3BusMasterIdleTimer;
  SbCfg->ShortC3C4Mode= SetupData->ShortC3C4Mode;
  SbCfg->DPSLPtoSLP= SetupData->DPSLPtoSLP;
  SbCfg->VRDSLPtoDPSLP= SetupData->VRDSLPtoDPSLP;
  SbCfg->FixedFreeCxLatency= SetupData->FixedFreeCxLatency;
  SbCfg->DynamicT05= SetupData->DynamicT05;
  ///
  
  //HDAC
  SbCfg->Azalia= SetupData->ObAudioEn;
  SbCfg->HDACFLREn=SetupData->HDACFLREn;

  //SNMI
  SbCfg->IsoLPC= SetupData->IsoLPC;
  SbCfg->IsoVART= SetupData->IsoVART;
  SbCfg->IsoAZALIA= SetupData->IsoAZALIA;
  SbCfg->IsoESPI= SetupData->IsoESPI;  
  SbCfg->IsoSDIO= SetupData->IsoSDIO;
  SbCfg->IsoSPI= SetupData->IsoSPI;
  
  //SDIO 
  SbCfg->SdioCtrlEn  = SetupData->SdioCtrlEn;
  //DBZ-20160808 add, Start.
  SbCfg->EmmcHs400StrobeMode = SetupData->EmmcHs400StrobeMode;
  SbCfg->EmmcCqeEnable = SetupData->EmmcCqeEnable;
  //DBZ-20160808 add, End.
  //DBZ-20160808 SbCfg->SdioDTSel = 1;
  //DBZ-20160808 SbCfg->SdioSDR50En = 1;
  //DBZ-20160808 SbCfg->SdioSDR104En=1;
  //DBZ-20160808 SbCfg->SdioDDR50En = 1;
  //DBZ-20160808 SbCfg->SdioSDR50TuningEn = 1;

  NbCfg->PrimaryGraphicAdapter = SetupData->VideoPrimaryAdapter; // Setup UI's default value = PCIE GFX Card
  NbCfg->DualVGA               = SetupData->VideoDualVga; // Setup UI's default value = Disabled (0)

  NbCfg->PcieFPGAMode = TRUE;
  
  //RAIDA
  NbCfg->RAIDA0Enable = SetupData->RAIDA0En;
  NbCfg->RAIDA1Enable = SetupData->RAIDA1En;
  NbCfg->RAIDASIDLock = SetupData->RAIDASIDLock;

  NbCfg->ApplicationModeSelection= SetupData->ApplicationModeSelection;
  NbCfg->NBSPEValue= SetupData->NBSPEValue;
  NbCfg->D0F0SPEValue= SetupData->D0F0SPEValue;
  NbCfg->D0F1SPEValue= SetupData->D0F1SPEValue;
  NbCfg->D0F2SPEValue= SetupData->D0F2SPEValue;
  NbCfg->D0F3SPEValue= SetupData->D0F3SPEValue;
  NbCfg->D0F4SPEValue= SetupData->D0F4SPEValue;
  NbCfg->D0F5SPEValue= SetupData->D0F5SPEValue;
  NbCfg->D0F6SPEValue= SetupData->D0F6SPEValue;
  NbCfg->D2F0SPEValue= SetupData->D2F0SPEValue;
  NbCfg->D3F0SPEValue= SetupData->D3F0SPEValue;
  NbCfg->D3F1SPEValue =  SetupData->D3F1SPEValue;
  NbCfg->D3F2SPEValue =  SetupData->D3F2SPEValue;
  NbCfg->D3F3SPEValue =  SetupData->D3F3SPEValue;
  ///
  NbCfg->D4F0SPEValue= SetupData->D4F0SPEValue;
  NbCfg->D4F1SPEValue =  SetupData->D4F1SPEValue;
  NbCfg->D5F0SPEValue= SetupData->D5F0SPEValue;
  NbCfg->D5F1SPEValue =  SetupData->D5F1SPEValue;
  NbCfg->D8F0SPEValue =  SetupData->D8F0SPEValue;
  NbCfg->D9F0SPEValue =  SetupData->D9F0SPEValue;
  NbCfg->RCRBHSPEValue= SetupData->RCRBHSPEValue;
  NbCfg->PCIEEPHYSPEValue= SetupData->PCIEEPHYSPEValue;
  NbCfg->D1F0SPEValue= SetupData->D1F0SPEValue; // 2 : means Energy Value
  NbCfg->MMIOSPEValue= SetupData->MMIOSPEValue;

  //PCIE related
  NbCfg->PcieRst= SetupData->PcieRst;
  NbCfg->PcieRstPEG= SetupData->PcieRstPEG;
  NbCfg->PcieRstPE0= SetupData->PcieRstPE0;
  NbCfg->PcieRstPE1= SetupData->PcieRstPE1;
  NbCfg->PcieRstPE2= SetupData->PcieRstPE2;
  NbCfg->PcieRstPE3= SetupData->PcieRstPE3;
  NbCfg->PcieRstPEG0= SetupData->PcieRstPEG0;
  NbCfg->PcieRstPEG1= SetupData->PcieRstPEG1;
  NbCfg->PcieRstPEG2= SetupData->PcieRstPEG2;
  NbCfg->PcieRstPEG3= SetupData->PcieRstPEG3;

  NbCfg->PcieLinkSpeed= SetupData->PcieLinkSpeed;
  NbCfg->PciePEGLinkSpeed= SetupData->PciePEGLinkSpeed;		//RP Speed Select
  NbCfg->PciePE0LinkSpeed= SetupData->PciePE0LinkSpeed;
  NbCfg->PciePE1LinkSpeed= SetupData->PciePE1LinkSpeed;
  NbCfg->PciePE2LinkSpeed= SetupData->PciePE2LinkSpeed;
  NbCfg->PciePE3LinkSpeed= SetupData->PciePE3LinkSpeed;
  NbCfg->PciePEG0LinkSpeed= SetupData->PciePEG0LinkSpeed;
  NbCfg->PciePEG1LinkSpeed= SetupData->PciePEG1LinkSpeed;
  NbCfg->PciePEG2LinkSpeed= SetupData->PciePEG2LinkSpeed;
  NbCfg->PciePEG3LinkSpeed= SetupData->PciePEG3LinkSpeed;
  ///  
  NbCfg->PcieRP= SetupData->PcieRP;  
  NbCfg->PciePEG= SetupData->PciePEG;
  NbCfg->PciePE0= SetupData->PciePE0;
  NbCfg->PciePE1= SetupData->PciePE1;
  NbCfg->PciePE2= SetupData->PciePE2;
  NbCfg->PciePE3= SetupData->PciePE3;
  NbCfg->PciePEG0= SetupData->PciePEG0;
  NbCfg->PciePEG1= SetupData->PciePEG1;
  NbCfg->PciePEG2= SetupData->PciePEG2;
  NbCfg->PciePEG3= SetupData->PciePEG3; 

  NbCfg->PcieEQ=SetupData->PcieEQ;
  NbCfg->PciePreDSTxX8BL0=SetupData->PciePreDSTxX8BL0; 
  NbCfg->PciePreDSRxX8BL0=SetupData->PciePreDSRxX8BL0; 
  NbCfg->PciePreUSTxX8BL0=SetupData->PciePreUSTxX8BL0; 
  NbCfg->PciePreUSRxX8BL0=SetupData->PciePreUSRxX8BL0; 
  NbCfg->PciePreDSTxX8BL1=SetupData->PciePreDSTxX8BL1; 
  NbCfg->PciePreDSRxX8BL1=SetupData->PciePreDSRxX8BL1; 
  NbCfg->PciePreUSTxX8BL1=SetupData->PciePreUSTxX8BL1; 
  NbCfg->PciePreUSRxX8BL1=SetupData->PciePreUSRxX8BL1; 
  NbCfg->PciePreDSTxX8BL2=SetupData->PciePreDSTxX8BL2; 
  NbCfg->PciePreDSRxX8BL2=SetupData->PciePreDSRxX8BL2; 
  NbCfg->PciePreUSTxX8BL2=SetupData->PciePreUSTxX8BL2; 
  NbCfg->PciePreUSRxX8BL2=SetupData->PciePreUSRxX8BL2; 
  NbCfg->PciePreDSTxX8BL3=SetupData->PciePreDSTxX8BL3; 
  NbCfg->PciePreDSRxX8BL3=SetupData->PciePreDSRxX8BL3; 
  NbCfg->PciePreUSTxX8BL3=SetupData->PciePreUSTxX8BL3; 
  NbCfg->PciePreUSRxX8BL3=SetupData->PciePreUSRxX8BL3; 
  NbCfg->PciePreDSTxX8BL4=SetupData->PciePreDSTxX8BL4; 
  NbCfg->PciePreDSRxX8BL4=SetupData->PciePreDSRxX8BL4; 
  NbCfg->PciePreUSTxX8BL4=SetupData->PciePreUSTxX8BL4; 
  NbCfg->PciePreUSRxX8BL4=SetupData->PciePreUSRxX8BL4; 
  NbCfg->PciePreDSTxX8BL5=SetupData->PciePreDSTxX8BL5; 
  NbCfg->PciePreDSRxX8BL5=SetupData->PciePreDSRxX8BL5; 
  NbCfg->PciePreUSTxX8BL5=SetupData->PciePreUSTxX8BL5; 
  NbCfg->PciePreUSRxX8BL5=SetupData->PciePreUSRxX8BL5; 
  NbCfg->PciePreDSTxX8BL6=SetupData->PciePreDSTxX8BL6; 
  NbCfg->PciePreDSRxX8BL6=SetupData->PciePreDSRxX8BL6; 
  NbCfg->PciePreUSTxX8BL6=SetupData->PciePreUSTxX8BL6; 
  NbCfg->PciePreUSRxX8BL6=SetupData->PciePreUSRxX8BL6; 
  NbCfg->PciePreDSTxX8BL7=SetupData->PciePreDSTxX8BL7; 
  NbCfg->PciePreDSRxX8BL7=SetupData->PciePreDSRxX8BL7; 
  NbCfg->PciePreUSTxX8BL7=SetupData->PciePreUSTxX8BL7; 
  NbCfg->PciePreUSRxX8BL7=SetupData->PciePreUSRxX8BL7; 
  NbCfg->PciePreDSTxX8CL0=SetupData->PciePreDSTxX8CL0; 
  NbCfg->PciePreDSRxX8CL0=SetupData->PciePreDSRxX8CL0; 
  NbCfg->PciePreUSTxX8CL0=SetupData->PciePreUSTxX8CL0; 
  NbCfg->PciePreUSRxX8CL0=SetupData->PciePreUSRxX8CL0; 
  NbCfg->PciePreDSTxX8CL1=SetupData->PciePreDSTxX8CL1; 
  NbCfg->PciePreDSRxX8CL1=SetupData->PciePreDSRxX8CL1; 
  NbCfg->PciePreUSTxX8CL1=SetupData->PciePreUSTxX8CL1; 
  NbCfg->PciePreUSRxX8CL1=SetupData->PciePreUSRxX8CL1; 
  NbCfg->PciePreDSTxX8CL2=SetupData->PciePreDSTxX8CL2; 
  NbCfg->PciePreDSRxX8CL2=SetupData->PciePreDSRxX8CL2; 
  NbCfg->PciePreUSTxX8CL2=SetupData->PciePreUSTxX8CL2; 
  NbCfg->PciePreUSRxX8CL2=SetupData->PciePreUSRxX8CL2; 
  NbCfg->PciePreDSTxX8CL3=SetupData->PciePreDSTxX8CL3; 
  NbCfg->PciePreDSRxX8CL3=SetupData->PciePreDSRxX8CL3; 
  NbCfg->PciePreUSTxX8CL3=SetupData->PciePreUSTxX8CL3; 
  NbCfg->PciePreUSRxX8CL3=SetupData->PciePreUSRxX8CL3; 
  NbCfg->PciePreDSTxX8CL4=SetupData->PciePreDSTxX8CL4; 
  NbCfg->PciePreDSRxX8CL4=SetupData->PciePreDSRxX8CL4; 
  NbCfg->PciePreUSTxX8CL4=SetupData->PciePreUSTxX8CL4; 
  NbCfg->PciePreUSRxX8CL4=SetupData->PciePreUSRxX8CL4; 
  NbCfg->PciePreDSTxX8CL5=SetupData->PciePreDSTxX8CL5; 
  NbCfg->PciePreDSRxX8CL5=SetupData->PciePreDSRxX8CL5; 
  NbCfg->PciePreUSTxX8CL5=SetupData->PciePreUSTxX8CL5; 
  NbCfg->PciePreUSRxX8CL5=SetupData->PciePreUSRxX8CL5; 
  NbCfg->PciePreDSTxX8CL6=SetupData->PciePreDSTxX8CL6; 
  NbCfg->PciePreDSRxX8CL6=SetupData->PciePreDSRxX8CL6; 
  NbCfg->PciePreUSTxX8CL6=SetupData->PciePreUSTxX8CL6; 
  NbCfg->PciePreUSRxX8CL6=SetupData->PciePreUSRxX8CL6; 
  NbCfg->PciePreDSTxX8CL7=SetupData->PciePreDSTxX8CL7; 
  NbCfg->PciePreDSRxX8CL7=SetupData->PciePreDSRxX8CL7; 
  NbCfg->PciePreUSTxX8CL7=SetupData->PciePreUSTxX8CL7; 
  NbCfg->PciePreUSRxX8CL7=SetupData->PciePreUSRxX8CL7;
  NbCfg->PciePreDSTxX8AL0=SetupData->PciePreDSTxX8AL0; 
  NbCfg->PciePreDSRxX8AL0=SetupData->PciePreDSRxX8AL0; 
  NbCfg->PciePreUSTxX8AL0=SetupData->PciePreUSTxX8AL0; 
  NbCfg->PciePreUSRxX8AL0=SetupData->PciePreUSRxX8AL0; 
  NbCfg->PciePreDSTxX8AL1=SetupData->PciePreDSTxX8AL1; 
  NbCfg->PciePreDSRxX8AL1=SetupData->PciePreDSRxX8AL1; 
  NbCfg->PciePreUSTxX8AL1=SetupData->PciePreUSTxX8AL1; 
  NbCfg->PciePreUSRxX8AL1=SetupData->PciePreUSRxX8AL1; 
  NbCfg->PciePreDSTxX8AL2=SetupData->PciePreDSTxX8AL2; 
  NbCfg->PciePreDSRxX8AL2=SetupData->PciePreDSRxX8AL2; 
  NbCfg->PciePreUSTxX8AL2=SetupData->PciePreUSTxX8AL2; 
  NbCfg->PciePreUSRxX8AL2=SetupData->PciePreUSRxX8AL2; 
  NbCfg->PciePreDSTxX8AL3=SetupData->PciePreDSTxX8AL3; 
  NbCfg->PciePreDSRxX8AL3=SetupData->PciePreDSRxX8AL3; 
  NbCfg->PciePreUSTxX8AL3=SetupData->PciePreUSTxX8AL3; 
  NbCfg->PciePreUSRxX8AL3=SetupData->PciePreUSRxX8AL3; 
  NbCfg->PciePreDSTxX8AL4=SetupData->PciePreDSTxX8AL4; 
  NbCfg->PciePreDSRxX8AL4=SetupData->PciePreDSRxX8AL4; 
  NbCfg->PciePreUSTxX8AL4=SetupData->PciePreUSTxX8AL4; 
  NbCfg->PciePreUSRxX8AL4=SetupData->PciePreUSRxX8AL4; 
  NbCfg->PciePreDSTxX8AL5=SetupData->PciePreDSTxX8AL5; 
  NbCfg->PciePreDSRxX8AL5=SetupData->PciePreDSRxX8AL5; 
  NbCfg->PciePreUSTxX8AL5=SetupData->PciePreUSTxX8AL5; 
  NbCfg->PciePreUSRxX8AL5=SetupData->PciePreUSRxX8AL5; 
  NbCfg->PciePreDSTxX8AL6=SetupData->PciePreDSTxX8AL6; 
  NbCfg->PciePreDSRxX8AL6=SetupData->PciePreDSRxX8AL6; 
  NbCfg->PciePreUSTxX8AL6=SetupData->PciePreUSTxX8AL6; 
  NbCfg->PciePreUSRxX8AL6=SetupData->PciePreUSRxX8AL6; 
  NbCfg->PciePreDSTxX8AL7=SetupData->PciePreDSTxX8AL7; 
  NbCfg->PciePreDSRxX8AL7=SetupData->PciePreDSRxX8AL7; 
  NbCfg->PciePreUSTxX8AL7=SetupData->PciePreUSTxX8AL7; 
  NbCfg->PciePreUSRxX8AL7=SetupData->PciePreUSRxX8AL7; 

  ////
  NbCfg->PEMCU_LoadFW_WhenBoot=SetupData->PEMCU_LoadFW_WhenBoot;
  //NbCfg->PEMCU_RstSys_WhenFail=SetupData->PEMCU_RstSys_WhenFail;   

  NbCfg->PcieHotReset= SetupData->PcieHotReset;
  NbCfg->MaxPayloadSize= SetupData->MaxPayloadSize; // Setup UI's default value = 256 bytes
  NbCfg->PcieASPM= SetupData->PcieASPM; // default = disabled
  NbCfg->RelaxedOrder= SetupData->RelaxedOrder;
  NbCfg->ExtTag= SetupData->ExtTag;
  NbCfg->ExtSync= SetupData->ExtSync; 

  NbCfg->PEGErrControl= SetupData->PEGErrControl;
  NbCfg->PE0ErrControl= SetupData->PE0ErrControl;
  NbCfg->PE1ErrControl= SetupData->PE1ErrControl;
  NbCfg->PE2ErrControl= SetupData->PE2ErrControl;
  NbCfg->PE3ErrControl= SetupData->PE3ErrControl;
  NbCfg->PEG0ErrControl= SetupData->PEG0ErrControl;
  NbCfg->PEG1ErrControl= SetupData->PEG1ErrControl;
  NbCfg->PEG2ErrControl= SetupData->PEG2ErrControl;
  NbCfg->PEG3ErrControl= SetupData->PEG3ErrControl; 

  NbCfg->PEG_Msgc2PcieIntx = SetupData->PEG_Msgc2PcieIntx;
  NbCfg->PE0_Msgc2PcieIntx = SetupData->PE0_Msgc2PcieIntx;
  NbCfg->PE1_Msgc2PcieIntx = SetupData->PE1_Msgc2PcieIntx;
  NbCfg->PE2_Msgc2PcieIntx = SetupData->PE2_Msgc2PcieIntx;
  NbCfg->PE3_Msgc2PcieIntx = SetupData->PE3_Msgc2PcieIntx;
  ///
  NbCfg->PEG0_Msgc2PcieIntx = SetupData->PEG0_Msgc2PcieIntx;
  NbCfg->PEG1_Msgc2PcieIntx = SetupData->PEG1_Msgc2PcieIntx;
  NbCfg->PEG2_Msgc2PcieIntx = SetupData->PEG2_Msgc2PcieIntx;
  NbCfg->PEG3_Msgc2PcieIntx = SetupData->PEG3_Msgc2PcieIntx;
  ///
  NbCfg->PEG_PcieIntx2Nb2sbIntx = SetupData->PEG_PcieIntx2Nb2sbIntx;
  NbCfg->PE0_PcieIntx2Nb2sbIntx = SetupData->PE0_PcieIntx2Nb2sbIntx;
  NbCfg->PE1_PcieIntx2Nb2sbIntx = SetupData->PE1_PcieIntx2Nb2sbIntx;
  NbCfg->PE2_PcieIntx2Nb2sbIntx = SetupData->PE2_PcieIntx2Nb2sbIntx;
  NbCfg->PE3_PcieIntx2Nb2sbIntx = SetupData->PE3_PcieIntx2Nb2sbIntx;
  ///
  NbCfg->PEG0_PcieIntx2Nb2sbIntx = SetupData->PEG0_PcieIntx2Nb2sbIntx;
  NbCfg->PEG1_PcieIntx2Nb2sbIntx = SetupData->PEG1_PcieIntx2Nb2sbIntx;
  NbCfg->PEG2_PcieIntx2Nb2sbIntx = SetupData->PEG2_PcieIntx2Nb2sbIntx;
  NbCfg->PEG3_PcieIntx2Nb2sbIntx = SetupData->PEG3_PcieIntx2Nb2sbIntx;
  ///
  NbCfg->MsiEnable = SetupData->MsiEnable;
  NbCfg->PegApciIrq = SetupData->PegApciIrq;
  NbCfg->Peg2ApciIrq = SetupData->Peg2ApciIrq;
  NbCfg->Peg3ApciIrq = SetupData->Peg3ApciIrq;

  NbCfg->CPU_FSBC_PCIE_ON=SetupData->CPU_FSBC_PCIE_ON;
  NbCfg->CPU_FSBC_TOPCIE=SetupData->CPU_FSBC_TOPCIE;

  NbCfg->CPU_FSBC_MISSPACKE_EN=SetupData->CPU_FSBC_MISSPACKE_EN;
  NbCfg->CPU_FSBC_TIGPULSE_EN=SetupData->CPU_FSBC_TIGPULSE_EN;
  NbCfg->CPU_FSBC_IFSBCSTP_EN=SetupData->CPU_FSBC_IFSBCSTP_EN;

  NbCfg->CPU_FSBC_STREAM_EN=SetupData->CPU_FSBC_STREAM_EN;
  
  
  /// Synched here.
  //OTHERS
  SbCfg->EnableMultimediaTimer= SetupData->EnableMultimediaTimer;
  SbCfg->EnableMultimediaTimerMsi= SetupData->EnableMultimediaTimerMsi;
  SbCfg->MultimediaTimerMode= SetupData->MultimediaTimerMode;
  SbCfg->WatchDogTimer= SetupData->WatchDogTimer;
  SbCfg->WatchDogTimerRunStop= SetupData->WatchDogTimerRunStop;
  SbCfg->WatchDogTimerAction= SetupData->WatchDogTimerAction;
  SbCfg->WatchDogTimerCount= SetupData->WatchDogTimerCount;
  SbCfg->KBMouseWakeupControl= SetupData->KBMouseWakeupControl;
  SbCfg->SMBusControllerUnderOS= SetupData->SMBusControllerUnderOS;
  SbCfg->SMBHostClockFrequencySelect= SetupData->SMBusHostClockFrequencySelect;
  SbCfg->SMBHostClockFrequency= SetupData->SMBusHostClockFrequency;

  
  //Chipset NB  
  NbCfg->NbApicID = 10; 


  NbCfg->CpuClockControl= SetupData->CpuClockControl;
  //NbCfg->CpuSSCControl= SetupData->CpuSSCControl;
  NbCfg->PcieClockControl= SetupData->PcieClockControl;
  //NbCfg->CpuBclkControl= SetupData->CpuBclkControl;

  NbCfg->SERRNBControl=SetupData->SERRNBControl; 


  NbCfg->UMAEn                 = SetupData->UMAEn;
  NbCfg->SelectDisplayDevice   = SetupData->SelectDisplayDevice;
  NbCfg->DP1        = SetupData->DP1;
  NbCfg->DP2        = SetupData->DP2;
  NbCfg->DVO        = SetupData->DVO;
  NbCfg->CRT        = SetupData->CRT;
  NbCfg->PanelType= SetupData->PanelType;
  NbCfg->PanelType2= SetupData->PanelType2;
  NbCfg->ECLKCtrl= SetupData->ECLKCtrl;
  NbCfg->ECLKFreq= SetupData->ECLKFreq; 
  NbCfg->VCLKCtrl= SetupData->VCLKCtrl;
  NbCfg->VCLKFreq= SetupData->VCLKFreq;
  NbCfg->ICLKCtrl= SetupData->ICLKCtrl;
  NbCfg->ICLKFreq= SetupData->ICLKFreq;
  NbCfg->DP1SSCEn= SetupData->DP1SSCEn;
  NbCfg->DP1SSCMode= SetupData->DP1SSCMode;
  NbCfg->DP2SSCEn= SetupData->DP2SSCEn;
  NbCfg->DP2SSCMode= SetupData->DP2SSCMode;
  NbCfg->IOVEnable= SetupData->IOVEnable;   
  //NbCfg->RoundRobin=SetupData->RoundRobin;

  //DRAM
  	 DramCfg->DramClk = SetupData->DramClk;    
	 DramCfg->RxIoTimingMethod= SetupData->RxIoTimingMethod;
     DramCfg->TxIoTimingMethod= SetupData->TxIoTimingMethod;
	 DramCfg->LimitRankSize= SetupData->LimitRankSize;
     DramCfg->BankIntlv= SetupData->BankIntlv;
     DramCfg->MemoryChipODTDebug= SetupData->MemoryChipODTDebug;    
     DramCfg->MemoryChipODTWRDebug= SetupData->MemoryChipODTWRDebug;
	 DramCfg->MemoryChipODTParkDebug = SetupData->MemoryChipODTParkDebug;
     DramCfg->VRIntlv= SetupData->VRInlv;    
     DramCfg->ChannelIntlv= SetupData->ChannelIntlv;	
	 DramCfg->DRAMECC= SetupData->DramECC;
	 DramCfg->DataScmb= SetupData->DataScmb;	
     DramCfg->BurstLength= SetupData->BL;
     DramCfg->CmdRate= SetupData->CmdRate;    
     DramCfg->VGAShareMemory= SetupData->VGAShareMemory;    
     DramCfg->DramInitMethod= SetupData->DramInitMethod;        
     DramCfg->DramSelfRefresh= SetupData->DramSelfRefresh;
     DramCfg->DynamicCKE= SetupData->DynamicCKE;
     DramCfg->RemapEn= SetupData->RemapEn;
	 DramCfg->DramFastBoot= SetupData->DramFastBoot;
	 DramCfg->CRCEn= SetupData->CRCEn;   //CRC Enable
	 DramCfg->CAParEn= SetupData->CAParEn;  //CA Parity check Enable
	 DramCfg->CAParPerEn= SetupData->CAParPerEn; //CA Parity persistent Enable
	 DramCfg->WPREA= SetupData->WPREA;  //Write preamble	    
	 DramCfg->RPREA= SetupData->RPREA;  //read preambleR	   
	 DramCfg->CALEn= SetupData->CALEn; //CAL Latency enable
	 DramCfg->ACTimingOption = SetupData->ACTimingOption;	
	 DramCfg->DramCL= SetupData->DramCL;	
	 DramCfg->DramTrcd= SetupData->DramTrcd;	
	 DramCfg->DramTrp= SetupData->DramTrp;	
	 DramCfg->DramTras= SetupData->DramTras;	
	 DramCfg->CRCParRetryEn= SetupData->CRCParRetryEn;
 	 DramCfg->ParErrControl= SetupData->ParErrControl;   
	 DramCfg->CRCErrControl= SetupData->CRCErrControl;		
	 DramCfg->EccRetry= SetupData->EccRetry;	
	 DramCfg->BASelect= SetupData->BASelect;
	 DramCfg->EccPatrolScrub= SetupData->EccPatrolScrub;
	 DramCfg->DRAMErrControl= SetupData->DRAMErrControl;
	 DramCfg->CPU_FSBC_ON= SetupData->CPU_FSBC_DRAM_ON; //FSBC Debug Mode



// do override here.
//-----------------------------------------------------------
  //NbCfg->PciePE1 = SetupData->OnboardLan; // should check CHX001 EVB Board.

  ////
  NbCfg->DebugMode= SetupData->DebugMode;
  NbCfg->DebugOutputSelect= SetupData->DebugOutputSelect;
  NbCfg->OutPadSelect= SetupData->OutPadSelect;
  NbCfg->DebugSignalSelect0= SetupData->DebugSignalSelect0;
  NbCfg->DebugSignalSelect1= SetupData->DebugSignalSelect1;  
  NbCfg->DebugModuleSelect0= SetupData->DebugModuleSelect0;
  NbCfg->DebugModuleSelect1= SetupData->DebugModuleSelect1;  

  NbCfg->NBGroup0TopSelect= SetupData->NBGroup0TopSelcet;
  NbCfg->NBGroup0ModuleSubSelect= SetupData->NBGroup0ModuleSubSelect;
  NbCfg->NBGroup1TopSelect= SetupData->NBGroup1TopSelcet;
  NbCfg->NBGroup1ModuleSubSelect= SetupData->NBGroup1ModuleSubSelect;
  NbCfg->DebugSBselectByNB= SetupData->DebugSBSelectByNB;
  NbCfg->SBTopDbgMux1= SetupData->SBTopDbgMux1;

  NbCfg->CCAMux1Sub= SetupData->CCAMux1Sub; 
  NbCfg->PCI1Mux1Sub= SetupData->PCI1Mux1Sub; 
  NbCfg->SMBMux1Sub= SetupData->SMBMux1Sub;
  NbCfg->SDHCMux1Sub= SetupData->SDHCMux1Sub;  
  NbCfg->ESPIMux1Sub= SetupData->ESPIMux1Sub; 
  NbCfg->SPIMux1Sub= SetupData->SPIMux1Sub; 
  NbCfg->PMUMux1Sub= SetupData->PMUMux1Sub;	
  NbCfg->AZMux1Sub= SetupData->AZMux1Sub;	
  NbCfg->HPETMux1Sub= SetupData->HPETMux1Sub;	
  NbCfg->eBMC_OUT1Mux1Sub=SetupData->EBMC_OUT1Mux1Sub;
  NbCfg->VARTMux1Sub= SetupData->VARTMux1Sub;   
  NbCfg->SDHC_TXRXMux1Sub=SetupData->SDHC_TXRXMux1Sub; 
  NbCfg->DMACMux1Sub= SetupData->DMACMux1Sub;	 
  NbCfg->PNPMux1Sub= SetupData->PNPMux1Sub; 
  NbCfg->CCAG2Mux1Sub= SetupData->CCAG2Mux1Sub;
  NbCfg->PCI1G2Mux1Sub= SetupData->PCI1G2Mux1Sub;
  NbCfg->PMUG2Mux1Sub= SetupData->PMUG2Mux1Sub;
  NbCfg->KBDCMux1Sub= SetupData->KBDCMux1Sub;   
  NbCfg->TMRCMux1Sub= SetupData->TMRCMux1Sub;
  NbCfg->PICMux1Sub= SetupData->PICMux1Sub;
  NbCfg->LPCMux1Sub= SetupData->LPCMux1Sub; 
  NbCfg->BUSC_MISCMux1Sub= SetupData->BUSC_MISCMux1Sub; 
  NbCfg->RTCMux1Sub= SetupData->RTCMux1Sub; 

  NbCfg->SBTopDbgMux2= SetupData->SBTopDbgMux2;
  NbCfg->PMUG2Mux2Sub= SetupData->PMUG2Mux2Sub;
  NbCfg->CCAMux2Sub= SetupData->CCAMux2Sub;
  NbCfg->PCI1Mux2Sub= SetupData->PCI1Mux2Sub;
  NbCfg->SMB_G2Mux2Sub= SetupData->SMB_G2Mux2Sub;
  NbCfg->SDHC_G2Mux2Sub= SetupData->SDHC_G2Mux2Sub; 
  NbCfg->ESPI_G2Mux2Sub= SetupData->ESPI_G2Mux2Sub; 
  NbCfg->SPI_G2Mux2Sub= SetupData->SPI_G2Mux2Sub;
  NbCfg->PMUMux2Sub= SetupData->PMUMux2Sub;	
  NbCfg->AZ_G2Mux2Sub= SetupData->AZ_G2Mux2Sub;	
  NbCfg->HPETMux2Sub= SetupData->HPETMux2Sub;	
  NbCfg->eBMC_OUT2Mux2Sub=SetupData->EBMC_OUT1Mux2Sub;
  NbCfg->VART_G2Mux2Sub=SetupData->VART_G2Mux2Sub; 
  NbCfg->DMAC_G2Mux2Sub=SetupData->DMAC_G2Mux2Sub; 	   
  NbCfg->PNP_G2Mux2Sub=SetupData->PNP_G2Mux2Sub; 
  NbCfg->CCA_G2Mux2Sub=SetupData->CCA_G2Mux2Sub; 
  NbCfg->PCI1_G2Mux2Sub=SetupData->PCI1_G2Mux2Sub; 
  NbCfg->KBDC_G2Mux2Sub= SetupData->KBDC_G2Mux2Sub;
  NbCfg->TMRC_G2Mux2Sub= SetupData->TMRC_G2Mux2Sub;
  NbCfg->PIC_G2Mux2Sub= SetupData->PIC_G2Mux2Sub;  
  NbCfg->LPC_G2Mux2Sub= SetupData->LPC_G2Mux2Sub;
  NbCfg->BUSC_MISCMux2Sub= SetupData->BUSC_MISCMux2Sub;
  NbCfg->RTCMux2Sub= SetupData->RTCMux2Sub;
  NbCfg->TOPGroup0TopSelcet= SetupData->TOPGroup0TopSelcet;  
  NbCfg->TOPGroup0ModuleSubSelcet= SetupData->TOPGroup0ModuleSubSelcet;  

  NbCfg->TOPGroup1TopSelcet= SetupData->TOPGroup1TopSelcet;
  NbCfg->TOPGroup1ModuleSubSelcet= SetupData->TOPGroup1ModuleSubSelcet;  
  NbCfg->SB_HS_DBG_SEL= SetupData->SB_HS_DBG_SEL;
  NbCfg->SB_HS_PCIE_PORT_SEL= SetupData->SB_HS_PCIE_PORT_SEL;
  NbCfg->SB_HS_DBG_CH_SEL= SetupData->SB_HS_DBG_CH_SEL;

//#ifdef CHIPSET100_EVB3
//  SbCfg->Port12TOPort13 = FALSE;
//#endif

  if(BootMode == BOOT_IN_RECOVERY_MODE){
    //SbCfg->Port0ToPort3 = TRUE;
    //SbCfg->SataCfg = SATA_CFG_IDE;
    //SbCfg->SataChannelMode = SATA_CHN_MODE_COMPATIBLE; 
    //SbCfg->UsbModeSelect = USB_MODE_SEL_MODEB;
  }    
  
  SetAfterPowerLoss(SetupData->AfterPowerLoss);
  HandleRtcWake(BootMode, SetupData);

  //if(SetupData->SbIdeLinuxPatchEn && SbCfg->SataCfg == SATA_CFG_IDE){
  //  PatchSataIdeDevIdForLinux();
  //} else {
  //  UpdateLpcSsid();
  //}

  
#ifdef ZX_SECRET_CODE
  PcdSet8(PcdCpuCoreEnabled, SetupData->KillAp);
#endif
  
  return Status;  
}  

