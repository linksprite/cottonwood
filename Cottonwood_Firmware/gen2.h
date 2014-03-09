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
  * @brief
  * This file provides declarations for functions for the GEN2 aka ISO6c protocol.
  *
  * @author Ulrich Herrmann
  *
  * Before calling any of the functions herein the AS399X chip needs to
  * be initialized using as399xInitialize(). Thereafter the function 
  * gen2Open needs to be called for opening a session. 
  * gen2SearchForTags() should be called to identify the tags in
  * reach. Usually the the user then wants to select one of the found
  * tags. For doing this gen2SearchForTags() should be again called
  * providing a proper mask to select (isolate) one of the tags. This time
  * only one tag should be returned by gen2SearchForTags(). This one found
  * tag is then in the Open/Secured state
  * can then be accessed using the other functions ,
  * gen2WriteWordToTag(), gen2ReadFromTag()...
  * When finished with gen2 operations this session should be closed using 
  * gen2Close(). 
  *
  * The tag state diagram looks as follows using the
  * provided functions. For exact details please refer to
  * the standard document provided by EPCglobal under 
  * <a href="http://www.epcglobalinc.org/standards/uhfc1g2/uhfc1g2_1_2_0-standard-20080511.pdf">uhfc1g2_1_2_0-standard-20080511.pdf</a>
  *
  * If the tag in question does have a password set:
  * \dot
  * digraph gen2_statesp{
  * Ready -> Open [ label="gen2SearchForTags()" ];
  * Ready->Ready;
  * Open -> Secured [ label="gen2AccessTag() + correct PW" ];
  * Open -> Open [ label="gen2ReadFromTag()\n..." ];
  * Secured -> Secured [ label="gen2WriteWordToTag()\ngen2ReadFromTag()\n..." ];
  * }
  * \enddot
  *
  * If the tag in question does <b>not</b> have a password set:
  * \dot
  * digraph gen2_statesn{
  * Ready -> Secured [ label="gen2SearchForTags()" ];
  * Secured -> Secured [ label="gen2WriteWordToTag()\ngen2ReadFromTag()\n..." ];
  * }
  * \enddot
  *
  * So a typical use case may look like this:
  * \code
  * Tag tags[16];
  * struct gen2Config config = {GEN2_LF_160, GEN2_COD_MILLER2, GEN2_IINV_S0, 1};
  * unsigned n;
  * u8 buf[4];
  * ...
  * as399xInitialize(912000);
  *
  * gen2Open(&config);
  *
  * n = gen2SearchForTags(tags,16,0,0,4);
  * if ( n == 0) return;
  *
  * //Pick one of the tags based on the contents of tags. Here we use the very first tag
  *
  * if (gen2ReadFromTag(tags+0, MEM_USER, 0, 2, buf))
  *     return;
  *
  * buf[0] ^= 0xff; buf[1]^= 0x55; // change the data
  *
  * if (gen2WriteWordToTag( tags+0, MEM_USER, 0,  buf))
  * { // wrote back one of the two read words
  *     gen2Close();
  *     return;
  * }
  *
  * //...
  *
  * gen2Close();
  *
  * \endcode
  *
  */

#ifndef __GEN2_H__
#define __GEN2_H__

#include "as399x_public.h"

/* Protocol configuration settings */
#define GEN2_LF_40  0   /*!<link frequency 40 kHz*/
#if !RUN_ON_AS3992
#define GEN2_LF_80  3   /*!<link frequency 80 kHz*/
#endif
#define GEN2_LF_160 6   /*!<link frequency 160 kHz*/
#define GEN2_LF_213 8   /*!<link frequency 213 kHz*/
#define GEN2_LF_256 9   /*!<link frequency 213 kHz*/
#define GEN2_LF_320 12  /*!<link frequency 213 kHz*/
#define GEN2_LF_640 15  /*!<link frequency 640 kHz*/

/** Rx coding values */
#define GEN2_COD_FM0      0 /*!<FM coding for rx */
#define GEN2_COD_MILLER2  1 /*!<MILLER2 coding for rx*/
#define GEN2_COD_MILLER4  2 /*!<MILLER4 coding for rx*/
#define GEN2_COD_MILLER8  3 /*!<MILLER8 coding for rx*/

