
#include <PlatformDefinition2.h>

#ifndef MDEPKG_NDEBUG      // debug mode
#define ASL_COM(x)   DBGC x
#else
#define ASL_COM(x)
#endif

DefinitionBlock ("DSDT.aml", "DSDT", 2, "_BYO_ ", "ZX_PLAT", 0x00000001)
{
    Name (BRB, 0x0000)          // bus start
    Name (BRL, 0x0100)          // bus length
    Name (IOB, 0x0D00)
    Name (IOL, 0xF300)
            
    Name (PEBS, 0xE0000000)     // PcieBase
    Name (PESZ, 0x10000000)
    Name (PMBS, 0x0800)         // AcpiPmIoBase
    Name (PMLN, 0x0100)
    Name (PM28, 0x0828)    
    Name (PICM, Zero)           // PIC Mode    
    Name (INTE, 0x80)
    Name (INTF, 0x80)
    Name (INTG, 0x80)
    Name (INTH, 0x80)

    Name (PEHP, One)            // PCIE Hot Plug
    Name (SHPC, One)            // PCI/PCI-X Standard Hot Plug
    Name (PEPM, Zero)           // PCI Express Native Power Management Events control
    Name (PEER, Zero)           // PCI Express Advanced Error Reporting control    
    Name (PECS, One)            // PCI Express Capability Structure control 

    
    Name(_S0, Package(4){0x00, 0x00, 0x00, 0x00})
    Name(_S1, Package(4){0x04, 0x00, 0x00, 0x00})
    Name(_S3, Package(4){0x01, 0x00, 0x00, 0x00})
    Name(_S4, Package(4){0x03, 0x00, 0x00, 0x00})
    Name(_S5, Package(4){0x02, 0x00, 0x00, 0x00})       

    
#include "AcpiRam.asl"
#include "misc.asl"

    Scope (_SB)
    {
//Dongbo modified, Start.
#if defined(HX001EA1_10)
		include("HX001EA1_10\pcitree.asl")
#elif defined(HX001EB1_10)
		include("HX001EB1_10\pcitree.asl")
#elif defined(HX001EC1_04)
		include("HX001EC1_04\pcitree.asl")
#elif defined(HX001EC1_05)
		include("HX001EC1_05\pcitree.asl")
#elif defined(HX001EC1_08)
		include("HX001EC1_08\pcitree.asl")
#elif defined(HX001EC1_09)
		include("HX001EC1_09\pcitree.asl")
#elif defined(HX001EC1_12)
		include("HX001EC1_12\pcitree.asl")
#elif defined(HX001EC1_13)
		include("HX001EC1_13\pcitree.asl")
#elif defined(HX001ED0_03)
		include("HX001ED0_03\pcitree.asl")
#elif defined(HX001EE0_03)
		include("HX001EE0_03\pcitree.asl")
#elif defined(HX001EF0_06)
		include("HX001EF0_06\pcitree.asl")
#elif defined(HX001EF0_10)
		include("HX001EF0_10\pcitree.asl")
#elif defined(HX001EF0_14)
		include("HX001EF0_14\pcitree.asl")
#elif defined(HX001EG0_09)
		include("HX001EG0_09\pcitree.asl")
#elif defined(HX001EG0_10)
		include("HX001EG0_10\pcitree.asl")
#elif defined(HX001EG0_11)
		include("HX001EG0_11\pcitree.asl")
#elif defined(HX001EH0_09)
		include("HX001EH0_09\pcitree.asl")
#elif defined(HX001EH0_10)
		include("HX001EH0_10\pcitree.asl")
#elif defined(HX001EH0_11)
		include("HX001EH0_11\pcitree.asl")
#elif defined(HX001EI0_01)
		include("HX001EI0_01\pcitree.asl")
#elif defined(HX001EI0_02)
		include("HX001EI0_02\pcitree.asl")
#elif defined(HX001EI0_03)
		include("HX001EI0_03\pcitree.asl")
#endif
//Dongbo modified, End.

        Device (PCI0)
        {
            Name (_HID, EisaId ("PNP0A08"))  
            Name (_CID, EisaId ("PNP0A03"))  // _CID: Compatible ID
            Name (_ADR, Zero)                
            Method (^BN00, 0, NotSerialized)
            {
                Return (Zero)
            }

            Method (_BBN, 0, NotSerialized)  // _BBN: BIOS Bus Number
            {
                Return (BN00())
            }

            Name (_UID, Zero)  
            Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
            {
                If (PICM)
                {
                    Return (AR00)
                }

                Return (PR00)
            }

            Method (_PRW, 0, NotSerialized)  
            {
                Return (GPRW (0x05, \MXDW))
            }

            Name (CRS1, ResourceTemplate ()
            {
                WordBusNumber (ResourceProducer, MinFixed, MaxFixed, PosDecode,         // Pci Bus Range
                    0x0000, 
                    0x0000,             
                    0x0000,             
                    0x0000, 
                    0x0000,             
                    ,, BUSN
                    )

                IO (Decode16, 0x0CF8, 0x0CF8, 0x01, 0x08, )                             // Io[CF8, CFF]

                WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,   // Io[0,3AF]
                    0x0000,             // Granularity
                    0x0000,             
                    0x03AF,             
                    0x0000,             // Translation Offset
                    0x03B0,             
                    ,, , TypeStatic)

                WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,   // Io[3E0, CF7]
                    0x0000,             // Granularity
                    0x03E0,             
                    0x0CF7,             
                    0x0000,             // Translation Offset
                    0x0918,             
                    ,, , TypeStatic)

                WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,   // Pci_Io
                    0x0000,
                    0x0000,             
                    0x0000,             
                    0x0000,
                    0x0000,             
                    ,, IORG, TypeStatic)

                WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,   // VGA_Io[3B0,3DF]
                    0x0000,
                    0x03B0,             
                    0x03DF,             
                    0x0000,
                    0x0030,             
                    ,,, TypeStatic)

                DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, Cacheable, ReadWrite,   // VGA_Mem[A0000,BFFFF]
                    0x00000000,
                    0x000A0000,         
                    0x000BFFFF,         
                    0x00000000,
                    0x00020000,         
                    ,,, AddressRangeMemory, TypeStatic)

                DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,  // Mem[C0000,DFFFF]
                    0x00000000,
                    0x000C0000,         
                    0x000DFFFF,         
                    0x00000000,
                    0x00020000,         
                    ,,, AddressRangeMemory, TypeStatic)

                DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,  // PciMmio
                    0x00000000,
                    0x00000000,         
                    0x00000000,         
                    0x00000000,
                    0x00000000,         
                    ,, MMIO, AddressRangeMemory, TypeStatic)

                QWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,  // PCI64
                    0x0000000000000000,
                    0x0000000030584946,     // "FIX0", update it in acpiplatform.
                    0x0000000030584946,     // "FIX0", update it in acpiplatform.
                    0x0000000000000000,
                    0x0000000000000001,     // update it in acpiplatform.
                    ,,, AddressRangeMemory, TypeStatic)
            })

            Method (_STA, 0, NotSerialized)  
            {
                Return (0xF)
            }

            Method (_CRS, 0, NotSerialized)  
            {
                CreateWordField (CRS1, \_SB.PCI0.BUSN._MIN, MIN0)  
                CreateWordField (CRS1, \_SB.PCI0.BUSN._MAX, MAX0)  
                CreateWordField (CRS1, \_SB.PCI0.BUSN._LEN, LEN0)  
                Store (BRB, MIN0)
                Store (BRL, LEN0)
                Store (LEN0, Local0)
                Add (MIN0, Decrement(Local0), MAX0)

                CreateWordField (CRS1, \_SB.PCI0.IORG._MIN, MIN1)  
                CreateWordField (CRS1, \_SB.PCI0.IORG._MAX, MAX1)  
                CreateWordField (CRS1, \_SB.PCI0.IORG._LEN, LEN1)  
                Store (IOB, MIN1)
                Store (IOL, LEN1)
                Store (LEN1, Local0)
                Add (MIN1, Decrement(Local0), MAX1)

                CreateDWordField (CRS1, \_SB.PCI0.MMIO._MIN, MIN3)  
                CreateDWordField (CRS1, \_SB.PCI0.MMIO._MAX, MAX3)  
                CreateDWordField (CRS1, \_SB.PCI0.MMIO._LEN, LEN3)  
                Store (MBB, MIN3)
                Store (MBL, LEN3)
                Store (LEN3, Local0)
                Add (MIN3, Decrement(Local0), MAX3)

                Return (CRS1)
            }

            
            
            
            
