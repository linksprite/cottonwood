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
  * @brief Implementation of UART functionality.
  *
  * This file provides the whole functionality to send/receive data with the UART.
  *
  * @author Ulrich Herrmann
  */

#include "c8051F340.h"
#include "as399x_config.h"
#include "uart.h"
#include "stdarg.h"
#include "global.h"
#include "string.h"

/** received databyte */
IDATA volatile u8 receive_ = 0;

/** receive flag */
IDATA volatile s8 check_receive_ = RECEIVE_RESET;

/** transmit flag */
IDATA volatile s8 check_transmitted_ = AS399X_CMD_TRANSMIT_RESET;

IDATA u8 *uartTransferPtr;
IDATA volatile u8 uartTransferPos;
IDATA volatile u8 uartTransferLen;

/* for a simple lazy buffer handling */
u8 conSerArray[100];
u8 conSerIdx;
u8 conSerTxInProgress;
/*------------------------------------------------------------------------- */
/** Transmit Interrupt Function
  * sets the transmit flag: check_transmitted_
  * clears the transmit interrupt flag manually
  * This function can not take or return a parameter
  */
void uartInt(void) interrupt 4 {
    CLEARINT(TI0);  /*clears transmit interrupt flag */
    if (uartTransferPos == uartTransferLen )
    {
        check_transmitted_ = AS399X_CMD_TRANSMITTED;
    }
    else
    {
        WRITEDATA(uartTransferPtr[uartTransferPos++]);
    }
#if UARTSUPPORT
    if (RI0) { 
        check_receive_=1;  
        CLEARINT(RI0);
    }
#endif
}

/*------------------------------------------------------------------------- */
/** Initialsing the UART and all other registers to get the UART working
  *  Baudrate: 115200
  * This function does not take or return a parameter
  */
void initUART(void)
{
    check_receive_ = RECEIVE_RESET;
    check_transmitted_ = AS399X_CMD_TRANSMITTED;
    uartTransferPos = uartTransferLen = 0;
    conSerIdx = conSerTxInProgress = 0;

    S0MODE = 0;         /*8 Bit Mode */
    MCE0 = 1;           /*Multiprocessor Mode disabled */
#if UARTSUPPORT
    REN0 = 1;           /*Receive enable Bit */
#endif

    TCON      = 0x40;
    TMOD      = 0x20;
    CKCON     |= 0x08; /* Use system clock */
#if (CLK == 48000000)
    TH1       = 256 - 208;
#elif (CLK == 24000000)
    TH1       = 256 - 104;
#elif (CLK == 12000000)
    TH1       = 256 - 52;
#else
#error CLK unknown
#endif

    TR1 = 1;

    P0MDOUT   |= 0x10;   /*P0-4 & P0.5 in Push Pull Mode */
    XBR0      = 0x01;

    OSCICN    = 0x83;

    PS0 = 1;

    IE        = 0x90;
    ES0 = 1;             /*Enable UART Interrupt */
    EA = 1;              /*Enable all interrupts */
    XBR1 |= 0x40;        /*Enable crossbar in case not yet enabled */
}

/*------------------------------------------------------------------------- */
/** Sends a s8 Array
  *
  * @param *dat Pointer to the first byte of the Array
  * @param n the size of the array to be transmitted
  */
void sendArrayN(char *dat, u8 n)
{
    u8 count = 0;
    /* Wait in case previous transfer is ongoing */
    while ( check_transmitted_ != AS399X_CMD_TRANSMITTED );
    check_transmitted_ = AS399X_CMD_TRANSMIT_RESET;  /*resets transmit flag */
    uartTransferPtr = dat;
    uartTransferPos = 1;
    uartTransferLen = n;
    /* Setup the transfer and return, rest will be done in interrupt */
    WRITEDATA(uartTransferPtr[0]);
}
/*------------------------------------------------------------------------- */
/** Sends a s8 Array
  *  The last byte has to be 0
  * @param *dat Pointer to the first byte of the Array
  */
void sendArray(char *dat)
{
    sendArrayN(dat, strlen(dat));
}

/*------------------------------------------------------------------------- */
/** Sends one byte
  * @param value value to send
  */
