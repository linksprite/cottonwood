/*----------------------------------------------------------------------------- */
/* F3xx_USB0_InterruptServiceRoutine.c */
/*----------------------------------------------------------------------------- */
/* Copyright 2005 Silicon Laboratories, Inc. */
/* http://www.silabs.com */
/* */
/* Program Description: */
/* */
/* Source file for USB firmware. Includes top level ISR with SETUP, */
/* and Endpoint data handlers.  Also includes routine for USB suspend, */
/* reset, and procedural stall. */
/* */
/* */
/* How To Test:    See Readme.txt */
/* */
/* */
/* FID:            3XX000005 */
/* Target:         C8051F32x */
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
/* Includes */
/*----------------------------------------------------------------------------- */
#include "c8051F340.h"
/*#include "c8051f3xx.h" */
#include "timer.h"
#include "uart.h"
#include "F3xx_USB0_Register.h"
#include "F3xx_USB0_InterruptServiceRoutine.h"
#include "F3xx_USB0_Descriptor.h"
#include "F3xx_USB0_ReportHandler.h"

/*----------------------------------------------------------------------------- */
/* Global Variable Definitions */
/*----------------------------------------------------------------------------- */
unsigned char USB0_STATE;              /* Holds the current USB State */
/* def. in F3xx_USB0_InterruptServiceRoutine.h */

setup_buffer SETUP;                    /* Buffer for current device */
/* request information */

unsigned int DATASIZE;                 /* Size of data to return */
unsigned int DATASENT;                 /* Amount of data sent so far */
unsigned char* DATAPTR;                /* Pointer to data to return */

unsigned char EP_STATUS[3] = {EP_IDLE, EP_HALT, EP_HALT};
/* Holds the status for each endpoint */

/*----------------------------------------------------------------------------- */
/* Local Function Definitions */
/*----------------------------------------------------------------------------- */
void Usb_Resume (void);                /* Resumes USB operation */
void Usb_Reset (void);                 /* Called after USB bus reset */
void Handle_Control (void);            /* Handle SETUP packet on EP 0 */
void Handle_In1 (void);                /* Handle in packet on EP 1 */
void Handle_Out1 (void);               /* Handle out packet on EP 1 */
void Usb_Suspend (void);               /* This routine called when */
/* Suspend signalling on bus */
void Fifo_Read (unsigned char, unsigned int, unsigned char *);
/* Used for multiple byte reads */
/* of Endpoint fifos */
void Fifo_Write_Foreground (unsigned char, unsigned int, unsigned char *);
/* Used for multiple byte writes */
/* of Endpoint fifos in foreground */
void Fifo_Write_InterruptServiceRoutine (unsigned char, unsigned int,
        unsigned char *);
/* Used for multiple byte */
/* writes of Endpoint fifos */

/*----------------------------------------------------------------------------- */
/* Usb_ISR */
/*----------------------------------------------------------------------------- */
/* */
/* Called after any USB type interrupt, this handler determines which type */
/* of interrupt occurred, and calls the specific routine to handle it. */
/* */
/*----------------------------------------------------------------------------- */
void Usb_ISR (void) interrupt 8        /* Top-level USB ISR */
{

    unsigned char bCommon, bIn, bOut;
    POLL_READ_BYTE (CMINT, bCommon);    /* Read all interrupt registers */
    POLL_READ_BYTE (IN1INT, bIn);       /* this read also clears the register */
    POLL_READ_BYTE (OUT1INT, bOut);
    {
        if (bCommon & rbRSUINT)          /* Handle Resume interrupt */
        {
            Usb_Resume ();
        }
        if (bCommon & rbRSTINT)          /* Handle Reset interrupt */
        {
            Usb_Reset ();
        }
        if (bIn & rbEP0)                 /* Handle SETUP packet received */
        {                                /* or packet transmitted if Endpoint 0 */
            Handle_Control();             /* is in transmit mode */
        }
        if (bIn & rbIN1)                 /* Handle In Packet sent, put new data */
        {                                /* on endpoint 1 fifo */
            Handle_In1 ();
        }
        if (bOut & rbOUT1)               /* Handle Out packet received, take */
        {                                /* data off endpoint 2 fifo */
            Handle_Out1 ();
        }
        if (bCommon & rbSUSINT)          /* Handle Suspend interrupt */
        {
            Usb_Suspend ();
        }
    }
}

