

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/TimerLib.h>
#include <Ppi/Smbus.h>
#include <PlatformDefinition.h>
#include <CHX001Reg.h>



//----------------------------------------------------------------------;
//  System Management Bus I/O Space Equates                             ;
//----------------------------------------------------------------------;

#define HSTSTS_ALL              0xDF// R/WC
#define SMBCMD_QUICK            0x00
#define SMBCMD_BYTE             0x04
#define SMBCMD_BYTE_DATA        0x08
#define SMBCMD_WORD_DATA        0x0C
#define SMBCMD_PROCESS_CALL     0x10
#define SMBCMD_BLOCK            0x14
#define SMBCMD_IIC_10BIT        0x18
#define SMBCMD_IIC_PCALL        0x30
#define SMBCMD_IIC_BLOCK        0x34
#define SMBCMD_IIC_7BIT         0x38



BOOLEAN
IoDone (
    IN CONST EFI_PEI_SERVICES       **PeiServices,
    OUT UINT8                 *SmbusStatus
)
{
	UINT16		SmbusBase;
	UINT8		Data8;
	UINTN		Index;
	UINTN		Count;

	SmbusBase = SMBUS_BASE_ADDRESS;

	Count = 1000000/50;
	Data8 = 0xFF;
	Index = 0;

	while ((Index < Count)) {
		*SmbusStatus = IoRead8(SmbusBase+SMIO_SMBUS_HOST_STA);
		if ( (!(*SmbusStatus & (SMIO_HOST_BUSY))) || (*SmbusStatus & (SMIO_FAILED | SMIO_BUS_ERR | SMIO_DEV_ERR))) {
			return TRUE;
		} else {
			MicroSecondDelay(50);
			Index++;
		}
	}
  return FALSE;
}
// Now only designed for ByteRead and ByteWrite
EFI_STATUS
EFIAPI
SmbusExec (
  IN  CONST EFI_PEI_SERVICES          **PeiServices,
  IN      EFI_PEI_SMBUS_PPI         *This,
  IN      EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN      EFI_SMBUS_DEVICE_COMMAND  Command,
  IN      EFI_SMBUS_OPERATION       Operation,
  IN      BOOLEAN                   PecCheck,
  IN OUT  UINTN                     *Length,
  IN OUT  VOID                      *Buffer
  )
{
UINT16      SmbusBase;
EFI_STATUS								Status;
UINT8									SmbusCommandReg;
UINT8									SlaveAddressReg;
UINT8									HostCommandReg;
UINT8									*DataBuf;
UINT8									SmbusSts;
UINT8									BlockCount;
BOOLEAN 								SmallBuffer;
UINTN									Index;
UINTN									RetryCnt;
UINT8		Data8;
UINTN		Count;

Status = EFI_SUCCESS;

SmbusBase = SMBUS_BASE_ADDRESS;

DataBuf = Buffer;
SmbusSts = 0;
BlockCount = 0;

//
// For any operations besides quick read & write, the pointers to
// Length and Buffer must not be NULL.
//
if (!((Operation == EfiSmbusQuickRead) || (Operation == EfiSmbusQuickWrite)))
{
	if ((Length == NULL) || (Buffer == NULL))
	{
		return EFI_INVALID_PARAMETER;
	}
}

//
// Reset before next execute.
//
IoWrite8(SmbusBase+SMIO_SMBUS_HOST_STA, 0x20); 

Count = 1000/50;
Data8 = 0xFF;
Index = 0;

while ((Index < Count)) {
	MicroSecondDelay(1);
	Index++;
}

IoWrite8(SmbusBase+SMIO_SMBUS_HOST_STA, 0x00); 

for (RetryCnt = 0; RetryCnt < 3; RetryCnt++)
{
	Status = EFI_SUCCESS;
	SmbusCommandReg = SMBCMD_QUICK;
	SlaveAddressReg = (UINT8) ((SlaveAddress.SmbusDeviceAddress << 1) | 1);///default Execute as a READ command 
	HostCommandReg = (UINT8) Command;

	switch (Operation)
	{

	case EfiSmbusQuickWrite:
		SlaveAddressReg--;
		break;

		//
		// The "break;" command is not present here to allow code execution
		// do drop into the next case, which contains common code to this case.
		//
	case EfiSmbusQuickRead:
		if (PecCheck == TRUE)
		{
			Status = EFI_UNSUPPORTED;
		}
		break;

	case EfiSmbusSendByte:
		SmbusCommandReg = SMBCMD_BYTE;
		HostCommandReg = DataBuf[0];
		SlaveAddressReg--;
		break;

		//
		// The "break;" command is not present here to allow code execution
		// do drop into the next case, which contains common code to this case.
		//
	case EfiSmbusReceiveByte:
		SmbusCommandReg = SMBCMD_BYTE;
		if (*Length < 1)
		{
			Status = EFI_BUFFER_TOO_SMALL;
		}
		*Length = 1;
		SlaveAddressReg |= BIT0;// 1:Execute a READ command 
		break;

	case EfiSmbusWriteByte:		
		SmbusCommandReg = SMBCMD_BYTE_DATA;
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_DATA_0, DataBuf[0]); //SMIO Rx05
		SlaveAddressReg--;
		break;

		//
		// The "break;" command is not present here to allow code execution
		// do drop into the next case, which contains common code to this case.
		//
	case EfiSmbusReadByte:
		SmbusCommandReg = SMBCMD_BYTE_DATA;
		if (*Length < 1)
		{
			Status = EFI_BUFFER_TOO_SMALL;
		}
		*Length = 1;		
		SlaveAddressReg |= BIT0;// 1:Execute a READ command 
		break;

	case EfiSmbusReadWord:
		SmbusCommandReg = SMBCMD_WORD_DATA;
		if (*Length < 2)
		{
			Status = EFI_BUFFER_TOO_SMALL;
		}
		*Length = 2;
		break;

	case EfiSmbusWriteWord:
		SmbusCommandReg = SMBCMD_WORD_DATA;
		SlaveAddressReg--;
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_DATA_1, DataBuf[1]); //SMIO Rx06
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_DATA_0, DataBuf[0]); //SMIO Rx05
		if (*Length < 2)
		{
			Status = EFI_BUFFER_TOO_SMALL;
		}
		*Length = 2;
		break;

	case EfiSmbusWriteBlock:
		SlaveAddressReg--;
		BlockCount = (UINT8) (*Length);
		SmbusCommandReg = SMBCMD_BLOCK;
		break;

	case EfiSmbusReadBlock:
		SmbusCommandReg = SMBCMD_BLOCK;
		if ((*Length < 1) || (*Length > 32))
		{
			Status = EFI_INVALID_PARAMETER;
			break;
		}
		break;

	case EfiSmbusProcessCall:
		SmbusCommandReg = SMBCMD_PROCESS_CALL;
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_DATA_1, DataBuf[1]); //SMIO Rx06
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_DATA_0, DataBuf[0]); //SMIO Rx05
		if (*Length < 2)
		{
			Status = EFI_BUFFER_TOO_SMALL;
		}
		*Length = 2;
		break;

	case EfiSmbusBWBRProcessCall:
		Status = EFI_UNSUPPORTED;
		break;

	default:
		Status = EFI_INVALID_PARAMETER;
		break;
	}

	///DEBUG((DEBUG_INFO, "[Smbus]+++ checkpoint 1\n"));  
	if (EFI_ERROR(Status))
	{
		break;
	}

	if (PecCheck)///False
	{
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_CTL, SMIO_HT_PECEN); //SMIO Rx02
	}
 
	if (Operation == EfiSmbusWriteBlock)
	{
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_STA, 0xDE);			  //SMIO Rx00
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_ADR, SlaveAddressReg);   //SMIO Rx04
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_CMD, HostCommandReg);	  //SMIO Rx03
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_DATA_0, *(UINT8 *) Length); //SMIO Rx05

		for (Index = 0; Index < BlockCount; Index++)
		{
			IoWrite8(SmbusBase+ SMIO_SMBUS_BLOCK_DATA, DataBuf[Index]);
			if (!(IoDone (PeiServices, &SmbusSts))) {
			  Status = EFI_TIMEOUT;
			  break;
			} else if (SmbusSts & SMIO_DEV_ERR) {
			  Status = EFI_DEVICE_ERROR;
			  break;
			} else if (SmbusSts & SMIO_BUS_ERR) {
			  Status = EFI_DEVICE_ERROR;
			  IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_STA, 0xDE);
			  continue;
			}
		}
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_CTL, (UINT8) (SmbusCommandReg|SMIO_START)); //SMIO Rx02
		
	} else {

		IoRead8(SmbusBase+ SMIO_SMBUS_HOST_CTL);		 //SMIO Rx02
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_STA, 0xDE); //SMIO Rx00
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_ADR, SlaveAddressReg); //SMIO Rx04
		
		if (Operation != EfiSmbusReceiveByte)
		{
			IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_CMD, HostCommandReg);	//SMIO Rx03
		}
		
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_CTL, (UINT8) (SmbusCommandReg | SMIO_START));  //SMIO Rx02
	}
	
	//
	// Wait SMBus process complete
	//
	SmbusSts = IoRead8(SmbusBase+ SMIO_SMBUS_HOST_STA);  //SMIO Rx00
	while ((SmbusSts & SMIO_HOST_BUSY)==SMIO_HOST_BUSY)
	{
		SmbusSts = IoRead8(SmbusBase+ SMIO_SMBUS_HOST_STA);
	}
	
	if (SmbusSts & SMIO_HT_PECERR)
	{
		Status = EFI_CRC_ERROR;
	}

	if (SmbusSts & SMIO_FAILED)
	{
		Status = EFI_DEVICE_ERROR;
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_STA, SMIO_FAILED);  //SMIO Rx00
		continue;
	}

	if (SmbusSts & SMIO_BUS_ERR)
	{
		Status = EFI_DEVICE_ERROR;
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_STA, SMIO_BUS_ERR);  //SMIO Rx00
		continue;
	}

	if (SmbusSts & SMIO_DEV_ERR)
	{
		Status = EFI_DEVICE_ERROR;
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_STA, SMIO_DEV_ERR);  //SMIO Rx00
		continue;
	}
	
	switch (Operation)
	{
	case EfiSmbusReadWord:

	case EfiSmbusProcessCall:
		DataBuf[1] = IoRead8(SmbusBase+ SMIO_SMBUS_HOST_DATA_1);  //SMIO Rx06
		break;
	case EfiSmbusReceiveByte:
		DataBuf[0] = IoRead8(SmbusBase+ SMIO_SMBUS_HOST_DATA_0);  //SMIO Rx05
		DEBUG((DEBUG_INFO, "[Smbus]: Receive Byte DataBuf[0]=0x%x\n",DataBuf[0]));	
		break;

	case EfiSmbusReadByte:
		DataBuf[0] = IoRead8(SmbusBase+ SMIO_SMBUS_HOST_DATA_0);  //SMIO Rx05
		break;

	case EfiSmbusReadBlock:
		SmallBuffer = FALSE;
		BlockCount = IoRead8(SmbusBase+ SMIO_SMBUS_HOST_DATA_0);  //SMIO Rx05
		if (*Length < BlockCount)
		{
			SmallBuffer = TRUE;
		}
		else
		{
			for (Index = 0; Index < BlockCount; Index++)
			{
				DataBuf[Index] = IoRead8(SmbusBase+ SMIO_SMBUS_BLOCK_DATA);  //SMIO Rx07
			}
		}
		*Length = BlockCount;
		if (SmallBuffer)
		{
			Status = EFI_BUFFER_TOO_SMALL;
		}
		break;
	default:
		break;
	}

	if ((SmbusSts & SMIO_BUS_ERR) && (Status != EFI_BUFFER_TOO_SMALL))
	{
		Status = EFI_DEVICE_ERROR;
		IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_STA, SMIO_BUS_ERR);  //SMIO Rx00
	}
	else
	{
		break;
	}
}

