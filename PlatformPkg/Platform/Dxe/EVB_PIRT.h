
#ifndef __EVB_PIRT_H__
#define __EVB_PIRT_H__

#include <Protocol/LegacyBiosPlatform.h>
#include "PlatformDxe.h"


#define FIXED_ENTRY_NUM 11

//LPC[45].[7:4]  INTH#
//LPC[45].[3:0]  INTG#
//LPC[44].[7:4]  INTF#
//LPC[44].[3:0]  INTE#
//LPC[55].[7:4]  INTA#
//LPC[56].[7:4]  INTC#
//LPC[56].[3:0]  INTB#
//LPC[57].[7:4]  INTD#

//YKN_20160222 Modify -s
/*
#define PIRQA_LINK         0x55
#define PIRQB_LINK         0x56
#define PIRQC_LINK         0x56
#define PIRQD_LINK         0x57
#define PIRQE_LINK         0x44
#define PIRQF_LINK         0x44
#define PIRQG_LINK         0x45
#define PIRQH_LINK         0x45
*/
#define PIRQA_LINK         1
#define PIRQB_LINK         2
#define PIRQC_LINK         3
#define PIRQD_LINK         4
#define PIRQE_LINK         5
#define PIRQF_LINK         6
#define PIRQG_LINK         7
#define PIRQH_LINK         8
//YKN_20160222 Modify -e

#define PIRQ_MASK          0xDCF8

#if defined(HX001EA1_10)
    #define IRQ_ROUTING_COUNT  19
#elif defined(HX001EB1_10)
    #define IRQ_ROUTING_COUNT  19	
#elif defined(HX001EC1_04)
    //MTN-2016102701 #define IRQ_ROUTING_COUNT  14	
    #define IRQ_ROUTING_COUNT  13	//MTN-2016110201
#elif defined(HX001EC1_05)
    #define IRQ_ROUTING_COUNT	 15
#elif defined(HX001EC1_08)
    #define IRQ_ROUTING_COUNT	 14
#elif defined(HX001EC1_09)
    #define IRQ_ROUTING_COUNT	 16
#elif defined(HX001EC1_12)   
    #define IRQ_ROUTING_COUNT	 15
#elif defined(HX001EC1_13)
    #define IRQ_ROUTING_COUNT	 17
#elif defined(HX001ED0_03)
    #define IRQ_ROUTING_COUNT	 14	
#endif






//YKN_20160309 +S
typedef  struct {
    UINT8 Seg;
    UINT8 Bus;
	UINT8 Dev;
	UINT8 Fun;
	UINT8 PriBus;
	UINT8 SecBus;
	UINT8 Enabled;
	UINT8 SlotNum;
} STRUCT_PCI_BRIDGE;

typedef struct {
  EFI_LEGACY_PIRQ_TABLE_HEADER  Hdr;
  EFI_LEGACY_IRQ_ROUTING_ENTRY  Entry[IRQ_ROUTING_COUNT];
} PIR_DATA;

VOID CollectPCIBusInfo (VOID);
VOID UpdatePIRTable (VOID);

#if defined(HX001EA1_10)

#define PCI_BRIDGES_NUM 8
#define SLOT_NUM 8//MTN-2016110201


STRUCT_PCI_BRIDGE gPlatformPciBridgesInfo[] = {
		{0, 0, 5, 1, 0, 0, 0, 1},///PEG3	
		{0, 0, 5, 0, 0, 0, 0, 2},///PEG2
		{0, 0, 4, 0, 0, 0, 0, 3},///PEG0
		{0, 0, 3, 1, 0, 0, 0, 4},///PE1
		{0, 0, 3, 0, 0, 0, 0, 5},///PE0
		{0, 0, 2, 0, 0, 0, 0, 6},///PEG
		{0, 0, 3, 2, 0, 0, 0, 7},///PE2
		{0, 0, 3, 3, 0, 0, 0, 8},///PE3		
};


STATIC PIR_DATA gPirData = {
  {
    EFI_LEGACY_PIRQ_TABLE_SIGNATURE,
    0,
    1,
    sizeof(PIR_DATA),
    LPC_BUS_NO,
    (LPC_DEV_NO<<3) + LPC_FUNC_NO,
    0,
    0x1106,
    0x300A,
    0,
    {0},
    0               // checksum
  },
  {
			{0x00, 0x01 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x02 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x03 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x04 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x05 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x08 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x09 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0A << 3, {{PIRQA_LINK, PIRQ_MASK},{PIRQB_LINK, PIRQ_MASK},{PIRQC_LINK, PIRQ_MASK},{PIRQD_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x0B << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0C << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x14 << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			
			{0x09, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 1 ,0},///PEG3
			{0x08, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 2 ,0},///PEG2
			{0x06, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 3 ,0},///PEG0
			{0x03, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 4 ,0},///PE1
			{0x02, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 5 ,0},///PE0
			{0x01, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 6 ,0},///PEG
			//USB3: PE2 1x
			{0x04, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0, 0},{0, 0}}, 0 ,0},
			//LAN: PE3 1x
			 {0x05, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0, 0},{0, 0}}, 0 ,0}
   }
};


