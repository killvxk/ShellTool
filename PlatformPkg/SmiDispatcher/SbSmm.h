
#ifndef __PLATFORM_SMM_H__
#define __PLATFORM_SMM_H__

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SmmSxDispatch2.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmPowerButtonDispatch2.h>
#include <Protocol/SmmUsbDispatch2.h>
#include <Protocol/SmmPeriodicTimerDispatch2.h>
#include <Protocol/PciRootBridgeIo.h>
#include <PlatformDefinition.h>


//
// Define an enumeration for all the supported protocols
//
typedef enum {
  UsbType,
  SxType,
  SwType,
  PowerButtonType,
  PeriodicTimerType,
  SbSmmProtocolTypeMax
} SB_SMM_PROTOCOL_TYPE;

//
// /////////////////////////////////////////////////////////////////////////////
// SPECIFYING A REGISTER
// We want a general way of referring to addresses.  For this case, we'll only
// need addresses in the ACPI table (and the TCO entries within the ACPI table).
// However, it's interesting to consider what it would take to support other types
// of addresses.  To address Will's concern, I think it prudent to accommodate it
// early on in the design.
//
// Addresses we need to consider:
//
//  Type:                           Required:
//  I/O                             Yes
//    ACPI (special case of I/O)    Only if we want to
//    TCO  (special case of ACPI)   Only if we want to
//  Memory (or Memory Mapped I/O)   Only if we want to
//  PCIE                            Yes, for BiosWp
//
typedef enum {
  //
  //  IO_ADDR_TYPE, // unimplemented
  //
  ACPI_ADDR_TYPE,
  //
  //  MEMORY_ADDR_TYPE, // unimplemented
  //
  MEMORY_MAPPED_IO_ADDRESS_TYPE,
  PCIE_ADDR_TYPE,
  NUM_ADDR_TYPES,                     // count of items in this enum
  SB_SMM_ADDR_TYPE_NULL        = -1  // sentinel to indicate NULL or to signal end of arrays
} ADDR_TYPE;


typedef UINT16 IO_ADDR;
typedef IO_ADDR ACPI_ADDR;  // can omit
typedef IO_ADDR TCO_ADDR;   // can omit
typedef UINTN MEM_ADDR;
typedef MEM_ADDR *MEMORY_MAPPED_IO_ADDRESS;
typedef union {
  UINT32  Raw;
  struct {
    UINT8 Reg;
    UINT8 Fnc;
    UINT8 Dev;
    UINT8 Bus;
  } Fields;
} PCIE_ADDR;

typedef struct {
  ADDR_TYPE Type;
  union {
    //
    // used to initialize during declaration/definition
    //
    UINT32                    raw;

    //
    // used to access useful data
    //
    IO_ADDR                   io;
    ACPI_ADDR                 acpi;
    TCO_ADDR                  tco;
    MEM_ADDR                  mem;
    MEMORY_MAPPED_IO_ADDRESS  Mmio;
    PCIE_ADDR                 pcie;

  } Data;

} SB_SMM_ADDRESS;


typedef struct {
  SB_SMM_ADDRESS  Reg;
  UINT8           SizeInBytes;  // of the register
  UINT8           Bit;
} SB_SMM_BIT_DESC;


#define IS_BIT_DESC_NULL(BitDesc)   ((BitDesc).Reg.Type == SB_SMM_ADDR_TYPE_NULL)  // "returns" true when BitDesc is NULL
#define NULL_THIS_BIT_DESC(BitDesc) ((BitDesc).Reg.Type = SB_SMM_ADDR_TYPE_NULL)   // will "return" an integer w/ value of 0
#define NULL_BIT_DESC_INITIALIZER \
  { \
    { \
      SB_SMM_ADDR_TYPE_NULL, \
      { \
        0 \
      } \
    }, \
    0, 0 \
  }

#define NUM_EN_BITS   2
#define NUM_STS_BITS  1


typedef UINT8 SB_SMM_SOURCE_FLAGS;


#define SB_SMM_NO_FLAGS          0
#define SB_SMM_SCI_EN_DEPENDENT  1


typedef struct {
  SB_SMM_SOURCE_FLAGS  Flags;
  SB_SMM_BIT_DESC      En[NUM_EN_BITS];
  SB_SMM_BIT_DESC      Sts[NUM_STS_BITS];
} SB_SMM_SOURCE_DESC;

#define NULL_SOURCE_DESC_INITIALIZER \
  { \
    SB_SMM_NO_FLAGS, \
    { \
      NULL_BIT_DESC_INITIALIZER, NULL_BIT_DESC_INITIALIZER \
    }, \
    { \
      NULL_BIT_DESC_INITIALIZER \
    } \
  }