/*EPC Mem Banks */
/** Definition for EPC tag memory bank: reserved */
#define MEM_RES           0x00
/** Definition for EPC tag memory bank: EPC memory */
#define MEM_EPC           0x01
/** Definition for EPC tag memory bank: TID */
#define MEM_TID           0x02
/** Definition for EPC tag memory bank: user */
#define MEM_USER          0x03

/*EPC Wordpointer Addresses */
/** Definition for EPC wordpointer: Address for CRC value */
#define MEMADR_CRC        0x00
/** Definition for EPC wordpointer: Address for PC value Word position*/
#define MEMADR_PC         0x01
/** Definition for EPC wordpointer: Address for EPC value */
#define MEMADR_EPC        0x02

/** Definition for EPC wordpointer: Address for kill password value */
#define MEMADR_KILLPWD    0x00
/** Definition for EPC wordpointer: Address for access password value */
#define MEMADR_ACCESSPWD  0x02

/** Definition for EPC wordpointer: Address for TID value */
#define MEMADR_TID        0x00

/*EPC SELECT TARGET */
/** Definition for inventory: Inventoried (S0) */
#define GEN2_IINV_S0           0x00 /*Inventoried (S0) */
/** Definition for inventory: 001: Inventoried (S1) */
#define GEN2_IINV_S1           0x01 /*001: Inventoried (S1) */
/** Definition for inventory: 010: Inventoried (S2) */
#define GEN2_IINV_S2           0x02 /*010: Inventoried (S2) */
/** Definition for inventory: 011: Inventoried (S3) */
#define GEN2_IINV_S3           0x03 /*011: Inventoried (S3) */
/** Definition for inventory: 100: SL */
#define GEN2_IINV_SL           0x04 /*100: SL */

/* Challenge command flags bits definition */
#define GEN2_CHAL_CMD_IMMED		(1 << 0) /* transmit result with EPC */
#define GEN2_CHAL_CMD_IRL		(1 << 1) /* include length in reply */
#define GEN2_CHAL_CMD_BUF		(1 << 2) /* buffer to be used for storage, hard-coded 0 */
#define GEN2_CHAL_CMD_RFU		(3 << 3) /* reserved for further use */

/** Defines for various return codes */
#define GEN2_OK                  0  /**< No error */
#define GEN2_ERR_REQRN           1  /**< Error sending ReqRN */
#define GEN2_ERR_ACCESS          2  /**< Error sending Access password */
#define GEN2_ERR_KILL            3  /**< Error sending Kill */
#define GEN2_ERR_NOREPLY         4  /**< Error no reply received */
#define GEN2_ERR_LOCK            5  /**< Error locking command */
#define GEN2_ERR_BLOCKWRITE      6  /**< Error Blockwrite */
#define GEN2_ERR_BLOCKERASE      7  /**< Erorr Blockerase */
#define GEN2_ERR_READ            8  /**< Error Reading */
#define GEN2_ERR_SELECT          9  /**< Error when selecting tag*/
#define GEN2_ERR_CHANNEL_TIMEOUT 10 /**< Error RF channel timed out*/
#define GEN2_CRC                 11 /**< Error CRC */


struct gen2Config{
    u8 linkFreq; /* GEN2_LF_40, ... */
    u8 miller; /* GEN2_COD_FM0, ... */
    u8 session; /* GEN2_IINV_S0, ... */
    u8 trext; /* 1 if the preamble is long, i.e. with pilot tone */
    u8 tari;    /* Tari setting */
};

struct gen2GenericCmdData{
	u16 txBitCount;			/* INPUT: number of bits to be TX'ed, excluding RN16 and CRC16 */
	u16 rxBitCount;			/* INPUT: number of bits to be RX'ed, excluding RN16 and CRC16 */
	u8 * txData;			/* INPUT: TX-data buffer */
	u8 * rxData;			/* OUTPUT: RX-data buffer */
	u8 directCommand;		/* INPUT: direct-command to be used */
	u8 rxNoRespTimeout;		/* INPUT: RX-no-response Timeout */
	u8 actRxByteCount;	/* OUTPUT: actual number of RX'ed bytes */
};

/*------------------------------------------------------------------------- */
/** This function sends the Challenge Command D4h to all tags in range.
  * The challenge is used to trigger all tags within range to pre-calculate the
  * the crypto-authentication for later request when tags are selected.
  *
  * @param commandFlags bit-coded command info (RFU, Buffer, IncRepLen, immed).
  * @param cryptoSuiteId Crypto Suite ID to define the used crypto algorithm.
  * @param msgBitLength bit length of the message (can be EBV-encoded for proprietary implementations).
  * @param *message pointer to message bit stream.
  * @param *output pointer to response buffer.
  * @param *outputLength pointer to variable which stores size of valid data in response buffer in bytes.
  */
