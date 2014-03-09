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
 *  \author C. Eisendle
 *  \author T. Luecker (Substitute)
 *
 *  \brief Bitbang engine
 *
 *  The bitbang module is needed to support serial protocols via bitbang technique
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "c8051F340.h"
#include "as399x_config.h"
#include "timer.h"
#include "bitbang.h"

#if ISO6B

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
static IDATA volatile u8 bbLength_;
static IDATA volatile u8 bbCount_;
static IDATA volatile t_bbData *bbData_;

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void bbSetup(s16 microsec)
{
    s16 cnt;

    cnt = 0xffff;
    cnt -= ((TIMER2_TICK * microsec) & 0xffff);
    TMR2CN = 0x00;     /* disable the timer, configure the timer for normal 16bit reload operation */
    CKCON |= (1 << 4) | (1 << 5); /* low and high byte count register are clocked by SYSCLK */
    TMR2L = TMR2RLL = cnt & 0xff;
    TMR2H = TMR2RLH = (cnt & 0xff00) >> 8;
}

void bbRun(t_bbData *bbdata, s16 length)
{
    s8 bbFlags;

    bbData_ = bbdata;
    bbCount_ = 0;
    bbLength_ = length;
    TMR2CN = 0x04;
    bbFlags = IE; /* save enabled interrupts */
    IE = 0x20;    /* enable timer 2 interrupt but disable all other interrupts */
    EA = 1;
    while (bbLength_);
    while (!TF2H);  /* wait an extra clock cycle */
    IE = bbFlags;    /* restore interrupts */
    TMR2CN = 0x0;  /* stop the timer */
}

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
static void bbInterrupt(void) interrupt 5 /* in fact timer 2 interrupt */
{
    s8 workdata;
    if (bbLength_)
    {
        TF2H = 0; /* reset the Timer overflow Flag */
        workdata = (*bbData_).bbdata >> --(*bbData_).length;
        if (!(*bbData_).length)
        {
            bbData_ ++;
            bbLength_ --;
        }
        BB_PIN = workdata & 1; /* shift out the data */
    }
}

#endif
