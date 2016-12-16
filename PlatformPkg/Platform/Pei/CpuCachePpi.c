
#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Ppi/CpuCachePpi.h>

#define  ALIGNED_SEED          0x01010101


#define MAXIMUM_VARIABLE_MTRR_NUMBER  10
#define V_EFI_FIXED_MTRR_NUMBER       11

#define EFI_CACHE_VARIABLE_MTRR_BASE          0x00000200
#define   V_EFI_FIXED_MTRR_NUMBER                     11
#define EFI_IA32_MTRR_FIX64K_00000            0x00000250
#define EFI_IA32_MTRR_FIX16K_80000            0x00000258
#define EFI_IA32_MTRR_FIX16K_A0000            0x00000259
#define EFI_IA32_MTRR_FIX4K_C0000             0x00000268
#define EFI_IA32_MTRR_FIX4K_C8000             0x00000269
#define EFI_IA32_MTRR_FIX4K_D0000             0x0000026A
#define EFI_IA32_MTRR_FIX4K_D8000             0x0000026B
#define EFI_IA32_MTRR_FIX4K_E0000             0x0000026C
#define EFI_IA32_MTRR_FIX4K_E8000             0x0000026D
#define EFI_IA32_MTRR_FIX4K_F0000             0x0000026E
#define EFI_IA32_MTRR_FIX4K_F8000             0x0000026F
#define EFI_CACHE_IA32_MTRR_DEF_TYPE          0x000002FF
#define   B_EFI_CACHE_MTRR_VALID                BIT11
#define   B_EFI_CACHE_FIXED_MTRR_VALID          BIT10

typedef struct {
  UINT32  RegEax;
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;
} EFI_CPUID_REGISTER;

#define EFI_CPUID_EXTENDED_FUNCTION           0x80000000
#define EFI_CPUID_VIR_PHY_ADDRESS_SIZE        0x80000008

#define EFI_CACHE_UNCACHEABLE                 0
#define EFI_CACHE_WRITECOMBINING              1
#define EFI_CACHE_WRITETHROUGH                4
#define EFI_CACHE_WRITEPROTECTED              5
#define EFI_CACHE_WRITEBACK                   6

#define EFI_IA32_MTRR_CAP                     0x000000FE
#define   B_EFI_IA32_MTRR_VARIABLE_SUPPORT      0xFF
#define   B_EFI_IA32_MTRR_CAP_FIXED_SUPPORT     BIT8
#define   B_EFI_IA32_MTRR_CAP_SMRR_SUPPORT      BIT11
#define   B_EFI_IA32_MTRR_CAP_EMRR_SUPPORT      BIT12



typedef struct {
  UINT64  MsrValue;
  BOOLEAN Changed;
} MTRR_VALUE;

typedef struct {
  PEI_CACHE_PPI           Ppi;
  EFI_PEI_PPI_DESCRIPTOR  PpiDesc;
  MTRR_VALUE              FixedMtrrValue[V_EFI_FIXED_MTRR_NUMBER];
  MTRR_VALUE              VariableMtrrValue[MAXIMUM_VARIABLE_MTRR_NUMBER*2];
  BOOLEAN                 FixedMtrrChanged;
  BOOLEAN                 VariableMtrrChanged;
} CACHE_PPI_INSTANCE;

#define PEI_CACHE_PPI_INSTANCE_FROM_THIS(a) BASE_CR(a, CACHE_PPI_INSTANCE, Ppi)



INT8
CheckDirection (
  IN UINT64      Input
  );

UINT64
PeiPower2MaxMemory (
  IN UINT64     MemoryLength
  );

VOID
EfiDisableCacheMtrr (
  IN UINT64     *OldMtrr
  );

VOID
EfiRecoverCacheMtrr (
  IN BOOLEAN     EnableMtrr,
  IN UINT64      OldMtrr
  );

VOID
EfiProgramMtrr (
  IN  PEI_CACHE_PPI            *This,
  IN  UINTN                     MtrrNumber,
  IN  EFI_PHYSICAL_ADDRESS      MemoryAddress,
  IN  UINT64                    MemoryLength,
  IN  EFI_MEMORY_CACHE_TYPE     MemoryCacheType,
  IN  UINT64                    ValidMtrrAddressMask
  );

