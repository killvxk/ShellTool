/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  PchSpiFlashLib.c

Abstract:

Revision History:

**/
#include "SpiFlashLib.h"
#include "Library/PcdLib.h"

SPI_INIT_TABLE  mInitTable =   {
    0x0,   // VendorId
    0x0,   // DeviceId 0
    0x0,   // DeviceId 1
    {
        SF_INST_WREN,       // Prefix Opcode 0: Write Enable
        SF_INST_EWSR        // Prefix Opcode 1: Enable Write Status Register
    },
    {
        {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz,  EnumSpiOperationJedecId},             // Opcode 0: Read ID
        {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz,  EnumSpiOperationReadData},            // Opcode 1: Read
        {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz,  EnumSpiOperationReadStatus},          // Opcode 2: Read Status Register
        {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz,  EnumSpiOperationWriteDisable},        // Opcode 3: Write Disable
        {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz,  EnumSpiOperationErase_4K_Byte},       // Opcode 4: Sector Erase (4KB)
        {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz,  EnumSpiOperationErase_64K_Byte},      // Opcode 5: Block Erase (64KB)
        {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz,  EnumSpiOperationProgramData_1_Byte},  // Opcode 6: Byte Program
        {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz,  EnumSpiOperationWriteStatus},         // Opcode 7: Write Status Register
    },
    //
    // The offset of the start of the BIOS image in flash. This value is platform specific
    // and depends on the system flash map. If BIOS size is bigger then flash return -1
    //
    // ((WINBOND_W25Q64B_SIZE >= BIOS_FLASH_SIZE) ? WINBOND_W25Q64B_SIZE - BIOS_FLASH_SIZE : (UINTN) (-1)),
    0x0,
    //
    // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map
    //
    // BIOS_FLASH_SIZE
    0x0
};

void InitSpiTable (
  IN OUT SPI_INIT_TABLE         **pInitTable,
  IN DEVICE_TABLE               *pDeviceTable,
  IN SPI_OPCODE_MENU_ENTRY      *pOpcodeMenuList,
  IN UINT8                      *PrefixOpcodeList
  )
{
    INTN i;

    *pInitTable = &mInitTable;
    mInitTable.VendorId = pDeviceTable->VendorId;
    mInitTable.DeviceId0 = pDeviceTable->DeviceId0;
    mInitTable.DeviceId1 = pDeviceTable->DeviceId1;

    for (i=0; i < SPI_NUM_OPCODE; i++) {
      mInitTable.OpcodeMenu[i].Type = pOpcodeMenuList[i].Type;
      mInitTable.OpcodeMenu[i].Code = pOpcodeMenuList[i].Code;
      mInitTable.OpcodeMenu[i].Frequency = pOpcodeMenuList[i].Frequency;
      mInitTable.OpcodeMenu[i].Operation = pOpcodeMenuList[i].Operation;
    }

    for (i = 0; i < SPI_NUM_PREFIX_OPCODE; i++) {
      mInitTable.PrefixOpcode[i] = PrefixOpcodeList[i];
    }

    mInitTable.BiosStartOffset = pDeviceTable->Size - PcdGet32(PcdFlashAreaSize); // this will be set by Fixed PCD value.
    mInitTable.BiosSize = PcdGet32(PcdFlashAreaSize);                             // this will be set by Fixed PCD value.
}

