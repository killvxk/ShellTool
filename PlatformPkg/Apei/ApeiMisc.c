

#include <IndustryStandard/Acpi.h>
#include "Apei.h"


#define GAS_WORD_IO { \
  EFI_ACPI_5_0_SYSTEM_IO,     \
  8,                          \
  0,                          \
  EFI_ACPI_5_0_WORD,          \
  0}

#define GAS_QWORD_MEM  { \
  EFI_ACPI_5_0_SYSTEM_MEMORY, \
  64,                         \
  0,                          \
  EFI_ACPI_5_0_QWORD,         \
  0}
  
  
  
EFI_ACPI_5_0_ERST_SERIALIZATION_INSTRUCTION_ENTRY gSerialInsEntry[16] = {
    {
      EFI_ACPI_5_0_ERST_BEGIN_WRITE_OPERATION,                 // Serialization Action - 0
      EFI_ACPI_5_0_ERST_WRITE_REGISTER_VALUE,             // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      ERST_ERROR_SET_RECORD,                             // Value for SetErrorRecord()
      0x0000FFFF                                    // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_BEGIN_READ_OPERATION,                  // Serialization Action - 1
      EFI_ACPI_5_0_ERST_WRITE_REGISTER_VALUE,             // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      ERST_ERROR_GET_RECORD,                             // Value for GetErrorRecord()
      0x0000FFFF                                    // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_BEGIN_CLEAR_OPERATION,                 // Serialization Action - 2
      EFI_ACPI_5_0_ERST_WRITE_REGISTER_VALUE,             // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      ERST_ERROR_CLEAR_RECORD,                           // Value for SetErrorRecord()
      0x0000FFFF                                    // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_END_OPERATION,                         // Serialization Action - 3
      EFI_ACPI_5_0_ERST_NOOP,                     // Instruction
      EFI_ACPI_5_0_ERST_PRESERVE_REGISTER,                       // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      ERST_ERROR_END_OP,                            // Value for SetErrorRecord()
      0x0000FFFF                                    // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_SET_RECORD_OFFSET,                     // Serialization Action - 4
      EFI_ACPI_5_0_ERST_WRITE_REGISTER,                   // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      0,                                            // Value for SetErrorRecord()
      0xFFFFFFFF                                    // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_EXECUTE_OPERATION,                     // Serialization Action - 5
      EFI_ACPI_5_0_ERST_WRITE_REGISTER_VALUE,             // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_WORD_IO,                          // GAS (QWORD Memory) Address will be filled during boot.
      ERST_ERROR_RECORD_SWSMI,                      // Value field provides the data to write to SMI port 0xb2
      0x0000FFFF                                    // Mask  16-bit
    },
    {
      EFI_ACPI_5_0_ERST_CHECK_BUSY_STATUS,                     // Serialization Action - 6
      EFI_ACPI_5_0_ERST_READ_REGISTER_VALUE,                // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      0x00000001,                                            // Value for SetErrorRecord()
      0x00000001                                    // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_GET_COMMAND_STATUS,                    // Serialization Action - 7
      EFI_ACPI_5_0_ERST_READ_REGISTER,                    // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      0,                                            // Value for SetErrorRecord()
      0xffffffff                                    // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_GET_RECORD_IDENTIFIER,                 // Serialization Action - 8
      EFI_ACPI_5_0_ERST_READ_REGISTER,                    // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      0,                                            // Value for SetErrorRecord()
      0xFFFFFFFFFFFFFFFF                            // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_SET_RECORD_IDENTIFIER,                 // Serialization Action - 9
      EFI_ACPI_5_0_ERST_WRITE_REGISTER,                   // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      0,                                            // Value for SetErrorRecord()
      0xFFFFFFFFFFFFFFFF                            // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_GET_RECORD_COUNT,                      // Serialization Action - 10
      EFI_ACPI_5_0_ERST_READ_REGISTER,                    // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      0,                                            // Value for SetErrorRecord()
      0xFFFFFFFF                                    // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_BEGIN_DUMMY_WRITE_OPERATION,           // Serialization Action - 11
      EFI_ACPI_5_0_ERST_WRITE_REGISTER_VALUE,             // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      ERST_ERROR_DUMMY_WRITE,                       // Value for Dummy write()
      0xFFFF                                        // Mask is only valid for 32-bits 
    },
    {
      0xFF,                                         // Serialization Action - 12, reserved
      0xFF,                                         // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      0,                                            // Value for SetErrorRecord()
      0xFFFFFFFF                                    // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_GET_ERROR_LOG_ADDRESS_RANGE,           // Serialization Action - 13
      EFI_ACPI_5_0_ERST_READ_REGISTER,                    // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      0,                                            // Value for SetErrorRecord()
      0xFFFFFFFFFFFFFFFF                            // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_GET_ERROR_LOG_ADDRESS_RANGE_LENGTH,    // Serialization Action - 14
      EFI_ACPI_5_0_ERST_READ_REGISTER,                    // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      0,                                            // Value for SetErrorRecord()
      0xFFFFFFFFFFFFFFFF                            // Mask is only valid for 32-bits 
    },
    {
      EFI_ACPI_5_0_ERST_GET_ERROR_LOG_ADDRESS_RANGE_ATTRIBUTES,// Serialization Action - 15
      EFI_ACPI_5_0_ERST_READ_REGISTER,                    // Instruction
      0,                                 // Flags that qualify the instruction
      0x00,                                         // Reserved
      GAS_QWORD_MEM,                           // GAS (QWORD Memory) Address will be filled during boot.
      0,                                            // Value for SetErrorRecord()
      0xFFFFFFFFFFFFFFFF                            // Mask is only valid for 32-bits 
    }
};

UINTN gSerialInsEntrySize = sizeof(gSerialInsEntry);

  