//-----------------------------------------------------------------------------
// F3xx_USB0_ReportHandler.c
//-----------------------------------------------------------------------------
// Copyright 2005 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Contains functions and variables dealing with Input and Output
// HID reports.
// How To Test:    See Readme.txt
//
//
// FID:            3XX000007
// Target:         C8051F340
// Tool chain:     Keil C51 7.50 / Keil EVAL C51
//                 Silicon Laboratories IDE version 2.6
// Command Line:   See Readme.txt
// Project Name:   F3xx_BlinkyExample
//
// Release 1.1
//    -Added feature reports for dimming controls
//    -Added PCA dimmer functionality
//    -16 NOV 2006
// Release 1.0
//    -Initial Revision (PD)
//    -07 DEC 2005
//

// ----------------------------------------------------------------------------
// Header files
// ----------------------------------------------------------------------------

#include "global.h"
#include "usb_commands.h"
#include "F3xx_USB0_InterruptServiceRoutine.h"
#include "F3xx_Blink_Control.h"
#include "F3xx_USB0_ReportHandler.h"
#include "uart.h"

// ----------------------------------------------------------------------------
// Local Function Prototypes
// ----------------------------------------------------------------------------

// ****************************************************************************
// Add custom Report Handler Prototypes Here
// ****************************************************************************

void IN_Report(void);
void OUT_Report(void);

void IN_BLINK_SELECTOR(void);
void OUT_BLINK_ENABLE(void);
void OUT_BLINK_PATTERN(void);
void OUT_BLINK_RATE(void);
void IN_BLINK_STATS(void);
void FEATURE_BLINK_DIMMER_INPUT (void);
void FEATURE_BLINK_DIMMER_OUTPUT (void);
// ----------------------------------------------------------------------------
// Local Definitions
// ----------------------------------------------------------------------------

// ****************************************************************************
// Set Definitions to sizes corresponding to the number of reports
// ****************************************************************************

#define IN_VECTORTABLESize 3
#define OUT_VECTORTABLESize 4

// ----------------------------------------------------------------------------
// Global Constant Declaration
// ----------------------------------------------------------------------------

// ****************************************************************************
// Link all Report Handler functions to corresponding Report IDs
// ****************************************************************************

const VectorTableEntry IN_VECTORTABLE[IN_VECTORTABLESize] =
{
    // FORMAT: Report ID, Report Handler
//   IN_BLINK_SELECTORID, IN_BLINK_SELECTOR,
    IN_BLINK_STATSID, IN_BLINK_STATS,
//   FEATURE_BLINK_DIMMERID, FEATURE_BLINK_DIMMER_INPUT
};

// ****************************************************************************
// Link all Report Handler functions to corresponding Report IDs
// ****************************************************************************
const VectorTableEntry OUT_VECTORTABLE[OUT_VECTORTABLESize] =
{
    // FORMAT: Report ID, Report Handler
    OUT_BLINK_ENABLEID, OUT_BLINK_ENABLE,
//   OUT_BLINK_PATTERNID, OUT_BLINK_PATTERN,
//   OUT_BLINK_RATEID, OUT_BLINK_RATE,
//   FEATURE_BLINK_DIMMERID, FEATURE_BLINK_DIMMER_OUTPUT

};

// ----------------------------------------------------------------------------
// Global Variable Declaration
// ----------------------------------------------------------------------------

BufferStructure IN_BUFFER, OUT_BUFFER;

xdata unsigned char getBuffer_[EP1_PACKET_SIZE];

unsigned char busy_ = 0;

// ----------------------------------------------------------------------------
// Local Variable Declaration
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Local Functions
// ----------------------------------------------------------------------------

// ****************************************************************************
// Add all Report Handler routines here
// ****************************************************************************

// ****************************************************************************
// For Input Reports:
// Point IN_BUFFER.Ptr to the buffer containing the report
// Set IN_BUFFER.Length to the number of bytes that will be
// transmitted.
//
// REMINDER:
// Systems using more than one report must define Report IDs
// for each report.  These Report IDs must be included as the first
// byte of an IN report.
// Systems with Report IDs should set the FIRST byte of their buffer
// to the value for the Report ID
// AND
// must transmit Report Size + 1 to include the full report PLUS
// the Report ID.
//
// ****************************************************************************

