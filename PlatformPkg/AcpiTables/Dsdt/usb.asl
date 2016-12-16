            Device (USB1)
            {
                Name (_ADR, 0x00100000)  
                OperationRegion (WAU1, PCI_Config, 0x84, One)
                Field (WAU1, ByteAcc, NoLock, Preserve)
                {
                    U184,   8
                }

                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
                {
                  Return (0x03)
                }

                Method (_PRW, 0, NotSerialized)  
                {
                  Return (GPRW (0x0E, \MXDW))
                }
            }

            Device (USB2)
            {
                Name (_ADR, 0x00100001)  
                OperationRegion (WAU2, PCI_Config, 0x84, One)
                Field (WAU2, ByteAcc, NoLock, Preserve)
                {
                    U284,   8
                }

                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
                {
                  Return (0x03)
                }

                Method (_PRW, 0, NotSerialized)  
                {
                  Return (GPRW (0x0E, \MXDW))
                }
            }

            Device (USB3)
            {
                Name (_ADR, 0x00100002)  
                OperationRegion (WAU3, PCI_Config, 0x84, One)
                Field (WAU3, ByteAcc, NoLock, Preserve)
                {
                    U384,   8
                }

                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
                {
                  Return (0x03)
                }

                Method (_PRW, 0, NotSerialized)  
                {
                  Return (GPRW (0x0E, \MXDW))
                }
            }


            Device (EHCI)
            {
                Name (_ADR, 0x00100007)  
                OperationRegion (WAEH, PCI_Config, 0x84, One)
                Field (WAEH, ByteAcc, NoLock, Preserve)
                {
                    EH84,   8
                }

                Method (_PRW, 0, NotSerialized)  
                {
                  Return (GPRW (0x0E, \MXDW))
                }
            }
            
            Device (XHCI)
            {
                Name (_ADR, 0x00120000)  
                OperationRegion (WAXH, PCI_Config, 0x84, One)
                Field (WAXH, ByteAcc, NoLock, Preserve)
                {
                    XH84,   8
                }

                Method (_S3D, 0, NotSerialized)  // _S3D: S3 Device State
                {
                    Return (0x02)
                }

                Method (_S4D, 0, NotSerialized)  // _S4D: S4 Device State
                {
                    Return (0x02)
                }

                Method(_S0W, 0, NotSerialized)
                {
                  Return(0x03)
                }
                
                OperationRegion (XIPC, PCI_Config, Zero, 0xC2)
                Field (XIPC, ByteAcc, NoLock, Preserve)
                {
                    Offset (0x43), 
                    XI43,   1, 
                    Offset (0x78), 
                    XI78,   32, 
                    XI7C,   16
                }

                Method (_INI, 0, NotSerialized)  
                {
                    Store (One, XI43)
                    Store (0xE0, XI78)
                    Store (XI7C, Local0)
                    And (Local0, 0xFF, Local0)
                    Or (Local0, 0x7700, Local0)
                    Store (Local0, XI7C)
                    Store (Zero, ^^NBF6.W8FL)
                    If (_OSI ("Windows 2012"))
                    {
                        Store (One, ^^NBF6.W8FL)
                        Store (0xE0, XI78)
                        Store (XI7C, Local0)
                        And (Local0, 0xFF, Local0)
                        Or (Local0, 0x8800, Local0)
                        Store (Local0, XI7C)
                    }

                    Store (Zero, XI43)
                }

                Method (_PRW, 0, NotSerialized)  
                {
                  Return (GPRW (0x0E, \MXDW))
                }
            }

            