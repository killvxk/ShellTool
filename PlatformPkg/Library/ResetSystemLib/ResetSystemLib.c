
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
////
#include "PlatformDefinition.h"
////
VOID ZXChipsetCommonFunc(VOID)
{
	//UINT8 Buffer8;
	UINT32 PmTimer32, Buffer32;

	PmTimer32 = (IoRead32(PMIO_REG(PMIO_TIMER_REG)) + 0x280000) & 0xF00000;	//Rx08
	do {
		Buffer32 = IoRead32(PMIO_REG(PMIO_TIMER_REG)) & 0xF00000;	//Rx08
	} while (PmTimer32 != Buffer32);

	// Clear GP3 2nd Timout Status
	IoWrite8(PMIO_REG(0x40), 0x04);	//Rx40
	// Clear GP3 Timeout Status
	IoWrite8(PMIO_REG(0x28 + 1), 0x20);	//Rx29

}
////

VOID
AcpiPmControl (
  UINTN SuspendType
  )
{
  UINT16  AcpiIoBase;
  
  ASSERT (SuspendType <= 7);         // 3 bits max value is 7
  
  AcpiIoBase = PcdGet16(AcpiIoPortBaseAddress);
  IoBitFieldWrite16(AcpiIoBase + 4, 10, 13, (UINT16)SuspendType);
  IoOr16(AcpiIoBase + 4, BIT13);
  CpuDeadLoop ();
}

/**
  Calling this function causes a system-wide reset. This sets
  all circuitry within the system to its initial state. This type of reset
  is asynchronous to system operation and operates without regard to
  cycle boundaries.

  System reset should not return, if it returns, it means the system does
  not support cold reset.
**/
VOID
EFIAPI
ResetCold (
  VOID
  )
{
  IoWrite8 (0xCF9, BIT2 | BIT1); // 1st choice: PIIX3 RCR, RCPU|SRST
  MicroSecondDelay (50);

  IoWrite8 (0x64, 0xfe);         // 2nd choice: keyboard controller
  CpuDeadLoop ();
}

/**
  Calling this function causes a system-wide initialization. The processors
  are set to their initial state, and pending cycles are not corrupted.

  System reset should not return, if it returns, it means the system does
  not support warm reset.
**/
VOID
EFIAPI
ResetWarm (
  VOID
  )
{
  IoWrite8 (0x64, 0xfe);
  CpuDeadLoop ();
}

/**
  Calling this function causes the system to enter a power state equivalent
  to the ACPI G2/S5 or G3 states.

  System shutdown should not return, if it returns, it means the system does
  not support shut down reset.
**/
VOID
EFIAPI
ResetShutdown (
  VOID
  )
{
  AcpiPmControl (2);
  ASSERT (FALSE);
}


/**
  Calling this function causes the system to enter a power state for capsule
  update.

  Reset update should not return, if it returns, it means the system does
  not support capsule update.

**/
VOID
EFIAPI
EnterS3WithImmediateWake (
  VOID
  )
{
  AcpiPmControl (1);
  ASSERT (FALSE);
}
