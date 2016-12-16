
#ifndef __PLATFORM_BOOT_MANAGER_LIB_H_
#define __PLATFORM_BOOT_MANAGER_LIB_H_
#include <Library/UefiBootManagerLib.h>

//
// Bit mask returned from PlatformBootManagerInit
// We can define more value in future per requirement
//
#define PLATFORM_BOOT_MANAGER_ENABLE_HOTKEY      0x00000001
#define PLATFORM_BOOT_MANAGER_ENABLE_TIMEOUT     0x00000002
#define PLATFORM_BOOT_MANAGER_ENABLE_ALL         0xFFFFFFFF

/**
  Do the platform specific action before the console is ready
  Possible things that can be done in PlatformBootManagerInit:
  > Update console variable: 1. include hot-plug devices; 2. Clear ConIn and add SOL for AMT
  > Register new Driver#### or Boot####
  > Signal ReadyToLock event
  > Authentication action: 1. connect Auth devices; 2. Identify auto logon user.

  @return  Bit mask to indicate the platform boot manager requirement.
**/
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  );

/**
  Do the platform specific action after the console is ready
  Possible things that can be done in PlatformBootManagerConsoleReady:
  > Dynamically switch output mode from 100x31 to 80x25 for AMT SOL
  > Signal console ready platform customized event
  > Run diagnostics: 1. driver health check; 2. memory test
  > Connect certain devices: 1. connect all; 2. connect nothing; 3. connect platform specified devices
  > Dispatch aditional option roms
  > Special boot: e.g.: AMT boot, enter UI (AMT boot also can be achieved by insert boot option in PlatformBootManagerInit)
  
   @param   PlatformBdsRequest  Original platform boot manager requirement.
   
   @return  Bit mask to indicate the udpated platform boot manager requirement.
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  );

VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16  Timeout,  
  UINT16  TimeoutRemain
  );


/**
  Clean the boot prompt.
**/
VOID 
CleanBootPrompt (
  VOID
  );
#endif
