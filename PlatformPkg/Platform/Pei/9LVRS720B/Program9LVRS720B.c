////
#include "../ClockGen.h"
#include <CHX001Reg.h>

UINT8 Fsb100CpuOC[][2] = {{0x99, 0x40},{0x99, 0x54},{0x99, 0x68},{0x99, 0x7C},{0x98, 0x80},
                          {0x99, 0x94},{0x99, 0x98},{0x99, 0x9C},{0x99, 0xA0},{0x99, 0xA4},
                          {0x99, 0xA8},{0x99, 0xAC},{0x99, 0xB0},{0x99, 0xB4},{0x99, 0xB8},
                          {0x99, 0xBC},{0x99, 0xC0},{0x99, 0xC4},{0x99, 0xC8},{0x99, 0xCC},
                          {0x99, 0xD0},{0x99, 0xD4},{0x99, 0xD8},{0x99, 0xDC},{0x99, 0xE0}
                         };
UINT8 Fsb133CpuOC[][2] = {{0x99, 0x40},{0x99, 0x54},{0x99, 0x68},{0x99, 0x7C},{0x98, 0x80},
                          {0x99, 0x94},{0x99, 0x98},{0x99, 0x9C},{0x99, 0xA0},{0x99, 0xA4},
                          {0x99, 0xA8},{0x99, 0xAC},{0x99, 0xB0},{0x99, 0xB4},{0x99, 0xB8},
                          {0x99, 0xBC},{0x99, 0xC0},{0x99, 0xC4},{0x99, 0xC8},{0x99, 0xCC},
                          {0x99, 0xD0},{0x99, 0xD4},{0x99, 0xD8},{0x99, 0xDC},{0x99, 0xE0}
                         };
UINT8 Fsb166CpuOC[][2] = {{0x94, 0x3F},{0x94, 0x53},{0x94, 0x67},{0x94, 0x7A},{0x94, 0x90},
                          {0x94, 0x92},{0x94, 0x96},{0x94, 0x9A},{0x94, 0x9E},{0x94, 0xA2},
                          {0x94, 0xA6},{0x94, 0xAA},{0x94, 0xAE},{0x94, 0xB2},{0x94, 0xB6},
                          {0x94, 0xBA},{0x94, 0xBE},{0x94, 0xC2},{0x94, 0xC6},{0x94, 0xCA},
                          {0x94, 0xCE},{0x94, 0xD2},{0x94, 0xD6},{0x94, 0xDA},{0x94, 0xDE}
                         };
////

UINT8 Fsb200CpuOC[][2] = {{0x99, 0x40},{0x99, 0x54},{0x99, 0x68},{0x99, 0x7C},{0x98, 0x80},
                          {0x99, 0x94},{0x99, 0x98},{0x99, 0x9C},{0x99, 0xA0},{0x99, 0xA4},
                          {0x99, 0xA8},{0x99, 0xAC},{0x99, 0xB0},{0x99, 0xB4},{0x99, 0xB8},
                          {0x99, 0xBC},{0x99, 0xC0},{0x99, 0xC4},{0x99, 0xC8},{0x99, 0xCC},
                          {0x99, 0xD0},{0x99, 0xD4},{0x99, 0xD8},{0x99, 0xDC},{0x99, 0xE0}
                         };

UINT8 Fsb266CpuOC[][2] = {{0x99, 0xAA},{0x99, 0xC4},{0x99, 0xDF},{0x99, 0xF9},{0x58, 0x00},
                          {0x58, 0x04},{0x58, 0x09},{0x58, 0x0E},{0x58, 0x13},{0x58, 0x18},
                          {0x58, 0x1D},{0x58, 0x22},{0x58, 0x28},{0x58, 0x2D},{0x58, 0x32},
                          {0x58, 0x37},{0x58, 0x3C},{0x58, 0x41},{0x58, 0x46},{0x58, 0x4B},
                          {0x58, 0x50},{0x58, 0x56},{0x58, 0x5B},{0x58, 0x60},{0x58, 0x65}
                         };

