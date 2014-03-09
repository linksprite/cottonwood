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
 *   \brief Declaration of low level functions provided by the AS399x series chips
 *
 *   Declares low level functions provided by the AS399x series chips. All 
 *   higher level functions are provided by as399x_public.h and protocol
 *   work is contained in gen2.h and iso6b.h
 */

#ifndef _AS399X_H_
#define _AS399X_H_

#include "global.h"

/*!
  * macro for setting the output ports used for direct mode
  */
#define AS399X_ENABLE_SENDER()  do{IO2(LOW);IO3(HIGH);}while(0)

/*!
  * macro for setting the input ports used for direct mode
  */
#define AS399X_ENABLE_RECEIVER() do{IO2(HIGH);IO3(HIGH);}while(0)

/* COMMANDS */
/** AS3990 direct command: idle */
#define AS399X_CMD_IDLE                      0x80
/** AS3990 direct command: soft init */
#define AS399X_CMD_SOFTINIT                  0x83
/** AS3990 direct command: hop to main frequency */
#define AS399X_CMD_HOPMAINFREQ               0x84
/** AS3990 direct command: hop to aux frequency */
#define AS399X_CMD_HOPAUXFREQ                0x85
/** AS3990 direct command: trigger AS399X_REG_ADC conversion */
#define AS399X_CMD_TRIGGERADCCON             0x87
#if RUN_ON_AS3992
/** AS3992 direct command: trigger rx filter calibration */
#define AS399X_CMD_TRIGGER_CALIBRATION       0x88
#endif
/** AS3990 direct command: reset AS399X_REG_FIFO */
#define AS399X_CMD_RESETFIFO                 0x8F
/** AS3990 direct command: transmition to tag with CRC */
#define AS399X_CMD_TRANSMCRC                 0x90
/** AS3990 direct command: transmition to tag with CRC expecting headerbit */
#define AS399X_CMD_TRANSMCRCEHEAD            0x91
/** AS3990 direct command: transmition to tag without CRC */
#define AS399X_CMD_TRANSM                    0x92
/** AS3990 direct command: delayed transmition without CRC */
#define AS399X_CMD_DELAYEDTRANSM             0x93
/** AS3990 direct command: delayed transmition with CRC */
#define AS399X_CMD_DELAYEDTRANSMCRC          0x94
/** AS3990 direct command: block receive operation */
#define AS399X_CMD_BLOCKRX                   0x96
/** AS3990 direct command: enable receive operation */
#define AS399X_CMD_ENABLERX                  0x97
/** AS3990 direct command: sending query command to tag */
#define AS399X_CMD_QUERY                     0x98
/** AS3990 direct command: sending queryrep command to tag */
#define AS399X_CMD_QUERYREP                  0x99
/** AS3990 direct command: sending query adjust up command to tag */
#define AS399X_CMD_QUERYADJUSTUP             0x9A
/** AS3990 direct command: sending query adjust nic command to tag */
#define AS399X_CMD_QUERYADJUSTNIC            0x9B
/** AS3990 direct command: sending query adjust down command to tag */
#define AS399X_CMD_QUERYADJUSTDOWN           0x9C
/** AS3990 direct command: sending acknowledge command to tag */
#define AS399X_CMD_ACKN                      0x9D
/** AS3990 direct command: sending not acknowledge command to tag */
#define AS399X_CMD_NAK                       0x9E
/** AS3990 direct command: sending request new RN16 or Handle command to tag */
#define AS399X_CMD_REQRN                     0x9F

/*Registers */
/*Main Control */
/** AS3990 register: status control */
#define AS399X_REG_STATUSCTRL                0x00
/** AS3990 register: protocol control */
#define AS399X_REG_PROTOCOLCTRL              0x01

/*Protocol */
/** AS3990 register: transmit options GEN2 */
#define AS399X_REG_TXOPTGEN2                 0x02
/** AS3990 register: receive options GEN2 */
#define AS399X_REG_RXOPTGEN2                 0x03
/** AS3990 register: TRcal low */
#define AS399X_REG_TRCALREGGEN2              0x04
/** AS3990 register: TRcal and misc */
#define AS399X_REG_TRCALGEN2MISC             0x05
/** AS3990 register: delayed reply transmission wait time */
#define AS399X_REG_TXREPLYDELAY              0x06
/** AS3990 register: wait time until no response interrupt is generated */
#define AS399X_REG_RXNORESPWAIT              0x07
/** AS3990 register: wait time after transmition until the receiver is enabled */
#define AS399X_REG_RXWAITTIME                0x08
/** AS3990 register: receive special settings */
#define AS399X_REG_RXSPECIAL                 0x09
/** AS3990 register: receive special settings 2 */
#define AS399X_REG_RXSPECIAL2                0x0A
/** AS3990 register: regulator and IO control */
#define AS399X_REG_IOCONTROL                 0x0B
/** AS3990 register: chip version */
#define AS399X_REG_TESTSETTING               0x12