IoWrite8(SmbusBase+ SMIO_SMBUS_HOST_STA, 0xDE);  //SMIO Rx00
return Status; 
}

EFI_STATUS
EFIAPI
SmbusArpDev ( 
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      EFI_PEI_SMBUS_PPI         *This,
  IN      BOOLEAN                   ArpAll,
  IN      EFI_SMBUS_UDID            *SmbusUdid, OPTIONAL
  IN OUT  EFI_SMBUS_DEVICE_ADDRESS  *SlaveAddress OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
SmbusGetArpMap (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      EFI_PEI_SMBUS_PPI         * This,
  IN OUT  UINTN                     * Length,
  IN OUT  EFI_SMBUS_DEVICE_MAP      **SmbusDeviceMap
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
SmbusNotify (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      EFI_PEI_SMBUS_PPI         * This,
  IN      EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN      UINTN                     Data,
  IN      EFI_PEI_SMBUS_NOTIFY_FUNCTION NotifyFunction
  )
{
  return EFI_UNSUPPORTED;  
}

EFI_PEI_SMBUS_PPI gPeiSmbusPpi = {
  SmbusExec,
  SmbusArpDev,
  SmbusGetArpMap,
  SmbusNotify
};

EFI_PEI_PPI_DESCRIPTOR gSmbusPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiSmbusPpiGuid,
    &gPeiSmbusPpi
  },
};