EFI_STATUS
EnableBlockProtectionForSST25 (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN  BOOLEAN ProtectionEnable
)
{
    EFI_STATUS          Status;
    UINT8               SpiData;
    UINT8               SpiStatus;
    NV_DEVICE_INSTANCE  *DeviceInstance;
    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    SpiStatus=0x01;
    if (ProtectionEnable) {
        SpiData = SF_SR_WPE;
    } else {
        SpiData = 0;
    }
    SpiData = 0;  
 	Status = DeviceInstance->SpiProtocol->Execute (
                 DeviceInstance->SpiProtocol,
                 SPI_RDSR,
                 SPI_WREN,
                 TRUE,
                 FALSE,
                 FALSE,
                 0,
                 1,
                 &SpiStatus,
                 EnumSpiRegionBios
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }
	DEBUG((EFI_D_ERROR,"HX debug:flash block protection Status before unlock:%X\n",SpiStatus));
	
    //
    // Always disable block protection to workaround tool issue.
    // Feature may be re-enabled in a future bios.
    //
    
    Status = DeviceInstance->SpiProtocol->Execute (
                 DeviceInstance->SpiProtocol,
                 SPI_WRSR,
                 SPI_EWSR,
                 TRUE,
                 TRUE,
                 TRUE,
                 0,
                 1,
                 &SpiData,
                 EnumSpiRegionAll
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }
	

    Status = DeviceInstance->SpiProtocol->Execute (
                 DeviceInstance->SpiProtocol,
                 SPI_RDSR,
                 SPI_WREN,
                 TRUE,
                 FALSE,
                 FALSE,
                 0,
                 1,
                 &SpiStatus,
                 EnumSpiRegionBios
             );
    if (EFI_ERROR (Status)) {
        return Status;
    }

	DEBUG((EFI_D_ERROR,"HX debug:flash block protection Status after unlock:%X\n",SpiStatus));
    if ((SpiStatus & SpiData) != SpiData) {
        Status = EFI_DEVICE_ERROR;
    }

    return Status;
   
}

EFI_STATUS
EnableBlockProtectionForSST26 (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN  BOOLEAN ProtectionEnable
)
{
	EFI_STATUS          Status;
    NV_DEVICE_INSTANCE  *DeviceInstance;
	UINT8               SpiStatus;
    
    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);
    
      //
    // Always disable block protection to workaround tool issue.
    // Feature may be re-enabled in a future bios.
    //
   
    if (ProtectionEnable) {
		return EFI_SUCCESS;
    } else {
    
    	
    	Status = DeviceInstance->SpiProtocol->Execute (
	                 DeviceInstance->SpiProtocol,
	                 SPI_ULBPR,
	                 SPI_WREN,
	                 FALSE,
	                 TRUE,      //Atomic cycle:execute WPEN before globally unlock
	                 FALSE,
	                 0,
	                 0,
	                 NULL,
	                 EnumSpiRegionAll
	             );
    	if (EFI_ERROR (Status)) {
        	return Status;
    	}
		
		Status = DeviceInstance->SpiProtocol->Execute(
							DeviceInstance->SpiProtocol,
							SPI_RBPR,
							SPI_WREN,
							TRUE,
							FALSE,
							FALSE,
							0,
							2,
							&SpiStatus,
							EnumSpiRegionAll
						);
		if(EFI_ERROR(Status)){
			return Status;
		}		
		DEBUG((EFI_D_ERROR,"HX debug:flash block protection Status before unlock:%X\n",SpiStatus));
    }
	return Status;
}


EFI_STATUS
EnableBlockProtection (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN  BOOLEAN ProtectionEnable
)
{
	EFI_STATUS          Status;
    NV_DEVICE_INSTANCE  *DeviceInstance;
	UINT8		  FlashPartId[3];

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    Status = DeviceInstance->SpiProtocol->Execute(
    				DeviceInstance->SpiProtocol,
    				SPI_READ_ID,
    				SPI_WREN,
    				TRUE,
    				FALSE,     // 20161010-HX:No Atomic cycle
    				FALSE,     // 20161010-HX:read
    				0,
    				3,
    				FlashPartId,
    				EnumSpiRegionAll
    				);
	if(EFI_ERROR(Status)){
		return Status;
	}
	
	DEBUG((EFI_D_INFO, "HX debug-DeviceSense ID:%02X%02X%02X\n", FlashPartId[0], FlashPartId[1], FlashPartId[2]));

	if(FlashPartId[0]==0xBF)
	{
		if(FlashPartId[1]==0x25)
		{
			DEBUG((EFI_D_INFO, "HX debug- firstly Disable flash block protection\n"));
			Status = EnableBlockProtectionForSST25 (this, ProtectionEnable);
   			 if (EFI_ERROR (Status)) {
       			 return EFI_ACCESS_DENIED;
   			 }
		}else if(FlashPartId[1]==0x26){
			DEBUG((EFI_D_INFO, "HX debug- firstly Disable flash block protection for SST26\n"));
			Status = EnableBlockProtectionForSST26 (this, ProtectionEnable);
   			if (EFI_ERROR (Status)) {
       			 return EFI_ACCESS_DENIED;
   		 	}
		}else{
			DEBUG((EFI_D_INFO, "HX debug- unknown flash decide\n"));
			return EFI_UNSUPPORTED;
		}
	}
	return Status;
}

