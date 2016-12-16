
#include "SmmPlatform.h"




VOID EnPciLanPmeOnBridge(UINT8 BridgeBus, UINT8 BridgeDev, UINT8 BridgeFunc, BOOLEAN Enable)
{
  UINTN  PciBase;
  UINT32 Data32;
  UINT8  PciBusNo;
  UINT8  Data8;
  UINT8  CapId;
  UINT8  CapPm;
  UINTN  Count;
  
  PciBase = PCI_DEV_MMBASE(BridgeBus, BridgeDev, BridgeFunc);
  if(MmioRead16(PciBase+PCI_VID_REG) == 0xFFFF){
    return;
  }
  
  Data32 = MmioRead32(PciBase+PCI_PBN_REG);
  PciBusNo = (UINT8)((Data32>>8) & 0xFF);
  
  PciBase = PCI_DEV_MMBASE(PciBusNo, 0, 0);
  if(MmioRead16(PciBase+PCI_VID_REG) == 0xFFFF){
    return;
  }
  Data32 = MmioRead32(PciBase+PCI_REV_ID_REG);
  Data32 = (Data32>>8)&0xFFFFFF;
  if(Data32 != 0x020000){
    return;
  }
  if(!(PcieRead8(PciBase+PCI_PRI_STS_REG) & PCI_STS_CAP_LIST)){
    return;
  }
  
  Data8 = PcieRead8(PciBase+PCI_CAP_POINT_REG);
  Count = 20;
  CapPm = 0;
  while(Count--){
    CapId = PcieRead8(PciBase+Data8);
    if(CapId == PCI_CAP_ID_PM){
      CapPm = Data8;
      break;
    } else {
      Data8 = PcieRead8(PciBase+Data8+1);
      if(Data8 == 0){
        break;
      }
    }    
  }
  if(CapPm == 0){
    return;
  }
  
  DEBUG((EFI_D_ERROR, "PM:(%X,%X,%X,%X)\n", PciBusNo, 0, 0, CapPm));
  CapPm += 4;
  MmioOr16(PciBase+CapPm, BIT15);               // clear PME Status
  
  if(Enable){
    MmioOr16(PciBase+CapPm, BIT8);              // PME_EN
  } else {
    MmioAnd16(PciBase+CapPm, (UINT16)~BIT8); 
  }
}


/*  
// For AddOn Lan.
  if(SleepType == 5 || SleepType == 0xFF){
    EnPciLanPmeOnBridge(0,    2, 0, WakeOnLanEn);
    EnPciLanPmeOnBridge(0,    6, 0, WakeOnLanEn);
    EnPciLanPmeOnBridge(0, 0x13, 0, WakeOnLanEn);
    IoWrite16(PMIO_REG(PMIO_GP_STS), PMIO_GP_STS_PME);
    if(WakeOnLanEn){
      IoOr16(PMIO_REG(PMIO_GP_SMI_EN), PMIO_GP_SMI_EN_PME);
    } else {
      IoAnd16(PMIO_REG(PMIO_GP_SMI_EN), (UINT16)~PMIO_GP_SMI_EN_PME);
    }    
  }
*/
/*
VOID SetWakeOnLan(UINT8 SleepType)
{
  BOOLEAN  WakeOnLanEn = TRUE;

  if(SleepType == 5 || SleepType == 0xFF){
    WakeOnLanEn = gSetupData.WakeOnLan;
  } 

// For OnBoard Lan.
  if(WakeOnLanEn){
    DEBUG((EFI_D_ERROR, "LanWake\n"));
    IoAnd32(mAcpiBaseAddr + PMIO_GPO_REG, (UINT32)~PMIO_GPO_GPO7);		
  } else {
    DEBUG((EFI_D_ERROR, "LanNoWake\n"));
  }
}
*/


VOID SetAfterPowerLoss()
{
  if(gSetupData.AfterPowerLoss == AFTER_POWER_LOSS_PREVIOUS){
    SetCmosVRT(TRUE);
  }
}