unsigned char getReceiveFlag(void)
{
    return(busy_);
}

void resetUSBReceiveFlag(void)
{
//  EA = 0;
    busy_ = 0;
//  EA = 1;
}

// ----------------------------------------------------------------------------
// IN_Blink_Stats()
// ----------------------------------------------------------------------------
// This routine sends statistics calculated in Blink_Control_...c to
// the host application.
//-----------------------------------------------------------------------------
void IN_BLINK_STATS(void)
{
    IN_PACKET[0] = IN_BLINK_STATSID;
    IN_PACKET[1] = BLINK_LED1ACTIVE;
    IN_PACKET[2] = BLINK_LED2ACTIVE;

    IN_BUFFER.Ptr = IN_PACKET;
    IN_BUFFER.Length = IN_BLINK_STATSSIZE + 1;
}

// ****************************************************************************
// For Output Reports:
// Data contained in the buffer OUT_BUFFER.Ptr will not be
// preserved after the Report Handler exits.
// Any data that needs to be preserved should be copyed from
// the OUT_BUFFER.Ptr and into other user-defined memory.
//
// ****************************************************************************

// ----------------------------------------------------------------------------
// OUT_BLINK_ENABLE()
// ----------------------------------------------------------------------------
// This handler saves the Blink enable state sent from the host.
//-----------------------------------------------------------------------------
void OUT_BLINK_ENABLE(void)
{

    BLINK_ENABLE = OUT_BUFFER.Ptr[1];

}

// ----------------------------------------------------------------------------
// Global Functions
// ----------------------------------------------------------------------------

// ****************************************************************************
// Configure Setup_OUT_BUFFER
//
// Reminder:
// This function should set OUT_BUFFER.Ptr so that it
// points to an array in data space big enough to store
// any output report.
// It should also set OUT_BUFFER.Length to the size of
// this buffer.
//
// ****************************************************************************

void Setup_OUT_BUFFER(void)
{
    OUT_BUFFER.Ptr = OUT_PACKET;
    OUT_BUFFER.Length = 10;
}

// ----------------------------------------------------------------------------
// Vector Routines
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ReportHandler_IN...
// ----------------------------------------------------------------------------
//
// Return Value - None
// Parameters - Report ID
//
// These functions match the Report ID passed as a parameter
// to an Input Report Handler.
// the ...FG function is called in the SendPacket foreground routine,
// while the ...ISR function is called inside the USB ISR.  A lock
// is set whenever one function is called to prevent a call from the
// other from disrupting the routine.
// However, this should never occur, as interrupts are disabled by SendPacket
// before USB operation begins.
// ----------------------------------------------------------------------------
void ReportHandler_IN_ISR(unsigned char R_ID)
{

    unsigned char index;

    index = 0;

    while (index <= IN_VECTORTABLESize)
    {
        // Check to see if Report ID passed into function
        // matches the Report ID for this entry in the Vector Table
        if (IN_VECTORTABLE[index].ReportID == R_ID)
        {
            IN_VECTORTABLE[index].hdlr();
            break;
        }

        // If Report IDs didn't match, increment the index pointer
        index++;
    }

}
void ReportHandler_IN_Foreground(unsigned char R_ID)
{
    call_fkt_[R_ID]();

}

// ----------------------------------------------------------------------------
// ReportHandler_OUT
// ----------------------------------------------------------------------------
//
// Return Value - None
// Parameters - None
//
// This function matches the Report ID passed as a parameter
// to an Output Report Handler.
//
// ----------------------------------------------------------------------------
void ReportHandler_OUT(unsigned char R_ID)
{
    R_ID = R_ID; // not used

    if (!busy_)
    {
        busy_ = 1;
        copyBuffer(OUT_BUFFER.Ptr, getBuffer_, OUT_BUFFER.Ptr[LENGTH_BYTE]+1);
    }
    else
    {
        CON_print("usb error - report %hhx lost after %hhx\n",OUT_BUFFER.Ptr[0],getBuffer_[0]);
    }
}
