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
  * @brief Implementation of serial interface communication with AS399x.
  *
  * The file provides functions to initialize the uController and the board
  * for SPI communication with AS399x. The function writeReadAS399x() provides
  * communication functionality. The function writeReadAS399xIsr() does the same, but is
  * only called from the ISR. On the SiLabs uController a dedicated function for
  * the ISR improves runtime performance (no reentrant function necessary).
  * \n
  * For porting to another uController/board the functions in this file have to be
  * adapted.
  * \n\n
  * For serial communication on LEO boards a patch is required. The patch does the following:
    <ul>
        <li> P0_0(SPI SCK) - P2_0 - CLK(AS399X)
        <li> P0_1(SPI MISO) - P1_6 - IO6(AS399X)
        <li> P0_2(SPI MOSI) - P1_7 - IO7(AS399X)
    </ul>
    Therefore P1_6, P1_7, P2_0, P0_1 need to be configured input (open drain).
    And P0_0 and P0_2 need to be configured output (push pull).
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


/*------------------------------------------------------------------------- */
/** This function initialising the external interrupt for comunication with the
  * AS399x. \n
  * This function does not take or return a parameter.
  */
static void initExtInterrupt(void )
{
    IRQPIN = 1;          /*Get Portpin in "tristate" */
    IT01CF = 0x0B;       /*Interrupt active high and use P0.3 for Interrupt Input Pin */
    IT0 = 1;             /*rising edge triggered */
    EX0 = 1;             /*Enable external interrupts */
    EA = 1;              /*Enable all interrupts */
}

/*------------------------------------------------------------------------- */
/** This function initializes the in/out ports of the controller for communication
 * with AS399x via SPI. \n
 * This function has to be adapted for new boards/controllers/...
 */
void initInterface(void)
{

#if ROGER || MICROREADER
    P0MDOUT |= 0x04;     /* Set P0_2 (DCDC enable) to output */
    P3MDOUT |= 0x01;     /* Set P3_0 (PA  enable) to output */
    DCDC(1);             /* Disable DCDC */
    EN_PA(0);            /* Disable PA */
#endif
#if ROLAND
    P0MDOUT = 0xd5; /* IO4,7 and CLK output, IRQ, IO6, uart rx, gpio1 input */
    P0 = (~P0MDOUT) | 0xc0; /* enable LDO and DCDC */
    P1MDOUT = 0xfc;
    P1 = (~P1MDOUT) | 0xe4; /* set SW_ANT_P2, SW_RSSI, SEN_TUNE_3, SEN_TUNE_2 */
    P2MDOUT = 0x06;
    P2 = (~P2MDOUT) | 0x6;
    P3MDOUT = 0x04;
    P3 = (~P3MDOUT);
    P4MDOUT = 0x06;
    P4 = (~P4MDOUT);
#endif
#if ARNIE
    P0MDOUT = 0x55; /* IO4,7, LED and CLK output; IRQ, IO6, uart rx, clksys input */
    P0 = (~P0MDOUT) | 0x40;
    P1MDOUT = 0xFF; /* all outputs */
    P1 = (~P1MDOUT) | 0x43;     /* disable tune caps and switch on antenna P1 */
    P2MDOUT = 0xE7;             /* all outputs except IO5 */
    P2 = (~P2MDOUT) | 0x16;     /* enable PA_LDO and VDD_IO, set IO5 as input */
    P3MDOUT = 0x0B;
    P3 = (~P3MDOUT) | 0x01;    /* IO1=1, IO0=0, enable of AS399x set to high later (as399xInitialize) */
    P4MDOUT = 0x00;
    P4 = (~P4MDOUT);
#endif

    SPI0CFG = 0x60;/*  MSTEN=1, CPHA=1, CKPOL=0, */
#if (CLK == 48000000) /* reg = f_sys/(2f) - 1 */
    SPI0CKR = 48/(2*2)-1; /* 2 MHz */
#elif (CLK == 24000000)
    SPI0CKR = 24/(2*2)-1; /* 2 MHz */
#elif (CLK == 12000000)
    SPI0CKR = 12/(2*2)-1; /* 2 MHz */
#endif
    SPI0CN  = 1; /* 3-wire, enable SPI */
    initExtInterrupt();

#if (LEO || PICO || ROGER || MICROREADER)
    P0MDOUT   |= 0x05;   /* Push pull for MOSI and SCK */
    P2MDOUT   |= 0x06;  /*Push Pull for Enable and DEBUG PIN */
    DPORTDIR(0x1F);     /*Push Pull for complete data bus, excluding IO5, IO6 and IO7 */
    /* Writing to open-drain port a 1 enables high-impedance! */
    P2_0 = 1;  /* CLK pin high-impedance */
#endif

    XBR0 |= 0x2;  /* enable SPI interface */
    XBR1 |= 0xC0; /* enable the crossbar, disable weak pull ups  */

#if (LEO || PICO || ROGER || MICROREADER)
    P1_0 = 0;   /* Set IO0 to vss and */
    P1_1 = 1;   /*     IO1 to vdd to force to SPI mode with following enable toggling */
#endif
    IO4(1); /* SPI cs_ */

#if 0
    CON_print("P0MDOUT = %hhx, P0 = %hhx\n",P0MDOUT,P0);
    CON_print("P1MDOUT = %hhx, P1 = %hhx\n",P1MDOUT,P1);
    CON_print("P2MDOUT = %hhx, P2 = %hhx\n",P2MDOUT,P2);
    CON_print("XBR0 = %hhx\n",XBR0);
    CON_print("XBR1 = %hhx\n",XBR1);
    CON_print("XBR2 = %hhx\n",XBR2);
#endif

    EN(LOW); 
    udelay(100);
    EN(HIGH); /* Now we should be in SPI mode */
    return;
}

/*------------------------------------------------------------------------- */
void writeReadAS399x( const u8* wbuf, u8 wlen, u8* rbuf, u8 rlen, u8 stopMode, u8 doStart )
{
    u8 i;
    u8 d;

    if ( doStart) IO4(0);

    for ( i = 0; i< wlen; i++ )
    {
        SPI0DAT =  wbuf[i];
        while(!(SPI0CN & 0x80));
        SPI0CN = 1; /* Clear the interrupt flag */
        d = SPI0DAT; /* dummy receive this byte */
    }

    for ( i = 0; i< rlen; i++ )
    {
        SPI0DAT = 0;
        while(!(SPI0CN & 0x80));
        SPI0CN = 1; /* Clear the interrupt flag */
        rbuf[i] = SPI0DAT; /* receive this byte */
    }

    if ( stopMode != STOP_NONE) IO4(1);
}

/*------------------------------------------------------------------------- */
void writeReadAS399xIsr( const u8 wbuf, u8* rbuf )
{

    IO4(0);     // doStart

    SPI0DAT =  wbuf;
    while(!(SPI0CN & 0x80));
    SPI0CN = 1; /* Clear the interrupt flag */
    *rbuf = SPI0DAT; /* dummy receive this byte */

    SPI0DAT = 0;
    while(!(SPI0CN & 0x80));
    SPI0CN = 1; /* Clear the interrupt flag */
    *rbuf = SPI0DAT; /* receive this byte */

    IO4(1);     // doStop
}


void setPortDirect()
{
    IO4(0);
}

void setPortNormal()
{
    IO4(1);
}
