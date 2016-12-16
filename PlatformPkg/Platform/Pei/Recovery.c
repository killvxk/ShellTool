/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

Module Name:

  Recovery.c

Abstract:

  PEIM to provide the platform recovery functionality.

--*/


#include "PlatformPei.h"
#include "CHX001Reg.h"
#include <Ppi/AtaController.h>
#include <Ppi/BootInRecoveryMode.h>
#include <IndustryStandard/Pci.h>
#include <Library/TimerLib.h>
#include <SetupVariable.h>

EFI_STATUS
EFIAPI
BuildDefaultDataHobForRecoveryVariable (
  IN EFI_PEI_SERVICES  **PeiServices
  );


#define SB_SATA_PRIMARY_CMD_BAR      0x1F0
#define SB_SATA_PRIMARY_CTL_BAR      0x3F6
#define SB_SATA_SECONDARY_CMD_BAR    0x170
#define SB_SATA_SECONDARY_CTL_BAR    0x376

//
// Required Service
//
EFI_STATUS
EnableAtaChannel (
  IN EFI_PEI_SERVICES       **PeiServices,
  IN PEI_ATA_CONTROLLER_PPI *This,
  IN UINT8                  ChannelIndex
  );

UINT32
GetIdeRegsBaseAddr (
  IN  EFI_PEI_SERVICES       **PeiServices,
  IN  PEI_ATA_CONTROLLER_PPI *This,
  OUT IDE_REGS_BASE_ADDR     *IdeRegsBaseAddr
  );

//
// Module globals
//
static PEI_ATA_CONTROLLER_PPI mAtaControllerPpi = {
  EnableAtaChannel,
  GetIdeRegsBaseAddr
};

static EFI_PEI_PPI_DESCRIPTOR mRecoveryPpiList[] = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiBootInRecoveryModePpiGuid,
  NULL
};
  
static EFI_PEI_PPI_DESCRIPTOR mAtaControllerPpiList[] = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiAtaControllerPpiGuid,
  &mAtaControllerPpi
};

//
// IDE registers' fixed address
//
static IDE_REGS_BASE_ADDR   mAtaChannelIoSpace[2] = {
  {SB_SATA_PRIMARY_CMD_BAR,   SB_SATA_PRIMARY_CTL_BAR},
  {SB_SATA_SECONDARY_CMD_BAR, SB_SATA_SECONDARY_CTL_BAR},    
};

EFI_STATUS
EnableAtaChannel (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_ATA_CONTROLLER_PPI         *This,
  IN UINT8                          ChannelMask
  )
{
  /// TGR-2016062201. SOC not include SATA Controller
#if 0
  DEBUG((EFI_D_INFO, __FUNCTION__"(0x%X)\n", ChannelMask));

  WaitIdeDeviceReady(0, 15, 0, 3000);

  DEBUG((EFI_D_INFO, "SATA0[4]  = %02X\n", MmioRead8(SATA_PCI_REG(4))));
  DEBUG((EFI_D_INFO, "SATA0[8]  = %08X\n", MmioRead32(SATA_PCI_REG(8))));
  DEBUG((EFI_D_INFO, "SATA0[A0] = %08X\n", MmioRead32(SATA_PCI_REG(0xA0))));
#else
#endif

  return EFI_SUCCESS;
}

UINT32
GetIdeRegsBaseAddr (
  IN  EFI_PEI_SERVICES       **PeiServices,
  IN  PEI_ATA_CONTROLLER_PPI *This,
  OUT IDE_REGS_BASE_ADDR     *IdeRegsBaseAddr
  )
{
  ((*PeiServices)->CopyMem) (IdeRegsBaseAddr, mAtaChannelIoSpace, sizeof (mAtaChannelIoSpace));
  return sizeof (mAtaChannelIoSpace) / sizeof (IDE_REGS_BASE_ADDR);
}

EFI_STATUS
EFIAPI
PeimInitializeRecovery (
  IN EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                       Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi; 
  SETUP_DATA                       SetupData;
  UINTN                            Size;

  Status = (*PeiServices)->InstallPpi (PeiServices, &mRecoveryPpiList[0]);
  ASSERT_EFI_ERROR (Status);
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID**)&Var2Ppi
             );
  ASSERT_EFI_ERROR(Status);
  Size   = sizeof(SETUP_DATA);  
  Status = Var2Ppi->GetVariable (
                      Var2Ppi,
                      PLATFORM_SETUP_VARIABLE_NAME,
                      &gPlatformSetupVariableGuid,
                      NULL,
                      &Size,
                      &SetupData
                      );
  if (SetupData.SataEn) {
    Status = (*PeiServices)->InstallPpi (PeiServices, &mAtaControllerPpiList[0]);
    ASSERT_EFI_ERROR (Status);
  }
  
  MmioAnd8 (ERRRPT_PCI_REG(0x62), (UINT8)(~BIT2));

  BuildDefaultDataHobForRecoveryVariable(PeiServices);

  return Status;
}

