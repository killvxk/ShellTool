/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SetupVarible.h

Abstract:
  The declaration header file for SYSTEM_CONFIGURATION_GUID.

Revision History:

$END--------------------------------------------------------------------

**/

#ifndef _SETUP_VARIABLE_H
#define _SETUP_VARIABLE_H

#define TSE_SETUP_ENTER_GUID \
  { 0x71202EEE, 0x5F53, 0x40d9, 0xAB, 0x3D, 0x9E, 0x0C, 0x26, 0xD9, 0x66, 0x57 }

#define EFI_IFR_FRONT_PAGE_GUID \
  { 0xe58809f8, 0xfbc1, 0x48e2, { 0x88, 0x3a, 0xa3, 0xf, 0xdc, 0x4b, 0x44, 0x1e } }


#define DEFAULT_BOOT_TIMEOUT                          1
#define PASSWORD_MAX_SIZE                             20
#define PLATFORM_SETUP_VARIABLE_NAME                  L"Setup"
#define SETUP_VOLATILE_VARIABLE_NAME                  L"SetupVolatileData"
#define SETUP_TEMP_VARIABLE_NAME                      L"SetupTemporary"

#define PLATFORM_SETUP_VARIABLE_GUID \
  { \
     0xEC87D643, 0xEBA4, 0x4BB5, 0xA1, 0xE5, 0x3F, 0x3E, 0x36, 0xB2, 0x0D, 0xA9 \
  }

extern EFI_GUID gPlatformSetupVariableGuid;


#define RTC_WAKE_VAL_SINGLE_EVENT    1
#define RTC_WAKE_VAL_DAILY_EVENT     2
#define RTC_WAKE_VAL_WEEKLY_EVENT    3
#define RTC_WAKE_VAL_USER_DEFINED    4
#define RTC_WAKE_VAL_DISABLE         0

#define SMRR_TYPE_DISABLE            0
#define SMRR_TYPE_WT                 1
#define SMRR_TYPE_WB                 2

#define AFTER_POWER_LOSS_OFF         0
#define AFTER_POWER_LOSS_PREVIOUS    1
#define AFTER_POWER_LOSS_ON          2

#define DISPLAY_PRIMARY_PCIE         0
#define DISPLAY_PRIMARY_IGD          1

#pragma pack(1)

