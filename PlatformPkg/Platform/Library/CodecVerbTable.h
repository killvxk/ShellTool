//; Module Name :
//;   OemVerbTable.h
//; Abstract :
//;   VerbTable definition for OEM

#ifndef _OEM_VERB_TABLE_H_
#define _OEM_VERB_TABLE_H_

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sample :                                                                                                      //
//                                                                                                               //
//   #define OEM_VERB_TABLE_ID_(n)         n                                                                     //
//                                                                                                               //
//   #define OEM_VERB_TABLE_(n)_HEADER(a)  0x10EC0880, // Vendor ID / Device ID                                  //
//                                         0x00000000, // SubSystem ID                                           //
//                                         0x02,       // RevisionId                                             //
//                                         0x01,       // Front panel support ( 1 = Yes, 2 = No )                //
//                                         0x000A,     // Number of Rear Jacks = 10                              //
//                                         0x0002      // Number of Front Jacks = 2                              //
//                                                                                                               //
//   #define OEM_VERB_TABLE_(n)_DATA(a)    0x01171CF0,0x01171D11,0x01171E11,0x01171F41, // NID(0x11): 0x411111F0 //
//                                         0x01271C40,0x01271D09,0x01271EA3,0x01271F99, // NID(0x12): 0x99A30940 //
//                                                                                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// To define Verb Table ID.
//


//
// To define verb table here.
//
//alex20131203b,s
//
// Realtek HD Audio Codec : ALC892
//
//   OEM_VERB_TABLE_HEADER0        0x10EC0892, // Vendor ID / Device ID
//                                 0x00000000, // SubSystem ID
//                                 0x02,       // RevisionId
//                                 0x01,       // Front panel support ( 1 = Yes, 2 = No )
//                                 0x000A,     // Number of Rear Jacks = 10
//                                 0x0002      // Number of Front Jacks = 2

//
//  0446 Chipset1802P_PinConfig_TBL
//
#define OEM_VERB_TABLE_HEADER0       0x10EC0892, \
                                     0x00000000, \
                                     0xFF,       \
                                     0x01,       \
                                     0x000A,     \
                                     0x0002

#define OEM_VERB_TABLE_DATA0         0x0017208E,0x00172136,0x001722AA,0x00172317,\
                                     0x0017FF00,0x0017FF00,0x0017FF00,0x0017FF00,\
                                     0x01171C00,0x01171D00,0x01171E00,0x01171F40,\
                                     0x01271CF0,0x01271D11,0x01271E11,0x01271F41,\
                                     0x01471C10,0x01471D40,0x01471E01,0x01471F01,\
                                     0x01571C12,0x01571D10,0x01571E01,0x01571F01,\
                                     0x01671C11,0x01671D60,0x01671E01,0x01671F01,\
                                     0x01771CF0,0x01771D11,0x01771E11,0x01771F41,\
                                     0x01871C40,0x01871D90,0x01871EA1,0x01871F01,\
                                     0x01971C50,0x01971D90,0x01971EA1,0x01971F02,\
                                     0x01A71C4F,0x01A71D30,0x01A71E81,0x01A71F01,\
                                     0x01b71C20,0x01b71D40,0x01b71E21,0x01b71F02,\
                                     0x01C71CF0,0x01C71D11,0x01C71E11,0x01C71F41,\
                                     0x01D71C29,0x01D71DE6,0x01D71E25,0x01D71F40,\
                                     0x01E71C30,0x01E71D21,0x01E71E45,0x01E71F01,\
                                     0x01F71CF0,0x01F71D11,0x01F71E11,0x01F71F41,\
                                     0x02050007,0x020409C8,0x02050007,0x020409C8
//alex20131203b,e

//
//  0446 Chipset1802P_PinConfig_TBL
//
#define OEM_VERB_TABLE_HEADER1       0x11060446, \
                                     0x00000000, \
                                     0xFF,       \
                                     0x01,       \
                                     0x000D,     \
                                     0x0000
                                   
