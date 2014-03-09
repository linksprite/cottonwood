/*----------------------------------------------------------------------------- */
/* F3xx_Blink_Control.h */
/*----------------------------------------------------------------------------- */
/* Copyright 2005 Silicon Laboratories, Inc. */
/* http://www.silabs.com */
/* */
/* Program Description: */
/* */
/* This file includes all of the Report IDs and variables needed by */
/* USB_ReportHandler.c to process input and output reports, */
/* as well as initialization routine prototypes. */
/* */
/* */
/* How To Test:    See Readme.txt */
/* */
/* */
/* FID:            3XX000010 */
/* Target:         C8051F3xx */
/* Tool chain:     Keil C51 7.50 / Keil EVAL C51 */
/*                 Silicon Laboratories IDE version 2.6 */
/* Command Line:   See Readme.txt */
/* Project Name:   F3xx_BlinkyExample */
/* */
/* Release 1.1 */
/*    -Added feature reports for dimming controls */
/*    -Added PCA dimmer functionality */
/*    -16 NOV 2006 */
/* Release 1.0 */
/*    -Initial Revision (PD) */
/*    -07 DEC 2005 */
/* */

#ifndef  _BLINK_C_H_
#define  _BLINK_C_H_

extern unsigned char xdata BLINK_SELECTOR;
extern unsigned char BLINK_PATTERN[];
extern unsigned int xdata BLINK_RATE;
extern unsigned char xdata BLINK_ENABLE;
extern unsigned char xdata BLINK_SELECTORUPDATE;
extern unsigned char xdata BLINK_LED1ACTIVE;
extern unsigned char xdata BLINK_LED2ACTIVE;
extern unsigned char xdata BLINK_DIMMER;
extern unsigned char xdata BLINK_DIMMER_SUCCESS;

extern unsigned char xdata OUT_PACKET[];
extern unsigned char xdata IN_PACKET[];

void System_Init(void);
void Usb_Init(void);

/* ---------------------------------------------------------------------------- */
/* Report IDs */
/* ---------------------------------------------------------------------------- */
/*#define OUT_FIRM_HARDW_ID       0x07 */
/*#define OUT_BLINK_PATTERNID     0x01 */
#define OUT_BLINK_ENABLEID      0x02
/*#define OUT_BLINK_RATEID        0x03 */
/*#define IN_BLINK_SELECTORID     0x04 */
#define IN_BLINK_STATSID        0x05
/*#define FEATURE_BLINK_DIMMERID  0x06 */

/*#define OUT_FIRM_HARDW_ID       0x10 */
/*#define IN_FIRM_HARDW_ID        0x11 */

/* ---------------------------------------------------------------------------- */
/* Report Sizes (in bytes) */
/* ---------------------------------------------------------------------------- */
/*#define OUT_FIRM_HARDW_IDSize    0x04 */
/*#define OUT_BLINK_PATTERNSize    0x08 */
/*#define OUT_BLINK_ENABLESize     0x01 */
/*#define OUT_BLINK_RATESize       0x02 */
/*#define IN_BLINK_SELECTORSize    0x01 */
#define IN_BLINK_STATSSIZE       0x02
/*#define FEATURE_BLINK_DIMMERSIZE 0x01 */

/*#define OUT_FIRM_HARDW_IDSize    0x02 */
/*#define IN_FIRM_HARDW_IDSize     0x30 */

#endif
