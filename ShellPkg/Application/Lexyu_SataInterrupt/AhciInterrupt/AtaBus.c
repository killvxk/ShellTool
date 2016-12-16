/** @file
  This file implements protocol interfaces for ATA bus driver.
  
  This file implements protocol interfaces: Driver Binding protocol,
  Block IO protocol and DiskInfo protocol.
    
  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "AtaBus.h"
#include <Register/LocalApic.h>
#include <Register/IoApic.h>
#include <Protocol/Legacy8259.h>
#include <Library/LocalApicLib.h>
#include <Library/IoApicLib.h>
#include <Protocol/Cpu.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Protocol/PciIo.h>
#include <Library/BaseLib.h>
#include <Library/PciLib.h>
#include <IndustryStandard/Pci.h>
#include <Protocol/AcpiTable.h>
#include <IndustryStandard/Acpi.h>
#include <Protocol/AcpiSupport.h>






//
// ATA Bus Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gAtaBusDriverBinding = {
    AtaBusDriverBindingSupported,
    AtaBusDriverBindingStart,
    AtaBusDriverBindingStop,
    0x10,
    NULL,
    NULL
};

//
// Template for ATA Child Device.
//
ATA_DEVICE gAtaDeviceTemplate = {
  ATA_DEVICE_SIGNATURE,        // Signature
  NULL,                        // Handle
  {                            // BlockIo
    EFI_BLOCK_IO_PROTOCOL_REVISION,
    NULL,
    AtaBlockIoReset,
    AtaBlockIoReadBlocks,
    AtaBlockIoWriteBlocks,
    AtaBlockIoFlushBlocks
  },
  {                            // BlockIo2
    NULL,
    AtaBlockIoResetEx,
    AtaBlockIoReadBlocksEx,
    AtaBlockIoWriteBlocksEx,
    AtaBlockIoFlushBlocksEx
  },
  {                            // BlockMedia
    0,                         // MediaId
    FALSE,                     // RemovableMedia
    TRUE,                      // MediaPresent
    FALSE,                     // LogicPartition
    FALSE,                     // ReadOnly
    FALSE,                     // WritingCache
    0x200,                     // BlockSize 
    0,                         // IoAlign
    0,                         // LastBlock
    0,                         // LowestAlignedLba
    1                          // LogicalBlocksPerPhysicalBlock
},
  {                            // DiskInfo
    EFI_DISK_INFO_IDE_INTERFACE_GUID,
    AtaDiskInfoInquiry,
    AtaDiskInfoIdentify,
    AtaDiskInfoSenseData,
    AtaDiskInfoWhichIde
  },
  NULL,                        // DevicePath
  {
    AtaStorageSecurityReceiveData,
    AtaStorageSecuritySendData
  },
  NULL,                        // AtaBusDriverData
  0,                           // Port
  0,                           // PortMultiplierPort
  { 0, },                      // Packet
  {{ 0}, },                    // Acb
  NULL,                        // Asb
  FALSE,                       // UdmaValid
  FALSE,                       // Lba48Bit
  FALSE,                       // Locked  
  NULL,                        // IdentifyData
  NULL,                        // ControllerNameTable
  {L'\0', },                   // ModelName
  {NULL, NULL}                 // AtaTaskList
};
///////////////////////////Mike_Add_Test_ACHI_Code ////////////////////////////
#define PCI_DEV_MMBASE(Bus, Device, Function) \
( \
    (UINTN)PcdGet64(PcdPciExpressBaseAddress) + (UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) \
    (Function << 12) \
    )
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
Ahci_APIC_Test(EFI_BLOCK_IO_PROTOCOL    *BlockIo,UINT8*  pRBuffer)
{
    EFI_STATUS   Status;
    UINTN    BufSize = 512;
    Status = BlockIo->ReadBlocks(
        BlockIo,
        0,
        0,
        BufSize,
        pRBuffer
        );
    if(Status!=0){
        DEBUG((EFI_D_ERROR,"mike_ReadBlocks_error:%x\n",Status));
    }
}
UINT32 mPort = 0;
VOID Clear_Interrupt()
{
    UINT32 Value32 = 0;
    while(Value32!=0x21){
        Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+mPort*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS);
        Value32&=0x21;
    }
    Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+mPort*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS);
    AhciWriteReg(gPciIo,AHCI_PORT_START+mPort*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS,Value32);
    Value32 = AhciReadReg(gPciIo,AHCI_IS);
    AhciWriteReg(gPciIo,AHCI_IS,Value32);
    HasEnterInterrupt = TRUE;
}
VOID
EFIAPI
AHCI_ReadInterruptHandler (
    IN EFI_EXCEPTION_TYPE   InterruptType,
    IN EFI_SYSTEM_CONTEXT   SystemContext
    )
{
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
    gLegacy8259->EndOfInterrupt (gLegacy8259, 0xB);
    
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

    Version.Uint32 = Mike_IoApicRead (IO_APIC_VERSION_REGISTER_INDEX);
    ASSERT (Version.Bits.MaximumRedirectionEntry < 0xF0);
    ASSERT (Irq <= Version.Bits.MaximumRedirectionEntry);
    ASSERT (Vector <= 0xFF);
    ASSERT (DeliveryMode < 8 && DeliveryMode != 6 && DeliveryMode != 3);
    
    Entry.Uint32.Low = Mike_IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2);
    Entry.Bits.Vector          = (UINT8)Vector;
    Entry.Bits.DeliveryMode    = (UINT32)DeliveryMode;
    Entry.Bits.DestinationMode = 0; 
    Entry.Bits.Polarity        = AssertionLevel ? 0 : 1;
    Entry.Bits.TriggerMode     = LevelTriggered ? 1 : 0;
    Entry.Bits.Mask            = 1;
    Mike_IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2, Entry.Uint32.Low);

    Entry.Uint32.High = Mike_IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2 + 1);
    Entry.Bits.DestinationID = GetApicId ();
    Entry.Uint32.High&=~(0xFF0000);
    Mike_IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2 + 1, Entry.Uint32.High);
}
#define IS_CND001_Board FALSE
#define IS_CHX001_Board TRUE
/*#define PCI_DEV_MMBASE(Bus, Device, Function) \
    ( \
      (UINTN)0xE0000000 + (UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) \
        (Function << 12) \
    )*/

