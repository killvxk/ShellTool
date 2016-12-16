
#ifndef __PLAT_CMOS_H__
#define __PLAT_CMOS_H__

//------------------------------------------------------------------------------
// [Common]
//   10    - Floppy
//   14    - Bda410 | BIT3
//   15,16 - 640K = 280
//   17,18 - MemorySize
//   2e,2f - checksum(10, 2d)
//   30,31 - MemorySize
//   32    - Century

#define MTC_CMOS_OFFSET            0x70
#define MTC_CMOS_LENGTH            4

#endif
