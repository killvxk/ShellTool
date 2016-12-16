

Device (PS2M)
{
    Name (_HID, EisaId ("PNP0F03"))
    Name (_CID, EisaId ("PNP0F13"))
    
    Method (_STA, 0, NotSerialized)
    {
      If (And (IOST, 0x4000)) {         // BIT14
        Return (0x0F)
      } Else {
        Return (Zero)
      }
    }

    Name (CRS1, ResourceTemplate() { IRQNoFlags(){12} })
    Name (CRS2, ResourceTemplate()
    {
        IO (Decode16,
            0x0060,             
            0x0060,             
            0x00,               
            0x01,               
            )
        IO (Decode16,
            0x0064,             
            0x0064,             
            0x00,               
            0x01,               
            )
        IRQNoFlags(){12}
    })
    
    Method (_CRS, 0, NotSerialized)
    {
      If (And (IOST, 0x0400)) {
        Return (CRS1)
      } Else {
        Return (CRS2)
      }
    }

    Method (_PSW, 1, NotSerialized)  // _PSW: Power State Wake
    {
        Store (Arg0, MSFG)
    }

    Method (_PRW, 0, NotSerialized)  
    {
        Return (GPRW(0x09, \MXDW))
    }
}

Scope (\)
{
    Name (MSFG, One)
}

