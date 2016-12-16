
////

#include "../ClockGen.h"
#include <CHX001Reg.h>

//OC 5%, VC0 = 840,
UINT16 CpuVCO[] = {640, 680, 720, 760, 800,
                   808, 816, 824, 832, 840, 848, 856, 864, 872, 880,
                   888, 896, 904, 912, 920, 928, 936, 944, 952, 960};

UINT16 PcieVCO[] = {800,
                   808, 816, 824, 832, 840, 848, 856, 864, 872, 880,
                   888, 896, 904, 912, 920, 928, 936, 944, 952, 960,
                   792, 784, 776, 768, 760};

//PLL1 VC0 = 1000
UINT16 Cpu1000VCO[] = {800, 850, 900, 950, 1000,
                   1010, 1020, 1030, 1040, 1050, 1060, 1070, 1080, 1090, 1100,
                   1110, 1120, 1130, 1140, 1150, 1160, 1170, 1180, 1190, 1200};


///

EFI_STATUS PreSetRTM875ClkGen(IN EFI_PEI_SERVICES   **PeiServices,
                             IN EFI_PEI_SMBUS_PPI     *SmbusPpi)
{
    EFI_SMBUS_DEVICE_COMMAND    SmbusDeviceCommand;
    UINT8  Data;
    EFI_STATUS Status;

    //set read back byte count = 1, CR_0C[5:0] = 1b
    Data = 1;
    SmbusDeviceCommand = 0x0C;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_0C error: %r\n", Status));
        return Status;
    }

    //disable PD Restore mode
    //don't save config in power-down, set CR_00h[0]=0b
    SmbusDeviceCommand = 0x00;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Read clock Gen CR_00 error: %r\n", Status));
        return Status;
    }
    Data &= ~0x01;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Write clock Gen CR_00 error: %r\n", Status));
        return Status;
    }

   return EFI_SUCCESS;
}