UINT16 Find_Device_PCIEPort(EFI_DEVICE_PATH_PROTOCOL  *DevicePath,UINT8 INTX){
    UINT8 Value8;
    UINTN Device= 0;
    UINTN Func = 0;
    //UINT32 ClassCode;
    //UINT8  SecondNum;
    UINT16 IrqNum = 0;
    EFI_STATUS      Status;
    UINT8           Rintrtapic;
    //EFI_STATUS   Status;
    //EFI_ACPI_SUPPORT_PROTOCOL                     *AcpiSupport=NULL;
    //EFI_ACPI_COMMON_HEADER  *Table=NULL;
    BOOLEAN  IsFound = FALSE;
    //CHAR8* Sign;   
    //UINTN                  Handle;
    //UINTN     i = 0;
    while(!IsDevicePathEnd(DevicePath)){
        DevicePath = NextDevicePathNode(DevicePath);
        if((DevicePath->Type==0x1)&&(DevicePath->Type==0x1)){
            Device = *((UINT8*)((UINT8*)DevicePath+0x5))&0xff;
            Func = *((UINT8*)((UINT8*)DevicePath+0x4))&0xff;
            IsFound = TRUE;
            goto Found;
        }
        
    }
/*  for(Device=0;Device<0xff;Device++){
        for(Func=0;Func<0x8;Func++){
          ClassCode = MmioRead32(PCI_DEV_MMBASE(0,  Device, Func)+0x08);
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
    Found:
    if(IsFound==FALSE){
        DEBUG((EFI_D_ERROR,"Can't Find Device In Which PCIE Port\n"));
        return IrqNum;
    }
    DEBUG((EFI_D_ERROR,"Device In PCIE Port:%x-%x\n",Device,Func));
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
    
    return  IrqNum;
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
    IN BOOLEAN              Enable,
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
    IN BOOLEAN              Enable,
    IN UINT8                  Capability
    )
{
    EFI_STATUS   Status;
    UINT16       Value8;
    
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
    IN BOOLEAN              Enable,
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
        Value16&=~(1);
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
VOID AHCI_Set_Port(UINT16 Port)
{
    UINT32 Value32;
    Value32 = 0xFFFFFFFF;
    AhciWriteReg(gPciIo, AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS,Value32);
  //Mike_Set Port 0 IS_E
  //Mike_Set Port 0 IE _S
    Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE);
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
#define AHCI_MMSI_IRQ  0x14;
EFI_STATUS
EFIAPI
Ahci_MMSI_Initialize(
    IN EFI_HANDLE                   Controller,
    EFI_BLOCK_IO_PROTOCOL     *BlockIo,
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
    UINT32     Index=0;
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
        while(1);
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
        DEBUG((EFI_D_ERROR,"Device Not Support MMSI\n"));
        while(1);
    }
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
    while(1);
    
    Exit:
    Ahci_Report_Error(Port);
    while(1);
    return EFI_SUCCESS;
}
#define Ahci_SMSI_IRQ 0x15
EFI_STATUS
EFIAPI
Ahci_SMSI_Initialize(
    IN EFI_HANDLE                   Controller,
    EFI_BLOCK_IO_PROTOCOL     *BlockIo,
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
    UINT32     Index=0;
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
    while(1);

    
    Exit:
    Ahci_Report_Error(Port);
    while(1);
    return EFI_SUCCESS;
}
#define AHCI_MSIX_IRQ 0x14;
EFI_STATUS
EFIAPI
Ahci_MSIX_Initialize(
    IN EFI_HANDLE                   Controller,
    EFI_BLOCK_IO_PROTOCOL     *BlockIo,
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
        while(1);
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
    while(1);
    
    Exit:
    Ahci_Report_Error(Port);
    while(1);
    return EFI_SUCCESS;
}
#define AHCI_PIC_IRQ 0xA
EFI_STATUS
EFIAPI
Ahci_PIC_Initialize (
    IN EFI_HANDLE                   Controller,
    EFI_BLOCK_IO_PROTOCOL   *BlockIo,
    UINT16                     Port
    )
{
    EFI_STATUS   Status;
    UINT8        Value8;
    UINT32     Index=0;
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
    Ahci_APIC_Test(BlockIo,pRBuffer1);
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
    Status    = mLegacy8259->GetVector (mLegacy8259, IRqNum, (UINT8 *) &AHCIVector);
 // Find the CPU architectural protocol.
    Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &mCpu);
    if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Unable to register Sata interrupt with CPU Arch Protocol.\n"));
        return EFI_DEVICE_ERROR;
    }
    Status = mCpu->RegisterInterruptHandler(mCpu, AHCIVector, AHCI_PIC_ReadInterruptHandler);
    AHCI_Set_Port(Port);
  //Mike_Enable_IRQ_UMask_S
    Status    = mLegacy8259->EnableIrq(mLegacy8259, IRqNum,TRUE);
    if(IRqNum>0x7){
        Status    = mLegacy8259->EnableIrq(mLegacy8259, 0x2,FALSE);
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
    while(1);
    Exit:
    Ahci_Report_Error(Port);
    while(1);
    return EFI_SUCCESS;
}

#define AHCI_SB_IRQ 0x15
#define AHCI_NB_IRQ 0x18
EFI_STATUS
EFIAPI
Ahci_APIC_Initialize (
    IN EFI_HANDLE                   Controller,
    EFI_BLOCK_IO_PROTOCOL   *BlockIo,
    EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
    UINT16                    Port
    )
{
    EFI_STATUS   Status;
    UINT8  Value8;
    UINT16       Value16;
    UINT32     Index=0;
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
  //Read Data Before Enable Interrupt for compare with Enable Interrupt Read Data_E
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
 //set  APIC use FSB
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
    Status = mCpu->RegisterInterruptHandler(mCpu, 0x80+IRqNum, AHCI_ReadInterruptHandler);
    if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Unable to register Sata interrupt with CPU Arch Protocol.\n"));
        return EFI_DEVICE_ERROR;
    }
    DEBUG((EFI_D_ERROR,"mike_true_false\n"));
    Mike_IoApicConfigureInterrupt (IRqNum%0x18, 0x80+IRqNum, IO_APIC_DELIVERY_MODE_FIXED,TRUE, FALSE);
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
    Mike_IoApicEnableInterrupt(IRqNum%0x18,TRUE); 
    Ahci_APIC_Test(BlockIo,pRBuffer2);
    while(HasEnterInterrupt==FALSE)
    {
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
    while(1);
    Exit:
    Ahci_Report_Error(Port);
    MmioWrite8(IO_Apic_Base, (UINT8)(0x10+(IRqNum%0x18)*2));
    DEBUG((EFI_D_ERROR,"IRQ_Entry_Low:%x\n",MmioRead64(IO_Apic_Base+0x10)));
    MmioWrite8(IO_Apic_Base, (UINT8)(0x10+(IRqNum%0x18)*2+1));
    DEBUG((EFI_D_ERROR,"IRQ_Entry_High:%x\n",MmioRead64(IO_Apic_Base+0x10)));
    while(1);
    return EFI_SUCCESS;
}
///////////////////////////Mike_Add_Test_ACHI_Code  ////////////////////////////


/**
  Allocates an aligned buffer for ATA device.

  This function allocates an aligned buffer for the ATA device to perform
  ATA pass through operations. The alignment requirement is from ATA pass
  through interface.

  @param  AtaDevice         The ATA child device involved for the operation.
  @param  BufferSize        The request buffer size.

  @return A pointer to the aligned buffer or NULL if the allocation fails.

**/
VOID *
AllocateAlignedBuffer (
    IN ATA_DEVICE               *AtaDevice,
    IN UINTN                    BufferSize
    )
{
    return AllocateAlignedPages (EFI_SIZE_TO_PAGES (BufferSize), AtaDevice->AtaBusDriverData->AtaPassThru->Mode->IoAlign);
}

/**
  Frees an aligned buffer for ATA device.

  This function frees an aligned buffer for the ATA device to perform
  ATA pass through operations.

  @param  Buffer            The aligned buffer to be freed.
  @param  BufferSize        The request buffer size.

**/
VOID
FreeAlignedBuffer (
    IN VOID                     *Buffer,
    IN UINTN                    BufferSize
    )
{
    if (Buffer != NULL) {
        FreeAlignedPages (Buffer, EFI_SIZE_TO_PAGES (BufferSize));
    }
}


/**
  Release all the resources allocated for the ATA device.

  This function releases all the resources allocated for the ATA device.

  @param  AtaDevice         The ATA child device involved for the operation.

**/
VOID
ReleaseAtaResources (
    IN ATA_DEVICE  *AtaDevice
    )
{
    ATA_BUS_ASYN_TASK *Task;
    LIST_ENTRY        *Entry;
    LIST_ENTRY        *DelEntry;
    EFI_TPL           OldTpl;

    FreeUnicodeStringTable (AtaDevice->ControllerNameTable);
    FreeAlignedBuffer (AtaDevice->Asb, sizeof (EFI_ATA_STATUS_BLOCK));
    FreeAlignedBuffer (AtaDevice->IdentifyData, sizeof (ATA_IDENTIFY_DATA));
    if (AtaDevice->DevicePath != NULL) {
        FreePool (AtaDevice->DevicePath);
    }
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    if (!IsListEmpty (&AtaDevice->AtaTaskList)) {
    //
    // Free the Subtask list.
    //
        for(Entry = AtaDevice->AtaTaskList.ForwardLink; 
            Entry != (&AtaDevice->AtaTaskList);
            ) {
            DelEntry = Entry;
        Entry    = Entry->ForwardLink;
        Task     = ATA_AYNS_TASK_FROM_ENTRY (DelEntry);
        
        RemoveEntryList (DelEntry);
        FreeAtaSubTask (Task);
    }
}
gBS->RestoreTPL (OldTpl);
FreePool (AtaDevice);
}


/**
  Registers an ATA device.

  This function allocates an ATA device structure for the ATA device specified by
  Port and PortMultiplierPort if the ATA device is identified as a valid one. 
  Then it will create child handle and install Block IO and Disk Info protocol on
  it.

  @param  AtaBusDriverData      The parent ATA bus driver data structure.
  @param  Port                  The port number of the ATA device.
  @param  PortMultiplierPort    The port multiplier port number of the ATA device.

  @retval EFI_SUCCESS           The ATA device is successfully registered.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory to allocate the ATA device
                                and related data structures.
  @return Others                Some error occurs when registering the ATA device.
**/
EFI_STATUS
RegisterAtaDevice (
    IN OUT ATA_BUS_DRIVER_DATA        *AtaBusDriverData,
    IN     UINT16                     Port,
    IN     UINT16                     PortMultiplierPort
    )
{
    EFI_STATUS                        Status;
    ATA_DEVICE                        *AtaDevice;
    EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;
    EFI_DEVICE_PATH_PROTOCOL          *NewDevicePathNode;
    EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
    EFI_DEVICE_PATH_PROTOCOL          *RemainingDevicePath;
    EFI_HANDLE                        DeviceHandle;

    AtaDevice         = NULL;
    NewDevicePathNode = NULL;
    DevicePath        = NULL;
    RemainingDevicePath = NULL;

  //
  // Build device path 
  //
    AtaPassThru = AtaBusDriverData->AtaPassThru;
    Status = AtaPassThru->BuildDevicePath (AtaPassThru, Port, PortMultiplierPort, &NewDevicePathNode);
    if (EFI_ERROR (Status)) {
        goto Done;
    }

    DevicePath = AppendDevicePathNode (AtaBusDriverData->ParentDevicePath, NewDevicePathNode);
    if (DevicePath == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }

    DeviceHandle = NULL;
    RemainingDevicePath = DevicePath;
    Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &DeviceHandle);
    if (!EFI_ERROR (Status) && (DeviceHandle != NULL) && IsDevicePathEnd(RemainingDevicePath)) {
        Status = EFI_ALREADY_STARTED;
        FreePool (DevicePath);
        goto Done;
    }

    REPORT_STATUS_CODE (
        EFI_PROGRESS_CODE,
        (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_DETECT)
        );

  //
  // Allocate ATA device from the template.
  //
    AtaDevice = AllocateCopyPool (sizeof (ATA_DEVICE), &gAtaDeviceTemplate);
    if (AtaDevice == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }

  //
  // Initializes ATA device structures and allocates the required buffer.
  //
    AtaDevice->BlockIo.Media      = &AtaDevice->BlockMedia;
    AtaDevice->BlockIo2.Media     = &AtaDevice->BlockMedia;
    AtaDevice->AtaBusDriverData   = AtaBusDriverData;
    AtaDevice->DevicePath         = DevicePath;
    AtaDevice->Port               = Port;
    AtaDevice->PortMultiplierPort = PortMultiplierPort;
    AtaDevice->Asb = AllocateAlignedBuffer (AtaDevice, sizeof (EFI_ATA_STATUS_BLOCK));
    if (AtaDevice->Asb == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }
    AtaDevice->IdentifyData = AllocateAlignedBuffer (AtaDevice, sizeof (ATA_IDENTIFY_DATA));
    if (AtaDevice->IdentifyData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }

  //
  // Initial Ata Task List
  //
    InitializeListHead (&AtaDevice->AtaTaskList);

  //
  // Try to identify the ATA device via the ATA pass through command. 
  //
    Status = DiscoverAtaDevice (AtaDevice);
    if (EFI_ERROR (Status)) {
        goto Done;
    }

  //
  // Build controller name for Component Name (2) protocol.
  //
    Status = AddUnicodeString2 (
        "eng",
        gAtaBusComponentName.SupportedLanguages,
        &AtaDevice->ControllerNameTable,
        AtaDevice->ModelName,
        TRUE
        );
    if (EFI_ERROR (Status)) {
        goto Done;
    }

    Status = AddUnicodeString2 (
        "en",
        gAtaBusComponentName2.SupportedLanguages,
        &AtaDevice->ControllerNameTable,
        AtaDevice->ModelName,
        FALSE
        );
    if (EFI_ERROR (Status)) {
        goto Done;
    }

  //
  // Update to AHCI interface GUID based on device path node. The default one
  // is IDE interface GUID copied from template.
  //
    if (NewDevicePathNode->SubType == MSG_SATA_DP) {
        CopyGuid (&AtaDevice->DiskInfo.Interface, &gEfiDiskInfoAhciInterfaceGuid);
    }

    REPORT_STATUS_CODE (
        EFI_PROGRESS_CODE, 
        (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_ENABLE)
        );
    
    Status = gBS->InstallMultipleProtocolInterfaces (
        &AtaDevice->Handle,
        &gEfiDevicePathProtocolGuid,
        AtaDevice->DevicePath,
        &gEfiDiskInfoProtocolGuid,
        &AtaDevice->DiskInfo,
        NULL
        );
    if (EFI_ERROR (Status)) {
        goto Done;
    }

    if(!AtaDevice->Locked){
        Status = gBS->InstallMultipleProtocolInterfaces (
            &AtaDevice->Handle,
            &gEfiBlockIoProtocolGuid,
            &AtaDevice->BlockIo,
            &gEfiBlockIo2ProtocolGuid,
            &AtaDevice->BlockIo2,
            NULL
            );
        if (EFI_ERROR (Status)) {
            goto Done;
        }
    }
    Ahci_APIC_Initialize(AtaBusDriverData->Controller,&(AtaDevice->BlockIo),AtaDevice->DevicePath,Port);
   //Ahci_PIC_Initialize(AtaBusDriverData->Controller,&(AtaDevice->BlockIo),Port);  
   //Ahci_SMSI_Initialize(AtaBusDriverData->Controller,&(AtaDevice->BlockIo),Port);
  //Ahci_MMSI_Initialize(AtaBusDriverData->Controller,&(AtaDevice->BlockIo),Port);
  //Ahci_MSIX_Initialize(AtaBusDriverData->Controller,&(AtaDevice->BlockIo),Port);
  //
  // See if the ata device support trust computing feature or not.
  // If yes, then install Storage Security Protocol at the ata device handle.
  //
    if ((AtaDevice->IdentifyData->trusted_computing_support & BIT0) != 0) {
        DEBUG ((EFI_D_INFO, "Found TCG support in Port %x PortMultiplierPort %x\n", Port, PortMultiplierPort));
        Status = gBS->InstallProtocolInterface (
            &AtaDevice->Handle,
            &gEfiStorageSecurityCommandProtocolGuid,
            EFI_NATIVE_INTERFACE,
            &AtaDevice->StorageSecurity
            );
        if (EFI_ERROR (Status)) {
            goto Done;
        }
    }

    gBS->OpenProtocol (
        AtaBusDriverData->Controller,
        &gEfiAtaPassThruProtocolGuid,
        (VOID **) &AtaPassThru,
        AtaBusDriverData->DriverBindingHandle,
        AtaDevice->Handle,
        EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
        );

    Done:
    if (NewDevicePathNode != NULL) {
        FreePool (NewDevicePathNode);
    }

    if (EFI_ERROR (Status) && (AtaDevice != NULL)) {
        ReleaseAtaResources (AtaDevice);  
        DEBUG ((EFI_D_ERROR | EFI_D_INIT, "Failed to initialize Port %x PortMultiplierPort %x, status = %r\n", Port, PortMultiplierPort, Status));
    }
    return Status;
}