void sendByte(u8 value)
{
    while (check_transmitted_); /*waits for transmit complete */
    check_transmitted_ = AS399X_CMD_TRANSMIT_RESET;  /*resets transmit flag */
    WRITEDATA(value);
}

#if UARTSUPPORT
/*------------------------------------------------------------------------- */
/** Waits for data until the receive flag is set in the interrupt function
  * This function does not take or return a parameter
  */
void waitForData(void)
{
    while (check_receive_); /*waits for receive complete */
    check_receive_ = RECEIVE_RESET; /*resets receive flag */
}

u8 checkByte(void)
{    return(check_receive_);}

/*------------------------------------------------------------------------- */
/** Waits for data until the receive flag is set in the interrupt function
  * This function does not take a parameter
  * @return Returns the byte got from the UART
  */
u8 getByte(void)
{
    check_receive_ = RECEIVE_RESET; /*resets receive flag */
    return(SBUF0);
}

/*------------------------------------------------------------------------- */
/** Resets the reicieve Flag
  * This function does not take or return a parameter
  */
void resetReceiveFlag(void)
{
    check_receive_ = RECEIVE_RESET;
}
#endif


void Serial_SendBufferedChar(u8 ch)
{
    if (conSerTxInProgress)
    { /* Waiting for tx done is lazy, i.e. it is done at the next character
         after a flush */
        conSerTxInProgress = 0;
        conSerIdx = 0;
        while ( check_transmitted_ != AS399X_CMD_TRANSMITTED );
    }

    conSerArray[conSerIdx] = ch;
    conSerIdx++;

    if(conSerIdx >= sizeof(conSerArray) || ch=='\n')
    { /* if buffer is full or we send a newline */
        sendArrayN(conSerArray, conSerIdx);
        conSerTxInProgress = 1;
    }
}


#define NO_OF_DIGITS  10 /* maximum digits we can have in a decimal string 4294967295 = 0xffffffff*/
#define NO_OF_NIBBLES 4
#define NULL_CHARACTER '\0'
#define ALPHA_LC_TO_INTEGER 87  /*!< small case conversion value              */
#define ALPHA_UC_TO_INTEGER 55  /*!< upper case conversion value              */
#define NUMCHAR_TO_INTEGER 48
#define uint32 u16
#define uint8 u8
#define FALSE 0
#define TRUE  1
#define CON_putchar Serial_SendBufferedChar
//#define CON_putchar sendByte

