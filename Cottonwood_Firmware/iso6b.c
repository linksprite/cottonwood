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
 *   \brief Implementation of ISO18000-6b protocol
 *
 *  Implementation of ISO18000-6b protocol.
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "c8051F340.h"
#include "as399x_config.h"
#include "global.h"
#include "uart.h"
#include "timer.h"
#include "bitbang.h"
#include "as399x.h"
#include "iso6b.h"
#include "crc16.h"

#if ISO6B

#define ISO6B_DEBUG 0 /*!< enable debug information via the serial interface */
#define ISO6B_READ_FIFO 0

/*
******************************************************************************
* LOCAL DEFINES
******************************************************************************
*/
#define IS06B_BACKSCATTER_TIMEOUT	4/*!< backscatter - the time needed by the tag to answer */
#define IS06B_WRITE_WAIT	15 /*!< according to iso6b the interrogator has to wait 15ms after issueing a write  */

/*
******************************************************************************
* LOCAL MACROS
******************************************************************************
*/

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
static XDATA t_bbData cmdData[50]; /*!< buffer used for the bitbang module */

/*
******************************************************************************
* LOCAL DATATYPES
******************************************************************************
*/
/*!
  * function pointer indicating the function to be used
  * in the next collision arbitration iteration
  */
typedef void (*iso6bCmd)(void);

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
static s8 iso6bPreparePreambleDelimiter(t_bbData *preData);
static void iso6bResync(void);
static s8 iso6bPrepareByte (t_bbData *manchdata, u8 info);
static s8 iso6bPrepareCRC(t_bbData *crcData, s16 crc);
static void iso6bSendCmd(u8 cmd);
static void iso6bSendResend(void);
static void iso6bSendInitialize(void);
static void iso6bSendSuccess(void);
static void iso6bSendFail(void);
static void iso6bSendSelect(u8 mask, u8* filter, u8 startaddress);
static s8 iso6bReadFifo(u8* uid, u8 length);

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
s8 iso6bInitialize()
{
    bbSetup(13); /* actually 12.5us */
    return ISO6B_ERR_NONE;
}

s8 iso6bDeinitialize()
{
    return ISO6B_ERR_NONE;
}

s8 iso6bOpen()
{
    as399xSelectLinkFrequency(0); /*  Set to 40kHz Link frequency */
    return ISO6B_ERR_NONE;
}

s8 iso6bClose()
{
    return ISO6B_ERR_NONE;
}

s8 iso6bInventoryRound (Tag* tags, u8 maxtags, u8 mask, u8* filter, u8 startaddress)
{
    u8 found = 0;
    u8 uid[8];
    u8 mem[8];
    s8 status;
    u8 cnt;
    u8 max_depth = 10;
    u8 max_commands = 50;
    iso6bCmd nextCmd;

    /* Set the length of the UID (8bytes + CRC) */
    as399xSingleWrite(AS399X_REG_RXLENGTHUP, 0);
    as399xSingleWrite(AS399X_REG_RXLENGTHLOW, 0x50);

    as399xEnterDirectMode();
    AS399X_ENABLE_SENDER();
    iso6bSendInitialize();
    iso6bSendSelect(mask, filter, startaddress);
    AS399X_ENABLE_RECEIVER(); /* Enable also the receiver and now the Tag should respond */
    status = iso6bReadFifo(uid, 8);
    if (ISO6B_ERR_NONE == status) /* only one tag in field */
    {
        iso6bRead(uid, 0x0, mem, 8);
        /* we received the data from the tag, the tag is now in DATA_EXCHANGE state
           and will not continue to participate in the collision arbitration */
        tags[0].epclen = 8;
        copyBuffer(uid, tags[0].epc, tags[0].epclen);
        found = 1;
    }
    else if (ISO6B_ERR_NOTAG == status) /* no tags found */
    {
        found = 0;
    }
    else
    {
        /* crc error - two  or more tags responded */
        nextCmd = iso6bSendFail;
        cnt = 2;
        while (maxtags && (cnt < max_depth) && max_commands--)
        {
            as399xEnterDirectMode();
            AS399X_ENABLE_SENDER();
            nextCmd();
            AS399X_ENABLE_RECEIVER();
            status = iso6bReadFifo(uid, 8);
            switch (status)
            {
            case ISO6B_ERR_PREAMBLE:
                nextCmd = iso6bSendFail;
                cnt ++;
                break;
            case ISO6B_ERR_NONE:
                /* send data_read command so the tag changes its state */
                iso6bRead(uid, 0x0, mem, 8);
                /* we received the data from the tag, the tag is now in DATA_EXCHANGE state
                   and will not continue to participate in the collision arbitration */
                tags[found].epclen = 8;
                copyBuffer(uid, tags[found].epc, tags[found].epclen);
                found ++;
                maxtags --;
                cnt --;
                nextCmd = iso6bSendSuccess;
                break;
            case ISO6B_ERR_NOTAG:
                nextCmd = iso6bSendSuccess;
                cnt --;
                break;
            default:
                if ( nextCmd != iso6bSendResend )
                {
                    nextCmd = iso6bSendResend;
                }
                else
                {
                    nextCmd = iso6bSendFail;
                    cnt ++;
                }
            }
        }
    }
#if ISO6B_DEBUG
    {
        u8 a;
        CON_print("Number of 6B Tags found: %hhx\n", found);
        for (a = 0; a < found; a++)
        {
            for (cnt = 0; cnt < tags[a].epclen; cnt++)
            {
                CON_print("%hhx ",tags[a].epc[cnt]);
            }
            CON_print("\n");
        }
        CON_print("\n");
    }
#endif
    if ( found == 0 )
    { /* From time to time reader ends up in invalid state. */
        as399xReset();
    }
    return found;
}

