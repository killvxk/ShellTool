/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SmmPlatform.c

Abstract:
  Source file for the Smm platform driver.

Revision History:

**/

#include "SmmPlatform.h"
#include <PlatS3Record.h>


SMMCALL_ENTRY             *mSmmCallTablePtr = NULL;
UINT16                    mAcpiBaseAddr;
SETUP_DATA                gSetupData;
EFI_SMM_VARIABLE_PROTOCOL *mSmmVariable;
EFI_ACPI_RAM_DATA         *gAcpiRam;
PLATFORM_S3_RECORD        *gS3Record;


VOID ClearDateAlarm();
VOID SleepSmiDebug(UINT8 SleepType);

/// has checked for CHX001 by Tiger. 2016-06-23
EFI_STATUS
EnableAcpiCallback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE, 
    (EFI_SOFTWARE_SMM_DRIVER | BYO_ACPI_ENABLE)
    );

  //if(MmioRead16(EHCI_PCI_REG(PCI_VID_REG)) != 0xFFFF){
  //  MmioOr32(EHCI_PCI_REG(SB_EHCI_LEGEXT_CAP_REG), SB_EHCI_LEGEXT_CAP_HCOS);
  //}
  
  IoWrite16(0x80, 0);

  ClearDateAlarm(); 
  
  // Disable PM sources except power button
  IoWrite16(mAcpiBaseAddr + PMIO_PM_EN, PMIO_PM_EN_PWRBTN); // Rx02[8]
  IoWrite16(mAcpiBaseAddr + PMIO_STS_REG, 0xFFFF);        // Rx00
  IoWrite16(mAcpiBaseAddr + PMIO_GPI_SCIEN_REG, 0);      // Rx56
  IoWrite16(mAcpiBaseAddr + PMIO_GPI_SCIEN1_REG, 0);    // Rx58
  IoWrite16(mAcpiBaseAddr + PMIO_GPI_SCIEN3_REG, 0);  //Rx5A

  IoWrite16(mAcpiBaseAddr + PMIO_GPI_STS_REG, 0xFFFF); // Rx50
  IoWrite16(mAcpiBaseAddr + PMIO_GPI_STS_REG1, 0xFFFF);//Rx52
  IoWrite16(mAcpiBaseAddr + PMIO_GPI_STS_REG3, 0xFFFF);// Rx54

  // Disable GP2 & GP3 SMI, Disable GP2 & GP3 timer tick
  IoAnd16(mAcpiBaseAddr + PMIO_GBLEN_REG, (UINT16)~(PMIO_GBLEN_GP2SMI | PMIO_GBLEN_GP3SMI)); //Rx2A
  IoWrite16(mAcpiBaseAddr + PMIO_GBLSTS_REG, PMIO_GBLSTS_GP2SMI|PMIO_GBLSTS_GP3SMI); //Rx28
  MmioWrite8(LPC_PCI_REG(LPC_GP23TIMER_CTRL_REG), LPC_GP23TIMER_CTRL_REG_DEF_VALUE);

  // Enable SCI
  IoOr16(mAcpiBaseAddr + PMIO_PM1_CNT_REG, PMIO_PM1_CNT_SCI_EN);  

  // USB hand off
  if (mSmmCallTablePtr) {
    mSmmCallTablePtr[__ID_EndLegacyUsb]();
  }

  return EFI_SUCCESS;
}


EFI_STATUS
DisableAcpiCallback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  DEBUG((EFI_D_ERROR, "AcpiOff\n"));
  IoAnd16(mAcpiBaseAddr + PMIO_PM1_CNT_REG, (UINT16)~PMIO_PM1_CNT_SCI_EN);   // Disable SCI
  return EFI_SUCCESS;
}


EFI_STATUS
S3PeiEndCallback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  if(gS3Record->S3Sleep == PLAT_S3_SLEEP_SLEEP){
    REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PcdGet32(PcdProgressCodeS3SuspendEnd));
    gS3Record->S3Sleep = PLAT_S3_SLEEP_RESUME;
  }  
  return EFI_SUCCESS;
}




