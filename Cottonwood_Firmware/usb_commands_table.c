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
  * @brief Table to branch into the functions associated with a specific command.
  *
  * This file provides a table (call_fkt_()) which allows to branch into functions
  * depending on which command has been received. Each command has an 8bit identifier.
  * This identifier is also used as offset in the table, to branch into the corresponding
  * function.
  *
  * @author Ulrich Herrmann
  * @author Bernhard Breinbauer
  * @author Wolfgang Reichart
  */

#include "as399x_config.h"
#include "as399x.h"
#include "global.h"

void callWrongCommand(void);
void callSendFirmwareHardwareID(void);
void callAntennaPower(void);
void callWriteRegister(void);
void callReadRegister(void);
void callReadRegisters(void);
void callConfigGen2(void);
void callAntennaTune(void);
void callInventory6B(void);
void callReadFromTag6B(void);
void callWriteToTag6B(void);
void callInventory(void);
void callInventoryRSSI(void);
void callSelectTag(void);
void callWriteToTag(void);
void callChangeFreq(void);
void callReadFromTag(void);
void callLockUnlock(void);
void callKillTag(void);
void callNXPCommands(void);
void callEnableBootloader(void);
void callStartStop(void);
void callUpdateAntennaPower(void);
void callGenericCommand(void);
void callAuthenticateCommand(void);
void callChallengeCommand(void);
void callReadBufferCommand(void);

/**
 * Table of function pointers. The offset in the table matches the command identifier.
 * When a command is received the corresponding function will be called via this table.
 */
