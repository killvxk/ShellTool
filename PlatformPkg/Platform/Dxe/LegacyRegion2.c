
#include "PlatformDxe.h"
#include <Protocol/LegacyRegion2.h>
#include <Protocol/Cpu.h>


#define PAM_GRANULARITY           0x4000
#define UNLOCK                    0x0000
#define LOCK                      0x0001
#define BOOTLOCK                  0x0002
#define LEGACY_REGION_DECODE_ROM  0x0003

  EFI_CPU_ARCH_PROTOCOL           *gCpuArch;
//STATIC UINT8 gRomData[16] = {
//            0x00, 0x00, 0x00, 0x00,
//            0x00, 0x00, 0x00, 0x00,
//            0x00, 0x00, 0x00, 0x00,
//            0x00, 0x00, 0x00, 0x00
//            };

/// D0F3 regs' Lock value.
//STATIC UINT8 gLockData[16] = {
//            0x02, 0x08, 0x20, 0x80,
//            0x02, 0x08, 0x20, 0x80,
//            0x02, 0x08, 0x20, 0x80,
//            0x20, 0x20, 0x20, 0x20
//            };

//STATIC UINT8 gUnlockData[16] = {
//            0x03, 0x0C, 0x30, 0xC0,
//            0x03, 0x0C, 0x30, 0xC0,
//            0x03, 0x0C, 0x30, 0xC0,
//            0x30, 0x30, 0x30, 0x30
//            };

/// D0F3 regs' Mask Value.
//STATIC UINT8 gMaskData[] = {
//            0x03, 0x0C, 0x30, 0xC0,
//            0x03, 0x0C, 0x30, 0xC0,
//            0x03, 0x0C, 0x30, 0xC0,
//            0x30 /*, 0x30, 0x30, 0x30*/
//            };

/// D0F2 regs 
STATIC UINT16 gReg[] = {
            0xA0, 0xA0, 0xA0, 0xA0, // Page-C ROM Shadow 
            0xA1, 0xA1, 0xA1, 0xA1, // Page-D ROM Shadow 
            0xA2, 0xA2, 0xA2, 0xA2, // Page-E ROM Shadow 
            0xA3 /*, 0x83, 0x83, 0x83,*/ // Page-F ROM Shadow
            };

STATIC UINT8 gShiftBits [] = {
            0x6, 0x4, 0x2, 0x0,
            0x6, 0x4, 0x2, 0x0,
            0x6, 0x4, 0x2, 0x0,
            0x0};

            
#define MAX_SHADOW_REGIONS 13            
EFI_STATUS
LegacyRegionManipulation (
  IN  EFI_LEGACY_REGION2_PROTOCOL *This,
  IN  UINT32                      Start,
  IN  UINT32                      Length,
  IN  UINT32                      Mode,
  OUT UINT32                      *Granularity
  )
{
  UINTN                           Index;
  UINT8                           Data;
  UINT32                          TempAddr;
  UINT64                          Attributes;
  EFI_STATUS                      Status;
UINTN StartIndex, EndIndex;
UINT8 Setting, Mask;
  //DEBUG((EFI_D_ERROR,"into LegacyRegionManipulation(). Addr=%x,Len=%x,Mode=%x\n",Start, Length, Mode));
  ///
  if ((Start < 0xC0000) || ((Start + Length - 1) > 0xFFFFF)) {
    return EFI_INVALID_PARAMETER;
  }
 // ASSERT(Granularity != NULL);
 if(Granularity) {
 	*Granularity = ( (Start+Length) < 0xf0000 ) ? 0x4000	: 0x10000;
 }
 // *Granularity = PAM_GRANULARITY;  

  if(Start >= 0xF0000) {
  	StartIndex = MAX_SHADOW_REGIONS - 1;
  } else {
	StartIndex = (Start - 0xC0000) / 0x4000;
  }
  TempAddr = Start + Length - 1;
  if(TempAddr >= 0xF0000) {
  	EndIndex = MAX_SHADOW_REGIONS - 1;
  } else {
	EndIndex = (TempAddr - 0xC0000) / 0x4000;
  }
//DEBUG((EFI_D_ERROR,"StartIndex=%x,EndIndex=%x\n",StartIndex, EndIndex));

  switch(Mode) {
    case UNLOCK:
	    Setting = 03;
        Attributes = EFI_MEMORY_WT;
	    break;
	case LOCK:
	case BOOTLOCK:
		Setting = 02;
        Attributes = EFI_MEMORY_WP;
		break;
	default:
		Setting = 0;
        Attributes = EFI_MEMORY_UC;
  }
  //ASSERT(StartIndex<=EndIndex && EndIndex<13);
  for(Index = StartIndex; Index <= EndIndex; Index++) {
  	 Data = (Setting << gShiftBits[Index]);
	 Mask = (0x3 << gShiftBits[Index]);
     MmioAndThenOr8(HIF_PCI_REG(gReg[Index]),   ~Mask, Data);
  }

//Adjust address and length for FiX MTRR programming.
 //YKN-20161008 -S
 Length += Start & 0xFFF;  //If start not align to 4K, add the offset to Length
 //YKN-20161008 -E

 Start &= ~0xfff;
 Length = (Length + 0xfff) & ~0xfff;
 Status = gCpuArch->SetMemoryAttributes (
                      gCpuArch,
                      Start,
                      Length,
                      Attributes
                      );
  //if(EFI_ERROR(Status)){
    DEBUG((EFI_D_ERROR,"SetMemAttributes(%x,%x) %x result: %r\n", Start, Length,Attributes,Status));
  //}		
  return Status;
}