s8 iso6bRead(u8* uid, u8 addr, u8* buffer, u8 length)
{
    s8 cnt, i, result;
    u8 payload[11];
    u8 cmd = 0x51;
    u16 crc;

    length += 2; /* add CRC */
    /* Set the length of the UID */
    if (length > 32)
    {
        as399xSingleWrite(AS399X_REG_RXLENGTHUP, (length / 32) );
    }
    else
    {
        as399xSingleWrite(AS399X_REG_RXLENGTHUP, 0);
    }
    as399xSingleWrite(AS399X_REG_RXLENGTHLOW, (length % 32) * 8);
    length -= 2; /* remove CRC again */

    as399xEnterDirectMode();
    AS399X_ENABLE_SENDER();
    IO3(HIGH);
    mdelay(1);
    cnt = iso6bPreparePreambleDelimiter(cmdData);
    cnt += iso6bPrepareByte(&cmdData[cnt], cmd);
    payload[0] = cmd;

    for (i = 0; i < 8; i++)
    {
        payload[i+1] = uid[i];
        cnt += iso6bPrepareByte(&cmdData[cnt], payload[i+1]);
    }
    payload[9] = addr;
    cnt += iso6bPrepareByte(&cmdData[cnt], addr);
    payload[10] = length-1;
    cnt += iso6bPrepareByte(&cmdData[cnt], length-1);
    
    crc = calcCrc16(payload, 11);
    cnt += iso6bPrepareCRC(&cmdData[cnt], crc);

    bbRun(cmdData, cnt);
    IO3(HIGH);
    AS399X_ENABLE_RECEIVER();
    result = iso6bReadFifo(buffer, length);
    if ( result != 0 )
    { /* From time to time reader ends up in invalid state. */
        as399xReset();
    }
    return result;
}

