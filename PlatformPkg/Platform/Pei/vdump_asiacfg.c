///
VOID VPei_DumpAsiaNbCfg(
	IN EFI_PEI_SERVICES** PeiServices,
	IN ASIA_NB_CONFIGURATION            *NbCfg
)
{
	////
	DEBUG((DEBUG_ERROR,"ASIA_NB_CONFIGURATION struct : \n ----------\n"));
	/*DEBUG((DEBUG_ERROR,"ApplicationModeSelection = %x \n",NbCfg->ApplicationModeSelection));
	DEBUG((DEBUG_ERROR,"NBSPEValue = %x \n",NbCfg->NBSPEValue));	
	DEBUG((DEBUG_ERROR,"D0F0SPEValue = %x \n",NbCfg->D0F0SPEValue));	
	
	DEBUG((DEBUG_ERROR,"D0F1SPEValue = %x \n",NbCfg->D0F1SPEValue));	
	DEBUG((DEBUG_ERROR,"D0F2SPEValue = %x \n",NbCfg->D0F2SPEValue));	
	DEBUG((DEBUG_ERROR,"D0F3SPEValue = %x \n",NbCfg->D0F3SPEValue));	
	DEBUG((DEBUG_ERROR,"D0F4SPEValue = %x \n",NbCfg->D0F4SPEValue));	

	DEBUG((DEBUG_ERROR,"D0F5SPEValue = %x \n",NbCfg->D0F5SPEValue));	
	DEBUG((DEBUG_ERROR,"D0F6SPEValue = %x \n",NbCfg->D0F6SPEValue));	
	DEBUG((DEBUG_ERROR,"D0F7SPEValue = %x \n",NbCfg->D0F7SPEValue));	
	DEBUG((DEBUG_ERROR,"D2F0SPEValue = %x \n",NbCfg->D2F0SPEValue));	

	DEBUG((DEBUG_ERROR,"D3F0SPEValue = %x \n",NbCfg->D3F0SPEValue));
	DEBUG((DEBUG_ERROR,"D3F1SPEValue = %x \n",NbCfg->D3F1SPEValue));
	DEBUG((DEBUG_ERROR,"D3F2SPEValue = %x \n",NbCfg->D3F2SPEValue));
	DEBUG((DEBUG_ERROR,"D3F3SPEValue = %x \n",NbCfg->D3F3SPEValue));
	DEBUG((DEBUG_ERROR,"D4F0SPEValue = %x \n",NbCfg->D4F0SPEValue));
	DEBUG((DEBUG_ERROR,"D4F1SPEValue = %x \n",NbCfg->D4F1SPEValue));
	DEBUG((DEBUG_ERROR,"D5F0SPEValue = %x \n",NbCfg->D5F0SPEValue));
	DEBUG((DEBUG_ERROR,"D5F1SPEValue = %x \n",NbCfg->D5F1SPEValue));
	//CHX001-CJW-20151014   DEBUG((DEBUG_ERROR,"D6F0SPEValue = %x \n",NbCfg->D6F0SPEValue));	

	//CHX001-CJW-20151014   DEBUG((DEBUG_ERROR,"D7F0SPEValue = %x \n",NbCfg->D7F0SPEValue));	
	DEBUG((DEBUG_ERROR,"D1F0SPEValue = %x \n",NbCfg->D1F0SPEValue));	
	DEBUG((DEBUG_ERROR,"MMIOSPEValue = %x \n",NbCfg->MMIOSPEValue));	
	DEBUG((DEBUG_ERROR,"PcieBar = %lx \n",NbCfg->PcieBar));	

	DEBUG((DEBUG_ERROR,"PcieEPHYBar = %lx \n",NbCfg->PcieEPHYBar));	
	DEBUG((DEBUG_ERROR,"RcrbhBar = %lx \n",NbCfg->RcrbhBar));	
	DEBUG((DEBUG_ERROR,"RcrbvBar = %lx \n",NbCfg->RcrbvBar));*/	

	DEBUG((DEBUG_ERROR,"PcieRst = %x \n",NbCfg->PcieRst)); 
	DEBUG((DEBUG_ERROR,"PcieRstPEG = %x \n",NbCfg->PcieRstPEG));	
	DEBUG((DEBUG_ERROR,"PcieRstPEG0 = %x \n",NbCfg->PcieRstPEG0));	
	DEBUG((DEBUG_ERROR,"PcieRstPE0 = %x \n",NbCfg->PcieRstPE0));	

	DEBUG((DEBUG_ERROR,"PcieRstPE1 = %x \n",NbCfg->PcieRstPE1)); 
	DEBUG((DEBUG_ERROR,"PcieRstPE2 = %x \n",NbCfg->PcieRstPE2));	
	DEBUG((DEBUG_ERROR,"PcieRstPE3 = %x \n",NbCfg->PcieRstPE3));	

	DEBUG((DEBUG_ERROR,"PcieRstPEG1 = %x \n",NbCfg->PcieRstPEG1)); 
	DEBUG((DEBUG_ERROR,"PcieRP = %x \n",NbCfg->PcieRP));	
	DEBUG((DEBUG_ERROR,"PciePEG = %x \n",NbCfg->PciePEG));	
	DEBUG((DEBUG_ERROR,"PciePEG0 = %x \n",NbCfg->PciePEG0));	

	DEBUG((DEBUG_ERROR,"PciePE0 = %x \n",NbCfg->PciePE0)); 
	DEBUG((DEBUG_ERROR,"PciePE1 = %x \n",NbCfg->PciePE1));	
	DEBUG((DEBUG_ERROR,"PciePE2 = %x \n",NbCfg->PciePE2));	
	DEBUG((DEBUG_ERROR,"PciePE3 = %x \n",NbCfg->PciePE3));	

	DEBUG((DEBUG_ERROR,"PciePEG1 = %x \n",NbCfg->PciePEG1));	
	DEBUG((DEBUG_ERROR,"PciePEG2 = %x \n",NbCfg->PciePEG2));	
	DEBUG((DEBUG_ERROR,"PciePEG3 = %x \n",NbCfg->PciePEG3));	
	DEBUG((DEBUG_ERROR,"PcieHotReset = %x \n",NbCfg->PcieHotReset));	
	DEBUG((DEBUG_ERROR,"PcieLinkSpeed = %x \n",NbCfg->PcieLinkSpeed));	

	DEBUG((DEBUG_ERROR,"MaxPayloadSize = %x \n",NbCfg->MaxPayloadSize));

	DEBUG((DEBUG_ERROR,"PcieASPM = %x \n",NbCfg->PcieASPM));
	/*DEBUG((DEBUG_ERROR,"RelaxedOrder = %x \n",NbCfg->RelaxedOrder));	
	DEBUG((DEBUG_ERROR,"ExtTag = %x \n",NbCfg->ExtTag));	
	DEBUG((DEBUG_ERROR,"ExtSync = %x \n",NbCfg->ExtSync));

	DEBUG((DEBUG_ERROR,"PcieRpDisplaySwitch = %x \n",NbCfg->PcieRpDisplaySwitch));
	DEBUG((DEBUG_ERROR,"PcieSpeedDisplaySwitch = %x \n",NbCfg->PcieSpeedDisplaySwitch));	
	DEBUG((DEBUG_ERROR,"NbApicEnable = %x \n",NbCfg->NbApicEnable));	
	DEBUG((DEBUG_ERROR,"NbApicBaseAddress = %lx \n",NbCfg->NbApicBaseAddress));
	DEBUG((DEBUG_ERROR,"NbApicID = %x \n",NbCfg->NbApicID));
	DEBUG((DEBUG_ERROR,"DramSelfRefresh = %x \n",NbCfg->DramSelfRefresh));	
	DEBUG((DEBUG_ERROR,"UMAEn = %x \n",NbCfg->UMAEn));	
	DEBUG((DEBUG_ERROR,"DualVGA = %x \n",NbCfg->DualVGA));

	DEBUG((DEBUG_ERROR,"PrimaryGraphicAdapter = %x \n",NbCfg->PrimaryGraphicAdapter));
	DEBUG((DEBUG_ERROR,"SelectDisplayDevice = %x \n",NbCfg->SelectDisplayDevice));	
	DEBUG((DEBUG_ERROR,"DisplayDevice1 = %x \n",NbCfg->DisplayDevice1));	
	DEBUG((DEBUG_ERROR,"DisplayDevice2 = %x \n",NbCfg->DisplayDevice2));
	DEBUG((DEBUG_ERROR,"PanelType = %x \n",NbCfg->PanelType));
	DEBUG((DEBUG_ERROR,"PanelType2 = %x \n",NbCfg->PanelType2));	
	DEBUG((DEBUG_ERROR,"TVLayout = %x \n",NbCfg->TVLayout));	
	DEBUG((DEBUG_ERROR,"TVType = %x \n",NbCfg->TVType));

	DEBUG((DEBUG_ERROR,"TVConnector = %x \n",NbCfg->TVConnector));
	DEBUG((DEBUG_ERROR,"HDTVType = %x \n",NbCfg->HDTVType));	
	DEBUG((DEBUG_ERROR,"HDTVConnector = %x \n",NbCfg->HDTVConnector));	
	DEBUG((DEBUG_ERROR,"ECLKCtrl = %x \n",NbCfg->ECLKCtrl));
	DEBUG((DEBUG_ERROR,"ECLKFreq = %x \n",NbCfg->ECLKFreq));
	DEBUG((DEBUG_ERROR,"VCLKCtrl = %x \n",NbCfg->VCLKCtrl));	
	DEBUG((DEBUG_ERROR,"VCLKFreq = %x \n",NbCfg->VCLKFreq));	
	DEBUG((DEBUG_ERROR,"LCD1SSCEn = %x \n",NbCfg->LCD1SSCEn));

	DEBUG((DEBUG_ERROR,"LCD2SSCEn = %x \n",NbCfg->LCD2SSCEn));
	DEBUG((DEBUG_ERROR,"LCD1SSCPath = %x \n",NbCfg->LCD1SSCPath));	
	DEBUG((DEBUG_ERROR,"LCD2SSCPath = %x \n",NbCfg->LCD2SSCPath));	
	DEBUG((DEBUG_ERROR,"LCD1SSCMode = %x \n",NbCfg->LCD1SSCMode));
	DEBUG((DEBUG_ERROR,"LCD2SSCMode = %x \n",NbCfg->LCD2SSCMode));
	DEBUG((DEBUG_ERROR,"LinuxVideoIP = %x \n",NbCfg->LinuxVideoIP));	
	DEBUG((DEBUG_ERROR,"CpuClockControl = %x \n",NbCfg->CpuClockControl));	
	DEBUG((DEBUG_ERROR,"PcieClockControl = %x \n",NbCfg->PcieClockControl));

	DEBUG((DEBUG_ERROR,"VIACpuBclkControl = %x \n",NbCfg->VIACpuBclkControl));
	DEBUG((DEBUG_ERROR,"DebugMode = %x \n",NbCfg->DebugMode));	
	DEBUG((DEBUG_ERROR,"DbgSignalNumSelect = %x \n",NbCfg->DbgSignalNumSelect));
	DEBUG((DEBUG_ERROR,"DebugSelect = %x \n",NbCfg->DebugSelect));
	DEBUG((DEBUG_ERROR,"NBGroup0TopSelect = %x \n",NbCfg->NBGroup0TopSelect));
	DEBUG((DEBUG_ERROR,"NBGroup0ModuleSubSelect = %x \n",NbCfg->NBGroup0ModuleSubSelect));	
	DEBUG((DEBUG_ERROR,"NBGroup1TopSelect = %x \n",NbCfg->NBGroup1TopSelect));	
	DEBUG((DEBUG_ERROR,"NBGroup1ModuleSubSelect = %x \n",NbCfg->NBGroup1ModuleSubSelect));

	DEBUG((DEBUG_ERROR,"SBTopDbgMux1 = %x \n",NbCfg->SBTopDbgMux1));
	DEBUG((DEBUG_ERROR,"CCAMux1Sub     = %x \n",NbCfg->CCAMux1Sub));	
	DEBUG((DEBUG_ERROR,"CCAG2Mux1Sub = %x \n",NbCfg->CCAG2Mux1Sub));
	DEBUG((DEBUG_ERROR,"PCI1Mux1Sub     = %x \n",NbCfg->PCI1Mux1Sub));
	DEBUG((DEBUG_ERROR,"PCI1G2Mux1Sub = %x \n",NbCfg->PCI1G2Mux1Sub));
	DEBUG((DEBUG_ERROR,"CRMuxSub          = %x \n",NbCfg->CRMuxSub));	
	DEBUG((DEBUG_ERROR,"SDIOMux1Sub     = %x \n",NbCfg->SDIOMux1Sub));	
	DEBUG((DEBUG_ERROR,"USBMux1Sub      = %x \n",NbCfg->USBMux1Sub));

	DEBUG((DEBUG_ERROR,"AZMux1Sub       = %x \n",NbCfg->AZMux1Sub));
	DEBUG((DEBUG_ERROR,"USBDMux1Sub   = %x \n",NbCfg->USBDMux1Sub));	
	DEBUG((DEBUG_ERROR,"SATAMux1Sub   = %x \n",NbCfg->SATAMux1Sub));
	DEBUG((DEBUG_ERROR,"VARTMux1Sub   = %x \n",NbCfg->VARTMux1Sub));
	DEBUG((DEBUG_ERROR,"PMUG2Mux1Sub = %x \n",NbCfg->PMUG2Mux1Sub));
	DEBUG((DEBUG_ERROR,"HDAC2MCUMux1Sub = %x \n",NbCfg->HDAC2MCUMux1Sub));	
	DEBUG((DEBUG_ERROR,"PNPMux1Sub             = %x \n",NbCfg->PNPMux1Sub));	
	DEBUG((DEBUG_ERROR,"SBTopDbgMux2         = %x \n",NbCfg->SBTopDbgMux2));

	DEBUG((DEBUG_ERROR,"CCAMux2Sub = %x \n",NbCfg->CCAMux2Sub));
	DEBUG((DEBUG_ERROR,"CCAG2Mux2Sub = %x \n",NbCfg->CCAG2Mux2Sub));	
	DEBUG((DEBUG_ERROR,"PCI1Mux2Sub = %x \n",NbCfg->PCI1Mux2Sub));
	DEBUG((DEBUG_ERROR,"PCI1G2Mux2Sub = %x \n",NbCfg->PCI1G2Mux2Sub));
	DEBUG((DEBUG_ERROR,"CRG2Mux2Sub = %x \n",NbCfg->CRG2Mux2Sub));
	DEBUG((DEBUG_ERROR,"SDIOG2Mux2Sub = %x \n",NbCfg->SDIOG2Mux2Sub));	
	DEBUG((DEBUG_ERROR,"PMUG2Mux2Sub = %x \n",NbCfg->PMUG2Mux2Sub));	
	DEBUG((DEBUG_ERROR,"AZG2Mux2Sub = %x \n",NbCfg->AZG2Mux2Sub));

	DEBUG((DEBUG_ERROR,"VARTG2Mux2Sub = %x \n",NbCfg->VARTG2Mux2Sub));
	DEBUG((DEBUG_ERROR,"USBCG2Mux2Sub = %x \n",NbCfg->USBCG2Mux2Sub));	
	DEBUG((DEBUG_ERROR,"USBDG2Mux2Sub = %x \n",NbCfg->USBDG2Mux2Sub));
	DEBUG((DEBUG_ERROR,"SATAG2Mux2Sub = %x \n",NbCfg->SATAG2Mux2Sub));
	DEBUG((DEBUG_ERROR,"HDAC2MCUG2Mux2Sub = %x \n",NbCfg->HDAC2MCUG2Mux2Sub));
	DEBUG((DEBUG_ERROR,"PNPG2Mux2Sub             = %x \n",NbCfg->PNPG2Mux2Sub));	
	DEBUG((DEBUG_ERROR,"CPU_OPTCFG                = %x \n",NbCfg->CPU_OPTCFG));	
	DEBUG((DEBUG_ERROR,"TOP_DBG_SEL               = %x \n",NbCfg->TOP_DBG_SEL));

	DEBUG((DEBUG_ERROR,"TOP_DBG_SEL2    = %x \n",NbCfg->TOP_DBG_SEL2));
	DEBUG((DEBUG_ERROR,"XHCI_DBG_SEL     = %x \n",NbCfg->XHCI_DBG_SEL));	
	DEBUG((DEBUG_ERROR,"XHCI_DBG_SEL2    = %x \n",NbCfg->XHCI_DBG_SEL2));
	DEBUG((DEBUG_ERROR,"MCU_DBG_SEL       = %x \n",NbCfg->MCU_DBG_SEL));
	DEBUG((DEBUG_ERROR,"U2HUB_DBG_SEL   = %x \n",NbCfg->U2HUB_DBG_SEL));
	DEBUG((DEBUG_ERROR,"U2HUB_DBG_SEL2 = %x \n",NbCfg->U2HUB_DBG_SEL2));	
	DEBUG((DEBUG_ERROR,"USB3IP_DBG_SEL   = %x \n",NbCfg->USB3IP_DBG_SEL));	
	DEBUG((DEBUG_ERROR,"USB3IP_DBG_SEL2 = %x \n",NbCfg->USB3IP_DBG_SEL2));

	DEBUG((DEBUG_ERROR,"SS_ASYNCSTP_PARM = %x \n",NbCfg->SS_ASYNCSTP_PARM));
	DEBUG((DEBUG_ERROR,"HS_ASYNCSTP_PARM = %x \n",NbCfg->HS_ASYNCSTP_PARM));	
	DEBUG((DEBUG_ERROR,"SB_HS_DBG_SEL = %x \n",NbCfg->SB_HS_DBG_SEL));
	DEBUG((DEBUG_ERROR,"SB_HS_DBG_CH_SEL = %x \n",NbCfg->SB_HS_DBG_CH_SEL));
	DEBUG((DEBUG_ERROR,"MCU_UART_SEL = %x \n",NbCfg->MCU_UART_SEL));
	DEBUG((DEBUG_ERROR,"VcpFileBaseAddr = %x \n",NbCfg->VcpFileBaseAddr));	
	DEBUG((DEBUG_ERROR,"VcpFileSize = %x \n",NbCfg->VcpFileSize));	
	DEBUG((DEBUG_ERROR,"IOVEnable = %x \n",NbCfg->IOVEnable));

	DEBUG((DEBUG_ERROR,"TABar = %lx \n",NbCfg->TABar));	

	///
	DEBUG((DEBUG_ERROR,"End of Dump Asia_Nb_Config struct \n ----------\n"));	*/
}

