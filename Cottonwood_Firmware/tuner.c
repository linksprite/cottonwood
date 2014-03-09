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
  * @brief Implementation of tuner functionality.
  *
  * This file includes the whole functionality to adjust the PI tuner network 
  * on tuner enabled boards.
  * The communication pins for the tuner network are defined in platform.h.
  * The tuner network consists of 3 variable capacitors: Cin, Clen and Cout
  * and looks like this:
  \code
                         .--- L ---.
                         |         |
  in ----.----.----- L --.--C_len--.--.-----.-----.----- out
         |    |                       |     |     |
        C_in  L                     C_out   L     R
         |    |                       |     |     |
        ___  ___                     ___   ___   ___
         -    -                       -     -     -
         '    '                       '     '     '
  \endcode
  *
  * @author Ulrich Herrmann
  * @author Bernhard Breinbauer
  */

#include "c8051F340.h"
#include "as399x_config.h"
#include "global.h"
#include "uart.h"
#include "tuner.h"
#include "as399x_public.h"
#include "platform.h"
#include <math.h>

#ifdef CONFIG_TUNER
/*------------------------------------------------------------------------- */
#if 0
void tunerDebug();
#endif

/*------------------------------------------------------------------------- */
void tunerInit()
{
    /* Assuming everything was already initialised by initInterface of serial_inteface.c */
    //tunerDebug();

    tunerSetCap(TUNER_Cin,  15);
    tunerSetCap(TUNER_Clen, 15);
    tunerSetCap(TUNER_Cout, 15);

    return;
}

#if 0
/*------------------------------------------------------------------------- */
void tunerDebug()
{
    s16 val;
    s16 noiseval;
    s16 ch_val_i;
    s16 ch_val_q;
    s16 rf;
    u8 i;
    for ( i = 0; i < 32; i++)
    {
        tunerSetCap(TUNER_Cout,i);
        noiseval = as399xGetReflectedPowerNoiseLevel();
        val = as399xGetReflectedPower( );
        ch_val_i = (s8)((val&0xff) - (noiseval&0xff));
        ch_val_q = (s8)(((val>>8)&0xff) - ((noiseval>>8)&0xff));
        rf = sqrt(ch_val_i * ch_val_i + ch_val_q * ch_val_q);
        CON_print("i=%hhd rf=%hx\n",i,rf);
    }
    for ( i = 0; i < 32; i++)
    {
        tunerSetCap(TUNER_Cin,i);
        noiseval = as399xGetReflectedPowerNoiseLevel();
        val = as399xGetReflectedPower( );
        ch_val_i = (s8)((val&0xff) - (noiseval&0xff));
        ch_val_q = (s8)(((val>>8)&0xff) - ((noiseval>>8)&0xff));
        rf = sqrt(ch_val_i * ch_val_i + ch_val_q * ch_val_q);
        CON_print("i=%hhd rf=%hx\n",i,rf);
    }
    for ( i = 0; i < 32; i++)
    {
        tunerSetCap(TUNER_Clen,i);
        noiseval = as399xGetReflectedPowerNoiseLevel();
        val = as399xGetReflectedPower( );
        ch_val_i = (s8)((val&0xff) - (noiseval&0xff));
        ch_val_q = (s8)(((val>>8)&0xff) - ((noiseval>>8)&0xff));
        rf = sqrt(ch_val_i * ch_val_i + ch_val_q * ch_val_q);
        CON_print("i=%hhd rf=%hx\n",i,rf);
    }
}
#endif

/*------------------------------------------------------------------------- */
void tunerSetCap( u8 component, u8 val )
{
    u8 d;
    u8 spi0cfg, spi0ckr;

    /* save spi defaults */
    spi0cfg = SPI0CFG;
    spi0ckr = SPI0CKR;

    /* reconfigure spi */
    SPI0CFG = 0x00; /* off */
#if (CLK == 48000000) /* reg = f_sys/(2f) - 1 */
    SPI0CKR = 48/(1*2)-1; /* 1 MHz */
#elif (CLK == 24000000)
    SPI0CKR = 24/(1*2)-1; /* 1 MHz */
#elif (CLK == 12000000)
    SPI0CKR = 12/(1*2)-1; /* 1 MHz */
#endif
    SPI0CFG = 0x40;/*  MSTEN=1, CPHA=0, CKPOL=0, */

    switch (component)
    {
        case TUNER_Cin:
            SEN_TUNER_CIN(1);
            break;
        case TUNER_Clen:
            SEN_TUNER_CLEN(1);
            break;
        case TUNER_Cout:
            SEN_TUNER_COUT(1);
            break;
    }

    SPI0DAT = val;
    while(!(SPI0CN & 0x80));
    SPI0CN = 1; /* Clear the interrupt flag */
    d = SPI0DAT; /* dummy receive this byte */

    switch (component)
    {
        case TUNER_Cin:
            SEN_TUNER_CIN(0);
            break;
        case TUNER_Clen:
            SEN_TUNER_CLEN(0);
            break;
        case TUNER_Cout:
            SEN_TUNER_COUT(0);
            break;
    }
    /* restore spi to default */
    SPI0CFG = spi0cfg;
    SPI0CKR = spi0ckr;
}
u16 tunerGetReflected(void)
{
    u16 r; u32 refl;
    s8 i, q;
#ifdef POWER_DETECTOR
    as399xCyclicPowerRegulation();
#endif
    r = as399xGetReflectedPower();
    i = r & 0xff;
    q = r >> 8;
    refl = (i * i) + (q * q);

    r = refl;
    if (refl > 0xffff) r= 0xffff;

    return r;
}

