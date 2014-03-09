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
  * @brief This file includes functions providing an implementation of the ISO6c aka GEN2 RFID EPC protocol.
  *
  * Detailed documentation of the provided functionality can be found in gen2.h.
  *
  * @author Ulrich Herrmann
  * @author Bernhard Breinbauer
  * @author Wolfgang Reichart
  */
#include "as399x_config.h"
#include "as399x.h"
#include "as399x_com.h"
#include "uart.h"
#include "timer.h"
#include "gen2.h"
#include "string.h"

/** Definition for debug output: epc.c */
#define EPCDEBUG          0
#define GENER_CMD_DEBUG   0

/*EPC Commands */
/** Definition for queryrep EPC command */
#define EPC_QUERYREP      0
/** Definition for acknowldege EPC command */
#define EPC_ACK           1
/** Definition for query EPC command */
#define EPC_QUERY         0x08
/** Definition for query adjust EPC command */
#define EPC_QUERYADJUST   0x09
/** Definition for select EPC command */
#define EPC_SELECT        0x0A
/** Definition for not acknowldege EPC command */
#define EPC_NAK           0xC0
/** Definition for request new RN16 or handle */
#define EPC_REQRN         0xC1
/** Definition for read EPC command */
#define EPC_READ          0xC2
/** Definition for write EPC command */
#define EPC_WRITE         0xC3
/** Definition for kill EPC command */
#define EPC_KILL          0xC4
/** Definition for lock EPC command */
#define EPC_LOCK          0xC5
/** Definition for access EPC command */
#define EPC_ACCESS        0xC6
/** Definition for blockwrite EPC command */
#define EPC_BLOCKWRITE    0xC7
/** Definition for blockerase EPC command */
#define EPC_BLOCKERASE    0xC8

#define GEN2_RESET_TIMEOUT 10

/*------------------------------------------------------------------------- */
/* local types */
/*------------------------------------------------------------------------- */

struct gen2InternalConfig{
    struct gen2Config config;
    u8 DR; /* Division ratio */
};

/*------------------------------------------------------------------------- */
/*global variables */
/*------------------------------------------------------------------------- */
static u16 gen2ResetTimeout = GEN2_RESET_TIMEOUT;

/** Global buffer for generating data, sending to the Tag.\n
  * Caution: In case of a global variable pay attention to the sequence of generating
  * and executing epc commands.
  */
static IDATA u8 buf_[8+EPCLENGTH+PCLENGTH+CRCLENGTH]; /*8->tx_length+EPCcommand+wordptr+wordcount+handle+broken byte */

/** Command buffer for generating epc commands. \n
  * Caution: In case of a global variable pay attention to the sequence of generating
  * and executing epc commands.
  */
static DATA u8 command_[4];

/** Temporary PC and EPC Buffer. \n
  */
static XDATA u8 pc_epc_temp_[EPCLENGTH+PCLENGTH];

/** Temporary Handle Buffer. \n
  */
static XDATA u8 handle_temp_[2];

static XDATA struct gen2InternalConfig gen2Config;

/*------------------------------------------------------------------------- */
/* local prototypes */
/*------------------------------------------------------------------------- */

/*------------------------------------------------------------------------- */
/** EPC SELECT command. send to the tag.
  * This function does not take or return a parameter
  */
static void gen2Select(u8* mask, u8 len);

/*------------------------------------------------------------------------- */
/** EPC QUERY command send to the EPC.
  * This function does not take or return a parameter
  */
static void gen2QueryStandard(u8 q);

/*------------------------------------------------------------------------- */
/** Reads the RN16 or Handle after an query, queryRep, QueryAdjust,
  * ReqRN or a reqRNTemp request.
  * @param *readbuf Buffer in which the RN16 or the handle is stored.
                           The Buffer has to be at least 2 bytes long.
  */
static void gen2ReadRN16OrHandle(u8 *readbuf);

/*------------------------------------------------------------------------- */
/** EPC ACK command send to the Tag. It uses the AS3990
  * internal ACK direct command.
  * This function does not take or return a parameter
  */
static void gen2Ack(void);

/*------------------------------------------------------------------------- */
/** EPC REQRN command send to the Tag. This function is used to
  * request a new RN16 or a handle from the tag.
  *
  * @param *handle Pointer to the first byte of the handle.
  * @param *dest_handle Pointer to the first byte of the backscattered handle.
  * @return The function returns an errorcode.
                  0x00 means no Error occoured.
                  0xff means Error occoured.
  */
static u8 gen2ReqRNHandleChar(u8 *handle, u8 *dest_handle);

static u8 gen2GetReply(u8 *handle);
static u8 gen2GetWriteToTagReply(u8 *handle);
static u8 gen2GetNxpCCReply(u8 *handle, u8 *config);

/*------------------------------------------------------------------------- */
/* global functions */
/*------------------------------------------------------------------------- */

static void gen2Select(u8* mask, u8 len)
{
    const u8 thr = 18; /* threshold */
    u8 *ptr;
    u8 j,i;
    //CON_print("gen2Select() session: %hhx\n", gen2Config.config.session);
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    as399xSingleCommand(AS399X_CMD_BLOCKRX);
    command_[0] = AS399X_CMD_RESETFIFO;
    command_[1] = AS399X_CMD_TRANSMCRC;
    buf_[0] = (len + 3) >> 4;           /* Upper transmit length byte (@the AS3990) */
    buf_[1] = 0xB | (((len + 3)&0xf)<<4) ;/* 3 Bytes to transmit & 5 broken bits & Broken Byte Bit = High */
    len = len<<3;            /* To have the length in Bit instead of Bytes */
    buf_[2] = ((EPC_SELECT<<4)&0xF0) | ((gen2Config.config.session<<1)&0x0E) /*Target*/ | ((0x00>>2)&0x01)/*Action*/;
    buf_[3] = ((0x00<<6)&0xC0)/*Action*/ | ((MEM_EPC<<4)&0x30)/*MEMBANK*/ | ((0x20>>4)&0x0F)/*EBV*/;
    buf_[4] = ((0x20<<4)&0xF0)/*EBV*/ | ((len>>4)&0x0F)/*Length*/;
    buf_[5] = ((len<<4)&0xF0)/*Length*/;

    i = 5; /* the counter set to last buffer location */
    for (j = len; j >= 8 ; j -= 8, mask++)
    {
        buf_[i] |= 0x0F & (*mask >> 4);
        i++;
        buf_[i] = 0xF0 & (*mask << 4);
    }
    buf_[i] |=  ((0x00<<3)&0x08)/*Truncate*/;
    len=len>>3;
    len += 6; /* Add the bytes for tx length and EPC command before continuing */
    ptr = buf_;

    if ( len > 26 )
    {
        as399xClrResponseMask( RESP_LOWLEVEL );
        as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, ptr, 26);
        len -= 26;
        ptr += 26;
        while ( len > thr )
        {
            as399xWaitForResponseTimed( RESP_LOWLEVEL, 15 );
            as399xClrResponseMask( RESP_LOWLEVEL );
            as399xContinuousWrite(AS399X_REG_FIFO, ptr,  thr);
            len -= thr;
            ptr += thr;
        }
        as399xWaitForResponseTimed( RESP_LOWLEVEL, 15 );
        as399xContinuousWrite(AS399X_REG_FIFO, ptr,  len);
    }
    else
    {
        as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, ptr, len);
    }
    as399xWaitForResponseTimed( RESP_TXIRQ, 15 );
    as399xSingleCommand(AS399X_CMD_BLOCKRX);

}

static void gen2QueryStandard(u8 q)
{
    u8 tx;

    // set session flags for QueryRep
    tx = as399xSingleRead(AS399X_REG_TXOPTGEN2);
    as399xSingleWrite(AS399X_REG_TXOPTGEN2, (tx & 0xFC) | (gen2Config.config.session & 0x03));

    as399xSingleCommand(AS399X_CMD_RESETFIFO);

    command_[0] = AS399X_CMD_QUERY;

    buf_[0] = 0x00;
    buf_[0] = ((gen2Config.DR<<5)&0x20)/*DR*/ | ((gen2Config.config.miller<<3)&0x18)/*M*/ | ((gen2Config.config.trext<<2)&0x04)/*TREXT*/ | 0x00/*SL*/;

    buf_[1] = ((gen2Config.config.session<<6)&0xC0)/*SESSION*/ | ((0x00<<5)&0x20)/*TARGET*/ | ((q<<1)&0x1E)/*Q*/;

    as399xCommandContinuousAddress(&command_[0], 1, AS399X_REG_FIFO, buf_, 2);
}

/*------------------------------------------------------------------------- */
static void gen2ReadRN16OrHandle(u8 *readbuf)
{
    as399xFifoRead(2, readbuf);
}

/*------------------------------------------------------------------------- */
static void gen2Ack(void)
{
    as399xSingleCommand(AS399X_CMD_ACKN);
}