EFI_STATUS
SearchForExactMtrr (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  PEI_CACHE_PPI             *This,
  IN  EFI_PHYSICAL_ADDRESS      MemoryAddress,
  IN  UINT64                    MemoryLength,
  IN  UINT64                    ValidMtrrAddressMask,
  OUT UINT32                    *UsedMsrNum,
  OUT EFI_MEMORY_CACHE_TYPE     *MemoryCacheType
  );

BOOLEAN
IsDefaultType (
  IN EFI_MEMORY_CACHE_TYPE     MemoryCacheType
  );

EFI_STATUS
DisableCacheAsRam (
  VOID
  );

typedef struct _ALIGNED_DWORD {
  UINT32 High;
  UINT32 Low;
} ALIGNED_DWORD;

typedef union _ALIGNED {
  UINT64        AlignedQword;
  ALIGNED_DWORD AlignedDword;
} ALIGNED;

typedef struct {
  UINT32  Msr;
  UINT32  BaseAddress;
  UINT32  Length;
} FIXED_MTRR;

FIXED_MTRR  mFixedMtrrTable[] = {
  {
    EFI_IA32_MTRR_FIX64K_00000,
    0,
    0x10000
  },
  {
    EFI_IA32_MTRR_FIX16K_80000,
    0x80000,
    0x4000
  },
  {
    EFI_IA32_MTRR_FIX16K_A0000,
    0xA0000,
    0x4000
  },
  {
    EFI_IA32_MTRR_FIX4K_C0000,
    0xC0000,
    0x1000
  },
  {
    EFI_IA32_MTRR_FIX4K_C8000,
    0xC8000,
    0x1000
  },
  {
    EFI_IA32_MTRR_FIX4K_D0000,
    0xD0000,
    0x1000
  },
  {
    EFI_IA32_MTRR_FIX4K_D8000,
    0xD8000,
    0x1000
  },
  {
    EFI_IA32_MTRR_FIX4K_E0000,
    0xE0000,
    0x1000
  },
  {
    EFI_IA32_MTRR_FIX4K_E8000,
    0xE8000,
    0x1000
  },
  {
    EFI_IA32_MTRR_FIX4K_F0000,
    0xF0000,
    0x1000
  },
  {
    EFI_IA32_MTRR_FIX4K_F8000,
    0xF8000,
    0x1000
  },
  {
    0,
    0x100000,
    0
  }
};

EFI_GUID gCpuCachePpiGuid = CPU_CACHE_PPI_GUID;

VOID
WriteMsrToBuffer (
  IN PEI_CACHE_PPI  *This,
  IN UINT32         MsrNum,
  IN UINT64         UpdateValue
)
  /*++
  
  Routine Description:
  
    Update MTRR setting to memory buffer
  
  Arguments:
  
    This                  - Current instance of Pei Cache PPI.
    MsrNum                - offset 0-10 maps to Fixed MTRR table
                            offset above 0x200 maps to Variable MTRR table 
    UpdateValue           - MTRR setting

  Returns:
  
    No
  
  --*/
{
  CACHE_PPI_INSTANCE *CachePpiInstance;
  CachePpiInstance = PEI_CACHE_PPI_INSTANCE_FROM_THIS (This);
  if (MsrNum >= EFI_CACHE_VARIABLE_MTRR_BASE) {
    if ((MsrNum - EFI_CACHE_VARIABLE_MTRR_BASE) >= MAXIMUM_VARIABLE_MTRR_NUMBER*2) {
      ASSERT (FALSE);
      return;
    }
    CachePpiInstance->VariableMtrrChanged = TRUE;
    CachePpiInstance->VariableMtrrValue[MsrNum-EFI_CACHE_VARIABLE_MTRR_BASE].Changed = TRUE;
    CachePpiInstance->VariableMtrrValue[MsrNum-EFI_CACHE_VARIABLE_MTRR_BASE].MsrValue = UpdateValue; 
  } else {
    if (MsrNum >= V_EFI_FIXED_MTRR_NUMBER) {
      ASSERT (FALSE);
      return;
    }
    CachePpiInstance->FixedMtrrChanged = TRUE;
    CachePpiInstance->FixedMtrrValue[MsrNum].Changed = TRUE;
    CachePpiInstance->FixedMtrrValue[MsrNum].MsrValue = UpdateValue; 
  }
}