#define AS399X_REG_VERSION                   0x13
/** AS3990 register: CL_sys,analouge out and CP control */
#define AS399X_REG_CLSYSAOCPCTRL             0x14
/** AS3990 register: modulator control */
#define AS399X_REG_MODULATORCTRL             0x15
/** AS3990 register: PLL R, A/B divider main register (three bytes deep) */
#define AS399X_REG_PLLMAIN                   0x16
/** AS3990 register: PLL A/B divider auxiliary register (three bytes deep) */
#define AS399X_REG_PLLAUX                    0x17
/** AS3990 register: AS399X_REG_DAC control */
#define AS399X_REG_DAC                       0x18
/** AS3990 register: AS399X_REG_ADC control */
#define AS399X_REG_ADC                       0x19

/*Status */
/** AS3990 register: IRQ status */
#define AS399X_REG_IRQSTATUS                 0x0C
/** AS3990 register: IRQ mask */
#define AS399X_REG_IRQMASKREG                0x0D
/** AS3990 register: AGC and Internal status */
#define AS399X_REG_AGCINTERNALSTATUS         0x0E
/** AS3990 register: RSSI levels */
#define AS399X_REG_RSSILEVELS                0x0F
/** AS3990 register: AGL status */
#define AS399X_REG_AGLSTATUS                 0x10

/*Testregisters */
/** AS3990 register: Test setting 1 and measurement selection */
#define AS399X_REG_MEASURE_SELECTOR          0x11
/** AS3990 register: Test setting 2 */

/*AS399X_REG_FIFO registers */
/** AS3990 register: number of bytes to receive (upper byte) */
#define AS399X_REG_RXLENGTHUP                0x1A
/** AS3990 register: number of bytes to receive (lower byte) */
#define AS399X_REG_RXLENGTHLOW               0x1B
/** AS3990 register: AS399X_REG_FIFO status */
#define AS399X_REG_FIFOSTATUS                0x1C
/** AS3990 register: number of bytes to transmit (upper byte) */
#define AS399X_REG_TXLENGTHUP                0x1D
/** AS3990 register: number of bytes to transmit (lower byte) */
#define AS399X_REG_TXLENGTHLOW               0x1E
/** AS3990 register: data AS399X_REG_FIFO */
#define AS399X_REG_FIFO                      0x1F

/*IRQ Mask register */
/** AS3990 interrupt: no response bit */
#define AS399X_IRQ_NORESP               0x01
/** AS3990 interrupt: error 3 bit */
#define AS399X_IRQ_PREAMBLE             0x02
/** AS3990 interrupt: error 2 bit */
#define AS399X_IRQ_RXCOUNT              0x04
/** AS3990 interrupt: header bit */
#define AS399X_IRQ_HEADER               0x08
/** AS3990 interrupt: error1 bit */
#define AS399X_IRQ_CRCERROR             0x10
/** AS3990 interrupt: fifo bit */
#define AS399X_IRQ_FIFO                 0x20
/** AS3990 IRQ status register: receive complete */
#define AS399X_IRQ_RX                   0x40
/** AS3990 IRQ status register: transmit complete */
#define AS399X_IRQ_TX                   0x80

#define AS399X_IRQ_MASK_ALL             0x3f
#define AS399X_IRQ_ALL                  0xff
#define ALLDONEIRQ                (AS399X_IRQ_ALL & ~AS399X_IRQ_FIFO)

/*AS399X_REG_FIFO STATUS register */
/** AS3990 AS399X_REG_FIFO status register: FIFO 2/3 full */
#define AS399X_FIFOSTAT_HIGHLEVEL             0x80
/** AS3990 AS399X_REG_FIFO status register: FIFO 1/3 full */
#define AS399X_FIFOSTAT_LOWLEVEL              0x40
/** AS3990 AS399X_REG_FIFO status register: FIFO overflow */
#define AS399X_FIFOSTAT_OVERFLOW              0x20

