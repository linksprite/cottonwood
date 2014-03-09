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
  *
  * @brief This file provides macros and declarations for the interface to the AS399x chip.
  *
  * The implementation of the functions is in parallelinterface.c or serialinterface.c
  * respectively. Depending on the configuration define in as399x_config.h
  *
  * @author Ulrich Herrmann
  * @author Bernhard Breinbauer
  */

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include "c8051F340.h"
#include <intrins.h>
#include "global.h"

/** define for stopMode parameter of writeReadAS399x() */
#define STOP_NONE 0
/** define for stopMode parameter of writeReadAS399x() */
#define STOP_SGL  1
/** define for stopMode parameter of writeReadAS399x() */
#define STOP_CONT 2

/*------------------------------------------------------------------------- */
/** This function initializes uController/board for
  * comunication with the AS399x.
  *
  * This function does not take or return a parameter.
  */
void initInterface(void);

/*------------------------------------------------------------------------- */
/** This function talks with the AS399x chip.
  */
void writeReadAS399x( const u8* wbuf, u8 wlen, u8* rbuf, u8 rlen, u8 stopMode, u8 doStart );

/** This function talks with the AS399x chip from ISR.
  */
void writeReadAS399xIsr( const u8 wbuf, u8* rbuf );

/*------------------------------------------------------------------------- */
/** This function sets the interface to the AS399X for accessing it via 
  * direct mode. IO3() macro needs to be used to generate TX signals
  */
void setPortDirect();

/*------------------------------------------------------------------------- */
/** This function sets the interface to the AS399X for accessing it via 
  * normal (parallel) mode using writeReadAS399x()
  */
void setPortNormal();

#endif

