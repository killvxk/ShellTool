
//for laptop application and only will be implemented when engaging with customers
            Device (EUMA)
            {
                Name (_ADR, 0x00010000)  
                Method (_STA, 0, NotSerialized)  
                {
                    Return (0x0F)
                }

                Name (DEVA, 0x03)
                Name (SWIT, One)
                Method (_DOS, 1, NotSerialized)  // _DOS: Disable Output Switching
                {
                    Store (Arg0, SWIT)
                }

                Method (_INI, 0, NotSerialized)  
                {
                    Store (DEVA, Local1)
                    If (LNotEqual (And (Local1, One), Zero))
                    {
                        Store (One, ^CRT._DGS)
                    }
                    Else
                    {
                        Store (Zero, ^CRT._DGS)
                    }

                    If (LNotEqual (And (Local1, 0x02), Zero))
                    {
                        Store (One, ^LCD._DGS)
                    }
                    Else
                    {
                        Store (Zero, ^LCD._DGS)
                    }

                    If (LNotEqual (And (Local1, 0x04), Zero))
                    {
                        Store (One, ^TV._DGS)
                    }
                    Else
                    {
                        Store (Zero, ^TV._DGS)
                    }

                    If (LNotEqual (And (Local1, 0x20), Zero))
                    {
                        Store (One, ^DVI._DGS)
                    }
                    Else
                    {
                        Store (Zero, ^DVI._DGS)
                    }
                }

                Name (_DOD, Package (0x08)  // _DOD: Display Output Devices
                {
                    0x00010100, 
                    0x00010110, 
                    0x00010120, 
                    0x00010240, 
                    0x00010200, 
                    0x00010210, 
                    0x00010280, 
                    0x000102A0
                })
                Device (CRT)
                {
                    Name (_ADR, 0x0100)  
                    Name (_DCS, 0x1F)  // _DCS: Display Current Status
                    Name (_DGS, One)  // _DGS: Display Graphics State
                    Method (_DSS, 1, NotSerialized)  // _DSS: Device Set State
                    {
                        If (And (Arg0, 0xC0000000))
                        {
                            If (LEqual (And (Arg0, 0x40000000), Zero))
                            {
                                Store (Arg0, Local0)
                                Store (And (Local0, One), Local0)
                                Store (0xFFFFFFFFFFFFFFFE, Local1)
                                Store (And (_DGS, Local1), _DGS)
                                Store (Or (_DGS, Local0), _DGS)
                            }
                        }
                    }
                }

                Device (LCD)
                {
                    Name (_ADR, 0x0110)  
                    Name (_DCS, 0x1F)  // _DCS: Display Current Status
                    Name (_DGS, One)  // _DGS: Display Graphics State
                    Method (_DSS, 1, NotSerialized)  // _DSS: Device Set State
                    {
                        If (And (Arg0, 0xC0000000))
                        {
                            If (LEqual (And (Arg0, 0x40000000), Zero))
                            {
                                Store (Arg0, Local0)
                                Store (And (Local0, One), Local0)
                                Store (0xFFFFFFFFFFFFFFFE, Local1)
                                Store (And (_DGS, Local1), _DGS)
                                Store (Or (_DGS, Local0), _DGS)
                            }
                        }
                    }

                    Method (_BCL, 0, NotSerialized)  // _BCL: Brightness Control Levels
                    {
                        Return (Package (0x08)
                        {
                            0x64, 
                            0x32, 
                            0x14, 
                            0x1E, 
                            0x28, 
                            0x3C, 
                            0x50, 
                            0x5A
                        })
                    }

                    Method (_BCM, 1, NotSerialized)  // _BCM: Brightness Control Method
                    {
                    }
                }

                Device (TV)
                {
                    Name (_ADR, 0x0200)  
                    Name (_DCS, 0x1F)  // _DCS: Display Current Status
                    Name (_DGS, One)  // _DGS: Display Graphics State
                    Method (_DSS, 1, NotSerialized)  // _DSS: Device Set State
                    {
                        If (And (Arg0, 0xC0000000))
                        {
                            If (LEqual (And (Arg0, 0x40000000), Zero))
                            {
                                Store (Arg0, Local0)
                                Store (And (Local0, One), Local0)
                                Store (0xFFFFFFFFFFFFFFFE, Local1)
                                Store (And (_DGS, Local1), _DGS)
                                Store (Or (_DGS, Local0), _DGS)
                            }
                        }
                    }
                }

                Device (DVI)
                {
                    Name (_ADR, 0x0120)  
                    Name (_DCS, 0x1F)  // _DCS: Display Current Status
                    Name (_DGS, One)  // _DGS: Display Graphics State
                    Method (_DSS, 1, NotSerialized)  // _DSS: Device Set State
                    {
                        If (And (Arg0, 0xC0000000))
                        {
                            If (LEqual (And (Arg0, 0x40000000), Zero))
                            {
                                Store (Arg0, Local0)
                                Store (And (Local0, One), Local0)
                                Store (0xFFFFFFFFFFFFFFFE, Local1)
                                Store (And (_DGS, Local1), _DGS)
                                Store (Or (_DGS, Local0), _DGS)
                            }
                        }
                    }
                }

                Device (HDMI)
                {
                    Name (_ADR, 0x0240)  
                    Name (_DCS, 0x1F)  // _DCS: Display Current Status
                    Name (_DGS, One)  // _DGS: Display Graphics State
                    Method (_DSS, 1, NotSerialized)  // _DSS: Device Set State
                    {
                        If (And (Arg0, 0xC0000000))
                        {
                            If (LEqual (And (Arg0, 0x40000000), Zero))
                            {
                                Store (Arg0, Local0)
                                Store (And (Local0, One), Local0)
                                Store (0xFFFFFFFFFFFFFFFE, Local1)
                                Store (And (_DGS, Local1), _DGS)
                                Store (Or (_DGS, Local0), _DGS)
                            }
                        }
                    }
                }

                Device (HDTV)
                {
                    Name (_ADR, 0x0210)  
                    Name (_DCS, 0x1F)  // _DCS: Display Current Status
                    Name (_DGS, One)  // _DGS: Display Graphics State
                    Method (_DSS, 1, NotSerialized)  // _DSS: Device Set State
                    {
                        If (And (Arg0, 0xC0000000))
                        {
                            If (LEqual (And (Arg0, 0x40000000), Zero))
                            {
                                Store (Arg0, Local0)
                                Store (And (Local0, One), Local0)
                                Store (0xFFFFFFFFFFFFFFFE, Local1)
                                Store (And (_DGS, Local1), _DGS)
                                Store (Or (_DGS, Local0), _DGS)
                            }
                        }
                    }
                }

                Device (DP)
                {
                    Name (_ADR, 0x0280)  
                    Name (_DCS, 0x1F)  // _DCS: Display Current Status
                    Name (_DGS, One)  // _DGS: Display Graphics State
                    Method (_DSS, 1, NotSerialized)  // _DSS: Device Set State
                    {
                        If (And (Arg0, 0xC0000000))
                        {
                            If (LEqual (And (Arg0, 0x40000000), Zero))
                            {
                                Store (Arg0, Local0)
                                Store (And (Local0, One), Local0)
                                Store (0xFFFFFFFFFFFFFFFE, Local1)
                                Store (And (_DGS, Local1), _DGS)
                                Store (Or (_DGS, Local0), _DGS)
                            }
                        }
                    }
                }

                Device (DP2)
                {
                    Name (_ADR, 0x02A0)  
                    Name (_DCS, 0x1F)  // _DCS: Display Current Status
                    Name (_DGS, One)  // _DGS: Display Graphics State
                    Method (_DSS, 1, NotSerialized)  // _DSS: Device Set State
                    {
                        If (And (Arg0, 0xC0000000))
                        {
                            If (LEqual (And (Arg0, 0x40000000), Zero))
                            {
                                Store (Arg0, Local0)
                                Store (And (Local0, One), Local0)
                                Store (0xFFFFFFFFFFFFFFFE, Local1)
                                Store (And (_DGS, Local1), _DGS)
                                Store (Or (_DGS, Local0), _DGS)
                            }
                        }
                    }
                }
            }
       
            
            Device(S3AC)
            {
              Name(_ADR, 0x00010001)
            }
            
            
            