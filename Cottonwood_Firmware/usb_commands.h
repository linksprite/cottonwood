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
  * @brief This file is the include file for the usb_commands.c file.
  *
  * @author Ulrich Herrmann
  * @author Bernhard Breinbauer
  * @author Wolfgang Reichart
  */

#ifndef __USB_COMMANDS_H__
#define __USB_COMMANDS_H__

#include "global.h"

extern void commands(void);
extern void initCommands(void);
extern void uartCommands(void) ;


extern CODE void (* const call_fkt_[256])(void);

/*USB COMMANDS (READER) */
#define OUT_FIRM_HARDW_ID       0x10
#define IN_FIRM_HARDW_ID        0x11

#define OUT_UPDATE_POWER_ID     0x14
#define IN_UPDATE_POWER_ID      0x15

#define OUT_ANTENNA_POWER_ID    0x18 
#define IN_ANTENNA_POWER_ID    	0x19

#define OUT_WRITE_REG_ID        0x1A
#define IN_WRITE_REG_ID         0x1B

#define OUT_READ_REG_ID         0x1C
#define IN_READ_REG_ID          0x1D


/*USB COMMANDS (TAG) */
#define OUT_INVENTORY_ID        0x31
#define IN_INVENTORY_ID         0x32

#define OUT_SELECT_TAG_ID       0x33
#define IN_SELECT_TAG_ID        0x34

#define OUT_WRITE_TO_TAG_ID     0x35
#define IN_WRITE_TO_TAG_ID      0x36

#define OUT_READ_FROM_TAG_ID    0x37
#define IN_READ_FROM_TAG_ID     0x38

#define OUT_LOCK_UNLOCK_ID      0x3B
#define IN_LOCK_UNLOCK_ID       0x3C

#define OUT_KILL_TAG_ID         0x3D
#define IN_KILL_TAG_ID          0x3E

/* implementation of ISO18000-6B */
#define OUT_INVENTORY_6B_ID     0x3F
#define IN_INVENTORY_6B_ID      0x40

#define OUT_CHANGE_FREQ_ID      0x41
#define IN_CHANGE_FREQ_ID       0x42

#define OUT_INVENTORY_RSSI_ID   0x43
#define IN_INVENTORY_RSSI_ID    0x44

#define OUT_NXP_COMMAND_ID      0x45
#define IN_NXP_COMMAND_ID       0x46

#define OUT_WRITE_TO_TAG_6B_ID     0x47
#define IN_WRITE_TO_TAG_6B_ID      0x48

#define OUT_READ_FROM_TAG_6B_ID     0x49
#define IN_READ_FROM_TAG_6B_ID      0x4a

/* crypto-specific commands */
#define OUT_AUTHENTICATE_ID		0x4b
#define IN_AUTHENTICATE_ID		0x4c

#define OUT_CHALLENGE_ID		0x4d
#define IN_CHALLENGE_ID			0x4e

#define OUT_READ_BUFFER_ID		0x4f
#define IN_READ_BUFFER_ID		0x50

/* firmware application commands */
#define OUT_FIRM_PROGRAM_ID     0x55
#define IN_FIRM_PROGRAM_ID     0x56

#define OUT_REGS_COMPLETE_ID   0x57
#define IN_REGS_COMPLETE_ID    0x58

#define OUT_GEN2_SETTINGS_ID   0x59
#define IN_GEN2_SETTINGS_ID    0x5a

#define OUT_TUNER_SETTINGS_ID   0x5b
#define IN_TUNER_SETTINGS_ID    0x5c

#define OUT_START_STOP_ID      0x5d
#define IN_START_STOP_ID       0x5e

/* Generic Gen2 Command */
#define OUT_GENERIC_COMMAND_ID  0x5F
#define IN_GENERIC_COMMAND_ID   0x60


/*WRONG USB COMMAND */
#define IN_COMMAND_WRONG_ID     0xff

/*Size */
#define OUT_FIRM_PROGRAM_IDSize  0x3f
#define IN_FIRM_PROGRAM_IDSize   0x02

