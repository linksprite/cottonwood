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
  * @brief This file is the include file for the timer.c file.
  *
  * @author Ulrich Herrmann
  */

#ifndef __TIMER_H__
#define __TIMER_H__
#include "global.h"

/*!
  Poll for timer hit. Use in conjunction with timerStart_us().
*/
#define TIMER_IS_DONE() (TMR3CN & 0x80)

#if (CLK == 48000000)
#define SLOWTICKS_2_MS( TICKS ) (((TICKS)>1000)?((((TICKS)+24)/48)*65):(((TICKS)*65)/48)) 
#define MS_2_SLOWTICKS( MS    ) (((MS   )>1000)?((((MS   )+32)>>6)*48):(((MS   )*48)>>6))
#elif (CLK == 24000000)
#elif (CLK == 12000000)
#else 
#error CLK not supported
#endif

void timerStart_ms( u16 ms );

/*!
  Delay execution for ms milliseconds.

  \param ms : milliseconds till return. Works only up to ~4 seconds. 
*/
void mdelay( u16 ms );

/*!
  Start a timer which generates an interrupt after us microseconds. Use
  TIMER_IS_DONE() to poll expiration. No interrupt is being used, make
  sure no ISR clears the interrupt bit.

  \param us : microseconds until expiration.
*/
void timerStart_us( u16 us );

/*!
  Delay execution for us microseconds.

  \param us : microseconds till return.
*/
void udelay( u16 us );

/*! 
  Start measurement using timer0 and PCA
  */
void timerStartMeasure( );

/*! 
  Measure time passed since last call of timerStartMeasure(). Resolution is below 1 ms.
  Value returned is in slow ticks. Use SLOWTICKS_2_MS() and MS_2_SLOWTICKS() to convert.
  */
u16 timerMeasure_slowTicks( );
#endif
