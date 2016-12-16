

#include "AcpiPlatform.h"
#include <AsiaCpuProtocol.h>






STATIC
EFI_STATUS 
GetPStateTableData (
  IN ACPU_PSTATE_TABLE  **PState
  )
{
  EFI_ASIA_CPU_PROTOCOL  *AsiaCpu;
  EFI_STATUS             Status;
  STATIC ACPU_PSTATE_TABLE  gPStateTbl;
  STATIC ACPU_PSTATE_TABLE  *gCpuPStateTbl = NULL;

  if(gCpuPStateTbl == NULL){
    Status = gBS->LocateProtocol(&gAsiaCpuProtocolGuid, NULL, (VOID**)&AsiaCpu);
    ASSERT_EFI_ERROR(Status); 
    Status = AsiaCpu->GetCpuPstateTable(&gPStateTbl);
    ASSERT_EFI_ERROR(Status); 
    gCpuPStateTbl = &gPStateTbl;
  }  

  *PState = gCpuPStateTbl;

  return EFI_SUCCESS;
}  


/*
DefPackage  := PackageOp PkgLength NumElements PackageElementList 
PackageOp := 0x12 

PkgLength  := PkgLeadByte | 
              <PkgLeadByte ByteData> | 
              <PkgLeadByte ByteData ByteData> | 
              <PkgLeadByte ByteData ByteData ByteData> 
 
PkgLeadByte  := <bit 7-6: ByteData count that follows (0-3)> 
                <bit 5-4: Only used if PkgLength < 63> 
                <bit 3-0: Least significant package length nybble> 
 
Note: The high 2 bits of the first byte reveal how many follow bytes are in the 
PkgLength. If the PkgLength has only one byte, bit 0 through 5 are used to encode the 
package length (in other words, values 0-63). If the package length value is more than 
63, more than one byte must be used for the encoding in which case bit 4 and 5 of the 
PkgLeadByte are reserved and must be zero. If the multiple bytes encoding is used, 
bits 0-3 of the PkgLeadByte become the least significant 4 bits of the resulting 
package length value. The next ByteData will become the next least significant 8 bits 
of the resulting value and so on, up to 3 ByteData bytes.  Thus, the maximum package 
length is 2**28.

0 ~ 63  00XXXXXX

43 24 -> [01]00[0011] 00100100 -> 243 = 2 + 1 + (1+47)*8 


[08] [56 50 53 53] [12]      [43 24] [08]
                         
Name  V  P  S  S   PackageOp  size    count                   
*/