EFI_STATUS ProgramRTM875NoOC(IN EFI_PEI_SERVICES   **PeiServices,
                             IN EFI_PEI_SMBUS_PPI     *SmbusPpi)
{

    /*EFI_SMBUS_DEVICE_COMMAND    SmbusDeviceCommand;
    UINT8  Data;
    EFI_STATUS Status;

    //set read back byte count = 1, CR_0C[5:0] = 1b
    Data = 1;
    SmbusDeviceCommand = 0x0C;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_0C error: %r\n", Status));
        return Status;
    }

    SmbusDeviceCommand = 0x00;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Read clock Gen CR_00 error: %r\n", Status));
        return Status;
    }
    Data |= 0x02;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Write clock Gen CR_00 error: %r\n", Status));
        return Status;
    }

    /// 3456-TGR-08+S
    /// (For Hulu C2/C3 test. 20110729)
    SmbusDeviceCommand = 0x0A;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Read clock Gen CR_0A error: %r\n", Status));
        return Status;
    }
    Data &= 0xFD; //  set CA-0A[1] == 0;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Write clock Gen CR_0A error: %r\n", Status));
        return Status;
    }
    /// 3456-TGR-08+E*/
    
    ///
    EFI_SMBUS_DEVICE_COMMAND    SmbusDeviceCommand;
    UINT8  Data, RegData8;
    EFI_STATUS Status;

    //if NO OC, 
    // PLL1----CPU/PCIE/USB3/PCI(SSC on - Down 0.4)
    // PLL2----USB(NON SSC)
    // PLL3----SATA(NON SSC)

    Status = PreSetRTM875ClkGen(PeiServices, SmbusPpi);
    if (EFI_ERROR(Status))
        return Status;

#if 0
    (**PeiServices).PciCfg->Read (
                                  PeiServices,
                                  (**PeiServices).PciCfg,
                                  PeiPciCfgWidthUint8,
                                  VX11_V4IF| D0F2_CPU_IF_CTL_FSB_FREQ_CTL_BY_STRAPPING,  
                                  &RegData8
                                  );
#else
    #define D0F2_RCKMD_2_0 0x10
	RegData8 = 0x10;
#endif
    if (((RegData8 & D0F2_RCKMD_2_0)!= D0F2_RCKMD_166MHZ) &&
        ((RegData8 & D0F2_RCKMD_2_0)!= D0F2_RCKMD_333MHZ)){
        //USB 3.0 setting
        //set PLL1 M Code = 0x51, CR21 = 0x51
        Data = 0x51;
        SmbusDeviceCommand = 0x21;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_21 error: %r\n", Status));
            return Status;
        }
        //set PLL1 SSC-f Code[7:0] = 0xA6, CR24 = 0xA6
        Data = 0xA6;
        SmbusDeviceCommand = 0x24;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_24 error: %r\n", Status));
            return Status;
        }
        // Set PLL1 Programable Selection = 1 (PLL1 Programable by PLL1 M Code and SSC-f Code Setting)
        // CR23[7] = 1b
        SmbusDeviceCommand = 0x23;
        Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "read clock Gen CR_23 error: %r\n", Status));
            return Status;
        }
        Data = Data & 0x7F;
        Data = Data | 0x80;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_23 error: %r\n", Status));
            return Status;
        }
        //Set PLL1 SSC-f Code[9:8] = 01b, CR23[5:4] =01b
        SmbusDeviceCommand = 0x23;
        Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "read clock Gen CR_23 error: %r\n", Status));
            return Status;
        }
        Data = Data & 0xCF;
        Data = Data | 0x10;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_23 error: %r\n", Status));
            return Status;
        }
        // Set PLL1 SSC Range Control = 1 (SSC Range Control by CR2Ch[5:0])
        // CR2C[7] = 1b
        SmbusDeviceCommand = 0x2C;
        Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "read clock Gen CR_2C error: %r\n", Status));
            return Status;
        }
        Data = Data & 0x7F;
        Data = Data | 0x80;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_2C error: %r\n", Status));
            return Status;
        }
        // Set PLL1 Spread Spectrun Range = -0.40% CR2C[5:0] = 0x03
        SmbusDeviceCommand = 0x2C;
        Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "read clock Gen CR_2C error: %r\n", Status));
            return Status;
        }
        Data = Data & 0xC0;
        Data = Data | 0x03;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_2C error: %r\n", Status));
            return Status;
        } 
        // Set PLL1 VCO Code Mode = 1 (PLL1 Programable Enable & Latching)
        // set CR1F[0] = 1
        SmbusDeviceCommand = 0x1F;
        Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "read clock Gen CR_1F error: %r\n", Status));
            return Status;
        }
        Data = Data & 0xFE;
        Data = Data | 0x01;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_1F error: %r\n", Status));
            return Status;
        }
    }

    //SATA setting
    //read CR_04
    SmbusDeviceCommand = 0x04;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_04 error: %r\n", Status));
        return Status;
    }
    //set CR_04[10]= 10b
    //PLL1---SSC on, PLL3---SSC off
    Data = Data & 0xFC;
    Data = Data | 0x02;
    SmbusDeviceCommand = 0x04;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_04 error: %r\n", Status));
        return Status;
    }

    //Program CPU Divider, CR1C[6:4]
    SmbusDeviceCommand = 0x1C;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
         DEBUG(( EFI_D_ERROR, "read clock Gen CR_1F error: %r\n", Status));
         return Status;
    }
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
         DEBUG(( EFI_D_ERROR, "write clock Gen CR_1C error: %r\n", Status));
         return Status;
    }

    //Program PCI-E Divider, CR1D[2:0]
    SmbusDeviceCommand = 0x1D;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_1D error: %r\n", Status));
        return Status;
    }
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_1D error: %r\n", Status));
        return Status;
    }

    //Program SATA Divider = PLL3/8 CR1D[6:4] = 0x02
    SmbusDeviceCommand = 0x1D;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_1D error: %r\n", Status));
        return Status;
    }
    Data = Data & 0x8F;
    Data = Data | 0x20;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_1D error: %r\n", Status));
        return Status;
    }

    //Program PCI Divider CR1E[6:4]
    SmbusDeviceCommand = 0x1E;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_1E error: %r\n", Status));
        return Status;
    }
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_1E error: %r\n", Status));
        return Status;
    }

    // Set Divider Select by REG CR1F[1]=1b
    SmbusDeviceCommand = 0x1F;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_1F error: %r\n", Status));
        return Status;
    }
    Data = Data & 0xFD;
    Data = Data | 0x02;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_1F error: %r\n", Status));
        return Status;
    }

    // CPU1_STOP_EN = 0 (CPU1 Free Running)
    // CR0A[1] = 0b
    SmbusDeviceCommand = 0x0A;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Read clock Gen CR_0A error: %r\n", Status));
        return Status;
    }
    Data &= 0xFD; //  set CA-0A[1] == 0;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Write clock Gen CR_0A error: %r\n", Status));
        return Status;
    }
    return EFI_SUCCESS;
}