s8 iso6bWrite(u8* uid, u8 startaddr, u8* buffer, u8 length)
{
    s8 cnt, i, result;
    u8 cmd = 0xd; /* command WRITE */
    u8 payload[11];
    u8 fifobuffer;
    u16 crc;

#if ISO6B_DEBUG
    CON_print("iso6bwrite %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx: addr=%hhx len=%hhx\n"
            ,uid[0] ,uid[1] ,uid[2] ,uid[3] ,uid[4] ,uid[5] ,uid[6] ,uid[7]
            ,startaddr, length);
#endif
    result = ISO6B_ERR_NONE;
    /* Set size of response (1byte + CRC) */
    as399xSingleWrite(AS399X_REG_RXLENGTHUP, 0);
    as399xSingleWrite(AS399X_REG_RXLENGTHLOW, 0x18);
    while (length -- && (ISO6B_ERR_NONE == result))
    {
        as399xEnterDirectMode();
        AS399X_ENABLE_SENDER();
        IO3(HIGH);
        mdelay(1);
        cnt = iso6bPreparePreambleDelimiter(cmdData);
        cnt += iso6bPrepareByte(&cmdData[cnt], cmd);
        payload[0] = cmd;

        for (i = 0; i < 8; i++)
        {
            payload[i+1] = uid[i];
            cnt += iso6bPrepareByte(&cmdData[cnt], payload[i+1]);
        }
        payload[9] = startaddr;
        cnt += iso6bPrepareByte(&cmdData[cnt], startaddr);
        startaddr ++;
        payload[10] = *buffer;
        cnt += iso6bPrepareByte(&cmdData[cnt], *buffer);
        buffer ++;
        crc = calcCrc16(payload, 11);
        cnt += iso6bPrepareCRC(&cmdData[cnt], crc);

        bbRun(cmdData, cnt);
        IO3(HIGH);
        AS399X_ENABLE_RECEIVER();
        result = iso6bReadFifo(&fifobuffer, 1);
        if (ISO6B_ERR_NONE == result)
        {
#if ISO6B_DEBUG
            CON_print("result =  %hhx\n",fifobuffer);
#endif
            result = (fifobuffer == 0) ?
                ISO6B_ERR_NONE :
                ISO6B_ERR_NACK;
        }
        as399xEnterDirectMode();
        AS399X_ENABLE_SENDER();
        IO3(HIGH);
        /* give the tag some time to write ... */
        mdelay(IS06B_WRITE_WAIT);
        /* ... and send a resync */
        iso6bResync();
        as399xExitDirectMode();
    }
    if ( result != 0 )
    { /* From time to time reader ends up in invalid state. */
        as399xReset();
    }
    return result;
}

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
/*!
 *****************************************************************************
 *  \brief  Write the preamble and delimiter into the output packet
 *
 *  This function writes the preamble and delimiter into the output packet.
 *  According to the ISO18000-6B protocol, a packet from the interrogator
 *  must start with a preamble and a delimiter. See section 8.1.4.3. and
 *  8.1.4.4.2. of the ISO18000-6 document.
 *  The data is written to an output buffer which is then sent to the
 *  AS3991 via the bitbang module.
 *
 *  \param  preData : A buffer where the preamble and delimiter will be
 *                    written. Buffer must be allocated
 *                    and has to have a size of 4 (at least)
 *
 *\return : Actual size of the buffer used by the function
 *****************************************************************************
 */
static s8 iso6bPreparePreambleDelimiter(t_bbData *preData)
{
    /* prepare the preamble and the delimiter1 */
    /* preamble is 9 times 01 - see section 8.1.4.3 of ISO18000-6b doc */
    /* delimiter 1 is 1100111010 - see section 8.1.4.4.2 of ISO18000-6b doc */
    preData[0].bbdata = 0x55; /* 01010101 */
    preData[1].bbdata = 0x55; /* 01010101 */
    preData[2].bbdata = 0x73;  /* 01 and 110011 */
    preData[3].bbdata = 0xa;  /* 1010 */
    preData[0].length = 8;
    preData[1].length = 8;
    preData[2].length = 8;
    preData[3].length = 4;
    return 4;
}

/*!
 *****************************************************************************
 *  \brief  Send out a resync to the tags via the AS3991
 *
 *  This function is used to send out a resync signal to the tags. This is
 *  needed to initialize the internal data recovery circuit of the tag
 *  which calibrates the tag. (Especially after read and write commands)
 *
 *****************************************************************************
 */
