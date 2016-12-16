
#include <Uefi.h>
#include <Pi/PiBootMode.h>      // HobLib.h +
#include <Pi/PiHob.h>           // HobLib.h +
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/SocketLga775Lib.h>
#include <Library/LocalApicLib.h>
#include <PlatformDefinition.h>
#include <PlatS3Record.h>
#include <SmmFeatures.h>


#define BACK_BUF_SIZE               0x20

typedef struct {
  UINT32                            Offset;
  UINT16                            Segment;
  UINT16                            Reserved;
} IA32_FAR_ADDRESS;

extern UINT32                       gSmmCr0;
extern UINT32                       gSmmCr3;
extern UINT32                       gSmmCr4;
extern CONST UINT8                  gcSmmInitTemplate[];
extern CONST UINT16                 gcSmmInitSize;
extern IA32_FAR_ADDRESS             gSmmJmpAddr;
extern UINTN                        gSmmInitStack;

VOID
SemaphoreHook (
  IN BOOLEAN                       *RebasedFlag,
  IN SOCKET_LGA_775_SMM_CPU_STATE  *CpuState
  );

STATIC volatile BOOLEAN gRebased[PLATFORM_MAX_CPU_PROC_COUNT];
PLATFORM_S3_RECORD      *gS3Record;



VOID
EFIAPI
SmmRelocateBases (
  VOID
  )
{
  UINT8                             BakBuf[BACK_BUF_SIZE];
  SOCKET_LGA_775_SMM_CPU_STATE      BakBuf2;
  SOCKET_LGA_775_SMM_CPU_STATE      *CpuStatePtr;
  UINT8                             *U8Ptr;
  UINT32                            ApicId;
  UINT32                            Index;
 
         
  DEBUG((EFI_D_INFO, __FUNCTION__"\n"));
  ASSERT (sizeof(BakBuf) >= gcSmmInitSize);  

  gS3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);

// Patch ASM code template with current CR0, CR3, and CR4 values
  gSmmCr0 = (UINT32)AsmReadCr0();
  gSmmCr3 = (UINT32)AsmReadCr3();
  gSmmCr4 = (UINT32)AsmReadCr4();
  if (sizeof (UINTN) == sizeof (UINT64)) {
    gSmmJmpAddr.Segment = 8;
  }  
//gSmmInitStack = (UINT32)(UINTN)AllocatePages(1) + SIZE_4KB - sizeof(UINTN);
  gSmmInitStack = gS3Record->S3StackBase + gS3Record->S3StackSize - 8;

  U8Ptr = (UINT8*)(UINTN)(SMM_DEFAULT_SMBASE + SMM_HANDLER_OFFSET);
  CpuStatePtr = (SOCKET_LGA_775_SMM_CPU_STATE*)(UINTN)(SMM_DEFAULT_SMBASE + SMM_CPU_STATE_OFFSET);

// Backup original contents @ 0x38000
  CopyMem (BakBuf, U8Ptr, sizeof(BakBuf));
  CopyMem (&BakBuf2, CpuStatePtr, sizeof(BakBuf2));
  CopyMem (U8Ptr, gcSmmInitTemplate, gcSmmInitSize);
  
  ApicId = GetApicId(); 
//DEBUG((EFI_D_INFO, "CpuCount:%d\n", gS3Record->CpuCount));  
  for (Index = 0; Index < gS3Record->CpuCount; Index++) {
    if (ApicId != gS3Record->CpuApicId[Index]) {
      DEBUG((EFI_D_INFO, "CPU[%d] %X\n", Index, gS3Record->CpuApicId[Index]));
      SendSmiIpi (gS3Record->CpuApicId[Index]);
      while (!gRebased[Index]){};     // Wait for this AP to finish its 1st SMI
      gRebased[Index] = FALSE;
    }
  }
  DEBUG((EFI_D_INFO, "RebaseBSP\n"));   
  SendSmiIpi(ApicId);
  
  CopyMem(CpuStatePtr, &BakBuf2, sizeof(BakBuf2));	// Restore contents @ 0x38000
  CopyMem(U8Ptr, BakBuf, sizeof(BakBuf));

}


VOID
ACpuInitSmrr (
  IN UINT32                SmrrBase,
  IN UINT32                SmrrSize,
  IN UINT32                SmrrType
  )
{
  AsmWriteMsr64(EFI_MSR_SMRR_PHYS_BASE, SmrrBase | SmrrType);
  AsmWriteMsr64(EFI_MSR_SMRR_PHYS_MASK, (~(SmrrSize - 1) & EFI_MSR_SMRR_MASK));
}


VOID
EFIAPI
SmmInitHandler (
  VOID
  )
{
  UINT32                            ApicId;
  UINTN                             Index;
  SOCKET_LGA_775_SMM_CPU_STATE      *CpuState;
	BOOLEAN                           IsBsp;


  DEBUG((EFI_D_INFO, __FUNCTION__"\n"));
  
  ApicId = GetApicId();
	IsBsp  = (BOOLEAN)((AsmReadMsr64(EFI_MSR_IA32_APIC_BASE) & BIT8)?TRUE:FALSE);

  for (Index = 0; Index < gS3Record->CpuCount; Index++) {
    if (ApicId == gS3Record->CpuApicId[Index]) {
      CpuState = (SOCKET_LGA_775_SMM_CPU_STATE *)(UINTN)(SMM_DEFAULT_SMBASE + SMM_CPU_STATE_OFFSET);
      CpuState->x86.SMBASE = gS3Record->CpuSmBase[Index];

      if(gS3Record->SmrrBase && gS3Record->SmrrSize){
        ACpuInitSmrr(gS3Record->SmrrBase, gS3Record->SmrrSize, gS3Record->SmrrType);
      } 
      
      if (!IsBsp) {
        SemaphoreHook((BOOLEAN*)&gRebased[Index], CpuState);
      } else {
        if((VOID*)(UINTN)gS3Record->SmmUcData != NULL){
          ZeroMem((VOID*)(UINTN)gS3Record->SmmUcData, gS3Record->SmmUcDataSize);
        }					
      }

      return;
    }
  }
  ASSERT(FALSE);
}


EFI_STATUS
EFIAPI
SmmRebaseS3Entry (
  IN       EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_BOOT_MODE   BootMode;  
  EFI_STATUS      Status;
  
  Status = (*PeiServices)->GetBootMode(PeiServices, &BootMode);  
  ASSERT_EFI_ERROR(Status);   
  
  if(BootMode == BOOT_ON_S3_RESUME){
    SmmRelocateBases();
  }  
  
  return Status;
}  
