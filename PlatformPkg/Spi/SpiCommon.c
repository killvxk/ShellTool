
#include "SpiCommon.h"
#include <Library/S3BootScriptLib.h>


#define SPI_PRIVATE_DATA_SIGNATURE  SIGNATURE_32('S', 'P', 'I', 'P')
#define SPI_INSTANCE_FROM_SPIPROTOCOL(a)  CR(a, SPI_INSTANCE, SpiProtocol, SPI_PRIVATE_DATA_SIGNATURE)

#define WAIT_TIME             6000000
#define WAIT_PERIOD           10
#define SPI_FLA_MASK          0x01FFFFFF

#define FLASH_CYCLE_MASK      (3 << 29)
#define FLASH_CYCLE_IN        (0 << 29)
#define FLASH_CYCLE_OUT       (1 << 29)
#define FLASH_CYCLE_OUT_IN    (2 << 29)

EFI_STATUS
EFIAPI
EnableSpiflashWrites (
  IN  BOOLEAN   EnableWrites
)
/*++

Arguments:

  EnableWrites       - Boolean to enable/disable flash

Returns:
  EFI_SUCCESS

--*/
{
  UINT8                                 Buffer8;

  Buffer8 =MmioRead8(LPC_PCI_REG(LPC_ISA_BUS_CTRL_REG));
  
  if (EnableWrites) {
    Buffer8 = (UINT8) (Buffer8 | ROM_WRITE_EN);
  } else {
    Buffer8 = (UINT8) (Buffer8 & (~ROM_WRITE_EN));
  }
  MmioWrite8(LPC_PCI_REG(LPC_ISA_BUS_CTRL_REG),Buffer8);              
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
SpiExec (
    IN     EFI_SPI_PROTOCOL   *This,
    IN     UINT8              OpcodeIndex,
    IN     UINT8              PrefixOpcodeIndex,
    IN     BOOLEAN            DataCycle,
    IN     BOOLEAN            Atomic,
    IN     BOOLEAN            ShiftOut,
    IN     UINTN              Address,
    IN     UINT32             DataByteCount,
    IN OUT UINT8              *Buffer,
    IN     SPI_REGION_TYPE    SpiRegionType
  );



//------------------------------------------------------------------------------
EFI_STATUS
SaveSpiRegForS3 (
  IN UINT32  SpiBar
  )
{
  UINTN       Index;
  UINTN       Count;
  EFI_STATUS  Status;
  STATIC CONST UINT32 gSpiRegister[] = {
    SPI0_PREOP_REG,
    SPI0_OPMENU_REG,
    SPI0_OPMENU_REG+4
  };

  Status = EFI_SUCCESS;
  Count = sizeof(gSpiRegister)/sizeof(gSpiRegister[0]);
  for(Index=0;Index<Count;Index++){
    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint32, 
               SpiBar + gSpiRegister[Index],
               1,
               (VOID*)(UINTN)(SpiBar + gSpiRegister[Index])
               );
    ASSERT_EFI_ERROR(Status);
  }

  return Status;
}  



