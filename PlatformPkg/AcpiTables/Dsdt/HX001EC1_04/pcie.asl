
	Device (NPEG)
	{
		Name (_ADR, 0x00020000)           
		Method (_PRT, 0, NotSerialized)   // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR01)
			}

			Return (PR01)
		}

		Device (PEGS)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NPG0)
	{
		Name (_ADR, 0x00040000)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR06)
			}

			Return (PR06)
		}

		Device (PG0S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NPG2)
	{
		Name (_ADR, 0x00050000)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR08)
			}

			Return (PR08)
		}

		Device (PG2S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