/*------------------------------------------------------------------------- */
static u8 gen2ReqRNHandleChar(u8 *handle, u8 *dest_handle)
{
    u8 ret = GEN2_ERR_REQRN;

    command_[0] = AS399X_CMD_RESETFIFO;
    command_[1] = AS399X_CMD_TRANSMCRC;

    buf_[0] = 0x00;                      /*Upper Transmit length register */
    buf_[1] = 0x30;                      /*Lower Transmit length register: 3 Bytes to transmit */
    buf_[2] = EPC_REQRN;                 /*Command REQRN */

    buf_[3] = handle[0];
    buf_[4] = handle[1];

    as399xClrResponse();

    as399xSingleWrite( AS399X_REG_RXLENGTHUP, 0); 
    as399xSingleWrite( AS399X_REG_RXLENGTHLOW, 32); 

    as399xSingleCommand(AS399X_CMD_RESETFIFO);

    as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 5);
    as399xWaitForResponse(RESP_TXIRQ);
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    as399xWaitForResponse(RESP_RXDONE_OR_ERROR);
    if ((as399xGetResponse() & RESP_RXIRQ))                  /*getting response */
    {
        if (as399xGetResponse() & RESP_ERROR)                /*getting response */
        {
            ret = GEN2_ERR_REQRN;
            goto exit;
        }
        as399xClrResponse();
        gen2ReadRN16OrHandle(dest_handle);
        ret = GEN2_OK;
        goto exit;
    }
    else                                    /*no Response or error */
    {
        as399xClrResponse();
        ret = GEN2_ERR_REQRN;
        goto exit;
    }
exit:
#if EPCDEBUG
    if ( ret != GEN2_OK )
    {
        CON_print("AS399X_CMD_REQRNHANDLECHAR got no handle\n");
    }
#endif
    as399xSingleWrite( AS399X_REG_RXLENGTHLOW, 0); 
    as399xClrResponse();
    return ret;
}

/*------------------------------------------------------------------------- */
u8 gen2AccessTag(Tag *tag, u8 *password)
{
    u8 error;
    u8 count;
    u8 temp_rn16[2];
    u8 tagResponse[5];
    u8 dataLength;

    for (count = 0; count < 2; count++)
    {
        error = gen2ReqRNHandleChar(tag->handle, temp_rn16);
        if (error)
        {
            as399xClrResponse();
            return(error);
        }
#if EPCDEBUG
        else CON_print("got access RN16\n");
#endif

        command_[0] = AS399X_CMD_RESETFIFO;
        command_[1] = AS399X_CMD_TRANSMCRC;  /* no header Bit is expected in this command */

        buf_[0] = 0x00;
        buf_[1] = 0x50;                     /* no broken byte */
        buf_[2] = EPC_ACCESS;               /* Command EPC_ACCESS */
        buf_[3] = password[0] ^ temp_rn16[0];
        buf_[4] = password[1] ^ temp_rn16[1];
        buf_[5] = tag->handle[0];
        buf_[6] = tag->handle[1];

        as399xSingleCommand(AS399X_CMD_RESETFIFO);
        as399xClrResponse();
        as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 7);
        as399xClrResponse();
        as399xWaitForResponse(RESP_RXDONE_OR_ERROR);
        if (!(as399xGetResponse() & RESP_NORESINTERRUPT))                        /*getting response */
        {
            dataLength = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x0F;   /*Read response datacount */
            if (dataLength >=3) dataLength=3;
            as399xFifoRead(dataLength, tagResponse);/*Read Errorbyte */
            if  ( (tagResponse[1]== tag->handle[1] ) &&  \
                    (tagResponse[0]== (tag->handle[0] ) ) )
            {
#if EPCDEBUG
                if (count ==0) CON_print("first  part of access ok\n");
                if (count ==1) CON_print("second part of access ok\n");
#endif
                as399xSingleCommand(AS399X_CMD_RESETFIFO);
                as399xClrResponse();
                password += 2;  /* Increase pointer by two to fetch the next bytes;- */
                if (count ==1) return GEN2_OK;
            }
            else
            {
#if EPCDEBUG
                CON_print("handle not correct\n");
#endif
                as399xClrResponse();
                return GEN2_ERR_ACCESS;
            }
        }
        else
        {
#if EPCDEBUG
            CON_print("No Access Reply\n");
#endif
            count = 2;
            return GEN2_ERR_ACCESS;
        }
        as399xClrResponse();
    }
    return GEN2_OK;
}

/*------------------------------------------------------------------------- */
u8 gen2LockTag(Tag *tag, u8 *mask_action)
{
    u8 reply;
#if EPCDEBUG
    u8 count;
#endif

    command_[0] = AS399X_CMD_RESETFIFO;
    command_[1] = AS399X_CMD_TRANSMCRCEHEAD;

    buf_[0] = 0x00;
    buf_[1] = 0x59;                     /*broken byte (4 bits) and broken byte bit high */
    buf_[2] = EPC_LOCK;                 /*Command EPC_LOCK */

    buf_[3] = mask_action[0];
    buf_[4] = mask_action[1];

    buf_[5] = ((mask_action[2] ) & 0xF0) | ((tag->handle[0] >> 4) & 0x0F);
    buf_[6] = ((tag->handle[0] << 4) & 0xF0) | ((tag->handle[1] >> 4) & 0x0F);
    buf_[7] = (tag->handle[1] << 4) & 0xF0;
    as399xSingleWrite(AS399X_REG_IRQMASKREG, (AS399X_IRQ_MASK_ALL & ~AS399X_IRQ_NORESP & ~AS399X_IRQ_HEADER));  /*Disables the No Response Interrupt */
    as399xSingleCommand(AS399X_CMD_RESETFIFO);                                /*Resets the FIFO */
    as399xClrResponse();

#if EPCDEBUG
    CON_print("lock code\n");
    for (count=0; count<8; count++)
    {
        CON_print("%hhx ",buf_[count]);
    }
#endif
    as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 8);
    reply = gen2GetReply(tag->handle);
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);  /*Enable No Response Interrupt */
    return reply;
}

/*------------------------------------------------------------------------- */
u8 gen2KillTag(Tag *tag, u8 *password, u8 rfu)
{
    u8 error;
    u8 count;
    u8 temp_rn16[2];
    for (count = 0; count < 2; count++)
    {
        error = gen2ReqRNHandleChar(tag->handle, temp_rn16);
        if (error)
        {
            return(error);
        }

        command_[0] = AS399X_CMD_RESETFIFO;
        if (count==0)   command_[1] = AS399X_CMD_TRANSMCRC;      /* first command has no header Bit */
        if (count==1)   command_[1] = AS399X_CMD_TRANSMCRCEHEAD; /* second command has header Bit */

        buf_[0] = 0x00;
        buf_[1] = 0x57;                     /*broken byte (3 bits) and broken byte bit high */
        buf_[2] = EPC_KILL;                 /*Command EPC_KILL */

        buf_[3] = password[0] ^ temp_rn16[0];
        buf_[4] = password[1] ^ temp_rn16[1];

        buf_[5] = ((rfu << 5) & 0xE0) | ((tag->handle[0] >> 3) & 0x1F);
        buf_[6] = ((tag->handle[0] << 5) & 0xE0) | ((tag->handle[1] >> 3) & 0x1F);
        buf_[7] = (tag->handle[1] << 5) & 0xE0;

        as399xSingleWrite(AS399X_REG_IRQMASKREG, (AS399X_IRQ_MASK_ALL & ~AS399X_IRQ_NORESP& ~AS399X_IRQ_HEADER));  /*Disables the No Response Interrupt */

        as399xSingleCommand(AS399X_CMD_RESETFIFO);                               /*Resets the FIFO */
        as399xClrResponse();
        as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 8);
        error = gen2GetReply(tag->handle);
        as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);  /*Enable No Response Interrupt */
        if ( error ) 
        {
            return error;
        }
        password += 2;
    }
    return GEN2_OK;
}

/*------------------------------------------------------------------------- */
u8 gen2WriteWordToTag(Tag *tag, u8 memBank, u8 wordPtr,
                                  u8 *databuf)
{
    u8 reply, datab;

    u8 error;
    u8 temp_rn16[2];

    u16 bit_count = (2 + 2) * 8 + 1; /* + 2 bytes rn16 + 2bytes crc + 1 header bit */

    error = gen2ReqRNHandleChar(tag->handle, temp_rn16);
#if EPCDEBUG
    CON_print("wDtT %hhx->%hx\n",wordPtr,*((u16*)databuf));
#endif
    if (error)
    {
        as399xClrResponse();
        return(error);
    }

    command_[0] = AS399X_CMD_RESETFIFO;
    command_[1] = AS399X_CMD_TRANSMCRCEHEAD;

    buf_[0] = 0x00;
    buf_[1] = 0x65;                      /*6 bytes & 2 broken byte extra bits have to be transmitted */
    buf_[2] = EPC_WRITE;                 /*Command EPC_WRITE */

    buf_[3] = (memBank << 6) & 0xC0;
    buf_[3] = buf_[3] | ((wordPtr >> 2) & 0x3F);
    buf_[4] = (wordPtr << 6) & 0xC0;

    datab = databuf[0] ^ temp_rn16[0];
    buf_[4] = buf_[4] | ((datab >> 2) & 0x3F);
    buf_[5] = (datab << 6) & 0xC0;

    datab = databuf[1] ^ temp_rn16[1];
    buf_[5] = buf_[5] | ((datab >> 2) & 0x3F);
    buf_[6] = (datab << 6) & 0xC0;

    buf_[6] = buf_[6] | ((tag->handle[0] >> 2) & 0x3F);
    buf_[7] = (tag->handle[0] << 6) & 0xC0;
    buf_[7] = buf_[7] | ((tag->handle[1] >> 2) & 0x3F);
    buf_[8] = (tag->handle[1] << 6) & 0xC0;

    as399xSingleWrite(AS399X_REG_RXLENGTHLOW, bit_count & 0xff);
    as399xSingleWrite(AS399X_REG_RXLENGTHUP, (bit_count>>8) & 0x03);

    as399xSingleWrite(AS399X_REG_IRQMASKREG, (AS399X_IRQ_MASK_ALL & ~AS399X_IRQ_NORESP));  /*Disables the No Response Interrupt and Header Interrrupt */

    as399xSingleCommand(AS399X_CMD_RESETFIFO);                                /*Resets the FIFO */

    as399xClrResponse();

    as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 9);
    reply = gen2GetWriteToTagReply(tag->handle);
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);  /*Enable No Response Interrupt */
    return (reply);
}

