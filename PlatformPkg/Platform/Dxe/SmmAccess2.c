
#include "PlatformDxe.h"
#include <Protocol/SmmAccess2.h>


STATIC
EFI_STATUS
EFIAPI
SmmAccess2Open (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

STATIC
EFI_STATUS
EFIAPI
SmmAccess2Close (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

STATIC  
EFI_STATUS
EFIAPI
SmmAccess2Lock (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

STATIC
EFI_STATUS
EFIAPI
SmmAccess2GetCapabilities (
  IN CONST EFI_SMM_ACCESS2_PROTOCOL  *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  );

STATIC EFI_SMRAM_DESCRIPTOR  gSmramDesc[SMRAM_DESC_COUNT];
STATIC BOOLEAN               gSmramCloseLock;

STATIC EFI_SMM_ACCESS2_PROTOCOL gSmmAccess2 = {
  SmmAccess2Open,
  SmmAccess2Close,
  SmmAccess2Lock,
  SmmAccess2GetCapabilities,
  FALSE,
  FALSE
};


STATIC
EFI_STATUS
EFIAPI
SmmAccess2Open (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  EFI_SMRAM_DESCRIPTOR   *SmramDesc;
  UINTN                  Index;


  if(gSmramCloseLock){
    return EFI_ACCESS_DENIED;
  }  
  
  SmramDesc = gSmramDesc;

  for (Index = 0; Index < SMRAM_DESC_COUNT; Index++){
    if (SmramDesc[Index].RegionState & EFI_SMRAM_LOCKED){	    
      DEBUG ((EFI_D_INFO, "Cannot open a locked SMRAM region\n"));
      continue;
    }
    SmramDesc[Index].RegionState &= (~(UINT64)(EFI_SMRAM_CLOSED |EFI_ALLOCATED));
    SmramDesc[Index].RegionState |= EFI_SMRAM_OPEN;  	
  }	  	
  
  gSmmAccess2.OpenState = TRUE;
  
  return EFI_SUCCESS;
}  


STATIC
EFI_STATUS
EFIAPI
SmmAccess2Close (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  EFI_SMRAM_DESCRIPTOR   *SmramDesc;
  UINTN                  Index;

  SmramDesc = gSmramDesc;
  
  for (Index = 0; Index < SMRAM_DESC_COUNT; Index++){

    if (SmramDesc[Index].RegionState & EFI_SMRAM_LOCKED){	    
      DEBUG ((EFI_D_INFO, "Cannot close a locked SMRAM region\n"));
      continue;
    }
    if (SmramDesc[Index].RegionState & EFI_SMRAM_CLOSED){
      continue;
    }

    SmramDesc[Index].RegionState &= (~(UINT64)EFI_SMRAM_OPEN);
    SmramDesc[Index].RegionState |= (UINT64)(EFI_SMRAM_CLOSED |EFI_ALLOCATED);
  }	

  if(IsSmrrTypeSetWB()) {
      MmioAnd8(HIF_PCI_REG(SVAD_VGA_DECODE_REG), (UINT8)~SVAD_ABSEG_SEL);
      MmioOr8(HIF_PCI_REG(SVAD_VGA_DECODE_REG), TSEG_C2M_PROTECT_DISABLE);    
  }

  MmioOr8(HIF_PCI_REG(SMM_APIC_DECODE_REG), TSMM_EN);    

  gSmmAccess2.OpenState = FALSE;
  
  return EFI_SUCCESS;
}


STATIC  
EFI_STATUS
EFIAPI
SmmAccess2Lock (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  EFI_SMRAM_DESCRIPTOR   *SmramDesc;
  UINTN                  Index;

  SmramDesc = gSmramDesc;

  for (Index = 0; Index < SMRAM_DESC_COUNT; Index++){
    if (gSmmAccess2.OpenState){
      DEBUG ((EFI_D_ERROR, "Cannot lock SMRAM when SMRAM regions are still open\n"));
      return EFI_DEVICE_ERROR;
    }

    SmramDesc[Index].RegionState |= EFI_SMRAM_LOCKED; 	
  }	

   
  gSmmAccess2.LockState = TRUE;  
  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
EFIAPI
SmmAccess2GetCapabilities (
  IN CONST EFI_SMM_ACCESS2_PROTOCOL  *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  )
{
  UINTN       DescSize;
  EFI_STATUS  Status;
  
  DescSize = sizeof(gSmramDesc);
  if (*SmramMapSize < DescSize) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    CopyMem (SmramMap, gSmramDesc, DescSize);
    Status = EFI_SUCCESS;
  }

  *SmramMapSize = DescSize;
  return Status;
}

  






EFI_STATUS
SmmAccess2Install (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  PLATFORM_MEMORY_INFO  *MemInfo;
    
  ASSERT(SMRAM_DESC_COUNT >= 1);

  ZeroMem (gSmramDesc, sizeof(gSmramDesc));
  
  MemInfo = (PLATFORM_MEMORY_INFO*)GetPlatformMemInfo();
  gSmramDesc[0].PhysicalStart = MemInfo->TSegAddr;
  gSmramDesc[0].CpuStart      = MemInfo->TSegAddr;
  gSmramDesc[0].PhysicalSize  = MemInfo->TSegSize;

  gSmramCloseLock = (MmioRead8(HIF_PCI_REG(SMM_APIC_DECODE_REG)) & TSMM_EN)?TRUE:FALSE;  
  if(!gSmramCloseLock){
    gSmmAccess2.LockState = FALSE;
    gSmmAccess2.OpenState = TRUE;
    gSmramDesc[0].RegionState = EFI_SMRAM_OPEN;
  } else {
    gSmmAccess2.LockState = TRUE;
    gSmmAccess2.OpenState = FALSE; 
    gSmramDesc[0].RegionState = EFI_SMRAM_CLOSED | EFI_SMRAM_LOCKED;
  }
  
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiSmmAccess2ProtocolGuid,
                  &gSmmAccess2,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  return Status;
}



