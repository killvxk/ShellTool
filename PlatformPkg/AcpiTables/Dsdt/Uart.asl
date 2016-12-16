
Scope (^^PCI0)
{
    OperationRegion (SBRG.UAIR, PCI_Config, 0xB1, 0x02)
    Field (SBRG.UAIR, ByteAcc, NoLock, Preserve)
    {
        UAI3,   4, 
        UAI4,   4, 
        UAI1,   4, 
        UAI2,   4
    }

    OperationRegion (SBRG.UA1B, PCI_Config, 0xB3, One)
    Field (SBRG.UA1B, ByteAcc, NoLock, Preserve)
    {
        UAB1,   7, 
        UAE1,   1
    }

    OperationRegion (SBRG.UA2B, PCI_Config, 0xB4, One)
    Field (SBRG.UA2B, ByteAcc, NoLock, Preserve)
    {
        UAB2,   7, 
        UAE2,   1
    }

    OperationRegion (SBRG.UA3B, PCI_Config, 0xB5, One)
    Field (SBRG.UA3B, ByteAcc, NoLock, Preserve)
    {
        UAB3,   7, 
        UAE3,   1
    }

    OperationRegion (SBRG.UA4B, PCI_Config, 0xB6, One)
    Field (SBRG.UA4B, ByteAcc, NoLock, Preserve)
    {
        UAB4,   7, 
        UAE4,   1
    }

    Device (UAR1)
    {
        Name (_HID, EisaId ("PNP0501"))  // _HID: Hardware ID
        Name (_UID, One)  // _UID: Unique ID
        Name (_DDN, "COM1")  // _DDN: DOS Device Name
        Method (_STA, 0, NotSerialized)  // _STA: Status
        {
            If (UAE1)
            {
                Return (0x0F)
            }
            Else
            {
                Return (Zero)
            }
        }

        Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
        {
            Name (BFU1, ResourceTemplate ()
            {
                IO (Decode16,
                    0x03F8,             // Range Minimum
                    0x03F8,             // Range Maximum
                    0x08,               // Alignment
                    0x08,               // Length
                    CB1)
                IRQ (Level, ActiveLow, Shared, CI1)
                    {4}
            })
            CreateWordField (BFU1, \_SB.PCI0.UAR1._CRS.CB1._MIN, U1BL)  // _MIN: Minimum Base Address
            CreateWordField (BFU1, \_SB.PCI0.UAR1._CRS.CB1._MAX, U1BH)  // _MAX: Maximum Base Address
            CreateWordField (BFU1, \_SB.PCI0.UAR1._CRS.CI1._INT, U1IN)  // _INT: Interrupts
            Store (0x03, Local1)
            ShiftLeft (UAB1, Local1, Local0)
            Store (Local0, U1BL)
            Store (Local0, U1BH)
            Store (One, Local1)
            ShiftLeft (Local1, UAI1, U1IN)
            Return (BFU1)
        }
    }

    Device (UAR2)
    {
        Name (_HID, EisaId ("PNP0501"))  // _HID: Hardware ID
        Name (_UID, 0x02)  // _UID: Unique ID
        Name (_DDN, "COM2")  // _DDN: DOS Device Name
        Method (_STA, 0, NotSerialized)  // _STA: Status
        {
            If (UAE2)
            {
                Return (0x0F)
            }
            Else
            {
                Return (Zero)
            }
        }

        Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
        {
            Name (BFU3, ResourceTemplate ()
            {
                IO (Decode16,
                    0x02F8,             // Range Minimum
                    0x02F8,             // Range Maximum
                    0x08,               // Alignment
                    0x08,               // Length
                    CB2)
                IRQ (Level, ActiveLow, Shared, CI2)
                    {3}
            })
            CreateWordField (BFU3, \_SB.PCI0.UAR2._CRS.CB2._MIN, U2BL)  // _MIN: Minimum Base Address
            CreateWordField (BFU3, \_SB.PCI0.UAR2._CRS.CB2._MAX, U2BH)  // _MAX: Maximum Base Address
            CreateWordField (BFU3, \_SB.PCI0.UAR2._CRS.CI2._INT, U2IN)  // _INT: Interrupts
            Store (0x03, Local1)
            ShiftLeft (UAB2, Local1, Local0)
            Store (Local0, U2BL)
            Store (Local0, U2BH)
            Store (One, Local1)
            ShiftLeft (Local1, UAI2, U2IN)
            Return (BFU3)
        }
    }

    Device (UAR3)
    {
        Name (_HID, EisaId ("PNP0501"))  // _HID: Hardware ID
        Name (_UID, 0x03)  // _UID: Unique ID
        Name (_DDN, "COM3")  // _DDN: DOS Device Name
        Method (_STA, 0, NotSerialized)  // _STA: Status
        {
            If (UAE3)
            {
                Return (0x0F)
            }
            Else
            {
                Return (Zero)
            }
        }

        Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
        {
            Name (BFU5, ResourceTemplate ()
            {
                IO (Decode16,
                    0x03E8,             // Range Minimum
                    0x03E8,             // Range Maximum
                    0x08,               // Alignment
                    0x08,               // Length
                    CB3)
                IRQ (Level, ActiveLow, Shared, CI3)
                    {4}
            })
            CreateWordField (BFU5, \_SB.PCI0.UAR3._CRS.CB3._MIN, U3BL)  // _MIN: Minimum Base Address
            CreateWordField (BFU5, \_SB.PCI0.UAR3._CRS.CB3._MAX, U3BH)  // _MAX: Maximum Base Address
            CreateWordField (BFU5, \_SB.PCI0.UAR3._CRS.CI3._INT, U3IN)  // _INT: Interrupts
            Store (0x03, Local1)
            ShiftLeft (UAB3, Local1, Local0)
            Store (Local0, U3BL)
            Store (Local0, U3BH)
            Store (One, Local1)
            ShiftLeft (Local1, UAI3, U3IN)
            Return (BFU5)
        }
    }

    Device (UAR4)
    {
        Name (_HID, EisaId ("PNP0501"))  // _HID: Hardware ID
        Name (_UID, 0x04)  // _UID: Unique ID
        Name (_DDN, "COM4")  // _DDN: DOS Device Name
        Method (_STA, 0, NotSerialized)  // _STA: Status
        {
            If (UAE4)
            {
                Return (0x0F)
            }
            Else
            {
                Return (Zero)
            }
        }

        Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
        {
            Name (BFU7, ResourceTemplate ()
            {
                IO (Decode16,
                    0x02E8,             // Range Minimum
                    0x02E8,             // Range Maximum
                    0x08,               // Alignment
                    0x08,               // Length
                    CB4)
                IRQ (Level, ActiveLow, Shared, CI4)
                    {3}
            })
            CreateWordField (BFU7, \_SB.PCI0.UAR4._CRS.CB4._MIN, U4BL)  // _MIN: Minimum Base Address
            CreateWordField (BFU7, \_SB.PCI0.UAR4._CRS.CB4._MAX, U4BH)  // _MAX: Maximum Base Address
            CreateWordField (BFU7, \_SB.PCI0.UAR4._CRS.CI4._INT, U4IN)  // _INT: Interrupts
            Store (0x03, Local1)
            ShiftLeft (UAB4, Local1, Local0)
            Store (Local0, U4BL)
            Store (Local0, U4BH)
            Store (One, Local1)
            ShiftLeft (Local1, UAI4, U4IN)
            Return (BFU7)
        }
    }
}