UINT8 Fsb333CpuOC[][2] = {{0x9E, 0x40},{0x9E, 0x54},{0x9E, 0x68},{0x9E, 0x7C},{0x9E, 0x90},
                          {0x9E, 0x94},{0x9E, 0x98},{0x9E, 0x9C},{0x9E, 0xA0},{0x9E, 0xA4},
                          {0x9E, 0xA8},{0x9E, 0xAC},{0x9E, 0xB0},{0x9E, 0xB4},{0x9E, 0xB8},
                          {0x9E, 0xBC},{0x9E, 0xC0},{0x9E, 0xC4},{0x9E, 0xC8},{0x9E, 0xCC},
                          {0x9E, 0xD0},{0x9E, 0xD4},{0x9E, 0xD8},{0x9E, 0xDC},{0x9E, 0xE0}
                         };

UINT8 PcieOC[][2] =      {{0x99, 0x8F},{0x99, 0x94},{0x99, 0x98},{0x99, 0x9C},{0x98, 0xA0},
                          {0x99, 0xA4},{0x99, 0xA8},{0x99, 0xAC},{0x99, 0xB0},{0x99, 0xB4},
                          {0x99, 0xB8},{0x99, 0xBC},{0x99, 0xC0},{0x99, 0xC4},{0x99, 0xC8},
                          {0x99, 0xCC},{0x99, 0xD0},{0x99, 0xD4},{0x99, 0xD8},{0x99, 0xDC},
                          {0x99, 0xE0},{0x99, 0x8C},{0x99, 0x88},{0x99, 0x84},{0x99, 0x80},
                          {0x99, 0x7C}
                         };


EFI_STATUS PreSet9LVRS720BClkGen(IN EFI_PEI_SERVICES   **PeiServices,
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
        DEBUG((  EFI_D_ERROR, "write clock Gen CR_0C error: %r\n", Status));
        return Status;
    }

   return EFI_SUCCESS;
}

////
EFI_STATUS Set9LVRS720BIoOutputVoltage(IN EFI_PEI_SERVICES   **PeiServices,
                                                IN EFI_PEI_SMBUS_PPI     *SmbusPpi)
{
  
  EFI_SMBUS_DEVICE_COMMAND	  SmbusDeviceCommand;
  UINT8  Data;
  EFI_STATUS Status;

  //USB3_IO_VOUT=10b=800mv
  //CR0A[5:4] = 10b
  SmbusDeviceCommand = 0x0A;
  Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
  if (EFI_ERROR(Status)){
      DEBUG((  EFI_D_ERROR, "Read clock Gen CR_0A error: %r\n", Status));
      return Status;
  }
  Data &= 0xCF;
  Data |= 0x20;
  Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
  if (EFI_ERROR(Status)){
      DEBUG((  EFI_D_ERROR, "Write clock Gen CR_0A error: %r\n", Status));
      return Status;
  }

  //DOT96/SATA/SRC/CPU_IO_VOUT=10b=800mv
  //CR0B[7:0]=0AAh
  SmbusDeviceCommand = 0x0B;
  Data = 0xAA;
  Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
  if (EFI_ERROR(Status)){
      DEBUG((  EFI_D_ERROR, "Write clock Gen CR_0B error: %r\n", Status));
      return Status;
  }

  return EFI_SUCCESS;
}


EFI_STATUS Program9LVRS720BNoOC(IN EFI_PEI_SERVICES   **PeiServices,
                             IN EFI_PEI_SMBUS_PPI     *SmbusPpi)
{
    EFI_SMBUS_DEVICE_COMMAND    SmbusDeviceCommand;
    UINT8  Data;
    EFI_STATUS Status;

    Status = PreSet9LVRS720BClkGen(PeiServices, SmbusPpi);
    if (EFI_ERROR(Status))
        return Status;

	//set  Rx17[1:0]==00b when NO OC
	SmbusDeviceCommand = 0x17;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG((  EFI_D_ERROR, "Read clock Gen CR_17 error: %r\n", Status));
        return Status;
    }
    Data &= 0xFC; //  set CR-17[1:0] == 0;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Write clock Gen CR_17 error: %r\n", Status));
        return Status;
    }

	// CPU1_STOP_EN = 0 (CPU1 Free Running)
    // CR0A[1] = 0b
    SmbusDeviceCommand = 0x0A;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG((  EFI_D_ERROR, "Read clock Gen CR_0A error: %r\n", Status));
        return Status;
    }
    Data &= 0xFD; //  set CA-0A[1] == 0;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Write clock Gen CR_0A error: %r\n", Status));
        return Status;
    }

    Set9LVRS720BIoOutputVoltage(PeiServices,SmbusPpi);
    
    return EFI_SUCCESS;
}