#define OEM_VERB_TABLE_DATA1         0x0017FF00,0x0017FF00,0x001F93C3,0x001FA270,\
                                     0x001FE003,0x001FE390,0x001FF039,0x001FA158,\
                                     0x001FD90B,0x001FDB20,0x001FB02B,0x001FF100,\
                                     0x00172000,0x00172100,0x00172206,0x00172311,\
                                     0x02471C10,0x02471D01,0x02471E17,0x02471F90,\
                                     0x02571C1F,0x02571D40,0x02571E21,0x02571F02,\
                                     0x02871CF0,0x02871D40,0x02871E21,0x02871F42,\
                                     0x02971C37,0x02971D90,0x02971EA1,0x02971F02,\
                                     0x02A71C38,0x02A71D30,0x02A71E81,0x02A71F01,\
                                     0x02B71C3E,0x02B71D90,0x02B71EA1,0x02B71F01,\
                                     0x02D71CF0,0x02D71D11,0x02D71E44,0x02D71F07,\
                                     0x03071CF0,0x03071D01,0x03071EA6,0x03071F50,\
                                     0x03371CF0,0x03371D01,0x03371E17,0x03371F50

//
//  4446 Chipset1802P_PinConfig_TBL
//
#define OEM_VERB_TABLE_HEADER2       0x11064446, \
                                     0x00000000, \
                                     0xFF,       \
                                     0x01,       \
                                     0x000D,     \
                                     0x0000
                                   
#define OEM_VERB_TABLE_DATA2         0x0017FF00,0x0017FF00,0x001F93C3,0x001FA270,\
                                     0x001FE003,0x001FE390,0x001FF039,0x001FA158,\
                                     0x001FD90B,0x001FDB20,0x001FB02B,0x001FF100,\
                                     0x00172000,0x00172100,0x00172206,0x00172311,\
                                     0x02471C10,0x02471D01,0x02471E17,0x02471F90,\
                                     0x02571C1F,0x02571D40,0x02571E21,0x02571F02,\
                                     0x02871CF0,0x02871D40,0x02871E21,0x02871F42,\
                                     0x02971C37,0x02971D90,0x02971EA1,0x02971F02,\
                                     0x02A71C38,0x02A71D30,0x02A71E81,0x02A71F01,\
                                     0x02B71C3E,0x02B71D90,0x02B71EA1,0x02B71F01,\
                                     0x02D71CF0,0x02D71D11,0x02D71E44,0x02D71F07,\
                                     0x03071CF0,0x03071D01,0x03071EA6,0x03071F50,\
                                     0x03371CF0,0x03371D01,0x03371E17,0x03371F50

//
//  8446 Chipset1802P_PinConfig_TBL
//
#define OEM_VERB_TABLE_HEADER3       0x11068446, \
                                     0x00000000, \
                                     0xFF,       \
                                     0x01,       \
                                     0x000D,     \
                                     0x0000
                                   
#define OEM_VERB_TABLE_DATA3         0x0017FF00,0x0017FF00,0x001F93C3,0x001FA270,\
                                     0x001FE003,0x001FE390,0x001FF039,0x001FA158,\
                                     0x001FD90B,0x001FDB20,0x001FB02B,0x001FF100,\
                                     0x00172000,0x00172100,0x00172206,0x00172311,\
                                     0x02471C10,0x02471D01,0x02471E17,0x02471F90,\
                                     0x02571C1F,0x02571D40,0x02571E21,0x02571F02,\
                                     0x02871CF0,0x02871D40,0x02871E21,0x02871F42,\
                                     0x02971C37,0x02971D90,0x02971EA1,0x02971F02,\
                                     0x02A71C38,0x02A71D30,0x02A71E81,0x02A71F01,\
                                     0x02B71C3E,0x02B71D90,0x02B71EA1,0x02B71F01,\
                                     0x02D71CF0,0x02D71D11,0x02D71E44,0x02D71F07,\
                                     0x03071CF0,0x03071D01,0x03071EA6,0x03071F50,\
                                     0x03371CF0,0x03371D01,0x03371E17,0x03371F50