/**
  Unregisters an ATA device.

  This function removes the protocols installed on the controller handle and 
  frees the resources allocated for the ATA device. 

  @param  This                  The pointer to EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller            The controller handle of the ATA device.
  @param  Handle                The child handle.

  @retval EFI_SUCCESS           The ATA device is successfully unregistered.
  @return Others                Some error occurs when unregistering the ATA device.

**/
EFI_STATUS
UnregisterAtaDevice (
    IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
    IN  EFI_HANDLE                     Controller,
    IN  EFI_HANDLE                     Handle
    )
{
    EFI_STATUS                               Status;
    EFI_BLOCK_IO_PROTOCOL                    *BlockIo;
    ATA_DEVICE                               *AtaDevice;
    EFI_ATA_PASS_THRU_PROTOCOL               *AtaPassThru;
    EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *StorageSecurity;
    EFI_DISK_INFO_PROTOCOL                   *DiskInfo;
    

    Status = gBS->OpenProtocol (
        Handle,
        &gEfiDiskInfoProtocolGuid,
        (VOID**)&DiskInfo,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );
    if (EFI_ERROR (Status)) {
        return Status;
    }
    
    AtaDevice = ATA_DEVICE_FROM_DISK_INFO(DiskInfo);

    gBS->CloseProtocol (
        Controller,
        &gEfiAtaPassThruProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );  
    
    Status = gBS->UninstallMultipleProtocolInterfaces (
        Handle,
        &gEfiDevicePathProtocolGuid,
        AtaDevice->DevicePath,
        &gEfiDiskInfoProtocolGuid,
        &AtaDevice->DiskInfo,
        NULL
        );  
    if(EFI_ERROR(Status)){
        gBS->OpenProtocol (
            Controller,
            &gEfiAtaPassThruProtocolGuid,
            (VOID **) &AtaPassThru,
            This->DriverBindingHandle,
            Handle,
            EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
            );      
        return Status;
    }  
    
    DEBUG((EFI_D_INFO, "[AtaBus] UnInstall DP & DiskInfo OK\n"));
    
    Status = gBS->OpenProtocol (
        Handle,
        &gEfiBlockIoProtocolGuid,
        (VOID **)&BlockIo,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );   
    if(!EFI_ERROR(Status)){  
        Status = gBS->UninstallMultipleProtocolInterfaces (
            Handle,
            &gEfiBlockIoProtocolGuid,
            &AtaDevice->BlockIo,
            &gEfiBlockIo2ProtocolGuid,
            &AtaDevice->BlockIo2,
            NULL
            );
        if(EFI_ERROR(Status)){
            gBS->OpenProtocol (
                Controller,
                &gEfiAtaPassThruProtocolGuid,
                (VOID **) &AtaPassThru,
                This->DriverBindingHandle,
                Handle,
                EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                );          
            return Status;
        } 
        DEBUG((EFI_D_INFO, "[AtaBus] UnInstall BlockIo & BlockIo2 OK\n"));    
    }
    
  //
  // If Storage Security Command Protocol is installed, then uninstall this protocol.
  //
    Status = gBS->OpenProtocol (
        Handle,
        &gEfiStorageSecurityCommandProtocolGuid,
        (VOID **) &StorageSecurity,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );

    if (!EFI_ERROR (Status)) {
        Status = gBS->UninstallProtocolInterface (
            Handle,
            &gEfiStorageSecurityCommandProtocolGuid,
            &AtaDevice->StorageSecurity
            );
        if (EFI_ERROR (Status)) {
            gBS->OpenProtocol (
                Controller,
                &gEfiAtaPassThruProtocolGuid,
                (VOID **) &AtaPassThru,
                This->DriverBindingHandle,
                Handle,
                EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                );
            return Status;
        }
    }

    ReleaseAtaResources (AtaDevice);
    return EFI_SUCCESS;
}