/*----------------------------------------------------------------------------- */
/* Support Routines */
/*----------------------------------------------------------------------------- */

/*----------------------------------------------------------------------------- */
/* Usb_Reset */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value : None */
/* Parameters   : None */
/* */
/* - Set state to default */
/* - Clear Usb Inhibit bit */
/* */
/*----------------------------------------------------------------------------- */

void Usb_Reset (void)
{
    USB0_STATE = DEV_DEFAULT;           /* Set device state to default */

    POLL_WRITE_BYTE (POWER, 0x01);      /* Clear usb inhibit bit to enable USB */
    /* suspend detection */

    EP_STATUS[0] = EP_IDLE;             /* Set default Endpoint Status */
    EP_STATUS[1] = EP_HALT;
    EP_STATUS[2] = EP_HALT;
}

/*----------------------------------------------------------------------------- */
/* Usb_Resume */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value : None */
/* Parameters   : None */
/* */
/* Resume normal USB operation */
/* */
/*----------------------------------------------------------------------------- */

void Usb_Resume(void)
{
    volatile int k;

    k++;

    /* Add code for resume */
}

/*----------------------------------------------------------------------------- */
/* Handle_Control */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value : None */
/* Parameters   : None */
/* */
/* - Decode Incoming SETUP requests */
/* - Load data packets on fifo while in transmit mode */
/* */
/*----------------------------------------------------------------------------- */