#elif defined(HX001EB1_10)

#define PCI_BRIDGES_NUM 8
#define SLOT_NUM 8//MTN-2016110201


STRUCT_PCI_BRIDGE gPlatformPciBridgesInfo[] = {
		{0, 0, 5, 1, 0, 0, 0, 1},///PEG3	
		{0, 0, 5, 0, 0, 0, 0, 2},///PEG2
		{0, 0, 4, 0, 0, 0, 0, 3},///PEG0
		{0, 0, 3, 1, 0, 0, 0, 4},///PE1
		{0, 0, 3, 0, 0, 0, 0, 5},///PE0
		{0, 0, 2, 0, 0, 0, 0, 6},///PEG
		{0, 0, 3, 2, 0, 0, 0, 7},///PE2
		{0, 0, 3, 3, 0, 0, 0, 8}///PE3		
};


STATIC PIR_DATA gPirData = {
  {
    EFI_LEGACY_PIRQ_TABLE_SIGNATURE,
    0,
    1,
    sizeof(PIR_DATA),
    LPC_BUS_NO,
    (LPC_DEV_NO<<3) + LPC_FUNC_NO,
    0,
    0x1106,
    0x300A,
    0,
    {0},
    0               // checksum
  },
  {
			{0x00, 0x01 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x02 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x03 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x04 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x05 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x08 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x09 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0A << 3, {{PIRQA_LINK, PIRQ_MASK},{PIRQB_LINK, PIRQ_MASK},{PIRQC_LINK, PIRQ_MASK},{PIRQD_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x0B << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0C << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x14 << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			
			{0x09, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 1 ,0},///PEG3
			{0x08, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 2 ,0},///PEG2
			{0x06, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 3 ,0},///PEG0
			{0x03, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 4 ,0},///PE1
			{0x02, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 5 ,0},///PE0
			{0x01, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 6 ,0},///PEG
			//USB3: PE2 1x
			{0x04, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0, 0},{0, 0}}, 0 ,0},
			//LAN: PE3 1x
			 {0x05, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0, 0},{0, 0}}, 0 ,0}
   }
};


#elif defined(HX001EC1_04)

#define PCI_BRIDGES_NUM 3
#define SLOT_NUM 3


STRUCT_PCI_BRIDGE gPlatformPciBridgesInfo[] = {
		{0, 0, 5, 0, 0, 0, 0, 1},///PEG2
		{0, 0, 4, 0, 0, 0, 0, 3},///PEG0
		{0, 0, 2, 0, 0, 0, 0, 5}///PEG		
};


