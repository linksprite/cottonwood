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
  * @brief This file includes all functionality to use some hardware timers
  *
  * @author Ulrich Herrmann
  */

#include "c8051F340.h"
#include "as399x_config.h"
#include "timer.h"
#include "global.h"

#if (CLK == 48000000)
#define TICKS_PER_US 4UL
#elif (CLK == 24000000)
#define TICKS_PER_US 2UL
#elif (CLK == 12000000)
#define TICKS_PER_US 1UL
#else 
#error CLK not supported
#endif


void mdelay( u16 ms )
{
    while (ms --)
    {
        udelay(1000);
    }
}

void timerStart_us( u16 us )
{
    u16 loadVal;

    loadVal = 0xffffUL - (us * TICKS_PER_US);

    CKCON     &= ~0x40;     /* SYSSCLK/12 */
    TMR3CN    = 0x00;      /* SYSSCLK/12 */

    TMR3RLL   = loadVal & 0xff;         /*Reloadregister */
    TMR3RLH   = (loadVal >> 8) & 0xff;  /*Reloadregister */
    TMR3L     = loadVal & 0xff;
    TMR3H     = (loadVal >> 8) & 0xff;

    TMR3CN    |= 0x04;     /* start timer */
}

void udelay( u16 us )
{
    timerStart_us(us);
    while(!TIMER_IS_DONE());
}

void timerStartMeasure( )
{
    CKCON     |= 0x04;     /* SYSCLK for Timer 0*/
    TCON      &= ~0x10;    /* Stop timer 0 */
    PCA0CN    &= ~0x40;    /* Stop PCA */

    TMOD       = (TMOD & 0xf0) | 0x01; /* 16-bit mode, running on sysclock */
    TL0        = 0;
    TH0        = 0;

    PCA0MD     = 0x04;     /* select timer 0 overflow */ 
    PCA0CN     = 0x40;     /* Start PCA */
    PCA0L      = 0;
    PCA0H      = 0;

    TCON      |= 0x10;     /* Start timer 0 */

}
u16 timerMeasure_slowTicks( )
{
    u16 vall,valh;
    do
    {
        valh = PCA0H;
        vall = PCA0L;
    } while ( valh != PCA0H);
    return (valh<<8)|vall;
}
