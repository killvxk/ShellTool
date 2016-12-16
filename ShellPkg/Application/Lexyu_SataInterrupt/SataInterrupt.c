/** @file
  This is a test application that demonstrates how to use the C-style entry point
  for a shell application.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/ShellCEntryLib.h>



#include "SataInterrupt.h"


#define PCI_DEV_MMBASE(Bus, Device, Function) \
    ( \
      (UINTN)PcdGet64(PcdPciExpressBaseAddress) + (UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) \
        (Function << 12) \
    )
    
/*#define ATA_Device_DATA_FROM_THIS(a) \
	  CR (a, \
		  ATA_ATAPI_PASS_THRU_INSTANCE, \
		  AtaPassThru, \
		  ATA_ATAPI_PASS_THRU_SIGNATURE \
		  )
*/

#define ATA_DEVICE_SIGNATURE  SIGNATURE_32 ('A', 'B', 'I', 'D')

#define ATA_DEVICE_FROM_BLOCK_IO(a)         CR (a, ATA_DEVICE, BlockIo, ATA_DEVICE_SIGNATURE)

#define LPC_PCI_REG(Reg)          (PCI_DEV_MMBASE(0, 17, 0)+Reg)
#define APIC_PCI_REG(Reg)         (PCI_DEV_MMBASE(0,  0, 5)+Reg)
#define D2F0_REG(Reg)             (PCI_DEV_MMBASE(0,  2, 0)+Reg)

#define AHCI_GHC  0x04
#define AHCI_IS   0x08
#define AHCI_PORT_START                    0x0100
#define AHCI_PORT_REG_WIDTH                0x0080
#define AHCI_PORT_IE                       0x14
#define AHCI_PORT_IS                       0x10
#define DPE_INT                            (1<<5)
#define DHRE_INT                           (1)
#define EFI_AHCI_BAR_INDEX                  5

EFI_PCI_IO_PROTOCOL *gPciIo=NULL;
BOOLEAN HasEnterInterrupt = FALSE;

UINT32
EFIAPI
AhciReadReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;

  ASSERT (PciIo != NULL);
  
  Data = 0;

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint32,
               EFI_AHCI_BAR_INDEX,
               (UINT64) Offset,
               1,
               &Data
               );

  return Data;
}

/**
  Write AHCI Operation register.

  @param  PciIo        The PCI IO protocol instance.
  @param  Offset       The operation register offset.
  @param  Data         The data used to write down.

**/
VOID
EFIAPI
AhciWriteReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  ASSERT (PciIo != NULL);

  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint32,
               EFI_AHCI_BAR_INDEX,
               (UINT64) Offset,
               1,
               &Data
               );

  return ;
}
VOID
Ahci_APIC_Test(EFI_BLOCK_IO_PROTOCOL 	*BlockIo,UINT8*  pRBuffer)
{
	EFI_STATUS   Status;
	UINTN    BufSize = 512;
	DEBUG ((EFI_D_ERROR, __FUNCTION__"\n"));
	if(BlockIo==NULL){
		DEBUG ((EFI_D_ERROR, "BlockIo is NULL\n"));
		return;
	}
	Status = BlockIo->ReadBlocks(
		                        BlockIo,
		                        0,
		                        0,
		                        BufSize,
		                        pRBuffer
		                        );
	DEBUG ((EFI_D_ERROR, "Status %x \n",Status));
	if(Status!=0){
		DEBUG((EFI_D_ERROR,"mike_ReadBlocks_error:%x\n",Status));
	}
}
UINT32 mPort = 0;
VOID Clear_Interrupt()
{
//Clear PxIs bit0(Device to Host Register FIS Interrupt), bit5(Descriptor Processed)
  UINT32 Value32 = 0;
  while(Value32!=0x21){
  Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+mPort*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS);
  Value32&=0x21;
  }
  Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+mPort*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS);
  AhciWriteReg(gPciIo,AHCI_PORT_START+mPort*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS,Value32);
  Value32 = AhciReadReg(gPciIo,AHCI_IS);
  AhciWriteReg(gPciIo,AHCI_IS,Value32);//Clear IS all bits
  HasEnterInterrupt = TRUE;
}
VOID
EFIAPI
AHCI_ReadInterruptHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
	DEBUG ((EFI_D_ERROR, __FUNCTION__"\n"));

  Clear_Interrupt();
  // Local APIC EOI
  SendApicEoi ();

  
}
EFI_LEGACY_8259_PROTOCOL  *gLegacy8259;

VOID
EFIAPI
AHCI_PIC_ReadInterruptHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  Clear_Interrupt();
  gLegacy8259->EndOfInterrupt (gLegacy8259, 0xB);//send an EOI to 8259
  
}

VOID
EFIAPI
MSI_ReadInterruptHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  Clear_Interrupt();
  // Local APIC EOI
  SendApicEoi ();

  
}

VOID
EFIAPI
MSIX_ReadInterruptHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
 Clear_Interrupt();
  // Local APIC EOI
  SendApicEoi ();
  
}


VOID
EFIAPI
MMSI_ReadInterruptHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  Clear_Interrupt();
  // Local APIC EOI
  SendApicEoi ();

  
}

#define NB_IO_APIC_BASE  0xFECC0000
#define SB_IO_APIC_BASE  0xFEC00000
#define NB_IO_APIC_ID    10
#define SB_IO_APIC_ID     9
UINTN  IO_Apic_Base;
UINT32
EFIAPI
Mike_IoApicRead (
  IN UINTN  Index
  )
{
  ASSERT (Index < 0x100);
  MmioWrite8 (IO_Apic_Base + IOAPIC_INDEX_OFFSET, (UINT8)Index);
  return MmioRead32 (IO_Apic_Base + IOAPIC_DATA_OFFSET);
}
UINT32
EFIAPI
Mike_IoApicWrite (
  IN UINTN   Index,
  IN UINT32  Value
  )
{
  ASSERT (Index < 0x100);
  MmioWrite8 (IO_Apic_Base + IOAPIC_INDEX_OFFSET, (UINT8)Index);
  return MmioWrite32 (IO_Apic_Base + IOAPIC_DATA_OFFSET, Value);
}
VOID
EFIAPI
Mike_IoApicEnableInterrupt (
  IN UINTN    Irq,
  IN BOOLEAN  Enable
  )
{
  IO_APIC_VERSION_REGISTER         Version;
  IO_APIC_REDIRECTION_TABLE_ENTRY  Entry;

  Version.Uint32 = Mike_IoApicRead (IO_APIC_VERSION_REGISTER_INDEX);
  ASSERT (Version.Bits.MaximumRedirectionEntry < 0xF0);
  ASSERT (Irq <= Version.Bits.MaximumRedirectionEntry);

  Entry.Uint32.Low = Mike_IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2);
  Entry.Bits.Mask = Enable ? 0 : 1;
  Mike_IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2, Entry.Uint32.Low);
}
VOID
EFIAPI
Mike_IoApicConfigureInterrupt (
  IN UINTN    Irq,
  IN UINTN    Vector,
  IN UINTN    DeliveryMode,
  IN BOOLEAN  LevelTriggered,
  IN BOOLEAN  AssertionLevel
  )
{
  IO_APIC_VERSION_REGISTER         Version;
  IO_APIC_REDIRECTION_TABLE_ENTRY  Entry;

  Version.Uint32 = Mike_IoApicRead (IO_APIC_VERSION_REGISTER_INDEX);//Zoey: Read Index=01,which contain APIC Version and MaximumRedirectionEntry
  ASSERT (Version.Bits.MaximumRedirectionEntry < 0xF0);
  ASSERT (Irq <= Version.Bits.MaximumRedirectionEntry);
  ASSERT (Vector <= 0xFF);
  ASSERT (DeliveryMode < 8 && DeliveryMode != 6 && DeliveryMode != 3);

  //Zoey:Set RT entry low 32 bits
  Entry.Uint32.Low = Mike_IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2);
  Entry.Bits.Vector          = (UINT8)Vector;
  Entry.Bits.DeliveryMode    = (UINT32)DeliveryMode;
  Entry.Bits.DestinationMode = 0; 
  Entry.Bits.Polarity        = AssertionLevel ? 0 : 1;
  Entry.Bits.TriggerMode     = LevelTriggered ? 1 : 0;
  Entry.Bits.Mask            = 1;
  Mike_IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2, Entry.Uint32.Low);

  //Zoey:Set RT entry high 32 bits
  Entry.Uint32.High = Mike_IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2 + 1);
  Entry.Bits.DestinationID = GetApicId ();
  Entry.Uint32.High&=~(0xFF0000);
  Mike_IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2 + 1, Entry.Uint32.High);
}
#define IS_CND001_Board TRUE
#define IS_CHX001_Board FALSE
/*#define PCI_DEV_MMBASE(Bus, Device, Function) \
    ( \
      (UINTN)0xE0000000 + (UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) \
        (Function << 12) \
    )*/
