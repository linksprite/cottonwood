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
 *  \brief Bitbang module header file
 *
 *  The bitbang module is needed to support serial protocols via bitbang technique
 */

#ifndef BITBANG_H
#define BITBANG_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "global.h"
#include "platform.h"

/*
******************************************************************************
* GLOBAL DATATYPES
******************************************************************************
*/
/*!
 * Bitbang data structure
 */
typedef struct {
    u8 bbdata; /*!< The data to be bitbang'ed */
    s8 length; /*!< Length of the data, min. 1, max. 8 */
}t_bbData;

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
#define TIMER2_TICK   (CLK / 1000000) /* one tick per us */
#define BB_PIN IO3PIN /*!< specifies the pin where the data will be shifted out */

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/
/*!
 *****************************************************************************
 *  \brief  Setup the bitbang module
 *
 *  This function needs to be called prior to the use of the module.
 *  It configures the timer #2 by setting the period time.
 *  Period time is (1/freq)/2, e.g. freq = 40khz --> 12.5us
 *
 *  \param  microsec : The period time in microseconds
 *
 *****************************************************************************
 */
void bbSetup(s16 microsec);

/*!
 *****************************************************************************
 *  \brief  Start the bitbang engine
 *
 *  When calling this function the bitbang engine is started. In fact,
 *  timer 2 will be enabled and at each overrun the next bit is shifted out.
 *  NOTE: All other interrupts are DISABLED during this function in order
 *  to allow exact timings.
 *
 *  \param  bbdata : Array of bitbang data structs. The module can process
 *                   more bitbang structures. So it's possible to setup
 *                   a complete transfer without interruption.
 *  \param  length : Length of the array
 *
 *****************************************************************************
 */
void bbRun(t_bbData *bbdata, s16 length);

#endif /* BITBANG_H */