/**
  Tests to see if this driver supports a given controller. If a child device is provided, 
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by 
  ControllerHandle. Drivers will typically use the device path attached to 
  ControllerHandle and/or the services from the bus I/O abstraction attached to 
  ControllerHandle to determine if the driver supports ControllerHandle. This function 
  may be called many times during platform initialization. In order to reduce boot times, the tests 
  performed by this function must be very small, and take as little time as possible to execute. This 
  function must not change the state of any hardware devices, and this function must be aware that the 
  device specified by ControllerHandle may already be managed by the same driver or a 
  different driver. This function must match its calls to AllocatePages() with FreePages(), 
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().  
  Since ControllerHandle may have been previously started by the same driver, if a protocol is 
  already in the opened state, then it must not be closed with CloseProtocol(). This is required 
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This 
                                   parameter is ignored by device drivers, and is optional for bus 
                                   drivers. For bus drivers, if this parameter is not NULL, then 
                                   the bus driver must determine if the bus controller specified 
                                   by ControllerHandle and the child controller specified 
                                   by RemainingDevicePath are both supported by this 
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
AtaBusDriverBindingSupported (
    IN EFI_DRIVER_BINDING_PROTOCOL  *This,
    IN EFI_HANDLE                   Controller,
    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
    )
{
    EFI_STATUS                        Status;
    EFI_DEVICE_PATH_PROTOCOL          *ParentDevicePath;
    EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;
    UINT16                            Port;
    UINT16                            PortMultiplierPort;

  //
  // Test EFI_ATA_PASS_THRU_PROTOCOL on controller handle.
  //
    Status = gBS->OpenProtocol (
        Controller,
        &gEfiAtaPassThruProtocolGuid,
        (VOID **) &AtaPassThru,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_BY_DRIVER
        );

    if (Status == EFI_ALREADY_STARTED) {
        return EFI_SUCCESS;
    }

    if (EFI_ERROR (Status)) {
        return Status;
    }

  //
  // Test RemainingDevicePath is valid or not.
  //
    if ((RemainingDevicePath != NULL) && !IsDevicePathEnd (RemainingDevicePath)) {
        Status = AtaPassThru->GetDevice (AtaPassThru, RemainingDevicePath, &Port, &PortMultiplierPort);
        if (EFI_ERROR (Status)) {
            return Status;
        }
    }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
    gBS->CloseProtocol (
        Controller,
        &gEfiAtaPassThruProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
    Status = gBS->OpenProtocol (
        Controller,
        &gEfiDevicePathProtocolGuid,
        (VOID **) &ParentDevicePath,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );
    return Status;
}


/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this 
  common boot service. It is legal to call Start() from other locations, 
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.  

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This 
                                   parameter is ignored by device drivers, and is optional for bus 
                                   drivers. For a bus driver, if this parameter is NULL, then handles 
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is 
                                   not the End of Device Path Node, then only the handle for the 
                                   child device specified by the first Device Path Node of 
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is 
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
AtaBusDriverBindingStart (
    IN EFI_DRIVER_BINDING_PROTOCOL  *This,
    IN EFI_HANDLE                   Controller,
    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
    )
{
    EFI_STATUS                        Status;
    EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;
    EFI_DEVICE_PATH_PROTOCOL          *ParentDevicePath;
    ATA_BUS_DRIVER_DATA               *AtaBusDriverData;
    UINT16                            Port;
    UINT16                            PortMultiplierPort;

    REPORT_STATUS_CODE (
        EFI_PROGRESS_CODE,
        (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_INIT)
        );

    AtaBusDriverData = NULL;

    Status = gBS->OpenProtocol (
        Controller,
        &gEfiDevicePathProtocolGuid,
        (VOID **) &ParentDevicePath,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    Status = gBS->OpenProtocol (
        Controller,
        &gEfiAtaPassThruProtocolGuid,
        (VOID **) &AtaPassThru,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_BY_DRIVER
        );
    if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
        goto ErrorExit;
    }

  //
  // Check EFI_ALREADY_STARTED to reuse the original ATA_BUS_DRIVER_DATA.
  //
    if (Status != EFI_ALREADY_STARTED) {
        AtaBusDriverData = AllocateZeroPool (sizeof (ATA_BUS_DRIVER_DATA));
        if (AtaBusDriverData == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            goto ErrorExit;
        }

        AtaBusDriverData->AtaPassThru = AtaPassThru;
        AtaBusDriverData->Controller  = Controller;
        AtaBusDriverData->ParentDevicePath = ParentDevicePath;
        AtaBusDriverData->DriverBindingHandle = This->DriverBindingHandle;

        Status = gBS->InstallMultipleProtocolInterfaces (
            &Controller,
            &gEfiCallerIdGuid,
            AtaBusDriverData,
            NULL
            );
        if (EFI_ERROR (Status)) {
            goto ErrorExit;
        }

    } else {
        Status = gBS->OpenProtocol (
            Controller,
            &gEfiCallerIdGuid,
            (VOID **) &AtaBusDriverData,
            This->DriverBindingHandle,
            Controller,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );
        if (EFI_ERROR (Status)) {
            AtaBusDriverData = NULL;
            goto ErrorExit;
        }
    }

    if (RemainingDevicePath == NULL) {
        Port = 0xFFFF;
        while (TRUE) {
            Status = AtaPassThru->GetNextPort (AtaPassThru, &Port);
            if (EFI_ERROR (Status)) {
        //
        // We cannot find more legal port then we are done.
        //
                break;
            }

            PortMultiplierPort = 0xFFFF;
            while (TRUE) {
                Status = AtaPassThru->GetNextDevice (AtaPassThru, Port, &PortMultiplierPort);
                if (EFI_ERROR (Status)) {
          //
          // We cannot find more legal port multiplier port number for ATA device
          // on the port, then we are done.
          //
                    break;
                }
                RegisterAtaDevice (AtaBusDriverData, Port, PortMultiplierPort);
            }
        }
        Status = EFI_SUCCESS;
    } else if (!IsDevicePathEnd (RemainingDevicePath)) {
        Status = AtaPassThru->GetDevice (AtaPassThru, RemainingDevicePath, &Port, &PortMultiplierPort);
        if (!EFI_ERROR (Status)) {
            Status = RegisterAtaDevice (AtaBusDriverData,Port, PortMultiplierPort);
        }
    }

    return Status;

    ErrorExit:

    if (AtaBusDriverData != NULL) {
        gBS->UninstallMultipleProtocolInterfaces (
            Controller,
            &gEfiCallerIdGuid,
            AtaBusDriverData,
            NULL
            );
        FreePool (AtaBusDriverData);
    }

    gBS->CloseProtocol (
        Controller,
        &gEfiAtaPassThruProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

    return Status;

}


/**
  Stops a device controller or a bus controller.
  
  The Stop() function is designed to be invoked from the EFI boot service DisconnectController(). 
  As a result, much of the error checking on the parameters to Stop() has been moved 
  into this common boot service. It is legal to call Stop() from other locations, 
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.
  
  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must 
                                support a bus specific I/O protocol for the driver 
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL 
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
AtaBusDriverBindingStop (
    IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
    IN  EFI_HANDLE                      Controller,
    IN  UINTN                           NumberOfChildren,
    IN  EFI_HANDLE                      *ChildHandleBuffer
    )
{
    EFI_STATUS                  Status;
    BOOLEAN                     AllChildrenStopped;
    UINTN                       Index;
    ATA_BUS_DRIVER_DATA         *AtaBusDriverData;

    if (NumberOfChildren == 0) {
        Status = gBS->OpenProtocol (
            Controller,
            &gEfiCallerIdGuid,
            (VOID **) &AtaBusDriverData,
            This->DriverBindingHandle,
            Controller,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );
        if (!EFI_ERROR (Status)) {
            gBS->UninstallMultipleProtocolInterfaces (
                Controller,
                &gEfiCallerIdGuid,
                AtaBusDriverData,
                NULL
                );
            FreePool (AtaBusDriverData);
        }

        gBS->CloseProtocol (
            Controller,
            &gEfiAtaPassThruProtocolGuid,
            This->DriverBindingHandle,
            Controller
            );

        return EFI_SUCCESS;
    }

    AllChildrenStopped = TRUE;

    for (Index = 0; Index < NumberOfChildren; Index++) {

        Status = UnregisterAtaDevice (This, Controller, ChildHandleBuffer[Index]);
        if (EFI_ERROR (Status)) {
            AllChildrenStopped = FALSE;
        }
    }

    if (!AllChildrenStopped) {
        return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
}


/**
  Reset the Block Device.

  @param  This                 Indicates a pointer to the calling context.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
AtaBlockIoReset (
    IN  EFI_BLOCK_IO_PROTOCOL   *This,
    IN  BOOLEAN                 ExtendedVerification
    )
{
    EFI_STATUS      Status;
    ATA_DEVICE      *AtaDevice;
    EFI_TPL         OldTpl;

    OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

    AtaDevice = ATA_DEVICE_FROM_BLOCK_IO (This);

    Status = ResetAtaDevice (AtaDevice);

    if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
    }

    gBS->RestoreTPL (OldTpl);
    return Status;
}


/**
  Read/Write BufferSize bytes from Lba from/into Buffer.

  @param[in]       This       Indicates a pointer to the calling context. Either be
                              block I/O or block I/O2. 
  @param[in]       MediaId    The media ID that the read/write request is for.
  @param[in]       Lba        The starting logical block address to be read/written.
                              The caller is responsible for reading/writing to only
                              legitimate locations.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[out]      Buffer     A pointer to the destination/source buffer for the data.
  @param[in]       IsBlockIo2 Indicate the calling is from BlockIO or BlockIO2. TURE is
                              from BlockIO2, FALSE is for BlockIO.
  @param[in]       IsWrite    Indicates whether it is a write operation.

  @retval EFI_SUCCESS           The data was read/written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be read/written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read/write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read/write request contains LBAs that are not valid, 
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
BlockIoReadWrite (
    IN     VOID                    *This,
    IN     UINT32                  MediaId,
    IN     EFI_LBA                 Lba,
    IN OUT EFI_BLOCK_IO2_TOKEN     *Token,
    IN     UINTN                   BufferSize,
    OUT    VOID                    *Buffer,
    IN     BOOLEAN                 IsBlockIo2,
    IN     BOOLEAN                 IsWrite
    )
{
    ATA_DEVICE                        *AtaDevice;
    EFI_STATUS                        Status;
    EFI_TPL                           OldTpl;
    EFI_BLOCK_IO_MEDIA                *Media;
    UINTN                             BlockSize;
    UINTN                             NumberOfBlocks;
    UINTN                             IoAlign;

    if (IsBlockIo2) {
        Media     = ((EFI_BLOCK_IO2_PROTOCOL *) This)->Media;
        AtaDevice = ATA_DEVICE_FROM_BLOCK_IO2 (This);
    } else {
        Media     = ((EFI_BLOCK_IO_PROTOCOL *) This)->Media;
        AtaDevice = ATA_DEVICE_FROM_BLOCK_IO (This);
    }

    if (MediaId != Media->MediaId) {
        return EFI_MEDIA_CHANGED;
    }

  //
  // Check parameters.
  //
    if (Buffer == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (BufferSize == 0) {
        return EFI_SUCCESS;
    }  

    BlockSize = Media->BlockSize;
    if ((BufferSize % BlockSize) != 0) {
        return EFI_BAD_BUFFER_SIZE;
    }
    
    NumberOfBlocks  = BufferSize / BlockSize;
    if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
        return EFI_INVALID_PARAMETER;
    }

    IoAlign = Media->IoAlign;
    if (IoAlign > 0 && (((UINTN) Buffer & (IoAlign - 1)) != 0)) {
        return EFI_INVALID_PARAMETER;
    }

    OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
    
  //
  // Invoke low level AtaDevice Access Routine.
  //
    Status = AccessAtaDevice (AtaDevice, Buffer, Lba, NumberOfBlocks, IsWrite, Token);
    
    gBS->RestoreTPL (OldTpl);

    return Status;
}


/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid, 
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
AtaBlockIoReadBlocks (
    IN  EFI_BLOCK_IO_PROTOCOL   *This,
    IN  UINT32                  MediaId,
    IN  EFI_LBA                 Lba,
    IN  UINTN                   BufferSize,
    OUT VOID                    *Buffer
    )
{
    return BlockIoReadWrite ((VOID *) This, MediaId, Lba, NULL, BufferSize, Buffer, FALSE, FALSE);
}


/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid, 
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
AtaBlockIoWriteBlocks (
    IN  EFI_BLOCK_IO_PROTOCOL   *This,
    IN  UINT32                  MediaId,
    IN  EFI_LBA                 Lba,
    IN  UINTN                   BufferSize,
    IN  VOID                    *Buffer
    )
{
    return BlockIoReadWrite ((VOID *) This, MediaId, Lba, NULL, BufferSize, Buffer, FALSE, TRUE);
}


/**
  Flush the Block Device.

  @param  This              Indicates a pointer to the calling context.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writing back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
AtaBlockIoFlushBlocks (
    IN  EFI_BLOCK_IO_PROTOCOL   *This
    )
{
  //
  // return directly
  //
    return EFI_SUCCESS;
}

/**
  Reset the Block Device.

  @param[in]  This                 Indicates a pointer to the calling context.
  @param[in]  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
AtaBlockIoResetEx (
    IN  EFI_BLOCK_IO2_PROTOCOL  *This,
    IN  BOOLEAN                 ExtendedVerification
    )
{
    EFI_STATUS      Status;
    ATA_DEVICE      *AtaDevice;
    EFI_TPL         OldTpl;

    OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

    AtaDevice = ATA_DEVICE_FROM_BLOCK_IO2 (This);

    Status = ResetAtaDevice (AtaDevice);

    if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
    }

    gBS->RestoreTPL (OldTpl);
    return Status;
}

/**
  Read BufferSize bytes from Lba into Buffer.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    Id of the media, changes every time the media is replaced.
  @param[in]       Lba        The starting Logical Block Address to read from.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[out]      Buffer     A pointer to the destination buffer for the data. The caller is
                              responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The read request was queued if Event is not NULL.
                                The data was read correctly from the device if
                                the Event is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing
                                the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of the
                                intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid, 
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.

**/
EFI_STATUS
EFIAPI
AtaBlockIoReadBlocksEx (
    IN  EFI_BLOCK_IO2_PROTOCOL  *This,
    IN  UINT32                  MediaId,
    IN  EFI_LBA                 Lba,
    IN OUT EFI_BLOCK_IO2_TOKEN  *Token,
    IN  UINTN                   BufferSize,
    OUT VOID                    *Buffer
    )
{
    return BlockIoReadWrite ((VOID *) This, MediaId, Lba, Token, BufferSize, Buffer, TRUE, FALSE);
}


