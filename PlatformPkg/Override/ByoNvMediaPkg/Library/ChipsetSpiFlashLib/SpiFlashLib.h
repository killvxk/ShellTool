/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  PchSpiFlashLib.h

Abstract: for SST25/26 series

Revision History:

**/
#ifndef _PCH_SPI_FLASH_LIB_H_
#define _PCH_SPI_FLASH_LIB_H_

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <Library/Include/SpiFlashLib.h>

//
// SST25 Serial Flash Status Register definitions
//
#define SF_SR_BUSY        0x01      // Indicates if internal write operation is in progress
#define SF_SR_WEL         0x02      // Indicates if device is memory write enabled
#define SF_SR_BP0         0x04      // Block protection bit 0
#define SF_SR_BP1         0x08      // Block protection bit 1
#define SF_SR_BP2         0x10      // Block protection bit 2
#define SF_SR_BP3         0x20      // Block protection bit 3
#define SF_SR_AAI         0x40      // Auto Address Increment Programming status
#define SF_SR_BPL         0x80      // Block protection lock-down

#define SF_SR_WPE         0x3C      // Enable write protection on all blocks for SST25 

//
// SST26 Serial Flash Status Register definitions
//
// #define SF_SR_BUSY        0x01      // Indicates if internal write operation is in progress
// #define SF_SR_WEL         0x02      // Indicates if device is memory write enabled
#define SF_SR_WSE         0x04      // Write Suspend-Erase status
#define SF_SR_WSP         0x08      // Write Suspend-Program status
#define SF_SR_WPLD        0x10      // Indicates if the Block-Protection register is locked-down
#define SF_SR_SEC         0x20      // Security ID status
                                    // bit 6 resevered
                                    // bit 7 Busy status

//
// SST26 Serial Flash configuration Register definitions
//
#define SF_CF_IOC         0x02      // disable the WP# and HOLD# pin
#define SF_CF_BPNV        0x08      // indicates whether any block has been permanently locked with the nVWLDR
#define SF_CF_WPEN        0x80      // enable the WP# pin for the hardware write-protection function
                                    // others are reserved
                                    
//
// SST26 Serial Flash Block Protection Register Write-protected Value (defaule value) 
//
// BPR[143:0] = 5555 FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
#define SF_BPR_WPV0        0xFFFFFFFF
#define SF_BPR_WPV1        0xFFFFFFFF
#define SF_BPR_WPV2        0xFFFFFFFF
#define SF_BPR_WPV3        0xFFFFFFFF
#define SF_BPR_WPV5        0x00005555


//
// General Operation Instruction definitions for the Serial Flash Device
// only for SPI_INIT_TABLE
//
#define SF_INST_NOP             0x00     // No operation
#define SF_INST_WRSR            0x01     // Write Status Register£¨configuration register£©
#define SF_INST_PROG            0x02     // Byte Program    
#define SF_INST_READ            0x03     // Read
#define SF_INST_WRDI            0x04     // Write Disable
#define SF_INST_RDSR            0x05     // Read Status Register
#define SF_INST_WREN            0x06     // Write Enable
#define SF_INST_HS_READ         0x0B     // High-speed Read 
#define SF_INST_SERASE          0x20     // Sector Erase (4KB)
#define SF_INST_64KB_ERASE      0xD8     // Block Erase (64KB for SST25 or 8KB/32KB/64KB depending on addressfor SST26) 
#define SF_INST_CERASE          0xC7     // Chip Erase

#define SF_INST_JEDEC_READ_ID   0x9F     // JEDEC Read ID

// for SST25 series only

#define SF_INST_EWSR            0x50     // Enable Write Status Register      
#define SF_INST_READ_ID         0xAB     // Read ID
#define SF_INST_BERASE          0x52     // block Erase(32K)

// for SST26 series only 

#define SF_INST_RDCR            0x35     // Read Configuration register
#define SF_INST_RBPR            0x72     // Read Block-protection register
#define SF_INST_WBPR            0x42     // Write Block-protection register
#define SF_INST_LBPR            0x8D     // lock-down Block-protection register
#define SF_INST_ULBPR           0x98     // Global Block-protection register
//
// Prefix Opcode Index on the host SPI controller
//
typedef enum {
    SPI_WREN = 0,             // Prefix Opcode 0: Write Enable
    SPI_EWSR = 1,             // Prefix Opcode 1: Enable Write Status Register
    SPI_NOP  = 1,             // Prefix Opcode 1: No Operation
} PREFIX_OPCODE_INDEX;

//
// Opcode Menu Index on the host SPI controller
//
typedef enum {
    SPI_READ_ID = 0,           // Opcode 0: READ ID, Read cycle with address
    SPI_READ    = 1,           // Opcode 1: READ, Read cycle with address
    SPI_RDSR    = 2,           // Opcode 2: Read Status Register, No address
    SPI_WRDI    = 3,           // Opcode 3: Write Disable, No address
    SPI_RBPR    = 3,           // Opcode 3: Read Block Protection Register,No address
    SPI_SERASE  = 4,           // Opcode 4: Sector Erase (4KB), Write cycle with address
    SPI_BERASE  = 5,           // Opcode 5: Block Erase (32KB), Write cycle with address
    SPI_PROG    = 6,           // Opcode 6: Page Program, Write cycle with address
    SPI_WRSR    = 7,           // Opcode 7: Write Status Register, No address
    SPI_ULBPR   = 7,           // Opcode 7: Global block protection unlock,No address
} SPI_OPCODE_INDEX;

#endif  // _PCH_SPI_FLASH_LIB_H_
