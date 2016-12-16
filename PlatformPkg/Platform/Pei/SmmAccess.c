
#include "PlatformPei.h"
#include <Guid/AcpiS3Context.h>
#include <Ppi/SmmAccess.h>


#define SMM_ACCESS_PRIVATE_DATA_SIGNATURE   SIGNATURE_32('4','5','s','a')

typedef struct {
  UINT32                          Signature;
  BOOLEAN                         CloseLock;
  PEI_SMM_ACCESS_PPI              SmmAccess;
  EFI_PEI_PPI_DESCRIPTOR          PpiList;
  EFI_SMRAM_DESCRIPTOR            SmramDesc[SMRAM_DESC_COUNT];
} SMM_ACCESS_PRIVATE_DATA;

#define SMM_ACCESS_PRIVATE_DATA_FROM_THIS(a) \
  CR(a, SMM_ACCESS_PRIVATE_DATA, SmmAccess, SMM_ACCESS_PRIVATE_DATA_SIGNATURE)

STATIC
EFI_STATUS
EFIAPI
PeiSmmOpen (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  )
{
  SMM_ACCESS_PRIVATE_DATA  *Private;

  if(DescriptorIndex >= SMRAM_DESC_COUNT){
    return EFI_INVALID_PARAMETER;
  }
  
  Private = SMM_ACCESS_PRIVATE_DATA_FROM_THIS(This);    
  if(Private->CloseLock){
    return EFI_ACCESS_DENIED;
  } else {
    return EFI_SUCCESS;
  }  
}


STATIC
EFI_STATUS
EFIAPI
PeiSmmClose (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  )
{
  SMM_ACCESS_PRIVATE_DATA  *Private;

  if(DescriptorIndex >= SMRAM_DESC_COUNT){
    return EFI_INVALID_PARAMETER;
  }

  Private = SMM_ACCESS_PRIVATE_DATA_FROM_THIS(This);  
  if(!Private->CloseLock){
    MmioOr8(HIF_PCI_REG(SMM_APIC_DECODE_REG), TSMM_EN); 
    Private->CloseLock = TRUE;    
  }  
  return EFI_SUCCESS;
}


STATIC  
EFI_STATUS
EFIAPI
PeiSmmLock (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  )
{
  SMM_ACCESS_PRIVATE_DATA  *Private;
  
  if(DescriptorIndex >= SMRAM_DESC_COUNT){
    return EFI_INVALID_PARAMETER;
  }  
  
  Private = SMM_ACCESS_PRIVATE_DATA_FROM_THIS(This);  
  if(!Private->CloseLock){
    MmioOr8(HIF_PCI_REG(SMM_APIC_DECODE_REG), TSMM_EN); 
    Private->CloseLock = TRUE;    
  }  
  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
EFIAPI
PeiSmmGetCapabilities (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  )
{
  UINTN                    DescSize;
  EFI_STATUS               Status;
  SMM_ACCESS_PRIVATE_DATA  *Private;
  
  Private = SMM_ACCESS_PRIVATE_DATA_FROM_THIS(This);
  DescSize = sizeof(Private->SmramDesc);
  if (*SmramMapSize < DescSize) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    CopyMem (SmramMap, Private->SmramDesc, DescSize);
    Status = EFI_SUCCESS;
  }

  *SmramMapSize = DescSize;  
  return Status;
}


EFI_STATUS
SmmAccessPpiInstall (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS               Status;
  PLATFORM_MEMORY_INFO     *MemInfo;
  SMM_ACCESS_PRIVATE_DATA  *Private;  
  BOOLEAN                  CloseLock;

  
  Private = AllocateZeroPool(sizeof(*Private));
  if (Private == NULL) {
    DEBUG((EFI_D_ERROR, "Alloc SmmAccessPrivate fail.\n"));
    return EFI_OUT_OF_RESOURCES;
  }  
 
  CloseLock = (MmioRead8(HIF_PCI_REG(SMM_APIC_DECODE_REG)) & TSMM_EN)?TRUE:FALSE;   
 
  Private->Signature = SMM_ACCESS_PRIVATE_DATA_SIGNATURE;  
  Private->SmmAccess.Open            = PeiSmmOpen;
  Private->SmmAccess.Close           = PeiSmmClose;
  Private->SmmAccess.Lock            = PeiSmmLock;
  Private->SmmAccess.GetCapabilities = PeiSmmGetCapabilities;
  Private->SmmAccess.LockState       = CloseLock;
  Private->SmmAccess.OpenState       = !CloseLock;
  Private->CloseLock                 = CloseLock;

  MemInfo = (PLATFORM_MEMORY_INFO*)GetPlatformMemInfo(); 	
  ASSERT(SMRAM_DESC_COUNT >= 1);
  Private->SmramDesc[0].PhysicalStart = MemInfo->TSegAddr;
  Private->SmramDesc[0].CpuStart      = MemInfo->TSegAddr;
  Private->SmramDesc[0].PhysicalSize  = MemInfo->TSegSize;
  if(CloseLock){
    Private->SmramDesc[0].RegionState = EFI_SMRAM_CLOSED | EFI_SMRAM_LOCKED;
  } else {
    Private->SmramDesc[0].RegionState = EFI_SMRAM_OPEN;
  }
  
  Private->PpiList.Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  Private->PpiList.Guid  = &gPeiSmmAccessPpiGuid;
  Private->PpiList.Ppi   = &Private->SmmAccess;

  Status = PeiServicesInstallPpi(&Private->PpiList);
  ASSERT_EFI_ERROR (Status);

  return Status;
}




