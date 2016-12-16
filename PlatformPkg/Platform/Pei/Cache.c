
#include "PlatformPei.h"
#include <Ppi/CpuCachePpi.h>


typedef struct {
  UINT8   Type;
  UINT8   Reserved0:2;
  UINT8   FE:1;
  UINT8   E:1;
  UINT8   Reserved1:4;
  UINT8   Reserved2[6];
} IA32_MTRR_DEF;







VOID
SetCacheMtrr (
  IN EFI_PEI_SERVICES      **PeiServices,
  IN PLATFORM_MEMORY_INFO  *MemInfo
  )
{
  EFI_STATUS                  Status;
  UINT64                      MemoryBase;
  UINT64                      MemoryLength;
  PEI_CACHE_PPI               *CachePpi;
	
  
  Status = PeiServicesLocatePpi (
             &gCpuCachePpiGuid,
             0,
             NULL,
             (VOID**)&CachePpi
             );
  ASSERT_EFI_ERROR(Status);

  CachePpi->ResetCache (
              PeiServices,
              CachePpi
              );

  CachePpi->SetCache (
              PeiServices,
              CachePpi,
              PcdGet32(PcdFlashAreaBaseAddress),
              PcdGet32(PcdFlashAreaSize),
              EfiCacheTypeWriteProtected
              );

  MemoryBase   = 0;
  MemoryLength = MemInfo->VgaBufAddr;
  CachePpi->SetCache (
              PeiServices,
              CachePpi,
              MemoryBase,
              MemoryLength,
              EfiCacheTypeWriteBack
              );
         
  MemoryBase   = 0x00000;
  MemoryLength = 0xA0000;
  CachePpi->SetCache (
              PeiServices,
              CachePpi,
              MemoryBase,
              MemoryLength,
              EfiCacheTypeWriteBack
              );              
              
  MemoryBase   = 0xA0000;
  MemoryLength = 0x20000;
  CachePpi->SetCache (
              PeiServices,
              CachePpi,
              MemoryBase,
              MemoryLength,
              EfiCacheTypeUncacheable
              );

  MemoryBase   = 0xC0000;
  MemoryLength = 0x40000;
  CachePpi->SetCache (
              PeiServices,
              CachePpi,
              MemoryBase,
              MemoryLength,
              EfiCacheTypeWriteProtected
              );              
              
  CachePpi->ActivateCache (
              PeiServices,
              CachePpi
              );
}






VOID
SetCacheMtrrAtNormalPeiEnd (
  IN EFI_PEI_SERVICES      **PeiServices,
  IN PLATFORM_MEMORY_INFO  *MemInfo
  )
{
  EFI_STATUS                  Status;
  UINT64                      MemoryBase;
  UINT64                      MemoryLength;
  PEI_CACHE_PPI               *CachePpi;
  UINT64                      Data64;

  
  DEBUG((DEBUG_INFO, __FUNCTION__"()\n"));

  Status = PeiServicesLocatePpi (
             &gCpuCachePpiGuid,
             0,
             NULL,
             (VOID**)&CachePpi
             );
  ASSERT_EFI_ERROR(Status);

  CachePpi->ResetCache (
              PeiServices,
              CachePpi
              );

  MemoryBase   = 0x00000;
  MemoryLength = 0xA0000;
  CachePpi->SetCache (
              PeiServices,
              CachePpi,
              MemoryBase,
              MemoryLength,
              EfiCacheTypeWriteBack
              );              
              
  MemoryBase   = 0xA0000;
  MemoryLength = 0x20000;
  CachePpi->SetCache (
              PeiServices,
              CachePpi,
              MemoryBase,
              MemoryLength,
              EfiCacheTypeUncacheable
              );

  MemoryBase   = 0xC0000;
  MemoryLength = 0x40000;
  CachePpi->SetCache (
              PeiServices,
              CachePpi,
              MemoryBase,
              MemoryLength,
              EfiCacheTypeWriteProtected
              );  
  
  //ERC20161103 Update Uncacheable Base to ToLum -S
/*
  MemoryBase   = MemInfo->VgaBufAddr;
  MemoryLength = SIZE_4GB - MemoryBase;
  CachePpi->SetCache (
              PeiServices,
              CachePpi,
              MemoryBase,
              MemoryLength,
              EfiCacheTypeUncacheable
              );
              */
  MemoryBase   = MemInfo->Tolum;
  MemoryLength = SIZE_4GB - MemoryBase;
  CachePpi->SetCache (
			  PeiServices,
			  CachePpi,
			  MemoryBase,
			  MemoryLength,
			  EfiCacheTypeUncacheable
			  );
  //ERC20161103 Update Uncacheable Base to ToLum -E

  CachePpi->SetCache (
              PeiServices,
              CachePpi,
              MemInfo->Pci64Base,
              MemInfo->Pci64Size,
              EfiCacheTypeUncacheable
              );
  
  CachePpi->ActivateCache (
              PeiServices,
              CachePpi
              );
  
// default cache as WB.
  Data64 = AsmReadMsr64(MTRR_LIB_IA32_MTRR_DEF_TYPE);
  Data64 = (Data64 & (~0xFF))|CacheWriteBack;
  AsmWriteMsr64(MTRR_LIB_IA32_MTRR_DEF_TYPE, Data64);
  
}



VOID
SetCacheMtrrAtS3PeiEnd (
  IN EFI_PEI_SERVICES      **PeiServices
  )
{
  MtrrSetAllMtrrs((MTRR_SETTINGS*)GetS3MtrrTable());
}




VOID
ShowCacheSetting (
  VOID
  )
{
  MTRR_SETTINGS  MtrrSet;
  UINTN          Index;
  UINTN          VariableMtrrCount;
	IA32_MTRR_DEF  *DefMtrr;


//DEBUG((DEBUG_INFO, "%a()\n", __FUNCTION__));
  MtrrGetAllMtrrs(&MtrrSet);
  DefMtrr = (IA32_MTRR_DEF*)&MtrrSet.MtrrDefType;
  DEBUG((DEBUG_INFO, "T:%d, FE:%d, E:%d\n",   DefMtrr->Type, DefMtrr->FE, DefMtrr->E));  
  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
    DEBUG((DEBUG_INFO, "%016lX\n", MtrrSet.Fixed.Mtrr[Index]));
  }
  
  VariableMtrrCount = GetVariableMtrrCount();
  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if(!(MtrrSet.Variables.Mtrr[Index].Mask & BIT11)){     // unused, skip
      continue;
    }  
    DEBUG((DEBUG_INFO, "VarMTRR[%02d]: (%016lX,%016lX)\n", 
                       Index, 
                       MtrrSet.Variables.Mtrr[Index].Base, 
                       MtrrSet.Variables.Mtrr[Index].Mask
                       ));
  }  
}


VOID  ShowVarCacheInfo()
{
  UINT32  VariableMtrrCount;
  UINTN   Index;
  UINT64  Mask;

  if (IsMtrrSupported ()) {
    VariableMtrrCount = GetVariableMtrrCount();
    for (Index = 0; Index < VariableMtrrCount; Index++) {
      Mask = AsmReadMsr64(MTRR_LIB_IA32_VARIABLE_MTRR_BASE + (Index << 1) + 1);
      if(Mask & BIT11){
        DEBUG((EFI_D_INFO, "[%d] %lX ",  Index, AsmReadMsr64(MTRR_LIB_IA32_VARIABLE_MTRR_BASE + (Index << 1))));
        DEBUG((EFI_D_INFO, "%lX\n", Mask));
      }
    }
  }
}  


