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
/*! \file
 *
 *  \author B. Breinbauer
 *
 *   \brief This file provides platform (board) specific macros and declarations
 *
 *   Contains configuration macros which are used throughout the code base,
 *   to enable/disable board specific features.
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "c8051F340.h"
#include <intrins.h>
#include "global.h"
#include "as399x_config.h"

/** Macro for enable external IRQ */
#define ENEXTIRQ()                {EX0 = 1;}

/** Macro for disable external IRQ */
#define DISEXTIRQ()               EX0 = 0

/** Macro setting data output port */
#define SETOUTPORT(x)             DATAOUTPORT=(x)

/** Macro change port direction  */
#define DPORTDIR(x)               P1MDOUT=(x);


/** Macro for setting enable pin */
#if ROLAND
#define EN(x)                     P4 = P4 & 0xfd | ((x)<<1) //P4 is not bit addressable
#else
#define EN(x)                     ENABLE=(x)
#endif

/** Macro for enabling LDO which powers AS399x (not available on all boards) */
#if ARNIE
#define EN_LDO_AS399x(x)          P1_1=(x)      //ARNIE has an LDO for the AS399x
#else
#define EN_LDO_AS399x(x)          ;             //do nothing for other boards
#endif

/** Macro for setting DCDC on/off */
#define DCDC(x)                   DCDCPIN=(x)

/** Macro for setting PA on/off */
#define EN_PA(x)                  EN_PAPIN=(x)

/** Macro for setting IO2 pin, for direct mode */
#define IO2(x)                    IO2PIN=(x)

/** Macro for setting IO3 pin, for direct mode */
#define IO3(x)                    IO3PIN=(x)

/** Macro for setting IO4 pin, serial enable line */
#if ROLAND
#define IO4(x)                    P4=P4 & 0xfb | ((x)<<2)
#elif ARNIE
#define IO4(x)                    P2_5=(x)
#else
#define IO4(x)                    P1_4=(x)
#endif

#define SEN(x)                    IO4(x)

/** Definition for the external IRQ pin */
#define IRQPIN                    P0_3

#if ARNIE
#define LED1PIN                   P0_6
#define LED1(x)                   LED1PIN=x
#else
/** Definition for the LED 1 (LEO) */
#define LED1PIN                   P2_2
#define LED1(x)                   LED1PIN=x
#endif

#define LED2 0x01;
#define SETLED2                   P4 |= LED2;
#define CLRLED2                   P4 &=~LED2;

#define LED3 0x02;
#define SETLED3                   P4 |= LED3;
#define CLRLED3                   P4 &=~LED3;

#ifdef CONFIG_TUNER
#define SEN_TUNER_CIN(x)          SEN_CINPIN=(x)
#define SEN_TUNER_CLEN(x)         SEN_CLENPIN=(x)
#define SEN_TUNER_COUT(x)         SEN_COUTPIN=(x)
#endif




/*------------------------------------------------------------------------- */
/* Not to be used directly, use the macros at the top of this file          */
/*------------------------------------------------------------------------- */

#if ROLAND
/** Definition for the enable pin */
#define ENABLE                    P4_1
/** Definition for the DCDC enable pin */
#define DCDCPIN                   P0_7

#elif ARNIE
/** Definition for the enable pin */
#define ENABLE                    P3_3
/** Definition for the DCDC enable pin */
#define DCDCPIN                   P1_0
/** Definition for the Direct data mode Pin*/
#define IO3PIN                    P2_6
/** Definition for the Direct data mode Pin*/
#define IO2PIN                    P2_7
/** Definition for the Direct mode data bitclock Pin*/
#define IO5PIN                    P2_4
/** Definition for the Direct mode data Pin*/
#define IO6PIN                    P0_1

#else
/** Definition for the enable pin */
#define ENABLE                    P2_1
/** Definition for the DCDC enable pin */
#define DCDCPIN                   P0_2
/** Definition for the PA enable Pin */
#define EN_PAPIN                  P3_0
/** Definition for the Direct data mode Pin*/
#define IO3PIN                    P1_3
/** Definition for the Direct data mode Pin*/
#define IO2PIN                    P1_2
/** Definition for the Direct mode data bitclock Pin*/
#define IO5PIN                    P1_5
/** Definition for the Direct mode data Pin*/
#if MICROREADER
#define IO6PIN                    P0_1
#else
#define IO6PIN                    P1_6

#endif


#endif

#if COMMUNICATION_SERIAL

#else
/** Definition for the 8 bit (parallel) data outport */
#define DATAOUTPORT               P1
/** Definition for the 8 bit (parallel) data input port */
#define DATAINPORT                P1
/** Definition for the clk pin of the for the parallel interface */
#define CLKPIN                    P2_0
/** Definition for the uC CLK Input pin */
#define SYSCLKIN                  P0_7
#endif

#if ROLAND
/** Antenna switch is used to toggle between horizontal and vertical polarisation */
#define SWITCH_ANTENNA(X)  P1 = P1 & 0xf3 | (((X)&0x3)<<2)
#define SWITCH_ANT_P1               0x1
#define SWITCH_ANT_P2               0x2

/** The switch just after PA switches between normal (tuner enabled) operation and
  power detector */
#define SWITCH_POST_PA(X)  P1 = P1 & 0xcf | (X)
#define SWITCH_POST_PA_TUNING       0x20
#define SWITCH_POST_PA_PD           0x10

/** Definition for the PA enable Pin */
#define EN_BIAS                  P3_2
#define EN_LDO                   P0_6
#define EN_PAPIN                 EN_LDO
#endif

#if ARNIE
/** Antenna switch is used to toggle between horizontal and vertical polarisation */
#define SWITCH_ANTENNA(X)  P1 = P1 & 0x3F | (((X)&0x3)<<6)
#define SWITCH_ANT_P1               0x1
#define SWITCH_ANT_P2               0x2

/** Definition for the PA enable Pin */
#define EN_BIAS                  P2_0
#define EN_LDO                   P2_1
#define EN_PAPIN                 EN_LDO

/* Definition for the split power setup */
#define EXT_SUPPLY               P3_4
#define VDD_IO                   P2_2
#endif

#endif

#ifdef CONFIG_TUNER
#if ROLAND
#define SEN_CINPIN               P2_1
#define SEN_CLENPIN              P1_7
#define SEN_COUTPIN              P1_6
#elif ARNIE
#define SEN_CINPIN               P1_3
#define SEN_CLENPIN              P1_4
#define SEN_COUTPIN              P1_5
#endif


#endif /* PLATFORM_H_ */
