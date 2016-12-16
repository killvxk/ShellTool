
#include "SmmPlatform.h"
#include <Library/TimerLib.h>
#include <Library/IoLib.h>

#define KEY_DATA                    0x60
#define KEY_CMD_STATE               0x64
#define KEY_STATUS                  0x64
#define KBC_IN_BUFFUL               BIT1  
#define KEYBOARD_TIMEOUT            65536   // 0.07s

/**
  Read data register .

  @return return the value 

**/
UINT8
KeyReadDataRegister (
  )

{
  UINT8                               Data;

  Data = IoRead8(KEY_DATA);

  return Data;
}

/**
  Write data register.

  @param Data      value wanted to be written

**/
VOID
KeyWriteDataRegister (
  IN UINT8                   Data
  )
{
  IoWrite8(KEY_DATA, Data);
}

/**
  Read status register.

  @return value in status register

**/
UINT8
KeyReadStatusRegister (
  )
{
  UINT8                               Data;
  Data = IoRead8(KEY_STATUS);
  return Data;
}

/**
  Write command register .

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Data      The value wanted to be written

**/
VOID
KeyWriteCommandRegister (
  IN UINT8                   Data
  )
{
  IoWrite8(KEY_CMD_STATE, Data);
}


/**
  write key to keyboard
  @param Data      value wanted to be written

  @retval EFI_TIMEOUT   The input buffer register is full for putting new value util timeout
  @retval EFI_SUCCESS   The new value is sucess put into input buffer register.

**/
EFI_STATUS
KeyboardWrite (
  IN UINT8                   Data
  )
{
  UINT32  TimeOut;
  UINT32  RegEmptied;

  TimeOut     = 0;
  RegEmptied  = 0;

  //
  // wait for input buffer empty
  //
  for (TimeOut = 0; TimeOut < KEYBOARD_TIMEOUT; TimeOut += 30) {
    if ((KeyReadStatusRegister () & 0x02) == 0) {
      RegEmptied = 1;
      break;
    }

    MicroSecondDelay (30);
  }

  if (RegEmptied == 0) {
    return EFI_TIMEOUT;
  }
  //
  // Write it
  //
  KeyWriteDataRegister (Data);

  return EFI_SUCCESS;
}


/**
  wait for a specific value to be presented on
  8042 Data register by keyboard and then read it,
  used in keyboard commands ack

  @param Value     the value wanted to be waited.

  @retval EFI_TIMEOUT Fail to get specific value in given time
  @retval EFI_SUCCESS Success to get specific value in given time.
  
**/
EFI_STATUS
KeyboardWaitForValue (
  IN UINT8                   Value
  )
{
  UINT8   Data;
  UINT32  TimeOut;
  UINT32  SumTimeOut;
  UINT32  GotIt;

  GotIt       = 0;
  TimeOut     = 0;
  SumTimeOut  = 0;

  //
  // Make sure the initial value of 'Data' is different from 'Value'
  //
  Data = 0;
  if (Data == Value) {
    Data = 1;
  }
  //
  // Read from 8042 (multiple times if needed)
  // until the expected value appears
  // use SumTimeOut to control the iteration
  //
  while (1) {
    //
    // Perform a read
    //
    for (TimeOut = 0; TimeOut < KEYBOARD_TIMEOUT; TimeOut += 30) {
      if (KeyReadStatusRegister () & 0x01) {
        Data = KeyReadDataRegister ();
        break;
      }

      MicroSecondDelay (30);
    }

    SumTimeOut += TimeOut;

    if (Data == Value) {
      GotIt = 1;
      break;
    }

    if (SumTimeOut >= 1000000) {
      break;
    }
  }
  //
  // Check results
  //
  if (GotIt == 1) {
    return EFI_SUCCESS;
  } else {
    return EFI_TIMEOUT;
  }

}



/// TGR-2016062201. SOC not include USB related controller.
VOID DisableUhciIo6064Trap()
{
#if 0
  UINTN  Index;
  UINT8  Data8;	

  for (Index = 0; Index < 5; Index++) {
    if(MmioRead16(UHCI_PCI_REG(Index, 0)) == 0xFFFF){
      continue;
    }
		Data8 = MmioRead8(UHCI_PCI_REG(Index, UHCI_LEGSPT_REG));
    if(Data8 & 0xF){
      Data8 &= 0xF0;
      DEBUG((EFI_D_INFO, "UHCI%d.%X <- %X\n", Index, UHCI_LEGSPT_REG, Data8));
      MmioWrite8(UHCI_PCI_REG(Index, UHCI_LEGSPT_REG), Data8);
    }
  }
#endif
  ////
}


/**
  Show keyboard status lights according to
  indicators in ConsoleIn.

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  
  @return status of updating keyboard register

**/
EFI_STATUS
TurnOffKbLed (
  )
{
  EFI_STATUS  Status;

  if(!(gAcpiRam->IoStates & IO_STATE_PS2_KB_PRESENT)){
    return EFI_SUCCESS;
  }		

  DisableUhciIo6064Trap(); // empty function for CHX001.

  Status = KeyboardWrite(0xed);  
  if (EFI_ERROR (Status)) {
    return Status;
  }
  KeyboardWaitForValue(0xfa);

  KbcWaitInputBufferFree();
  Status = KeyboardWrite(0);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  KeyboardWaitForValue(0xfa);
  
  return Status;
}
 
