
#include <Uefi.h>
#include <IndustryStandard/Pci.h>
#include <PlatformDefinition.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <CHX001Cfg.h>



STATIC
VOID
FindPciDeviceMmioLength (
  IN     UINT32  BusNum,
  IN     UINT32  DevNum,
  IN     UINT32  FunNum,
  IN OUT UINT32  *MmioLength
  )
{
  UINT32  CurrentMmioLength;
  UINT32  SavedBAR;
  UINT32  i;
  UINTN   PciBase;  


  PciBase  = PCI_DEV_MMBASE(BusNum, DevNum, FunNum);
  if(MmioRead8(PciBase+PCI_BCC_REG) == PCI_CLASS_BRIDGE){
    return;
  }
  
  for (i = PCI_BAR0_REG; i <= PCI_BAR5_REG; i += 4) {
    SavedBAR =  MmioRead32(PciBase+i);
    MmioAnd32(PciBase+i, 0);
    MmioOr32(PciBase+i, (UINT32)-1);

    if (SavedBAR == MmioRead32(PciBase+i)) {
      MmioWrite32(PciBase+i, SavedBAR);
      continue;
    }
    if ((SavedBAR & BIT0) != 0) {
      MmioWrite32(PciBase+i, SavedBAR);
      continue;
    }
    
    CurrentMmioLength = ~(MmioRead32(PciBase+i) & ~0xF) + 1;
//  DEBUG((EFI_D_ERROR, "(%X,%X,%X,%X):%X\n", BusNum, DevNum, FunNum, i, CurrentMmioLength));
    *MmioLength += CurrentMmioLength;
    MmioWrite32(PciBase+i, SavedBAR);

    if ((SavedBAR & (BIT1 + BIT2)) == 0x4) {
      i += 4;
    }
  }
}



STATIC
UINT8
EnumerateDownstream (
  IN UINT8  BusNum
  )
{
  UINT8   DevNum;
  UINT8   SubBus;
  UINT8   SecBus;
  UINTN   PciBase;  
  UINTN   SubPciBase;

  SubBus  = 0;
  SecBus  = BusNum;

  for (DevNum = 0; DevNum < 32; DevNum++) {
    PciBase = PCI_DEV_MMBASE(BusNum, DevNum, 0);
    if(MmioRead16(PciBase+PCI_VID_REG) == 0xFFFF){
      continue;
    }

    if (MmioRead16(PciBase+PCI_SCC_REG) == 0x0604) {
      SecBus++;
      MmioWrite8(PciBase+PCI_PBN_REG, BusNum);
      MmioWrite8(PciBase+PCI_SCBN_REG, SecBus);
      MmioWrite8(PciBase+PCI_SBBN_REG, 0xFF);
      
      SubPciBase = PCI_DEV_MMBASE(SecBus, 0, 0);
      MmioWrite16(SubPciBase+PCI_VID_REG, 0);

      SubBus = EnumerateDownstream(SecBus);
      
      MmioWrite8(PciBase+PCI_SBBN_REG, SubBus);
      SecBus = SubBus;
    }
  }

  if (SubBus == 0) {
    return BusNum;
  } else {
    return SubBus;
  }
}




STATIC
VOID
EnumerateBridgeDevice (
  IN UINT8      BridgeBus,
  IN UINT8      BridgeDev,
  IN UINT8      BridgeFunc,
  IN OUT UINT32 *MmioTotalSize
  )
{

  UINT8   Bus;
  UINT8   Dev;
  UINT8   Func;
  UINT8   SubBusNum;
  UINTN   PciBase;
  UINTN   SubPciBase;


  PciBase = PCI_DEV_MMBASE(BridgeBus, BridgeDev, BridgeFunc);
  if(MmioRead16(PciBase+PCI_VID_REG) == 0xFFFF){
    return;
  }		

  Bus = 1;
  MmioWrite8(PciBase+PCI_SCBN_REG, Bus);
  MmioWrite8(PciBase+PCI_SBBN_REG, 0xFF);  

// A config write is required in order for the device to re-capture the Bus number,
// according to PCI Express Base Specification, 2.2.6.2
  MmioWrite16(PCI_DEV_MMBASE(Bus, 0, 0)+PCI_VID_REG, 0);
  SubBusNum = EnumerateDownstream(Bus);

  for (Bus = 1; Bus <= SubBusNum; Bus++) {
    for (Dev = 0; Dev < 32; Dev++) {
      for(Func = 0; Func < 7; Func++){
        SubPciBase = PCI_DEV_MMBASE(Bus, Dev, Func);
        if(MmioRead16(SubPciBase+PCI_VID_REG) == 0xFFFF){
          if(Func == 0){Func = 7;}
          continue;
        }
        FindPciDeviceMmioLength(Bus, Dev, Func, MmioTotalSize);
        if(Func == 0 && !(MmioRead8(SubPciBase+PCI_HDR_REG) & BIT7)){
          Func = 7;
        }        
      }    
    }
  }

  for (Bus = SubBusNum; Bus >= 1; Bus--) {
    for (Dev = 0; Dev < 32; Dev++) {
      SubPciBase = PCI_DEV_MMBASE(Bus, Dev, 0);
      if(MmioRead16(SubPciBase+PCI_VID_REG) == 0xFFFF){
        continue;
      }
      if (MmioRead16(SubPciBase+PCI_SCC_REG) == 0x0604) {
        MmioWrite32(SubPciBase+PCI_PBN_REG, 0);      
      }
    }
  }
  
  MmioWrite32(PciBase+PCI_PBN_REG, 0);
}


