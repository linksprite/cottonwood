//-----------------------------------------------------------------------------
//  F340_FlashPrimitives.h
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This program contains several useful utilities for writing and updating
// FLASH memory.
//
// FID:            34X000025
// Target:         C8051F34x
// Tool chain:     Keil C51 8.00 / Keil EVAL C51
// Command Line:   None
//
// Release 1.0
//    -Initial Revision (GP)
//    -30 JAN 2006
//

#ifndef F340_FLASHPRIMITIVES_H
#define F340_FLASHPRIMITIVES_H

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Structures, Unions, Enumerations, and Type Definitions
//-----------------------------------------------------------------------------

typedef unsigned long ULONG;
typedef unsigned int  UINT;
typedef unsigned char UCHAR;

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

#ifndef FLASH_PAGESIZE
#define FLASH_PAGESIZE 512
#endif

#ifndef FLASH_TEMP
#define FLASH_TEMP 0x0F800L
#endif

#ifndef FLASH_LAST
#define FLASH_LAST 0x0FA00L
#endif

typedef UINT FLADDR;

//-----------------------------------------------------------------------------
// Exported Function Prototypes
//-----------------------------------------------------------------------------

// FLASH read/write/erase routines
extern void FLASH_ByteWrite (FLADDR addr, char byte);
extern unsigned char FLASH_ByteRead (FLADDR addr);
extern void FLASH_PageErase (FLADDR addr);

#endif                                 // F340_FLASHPRIMITIVES_H

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------