// _OSC: Operating System Capabilities
// Arg0 (Buffer)  : UUID
// Arg1 (Integer) : Revision ID
// Arg2 (Integer) : Count
// Arg3 (Buffer)  : Capabilities Buffer
//
//BIT0: Extended PCI Config operation regions supported
//BIT1: Active State Power Management supported
//BIT2: Clock Power Management Capability supported
//BIT3: PCI Segment Groups supported
//BIT4: MSI supported 

//BIT0: PCI Express Native Hot Plug control
//BIT1: SHPC Native Hot Plug control 
//BIT2: PCI Express Native Power Management Events control 
//BIT3: PCI Express Advanced Error Reporting control
//BIT4: PCI Express Capability Structure control 

            Method(_OSC, 4, Serialized)
            {
                Name (SUPP, Zero)
                Name (CTRL, Zero)
                CreateDWordField (Arg3, Zero, CDW1)   // generic
                CreateDWordField (Arg3, 0x04, CDW2)   // Support Field
                CreateDWordField (Arg3, 0x08, CDW3)   // Control Field
                If (LEqual (Arg0, ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766"))){   // PCI/PCI-X/PCI Express hierarchy
                    Store (CDW2, SUPP)
                    Store (CDW3, CTRL)
                     
                    If (LNotEqual (And (SUPP, 0x16), 0x16))
                    {
                        And (CTRL, 0x1E, CTRL)
                    }

                    If (LNot (PEHP))
                    {
                        And (CTRL, 0x1E, CTRL)
                    }

                    If (LNot (SHPC))
                    {
                        And (CTRL, 0x1D, CTRL)
                    }

                    If (LNot (PEPM))
                    {
                        And (CTRL, 0x1B, CTRL)
                    }

                    If (LNot (PEER))
                    {
                        And (CTRL, 0x15, CTRL)
                    }

                    If (LNot (PECS))
                    {
                        And (CTRL, 0x0F, CTRL)
                    }

                    If (LNotEqual (Arg1, One))
                    {
                        Or (CDW1, 0x08, CDW1)     // Unrecognized Revision
                    }

                    If (LNotEqual (CDW3, CTRL))
                    {
                        Or (CDW1, 0x10, CDW1)     // Capabilities Masked
                    }

                    Store (CTRL, CDW3)
                    Return (Arg3)
                }
                Else
                {
                    Or (CDW1, 0x04, CDW1)         // Unrecognized UUID
                    Return (Arg3)
                }
            }

            Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
            {
                Return (0x03)
            }

            Device (MCH)
            {
                Name (_HID, EisaId ("PNP0C01"))   
                Name (_UID, 0x0A)                 
                Name (VNBR, ResourceTemplate ()
                {
                    Memory32Fixed (ReadWrite, 0x00000000, 0x00000000, PCIB)  // PCIE Base
                    Memory32Fixed (ReadWrite, 0xFEE00000, 0x00100000 )       // LocalApic
                    Memory32Fixed (ReadWrite, 0xFECC0000, 0x000C0000 )       // NbIoApic
                    Memory32Fixed (ReadWrite, 0xFE014000, 0x00004000 )       // PcieEPHY
                    Memory32Fixed (ReadWrite, 0xFE011000, 0x00002000 )       // RCRB
                })
                
                Method (_CRS, 0, NotSerialized)  
                {
                  CreateDWordField (VNBR, \_SB.PCI0.MCH.PCIB._LEN, PENL)  
                  CreateDWordField (VNBR, \_SB.PCI0.MCH.PCIB._BAS, PENB)  
                  Store (PEBS, PENB)
                  Store (PESZ, PENL)

                  Return (VNBR)
                }
            }

            Device (NBF1)
            {
                Name (_ADR, One)  
            }

            Device (NBF2)
            {
                Name (_ADR, 0x02)  
            }

            Device (NBF3)
            {
                Name (_ADR, 0x03)  
            }

            Device (NBF4)
            {
                Name (_ADR, 0x04)  
            }

            Device (NBF5)
            {
                Name (_ADR, 0x05)  
                Method (_STA, 0, NotSerialized)  
                {
                    Return (0x0B)
                }
            }

            Device (NBF6)
            {
                Name (_ADR, 0x06)  
                Scope (\_SB.PCI0.NBF6)
                {
                    OperationRegion (NSCR, PCI_Config, Zero, 0x0100)
                    Field (NSCR, ByteAcc, NoLock, Preserve)
                    {
                        Offset (0x5D),
                        MOBC,   1,
                        ACAD,   1,
                        US4C,   1,

                        Offset (0x71),
                            ,   3,
                        W8FL,   1,
                        Offset (0x72),
                        CSTC,   4,        // cstate
                        Offset (0x74),
                        NOPR,   8         // CpuCoreCount
                    }
                }
            }

            
//Dongbo modified, Start.
#if defined(HX001EA1_10)
		include("HX001EA1_10\pcie.asl")
#elif defined(HX001EB1_10)
		include("HX001EB1_10\pcie.asl")
#elif defined(HX001EC1_04)
		include("HX001EC1_04\pcie.asl")
#elif defined(HX001EC1_05)
		include("HX001EC1_05\pcie.asl")
#elif defined(HX001EC1_08)
		include("HX001EC1_08\pcie.asl")
#elif defined(HX001EC1_09)
		include("HX001EC1_09\pcie.asl")
#elif defined(HX001EC1_12)
		include("HX001EC1_12\pcie.asl")
#elif defined(HX001EC1_13)
		include("HX001EC1_13\pcie.asl")
#elif defined(HX001ED0_03)
		include("HX001ED0_03\pcie.asl")
#elif defined(HX001EE0_03)
		include("HX001EE0_03\pcie.asl")
#elif defined(HX001EF0_06)
		include("HX001EF0_06\pcie.asl")
#elif defined(HX001EF0_10)
		include("HX001EF0_10\pcie.asl")
#elif defined(HX001EF0_14)
		include("HX001EF0_14\pcie.asl")
#elif defined(HX001EG0_09)
		include("HX001EG0_09\pcie.asl")
#elif defined(HX001EG0_10)
		include("HX001EG0_10\pcie.asl")
#elif defined(HX001EG0_11)
		include("HX001EG0_11\pcie.asl")
#elif defined(HX001EH0_09)
		include("HX001EH0_09\pcie.asl")
#elif defined(HX001EH0_10)
		include("HX001EH0_10\pcie.asl")
#elif defined(HX001EH0_11)
		include("HX001EH0_11\pcie.asl")
#elif defined(HX001EI0_01)
		include("HX001EI0_01\pcie.asl")
#elif defined(HX001EI0_02)
		include("HX001EI0_02\pcie.asl")
#elif defined(HX001EI0_03)
		include("HX001EI0_03\pcie.asl")
#endif
//Dongbo modified, End.
            
            include("gfx.asl")
            //include("sata.asl")
            //include("usb.asl")


            Device (SBRG)
            {
                Name(_ADR, 0x00110000)

                Method (SPTS, 1, NotSerialized)
                {
                  If (LEqual (Arg0, One)) {
                      While (PIRS) {
                          Stall (0x50)
                          Store (One, PIRS)
                      }
                  }
                }

                Method (SWAK, 1, NotSerialized) {
        
                  If (RTCS) {
                    If (LEqual(Arg0, 0x04)) {
                       Notify(PWRB, 0x02)
                    }
                  } Else {
                    Notify(PWRB, 0x02)
                  }
                  
                  //^^SATA._INI()
                  //Store (Zero, ^^USB1.U184)
                  //Store (Zero, ^^USB2.U284)
                  //Store (Zero, ^^USB3.U384)
                  //Store (Zero, ^^EHCI.EH84)
                  //Store (Zero, ^^XHCI.XH84)
                }

                Scope (\_SB.PCI0.SBRG)
                {
                    OperationRegion (SMBA, PCI_Config, 0xD2, One)
                    Field (SMBA, ByteAcc, NoLock, Preserve)
                    {
                            ,   3,
                        SCIS,   1
                    }
                }

                OperationRegion (SMIE, SystemIO, PM28, 4)
                Field (SMIE, ByteAcc, NoLock, Preserve) {
                        , 7,
                    PIRS, 1,
                }
               
                Device (ACDP)
                {
                    Name (_HID, "ACPI0003")
                    Name (_PCL, Package (0x01)
                    {
                        _SB
                    })
                    Method (_STA, 0, NotSerialized)  
                    {
                        If (LEqual (^^^NBF6.MOBC, One))
                        {
                            Return (0x0F)
                        }
                        Else
                        {
                            Return (Zero)
                        }
                    }

                    Method (_PSR, 0, NotSerialized)  // _PSR: Power Source
                    {
                        If (LEqual (^^^NBF6.ACAD, One))
                        {
                            Return (One)
                        }
                        Else
                        {
                            Return (Zero)
                        }
                    }
                }

                Scope (\_SB)
                {
                    Scope (PCI0)
                    {
                        Method(_INI,0)
                        {
                       
                          // Determine the OS and store the value, where:
                          //
                          //   OSYS = 0x1000 = Linux.
                          //   OSYS = 0x2000 = WIN2000.
                          //   OSYS = 0x2001 = WINXP, RTM or SP1.
                          //   OSYS = 0x2002 = WINXP SP2.
                          //   OSYS = 0x2006 = Vista.
                          //   OSYS = 0x2009 = Windows 7 and Windows Server 2008 R2.
                          //   OSYS = 0x2012 = Windows 8	
                          //   OSYS = 0x2013 = Windows 8.1
                          //
                          // Assume Windows 2000 at a minimum.
                          
                          Store(0x2000,OSYS)
                          
                          If(CondRefOf(\_OSI))
                          {
                            //If(\_OSI("Linux"))       // deprecated
                            //{
                            //  Store(0x1000, OSYS)
                            //}
                            
                            If(\_OSI("Windows 2001"))
                            {
                              Store(0x2001, OSYS)
                            }
                            
                            If(\_OSI("Windows 2001 SP1"))
                            {
                              Store(0x2001, OSYS)
                            }
                            
                            If(\_OSI("Windows 2001 SP2"))
                            {
                              Store(0x2002, OSYS)
                            }
                            
                            If (\_OSI( "Windows 2001.1"))
                            {
                              Store(0x2003, OSYS)
                            }

                            If(\_OSI("Windows 2006"))
                            {
                              Store(0x2006, OSYS)
                            }

                            If(\_OSI("Windows 2009"))
                            {
                              Store(0x2009, OSYS)
                            }

                            If(\_OSI("Windows 2012"))
                            {
                              Store(0x2012, OSYS)
                            } 

                            If(\_OSI("Windows 2013"))
                            {
                              Store(0x2013, OSYS)
                            }                               
                          }
                        }                    
                    
                        Device (VTSB)
                        {
                            Name (_HID, EisaId ("PNP0C01"))       // System board
                            Name (_UID, 0x01C7)
                            Name (_STA, 0x0F)
                            Name (VSBR, ResourceTemplate ()
                            {
                                IO (Decode16,                     // PMBase
                                    0x0000,             
                                    0x0000,             
                                    0x00,               
                                    0x00,               
                                    _Y0D)
                                    
                                IO (Decode16, SMBUS_BASE_ADDRESS, SMBUS_BASE_ADDRESS, 0x01, SMBUS_IO_LENGTH, ) 

                                Memory32Fixed (ReadWrite,         // SbIoApic
                                    0xFEC00000,
                                    0x00040000,
                                    )
                                Memory32Fixed (ReadWrite,         // Bios FW MMIO
                                    0x00000000,
                                    0x00000000,
                                    _Y10)
                            })
                            
                            Method (_CRS, 0, NotSerialized)  
                            {
                                CreateWordField (VSBR, \_SB.PCI0.VTSB._Y0D._MIN, PBB)  
                                CreateWordField (VSBR, \_SB.PCI0.VTSB._Y0D._MAX, PBH)  
                                CreateByteField (VSBR, \_SB.PCI0.VTSB._Y0D._LEN, PML)  
                                Store (PMBS, PBB)
                                Store (PMBS, PBH)
                                Store (PMLN, PML)

                                CreateDWordField (VSBR, \_SB.PCI0.VTSB._Y10._BAS, FWB)  
                                CreateDWordField (VSBR, \_SB.PCI0.VTSB._Y10._LEN, FWL)  
                                Subtract (0xFFFFFFFF, FLSZ, FWB)
                                Increment (FWB)
                                Store (FLSZ, FWL)

                                Return (VSBR)
                            }
                        }

                        Device (SMBC)
                        {
                            Name (_HID, EisaId ("SMB3324"))  
                            
                            OperationRegion(SMRE, SystemIO, SMBUS_BASE_ADDRESS, 0x08)
                            Field(SMRE, ByteAcc, NoLock, Preserve)
                            {
                              HSTS, 8,
                              , 8,
                              HCTL, 8,
                              HCMD, 8,
                              SADD, 8,
                              HDA0, 8,
                              HDA1, 8,
                              BDAT, 8,
                            }
                            
                            Method (_STA, 0, NotSerialized)  
                            {
                                If (^^SBRG.SCIS)
                                {
                                    Return (0x0F)
                                }
                                Else
                                {
                                    Return (Zero)
                                }
                            }

                            Method (_CRS, 0, Serialized)  
                            {
                                Name (SMBC, ResourceTemplate ()
                                {
                                    IO (Decode16,
                                        SMBUS_BASE_ADDRESS,             
                                        SMBUS_BASE_ADDRESS,             
                                        0x01,               
                                        0x10,               
                                        )
                                    IRQ (Level, ActiveLow, Shared, )
                                        {9}
                                })
                                Return (SMBC)
                            }
                            
                        }
                    }
                }

#include "Gpe.asl"

                Device (WDRT)
                {
                    Name (_HID, EisaId ("PNP0C02"))  
                    Name (_UID, 0x87)
                    Method (_CRS, 0, Serialized)
                    {
                        Name (BUF0, ResourceTemplate ()
                        {
                            Memory32Fixed (ReadWrite,
                                0xFED01000,         // Address Base
                                0x00000100,         // Address Length
                                )
                        })
                        Return (BUF0)
                    }
                }

                Device (PIC)
                {
                    Name (_HID, EisaId ("PNP0000"))  
                    Name (_CRS, ResourceTemplate ()  
                    {
                        IO (Decode16,
                            0x0020,             
                            0x0020,             
                            0x00,               
                            0x02,               
                            )
                        IO (Decode16,
                            0x00A0,             
                            0x00A0,             
                            0x00,               
                            0x02,               
                            )
                        IRQNoFlags ()
                            {2}
                    })
                }

                Device (DMAD)
                {
                    Name (_HID, EisaId ("PNP0200"))  
                    Name (_CRS, ResourceTemplate ()  
                    {
                        DMA (Compatibility, BusMaster, Transfer8, )
                            {4}
                        IO (Decode16,
                            0x0000,             
                            0x0000,             
                            0x00,               
                            0x10,               
                            )
                        IO (Decode16,
                            0x0081,             
                            0x0081,             
                            0x00,               
                            0x03,               
                            )
                        IO (Decode16,
                            0x0087,             
                            0x0087,             
                            0x00,               
                            0x01,               
                            )
                        IO (Decode16,
                            0x0089,             
                            0x0089,             
                            0x00,               
                            0x03,               
                            )
                        IO (Decode16,
                            0x008F,             
                            0x008F,             
                            0x00,               
                            0x01,               
                            )
                        IO (Decode16,
                            0x00C0,             
                            0x00C0,             
                            0x00,               
                            0x20,               
                            )
                    })
                }

                Scope (\_SB)
                {
                    OperationRegion (PCI0.SBRG.KEN1, PCI_Config, 0x68, One)
                    Field (PCI0.SBRG.KEN1, ByteAcc, NoLock, Preserve)
                    {
                            ,   7,
                        HPTF,   1
                    }
                }

                Device (TMR)
                {
                    Name (_HID, EisaId ("PNP0100"))  
                    Name (TMR0, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x0040,             
                            0x0040,             
                            0x00,               
                            0x04,               
                            )
                    })
                    Name (TMR1, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x0040,             
                            0x0040,             
                            0x00,               
                            0x04,               
                            )
                        IRQNoFlags ()
                            {0}
                    })
                    Method (_CRS, 0, NotSerialized)  
                    {
                        If (HPTF)
                        {
                            Return (TMR0)
                        }
                        Else
                        {
                            Return (TMR1)
                        }
                    }
                }

                Scope (\_SB)
                {
                    OperationRegion (PCI0.SBRG.KEN0, PCI_Config, 0x68, One)
                    Field (PCI0.SBRG.KEN0, ByteAcc, NoLock, Preserve)
                    {
                            ,   7,
                        HPTE,   1
                    }
                }

                Device (RTC0)
                {
                    Name (_HID, EisaId ("PNP0B00"))  
                    Name (RTR0, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x0070,             
                            0x0070,             
                            0x00,               
                            0x02,               
                            )
                    })
                    Name (RTR1, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x0070,             
                            0x0070,             
                            0x00,               
                            0x02,               
                            )
                        IRQNoFlags ()
                            {8}
                    })
                    Method (_CRS, 0, NotSerialized)  
                    {
                        If (HPTE)
                        {
                            Return (RTR0)
                        }
                        Else
                        {
                            Return (RTR1)
                        }
                    }
                }

                Device (SPKR)
                {
                    Name (_HID, EisaId ("PNP0800"))  
                    Name (_CRS, ResourceTemplate ()  
                    {
                        IO (Decode16,
                            0x0061,             
                            0x0061,             
                            0x00,               
                            0x01,               
                            )
                    })
                }

                include("Ps2Kb.asl")
                include("Ps2Ms.asl")
                