UINT64
ReadMsrFromBuffer (
  IN PEI_CACHE_PPI  *This,
  IN UINT32         MsrNum
)
  /*++
  
  Routine Description:
  
    Read MTRR from Buffer. If buffer not ready, read from real MSR instead.
  
  Arguments:
  
    This                  - Current instance of Pei Cache PPI.
    MsrNum                - offset 0-10 maps to Fixed MTRR table
                            offset above 0x200 maps to Variable MTRR table 
  
  Returns:
  
    Return MTRR setting
  
  --*/
{
  UINT64 MtrrVal;
  CACHE_PPI_INSTANCE *CachePpiInstance;
  CachePpiInstance = PEI_CACHE_PPI_INSTANCE_FROM_THIS (This);
  if (MsrNum >= EFI_CACHE_VARIABLE_MTRR_BASE) {
    if ((MsrNum - EFI_CACHE_VARIABLE_MTRR_BASE) >= MAXIMUM_VARIABLE_MTRR_NUMBER*2) {
      ASSERT (FALSE);
      return 0;
    }
    if (CachePpiInstance->VariableMtrrValue[MsrNum-EFI_CACHE_VARIABLE_MTRR_BASE].Changed) {
      MtrrVal   = CachePpiInstance->VariableMtrrValue[MsrNum-EFI_CACHE_VARIABLE_MTRR_BASE].MsrValue;
    } else {
      MtrrVal   = AsmReadMsr64 (MsrNum);
    }
  } else {
    if (MsrNum >= V_EFI_FIXED_MTRR_NUMBER) {
      ASSERT (FALSE);
      return 0;
    }
    if (CachePpiInstance->FixedMtrrValue[MsrNum].Changed) {
      MtrrVal   = CachePpiInstance->FixedMtrrValue[MsrNum].MsrValue;
    } else {
      MtrrVal   = AsmReadMsr64 (mFixedMtrrTable[MsrNum].Msr);
    }
  }
  return MtrrVal; 
}

VOID
EfiDisableCacheMtrr (
  OUT UINT64            *OldMtrr
  )
/*++
  Routine Description:
  
    Disable cache and its mtrr
    
  Arguments:
  
    OldMtrr - To return the Old MTRR value
    
  Returns:
  
    None 
    
--*/
{
  UINT64  TempQword;

  AsmDisableCache();
  //
  // Disable Cache MTRR
  //
  *OldMtrr  = AsmReadMsr64 (EFI_CACHE_IA32_MTRR_DEF_TYPE);
  TempQword = (*OldMtrr) &~B_EFI_CACHE_MTRR_VALID &~B_EFI_CACHE_FIXED_MTRR_VALID;
  AsmWriteMsr64 (EFI_CACHE_IA32_MTRR_DEF_TYPE, TempQword);
  return ;
}

VOID
EfiRecoverCacheMtrr (
  IN BOOLEAN                  EnableMtrr,
  IN UINT64                   OldMtrr
  )
/*++
  Routine Description:
  
    Recover cache MTRR
    
  Arguments:
  
    EnableMtrr - Whether to enable the MTRR
    OldMtrr    - The saved old MTRR value to restore when not to 
                 enable the MTRR
                 
  Returns:
  
    None 
--*/
{
  UINT64  TempQword;

  TempQword = 0;
  //
  // Enable Cache MTRR
  //
  if (EnableMtrr) {
    TempQword = AsmReadMsr64 (EFI_CACHE_IA32_MTRR_DEF_TYPE);
    TempQword |= (B_EFI_CACHE_MTRR_VALID | B_EFI_CACHE_FIXED_MTRR_VALID);
  } else {
    TempQword = OldMtrr;
  }
  AsmWriteMsr64 (EFI_CACHE_IA32_MTRR_DEF_TYPE, TempQword);

  AsmEnableCache();
  return ;
}

VOID
EfiProgramMtrr (
  IN PEI_CACHE_PPI             *This,
  IN UINT32                    MtrrNumber,
  IN EFI_PHYSICAL_ADDRESS      MemoryAddress,
  IN UINT64                    MemoryLength,
  IN EFI_MEMORY_CACHE_TYPE     MemoryCacheType,
  IN UINT64                    ValidMtrrAddressMask
  )
