
#ifndef __PLAT_COMM_LIB_H__
#define __PLAT_COMM_LIB_H__

#include <Uefi.h>

typedef union {
  struct {
    UINT32  Lower32;
    UINT32  Upper32;
  } Uint32;
  UINT64    Uint64;
} DATA_64;

typedef union {
  struct {
    UINT8  Data[4];
  } Uint8;
  UINT32  Uint32;
} DATA_32_4;

extern EFI_GUID gEfiPlatformMemInfoGuid;

UINT8 PcieRead8(UINTN PcieAddr);
VOID DumpMem8(VOID *Base, UINTN Size);
VOID DumpMem32(VOID *Base, UINTN Size);
VOID DumpIo4(UINTN Base, UINT16 Size);
VOID DumpCmos();
VOID DumpSysSetting();
VOID DumpAllPci();
VOID ZX_DumpPciDevSetting();

VOID DumpPci(UINT8 Bus, UINT8 Dev, UINT8 Func);


BOOLEAN CheckKbcPresent(VOID);
BOOLEAN CheckMsPresent(VOID);
BOOLEAN CheckKbPresent(VOID);
EFI_STATUS LoadXhciFw(VOID *HubFw, VOID *McuFw);
VOID *GetPlatformMemInfo(VOID);
VOID *GetPlatformDimmInfo(VOID);

EFI_STATUS
KbcCmdReadData (
  IN  UINT8  Cmd,
  OUT UINT8  *Data OPTIONAL
  );

typedef struct {
  UINT32  Vdid;
  UINT32  Ssid;
  UINT8   RevId;
  UINT8   FpSupport;
  UINT16  RearJacks;
  UINT16  FrontJacks;
} OEM_VERB_TABLE_HEADER;

typedef struct {
  OEM_VERB_TABLE_HEADER  Hdr;
  UINTN                  VerbDataSize;
  UINT32                 *VerbData;
} OEM_VERB_TABLE;  
  
EFI_STATUS 
AzaliaLoadVerbTable (
  IN OEM_VERB_TABLE  *OemVerbTable, 
  IN UINTN           OemVerbTableSize
  );

UINT8 WaitIdeDeviceReady(UINTN Bus, UINTN Dev, UINTN Func, UINTN DetectTimeOutInms);

extern OEM_VERB_TABLE gOemVerbTable[];
extern UINTN gOemVerbTableSize;


BOOLEAN LibVerifyDataCrc32(VOID *Data, UINTN DataSize, UINTN CrcOffset, UINT32 *CalcCrc32 OPTIONAL);

EFI_STATUS 
EFIAPI
LibCalcCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *CrcOut
  );  

UINT8 CmosRead(UINT8 Address);
VOID  CmosWrite(UINT8 Address, UINT8 Data);  
UINT8 CheckAndConvertBcd8ToDecimal8(UINT8 Value);
EFI_STATUS RtcWaitToUpdate();

VOID *GetAcpiRam(VOID);
VOID *GetAcpiTableScat(VOID);
VOID GetS3Cr3Stack(UINT32 *S3Cr3, UINT32 *S3StackBase, UINT32 *S3StackSize);
VOID *GetSetupDataHobData(VOID);
VOID PlatRecordS3DebugData(CHAR8 *Name, UINT32 Data32);
VOID *GetS3RecordTable();
VOID *GetS3MtrrTable();
VOID *GetCarTopData();
BOOLEAN IsSmrrTypeSetWB();

EFI_STATUS KbcWaitInputBufferFree();

VOID SetCmosVRT(BOOLEAN OnOff);

UINT8 CaculateWeekDay(UINT16 y, UINT8 m, UINT8 d);
BOOLEAN IsLeapYear(UINT16 Year);

VOID *LibGetGOP();
EFI_STATUS BltSaveAndRetore(VOID *BootServices, BOOLEAN Save);

VOID SystemSoftOff();

VOID 
AcpiTableUpdateChksum (
  IN VOID *AcpiTable
  );

extern VOID  *gObLanDp;
extern UINTN gObLanDpSize;



#endif


