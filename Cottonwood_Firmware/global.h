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
  * @brief This file provides declarations for global helper functions.
  *
  * @author Ulrich Herrmann
  */

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

/** Defines uC used clock frequency */
#define CLK    48000000

#include "as399x_config.h"

/** Macro for a assembler no operation command */
#define NOP()                     _nop_ ()

/** Definition high */
#define HIGH                      1

/** Definition all bits low */
#define LOW                       0

#define BIT0	0x01
#define BIT1	0x02
#define BIT2	0x04
#define BIT3	0x08
#define BIT4	0x10
#define BIT5	0x20
#define BIT6	0x40
#define BIT7	0x80

extern void bin2Chars(int value, unsigned char *destbuf);
extern void bin2Hex(char value, unsigned char *destbuf);

extern void copyBuffer(unsigned char *source, unsigned char *dest, unsigned char len);
extern unsigned char stringLength(char *source);
extern void bitArrayCopy(const u8 *src_org, s16 src_offset, s16 src_len, u8 *dst_org, s16 dst_offset);


/** Definition for the maximal EPC length */
#define EPCLENGTH              32  /* number of bytes for EPC, standard allows up to 62 bytes */
/** Definition for the PC length */
#define PCLENGTH                2
/** Definition for the CRC length */
#define CRCLENGTH               2
/** Additional size for crypto commands (ECC) not yet implemented */
#define CRYPTOLENGTH			20
/** Definition of the maximum frequencies for the hopping */
#define MAXFREQ                 53
/** Definition of the maximum tune entries in tuning table */
#define MAXTUNE                 40
/** Definition of the maximum number of tags, which can be read in 1 round */
#define MAXTAG					45

#endif
