/*
 *****************************************************************************
 * Copyright by ams AG                                                       *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************
 */
/** @file
  * @brief Implementation of parallel interface communication with AS399x.
  *
  * The file provides functions to initialize the uController and the board
  * for communication via the parallel interface with AS399x. The function writeReadAS399x() provides
  * communication functionality. The function writeReadAS399xIsr() does the same, but is
  * only called from the ISR. On the SiLabs uController a dedicated function for
  * the ISR improves runtime performance (no reentrant function necessary).
  * \n
  * For porting to another uController/board the functions in this file have to be
  * adapted.
  *
  * @author Ulrich Herrmann
  * @author Bernhard Breinbauer
  */

#include "c8051F340.h"
#include "as399x_config.h"
#include "platform.h"
#include "global.h"
#include "uart.h"
#include "timer.h"
#include "as399x_com.h"

#define NOPDELAY(x)               { int i; for(i = 0; i < (x); i++) { NOP(); }}

/** Macro for setting CLK pin */
#define CLOCK(x)                  CLKPIN=(x)

/** Macro change port direction to write */
#define DPORTDIRWR()              {DPORTDIR(0xFF);}

/** Macro change port direction to read */
#define DPORTDIRRD()              {DPORTDIR(0x00); SETOUTPORT(0xff);}

/** Macro read from data input port */
#define GETPORT()                 DATAINPORT

/** Macro for protocol start condition */
#define STARTCONDITION()          {CLOCK(LOW); SETOUTPORT(LOW); CLOCK(HIGH);\
                                   SETOUTPORT(0x80); NOP(); NOP(); CLOCK(LOW);}
/** Macro for protocol simple or single stop condition */
#define STOPSGL()                 {SETOUTPORT(0x80); NOP();\
                                   CLOCK(HIGH); SETOUTPORT(LOW); CLOCK(LOW);}
/** Macro for protocol continous stop condition */
#define STOPCONT()                {CLOCK(LOW); SETOUTPORT(LOW); NOP(); SETOUTPORT(0x80);\
                                   NOP(); NOP(); SETOUTPORT(LOW);}
/** Macro for ending the Direct mode */
#define STOPDIR()                 {SETOUTPORT(0x80); NOP(); NOP();\
                                   CLOCK(HIGH); NOP(); SETOUTPORT(LOW); NOP(); CLOCK(LOW);}

/*------------------------------------------------------------------------- */
/** This function initialising the dataport for comunication with the
  * AS3990. \n
  * This function does not take or return a parameter.
  */
static void initDataInOutPort(void)
{
    /* P1.0  -  Unassigned,  Open Drain,  Digital */
    /* P1.1  -  Unassigned,  Open Drain,  Digital */
    /* P1.2  -  Unassigned,  Open Drain,  Digital */
    /* P1.3  -  Unassigned,  Open Drain,  Digital */
    /* P1.4  -  Unassigned,  Open Drain,  Digital */
    /* P1.5  -  Unassigned,  Open Drain,  Digital */
    /* P1.6  -  Unassigned,  Open Drain,  Digital */
    /* P1.7  -  Unassigned,  Open Drain,  Digital */
    /* P2.0  -  Unassigned,  Open Drain,  Digital */
    /* P2.1  -  Unassigned,  Open Drain,  Digital */

    /*  P1MDOUT   |= 0xFF; */
    EN(LOW);                     /*Enable = 0 */
    P2MDOUT   |= 0x07;           /*Push Pull for Enable and CLK Output and DEBUG PIN */
    XBR1      |= 0x40;           /*Enable Crossbar*/
}

/*------------------------------------------------------------------------- */
/** This function initialising the external interrupt for comunication with the
  * AS3990. \n
  * This function does not take or return a parameter.
  */