/*------------------------------------------------------------------------- */
u8 gen2NXPChangeConfig(Tag *tag, u8 *databuf)
{
    u8 reply, datab;

    u8 error;
    u8 temp_rn16[2];

    error = gen2ReqRNHandleChar(tag->handle, temp_rn16);
    if (error)
    {
        as399xClrResponse();
        return(error);
    }

    command_[0] = AS399X_CMD_RESETFIFO;
    command_[1] = AS399X_CMD_TRANSMCRCEHEAD;

    buf_[0] = 0x00;
    buf_[1] = 0x70;                      /* exactly 7 bytes + CRC have to be transmitted */
    buf_[2] = 0xe0;                      /*Command 0xe007 change config */
    buf_[3] = 0x07;

    buf_[4] = 0;                         /* 8bit rfu */

    datab = databuf[0] ^ temp_rn16[0];
    buf_[5] = datab;

    datab = databuf[1] ^ temp_rn16[1];
    buf_[6] = datab;

    buf_[7] = tag->handle[0];
    buf_[8] = tag->handle[1];

    as399xSingleWrite(AS399X_REG_IRQMASKREG, (AS399X_IRQ_MASK_ALL & ~AS399X_IRQ_NORESP & ~AS399X_IRQ_HEADER));  /*Disables the No Response Interrupt and Header Interrrupt */

    as399xSingleCommand(AS399X_CMD_RESETFIFO);                                /*Resets the FIFO */

    as399xClrResponse();

    as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 9);
    reply = gen2GetNxpCCReply(tag->handle,databuf);
    return (reply);
}

/*------------------------------------------------------------------------- */
u8 gen2ReadFromTag(Tag *tag, u8 memBank, u8 wordPtr,
                          u8 wordCount, u8 *destbuf)
{

    u16 bit_count = (wordCount * 2 + 4) * 8 + 1; /* + 2 bytes rn16 + 2bytes crc + 1 header bit */
    u16 bit_count_tag_error_reply = (1 + 2 + 2) * 8 + 1; /* Error Code + 2 bytes rn16 + 2bytes crc + 1 header bit */

    u8 length = 0;
    u8 count = 0;
    u8 dataLength;
    command_[0] = AS399X_CMD_RESETFIFO;
    command_[1] = AS399X_CMD_TRANSMCRCEHEAD;

    buf_[0] = 0x00;
    buf_[1] = 0x55;                     /*broken byte 2 extra bits has to be transmitted */
    buf_[2] = EPC_READ;                 /*Command EPC_READ */
    buf_[3] = (memBank << 6) & 0xC0;
    buf_[3] = buf_[3] | ((wordPtr >> 2) & 0x3F);
    buf_[4] = (wordPtr << 6) & 0xC0;
    buf_[4] = buf_[4] | ((wordCount >> 2) & 0x3F);
    buf_[5] = (wordCount << 6) & 0xC0;
    buf_[5] = buf_[5] | ((tag->handle[0] >> 2) & 0x3F);
    buf_[6] = ((tag->handle[0] << 6) & 0xC0);
    buf_[6] = buf_[6] | ((tag->handle[1] >> 2) & 0x3F);
    buf_[7] = ((tag->handle[1] << 6) & 0xC0);

#if EPCDEBUG
    CON_print("gen2ReadFromTag() buf_[]\n");
    CON_hexdump(buf_, 8);
#endif


	as399xSingleWrite(AS399X_REG_IRQMASKREG, (AS399X_IRQ_MASK_ALL));

    as399xClrResponse();
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    as399xSingleWrite(AS399X_REG_RXLENGTHLOW, bit_count & 0xff);
    as399xSingleWrite(AS399X_REG_RXLENGTHUP, (bit_count>>8) & 0x03);
    as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 8);
    as399xWaitForResponse(RESP_TXIRQ);
    as399xClrResponse();
    as399xSingleCommand(AS399X_CMD_RESETFIFO);

    as399xWaitForResponse((RESP_RXDONE_OR_ERROR) | RESP_HIGHFIFOLEVEL);
    if (as399xGetResponse()&RESP_HEADERBIT) // Error Code Readout
    {
		as399xSingleWrite(AS399X_REG_RXLENGTHLOW, bit_count_tag_error_reply & 0xff);
		as399xSingleWrite(AS399X_REG_RXLENGTHUP, ( bit_count_tag_error_reply >> 8 ) & 0x03);

        as399xWaitForResponse(RESP_RXIRQ | RESP_CRCERROR | RESP_PREAMBLEERROR);
       	dataLength = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x0F;   /*Read response datacount */
        if (dataLength >=3) dataLength = 3;

        as399xFifoRead(dataLength, destbuf); /* Header Bit is set due to error */
        as399xSingleCommand(AS399X_CMD_RESETFIFO);
        return (destbuf[0] |= 0x80);
    }
    if (as399xGetResponse() & RESP_HIGHFIFOLEVEL)
    {
        count = AS399X_HIGHFIFOLEVEL;
        while (as399xGetResponse() & (RESP_HIGHFIFOLEVEL ) && count == AS399X_HIGHFIFOLEVEL)
        {
            if ( (count + length) > 2 * wordCount )
            { /* this omits RN16 */
                count = 2 * wordCount - length;
            }
            as399xFifoRead(count, destbuf);
            destbuf += count;
            length += count;
            as399xClrResponseMask(RESP_HIGHFIFOLEVEL );
            as399xWaitForResponse((RESP_RXDONE_OR_ERROR & ~RESP_NORESINTERRUPT) | RESP_HIGHFIFOLEVEL);
        }
    }
    as399xWaitForResponse(RESP_RXDONE_OR_ERROR);

    if (as399xGetResponse() & (RESP_NORESINTERRUPT | RESP_ERROR))
    {
        count = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x1F;            /*get the number of bytes */
        return GEN2_ERR_READ;   /*Maybe timeout, wrong handle or no response -> tag isn't present any more */
    }
    else
    {
        as399xClrResponse();
        count = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x1F;            /*get the number of bytes */
        if ( (count + length) > 2 * wordCount )
        { /* this omits RN16 */
            count = 2 * wordCount - length;
        }
        as399xFifoRead(count, destbuf);
        length += count;
    }
#if EPCDEBUG
    CON_print("received %hhx\n",length);
    CON_hexdump(destbuf, length);
#endif
    return (length ==  wordCount * 2)?0:GEN2_ERR_READ;
}