//HYL-2016101802 #ifdef MDEPKG_NDEBUG                        // release mode.  
/* //HYL-2016101802
                Scope (^^PCI0)
                {
                    OperationRegion (SBRG.UAIR, PCI_Config, 0xB1, 0x02)
                    Field (SBRG.UAIR, ByteAcc, NoLock, Preserve)
                    {
                        UAI3,   4,
                        UAI4,   4,
                        UAI1,   4,      // PCI UART 0 IRQ Routing 
                        UAI2,   4
                    }

                    OperationRegion (SBRG.UA1B, PCI_Config, 0xB3, One)
                    Field (SBRG.UA1B, ByteAcc, NoLock, Preserve)
                    {
                        UAB1,   7,      // PCI UART 0 & USIO I/O Base Address [9:3]
                        UAE1,   1       // PCI UART 0 Legacy Mode Enable 
                    }

                    Device (UAR1)
                    {
                        Name (_HID, EisaId ("PNP0501"))  
                        Name (_UID, One)  
                        Name (_DDN, "COM1")  
                        Method (_STA, 0, NotSerialized)  
                        {
                            If (UAE1){
                                Return (0x0F)
                            } Else {
                                Return (Zero)
                            }
                        }

                        Method (_CRS, 0, Serialized)  
                        {
                            Name (BFU1, ResourceTemplate() {
                                IO (Decode16, 0x03F8, 0x03F8, 0x08, 0x08, _Y12)
                                IRQ (Level, ActiveLow, Shared, _Y13){4}
                            })
                            CreateWordField(BFU1, \_SB.PCI0.UAR1._CRS._Y12._MIN, U1BL)  
                            CreateWordField(BFU1, \_SB.PCI0.UAR1._CRS._Y12._MAX, U1BH)  
                            CreateWordField(BFU1, \_SB.PCI0.UAR1._CRS._Y13._INT, U1IN)  
                            ShiftLeft(UAB1, 3, Local0)
                            Store(Local0, U1BL)
                            Store(Local0, U1BH)
                            ShiftLeft(1, UAI1, U1IN)
                            Return (BFU1)
                        }
                    }
                }
*/ //HYL-2016101802 
		include("Uart.asl")	//HYL-2016101802
