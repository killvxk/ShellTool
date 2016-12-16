
#ifndef __PLAT_S3_RECORD_H__
#define __PLAT_S3_RECORD_H__

#include <AcpiRam.h>
#include <Library/MtrrLib.h>

// PCD is hard to use in a common header file, so here we define 
// a big enough count and guard it with assert in a c code.
#define PLATFORM_MAX_CPU_PROC_COUNT    32  

#define PLAT_S3_RECORD_SIGNATURE  SIGNATURE_64('_', 'P', 'L', 'A', 'T', 'S', '3', 'R')

#define PLAT_S3_SLEEP_SLEEP            0x03
#define PLAT_S3_SLEEP_RESUME           0x30


#define PLAT_DBG_DATA_DD_COUNT         32

typedef struct {
  UINT32  Id;
  UINT32  Data;
} PLAT_S3_DBG_DATA;  

typedef struct {
  UINT64  Signature;
  
  UINT32  XhciMcuFw;
  UINT32  XhciMcuFwSize;
  UINT32  XhciMcuFwCrc32;
  
  UINT32  XhciHubFw;
  UINT32  XhciHubFwSize;
  UINT32  XhciHubFwCrc32;
  
  UINT32  S3StackBase;	
  UINT32  S3StackSize;
  UINT32  S3Cr3;
  UINT32  ScatAddr;				// EFI_SMM_COMMUNICATION_ACPI_TABLE

  UINT8   S3Sleep;
  UINT8   SmrrType;
  UINT32  CpuCount;
  UINT32  SmmUcData;
  UINT32  SmmUcDataSize;
  UINT32  SmrrBase;
  UINT32  SmrrSize;
  UINT32  CpuApVector;
  UINT32  CpuApicId[PLATFORM_MAX_CPU_PROC_COUNT];
  UINT32  CpuSmBase[PLATFORM_MAX_CPU_PROC_COUNT];
  
  MTRR_SETTINGS      MtrrTable;
  EFI_ACPI_RAM_DATA  AcpiRam;

  UINT32   DebugDataIndex;
  PLAT_S3_DBG_DATA DebugData[PLAT_DBG_DATA_DD_COUNT];
  
} PLATFORM_S3_RECORD;



#endif                        // __PLAT_S3_RECORD_H__