#define OUT_FIRM_HARDW_IDSize    0x02
#define IN_FIRM_HARDW_IDSize     0x30

#define OUT_UPDATE_POWER_IDSize  0x3D
#define IN_UPDATE_POWER_IDSize   0x02

#define OUT_ANTENNA_POWER_IDSize 0x0D
#define IN_ANTENNA_POWER_IDSize  0x0D


#define OUT_INVENTORY_IDSize     0x02
#define IN_INVENTORY_IDSize      0x3f

#define OUT_WRITE_TO_TAG_IDSize  0x3f
#define IN_WRITE_TO_TAG_IDSize   0x03

#define OUT_READ_FROM_TAG_IDSize 0x07
#define IN_READ_FROM_TAG_IDSize  0x3f

#define OUT_WRITE_REG_IDSize     0x05
#define IN_WRITE_REG_IDSize      0x02

#define OUT_READ_REG_IDSize      0x02
#define IN_READ_REG_IDSize       0x05

#define OUT_INVENTORY_6B_IDSize  0x0C 
#define IN_INVENTORY_6B_IDSize   0x0f

#define OUT_WRITE_TO_TAG_6B_IDSize  0x3f
#define IN_WRITE_TO_TAG_6B_IDSize   0x02

#define OUT_READ_FROM_TAG_6B_IDSize  0x0C
#define IN_READ_FROM_TAG_6B_IDSize   0x3f

#define OUT_CHANGE_FREQ_IDSize   0x3f
#define IN_CHANGE_FREQ_IDSize    0x3f

#define OUT_INVENTORY_RSSI_IDSize 0x02
#define IN_INVENTORY_RSSI_IDSize 0x3f

#define OUT_LOCK_IDSize          0x07
#define IN_LOCK_IDSize           0x02

#define OUT_SELECT_IDSize        0x3F
#define IN_SELECT_IDSize         0x02

#define OUT_KILL_TAG_IDSize      0x06
#define IN_KILL_TAG_IDSize       0x02

#define OUT_NXP_COMMAND_IDSize   0x09
#define IN_NXP_COMMAND_IDSize    0x04

#define OUT_REGS_COMPLETE_IDSize   0x01
#define IN_REGS_COMPLETE_IDSize    0x3f

#define OUT_GEN2_SETTINGS_IDSize   0x3f
#define IN_GEN2_SETTINGS_IDSize    0x3f

#define OUT_TUNER_SETTINGS_IDSize 0x3f
#define IN_TUNER_SETTINGS_IDSize  0x3f

#define OUT_START_STOP_IDSize      0x3
#define IN_START_STOP_IDSize       0x3f

#define OUT_GENERIC_COMMAND_IDSize 0x3f
#define IN_GENERIC_COMMAND_IDSize  0x3f

#define OUT_AUTHENTICATE_IDSize		0x3f
#define IN_AUTHENTICATE_IDSize		0x3f

#define OUT_CHALLENGE_IDSize		0x3f
#define IN_CHALLENGE_IDSize			0x3f

#define OUT_READ_BUFFER_IDSize		0x3f
#define IN_READ_BUFFER_IDSize		0x3f

/*Protocol Bytes */
#define USB_COMMAND             getBuffer_[0]
#define USB_COMM_LEN            getBuffer_[1]

#define FIRMWARE               0x00
#define HARDWARE                0x01

/*Command Antenna Power */
#define POWER_BYTE              getBuffer_[2]

#define ANT_POWER_OFF           0x00
#define ANT_POWER_ON            0xFF

/*Command Register Write */
#define WREG_ADDRESS            getBuffer_[2]
#define WREG_DATA               getBuffer_[3]

/*Command Register Read */
#define RREG_ADDRESS            getBuffer_[2]

/*Command Inventory */
#define START_NEXT_BYTE         getBuffer_[2]

#define STARTINVENTORY          0x01
#define NEXTTID                 0x02

#define LENGTH_BYTE             0x01

#endif

