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
  * @brief This file includes some useful functions.
  *
  * @author Ulrich Herrmann
  */

#include "c8051F340.h"
#include "as399x_config.h"
#include "global.h"
#include <string.h>

/*------------------------------------------------------------------------- */
/** This function is used to convert a 8bit value to a Hexstring. \n
  * @param value Databyte which should be converted
  * @param *destbuf Destination buffer. The Buffer should be at least 5 bytes long.
                 The first byte is always a 0 and the second a x.
  */
void bin2Hex(char value, u8 *destbuf)
{
    u8 temp_v;
    destbuf[0] = '0';
    destbuf[1] = 'x';

    temp_v = (value >> 4) & 0x0F;
    if (temp_v  <= 9)
    {
        destbuf[2] = temp_v + '0';
    }
    else
    {
        temp_v -= 10;
        destbuf[2] = temp_v + 'A';
    }

    temp_v = value & 0x0F;
    if (temp_v  <= 9)
    {
        destbuf[3] = temp_v + '0';
    }
    else
    {
        temp_v -= 10;
        destbuf[3] = temp_v + 'A';
    }
    destbuf[4] = 0x00;

}

/*------------------------------------------------------------------------- */
/** This function is used to convert a 8bit value to a Char string. \n
  * @param value Databyte which should be converted
  * @param *destbuf Destination buffer. The Buffer should be at least 6 bytes long.
  */
void bin2Chars(int value, u8 *destbuf)
{
    u8 tenthousand=0;
    u8 thousand=0;
    u8 hundred=0;
    u8 ten=0;
    u8 one=0;

    while (value-10000 >= 0)
    {
        value -= 10000;
        tenthousand++;
    }
    while (value-1000 >= 0)
    {
        value -= 1000;
        thousand++;
    }
    while (value-100 >= 0)
    {
        value -= 100;
        hundred++;
    }
    while (value-10 >= 0)
    {
        value -= 10;
        ten++;
    }
    while (value-1 >= 0)
    {
        value--;
        one++;
    }

    if (tenthousand)
    {
        destbuf[0]=tenthousand + '0';
        destbuf[1]=thousand + '0';
        destbuf[2]=hundred + '0';
        destbuf[3]=ten + '0';
        destbuf[4]=one + '0';
        destbuf[5]=0x00;
    }
    else if (thousand)
    {
        destbuf[0]=thousand + '0';
        destbuf[1]=hundred + '0';
        destbuf[2]=ten + '0';
        destbuf[3]=one + '0';
        destbuf[4]=0x00;
    }
    else if (hundred)
    {
        destbuf[0]=hundred + '0';
        destbuf[1]=ten + '0';
        destbuf[2]=one + '0';
        destbuf[3]=0x00;
    }
    else if (ten)
    {
        destbuf[0]=ten + '0';
        destbuf[1]=one + '0';
        destbuf[2]=0x00;
    }
    else if (one)
    {
        destbuf[0]=one + '0';
        destbuf[1]=0x00;
    }
    else
    {
        destbuf[0]='0';
        destbuf[1]=0x00;
    }
}

void copyBuffer(u8 *source, u8 *dest, u8 len)
{
    u8 count = 0;
    for (count = 0; count < len; count++)
    {
        dest[count] = source[count];
    }
}

u8 stringLength(char *source)
{
    u8 count = 0;
    while (source[count] != 0x00)
    {
        count++;
    }
    return(count);
}
#ifndef CHAR_BIT
	#define CHAR_BIT    8
#endif
#define PREPARE_FIRST_COPY()                                      \
    do {                                                          \
    if (src_len >= (CHAR_BIT - dst_offset_modulo)) {              \
        *dst     &= reverse_mask[dst_offset_modulo];              \
        src_len -= CHAR_BIT - dst_offset_modulo;                  \
    } else {                                                      \
        *dst     &= reverse_mask[dst_offset_modulo]               \
              | reverse_mask_xor[dst_offset_modulo + src_len + 1];\
         c       &= reverse_mask[dst_offset_modulo + src_len    ];\
        src_len = 0;                                              \
    } } while (0)

void
bitArrayCopy(const u8 *src_org, s16 src_offset, s16 src_len,
                    u8 *dst_org, s16 dst_offset)
{
//    static const u8 mask[] =
//        { 0x55, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
//    static const u8 mask_xor[] =
//        { 0x55, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00 };
    static const u8 reverse_mask[] =
        { 0x55, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
    static const u8 reverse_mask_xor[] =
        { 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00 };

    if (src_len) {
        const u8 *src;
              u8 *dst;
        s16                  src_offset_modulo,
                             dst_offset_modulo;

        src = src_org + (src_offset / CHAR_BIT);
        dst = dst_org + (dst_offset / CHAR_BIT);

        src_offset_modulo = src_offset % CHAR_BIT;
        dst_offset_modulo = dst_offset % CHAR_BIT;

        if (src_offset_modulo == dst_offset_modulo) {
            s16              byte_len;
            s16              src_len_modulo;
            if (src_offset_modulo) {
                u8   c;

                c = reverse_mask_xor[dst_offset_modulo]     & *src++;

                PREPARE_FIRST_COPY();
                *dst++ |= c;
            }

            byte_len = src_len / CHAR_BIT;
            src_len_modulo = src_len % CHAR_BIT;

            if (byte_len) {
                memcpy(dst, src, byte_len);
                src += byte_len;
                dst += byte_len;
            }
            if (src_len_modulo) {
                *dst     &= reverse_mask_xor[src_len_modulo];
                *dst |= reverse_mask[src_len_modulo]     & *src;
            }
        } else {
            s16             bit_diff_ls,
                            bit_diff_rs;
            s16             byte_len;
            s16             src_len_modulo;
            u8   c;
            /*
             * Begin: Line things up on destination.
             */
            if (src_offset_modulo > dst_offset_modulo) {
                bit_diff_ls = src_offset_modulo - dst_offset_modulo;
                bit_diff_rs = CHAR_BIT - bit_diff_ls;

                c = *src++ << bit_diff_ls;
                c |= *src >> bit_diff_rs;
                c     &= reverse_mask_xor[dst_offset_modulo];
            } else {
                bit_diff_rs = dst_offset_modulo - src_offset_modulo;
                bit_diff_ls = CHAR_BIT - bit_diff_rs;

                c = *src >> bit_diff_rs     &
                    reverse_mask_xor[dst_offset_modulo];
            }
            PREPARE_FIRST_COPY();
            *dst++ |= c;

            /*
             * Middle: copy with only shifting the source.
             */
            byte_len = src_len / CHAR_BIT;

            while (--byte_len >= 0) {
                c = *src++ << bit_diff_ls;
                c |= *src >> bit_diff_rs;
                *dst++ = c;
            }

            /*
             * End: copy the remaining bits;
             */
            src_len_modulo = src_len % CHAR_BIT;
            if (src_len_modulo) {
                c = *src++ << bit_diff_ls;
                c |= *src >> bit_diff_rs;
                c     &= reverse_mask[src_len_modulo];

                *dst     &= reverse_mask_xor[src_len_modulo];
                *dst |= c;
            }
        }
    }
}