//
//  C446 Chipset1802P_PinConfig_TBL
//
#define OEM_VERB_TABLE_HEADER4       0x1106C446, \
                                     0x00000000, \
                                     0xFF,       \
                                     0x01,       \
                                     0x000D,     \
                                     0x0000
                                   
#define OEM_VERB_TABLE_DATA4         0x0017FF00,0x0017FF00,0x001F93C3,0x001FA270,\
                                     0x001FE003,0x001FE390,0x001FF039,0x001FA158,\
                                     0x001FD90B,0x001FDB20,0x001FB02B,0x001FF100,\
                                     0x00172000,0x00172100,0x00172206,0x00172311,\
                                     0x02471C10,0x02471D01,0x02471E17,0x02471F90,\
                                     0x02571C1F,0x02571D40,0x02571E21,0x02571F02,\
                                     0x02871CF0,0x02871D40,0x02871E21,0x02871F42,\
                                     0x02971C37,0x02971D90,0x02971EA1,0x02971F02,\
                                     0x02A71C38,0x02A71D30,0x02A71E81,0x02A71F01,\
                                     0x02B71C3E,0x02B71D90,0x02B71EA1,0x02B71F01,\
                                     0x02D71CF0,0x02D71D11,0x02D71E44,0x02D71F07,\
                                     0x03071CF0,0x03071D01,0x03071EA6,0x03071F50,\
                                     0x03371CF0,0x03371D01,0x03371E17,0x03371F50

//
//  Chipset1828S_PinConfig_TBL for Chipset8614A
//
#define OEM_VERB_TABLE_HEADER5       0x11064441, \
                                     0x00000000, \
                                     0xFF,       \
                                     0x01,       \
                                     0x000F,     \
                                     0x0000
                                   
#define OEM_VERB_TABLE_DATA5         0x0017FF00,0x0017FF00,0x001F8200,0x001FBA37,\
                                     0x02470500,0x0243B000,0x02470740,0x00000000,\
                                     0x00172000,0x00172100,0x00172206,0x00172311,\
                                     0x02471C10,0x02471D40,0x02471E01,0x02471F01,\
                                     0x02571C12,0x02571D10,0x02571E01,0x02571F01,\
                                     0x02671C11,0x02671D60,0x02671E01,0x02671F01,\
                                     0x02771C14,0x02771D20,0x02771E01,0x02771F01,\
                                     0x02871C1F,0x02871D40,0x02871E21,0x02871F02,\
                                     0x02971C27,0x02971D90,0x02971EA1,0x02971F02,\
                                     0x02A71C2E,0x02A71D30,0x02A71E81,0x02A71F01,\
                                     0x02B71C26,0x02B71D90,0x02B71EA1,0x02B71F01,\
                                     0x02C71CF8,0x02C71D11,0x02C71E33,0x02C71F59,\
                                     0x02D71CF0,0x02D71D11,0x02D71E44,0x02D71F07,\
                                     0x02E71CF0,0x02E71D00,0x02E71E56,0x02E71F18,\
                                     0x02F71CF0,0x02F71D21,0x02F71EC4,0x02F71F07

//[-end-111111-IB09900063-modify]//


//
//  Chipset2021_PinConfig_TBL
//
#define OEM_VERB_TABLE_HEADER6       0x11060441, \
                                     0x00000000, \
                                     0xFF,       \
                                     0x01,       \
                                     0x000F,     \
                                     0x0000
                                   
