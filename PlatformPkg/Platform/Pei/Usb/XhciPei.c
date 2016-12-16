
#include <PlatformDefinition.h>
#include <UsbController.h>
#include <IndustryStandard/Pci.h>
#include <Library/TimerLib.h>
#include <Library/PlatformCommLib.h>


STATIC
EFI_STATUS
GetXhciFwFromFv (
  OUT VOID   **pHubFile,
  OUT UINT32  *pHubSize,
  OUT VOID   **pMcuFile,
  OUT UINT32  *pMcuSize
  )
{
  EFI_STATUS                 Status;
  UINTN                      Instance;
  EFI_PEI_FV_HANDLE          VolumeHandle;
  EFI_PEI_FILE_HANDLE        FileHandle;
  VOID                       *McuFile;
  VOID                       *HubFile;
  BOOLEAN                    HasFound;
  UINT32                     McuSize;
  UINT32                     HubSize;
  EFI_PHYSICAL_ADDRESS       Address;

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  Instance = 0;
  HasFound = FALSE;
  McuFile  = NULL;
  HubFile  = NULL;

  ASSERT(pHubFile!=NULL && pMcuFile!=NULL);

  while (1) {
    Status = PeiServicesFfsFindNextVolume(Instance, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
    if((UINT32)(UINTN)VolumeHandle >= PcdGet32(PcdFlashFvMainBase)){
      goto NextCycle;
    }

    Status = PeiServicesFfsFindFileByName((EFI_GUID*)PcdGetPtr(PcdXhciMcuFwFile), VolumeHandle, &FileHandle);
    if(EFI_ERROR(Status)){
      goto NextCycle;
    }

    Status = PeiServicesFfsFindSectionData(EFI_SECTION_RAW, FileHandle, &McuFile);
    ASSERT_EFI_ERROR(Status);
    Status = PeiServicesFfsFindFileByName((EFI_GUID*)PcdGetPtr(PcdXhciHubFwFile), VolumeHandle, &FileHandle);
    ASSERT_EFI_ERROR(Status);
    Status = PeiServicesFfsFindSectionData(EFI_SECTION_RAW, FileHandle, &HubFile);
    ASSERT_EFI_ERROR(Status);
    HasFound = TRUE;
    break;

NextCycle:
    Instance++;
  }

  if(!HasFound){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  McuSize  = *(UINT32*)(&((EFI_COMMON_SECTION_HEADER*)((UINT8*)McuFile - sizeof(EFI_COMMON_SECTION_HEADER)))->Size);
  McuSize &= 0xFFFFFF;
  McuSize -= sizeof(EFI_COMMON_SECTION_HEADER);
  HubSize  = *(UINT32*)(&((EFI_COMMON_SECTION_HEADER*)((UINT8*)HubFile - sizeof(EFI_COMMON_SECTION_HEADER)))->Size);
  HubSize &= 0xFFFFFF;
  HubSize -= sizeof(EFI_COMMON_SECTION_HEADER);

  DEBUG((EFI_D_INFO, "Mcu(%X,%X), Hub(%X,%X)\n", McuFile, McuSize, HubFile, HubSize));

  Status = PeiServicesAllocatePages(EfiBootServicesData, EFI_SIZE_TO_PAGES(McuSize+0x10000), &Address);
  ASSERT_EFI_ERROR(Status);
  DEBUG((EFI_D_INFO, "(L%d)Address:%lX\n", __LINE__, Address));
  Address = ALIGN_VALUE(Address, 0x10000);
  CopyMem((VOID*)(UINTN)Address, McuFile, McuSize);
  McuFile = (VOID*)(UINTN)Address;

  Status = PeiServicesAllocatePages(EfiBootServicesData, EFI_SIZE_TO_PAGES(HubSize+0x10000), &Address);
  ASSERT_EFI_ERROR(Status);
  DEBUG((EFI_D_INFO, "(L%d)Address:%lX\n", __LINE__, Address));
  Address = ALIGN_VALUE(Address, 0x10000);
  CopyMem((VOID*)(UINTN)Address, HubFile, HubSize);
  HubFile = (VOID*)(UINTN)Address;

  *pHubFile = HubFile;
  *pHubSize = HubSize;
  *pMcuFile = McuFile;
  *pMcuSize = McuSize;

ProcExit:
  DEBUG((EFI_D_INFO, "(L%d)%r\n", __LINE__, Status));
  return Status;
}


EFI_STATUS
HandleXhciFwForRecovery ()
{
  EFI_STATUS          Status;
  VOID                *McuFile;
  VOID                *HubFile;
  UINT32              McuSize;
  UINT32              HubSize;

  if(MmioRead16(XHCI_PCI_REG(PCI_VENDOR_ID_OFFSET)) == 0xFFFF){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }

  Status = GetXhciFwFromFv(&HubFile, &HubSize, &McuFile, &McuSize);
  ASSERT_EFI_ERROR(Status);
  Status = LoadXhciFw(HubFile, McuFile);
  ASSERT_EFI_ERROR(Status);

ProcExit:
  return Status;
}

