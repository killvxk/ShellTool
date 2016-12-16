
            Device (ATA0)
            {
                Name (_ADR, 0x000f0000)  
                Name (REGF, One)
                Name (TIM0, Package (0x04)
                {
                    Package (0x05)
                    {
                        0x78, 
                        0xB4, 
                        0xF0, 
                        0x017F, 
                        0x0258
                    }, 

                    Package (0x05)
                    {
                        0x20, 
                        0x22, 
                        0x33, 
                        0x47, 
                        0x5D
                    }, 

                    Package (0x07)
                    {
                        0x78, 
                        0x50, 
                        0x3C, 
                        0x2D, 
                        0x1E, 
                        0x14, 
                        0x0F
                    }, 

                    Package (0x0F)
                    {
                        0x06, 
                        0x05, 
                        0x04, 
                        0x04, 
                        0x03, 
                        0x03, 
                        0x02, 
                        0x02, 
                        One, 
                        One, 
                        One, 
                        One, 
                        One, 
                        One, 
                        Zero
                    }
                })
                Name (TMD0, Buffer (0x14) {})
                CreateDWordField (TMD0, Zero, PIO0)
                CreateDWordField (TMD0, 0x04, DMA0)
                CreateDWordField (TMD0, 0x08, PIO1)
                CreateDWordField (TMD0, 0x0C, DMA1)
                CreateDWordField (TMD0, 0x10, CHNF)
                Name (PMPT, 0x20)
                Name (PMUE, 0x07)
                Name (PMUT, Zero)
                Name (PSPT, 0x20)
                Name (PSUE, 0x07)
                Name (PSUT, Zero)
                Name (SMPT, 0x20)
                Name (SMUE, 0x07)
                Name (SMUT, Zero)
                Name (SSPT, 0x20)
                Name (SSUE, 0x07)
                Name (SSUT, Zero)
                Name (FZTF, Buffer (0x07)
                {
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF5
                })
                OperationRegion (SAPR, PCI_Config, Zero, 0xFF)
                Field (SAPR, ByteAcc, NoLock, Preserve)
                {
                    SVID,   16, 
                    Offset (0x04), 
                    RX04,   3, 
                    Offset (0x0A), 
                    RX0A,   8,
                   // Offset (0xA0), 
                   // DET0,   2, 
                   // Offset (0xA1), 
                   // DET1,   2, 
                   // Offset (0xA2), 
                   // Offset (0xF8), 
                   // P0HP,   1, 
                   // Offset (0xF9), 
                   // P1HP,   1, 
                   // Offset (0xFA)
                }

                
                Method (_REG, 2, NotSerialized)  // _REG: Region Availability
                {
                    If (LEqual (Arg0, 0x02))
                    {
                        Store (Arg1, REGF)
                    }
                }

                
                Device (CHN0)
                {
                    Name (_ADR, Zero)  
                    Method (_STA, 0, NotSerialized)  
                    {
                        Return (0x0F)
                    }

                    Method (_GTM, 0, NotSerialized)  // _GTM: Get Timing Mode
                    {
                        Return (GTM (PMPT, PMUE, PMUT, PSPT, PSUE, PSUT))
                    }

                    Method (_STM, 3, NotSerialized)  // _STM: Set Timing Mode
                    {
                    }

                    Device (DRV0)
                    {
                        Name (_ADR, Zero)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }

                    Device (DRV1)
                    {
                        Name (_ADR, One)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }
                }

                Device (CHN1)
                {
                    Name (_ADR, One)  
                    Method (_STA, 0, NotSerialized)  
                    {
                        Return (0x0F)
                    }

                    Method (_GTM, 0, NotSerialized)  // _GTM: Get Timing Mode
                    {
                        Return (GTM (SMPT, SMUE, SMUT, SSPT, SSUE, SSUT))
                    }

                    Method (_STM, 3, NotSerialized)  // _STM: Set Timing Mode
                    {
                    }

                    Device (DRV0)
                    {
                        Name (_ADR, Zero)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }

                    Device (DRV1)
                    {
                        Name (_ADR, One)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }
                }

                Method (GTM, 6, Serialized)
                {
                    Store (Ones, PIO0)
                    Store (Ones, PIO1)
                    Store (Ones, DMA0)
                    Store (Ones, DMA1)
                    Store (0x10, CHNF)
                    If (REGF) {}
                    Else
                    {
                        Return (TMD0)
                    }

                    Store (Match (DerefOf (Index (TIM0, One)), MEQ, Arg0, MTR, 
                        Zero, Zero), Local6)
                    Store (DerefOf (Index (DerefOf (Index (TIM0, Zero)), Local6)), 
                        Local7)
                    Store (Local7, DMA0)
                    Store (Local7, PIO0)
                    Store (Match (DerefOf (Index (TIM0, One)), MEQ, Arg3, MTR, 
                        Zero, Zero), Local6)
                    Store (DerefOf (Index (DerefOf (Index (TIM0, Zero)), Local6)), 
                        Local7)
                    Store (Local7, DMA1)
                    Store (Local7, PIO1)
                    If (Arg1)
                    {
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x03)), Arg2)), 
                            Local5)
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x02)), Local5)), 
                            DMA0)
                        Or (CHNF, One, CHNF)
                    }

                    If (Arg4)
                    {
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x03)), Arg5)), 
                            Local5)
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x02)), Local5)), 
                            DMA1)
                        Or (CHNF, 0x04, CHNF)
                    }

                    Return (TMD0)
                }
            }

            Device (ATA1)
            {
                Name (_ADR, 0x000f0001)  
                Name (REGF, One)
                Name (TIM0, Package (0x04)
                {
                    Package (0x05)
                    {
                        0x78, 
                        0xB4, 
                        0xF0, 
                        0x017F, 
                        0x0258
                    }, 

                    Package (0x05)
                    {
                        0x20, 
                        0x22, 
                        0x33, 
                        0x47, 
                        0x5D
                    }, 

                    Package (0x07)
                    {
                        0x78, 
                        0x50, 
                        0x3C, 
                        0x2D, 
                        0x1E, 
                        0x14, 
                        0x0F
                    }, 

                    Package (0x0F)
                    {
                        0x06, 
                        0x05, 
                        0x04, 
                        0x04, 
                        0x03, 
                        0x03, 
                        0x02, 
                        0x02, 
                        One, 
                        One, 
                        One, 
                        One, 
                        One, 
                        One, 
                        Zero
                    }
                })
                Name (TMD0, Buffer (0x14) {})
                CreateDWordField (TMD0, Zero, PIO0)
                CreateDWordField (TMD0, 0x04, DMA0)
                CreateDWordField (TMD0, 0x08, PIO1)
                CreateDWordField (TMD0, 0x0C, DMA1)
                CreateDWordField (TMD0, 0x10, CHNF)
                Name (PMPT, 0x20)
                Name (PMUE, 0x07)
                Name (PMUT, Zero)
                Name (PSPT, 0x20)
                Name (PSUE, 0x07)
                Name (PSUT, Zero)
                Name (SMPT, 0x20)
                Name (SMUE, 0x07)
                Name (SMUT, Zero)
                Name (SSPT, 0x20)
                Name (SSUE, 0x07)
                Name (SSUT, Zero)
                Name (FZTF, Buffer (0x07)
                {
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF5
                })
                OperationRegion (SAPR, PCI_Config, Zero, 0xFF)
                Field (SAPR, ByteAcc, NoLock, Preserve)
                {
                    SVID,   16, 
                    Offset (0x04), 
                    RX04,   3, 
                    Offset (0x0A), 
                    RX0A,   8,
                   // Offset (0xA0), 
                   // DET0,   2, 
                   // Offset (0xA1), 
                   // DET1,   2, 
                   // Offset (0xA2), 
                   // Offset (0xF8), 
                   // P0HP,   1, 
                   // Offset (0xF9), 
                   // P1HP,   1, 
                   // Offset (0xFA)
                }

                
                Method (_REG, 2, NotSerialized)  // _REG: Region Availability
                {
                    If (LEqual (Arg0, 0x02))
                    {
                        Store (Arg1, REGF)
                    }
                }

                
                Device (CHN0)
                {
                    Name (_ADR, Zero)  
                    Method (_STA, 0, NotSerialized)  
                    {
                        Return (0x0F)
                    }

                    Method (_GTM, 0, NotSerialized)  // _GTM: Get Timing Mode
                    {
                        Return (GTM (PMPT, PMUE, PMUT, PSPT, PSUE, PSUT))
                    }

                    Method (_STM, 3, NotSerialized)  // _STM: Set Timing Mode
                    {
                    }

                    Device (DRV0)
                    {
                        Name (_ADR, Zero)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }

                    Device (DRV1)
                    {
                        Name (_ADR, One)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }
                }

                Device (CHN1)
                {
                    Name (_ADR, One)  
                    Method (_STA, 0, NotSerialized)  
                    {
                        Return (0x0F)
                    }

                    Method (_GTM, 0, NotSerialized)  // _GTM: Get Timing Mode
                    {
                        Return (GTM (SMPT, SMUE, SMUT, SSPT, SSUE, SSUT))
                    }

                    Method (_STM, 3, NotSerialized)  // _STM: Set Timing Mode
                    {
                    }

                    Device (DRV0)
                    {
                        Name (_ADR, Zero)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }

                    Device (DRV1)
                    {
                        Name (_ADR, One)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }
                }

                Method (GTM, 6, Serialized)
                {
                    Store (Ones, PIO0)
                    Store (Ones, PIO1)
                    Store (Ones, DMA0)
                    Store (Ones, DMA1)
                    Store (0x10, CHNF)
                    If (REGF) {}
                    Else
                    {
                        Return (TMD0)
                    }

                    Store (Match (DerefOf (Index (TIM0, One)), MEQ, Arg0, MTR, 
                        Zero, Zero), Local6)
                    Store (DerefOf (Index (DerefOf (Index (TIM0, Zero)), Local6)), 
                        Local7)
                    Store (Local7, DMA0)
                    Store (Local7, PIO0)
                    Store (Match (DerefOf (Index (TIM0, One)), MEQ, Arg3, MTR, 
                        Zero, Zero), Local6)
                    Store (DerefOf (Index (DerefOf (Index (TIM0, Zero)), Local6)), 
                        Local7)
                    Store (Local7, DMA1)
                    Store (Local7, PIO1)
                    If (Arg1)
                    {
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x03)), Arg2)), 
                            Local5)
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x02)), Local5)), 
                            DMA0)
                        Or (CHNF, One, CHNF)
                    }

                    If (Arg4)
                    {
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x03)), Arg5)), 
                            Local5)
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x02)), Local5)), 
                            DMA1)
                        Or (CHNF, 0x04, CHNF)
                    }

                    Return (TMD0)
                }
            }

            Device (ATA2)
            {
                Name (_ADR, 0x000f0002)  
                Name (REGF, One)
                Name (TIM0, Package (0x04)
                {
                    Package (0x05)
                    {
                        0x78, 
                        0xB4, 
                        0xF0, 
                        0x017F, 
                        0x0258
                    }, 

                    Package (0x05)
                    {
                        0x20, 
                        0x22, 
                        0x33, 
                        0x47, 
                        0x5D
                    }, 

                    Package (0x07)
                    {
                        0x78, 
                        0x50, 
                        0x3C, 
                        0x2D, 
                        0x1E, 
                        0x14, 
                        0x0F
                    }, 

                    Package (0x0F)
                    {
                        0x06, 
                        0x05, 
                        0x04, 
                        0x04, 
                        0x03, 
                        0x03, 
                        0x02, 
                        0x02, 
                        One, 
                        One, 
                        One, 
                        One, 
                        One, 
                        One, 
                        Zero
                    }
                })
                Name (TMD0, Buffer (0x14) {})
                CreateDWordField (TMD0, Zero, PIO0)
                CreateDWordField (TMD0, 0x04, DMA0)
                CreateDWordField (TMD0, 0x08, PIO1)
                CreateDWordField (TMD0, 0x0C, DMA1)
                CreateDWordField (TMD0, 0x10, CHNF)
                Name (PMPT, 0x20)
                Name (PMUE, 0x07)
                Name (PMUT, Zero)
                Name (PSPT, 0x20)
                Name (PSUE, 0x07)
                Name (PSUT, Zero)
                Name (SMPT, 0x20)
                Name (SMUE, 0x07)
                Name (SMUT, Zero)
                Name (SSPT, 0x20)
                Name (SSUE, 0x07)
                Name (SSUT, Zero)
                Name (FZTF, Buffer (0x07)
                {
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF5
                })
                OperationRegion (SAPR, PCI_Config, Zero, 0xFF)
                Field (SAPR, ByteAcc, NoLock, Preserve)
                {
                    SVID,   16, 
                    Offset (0x04), 
                    RX04,   3, 
                    Offset (0x0A), 
                    RX0A,   8,
                   // Offset (0xA0), 
                   // DET0,   2, 
                   // Offset (0xA1), 
                   // DET1,   2, 
                   // Offset (0xA2), 
                   // Offset (0xF8), 
                   // P0HP,   1, 
                   // Offset (0xF9), 
                   // P1HP,   1, 
                   // Offset (0xFA)
                }

                
                Method (_REG, 2, NotSerialized)  // _REG: Region Availability
                {
                    If (LEqual (Arg0, 0x02))
                    {
                        Store (Arg1, REGF)
                    }
                }

                
                Device (CHN0)
                {
                    Name (_ADR, Zero)  
                    Method (_STA, 0, NotSerialized)  
                    {
                        Return (0x0F)
                    }

                    Method (_GTM, 0, NotSerialized)  // _GTM: Get Timing Mode
                    {
                        Return (GTM (PMPT, PMUE, PMUT, PSPT, PSUE, PSUT))
                    }

                    Method (_STM, 3, NotSerialized)  // _STM: Set Timing Mode
                    {
                    }

                    Device (DRV0)
                    {
                        Name (_ADR, Zero)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }

                    Device (DRV1)
                    {
                        Name (_ADR, One)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }
                }

                Device (CHN1)
                {
                    Name (_ADR, One)  
                    Method (_STA, 0, NotSerialized)  
                    {
                        Return (0x0F)
                    }

                    Method (_GTM, 0, NotSerialized)  // _GTM: Get Timing Mode
                    {
                        Return (GTM (SMPT, SMUE, SMUT, SSPT, SSUE, SSUT))
                    }

                    Method (_STM, 3, NotSerialized)  // _STM: Set Timing Mode
                    {
                    }

                    Device (DRV0)
                    {
                        Name (_ADR, Zero)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }

                    Device (DRV1)
                    {
                        Name (_ADR, One)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }
                }

                Method (GTM, 6, Serialized)
                {
                    Store (Ones, PIO0)
                    Store (Ones, PIO1)
                    Store (Ones, DMA0)
                    Store (Ones, DMA1)
                    Store (0x10, CHNF)
                    If (REGF) {}
                    Else
                    {
                        Return (TMD0)
                    }

                    Store (Match (DerefOf (Index (TIM0, One)), MEQ, Arg0, MTR, 
                        Zero, Zero), Local6)
                    Store (DerefOf (Index (DerefOf (Index (TIM0, Zero)), Local6)), 
                        Local7)
                    Store (Local7, DMA0)
                    Store (Local7, PIO0)
                    Store (Match (DerefOf (Index (TIM0, One)), MEQ, Arg3, MTR, 
                        Zero, Zero), Local6)
                    Store (DerefOf (Index (DerefOf (Index (TIM0, Zero)), Local6)), 
                        Local7)
                    Store (Local7, DMA1)
                    Store (Local7, PIO1)
                    If (Arg1)
                    {
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x03)), Arg2)), 
                            Local5)
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x02)), Local5)), 
                            DMA0)
                        Or (CHNF, One, CHNF)
                    }

                    If (Arg4)
                    {
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x03)), Arg5)), 
                            Local5)
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x02)), Local5)), 
                            DMA1)
                        Or (CHNF, 0x04, CHNF)
                    }

                    Return (TMD0)
                }
            }

            
            

            Device (SATA)
            {
                Name (_ADR, 0x00150000)  
                Name (REGF, One)
                Name (TIM0, Package (0x04)
                {
                    Package (0x05)
                    {
                        0x78, 
                        0xB4, 
                        0xF0, 
                        0x017F, 
                        0x0258
                    }, 

                    Package (0x05)
                    {
                        0x20, 
                        0x22, 
                        0x33, 
                        0x47, 
                        0x5D
                    }, 

                    Package (0x07)
                    {
                        0x78, 
                        0x50, 
                        0x3C, 
                        0x2D, 
                        0x1E, 
                        0x14, 
                        0x0F
                    }, 

                    Package (0x0F)
                    {
                        0x06, 
                        0x05, 
                        0x04, 
                        0x04, 
                        0x03, 
                        0x03, 
                        0x02, 
                        0x02, 
                        One, 
                        One, 
                        One, 
                        One, 
                        One, 
                        One, 
                        Zero
                    }
                })
                Name (TMD0, Buffer (0x14) {})
                CreateDWordField (TMD0, Zero, PIO0)
                CreateDWordField (TMD0, 0x04, DMA0)
                CreateDWordField (TMD0, 0x08, PIO1)
                CreateDWordField (TMD0, 0x0C, DMA1)
                CreateDWordField (TMD0, 0x10, CHNF)
                Name (PMPT, 0x20)
                Name (PMUE, 0x07)
                Name (PMUT, Zero)
                Name (PSPT, 0x20)
                Name (PSUE, 0x07)
                Name (PSUT, Zero)
                Name (SMPT, 0x20)
                Name (SMUE, 0x07)
                Name (SMUT, Zero)
                Name (SSPT, 0x20)
                Name (SSUE, 0x07)
                Name (SSUT, Zero)
                Name (FZTF, Buffer (0x07)
                {
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF5
                })
                OperationRegion (SAPR, PCI_Config, Zero, 0xFF)
                Field (SAPR, ByteAcc, NoLock, Preserve)
                {
                    SVID,   16, 
                    Offset (0x04), 
                    RX04,   3, 
                    Offset (0x0A), 
                    RX0A,   8,
                   // Offset (0xA0), 
                   // DET0,   2, 
                   // Offset (0xA1), 
                   // DET1,   2, 
                   // Offset (0xA2), 
                   // Offset (0xF8), 
                   // P0HP,   1, 
                   // Offset (0xF9), 
                   // P1HP,   1, 
                   // Offset (0xFA)
                }

                //Method (_INI, 0, NotSerialized)  
                //{
                //    If (LEqual (^^NBF6.SAHP, One)) {
                //        If (LEqual(OSYS, 0x2009)) {
                //            If (LAnd (LEqual (RX0A, 0x06), LOr (LEqual (P0HP, One), LEqual (P1HP, One)))) {
                //                Store (0x3C, ^^SBRG.G3CT)
                //                Store (One, ^^SBRG.G3RL)
                //                Store (0x02, ^^SBRG.G3TK)
                //                Store (One, ^^SBRG.G3ST)
                //                Store (One, ^^SBRG.G3SE)
                //            }
                //        }
                //    }
                //}

                Method (_REG, 2, NotSerialized)  // _REG: Region Availability
                {
                    If (LEqual (Arg0, 0x02))
                    {
                        Store (Arg1, REGF)
                    }
                }

                //Method (_STA, 0, NotSerialized)  
                //{
                //    If (LNotEqual (SVID, 0x1106))
                //    {
                //        Return (Zero)
                //    }
                //    Else
                //    {
                //        If (LEqual (RX04, Zero))
                //        {
                //            Return (0x0D)
                //        }
                //        Else
                //        {
                //            Return (0x0F)
                //        }
                //    }
                //}

                Device (CHN0)
                {
                    Name (_ADR, Zero)  
                    Method (_STA, 0, NotSerialized)  
                    {
                        Return (0x0F)
                    }

                    Method (_GTM, 0, NotSerialized)  // _GTM: Get Timing Mode
                    {
                        Return (GTM (PMPT, PMUE, PMUT, PSPT, PSUE, PSUT))
                    }

                    Method (_STM, 3, NotSerialized)  // _STM: Set Timing Mode
                    {
                    }

                    Device (DRV0)
                    {
                        Name (_ADR, Zero)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }

                    Device (DRV1)
                    {
                        Name (_ADR, One)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }
                }

                Device (CHN1)
                {
                    Name (_ADR, One)  
                    Method (_STA, 0, NotSerialized)  
                    {
                        Return (0x0F)
                    }

                    Method (_GTM, 0, NotSerialized)  // _GTM: Get Timing Mode
                    {
                        Return (GTM (SMPT, SMUE, SMUT, SSPT, SSUE, SSUT))
                    }

                    Method (_STM, 3, NotSerialized)  // _STM: Set Timing Mode
                    {
                    }

                    Device (DRV0)
                    {
                        Name (_ADR, Zero)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }

                    Device (DRV1)
                    {
                        Name (_ADR, One)  
                        Method (_GTF, 0, NotSerialized)  // _GTF: Get Task File
                        {
                            Return (FZTF)
                        }
                    }
                }

                Method (GTM, 6, Serialized)
                {
                    Store (Ones, PIO0)
                    Store (Ones, PIO1)
                    Store (Ones, DMA0)
                    Store (Ones, DMA1)
                    Store (0x10, CHNF)
                    If (REGF) {}
                    Else
                    {
                        Return (TMD0)
                    }

                    Store (Match (DerefOf (Index (TIM0, One)), MEQ, Arg0, MTR, 
                        Zero, Zero), Local6)
                    Store (DerefOf (Index (DerefOf (Index (TIM0, Zero)), Local6)), 
                        Local7)
                    Store (Local7, DMA0)
                    Store (Local7, PIO0)
                    Store (Match (DerefOf (Index (TIM0, One)), MEQ, Arg3, MTR, 
                        Zero, Zero), Local6)
                    Store (DerefOf (Index (DerefOf (Index (TIM0, Zero)), Local6)), 
                        Local7)
                    Store (Local7, DMA1)
                    Store (Local7, PIO1)
                    If (Arg1)
                    {
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x03)), Arg2)), 
                            Local5)
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x02)), Local5)), 
                            DMA0)
                        Or (CHNF, One, CHNF)
                    }

                    If (Arg4)
                    {
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x03)), Arg5)), 
                            Local5)
                        Store (DerefOf (Index (DerefOf (Index (TIM0, 0x02)), Local5)), 
                            DMA1)
                        Or (CHNF, 0x04, CHNF)
                    }

                    Return (TMD0)
                }
            }            
            