EFI_STATUS
EFIAPI
SpiInit (
  IN EFI_SPI_PROTOCOL     *This,
  IN SPI_INIT_TABLE       *InitTable
  )
{
  EFI_STATUS    Status;
  SPI_INSTANCE  *SpiInstance;
  UINT32        SpiBar;
  UINT8         Index;
  UINT16        OpcodeType;
  UINT8         FlashPartId[3];
  

  SpiInstance = SPI_INSTANCE_FROM_SPIPROTOCOL(This);
  SpiBar      = SpiInstance->SpiBar;
  
  if (InitTable != NULL) {
    CopyMem(&SpiInstance->SpiInitTable, InitTable, sizeof(SPI_INIT_TABLE));
  } else {
    return EFI_INVALID_PARAMETER;
  }  
  
  if ((MmioRead16(SpiBar + SPI0_SPIS_REG) & SPI0_SPIS_LOCKDOWN) != 0) {
    ASSERT_EFI_ERROR(EFI_ACCESS_DENIED);
    return EFI_ACCESS_DENIED;
  }  
  
  MmioOr16 (SpiBar + SPI0_SPIS_REG, SPI0_SPIS_CYCLE_DONE|SPI0_SPIS_BLKACC);
  MmioWrite16(SpiBar + SPI0_PREOP_REG, (SpiInstance->SpiInitTable.PrefixOpcode[1] << 8) | InitTable->PrefixOpcode[0]);

  for (Index = 0, OpcodeType = 0; Index < SPI_NUM_OPCODE; Index++) {
    switch (SpiInstance->SpiInitTable.OpcodeMenu[Index].Type) {
    case EnumSpiOpcodeRead:
      OpcodeType |= (UINT16)(SPI0_OPTYPE_RDADDR << (Index * 2));
      break;
    case EnumSpiOpcodeWrite:
      OpcodeType |= (UINT16)(SPI0_OPTYPE_WRADDR << (Index * 2));
      break;
    case EnumSpiOpcodeWriteNoAddr:
      OpcodeType |= (UINT16)(SPI0_OPTYPE_WRNOADDR << (Index * 2));
      break;
    default:
      OpcodeType |= (UINT16)(SPI0_OPTYPE_RDNOADDR << (Index * 2));
      break;
    }
  }    
  MmioWrite16(SpiBar + SPI0_OPTYPE_REG, OpcodeType);
  
  for (Index = 0; Index < SPI_NUM_OPCODE; Index++) {
    MmioWrite8(SpiBar + SPI0_OPMENU_REG + Index, SpiInstance->SpiInitTable.OpcodeMenu[Index].Code);
    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationJedecId) {
      Status = SpiExec (
                This,
                Index,
                0,
                TRUE,
                FALSE,//DLA:not Atomic
                FALSE,
                (UINTN) 0,
                3,
                FlashPartId,
                EnumSpiRegionAll
                );
      if (EFI_ERROR (Status)) {
        return Status;
      }
      DEBUG((EFI_D_INFO, "ID:%02X%02X%02X\n", FlashPartId[0], FlashPartId[1], FlashPartId[2]));
      if (FlashPartId[0] != SpiInstance->SpiInitTable.VendorId  ||
      	  FlashPartId[1] != SpiInstance->SpiInitTable.DeviceId0 ||
      	  FlashPartId[2] != SpiInstance->SpiInitTable.DeviceId1) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }
  
  Status = SaveSpiRegForS3(SpiBar);
  ASSERT_EFI_ERROR(Status);  
  
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
SpiLock (
  IN EFI_SPI_PROTOCOL  *This
  )
{
  SPI_INSTANCE  *SpiInstance;
  UINT32        SpiBar;

  SpiInstance = SPI_INSTANCE_FROM_SPIPROTOCOL(This);
  SpiBar      = SpiInstance->SpiBar;

  MmioOr16(SpiBar + SPI0_SPIS_REG, SPI0_SPIS_LOCKDOWN);

  if ((MmioRead16(SpiBar + SPI0_SPIS_REG) & SPI0_SPIS_LOCKDOWN) == 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}






BOOLEAN
WaitForSpiCycleDone (
  IN     EFI_SPI_PROTOCOL   *This
  )
{
  UINT64        WaitTicks;
  UINT64        WaitCount;
  UINT16        Data16;
  SPI_INSTANCE  *SpiInstance;
  UINT32        SpiBar;
  SpiInstance = SPI_INSTANCE_FROM_SPIPROTOCOL(This);
  SpiBar      = SpiInstance->SpiBar;  
  WaitCount   = WAIT_TIME / WAIT_PERIOD;
  for (WaitTicks = 0; WaitTicks < WaitCount; WaitTicks++) {
    Data16 = MmioRead16(SpiBar + SPI0_SPIS_REG);
	
    if(((Data16 & SPI0_SPIS_CYCLE_PROGRESS) == 0) &&((Data16 & SPI0_SPIS_CYCLE_DONE)== SPI0_SPIS_CYCLE_DONE)){
      MmioOr16(SpiBar + SPI0_SPIS_REG, SPI0_SPIS_CYCLE_DONE);
      return TRUE;
    }
    MicroSecondDelay(WAIT_PERIOD);
  }

  return FALSE;
}



BOOLEAN
WaitForSpiCycleComplete (
  IN     EFI_SPI_PROTOCOL   *This
  )
{
  UINT64        WaitTicks;
  UINT64        WaitCount;
  UINT16        Data16;
  SPI_INSTANCE  *SpiInstance;
  UINT32        SpiBar;

  SpiInstance = SPI_INSTANCE_FROM_SPIPROTOCOL(This);
  SpiBar      = SpiInstance->SpiBar;  
  WaitCount   = WAIT_TIME / WAIT_PERIOD;
  
  for (WaitTicks = 0; WaitTicks < WaitCount; WaitTicks++) {
    Data16 = MmioRead16(SpiBar + SPI0_SPIS_REG);
    if((Data16 & SPI0_SPIS_CYCLE_PROGRESS) == 0) {
      MmioOr16(SpiBar + SPI0_SPIS_REG, SPI0_SPIS_CYCLE_DONE|SPI0_SPIS_BLKACC);
      return TRUE;
    }
    MicroSecondDelay(WAIT_PERIOD);
  }

  return FALSE;
}






EFI_STATUS
EFIAPI
SpiExec (
    IN     EFI_SPI_PROTOCOL   *This,
    IN     UINT8              OpcodeIndex,
    IN     UINT8              PrefixOpcodeIndex,
    IN     BOOLEAN            DataCycle,
    IN     BOOLEAN            Atomic,
    IN     BOOLEAN            ShiftOut,
    IN     UINTN              Address,
    IN     UINT32             DataByteCount,
    IN OUT UINT8              *Buffer,
    IN     SPI_REGION_TYPE    SpiRegionType			// Do not care SpiRegionType
  )
{
  EFI_STATUS     Status;
  SPI_INSTANCE   *SpiInstance;
  UINT32         SpiBar;
  UINT32         Index;
  UINTN          SpiBiosSize;
  UINT32         SpiDataCount;
  UINT8          OpCode;
  SPI_OPERATION  Operation;

 /*
  DEBUG((EFI_D_INFO, "%a(I:%X,%X,A:%X,C:%X,B:%X)\n", 
                       __FUNCTION__, 
                       OpcodeIndex, 
                       PrefixOpcodeIndex,
                       Address,
                       DataByteCount,
                       Buffer
                       ));  
 */
  EnableSpiflashWrites (TRUE);
  Status = EFI_SUCCESS;
  if ((OpcodeIndex >= SPI_NUM_OPCODE) || (PrefixOpcodeIndex >= SPI_NUM_PREFIX_OPCODE)) {
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }
  
  if (!WaitForSpiCycleComplete(This)) {
    Status = EFI_DEVICE_ERROR;
    goto ProcExit;    
  }  
  
  SpiInstance = SPI_INSTANCE_FROM_SPIPROTOCOL(This);
  SpiBar      = SpiInstance->SpiBar;    
  SpiBiosSize = SpiInstance->SpiInitTable.BiosSize;
  Operation   = SpiInstance->SpiInitTable.OpcodeMenu[OpcodeIndex].Operation;
  OpCode      = MmioRead8(SpiBar + SPI0_OPMENU_REG + OpcodeIndex);

  if (OpCode == 0 || SpiBiosSize == 0) {
    ASSERT (FALSE);
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }  

if (SpiInstance->SpiInitTable.OpcodeMenu[OpcodeIndex].Type == EnumSpiOpcodeRead) {
  
  do{ 
	  MmioOr16(SpiBar + SPI0_SPIS_REG, SPI0_SPIS_CYCLE_DONE|SPI0_SPIS_BLKACC);
    CopyMem (
      Buffer,
      (UINT8*)(Address + (UINT32)(~(SpiBiosSize - 1))),
      DataByteCount
      );
	
	}while(((MmioRead8(SpiBar + SPI0_SPIS_REG ))&SPI0_SPIS_BLKACC)==SPI0_SPIS_BLKACC);
 
    Status = EFI_SUCCESS;
    goto ProcExit;
  }  
  
  if ((DataCycle == FALSE) && (DataByteCount > 0)) {
    DataByteCount = 0;
  }  

  do {
    if (Address + DataByteCount > ((Address + BIT8) &~(BIT8 - 1))) {
      SpiDataCount = (((UINT32) (Address) + BIT8) &~(BIT8 - 1)) - (UINT32) (Address);
    } else {
      SpiDataCount = DataByteCount;
    }  

    if (Operation == EnumSpiOperationProgramData_1_Byte) {
      SpiDataCount = 1;
    } else if (SpiDataCount >= 16) {
      SpiDataCount = 16;
    }  
    
    if (ShiftOut) {
      for (Index = 0; Index < SpiDataCount; Index++) {
        MmioWrite8(SpiBar + SPI0_SPID0_REG + Index, Buffer[Index]);
      }
    }
    MmioWrite32 (SpiBar + SPI0_SPIA_REG, (UINT32)(Address & SPI_FLA_MASK));
do{	
    MmioOr16(SpiBar + SPI0_SPIS_REG, SPI0_SPIS_CYCLE_DONE|SPI0_SPIS_BLKACC);

//  DEBUG((EFI_D_INFO, "(%d)%d %d\n", __LINE__, SpiDataCount, DataByteCount));
		
    MmioWrite16(SpiBar + SPI0_SPIC_REG, 
               (UINT16)(SPI0_SPIC_GO|
               ((Atomic&BIT0)<<2)|
               ((PrefixOpcodeIndex&BIT0)<<3)|
               ((OpcodeIndex&7)<<4)|
               (DataCycle?((((SpiDataCount - 1)&0xF) << 8)|BIT14):0))
               );
    if (!WaitForSpiCycleDone(This)) {
      Status = EFI_DEVICE_ERROR;
      goto ProcExit;
    }
   }while(((MmioRead8(SpiBar + SPI0_SPIS_REG ))&SPI0_SPIS_BLKACC)==SPI0_SPIS_BLKACC);
    
    if (!ShiftOut && Buffer!=NULL) {
      for (Index = 0; Index < SpiDataCount; Index++) {
        Buffer[Index] = MmioRead8(SpiBar + SPI0_SPID0_REG + Index);
      }
    }    

    Address += SpiDataCount;
    Buffer  += SpiDataCount;
    DataByteCount -= SpiDataCount;    
    
  }while (DataByteCount > 0);
  
ProcExit:  
//  DEBUG((EFI_D_INFO, "%r\n", Status));
  //
  // Hongxu-20161012
  // add remove Opcode Pointer to read status register to avoid block generation
  
 MmioWrite8(0xFED10000 + SPI0_SPIC_REG,0x20);

 EnableSpiflashWrites (FALSE); 

  return Status;
}



SPI_INSTANCE  gSpiInstance = {
  SPI_PRIVATE_DATA_SIGNATURE,
  NULL,
  {
    SpiInit,
    SpiLock,
    SpiExec
  },
  {0}
};


VOID
SpiVirtualddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  gRT->ConvertPointer (0, (VOID**)&(gSpiInstance.SpiBar));
  gRT->ConvertPointer (0, (VOID**)&(gSpiInstance.SpiProtocol.Init));
  gRT->ConvertPointer (0, (VOID**)&(gSpiInstance.SpiProtocol.Lock));
  gRT->ConvertPointer (0, (VOID**)&(gSpiInstance.SpiProtocol.Execute));
  gRT->ConvertPointer (0, (VOID**)&(gSpiInstance));
}