UINT16 Find_Device_PCIEPort(EFI_DEVICE_PATH_PROTOCOL  *DevicePath,UINT8 INTX)
{
	UINT8 Value8;
	UINTN Device= 0;
	UINTN Func = 0;
	UINTN PortFunc = 0;
	UINTN PortDev = 0;
	//UINT32 ClassCode;
	//UINT8  SecondNum;
	UINT16 IrqNum = 0;
	EFI_STATUS		Status;
	UINT8			Rintrtapic;
	//EFI_STATUS   Status;
	//EFI_ACPI_SUPPORT_PROTOCOL 					*AcpiSupport=NULL;
	//EFI_ACPI_COMMON_HEADER  *Table=NULL;
	UINT32 BridgeCount = 0;
	UINT8  TempINTX;
	//CHAR8* Sign;	 
	//UINTN 				 Handle;
	UINTN	  i = 0;
	UINT8 DeviceList[0x100];
	while(!IsDevicePathEnd(DevicePath)){
		DevicePath = NextDevicePathNode(DevicePath);
		if((DevicePath->Type==0x1)&&(DevicePath->Type==0x1)){
			PortDev = *((UINT8*)((UINT8*)DevicePath+0x5))&0xff;
			PortFunc = *((UINT8*)((UINT8*)DevicePath+0x4))&0xff;
			if(BridgeCount==0){
				Device = PortDev;
				Func = PortFunc;
			}
			DeviceList[BridgeCount] = (UINT8)PortDev;
			BridgeCount++;
		}
			
	}
/*	for(Device=0;Device<0xff;Device++){
		for(Func=0;Func<0x8;Func++){
		  ClassCode = MmioRead32(PCI_DEV_MMBASE(0,	Device, Func)+0x08);
		  ClassCode&=~0xff;
		  if(ClassCode==0x06040000){
		  SecondNum = MmioRead8(PCI_DEV_MMBASE(0,  Device, Func)+0x19);
		  if(SecondNum==((UINT8)BusNumber)){
			 IsFound = TRUE;
			 goto Found;
			}
		  }
		}
	}*/
	DEBUG((EFI_D_ERROR,"Device In PCIE Port:%x-%x\n",Device,Func));
	if(BridgeCount>2){
		TempINTX = INTX;
		DEBUG((EFI_D_ERROR,"Device Behind in PCIE-Bridge:"));
		for(i=BridgeCount-1;i>1;i--){
			TempINTX = ((TempINTX + DeviceList[i]) % 4);
		}
		DEBUG((EFI_D_ERROR,"INT%x Chang to INT%x\n",0xA+INTX,0xA+TempINTX));
		INTX = TempINTX;
	}
	if(IS_CND001_Board){
		if(Device==0x2&&Func==0){
			IrqNum = 0x18+INTX;
		}
		else if(Device==0x3&&Func==0){
			IrqNum = 0x18+4+INTX;
		}
		else if(Device==0x4&&Func==0){
			IrqNum = 0x18+8+INTX;
		}
		else if(Device==0x5&&Func==0){
			IrqNum = 0x18+12+INTX;
		}
		else if(Device==0x6&&Func==0){
			IrqNum = 0x18+16+INTX;
		}
		else if(Device==0x7&&Func==0){
			if(INTX==0x00||INTX==0x01){
				Value8 = 0x3;
				Status = gPciIo->Pci.Read (
						gPciIo,
						EfiPciIoWidthUint8,
						0x9B,
						1,
						&Value8
						);
				Status = gPciIo->Pci.Read (
						gPciIo,
						EfiPciIoWidthUint8,
						0x3D,
						1,
						&Value8
						);
			if(Value8==0x1||Value8==0x2){
				DEBUG((EFI_D_ERROR,"Can't Change INx.Can't Test APIC in Port %x-%x\n",Device,Func));
				IrqNum = 0;
				return IrqNum;
			}
			else{
				IrqNum = 0x18+22;
			}
			}
		}
	  }
	  if(IS_CHX001_Board){
		if(Device==0x2&&Func==0){
			Rintrtapic = MmioRead8(PCI_DEV_MMBASE(0,0,0)+0x362);
			DEBUG((EFI_D_ERROR,"Rintrtapic:%x\n",((Rintrtapic&0x30)>>4)));
			if(((Rintrtapic&0x30)>>4)==0x00){
				 IrqNum = 0x18+8+INTX;
			}
			else if(((Rintrtapic&0x30)>>4)==0x01){
				if(INTX==0x3){
				  IrqNum =0x18+8;
				}
				else{
					IrqNum = 0x18+9+INTX;
				}
				
			}
			else if(((Rintrtapic&0x30)>>4)==0x2){
				if(INTX==0x0){
				  IrqNum =0x18+10;
				}
				else if(INTX==0x01){
					IrqNum =0x18+11;
				}
				else if(INTX==0x02){
					IrqNum =0x18+8;
				}
				else if(INTX==0x03){
					IrqNum =0x18+9;
				}
			}
			else if(((Rintrtapic&0x30)>>4)==0x3){
				if(INTX==0x0){
				  IrqNum =0x18+11;
				}
				else if(INTX==0x01){
					IrqNum =0x18+8;
				}
				else if(INTX==0x02){
					IrqNum =0x18+9;
				}
				else if(INTX==0x03){
					IrqNum =0x18+10;
				}
			}
		}
		else if(Device==0x3&&Func==0){
			IrqNum = 0x18+4+INTX;
		}
		else if(Device==0x3&&Func==1){
			IrqNum = 0x18+8+INTX;
		}
		else if(Device==0x3&&Func==2){
			IrqNum = 0x18+12+INTX;
		}
		else if(Device==0x3&&Func==3){
			IrqNum = 0x18+16+INTX;
		}
		else if(Device==0x3&&Func==3){
			IrqNum = 0x18+16+INTX;
		}
		else if(Device==0x4&&Func==0){
			IrqNum = 0x18+INTX;
		}
		else if(Device==0x4&&Func==1){
			if(INTX==0x00||INTX==0x01){
				Value8 = 0x3;
				Status = gPciIo->Pci.Read (
						gPciIo,
						EfiPciIoWidthUint8,
						0x9B,
						1,
						&Value8
						);
				Status = gPciIo->Pci.Read (
						gPciIo,
						EfiPciIoWidthUint8,
						0x3D,
						1,
						&Value8
						);
			if(Value8==0x1||Value8==0x2){
				DEBUG((EFI_D_ERROR,"Can't Change INx.\nCan't Test APIC in Port %x-%x\n",Device,Func));
				IrqNum = 0;
				return IrqNum;
			}
			else{
				DEBUG((EFI_D_ERROR,"Change to INTC\n"));
				IrqNum = 0x18+22;
			}
		   }
		}
		else if(Device==0x5&&Func==0){
			Rintrtapic = MmioRead8(PCI_DEV_MMBASE(0,0,0)+0x360);
			DEBUG((EFI_D_ERROR,"Rintrtapic:%x\n",((Rintrtapic&0x30)>>4)));
			if(((Rintrtapic&0x30)>>4)==0x00){
				 IrqNum = 0x18+INTX;
			}
			else if(((Rintrtapic&0x30)>>4)==0x01){
				if(INTX==0x3){
				  IrqNum =0x18;
				}
				else{
					IrqNum = 0x18+1+INTX;
				}
				
			}
			else if(((Rintrtapic&0x30)>>4)==0x2){
				if(INTX==0x0){
				  IrqNum =0x18+2;
				}
				else if(INTX==0x01){
					IrqNum =0x18+3;
				}
				else if(INTX==0x02){
					IrqNum =0x18;
				}
				else if(INTX==0x03){
					IrqNum =0x18+1;
				}
			}
			else if(((Rintrtapic&0x30)>>4)==0x3){
				if(INTX==0x0){
				  IrqNum =0x18+3;
				}
				else if(INTX==0x01){
					IrqNum =0x18;
				}
				else if(INTX==0x02){
					IrqNum =0x18+1;
				}
				else if(INTX==0x03){
					IrqNum =0x18+2;
				}
			}
		}
		else if(Device==0x5&&Func==1){
			Rintrtapic = MmioRead8(PCI_DEV_MMBASE(0,0,0)+0x361);
			DEBUG((EFI_D_ERROR,"Rintrtapic:%x\n",((Rintrtapic&0x30)>>4)));
			if(((Rintrtapic&0x30)>>4)==0x00){
				 IrqNum = 0x18+4+INTX;
			}
			else if(((Rintrtapic&0x30)>>4)==0x01){
				if(INTX==0x3){
				  IrqNum =0x18+4;
				}
				else{
					IrqNum = 0x18+5+INTX;
				}
				
			}
			else if(((Rintrtapic&0x30)>>4)==0x2){
				if(INTX==0x0){
				  IrqNum =0x18+6;
				}
				else if(INTX==0x01){
					IrqNum =0x18+7;
				}
				else if(INTX==0x02){
					IrqNum =0x18+4;
				}
				else if(INTX==0x03){
					IrqNum =0x18+5;
				}
			}
			else if(((Rintrtapic&0x30)>>4)==0x3){
				if(INTX==0x0){
				  IrqNum =0x18+7;
				}
				else if(INTX==0x01){
					IrqNum =0x18+4;
				}
				else if(INTX==0x02){
					IrqNum =0x18+5;
				}
				else if(INTX==0x03){
					IrqNum =0x18+6;
				}
			}
		}
	  }
	
	return	IrqNum;
}