EFI_STATUS Program9LVRS720BOC(IN EFI_PEI_SERVICES   **PeiServices,
                          IN EFI_PEI_SMBUS_PPI     *SmbusPpi,
                          IN SETUP_DATA *Config
                          )
{

    EFI_SMBUS_DEVICE_COMMAND    SmbusDeviceCommand;
    UINT8  Data;
    EFI_STATUS Status;
	UINT8  RegData;

    Status = PreSet9LVRS720BClkGen(PeiServices, SmbusPpi);
    if (EFI_ERROR(Status))
        return Status;

	// CPU1_STOP_EN = 0 (CPU1 Free Running)
    // CR0A[1] = 0b
    SmbusDeviceCommand = 0x0A;
    Status = ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG((  EFI_D_ERROR, "Read clock Gen CR_0A error: %r\n", Status));
        return Status;
    }
    Data &= 0xFD; //  set CA-0A[1] == 0;
    Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    if (EFI_ERROR(Status)){
        DEBUG(( EFI_D_ERROR, "Write clock Gen CR_0A error: %r\n", Status));
        return Status;
    }

    Set9LVRS720BIoOutputVoltage(PeiServices,SmbusPpi);

	//program CPU OC
	//disalbe M/N enable (Rx17[1]=0b)
	SmbusDeviceCommand = 0x17;
    ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
	Data &= ~0x02;
	ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);

#if 0
	(**PeiServices).PciCfg->Read (
                                  PeiServices,
                                  (**PeiServices).PciCfg,
                                  PeiPciCfgWidthUint8,
                                  VX11_V4IF| D0F2_CPU_IF_CTL_FSB_FREQ_CTL_BY_STRAPPING,  
                                  &RegData
                                  );
#else
    #define D0F2_RCKMD_2_0 0x10
	RegData = 0x10;    