VOID VPei_DumpAsiaSbCfg(
	IN EFI_PEI_SERVICES** PeiServices,
	IN ASIA_SB_CONFIGURATION            *SbCfg
)
{
	////
	DEBUG((DEBUG_ERROR,"ASIA_SB_CONFIGURATION struct : \n ----------\n"));
	DEBUG((DEBUG_ERROR,"ApplicationModeSelection = %x \n",SbCfg->ApplicationModeSelection));
	DEBUG((DEBUG_ERROR,"SBSPEValue = %x \n",SbCfg->SBSPEValue));	
	DEBUG((DEBUG_ERROR,"VARTSPEValue = %x \n",SbCfg->VARTSPEValue));	
	
	DEBUG((DEBUG_ERROR,"SDIOSPEValue = %x \n",SbCfg->SDIOSPEValue));	
	//DEBUG((DEBUG_ERROR,"SATASPEValue = %x \n",SbCfg->SATASPEValue));	
	//DEBUG((DEBUG_ERROR,"UHCISPEValue = %x \n",SbCfg->UHCISPEValue));	
	//DEBUG((DEBUG_ERROR,"EHCISPEValue = %x \n",SbCfg->EHCISPEValue));	

	DEBUG((DEBUG_ERROR,"BusCtrlSPEValue = %x \n",SbCfg->BusCtrlSPEValue));	
	DEBUG((DEBUG_ERROR,"PMUSPEValue = %x \n",SbCfg->PMUSPEValue));	
	DEBUG((DEBUG_ERROR,"PCCASPEValue = %x \n",SbCfg->PCCASPEValue));	
	//DEBUG((DEBUG_ERROR,"xHCISPEValue = %x \n",SbCfg->xHCISPEValue));	

	//DEBUG((DEBUG_ERROR,"P2PBSPEValue = %x \n",SbCfg->P2PBSPEValue));	
	DEBUG((DEBUG_ERROR,"HDACSPEValue = %x \n",SbCfg->HDACSPEValue));	
	DEBUG((DEBUG_ERROR,"OnChipUartMode = %x \n",SbCfg->OnChipUartMode));	
	DEBUG((DEBUG_ERROR,"UartModuleSelection = %x \n",SbCfg->UartModuleSelection));	

	DEBUG((DEBUG_ERROR,"Uart0Enable = %x \n",SbCfg->Uart0Enable));	
	DEBUG((DEBUG_ERROR,"Uart0IoBaseSelection = %x \n",SbCfg->Uart0IoBaseSelection));	
	DEBUG((DEBUG_ERROR,"Uart0IRQSelection = %x \n",SbCfg->Uart0IRQSelection));	
	DEBUG((DEBUG_ERROR,"Uart1Enable = %x \n",SbCfg->Uart1Enable));	

	DEBUG((DEBUG_ERROR,"Uart1IoBaseSelection = %x \n",SbCfg->Uart1IoBaseSelection));	
	DEBUG((DEBUG_ERROR,"Uart1IRQSelection = %x \n",SbCfg->Uart1IRQSelection));	
	DEBUG((DEBUG_ERROR,"Uart2Enable = %x \n",SbCfg->Uart2Enable));	

	DEBUG((DEBUG_ERROR,"Uart2IoBaseSelection = %x \n",SbCfg->Uart2IoBaseSelection)); 
	DEBUG((DEBUG_ERROR,"Uart2IRQSelection = %x \n",SbCfg->Uart2IRQSelection));	
	DEBUG((DEBUG_ERROR,"Uart3Enable = %x \n",SbCfg->Uart3Enable));	

	DEBUG((DEBUG_ERROR,"Uart3IoBaseSelection = %x \n",SbCfg->Uart3IoBaseSelection)); 
	DEBUG((DEBUG_ERROR,"Uart3IRQSelection = %x \n",SbCfg->Uart3IRQSelection));	

	DEBUG((DEBUG_ERROR,"SdioCtrlEn = %x \n",SbCfg->SdioCtrlEn)); 
	//DBZ-20160808 DEBUG((DEBUG_ERROR,"SdioVer30En = %x \n",SbCfg->SdioVer30En));	
	//DBZ-20160808 DEBUG((DEBUG_ERROR,"SdioVS18En = %x \n",SbCfg->SdioVS18En));	
	//DBZ-20160808 
	//DBZ-20160808 DEBUG((DEBUG_ERROR,"SdioHSEn = %x \n",SbCfg->SdioHSEn)); 
	//DBZ-20160808 DEBUG((DEBUG_ERROR,"SdioCF = %x \n",SbCfg->SdioCF));	
	//DBZ-20160808 DEBUG((DEBUG_ERROR,"SdioDTSel = %x \n",SbCfg->SdioDTSel));	
	//DBZ-20160808 DEBUG((DEBUG_ERROR,"SdioSDR50En = %x \n",SbCfg->SdioSDR50En));	
	//DBZ-20160808 
	//DBZ-20160808 DEBUG((DEBUG_ERROR,"SdioSDR104En = %x \n",SbCfg->SdioSDR104En));	
	//DBZ-20160808 DEBUG((DEBUG_ERROR,"SdioDDR50En = %x \n",SbCfg->SdioDDR50En));	
	//DBZ-20160808 DEBUG((DEBUG_ERROR,"SdioSDR50TuningEn = %x \n",SbCfg->SdioSDR50TuningEn));	
	//DBZ-20160808 
	//DBZ-20160808 DEBUG((DEBUG_ERROR,"SdioTimerCountForRetuning = %x \n",SbCfg->SdioTimerCountForRetuning));
	//DEBUG((DEBUG_ERROR,"SataCfg = %x \n",SbCfg->SataCfg));
	//DEBUG((DEBUG_ERROR,"SataChannelMode = %x \n",SbCfg->SataChannelMode));	

	//DEBUG((DEBUG_ERROR,"AhciFuncEnable = %x \n",SbCfg->AhciFuncEnable));	
	//DEBUG((DEBUG_ERROR,"Port0HpcpEn = %x \n",SbCfg->Port0HpcpEn));
	//DEBUG((DEBUG_ERROR,"Port1HpcpEn = %x \n",SbCfg->Port1HpcpEn));

	//DEBUG((DEBUG_ERROR,"AsynchronousRecovery = %x \n",SbCfg->AsynchronousRecovery));	
	//DEBUG((DEBUG_ERROR,"SupportAhciHipmTimer = %x \n",SbCfg->SupportAhciHipmTimer));	
	//DEBUG((DEBUG_ERROR,"SataCapPort0 = %x \n",SbCfg->SataCapPort0));
	//DEBUG((DEBUG_ERROR,"SataCapPort1 = %x \n",SbCfg->SataCapPort1));

	//DEBUG((DEBUG_ERROR,"SataHIPMPort0 = %x \n",SbCfg->SataHIPMPort0));	
	//DEBUG((DEBUG_ERROR,"SataHIPMPort1 = %x \n",SbCfg->SataHIPMPort1));	
	//DEBUG((DEBUG_ERROR,"SataOddHipmDisabled = %x \n",SbCfg->SataOddHipmDisabled));

	//DEBUG((DEBUG_ERROR,"UsbModeSelect = %x \n",SbCfg->UsbModeSelect));
	//DEBUG((DEBUG_ERROR,"TRBCacheBypass = %x \n",SbCfg->TRBCacheBypass));	
	DEBUG((DEBUG_ERROR,"ProcessorCState = %x \n",SbCfg->ProcessorCState));	

	DEBUG((DEBUG_ERROR,"ConditionalC4 = %x \n",SbCfg->ConditionalC4));
	DEBUG((DEBUG_ERROR,"C5Control = %x \n",SbCfg->C5Control));
	//DEBUG((DEBUG_ERROR,"C4PControl = %x \n",SbCfg->C4PControl));	
	//DEBUG((DEBUG_ERROR,"BackC4PtoC2 = %x \n",SbCfg->BackC4PtoC2));	
	//DEBUG((DEBUG_ERROR,"C4PBusMasterIdleTimer = %x \n",SbCfg->C4PBusMasterIdleTimer));

	DEBUG((DEBUG_ERROR,"C4BusMasterIdleTimer = %x \n",SbCfg->C4BusMasterIdleTimer));
	DEBUG((DEBUG_ERROR,"C3BusMasterIdleTimer = %x \n",SbCfg->C3BusMasterIdleTimer));	
	//DEBUG((DEBUG_ERROR,"C4PResumeTimer = %x \n",SbCfg->C4PResumeTimer));	
	//DEBUG((DEBUG_ERROR,"SataC4P = %x \n",SbCfg->SataC4P));
	//DEBUG((DEBUG_ERROR,"UsbC4P = %x \n",SbCfg->UsbC4P));
	//DEBUG((DEBUG_ERROR,"XhciC4P = %x \n",SbCfg->XhciC4P));	
	//DEBUG((DEBUG_ERROR,"NMC4P = %x \n",SbCfg->NMC4P));	
	DEBUG((DEBUG_ERROR,"ShortC3C4Mode = %x \n",SbCfg->ShortC3C4Mode));

	DEBUG((DEBUG_ERROR,"DPSLPtoSLP = %x \n",SbCfg->DPSLPtoSLP));
	DEBUG((DEBUG_ERROR,"VRDSLPtoDPSLP = %x \n",SbCfg->VRDSLPtoDPSLP));	
	DEBUG((DEBUG_ERROR,"SBDynamicClkControl = %x \n",SbCfg->SBDynamicClkControl));	
	//DEBUG((DEBUG_ERROR,"EHCISleepTime = %x \n",SbCfg->EHCISleepTime));
	DEBUG((DEBUG_ERROR,"MobileCenterControl = %x \n",SbCfg->MobileCenterControl));	
	DEBUG((DEBUG_ERROR,"ACAdapterControl = %x \n",SbCfg->ACAdapterControl));	
	DEBUG((DEBUG_ERROR,"PLLOKSelectionControl = %x \n",SbCfg->PLLOKSelectionControl));

	//DEBUG((DEBUG_ERROR,"UsbS4Wakeup = %x \n",SbCfg->UsbS4Wakeup));
	DEBUG((DEBUG_ERROR,"MsiSupport = %x \n",SbCfg->MsiSupport));	
	DEBUG((DEBUG_ERROR,"FixedFreeCxLatency = %x \n",SbCfg->FixedFreeCxLatency));	
	DEBUG((DEBUG_ERROR,"DynamicT05 = %x \n",SbCfg->DynamicT05));

	DEBUG((DEBUG_ERROR,"PowerCtrleBMC = %x \n",SbCfg->PowerCtrleBMC));
	DEBUG((DEBUG_ERROR,"eBMCSettleTime = %x \n",SbCfg->eBMCSettleTime));
	DEBUG((DEBUG_ERROR,"KBDCLegacySelControl = %x \n",SbCfg->KBDCLegacySelControl));
	DEBUG((DEBUG_ERROR,"INTCLegacySelControl = %x \n",SbCfg->INTCLegacySelControl));
	DEBUG((DEBUG_ERROR,"DMACLegacySelControl = %x \n",SbCfg->DMACLegacySelControl));
	DEBUG((DEBUG_ERROR,"TMRCLegacySelControl = %x \n",SbCfg->TMRCLegacySelControl));

	DEBUG((DEBUG_ERROR,"Azalia = %x \n",SbCfg->Azalia));
	DEBUG((DEBUG_ERROR,"GoNonSnoopPath = %x \n",SbCfg->GoNonSnoopPath));	
	DEBUG((DEBUG_ERROR,"IsoLPC = %x \n",SbCfg->IsoLPC));
	DEBUG((DEBUG_ERROR,"IsoVART = %x \n",SbCfg->IsoVART));

	DEBUG((DEBUG_ERROR,"IsoAZALIA = %x \n",SbCfg->IsoAZALIA));
	DEBUG((DEBUG_ERROR,"IsoUSBC = %x \n",SbCfg->IsoESPI));	
	DEBUG((DEBUG_ERROR,"IsoSDIO = %x \n",SbCfg->IsoSDIO));	
	DEBUG((DEBUG_ERROR,"IsoSPI = %x \n",SbCfg->IsoSPI));
	DEBUG((DEBUG_ERROR,"IsoAPIC = %x \n",SbCfg->IsoAPIC));

	DEBUG((DEBUG_ERROR,"EnableMultimediaTimer = %x \n",SbCfg->EnableMultimediaTimer));	
	DEBUG((DEBUG_ERROR,"MultimediaTimerMode = %x \n",SbCfg->MultimediaTimerMode));	
	DEBUG((DEBUG_ERROR,"WatchDogTimer = %x \n",SbCfg->WatchDogTimer));

	DEBUG((DEBUG_ERROR,"WatchDogTimerRunStop = %x \n",SbCfg->WatchDogTimerRunStop));	
	DEBUG((DEBUG_ERROR,"WatchDogTimerAction = %x \n",SbCfg->WatchDogTimerAction));	
	DEBUG((DEBUG_ERROR,"WatchDogTimerCount = %x \n",SbCfg->WatchDogTimerCount));

	DEBUG((DEBUG_ERROR,"KBMouseWakeupControl = %x \n",SbCfg->KBMouseWakeupControl));
	DEBUG((DEBUG_ERROR,"SMBusControllerUnderOS = %x \n",SbCfg->SMBusControllerUnderOS));
	DEBUG((DEBUG_ERROR,"SMBHostClockFrequencySelect = %x \n",SbCfg->SMBHostClockFrequencySelect));
	DEBUG((DEBUG_ERROR,"SMBHostClockFrequency = %x \n",SbCfg->SMBHostClockFrequency));
	DEBUG((DEBUG_ERROR,"SPIBus0ClockSelect = %x \n",SbCfg->SPIBus0ClockSelect));

	DEBUG((DEBUG_ERROR,"FsbcMemSize = %x \n",SbCfg->FsbcMemSize));	
	DEBUG((DEBUG_ERROR,"ApicEnable = %x \n",SbCfg->ApicEnable));
	DEBUG((DEBUG_ERROR,"AcpiIrq = %x \n",SbCfg->AcpiIrq));

	DEBUG((DEBUG_ERROR,"PmioBar           = %x \n",SbCfg->PmioBar));
	DEBUG((DEBUG_ERROR,"ApicBar            = %x \n",SbCfg->ApicBar));	
	DEBUG((DEBUG_ERROR,"SbApicID          = %x \n",SbCfg->SbApicID));	
	DEBUG((DEBUG_ERROR,"HpetBar            = %x \n",SbCfg->HpetBar));
	DEBUG((DEBUG_ERROR,"WatchDogBar   = %x \n",SbCfg->WatchDogBar));
	DEBUG((DEBUG_ERROR,"D17F0MmioBar = %x \n",SbCfg->D17F0MmioBar));	
	DEBUG((DEBUG_ERROR,"SpiBar               = %x \n",SbCfg->SpiBar));	

	///
	DEBUG((DEBUG_ERROR,"End of Dump Asia_Sb_Config struct \n ----------\n"));
}