void
Ahci_ConfigMMSI(
     UINT8 Capability,
     UINT8 IrqNum
)
{
	EFI_STATUS                     Status;
	UINT32                         Address;
	UINT32                         Value;
	UINT16                         Value16;
	UINT16                         Tmp;
    // Program MSI Address and MSI Data values in the selected AHCI
    //
    Address = GetApicMsiAddress ();
    Value   = (UINT32)GetApicMsiValue ((UINT8)(0x80+IrqNum), LOCAL_APIC_DELIVERY_MODE_FIXED,FALSE,FALSE);
    Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint32,
                        Capability+0x4,
                        1,
                        &Address
                        );
	if(Status!=0){
		DEBUG((EFI_D_ERROR,"Pci Write:%x Error\n",Capability+0x4));
	}
	Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint32,
                        Capability+0x8,
                        1,
                        &Value
                        );
	if(Status!=0){
		DEBUG((EFI_D_ERROR,"Pci Write:%x Error\n",Capability+0x8));
	}
    Status = gPciIo->Pci.Read(
                        gPciIo,
                        EfiPciIoWidthUint16,
                        Capability+0x2,
                        1,
                        &Value16
                        );
	if((Value16&0xE)==0x0){
		DEBUG((EFI_D_ERROR,"Device can't support MSI\n"));
	}
	else{
		Tmp = Value16&0xE;
		DEBUG((EFI_D_ERROR,"Device Can Support 2^%x MSI\n",Tmp>>1));
		Value16|=(Tmp<<3);
	}
	Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint16,
                        Capability+0x2,
                        1,
                        &Value16
                        );
}

void
Ahci_ConfigMSI(
     UINT8 Capability,
     UINT8 IrqNum
)
{
	EFI_STATUS                     Status;
	UINT32                         Address;
	UINT32                         Value;
    // Program MSI Address and MSI Data values in the selected HPET Timer
    //
    Address = GetApicMsiAddress ();
    Value   = (UINT32)GetApicMsiValue ((UINT8)(0x80+IrqNum), LOCAL_APIC_DELIVERY_MODE_FIXED,FALSE,FALSE);
    Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint32,
                        Capability+0x4,
                        1,
                        &Address
                        );
	if(Status!=0){
		DEBUG((EFI_D_ERROR,"Pci Write:%x Error\n",Capability+0x4));
	}
	Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint32,
                        Capability+0x8,
                        1,
                        &Value
                        );
	if(Status!=0){
		DEBUG((EFI_D_ERROR,"Pci Write:%x Error\n",Capability+0x8));
	}
}
VOID
Ahci_ConfigMSIX(
     UINT32 table_Address,
     UINT8 IrqNum,
     BOOLEAN Open
)
{
	

	UINT32                         Address;
	UINT32                         Value;
	DEBUG ((EFI_D_INFO, __FUNCTION__"\n"));
    // Program MSI Address and MSI Data values in the selected HPET Timer
    //
    Address = GetApicMsiAddress ();
    Value   = (UINT32)GetApicMsiValue ((UINT8)(0x80+IrqNum), LOCAL_APIC_DELIVERY_MODE_FIXED,FALSE,FALSE);
    MmioWrite32(table_Address,Address);
	MmioWrite32(table_Address+0x4,0x0);
	MmioWrite32(table_Address+0x8,Value);
	if(Open){
		MmioWrite32(table_Address+0xC,0x0);
	}
	else{
	    MmioWrite32(table_Address+0xC,0x1);
	}
	
}

void
Ahci_EnableMSIInterrupt (
  IN BOOLEAN				Enable,
  IN UINT8                  Capability
  
  )
{
   EFI_STATUS   Status;
   UINT16       Value16;
   
   Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        Capability+0x2,
                        1,
                        &Value16
                        );
    if(Enable){
		Value16|=1;
    }
	else{
		Value16&=~(1);
	}
	Status = gPciIo->Pci.Write (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        Capability+0x2,
                        1,
                        &Value16
                        );
}
void
Ahci_EnableMSIXInterrupt (
  IN BOOLEAN				Enable,
  IN UINT8                  Capability
  )
{
   EFI_STATUS   Status;
   UINT16       Value8;
   DEBUG ((EFI_D_INFO, __FUNCTION__"\n"));
   Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint8,
                        Capability+0x3,
                        1,
                        &Value8
                        );
    if(Enable){
		Value8|=(1<<7);
		Value8&=~(1<<6);
    }
	else{
		Value8&=~(1<<7);
		//Value8|=(1<<6);
	}
	Status = gPciIo->Pci.Write (
                        gPciIo,
                        EfiPciIoWidthUint8,
                        Capability+0x3,
                        1,
                        &Value8
                        );
	
}

void
Ahci_EnableMMSIInterrupt (
  IN BOOLEAN				Enable,
  IN UINT8                  Capability
  
  )
{
   EFI_STATUS   Status;
   UINT16       Value16;
   UINT32       Value32;
   
   Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        Capability+0x2,
                        1,
                        &Value16
                        );
    if(Enable){
		Value16|=1;
    }
	else{
		//Value16&=~(1);
		Value16&=~(0x71);
	}
	Status = gPciIo->Pci.Write (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        Capability+0x2,
                        1,
                        &Value16
                        );
	Value32 = AhciReadReg (gPciIo,AHCI_GHC);
	Value32 &=0x4;
	if(Value32!=0){
		DEBUG((EFI_D_ERROR,"Fail to Enable MMSI ,Revert Single MSI\n"));
	}
}
VOID Ahci_Report_Error(UINT16 Port)
{
   EFI_STATUS   Status;
   UINT16      Value16;
   Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x06,
                        1,
                        &Value16
                        );
   DEBUG((EFI_D_ERROR,"Register_0x06:%x\n",Value16));
   Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  DEBUG((EFI_D_ERROR,"Register_0x04:%x\n",Value16));
  DEBUG((EFI_D_ERROR,"AHCI_GHC:%x\n",AhciReadReg (gPciIo,AHCI_GHC)));
  DEBUG((EFI_D_ERROR,"AHCI_IS:%x\n",AhciReadReg (gPciIo,0x08)));
  DEBUG((EFI_D_ERROR,"Port_IE:%x\n",AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE)));
  DEBUG((EFI_D_ERROR,"Port_IS:%x\n",AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS)));
}
/*VOID IDE_Set_Port(UINT16 Port)
{

}
*/
VOID AHCI_Set_Port(UINT16 Port)
{
  
  UINT32 Value32;
  Value32 = 0xFFFFFFFF;
  DEBUG ((EFI_D_ERROR, __FUNCTION__"\n"));
  AhciWriteReg(gPciIo, AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS,Value32);
  //Mike_Set Port 0 IS_E
  //Mike_Set Port 0 IE _S
  Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE);
  DEBUG((EFI_D_ERROR,"PxIE is %x",Value32));
  Value32 |=(DPE_INT|DHRE_INT);
  AhciWriteReg(gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE,Value32);
  //Mike_Set Port 0 IE_E
  //Mike_Set Global IS _S
  Value32 = 0xFFFFFFFF;
  AhciWriteReg(gPciIo,AHCI_IS,Value32);
  //Mike_Set Global IS_E
  //Mike_Set Global IE _S
  Value32 = AhciReadReg (gPciIo,AHCI_GHC);
  Value32 |=(1<<1);
  AhciWriteReg(gPciIo,AHCI_GHC,Value32);
  //Mike_Set Global IE_E
  
}
VOID AHCI_Clear_Port(UINT16 Port)
{
  
  UINT32 Value32;
  Value32 = 0xFFFFFFFF;
  DEBUG ((EFI_D_ERROR, __FUNCTION__"\n"));
  AhciWriteReg(gPciIo, AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS,Value32);
  //Mike_Set Port 0 IS_E
  //Mike_Set Port 0 IE _S
  Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE);
  Value32 &=~(DPE_INT|DHRE_INT);
  AhciWriteReg(gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE,Value32);
  //Mike_Set Port 0 IE_E
  //Mike_Set Global IS _S
  Value32 = 0xFFFFFFFF;
  AhciWriteReg(gPciIo,AHCI_IS,Value32);
  //Mike_Set Global IS_E
  //Mike_Set Global IE _S
  Value32 = AhciReadReg (gPciIo,AHCI_GHC);
  Value32 &=~(1<<1);
  AhciWriteReg(gPciIo,AHCI_GHC,Value32);
  //Mike_Set Global IE_E
  
}