EFI_STATUS ProgramRTM875OC(IN EFI_PEI_SERVICES   **PeiServices,
                          IN EFI_PEI_SMBUS_PPI     *SmbusPpi,
                          IN SETUP_DATA *Config
                          )
{

    EFI_SMBUS_DEVICE_COMMAND    SmbusDeviceCommand;
    UINT8  Data,RegData8;
    EFI_STATUS Status;

    //if OC, 
    // PLL1----CPU/PCI(NON SSC)
    // PLL2----SATA/USB2(NON SSC)
    // PLL3----PCIE/USB3(NON SSC)

    Status = PreSetRTM875ClkGen(PeiServices, SmbusPpi);
    if (EFI_ERROR(Status))
        return Status;

#if 0
    (**PeiServices).PciCfg->Read (
                                  PeiServices,
                                  (**PeiServices).PciCfg,
                                  PeiPciCfgWidthUint8,
                                  VX11_V4IF| D0F2_CPU_IF_CTL_FSB_FREQ_CTL_BY_STRAPPING,  
                                  &RegData8
                                  );
#else
	#define D0F2_RCKMD_2_0 0x10
	RegData8 = 0x10;
#endif

    ////
    if (((RegData8 & D0F2_RCKMD_2_0)!= D0F2_RCKMD_166MHZ) &&
        ((RegData8 & D0F2_RCKMD_2_0)!= D0F2_RCKMD_333MHZ)){
        //program PLL1
        //USB 3.0 setting
        //3456-04-PDZ-06 if (Config->CpuClockControl == 0x09)
            // Cpu OC 5%
            //3456-04-PDZ-06 Data = 84;
        //3456-04-PDZ-06 else
            // Cpu NO OC
            //3456-04-PDZ-06 Data = 80;
        //3456-04-PDZ-06 +S
        Data = (UINT8)CpuVCO[Config->CpuClockControl];
    }
	else
		Data = (UINT8)Cpu1000VCO[Config->CpuClockControl];
    //3456-04-PDZ-06 +E
    SmbusDeviceCommand = 0x11;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
         DEBUG(( EFI_D_ERROR, "write clock Gen CR_21 error: %r\n", Status));
         return Status;
    }

    //3456-04-PDZ-06 +S
	//set CR13[6:4] 
    SmbusDeviceCommand = 0x13;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_13 error: %r\n", Status));
        return Status;
    }
    Data = Data & 0x8F;
	if (((RegData8 & D0F2_RCKMD_2_0)!= D0F2_RCKMD_166MHZ) &&
        ((RegData8 & D0F2_RCKMD_2_0)!= D0F2_RCKMD_333MHZ)){
        RegData8 = (UINT8)(CpuVCO[Config->CpuClockControl] >> 8);
    }
	else
		RegData8 = (UINT8)(Cpu1000VCO[Config->CpuClockControl] >> 8);
    Data |= RegData8 << 4;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_13 error: %r\n", Status));
        return Status;
    }
    //3456-04-PDZ-06 +E

    // Set PLL1 Programable Selection = 0 (PLL1 Programable by VCO code frequency setting)
    // CR23[7] = 0b
    SmbusDeviceCommand = 0x23;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_23 error: %r\n", Status));
        return Status;
    }
    Data = Data & 0x7F;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
         DEBUG(( EFI_D_ERROR, "write clock Gen CR_23 error: %r\n", Status));
         return Status;
    }
       
    // Set PLL1 VCO Code Mode = 1 (PLL1 Programable Enable & Latching)
    // set CR1F[0] = 1
    SmbusDeviceCommand = 0x1F;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_1F error: %r\n", Status));
        return Status;
    }
    Data = Data & 0xFE;
    Data = Data | 0x01;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_1F error: %r\n", Status));
        return Status;
    }

    //program PLL3
    //3456-04-PDZ-06 if (Config->PcieClockControl == 0x05)
        //PCIE OC 5%
        //3456-04-PDZ-06 Data = 84;
    //3456-04-PDZ-06 else
        //PCIE NO OC
        //3456-04-PDZ-06 Data = 80;
    //3456-04-PDZ-06 +S
    Data = (UINT8)PcieVCO[Config->PcieClockControl];
    //3456-04-PDZ-06 +E
    SmbusDeviceCommand = 0x12;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
         DEBUG(( EFI_D_ERROR, "write clock Gen CR_22 error: %r\n", Status));
            return Status;
    }

	//3456-04-PDZ-06 +S
	//set CR13[2:0]
	SmbusDeviceCommand = 0x13;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
         DEBUG(( EFI_D_ERROR, "read clock Gen CR_13 error: %r\n", Status));
         return Status;
    }
    Data = Data & 0xF8;
	RegData8 = (UINT8)(PcieVCO[Config->PcieClockControl] >> 8);
	Data |= RegData8;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
         DEBUG(( EFI_D_ERROR, "write clock Gen CR_13 error: %r\n", Status));
         return Status;
    }
    //3456-04-PDZ-06 +E

    // Set PLL3 Programable Selection = 0 (PLL3 Programable by VCO code frequency setting)
    // CR23[3] = 0b
    SmbusDeviceCommand = 0x23;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_23 error: %r\n", Status));
        return Status;
    }
    Data = Data & 0x87;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_23 error: %r\n", Status));
        return Status;
    }
    
    // Set PLL3 VCO Code Mode = 1 (PLL3 Programable Enable & Latching)
    // set CR1F[2] = 1b
    SmbusDeviceCommand = 0x1F;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_1F error: %r\n", Status));
        return Status;
    }
    Data = Data & 0xFB;
    Data = Data | 0x04;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
         DEBUG(( EFI_D_ERROR, "write clock Gen CR_1F error: %r\n", Status));
         return Status;
    }
 
    //SATA setting
    //read CR_04
    SmbusDeviceCommand = 0x04;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_04 error: %r\n", Status));
        return Status;
    }
    //set CR_04[1:0]= 00b
    //PLL1---SSC off, PLL3---SSC off
    Data = Data & 0xFC;
    SmbusDeviceCommand = 0x04;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_04 error: %r\n", Status));
        return Status;
    }

    //Program CPU Divider, CR1C[6:4]
    SmbusDeviceCommand = 0x1C;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
         DEBUG(( EFI_D_ERROR, "read clock Gen CR_1F error: %r\n", Status));
         return Status;
    }
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
         DEBUG(( EFI_D_ERROR, "write clock Gen CR_1C error: %r\n", Status));
         return Status;
    }

    //Program PCI-E Divider, CR1D[2:0] = 010b
    // PCI-E = PLL3 / 8
    //Program SATA Divider, CR1D[6:4] = 011b
    // SATA = PLL2 fix 100
    SmbusDeviceCommand = 0x1D;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_1D error: %r\n", Status));
        return Status;
    }
    Data &= 0x88;
    Data |= 0x32;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_1D error: %r\n", Status));
        return Status;
    }

    //Program PCI Divider CR1E[6:4] = 011b
    // PCI = PLL2 / 9
    SmbusDeviceCommand = 0x1E;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_1E error: %r\n", Status));
        return Status;
    }
    Data &= 0x8F;
	Data |= 0x30;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_1E error: %r\n", Status));
        return Status;
    }

    // Set Divider Select by REG CR1F[1]=1b
    SmbusDeviceCommand = 0x1F;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_1F error: %r\n", Status));
        return Status;
    }
    Data = Data & 0xFD;
    Data = Data | 0x02;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "write clock Gen CR_1F error: %r\n", Status));
        return Status;
    }

    // CPU1_STOP_EN = 0 (CPU1 Free Running)
    // CR0A[1] = 0b
    SmbusDeviceCommand = 0x0A;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Read clock Gen CR_0A error: %r\n", Status));
        return Status;
    }
    Data &= 0xFD; //  set CA-0A[1] == 0;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Write clock Gen CR_0A error: %r\n", Status));
        return Status;
    }
    return EFI_SUCCESS;

}

