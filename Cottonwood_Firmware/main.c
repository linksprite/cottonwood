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
  * @brief System initialization and main loop.
  *
  * @author Ulrich Herrmann
  */

#include "as399x_config.h"
#include "platform.h"
#include "c8051F340.h"
#include "uart.h"
#include "as399x_public.h"
#include "as399x.h"
#include "as399x_com.h"
#include "gen2.h"
#include "global.h"
#include "iso6b.h"
#include "tuner.h"
#include "timer.h"
#include "usb_commands.h"
#include "F340_FlashPrimitives.h"
#include "F3xx_USB0_Register.h"
#include "F3xx_Blink_Control.h"
#include "F3xx_USB0_InterruptServiceRoutine.h"
#include "F3xx_USB0_Descriptor.h"
#include <math.h>

extern  Freq Frequencies;
extern Tag tags_[MAXTAG];
#ifdef CONFIG_TUNER
extern TuningTable tuningTable;
#endif

static bool continueAlways( ) {return 1; }
static void calcBestSlotSize( void )
{
    u8 round;
    u8 powSlot;
    u8 numTags;

    for ( powSlot = 0; powSlot < 15 ; powSlot ++)
    {
        u8 maxTags=0;
        u16 tagsFound=0;
        for ( round = 0; round < 255 ; round ++)
        {
            numTags = gen2SearchForTags(tags_,16,0,0,powSlot,continueAlways,1);
            tagsFound+= numTags;
            if ( numTags > maxTags) maxTags = numTags;
        }
        CON_print("found with powSlot=%hhx %x tags, max %hhx\n",
                                      powSlot,        tagsFound,maxTags);
    }
}

static const u8 codes[0x10] = 
{
    /* upper 3 bits length, lower 5 bits shorts/longs */
    0xbf, /* 0 ----- */
    0xaf, /* 1 .---  */
    0xa7, /* 2 ..--- */
    0xa3, /* 3 ...-- */
    0xa1, /* 4 ....- */
    0xa0, /* 5 ..... */
    0xb0, /* 6 -.... */
    0xb8, /* 7 --... */
    0xbc, /* 8 ---.. */
    0xbe, /* 9 ----. */
    0x41, /* a .-    */
    0x88, /* b -...  */
    0x8a, /* c -.-.  */
    0x64, /* d -..   */
    0x20, /* e .     */
    0x82, /* f ..-.  */
};

static void morse_nibble( u8 nibble )
{
    u8 i, cod, code_len;

    if (nibble > 0xf) nibble = 0x89; /* -..- = "x" */

    cod = codes[nibble];
    code_len = cod >> 5;

    for ( i = code_len; i>0; i-- )
    {
        LED1(0);
        if ( cod & (1<<(i-1)) ) 
        {
            CON_print("-");
            mdelay(450);
        }
        else
        {
            CON_print(".");
            mdelay(150);
        }
        LED1(1);
        mdelay(150);
    }
    CON_print(" %hhx\n", nibble);
    mdelay(450);
}

/**
 * Some boards (eg: Arnie 2v3) has split power supply setup: SiLabs controller is supplied via USB,
 * the AS399x with an extra power supply. This procedure should be called regularly to check if the power
 * for the AS399x is still available or if it has been restored.
 * @return 0, if AS399x is powered up and init test passed
 */
static u8 splitPowerCheck(void)
{
#if ARNIE
    static u8 startUp = 1;          // will be cleared after the first time this function is called
    static u8 as399xPower = 0;      // as399x is without power supply
    u16 failure = 0;
    u8 ret = 0;

    if (as399xPower && EXT_SUPPLY)  //quick check if everything is ok
        return ret;

    if (startUp)
    {
        if (EXT_SUPPLY)             // we are powering up and AS399x is supplied, proceed with init
        {
            mdelay(1);
            failure = as399xInitialize(866900);
            if (!failure)
                as399xPower = 1;
        }
    }
    else
    {
        if(!as399xPower)            // no power on AS399x, if it returned we reset, otherwise we report error
        {
            ret = 0x80;             // return error, except power is back than we reset
            if (EXT_SUPPLY)         // AS399x got power again
            {
                mdelay(50);
                RSTSRC = 0x10;      // reset the system
            }
        }
        else if (!EXT_SUPPLY)        // AS399x was powered but is not supplied anymore
        {
            EN(LOW);
            VDD_IO = 0;
            as399xPower = 0;
            ret = 0x80;
        }
    }

    // if error, display it
    if (failure)
    {
        ret = failure;
        mdelay(1000);
        morse_nibble(failure);
        mdelay(1000);

#if ! UARTSUPPORT
        CON_print("as399xInitialize() returned %x\n", failure);
#endif
    }
    if (startUp)
        CON_print("as399xInitialize() returned %x\n", failure);

    startUp = 0;
    return ret;
#else
    return 0;
#endif
}

/** main function
  * Initializes uController (System_Init()) and AS399x (as399xInitialize()). Afterwards
  * it enters main loop in which the commands received via USB or UART are processed.
  */
int main(void)
{

    u8 count = 0;
    u16 failure;
    unsigned  short  counter = 200;
    u8 a;
    PCA0MD &= ~0x40;                         /* WDTE = 0 (clear watchdog timer */

    /* System_Init already sets clocks, do this before initing uart */
    System_Init ();

    EA = 1; /* enable all interrupts */

    initUART();

    CON_print("Hello World\n");

    /* Fill frequencies interval used in usb_commands.c with values */
    /* European channels since we are developing in Europe, values can be changed using GUI */
    Frequencies.freq[0]= 866900;Frequencies.rssiThreshold[0]=-40;
    Frequencies.freq[1]= 865700;Frequencies.rssiThreshold[1]=-40;
    Frequencies.freq[2]= 866300;Frequencies.rssiThreshold[2]=-40;
    Frequencies.freq[3]= 867500;Frequencies.rssiThreshold[3]=-40;
    Frequencies.activefreq=4;
#ifdef CONFIG_TUNER
    tuningTable.tableSize = 0;
#endif
    initInterface();    /* set up communication interface with AS399x */
    mdelay(1);          /* AS3992 needs this, test showed the border at about 3us */
#if ARNIE
    failure = splitPowerCheck();
#else
    failure = as399xInitialize(Frequencies.freq[0]);
    if (failure)
    {
        mdelay(1000);
        morse_nibble(failure);
        mdelay(1000);
    }

    CON_print("as399xInitialize() returned %x\n",failure);
#endif
#if ISO6B
    iso6bInitialize();
#endif

#ifdef CONFIG_TUNER
    tunerInit();
#endif

    /*calcBestSlotSize();*/
#ifdef ENTRY_POINT_ADDR
    /* systems seems to be up and running. set the magic byte to tell the bootloader
       to boot the image also next time */
    if (ENTRY_POINT_ADDR > 0)
    {
        FLASH_ByteWrite(ENTRY_POINT_ADDR-1, 0xaa);
    }
#endif

    initCommands(); /* USB report commands */

#if ! UARTSUPPORT
    Usb_Init ();
#endif

    while (1)
    {
        if (counter)
        {
            counter--;
        }
        else
        {
#if ARNIE
        failure = splitPowerCheck();
#endif
            counter = (24000<<(failure?4:0));
            if (a) {
                a=0;
                LED1(0);
            }
            else {
                a=1;
                LED1(1);
            }
        }
#ifdef POWER_DETECTOR
        as399xCyclicPowerRegulation();  //check PA regulation
#endif
#if UARTSUPPORT
        uartCommands();
#else
        commands(); /* main trigger for operation commands. */
#endif
    }
}