u8 gen2ChallengeCommand(u8 commandFlags
					    , cryptoSuiteId
					    , u16 msgBitLength
					    , u8 * message
					    , u8 * output
					    , u8 * outputLength
					    );

/*------------------------------------------------------------------------- */
/** Search for tags (aka do an inventory round). Before calling any other
  * gen2 functions this routine has to be called. It first selects using the 
  * given mask a set of tags and then does an inventory round issuing query 
  * commands. All tags are stored in then tags array for examination by the 
  * user. The last found tag will be kept in the Open/Secured state and can 
  * subsequently treated using the other gen2 functions.
  * For accessing a specific tag, searchForTags should be called without a 
  * mask. Then a one of the read out epcs should be used as the mask for 
  * another inventory round thus isolating this single tag.
  *
  * @param *tags an array for the found tags to be stored to
  * @param maxtags the size of the tags array
  * @param *mask mask for selection of specific tags
  * @param length of the mask
  * @param q 2^q slots will be done first, additional 2 round with increased 
  * or decreased q may be performed
  * thus keeping it in open/secured state
  * @param cbContinueScanning callback is called after each slot to inquire if we should
  * @param useMaskToSelect if set to true the mask will be used for a SELECT 
  * command. If false then all tags will be selected, operation stops at 
  * first found tag which matches the mask.
  * continue scanning (e.g. for allowing a timeout)
  * @return the number of tags found
  */
unsigned gen2SearchForTags(Tag *tags
                          , u8 maxtags
                          , u8* mask
                          , u8 length
                          , u8 q
                          , bool (*cbContinueScanning)(void)
                          , bool useMaskToSelect
                          );


/** For reference see gen2SearchForTags(). The main difference is that it
  * does not put any tags into open/secured state. Thus this function is a
  * bit faster.
  */
unsigned gen2SearchForTagsFast(Tag *tags_
                          , u8 maxtags
                          , u8* mask
                          , u8 length
                          , u8 q
                          , bool (*cbContinueScanning)(void)
                          , u8 startCycle
                          );
/*------------------------------------------------------------------------- */
/** EPC ACCESS command send to the Tag.
  * This function is used to bring a tag with set access password from the Open
  * state to the Secured state.
  *
  * @attention This command works on the one tag which is currently in the open 
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the Tag structure.
  * @param *password Pointer to first byte of the access password
  * @return The function returns an errorcode.
                  0x00 means no Error occoured.
                  Any other value is the backscattered error code from the tag.
  */
u8 gen2AccessTag(Tag *tag, u8 *password);

/*------------------------------------------------------------------------- */
/** EPC LOCK command send to the Tag.
  * This function is used to lock some data region in the tag.
  *
  * @attention This command works on the one tag which is currently in the open 
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the Tag structure.
  * @param *mask_action Pointer to the first byte of the mask and
                                    action array.
  * @return The function returns an errorcode.
                  0x00 means no Error occoured.
                  Any other value is the backscattered error code from the tag.
  */
u8 gen2LockTag(Tag *tag, u8 *mask_action);

/*------------------------------------------------------------------------- */
/** EPC KILL command send to the Tag.
  * This function is used to permanently kill a tag. After that the
  * tag will never ever respond again.
  *
  * @attention This command works on the one tag which is currently in the open 
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the Tag structure.
  * @param *password Pointer to first byte of the kill password
  * @param rfu Some special configuration bits.
  * @return The function returns an errorcode.
                  0x00 means no Error occoured.
                  Any other value is the backscattered error code from the tag.
  */
u8 gen2KillTag(Tag *tag, u8 *password, u8 rfu);

/*------------------------------------------------------------------------- */
/** EPC WRITE command send to the Tag.
  * This function writes one word (16 bit) to the tag.
  * It first requests a new handle. The handle is then exored with the data.
  *
  * @attention This command works on the one tag which is currently in the open 
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the Tag structure.
  * @param memBank Memory Bank to which the data should be written.
  * @param wordPtr Word Pointer Address to which the data should be written.
  * @param *databuf Pointer to the first byte of the data array. The data buffer
                             has to be 2 bytes long.
  * @return The function returns an errorcode.
                  0x00 means no error occoured.
                  0xFF unknown error occoured.
                  Any other value is the backscattered error code from the tag.
  */