u8 gen2GenericCommand(Tag *tag, struct gen2GenericCmdData * commandData)
{
	u16 txByteCount = commandData->txBitCount / 8; /* number of full TX-bytes, including 2 bytes for RN16 */
	u8 txBrokenBits = commandData->txBitCount % 8; /* number of broken bits */
	u8 * destbuf = commandData->rxData;
	//u8 bitCount = commandData->rxBitCount;
	u8 length = 0;
	u8 byteCount = 0;
	u8 i = 0;
	u8 count = 0;
	u8 noRespTimeout = 0;
    u8 dataLength;
    u16 bit_count_tag_error_reply = ( 1 + 2 + 2) * 8 + 1; /* Error Code + 2 bytes rn16 + 2bytes crc + 1 header bit */

#if GENER_CMD_DEBUG
	CON_print("gen2GenericCommand txByteCount %u txBrokenBits %hhx, rxBitCount %u\n", txByteCount, txBrokenBits, commandData->rxBitCount);
	CON_print("handle %hhx-%hhx\n", tag->handle[0], tag->handle[1]);
#endif

	command_[0] = AS399X_CMD_RESETFIFO;
	command_[1] = commandData->directCommand;

	/* calculate the number of bytes which we await */
	byteCount = commandData->rxBitCount / 8;
	if (commandData->rxBitCount % 8)
	{
		/* also get the broken bits of last byte */
		byteCount++;
	}

	/* setup TX-Buffer */
	memset(buf_, 0x00, sizeof(buf_));
	/* first two bytes are written to TX-Length Byte 1 and 0 regs 0x1D and 0x1E */
	buf_[0] = (((txByteCount >> 8) & 0x0F) << 4) | ((txByteCount & 0xF0) >> 4); /* high nibble and middle nibble of full TX bytes */
	buf_[1] = (txByteCount & 0x0F) << 4; /* low nibble of full TX bytes */
	length = 2;
	if (txBrokenBits)
	{
		buf_[1] |= (0x01 | ((txBrokenBits << 1) & 0x0E)); /* number of broken bits in last byte and broken byte flag */
		txByteCount += 1; /* for copying the byte containing the broken bits */
	}
	/* the rest of the buffer will contain the command, buffer does NOT contain the RN a.k.a. tag->handle */
	for (i = 0; i < txByteCount - 2; i++)
	{
		buf_[length + i] = commandData->txData[i];
	}
	length += i;

#if GENER_CMD_DEBUG
    CON_print("buf_ w/o handle (length %hhx):\n", length);
    CON_hexdump(buf_, length);
#endif

	if (txBrokenBits)
	{
		/* clear all bits after broken bits */
		buf_[length-1] &= (0xFF << (8 - txBrokenBits));
		/* add the handle here */
		buf_[length-1] |= tag->handle[0] >> txBrokenBits;
		buf_[length]   |= tag->handle[0] << (8 - txBrokenBits);
		buf_[length]   |= tag->handle[1] >> txBrokenBits;
		buf_[length+1] |= tag->handle[1] << (8 - txBrokenBits);
	}
	else
	{
		/* add the handle here */
		buf_[length]   = tag->handle[0];
		buf_[length+1] = tag->handle[1];
	}
	length += 2;

#if GENER_CMD_DEBUG
	CON_print("buf_ with handle (length %hhx)\n", length);
	CON_hexdump(buf_, length);
#endif

	/* backup current value and set RX No Response Timeout to the desired value */
	noRespTimeout = as399xSingleRead(AS399X_REG_RXNORESPWAIT);
	as399xSingleWrite(AS399X_REG_RXNORESPWAIT, commandData->rxNoRespTimeout);

	/*Disables the No Response Interrupt and Header Interrrupt */
	if (commandData->rxNoRespTimeout == 0xFF)
	{
		as399xSingleWrite(AS399X_REG_IRQMASKREG, (AS399X_IRQ_MASK_ALL & ~AS399X_IRQ_NORESP)); // & ~AS399X_IRQ_HEADER
	}
	else
	{
		as399xSingleWrite(AS399X_REG_IRQMASKREG, (AS399X_IRQ_MASK_ALL));
	}

	as399xClrResponse();
	as399xSingleCommand(AS399X_CMD_RESETFIFO);
	as399xSingleWrite(AS399X_REG_RXLENGTHLOW, commandData->rxBitCount & 0xff);
	as399xSingleWrite(AS399X_REG_RXLENGTHUP, (commandData->rxBitCount>>8) & 0x03);
	as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, length);
	as399xWaitForResponse(RESP_TXIRQ);
	as399xClrResponse();
	as399xSingleCommand(AS399X_CMD_RESETFIFO);

	length = 0;

	if (commandData->rxBitCount)
	{
		as399xWaitForResponse((RESP_RXDONE_OR_ERROR & ~RESP_NORESINTERRUPT) | RESP_HIGHFIFOLEVEL);
		if (as399xGetResponse()&RESP_HEADERBIT)
		{
			as399xSingleWrite(AS399X_REG_RXLENGTHLOW, bit_count_tag_error_reply & 0xff);
			as399xSingleWrite(AS399X_REG_RXLENGTHUP, ( bit_count_tag_error_reply >> 8 ) & 0x03);

	        as399xWaitForResponse(RESP_RXIRQ | RESP_CRCERROR | RESP_PREAMBLEERROR);

	        dataLength = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x0F;   /*Read response datacount */
	        if (dataLength >=3) dataLength = 3;

	        as399xFifoRead(dataLength, destbuf); /* Header Bit is set due to error */
	        as399xSingleCommand(AS399X_CMD_RESETFIFO);

			as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);  /*Enable No Response Interrupt */
	        return (destbuf[0] |= 0x80);
		}
		if (as399xGetResponse() & RESP_HIGHFIFOLEVEL)
		{
			count = AS399X_HIGHFIFOLEVEL;
			while (as399xGetResponse() & (RESP_HIGHFIFOLEVEL) && count == AS399X_HIGHFIFOLEVEL)
			{
				as399xFifoRead(count, destbuf);
				destbuf += count;
				length += count;
				as399xClrResponseMask(RESP_HIGHFIFOLEVEL);
				as399xWaitForResponse((RESP_RXDONE_OR_ERROR & ~RESP_NORESINTERRUPT) | RESP_HIGHFIFOLEVEL);
			}
		}
		as399xWaitForResponse(RESP_RXDONE_OR_ERROR & ~ RESP_NORESINTERRUPT);

		if (as399xGetResponse() & (RESP_NORESINTERRUPT | RESP_ERROR))
		{
			commandData->actRxByteCount = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x1F;            /* get the number of bytes */
			return GEN2_ERR_READ;   /* Maybe timeout, wrong handle or no response -> tag isn't present any more */
		}
		else
		{
			as399xClrResponse();
			count = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x1F;            /* get the number of bytes */
			if ((count + length) > byteCount)
			{ /* this omits RN16 */
				count = byteCount - length;
			}
			as399xFifoRead(count, destbuf);
			length += count;
		}
		byteCount -= 2; /* CRC16 is omitted by reader chip */
#if GENER_CMD_DEBUG
		CON_print("received %hhx, required %hhx\n", length, byteCount);
		CON_hexdump(destbuf, length);
#endif
	}
	/* restore RX No Response Timeout */
	as399xSingleWrite(AS399X_REG_RXNORESPWAIT, noRespTimeout);
	commandData->actRxByteCount = length;
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);  /*Enable No Response Interrupt */
	/*
	 * if we end up here, we can return a GEN2_OK, even when expected data size (byteCount)
	 * does not match the received data size (length)
	 */
	return GEN2_OK;
}

/*------------------------------------------------------------------------------ */
void gen2PrintEPC(Tag *tag)
{

    u16 count;
    CON_print("Print PC %hhx %hhx\n",tag->pc[0], tag->pc[1]);
    CON_print("Print EPC ");
    for (count=0; count<(tag->epclen); count++)
    {
        CON_print("%hhx ",tag->epc[count]);
    }
    CON_print("\n");
}

/*------------------------------------------------------------------------- */
void gen2PrintTagInfo(Tag *tag, u8 epclen, u8 tagNr)
{
    u8 count = 0;

    CON_print("TAG %hhx:\n",tagNr);
    CON_print("RN16: %hhx %hhx\n",tag->rn16[1]
                                 ,tag->rn16[0]);
    CON_print("Number of read bytes: %d\n",epclen+2);
    CON_print("PC: %hhx %hhx", tag->pc[1]
                             , tag->pc[0]);
    CON_print("EPC: ");
    while (count < epclen)
    {
        CON_print("%hhx ",tag->epc[count]);
        count++;
    }
    CON_print("\n");

    CON_print("EPCLEN (bytes): %hhd\n",tag->epclen);

    CON_print("HANDLE: %hhx %hhx\n", tag->handle[0]
                                   , tag->handle[1]);
}

/*------------------------------------------------------------------------- */
/** Writes the information stored in the temporary global variables into
  * the Tags structure.
  * @param *tag Pointer to the Tag structure.
  * @param pc_epclen number of bytes of the pc_epclen.
  * @param *rn16 Pointer to the first byte of the rn16 buffer.
  * @param *pc_epc Pointer to the first byte of the pc and epc buffer.
  * @param *handle Pointer to the first byte of the handle buffer
  */
static void gen2SetTagInfo(Tag *tag, u8 epclen, u8 *pc,
                u8 *epc, u8 *handle)
{
    u8 count = 0;

    tag->pc[0] = pc[0];
    tag->pc[1] = pc[1];

    while (count < epclen)
    {
        tag->epc[count] = epc[count];
        count++;
    }

    tag->epclen = epclen;

    tag->handle[0] = handle[0];
    tag->handle[1] = handle[1];
}

/*------------------------------------------------------------------------- */
/** This function stores the tags in a global structure
 * It stores the tag information in the tag structure array.  This
 * function is highly timing critical as the caller gen2SearchForTags()
 * has put the tag into state Reply and this function has to put it into
 * state Acknowledged and thereafter into state Open/Secured. Both of
 * these transitions are protected by a timeout T2 which is defined to be
 * between 3 and 20 link * frequency periods.
 * @param *tag Pointer to the Tag structure.
 */
