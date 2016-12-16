

#include <Library/PlatformCommLib.h>
#include "CodecVerbTable.h"


#define CODEC_10EC0892_SUPPORT    0
#define CODEC_11060446_SUPPORT    0
#define CODEC_11064446_SUPPORT    0
#define CODEC_11068446_SUPPORT    0
#define CODEC_1106C446_SUPPORT    0
#define CODEC_11064441_SUPPORT    0
#define CODEC_11060441_SUPPORT    0
//HYL-2016101801 #define CODEC_11064760_SUPPORT    1
#define CODEC_11064760_SUPPORT    0		//HYL-2016101801
#define CODEC_10EC0662_SUPPORT    1		//HYL-2016101801


#if CODEC_10EC0892_SUPPORT
STATIC UINT32 gVerbTable0Data[] = {OEM_VERB_TABLE_DATA0};
#endif

#if CODEC_11060446_SUPPORT
STATIC UINT32 gVerbTable1Data[] = {OEM_VERB_TABLE_DATA1};
#endif

#if CODEC_11064446_SUPPORT
STATIC UINT32 gVerbTable2Data[] = {OEM_VERB_TABLE_DATA2};
#endif

#if CODEC_11068446_SUPPORT
STATIC UINT32 gVerbTable3Data[] = {OEM_VERB_TABLE_DATA3};
#endif

#if CODEC_1106C446_SUPPORT
STATIC UINT32 gVerbTable4Data[] = {OEM_VERB_TABLE_DATA4};
#endif

#if CODEC_11064441_SUPPORT
STATIC UINT32 gVerbTable5Data[] = {OEM_VERB_TABLE_DATA5};
#endif

#if CODEC_11060441_SUPPORT
STATIC UINT32 gVerbTable6Data[] = {OEM_VERB_TABLE_DATA6};
#endif

#if CODEC_11064760_SUPPORT
STATIC UINT32 gVerbTable7Data[] = {OEM_VERB_TABLE_DATA7}; // Chipset1705CF
#endif

//HYL-2016101801-start
#if CODEC_10EC0662_SUPPORT
STATIC UINT32 gVerbTable8Data[] = {OEM_VERB_TABLE_DATA8}; //Realtek ALC662
#endif
//HYL-2016101801-end

OEM_VERB_TABLE  gOemVerbTable[] = {

#if CODEC_10EC0892_SUPPORT
  {
    OEM_VERB_TABLE_HEADER0,
    sizeof(gVerbTable0Data),
    gVerbTable0Data
  },
#endif

#if CODEC_11060446_SUPPORT
  {
    OEM_VERB_TABLE_HEADER1,
    sizeof(gVerbTable1Data),
    gVerbTable1Data
  },
#endif

#if CODEC_11064446_SUPPORT		
  {
    OEM_VERB_TABLE_HEADER2,
    sizeof(gVerbTable2Data),
    gVerbTable2Data
  },
#endif

#if CODEC_11068446_SUPPORT		
  {
    OEM_VERB_TABLE_HEADER3,
    sizeof(gVerbTable3Data),
    gVerbTable3Data
  },
#endif

#if CODEC_1106C446_SUPPORT
  {
    OEM_VERB_TABLE_HEADER4,
    sizeof(gVerbTable4Data),
    gVerbTable4Data
  },
#endif

#if CODEC_11064441_SUPPORT		
  {
    OEM_VERB_TABLE_HEADER5,
    sizeof(gVerbTable5Data),
    gVerbTable5Data
  },
#endif

#if CODEC_11060441_SUPPORT
  {
    OEM_VERB_TABLE_HEADER6,
    sizeof(gVerbTable6Data),
    gVerbTable6Data
  },
#endif

#if CODEC_11064760_SUPPORT
  {
    OEM_VERB_TABLE_HEADER7,
    sizeof(gVerbTable7Data),
    gVerbTable7Data
  } 
#endif

//HYL-2016101801-start
#if CODEC_10EC0662_SUPPORT
  {
    OEM_VERB_TABLE_HEADER8,
    sizeof(gVerbTable8Data),
    gVerbTable8Data
  } 
#endif
//HYL-2016101801-end

};

UINTN gOemVerbTableSize = sizeof(gOemVerbTable);