//HYL-2016101802 #endif

                Device (HPET)
                {
                    Name (_HID, EisaId ("PNP0103"))  
                    OperationRegion (^GCNT, PCI_Config, 0x68, 0x04)
                    Field (GCNT, ByteAcc, NoLock, Preserve)
                    {
                            ,   7,
                        HPTE,   1,
                            ,   2,
                        HPTA,   22
                    }

                    Method (_STA, 0, NotSerialized)  
                    {
                        If (HPTE)
                        {
                            Return (0x0F)
                        }
                        Else
                        {
                            Return (Zero)
                        }
                    }

                    Name (HCRS, ResourceTemplate ()
                    {
                        IRQNoFlags ()
                            {0}
                        IRQNoFlags ()
                            {8}
                        Memory32Fixed (ReadOnly,
                            0xFED00000,         // Address Base
                            0x00000400,         // Address Length
                            _Y1A)
                    })
                    CreateDWordField (HCRS, \_SB.PCI0.SBRG.HPET._Y1A._BAS, HPT)  
                    Method (_CRS, 0, NotSerialized)  
                    {
                        ShiftLeft (HPTA, 0x0A, HPT)
                        Return (HCRS)
                    }
                }

                Device (RMSC)
                {
                    Name (_HID, EisaId ("PNP0C02"))  
                    Name (_UID, 0x10)
                    Name (CRS1, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x0010,             
                            0x0010,             
                            0x00,               
                            0x10,               
                            )
                        IO (Decode16,
                            0x0022,             
                            0x0022,             
                            0x00,               
                            0x1E,               
                            )
                        IO (Decode16,
                            0x0044,             
                            0x0044,             
                            0x00,               
                            0x1C,               
                            )
                        IO (Decode16,
                            0x0062,             
                            0x0062,             
                            0x00,               
                            0x02,               
                            )
                        IO (Decode16,
                            0x0065,             
                            0x0065,             
                            0x00,               
                            0x0B,               
                            )
                        IO (Decode16,
                            0x0072,             
                            0x0072,             
                            0x00,               
                            0x0E,               
                            )
                        IO (Decode16,
                            0x0080,             
                            0x0080,             
                            0x00,               
                            0x01,               
                            )
                        IO (Decode16,
                            0x0084,             
                            0x0084,             
                            0x00,               
                            0x03,               
                            )
                        IO (Decode16,
                            0x0088,             
                            0x0088,             
                            0x00,               
                            0x01,               
                            )
                        IO (Decode16,
                            0x008C,             
                            0x008C,             
                            0x00,               
                            0x03,               
                            )
                        IO (Decode16,
                            0x0090,             
                            0x0090,             
                            0x00,               
                            0x10,               
                            )
                        IO (Decode16,
                            0x00A2,             
                            0x00A2,             
                            0x00,               
                            0x1E,               
                            )
                        IO (Decode16,
                            0x00E0,             
                            0x00E0,             
                            0x00,               
                            0x10,               
                            )
                        IO (Decode16,
                            0x04D0,             
                            0x04D0,             
                            0x00,               
                            0x02,               
                            )
                    })
                    Method (_CRS, 0, NotSerialized)
                    {
                      Return (CRS1)
                    }
                }

                Device (COPR)
                {
                    Name (_HID, EisaId ("PNP0C04"))  // Math coprocessor
                    Name (_CRS, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x00F0,             
                            0x00F0,             
                            0x00,               
                            0x10,               
                            )
                        IRQNoFlags ()
                            {13}
                    })
                }
            }

            Device (SVLK)
            {
                Name (_ADR, 0x00110007)  
            }

        }

        Device (PWRB)
        {
            Name (_HID, EisaId ("PNP0C0C"))  
            Name (_UID, 0xAA)  
            Name (_STA, 0x0B)  
        }
    }

    OperationRegion (_SB.PCI0.SBRG.PIX0, PCI_Config, 0x55, 0x04)
    OperationRegion (_SB.PCI0.SBRG.PIX1, PCI_Config, 0x44, 0x02)
    Field (\_SB.PCI0.SBRG.PIX0, ByteAcc, NoLock, Preserve)
    {
            ,   4,
        PIRA,   4,
        PIRB,   4,
        PIRC,   4,
            ,   4,
        PIRD,   4,
            ,   4
    }

    Field (\_SB.PCI0.SBRG.PIX1, ByteAcc, NoLock, Preserve)
    {
        PIRE,   4,
        PIRF,   4,
        PIRG,   4,
        PIRH,   4
    }

    OperationRegion (_SB.PCI0.SBRG.PIEF, PCI_Config, 0x46, One)
    Field (\_SB.PCI0.SBRG.PIEF, ByteAcc, NoLock, Preserve)
    {
        POLE,   1,
        POLF,   1,
        POLG,   1,
        POLH,   1,
        ENR8,   1
    }

    Scope (_SB)
    {
        Name (BUFA, ResourceTemplate ()
        {
            IRQ (Level, ActiveLow, Shared, ){15}
        })
        CreateWordField (BUFA, One, IRA0)
        Device (LNKA)
        {
            Name (_HID, EisaId ("PNP0C0F"))  
            Name (_UID, One)  
            Method (_STA, 0, NotSerialized)  
            {
                If (PIRA)
                {
                    Return (0x0B)
                }
                Else
                {
                    Return (0x09)
                }
            }

            Method (_PRS, 0, NotSerialized)  // _PRS: Possible Resource Settings
            {
                Return (PRSA)
            }

            Method (_DIS, 0, NotSerialized)  // _DIS: Disable Device
            {
                Store (Zero, PIRA)
            }

            Method (_CRS, 0, NotSerialized)  
            {
                ShiftLeft (One, PIRA, IRA0)
                Return (BUFA)
            }

            Method (_SRS, 1, NotSerialized)  // _SRS: Set Resource Settings
            {
                CreateWordField (Arg0, One, IRA)
                FindSetRightBit (IRA, Local0)
                Decrement (Local0)
                Store (Local0, PIRA)
            }
        }

        Device (LNKB)
        {
            Name (_HID, EisaId ("PNP0C0F"))  
            Name (_UID, 0x02)  
            Method (_STA, 0, NotSerialized)  
            {
                If (PIRB)
                {
                    Return (0x0B)
                }
                Else
                {
                    Return (0x09)
                }
            }

            Method (_PRS, 0, NotSerialized)  // _PRS: Possible Resource Settings
            {
                Return (PRSB)
            }

            Method (_DIS, 0, NotSerialized)  // _DIS: Disable Device
            {
                Store (Zero, PIRB)
            }

            Method (_CRS, 0, NotSerialized)  
            {
                ShiftLeft (One, PIRB, IRA0)
                Return (BUFA)
            }

            Method (_SRS, 1, NotSerialized)  // _SRS: Set Resource Settings
            {
                CreateWordField (Arg0, One, IRA)
                FindSetRightBit (IRA, Local0)
                Decrement (Local0)
                Store (Local0, PIRB)
            }
        }

        Device (LNKC)
        {
            Name (_HID, EisaId ("PNP0C0F"))  
            Name (_UID, 0x03)  
            Method (_STA, 0, NotSerialized)  
            {
                If (PIRC)
                {
                    Return (0x0B)
                }
                Else
                {
                    Return (0x09)
                }
            }

            Method (_PRS, 0, NotSerialized)  // _PRS: Possible Resource Settings
            {
                Return (PRSC)
            }

            Method (_DIS, 0, NotSerialized)  // _DIS: Disable Device
            {
                Store (Zero, PIRC)
            }

            Method (_CRS, 0, NotSerialized)  
            {
                ShiftLeft (One, PIRC, IRA0)
                Return (BUFA)
            }

            Method (_SRS, 1, NotSerialized)  // _SRS: Set Resource Settings
            {
                CreateWordField (Arg0, One, IRA)
                FindSetRightBit (IRA, Local0)
                Decrement (Local0)
                Store (Local0, PIRC)
            }
        }

        Device (LNKD)
        {
            Name (_HID, EisaId ("PNP0C0F"))  
            Name (_UID, 0x04)  
            Method (_STA, 0, NotSerialized)  
            {
                If (PIRD)
                {
                    Return (0x0B)
                }
                Else
                {
                    Return (0x09)
                }
            }

            Method (_PRS, 0, NotSerialized)  // _PRS: Possible Resource Settings
            {
                Return (PRSD)
            }

            Method (_DIS, 0, NotSerialized)  // _DIS: Disable Device
            {
                Store (Zero, PIRD)
            }

            Method (_CRS, 0, NotSerialized)  
            {
                ShiftLeft (One, PIRD, IRA0)
                Return (BUFA)
            }

            Method (_SRS, 1, NotSerialized)  // _SRS: Set Resource Settings
            {
                CreateWordField (Arg0, One, IRA)
                FindSetRightBit (IRA, Local0)
                Decrement (Local0)
                Store (Local0, PIRD)
            }
        }

        Device (LNKE)
        {
            Name (_HID, EisaId ("PNP0C0F"))  
            Name (_UID, 0x05)  
            Method (_STA, 0, NotSerialized)  
            {
                If (PIRE)
                {
                    Return (0x0B)
                }
                Else
                {
                    Return (0x09)
                }
            }

            Method (_PRS, 0, NotSerialized)  // _PRS: Possible Resource Settings
            {
                Return (PRSE)
            }

            Method (_DIS, 0, NotSerialized)  // _DIS: Disable Device
            {
                Store (Zero, PIRE)
                Store (Zero, POLE)
            }

            Method (_CRS, 0, NotSerialized)  
            {
                ShiftLeft (One, PIRE, IRA0)
                Return (BUFA)
            }

            Method (_SRS, 1, NotSerialized)  // _SRS: Set Resource Settings
            {
                CreateWordField (Arg0, One, IRA)
                FindSetRightBit (IRA, Local0)
                Decrement (Local0)
                Store (Local0, PIRE)
                Store (One, ENR8)
                Store (INTE, POLE)
            }
        }

        Device (LNKF)
        {
            Name (_HID, EisaId ("PNP0C0F"))  
            Name (_UID, 0x06)  
            Method (_STA, 0, NotSerialized)  
            {
                If (PIRF)
                {
                    Return (0x0B)
                }
                Else
                {
                    Return (0x09)
                }
            }

            Method (_PRS, 0, NotSerialized)  // _PRS: Possible Resource Settings
            {
                Return (PRSF)
            }

            Method (_DIS, 0, NotSerialized)  // _DIS: Disable Device
            {
                Store (Zero, PIRF)
                Store (Zero, POLF)
            }

            Method (_CRS, 0, NotSerialized)  
            {
                ShiftLeft (One, PIRF, IRA0)
                Return (BUFA)
            }

            Method (_SRS, 1, NotSerialized)  // _SRS: Set Resource Settings
            {
                CreateWordField (Arg0, One, IRA)
                FindSetRightBit (IRA, Local0)
                Decrement (Local0)
                Store (Local0, PIRF)
                Store (One, ENR8)
                Store (INTF, POLF)
            }
        }

        Device (LNKG)
        {
            Name (_HID, EisaId ("PNP0C0F"))  
            Name (_UID, 0x07)  
            Method (_STA, 0, NotSerialized)  
            {
                If (PIRG)
                {
                    Return (0x0B)
                }
                Else
                {
                    Return (0x09)
                }
            }

            Method (_PRS, 0, NotSerialized)  // _PRS: Possible Resource Settings
            {
                Return (PRSG)
            }

            Method (_DIS, 0, NotSerialized)  // _DIS: Disable Device
            {
                Store (Zero, PIRG)
                Store (Zero, POLG)
            }

            Method (_CRS, 0, NotSerialized)  
            {
                ShiftLeft (One, PIRG, IRA0)
                Return (BUFA)
            }

            Method (_SRS, 1, NotSerialized)  // _SRS: Set Resource Settings
            {
                CreateWordField (Arg0, One, IRA)
                FindSetRightBit (IRA, Local0)
                Decrement (Local0)
                Store (Local0, PIRG)
                Store (One, ENR8)
                Store (INTG, POLG)
            }
        }

        Device (LNKH)
        {
            Name (_HID, EisaId ("PNP0C0F"))  
            Name (_UID, 0x08)  
            Method (_STA, 0, NotSerialized)  
            {
                If (PIRH)
                {
                    Return (0x0B)
                }
                Else
                {
                    Return (0x09)
                }
            }

            Method (_PRS, 0, NotSerialized)  // _PRS: Possible Resource Settings
            {
                Return (PRSH)
            }

            Method (_DIS, 0, NotSerialized)  // _DIS: Disable Device
            {
                Store (Zero, PIRH)
                Store (Zero, POLH)
            }

            Method (_CRS, 0, NotSerialized)  
            {
                ShiftLeft (One, PIRH, IRA0)
                Return (BUFA)
            }

            Method (_SRS, 1, NotSerialized)  // _SRS: Set Resource Settings
            {
                CreateWordField (Arg0, One, IRA)
                FindSetRightBit (IRA, Local0)
                Decrement (Local0)
                Store (Local0, PIRH)
                Store (One, ENR8)
                Store (INTH, POLH)
            }
        }
    }

#include "Cpu.asl"

    Name (WAKP, Package(0x02){Zero, Zero}) 
    
    Method (_PTS, 1, NotSerialized)
    {
      ASL_COM((1, " ", 0))  
      ASL_COM((1, "_PTS", 0))  
      Store (Arg0, DBG8)
      If(Arg0) {
        \_SB.PCI0.SBRG.SPTS(Arg0)
      }
      Store (Zero, Index (WAKP, Zero))
      Store (Zero, Index (WAKP, One))
      ASL_COM((1, "PTSE", 0))    
    }

    Method (_WAK, 1, NotSerialized)
    {
      ASL_COM((1, "_WAK", 0))
      ShiftLeft (Arg0, 0x04, DBG8)
      \_SB.PCI0.SBRG.SWAK(Arg0)

      If (DerefOf(Index (WAKP, Zero))) {
        Store(Zero, Index (WAKP, One))
      } Else {
        Store(Arg0, Index (WAKP, One))
      }
      ASL_COM((1, "WAKE", 0))
      Return (WAKP)
    }

 
    Method (_SB._OSC, 4, NotSerialized){
      Return (Arg3)
    }
    
#if defined TCM_ENABLE && TCM_ENABLE == 1
#include "Tcm.asl"
#endif

}