static s8 gen2StoreTagID (Tag *tag)
{
    u8 fifo_count, handle_byte_count;
    u16 bLength;
    u8 storeFlag, buf_[3];
    u8 *bufPtr;
    s8 ret_value = 0;

    as399xClrResponseMask(RESP_TXIRQ);
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    as399xSingleCommand(AS399X_CMD_ACKN); /* Transition to Reply->Acknowledged state, now we have T2 to go to Open state */
    as399xSingleWrite(AS399X_REG_RXLENGTHUP, 0x40);
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);
    as399xWaitForResponse(RESP_TXIRQ);

    as399xSingleCommand(AS399X_CMD_RESETFIFO);

    as399xClrResponse();
    as399xWaitForResponse( RESP_HEADERBIT | RESP_NORESINTERRUPT | RESP_PREAMBLEERROR | RESP_CRCERROR );
    /* HEADERBIT is special in this case, means: Got something from the Tag */
    /* NO RESESINTERRUPT is needed in case, ACK is not being understood or Tag die during response*/
    /* RESP_PREAMBLERROR is needed in case, the response of ACK is being not well understood.*/
    if (as399xGetResponse() & (RESP_CRCERROR | RESP_RXCOUNTERROR | RESP_PREAMBLEERROR | RESP_NORESINTERRUPT))
    {
#if EPCDEBUG
        CON_print("collision in gen2StoreTagId %x\n", as399xGetResponse());
#endif
        ret_value =  -1;
        goto end;
    }
    if (as399xGetResponse() & RESP_HEADERBIT ) 
    {
        /* Get PC and evaluate it, use continuous read otherwise data won't be removed from FIFO */
        as399xFifoRead (1,pc_epc_temp_);
        bLength = (pc_epc_temp_[0] & 0xF8) >> 2; /*  Have the EPC length in Byte */
        if ( bLength > EPCLENGTH ) 
        { /* This effectively prevents reading tags with epcs longer than our buffer, they will raise a crc error */
            bLength = EPCLENGTH;
        }
        tag->epclen = bLength;
        bLength += 4; /*  Add the length of PC and CRC */
        bLength <<= 3; /*  to have in Bit again */

        /* Write proper evaluated epc length into rx length register */
        buf_[0] = 0x40 | ((bLength>>8)&0x3);
        buf_[1] = bLength &0xff;
        bLength >>= 3; /*  to have in bytes again */

        as399xContinuousWrite(AS399X_REG_RXLENGTHUP, buf_, 2);
        as399xClrResponse();
        /* One byte was already received */
        bufPtr = pc_epc_temp_+1;
        bLength--;

        while (bLength >= 18)
        {
            as399xWaitForResponse( RESP_HIGHFIFOLEVEL | RESP_RXCOUNTERROR | RESP_PREAMBLEERROR );
            if(as399xGetResponse() & (RESP_RXCOUNTERROR | RESP_PREAMBLEERROR))
            {
                ret_value = -1;
                goto end;
            }
            fifo_count = as399xSingleRead(AS399X_REG_FIFOSTATUS);

            as399xClrResponseMask( RESP_HIGHFIFOLEVEL);

            as399xFifoRead(18, bufPtr);
            bufPtr += 18;
            bLength -= 18;
        }

        /* Wait for epc received */
        as399xWaitForResponse( RESP_RXIRQ );

        /* PREAMBLE error bit is set in this case, not an error! */
        if (!(as399xGetResponse() & (RESP_NORESINTERRUPT | RESP_RXCOUNTERROR | RESP_CRCERROR)))
        {
            storeFlag = 1;
        }
        else
        {
            storeFlag = 0;
        }

        as399xSingleCommand(AS399X_CMD_REQRN); /* Transition Acknowledged->Open */
        fifo_count = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x1F;
        if (fifo_count) /*  Only if something valid in the FIFO. */
        {
            bLength -= fifo_count;
            as399xFifoRead(fifo_count, bufPtr);
            }

        as399xSingleWrite (AS399X_REG_IRQMASKREG, 0x37);

        as399xWaitForResponse( RESP_TXIRQ);
        as399xSingleCommand(AS399X_CMD_RESETFIFO);
        as399xSingleWrite(AS399X_REG_RXLENGTHLOW, 0x20); /*  expecting the 2 bytes from the handle */
        as399xClrResponse();
        as399xWaitForResponse(RESP_RXDONE_OR_ERROR);
        handle_byte_count = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x1F;

        if (handle_byte_count)
        {
            as399xFifoRead(handle_byte_count, handle_temp_);
            gen2SetTagInfo(tag,tag->epclen , &pc_epc_temp_[0], &pc_epc_temp_[2], handle_temp_);
#if EPCDEBUG
            CON_print("rn16 = %hhx bytes %hhx %hhx\n",handle_byte_count,handle_temp_[0],handle_temp_[1]);
#endif
            tag->rssi = as399xSingleRead(AS399X_REG_RSSILEVELS);
            if (storeFlag)
            {
                ret_value = 1;
            }
        }
    }

end:
    /* Clean up */
    as399xSingleWrite(AS399X_REG_RXLENGTHUP, 0x00);
#if EPCDEBUG
    gen2PrintEPC(tag);
    CON_print("was gen2StoreTagID()\n");
    CON_print("last read %hhx\n",fifo_count);
    CON_print("bLength=%hx\n",bLength);
#endif
    return ret_value;
}

/** 
  This function is the iterator for the inventory round. It will listen
  for a new RN16, acknowledge the RN16 and as a last step do a Req_RN
  to get the found tag into Open state.

  @param
  @return -1 if a collision was probable
  @return 0 if no tag answered to the Query
  @return 1 if one tag was detected
  */
static s8 gen2GetTagInSlot( Tag *tag_ )
{
    s8 retval = 0;

    as399xWaitForResponse(RESP_TXIRQ);
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    as399xWaitForResponse(RESP_RXDONE_OR_ERROR);

    if (!(as399xGetResponse() & RESP_NORESINTERRUPT))         /*getting response */
    {
        if (as399xGetResponse() & RESP_ERROR)
        {
            retval = -1;
        }
        else
        {
            /* One tag is now in the Reply state, we have T2 to get to Acknowledged state */
            retval = gen2StoreTagID(tag_);
        }
    }
    else
    {
        *(u16*)tag_->rn16 = 0; /*no EPC response, maybe wrong RN16 */
    }
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    return retval;

}

/*------------------------------------------------------------------------- */
/** This function stores the tags in a global structure
 * It stores the tag information in the tag structure array.  This
 * function is highly timing critical as the caller gen2SearchForTags()
 * has put the tag into state Reply and this function has to put it into
 * state Acknowledged and thereafter into state Open/Secured. Both of
 * these transitions are protected by a timeout T2 which is defined to be
 * between 3 and 20 link * frequency periods.
 * @param *tag Pointer to the Tag structure.
 */
static s8 gen2StoreTagIDFast (Tag *tag, u8 postReplyCommand)
{
    u8 counter, fifo_count;
    u16 bLength;
    u16 fifo_bytes_read;
    u16 read_bytes_pc;
    u8 storeFlag, buf_[3];
    u8 *bufPtr;
    s8 ret_value = 0;
    read_bytes_pc = 0;
    fifo_bytes_read = 0;

    as399xWaitForResponse(RESP_TXIRQ | RESP_RXIRQ);
    as399xWaitForResponse(RESP_RXDONE_OR_ERROR);
    if (as399xGetResponse() & (RESP_NORESINTERRUPT|RESP_ERROR))         /*getting response */
    {
        ret_value = -1;
        goto error;
    }
    as399xClrResponseMask(RESP_TXIRQ);
    buf_[0] = AS399X_CMD_RESETFIFO;
    buf_[1] = AS399X_CMD_ACKN;
    as399xContinuousCommand(buf_, 2);
    as399xSingleWrite(AS399X_REG_RXLENGTHUP, 0x40); /* activate header bit == got something interrupt */
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);
    as399xWaitForResponse(RESP_TXIRQ);

    as399xSingleCommand(AS399X_CMD_RESETFIFO);

    as399xClrResponse();
    as399xWaitForResponse( RESP_HEADERBIT | RESP_NORESINTERRUPT | RESP_PREAMBLEERROR | RESP_CRCERROR );
    /* HEADERBIT is special in this case, means: Got something from the Tag */
    /* NO RESESINTERRUPT is needed in case, ACK is not being understood or Tag die during response*/
    /* RESP_PREAMBLERROR is needed in case, the response of ACK is being not well understood.*/
    if (as399xGetResponse() & (RESP_CRCERROR | RESP_RXCOUNTERROR | RESP_PREAMBLEERROR | RESP_NORESINTERRUPT))
    {
        ret_value =  -2;
        goto error;
    }
    if (as399xGetResponse() & RESP_HEADERBIT ) 
    {
        /* Get PC and evaluate it, use continuous read otherwise data won't be removed from FIFO */
        as399xFifoRead (1,tag->pc);
        bLength = (tag->pc[0] & 0xF8) >> 2; /*  Have the EPC length in Byte */
        if ( bLength > EPCLENGTH ) 
        { /* This effectively prevents reading tags with epcs longer than our buffer, they will raise a crc error */
            bLength = EPCLENGTH;
        }
        tag->epclen = bLength;
        bLength += 4; /*  Add the length of PC and CRC */
        read_bytes_pc = bLength;
        bLength <<= 3; /*  to have in Bit again */

        /* Write proper evaluated epc length into rx length register */
        buf_[0] = 0x40 | ((bLength>>8)&0x3);
        buf_[1] = bLength &0xff;
        bLength >>= 3; /*  to have in bytes again */

        as399xContinuousWrite(AS399X_REG_RXLENGTHUP, buf_, 2);
        as399xClrResponse();
        /* One byte was already received */
        bufPtr = tag->pc+1;
        bLength--;

        while (bLength >= 18)
        {
            as399xWaitForResponse( RESP_HIGHFIFOLEVEL | RESP_RXCOUNTERROR | RESP_PREAMBLEERROR );
            if(as399xGetResponse() & (RESP_RXCOUNTERROR | RESP_PREAMBLEERROR | RESP_NORESINTERRUPT))
            {
                ret_value = -3;
                goto error;
            }
            fifo_count = as399xSingleRead(AS399X_REG_FIFOSTATUS);



            as399xClrResponseMask( RESP_HIGHFIFOLEVEL);

            as399xFifoRead(18, bufPtr);
            fifo_bytes_read += 18;
            bufPtr += 18;
            bLength -= 18;
        }
        /* Start reading EPC from fifo even before receive has finished. This allows us to send the next command out faster. */
        counter = 0;
        while (!(as399xGetResponse() & RESP_RXDONE_OR_ERROR))
        {
            fifo_count += 3; NOP(); NOP(); fifo_count += 3; NOP(); NOP(); fifo_count += 3;  //waste some time to reduce polling on fifo status register
            fifo_count = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x1F;
            if (fifo_count > 1)     // Read 1 byte from fifo when at least 2 are available.
            {   /* read out only one byte at once because sometimes the parallel if is
                producing wrong data when at the same time rx is in progress. */
                as399xFifoRead(1, bufPtr);
                fifo_bytes_read += 1;
                bufPtr += 1;
                bLength -= 1;
            }
            counter++;
            if (counter > 200 )     // fail safe, if we loose a response of the tag we might get caught in this loop
            {
                //CON_print("\n loop failed \n");
                break;
            }
        }
        /* Wait for full epc (or no response) */

        as399xWaitForResponse( RESP_RXDONE_OR_ERROR );

        /* PREAMBLE error bit is set in this case, not an error! */
        if (!(as399xGetResponse() & (RESP_NORESINTERRUPT | RESP_RXCOUNTERROR | RESP_CRCERROR)))
        {
            storeFlag = 1;
        }
        else
        {
            storeFlag = 0;
        }

        /* Send out next command now, to prevent violation of T2 (if QueryRep is sent this will change current session flag on tag). */
        as399xClrResponse();
        as399xSingleCommand(postReplyCommand);

        /* Read the rest of the EPC */
        fifo_count = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x1F;
        if (fifo_count) /*  Only if something valid in the FIFO. */
        {
            bLength -= fifo_count;
            as399xFifoRead(fifo_count, bufPtr);
            fifo_bytes_read += fifo_count;
            }
        as399xSingleCommand(AS399X_CMD_RESETFIFO);
        tag->rssi = as399xSingleRead(AS399X_REG_RSSILEVELS);

        if (storeFlag)
        {
            ret_value = 1;
        }

        //Control the length
        read_bytes_pc-= 1;
        if ( read_bytes_pc != fifo_bytes_read)
        {
            ret_value = -4;
            goto error;
            //CON_print("expected Bytes pc: %hhhhx\n", read_bytes_pc);
            //CON_print("read fifo_bytes: %hhhhx\n", fifo_bytes_read);
        }

        goto end;
    }