u8 gen2WriteWordToTag(Tag *tag, u8 memBank, u8 wordPtr, u8 *databuf);

/*------------------------------------------------------------------------- */
/** NXP Custom command ChangeConfig sent to the tag.
  * @attention Before issuing tag needs to be in secured state using non-zero 
  *            password.
  * 
  * @param *tag Pointer to the Tag structure.
  * @param *databuf Pointer to the first byte of the config word array.
  *                 The config word as in the tags reply is returned within.
  *
  * @return The function returns an errorcode.
                  0x00 means no error occoured.
                  0xFF unknown error occoured.
                  Any other value is the backscattered error code from the tag.
  *
  */
u8 gen2NXPChangeConfig(Tag *tag, u8 *databuf);
/*------------------------------------------------------------------------- */
/** EPC READ command send to the Tag.
  *
  * @attention This command works on the one tag which is currently in the open 
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the Tag structure.
  * @param memBank Memory Bank to which the data should be written.
  * @param wordPtr Word Pointer Address to which the data should be written.
  * @param wordCount Number of bytes to read from the tag.
  * @param *destbuf Pointer to the first byte of the data array.
  * @return The function returns an errorcode.
                  0x00 means no error occoured.
                  0xFF unknown error occoured.
                  Any other value is the backscattered error code from the tag.
  */
u8 gen2ReadFromTag(Tag *tag, u8 memBank, u8 wordPtr,
                          u8 wordCount, u8 *destbuf);

/*------------------------------------------------------------------------- */
/** GENERIC COMMAND send to the Tag.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the Tag structure.
  * @param *commandData structure which contains all relevant data for generic command.
  * @return The function returns an error-code.
                  0x00 means no error occurred.
                  0xFF unknown error occurred.
                  Any other value is the backscattered error code from the tag.
  */
u8 gen2GenericCommand(Tag *tag, struct gen2GenericCmdData *commandData);

/*------------------------------------------------------------------------- */
/** This function executes the NXP special command Set protection Bit
  *
  * @attention This command works on the one tag which is currently in the open 
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the Tag structure.
  */
u8 gen2SetProtectBit(Tag *tag);

/*------------------------------------------------------------------------- */
/** This function executes the NXP special command ReSet protection Bit
  *
  * @attention This command works on the one tag which is currently in the open 
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the Tag structure.
  * @param *password pointer to the 4 byte passwords to be used
  */
u8 gen2ReSetProtectBit(Tag *tag, u8 *password);

/*------------------------------------------------------------------------- */
/** This function executes the NXP special command Change EAS
  *
  * @attention This command works on the one tag which is currently in the open 
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the Tag structure.
  * @param value to be set
  */
u8 gen2ChangeEAS(Tag *tag,u8 value);

/*------------------------------------------------------------------------- */
/** This function execute sthe NXP special command Calibrate
  *
  * @attention This command works on the one tag which is currently in the open 
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the Tag structure.
  */
u8 gen2Calibrate(Tag *tag);

/*------------------------------------------------------------------------------ */
/*Sends the tags EPC via the UART to the host */
/*The function needs the tags structure */
void gen2PrintEPC(Tag *tag);

/*------------------------------------------------------------------------- */
/** Prints the tag information out (UART).
  * @param *tag Pointer to the Tag structure.
  * @param epclen Length of the EPC.
  * @param tagNr Number of the tag.
  */
void gen2PrintTagInfo(Tag *tag, u8 epclen, u8 tagNr);

/*!
 *****************************************************************************
 *  \brief  Set the link frequency
 *
 *  Set the link frequency and gen2 specific parameters. After calling
 *  this function the AS3991 is in normal.
 *****************************************************************************
 */
void gen2Configure(const struct gen2Config *config);

/*!
 *****************************************************************************
 *  \brief  Open a session
 *
 * @param config: configuration to use 
 *
 *  Open a session for gen2 protocol
 *****************************************************************************
 */
void gen2Open(const struct gen2Config * config);

/*!
 *****************************************************************************
 *  \brief  Close a session
 *
 *  Close the session for gen2 protocol
 *****************************************************************************
 */
void gen2Close(void);


#endif
