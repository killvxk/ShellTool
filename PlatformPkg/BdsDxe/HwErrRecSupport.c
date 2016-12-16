
#include "HwErrRecSupport.h"

/**
  Set the HwErrRecSupport variable contains a binary UINT16 that supplies the
  level of support for Hardware Error Record Persistence that is implemented
  by the platform.

**/
VOID
InitializeHwErrRecSupport (
  VOID
  )
{
  UINT16 HardwareErrorRecordLevel;
  
  HardwareErrorRecordLevel = PcdGet16 (PcdHardwareErrorRecordLevel);
  
  if (HardwareErrorRecordLevel != 0) {
    //
    // Set original value again to make sure this value is stored into variable
    // area but not PCD database.
    // if level value equal 0, no need set to 0 to variable area because UEFI specification
    // define same behavior between no value or 0 value for L"HwErrRecSupport"
    //
    PcdSet16 (PcdHardwareErrorRecordLevel, HardwareErrorRecordLevel);
  }
}