EFI_STATUS
device_info (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN OUT MEDIA_BLOCK_MAP**           MapInfo
)
{
    NV_DEVICE_INSTANCE  *DeviceInstance;

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);
    *MapInfo = DeviceInstance->BlockMap;
    return EFI_SUCCESS;
}

EFI_STATUS
device_sense (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this
)
{
    EFI_STATUS          Status;
    NV_DEVICE_INSTANCE  *DeviceInstance;

    DEBUG((EFI_D_ERROR,"================device sense=================\n"));
    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    if (DeviceInstance->SpiProtocol == NULL)
        return EFI_UNSUPPORTED;

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Enable(DeviceInstance->PlatformAccessProtocol);

// SpiProtocol->Init() will check flash ID by itself.
    Status = DeviceInstance->SpiProtocol->Init (DeviceInstance->SpiProtocol, DeviceInstance->InitTable);
	if(EFI_ERROR(Status)){
		return EFI_UNSUPPORTED;
	}
//
// 20161010_HX_debug disable flash itself block protection once sense the flash device
//
    Status=EnableBlockProtection(this,FALSE);
	if(EFI_ERROR(Status)){
		return EFI_ACCESS_DENIED;
	}

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Disable(DeviceInstance->PlatformAccessProtocol);
    DEBUG((EFI_D_ERROR,"================device sense=================\n"));
    return Status;
}

EFI_STATUS
devcie_read (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN OUT UINT8* Buffer,
    IN OUT UINTN* Length
)
{
    EFI_STATUS                Status;
    UINTN                     LbaAddress;
    UINTN                     SpiAddress;

    NV_DEVICE_INSTANCE  *DeviceInstance;

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    LbaAddress = (UINTN)(0xFFFFFFFF) - (DeviceInstance->FlashSize * DeviceInstance->Number) + 1;

    if ((Address < LbaAddress) ||
            ((Address + *Length - 1) > (UINTN)(0xFFFFFFFF)))
        return EFI_INVALID_PARAMETER;

    if (DeviceInstance->SpiProtocol == NULL)
        return EFI_UNSUPPORTED;

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Enable(DeviceInstance->PlatformAccessProtocol);

//
//   20161011 hongxu: need no enable or disable block protection action for read cycle
//
/*
    Status = EnableBlockProtection (this, FALSE);
    if (EFI_ERROR (Status)) {
        return EFI_ACCESS_DENIED;
    }
*/
    //kirby: EfiCpuFlushCache (Address, *Length);

    //
    // Convert memory mapped address to SPI address
    //
    SpiAddress = Address - LbaAddress;

    WriteBackInvalidateDataCacheRange ((VOID *) Address, *Length);

    Status = DeviceInstance->SpiProtocol->Execute(
                 DeviceInstance->SpiProtocol,
                 SPI_READ,
                 SPI_WREN,
                 TRUE,
                 FALSE,
                 FALSE,
                 (UINT32) SpiAddress,
                 (UINT32) *Length,
                 Buffer,
                 EnumSpiRegionAll
             );

	//
	//	 20161011 hongxu: need no enable or disable block protection action for read cycle
	//

    //  EnableBlockProtection (this, TRUE);

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Disable(DeviceInstance->PlatformAccessProtocol);

    //
    // Flush the changed area to make the cache consistent
    //
    //kirby: EfiCpuFlushCache (Address, *Length);

    //
    // Flush cache after any write or erase
    // We can remove if EfiCpuFlushCache is implemented to do something.

    return Status;
}