#define AHCI_MMSI_IRQ  0x14;
EFI_STATUS
EFIAPI
Ahci_MMSI_Initialize(
	IN EFI_HANDLE					Controller,
	EFI_BLOCK_IO_PROTOCOL	  *BlockIo,
	UINT16                     Port
    )
{
    EFI_STATUS   Status;
	UINTN BusNumber;
    UINTN DeviceNumber;
    UINTN FunctionNumber;
    UINTN SegmentNumber;
	UINT8 Value8;
	UINT8 Capability = 0;
	UINT32 Value32;
	UINT16 Value16;
	BOOLEAN  FoundMSI = FALSE;
    UINT8    IrqNum ;
	EFI_CPU_ARCH_PROTOCOL     *mCpu;
	UINT8* pRBuffer1 = NULL;
    UINT8* pRBuffer2 = NULL; 
	UINT32 	   Index=0;
	mPort = Port;
	IrqNum = AHCI_MMSI_IRQ;
	DEBUG ((DEBUG_ERROR, "=======Mike Ahci  MMSI Test  Start=====================\n"));
	pRBuffer1 = AllocatePool(512);
    if(pRBuffer1==NULL){
	  DEBUG((EFI_D_ERROR,"Allocate_pRBuffer_Error\n"));
	  goto Exit;
    }
    pRBuffer2 = AllocatePool(512);
    if(pRBuffer2==NULL){
	  DEBUG((EFI_D_ERROR,"Allocate_pRBuffer_Error\n"));
	  goto Exit;
    }
    Ahci_APIC_Test(BlockIo,pRBuffer1);
	if(gPciIo==NULL)
	{
        Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID**)&gPciIo,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
       if(Status!=0||gPciIo==NULL){
          DEBUG((EFI_D_ERROR,"mike_OpenPciIo_Error:%p\n",gPciIo));
	      goto Exit;
       }
    }
  Status= gPciIo->GetLocation(
  	                          gPciIo,
  	                          &SegmentNumber,
  	                          &BusNumber,
  	                          &DeviceNumber,
  	                          &FunctionNumber
  	                          );
  if(Status!=0){
  	DEBUG((EFI_D_ERROR,"mike_Pci_GetLocation_Error:%x\n",Status));
	goto Exit;
  }
  DEBUG((EFI_D_ERROR,"Bus:%x;Device:%x;Func:%x;Port:%x\n",BusNumber,DeviceNumber,FunctionNumber,Port));
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &mCpu);
  ASSERT_EFI_ERROR (Status);
  Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IrqNum+Port, MMSI_ReadInterruptHandler);
  if (EFI_ERROR (Status)) {
	DEBUG ((DEBUG_ERROR, "Unable to register Sata interrupt with CPU Arch Protocol.\n"));
	return EFI_DEVICE_ERROR;
  }
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint8,
                        0x34,
                        1,
                        &Value8
                        );
  Capability = Value8;
  while(Capability!=0x00){
  	Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint32,
                        Capability,
                        1,
                        &Value32
                        );
    if((Value32&0xff)==0x5){
  	  FoundMSI = TRUE;
	  goto Here;
	}
    Capability = (UINT8)(Value32>>8);
  }
  if(FoundMSI==FALSE){
		DEBUG((EFI_D_ERROR,"Can't Find MSI Register\n"));
		//while(1);
		return FALSE;
  }
 Here:
  DEBUG((EFI_D_ERROR,"FoundMSI_Register:%x\n",Capability));
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        Capability+2,
                        1,
                        &Value8
                        );
  if((Value8&0xE)==0x00){
  	DEBUG((EFI_D_ERROR,"Device Not Support MMSI,MMC is %x\n",Value8));
	//while(1);
	return FALSE;
  }
  DEBUG((EFI_D_ERROR,"MMC is %x\n",Value8));
  //Mike_Enable_CMD.ID_S
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  Value16|=(1<<10);
  Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  //Mike_Enable_CMD.ID_E
  //config msi
  Ahci_ConfigMMSI(Capability,IrqNum);
  AHCI_Set_Port(Port);
  //enable msi
  Ahci_EnableMMSIInterrupt(TRUE,Capability);
  Ahci_APIC_Test(BlockIo,pRBuffer2);
  while(HasEnterInterrupt==FALSE)
  {
    Index++;
	if(Index>=0x1000){
    DEBUG((EFI_D_ERROR,"AHCI MMSI Interrupt Fail\n"));
	goto Exit;
	}
  }
  for(Index=0;Index<512;Index++){
  	if(pRBuffer1[Index]!=pRBuffer2[Index]){
		DEBUG((EFI_D_ERROR,"Read_Data_Error:(Index:%x)(First:%x;Second:%x)\n",Index,pRBuffer1[Index],pRBuffer2[Index]));
		goto Exit;
  	}
  }
  DEBUG((EFI_D_ERROR,"AHCI MMSI Interrupt Success\n"));
  DEBUG ((DEBUG_ERROR, "++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"));
  //while(1);
  HasEnterInterrupt=FALSE;
  Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IrqNum+Port, NULL);
  AHCI_Clear_Port(Port);
  Ahci_EnableMMSIInterrupt(FALSE,Capability);
  Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE);
  DEBUG((EFI_D_ERROR,"PxIE is %x",Value32));
    //Mike_Enable_CMD.ID_S
  //Mike_Enable_CMD.ID_E
  return EFI_SUCCESS;
  
