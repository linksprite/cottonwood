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
  * @brief This file is the include file for the uart.c file.
  *
  * @author Ulrich Herrmann
  */

#ifndef __UART_H__
#define __UART_H__

/*void sendArrayIRQ(char *dat); */

extern void initUART(void);
extern void initUARTTo20MHz(void);
extern void sendArray(char *dat);
extern void sendByte(unsigned char value);
extern void waitForData(void);
extern unsigned char getByte(void);
extern void resetReceiveFlag(void);
extern void sendArrayIRQ(char *dat);
extern void CON_print(void * format, ...);
extern void CON_hexdump(const u8 *buffer, u8 length);
extern void Serial_SendBufferedChar(unsigned char ch);

extern unsigned char checkByte(void);

/*Definitions */

/** Definition for the baudrate */
#define BAUD             			        56000
/*#define U2X0BIT_SET       	1 */
/** Definition for the clk divider */
#define CLKDIVIDER                  4
/** Definition for T1 clock */
#define T1CLK                       CLK/CLKDIVIDER
/** Definition for T1H */
#define T1H                         256-T1CLK/(2*BAUD)

/** Definition for a received byte */
#define RECEIVED          		        0x00
/** Definition for a received byte */
#define RECEIVE_RESET     	         0x00

/** Definition for transmittion comlpete */
#define AS399X_CMD_TRANSMITTED       	         0x00
/** Definition for transmittion not comlpete */
#define AS399X_CMD_TRANSMIT_RESET              0x01

/** Definition for DATA OUT register */
#define UART_DATA_OUT               SBUF0
/** Definition for DATA IN register */
#define UART_DATA_IN      	         SBUF0

/** Macro clear interrupt */
#define CLEARINT(x)                 x=0
/** Macro write data to buffer */
#define WRITEDATA(x)                UART_DATA_OUT=x

/*----TMOD Register */
/**TMOD Register - Bit 7 - GATE1: Serial Port 0 Operation Mode */
#define GATE1                       128

/**TMOD Register - Bit 6 - C/T1: Counter/Timer 1 Select */
#define CT1                         64

/**TMOD Register - Bit 5 - T1M1: Multiprocessor Communication Enable */
#define T1M1                        32

/**TMOD Register - Bit 4 - T1M0: Receive Enable */
#define T1M0                        16

/**TMOD Register - Bit 3 - GATE0: Ninth Transmission Bit */
#define GATE0                       8

/**TMOD Register - Bit 2 - CT0: Ninth Receive Bit */
#define CT0                         4

/**TMOD Register - Bit 1 - T0M1: Transmit Interrupt Flag */
#define T0M1                        2

/**TMOD Register - Bit 0 - T0M0: Receive Interrupt Flag */
#define T0M0                        1

#endif