EFI_STATUS
device_write (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINT8* Buffer,
    IN UINTN Length
)
{
    EFI_STATUS                Status;
    UINTN                     LbaAddress;
    UINTN                     SpiAddress;

    NV_DEVICE_INSTANCE  *DeviceInstance;
    DEBUG((EFI_D_ERROR,"================SPI write cycle Go=================\n"));

    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    LbaAddress = (UINTN)(0xFFFFFFFF) - (DeviceInstance->FlashSize * DeviceInstance->Number) + 1;
    
    if ((Address < LbaAddress) ||
            ((Address + Length - 1) > (UINTN)(0xFFFFFFFF)))
        return EFI_INVALID_PARAMETER;

    if (DeviceInstance->SpiProtocol == NULL)
        return EFI_UNSUPPORTED;

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Enable(DeviceInstance->PlatformAccessProtocol);

    //kirby: EfiCpuFlushCache (Address, Length);

    //
    // Convert memory mapped address to SPI address
    //
    SpiAddress = Address - LbaAddress;

    Status = DeviceInstance->SpiProtocol->Execute(
                 DeviceInstance->SpiProtocol,
                 SPI_PROG,
                 SPI_WREN,
                 TRUE,
                 TRUE,
                 TRUE,
                 (UINT32) SpiAddress,
                 (UINT32) Length,
                 Buffer,
                 EnumSpiRegionAll
             );
	//
	// Hongxu comment: after Add removing Opcode Pointer operation in
	// SpiExec to avoid block generation, EnableBlockProtection can be killed 
/*
    Status=EnableBlockProtection (this, TRUE);

	 if (EFI_ERROR (Status)) {
        return EFI_ACCESS_DENIED;
    }
*/
    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Disable(DeviceInstance->PlatformAccessProtocol);

    //
    // Flush the changed area to make the cache consistent
    //
    //kirby: EfiCpuFlushCache (Address, Length);

    //
    // Flush cache after any write or erase
    // We can remove if EfiCpuFlushCache is implemented to do something.
    //
    //TBD kirby: EfiWbinvd ();
    WriteBackInvalidateDataCacheRange ((VOID *) Address, Length);
    
    DEBUG((EFI_D_ERROR,"================SPI write cycle done=================\n"));
    return Status;
}
EFI_STATUS
device_erase (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINTN Length
)
{
    EFI_STATUS  Status;
    UINTN       LbaAddress;
    UINTN       BlockCount;
    UINTN       SpiAddress;
    UINT8       OpcodeIndex;

   
    NV_DEVICE_INSTANCE  *DeviceInstance;
	
	Status =EFI_SUCCESS;
    DeviceInstance = DEVICE_INSTANCE_FROM_DEVICEPROTOCOL (this);

    LbaAddress = (UINTN)(0xFFFFFFFF) - (DeviceInstance->FlashSize * DeviceInstance->Number) + 1;

    if ((Address < LbaAddress) ||
            ((Address + Length - 1) > (UINTN)(0xFFFFFFFF)))
        return EFI_INVALID_PARAMETER;

    if ((Length % DeviceInstance->SectorSize) != 0) {
        return EFI_UNSUPPORTED;
    }

    if (DeviceInstance->SpiProtocol == NULL)
        return EFI_UNSUPPORTED;

    if (DeviceInstance->PlatformAccessProtocol)
        DeviceInstance->PlatformAccessProtocol->Enable(DeviceInstance->PlatformAccessProtocol);

    //
    // Convert memory mapped address to SPI address
    //
    SpiAddress = (UINT32) (Address - LbaAddress);
    BlockCount = Length / DeviceInstance->SectorSize;

    if(DeviceInstance->SectorSize == SIZE_64KB){
      OpcodeIndex = SPI_BERASE;
    } else if(DeviceInstance->SectorSize == SIZE_4KB){
      OpcodeIndex = SPI_SERASE;
    } else {
      return EFI_UNSUPPORTED;
    }
    
    while (BlockCount > 0) {
        Status = DeviceInstance->SpiProtocol->Execute(
                     DeviceInstance->SpiProtocol,
                     OpcodeIndex,
                     SPI_WREN,
                     FALSE,
                     TRUE,
                     FALSE,
                     (UINT32) SpiAddress,
                     0,
                     NULL,
                     EnumSpiRegionAll
                 );
        if (EFI_ERROR(Status)) {
            break;
        }
        SpiAddress += DeviceInstance->SectorSize;
        BlockCount--;
    }

//    EnableBlockProtection (this, TRUE);

    if (DeviceInstance->PlatformAccessProtocol){
        DeviceInstance->PlatformAccessProtocol->Disable(DeviceInstance->PlatformAccessProtocol);
    }
    
    //
    // Flush cache after any write or erase
    // We can remove if EfiCpuFlushCache is implemented to do something.
    //
    WriteBackInvalidateDataCacheRange ((VOID *) Address, Length);

    return Status;
}

EFI_STATUS
device_lock (
    IN CONST NV_MEDIA_DEVICE_PROTOCOL* this,
    IN UINTN Address,
    IN UINTN Length
)
{
    return EFI_UNSUPPORTED;
}