/*++

Routine Description:

  Programming MTRR according to Memory address, length, and type.

Arguments:

  MtrrNumber           - the variable MTRR index number
  MemoryAddress        - the address of target memory
  MemoryLength         - the length of target memory
  MemoryCacheType      - the cache type of target memory
  ValidMtrrAddressMask - the MTRR address mask

Returns:

  none

--*/
{
  UINT64  TempQword;

  //
  // MTRR Physical Base
  //
  TempQword = (MemoryAddress & ValidMtrrAddressMask) | MemoryCacheType;
  WriteMsrToBuffer (This, MtrrNumber, TempQword); 

  //
  // MTRR Physical Mask
  //
  TempQword = ~(MemoryLength - 1);
  WriteMsrToBuffer (This, MtrrNumber + 1, (TempQword & ValidMtrrAddressMask) | B_EFI_CACHE_MTRR_VALID);

  return ;
}

UINT64
PeiPower2MaxMemory (
  IN UINT64     MemoryLength
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  MemoryLength  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  UINT64  Result;
  UINT32  *ResultPointer;
  UINT32  *MemoryLengthPointer;
  MemoryLengthPointer = (UINT32*)&MemoryLength;
  ResultPointer = (UINT32*)&Result; 
  Result = 0;
  if (MemoryLengthPointer[1] != 0) {
    ResultPointer[1] = GetPowerOfTwo32 (MemoryLengthPointer[1]);
  } else {
    ResultPointer[0] = GetPowerOfTwo32 (MemoryLengthPointer[0]);
  }

  return Result;
}

EFI_STATUS
PeiProgramUnalignedMtrr (
  IN PEI_CACHE_PPI             *This,
  IN UINT64                    AlignedQword,
  IN UINTN                     MsrNum,
  IN UINT32                    UnalignedBase,
  IN UINT32                    UnalignedLimit
  )
/*++

Routine Description:

  Program the unaligned MTRR register.

Arguments:

  AlignedQword          - The aligned 64-bit cache type.
  MsrNum                - The index of current MTRR.
  UnalignedBase         - Base Address of the current unaligned MTRR.
  UnalignedLimit        - Limit Address of the current unaligned MTRR.

Returns:

  EFI_SUCCESS           - The unaligned MTRR is set successfully.
  EFI_DEVICE_ERROR      - The unaligned address is not the multiple of the basic length of MTRR.

--*/
{
  UINT32      UnalignedOffset;
  UINT64      TempQword;
  UINT64      Mask;
  UINT8       ByteShift;

  UnalignedOffset = UnalignedBase - mFixedMtrrTable[MsrNum].BaseAddress;
  if (UnalignedOffset % mFixedMtrrTable[MsrNum].Length != 0) {
    return EFI_DEVICE_ERROR;
  }
  ByteShift   = (UINT8) (UnalignedOffset / mFixedMtrrTable[MsrNum].Length);
  Mask        = ~(LShiftU64 (1, ByteShift * 8) - 1);

  if (UnalignedLimit < mFixedMtrrTable[MsrNum + 1].BaseAddress) {
    UnalignedOffset = UnalignedLimit - mFixedMtrrTable[MsrNum].BaseAddress;
    if (UnalignedOffset % mFixedMtrrTable[MsrNum].Length!= 0) {
      return EFI_DEVICE_ERROR;
    }
    ByteShift = (UINT8) (UnalignedOffset / mFixedMtrrTable[MsrNum].Length);
    Mask     &= LShiftU64 (1, ByteShift * 8) - 1;
  }
  TempQword = ReadMsrFromBuffer (This, MsrNum) & ~Mask; 
  TempQword  |= AlignedQword & Mask;
  WriteMsrToBuffer (This, MsrNum, TempQword);
  return EFI_SUCCESS;
}

EFI_STATUS
PeiProgramLowMemoryMtrr (
  IN PEI_CACHE_PPI             *This,
  IN EFI_MEMORY_CACHE_TYPE     MemoryCacheType,
  IN UINT32                    MemoryBase,
  IN UINT32                    MemoryLimit
  )