#if THERMAL_IC_SUPPORT
typedef struct {
  UINT8  Offset;
  UINT8  Value;
} TM_DATA_TABLE;


/*
[00] - Local Temperature Value              (R)
[01] - External Temperature Value High Byte (R)
[02] - Status                               (R)
         [7] - ADC busy
         [6:3],[1],[0] - limit comparison flags
         [2] - remote sensor is open circuit
[03] - Configuration                        (R)
[09] - Configuration                        (W)
         [6] - Run(0)/Standby(1)
         [7] - ALERT Masked

[10] - External Temperature Value Low Byte  (R)
[19] - External THERM Limit                 (R/W)
[20] - Local THERM Limit                    (R/W)
*/
STATIC TM_DATA_TABLE  gTmDataTable[] = {
  {0x09, 0x80},
  {0x19, 45},
  {0x20, 85},
};

EFI_STATUS
InitThermalChip (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     Count;
  UINTN                     Length;
  UINT8                     Data8;
  EFI_SMBUS_DEVICE_ADDRESS  Address;


//DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));
    
  Count = sizeof(gTmDataTable)/sizeof(gTmDataTable[0]);
  Address.SmbusDeviceAddress = ADM1032_SMB_SLAVE_ADDR;
  Status = EFI_SUCCESS;
  
  for(Index=0;Index<Count;Index++){
    Length = 1;
    Data8  = gTmDataTable[Index].Value;
    Status = SmbusExec (
               (EFI_PEI_SERVICES**)PeiServices,
               &gPeiSmbusPpi,
               Address,
               gTmDataTable[Index].Offset,
               EfiSmbusWriteByte,
               FALSE,
               &Length,
               &Data8
               );
    if(EFI_ERROR(Status)){
      break;
    }
  }
  
  return Status;  
}
#endif


EFI_STATUS
InitSmbus (
  IN CONST EFI_PEI_SERVICES  **PeiServices
)
{
  EFI_STATUS  Status;
  
// PMU_RxD2[0]  Host Enable
// PMU_RxD0 PMU_RxD1 (SMBus I/O Base)
  MmioWrite16(LPC_PCI_REG(LPC_SMBUS_IOBASE_REG), SMBUS_BASE_ADDRESS);
  MmioOr8    (LPC_PCI_REG(LPC_SMBUS_HOST_CFG), SMBUS_HOST_EN | SMBUS_C128KS);
//MmioAnd8   (LPC_PCI_REG(LPC_MISC_CFG1_REG), (UINT8)~SMBUS_SC_RTC);

  Status = PeiServicesInstallPpi(gSmbusPpiList);
  ASSERT_EFI_ERROR(Status);

#if THERMAL_IC_SUPPORT 
  Status = InitThermalChip(PeiServices);
  ASSERT_EFI_ERROR(Status);  
#endif
  
  return Status;  
}

