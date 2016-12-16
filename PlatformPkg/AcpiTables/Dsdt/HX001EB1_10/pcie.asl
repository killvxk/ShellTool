
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

	Device (NPE0)
	{
		Name (_ADR, 0x00030000)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR02)
			}

			Return (PR02)
		}

		Device (PE0S)
		{
			Name(_ADR, Zero)
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NPE1)
	{
		Name (_ADR, 0x00030001)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR03)
			}

			Return (PR03)
		}

		Device (PE1S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NPE2)
	{
		Name (_ADR, 0x00030002)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR04)
			}

			Return (PR04)
		}

		Device (PE2S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NPE3)
	{
		Name (_ADR, 0x00030003)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR05)
			}

			Return (PR05)
		}

		Device (PE3S)
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

	Device (NPG3)
	{
		Name (_ADR, 0x00050001)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR09)
			}

			Return (PR09)
		}

		Device (PG3S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}