VOID VPei_DumpAsiaDramCfg(
	IN EFI_PEI_SERVICES** PeiServices,
	IN ASIA_DRAM_CONFIGURATION          *DramCfg
)
{
	////
	DEBUG((DEBUG_ERROR,"ASIA_DRAM_CONFIGURATION struct : \n ----------\n"));
	DEBUG((DEBUG_ERROR,"DramClk = %x \n",DramCfg->DramClk));
	DEBUG((DEBUG_ERROR,"RxIoTimingMethod = %x \n",DramCfg->RxIoTimingMethod));
	DEBUG((DEBUG_ERROR,"TxIoTimingMethod = %x \n",DramCfg->TxIoTimingMethod));
	DEBUG((DEBUG_ERROR,"LimitRankSize = %x \n",DramCfg->LimitRankSize));	

	DEBUG((DEBUG_ERROR,"BankIntlv = %x \n",DramCfg->BankIntlv));	
	DEBUG((DEBUG_ERROR,"MemoryChipODTDebug = %x \n",DramCfg->MemoryChipODTDebug));	
	DEBUG((DEBUG_ERROR,"MemoryChipODTWRDebug = %x \n",DramCfg->MemoryChipODTWRDebug));	
	DEBUG((DEBUG_ERROR,"VRIntlv = %x \n",DramCfg->VRIntlv));	

	DEBUG((DEBUG_ERROR,"RDRDY = %x \n",DramCfg->RDRDY));	
	DEBUG((DEBUG_ERROR,"ConversionCircuit = %x \n",DramCfg->ConversionCircuit));
	
	DEBUG((DEBUG_ERROR,"BurstLength = %x \n",DramCfg->BurstLength));	
	DEBUG((DEBUG_ERROR,"CmdRate = %x \n",DramCfg->CmdRate));	
	DEBUG((DEBUG_ERROR,"VGAShareMemory = %x \n",DramCfg->VGAShareMemory));	

	DEBUG((DEBUG_ERROR,"DramInitMethod = %x \n",DramCfg->DramInitMethod));	
	DEBUG((DEBUG_ERROR,"DramSelfRefresh = %x \n",DramCfg->DramSelfRefresh));	
	DEBUG((DEBUG_ERROR,"DynamicCKE = %x \n",DramCfg->DynamicCKE));
	///
	DEBUG((DEBUG_ERROR,"RemapEn = %x \n",DramCfg->RemapEn));	
	DEBUG((DEBUG_ERROR,"PCIMemoryStart = %x \n",DramCfg->PCIMemoryStart));	

	DEBUG((DEBUG_ERROR,"End of Dump Asia_Dram_Config struct \n ----------\n"));	
}