/*++

Routine Description:

  Given the low memory range ( <= 1MB) and cache type, program the MTRRs.

Arguments:

  This                  - Current instance of Pei Cache PPI.
  MemoryCacheType       - Cache Type.
  MemoryBase            - Base Address of Memory to program MTRR.
  MemoryLimit           - Limit Address of Memory to program MTRR.

Returns:

  EFI_SUCCESS           - Low memory MTRR is set successfully.
  others                - An error occurs when setting Low memory MTRR.

--*/
{
  EFI_STATUS  Status;
  ALIGNED     Aligned;
  UINTN       MsrNum;

  Status = EFI_SUCCESS;

  Aligned.AlignedDword.High = MemoryCacheType * ALIGNED_SEED;
  Aligned.AlignedDword.Low  = Aligned.AlignedDword.High;

  for (MsrNum = 0; mFixedMtrrTable[MsrNum].BaseAddress < MemoryBase; MsrNum++) {
    ;
  }
  if (MemoryBase < mFixedMtrrTable[MsrNum].BaseAddress) {
    Status = PeiProgramUnalignedMtrr (This, Aligned.AlignedQword, MsrNum - 1, MemoryBase, MemoryLimit);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }
  while (MsrNum < V_EFI_FIXED_MTRR_NUMBER && MemoryLimit >= mFixedMtrrTable[MsrNum + 1].BaseAddress) {
    //
    // Program aligned MTRR
    //
    WriteMsrToBuffer (This, MsrNum, Aligned.AlignedQword); 
    MsrNum++;
  }
  if (MemoryLimit > mFixedMtrrTable[MsrNum].BaseAddress) {
    Status = PeiProgramUnalignedMtrr (This, Aligned.AlignedQword, MsrNum,  mFixedMtrrTable[MsrNum].BaseAddress, MemoryLimit);
  }

Done:
  return Status;
}

EFI_STATUS
EFIAPI
PeiSetCacheAttributes (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN PEI_CACHE_PPI             *This,
  IN EFI_PHYSICAL_ADDRESS      MemoryAddress,
  IN UINT64                    MemoryLength,
  IN EFI_MEMORY_CACHE_TYPE     MemoryCacheType
  )
