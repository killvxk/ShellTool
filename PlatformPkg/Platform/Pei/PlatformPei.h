

#ifndef __PLATFORM_PEI_H__
#define __PLATFORM_PEI_H__
//-----------------------------------------------------------------------------


#include <PiPei.h>
#include <Pi/PiBootMode.h>      // HobLib.h +
#include <Pi/PiHob.h>           // HobLib.h +
#include <Guid/MemoryTypeInformation.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Ppi/Smbus.h>
#include <Ppi/MasterBootMode.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/Capsule.h>
#include <Library/PlatformCommLib.h>


#ifndef EFI_FORWARD_DECLARATION
#define EFI_FORWARD_DECLARATION(x) typedef struct _##x x
#endif

#include <AsiaSbPpi.h>
#include <SbLibPpi.h>
#include <AsiaNbPpi.h>
#include <AsiaCpuPpi.h>
#include <AsiaDramPpi.h>
#include <CHX001Cfg.h>
#include <PlatformDefinition.h>
#include <PlatS3Record.h>



#define EFI_DEBUG_MASK_PPI_GUID \
  { 0x3bd930fd, 0xf823, 0x4948, {0x86, 0x91, 0x98, 0xe6, 0xfe, 0x36, 0xac, 0xe2 }}

typedef struct {
  UINTN  ImageDebugMask;
} EFI_DEBUG_MASK_PPI;

extern EFI_GUID gPeiRecoveryServicePpiGuid;
extern EFI_GUID gAsiaCpuCfgHobGuid;


VOID UpdateSsid(VOID);

VOID
SetCacheMtrr (
  IN EFI_PEI_SERVICES      **PeiServices,
  IN PLATFORM_MEMORY_INFO  *MemInfo
  );

VOID
SetCacheMtrrAtNormalPeiEnd (
  IN EFI_PEI_SERVICES      **PeiServices,
  IN PLATFORM_MEMORY_INFO  *MemInfo
  );

VOID
SetCacheMtrrAtS3PeiEnd (
  IN EFI_PEI_SERVICES      **PeiServices
  );


EFI_STATUS 
BootModeInit (
  IN CONST EFI_PEI_SERVICES                 **PeiServices,
  IN       EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi,
  OUT      EFI_BOOT_MODE                    *BootMode OPTIONAL
  );

VOID
EndOfPeiDebug (
  IN EFI_BOOT_MODE  BootMode
  );  
  
VOID
PlatformPeiEntryDebug (
  IN EFI_ASIA_CPU_PPI_PROTOCOL  *CpuPpi
  );  
  
EFI_STATUS  
GetAsiaPpi (
  EFI_ASIA_SB_PPI                  **SbPpi,
  EFI_ASIA_NB_PPI                  **NbPpi,
  EFI_ASIA_DRAM_PPI                **DramPpi,
  EFI_ASIA_CPU_PPI_PROTOCOL        **CpuPpi,
  ASIA_SB_LIB_PPI                  **SbLibPpi  
  );

EFI_STATUS 
CpuCachePpiInit (
  VOID
  );

EFI_STATUS
EFIAPI
UpdateAsiaConfig (
  IN     EFI_BOOT_MODE                    BootMode,
  IN OUT ASIA_SB_CONFIGURATION            *SbCfg,
  IN OUT ASIA_NB_CONFIGURATION            *NbCfg,
  IN OUT ASIA_DRAM_CONFIGURATION          *DramCfg,
  IN     EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Var2Ppi
  );

VOID ShowCacheSetting(VOID);
VOID ShowVarCacheInfo();

EFI_STATUS
SmmAccessPpiInstall (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  );

EFI_STATUS
SmmControlPpiInstall (
  IN CONST EFI_PEI_SERVICES  **PeiServices
);

EFI_STATUS
EFIAPI
PeimInitializeRecovery (
  IN EFI_PEI_SERVICES     **PeiServices
  );

EFI_STATUS
EFIAPI
HandleCapsuleBeforeMemInstall (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN       PLATFORM_MEMORY_INFO  *MemInfo,
  OUT      UINT32                *PeiMemAddr
  );

EFI_STATUS
EFIAPI
HandleCapsuleAfterMemInstall (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN       PLATFORM_MEMORY_INFO  *MemInfo
  );  


VOID IssueS3PeiEndSwSmi(EFI_PEI_SERVICES **PeiServices);
  
//------------------------------------------------------------------------------
#endif