EFI_STATUS ProgramRTM875CpuClkStoppable(IN EFI_PEI_SERVICES   **PeiServices,
                                            IN EFI_PEI_SMBUS_PPI     *SmbusPpi,
                                            IN BOOLEAN           Enable
                                           )
{
    EFI_SMBUS_DEVICE_COMMAND    SmbusDeviceCommand;
    UINT8  Data;
    EFI_STATUS Status;

    //set CR_0A[0]
    //read CR_0A
    SmbusDeviceCommand = 0x0A;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_0A error: %r\n", Status));
        return Status;
    }

    if (Enable){
        //enable CPU STOP, CR_0A[0] = 1b
        Data |= 0x01;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_0A error: %r\n", Status));
            return Status;
        }
    }
    else {
        //disable CPU STOP, CR_0A[1] = 0b
        Data &= ~0x01;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_0A error: %r\n", Status));
            return Status;
        }
   }

   return EFI_SUCCESS;
}

//3456-R17-PDZ-07 +S

EFI_STATUS ProgramRTM875OnBoard1394ClockControl(IN EFI_PEI_SERVICES   **PeiServices,
                                                             IN EFI_PEI_SMBUS_PPI     *SmbusPpi,
                                                             IN BOOLEAN           Enable
                                                             )
{
    EFI_SMBUS_DEVICE_COMMAND    SmbusDeviceCommand;
    UINT8  Data;
    EFI_STATUS Status;

    //set CR_02[4]
    //read CR_02
    SmbusDeviceCommand = 0x02;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "read clock Gen CR_02 error: %r\n", Status));
        return Status;
    }

    if (Enable){
        //enable PCI4 clock to enable OnBoard1394
        Data |= 0x10;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_02 error: %r\n", Status));
            return Status;
        }
    }
    else {
        //disable PCI4 clock to disable OnBoard1394
        Data &= ~0x10;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_02 error: %r\n", Status));
            return Status;
        }
   }

   return EFI_SUCCESS;
}
//3456-R17-PDZ-07 +E
