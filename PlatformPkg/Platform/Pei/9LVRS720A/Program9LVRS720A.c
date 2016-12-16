////
#include "../ClockGen.h"

EFI_STATUS PreSet9LVRS720AClkGen(IN EFI_PEI_SERVICES   **PeiServices,
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


EFI_STATUS Program9LVRS720ANoOC(IN EFI_PEI_SERVICES   **PeiServices,
                             IN EFI_PEI_SMBUS_PPI     *SmbusPpi)
{
    EFI_SMBUS_DEVICE_COMMAND    SmbusDeviceCommand;
    UINT8  Data;
    EFI_STATUS Status;

    //if NO OC, 
    // PLL1----CPU/PCIE/USB3/PCI(SSC on - Down 0.4)
    // PLL2----USB(NON SSC)
    // PLL3----SATA(NON SSC)

    Status = PreSet9LVRS720AClkGen(PeiServices, SmbusPpi);
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
        DEBUG((  EFI_D_ERROR, "Write clock Gen CR_0A error: %r\n", Status));
        return Status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS Program9LVRS720AOC(IN EFI_PEI_SERVICES   **PeiServices,
                          IN EFI_PEI_SMBUS_PPI     *SmbusPpi,
                          IN SETUP_DATA *Config
                          )
{

    EFI_SMBUS_DEVICE_COMMAND    SmbusDeviceCommand;
    UINT8  Data;
    EFI_STATUS Status;

    //if OC, 
    // PLL1----CPU/PCI(NON SSC)
    // PLL2----SATA/USB2(NON SSC)
    // PLL3----PCIE/USB3(NON SSC)

    Status = PreSet9LVRS720AClkGen(PeiServices, SmbusPpi);
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
	
    return EFI_SUCCESS;

}

EFI_STATUS Program9LVRS720ACpuClkStoppable(IN EFI_PEI_SERVICES   **PeiServices,
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
            DEBUG(( EFI_D_ERROR, "write clock Gen CR_0A error: %r\n", Status));
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
EFI_STATUS Program9LVRS720AOnBoard1394ClockControl(IN EFI_PEI_SERVICES   **PeiServices,
                                                             IN EFI_PEI_SMBUS_PPI     *SmbusPpi,
                                                             IN BOOLEAN           Enable
                                                             )
{
   return EFI_SUCCESS;
}
///