typedef struct {
  UINT8  Revision;

  // Main Page
  UINT8  Language;

  UINT8  Numlock;
  UINT8  BootTimeout;
  UINT8  OpromMessage;

  UINT8  UCREnable;
  UINT8  TerminalType;
  UINT8  SerialPortSelect;
  UINT8  SerialBaudrate;

  UINT8  PrimaryDisplay;
  UINT8  UmaFbSize;	
  UINT8  SataEn;

  UINT8  DisplayBootMode;
  UINT8  Csm;
  UINT8  BootModeType;
  
  //
  //Controls whether to display the hotkey 
  //

  
  UINT8  SataMode;  
  UINT8  SbSataChannelMode; 
  UINT8  SbIdeLinuxPatchEn;

  UINT8  UsbMode;
  UINT8  LegacyUsbEnable;
  
  UINT8 IOVEnable;//NBconfig

  UINT8 Pmon;
  UINT8 EpsEnable;
	UINT8 SMRREnable;
#ifdef ZX_SECRET_CODE
  UINT8 KillAp;
  UINT8 FixCpuRatio;
#endif  
  ////
  UINT8  CpuCState;
  UINT8 ConditionalC4;
  UINT8 C5Control;
  UINT8 C5L2sizingControl;
  UINT8 CxIntFilterControl;
  UINT8 CxEEnable;
  UINT8 RoundRobin;
  UINT8 TPREcho;
  UINT8  Msr3A;
  UINT8	PartialResetEn;
  UINT8  TSC_Deadline_mode;
  UINT8  ExecuteDisable;
  ////
  //YKN-20160627 -S
  //UINT8  CpuCoreCount;
  //UINT8  CpuC1E;	
  //UINT8  CpuVT;
  //YKN-20160627 -E
  /// 
  // DRAM
   UINT8 DramClk;
  
   //UINT8 IoTimingMethod;
   UINT8 RxIoTimingMethod;
   UINT8 DQSIByRank;
   UINT8 TxIoTimingMethod; 
   UINT8 LimitRankSize;
   UINT8 DqsiAdjustEn;//DLA_ADD_0406
   UINT8 BankIntlv;
   UINT8 MemoryChipODTDebug;	
   UINT8 MemoryChipODTWRDebug;
   UINT8 MemoryChipODTParkDebug;
   UINT8 VRInlv;
   UINT8 ChannelIntlv;
   UINT8 CHdecode;
   UINT8 RA0Select;
   UINT8 RA1Select;
   UINT8 BASelect;
   //UINT8 BaScmb;
   
   UINT8 CRCParRetryEn;
   UINT8 ParErrControl;
   UINT8 CRCErrControl;
   UINT8 EccRetry;
   UINT8 EccPatrolScrub;
   // UINT8 SwapByte78;//DLA_Rename_0406
   UINT8 SwapChAB; //DLA_Rename_0406
   UINT8 DramECC;
   UINT8 ASPDI;//DLA_ADD_0406
   UINT8 VDD; //DLA_Rename_0406
   UINT8 DataScmb;
   //UINT8 DQMPinOut;
   UINT8 RequestInorder;  
   UINT8 RFW2W;
   UINT8 RFR2R;
   UINT8 RFR2W;
   UINT8 RDFW2R;
   UINT8 BL; //DLA_Rename_0406
   UINT8 CmdRate;
   UINT8 VGAShareMemory;
   UINT8 DramInitMethod;
   UINT8 DramSelfRefresh;
   UINT8 DynamicCKE;
   UINT8 RemapEn;
   UINT8 DramFastBoot;
   UINT8 CRCEn;
   UINT8 CAParEn;
   UINT8 CAParPerEn;
   UINT8 WPREA;
   UINT8 RPREA;
   UINT8 CALEn;
   UINT8 ACTimingOption;
   UINT8 DramCL;
   UINT8 DramTrcd;
   UINT8 DramTrp;
   UINT8 DramTras;
   ////
   ////
   // Graphic
   UINT8 UMAEn;
   UINT8 SelectDisplayDevice;
   UINT8 DP1;
   UINT8 DP2;
   UINT8 DVO; 
   UINT8 CRT;
   UINT8 PanelType;
   UINT8 PanelType2;
   UINT8 ECLKCtrl;
   UINT16 ECLKFreq;
   UINT8 VCLKCtrl;
   UINT16 VCLKFreq;
   UINT8 ICLKCtrl;
   UINT16 ICLKFreq;
   UINT8  DP1SSCEn;
   UINT8  DP2SSCEn;
   UINT8 DP1SSCMode;
   UINT8 DP2SSCMode;

   UINT8 LinuxVideoIP;
   UINT8 PWM0OutputEn;
   UINT8 PWM1OutputEn;
   UINT8 PWM0Frequency;
   UINT8 PWM1Frequency;
   UINT16 PWM0DutyCycle;
   UINT16 PWM1DutyCycle;
   ///
   UINT8 PcieFPGAMode;	
   ///
   //UINT8 SwapByte7_Byte8;
   //UINT8 SwapChannelA_ChannelB;
   //UINT8 ECCClearMemory;
   //UINT8 VDDSel;
   //UINT8 BurstLength;

   ///
  UINT8 ApplicationModeSelection;  
  UINT8 NBSPEValue;
  UINT8 D0F0SPEValue;
  UINT8 D0F1SPEValue;
  UINT8 D0F2SPEValue;
  UINT8 D0F3SPEValue;
  UINT8 D0F4SPEValue;
  UINT8 D0F5SPEValue;
  UINT8 D0F6SPEValue;
  ///
  UINT8 D2F0SPEValue;
  UINT8 D3F0SPEValue;
  UINT8 D3F1SPEValue;	 //PE1
  UINT8 D3F2SPEValue;	 //PE2
  UINT8 D3F3SPEValue;	 //PE3
  ///
  UINT8 D4F0SPEValue;
  UINT8 D4F1SPEValue;	 //PEG1  
  UINT8 D5F0SPEValue;
  UINT8 D5F1SPEValue;	 //PEG3    
  ///
  UINT8 RCRBHSPEValue;
  UINT8 PCIEEPHYSPEValue;
  UINT8 D1F0SPEValue;
  UINT8 MMIOSPEValue;
  UINT8 D8F0SPEValue;
  UINT8 D9F0SPEValue;

  ////
  UINT8 SBSPEValue;
  UINT8 VARTSPEValue;
  UINT8 ESPISPEValue;  
  UINT8 SDIOSPEValue;
  UINT8 BusCtrlSPEValue;
  UINT8 PMUSPEValue;
  UINT8 PCCASPEValue;
	 UINT8 HDACSPEValue;	 
	 UINT8 SPISPEValue; 


	 
	 //NB DEBUG
	  UINT8 CpuClockControl;
	  UINT8 CpuSSCControl;
	  UINT8 PcieClockControl;
	 UINT8 PcieSSCControl;
	 UINT8 SkwClockControl;
	 UINT8 ChipsetCpuBclkControl;
	 UINT8 CPUSlewRate;
	  UINT8 OnBoard1394ClockControl;
   
	  //UINT8 CpuBclkControl;
	  UINT8 SERRNBControl;
	  UINT8 LoopERSMIControl;
	  UINT8 V4IFErrControl;
	  UINT8 DRAMErrControl;
	  UINT8 PEGErrControl;
	  UINT8 PE0ErrControl;
	  UINT8 PE1ErrControl;
	  UINT8 PE2ErrControl;
	  UINT8 PE3ErrControl;
	  UINT8 PEG0ErrControl;
	  UINT8 PEG1ErrControl;
      UINT8 PEG2ErrControl;
      UINT8 PEG3ErrControl;    

	 //UINT8 DID_VID_LOCK;
	 UINT8 VID_SEL;
	
	//UART
	 UINT8 OnChipUartMode;
	 UINT8 UartModuleSelection;
	 UINT8 Uart0Enable;
	 UINT8 Uart0IoBaseSelection;
	 UINT8 Uart0IRQSelection;
	 UINT8 Uart1Enable;
	 UINT8 Uart1IoBaseSelection;
	 UINT8 Uart1IRQSelection;
	 UINT8 Uart2Enable;
	 UINT8 Uart2IoBaseSelection;
	 UINT8 Uart2IRQSelection;
	 UINT8 Uart3Enable;
	 UINT8 Uart3IoBaseSelection;
	 UINT8 Uart3IRQSelection;  
	 


	 //SDIO
	 UINT8 SdioCtrlEn;
	 UINT8   EmmcHs400StrobeMode;
	 UINT8   EmmcCqeEnable;

	 //RAIDA
	 UINT8	RAIDA0En;
	 UINT8	RAIDA1En;
	 UINT8	RAIDASIDLock;
	   
	 //debug signal
	 UINT8 DebugMode;
	 UINT8 DebugOutputSelect;
	 UINT8 OutPadSelect;
	 UINT8 DebugSignalSelect0;
	 UINT8 DebugSignalSelect1;
	 UINT8 DebugModuleSelect0;
	 UINT8 DebugModuleSelect1;
	 UINT8	 NBGroup0TopSelcet;
	 UINT32  NBGroup0ModuleSubSelect;
	 UINT8	 NBGroup1TopSelcet;
	 UINT32  NBGroup1ModuleSubSelect;
   
	 UINT8	 DebugSBSelectByNB;
	 UINT8	 SBTopDbgMux1;	 
	 UINT8	 CCAMux1Sub;
	 UINT8	 PCI1Mux1Sub;
	 UINT8	 SMBMux1Sub;
	 UINT8	 SDHCMux1Sub;
	 UINT8	 ESPIMux1Sub;
	 UINT8	 SPIMux1Sub;
	 UINT8	 PMUMux1Sub;
	 UINT8	 AZMux1Sub;
	 UINT8	 HPETMux1Sub;
	 UINT8	 EBMC_OUT1Mux1Sub;
	 UINT8	 VARTMux1Sub;	 
	 UINT8	 SDHC_TXRXMux1Sub;
	 UINT8	 DMACMux1Sub;
	 UINT8	 PNPMux1Sub;
	 UINT8	 CCAG2Mux1Sub;	 
	 UINT8	 PCI1G2Mux1Sub; 
	 UINT8	 PMUG2Mux1Sub;
	 UINT8	 KBDCMux1Sub;
	 UINT8	 TMRCMux1Sub;
	 UINT8	 PICMux1Sub;
	 UINT8	 LPCMux1Sub;
	 UINT8	BUSC_MISCMux1Sub;
	 UINT8	RTCMux1Sub;
   
	 UINT8	  SBTopDbgMux2;
	 UINT8	  PMUG2Mux2Sub;
	 UINT8	  CCAMux2Sub;
	 UINT8	  PCI1Mux2Sub;
	 UINT8	 SMB_G2Mux2Sub;
	 UINT8	 SDHC_G2Mux2Sub;
	 UINT8	 ESPI_G2Mux2Sub;
	 UINT8	 SPI_G2Mux2Sub;
	 UINT8	 PMUMux2Sub;
	 UINT8	 AZ_G2Mux2Sub;
	 UINT8	 HPETMux2Sub;
	 UINT8	 EBMC_OUT1Mux2Sub;
	 UINT8	 VART_G2Mux2Sub;
	 UINT8	 DMAC_G2Mux2Sub;
	 UINT8	 PNP_G2Mux2Sub; 
	 UINT8	 CCA_G2Mux2Sub;	 
	 UINT8	 PCI1_G2Mux2Sub;
	 UINT8	 KBDC_G2Mux2Sub;
	 UINT8	 TMRC_G2Mux2Sub;
	 UINT8	 PIC_G2Mux2Sub;
	 UINT8	 LPC_G2Mux2Sub;
	 UINT8	BUSC_MISCMux2Sub;
	 UINT8	RTCMux2Sub;
	 UINT8	 TOPGroup0TopSelcet;
	 UINT32	TOPGroup0ModuleSubSelcet;
	 UINT8	 TOPGroup1TopSelcet;
	 UINT32	TOPGroup1ModuleSubSelcet;
 	 UINT8	 FSBCGroup0TopSelcet;
	 UINT8	 FSBCGroup1TopSelcet;  
	 UINT64   CPU_FSBC_DBG_04;
	 UINT64  CPU_FSBC_DBG_05;
	 UINT64  CPU_FSBC_DBG_06;
	 UINT64  CPU_FSBC_DBG_07;
	 UINT64  CPU_FSBC_DBG_08;
	 UINT64   CPU_FSBC_DBG_09;
	 UINT64  CPU_FSBC_DBG_0A;
	 UINT64  CPU_FSBC_DBG_0B;
	 UINT64  CPU_FSBC_DBG_0C;
	 UINT8	 SOCCAP_Mem_Size;
	 UINT8	 CPU_FSBC_PCIE_ON;
	 UINT8	 CPU_FSBC_TOPCIE;
	 UINT8	 CPU_FSBC_DRAM_ON;
	 UINT8	 CPU_FSBC_STREAM_EN;
	 UINT8	 CPU_FSBC_MISSPACKE_EN;
	 UINT8	 CPU_FSBC_TIGPULSE_EN;
	 UINT8	 CPU_FSBC_IFSBCSTP_EN;
   	 UINT8	  SB_HS_DBG_SEL;
	  UINT8	 SB_HS_PCIE_PORT_SEL;	  
	  UINT8   SB_HS_DBG_CH_SEL;  



	 
	 //HDAC
	 UINT8 GoNonSnoopPath;
	 UINT8 HDACFLREn;
	 
	 //SNMI
	 UINT8 IsoLPC;
	 UINT8 IsoVART;
	 UINT8 IsoAZALIA;
	 UINT8 IsoESPI;
	 UINT8 IsoSDIO;
	 UINT8 IsoSPI;
	 UINT8 IsoAPIC;


   //PMU_ACPI
   UINT8 C3BusMasterIdleTimer;
   UINT8 C4BusMasterIdleTimer;
   UINT8 FixedFreeCxLatency;
   UINT8 ShortC3C4Mode;
   UINT8 DPSLPtoSLP;
   UINT8 VRDSLPtoDPSLP;
   UINT8 DynamicT05;
   UINT8 SBDynamicClkControl;
   UINT8 MobileCenterControl;
   UINT8 ACAdapterControl;
   UINT8 MsiSupport;
   
   UINT8 PowerCtrleBMC;
   UINT8 eBMCSettleTime;
   UINT8 KBDCLegacySelControl;
   UINT8 INTCLegacySelControl;
   UINT8 DMACLegacySelControl;
   UINT8 TMRCLegacySelControl;
   
   //OTHERS
	UINT8 EnableMultimediaTimer;
	UINT8 MultimediaTimerMode; 
	UINT8 EnableMultimediaTimerMsi;
	UINT8 WatchDogTimer;
	UINT8 WatchDogTimerRunStop;
	UINT8 WatchDogTimerAction;
	UINT8 WatchDogTimerCount;
	UINT8 KBMouseWakeupControl;
	UINT8 SMBusControllerUnderOS;
	UINT8 SMBusHostClockFrequencySelect;
	UINT8 SMBusHostClockFrequency;
	UINT8 SPIBus0ClockSelect;
    UINT8 ESPI;
	UINT8 TXT;
	   
   //DLA_ADD_0406_E
  
  //Controls whether to display the hotkey 
  //
  UINT8  VideoDualVga;  
  UINT8  VideoPrimaryAdapter;  
  //UINT8 Port4TOPort7;
  //UINT8 Port8TOPort11;
  //UINT8 Port12TOPort13;
 
  //UINT8 AhciFuncEnable;
  //UINT8 SataCapPort0;
  //UINT8 SataHIPMPort0;
  //UINT8 Port0HpcpEn;
  //UINT8 Port0DIPMEn;
  //UINT8 SupportAhciHipmTimer;
  //UINT8 AsynchronousRecovery;
  //UINT8 SrundWaitTime;
  //UINT8 FunctionLevelEnabled;
  //UINT8 SataGen; 
  //UINT8 AHCIGen;
  //UINT8 SATA_MPLLGATE;
  //UINT8 AHCI_MSIX;
  //UINT8 AHCI_SGPIO;
  //RAIDA
  //UINT8 RaidaCtrlEn;

    UINT8  AfterPowerLoss;
    UINT8  WakeOnLan;  
	UINT8  WakeOnRTC;  
	EFI_HII_DATE  RTCWakeupDate;
	EFI_HII_TIME  RTCWakeupTime;
	UINT8  UserDefMon;
	UINT8  UserDefTue;
	UINT8  UserDefWed;
	UINT8  UserDefThu;
	UINT8  UserDefFri;
	UINT8  UserDefSat;
	UINT8  UserDefSun;
	UINT8  AlarmWeekDay;

  UINT8  OnboardLan;
  UINT8  LegacyPxeBoot;
  UINT8  UefiPxeIPV4;
  UINT8  UefiPxeIPV6;

  UINT8  ObAudioEn;
  UINT8  AcpiSleepStatus;

  //MCA
  UINT8 Mca_Msmi_En;
  UINT8 Mca_Csmi_En;

  //DLA_ADD_S
  UINT8 SPECIALLY_SI_SETTING;
  UINT8 PcieRoutingCtrl;
  UINT8 PEG_Msgc2PcieIntx;
  UINT8 PE0_Msgc2PcieIntx;
  UINT8 PE1_Msgc2PcieIntx;
  UINT8 PE2_Msgc2PcieIntx;	  
  UINT8 PE3_Msgc2PcieIntx;
  UINT8 PEG0_Msgc2PcieIntx;
  UINT8 PEG1_Msgc2PcieIntx;
  UINT8 PEG2_Msgc2PcieIntx;
  UINT8 PEG3_Msgc2PcieIntx;
  
  UINT8 PEG_PcieIntx2Nb2sbIntx;
  UINT8 PE0_PcieIntx2Nb2sbIntx;
  UINT8 PE1_PcieIntx2Nb2sbIntx;
  UINT8 PE2_PcieIntx2Nb2sbIntx;
  UINT8 PE3_PcieIntx2Nb2sbIntx;
  UINT8 PEG0_PcieIntx2Nb2sbIntx;
  UINT8 PEG1_PcieIntx2Nb2sbIntx;
  UINT8 PEG2_PcieIntx2Nb2sbIntx;
  UINT8 PEG3_PcieIntx2Nb2sbIntx;
  
  UINT8 MsiEnable;
  UINT8 PegApciIrq;
  UINT8 Peg2ApciIrq;
  UINT8 Peg3ApciIrq;  
  //DLA_ADD_E

  ////  Synch here. Tiger 2016-06-17 	  
  //reset link when fail
  UINT8 PcieRst;
  UINT8 PcieRstPEG;
  UINT8 PcieRstPE0;
  UINT8 PcieRstPE1;
  UINT8 PcieRstPE2;
  UINT8 PcieRstPE3;
  UINT8 PcieRstPEG0;
  UINT8 PcieRstPEG1;
  UINT8 PcieRstPEG2;
  UINT8 PcieRstPEG3;
  //rp 
  UINT8 PcieRP;
  UINT8 PciePEG;
  UINT8 PciePE0;
  UINT8 PciePE1;
  UINT8 PciePE2;
  UINT8 PciePE3;
  UINT8 PciePEG0;
  UINT8 PciePEG1;
  UINT8 PciePEG2;
  UINT8 PciePEG3;
  //
  UINT8 PcieHotReset;
  //Fmw related
  UINT8 PEMCU_LoadFW_WhenBoot;
  UINT8 PEMCU_RstSys_WhenFail;
  //Link Speed
  UINT8 PcieLinkSpeed;
  UINT8 PciePEGLinkSpeed;
  UINT8 PciePE0LinkSpeed;
  UINT8 PciePE1LinkSpeed;
  UINT8 PciePE2LinkSpeed;
  UINT8 PciePE3LinkSpeed;
  UINT8 PciePEG0LinkSpeed;
  UINT8 PciePEG1LinkSpeed;
  UINT8 PciePEG2LinkSpeed;
  UINT8 PciePEG3LinkSpeed;
  //OBFF 
  UINT8 PcieOBFFCtrl_PCIE;
  //EQ related
  UINT8 PcieEQ;
		  UINT8 PciePreDSTxX8BL0;
		  UINT8 PciePreDSRxX8BL0;
		  UINT8 PciePreUSTxX8BL0;
		  UINT8 PciePreUSRxX8BL0;
		  UINT8 PciePreDSTxX8BL1;
		  UINT8 PciePreDSRxX8BL1;
		  UINT8 PciePreUSTxX8BL1;
		  UINT8 PciePreUSRxX8BL1;
		  UINT8 PciePreDSTxX8BL2; 
		  UINT8 PciePreDSRxX8BL2;
		  UINT8 PciePreUSTxX8BL2;
		  UINT8 PciePreUSRxX8BL2;
		  UINT8 PciePreDSTxX8BL3;
		  UINT8 PciePreDSRxX8BL3;
		  UINT8 PciePreUSTxX8BL3;
		  UINT8 PciePreUSRxX8BL3; 
		  UINT8 PciePreDSTxX8BL4;
		  UINT8 PciePreDSRxX8BL4;
		  UINT8 PciePreUSTxX8BL4;
		  UINT8 PciePreUSRxX8BL4; // 20
		  UINT8 PciePreDSTxX8BL5;
		  UINT8 PciePreDSRxX8BL5;
		  UINT8 PciePreUSTxX8BL5;
		  UINT8 PciePreUSRxX8BL5;
		  UINT8 PciePreDSTxX8BL6;
		  UINT8 PciePreDSRxX8BL6;
		  UINT8 PciePreUSTxX8BL6;
		  UINT8 PciePreUSRxX8BL6; 
		  UINT8 PciePreDSTxX8BL7;
		  UINT8 PciePreDSRxX8BL7; // 30
		  UINT8 PciePreUSTxX8BL7;
		  UINT8 PciePreUSRxX8BL7;
		  UINT8 PciePreDSTxX8CL0;
		  UINT8 PciePreDSRxX8CL0;
		  UINT8 PciePreUSTxX8CL0;
		  UINT8 PciePreUSRxX8CL0;
		  UINT8 PciePreDSTxX8CL1;
		  UINT8 PciePreDSRxX8CL1;
		  UINT8 PciePreUSTxX8CL1;
		  UINT8 PciePreUSRxX8CL1; // 40
		  UINT8 PciePreDSTxX8CL2;
		  UINT8 PciePreDSRxX8CL2;
		  UINT8 PciePreUSTxX8CL2;
		  UINT8 PciePreUSRxX8CL2;
		  UINT8 PciePreDSTxX8CL3;
		  UINT8 PciePreDSRxX8CL3;
		  UINT8 PciePreUSTxX8CL3;
		  UINT8 PciePreUSRxX8CL3;
		  UINT8 PciePreDSTxX8CL4;
		  UINT8 PciePreDSRxX8CL4; // 50
		  UINT8 PciePreUSTxX8CL4;
		  UINT8 PciePreUSRxX8CL4;
		  UINT8 PciePreDSTxX8CL5;
		  UINT8 PciePreDSRxX8CL5;
		  UINT8 PciePreUSTxX8CL5;
		  UINT8 PciePreUSRxX8CL5;
		  UINT8 PciePreDSTxX8CL6;
		  UINT8 PciePreDSRxX8CL6;
		  UINT8 PciePreUSTxX8CL6;
		  UINT8 PciePreUSRxX8CL6; // 60
		  UINT8 PciePreDSTxX8CL7;
		  UINT8 PciePreDSRxX8CL7;
		  UINT8 PciePreUSTxX8CL7;
		  UINT8 PciePreUSRxX8CL7;
		  UINT8 PciePreDSTxX8AL0; 
		  UINT8 PciePreDSRxX8AL0;
		  UINT8 PciePreUSTxX8AL0;
		  UINT8 PciePreUSRxX8AL0;
		  UINT8 PciePreDSTxX8AL1;
		  UINT8 PciePreDSRxX8AL1; // 70
		  UINT8 PciePreUSTxX8AL1;
		  UINT8 PciePreUSRxX8AL1;
		  UINT8 PciePreDSTxX8AL2;
		  UINT8 PciePreDSRxX8AL2;
		  UINT8 PciePreUSTxX8AL2;
		  UINT8 PciePreUSRxX8AL2;
		  UINT8 PciePreDSTxX8AL3;
		  UINT8 PciePreDSRxX8AL3;
		  UINT8 PciePreUSTxX8AL3;
		  UINT8 PciePreUSRxX8AL3; // 80
		  UINT8 PciePreDSTxX8AL4;
		  UINT8 PciePreDSRxX8AL4;
		  UINT8 PciePreUSTxX8AL4;
		  UINT8 PciePreUSRxX8AL4;
		  UINT8 PciePreDSTxX8AL5;
		  UINT8 PciePreDSRxX8AL5;
		  UINT8 PciePreUSTxX8AL5;
		  UINT8 PciePreUSRxX8AL5;
		  UINT8 PciePreDSTxX8AL6;
		  UINT8 PciePreDSRxX8AL6; // 90
		  UINT8 PciePreUSTxX8AL6;
		  UINT8 PciePreUSRxX8AL6;
		  UINT8 PciePreDSTxX8AL7;
		  UINT8 PciePreDSRxX8AL7;
		  UINT8 PciePreUSTxX8AL7;
		  UINT8 PciePreUSRxX8AL7;

  //Else
  UINT8 MaxPayloadSize;
  UINT8 PcieASPM;
  UINT8 RelaxedOrder;
  UINT8 ExtTag;
  UINT8 ExtSync;
  UINT8 NoSnoop;
  
  //pcie
  UINT8 Pci64;           // winddy +
	
//NB DEBUG
//LGE20160308-START
	 UINT8 HIFErrControl;
	///	
}SETUP_DATA;

typedef struct {
  UINT8  PlatId;
  UINT8  TpmFormsetPresent;
  UINT8  TcmFormsetPresent;
  UINT8  TcFormsetPresent;  
  UINT8  Tpm2FormsetPresent;
  UINT8  HdpFormsetPresent;
  UINT8  SecureBootFormsetPresent;	
} SETUP_VOLATILE_DATA;

#pragma pack()


#endif  // #ifndef _SETUP_VARIABLE_H