void WaitxHCIPDU2U3 (void)
{
	//UINT32 Temp32;
	//Temp32 = MmioRead32(XHCI_PCI_REG(XHCI_FWSWMSG2_REG));
	//if (Temp32 == 0x0) MicroSecondDelay(150000);
}




// SleepType: 3, 4, 5, 0xFF(SoftOff)
/// (has checked for CHX001 by Tiger. 2016-06-23)
VOID SleepCommonHandler(UINT8 SleepType)
{
  BOOLEAN  BiosUpdate = FALSE;

  if(SleepType == 3 || SleepType == 4 || SleepType == 5){
    WaitxHCIPDU2U3(); // empty function for CHX001
    IoAnd8(mAcpiBaseAddr + PMIO_SUS_PWR_DOMAIN_REG, (UINT8)~BIT7); //PMIO_Rx86
  }  
  
  SetAfterPowerLoss();
  TurnOffKbLed();

  if(SleepType == 5){
    IoWrite16(mAcpiBaseAddr + PMIO_GP_STS, PMIO_GP_STS_PME);
    BiosUpdate = IsBiosWantUpdate();
  }

  if(!BiosUpdate && (SleepType == 5 || SleepType == 0xFF)){
    EnableS5RtcWake();  // RTC/CMOS operation.
  }
  
//SetWakeOnLan(SleepType);

// Capsule RTC
  if(BiosUpdate){
    SetSleepTypeS3();    
    SetRtcWakeUpForCapsule(5);
  }  

}



/// has checked for CHX001 Project. 2016-06-23
EFI_STATUS
S3SleepEntryCallBack (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, PcdGet32(PcdProgressCodeS3SuspendStart));
  gS3Record->S3Sleep = PLAT_S3_SLEEP_SLEEP;

  SleepCommonHandler(3);

/*
// Add GFX Power gating feature settings.
  if((UINT16)MmioRead32(IGD_PCI_REG(PCI_VID_REG)) != 0xFFFF){
    DEBUG((EFI_D_INFO, "IGD PG\n"));
    // Disable BMU/VPP/S0EUX/S0TUF before go to STR
    // Disable S0EUX
    IoAnd8(mAcpiBaseAddr + PMIO_GFX_SOEUX_POWER_GATING_REG, (UINT8)~BIT4);
    // Disable  S0TUF
    IoAnd8(mAcpiBaseAddr + PMIO_GFX_SOTUF_POWER_GATING_REG, (UINT8)~BIT4);
    // Keep BMU always being powered on after R25 for S3/S4 burn-in hang issue
    // Disable  BMU
    //IoWrite8(PmioBar|GFX_BMU_POWER_GATING, (IoRead8(PmioBar|GFX_BMU_POWER_GATING)&0xF7));
    // Disable  VPP
    IoAnd8(mAcpiBaseAddr + PMIO_GFX_VPP_POWER_GATING_REG, (UINT8)~BIT4);
  } else {
      // Enable DIU before go to STR
      // Enable  DIU
      // Always keep DIU enabled after R23. Therefore, it is not necessary to enable again here.
      // IoWrite8(PmioBar|GFX_DIU_POWER_GATING, (IoRead8(PmioBar|GFX_DIU_POWER_GATING)|POWER_CONTROL_FOR_DIU));
  }
*/

// D0F4Rx71[7] = 0, Disable MCKE dynamic assertion
  MmioAnd32(NPMC_PCI_REG(DRAM_DYNCLK_CTRL_REG), (UINT32)~BIT15);  // Dynamic CKE When DRAM Is Idle (disable)

// patch F0000 readable when S3 resume 
// (0,0,3,83)[5:4] - F0000-FFFFFh Memory Space Access Control 
  MmioAnd32(HIF_PCI_REG(PAGE_C_SHADOW_CTRL_REG), (UINT32)~((BIT4|BIT5)<<24));


