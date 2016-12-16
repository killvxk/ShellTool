/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SmmPlatform.h

Abstract:
  Header file for the Smm platform driver.

Revision History:

**/


#ifndef _SMM_PLATFORM_H_
#define _SMM_PLATFORM_H_


#include <PiSmm.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/SocketLga775Lib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/TimerLib.h>
#include <Library/HobLib.h>
#include <Library/UefiLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmPowerButtonDispatch2.h>
#include <Protocol/SmmSxDispatch2.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmCpuSaveState.h>
#include <Protocol/SmmPowerButtonDispatch.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/LegacyUsbInf.h>
#include <PlatformDefinition.h>
#include <SetupVariable.h>
#include <AcpiRam.h>
#include <Library/PlatformCommLib.h>



BOOLEAN IsBiosWantUpdate();
EFI_STATUS SetRtcWakeUpForCapsule(UINT8 SleepTimeSecond);
VOID SetSleepTypeS3();
EFI_STATUS EnableS5RtcWake();
//VOID SetWakeOnLan(UINT8 SleepType);
VOID SetAfterPowerLoss();

VOID WaitPcieL2L3Ready();
VOID EUMAPowerDown();   

extern UINT16                     mAcpiBaseAddr;
extern SETUP_DATA                 gSetupData;
extern EFI_SMM_VARIABLE_PROTOCOL  *mSmmVariable;
extern EFI_ACPI_RAM_DATA          *gAcpiRam;

#endif

EFI_STATUS
TurnOffKbLed();