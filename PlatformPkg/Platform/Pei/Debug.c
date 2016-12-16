
#include "PlatformPei.h"
#include <Library/PerformanceLib.h>

VOID DumpPci(UINT8 Bus, UINT8 Dev, UINT8 Func);

#ifndef MDEPKG_NDEBUG

VOID
PlatformPeiEntryDebug (
  IN EFI_ASIA_CPU_PPI_PROTOCOL  *CpuPpi
  )
{
//  UINT32  TopOfCar;
//  UINT32  *Data32;
//  UINT32  Reg32;
//  UINT32  Edx;	


  PERF_START (NULL, "PLATS", "PEI", 1);
  PERF_END   (NULL, "PLATS", "PEI", 0);

// (0,0,0,FB) PCIE ROMSIP Reg
//DEBUG((EFI_D_INFO, "(0,0,0,FB)=%02X\n", MmioRead8(HC_PCI_REG(0xFB))));

  // |--------------| <- TopOfCar
  // |Number of BSPs|
  // |--------------|
  // |     BIST     |
  // |--------------|
  // |     ....     |
  // |--------------|
  // |  TSC[63:32]  |
  // |--------------|
  // |  TSC[31:00]  |
  // |--------------|
//  TopOfCar = PcdGet32(PcdTemporaryRamBase) + PcdGet32(PcdTemporaryRamSize);
//  Data32	 = (UINT32*)(UINTN)(TopOfCar - 5*sizeof(UINT32));
//  DEBUG((EFI_D_INFO, "TopCar:%X %X %X %X %X\n", Data32[4], Data32[3], Data32[2], Data32[1], Data32[0]));
//
//  AsmCpuid(0, &Reg32, NULL, NULL, NULL);
//  DEBUG((EFI_D_INFO, "Cpuid(0).Eax:%08X, Msr(1B).L:%X\n", Reg32, (UINT32)AsmReadMsr64(0x1B)));
//  if(Reg32 >= 11){
//    AsmCpuidEx(11, 0, NULL, NULL, NULL, &Reg32);
//    DEBUG((EFI_D_INFO, "Cpuid(11,0).Edx:%08X\n", Reg32));		
//  }		
//  AsmCpuid(1, NULL, &Reg32, NULL, &Edx);
//  DEBUG((EFI_D_INFO, "Cpuid(1).Ebx:%08X, Edx:%08X\n", Reg32, Edx));
//
//DEBUG((EFI_D_INFO, "Msr(CE).L:%X\n", (UINT32)AsmReadMsr64(0xCE)));
//  DEBUG((EFI_D_INFO, "Msr(198):%lX\n", AsmReadMsr64(0x198)));
//  DEBUG((EFI_D_INFO, "Msr(199):%lX\n", AsmReadMsr64(0x199)));

  DumpIo4(0x800, 0x14);
  DumpIo4(0x820, 0x10);

//  DEBUG((EFI_D_INFO, "IGD_SSID  :%08X\n", MmioRead32(IGD_PCI_REG(PCI_SSID_REG))));
//  DEBUG((EFI_D_INFO, "IGDAC_SSID:%08X\n", MmioRead32(IGDAC_PCI_REG(PCI_SSID_REG)))); 

//DEBUG((EFI_D_INFO, "HC(2C):%X\n", MmioRead32(HC_PCI_REG(0x2C))));
//DEBUG((EFI_D_INFO, "LPC(2C):%X\n", MmioRead32(LPC_PCI_REG(0x2C))));
//DEBUG((EFI_D_INFO, "IGD(2C):%X\n", MmioRead32(IGD_PCI_REG(0x2C))));  
//
//DumpAllPci();

}

VOID
EndOfPeiDebug (
  IN EFI_BOOT_MODE  BootMode
  )
{
//ShowCacheSetting();
}

#endif
