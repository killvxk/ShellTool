
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <PlatformDefinition.h>


VOID UpdateTolumVariable(UINTN BitsOfAlignment, UINT64 AddrLen)
{
  UINT32                 TotalSize;
  UINT32                 Tolum;
  UINTN                  VarSize;
  PLAT_NV_INFO           NvInfo;
  UINT16                 TolumMB;
  UINT16                 HobTolumMB;
  UINT32                 MemSizeMB;
  EFI_STATUS             Status;
  EFI_BOOT_MODE          BootMode; 
  PLATFORM_MEMORY_INFO   *MemInfo;
  EFI_PEI_HOB_POINTERS   GuidHob;  
  
  DEBUG((EFI_D_ERROR, "into UpdateTolumVariable() \n"));

  BootMode = GetBootModeHob();
  if (BootMode == BOOT_ON_FLASH_UPDATE) {
    return;
  }  
  
  ASSERT(AddrLen < SIZE_4GB);
  TotalSize = (UINT32)AddrLen;
  
  GuidHob.Raw = GetFirstGuidHob(&gEfiPlatformMemInfoGuid);
  ASSERT(GuidHob.Raw != NULL);  
  MemInfo  = (PLATFORM_MEMORY_INFO*)(GuidHob.Guid+1);   
  MemSizeMB = (UINT32)RShiftU64(MemInfo->PhyMemSize, 20);  
  
  if(TotalSize > (PCI_MMIO_TOP_ADDRESS - 0xF0000000)){    // F0000000 ~ FE000000 is the second part of MMIO.
    Tolum  = 0xE0000000 - TotalSize;
    if(Tolum & (SIZE_256MB  - 1)){
      Tolum  = ALIGN_VALUE(Tolum, SIZE_256MB);
      Tolum -= SIZE_256MB;
    }    
  } else {
    Tolum = 0xE0000000;
  }
  TolumMB = (UINT16)(Tolum >> 20);
  if((UINT32)TolumMB > MemSizeMB){
    TolumMB = (UINT16)MemSizeMB;
  }
  
  DEBUG((EFI_D_ERROR, "WantTolumMB:%X\n", TolumMB));

  VarSize = sizeof(PLAT_NV_INFO);
  ZeroMem(&NvInfo, VarSize);  
  Status = gRT->GetVariable (
                  NVINFO_TOLUM_VAR_NAME,
                  &gEfiPlatformNvInfoGuid,
                  NULL,
                  &VarSize,
                  &NvInfo
                  );   
  if(EFI_ERROR(Status) || TolumMB != NvInfo.Tolum){
    NvInfo.Tolum = TolumMB;
    Status = gRT->SetVariable (
                    NVINFO_TOLUM_VAR_NAME,
                    &gEfiPlatformNvInfoGuid,
                    NVINFO_TOLUM_VAR_ATTRIBUTE,
                    sizeof(PLAT_NV_INFO),
                    &NvInfo
                    );
    ASSERT(!EFI_ERROR(Status));
    
    HobTolumMB = (UINT16)(MemInfo->Tolum >> 20); 
    if(HobTolumMB != TolumMB){
      DEBUG((EFI_D_INFO, "Tolum Changed(Hob:%X, Var:%X, Want:%X), Reset ...\n", HobTolumMB, NvInfo.Tolum, TolumMB));
      /// TGR-2016062801+S	  
      DEBUG((EFI_D_INFO, "Not reset for HAPS low speed \n"));
      gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
      /// TGR-2016062801+E
    }  
  }
}