/**
  Modify the hardware to allow (decode) or disallow (not decode) memory reads in a region.

  If the On parameter evaluates to TRUE, this function enables memory reads in the address range 
  Start to (Start + Length - 1).
  If the On parameter evaluates to FALSE, this function disables memory reads in the address range 
  Start to (Start + Length - 1).

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose attributes
                                should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address
                                was not aligned to a region's starting address or if the length
                                was greater than the number of bytes in the first region.
  @param  On[in]                Decode / Non-Decode flag.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2Decode (
  IN  EFI_LEGACY_REGION2_PROTOCOL  *This,
  IN  UINT32                       Start,
  IN  UINT32                       Length,
  OUT UINT32                       *Granularity,
  IN  BOOLEAN                      *On
  )
{
  DEBUG((EFI_D_ERROR, "%a(%X,%X) On:%d\n", __FUNCTION__, Start, Length, *On));
  if (*On) {
    return LegacyRegionManipulation (This, Start, Length, UNLOCK, Granularity);
  } else {
    return LegacyRegionManipulation (This, Start, Length, LEGACY_REGION_DECODE_ROM, Granularity);
    //return EFI_SUCCESS;
  }
}

/**
  Modify the hardware to disallow memory writes in a region.

  This function changes the attributes of a memory range to not allow writes.

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose
                                attributes should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address was
                                not aligned to a region's starting address or if the length was
                                greater than the number of bytes in the first region.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2Lock (
  IN  EFI_LEGACY_REGION2_PROTOCOL *This,
  IN  UINT32                      Start,
  IN  UINT32                      Length,
  OUT UINT32                      *Granularity
  )
{
  DEBUG((EFI_D_INFO, "%a(%X,%X)\n", __FUNCTION__, Start, Length));
  return LegacyRegionManipulation (This, Start, Length, LOCK, Granularity);
}

/**
  Modify the hardware to disallow memory attribute changes in a region.

  This function makes the attributes of a region read only. Once a region is boot-locked with this 
  function, the read and write attributes of that region cannot be changed until a power cycle has
  reset the boot-lock attribute. Calls to Decode(), Lock() and Unlock() will have no effect.

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose
                                attributes should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address was
                                not aligned to a region's starting address or if the length was
                                greater than the number of bytes in the first region.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.
  @retval EFI_UNSUPPORTED       The chipset does not support locking the configuration registers in
                                a way that will not affect memory regions outside the legacy memory
                                region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2BootLock (
  IN  EFI_LEGACY_REGION2_PROTOCOL         *This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  OUT UINT32                              *Granularity
  )
{
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  return LegacyRegionManipulation(This, Start, Length, BOOTLOCK, Granularity);
}

/**
  Modify the hardware to allow memory writes in a region.

  This function changes the attributes of a memory range to allow writes.  

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose
                                attributes should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address was
                                not aligned to a region's starting address or if the length was
                                greater than the number of bytes in the first region.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2Unlock (
  IN  EFI_LEGACY_REGION2_PROTOCOL  *This,
  IN  UINT32                       Start,
  IN  UINT32                       Length,
  OUT UINT32                       *Granularity
  )
{
  DEBUG((EFI_D_INFO, "%a(%X,%X)\n", __FUNCTION__, Start, Length));
  return LegacyRegionManipulation (This, Start, Length, UNLOCK, Granularity);
}

/**
  Get region information for the attributes of the Legacy Region.

  This function is used to discover the granularity of the attributes for the memory in the legacy 
  region. Each attribute may have a different granularity and the granularity may not be the same
  for all memory ranges in the legacy region.  

  @param  This[in]              Indicates the EFI_LEGACY_REGION2_PROTOCOL instance.
  @param  DescriptorCount[out]  The number of region descriptor entries returned in the Descriptor
                                buffer.
  @param  Descriptor[out]       A pointer to a pointer used to return a buffer where the legacy
                                region information is deposited. This buffer will contain a list of
                                DescriptorCount number of region descriptors.  This function will
                                provide the memory for the buffer.

  @retval EFI_SUCCESS           The information structure was returned.
  @retval EFI_UNSUPPORTED       This function is not supported.

**/
EFI_STATUS
EFIAPI
LegacyRegionGetInfo (
  IN  EFI_LEGACY_REGION2_PROTOCOL   *This,
  OUT UINT32                        *DescriptorCount,
  OUT EFI_LEGACY_REGION_DESCRIPTOR  **Descriptor
  )
{
//DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  return EFI_UNSUPPORTED;
}


STATIC EFI_LEGACY_REGION2_PROTOCOL  gLegacyRegion2 = {
  LegacyRegion2Decode,
  LegacyRegion2Lock,
  LegacyRegion2BootLock,
  LegacyRegion2Unlock,
  LegacyRegionGetInfo
};


EFI_STATUS
EFIAPI
LegacyRegion2Install (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  
  Status = gBS->LocateProtocol (
                  &gEfiCpuArchProtocolGuid,
                  NULL,
                  &gCpuArch
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiLegacyRegion2ProtocolGuid, &gLegacyRegion2,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