void Handle_Control (void)
{
    unsigned char ControlReg;           /* Temporary storage for EP control */
    /* register */

    POLL_WRITE_BYTE (INDEX, 0);         /* Set Index to Endpoint Zero */
    POLL_READ_BYTE (E0CSR, ControlReg); /* Read control register */

    if (EP_STATUS[0] == EP_ADDRESS)     /* Handle Status Phase of Set Address */
        /* command */
    {
        POLL_WRITE_BYTE (FADDR, SETUP.wValue.c[LSB]);
        EP_STATUS[0] = EP_IDLE;
    }

    if (ControlReg & rbSTSTL)           /* If last packet was a sent stall, */
    {                                   /* reset STSTL bit and return EP0 */
        /* to idle state */
        POLL_WRITE_BYTE (E0CSR, 0);
        EP_STATUS[0] = EP_IDLE;
        return;
    }

    if (ControlReg & rbSUEND)           /* If last SETUP transaction was */
    {                                   /* ended prematurely then set */
        POLL_WRITE_BYTE (E0CSR, rbDATAEND);
        /* Serviced SETUP End bit and return EP0 */
        POLL_WRITE_BYTE (E0CSR, rbSSUEND);
        EP_STATUS[0] = EP_IDLE;          /* To idle state */
    }

    if (EP_STATUS[0] == EP_IDLE)        /* If Endpoint 0 is in idle mode */
    {
        if (ControlReg & rbOPRDY)        /* Make sure that EP 0 has an Out Packet */
        {                                /* ready from host although if EP0 */
            /* is idle, this should always be the */
            /* case */
            Fifo_Read (FIFO_EP0, 8, (unsigned char *)&SETUP);
            /* Get SETUP Packet off of Fifo, */
            /* it is currently Big-Endian */

            /* Compiler Specific - these next three */
            /* statements swap the bytes of the */
            /* SETUP packet words to Big Endian so */
            /* they can be compared to other 16-bit */
            /* values elsewhere properly */
            SETUP.wValue.i = SETUP.wValue.c[MSB] + 256*SETUP.wValue.c[LSB];
            SETUP.wIndex.i = SETUP.wIndex.c[MSB] + 256*SETUP.wIndex.c[LSB];
            SETUP.wLength.i = SETUP.wLength.c[MSB] + 256*SETUP.wLength.c[LSB];

            /* Intercept HID class-specific requests */
            if ( (SETUP.bmRequestType & ~0x80) == DSC_HID) {
                switch (SETUP.bRequest) {
                case GET_REPORT:
                    Get_Report ();
                    break;
                case SET_REPORT:
                    Set_Report ();
                    break;
                case GET_IDLE:
                    Get_Idle ();
                    break;
                case SET_IDLE:
                    Set_Idle ();
                    break;
                case GET_PROTOCOL:
                    Get_Protocol ();
                    break;
                case SET_PROTOCOL:
                    Set_Protocol ();
                    break;
                default:
                    Force_Stall ();      /* Send stall to host if invalid */
                    break;                  /* request */
                }
            } else

                switch (SETUP.bRequest)       /* Call correct subroutine to handle */
                {                             /* each kind of standard request */
                case GET_STATUS:
                    Get_Status ();
                    break;
                case CLEAR_FEATURE:
                    Clear_Feature ();
                    break;
                case SET_FEATURE:
                    Set_Feature ();
                    break;
                case SET_ADDRESS:
                    Set_Address ();
                    break;
                case GET_DESCRIPTOR:
                    Get_Descriptor ();
                    break;
                case GET_CONFIGURATION:
                    Get_Configuration ();
                    break;
                case SET_CONFIGURATION:
                    Set_Configuration ();
                    break;
                case GET_INTERFACE:
                    Get_Interface ();
                    break;
                case SET_INTERFACE:
                    Set_Interface ();
                    break;
                default:
                    Force_Stall ();         /* Send stall to host if invalid request */
                    break;
                }
        }
    }

    if (EP_STATUS[0] == EP_TX)          /* See if endpoint should transmit */
    {
        if (!(ControlReg & rbINPRDY) )   /* Don't overwrite last packet */
        {
            /* Read control register */
            POLL_READ_BYTE (E0CSR, ControlReg);

            /* Check to see if SETUP End or Out Packet received, if so do not put */
            /* any new data on FIFO */
            if ((!(ControlReg & rbSUEND)) || (!(ControlReg & rbOPRDY)))
            {
                /* Add In Packet ready flag to E0CSR bitmask */
                ControlReg = rbINPRDY;
                if (DATASIZE >= EP0_PACKET_SIZE)
                {
                    /* Break Data into multiple packets if larger than Max Packet */
                    Fifo_Write_InterruptServiceRoutine (FIFO_EP0, EP0_PACKET_SIZE,
                                                        (unsigned char*)DATAPTR);
                    /* Advance data pointer */
                    DATAPTR  += EP0_PACKET_SIZE;
                    /* Decrement data size */
                    DATASIZE -= EP0_PACKET_SIZE;
                    /* Increment data sent counter */
                    DATASENT += EP0_PACKET_SIZE;
                }
                else
                {
                    /* If data is less than Max Packet size or zero */
                    Fifo_Write_InterruptServiceRoutine (FIFO_EP0, DATASIZE,
                                                        (unsigned char*)DATAPTR);
                    ControlReg |= rbDATAEND;/* Add Data End bit to bitmask */
                    EP_STATUS[0] = EP_IDLE; /* Return EP 0 to idle state */
                }
                if (DATASENT == SETUP.wLength.i)
                {
                    /* This case exists when the host requests an even multiple of */
                    /* your endpoint zero max packet size, and you need to exit */
                    /* transmit mode without sending a zero length packet */
                    ControlReg |= rbDATAEND;/* Add Data End bit to mask */
                    EP_STATUS[0] = EP_IDLE; /* Return EP 0 to idle state */
                }
                /* Write mask to E0CSR */
                POLL_WRITE_BYTE(E0CSR, ControlReg);
            }
        }
    }

    if (EP_STATUS[0] == EP_RX)          /* See if endpoint should transmit */
    {
        /* Read control register */
        POLL_READ_BYTE (E0CSR, ControlReg);
        if (ControlReg & rbOPRDY)        /* Verify packet was received */
        {
            ControlReg = rbSOPRDY;
            if (DATASIZE >= EP0_PACKET_SIZE)
            {
                Fifo_Read(FIFO_EP0, EP0_PACKET_SIZE, (unsigned char*)DATAPTR);
                /* Advance data pointer */
                DATAPTR  += EP0_PACKET_SIZE;
                /* Decrement data size */
                DATASIZE -= EP0_PACKET_SIZE;
                /* Increment data sent counter */
                DATASENT += EP0_PACKET_SIZE;
            }
            else
            {
                /* read bytes from FIFO */
                Fifo_Read (FIFO_EP0, DATASIZE, (unsigned char*) DATAPTR);

                ControlReg |= rbDATAEND;   /* Signal end of data */
                EP_STATUS[0] = EP_IDLE;    /* set Endpoint to IDLE */
            }
            if (DATASENT == SETUP.wLength.i)
            {
                ControlReg |= rbDATAEND;
                EP_STATUS[0] = EP_IDLE;
            }
            /* If EP_RX mode was entered through a SET_REPORT request, */
            /* call the ReportHandler_OUT function and pass the Report */
            /* ID, which is the first by the of DATAPTR's buffer */
            if ( (EP_STATUS[0] == EP_IDLE) && (SETUP.bRequest == SET_REPORT) )
            {
                ReportHandler_OUT (*DATAPTR);
            }

            if (EP_STATUS[0] != EP_STALL) POLL_WRITE_BYTE (E0CSR, ControlReg);
        }
    }

}