// Set D0F3 Rx63[0]=1 to fix S3 resume fail issue. (CHX001 has deprecated this bit!!)
  //if(MmioRead8(NPMC_PCI_REG(D0F4_INTERNAL_REV_ID)) == 0x00){
  //  MmioOr8(DRAM_PCI_REG(IBV_D0F3_RESERVED_Z4), D0F3_REF_STP); // 
  //}

#ifndef MDEPKG_NDEBUG	
  SleepSmiDebug(3);
#endif  
    
  return EFI_SUCCESS;
}

/// has checked for CHX001 Project. 2016-06-23
EFI_STATUS
S4SleepEntryCallBack (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  SleepCommonHandler(4);
  return EFI_SUCCESS;
}

/// has checked for CHX001 Project. 2016-06-23
EFI_STATUS
S5SleepEntryCallBack (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  SleepCommonHandler(5);
  return EFI_SUCCESS;
}

/// has checked for CHX001 Project. 2016-06-23
EFI_STATUS
PowerButtonCallback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  UINT16  Buffer;
  //UINT16  BaseAddress;
  //UINT16  Index;

  SleepCommonHandler(0xFF);

// Clear Sleep Type Enable
  IoAnd16(mAcpiBaseAddr + PMIO_GBLEN_REG,  (UINT16)~PMIO_GBLEN_SLPSMI); // PMIO_Rx2A

// Clear Power Button Status
  IoWrite16(mAcpiBaseAddr + PMIO_STS_REG, PMIO_STS_PWRBTN); // PMIO_Rx00

// Stop all UHCI controllers before trying to shut down
  //for (Index = 0; Index < 3; Index++) {
  //  BaseAddress = MmioRead16(UHCI_PCI_REG(Index, 0x20)) & ~BIT0;
  //  if(BaseAddress == 0xFFFE){continue;}
  //  IoWrite8(BaseAddress, 0);
  //}

// Shut it off now
  Buffer = IoRead16 (mAcpiBaseAddr + PMIO_PM1_CNT_REG) & (~(PMIO_PM1_CNT_SLP_EN | PMIO_PM1_CNT_SLP_TYP)); // PMIO_Rx04
  Buffer |= PMIO_PM1_CNT_S5;
  IoWrite16 (mAcpiBaseAddr + PMIO_PM1_CNT_REG, Buffer);
  Buffer |= PMIO_PM1_CNT_SLP_EN;
  IoWrite16 (mAcpiBaseAddr + PMIO_PM1_CNT_REG, Buffer);

  return EFI_SUCCESS;
}

EFI_STATUS
LegacyUsbCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS                    Status;
  EFI_LEGACY_USB_INF_PROTOCOL   *LegacyUsbInf;

  Status = gSmst->SmmLocateProtocol (
                  &gEfiLegacyUsbInfProtocolGuid,
                  NULL,
                  &LegacyUsbInf
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mSmmCallTablePtr = LegacyUsbInf->SmmCallTablePtr;
  return EFI_SUCCESS;
}