/*
PciBus: Resource Map for Root Bridge PciRoot(0x0)
Type =  Mem32; Base = 0xFDE00000;	Length = 0x200000;	Alignment = 0xFFFFF
 Base = 0xFDE00000;	Length = 0x100000;	Alignment = 0xFFFFF;	Owner = PPB  [00|03|00:**]
 Base = 0xFDF00000;	Length = 0x80000;	  Alignment = 0x7FFFF;	Owner = PCI  [00|01|00:10]
 Base = 0xFDF80000;	Length = 0x4000;	  Alignment = 0x3FFF;	  Owner = PCI  [00|14|00:10]
 Base = 0xFDF84000;	Length = 0x4000;	  Alignment = 0x3FFF;	  Owner = PCI  [00|01|01:10]
 Base = 0xFDF88000;	Length = 0x1000;	  Alignment = 0xFFF;	  Owner = PCI  [00|01|00:14]
 Base = 0xFDF89000;	Length = 0x1000;	  Alignment = 0xFFF;	  Owner = PCI  [00|12|00:10]
 Base = 0xFDF8A000;	Length = 0x400;	    Alignment = 0xFFF;	  Owner = PCI  [00|0F|00:24]
 Base = 0xFDF8B000;	Length = 0x100;	    Alignment = 0xFFF;	  Owner = PCI  [00|10|07:10]

PciBus: Discovered PCI @ [02|00|00]
   BAR[2]: Type = PMem64; Alignment = 0x0FFF;	Length = 0x1000;	Offset = 0x18
   BAR[3]: Type = PMem64; Alignment = 0x3FFF;	Length = 0x4000;	Offset = 0x20
 
 PCI(0,2,0)     // 4x
 PCI(0,6,0)     // 1x
 PCI(0,13,0)    // PCI
*/
 
UINT16 FindTolum(IN ASIA_NB_CONFIGURATION *NbCfg)
{
  UINT32 TotalSize;
  UINT32 Tolum;
 
  TotalSize = 0;
  if(NbCfg->PciePEG){EnumerateBridgeDevice(0, 2, 0, &TotalSize);}	
  if(NbCfg->PciePE0){EnumerateBridgeDevice(0, 3, 0, &TotalSize);}  //D3F0
  if(NbCfg->PciePE1){EnumerateBridgeDevice(0, 3, 1, &TotalSize);}  //D3F1
  if(NbCfg->PciePE2){EnumerateBridgeDevice(0, 3, 2, &TotalSize);} 
  if(NbCfg->PciePE3){EnumerateBridgeDevice(0, 3, 3, &TotalSize);}  //D3F3
  if(NbCfg->PciePEG0){EnumerateBridgeDevice(0, 4, 0, &TotalSize);}  //D4F0
  if(NbCfg->PciePEG1){EnumerateBridgeDevice(0, 4, 1, &TotalSize);}  //D4F1
  if(NbCfg->PciePEG2){EnumerateBridgeDevice(0, 5, 0, &TotalSize);}  //D5F0
  if(NbCfg->PciePEG3){EnumerateBridgeDevice(0, 5, 1, &TotalSize);}  //D5F1
  TotalSize = ALIGN_VALUE(TotalSize, 0x200000);
  TotalSize += 0x200000;
  
  if(TotalSize > (PCI_MMIO_TOP_ADDRESS - 0xF0000000)){    // F0000000 ~ FE000000 is the second part of MMIO.
    Tolum  = 0xE0000000 - TotalSize;
    if(Tolum & (SIZE_256MB  - 1)){
      Tolum  = ALIGN_VALUE(Tolum, SIZE_256MB);
      Tolum -= SIZE_256MB;
    }    
  } else {
    Tolum = 0xE0000000;
  }
 
  return (UINT16)(Tolum>>20);
}