error:
    as399xClrResponse();
    /* The post reply (QUERYREP) command needs a RESETFIFO */
    buf_[0] = AS399X_CMD_RESETFIFO;
    buf_[1] = postReplyCommand;
    as399xContinuousCommand(buf_, 2);
end:
    /* Clean up */
    as399xSingleWrite(AS399X_REG_RXLENGTHUP, 0x00);
    return ret_value;
}

unsigned gen2SearchForTags(Tag *tags_
                          , u8 maxtags
                          , u8* mask
                          , u8 length
                          , u8 q
                          , bool (*cbContinueScanning)(void)
                          , bool useMaskToSelect
                          )
{
    unsigned num_of_tags_ = 0;
    u16 collisions = 0;
    u16 slot_count;
    u8 count1 = 0;
    u8 addRounds = 3; /* the maximal number of rounds performed */
#if EPCDEBUG
    CON_print("Searching for Tags, maxtags=%hhd, length=%hhd q=%hhd\n", maxtags, length, q);
    {
        int i;
        CON_print("mask= ");
        for ( i = 0; i< length; i++)
        {
            CON_print("%hhx ",mask[i]);
        }
        CON_print("\n");
    }
    CON_print("-------------------------------\n");
#endif

    as399xClrResponse();
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    for (count1=0; count1 < maxtags; count1++)   /*Reseting the TAGLIST */
    {
        *((u16*)tags_[count1].rn16) = 0;
        tags_[count1].epclen=0;
    }
#if EPCDEBUG
    CON_print("Sending Select\n");
#endif
    as399xClrResponse();

    if (useMaskToSelect)
        gen2Select(mask,length); /* select command with mask and length of mask */
    else
        gen2Select(0,0); /* Select all */
    udelay(300); /* According Standard we have to wait 300 us */

    as399xClrResponse();
    gen2QueryStandard(q);            /*StandardQuery on the Beginning */
#if EPCDEBUG
    CON_print(" ");
#endif
    do
    {
        bool goOn;
        collisions = 0;
        slot_count = 1UL<<q;   /*get the maximum slot_count */
        do
        {
            if (num_of_tags_ >= maxtags)
            {/*    ERROR it is not possible to store more than maxtags Tags */
                break;
            }
            switch (gen2GetTagInSlot(tags_+num_of_tags_))
            {
                case -1:
#if EPCDEBUG
                    CON_print("collision\n");
#endif
                    collisions++;
                    break;
                case 1:
                    if ( memcmp(tags_[num_of_tags_].epc,mask,length ))
                    { /* normally the should always be equal, just to be sure... */
#if EPCDEBUG
                        CON_print("found EPC did not match mask!");
#endif
                    }
                    else
                    {
                        num_of_tags_++;
                    }
                    break;
                case 0:
#if EPCDEBUG
                    CON_print("NO EPC response -> empty Slot\n");
#endif
                default:
                    break;
            }
            slot_count--;
            as399xClrResponse();
            goOn = cbContinueScanning();
            if (num_of_tags_ < maxtags && slot_count && goOn ) as399xSingleCommand(AS399X_CMD_QUERYREP);
        } while (slot_count && goOn );
        addRounds--;
#if EPCDEBUG
        CON_print("q=%hhx, collisions=%x, num_of_tags=%x",q,collisions,num_of_tags_);
#endif
        if( collisions )
            if( collisions >= (1UL<<q) /4)
            {
                q++;
#if EPCDEBUG
                CON_print("->++\n");
#endif
                as399xSingleCommand(AS399X_CMD_QUERYADJUSTUP);
            }
            else if( collisions < (1UL<<q) /8)
            {
                q--;
#if EPCDEBUG
                CON_print("->--\n");
#endif
                as399xSingleCommand(AS399X_CMD_QUERYADJUSTDOWN);
            }
            else
            {
#if EPCDEBUG
                CON_print("->==\n");
#endif
                as399xSingleCommand(AS399X_CMD_QUERYADJUSTNIC);
            }
        else
        {
#if EPCDEBUG
            CON_print("->!!\n");
#endif
            addRounds = 0;
        }
    }while(num_of_tags_ < maxtags && addRounds && cbContinueScanning() );

#if EPCDEBUG
    CON_print("-------------------------------\n");
    bin2Chars(num_of_tags_, buf_);
    CON_print(buf_);
    CON_print("  Tags found");
    CON_print("\n");
#endif
    if (num_of_tags_ == 0)
    {
        gen2ResetTimeout--;
        if (gen2ResetTimeout == 0)
        {
            as399xAntennaPower(0);      /* we do not want that rf power is turned on after reset */
            as399xReset();
            gen2Configure(&gen2Config.config);
            gen2ResetTimeout = GEN2_RESET_TIMEOUT;
    }
    }
    else
    {
        gen2ResetTimeout = GEN2_RESET_TIMEOUT;
    }
    return num_of_tags_;
}

unsigned gen2SearchForTagsFast(Tag *tags_
                          , u8 maxtags
                          , u8* mask
                          , u8 length
                          , u8 q
                          , bool (*cbContinueScanning)(void)
                          , u8 startCycle
                          )
{
    unsigned num_of_tags_ = 0;
    u16 slot_count;
    u8 count1 = 0;

#if EPCDEBUG
    CON_print("Searching for Tags, maxtags=%hhd, length=%hhd q=%hhd\n",maxtags,length,q);
    {
        int i;
        CON_print("mask= ");
        for ( i = 0; i< length; i++)
        {
            CON_print("%hhx ",mask[i]);
        }
        CON_print("\n");
    }
    CON_print("-------------------------------\n");
#endif

    as399xClrResponse();
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    for (count1=0; count1 < maxtags; count1++)   /*Reseting the TAGLIST */
    {
        *((u16*)tags_[count1].rn16) = 0;
        tags_[count1].epclen=0;
    }
    as399xClrResponse();

    if (startCycle)
        gen2Select(mask,length); /* select command with mask and length of mask */

    udelay(300); /* According Standard we have to wait 300 us */

    as399xClrResponse();
    gen2QueryStandard(q);            /*StandardQuery on the Beginning */

    {
        bool goOn = 1;
        u8 cmd = AS399X_CMD_QUERYREP;
        slot_count = 1UL<<q;   /*get the maximum slot_count */
        do
        {
            if (num_of_tags_ >= maxtags)
            {/*    ERROR it is not possible to store more than maxtags Tags */
                break;
            }
            slot_count--;
            if (gen2StoreTagIDFast(tags_+num_of_tags_, cmd) == 1)
            {
                num_of_tags_++;
            }
            goOn = cbContinueScanning();
        } while (slot_count && goOn );
        /* Wait until last cmd has been sent */
        as399xWaitForResponse(RESP_TXIRQ);
        as399xSingleCommand(AS399X_CMD_BLOCKRX);
        as399xSingleCommand(AS399X_CMD_RESETFIFO);
        as399xClrResponse();
    }

#if EPCDEBUG
    CON_print("-------------------------------\n");
    bin2Chars(num_of_tags_, buf_);
    CON_print(buf_);
    CON_print("  Tags found");
    CON_print("\n");
#endif
    if (num_of_tags_ == 0)
    {
        gen2ResetTimeout--;
        if (gen2ResetTimeout == 0)
        {
            as399xAntennaPower(0);      /* we do not want that rf power is turned on after reset */
            as399xReset();
            gen2Configure(&gen2Config.config);
            gen2ResetTimeout = GEN2_RESET_TIMEOUT;
        }
    }
    else
    {
        gen2ResetTimeout = GEN2_RESET_TIMEOUT;
    }
    return num_of_tags_;
}
/*------------------------------------------------------------------------- */
u8 gen2SetProtectBit(Tag *tag)
{
    u8 reply;
    u8 error;
    u8 temp_rn16[2];
    error = gen2ReqRNHandleChar(tag->handle, temp_rn16);
    if (error)
    {
        as399xClrResponse();
        return(error);
    }
    command_[0] = AS399X_CMD_RESETFIFO;
    command_[1] = AS399X_CMD_TRANSMCRCEHEAD;

    buf_[0] = 0x00;
    buf_[1] = 0x40;                      /*4bytes  have to be transmittet */
    buf_[2] = 0xE0;                       /* EPC_SETPROTECT       Command EPC_ Set Protection Bit */
    buf_[3] = 0x01;                       /* EPC_SETPROTECT       Command EPC_ Set Protection Bit */
    buf_[4] = tag->handle[0];
    buf_[5] = tag->handle[1];
    as399xSingleWrite(AS399X_REG_IRQMASKREG, (AS399X_IRQ_MASK_ALL & ~AS399X_IRQ_NORESP & ~AS399X_IRQ_HEADER));  /*Disables the No Response Interrupt and Header Interrrupt */
    as399xSingleCommand(AS399X_CMD_RESETFIFO);                                /*Resets the FIFO */
    as399xClrResponse();
    as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 6);
    reply = gen2GetReply(tag->handle);
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);  /*Enable No Response Interrupt */
    return (reply);
}

