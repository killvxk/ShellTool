

Device (PS2K)
{
  Name (_HID, EisaId ("PNP0303"))
  Name (_CID, EisaId ("PNP030B"))
  
  Method (_STA, 0, NotSerialized)
  {
    If (And (IOST, 0x0400)) {      // BIT10
      Return (0x0F)
    } Else {
      Return (Zero)
    }
  }

  Name (_CRS, ResourceTemplate ()  
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
      IRQNoFlags(){1}
  })

  Method (_PSW, 1, NotSerialized)
  {
    Store (Arg0, KBFG)
  }

  Method (_PRW, 0, NotSerialized)  
  {
    Return (GPRW (0x02, \MXDW))
  }
}

Scope (\)
{
  Name (KBFG, One)
}