#define OEM_VERB_TABLE_DATA6  \
  0x0017FF00, 0x0017FF00, 0x001F8201, 0x00172000, \
  0x00172100, 0x00172206, 0x00172311,             \
  0x02471CF0, 0x02471D40, 0x02471E01, 0x02471F01, \
  0x02571CF0, 0x02571D10, 0x02571E01, 0x02571F41, \
  0x02671CF0, 0x02671D60, 0x02671E01, 0x02671F41, \
  0x02771CF0, 0x02771D20, 0x02771E01, 0x02771F41, \
  0x02871CF0, 0x02871D40, 0x02871E21, 0x02871F02, \
  0x02971CF0, 0x02971D90, 0x02971EA1, 0x02971F02, \
  0x02A71C2E, 0x02A71D30, 0x02A71E81, 0x02A71F01, \
  0x02B71C26, 0x02B71D90, 0x02B71EA1, 0x02B71F01, \
  0x02C71CF0, 0x02C71D01, 0x02C71E37, 0x02C71F50, \
  0x02D71CF0, 0x02D71D11, 0x02D71E43, 0x02D71F07, \
  0x02E71CF0, 0x02E71D11, 0x02E71E43, 0x02E71F07, \
  0x02F71CF0, 0x02F71D11, 0x02F71EC3, 0x02F71F07, \
  0xFFFFFFFF

////  Chipset1705CF
#define OEM_VERB_TABLE_HEADER7       0x11064760, \
                                     0x00000000, \
                                     0xFF,       \
                                     0x01,       \
                                     0x000F,     \
                                     0x0000

#define OEM_VERB_TABLE_DATA7  \
0x0017FF00, 0x0017FF00, 0x001F9114, \
0x00172000, 0x00172100, 0x00172206, 0x00172311, \
0x02471CF0, 0x02471D40, 0x02471E11, 0x02471F01, \
0x02571CF0, 0x02571D10, 0x02571E01, 0x02571F41, \
0x02671CF0, 0x02671D60, 0x02671E01, 0x02671F41, \
0x02771CF0, 0x02771D20, 0x02771E01, 0x02771F41, \
0x02871CF0, 0x02871D40, 0x02871E21, 0x02871F02, \
0x02971C20, 0x02971D90, 0x02971EA1, 0x02971F02, \
0x02A71C2E, 0x02A71D30, 0x02A71E81, 0x02A71F01, \
0x02B71C26, 0x02B71D90, 0x02B71EA1, 0x02B71F01, \
0x02C71CF0, 0x02C71D11, 0x02C71E37, 0x02C71F07, \
0x02D71CF0, 0x02D71D11, 0x02D71E44, 0x02D71F07, \
0x02E71CF0, 0x02E71D11, 0x02E71E44, 0x02E71F07, \
0x02F71CF0, 0x02F71D21, 0x02F71EC4, 0x02F71F47, \
0x03071CF0, 0x03071D01, 0x03071EA6, 0x03071F50, \
0xFFFFFFFF

//HYL-2016101801-start
////  Realtek ALC662
#define OEM_VERB_TABLE_HEADER8       0x10EC0662, \
                                     0x00000000, \
                                     0xFF,       \
                                     0x01,       \
                                     0x000F,     \
                                     0x0000

#define OEM_VERB_TABLE_DATA8  \
0x00172000, 0x00172100, 0x00172217, 0x0017231D, \
0x0017FF00, 0x0017FF00, 0x0017FF00, 0x0017FF00, \
0x01271C00, 0x01271D00, 0x01271E13, 0x01271F40, \
0x01471C10, 0x01471D40, 0x01471E01, 0x01471F01, \
0x01571CF0, 0x01571D11, 0x01571E11, 0x01571F41, \
0x01671CF0, 0x01671D11, 0x01671E11, 0x01671F41, \
0x01871C40, 0x01871D90, 0x01871EA1, 0x01871F01, \
0x01971C50, 0x01971D90, 0x01971EA1, 0x01971F02, \
0x01A71C4F, 0x01A71D30, 0x01A71E81, 0x01A71F01, \
0x01B71C20, 0x01B71D40, 0x01B71E21, 0x01B71F02, \
0x01C71CF0, 0x01C71D01, 0x01C71E33, 0x01C71F59, \
0x01D71C01, 0x01D71DC6, 0x01D71E44, 0x01D71F40, \
0x01E71C30, 0x01E71D11, 0x01E71E44, 0x01E71F01, \
0x02050004, 0x02040001, 0x02050004, 0x02040001, \
0x02050005, 0x02040080, 0x02050001, 0x0204A9B8, \
0xFFFFFFFF
//HYL-2016101801-end

#endif