Exit:
	Ahci_Report_Error(Port);
    //while(1);
    HasEnterInterrupt=FALSE;
    //Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IrqNum+Port, NULL);
    AHCI_Clear_Port(Port);
    Ahci_EnableMMSIInterrupt(FALSE,Capability);
	return EFI_SUCCESS;
}
#define Ahci_SMSI_IRQ 0x15
EFI_STATUS
EFIAPI
Ahci_SMSI_Initialize(
	IN EFI_HANDLE					Controller,
	EFI_BLOCK_IO_PROTOCOL	  *BlockIo,
	UINT16                    Port
    )
{
    EFI_STATUS   Status;
	UINTN BusNumber;
    UINTN DeviceNumber;
    UINTN FunctionNumber;
    UINTN SegmentNumber;
	UINT8 Value8;
	UINT16 Value16;
	UINT8 Capability = 0;
	UINT32 Value32;
	BOOLEAN  FoundMSI = FALSE;
    UINT8    IrqNum = Ahci_SMSI_IRQ;
	EFI_CPU_ARCH_PROTOCOL     *mCpu;
	UINT8* pRBuffer1 = NULL;
    UINT8* pRBuffer2 = NULL; 
	UINT32 	   Index=0;
	DEBUG ((DEBUG_ERROR, "=======Mike Ahci  SMSI Test  Start==============\n"));
	mPort = Port;
	pRBuffer1 = AllocatePool(512);
    if(pRBuffer1==NULL){
	  DEBUG((EFI_D_ERROR,"Allocate_pRBuffer_Error\n"));
	  goto Exit;
    }
    pRBuffer2 = AllocatePool(512);
    if(pRBuffer2==NULL){
	  DEBUG((EFI_D_ERROR,"Allocate_pRBuffer_Error\n"));
	  goto Exit;
    }
    Ahci_APIC_Test(BlockIo,pRBuffer1);
	DEBUG((EFI_D_ERROR,"first test OK\n"));
	if(gPciIo==NULL)
	{
		DEBUG((EFI_D_ERROR,"gPciIo NULL\n"));

		Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID**)&gPciIo,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
		DEBUG((EFI_D_ERROR,"gPciIo %x\n",Status));
       if(Status!=0||gPciIo==NULL){
          DEBUG((EFI_D_ERROR,"mike_OpenPciIo_Error:%p\n",gPciIo));
	      goto Exit;
       }
	}
  Status= gPciIo->GetLocation(
  	                          gPciIo,
  	                          &SegmentNumber,
  	                          &BusNumber,
  	                          &DeviceNumber,
  	                          &FunctionNumber
  	                          );
  if(Status!=0){
  	DEBUG((EFI_D_ERROR,"mike_Pci_GetLocation_Error:%x\n",Status));
	goto Exit;
  }
  DEBUG((EFI_D_ERROR,"Bus:%x;Device:%x;Func:%x;Port:%x\n",BusNumber,DeviceNumber,FunctionNumber,Port));
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint8,
                        0x34,
                        1,
                        &Value8
                        );
  Capability = Value8;
  while(Capability!=0x00){
  	Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint32,
                        Capability,
                        1,
                        &Value32
                        );
    if((Value32&0xff)==0x5){
  	  FoundMSI = TRUE;
	  goto Here;
	}
    Capability = (UINT8)(Value32>>8);
  }
  if(FoundMSI==FALSE){
		DEBUG((EFI_D_ERROR,"Device Can't Support SMI\n"));
		while(1);
  }
 Here:
  DEBUG((EFI_D_ERROR,"FoundSMSI_Register:%x\n",Capability));
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        Capability+2,
                        1,
                        &Value8
                        );

  DEBUG((EFI_D_ERROR,"MMC is %x\n",Value8));
  //Mike_Enable_CMD.ID_S
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  Value16|=(1<<10);
  Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  //Mike_Enable_CMD.ID_E
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &mCpu);
  ASSERT_EFI_ERROR (Status);
  Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IrqNum, MSI_ReadInterruptHandler);
  if (EFI_ERROR (Status)) {
	DEBUG ((DEBUG_ERROR, "Unable to register Sata interrupt with CPU Arch Protocol.\n"));
	return EFI_DEVICE_ERROR;
  }
  //config msi
  Ahci_ConfigMSI(Capability,IrqNum);
  AHCI_Set_Port(Port);
  //enable msi
  Ahci_EnableMSIInterrupt(TRUE,Capability);
  Ahci_APIC_Test(BlockIo,pRBuffer2);
  while(HasEnterInterrupt==FALSE)
  {
    Index++;
	if(Index>=0x1000){
    DEBUG((EFI_D_ERROR,"AHCI SMSI Interrupt Fail\n"));
	goto Exit;
	}
  }
  for(Index=0;Index<512;Index++){
  	if(pRBuffer1[Index]!=pRBuffer2[Index]){
		DEBUG((EFI_D_ERROR,"Read_Data_Error:(Index:%x)(First:%x;Second:%x)\n",Index,pRBuffer1[Index],pRBuffer2[Index]));
		goto Exit;
  	}
  }
  DEBUG((EFI_D_ERROR,"AHCI SMSI Interrupt Success\n"));
  DEBUG ((DEBUG_ERROR, "++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"));
  //while(1);
  HasEnterInterrupt=FALSE;
  Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IrqNum, NULL);
  AHCI_Clear_Port(Port);
  Ahci_EnableMSIInterrupt(FALSE,Capability);
  Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE);
  DEBUG((EFI_D_ERROR,"PxIE is %x",Value32));
  return EFI_SUCCESS;

  
Exit:
	Ahci_Report_Error(Port);
    //while(1);
    HasEnterInterrupt=FALSE;
    //Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IrqNum, NULL);
    AHCI_Clear_Port(Port);
    Ahci_EnableMSIInterrupt(FALSE,Capability);
	return EFI_SUCCESS;
}
#define AHCI_MSIX_IRQ 0x14;
EFI_STATUS
EFIAPI
Ahci_MSIX_Initialize(
	IN EFI_HANDLE					Controller,
	EFI_BLOCK_IO_PROTOCOL	  *BlockIo,
	UINT16                     Port
    )
{
    EFI_STATUS   Status;
	UINTN BusNumber;
    UINTN DeviceNumber;
    UINTN FunctionNumber;
    UINTN SegmentNumber;
	UINT8 Value8;
	UINT8 Capability = 0;
	UINT32 Value32;
	UINT16 Value16;
	BOOLEAN  FoundMSIX = FALSE;
    UINT8    IrqNum;
	EFI_CPU_ARCH_PROTOCOL     *mCpu;
	UINT8* pRBuffer1 = NULL;
    UINT8* pRBuffer2 = NULL; 
	UINT32 Index=0;
	UINT16  MSIX_Table_Size = 0; 
	UINT32  MSIX_Table_Offset = 0;
	UINT32  MSIX_Pening_Offset = 0;
	UINT32  pMSIX_Table_Address = 0;
	UINT32  pMSIX_Pening_Address = 0;
	BOOLEAN Open = FALSE;
	DEBUG ((DEBUG_ERROR, "=======Mike Ahci  MSI-X Test  Start==============\n"));
	IrqNum = AHCI_MSIX_IRQ;
	mPort = Port;
	pRBuffer1 = AllocatePool(512);
    if(pRBuffer1==NULL){
	  DEBUG((EFI_D_ERROR,"Allocate_pRBuffer_Error\n"));
	  goto Exit;
    }
    pRBuffer2 = AllocatePool(512);
    if(pRBuffer2==NULL){
	  DEBUG((EFI_D_ERROR,"Allocate_pRBuffer_Error\n"));
	  goto Exit;
    }
    Ahci_APIC_Test(BlockIo,pRBuffer1);
	if(gPciIo==NULL)
	{
        Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID**)&gPciIo,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
       if(Status!=0||gPciIo==NULL){
          DEBUG((EFI_D_ERROR,"mike_OpenPciIo_Error:%p\n",gPciIo));
	      goto Exit;
       }
	}
  Status= gPciIo->GetLocation(
  	                          gPciIo,
  	                          &SegmentNumber,
  	                          &BusNumber,
  	                          &DeviceNumber,
  	                          &FunctionNumber
  	                          );
  if(Status!=0){
  	DEBUG((EFI_D_ERROR,"mike_Pci_GetLocation_Error:%x\n",Status));
	goto Exit;
  }
  DEBUG((EFI_D_ERROR,"Bus:%x;Device:%x;Func:%x;Port:%x\n",BusNumber,DeviceNumber,FunctionNumber,Port));
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint8,
                        0x34,
                        1,
                        &Value8
                        );
  Capability = Value8;
  while(Capability!=0x00){
  	Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint32,
                        Capability,
                        1,
                        &Value32
                        );
    if((Value32&0xff)==0x11){
  	  FoundMSIX = TRUE;
	  goto Here;
	}
    Capability = (UINT8)(Value32>>8);
  }
 Here:
  if(FoundMSIX==FALSE){
  	DEBUG((EFI_D_ERROR,"Can't Find MSI-X\n"));
	//while(1);
	return FALSE;
  }
  else{
  	 DEBUG((EFI_D_ERROR,"Found MSI-X_Register:%x\n",Capability));
  }
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        Capability+0x2,
                        1,
                        &Value16
                        );
  MSIX_Table_Size=Value16&0x7FF;
  MSIX_Table_Size+=0x1;
  DEBUG((EFI_D_ERROR,"Device Has %x Table\n",MSIX_Table_Size));
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint32,
                        Capability+0x4,
                        1,
                        &Value32
                        );
   MSIX_Table_Offset = Value32&(~0x7);
   Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint32,
                        (0x10+(Value32&0x7)*0x4),
                        1,
                        &Value32
                        );
  pMSIX_Table_Address = Value32+MSIX_Table_Offset;
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint32,
                        Capability+0x8,
                        1,
                        &Value32
                        );
   MSIX_Pening_Offset= Value32&(~0x7);
   Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint32,
                        (0x10+(Value32&0x7)*0x4),
                        1,
                        &Value32
                        );
  pMSIX_Pening_Address = Value32+MSIX_Pening_Offset;
  DEBUG((EFI_D_ERROR,"Zoey:Find the pMSIX_Pening_Address\n"));
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &mCpu);
  ASSERT_EFI_ERROR (Status);
  //config msi-x
  for(Index = 0;Index<MSIX_Table_Size;Index++){
  	 Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IrqNum+Index*4, MSIX_ReadInterruptHandler);
  }
  if (EFI_ERROR (Status)) {
	DEBUG ((DEBUG_ERROR, "Unable to register Sata interrupt with CPU Arch Protocol.\n"));
	return EFI_DEVICE_ERROR;
  }
  //config msi-x
  for(Index = 0;Index<MSIX_Table_Size;Index++){
  	 if(Index==Port){
	 	Open = TRUE;
  	 }
	 else{
	 	Open = FALSE;
	 }
  	 Ahci_ConfigMSIX(pMSIX_Table_Address+(0x10*Index),(UINT8)(IrqNum+Index*4),Open);
  }
  AHCI_Set_Port(Port);
  //enable msix
  Ahci_EnableMSIXInterrupt(TRUE,Capability);
  Ahci_APIC_Test(BlockIo,pRBuffer2);
  while(HasEnterInterrupt==FALSE)
  {
    Index++;
	if(Index>=0x1000){
    DEBUG((EFI_D_ERROR,"AHCI MSI-X Interrupt Fail\n"));
	goto Exit;
	}
  }
  for(Index=0;Index<512;Index++){
  	if(pRBuffer1[Index]!=pRBuffer2[Index]){
		DEBUG((EFI_D_ERROR,"Read_Data_Error:(Index:%x)(First:%x;Second:%x)\n",Index,pRBuffer1[Index],pRBuffer2[Index]));
		goto Exit;
  	}
  }
  DEBUG((EFI_D_ERROR,"AHCI MSI-X Interrupt Success\n"));
  DEBUG ((DEBUG_ERROR, "++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"));
  //while(1);
  HasEnterInterrupt=FALSE;
  for(Index = 0;Index<MSIX_Table_Size;Index++){
  	 Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IrqNum+Index*4, NULL);
  }
  Ahci_EnableMSIXInterrupt(FALSE,Capability);
  AHCI_Clear_Port(Port);
  Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE);
  DEBUG((EFI_D_ERROR,"PxIE is %x",Value32));
  return EFI_SUCCESS;
  