static void initExtInterrupt(void )
{
    SYSCLKIN = 1;        /*Get Portpin in "tristate", reqires proper P0MDOUT bit to be 0 */
    IRQPIN = 1;          /*Get Portpin in "tristate", reqires proper P0MDOUT bit to be 0 */
    IT01CF = 0x0B;       /*Interrupt active high and use P0.3 for Interrupt Input Pin */
    IT0 = 1;             /*rising edge triggered */
    EX0 = 1;             /*Enable external interrupts */
    EA = 1;              /*Enable all interrupts */
}

/*------------------------------------------------------------------------- */
void initInterface(void)
{
#if ROGER || MICROREADER
    P0MDOUT |= 0x04;     /* Set P0_2 (DCDC enable) to output */
    P3MDOUT |= 0x01;     /* Set P3_0 (PA  enable) to output */
    DCDC(1);             /* Disable DCDC */
    EN_PA(0);            /* Disable PA */
#endif
    initDataInOutPort();
    initExtInterrupt();
    SETOUTPORT(LOW);        /*all bits high or low while rising edge on EN */
    CLOCK(LOW);
    EN(HIGH);               /*Enable AS3990 Interface */
}

/*------------------------------------------------------------------------- */
void writeReadAS399x( const u8* wbuf, u8 wlen, u8* rbuf, u8 rlen, u8 stopMode, u8 doStart )
{
    if (wlen)
    {
        DPORTDIRWR();                         /*Changes Portdirection to Write */
        if (doStart) STARTCONDITION();
        while (wlen--)
        {
            SETOUTPORT(*wbuf); /*writes address */
            NOP();
            CLOCK(HIGH);
            NOP();
            NOP();
            NOP();
            CLOCK(LOW);
            wbuf++;
        }
    }
    if (rlen)
    {
        DPORTDIRRD();                         /*Changes Portdirection to Read */
        while (rlen--)
        {
            SETOUTPORT(0xFF);
            CLOCK(HIGH);                        /*read data */
            NOP();                              /*No Operation */
            *rbuf = GETPORT();
            CLOCK(LOW);
            rbuf++;
        }
    }
    DPORTDIRWR();
    if ( stopMode == STOP_CONT) STOPCONT();  /*Stopcondition in Continous Mode */
    if ( stopMode == STOP_SGL ) STOPSGL();   /*Stopcondition in Single Mode */
}

/*------------------------------------------------------------------------- */
void writeReadAS399xIsr( const u8 wbuf, u8* rbuf )
{
    /* The NOPDELAY() macros should originally provide a convenient way of producing a
     * few nops in a sequence. But actually the macros produce much a bigger delay, because
     * the loop index is put into XDATA by the compiler and the fetching/storing from
     * XDATA produces quite some overhead. Nevertheless we keep the macros as the
     * parallel interface needs some delays and I do not want to put huge sequences
     * of NOP() into the function.
     */
    NOPDELAY(1);
    DPORTDIRWR();                         /*Changes Portdirection to Write */
    NOPDELAY(1);
    STARTCONDITION();
    NOPDELAY(1);
    SETOUTPORT(wbuf); /*writes address */
    NOPDELAY(1);
    CLOCK(HIGH);
    NOPDELAY(1);
    CLOCK(LOW);
    NOPDELAY(1);

    NOPDELAY(1);
    DPORTDIRRD();                         /*Changes Portdirection to Read */
    NOPDELAY(1);
    SETOUTPORT(0xFF);
    CLOCK(HIGH);                        /*read data */
    NOPDELAY(1);                        /*No Operation */
    *rbuf = GETPORT();
    CLOCK(LOW);
    NOPDELAY(1);

    DPORTDIRWR();
    NOPDELAY(1);
    STOPSGL();   /*Stopcondition in Single Mode */
}


void setPortDirect()
{
    DPORTDIR (0x9F); /* IO6 and IO5 inputs for direct mode */
    IO5PIN = 1;
    IO6PIN = 1;
}

void setPortNormal()
{
    DPORTDIRWR();  /* whole port is output */
    STOPDIR(); /* end the direct mode now. */
}
