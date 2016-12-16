
#include "SmmPlatform.h"

#ifndef MDEPKG_NDEBUG	

VOID SleepSmiDebug(UINT8 SleepType)
{
  DEBUG((EFI_D_ERROR, "SLP:%X\n", SleepType));

  if(SleepType == 3){
    DumpIo4(0x800, 0x14);
    DumpIo4(0x820, 0xE8 - 0x20); 
  }
 
  DEBUG((EFI_D_ERROR, "\n"));  
}

#endif