void CON_print(void* format,...)
{
    uint32 i=0;
    uint32 argint=0;
    uint32 PercentageFound=FALSE,ZeroFound=FALSE;
    uint32 NoOfNonzeroChars,NoOfZeros;
    uint32 NoOfChars = 0;
    uint32 upperCase = FALSE;   /*user ABCDEF of abcdef for hex digits        */
    uint8 argchar=0;
    uint8 CharSent[NO_OF_NIBBLES + 1];
    s16 hs;

    va_list argptr;
    va_start(argptr,format);

    while (*((uint8*)format) != NULL_CHARACTER)
    {
        switch (*((uint8*)format))
        {
        case '%':
            hs = 0;
            if (PercentageFound == TRUE)
            {
                PercentageFound=FALSE;
                CON_putchar('%');
            }
            else
            {
                PercentageFound=TRUE;
            }

            break;
        case 'h':
            if (PercentageFound)
            {
                hs++;
            }
            else
            {
                CON_putchar(*((uint8*)format));
            }
            break;
        case 'X':
            upperCase = TRUE;
            /* fall through                                               */
        case 'c':
        case 'l':
        case 'D':
        case 'U':
        case 'd':
        case 'u':
        case 'x':

            /* Initialise the counters to 0 */
            /*NoOfChars=0;*/
            NoOfNonzeroChars=0;
            NoOfZeros=0;

            /*if d/x was preceeded by % it represents some format */
            if (PercentageFound == TRUE)
            {
                int nibbles = NO_OF_NIBBLES;
                if ( hs == 2)
                {
                    nibbles = 2;
                    argint=(u8)va_arg(argptr,u8);
                }
                else if ( hs == 1) argint=(unsigned short)va_arg(argptr,unsigned short);
                else argint=(s16)va_arg(argptr,int);

                /*shift and take nibble by nibble from MSB to LSB*/
                for (i=nibbles; i > 0; i--)
                {
                    CharSent[i-1]=(uint8)((argint & ((0xf)<<(4*(nibbles-1)))) >> (4*(nibbles-1)));
                    if ((CharSent[i-1] >= 0x0a) && (CharSent[i-1] <= 0x0f))
                    {
                        /*if the nibble is greater than 9 */
                        CharSent[i-1] = (uint8)
                            (   (uint32)CharSent[i-1]
                                + (upperCase ? ALPHA_UC_TO_INTEGER
                                    : ALPHA_LC_TO_INTEGER)
                            );
                    }
                    else
                    {
                        /*if nibble lies from 0 to 9*/
                        CharSent[i-1]=(uint8)((uint32)CharSent[i-1]+NUMCHAR_TO_INTEGER);
                    }
                    /*see how many nibbles the number is actually filling*/
                    if (CharSent[i-1] == '0')
                    {
                        if (NoOfNonzeroChars == 0)
                        {
                            NoOfZeros++;
                        }
                        else
                        {
                            NoOfNonzeroChars++;
                        }
                    }
                    else
                    {
                        NoOfNonzeroChars++;
                    }
                    argint=argint << 4;
                }

                CharSent[nibbles] = '\0' ;

                PercentageFound=FALSE;

                /*If the user hasnt specified any number along with format display by default
                word size*/
                if (NoOfChars == 0)
                {
                    NoOfChars=nibbles;
                }

                /*print the number displaying as many nibbles as the user has given in print format*/
                if (NoOfChars >= NoOfNonzeroChars)
                {
                    for (i=0; i < (NoOfChars - NoOfNonzeroChars); i++)
                    {
                        CON_putchar('0');
                    }

                    for (i=NoOfNonzeroChars; i > 0; i--)
                    {
                        CON_putchar(CharSent[i-1]);
                    }
                }
                else
                {
                    for (i=NoOfNonzeroChars; i > 0; i--)
                    {
                        CON_putchar(CharSent[i-1]);
                    }
                }
            }

            /*if d/x wasnt preceeded by % it is not a format but a string;so just print it*/
            else
            {
                CON_putchar(*((uint8*)format));
            }
            /*clean the number of s8 for next cycle                     */
            NoOfChars = 0;
            /*use lower case by default                                   */
            upperCase = FALSE;
            break;
        case '0':

            /*if 0 was preceeded by % it represents some format */
            if (PercentageFound == TRUE)
            {
                ZeroFound=TRUE;
            }

            /*if 0 wasnt preceeded by % it is not a format but a string;so just print it*/
            else
            {
                CON_putchar(*((uint8*)format));
            }
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':

            /*if the number was preceeded by % and then a 0 it represents some format */
            if (ZeroFound == TRUE)
            {
                NoOfChars = (uint32) (*((uint8*)format) - NUMCHAR_TO_INTEGER);
                ZeroFound=FALSE;
            }

            /*if the number wasnt preceeded by % it is not a format but a string;so just print it*/
            else
            {
                CON_putchar(*((uint8*)format));
            }
            break;

        case '\n':
            /* Implicitly print a carriage return before each newline */
            CON_putchar('\r');
            CON_putchar(*((uint8*)format));
            break;
            /*if the character doesnt represent any of these formats just print it*/
        default:
            CON_putchar(*((uint8*)format));
            break;
        }
        format = ((uint8*)format)+1;
    }
    va_end(argptr);
}

void CON_hexdump(const u8 *buffer, u8 length)
{
    u8 i, rest;

    rest = length % 8;

    if (length >= 8)
        {
            for (i = 0; i < length/8; i++)
            {
                CON_print("%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx\n",
                         buffer[i*8],
                         buffer[i*8+1],
                         buffer[i*8+2],
                         buffer[i*8+3],
                         buffer[i*8+4],
                         buffer[i*8+5],
                         buffer[i*8+6],
                         buffer[i*8+7]
                        );
            }
        }
        if (rest > 0)
        {
            for (i = length/8 * 8; i < length; i++)
            {
            	CON_print("%hhx ", buffer[i]);
            }
            CON_print("\n");
        }
}
