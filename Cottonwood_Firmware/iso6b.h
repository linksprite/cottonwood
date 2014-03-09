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
 *  \brief ISO6B protocol header file
 *
 * Before calling any of the functions herein the AS399X chip needs to
 * be initialized using as399xInitialize(). Thereafter the function 
 * iso6bOpen() needs to be called for opening a session. 
 * 
 * The following graph shows several states of an ISO 6B tag as well as their 
 * transitions based on iso6b* commands.
 *
 * \dot
 * digraph iso6b_states{
 * Power_off -> Ready [ label="as399xAntennaPower(1)" ];
 * Ready -> Data_Exchange [ label="iso6bRead()" ];
 * Ready -> Data_Exchange [ label="iso6bWrite()" ];
 * Ready -> ID [ label="iso6bInventoryRound(),\nafter select command has been emitted" ];
 * ID -> Data_Exchange [ label="iso6bInventoryRound(),\nafter collision arbitration" ];
 * Data_Exchange -> Power_off [ label="as399xAntennaPower(0)" ];
 * }
 * \enddot
 *
 * It can be seen that iso6bRead() and iso6bWrite() can be called without a prior
 * inventory round. Both commands, however, do need the UID of the tag which can
 * only be determined by calling iso6bInventoryRound().
 */

#if ISO6B

#ifndef ISO6B_H
#define ISO6B_H

#include "as399x_public.h"

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
#define ISO6B_ERR_NONE	0  /*!< no error occured during, tag has been found */
#define ISO6B_ERR_CRC	-1  /*!< CRC error occured, two or more tags responded at the same time */
#define ISO6B_ERR_AS399X_REG_FIFO	-2  /*!< FIFO error occured in the AS3991 */
#define ISO6B_ERR_NOTAG	-3  /*!< No response from any tag */
#define ISO6B_ERR_NACK -4  /*!< tag sent not acknowledge */
#define ISO6B_ERR_PREAMBLE	-5  /*!< Preamble error occurred */

/**
 * This function performs an iso6b inventory round and stores up to \a maxtags 
 * tags into the variable \a tags.
 * It performs a select function which compares up to eight consecutive bytes within
 * the tags to a given buffer. If the tag's memory content which starts at address 
 * \a startaddress matches the buffer given by \a filter the tag backscatters his
 * UID. The bitmask \a mask is used to indicate which bytes of the buffer shall
 * be compared with the tag's memory content. If \a mask is zero all tags are
 * selected.
 *  \note Direct mode needs to be enabled (#iso6bOpen()) 
 *        before calling this function.
 *
 * \param[out] tags : an array of Tag structs which will be filled by this function
 * \param[in] maxtags : the size of the array
 * \param[in] mask : mask identifying which bytes have to be compared
 * \param[in] filter : 8 byte long compare buffer
 * \param[in] startaddress : address of the first register to compare within the tag
 *
 * \returns the number of tags found.
 */
extern s8 iso6bInventoryRound (Tag* tags, u8 maxtags, u8 mask, u8* filter, u8 startaddress);

/*!
 *****************************************************************************
 *  \brief  Issue READ_VARIABLE command according to ISO18000-6 and stores
 *          the result in a buffer
 *
 *  This function sends the READ_VARIABLE command via the AS3991 to the tag with a given
 *  uid. The data is read from start address \a startaddr until \a len bytes
 *  have been read. 
 *  The answer (\a len bytes long) is then written to the memory location 
 *  pointed by \a buffer.
 *  \note Direct mode needs to be enabled (#iso6bOpen()) 
 *  before calling this function.
 *
 *  \param[in]  uid : 8 byte long uid of the tag
 *  \param[in]  startaddr : start address within the AS3991
 *  \param[out] buffer : pointer to a memory location (\a len bytes long) where
 *                    the result shall be stored
 *  \param[in]  len : number of bytes to read out
 *
 *  \return #ISO6B_ERR_AS399X_REG_FIFO : Error reading the FIFO
 *  \return #ISO6B_ERR_NOTAG : No response from tag
 *  \return #ISO6B_ERR_CRC : CRC error while receiving data from tag
 *  \return #ISO6B_ERR_NONE : No error
 *
 *****************************************************************************
 */
extern s8 iso6bRead(u8* uid, u8 startaddr, u8* buffer, u8 len);

/*!
 *****************************************************************************
 *  \brief  Issue WRITE command according to ISO18000-6 to write a specified
 *          number of bytes to a memory location within a tag
 *
 *  This function sends the WRITE command via the AS3991 to the 
 *  tag with a given \a uid. This command writes \a len bytes, stored within 
 *  \a buffer, to memory location with start address \a startaddr within a tag.
 *  After every written byte the address gets incremented by one.
 *  \note Direct mode needs to be enabled (#iso6bOpen()) 
 *  before calling this function.
 *
 *  \param[in]  uid : 8 byte long uid of the tag
 *  \param[in]  startaddr : address of the first register to write
 *  \param[in]  buffer : buffer to write starting at register \a addr
 *  \param[in]  len : number of bytes to write
 *
 *  \return #ISO6B_ERR_AS399X_REG_FIFO : Error reading the FIFO
 *  \return #ISO6B_ERR_NOTAG : No response from tag
 *  \return #ISO6B_ERR_CRC : CRC error while receiving data from tag
 *  \return #ISO6B_ERR_NACK : Tag sent error response
 *  \return #ISO6B_ERR_NONE : No error
 *
 *****************************************************************************
 */
extern s8 iso6bWrite(u8* uid, u8 startaddr, u8* buffer, u8 len);

/*!
 *****************************************************************************
 *  \brief  Leave the iso6b mode
 *
 *  Since gen2 module access is not possible after iso6b module initialization
 *  the iso6b module needs to be closed when execution of iso6b commands
 *  are done.
 *
 *  \return #ISO6B_ERR_NONE : No error
 *
 *****************************************************************************
 */
extern s8 iso6bClose(void);

/*!
 *****************************************************************************
 *  \brief  Enters the iso6b mode
 *
 *  This function is used to initialize the iso6b module.
 *  Since iso6b implementation is based on the "direct mode" (i.e. data
 *  sent are directly modulated) and the bitbang module
 *  an initialization of the 6b module is mandatory for proper usage. 
 *  Consequently, after calling this function gen2 module might not work
 *  correctly as the chip is in direct mode and therefore the module needs
 *  to be deinitialized (using #iso6bClose) as soon as 6b access is 
 *  not needed any more. 
 *
 *  \return #ISO6B_ERR_NONE : No error
 *
 *****************************************************************************
 */
extern s8 iso6bOpen(void);

/*!
 *****************************************************************************
 *  \brief  Initializes the iso6b module
 *
 *  \return #ISO6B_ERR_NONE : No error
 *
 *****************************************************************************
 */
extern s8 iso6bInitialize(void);

#endif /* ISO6B_H */

#endif