EFI_STATUS
UpdateVpssPackage (
  IN UINT8  *VpssName
)
{
  UINTN                  Index;
  UINTN                  Count;
  UINTN                  AslCount;
  ACPU_PSTATE            *PState;  
  UINT8                  *Data8;
  EFI_STATUS             Status;
  ACPU_PSTATE_TABLE      *PStateTable;
  UINTN                  NewPkgLengh;
  

  Status = GetPStateTableData(&PStateTable);
  ASSERT_EFI_ERROR(Status);

  Count    = PStateTable->StatesNum;     // Real
  AslCount = *(VpssName+7);              // Pre-define

  if(Count > AslCount){Count = AslCount;}  
  NewPkgLengh = 3 + 0x48 * Count;
  *(VpssName+5) = (UINT8)(0x40 | (NewPkgLengh & 0xF));
  *(VpssName+6) = (UINT8)(((UINT32)NewPkgLengh)>>4);
  *(VpssName+7) = (UINT8)Count;          // update count

  Data8 = VpssName + 13;                 // first dword  
  ASSERT(*(Data8-1) == 0x0C);
  
  for(Index=0;Index<Count;Index++){

    PState = &PStateTable->PStateTable[Index];
    
    ASSERT(*(Data8-1) == 0x0C);    
    *(UINT32*)Data8 = PState->CoreFreq;
    Data8 += 5;
    
    ASSERT(*(Data8-1) == 0x0C);    
    *(UINT32*)Data8 = PState->Power;
    Data8 += 5;
    
    ASSERT(*(Data8-1) == 0x0C);  
    *(UINT32*)Data8 = 10;
    Data8 += 5;
    
    ASSERT(*(Data8-1) == 0x0C);    
    *(UINT32*)Data8 = 10;
    Data8 += 8;  
    
/*
If the ControlMask contains a value of 0, OSPM will simply perform a Write MSR 
using the ControlValue and the Address described in the _PCT to initiate the 
PState transition.

TargetMSRValue = ReadMSR(_PCT ControlAddress)		      // read current MSR data
TargetMSRValue = (TargetMSRValue & XPSS ControlMask)	// mask unwanted data
TargetMSRValue = (TargetMSRValue | XPSS ControlValue)	// apply new state data

CurrentMSRValue = ReadMSR(_PCT StatusAddress)		      // read current MSR data
CurrentMSRValue = (CurrentMSRValue & XPSS StatusMask)	// mask unwanted data
If (CurrentMSRValue equals XPSS StatusValue) then transition successful.
If (CurrentMSRValue not equal to XPSS StatusValue) then transition failed.
*/    
    
    ASSERT(*(Data8-2) == 0x0A);       // Control 
    *(UINT16*)Data8 = (PState->BusRatio<<8)|PState->VID;
    Data8 += 12;

    ASSERT(*(Data8-2) == 0x0A);       // Status  
    *(UINT16*)Data8 = (PState->BusRatio<<8)|PState->VID;    
    Data8 += 12;
    
    ASSERT(*(Data8-2) == 0x0A);       // Control Mask, keep 0 
    Data8 += 12;  
    
    ASSERT(*(Data8-2) == 0x0A);       // Status Mask
    *(UINT16*)Data8 = 0xFFFF;
    Data8 += 13;
  }  
  
  return Status;
}


EFI_STATUS
UpdateNpssPackage (
  IN UINT8  *NpssName
)
{
  UINTN                  Index;
  UINTN                  Count;
  UINTN                  AslCount;
  ACPU_PSTATE            *PState;  
  UINT8                  *Data8;
  EFI_STATUS             Status;
  ACPU_PSTATE_TABLE      *PStateTable;
  UINTN                  NewPkgLengh;


  Status = GetPStateTableData(&PStateTable);
  ASSERT_EFI_ERROR(Status);

  Count    = PStateTable->StatesNum;     // Real
  AslCount = *(NpssName+7);              // Pre-define

  if(Count > AslCount){Count = AslCount;} 

// 4B 10 -> 10B = 2 + 1 + (1+20)*8
  NewPkgLengh = 3 + 0x21 * Count;
  *(NpssName+5) = (UINT8)(0x40 | (NewPkgLengh & 0xF));
  *(NpssName+6) = (UINT8)(((UINT32)NewPkgLengh)>>4);
  *(NpssName+7) = (UINT8)Count;          // update count
   
  Data8 = NpssName + 12;        // first dword  
  ASSERT(*(Data8-1) == 0x0C);

  for(Index=0;Index<Count;Index++){

    PState = &PStateTable->PStateTable[Index];
    
    ASSERT(*(Data8-1) == 0x0C);    
    *(UINT32*)Data8 = PState->CoreFreq;
    Data8 += 5;
    
    ASSERT(*(Data8-1) == 0x0C);    
    *(UINT32*)Data8 = PState->Power;
    Data8 += 5;
    
    ASSERT(*(Data8-1) == 0x0C);  
    *(UINT32*)Data8 = 10;
    Data8 += 5;
    
    ASSERT(*(Data8-1) == 0x0C);    
    *(UINT32*)Data8 = 10;
    Data8 += 5;  

    ASSERT(*(Data8-1) == 0x0C);    
    *(UINT32*)Data8 = (PState->BusRatio<<8)|PState->VID;
    Data8 += 5;

    ASSERT(*(Data8-1) == 0x0C);    
    *(UINT32*)Data8 = (PState->BusRatio<<8)|PState->VID;
    Data8 += 8;
  }  
  
  return Status;
}