u8 gen2ReSetProtectBit(Tag *tag, u8 *password)
{
    u8 reply;
    u8 error;
    u8 temp_rn16[2];
    error = gen2ReqRNHandleChar(tag->handle, temp_rn16);
    if (error)
    {
        as399xClrResponse();
        return(error);
    }
    command_[0] = AS399X_CMD_RESETFIFO;
    command_[1] = AS399X_CMD_TRANSMCRCEHEAD;
    buf_[0] = 0x00;
    buf_[1] = 0x80;                      /*8bytes  have to be transmittet */
    buf_[2] = 0xE0;                      /* NXP- Command set */
    buf_[3] = 0x02;                      /* EPC_RESETPROTECT       Command EPC_ Set Protection Bit */
    buf_[4] = password[0] ^ temp_rn16[0];
    buf_[5] = password[1] ^ temp_rn16[1];
    buf_[6] = password[2] ^ temp_rn16[0];
    buf_[7] = password[3] ^ temp_rn16[1];
    buf_[8] = tag->handle[0];
    buf_[9] = tag->handle[1];
    as399xSingleWrite(AS399X_REG_IRQMASKREG,(AS399X_IRQ_MASK_ALL & ~AS399X_IRQ_NORESP & ~AS399X_IRQ_HEADER));  /*Disables the No Response Interrupt and Header Interrrupt */
    as399xSingleCommand(AS399X_CMD_RESETFIFO);                                /*Resets the FIFO */
    as399xClrResponse();
    as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 10);
    reply = gen2GetReply(tag->handle);
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);  /*Enable No Response Interrupt */
    return (reply);
}

/*------------------------------------------------------------------------- */
u8 gen2ChangeEAS(Tag *tag,u8 value)
{
    u8 reply;
    command_[0] = AS399X_CMD_RESETFIFO;
    command_[1] = AS399X_CMD_TRANSMCRCEHEAD;

    buf_[0] = 0x00;
    buf_[1] = 0x43;                      /*4bytes  have to be transmittet plus one Bit */
    buf_[2] = 0xE0;                      /* EPC_Change EAS */
    buf_[3] = 0x03;                      /* EPC_Change EAS */
    if (value) buf_[4]=0x80; /* EAS Bit is set or not. */
    else buf_[4]=0x00;

    buf_[4] = (buf_[4] | (tag->handle[0]&0xFE) >>1) ;
    buf_[5] = ((tag->handle[0]&0x01) <<7) | ((tag->handle[1]&0xFE) >>1);
    buf_[6] = ((tag->handle[1]&0x01) <<7);

    as399xSingleWrite(AS399X_REG_IRQMASKREG, (AS399X_IRQ_MASK_ALL & ~AS399X_IRQ_NORESP & ~AS399X_IRQ_HEADER));  /*Disables the No Response Interrupt and Header Interrrupt */
    as399xSingleCommand(AS399X_CMD_RESETFIFO);                                /*Resets the FIFO */
    as399xClrResponse();
    as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 7);
    reply = gen2GetReply(tag->handle);
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);  /*Enable No Response Interrupt */
    return (reply);
}

/*------------------------------------------------------------------------- */
u8 gen2Calibrate(Tag *tag)
{
    u8 reply, error;
    u8 temp_rn16[2];
    error = gen2ReqRNHandleChar(tag->handle, temp_rn16);
    if (error)
    {
        as399xClrResponse();
        return(error);
    }
    command_[0] = AS399X_CMD_RESETFIFO;
    command_[1] = AS399X_CMD_TRANSMCRCEHEAD;

    buf_[0] = 0x00;
    buf_[1] = 0x40;                      /*4bytes  have to be transmitted */
    buf_[2] = 0xE0;                      /* EPC_SETPROTECT       Command EPC_ Set NXP COMMAND */
    buf_[3] = 0x05;                      /* EPC_SETCALIBRATE     Command EPC_ Set Calibration */
    buf_[4] = tag->handle[0];
    buf_[5] = tag->handle[1];
    /*Disable the No Response Interrupt and Header Interrupt */
    as399xSingleWrite(AS399X_REG_IRQMASKREG, (AS399X_IRQ_MASK_ALL & ~AS399X_IRQ_NORESP & ~AS399X_IRQ_HEADER));
    as399xSingleCommand(AS399X_CMD_RESETFIFO);                                /*Resets the FIFO */
    as399xClrResponse();
    as399xCommandContinuousAddress(&command_[1], 1, AS399X_REG_TXLENGTHUP, buf_, 6);
    reply = gen2GetReply(tag->handle);
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);  /*Enable No Response Interrupt */
    return (reply);
}

u8 gen2ChallengeCommand(u8 commandFlags, cryptoSuiteId, u16 msgBitLength, u8 * message, u8 * output, u8 *outputLength)
{
	u8 ret = GEN2_OK;

	memset(buf_, 0x00, sizeof(buf_));
	buf_[2] =  0xD4;								/* challenge command				*/
	buf_[3] =  commandFlags << 3;					/* RFU, Buffer, IncRepLen, immed)	*/
	buf_[3] |= ((cryptoSuiteId >> 8) & 0x03) << 1;	/* bits 9 and 8 of CSI				*/
	buf_[3] |= (cryptoSuiteId >> 7) & 0x01;			/* bit 7 of CSI						*/
	buf_[4] =  (cryptoSuiteId << 1) & 0xfe;			/* bits 6 to 0 of CSI				*/
	if (msgBitLength == 0x8266)
	{
		/* proprietary NXP ECC challenge -> 16 bits EBV coded length */
		buf_[4] |= msgBitLength >> 15;				/* bit 15 of EBV coded length		*/
		buf_[5] = (msgBitLength >> 7);				/* bits 14 to 7 of EBV coded length	*/
		buf_[6] = (msgBitLength & 0xFF) << 1;		/* bits 6 to 0 of EBV coded length	*/
													/* one bit remaining in byte buf_[6]*/
		/* FIXME: NXP specific ECC Challenge Command layout: */
		{
			/* 3 bits auth method + 1 bit step + 4 bits flags + 8 bits key-ID = 16bits extra data (all zeros) */
			u8 zeroBits[2] = { 0 };
			bitArrayCopy(zeroBits, 0, 16, &buf_[6], 7);
		}
		bitArrayCopy(message, 0, 342, &buf_[8], 7);
	}
	else
	{
		/* Challenge according to GEN2 Specification -> 12 bits length */
		buf_[4] |= msgBitLength >> 11;				/* bit 11 of 12 bit length word		*/
		buf_[5] =  msgBitLength >> 3;				/* bits 10 to 3 of 12 bit length	*/
		buf_[6] =  msgBitLength << 5;				/* bits 2 to 0 of 12 bit length 	*/
													/* 5 bits remaining in byte buf_[6]	*/
		bitArrayCopy(message, 0, msgBitLength, &buf_[6], 3);
	}
	// FIXME copy output into return buffer
	output = output;
	// FIXME copy output length into return variable
	outputLength = 0;

	return ret;
}

static u8 gen2GetReply(u8 *handle)
{
    u8 ret = GEN2_ERR_NOREPLY;
    u8 tagResponse [2];
    as399xWaitForResponse(RESP_TXIRQ);
    as399xClrResponse();
    as399xSingleCommand(AS399X_CMD_RESETFIFO);                   /*Resets the FIFO */
    as399xWaitForResponseTimed(RESP_RXIRQ | RESP_CRCERROR | RESP_PREAMBLEERROR, 20);

    if ((as399xGetResponse() & RESP_RXIRQ))                  /*getting response */
    {
        as399xFifoRead(2, tagResponse);/* Header Bit is set due to error */
        as399xSingleCommand(AS399X_CMD_RESETFIFO);
        if (as399xGetResponse()&RESP_HEADERBIT)
        {
            tagResponse[0] |= 0x80; /* to identify the error */
            ret = (tagResponse[0]);
        }
        else             /*Write ok */
        {
            if ( tagResponse[0] == handle[0] || tagResponse[1] == handle[1])
            {
                ret = GEN2_OK;
            }
        }
    }
    else                                                     /*No Response, Error or TimerInterrupt */
    {                                                        /*Maybe Tag not in the field */
#if EPCDEBUG
        CON_print("No Reply\n");
#endif
    }
    as399xClrResponse();
    return ret;
}



