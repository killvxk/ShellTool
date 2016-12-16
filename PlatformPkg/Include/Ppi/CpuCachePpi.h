
#ifndef __CPU_CACHE_PPI_H__
#define __CPU_CACHE_PPI_H__

#include <Uefi.h>

// {F3EBCDE2-400A-4fc6-9FC3-4FDC4126A1B4}
#define CPU_CACHE_PPI_GUID \
{ 0xf3ebcde2, 0x400a, 0x4fc6, {0x9f, 0xc3, 0x4f, 0xdc, 0x41, 0x26, 0xa1, 0xb4 }}




typedef struct _PEI_CACHE_PPI  PEI_CACHE_PPI;

typedef enum _EFI_MEMORY_CACHE_TYPE
{
  EfiCacheTypeUncacheable   = 0,
  EfiCacheTypeWriteCombining= 1,
  EfiCacheTypeReserved2     = 2,
  EfiCacheTypeReserved3     = 3,
  EfiCacheTypeWriteThrough  = 4,
  EfiCacheTypeWriteProtected= 5,
  EfiCacheTypeWriteBack     = 6,
  EfiCacheTypeMaximumType   = 7
} EFI_MEMORY_CACHE_TYPE;


typedef
EFI_STATUS
(EFIAPI *PEI_SET_CACHE_PPI) (
  IN  CONST EFI_PEI_SERVICES            **PeiServices,
  IN  PEI_CACHE_PPI                     * This,
  IN  EFI_PHYSICAL_ADDRESS              MemoryAddress,
  IN  UINT64                            MemoryLength,
  IN  EFI_MEMORY_CACHE_TYPE             MemoryCacheType
  );

typedef
EFI_STATUS
(EFIAPI *PEI_RESET_CACHE_PPI) (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN PEI_CACHE_PPI                      * This
  );

typedef
EFI_STATUS
(EFIAPI *PEI_ACTIVATE_CACHE_PPI) (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN PEI_CACHE_PPI                      * This
  );

typedef struct _PEI_CACHE_PPI {
  PEI_SET_CACHE_PPI      SetCache;
  PEI_RESET_CACHE_PPI    ResetCache;
  PEI_ACTIVATE_CACHE_PPI ActivateCache;
} PEI_CACHE_PPI;


extern EFI_GUID gCpuCachePpiGuid;

#endif

