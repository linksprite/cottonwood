/*----------------------------------------------------------------------------- */
/* F3xx_USB0_ReportHandler.h */
/*----------------------------------------------------------------------------- */
/* Copyright 2005 Silicon Laboratories, Inc. */
/* http://www.silabs.com */
/* */
/* Program Description: */
/* */
/* Includes functions called by USB_ISR.c to handle input and output reports.// */
/* */
/* How To Test:    See Readme.txt */
/* */
/* */
/* FID             3XX000014 */
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

#ifndef  _USB_REPORTHANDLER_H_
#define  _USB_REPORTHANDLER_H_

typedef struct {
    unsigned char ReportID;
    void (*hdlr)();
} VectorTableEntry;

typedef struct {
    unsigned char Length;
    unsigned char* Ptr;
} BufferStructure;

extern void ReportHandler_IN_ISR(unsigned char);
extern void ReportHandler_IN_Foreground(unsigned char);
extern void ReportHandler_OUT(unsigned char);
extern void Setup_OUT_BUFFER(void);

extern unsigned char getReceiveFlag(void);
extern void resetUSBReceiveFlag(void);

extern BufferStructure IN_BUFFER, OUT_BUFFER;

extern xdata unsigned char getBuffer_[];

#endif

