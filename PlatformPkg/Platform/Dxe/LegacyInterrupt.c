
#include "PlatformDxe.h"
#include <Protocol/LegacyInterrupt.h>
#include <CHX001Reg.h>

/*
PIRQ A          LPC_Rx55[7:4] 
PIRQ B          LPC_Rx56[3:0] 
PIRQ C          LPC_Rx56[7:4] 
PIRQ D          LPC_Rx57[7:4] 
PIRQ E          LPC_Rx44[3:0] 
PIRQ F          LPC_Rx44[7:4] 
PIRQ G          LPC_Rx45[3:0] 
PIRQ H          LPC_Rx45[7:4] 
*/
typedef struct {
  UINT8  Reg;
  UINT8  Offset;
  UINT8  Width;
} PIRQ_REG_INFO;  

STATIC PIRQ_REG_INFO  gPirqRegInfo[] = {
  {0x55, 4, 4},     // A
  {0x56, 0, 4},     // B
  {0x56, 4, 4},     // C
  {0x57, 4, 4},     // D
  {0x44, 0, 4},     // E
  {0x44, 4, 4},     // F
  {0x45, 0, 4},     // G
  {0x45, 4, 4},     // H  
};

STATIC
EFI_STATUS
GetNumberPirqs (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  OUT UINT8                          *NumberPirqs
  )
{
  *NumberPirqs = sizeof(gPirqRegInfo)/sizeof(gPirqRegInfo[0]);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetLocation (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  OUT UINT8                          *Bus,
  OUT UINT8                          *Device,
  OUT UINT8                          *Function
  )
{
  /// D17F0
  *Bus      = LPC_BUS_NO;
  *Device   = LPC_DEV_NO;
  *Function = LPC_FUNC_NO;
  return EFI_SUCCESS;
}

STATIC
UINTN
GetAddress (
  UINT8  PirqNumber
  )
{
  return (UINTN)PcdGet64(PcdPciExpressBaseAddress) 
         + (UINTN)(((LPC_BUS_NO << 8) + (LPC_DEV_NO << 3) + LPC_FUNC_NO) << 12)
         + gPirqRegInfo[PirqNumber].Reg;
}

STATIC
EFI_STATUS
ReadPirq (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  IN  UINT8                          PirqNumber,
  OUT UINT8                          *PirqData
  )
{
  UINTN  Count;
  UINT8  Data8;
    
  Count = sizeof(gPirqRegInfo)/sizeof(gPirqRegInfo[0]);
  if (PirqNumber >= Count) {
    return EFI_INVALID_PARAMETER;
  }

  Data8   = MmioRead8(GetAddress(PirqNumber));
  Data8 >>= gPirqRegInfo[PirqNumber].Offset;
  Data8  &= (1<<gPirqRegInfo[PirqNumber].Width) - 1;
  
  *PirqData = Data8;

  DEBUG((EFI_D_INFO, "%a(%X,%X)\n", __FUNCTION__, PirqNumber, *PirqData));  
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
WritePirq (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  IN  UINT8                          PirqNumber,
  IN  UINT8                          PirqData
  )
{
  UINTN  Count;
  UINT8  Data8;
  UINT8  Mask;

//DEBUG((EFI_D_INFO, "%a(%X,%X)\n", __FUNCTION__, PirqNumber, PirqData));  
  
  Count = sizeof(gPirqRegInfo)/sizeof(gPirqRegInfo[0]);
  if (PirqNumber >= Count) {
    return EFI_INVALID_PARAMETER;
  }

  Data8  = MmioRead8(GetAddress(PirqNumber));
  Mask   = (1<<gPirqRegInfo[PirqNumber].Width) - 1;
  PirqData &= Mask;
  PirqData <<= gPirqRegInfo[PirqNumber].Offset;	
  Mask <<= gPirqRegInfo[PirqNumber].Offset;
  Data8 &= ~Mask;
  Data8 |= PirqData;

//DEBUG((EFI_D_INFO, "[%08X] = %02X\n", GetAddress(PirqNumber), Data8));  
  MmioWrite8(GetAddress(PirqNumber), Data8);
  return EFI_SUCCESS;
}


STATIC EFI_LEGACY_INTERRUPT_PROTOCOL gLegacyInterrupt = {
  GetNumberPirqs,
  GetLocation,
  ReadPirq,
  WritePirq
};

///
STATIC EFI_STATUS
SBGen_InitializeRouterRegisters (
	EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRBIo)
{
	EFI_STATUS	Status = EFI_SUCCESS;
	UINT16      Data16;
	UINT32      Data32;
	
	///
	Data16 = 0;
	//0x44-0x45 EFGH
	Status = PciRBIo->Pci.Write( PciRBIo, EfiPciWidthUint16, CHX001_BUSC | D17F0_PCI_PNP_INTR_ROUTING_INTE_INTF, 1, &Data16);
	//0x54-0x57 ABCD
	Status = PciRBIo->Pci.Read( PciRBIo, EfiPciWidthUint32, CHX001_BUSC | D17F0_PCI_BUS_AND_CPU_IF_CTL, 1, &Data32);
	Data32 &=~(0xF0FFF00F);
	Status = PciRBIo->Pci.Write( PciRBIo, EfiPciWidthUint32, CHX001_BUSC | D17F0_PCI_BUS_AND_CPU_IF_CTL, 1, &Data32);


	return Status;
}

///
EFI_STATUS
LegacyInterruptInstall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  /// synch with Chipset\Sb\SBGeneric.c (AMI)
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;  
  Status = gBS->LocateProtocol (&gEfiPciRootBridgeIoProtocolGuid, NULL, &PciRootBridgeIo);
  ASSERT_EFI_ERROR (Status);
  SBGen_InitializeRouterRegisters(PciRootBridgeIo);
  ///
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiLegacyInterruptProtocolGuid,
                  &gLegacyInterrupt,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  return Status;
}
