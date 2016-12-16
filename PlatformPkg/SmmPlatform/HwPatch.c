
#include "SmmPlatform.h"

// Get from AsiaPkg\Porting\Pei\Dram\VX11\UmaSet.c

/*
VOID EUMAPowerDown()
{
  if(MmioRead16(IGD_PCI_REG(PCI_VID_REG) == 0xFFFF)){
    return;
  }

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  
// OpenUMApath
  MmioOr32(IGD_PCI_REG(PCI_CMD_REG),  BIT0);    
	
//VGA Enable, 3C3[0]=1, 3C2 = 67H, CR38 = 48H , CR39 = A5H, SR08 = 06H
	IoWrite8(0x03C3, ((IoRead8(0x03C3)&(~0x01)))|0x01);
	IoWrite8(0x3C2, 0x67);
	
//To access SR higher than SR09, set SR08= 06h
	IoWrite16(0x3C4, 0x0608);
	
//To access CR higher than CR40, set CR38=48h, CR39=A5h
	IoWrite16(0x3D4, 0x4838);
	IoWrite16(0x3D4, 0xA539);

//Power Down DCLK2 PLL, SR0B[0]=1
	IoWrite8(0x3C4, 0x0B);
	IoWrite8(0x3C5, ((IoRead8(0x3C5)&(~0x01)))|0x01);

//SR1C[4:2] = 111b
	IoWrite8(0x3C4, 0x1C);
	IoWrite8(0x3C5, ((IoRead8(0x3C5)&(~0x1C)))|0x1C);

//SR1D[6:3,0] = 11111b
	IoWrite8(0x3C4, 0x1D);
	IoWrite8(0x3C5, ((IoRead8(0x3C5)&(~0x79)))|0x79);

//SR1E[4:0] = 11111b
	IoWrite8(0x3C4, 0x1E);
	IoWrite8(0x3C5, ((IoRead8(0x3C5)&(~0x1F)))|0x1F);

//To access CRC2_B/CRC2_C, must set SR2a[1:3,6]=0010b
	IoWrite8(0x3C4, 0x2A);
	IoWrite8(0x3C5, ((IoRead8(0x3C5)&(~0x08)))|0x08);

//CRFC_B[2:0]=7
	IoWrite8(0x3D4, 0xFC);
	IoWrite8(0x3D5, ((IoRead8(0x3D5)&(~0x07)))|0x07);

//CRFC_B[2]=0
	IoWrite8(0x3D4, 0xF2);
	IoWrite8(0x3D5, ((IoRead8(0x3D5)&(~0x04)))|0x00);
	
//To access standard CR/SR, set SR26=0 and SR2A=0
	IoWrite16(0x3C4, 0x0026);
	IoWrite16(0x3C4, 0x002A);
	
//Disable 3C3[0]
	IoWrite8(0x3C3, ((IoRead8(0x3C3)&(~0x01)))|0x00);

// CloseUMApath	
  MmioAnd32(IGD_PCI_REG(PCI_CMD_REG),  (UINT32)~BIT0);
#endif
}

VOID WaitPcieL2L3Ready()
{
  UINTN  Count;

  MmioOr32(APIC_PCI_REG(APIC_PCIE_PMU_STS1_REG), STS_PME_PCIE_ACK);   // clear
  MmioOr32(APIC_PCI_REG(APIC_PMU_CTRL_REG), TRIGGER_PME_OFF_MSG);
  
  Count = 1000;
  while(Count--){
    if((MmioRead32(APIC_PCI_REG(APIC_PCIE_PMU_STS1_REG)) & STS_PME_PCIE_ACK) == STS_PME_PCIE_ACK){
      break;
    }
    MicroSecondDelay(10);
  }    
  
  DEBUG((EFI_D_ERROR, "(0,0,5,A0):%X Count:%d\n", MmioRead8(APIC_PCI_REG(APIC_PCIE_PMU_STS1_REG)), Count));
}
*/





