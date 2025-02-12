/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::File System:Drive
 * Copyright (c) 2004-2013 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    FS_Config_USB_0.h
 * Purpose: File System Configuration for USB Drive
 * Rev.:    V5.00
 *----------------------------------------------------------------------------*/

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>USB Drive 0
// <i>Configuration for USB device assigned to drive letter "U0:"
#define USB0_ENABLE             1

//   <o>Drive Cache Size <0=>OFF <1=>1 KB <2=>2 KB <4=>4 KB
//                       <8=>8 KB <16=>16 KB <32=>32 KB
//   <i>Drive Cache stores data sectors and may be increased to speed-up
//   <i>file read/write operations on this drive (default: 4 KB)
#define USB0_CACHE_SIZE         8

//   <q>Use FAT Journal
//   <i>Protect File Allocation Table and Directory Entries for
//   <i>fail-safe operation.
#define USB0_FAT_JOURNAL        0

//   <q>Default Drive "U0:"
//   <i>Use this drive when no drive letter is specified.
#define USB0_DEFAULT_DRIVE      1

// </h>
