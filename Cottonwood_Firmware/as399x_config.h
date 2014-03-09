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
 *  \author U.Herrmann ( based on work by E.Grubmueller
 *  \author T. Luecker (Substitute)
 *
 *   \brief Configuration file for all AS99x firmware
 *
 *   Do configuration of the driver in here.
 */

#ifndef _AS399X_CONFIG_H
#define _AS399X_CONFIG_H
#include <intrins.h>

/***************************************************************************/
/********************** configuration area *********************************/
/***************************************************************************/

#define XDATA xdata /*!< xdata is not available on all platforms, thus have a define for it */
#define IDATA idata /*!< idata is not available on all platforms, thus have a define for it */
#define CODE code /*!< code is not available on all platforms, thus have a define for it */
#define DATA data /*!< data is not available on all platforms, thus have a define for it */

/** Define this to 1 if you have a LEO board */
#define LEO 1
/** Define this to 1 if you have a PICO board */
#define PICO 0
/** Define this to 1 if you have a ROGER board */
#define ROGER 0
/** Define this to 1 if you have a ROLAND board */
#define ROLAND 0
/** Define this to 1 if you have a "Micro Reader/ UHF RFID Controller" board */
#define MICROREADER 0
/** Define this to 1 if you have a Arnie board */
#define ARNIE 0

/** Define to 1 if CRC16 should be used for iso6b */
#define CRC16_ENABLE 1

/** Define this to 1 if AS3992 should be supported, software will cease to run on AS3990/1 */
#define RUN_ON_AS3992 1

/** Define this to 1 if UART should be used for communicating with host */
#define UARTSUPPORT    0

/** Define this to 1 if as399xInitialize() should perform a proper selftest, 
  testing connection AS399x, crystal, pll, antenna */
#define AS399X_DO_SELFTEST 1

#define VERBOSE_INIT   0

/** Serial (SPI) communication with AS399x (if set to 0 parallel interface is used) */
#define COMMUNICATION_SERIAL 0

/** Set this to 1 to enable iso6b support */
#define ISO6B 1

/** Set to one if an antenna tuner is available */
#if ROLAND || ARNIE
#define CONFIG_TUNER   1
#endif

/** Set to one if a power detector is available on the board and the PA is biased with
 * the DAC of AS399x. */
#if ROLAND || ARNIE
#define POWER_DETECTOR   1
#endif

/** Set to one if a switch to switch between antenna ports is available */
#if ROLAND || ARNIE
#define ANTENNA_SWITCH   1
#endif

/***************************************************************************/
/******************** private definitions, not to be changed ***************/
/***************************************************************************/

#if !LEO && !PICO && !ROGER && !ROLAND && !MICROREADER && !ARNIE
#error unknown board
#endif

/* typedefs for easier portability */
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32;
typedef signed long s32;
/** USE WITH CARE!!! unsigned machine word : 8 bit on 8bit machines, 16 bit on 16 bit machines... */
typedef unsigned char umword;
/** USE WITH CARE!!! signed machine word : 8 bit on 8bit machines, 16 bit on 16 bit machines... */
typedef signed char smword;
/** type for boolean variables, should normally be natural machine size */
typedef umword bool;
/** type for integer types, useful as indices for arrays and loop variables */
typedef unsigned int uint;
/** type for integer types, useful as indices for arrays and loop variables */
typedef signed int sint;

#if RUN_ON_AS3992
#define CHIP                "AS3992"
#else
#define CHIP                "AS3991"
#endif

#define FIRMWARE_ID         CHIP" Reader Firmware 2.2.9\n"

#if LEO
#define HARDWARE_ID         CHIP" LEO Reader Hardware 1.3\n"
#endif
#if PICO
#define HARDWARE_ID         CHIP" PICO Reader Hardware 1.3\n"
#endif
#if ROGER
#define HARDWARE_ID         CHIP" ROGER Reader Hardware 1.2\n"
#endif
#if MICROREADER
#define HARDWARE_ID         CHIP" MICRO Reader Hardware 1.2\n"
#endif
#if ROLAND
#define HARDWARE_ID         CHIP" ROLAND Reader Hardware 1.0\n"
#endif
#if ARNIE
#define HARDWARE_ID         CHIP" ARNIE Reader Hardware 2.4\n"
#endif

#define WRONG_CHIP_ID       "caution wrong chip\n"

#if LEO || PICO
#define INTVCO
#elif ROGER
#define INTVCO
#elif ROLAND
#define INTVCO
#elif MICROREADER
#define INTVCO
#elif ARNIE
#define INTVCO
#endif

#if LEO || PICO
#define INTPA
#elif ROGER
#define EXTPA
#elif ROLAND
#define EXTPA
#elif MICROREADER
#define EXTPA
#elif ARNIE
#define EXTPA
#endif

#if LEO || PICO
#define LOWSUPPLY
#define SINGLEINP
#elif ROGER
#define BALANCEDINP
#elif ROLAND
#define LOWSUPPLY
#define BALANCEDINP
#elif MICROREADER
#define SINGLEINP
#elif ARNIE
#define BALANCEDINP
#endif

#define AS3990 0
#define AS3991 1
#define AS3992 2

/*!
  \mainpage
  \section Layering
  This software is layered as follows:

  <table cellpadding=20 cellspacing=10 rules="none">
  <tr>
      <td align="center" colspan=3 bgcolor="#FCCE9C">application code</td>
      <td>Upper layer in which the application run. In this example it is
      the \link usb_commands.c usb_commands.c\endlink</td>
  </tr>
  <tr>
      <td align="center" bgcolor="#04FEFC">\link gen2.h gen2\endlink</td>
      <td align="center" bgcolor="89C2FC">\link iso6b.h iso6b\endlink</td>
      <td align="center" bgcolor="CCFECC">\link as399x_public.h as399x_public\endlink</td>
      <td>Protocol layer in which the execution is being done</td>
  </tr>
  <tr>
      <td align="center" colspan=3 bgcolor="CCFECC">\link as399x.h as399x\endlink</td>
      <td>Device specific procedures of UHF Reader</td>
  </tr>
  <tr>
      <td  align="center" bgcolor="FCFE9C" colspan=3>\link as399x_com.h interface\endlink</td>
      <td>Interface layer for the communication with the UHF Reader
      chip</td>
  </tr>
  </table>

  The files gen2.h and iso6b.h contain the interface provided to the
  user for the two supported protocols. They rely on the interface provide
  by as399x.h. This in turn relies on the actual hardware communication
  interface provided by as399x_com.h.

  \section Configuration
  Configuration of this software is done in as399x_config.h.

  \section Using
  Before using any other functions the AS399X needs to be initialized
  using as399xInitialize(). Then functions from gen2.h and iso6b.h can
  be called.

  \section Porting
  The developer which needs to port this firmware to another hardware will need
  to change parallelinterface.c resp. serialinterface.c and platform.h. Additionally
  he should also provide a proper interrupt service routine (currently
  extInt() in as399x.c). Some settings need to be changed also in
  as399x_config.h.

*/
#endif
