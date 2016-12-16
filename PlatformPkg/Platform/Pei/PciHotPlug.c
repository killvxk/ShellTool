
#include <Uefi.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <PlatformDefinition.h>
#include <Ppi/Smbus.h>


/*
  P0.0 - AuxOn       OUT                          MASK
  P0.1 - AttButton   IN      Latch
  P0.2 - Present     IN              0:Present
  P0.3 - CtrlOn      OUT                          MASK
  P0.4 - Fault       IN     
  P0.5 - Gpio5                                    MASK
  P0.6 - Led3        OUT                          MASK
  P0.7 - PcieClkEn   OUT                          MASK

  P1.0 - PcieRst     OUT
  P1.1 - Ctrl1LED2   OUT
  P1.2 - Ctrl2LED2   OUT
*/


#define PcieHotPlugSMBusAddr 0x20
#define Pcal6416_GPIO_DirOut 0
#define Pcal6416_GPIO_DirIn 1
#define IntUnMask 0
#define IntEnMask 1
#define Pcal6416_InputPort0 0x00
#define Pcal6416_InputPort1 0x01
#define Pcal6416_OutputPort0 0x02
#define Pcal6416_OutputPort1 0x03
#define Pcal6416_PolarityInvPort0 0x04
#define Pcal6416_PolarityInvPort1 0x05
#define Pcal6416_CfgPort0 0x06
#define Pcal6416_CfgPort1 0x07
#define Pcal6416_OutputDriveStrengthReg0_0 0x40
#define Pcal6416_OutputDriveStrengthReg0_1 0x41
#define Pcal6416_OutputDriveStrengthReg1_0 0x42
#define Pcal6416_OutputDriveStrengthReg1_1 0x43
#define Pcal6416_InputLatchReg0 0x44
#define Pcal6416_InputLatchReg1 0x45
#define Pcal6416_PullUpDnEnReg0 0x46
#define Pcal6416_PullUpDnEnReg1 0x44
#define Pcal6416_PullUpDnSelReg0 0x48
#define Pcal6416_PullUpDnSelReg1 0x49
#define Pcal6416_IntMaskReg0 0x4A
#define Pcal6416_IntMaskReg1 0x4B
#define Pcal6416_IntStatusReg0 0x4C
#define Pcal6416_IntStatusReg1 0x4D
#define Pcal6416_OutPortCfgReg 0x4F


typedef union {
    UINT8       Port0;
    struct  {
        BOOLEAN   AUXON  : 1;
        BOOLEAN   AttButton  : 1;
        BOOLEAN   Pres_DET  : 1;
        BOOLEAN   CtrlON  : 1;
        BOOLEAN   Fault  : 1;
        BOOLEAN   GPIO5  : 1;
        BOOLEAN   CtrlLED3  : 1;
        BOOLEAN   CtrlPcieClkEn  : 1;
    } P0;
} HX001HP_P0;

typedef union {
    UINT8       Port1;
    struct {
        BOOLEAN   CtrlPcieRst : 1;
        BOOLEAN   Ctrl1LED2   : 1;
        BOOLEAN   Ctrl2LED2   : 1;
        UINT8     Rsv         : 5;
    } P1;
} HX001HP_P1;