static void iso6bResync()
{
    t_bbData resyncdata[3];
    IO3(HIGH);
    mdelay(6);
    /* send 10 times 10 */
    resyncdata[0].bbdata = 0xaa; /* 10101010 */
    resyncdata[1].bbdata = 0xaa; /* 10101010 */
    resyncdata[2].bbdata = 0xa;  /* 1010 */
    resyncdata[0].length = 8;
    resyncdata[1].length = 8;
    resyncdata[2].length = 4;

    bbRun(resyncdata, 3); /* BANG */
    IO3(HIGH);
    mdelay(1);
}

/*!
 *****************************************************************************
 *  \brief  Convert a byte to manchester coded data
 *
 *  The data within an ISO18000-6B packet is manchester encoded. E.g. a 0
 *  is encoded as 01 and a 1 is encoded as 10.
 *
 *  \param  preData : A buffer where the preamble and delimiter will be
 *                    written. Buffer must be allocated
 *                    and has to have a size of 4 (at least)
 *
 *\return : Actual size of the buffer used by the function
 *****************************************************************************
 */
static s8 iso6bPrepareByte (t_bbData *manchdata, u8 info)
{
    manchdata[0].bbdata = 0;
    manchdata[1].bbdata = 0;
    manchdata[0].length = 8;
    manchdata[1].length = 8;

    if (info & BIT7) manchdata[0].bbdata |= 2 << 6;
    else manchdata[0].bbdata |= 1 << 6;
    if (info & BIT6) manchdata[0].bbdata |= 2 << 4;
    else manchdata[0].bbdata |= 1 << 4;
    if (info & BIT5) manchdata[0].bbdata |= 2 << 2;
    else manchdata[0].bbdata |= 1 << 2;
    if (info & BIT4) manchdata[0].bbdata |= 2 << 0;
    else manchdata[0].bbdata |= 1 << 0;
    if (info & BIT3) manchdata[1].bbdata |= 2 << 6;
    else manchdata[1].bbdata |= 1 << 6;
    if (info & BIT2) manchdata[1].bbdata |= 2 << 4;
    else manchdata[1].bbdata |= 1 << 4;
    if (info & BIT1) manchdata[1].bbdata |= 2 << 2;
    else manchdata[1].bbdata |= 1 << 2;
    if (info & BIT0) manchdata[1].bbdata |= 2 << 0;
    else manchdata[1].bbdata |= 1 << 0;
    return 2;
}

/*!
 *****************************************************************************
 *  \brief  Convert a the CRC checksum to manchester coded data
 *
 *  The data within an ISO18000-6B packet is manchester encoded. E.g. a 0
 *  is encoded as 01 and a 1 is encoded as 10. This function is used to convert
 *  the already calculated CRC-16 sum to manchester encoded data. The checksum
 *  is decribed in section 6.5.7.3. in ISO18000-6 doc
 *
 *  \param  crcData : A buffer where the encoded CRC will be
 *                    written. Buffer must be allocated
 *                    and has to have a size of 4 (at least)
 *  \param  crcData : CRC sum to send
 *
 *\return : Actual size of the buffer used by the function
 *****************************************************************************
 */
static s8 iso6bPrepareCRC(t_bbData *crcData, s16 crc)
{
    /* send inverted crc (ISO18000-6 spec, chapter 6.5.7.3) */
    crc = ~crc;
    iso6bPrepareByte (crcData, (0xff00 & crc) >> 8);
    crcData += 2;
    iso6bPrepareByte (crcData, 0xff & crc);
    return 4; /* return number of used bytes */
}

/*!
 *****************************************************************************
 *  \brief  Send a command in direct mode according to ISO18000-6
 *
 *  This function sends the command #cmd via the AS3991 to the tags.
 *  It only supports simple commands, i.e. commands where no additional
 *  information or data payload is needed. Note that the AS3991 needs to be
 *  in direct mode when using this function.
 *
 *  \param  cmd : Command code to send
 *  \param  crcData : CRC sum to send
 *
 *\return : Actual size of the buffer used by the function
 *****************************************************************************
 */
static void iso6bSendCmd(u8 cmd)
{
    s8 cnt;
    u16 crc;

    IO3(HIGH);
    mdelay(1);
    cnt = iso6bPreparePreambleDelimiter(cmdData);
    cnt += iso6bPrepareByte(&cmdData[cnt], cmd);

    crc = calcCrc16(&cmd, 1);
    cnt += iso6bPrepareCRC(&cmdData[cnt], crc);

    bbRun(cmdData, cnt);
    IO3(HIGH);
}