Exit:
	Ahci_Report_Error(Port);
	//while(1);
   HasEnterInterrupt=FALSE;
  //for(Index = 0;Index<MSIX_Table_Size;Index++){
  	 //Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IrqNum+Index*4, NULL);
  //}
  Ahci_EnableMSIXInterrupt(FALSE,Capability);
  AHCI_Clear_Port(Port);
	return EFI_SUCCESS;
}
#define AHCI_PIC_IRQ 0xA
EFI_STATUS
EFIAPI
Ahci_PIC_Initialize (
  IN EFI_HANDLE                   Controller,
  EFI_BLOCK_IO_PROTOCOL 	*BlockIo,
  UINT16                     Port
  )
{
  EFI_STATUS   Status;
  UINT8        Value8;
  UINT32 	   Index=0;
  EFI_CPU_ARCH_PROTOCOL     *mCpu;
  EFI_LEGACY_8259_PROTOCOL  *mLegacy8259;
  UINTN BusNumber;
  UINTN DeviceNumber;
  UINTN FunctionNumber;
  UINTN SegmentNumber;
  UINT8* pRBuffer1 = NULL;
  UINT8* pRBuffer2 = NULL; 
  UINTN  IRqNum;
  UINT32 AHCIVector = 0;
  UINT16 Value16=0;
  DEBUG ((DEBUG_ERROR, "=======Mike Ahci PIC Test Start=============\n"));
  mPort = Port;
  pRBuffer1 = AllocatePool(512);
  if(pRBuffer1==NULL){
	  DEBUG((EFI_D_ERROR,"Allocate_pRBuffer_Error\n"));
	  goto Exit;
  }
  pRBuffer2 = AllocatePool(512);
  if(pRBuffer2==NULL){
	  DEBUG((EFI_D_ERROR,"Allocate_pRBuffer_Error\n"));
	  goto Exit;
  }
  
  if(gPciIo==NULL)
  {
      Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID**)&gPciIo,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
      if(Status!=0||gPciIo==NULL){
         DEBUG((EFI_D_ERROR,"mike_OpenPciIo_Error:%p\n",gPciIo));
	     goto Exit;
      }
  }
  Ahci_APIC_Test(BlockIo,pRBuffer1);
  Status= gPciIo->GetLocation(
  	                          gPciIo,
  	                          &SegmentNumber,
  	                          &BusNumber,
  	                          &DeviceNumber,
  	                          &FunctionNumber
  	                          );
  if(Status!=0){
  	DEBUG((EFI_D_ERROR,"mike_Pci_GetLocation_Error:%x\n",Status));
	goto Exit;
  }
  DEBUG((EFI_D_ERROR,"Bus:%x;Device:%x;Func:%x;Port:%x\n",BusNumber,DeviceNumber,FunctionNumber,Port));
  IRqNum = AHCI_PIC_IRQ;
  Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint8,
                        0x3C,
                        1,
                        &((UINT8)IRqNum)
                        );
    Status = gPciIo->Pci.Read(
                        gPciIo,
                        EfiPciIoWidthUint8,
                        0x3C,
                        1,
                        &Value8
                        );
 if(BusNumber>=1){
 	Value8&=0xF;
 	Value8|=(UINT8)(IRqNum<<4);
	MmioWrite8(LPC_PCI_REG(0x45),Value8);
    Value8 = MmioRead8(LPC_PCI_REG(0x45));
 }
 // Find the 8259  protocol.
 Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &mLegacy8259);
 ASSERT_EFI_ERROR (Status);
 gLegacy8259 = mLegacy8259;
 AHCIVector = 0;
 Status	  = mLegacy8259->GetVector (mLegacy8259, IRqNum, (UINT8 *) &AHCIVector);
 // Find the CPU architectural protocol.
 Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &mCpu);
 if (EFI_ERROR (Status)) {
	DEBUG ((DEBUG_ERROR, "Unable to register Sata interrupt with CPU Arch Protocol.Status is %d\n",Status));
	return EFI_DEVICE_ERROR;
  }
 
  Status = mCpu->RegisterInterruptHandler(mCpu, AHCIVector, AHCI_PIC_ReadInterruptHandler);
  AHCI_Set_Port(Port);
    //Mike_Enable_CMD.ID_S
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  Value16&=~(1<<10);
  Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  //Mike_Enable_IRQ_UMask_S
  Status	  = mLegacy8259->EnableIrq(mLegacy8259, IRqNum,TRUE);
  if(IRqNum>0x7){
  	Status	  = mLegacy8259->EnableIrq(mLegacy8259, 0x2,FALSE);
  }
  //Mike_Enable_IRQ_UMask_E
  Ahci_APIC_Test(BlockIo,pRBuffer2);
  while(HasEnterInterrupt==FALSE)
  {
    Index++;
	if(Index>=0x1000){
    DEBUG((EFI_D_ERROR,"AHCI PIC Interrupt Fail\n"));
	goto Exit;
	}
  }
  for(Index=0;Index<512;Index++){
  	if(pRBuffer1[Index]!=pRBuffer2[Index]){
		DEBUG((EFI_D_ERROR,"Read_Data_Error:(Index:%x)(First:%x;Second:%x)\n",Index,pRBuffer1[Index],pRBuffer2[Index]));
		goto Exit;
  	}
  }
  DEBUG((EFI_D_ERROR,"AHCI PIC Interrupt Success\n"));
  DEBUG ((DEBUG_ERROR, "++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"));
  //while(1);
  //Status=gBS->CloseProtocol(Controller,&gEfiPciIoProtocolGuid,NULL,Controller);
  //DEBUG((EFI_D_ERROR,"Close PciIo %x\n",Status));
  HasEnterInterrupt=FALSE;
  Status = mCpu->RegisterInterruptHandler(mCpu, AHCIVector, NULL);
  
  Status = mLegacy8259->DisableIrq(mLegacy8259, IRqNum);
  if(IRqNum>0x7){
  	Status	  = mLegacy8259->DisableIrq(mLegacy8259, 0x2);
  }
  AHCI_Clear_Port(Port);
  
  DEBUG((EFI_D_ERROR,"PxIE is %x",AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE)));
  return EFI_SUCCESS;