#endif
	//Cpu Non OC
	if (Config->CpuClockControl == 4){
		//enable Cpu PLL Spread (Rx04[1] = 1)
		SmbusDeviceCommand = 0x04;
        ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
	    Data |= 0x02;
	    ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
	}
    //Cpu OC
    else {
	    //program CPU PLL M/N (Rx0F, Rx0E)
        if ((RegData & D0F2_RCKMD_2_0) == D0F2_RCKMD_100MHZ){
		    SmbusDeviceCommand = 0x0E;
	        Data =Fsb100CpuOC[Config->CpuClockControl][0];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		    SmbusDeviceCommand = 0x0F;
	        Data =Fsb100CpuOC[Config->CpuClockControl][1];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
            //set Cpu Divider Ratio programming = 4 (Rx1C = ?)
            //SmbusDeviceCommand = 0x1C;
		    //Data = 0x00;
		    //ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);	
        }
	else if ((RegData & D0F2_RCKMD_2_0) == D0F2_RCKMD_133MHZ){
		    SmbusDeviceCommand = 0x0E;
	        Data =Fsb133CpuOC[Config->CpuClockControl][0];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		    SmbusDeviceCommand = 0x0F;
	        Data =Fsb133CpuOC[Config->CpuClockControl][1];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
            //set Cpu Divider Ratio programming = 3 (Rx1C = ?)
            //SmbusDeviceCommand = 0x1C;
		    //Data = 0x00;
		    //ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);	
        }
		else if ((RegData & D0F2_RCKMD_2_0) == D0F2_RCKMD_166MHZ){
		    SmbusDeviceCommand = 0x0E;
	        Data =Fsb166CpuOC[Config->CpuClockControl][0];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		    SmbusDeviceCommand = 0x0F;
	        Data =Fsb166CpuOC[Config->CpuClockControl][1];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
            //set Cpu Divider Ratio programming = 3 (Rx1C = ?)
            //SmbusDeviceCommand = 0x1C;
		    //Data = 0x00;
		    //ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);	
        }
		else if ((RegData & D0F2_RCKMD_2_0) == D0F2_RCKMD_200MHZ){
		    SmbusDeviceCommand = 0x0E;
	        Data =Fsb200CpuOC[Config->CpuClockControl][0];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		    SmbusDeviceCommand = 0x0F;
	        Data =Fsb200CpuOC[Config->CpuClockControl][1];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
            //set Cpu Divider Ratio programming = 2 (Rx1C = 0)
            //SmbusDeviceCommand = 0x1C;
		    //Data = 0x00;
		    //ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);	
        }
	    else if ((RegData & D0F2_RCKMD_2_0) == D0F2_RCKMD_266MHZ){
		    SmbusDeviceCommand = 0x0E;
	        Data =Fsb266CpuOC[Config->CpuClockControl][0];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		    SmbusDeviceCommand = 0x0F;
	        Data =Fsb266CpuOC[Config->CpuClockControl][1];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
            //set Cpu Divider Ratio programming = 2 (Rx1C = 0)
            //SmbusDeviceCommand = 0x1C;
		    //Data = 0x00;
		    //ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        }
		else if ((RegData & D0F2_RCKMD_2_0) == D0F2_RCKMD_333MHZ){
		    SmbusDeviceCommand = 0x0E;
	        Data =Fsb333CpuOC[Config->CpuClockControl][0];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		    SmbusDeviceCommand = 0x0F;
	        Data =Fsb333CpuOC[Config->CpuClockControl][1];
	        ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
            //set Cpu Divider Ratio programming = 1 (Rx1C = 1)
            //SmbusDeviceCommand = 0x1C;
		    //Data = 0x05;
		    //ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        }
	    //Enable CPU M/N (Rx17[1]=1b)
		SmbusDeviceCommand = 0x17;
        ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
	    Data |= 0x02;
	    ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		//Disable Cpu PLL Spread (Rx04[1] = 0)
		SmbusDeviceCommand = 0x04;
        ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
	    Data &= ~0x02;
	    ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
    }

	//program PCIE
	//disable PCIE MN (Rx17[0]=0)
	SmbusDeviceCommand = 0x17;
    ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
	Data &= ~0x01;
	ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);

	//PCIE NO OC
	if (Config->PcieClockControl == 0x00){
		//enable SRC(PCIE) PLL Spread (Rx04[0] = 1)
		SmbusDeviceCommand = 0x04;
        ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
	    Data |= 0x01;
	    ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);	
	}
	else {
		//Program SRC(PCIE) PLL MN (Rx12, Rx13)
		SmbusDeviceCommand = 0x12;
	    Data =PcieOC[Config->PcieClockControl][0];
	    ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		SmbusDeviceCommand = 0x13;
	    Data =PcieOC[Config->PcieClockControl][1];
	    ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		//set SRC diverder = 4 (Rx1D=0x02)
		//SmbusDeviceCommand = 0x1D;
		//ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		//Data &= ~0x38;
	    //Data |= 0x02;
	    //ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		//enable SRC M/N (Rx17[0]=1b)
		SmbusDeviceCommand = 0x17;
        ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
	    Data |= 0x01;
	    ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
		//disable SRC(PCIE) PLL Spread (Rx04[0] = 0)
		SmbusDeviceCommand = 0x04;
        ClockGenReadByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
	    Data &= ~0x01;
	    ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
	}
    
    return EFI_SUCCESS;

}