typedef union {
  EFI_SMM_USB_REGISTER_CONTEXT            Usb;
  EFI_SMM_SX_REGISTER_CONTEXT             Sx;
  EFI_SMM_SW_REGISTER_CONTEXT             Sw;
  EFI_SMM_POWER_BUTTON_REGISTER_CONTEXT   PowerButton;
  EFI_SMM_PERIODIC_TIMER_REGISTER_CONTEXT PeriodicTimer;
} SB_SMM_CONTEXT;



typedef union {
  //
  // (in no particular order)
  //
  EFI_SMM_SW_CONTEXT                      Sw;
  EFI_SMM_PERIODIC_TIMER_CONTEXT          PeriodicTimer;
} SB_SMM_BUFFER;

typedef struct _DATABASE_RECORD DATABASE_RECORD;

typedef
VOID
(EFIAPI *GET_CONTEXT) (
  IN  DATABASE_RECORD    *Record,
  OUT SB_SMM_CONTEXT     *Context
  );

typedef
BOOLEAN
(EFIAPI *CMP_CONTEXT) (
  IN SB_SMM_CONTEXT     *Context1,
  IN SB_SMM_CONTEXT     *Context2
  );


//
// This function is used to get the content of CommBuffer that will be passed
// to Callback function
//
typedef
VOID
(EFIAPI *GET_BUFFER) (
  IN  DATABASE_RECORD     * Record
  );

typedef struct {
  GET_CONTEXT GetContext;
  CMP_CONTEXT CmpContext;
  GET_BUFFER  GetBuffer;
} CONTEXT_FUNCTIONS;

extern CONTEXT_FUNCTIONS  ContextFunctions[SbSmmProtocolTypeMax];

VOID
MapUsbToSrcDesc (
  IN  SB_SMM_CONTEXT         *Context,
  OUT SB_SMM_SOURCE_DESC     *SrcDesc
  );

VOID
MapPeriodicTimerToSrcDesc (
  IN  SB_SMM_CONTEXT       *DispatchContext,
  OUT SB_SMM_SOURCE_DESC   *SrcDesc
  );

VOID
SbSmmPeriodicTimerClearSource (
  IN SB_SMM_SOURCE_DESC     *SrcDesc
  );

extern const SB_SMM_SOURCE_DESC  SW_SOURCE_DESC;
extern const SB_SMM_SOURCE_DESC  SX_SOURCE_DESC;
extern const SB_SMM_SOURCE_DESC  POWER_BUTTON_SOURCE_DESC;

#define MIN_SMI_VALUE 0x01
#define MAX_SWI_VALUE 0xFF

//
// /////////////////////////////////////////////////////////////////////////////
// GENERALIZING THE CALLBACK
// All SmmXxxDispatch callbacks have the same form:
//
// VOID Callback( EFI_HANDLE, EFI_SMM_Xxx_DISPATCH_CONTEXT )
// We need a typedef that'll allow us to call any callback
//
typedef
VOID
(EFIAPI *SB_SMM_CALLBACK) (
  IN      EFI_HANDLE        Handle,
  IN      SB_SMM_CONTEXT    *Context,
  IN  OUT VOID              *CommBuffer      OPTIONAL,
  IN  OUT UINTN             *CommBufferSize  OPTIONAL
  );


typedef
VOID
(EFIAPI *SB_SMM_CLEAR_SOURCE) (
  SB_SMM_SOURCE_DESC * SrcDesc
  );

#define DATABASE_RECORD_SIGNATURE SIGNATURE_32 ('D', 'B', 'R', 'C')

typedef struct _DATABASE_RECORD {
  UINT32                Signature;
  LIST_ENTRY            Link;

  //
  // Status and Enable bit description
  //
  SB_SMM_SOURCE_DESC   SrcDesc;

  //
  // Callback function
  //
  SB_SMM_CALLBACK      Callback;
  SB_SMM_CONTEXT       ChildContext;
  SB_SMM_BUFFER        CommBuffer;
  UINTN                SourceSize;

  //
  // Special handling hooks -- init them to NULL if unused/unneeded
  //
  SB_SMM_CLEAR_SOURCE  ClearSource;  // needed for SWSMI timer
  // Functions required to make callback code general
  //
  CONTEXT_FUNCTIONS     ContextFunctions;

  //
  // The protocol that this record dispatches
  //
  SB_SMM_PROTOCOL_TYPE ProtocolType;

} DATABASE_RECORD;

#define DATABASE_RECORD_FROM_LINK(_record)  CR (_record, DATABASE_RECORD, Link, DATABASE_RECORD_SIGNATURE)