Exit:
  Ahci_Report_Error(Port);
  
  //Status = mCpu->RegisterInterruptHandler(mCpu, AHCIVector, NULL);
  
  //Status = mLegacy8259->DisableIrq(mLegacy8259, IRqNum);
  //if(IRqNum>0x7){
  	//Status	  = mLegacy8259->DisableIrq(mLegacy8259, 0x2);
  //}
  AHCI_Clear_Port(Port);
  //while(1);
  return EFI_SUCCESS;
}

#define AHCI_SB_IRQ 0x15
#define AHCI_NB_IRQ 0x18
EFI_STATUS
EFIAPI
Ahci_APIC_Initialize (
  IN EFI_HANDLE                   Controller,
  EFI_BLOCK_IO_PROTOCOL 	*BlockIo,
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  UINT16                    Port
  )
{
  EFI_STATUS   Status;
  UINT8  Value8;
  UINT16       Value16;
  UINT32 	   Index=0;
  EFI_CPU_ARCH_PROTOCOL     *mCpu;
  EFI_LEGACY_8259_PROTOCOL  *mLegacy8259;
  UINTN BusNumber;
  UINTN DeviceNumber;
  UINTN FunctionNumber;
  UINTN SegmentNumber;
  UINT8* pRBuffer1 = NULL;
  UINT8* pRBuffer2 = NULL; 
  UINTN  IRqNum = 0;
  UINT8  Io_Apic_Id = 0;
  mPort = Port;
  DEBUG ((DEBUG_ERROR, "=======Mike_Ahci_APIC_Test_Start==============\n"));
  //Read Data Before Enable Interrupt for compare with Enable Interrupt Read Data_S
  pRBuffer1 = AllocatePool(512);
  if(pRBuffer1==NULL){
	  DEBUG((EFI_D_ERROR,"Allocate_pRBuffer_Error\n"));
	  goto Exit;
  }
  pRBuffer2 = AllocatePool(512);
  if(pRBuffer2==NULL){
	  DEBUG((EFI_D_ERROR,"Allocate_pRBuffer_Error\n"));
	  goto Exit;
  }
    Ahci_APIC_Test(BlockIo,pRBuffer1);
  //DEBUG((EFI_D_ERROR,"Ahci_APIC_Test OK\n"));
  //Read Data Before Enable Interrupt for compare with Enable Interrupt Read Data_E
  if(gPciIo==NULL)
  {  
      DEBUG((EFI_D_ERROR,"gPciIo==Null\n"));
	  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID**)&gPciIo,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
	  DEBUG((EFI_D_ERROR,"gPciIo open\n"));
      if(Status!=0||gPciIo==NULL){
         DEBUG((EFI_D_ERROR,"mike_OpenPciIo_Error:%p\n",gPciIo));
	     goto Exit;
      }
  }

  Status= gPciIo->GetLocation(
  	                          gPciIo,
  	                          &SegmentNumber,
  	                          &BusNumber,
  	                          &DeviceNumber,
  	                          &FunctionNumber
  	                          );
  DEBUG((EFI_D_ERROR,"mike_Pci_GetLocation_Error:%x\n",Status));
  if(Status!=0){
  	DEBUG((EFI_D_ERROR,"mike_Pci_GetLocation_Error:%x\n",Status));
	goto Exit;
  }
  DEBUG((EFI_D_ERROR,"Bus:%x;Device:%x;Func:%x;Port:%x\n",BusNumber,DeviceNumber,FunctionNumber,Port));
  if(BusNumber>=1){
  	Status = gPciIo->Pci.Read(
                        gPciIo,
                        EfiPciIoWidthUint8,
                        0x3D,
                        1,
                        &Value8
                        );
	switch(Value8)
	{
	case 1:
		DEBUG((EFI_D_ERROR,"Device USE  INTA\n"));
		break;
	case 2:
		DEBUG((EFI_D_ERROR,"Device USE  INTB\n"));
		break;
	case 3:
		DEBUG((EFI_D_ERROR,"Device USE  INTC\n"));
		break;
	case 4:
		DEBUG((EFI_D_ERROR,"Device USE  INTD\n"));
		break;
	default:
		DEBUG((EFI_D_ERROR,"Not Found Device USE INTX\n"));
		break;
	}
  	IRqNum = Find_Device_PCIEPort(DevicePath,Value8-1);
	if(IRqNum==0){
	DEBUG((EFI_D_ERROR,"Can't find IRQ\n"));
	while(1);
	}
	else{
	DEBUG((EFI_D_ERROR,"IRQ=%x\n",IRqNum));	
	}
  	Io_Apic_Id = NB_IO_APIC_ID;
  	IO_Apic_Base = NB_IO_APIC_BASE;
	//enable nb apic
	MmioWrite8(APIC_PCI_REG(0x40),MmioRead8(APIC_PCI_REG(0x40))|BIT7);
  }
  else{
  	Io_Apic_Id = SB_IO_APIC_ID;
  	IO_Apic_Base = SB_IO_APIC_BASE;
    IRqNum = AHCI_SB_IRQ;
	//enable sb apic
    MmioWrite8(LPC_PCI_REG(0x58),MmioRead8(LPC_PCI_REG(0x58))|BIT6);
  }

 //set  APIC ID ;
 MmioWrite8(IO_Apic_Base, 0x00);
 MmioWrite32(IO_Apic_Base + 0x10,Io_Apic_Id<<24);
 //set  APIC use FSB,Interrupt Delivery Mechanism is a Front-side Bus Message
 MmioWrite8(IO_Apic_Base, 0x03);
 MmioWrite32(IO_Apic_Base+ 0x10, 0x01);

 // Find the 8259  protocol.
 Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &mLegacy8259);
 ASSERT_EFI_ERROR (Status);
 
 //DLA:mask all  8259 interrupt  
 for(Index=0;Index<16;Index++){
   //mLegacy8259->DisableIrq (mLegacy8259, Index);
   }
 // Find the CPU architectural protocol.
 Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &mCpu);
 ASSERT_EFI_ERROR (Status);
 Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &mLegacy8259);
 ASSERT_EFI_ERROR (Status);
 Status = mCpu->Init(mCpu, 0x80+IRqNum);
 Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IRqNum, AHCI_ReadInterruptHandler);
  if (EFI_ERROR (Status)) {
	DEBUG ((DEBUG_ERROR, "Unable to register Sata interrupt with CPU Arch Protocol. Status is %d\n",Status));
	return EFI_DEVICE_ERROR;
  }
  
  DEBUG((EFI_D_ERROR,"mike_true_false\n"));
  Mike_IoApicConfigureInterrupt (IRqNum%0x18, 0x80+IRqNum, IO_APIC_DELIVERY_MODE_FIXED,TRUE, FALSE);
  //DEBUG((EFI_D_ERROR,"IoApicConfigureInterrupt\n"));
  //Mike_Enable_CMD.ID_S
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  Value16&=~(1<<10);
  Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  //Mike_Enable_CMD.ID_E
  
  AHCI_Set_Port(Port);
  //Ahci_Report_Error(Port);
 
  Mike_IoApicEnableInterrupt(IRqNum%0x18,TRUE);  
  
  Ahci_APIC_Test(BlockIo,pRBuffer2);
  //DEBUG((EFI_D_ERROR,"APIC_Test\n"));
  while(HasEnterInterrupt==FALSE)
  {
	  //DEBUG((EFI_D_ERROR,"HasEnterInterrupt%x\n",HasEnterInterrupt));

	Index++;
	if(Index>=0x100000){
    DEBUG((EFI_D_ERROR,"AHCI APIC Interrupt Fail\n"));
	goto Exit;
	}
  }
  for(Index=0;Index<512;Index++){
  	if(pRBuffer1[Index]!=pRBuffer2[Index]){
		DEBUG((EFI_D_ERROR,"Read_Data_Error:(Index:%x)(First:%x;Second:%x)\n",Index,pRBuffer1[Index],pRBuffer2[Index]));
		goto Exit;
  	}
  }
  DEBUG((EFI_D_ERROR,"AHCI APIC Interrupt Success\n"));
  DEBUG ((DEBUG_ERROR, "++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"));
  //while(1);
  HasEnterInterrupt=FALSE;
  
    Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IRqNum, NULL);
	Mike_IoApicEnableInterrupt(IRqNum%0x18,FALSE);
	AHCI_Clear_Port(Port);
	AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE);
  //Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IRqNum, AHCI_ReadInterruptHandler);	
  return EFI_SUCCESS;