/* FIFO Constants */
/** AS399x FIFO DEPTH is 24 bytes */
#define AS399X_FIFO_DEPTH					  24
#define AS399X_HIGHFIFOLEVEL				  18
#define AS399X_LOWFIFOLEVEL				      6

/** Indication that tx is done, corresponds to AS399X_IRQ_TX falling edge */
#define RESP_TXIRQ          AS399X_IRQ_TX
/** Indication that rx is done, corresponds to AS399X_IRQ_RX falling edge */
#define RESP_RXIRQ          AS399X_IRQ_RX
#define RESP_FIFO           AS399X_IRQ_FIFO
#define RESP_CRCERROR       AS399X_IRQ_CRCERROR
#define RESP_HEADERBIT      AS399X_IRQ_HEADER
#define RESP_RXCOUNTERROR   AS399X_IRQ_RXCOUNT
#define RESP_PREAMBLEERROR  AS399X_IRQ_PREAMBLE
#define RESP_NORESINTERRUPT AS399X_IRQ_NORESP
#define RESP_HIGHFIFOLEVEL  (AS399X_FIFOSTAT_HIGHLEVEL<<8)
#define RESP_LOWLEVEL       (AS399X_FIFOSTAT_LOWLEVEL<<8)
#define RESP_FIFOOVERFLOW   (AS399X_FIFOSTAT_OVERFLOW<<8)

#define RESP_RXDONE_OR_ERROR  (AS399X_IRQ_ALL & ~RESP_FIFO & ~RESP_TXIRQ)
/* RESP_FIFOOVERFLOW does not work reliably */
#define RESP_ERROR  (RESP_CRCERROR | RESP_HEADERBIT | RESP_RXCOUNTERROR | RESP_PREAMBLEERROR)
#define RESP_ANY    (AS399X_IRQ_ALL & ~RESP_FIFO)

extern volatile u16 data as399xResponse;
extern u32 as399xCurrentBaseFreq;

#ifdef POWER_DETECTOR
/** size of array for output power table dBm2Setting */
#define POWER_TABLE_SIZE 31

struct powerDetectorSetting
{
    u8 reg15_1;
    u16 adcVoltage_mV;
};
extern struct powerDetectorSetting dBm2Setting[];
#endif

/*------------------------------------------------------------------------- */
/** Sends only one command to the AS399x. \n
  * @param command The command which is send to the AS399x.
  */
void as399xSingleCommand(u8 command);

/**
 * Sends a number of commands to the AS399x
 * @param commands commands which should be sent to AS399x
 * @param len number of commands to send
 */
void as399xContinuousCommand(u8 *commands, s8 len);

/*------------------------------------------------------------------------- */
void as399xSingleWriteNoStop(u8 address, u8 value);

/*------------------------------------------------------------------------- */

/*------------------------------------------------------------------------- */
/** Reads data from a address and some following addresses from the
  * AS3990. The len parameter defines the number of address read. \n
  * @param address addressbyte
  * @param len Length of the buffer.
  * @param *readbuf Pointer to the first byte of the array where the data has to be stored in.
  */
void as399xContinuousRead(u8 address, s8 len, u8 *readbuf);

/** Reads data from the fifo and some following addresses from the
  * AS3990. The len parameter defines the number of address read.
  * Does this in a secure way so that it works also on AS3991 devices in serial mode
  * @param len Length of the buffer.
  * @param *readbuf Pointer to the first byte of the array where the data has to be stored in.
  */
void as399xFifoRead(s8 len, u8 *readbuf);

/** Single Read -> reads one byte from one address of the AS3990.  \n
  * This function is only called from application level (not interrupt level). From
  * interrupt level (extInt()) as399xSingleReadIrq is used.
  * @param address The addressbyte
  * @return The databyte read from the AS399x
  */
u8 as399xSingleRead(u8 address);

/**
 * This function is called only from AS399x ISR (extInt). From application level
 * as399xSingleRead is used.\n
 * The separation of application level and interrupt level removes the requirement of
 * as399xSingleRead being reentrant. This is necessary because the reentrant function
 * definition introduces quite some runtime overhead.
 * Especially on the interrupt level this runtime overhead is a problem because
 * we have to meet the T2 timing constrain, defined by the Gen2 protocol when sending back
 * ACK or RegRN commands.
 * @param address address to read from.
 * @return register value read from address.
 */
u8 as399xSingleReadIrq(u8 address);