EFI_STATUS Program9LVRS720BCpuClkStoppable(IN EFI_PEI_SERVICES   **PeiServices,
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
        DEBUG((  EFI_D_ERROR, "read clock Gen CR_0A error: %r\n", Status));
        return Status;
    }

    if (Enable){
        //enable CPU STOP, CR_0A[0] = 1b
        Data |= 0x01;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG((  EFI_D_ERROR, "write clock Gen CR_0A error: %r\n", Status));
            return Status;
        }
    }
    else {
        //disable CPU STOP, CR_0A[0] = 0b
        Data &= ~0x01;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_0A error: %r\n", Status));
            return Status;
        }
   }

   return EFI_SUCCESS;
}

///
EFI_STATUS Program9LVRS720BOnBoard1394ClockControl(IN EFI_PEI_SERVICES   **PeiServices,
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
        DEBUG((  EFI_D_ERROR, "read clock Gen CR_02 error: %r\n", Status));
        return Status;
    }

    if (Enable){
        //enable PCI4 clock to enable OnBoard1394
        Data |= 0x10;
        Status = ClockGenWriteByte(PeiServices, SmbusPpi,SmbusDeviceCommand, &Data);
        if (EFI_ERROR(Status)){
           DEBUG((  EFI_D_ERROR, "write clock Gen CR_02 error: %r\n", Status));
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

///
#define IDT_9LVRS720B_BYTE13	0x0D
#define IDT_720B_CPU_SR		BIT7

VOID ProgramCpuSlewRate(
    IN EFI_PEI_SERVICES   **PeiServices,
    IN EFI_PEI_SMBUS_PPI 	*pSmbusPpi,
    IN UINT8 CPUSlewRate)
{
    ///
    EFI_STATUS Status;
    UINT8	SMBData;
    UINTN						   SMBDataLength;
    EFI_SMBUS_DEVICE_COMMAND		SMBCMD;
    EFI_SMBUS_DEVICE_ADDRESS 	   SlaveAddress;
    //EFI_CPUID_REGISTER Cpuid;
	UINT32						   RegEax,RegEbx;
	

    SlaveAddress.SmbusDeviceAddress = 0xD2>>1;
    SMBDataLength = 0x01;

    SMBData = 0;
    SMBCMD = IDT_9LVRS720B_BYTE13;
    //Status = pSmbusPpi->Execute(PeiServices, pSmbusPpi, SlaveAddress, SMBCMD, EfiSmbusReadBlock, 0, &SMBDataLength, &SMBData);
    Status = ClockGenReadByte(PeiServices, pSmbusPpi,SMBCMD, &SMBData);
    DEBUG(( EFI_D_ERROR, "%a(%d) ClkGen Function -> SMBData : %x \n", __FILE__, __LINE__, SMBData));

    //EfiCpuid(1, &Cpuid);
    AsmCpuid(1, &RegEax, &RegEbx, NULL, NULL);

    if( CPUSlewRate==0 )			// Auto //
    {
        // Only CNQ 3&4 Core set to Slow //
        if( (((RegEax) & 0xFFF) >= 0x6FC) && ((((RegEbx)>>16)&0xFF)>=04) )
            SMBData &= ~IDT_720B_CPU_SR;	// Slow //
        else
            SMBData |= IDT_720B_CPU_SR;		// Fast //
    }
    else if( CPUSlewRate==1 )		// Slow //
    {
        SMBData &= ~IDT_720B_CPU_SR;
    }
    else if( CPUSlewRate==2 )		// Fast //
    {
        SMBData |= IDT_720B_CPU_SR;
    }

    DEBUG(( EFI_D_ERROR, "%a(%d) new SMBData : %x \n", __FILE__, __LINE__, SMBData));
    ///
    SMBCMD = IDT_9LVRS720B_BYTE13;
    Status = ClockGenWriteByte(PeiServices, pSmbusPpi,SMBCMD, &SMBData);
    //Status = pSmbusPpi->Execute(PeiServices, pSmbusPpi, SlaveAddress, SMBCMD, EfiSmbusWriteBlock, 0, &SMBDataLength, &SMBData);
    ASSERT_EFI_ERROR( Status);
    ///

}
////

