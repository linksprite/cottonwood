//-----------------------------------------------------------------------------
// F340_FlashPrimitives.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This program contains several useful utilities for writing and updating
// FLASH memory.
//
// FID:            34X000024
// Target:         C8051F34x
// Tool chain:     Keil C51 8.00 / Keil EVAL C51
// Command Line:   None
//
// Release 1.0
//    -Initial Revision (GP)
//    -30 JAN 2006
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "F340_FlashPrimitives.h"
#include <c8051F340.h>

//-----------------------------------------------------------------------------
// Structures, Unions, Enumerations, and Type Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

// FLASH read/write/erase routines
void FLASH_ByteWrite (FLADDR addr, char byte);
unsigned char FLASH_ByteRead (FLADDR addr);
void FLASH_PageErase (FLADDR addr);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FLASH Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FLASH_ByteWrite
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
//   1)  FLADDR addr - target address to write to
//                     range is 0 to (FLASH_TEMP-1)
//   2)  char byte   - byte to write
//
// This routine writes <byte> to the linear FLASH address <addr>.
//
//-----------------------------------------------------------------------------
void FLASH_ByteWrite (FLADDR addr, char byte)
{
   bit EA_SAVE = EA;                   // Preserve EA
   char xdata * data pwrite;           // FLASH write pointer

   EA = 0;                             // Disable interrupts

   // change clock speed to slow, then restore later

   VDM0CN = 0x80;                      // Enable VDD monitor


   RSTSRC = 0x02;                      // Enable VDD monitor as a reset source

   pwrite = (char xdata *) addr;

   FLKEY  = 0xA5;                      // Key Sequence 1
   FLKEY  = 0xF1;                      // Key Sequence 2
   PSCTL |= 0x01;                      // PSWE = 1


   VDM0CN = 0x80;                      // Enable VDD monitor


   RSTSRC = 0x02;                      // Enable VDD monitor as a reset source

   *pwrite = byte;                     // Write the byte

   PSCTL &= ~0x01;                     // PSWE = 0

   EA = EA_SAVE;                       // Restore interrupts
}
#if 0 /* not needed */
//-----------------------------------------------------------------------------
// FLASH_ByteRead
//-----------------------------------------------------------------------------
//
// Return Value : 
//   1)  unsigned char  - byte that was read from Flash
// Parameters   : 
//   1)  FLADDR addr    - target address to write to
//                        range is 0 to (FLASH_TEMP-1)
//
// This routine reads a <byte> from the linear FLASH address <addr>.
//
//-----------------------------------------------------------------------------
unsigned char FLASH_ByteRead (FLADDR addr)
{
   bit EA_SAVE = EA;                   // Preserve EA
   char code * data pread;             // FLASH read pointer
   unsigned char byte;

   EA = 0;                             // Disable interrupts

   pread = (char code *) addr;

   byte = *pread;                      // Read the byte

   EA = EA_SAVE;                       // Restore interrupts

   return byte;
}

//-----------------------------------------------------------------------------
// FLASH_PageErase
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
//   1)  FLADDR addr    - target address to write to
//                        range is 0 to (FLASH_TEMP-1)
//
// This routine erases the FLASH page containing the linear FLASH address
// <addr>.
//
//-----------------------------------------------------------------------------
void FLASH_PageErase (FLADDR addr)
{
   bit EA_SAVE = EA;                   // Preserve EA
   char xdata * data pwrite;           // FLASH write pointer

   EA = 0;                             // Disable interrupts
   // change clock speed to slow, then restore later

   VDM0CN = 0x80;                      // Enable VDD monitor


   RSTSRC = 0x02;                      // enable VDD monitor as a reset source

   pwrite = (char xdata *) addr;

   FLKEY  = 0xA5;                      // Key Sequence 1
   FLKEY  = 0xF1;                      // Key Sequence 2
   PSCTL |= 0x03;                      // PSWE = 1; PSEE = 1


   VDM0CN = 0x80;                      // Enable VDD monitor


   RSTSRC = 0x02;                      // Enable VDD monitor as a reset source
   *pwrite = 0;                        // Initiate page erase

   PSCTL &= ~0x03;                     // PSWE = 0; PSEE = 0

   EA = EA_SAVE;                       // Restore interrupts
}
#endif
//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