u8 tunerClimbOneParam( u8 el, u8 *elval, u16 *reflectedPower, u16 *maxSteps)
{
    u16 refl, start = *reflectedPower;
    s8 dir = 0;
    s8 add = 0;
    u8 improvement = 3;
    u8 bestelval;
    if (*maxSteps == 0)
    {
        return 0;
    }

    if ( *elval != 0 )
    {
        tunerSetCap( el, *elval - 1 ); 
        refl = tunerGetReflected();
        if ( refl <= *reflectedPower )
        {
            *reflectedPower = refl;
            dir = -1;
        }
    }

    if ( *elval < 31 )
    {
        tunerSetCap( el, *elval + 1 ); 
        refl = tunerGetReflected();
        if ( refl <= *reflectedPower )
        {
            *reflectedPower = refl;
            dir = 1;
        }
    }

    /* if it's possible we try to change the value by 2 and check what direction is better.
     * This improves tuning in the case of a local minima. */
    if ( *elval > 1 )
    {
        tunerSetCap( el, *elval - 2 );
        refl = tunerGetReflected();
        if ( refl <= *reflectedPower )
        {
            *reflectedPower = refl;
            dir = -1;
            add = -1;
        }
    }

    if ( *elval < 30 )
    {
        tunerSetCap( el, *elval + 2 );
        refl = tunerGetReflected();
        if ( refl <= *reflectedPower )
        {
            *reflectedPower = refl;
            dir = 1;
            add = 1;
        }
    }

    *elval += add;
    *elval += dir;
    bestelval = *elval;

    if (dir!=0)
    {
        (*maxSteps)--;
        while ( improvement && *maxSteps )
        {
            if ( *elval == 0 ) break;
            if ( *elval == 31 ) break;
            tunerSetCap( el, *elval + dir ); 
            refl = tunerGetReflected();
            if ( refl <= *reflectedPower )
            {
                (*maxSteps)--;
                *reflectedPower = refl;
                *elval += dir;
                bestelval = *elval;
                improvement = 3; /* we don't want to stop when a local minima is hit, so we
                continue tuning for 3 more steps, even if they are worse than the one before.
                If it does not improve in this 3 steps, we will go back to the best value. */
            }else{
                improvement--;
            }
        }
    }
    *elval = bestelval;
    tunerSetCap( el, *elval);
    return start > (*reflectedPower);
}

void tunerOneHillClimb( struct tunerParams *p, u16 maxSteps)
{
    u8 improvement = 1;

    tunerSetCap(TUNER_Cout,p->cout);
    tunerSetCap(TUNER_Clen,p->clen);
    tunerSetCap(TUNER_Cin,p->cin);

    p->reflectedPower = tunerGetReflected();

    while (maxSteps && improvement)
    {

        improvement = tunerClimbOneParam(TUNER_Clen, &p->clen, &p->reflectedPower, &maxSteps);
        improvement |= tunerClimbOneParam(TUNER_Cout, &p->cout, &p->reflectedPower, &maxSteps);
        improvement |= tunerClimbOneParam(TUNER_Cin, &p->cin, &p->reflectedPower, &maxSteps);

    }

    tunerSetCap(TUNER_Cin, p->cin);
    tunerSetCap(TUNER_Cout, p->cout);
    tunerSetCap(TUNER_Clen, p->clen);
}

static const u8 tunePoints[3] = {5,16,26};

void tunerMultiHillClimb( struct tunerParams *res )
{
    struct tunerParams p;
    u8 i,j;

    tunerSetCap(TUNER_Cout,res->cout);
    tunerSetCap(TUNER_Clen,res->clen);
    tunerSetCap(TUNER_Cin,res->cin);

    res->reflectedPower = tunerGetReflected();

    for( i = 0; i < 27; i++ )
    {
        j=i;
        p.cin  = tunePoints[j%3]; j/=3;
        p.cout = tunePoints[j%3]; j/=3;
        p.clen = tunePoints[j%3]; j/=3;
        tunerOneHillClimb(&p, 30);
        if ( p.reflectedPower < res->reflectedPower )
        {
            *res = p;
        }
    }

    tunerSetCap(TUNER_Cout,res->cout);
    tunerSetCap(TUNER_Clen,res->clen);
    tunerSetCap(TUNER_Cin,res->cin);
}

void tunerTraversal( struct tunerParams *res )
{
    u8 l,o,i;
    u16 refl;
    res->reflectedPower = 32767;
    for (l = 0; l < 32; l++)
    {
        tunerSetCap(TUNER_Clen,l);
        CON_print("%hhx\n",l);
        for (o = 0; o < 32; o++)
        {
            tunerSetCap(TUNER_Cout,o);
            for (i = 0; i < 32; i++)
            {
                tunerSetCap(TUNER_Cin,i);
                refl = tunerGetReflected();

                if ( refl < res->reflectedPower )
                {
                    res->cin  = i;
                    res->cout = o;
                    res->clen = l;
                    res->reflectedPower = refl;
                    CON_print("cin=%hhx,cout=%hhx,clen=%hhx r=%hx\n",res->cin,res->cout,res->clen,res->reflectedPower);
                }
            }
        }
    }
    tunerSetCap(TUNER_Cin, res->cin);
    tunerSetCap(TUNER_Cout, res->cout);
    tunerSetCap(TUNER_Clen, res->clen);
    CON_print("cin=%hhx,cout=%hhx,clen=%hhx r=%hx\n",res->cin,res->cout,res->clen,res->reflectedPower);
}

void tunerSetTuning(u8 cin, u8 clen, u8 cout )
{
    tunerSetCap(TUNER_Cin, cin);
    tunerSetCap(TUNER_Clen, clen);
    tunerSetCap(TUNER_Cout, cout);
}
#endif