/**
  Write BufferSize bytes from Lba into Buffer.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    The media ID that the write request is for.
  @param[in]       Lba        The starting logical block address to be written. The
                              caller is responsible for writing to only legitimate
                              locations.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[in]       Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid, 
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
AtaBlockIoWriteBlocksEx (
    IN  EFI_BLOCK_IO2_PROTOCOL  *This,
    IN  UINT32                  MediaId,
    IN  EFI_LBA                 Lba,
    IN OUT EFI_BLOCK_IO2_TOKEN  *Token,
    IN  UINTN                   BufferSize,
    IN  VOID                    *Buffer
    )
{
    return BlockIoReadWrite ((VOID *) This, MediaId, Lba, Token, BufferSize, Buffer, TRUE, TRUE);
}


/**
  Flush the Block Device.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in, out]  Token      A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writing back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
AtaBlockIoFlushBlocksEx (
    IN  EFI_BLOCK_IO2_PROTOCOL  *This,
    IN OUT EFI_BLOCK_IO2_TOKEN  *Token
    )
{
  //
  // Signal event and return directly.
  //
    if (Token != NULL && Token->Event != NULL) {
        Token->TransactionStatus = EFI_SUCCESS;
        gBS->SignalEvent (Token->Event);
    }
    return EFI_SUCCESS;
}
/**
  Provides inquiry information for the controller type.
  
  This function is used by the IDE bus driver to get inquiry data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]      This             Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] InquiryData      Pointer to a buffer for the inquiry data.
  @param[in, out] InquiryDataSize  Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class 
  @retval EFI_DEVICE_ERROR       Error reading InquiryData from device 
  @retval EFI_BUFFER_TOO_SMALL   InquiryDataSize not big enough 

**/
EFI_STATUS
EFIAPI
AtaDiskInfoInquiry (
    IN     EFI_DISK_INFO_PROTOCOL   *This,
    IN OUT VOID                     *InquiryData,
    IN OUT UINT32                   *InquiryDataSize
    )
{
    return EFI_NOT_FOUND;
}


