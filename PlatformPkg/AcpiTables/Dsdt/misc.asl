

    Method (_PIC, 1, NotSerialized)
    {
      If(Arg0) {
        Store(0xAA, DBG8)
      } Else {
        Store(0xAC, DBG8)
      }    
      Store (Arg0, PICM)
      ASL_COM((1, "_PIC ", PICM));
    }
    
    Name (PRWP, Package (0x02)
    {
        Zero, 
        Zero
    })
    Method (GPRW, 2, Serialized)
    {
        Name(SS1, 0)
        Name(SS3, 0)
        Name(SS4, 1)
        
        If(CondRefOf(\_S1)){
          Store(1, SS1)
        }
        If(CondRefOf(\_S3)){
          Store(1, SS3)
        }        
    
        Store (Arg0, Index (PRWP, Zero))
        Store (ShiftLeft (SS1, One), Local0)
        Or (Local0, ShiftLeft (SS3, 0x03), Local0)
        Or (Local0, ShiftLeft (SS4, 0x04), Local0)
        If (And (ShiftLeft (One, Arg1), Local0)) {
          Store (Arg1, Index (PRWP, One))
        } Else {
            ShiftRight (Local0, One, Local0)
            FindSetRightBit (Local0, Index (PRWP, One))
            If (LGreater (DerefOf (Index (PRWP, One)), Arg1)) {
                Store (Arg1, Index (PRWP, One))
            }
        }

        Return (PRWP)
    }

    OperationRegion (DEB0, SystemIO, 0x80, One)
    Field (DEB0, ByteAcc, NoLock, Preserve)
    {
        DBG8,   8
    }    

#ifndef MDEPKG_NDEBUG                                   // debug mode  
    Method(TCOM, 1, Serialized)
    {
      OperationRegion(COM3, SystemIO, 0x2E8, 0x08)
      Field(COM3, ByteAcc, NoLock, Preserve)
      {
        DAT8, 8,
        Offset(5),
            , 5,
        TRDY, 1,
      }
   
      Add(SizeOf(Arg0), One, Local0)
      Name(BUF0, Buffer(Local0){})
      Store(Arg0, BUF0)
      store(0, Local1)
      Decrement(Local0)
      While(LNotEqual(Local1, Local0)){
        while(LEqual(TRDY, Zero)){}
        Store(DerefOf(Index(BUF0, Local1)), DAT8)
        Increment(Local1)
      }
    }
    
    Method(DBGC, 3, Serialized)      // DBGC(count, string, int)
    {
      Name(CRLF, Buffer(2){0x0D, 0x0A})
      TCOM(Arg1)
      if(LEqual(Arg0, 2)){
        TCOM(ToHexString(Arg2))
      }  
      TCOM(CRLF)
    }        
#endif    


    