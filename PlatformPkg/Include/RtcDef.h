
#ifndef ___RTC_DEF_H__
#define ___RTC_DEF_H__

//------------------------------------------------------------------------------
// [Common]
//   10    - Floppy
//   14    - Bda410 | BIT3
//   15,16 - 640K = 280
//   17,18 - MemorySize
//   2e,2f - checksum(10, 2d)
//   30,31 - MemorySize
//   32    - Century
// 
// [70/71 define]
//   70 ~ 73 MonotonicCounter     (MonotonicCounter.c)
//     

//------------------------------------------------------------------------------
#define PCAT_RTC_ADDRESS_REGISTER     0x70
#define PCAT_RTC_DATA_REGISTER        0x71
#define RTC_ADDRESS_SECONDS           0   // R/W  Range 0..59
#define RTC_ADDRESS_SECONDS_ALARM     1   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES           2   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES_ALARM     3   // R/W  Range 0..59
#define RTC_ADDRESS_HOURS             4   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_HOURS_ALARM       5   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_DAY_OF_THE_WEEK   6   // R/W  Range 1..7
#define RTC_ADDRESS_DAY_OF_THE_MONTH  7   // R/W  Range 1..31
#define RTC_ADDRESS_MONTH             8   // R/W  Range 1..12
#define RTC_ADDRESS_YEAR              9   // R/W  Range 0..99
#define RTC_ADDRESS_REGISTER_A        10  // R/W[0..6]  R0[7]
#define RTC_ADDRESS_REGISTER_B        11  // R/W
#define RTC_ADDRESS_REGISTER_C        12  // RO
#define RTC_ADDRESS_REGISTER_D        13  // RO
#define RTC_ADDRESS_REGISTER_E        14
#define RTC_ADDRESS_DATE_ALARM        0x7D
#define RTC_ADDRESS_MONTH_ALARM       0x7E
#define RTC_ADDRESS_CENTURY           0x7F


#pragma pack(1)

typedef struct {
  UINT8 Rs : 4;   // Rate Selection Bits
  UINT8 Dv : 3;   // Divisor
  UINT8 Uip : 1;  // Update in progress
} RTC_REGISTER_A_BITS;

typedef union {
  RTC_REGISTER_A_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_A;

typedef struct {
  UINT8 Dse : 1;  // 0 - Daylight saving disabled  1 - Daylight savings enabled
  UINT8 Mil : 1;  // 0 - 12 hour mode              1 - 24 hour mode
  UINT8 Dm  : 1;  // 0 - BCD Format                1 - Binary Format
  UINT8 Sqwe: 1;  // 0 - Disable SQWE output       1 - Enable SQWE output
  UINT8 Uie : 1;  // 0 - Update INT disabled       1 - Update INT enabled
  UINT8 Aie : 1;  // 0 - Alarm INT disabled        1 - Alarm INT Enabled
  UINT8 Pie : 1;  // 0 - Periodic INT disabled     1 - Periodic INT Enabled
  UINT8 Set : 1;  // 0 - Normal operation.         1 - Updates inhibited
} RTC_REGISTER_B_BITS;

typedef union {
  RTC_REGISTER_B_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_B;

typedef struct {
  UINT8 Reserved : 7; // Read as zero.  Can not be written.
  UINT8 Vrt : 1;      // Valid RAM and Time
} RTC_REGISTER_D_BITS;

typedef union {
  RTC_REGISTER_D_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_D;

#pragma pack()

#endif