/*----------------------------------------------------------------------------- */
/* Handle_In1 */
/*----------------------------------------------------------------------------- */
/* */
/* Handler will be entered after the endpoint's buffer has been */
/* transmitted to the host.  In1_StateMachine is set to Idle, which */
/* signals the foreground routine SendPacket that the Endpoint */
/* is ready to transmit another packet. */
/*----------------------------------------------------------------------------- */
void Handle_In1 ()
{
    EP_STATUS[1] = EP_IDLE;
}

/*----------------------------------------------------------------------------- */
/* Handle_Out1 */
/*----------------------------------------------------------------------------- */
/* Take the received packet from the host off the fifo and put it into */
/* the OUT_PACKET array. */
/* */
/*----------------------------------------------------------------------------- */
void Handle_Out1 ()
{

    unsigned char Count = 0;
    unsigned char ControlReg;

    POLL_WRITE_BYTE (INDEX, 1);         /* Set index to endpoint 2 registers */
    POLL_READ_BYTE (EOUTCSR1, ControlReg);

    if (EP_STATUS[1] == EP_HALT)        /* If endpoint is halted, send a stall */
    {
        POLL_WRITE_BYTE (EOUTCSR1, rbOutSDSTL);
    }

    else                                /* Otherwise read received packet */
        /* from host */
    {
        if (ControlReg & rbOutSTSTL)     /* Clear sent stall bit if last */
            /* packet was a stall */
        {
            POLL_WRITE_BYTE (EOUTCSR1, rbOutCLRDT);
        }

        Setup_OUT_BUFFER ();             /* Configure buffer to save */
        /* received data */
        Fifo_Read(FIFO_EP1, OUT_BUFFER.Length, OUT_BUFFER.Ptr);

        /* Process data according to received Report ID. */
        /* In systems with Report Descriptors that do not define report IDs, */
        /* the host will still format OUT packets with a prefix byte */
        /* of '0x00'. */

        ReportHandler_OUT (OUT_BUFFER.Ptr[0]);

        POLL_WRITE_BYTE (EOUTCSR1, 0);   /* Clear Out Packet ready bit */
    }
}

/*----------------------------------------------------------------------------- */
/* Usb_Suspend */
/*----------------------------------------------------------------------------- */
/* Enter suspend mode after suspend signalling is present on the bus */
/* */
void Usb_Suspend (void)
{
    volatile int k;
    k++;
}

/*----------------------------------------------------------------------------- */
/* Fifo_Read */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value : None */
/* Parameters   : */
/*                1) BYTE addr : target address */
/*                2) unsigned int uNumBytes : number of bytes to unload */
/*                3) BYTE * pData : read data destination */
/* */
/* Read from the selected endpoint FIFO */
/* */
/*----------------------------------------------------------------------------- */
void Fifo_Read (unsigned char addr, unsigned int uNumBytes,
                unsigned char * pData)
{
    int i;

    if (uNumBytes)                      /* Check if >0 bytes requested, */
    {
        USB0ADR = (addr);                /* Set address */
        USB0ADR |= 0xC0;                 /* Set auto-read and initiate */
        /* first read */

        /* Unload <NumBytes> from the selected FIFO */
        for (i=0;i< (uNumBytes);i++)
        {
            while (USB0ADR & 0x80);       /* Wait for BUSY->'0' (data ready) */
            pData[i] = USB0DAT;           /* Copy data byte */
        }

        /*while(USB0ADR & 0x80);         // Wait for BUSY->'0' (data ready) */
        USB0ADR = 0;                     /* Clear auto-read */
    }
}

/*----------------------------------------------------------------------------- */
/* Fifo_Write */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value : None */
/* Parameters   : */
/*                1) BYTE addr : target address */
/*                2) unsigned int uNumBytes : number of bytes to unload */
/*                3) BYTE * pData : location of source data */
/* */
/* Write to the selected endpoint FIFO */
/* */
/* Fifo_Write_Foreground is used for function calls made in the foreground routines, */
/* and Fifo_Write_InterruptServiceRoutine is used for calls made in an ISR. */