typedef
EFI_STATUS
(EFIAPI *SB_SMM_GENERIC_REGISTER) (
  IN  VOID                                    *This,
  IN  VOID                                    *DispatchFunction,
  IN  VOID                                    *DispatchContext,
  OUT EFI_HANDLE                              *DispatchHandle
  );

typedef
EFI_STATUS
(EFIAPI *SB_SMM_GENERIC_UNREGISTER) (
  IN  VOID                                    *This,
  IN  EFI_HANDLE                              DispatchHandle
  );

//
// Define a memory "stamp" equivalent in size and function to most of the protocols
//
typedef struct {
  SB_SMM_GENERIC_REGISTER     Register;
  SB_SMM_GENERIC_UNREGISTER   Unregister;
  UINTN                       Extra1;
} SB_SMM_GENERIC_PROTOCOL;

EFI_STATUS
SbSmmCoreRegister (
  IN  SB_SMM_GENERIC_PROTOCOL                          *This,
  IN  SB_SMM_CALLBACK                                  DispatchFunction,
  IN  SB_SMM_CONTEXT                                   *DispatchContext,
  OUT EFI_HANDLE                                       *DispatchHandle
  );

EFI_STATUS
SbSmmCoreUnRegister (
  IN  SB_SMM_GENERIC_PROTOCOL                         *This,
  IN  EFI_HANDLE                                      *DispatchHandle
  );

typedef union {
  SB_SMM_GENERIC_PROTOCOL                    Generic;
  EFI_SMM_USB_DISPATCH2_PROTOCOL             Usb;	
  EFI_SMM_SX_DISPATCH2_PROTOCOL              Sx;
  EFI_SMM_SW_DISPATCH2_PROTOCOL              Sw;
  EFI_SMM_POWER_BUTTON_DISPATCH2_PROTOCOL    PowerButton;
  EFI_SMM_PERIODIC_TIMER_DISPATCH2_PROTOCOL  PeriodicTimer;
} SB_SMM_PROTOCOL;

//
// Define a structure to help us identify the generic protocol
//
#define PROTOCOL_SIGNATURE  SIGNATURE_32('P', 'R', 'O', 'T')

typedef struct {
  UINTN                 Signature;
  SB_SMM_PROTOCOL_TYPE  Type;
  SB_SMM_PROTOCOL       Protocols;
} SB_SMM_QUALIFIED_PROTOCOL;

#define QUALIFIED_PROTOCOL_FROM_GENERIC(_generic) \
  CR ( \
  _generic, \
  SB_SMM_QUALIFIED_PROTOCOL, \
  Protocols, \
  PROTOCOL_SIGNATURE \
  )

//
// Create private data for the protocols that we'll publish
//
typedef struct {
  LIST_ENTRY                  CallbackDataBase;
  EFI_HANDLE                  SmiHandle;	
  EFI_HANDLE                  ProtocolHandle[SbSmmProtocolTypeMax];
  SB_SMM_QUALIFIED_PROTOCOL   Protocols[SbSmmProtocolTypeMax];
} SB_SMI_PRIVATE_DATA;

extern SB_SMI_PRIVATE_DATA    mPrivateData;

//
// /////////////////////////////////////////////////////////////////////////////
//
extern UINT32                 mAcpiBaseAddr;


//
// /////////////////////////////////////////////////////////////////////////////
//
VOID
EFIAPI
SwGetContext (
  IN  DATABASE_RECORD    *Record,
  OUT SB_SMM_CONTEXT    *Context
  );

BOOLEAN
EFIAPI
SwCmpContext (
  IN SB_SMM_CONTEXT     *Context1,
  IN SB_SMM_CONTEXT     *Context2
  );


VOID
EFIAPI
SwGetBuffer (
  IN  DATABASE_RECORD     * Record
  );

VOID
EFIAPI
SxGetContext (
  IN  DATABASE_RECORD    *Record,
  OUT SB_SMM_CONTEXT    *Context
  );

BOOLEAN
EFIAPI
SxCmpContext (
  IN SB_SMM_CONTEXT     *Context1,
  IN SB_SMM_CONTEXT     *Context2
  );

VOID
EFIAPI
PowerButtonGetContext (
  IN  DATABASE_RECORD   *Record,
  OUT SB_SMM_CONTEXT    *Context
  );

BOOLEAN
EFIAPI
PowerButtonCmpContext (
  IN SB_SMM_CONTEXT     *Context1,
  IN SB_SMM_CONTEXT     *Context2
  );

VOID
SbSmmSxGoToSleep (
  VOID
  );

BOOLEAN
PeriodicTimerCmpContext (
  IN SB_SMM_CONTEXT     *HwContext,
  IN SB_SMM_CONTEXT     *ChildContext
  );

#endif