EFI_STATUS
InitializePlatformSmm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
  EFI_STATUS                                Status;
  EFI_HANDLE                                PowerButtonHandle = NULL;
  EFI_HANDLE                                SwHandle = NULL;
  EFI_HANDLE                                S4SleepEntryHandle;  
  EFI_HANDLE                                S5SleepEntryHandle;
  EFI_HANDLE                                S3SleepEntryHandle;
  EFI_SMM_POWER_BUTTON_DISPATCH2_PROTOCOL   *PowerButtonDispatch;
  EFI_SMM_SW_DISPATCH2_PROTOCOL             *SwDispatch;
  EFI_SMM_POWER_BUTTON_REGISTER_CONTEXT     PowerButtonContext;
  EFI_SMM_SW_REGISTER_CONTEXT               SwContext;
  EFI_SMM_SX_DISPATCH2_PROTOCOL             *SxDispatch;
  EFI_SMM_SX_REGISTER_CONTEXT               EntryDispatchContext;
  UINTN                                     VariableSize;
  SETUP_DATA	                              *SetupHob;
  VOID                                      *Registration;


  gAcpiRam = (EFI_ACPI_RAM_DATA*)GetAcpiRam();
  gS3Record = (PLATFORM_S3_RECORD*)GetS3RecordTable();
  
  Status = gSmst->SmmLocateProtocol(&gEfiSmmVariableProtocolGuid, NULL, (VOID**)&mSmmVariable);
  if(EFI_ERROR(Status)){
    mSmmVariable = NULL;		
    SetupHob = (SETUP_DATA*)GetSetupDataHobData();
    CopyMem(&gSetupData, SetupHob, sizeof(SETUP_DATA));		
  } else {
    VariableSize = sizeof(SETUP_DATA);  
    Status = mSmmVariable->SmmGetVariable (
                             PLATFORM_SETUP_VARIABLE_NAME,
                             &gPlatformSetupVariableGuid,
                             NULL,
                             &VariableSize,
                             &gSetupData
                             );  
    ASSERT_EFI_ERROR(Status);
  }

  mAcpiBaseAddr = PcdGet16(AcpiIoPortBaseAddress);

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmPowerButtonDispatch2ProtocolGuid,
                    NULL,
                    &PowerButtonDispatch
                    );
  ASSERT_EFI_ERROR (Status);

  PowerButtonContext.Phase = PowerButtonEntry;
  Status = PowerButtonDispatch->Register (
                                  PowerButtonDispatch,
                                  PowerButtonCallback,
                                  &PowerButtonContext,
                                  &PowerButtonHandle
                                  );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol(&gEfiSmmSwDispatch2ProtocolGuid, NULL, &SwDispatch);
  ASSERT_EFI_ERROR (Status);

  SwContext.SwSmiInputValue = EFI_ACPI_ENABLE_SW_SMI; // 0xF0
  Status = SwDispatch->Register (
                         SwDispatch,
                         EnableAcpiCallback,
                         &SwContext,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);

  SwContext.SwSmiInputValue = EFI_ACPI_DISABLE_SW_SMI; // 0xF1
  Status = SwDispatch->Register (
                         SwDispatch,
                         DisableAcpiCallback,
                         &SwContext,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);

  SwContext.SwSmiInputValue = EFI_ACPI_S3_PEI_END_SW_SMI; // 0xF2
  Status = SwDispatch->Register (
                         SwDispatch,
                         S3PeiEndCallback,
                         &SwContext,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);
  

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSxDispatch2ProtocolGuid,
                    NULL,
                    &SxDispatch
                    );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  EntryDispatchContext.Type  = SxS3;
  EntryDispatchContext.Phase = SxEntry;
  Status = SxDispatch->Register (
                         SxDispatch,
                         S3SleepEntryCallBack,
                         &EntryDispatchContext,
                         &S3SleepEntryHandle
                         );

  EntryDispatchContext.Type  = SxS4;
  EntryDispatchContext.Phase = SxEntry;
  Status = SxDispatch->Register (
                         SxDispatch,
                         S4SleepEntryCallBack,
                         &EntryDispatchContext,
                         &S4SleepEntryHandle
                         );

  EntryDispatchContext.Type  = SxS5;
  EntryDispatchContext.Phase = SxEntry;
  Status = SxDispatch->Register (
                         SxDispatch,
                         S5SleepEntryCallBack,
                         &EntryDispatchContext,
                         &S5SleepEntryHandle
                         );

  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiLegacyUsbInfProtocolGuid,
                    LegacyUsbCallback,
                    &Registration
                    );
  LegacyUsbCallback (NULL, NULL, NULL);

  return EFI_SUCCESS;
}



