// 
// Copyright (c) 2006-2012, Intel Corporation
// All rights reserved. This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
// 
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
// 
// Module Description:
//
// Formset guids, form id and VarStore data structure for Boot Maintenance Manager.

#ifndef _FORM_GUID_H_
#define _FORM_GUID_H_

#define BOOT_MAINT_FORMSET_GUID \
  { \
  0x642237c7, 0x35d4, 0x472d, {0x83, 0x65, 0x12, 0xe0, 0xcc, 0xf2, 0x7a, 0x22} \
  }
  

#define FORM_MAIN_ID                         0x01   
#define FORM_BOOT_ADD_ID                     0x1002
#define FORM_BOOT_DEL_ID                     0x1003
#define FORM_BOOT_CHG_ID                     0x1004
#define FORM_BOOT_SETUP_ID                   0x1014
#define FORM_DRIVER_SETUP_ID                 0x1015
#define FORM_BOOT_LEGACY_DEVICE_ID           0x1016
#define FORM_SET_FD_ORDER_ID                 0x1019
#define FORM_SET_HD_ORDER_ID                 0x101A
#define FORM_SET_CD_ORDER_ID                 0x101B
#define FORM_SET_NET_ORDER_ID                0x101C
#define FORM_SET_BEV_ORDER_ID                0x101D
#define FORM_FILE_EXPLORER_ID                0x101E
#define FORM_BOOT_ADD_DESCRIPTION_ID         0x101F
#define FORM_DRIVER_ADD_FILE_DESCRIPTION_ID  0x1020
#define FORM_CON_MODE_ID                     0x1021
#define FORM_MEMORY_CHECK_ID                 0x1022
#define FORM_UEFI_OPTIMIZED_BOOT_ID          0x1023

#define MAXIMUM_FORM_ID                      0x10FF

#define KEY_VALUE_MAIN_BOOT_NEXT             0x1106
#define KEY_VALUE_BOOT_ADD_DESC_DATA         0x1107
#define KEY_VALUE_BOOT_ADD_OPT_DATA          0x1108
#define KEY_VALUE_DRIVER_ADD_DESC_DATA       0x1109
#define KEY_VALUE_DRIVER_ADD_OPT_DATA        0x110A
#define KEY_VALUE_SAVE_AND_EXIT              0x110B
#define KEY_VALUE_NO_SAVE_AND_EXIT           0x110C
#define KEY_VALUE_BOOT_FROM_FILE             0x110D
#define KEY_VALUE_CSM_CONFIGURATION          0x110E

#define MAXIMUM_NORMAL_KEY_VALUE             0x11FF

//
// Varstore ID defined for Buffer Storage
//
#define VARSTORE_ID_BOOT_MAINT               0x1000
#define VARSTORE_ID_FILE_EXPLORER            0x1001

//
// End Label
//
#define LABEL_END   0xffff
#define MAX_MENU_NUMBER 100

///
/// This is the structure that will be used to store the
/// question's current value. Use it at initialize time to
/// set default value for each question. When using at run
/// time, this map is returned by the callback function,
/// so dynamically changing the question's value will be
/// possible through this mechanism
///
typedef struct {
  //
  // Boot or Driver Option Order storage
  // The value is the OptionNumber+1 because the order list value cannot be 0
  // Use UINT32 to hold the potential value 0xFFFF+1=0x10000
  //
  UINT32  OptionOrder[MAX_MENU_NUMBER];

  //
  // Boot or Driver Option Delete storage
  //
  BOOLEAN OptionDel[MAX_MENU_NUMBER];

  //
  // Legacy Device Order Selection Storage
  //
  UINT8   LegacyFD[MAX_MENU_NUMBER];
  UINT8   LegacyHD[MAX_MENU_NUMBER];
  UINT8   LegacyCD[MAX_MENU_NUMBER];
  UINT8   LegacyNET[MAX_MENU_NUMBER];
  UINT8   LegacyBEV[MAX_MENU_NUMBER];

  //
  // We use DisableMap array to record the enable/disable state of each boot device
  // It should be taken as a bit array, from left to right there are totally 256 bits
  // the most left one stands for BBS table item 0, and the most right one stands for item 256
  // If the bit is 1, it means the boot device has been disabled.
  //
  UINT8   DisableMap[32];

  //
  //  UINT16                    PadArea[10];
  //
} BMM_FAKE_NV_DATA;


#endif