EFI_STATUS 
PcieHotPlugInit (
  IN EFI_PEI_SERVICES  **PeiServices
	)
{
	EFI_STATUS						      Status;
	EFI_PEI_SMBUS_PPI 					*pSMBusPPI;
	EFI_SMBUS_DEVICE_ADDRESS		SlaveAddress;
	UINTN							          SMBDataLength;
	UINT8							          SMBData;
//UINT8 						        	Index; 
//UINT8                       Buffer[8];
	volatile HX001HP_P0         HP_P0;
	volatile HX001HP_P1         HP_P1;


  ASSERT(sizeof(HX001HP_P0) == 1);
  ASSERT(sizeof(HX001HP_P1) == 1);  

	Status = (*PeiServices)->LocatePpi(PeiServices, &gEfiPeiSmbusPpiGuid, 0, NULL, &pSMBusPPI);
	ASSERT(!EFI_ERROR(Status));

/*
	SlaveAddress.SmbusDeviceAddress = 0x6D;
	SMBDataLength = 0x08;
  Status = pSMBusPPI->Execute(
                        PeiServices, 
                        pSMBusPPI, 
                        SlaveAddress, 
                        0x00, 
                        EfiSmbusReadBlock, 
                        0x00, 
                        &SMBDataLength, 
                        &Buffer
                        );
	if(!EFI_ERROR(Status)) {
    DEBUG((EFI_D_INFO, "9DBU0231: "));
		for(Index = 0; Index < 8; Index++){
			DEBUG((EFI_D_INFO, "%02X \n", Buffer[Index]));
		}
    DEBUG((EFI_D_INFO, "\n"));  
	}
*/

	SlaveAddress.SmbusDeviceAddress = PcieHotPlugSMBusAddr;
	SMBDataLength = 0x01;
	
	// Set IO expander cfg
	SMBData = 0;
	Status = pSMBusPPI->Execute (
                      PeiServices, 
                      pSMBusPPI, 
                      SlaveAddress, 
                      Pcal6416_CfgPort0, 
                      EfiSmbusReadByte, 
                      FALSE, 
                      &SMBDataLength, 
                      &SMBData
                      );
	DEBUG((EFI_D_INFO, "[PCAL6416] [%X] = %X\n", Pcal6416_CfgPort0, SMBData));

	HP_P0.Port0            = SMBData;
	HP_P0.P0.AUXON         = Pcal6416_GPIO_DirOut;
	HP_P0.P0.AttButton     = Pcal6416_GPIO_DirIn;
	HP_P0.P0.Pres_DET      = Pcal6416_GPIO_DirIn;
	HP_P0.P0.CtrlON        = Pcal6416_GPIO_DirOut;
	HP_P0.P0.Fault         = Pcal6416_GPIO_DirIn;
	HP_P0.P0.CtrlLED3      = Pcal6416_GPIO_DirOut;
	HP_P0.P0.CtrlPcieClkEn = Pcal6416_GPIO_DirOut;
	SMBData = HP_P0.Port0;
	Status = pSMBusPPI->Execute(
                        PeiServices, 
                        pSMBusPPI, 
                        SlaveAddress, 
                        Pcal6416_CfgPort0, 
                        EfiSmbusWriteByte, 
                        FALSE, 
                        &SMBDataLength, 
                        &SMBData
                        );
	DEBUG((EFI_D_INFO, "[PCAL6416] [%X] = %X\n", Pcal6416_CfgPort0, SMBData));
	
	SMBData = 0;
	Status = pSMBusPPI->Execute(
                        PeiServices, 
                        pSMBusPPI, 
                        SlaveAddress, 
                        Pcal6416_CfgPort1, 
                        EfiSmbusReadByte, 
                        FALSE, 
                        &SMBDataLength, 
                        &SMBData
                        );
	DEBUG((EFI_D_INFO, "[PCAL6416] [%X] = %X\n", Pcal6416_CfgPort1, SMBData));
	HP_P1.Port1 = SMBData;
	HP_P1.P1.CtrlPcieRst = Pcal6416_GPIO_DirOut;
	HP_P1.P1.Ctrl1LED2   = Pcal6416_GPIO_DirOut;
	HP_P1.P1.Ctrl2LED2   = Pcal6416_GPIO_DirOut;
	SMBData = HP_P1.Port1;
	Status = pSMBusPPI->Execute(
                        PeiServices, 
                        pSMBusPPI, 
                        SlaveAddress, 
                        Pcal6416_CfgPort1, 
                        EfiSmbusWriteByte, 
                        FALSE, 
                        &SMBDataLength, 
                        &SMBData
                        );
	DEBUG((EFI_D_INFO, "[PCAL6416] [%X] = %X\n", Pcal6416_CfgPort1, SMBData));

	SMBData = 0x02;
	Status = pSMBusPPI->Execute(
                        PeiServices, 
                        pSMBusPPI, 
                        SlaveAddress, 
                        Pcal6416_InputLatchReg0, 
                        EfiSmbusWriteByte, 
                        FALSE, 
                        &SMBDataLength, 
                        &SMBData
                        );

	SMBData = 0;
	Status = pSMBusPPI->Execute(
                        PeiServices, 
                        pSMBusPPI, 
                        SlaveAddress, 
                        Pcal6416_InputPort0, 
                        EfiSmbusReadByte, 
                        FALSE, 
                        &SMBDataLength, 
                        &SMBData
                        );
	DEBUG((EFI_D_INFO, "[PCAL6416] [%X] = %X\n", Pcal6416_InputPort0, SMBData));

	HP_P0.Port0 = SMBData;

	if( HP_P0.P0.Pres_DET == 0 ){
		HP_P0.P0.AUXON = 1;
		HP_P0.P0.CtrlON  =1;
		HP_P0.P0.CtrlLED3  = 0;
		HP_P0.P0.CtrlPcieClkEn = 1;
    
		HP_P1.P1.CtrlPcieRst  = 1;
		HP_P1.P1.Ctrl2LED2  = 1;
		DEBUG((EFI_D_INFO, "Card Present!\n"));
	}	else {
		HP_P0.P0.AUXON = 0;
		HP_P0.P0.CtrlON  =0;
		HP_P0.P0.CtrlLED3  = 1;
		HP_P0.P0.CtrlPcieClkEn  = 0;
		HP_P1.P1.CtrlPcieRst  = 0;
		HP_P1.P1.Ctrl1LED2  = 1;
		HP_P1.P1.Ctrl2LED2  = 0;
		DEBUG((EFI_D_INFO, "Card Not Present!\n"));
	}

	SMBData = HP_P0.Port0;
	Status = pSMBusPPI->Execute(
                        PeiServices, 
                        pSMBusPPI, 
                        SlaveAddress, 
                        Pcal6416_OutputPort0, 
                        EfiSmbusWriteByte, 
                        FALSE, 
                        &SMBDataLength, 
                        &SMBData
                        );
	DEBUG((EFI_D_INFO, "[PCAL6416] [%X] = %X\n", Pcal6416_OutputPort0, SMBData));

	SMBData = HP_P1.Port1;
	Status = pSMBusPPI->Execute(
                        PeiServices, 
                        pSMBusPPI, 
                        SlaveAddress, 
                        Pcal6416_OutputPort1, 
                        EfiSmbusWriteByte, 
                        FALSE, 
                        &SMBDataLength, 
                        &SMBData
                        );
	DEBUG((EFI_D_INFO, "[PCAL6416] [%X] = %X\n", Pcal6416_OutputPort1, SMBData));

	if( HP_P0.P0.Pres_DET == 0 ){
		SMBData = 0xED;
	} else {
		SMBData = 0xEB;
	}
	Status = pSMBusPPI->Execute(
                        PeiServices, 
                        pSMBusPPI, 
                        SlaveAddress, 
                        Pcal6416_IntMaskReg0, 
                        EfiSmbusWriteByte, 
                        FALSE, 
                        &SMBDataLength, 
                        &SMBData
                        );

	return EFI_SUCCESS;
}