Exit:
  Ahci_Report_Error(Port);
  MmioWrite8(IO_Apic_Base, (UINT8)(0x10+(IRqNum%0x18)*2));
  DEBUG((EFI_D_ERROR,"IRQ_Entry_Low:%x\n",MmioRead64(IO_Apic_Base+0x10)));
  MmioWrite8(IO_Apic_Base, (UINT8)(0x10+(IRqNum%0x18)*2+1));
  DEBUG((EFI_D_ERROR,"IRQ_Entry_High:%x\n",MmioRead64(IO_Apic_Base+0x10)));
  //while(1);
  //Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IRqNum, NULL);
  Mike_IoApicEnableInterrupt(IRqNum%0x18,FALSE);
  AHCI_Clear_Port(Port);
  return EFI_SUCCESS;
}
///////////////////////////Mike_Add_Test_ACHI_Code  ////////////////////////////
INTN
EFIAPI
ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      SataController = NULL;
  EFI_BLOCK_IO_PROTOCOL           *zBlockIo;
  UINT8                           Port;
  UINT32								 i=0;
  UINT32								 j=0;
  UINT8								 Find_Flag=0;
  EFI_HANDLE							 *HandleBIOBuffer;
  UINTN								 NumberOfBIOHandles;
  EFI_HANDLE							 *HandleDIFBuffer;
  UINTN								 NumberOfDIFHandles;
  EFI_DEVICE_PATH_PROTOCOL			 *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL			 *DevPathNode;
  SHELL_STATUS                         ShellStatus;
  ATA_DEVICE                           *AtaDevice;

  ShellStatus = SHELL_SUCCESS;
  
	////////////Find BlockIoProtocol////////////////////////////
  Status = gBS->LocateHandleBuffer (
					 ByProtocol,
					 &gEfiBlockIoProtocolGuid,
					 NULL,
					 &NumberOfBIOHandles,
					 &HandleBIOBuffer
					 );
  if (EFI_ERROR (Status))
  {
	  DEBUG((EFI_D_ERROR,"LocateBlockIoProtocol Error!\n"));
	  return (ShellStatus);
  }
  Status = gBS->LocateHandleBuffer (
					 ByProtocol,
					 &gEfiDiskInfoProtocolGuid,
					 NULL,
					 &NumberOfDIFHandles,
					 &HandleDIFBuffer
					 );
  if (EFI_ERROR (Status))
  {
	  DEBUG((EFI_D_ERROR,"LocateDiskInfoProtocol Error!\n"));
	  return (ShellStatus);
  }

  for (i = 0; i < NumberOfBIOHandles; i++)
  {
	   for(j=0;j<NumberOfDIFHandles;j++)
	   {
		 if(HandleBIOBuffer[i]==HandleDIFBuffer[j])
		  {
  
			Status = gBS->HandleProtocol (
					   HandleBIOBuffer[i],
					   &gEfiDevicePathProtocolGuid,
					   (VOID**) &DevicePath
					   );
  
				DevPathNode = DevicePath;
				while (!IsDevicePathEnd (DevPathNode))
				{
				
				 if((DevPathNode->Type==0x3)&&(DevPathNode->SubType==0x1))
				  {    
					   DEBUG((EFI_D_ERROR,"Sata IDE Mode\n"));
					   Find_Flag = 1;
					   break;
				  }
				 else if((DevPathNode->Type==0x3)&&(DevPathNode->SubType==0x12))
				  {
					   DEBUG((EFI_D_ERROR,"Sata AHCI Mode\n"));
					   Find_Flag = 1;
					   break;
				  }
				 DevPathNode = NextDevicePathNode (DevPathNode);
				}
  
			if(Find_Flag==1)
			{
			  DevPathNode = DevicePath;
			  if(!IsDevicePathEnd (DevPathNode))
			  {
			   Print(L"(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
			   DevPathNode = NextDevicePathNode (DevPathNode);
			  }
			  while(!IsDevicePathEnd (DevPathNode))
			  {
			   Print(L"/(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
			   DevPathNode = NextDevicePathNode (DevPathNode);
			  }
			  Print(L"\n");
  
			  /*DevPathNode = DevicePath;
			  if (!IsDevicePathEnd (DevPathNode))
			  {
			   Textdevicepath=Device2TextProtocol->ConvertDevicePathToText(DevPathNode,TRUE,TRUE);
			   Print(L"%s",Textdevicepath);
			   if(Textdevicepath) gBS->FreePool(Textdevicepath);
			  }*/
			  Print(L"\n");
			  break;
			}
		  }
	   }
	   if(Find_Flag==1)
	   {
		 break;
	   }
  }
  if(Find_Flag==1)
  {
       //SataHandle = HandleBIOBuffer[i];
	  Status = gBS->HandleProtocol (
					   HandleBIOBuffer[i],
					   &gEfiBlockIoProtocolGuid,
					   (VOID**) &zBlockIo
					   );
	  
	  ASSERT_EFI_ERROR (Status);
	  DEBUG((EFI_D_ERROR,"Zoey:BlockIo open\n"));
	  AtaDevice = ATA_DEVICE_FROM_BLOCK_IO(zBlockIo);
	  SataController = AtaDevice->AtaBusDriverData->Controller;
  }
  else
  {
	  DEBUG((EFI_D_ERROR,"ERROR: Can't find BlockIO!\n"));
	  return (ShellStatus);
  }

  Port = (UINT8)AtaDevice->Port;
  DEBUG((EFI_D_ERROR,"port is %d!\n",Port));
 /* Status = gBS->HandleProtocol (
                      SataHandle,
                      &gEfiBlockIoProtocolGuid,
                      (VOID **) &zBlockIo
                      );
  */

  
  if (Argc<2){
  	Print(L"Please enter the right Interrupt Type(PIC/APIC/SMSI/MMSI/MSIX)\n");
	return 0;
  }
  	
  
  if(Argv[1][0]=='P')
		Ahci_PIC_Initialize(SataController,zBlockIo,Port);
  else if(Argv[1][0]=='A')
		Ahci_APIC_Initialize(SataController,zBlockIo,AtaDevice->DevicePath,Port);
  else if(Argv[1][0]=='S')
		Ahci_SMSI_Initialize(SataController,zBlockIo,Port);
  else if(Argv[1][0]=='M'){
  	if(Argv[1][1]=='M')
		Ahci_MMSI_Initialize(SataController,zBlockIo,Port);
	if(Argv[1][1]=='S')
		Ahci_MSIX_Initialize(SataController,zBlockIo,Port);
	
  }
  else if(Argv[1][0]=='a'){
  	    Ahci_PIC_Initialize(SataController,zBlockIo,Port);
		Ahci_APIC_Initialize(SataController,zBlockIo,AtaDevice->DevicePath,Port);
		Ahci_SMSI_Initialize(SataController,zBlockIo,Port);
		Ahci_MMSI_Initialize(SataController,zBlockIo,Port);
		Ahci_MSIX_Initialize(SataController,zBlockIo,Port);
  	}
  else
		Print(L"Please enter the right Interrupt Type(PIC/APIC/SMSI/MMSI/MSIX/all)\n");
 /*Status = gBS->CloseProtocol(HandleBIOBuffer[i],&gEfiBlockIoProtocolGuid,AtaDevice->AtaBusDriverData->DriverBindingHandle,SataController);
 DEBUG((EFI_D_ERROR,"close blockIo Status %d",Status));
 Status = gBS->CloseProtocol(SataController,&gEfiPciIoProtocolGuid,AtaDevice->AtaBusDriverData->DriverBindingHandle,NULL);
 DEBUG((EFI_D_ERROR,"close pciIo Status %d",Status));*/	
  return 0;
}