static u8 gen2GetWriteToTagReply(u8 *handle)
{
    u8 ret = GEN2_ERR_NOREPLY;
    u8 tagResponse [3];
    u16 bit_count_tag_error_reply = (1 + 2 + 2) * 8 + 1; /* Error Code + 2 bytes rn16 + 2bytes crc + 1 header bit */
    u8 dataLength;

    as399xWaitForResponse(RESP_TXIRQ);
    as399xClrResponse();
    as399xSingleCommand(AS399X_CMD_RESETFIFO);                   /* Resets the FIFO */
    as399xWaitForResponseTimed(RESP_RXIRQ | RESP_CRCERROR | RESP_HEADERBIT | RESP_PREAMBLEERROR, 20);

    if (as399xGetResponse() & RESP_HEADERBIT) // Tag Error Reply
    {
		as399xSingleWrite(AS399X_REG_RXLENGTHLOW, bit_count_tag_error_reply & 0xff);
		as399xSingleWrite(AS399X_REG_RXLENGTHUP, ( bit_count_tag_error_reply >> 8 ) & 0x03);

        as399xWaitForResponse(RESP_RXIRQ | RESP_CRCERROR | RESP_PREAMBLEERROR);


        dataLength = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x0F;   /*Read response datacount */
        if (dataLength >=3) dataLength = 3;

        as399xFifoRead(dataLength, tagResponse); /* Header Bit is set due to error */
        as399xSingleCommand(AS399X_CMD_RESETFIFO);

		as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);  /*Enable No Response Interrupt */
		return (tagResponse[0] |= 0x80);
    }

    if ((as399xGetResponse() & RESP_RXIRQ))                  /*getting response */
    {
    	dataLength = as399xSingleRead(AS399X_REG_FIFOSTATUS) & 0x0F;   /*Read response datacount */
    	if (dataLength >=2) dataLength = 2;

        as399xFifoRead(dataLength, tagResponse); /* Header Bit is set due to error */
        as399xSingleCommand(AS399X_CMD_RESETFIFO);

        if ( tagResponse[0] == handle[0] || tagResponse[1] == handle[1])
        {
             ret = GEN2_OK;
        }

    }
    else if (as399xGetResponse() & RESP_CRCERROR) // Tag Error Reply
    {
    	ret = GEN2_CRC;
    }
    else                                                     /*No Response, Error or TimerInterrupt */
    {                                                        /*Maybe Tag not in the field */
#if EPCDEBUG
        CON_print("No Reply\n");
#endif
        ret = GEN2_ERR_NOREPLY;
    }
    as399xClrResponse();
    return ret;
}

static u8 gen2GetNxpCCReply(u8 *handle, u8 *config)
{
    u8 ret = GEN2_ERR_NOREPLY;
    u8 tagResponse [2];
    as399xWaitForResponse(RESP_TXIRQ);
    as399xClrResponse();
    as399xSingleCommand(AS399X_CMD_RESETFIFO);                   /*Resets the FIFO */
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);/*Enable No Response Interrupt */
    as399xWaitForResponseTimed(RESP_RXIRQ | RESP_CRCERROR | RESP_PREAMBLEERROR, 20);

    if ((as399xGetResponse() & RESP_RXIRQ))                  /*getting response */
    {
        as399xFifoRead(2, config);
        as399xFifoRead(2, tagResponse);/* Header Bit is set due to error */
        as399xSingleCommand(AS399X_CMD_RESETFIFO);
        if (as399xGetResponse()&RESP_HEADERBIT)
        {
            config[0] |= 0x80; /* to identify the error */
            ret = (config[0]);
        }
        else             /*Write ok */
        {
            if ( tagResponse[0] == handle[0] || tagResponse[1] == handle[1])
            {
                ret = GEN2_OK;
            }
        }
    }
    else                                                     /*No Response, Error or TimerInterrupt */
    {                                                        /*Maybe Tag not in the field */
#if EPCDEBUG
        CON_print("No Reply\n");
#endif
    }
    as399xClrResponse();
    return ret;
}

void gen2Configure(const struct gen2Config *config)
{
    /* depending on link frequency setting adjust */
    /* registers 01, 03, 04, 05, 07, 08, and 09 */
    u8 reg[9];
    u8 session = config->session;
    gen2Config.DR = 1;
    gen2Config.config = *config;
    if (session > GEN2_IINV_S3) session = GEN2_IINV_S0; /* limit SL and invalid settings */
    if (gen2Config.config.miller == GEN2_COD_FM0) gen2Config.config.trext = 1;

    //CON_print("Tari: %hhx, LF: %hhx, Cod: %hhx\n", gen2Config.config.tari, gen2Config.config.linkFreq, gen2Config.config.miller);
    switch (config->linkFreq) {
    case GEN2_LF_640: /* 640kHz */
        reg[0] = 0x00; /* AS399X_REG_PROTOCOLCTRL     */
        reg[1] = 0xC0; /* AS399X_REG_TXOPTGEN2        */
        reg[2] = 0xF2; /* AS399X_REG_RXOPTGEN2        */
        reg[3] = 0x4D; /* AS399X_REG_TRCALREGGEN2     */
        reg[4] = 0x1;  /* AS399X_REG_TRCALGEN2MISC    */
        reg[5] = 0x04; /* AS399X_REG_RXNORESPWAIT     */
        reg[6] = 0x02; /* AS399X_REG_RXWAITTIME       */
#if RUN_ON_AS3992
        reg[7] = 0x07; /* AS399X_REG_RXSPECIAL        */
#else
        reg[7] = 0x03; /* AS399X_REG_RXSPECIAL        */
#endif
        break;
    case GEN2_LF_320: /* 320kHz */
        reg[0] = 0x00;
        reg[1] = 0xC0;
        reg[2] = 0xC2;
        if (gen2Config.config.tari == 0)    /* if Tari = 6.25us */
        {   /* TRcal = 25us */
            gen2Config.DR = 0;
            reg[3] = 0xFA;
            reg[4] = 0x00;
        } else
        {   /* TRcal = 66.6us */
            reg[3] = 0x9A;
            reg[4] = 0x02;
        }
        reg[5] = 0x06;
        reg[6] = 0x04;
#if RUN_ON_AS3992
        reg[7] = 0x27;
#else
        reg[7] = 0x02;
#endif
        break;
    case GEN2_LF_256: /* 256kHz */
        reg[0] = 0x00;
        reg[1] = 0xC0;
        reg[2] = 0x92;
        if (gen2Config.config.tari == 0)    /* if Tari = 6.25us */
        {   /* TRcal = 31.25us */
            gen2Config.DR = 0;
            reg[3] = 0x38;
            reg[4] = 0x01;
        } else
        {   /* TRcal = 83.3us */
            reg[3] = 0x41;
            reg[4] = 0x03;
        }
        reg[5] = 0x05;
        reg[6] = 0x04;
#if RUN_ON_AS3992
        reg[7] = 0x37;
#else
        reg[7] = 0x02;
#endif
        break;
    case GEN2_LF_213: /* 213.3kHz */
        reg[0] = 0x00;
        reg[1] = 0xC0;
        reg[2] = 0x82;
        if (gen2Config.config.tari == 0)    /* if Tari = 6.25us */
        {   /* TRcal = 37.51us */
            gen2Config.DR = 0;
            reg[3] = 0x77;
            reg[4] = 0x1;
        } else
        {   /* TRcal = 100us */
            reg[3] = 0xE8;
            reg[4] = 0x3;
        }
        reg[5] = 0x06;
        reg[6] = 0x04;
#if RUN_ON_AS3992
        reg[7] = 0x3B; /* Not in data sheet, just an estimation but works! */
#else
        reg[7] = 0x01;
#endif
        break;
    case GEN2_LF_160: /* 160kHz */
        reg[0] = 0x00;
        reg[1] = 0xC0;
        reg[2] = 0x62;
        if (gen2Config.config.tari == 1)    /* if Tari = 12.5us */
        {   /* TRcal = 50us */
            gen2Config.DR = 0;
            reg[3] = 0xF4;
            reg[4] = 0x1;
        } else
        {   /* TRcal = 133.3us */
            reg[3] = 0x35;
            reg[4] = 0x5;
        }
        reg[5] = 0x07;
        reg[6] = 0x08;
#if RUN_ON_AS3992
        if(gen2Config.config.miller == GEN2_COD_FM0)
        {
            reg[7] = 0xbf;
        }
        else
        {
            reg[7] = 0x3f;
        }
#else
        reg[7] = 0x01;
#endif
        break;
#if !RUN_ON_AS3992
    case GEN2_LF_80: /* 80kHz */
        reg[0] = 0x00;
        reg[1] = 0xC0;
        reg[2] = 0x32;
        reg[3] = 0xE8;
        reg[4] = 0x3;
        reg[5] = 0x0C;
        reg[6] = 0x07;
        reg[7] = 0x00;
        gen2Config.DR = 0;
        break;
#endif
    case GEN2_LF_40: /* 40kHz */
        reg[0] = 0x00;
        reg[1] = 0xC2;      /* increase TxOne length for 40kHz */
        reg[2] = 0x02;
        reg[3] = 0xD0;      /* only TRcal = 200us allowed */
        reg[4] = 0x7;
        reg[5] = 0x1B;
        reg[6] = 0x0F;
#if RUN_ON_AS3992
        reg[7] = 0xFF;
#else
        reg[7] = 0x00;
#endif
        gen2Config.DR = 0;
        break;
    default:
        return; /* use preset settings */
    }
    reg[0] = (reg[0] & ~0xc) | (gen2Config.config.miller<<2);
    reg[0] = (reg[0] & ~0x3) | (gen2Config.config.tari);
    reg[1] = (reg[1] & ~0x3) | session;
    reg[2] = (reg[2] & ~0x2) | (!!gen2Config.config.trext<<1);

    /* Modify only the gen2 relevant settings */
    as399xContinuousWrite(AS399X_REG_PROTOCOLCTRL, reg+0, 4);
    /* Leave upper nibble of global regs */
    as399xSingleWrite(AS399X_REG_TRCALGEN2MISC, 
                      (as399xSingleRead(AS399X_REG_TRCALGEN2MISC) & 0xf0) | reg[4] );
    as399xContinuousWrite(AS399X_REG_RXNORESPWAIT, reg+5 , 3);
}

void gen2Open(const struct gen2Config * config)
{
    gen2Configure( config );
}

void gen2Close(void)
{
}