/**
  Provides identify information for the controller type.

  This function is used by the IDE bus driver to get identify data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL 
                                    instance.
  @param[in, out] IdentifyData      Pointer to a buffer for the identify data.
  @param[in, out] IdentifyDataSize  Pointer to the value for the identify data
                                    size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class 
  @retval EFI_DEVICE_ERROR       Error reading IdentifyData from device 
  @retval EFI_BUFFER_TOO_SMALL   IdentifyDataSize not big enough 

**/
EFI_STATUS
EFIAPI
AtaDiskInfoIdentify (
    IN     EFI_DISK_INFO_PROTOCOL   *This,
    IN OUT VOID                     *IdentifyData,
    IN OUT UINT32                   *IdentifyDataSize
    )
{
    EFI_STATUS                      Status;
    ATA_DEVICE                      *AtaDevice;

    AtaDevice = ATA_DEVICE_FROM_DISK_INFO (This);

    Status = EFI_BUFFER_TOO_SMALL;
    if (*IdentifyDataSize >= sizeof (ATA_IDENTIFY_DATA)) {
        Status = EFI_SUCCESS;
        CopyMem (IdentifyData, AtaDevice->IdentifyData, sizeof (ATA_IDENTIFY_DATA));
    }
    *IdentifyDataSize = sizeof (ATA_IDENTIFY_DATA);

    return Status;
}