/*!
 *****************************************************************************
 *  \brief  Issue RESEND command according to ISO18000-6
 *
 *  The resend command is used during collision abitration. RESEND is issued
 *  when tags with count = 0 shall resend their UID while other tags shall
 *  keep their current count value.
 *
 *****************************************************************************
 */
static void iso6bSendResend()
{
#if ISO6B_DEBUG
    CON_print("R");
#endif
    iso6bSendCmd(0x15);
}

/*!
 *****************************************************************************
 *  \brief  Issue INITIALIZE command according to ISO18000-6
 *
 *  The initialize command sets all tags into state READY.
 *
 *****************************************************************************
 */
static void iso6bSendInitialize()
{
    iso6bSendCmd(0xa);
}

/*!
 *****************************************************************************
 *  \brief  Issue SUCCESS command according to ISO18000-6
 *
 *  The SUCCESS command is used during collision arbitration. SUCCESS is
 *  issued when NO reply has been received, i.e. all tags in the field
 *  have count > 0. As soon as the tags receive the SUCCESS command, they
 *  decrement their counter. If count is 0 the tag transmits its uid.
 *
 *****************************************************************************
 */
static void iso6bSendSuccess()
{
#if ISO6B_DEBUG
    CON_print("S");
#endif
    iso6bSendCmd(0x9);
}

/*!
 *****************************************************************************
 *  \brief  Issue FAIL command according to ISO18000-6
 *
 *  The FAIL command is used during collision arbitration. FAIL is
 *  issued when there was a CRC error, i.e. more then one tag have replied.
 *  Tags having a count of 0 receiving the FAIL command call a random function
 *  (rand(0, 1)). If the function returns 0 they leave their internal count
 *  and resend the UID. If it returns 1 then they increment their counter, i.e.
 *  count is 1 then. All tags having a count greater than 0 increment their
 *  counter when they receive the FAIL command, i.e. they move fruther away
 *  from sending their UID.
 *
 *****************************************************************************
 */
static void iso6bSendFail()
{
#if ISO6B_DEBUG
    CON_print("F");
#endif
    iso6bSendCmd(0x8);
}

/*!
 *****************************************************************************
 *  \brief  Issue the GROUP_SELECT command according to ISO18000-6
 *
 *  The GROUP_SELECT command starts the collision arbitration. All tags
 *  receiving this command change their state from READY to ID, i.e.
 *  they set their internal counter to 0 and send their UID.
 *
 *****************************************************************************
 */
