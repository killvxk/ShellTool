
	Device (NBPG)
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

		Device (NPGS)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NBP0)
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

		Device (NP0S)
		{
			Name(_ADR, Zero)
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NBP2)
	{
		Name (_ADR, 0x00030002)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR03)
			}

			Return (PR03)
		}

		Device (NP2S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NBP3)
	{
		Name (_ADR, 0x00030003)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR04)
			}

			Return (PR04)
		}

		Device (NP3S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

	Device (NBG0)
	{
		Name (_ADR, 0x00040000)  
		Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
		{
			If (PICM)
			{
				Return (AR05)
			}

			Return (PR05)
		}

		Device (NG0S)
		{
			Name (_ADR, Zero)  
			Method (_PRW, 0, NotSerialized)  
			{
				Return (GPRW (0x10, 0x04))
			}
		}
	}