/**
  Provides sense data information for the controller type.
  
  This function is used by the IDE bus driver to get sense data. 
  Data format of Sense data is defined by the Interface GUID.

  @param[in]      This             Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] SenseData        Pointer to the SenseData.
  @param[in, out] SenseDataSize    Size of SenseData in bytes.
  @param[out]     SenseDataNumber  Pointer to the value for the sense data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class.
  @retval EFI_DEVICE_ERROR       Error reading SenseData from device.
  @retval EFI_BUFFER_TOO_SMALL   SenseDataSize not big enough.

**/
EFI_STATUS
EFIAPI
AtaDiskInfoSenseData (
    IN     EFI_DISK_INFO_PROTOCOL   *This,
    IN OUT VOID                     *SenseData,
    IN OUT UINT32                   *SenseDataSize,
    OUT    UINT8                    *SenseDataNumber
    )
{
    return EFI_NOT_FOUND;
}


/**
  This function is used by the IDE bus driver to get controller information.

  @param[in]  This         Pointer to the EFI_DISK_INFO_PROTOCOL instance. 
  @param[out] IdeChannel   Pointer to the Ide Channel number.  Primary or secondary.
  @param[out] IdeDevice    Pointer to the Ide Device number.  Master or slave.

  @retval EFI_SUCCESS       IdeChannel and IdeDevice are valid.
  @retval EFI_UNSUPPORTED   This is not an IDE device.

**/
EFI_STATUS
EFIAPI
AtaDiskInfoWhichIde (
    IN  EFI_DISK_INFO_PROTOCOL   *This,
    OUT UINT32                   *IdeChannel,
    OUT UINT32                   *IdeDevice
    )
{
    ATA_DEVICE                   *AtaDevice;

    AtaDevice       = ATA_DEVICE_FROM_DISK_INFO (This);
    *IdeChannel     = AtaDevice->Port;
    *IdeDevice      = AtaDevice->PortMultiplierPort;

    return EFI_SUCCESS;
}