/*------------------------------------------------------------------------- */
/** Write 252 byte predistortion data, preserving version register
  * @param *buf Pointer to the first byte of the array.
  * @param buf pointer to the buffer.
  */
void as399xWritePredistortion(const u8 *buf);

/*------------------------------------------------------------------------- */
/** Continuous Write -> writes several bytes to subsequent addresses of the AS399X.  \n
  * @param address addressbyte
  * @param *buf Pointer to the first byte of the array.
  * @param len Length of the buffer.
  */
void as399xContinuousWrite(u8 address, u8 *buf, s8 len);

/** Single Write -> writes one byte to one address of the AS3990.  \n
  * @param address The addressbyte
  * @param value The databyte
  */
void as399xSingleWrite(u8 address, u8 value);

/*------------------------------------------------------------------------- */
/** Sends first some commands to the AS3990. The number of
  * commands is specified with the parameter com_len. Then it sets
  * the address where the first byte has to be written and after that
  * every byte from the buffer is written to the AS3990. \n
  * @param *command Pointer to the first byte of the command buffer
  * @param com_len Length of the command buffer.
  * @param address addressbyte
  * @param *buf Pointer to the first byte of the data array.
  * @param buf_len Length of the buffer.

  */
void as399xCommandContinuousAddress(u8 *command, u8 com_len,
                             u8 address, u8 *buf, u8 buf_len);

/*------------------------------------------------------------------------- */
/** This function waits for the specified response(IRQ).
  */
void as399xWaitForResponse(u16 waitMask);

/*------------------------------------------------------------------------- */
/** This function waits for the specified response(IRQ).
  */
void as399xWaitForResponseTimed(u16 waitMask, u16 ms);

#if 0
/*------------------------------------------------------------------------- */
/** This function gets the current response
  */
u16 as399xGetResponse(void);

/*------------------------------------------------------------------------- */
/** This function clears the response bits according to mask
  */
void as399xClrResponseMask(u16 mask);

/*------------------------------------------------------------------------- */
/** This function clears all responses
  */
void as399xClrResponse(void);
#else
#define as399xGetResponse() as399xResponse
#define as399xClrResponseMask(mask) as399xResponse&=~(mask)
#define as399xClrResponse() as399xResponse=0
#endif

/*!
 *****************************************************************************
 *  \brief  Enter the direct mode
 *
 *  The direct mode is needed since the AS3991 doesn't support ISO18000-6B
 *  directly. Direct mode means that the MCU directly issues the commands.
 *  The AS3991 modulates the serial stream from the MCU and sends it out.
 *
 *****************************************************************************
 */
void as399xEnterDirectMode();

/*!
 *****************************************************************************
 *  \brief  Leave the direct mode
 *
 *  After calling this function the AS3991 is in normal mode again.
 *
 *****************************************************************************
 */
void as399xExitDirectMode();

/*!
 *****************************************************************************
 *  \brief  Enter the standby mode
 *
 *****************************************************************************
 */
void as399xEnterStandbyMode();

/*!
 *****************************************************************************
 *  \brief  Leave the standby mode
 *
 *  After calling this function the AS3991 is in normal mode again.
 *
 *****************************************************************************
 */

void as399xExitStandbyMode();

/*!
 *****************************************************************************
 *  \brief  Enter the power down mode by setting EN pin to low, saving 
 *  registers beforehand
 *
 *****************************************************************************
 */
void as399xEnterPowerDownMode();

/*!
 *****************************************************************************
 *  \brief  Exit the power down mode by setting EN pin to high, restoring 
 *  registers afterwards.
 *
 *****************************************************************************
 */
void as399xExitPowerDownMode();

/*!
 *****************************************************************************
 * Sets DAC output voltage according to parameter natVal. The DAC values of AS399x
 * are in magnitude representation. The FW uses DAC values in natural
 * representation internally, as it makes computation easier. The FW handles the DAC values
 * in the code as it is a 8 bit DAC from 0V to 3.2V, LSB = 12.5mV. \n
 * The conversion from natural to magnitude representation is handled
 * by as399xWriteDAC() and as399xReadDAC().
 * @param natVal Sets the DAC output voltage according to the value.
 *****************************************************************************
 */
void as399xWriteDAC( u8 natVal );

/*!
 *****************************************************************************
 * Reads DAC value and returns it in natural representation. See as399xWriteDAC() for details.
 *****************************************************************************
 */
u8 as399xReadDAC( void );

extern u8 as399xChipVersion;

#endif /* _AS399X_H_ */