STATIC PIR_DATA gPirData = {
  {
    EFI_LEGACY_PIRQ_TABLE_SIGNATURE,
    0,
    1,
    sizeof(PIR_DATA),
    LPC_BUS_NO,
    (LPC_DEV_NO<<3) + LPC_FUNC_NO,
    0,
    0x1106,
    0x300A,
    0,
    {0},
    0               // checksum
  },
  {
			{0x00, 0x01 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x02 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},			
			{0x00, 0x04 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x05 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x08 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x09 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0A << 3, {{PIRQA_LINK, PIRQ_MASK},{PIRQB_LINK, PIRQ_MASK},{PIRQC_LINK, PIRQ_MASK},{PIRQD_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x0B << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0C << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x14 << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			
			{0x03, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 1 ,0},///PEG2
			{0x02, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 3 ,0},///PEG0
			{0x01, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 5 ,0},///PEG
   }
};


#elif defined(HX001EC1_05)

#define PCI_BRIDGES_NUM 4
#define SLOT_NUM 4


STRUCT_PCI_BRIDGE gPlatformPciBridgesInfo[] = {
		{0, 0, 5, 0, 0, 0, 0, 1},///PEG2
		{0, 0, 4, 0, 0, 0, 0, 3},///PEG0
		{0, 0, 3, 0, 0, 0, 0, 5},///PE0
		{0, 0, 2, 0, 0, 0, 0, 6}///PEG	
};


STATIC PIR_DATA gPirData = {
  {
    EFI_LEGACY_PIRQ_TABLE_SIGNATURE,
    0,
    1,
    sizeof(PIR_DATA),
    LPC_BUS_NO,
    (LPC_DEV_NO<<3) + LPC_FUNC_NO,
    0,
    0x1106,
    0x300A,
    0,
    {0},
    0               // checksum
  },
  {
			{0x00, 0x01 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x02 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x03 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0, 0},{0, 0}}, 0 ,0},
			{0x00, 0x04 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x05 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x08 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x09 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0A << 3, {{PIRQA_LINK, PIRQ_MASK},{PIRQB_LINK, PIRQ_MASK},{PIRQC_LINK, PIRQ_MASK},{PIRQD_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x0B << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0C << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x14 << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			
			{0x08, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 1 ,0},///PEG2
			{0x06, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 3 ,0},///PEG0
			{0x02, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 5 ,0},///PE0
			{0x01, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 6 ,0},///PEG
   }
};


#elif defined(HX001EC1_08)

#define PCI_BRIDGES_NUM 4
#define SLOT_NUM 4


STRUCT_PCI_BRIDGE gPlatformPciBridgesInfo[] = {
		{0, 0, 5, 1, 0, 0, 0, 1},///PEG3
		{0, 0, 5, 0, 0, 0, 0, 2},///PEG2
		{0, 0, 4, 0, 0, 0, 0, 3},///PEG0
		{0, 0, 2, 0, 0, 0, 0, 5}///PEG
};


STATIC PIR_DATA gPirData = {
  {
    EFI_LEGACY_PIRQ_TABLE_SIGNATURE,
    0,
    1,
    sizeof(PIR_DATA),
    LPC_BUS_NO,
    (LPC_DEV_NO<<3) + LPC_FUNC_NO,
    0,
    0x1106,
    0x300A,
    0,
    {0},
    0               // checksum
  },
  {
			{0x00, 0x01 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x02 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x04 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x05 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x08 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x09 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0A << 3, {{PIRQA_LINK, PIRQ_MASK},{PIRQB_LINK, PIRQ_MASK},{PIRQC_LINK, PIRQ_MASK},{PIRQD_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x0B << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0C << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x14 << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			
			{0x09, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 1 ,0},///PEG3
			{0x08, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 2 ,0},///PEG2
			{0x06, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 3 ,0},///PEG0
			{0x01, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 5 ,0},///PEG
   }
};


#elif defined(HX001EC1_09)

#define PCI_BRIDGES_NUM 5
#define SLOT_NUM 5


STRUCT_PCI_BRIDGE gPlatformPciBridgesInfo[] = {
		{0, 0, 5, 1, 0, 0, 0, 1},///PEG3
		{0, 0, 5, 0, 0, 0, 0, 2},///PEG2
		{0, 0, 4, 0, 0, 0, 0, 3},///PEG0
		{0, 0, 3, 0, 0, 0, 0, 5},///PE0
		{0, 0, 2, 0, 0, 0, 0, 6}///PEG		
};


STATIC PIR_DATA gPirData = {
  {
    EFI_LEGACY_PIRQ_TABLE_SIGNATURE,
    0,
    1,
    sizeof(PIR_DATA),
    LPC_BUS_NO,
    (LPC_DEV_NO<<3) + LPC_FUNC_NO,
    0,
    0x1106,
    0x300A,
    0,
    {0},
    0               // checksum
  },
  {
			{0x00, 0x01 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x02 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x03 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0, 0},{0, 0}}, 0 ,0},
			{0x00, 0x04 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x05 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x08 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x09 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0A << 3, {{PIRQA_LINK, PIRQ_MASK},{PIRQB_LINK, PIRQ_MASK},{PIRQC_LINK, PIRQ_MASK},{PIRQD_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x0B << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0C << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x14 << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			
			{0x09, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 1 ,0},///PEG3
			{0x08, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 2 ,0},///PEG2
			{0x06, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 3 ,0},///PEG0
			{0x02, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 5 ,0},///PE0
			{0x01, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 6 ,0},///PEG
   }
};


#elif defined(HX001EC1_12)

#define PCI_BRIDGES_NUM 5
#define SLOT_NUM 5


STRUCT_PCI_BRIDGE gPlatformPciBridgesInfo[] = {
		{0, 0, 5, 1, 0, 0, 0, 1},///PEG3
		{0, 0, 5, 0, 0, 0, 0, 2},///PEG2
		{0, 0, 4, 1, 0, 0, 0, 3},///PEG1
		{0, 0, 4, 0, 0, 0, 0, 4},///PEG0
		{0, 0, 2, 0, 0, 0, 0, 5}///PEG		
};


STATIC PIR_DATA gPirData = {
  {
    EFI_LEGACY_PIRQ_TABLE_SIGNATURE,
    0,
    1,
    sizeof(PIR_DATA),
    LPC_BUS_NO,
    (LPC_DEV_NO<<3) + LPC_FUNC_NO,
    0,
    0x1106,
    0x300A,
    0,
    {0},
    0               // checksum
  },
  {
			{0x00, 0x01 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x02 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x04 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x05 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x08 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x09 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0A << 3, {{PIRQA_LINK, PIRQ_MASK},{PIRQB_LINK, PIRQ_MASK},{PIRQC_LINK, PIRQ_MASK},{PIRQD_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x0B << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0C << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x14 << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			
			{0x09, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 1 ,0},///PEG3
			{0x08, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 2 ,0},///PEG2
			{0x07, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 3 ,0},///PEG1
			{0x06, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 4 ,0},///PEG0
			{0x01, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 5 ,0},///PEG
   }
};



#elif defined(HX001EC1_13)

#define PCI_BRIDGES_NUM 6
#define SLOT_NUM 6


STRUCT_PCI_BRIDGE gPlatformPciBridgesInfo[] = {
		{0, 0, 5, 1, 0, 0, 0, 1},///PEG3
		{0, 0, 5, 0, 0, 0, 0, 2},///PEG2
		{0, 0, 4, 1, 0, 0, 0, 3},///PEG1
		{0, 0, 4, 0, 0, 0, 0, 4},///PEG0
		{0, 0, 3, 0, 0, 0, 0, 5},///PE0
		{0, 0, 2, 0, 0, 0, 0, 6}///PEG		
};


STATIC PIR_DATA gPirData = {
  {
    EFI_LEGACY_PIRQ_TABLE_SIGNATURE,
    0,
    1,
    sizeof(PIR_DATA),
    LPC_BUS_NO,
    (LPC_DEV_NO<<3) + LPC_FUNC_NO,
    0,
    0x1106,
    0x300A,
    0,
    {0},
    0               // checksum
  },
  {
			{0x00, 0x01 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x02 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x03 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0, 0},{0, 0}}, 0 ,0},
			{0x00, 0x04 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x05 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x08 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x09 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0A << 3, {{PIRQA_LINK, PIRQ_MASK},{PIRQB_LINK, PIRQ_MASK},{PIRQC_LINK, PIRQ_MASK},{PIRQD_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x0B << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0C << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x14 << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			
			{0x09, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 1 ,0},///PEG3
			{0x08, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 2 ,0},///PEG2
			{0x07, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 3 ,0},///PEG1
			{0x06, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 4 ,0},///PEG0
			{0x02, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 5 ,0},///PE0
			{0x01, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 6 ,0},///PEG
   }
};



#elif defined(HX001ED0_03)

#define PCI_BRIDGES_NUM 5
#define SLOT_NUM 5


STRUCT_PCI_BRIDGE gPlatformPciBridgesInfo[] = {
		{0, 0, 4, 0, 0, 0, 0, 1},///PEG0
		{0, 0, 3, 2, 0, 0, 0, 2},///PE2
		{0, 0, 3, 0, 0, 0, 0, 3},///PE0
		{0, 0, 2, 0, 0, 0, 0, 4},///PEG
		{0, 0, 3, 3, 0, 0, 0, 5}///PE3	
};


STATIC PIR_DATA gPirData = {
  {
    EFI_LEGACY_PIRQ_TABLE_SIGNATURE,
    0,
    1,
    sizeof(PIR_DATA),
    LPC_BUS_NO,
    (LPC_DEV_NO<<3) + LPC_FUNC_NO,
    0,
    0x1106,
    0x300A,
    0,
    {0},
    0               // checksum
  },
  {
			{0x00, 0x01 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x02 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x03 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x04 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0,0},{0,0}}, 0 ,0},
			{0x00, 0x08 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x09 << 3, {{PIRQH_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0A << 3, {{PIRQA_LINK, PIRQ_MASK},{PIRQB_LINK, PIRQ_MASK},{PIRQC_LINK, PIRQ_MASK},{PIRQD_LINK, PIRQ_MASK}}, 0 ,0},
			{0x00, 0x0B << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x0C << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0}, 
			{0x00, 0x14 << 3, {{PIRQA_LINK, PIRQ_MASK},{0,0},{0,0},{0,0}}, 0 ,0},
			
			{0x06, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 1 ,0},///PEG0
			{0x04, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 2 ,0},///PE2
			{0x02, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 3 ,0},///PE0
			{0x01, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK},{PIRQH_LINK, PIRQ_MASK}}, 4 ,0},///PEG
			{0x05, 0x00 << 3, {{PIRQH_LINK, PIRQ_MASK},{0, 0},{0, 0},{0, 0}}, 0 ,0},///PE3 LAN
   }
};


#endif

#endif
  
  
  
  
  
  
  
  