/**
  Send a security protocol command to a device that receives data and/or the result
  of one or more commands sent by SendData.

  The ReceiveData function sends a security protocol command to the given MediaId.
  The security protocol command sent is defined by SecurityProtocolId and contains
  the security protocol specific data SecurityProtocolSpecificData. The function
  returns the data from the security protocol command in PayloadBuffer.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL IN command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED RECEIVE commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero.

  If the PayloadBufferSize is zero, the security protocol command is sent using the
  Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBufferSize is too small to store the available data from the security
  protocol command, the function shall copy PayloadBufferSize bytes into the
  PayloadBuffer and return EFI_WARN_BUFFER_TOO_SMALL.

  If PayloadBuffer or PayloadTransferSize is NULL and PayloadBufferSize is non-zero,
  the function shall return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function shall
  return EFI_UNSUPPORTED. If there is no media in the device, the function returns
  EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the device,
  the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall
  return EFI_SUCCESS. If the security protocol command completes with an error, the
  function shall return EFI_DEVICE_ERROR.

  @param  This                         Indicates a pointer to the calling context.
  @param  MediaId                      ID of the medium to receive data from.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param  PayloadBuffer                A pointer to a destination buffer to store the security
                                       protocol command specific payload data for the security
                                       protocol command. The caller is responsible for having
                                       either implicit or explicit ownership of the buffer.
  @param  PayloadTransferSize          A pointer to a buffer to store the size in bytes of the
                                       data written to the payload data buffer.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL    The PayloadBufferSize was too small to store the available
                                       data from the device. The PayloadBuffer contains the truncated data.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer or PayloadTransferSize is NULL and
                                       PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
AtaStorageSecurityReceiveData (
    IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
    IN UINT32                                   MediaId,
    IN UINT64                                   Timeout,
    IN UINT8                                    SecurityProtocolId,
    IN UINT16                                   SecurityProtocolSpecificData,
    IN UINTN                                    PayloadBufferSize,
    OUT VOID                                    *PayloadBuffer,
    OUT UINTN                                   *PayloadTransferSize
    )
{
    EFI_STATUS                       Status;
    ATA_DEVICE                       *Private;
    EFI_TPL                          OldTpl;

    DEBUG ((EFI_D_INFO, "EFI Storage Security Protocol - Read"));
    if ((PayloadBuffer == NULL || PayloadTransferSize == NULL) && PayloadBufferSize != 0) {
        return EFI_INVALID_PARAMETER;
    }

    Status  = EFI_SUCCESS;
    Private = ATA_DEVICE_FROM_STORAGE_SECURITY (This);

    if (MediaId != Private->BlockIo.Media->MediaId) {
        return EFI_MEDIA_CHANGED;
    }

    if (!Private->BlockIo.Media->MediaPresent) {
        return EFI_NO_MEDIA;
    }

    OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

    Status = TrustTransferAtaDevice (
        Private,
        PayloadBuffer,
        SecurityProtocolId,
        SecurityProtocolSpecificData,
        PayloadBufferSize,
        FALSE,
        Timeout,
        PayloadTransferSize
        );

    gBS->RestoreTPL (OldTpl);
    return Status;
}

/**
  Send a security protocol command to a device.

  The SendData function sends a security protocol command containing the payload
  PayloadBuffer to the given MediaId. The security protocol command sent is
  defined by SecurityProtocolId and contains the security protocol specific data
  SecurityProtocolSpecificData. If the underlying protocol command requires a
  specific padding for the command payload, the SendData function shall add padding
  bytes to the command payload to satisfy the padding requirements.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL OUT command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED SEND commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero. If the PayloadBufferSize is zero, the security protocol command is
  sent using the Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBuffer is NULL and PayloadBufferSize is non-zero, the function shall
  return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function
  shall return EFI_UNSUPPORTED. If there is no media in the device, the function
  returns EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the
  device, the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall return
  EFI_SUCCESS. If the security protocol command completes with an error, the function
  shall return EFI_DEVICE_ERROR.

  @param  This                         Indicates a pointer to the calling context.
  @param  MediaId                      ID of the medium to receive data from.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param  PayloadBuffer                A pointer to a destination buffer to store the security
                                       protocol command specific payload data for the security
                                       protocol command.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer is NULL and PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
AtaStorageSecuritySendData (
    IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
    IN UINT32                                   MediaId,
    IN UINT64                                   Timeout,
    IN UINT8                                    SecurityProtocolId,
    IN UINT16                                   SecurityProtocolSpecificData,
    IN UINTN                                    PayloadBufferSize,
    IN VOID                                     *PayloadBuffer
    )
{
    EFI_STATUS                       Status;
    ATA_DEVICE                       *Private;
    EFI_TPL                          OldTpl;

    DEBUG ((EFI_D_INFO, "EFI Storage Security Protocol - Send"));
    if ((PayloadBuffer == NULL) && (PayloadBufferSize != 0)) {
        return EFI_INVALID_PARAMETER;
    }

    Status  = EFI_SUCCESS;
    Private = ATA_DEVICE_FROM_STORAGE_SECURITY (This);

    if (MediaId != Private->BlockIo.Media->MediaId) {
        return EFI_MEDIA_CHANGED;
    }

    OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
    Status = TrustTransferAtaDevice (
        Private,
        PayloadBuffer,
        SecurityProtocolId,
        SecurityProtocolSpecificData,
        PayloadBufferSize,
        TRUE,
        Timeout,
        NULL
        );

    gBS->RestoreTPL (OldTpl);
    return Status;
}

/**
  The user Entry Point for module AtaBus. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeAtaBus(
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
    Status = EfiLibInstallDriverBindingComponentName2 (
        ImageHandle,
        SystemTable,
        &gAtaBusDriverBinding,
        ImageHandle,
        &gAtaBusComponentName,
        &gAtaBusComponentName2
        );
    ASSERT_EFI_ERROR (Status);

    return Status;
}
