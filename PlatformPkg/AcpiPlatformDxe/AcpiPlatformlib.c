
#include "AcpiPlatform.h"
#include <Protocol/MpService.h>




EFI_STATUS
GetCpuLocalApicInfo (
    CPU_APIC_ID_INFO      **CpuApicIdTables,
    UINTN                 *CpuCount
  )
{
  EFI_MP_SERVICES_PROTOCOL     *MpService;
  EFI_PROCESSOR_INFORMATION    ProcInfo;
  UINTN                        Index;
  EFI_STATUS                   Status;
  UINTN                        NumberOfCPUs;
  UINTN                        NumberOfEnCPUs;
  CPU_APIC_ID_INFO             *ApicIdInfo;


  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  &MpService
                  );
  ASSERT_EFI_ERROR(Status);

  MpService->GetNumberOfProcessors (
               MpService,
               &NumberOfCPUs,
               &NumberOfEnCPUs
               );
  ASSERT_EFI_ERROR(Status); 
  
  ApicIdInfo = AllocateZeroPool(NumberOfCPUs * sizeof(CPU_APIC_ID_INFO));         
  ASSERT_EFI_ERROR(Status);
              
  for (Index = 0; Index < NumberOfCPUs; Index++) {
    Status = MpService->GetProcessorInfo (
                          MpService,
                          Index,
                          &ProcInfo
                          );
    ASSERT_EFI_ERROR (Status);
    ApicIdInfo[Index].ApicId  = (UINT8)ProcInfo.ProcessorId;
    ApicIdInfo[Index].Flags   = 1;
  }

  *CpuCount = NumberOfCPUs;
  *CpuApicIdTables = ApicIdInfo;

  return  EFI_SUCCESS;
}
