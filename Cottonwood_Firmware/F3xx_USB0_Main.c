/*----------------------------------------------------------------------------- */
/* F3xx_USB_Main.c */
/*----------------------------------------------------------------------------- */
/* Copyright 2005 Silicon Laboratories, Inc. */
/* http://www.silabs.com */
/* */
/* Program Description: */
/* */
/* This application will communicate with a PC across the USB interface. */
/* The device will appear to be a mouse, and will manipulate the cursor */
/* on screen. */
/* */
/* How To Test:    See Readme.txt */
/* */
/* */
/* FID:            3XX000006 */
/* Target:         C8051F32x/C8051F340 */
/* Tool chain:     Keil C51 7.50 / Keil EVAL C51 */
/*                 Silicon Laboratories IDE version 2.6 */
/* Command Line:   See Readme.txt */
/* Project Name:   F3xx_BlinkyExample */
/* */
/* */
/* Release 1.1 */
/*    -Added feature reports for dimming controls */
/*    -Added PCA dimmer functionality */
/*    -16 NOV 2006 */
/* Release 1.0 */
/*    -Initial Revision (PD) */
/*    -07 DEC 2005 */
/* */
/*----------------------------------------------------------------------------- */
/* Header Files */
/*----------------------------------------------------------------------------- */
/*
#include "c8051f3xx.h"
#include "F3xx_USB0_Register.h"
#include "F3xx_Blink_Control.h"
#include "F3xx_USB0_InterruptServiceRoutine.h"
#include "F3xx_USB0_Descriptor.h"

/*----------------------------------------------------------------------------- */
/* Definitions */
/*----------------------------------------------------------------------------- */

/*----------------------------------------------------------------------------- */
/* Main Routine */
/*----------------------------------------------------------------------------- */
void main(void)
{

   System_Init ();
   Usb_Init ();

   EA = 1;

   while (1)
   {
      if (BLINK_SELECTORUPDATE)
      {
         BLINK_SELECTORUPDATE = 0;
         SendPacket (IN_BLINK_SELECTORID);
      }
   }
}
*/