/*++

Routine Description:

  Given the memory range and cache type, programs the MTRRs.

Arguments:

  PeiServices           - General purpose services available to every PEIM.
  This                  - Current instance of Pei Cache PPI.
  MemoryAddress         - Base Address of Memory to program MTRR.
  MemoryLength          - Length of Memory to program MTRR.
  MemoryCacheType       - Cache Type.

Returns:

  EFI_SUCCESS           - Mtrr are set successfully.
  EFI_LOAD_ERROR        - No empty MTRRs to use.
  EFI_INVALID_PARAMETER - The input parameter is not valid.
  others                - An error occurs when setting MTTR.

Note:

--*/
{
  EFI_STATUS            Status;
  UINT32                MsrNum;
  UINT64                TempQword;
  UINT32                UsedMsrNum;
  EFI_MEMORY_CACHE_TYPE UsedMemoryCacheType;
  UINT64                ValidMtrrAddressMask;
  EFI_CPUID_REGISTER    FeatureInfo;
  UINT64                Power2Length[8];
  UINT64                LengthArray[8];
  UINTN                 LengthSize;
  UINTN                 Index;
  UINTN                 Count;
  UINT32                Remainder;
  UINT32                VariableMtrrLimit;
  UINT32                *TempQwordPointer;
  UINT32                *Power2LengthPointer;

  TempQwordPointer = (UINT32*)&TempQword;

  VariableMtrrLimit     = (UINT32) (AsmReadMsr64 (EFI_IA32_MTRR_CAP) & B_EFI_IA32_MTRR_VARIABLE_SUPPORT);
  if (VariableMtrrLimit > MAXIMUM_VARIABLE_MTRR_NUMBER) {
    VariableMtrrLimit = MAXIMUM_VARIABLE_MTRR_NUMBER;
  }

  //
  // BIT36
  //
  ValidMtrrAddressMask  = 0x1000000000L;

  AsmCpuid (EFI_CPUID_EXTENDED_FUNCTION, &FeatureInfo.RegEax, &FeatureInfo.RegEbx, &FeatureInfo.RegEcx, &FeatureInfo.RegEdx);
  if (FeatureInfo.RegEax >= EFI_CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (EFI_CPUID_VIR_PHY_ADDRESS_SIZE, &FeatureInfo.RegEax, &FeatureInfo.RegEbx, &FeatureInfo.RegEcx, &FeatureInfo.RegEdx);
    ValidMtrrAddressMask = (LShiftU64((UINT64) 1, FeatureInfo.RegEax & 0xFF) - 1) & (~(UINT64)0x0FFF);
  }

  //
  // Check for invalid parameter
  //
  if ((MemoryAddress & ~ValidMtrrAddressMask) != 0 || (MemoryLength & ~ValidMtrrAddressMask) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  switch (MemoryCacheType) {
  case EFI_CACHE_UNCACHEABLE:
  case EFI_CACHE_WRITECOMBINING:
  case EFI_CACHE_WRITETHROUGH:
  case EFI_CACHE_WRITEPROTECTED:
  case EFI_CACHE_WRITEBACK:
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check if Fixed MTRR
  //
  if ((MemoryAddress + MemoryLength) <= (1 << 20)) {
    Status = PeiProgramLowMemoryMtrr (
               This,
               MemoryCacheType,
               (UINT32) MemoryAddress, 
               (UINT32) (MemoryAddress + MemoryLength)
               );
    return Status;
  }

  //
  // Special case for 1 MB base address
  //
  if (MemoryAddress == 0x100000) {
    MemoryAddress = 0;
    MemoryLength += 0x100000;
  }
  //
  // Split MemoryLength into a sum of power of 2
  //
  ZeroMem (Power2Length, sizeof (Power2Length));
  LengthSize = 0;
  TempQword  = MemoryLength;
  do {
    Power2Length[LengthSize] = PeiPower2MaxMemory (TempQword);
    TempQword -= Power2Length[LengthSize];
    LengthSize++;
  } while (TempQword != 0 && LengthSize < 8);
  if (TempQword != 0) {
    return EFI_LOAD_ERROR;
  }
  //
  // Work out an order of splitted power of 2
  // so that Base and Length are suitable for MTRR
  // setting constraints.
  //
  Count     = 0;
  TempQword = MemoryAddress;
  do {
    for (Index = 0; Index < LengthSize; Index++) {
      Power2LengthPointer = (UINT32*)&Power2Length[Index];
      if (Power2Length[Index] != 0) {
        if (Power2LengthPointer[1] != 0) {
          Remainder = (UINT32) TempQword;
          if (Remainder == 0) {
            DivU64x32Remainder (
              TempQwordPointer[1],
              Power2LengthPointer[1],
              &Remainder
              );
          }
        } else {
          DivU64x32Remainder (TempQword, (UINT32) Power2Length[Index], &Remainder);
        }
        if (Remainder == 0) {
          LengthArray[Count] = Power2Length[Index];
          TempQword += Power2Length[Index];
          Power2Length[Index] = 0;
          Count++;
          break;
        }
      }
    }
    if (Index == LengthSize) {
      return EFI_LOAD_ERROR;
    }
  } while (Count < LengthSize);
  //
  // Begin setting the MTRR according to the order
  //
  for (Index = 0; Index < LengthSize; Index++, MemoryAddress += MemoryLength) {
    MemoryLength = LengthArray[Index];
    //
    // Search if the range attribute has been set before
    //
    Status = SearchForExactMtrr (
               PeiServices,
               This,
               MemoryAddress,
               MemoryLength,
               ValidMtrrAddressMask,
               &UsedMsrNum,
               &UsedMemoryCacheType
               );

    if (!EFI_ERROR (Status)) {
      //
      // Compare if it has the same type as current setting
      //
      if (UsedMemoryCacheType != MemoryCacheType) {
        //
        // Different type
        //
        //
        // Check if the set type is the same as default type
        //
        if (IsDefaultType (MemoryCacheType)) {
          //
          // Clear the mtrr
          //
          WriteMsrToBuffer (This, UsedMsrNum, 0);
          WriteMsrToBuffer (This, UsedMsrNum + 1, 0);

        } else {
          //
          // Modify the mtrr type
          //
          EfiProgramMtrr (
            This,
            UsedMsrNum,
            MemoryAddress,
            MemoryLength,
            MemoryCacheType,
            ValidMtrrAddressMask
            );
        }
      }
      continue;
    }
    //
    // Find first unused MTRR
    //
    for (MsrNum = EFI_CACHE_VARIABLE_MTRR_BASE;
         MsrNum < (EFI_CACHE_VARIABLE_MTRR_BASE + VariableMtrrLimit * 2);
         MsrNum += 2
        ) {
      if (ReadMsrFromBuffer (This, MsrNum + 1) == 0) {
        break;
      }
    }
    //
    // Check if we ran out of variable-range MTRRs
    //
    if (MsrNum >= (EFI_CACHE_VARIABLE_MTRR_BASE + VariableMtrrLimit * 2)) {
      return EFI_LOAD_ERROR;
    }
    EfiProgramMtrr (
      This,
      MsrNum,
      MemoryAddress,
      MemoryLength,
      MemoryCacheType,
      ValidMtrrAddressMask
      );
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeiActivateCache (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN PEI_CACHE_PPI             *This
  )
/*++

Routine Description:

  Update MTRR setting from buffer to MSR.  Disable NEM when NEM is not disabled yet. 

Arguments:

  PeiServices           - General purpose services available to every PEIM.
  This                  - Current instance of Pei Cache PPI.

Returns:

  EFI_SUCCESS           - Mtrr are set successfully.

--*/
{
  UINT32  VariableMtrrLimit;
  UINT32  MsrNum;
  UINT64  OldMtrr;
  UINT16  Index;
  CACHE_PPI_INSTANCE *CachePpiInstance;
  CachePpiInstance = PEI_CACHE_PPI_INSTANCE_FROM_THIS (This);

  VariableMtrrLimit = (UINT32) (AsmReadMsr64 (EFI_IA32_MTRR_CAP) & B_EFI_IA32_MTRR_VARIABLE_SUPPORT);
  if (VariableMtrrLimit > MAXIMUM_VARIABLE_MTRR_NUMBER) {
    VariableMtrrLimit = MAXIMUM_VARIABLE_MTRR_NUMBER;
  }

  //
  // Disable/Enable cache only when MTRR configuration is changed in MTRR buffer
  //
  if (CachePpiInstance->FixedMtrrChanged || CachePpiInstance->VariableMtrrChanged) {
    EfiDisableCacheMtrr (&OldMtrr);
    if (CachePpiInstance->FixedMtrrChanged) {
      for (Index =0; Index < V_EFI_FIXED_MTRR_NUMBER; Index++) {
        if (CachePpiInstance->FixedMtrrValue[Index].Changed) {
          AsmWriteMsr64 (mFixedMtrrTable[Index].Msr, CachePpiInstance->FixedMtrrValue[Index].MsrValue);
          CachePpiInstance->FixedMtrrValue[Index].Changed = FALSE;
        }
      }
      CachePpiInstance->FixedMtrrChanged = FALSE;
    }

    if (CachePpiInstance->VariableMtrrChanged) {
      for (MsrNum = EFI_CACHE_VARIABLE_MTRR_BASE;
           MsrNum < (EFI_CACHE_VARIABLE_MTRR_BASE + VariableMtrrLimit*2);
           MsrNum++
          ) {
        if (CachePpiInstance->VariableMtrrValue[MsrNum-EFI_CACHE_VARIABLE_MTRR_BASE].Changed) {
          AsmWriteMsr64 (MsrNum, CachePpiInstance->VariableMtrrValue[MsrNum-EFI_CACHE_VARIABLE_MTRR_BASE].MsrValue);
          CachePpiInstance->VariableMtrrValue[MsrNum-EFI_CACHE_VARIABLE_MTRR_BASE].Changed = FALSE;
        }
        CachePpiInstance->VariableMtrrChanged = FALSE;
      }
    }
    EfiRecoverCacheMtrr (TRUE, OldMtrr);
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeiResetCacheAttributes (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN PEI_CACHE_PPI             *This
  )
/*++

Routine Description:

    Reset all the MTRRs to a known state.

Arguments:

    PeiServices - General purpose services available to every PEIM.
    This        - Pointer to the instance of the PEI_CACHE_PPI.
    
Returns:

    EFI_SUCCESS - All MTRRs have been reset successfully.

--*/
{
  UINT32  MsrNum;
  UINT16  Index;
  UINT32  VariableMtrrLimit;

  VariableMtrrLimit = (UINT32) (AsmReadMsr64 (EFI_IA32_MTRR_CAP) & B_EFI_IA32_MTRR_VARIABLE_SUPPORT);
  if (VariableMtrrLimit > MAXIMUM_VARIABLE_MTRR_NUMBER) {
    VariableMtrrLimit = MAXIMUM_VARIABLE_MTRR_NUMBER;
  }
  Index = 0;

  //
  // Reset Fixed Mtrrs
  //
  while (mFixedMtrrTable[Index].Msr != 0) {
    WriteMsrToBuffer (This, Index, 0);
    Index++;
  }

  //
  // Reset Variable Mtrrs
  //
  for (MsrNum = EFI_CACHE_VARIABLE_MTRR_BASE;
       MsrNum < (EFI_CACHE_VARIABLE_MTRR_BASE + VariableMtrrLimit * 2);
       MsrNum++
      ) {
    WriteMsrToBuffer (This, MsrNum, 0);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SearchForExactMtrr (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  PEI_CACHE_PPI             *This,
  IN  EFI_PHYSICAL_ADDRESS      MemoryAddress,
  IN  UINT64                    MemoryLength,
  IN  UINT64                    ValidMtrrAddressMask,
  OUT UINT32                    *UsedMsrNum,
  OUT EFI_MEMORY_CACHE_TYPE     *UsedMemoryCacheType
  )
/*++

Routine Description:

  Search the memory cache type for specific memory from MTRR.

Arguments:

  PeiServices          - General purpose services available to every PEIM.
  This                 - Current instance of Pei Cache PPI.
  MemoryAddress        - the address of target memory
  MemoryLength         - the length of target memory
  ValidMtrrAddressMask - the MTRR address mask
  UsedMsrNum           - the used MSR number
  UsedMemoryCacheType  - the cache type for the target memory

Returns:

  EFI_SUCCESS   - The memory is found in MTRR and cache type is returned
  EFI_NOT_FOUND - The memory is not found in MTRR

--*/
{
  UINT32  MsrNum;
  UINT64  TempQword;
  UINT32  VariableMtrrLimit;

  VariableMtrrLimit = (UINT32) (AsmReadMsr64 (EFI_IA32_MTRR_CAP) & B_EFI_IA32_MTRR_VARIABLE_SUPPORT);
  if (VariableMtrrLimit > MAXIMUM_VARIABLE_MTRR_NUMBER) {
    VariableMtrrLimit = MAXIMUM_VARIABLE_MTRR_NUMBER;
  }

  for (MsrNum = EFI_CACHE_VARIABLE_MTRR_BASE;
       MsrNum < (EFI_CACHE_VARIABLE_MTRR_BASE + VariableMtrrLimit * 2 - 1);
       MsrNum += 2
      ) {

    TempQword = ReadMsrFromBuffer (This, MsrNum + 1);

    if ((TempQword & B_EFI_CACHE_MTRR_VALID) == 0) {
      continue;
    }

    if ((TempQword & ValidMtrrAddressMask) != ((~(MemoryLength - 1)) & ValidMtrrAddressMask)) {
      continue;
    }

    TempQword = ReadMsrFromBuffer (This, MsrNum);

    if ((TempQword & ValidMtrrAddressMask) != (MemoryAddress & ValidMtrrAddressMask)) {
      continue;
    }

    *UsedMemoryCacheType  = (EFI_MEMORY_CACHE_TYPE) (TempQword & 0xFF);
    *UsedMsrNum           = MsrNum;

    return EFI_SUCCESS;

  }

  return EFI_NOT_FOUND;
}

BOOLEAN
IsDefaultType (
  IN  EFI_MEMORY_CACHE_TYPE     MemoryCacheType
  )
/*++

Routine Description:

  Compares provided Cache type to default type

Arguments:

  MemoryCacheType - Memory type for testing

Returns:

  TRUE  - Memory type instance is the default type
  FALSE - Memory type instance is not the default type

--*/
{

  if ((AsmReadMsr64(EFI_CACHE_IA32_MTRR_DEF_TYPE) & 0xFF) != MemoryCacheType) {
    return FALSE;
  } else {
    return TRUE;
  }
}

EFI_STATUS 
CpuCachePpiInit (
  VOID
  )
{
  EFI_STATUS          Status;
  CACHE_PPI_INSTANCE  *Instance;

//DEBUG((EFI_D_INFO, "sizeof(CACHE_PPI_INSTANCE)=%d\n", sizeof(CACHE_PPI_INSTANCE)));

  Instance = AllocateZeroPool(sizeof(CACHE_PPI_INSTANCE));
  ASSERT(Instance != NULL);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->Ppi.SetCache      = PeiSetCacheAttributes;
  Instance->Ppi.ResetCache    = PeiResetCacheAttributes;
  Instance->Ppi.ActivateCache = PeiActivateCache;

  Instance->PpiDesc.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  Instance->PpiDesc.Guid  = &gCpuCachePpiGuid;
  Instance->PpiDesc.Ppi   = &Instance->Ppi;

  Status = PeiServicesInstallPpi(&Instance->PpiDesc);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