CODE void (*const call_fkt_[256])(void) =
{
    callWrongCommand, /* 0 */
    callWrongCommand, /* 1 */
    callWrongCommand, /* 2 */
    callWrongCommand, /* 3 */
    callWrongCommand, /* 4 */
    callWrongCommand, /* 5 */
    callWrongCommand, /* 6 */
    callWrongCommand, /* 7 */
    callWrongCommand, /* 8 */
    callWrongCommand, /* 9 */
    callWrongCommand, /* 10 */
    callWrongCommand, /* 11 */
    callWrongCommand, /* 12 */
    callWrongCommand, /* 13 */
    callWrongCommand, /* 14 */
    callWrongCommand, /* 15 */
    callSendFirmwareHardwareID, /*  OUT_FIRM_HARDW_ID          */
    callWrongCommand, /* 17 */
    callWrongCommand, /* 18 */
    callWrongCommand, /* 19 */
    callUpdateAntennaPower,  /* OUT_UPDATE_POWER_ID */
    callWrongCommand, /* 21 */
    callWrongCommand, /* 22, in GUI defined as CPU reset, not implemented in FW */
    callWrongCommand, /* 23 */
    callAntennaPower          , /*  OUT_ANTENNA_POWER_ID       */
    callWrongCommand, /* 25 */
    callWriteRegister         , /*  OUT_WRITE_REG_ID           */
    callWrongCommand, /* 27 */
    callReadRegister          , /*  OUT_READ_REG_ID            */
    callWrongCommand, /* 29 */
    callWrongCommand, /* 30 */
    callWrongCommand, /* 31 */
    callWrongCommand, /* 32 */
    callWrongCommand, /* 33 */
    callWrongCommand, /* 34 */
    callWrongCommand, /* 35 */
    callWrongCommand, /* 36 */
    callWrongCommand, /* 37 */
    callWrongCommand, /* 38 */
    callWrongCommand, /* 39 */
    callWrongCommand, /* 40 */
    callWrongCommand, /* 41 */
    callWrongCommand, /* 42 */
    callWrongCommand, /* 43 */
    callWrongCommand, /* 44 */
    callWrongCommand, /* 45 */
    callWrongCommand, /* 46 */
    callWrongCommand, /* 47 */
    callWrongCommand, /* 48 */
    callInventory             , /*  OUT_INVENTORY_ID           */
    callWrongCommand, /* 50 */
    callSelectTag             , /*  OUT_SELECT_TAG_ID          */
    callWrongCommand, /* 52 */
    callWriteToTag            , /*  OUT_WRITE_TO_TAG_ID        */
    callWrongCommand, /* 54 */
    callReadFromTag           , /*  OUT_READ_FROM_TAG_ID       */
    callWrongCommand, /* 56 */
    callWrongCommand, /* 57 */
    callWrongCommand, /* 58 */
    callLockUnlock            , /*  OUT_LOCK_UNLOCK_ID         */
    callWrongCommand, /* 60 */
    callKillTag               , /*  OUT_KILL_TAG_ID            */
    callWrongCommand, /* 62 */
    callInventory6B           , /*  OUT_INVENTORY_6B_ID        */
    callWrongCommand, /* 64 */
    callChangeFreq            , /*  OUT_CHANGE_FREQ_ID         */
    callWrongCommand, /* 66 */
    callInventoryRSSI         , /*  OUT_INVENTORY_RSSI_ID      */
    callWrongCommand, /* 68 */
    callNXPCommands           , /*  OUT_NXP_COMMAND_ID         */
    callWrongCommand, /* 70 */
    callWriteToTag6B          , /*  OUT_WRITE_TO_TAG_6B_ID     */
    callWrongCommand, /* 72 */
    callReadFromTag6B         , /*  OUT_READ_FROM_TAG_6B_ID    */
    callWrongCommand, /* 74 */
    callAuthenticateCommand, 	/* OUT_AUTHENTICATE_ID 			*/
    callWrongCommand, /* 76 */
    callChallengeCommand, 		/* OUT_CHALLENGE_ID 			*/
    callWrongCommand, /* 78 */
    callReadBufferCommand, 		/* OUT_READ_BUFFER_ID 			*/
    callWrongCommand, /* 80 */
    callWrongCommand, /* 81 */
    callWrongCommand, /* 82 */
    callWrongCommand, /* 83 */
    callWrongCommand, /* 84 */
    callEnableBootloader      , /*  OUT_FIRM_PROGRAM_ID        */
    callWrongCommand, /* 86 */
    callReadRegisters         , /*  OUT_REGS_COMPLETE_ID       	*/
    callWrongCommand, /* 88 */
    callConfigGen2            , /*  OUT_GEN2_SETTINGS_ID       	*/
    callWrongCommand, /* 90 */
    callAntennaTune           , /*  OUT_TUNER_SETTINGS_ID       */
    callWrongCommand, /* 92 */
    callStartStop             , /* OUT_START_STOP_ID           	*/
    callWrongCommand, /* 94 */
    callGenericCommand,			/* OUT_GENERIC_COMMAND_ID	   	*/
    callWrongCommand, /* 96 */
    callWrongCommand, /* 97 */
    callWrongCommand, /* 98 */
    callWrongCommand, /* 99 */
    callWrongCommand, /* 100 */
    callWrongCommand, /* 101 */
    callWrongCommand, /* 102 */
    callWrongCommand, /* 103 */
    callWrongCommand, /* 104 */
    callWrongCommand, /* 105 */
    callWrongCommand, /* 106 */
    callWrongCommand, /* 107 */
    callWrongCommand, /* 108 */
    callWrongCommand, /* 109 */
    callWrongCommand, /* 110 */
    callWrongCommand, /* 111 */
    callWrongCommand, /* 112 */
    callWrongCommand, /* 113 */
    callWrongCommand, /* 114 */
    callWrongCommand, /* 115 */
    callWrongCommand, /* 116 */
    callWrongCommand, /* 117 */
    callWrongCommand, /* 118 */
    callWrongCommand, /* 119 */
    callWrongCommand, /* 120 */
    callWrongCommand, /* 121 */
    callWrongCommand, /* 122 */
    callWrongCommand, /* 123 */
    callWrongCommand, /* 124 */
    callWrongCommand, /* 125 */
    callWrongCommand, /* 126 */
    callWrongCommand, /* 127 */
    callWrongCommand, /* 128 */
    callWrongCommand, /* 129 */
    callWrongCommand, /* 130 */
    callWrongCommand, /* 131 */
    callWrongCommand, /* 132 */
    callWrongCommand, /* 133 */
    callWrongCommand, /* 134 */
    callWrongCommand, /* 135 */
    callWrongCommand, /* 136 */
    callWrongCommand, /* 137 */
    callWrongCommand, /* 138 */
    callWrongCommand, /* 139 */
    callWrongCommand, /* 140 */
    callWrongCommand, /* 141 */
    callWrongCommand, /* 142 */
    callWrongCommand, /* 143 */
    callWrongCommand, /* 144 */
    callWrongCommand, /* 145 */
    callWrongCommand, /* 146 */
    callWrongCommand, /* 147 */
    callWrongCommand, /* 148 */
    callWrongCommand, /* 149 */
    callWrongCommand, /* 150 */
    callWrongCommand, /* 151 */
    callWrongCommand, /* 152 */
    callWrongCommand, /* 153 */
    callWrongCommand, /* 154 */
    callWrongCommand, /* 155 */
    callWrongCommand, /* 156 */
    callWrongCommand, /* 157 */
    callWrongCommand, /* 158 */
    callWrongCommand, /* 159 */
    callWrongCommand, /* 160 */
    callWrongCommand, /* 161 */
    callWrongCommand, /* 162 */
    callWrongCommand, /* 163 */
    callWrongCommand, /* 164 */
    callWrongCommand, /* 165 */
    callWrongCommand, /* 166 */
    callWrongCommand, /* 167 */
    callWrongCommand, /* 168 */
    callWrongCommand, /* 169 */
    callWrongCommand, /* 170 */
    callWrongCommand, /* 171 */
    callWrongCommand, /* 172 */
    callWrongCommand, /* 173 */
    callWrongCommand, /* 174 */
    callWrongCommand, /* 175 */
    callWrongCommand, /* 176 */
    callWrongCommand, /* 177 */
    callWrongCommand, /* 178 */
    callWrongCommand, /* 179 */
    callWrongCommand, /* 180 */
    callWrongCommand, /* 181 */
    callWrongCommand, /* 182 */
    callWrongCommand, /* 183 */
    callWrongCommand, /* 184 */
    callWrongCommand, /* 185 */
    callWrongCommand, /* 186 */
    callWrongCommand, /* 187 */
    callWrongCommand, /* 188 */
    callWrongCommand, /* 189 */
    callWrongCommand, /* 190 */
    callWrongCommand, /* 191 */
    callWrongCommand, /* 192 */
    callWrongCommand, /* 193 */
    callWrongCommand, /* 194 */
    callWrongCommand, /* 195 */
    callWrongCommand, /* 196 */
    callWrongCommand, /* 197 */
    callWrongCommand, /* 198 */
    callWrongCommand, /* 199 */
    callWrongCommand, /* 200 */
    callWrongCommand, /* 201 */
    callWrongCommand, /* 202 */
    callWrongCommand, /* 203 */
    callWrongCommand, /* 204 */
    callWrongCommand, /* 205 */
    callWrongCommand, /* 206 */
    callWrongCommand, /* 207 */
    callWrongCommand, /* 208 */
    callWrongCommand, /* 209 */
    callWrongCommand, /* 210 */
    callWrongCommand, /* 211 */
    callWrongCommand, /* 212 */
    callWrongCommand, /* 213 */
    callWrongCommand, /* 214 */
    callWrongCommand, /* 215 */
    callWrongCommand, /* 216 */
    callWrongCommand, /* 217 */
    callWrongCommand, /* 218 */
    callWrongCommand, /* 219 */
    callWrongCommand, /* 220 */
    callWrongCommand, /* 221 */
    callWrongCommand, /* 222 */
    callWrongCommand, /* 223 */
    callWrongCommand, /* 224 */
    callWrongCommand, /* 225 */
    callWrongCommand, /* 226 */
    callWrongCommand, /* 227 */
    callWrongCommand, /* 228 */
    callWrongCommand, /* 229 */
    callWrongCommand, /* 230 */
    callWrongCommand, /* 231 */
    callWrongCommand, /* 232 */
    callWrongCommand, /* 233 */
    callWrongCommand, /* 234 */
    callWrongCommand, /* 235 */
    callWrongCommand, /* 236 */
    callWrongCommand, /* 237 */
    callWrongCommand, /* 238 */
    callWrongCommand, /* 239 */
    callWrongCommand, /* 240 */
    callWrongCommand, /* 241 */
    callWrongCommand, /* 242 */
    callWrongCommand, /* 243 */
    callWrongCommand, /* 244 */
    callWrongCommand, /* 245 */
    callWrongCommand, /* 246 */
    callWrongCommand, /* 247 */
    callWrongCommand, /* 248 */
    callWrongCommand, /* 249 */
    callWrongCommand, /* 250 */
    callWrongCommand, /* 251 */
    callWrongCommand, /* 252 */
    callWrongCommand, /* 253 */
    callWrongCommand, /* 254 */
    callWrongCommand  /* 255 */
};