static void iso6bSendSelect(u8 mask, u8* filter, u8 startaddress)
{
    u8 bsData[11];
    s8 cnt;
    s8 i;
    u16 crc;

    cnt = iso6bPreparePreambleDelimiter(cmdData);
    /* issue GROUP_SELECT_EQ command (0x0) */
    bsData[0] = 0;
    cnt += iso6bPrepareByte(&cmdData[cnt], bsData[0]);
    /* address */
    bsData[1] = startaddress;
    cnt += iso6bPrepareByte(&cmdData[cnt], bsData[1]);
    /* mask - if mask is 0 all tags are selected */
    bsData[2] = mask;
    cnt += iso6bPrepareByte(&cmdData[cnt], bsData[2]);
    /* data */
    for (i = 3; i < 11; i++)
    {
        bsData[i] = *filter++;
        cnt += iso6bPrepareByte(&cmdData[cnt], bsData[i]);
    }

    crc = calcCrc16(bsData, 11);
    cnt += iso6bPrepareCRC(&cmdData[cnt], crc);
    bbRun(cmdData, cnt);

}
#if ISO6B_READ_FIFO
static s8 iso6bReadFifo(u8* mem, u8 length)
{
    u8 fifolen;
    u16 i;
    s8 err = 0;

    mdelay(IS06B_BACKSCATTER_TIMEOUT*length);
    as399xExitDirectMode();

    i = as399xGetResponse(); /* Get response delivered from IRQ */
    as399xClrResponse();
    as399xSingleWrite(AS399X_REG_PROTOCOLCTRL, 0x06); /* back to the EPC settings */
    as399xSingleWrite(AS399X_REG_STATUSCTRL, 0x03); /* Tx and RX are on to keep the Tags powered */
    if (i & RESP_PREAMBLEERROR) /* preamble error */
    {
        err = ISO6B_ERR_PREAMBLE;
#if ISO6B_DEBUG
        CON_print("p");
#endif
    }
    else if (i & RESP_CRCERROR) /* crc error */
    {
        err = ISO6B_ERR_CRC;
#if ISO6B_DEBUG
        CON_print("c");
#endif
    }
    else if (i & RESP_RXIRQ) /* tag replied, read out fifo */
    {
        fifolen = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x1f;
        if (fifolen == length)
        {
            as399xFifoRead(length, mem);
#if ISO6B_DEBUG
            CON_print("+");
#endif
            err = ISO6B_ERR_NONE;

        }
        else
        {
#if ISO6B_DEBUG
            CON_print("s");
#endif
            err = ISO6B_ERR_AS399X_REG_FIFO;
        }
    }
    else /* no tag in field */
    {
        err = ISO6B_ERR_NOTAG;
    }
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    return err;
}
#else
/******************************************************************************************/
/******************************************************************************************/
/******************************************************************************************/
/******************************************************************************************/
/******************************************************************************************/
static s8 iso6bReadFifo(u8* mem, u8 length)
{
    s8 err = 0;
    u16 bits = length * 8;
    u16 bits2 = 16;
    u8 i = 0, mask = 0x80;
    u8 crc[2] = {0};
    u16 crcCalc;

    mem[0] = 0;

    timerStart_us( 1000 ); /* Documentation states 400 us @40kHz but it needs some more time through AS399x */
    /* Start receiving payload */
    while(bits)
    {
        while(!IO5PIN && !TIMER_IS_DONE());
        if (TIMER_IS_DONE()) break;
        if (IO6PIN) mem[i] |= mask;
        timerStart_us( 30 );
        bits--;
        mask >>= 1;
        if (mask == 0)
        {
            mask = 0x80;
            i++;
            mem[i] = 0;
        }
        while(IO5PIN && !TIMER_IS_DONE());
        if (TIMER_IS_DONE()) break;
        timerStart_us( 30 );
    }
    /* Now receive 16 bit crc */
    i = 0; mask = 0x80; 
    bits2 = 16;
    if (bits == 0) while(bits2)
    {
        while(!IO5PIN && !TIMER_IS_DONE());
        if (TIMER_IS_DONE()) break;
        if (IO6PIN) crc[i] |= mask;
        timerStart_us( 30 );
        bits2--;
        mask >>= 1;
        if (mask == 0)
        {
            mask = 0x80;
            i++;
            crc[i] = 0;
        }
        timerStart_us( 30 );
        while(IO5PIN && !TIMER_IS_DONE());
        timerStart_us( 30 );
    }
    /* Exit direct mode, now register access is again possible in parallel mode */
    as399xExitDirectMode();

    if ( bits != 0 || bits2 != 0)
    { /* not everything received */
        if ( bits == length * 8 )
        { /* no bits received */
            err = ISO6B_ERR_NOTAG;
            goto exit;
        }
#if ISO6B_DEBUG
        CON_print("s");
#endif
        err = ISO6B_ERR_CRC;
        goto exit;
    }
    crcCalc = ~calcCrc16(mem,length);
    if ( *((u16*)crc) != crcCalc )
    { /* crcs are not equal */
#if ISO6B_DEBUG
            CON_print("c");
#endif
        err = ISO6B_ERR_CRC;
    }
exit:
#if 0
    CON_print("ret %hhx length=%hhx, bits=%hx\n",err,length,bits);
    CON_print("bits2=%hx\n",bits2);
    CON_print("%x =crc= %x\n",*((u16*)crc), crcCalc);
    for ( i = 0; i< length; i++) CON_print("%hhx ",mem[i]);
    CON_print("\n");
#endif
    return err;
}
#endif
#endif