/*----------------------------------------------------------------------------- */

void Fifo_Write_Foreground (unsigned char addr, unsigned int uNumBytes,
                            unsigned char * pData)
{
    int i;

    /* If >0 bytes requested, */
    if (uNumBytes)
    {
        while (USB0ADR & 0x80);          /* Wait for BUSY->'0' */
        /* (register available) */
        USB0ADR = (addr);                /* Set address (mask out bits7-6) */

        /* Write <NumBytes> to the selected FIFO */
        for (i=0;i<uNumBytes;i++)
        {
            USB0DAT = pData[i];
            while (USB0ADR & 0x80);       /* Wait for BUSY->'0' (data ready) */
        }
    }
}

void Fifo_Write_InterruptServiceRoutine (unsigned char addr,
        unsigned int uNumBytes,
        unsigned char * pData)
{
    int i;

    /* If >0 bytes requested, */
    if (uNumBytes)
    {
        while (USB0ADR & 0x80);          /* Wait for BUSY->'0' */
        /* (register available) */
        USB0ADR = (addr);                /* Set address (mask out bits7-6) */

        /* Write <NumBytes> to the selected FIFO */
        for (i=0; i<uNumBytes; i++)
        {
            USB0DAT = pData[i];
            while (USB0ADR & 0x80);       /* Wait for BUSY->'0' (data ready) */
        }
    }
}

/*----------------------------------------------------------------------------- */
/* Force_Stall */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value : None */
/* Parameters   : None */
/* */
/* Force a procedural stall to be sent to the host */
/* */
/*----------------------------------------------------------------------------- */

void Force_Stall (void)
{
    POLL_WRITE_BYTE (INDEX, 0);
    POLL_WRITE_BYTE (E0CSR, rbSDSTL);   /* Set the send stall bit */
    EP_STATUS[0] = EP_STALL;            /* Put the endpoint in stall status */
}

/*----------------------------------------------------------------------------- */
/* SendPacket */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value - None */
/* Parameters - Report ID that's used to call the appropriate IN handler */
/* */
/* This function can be called by other routines to force an IN packet */
/* transmit.  It takes as an input the Report ID of the packet to be */
/* transmitted. */
/*----------------------------------------------------------------------------- */

void SendPacket (unsigned char ReportID)
{
    bit EAState;
    unsigned char ControlReg;
    unsigned int timeout = 1000; /* 100 ms */

    /* Guarantee sequential stream compatible to UART implementation */
    IN_BUFFER.Ptr[1] = IN_BUFFER.Length;

    if (EP_STATUS[1] == EP_TX)        /* If endpoint is currently transmitting, */
    {
        while (EP_STATUS[1] == EP_TX && --timeout)
        {
            udelay(100);
        }
        if ( ! timeout)
        {
            CON_print("\n\nSendPacket timed out\n\n");
        }
    }
    ReportID = 0; /* not used */
    EAState = EA;
    EA = 0;

    POLL_WRITE_BYTE (INDEX, 1);         /* Set index to endpoint 1 registers */

    /* Read contol register for EP 1 */
    POLL_READ_BYTE (EINCSR1, ControlReg);

    if (EP_STATUS[1] == EP_HALT)        /* If endpoint is currently halted, */
        /* send a stall */
    {
        POLL_WRITE_BYTE (EINCSR1, rbInSDSTL);
    }
    else if (EP_STATUS[1] == EP_IDLE)
    {
        /* the state will be updated inside the ISR handler */
        EP_STATUS[1] = EP_TX;

        if (ControlReg & rbInSTSTL)      /* Clear sent stall if last */
            /* packet returned a stall */
        {
            POLL_WRITE_BYTE (EINCSR1, rbInCLRDT);
        }

        if (ControlReg & rbInUNDRUN)     /* Clear underrun bit if it was set */
        {
            POLL_WRITE_BYTE (EINCSR1, 0x00);
        }

/*      ReportHandler_IN_Foreground (ReportID); */

        /* Put new data on Fifo */
        Fifo_Write_Foreground (FIFO_EP1, IN_BUFFER.Length,
                               (unsigned char *)IN_BUFFER.Ptr);
        POLL_WRITE_BYTE (EINCSR1, rbInINPRDY);
        /* Set In Packet ready bit, */
    }                                   /* indicating fresh data on FIFO 1 */

    EA = EAState;
}
