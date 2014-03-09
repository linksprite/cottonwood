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
 *  \author U. Herrmann (based on work by E.Grubmueller)
 *  \author T. Luecker (Substitute)
 *  \author B. Breinbauer
 *
 *   \brief Functions provided by the AS399x series chips
 *
 *   Functions provided by the AS399x series chips. All higher level
 *   and protocol work is contained in gen2.c and iso6b.c
 */
#include "c8051F340.h"
#include "as399x_config.h"
#include "platform.h"
#include "as399x.h"
#include "as399x_public.h"
#include "global.h"
#include "as399x_com.h"
#include "uart.h"
#include "timer.h"
#include "gen2.h"
#include "stdlib.h"
#include "string.h"
#if VERBOSE_INIT
#include <math.h>
#endif

/** Definition protocol read bit. */
#define READ                      0x40
/** Definition protocol continous bit */
#define CONTINUOUS                 0x20

#ifdef POWER_DETECTOR
/* We handle the DAC values in the code like it is a 8 bit DAC from 0V to 3.2V, LSB = 12.5mV.
 * But when we write to the DAC register of AS399x we have to convert our internal representation.
 * This is done in as399xWriteDAC() by the macro CONVERT_NAT_TO_DAC.
 * Side note: the DAC values 0x80 and 0x00 give the same DAC output voltage as they only differ in sign bit. */
#if ARNIE
/* Arnie has a different PA setup. Vapc1 is fixed and Vapc2 is controlled by DAC */
#define MIN_DAC  16 /* corresponds to 0.2V Bias on PA */
#define NOM_DAC 104 /* corresponds to 1.3V Bias on PA */
#define MAX_DAC 232 /* corresponds to 2.9V Bias on PA */
#else
#define MIN_DAC 152 /* corresponds to 1.9V Bias on PA */
#define NOM_DAC 160 /* corresponds to 2.0V Bias on PA */
#define MAX_DAC 224 /* corresponds to 2.8V Bias on PA */
#endif
#endif

/** Variable is used to store the IRQ status read from the AS399x */
u8 DATA as399xIrqStatus = 0;

/** Variable is used to store the AS399X_REG_FIFO status read from the AS399x */
u8 DATA as399xFifoStatus = 0;

/** This variable is used as flag to signal an data reception.
  *  It is a bit mask of the RESP_TXIRQ, RESP_RXIRQ, RESP_* values
  */
volatile u16 DATA as399xResponse = 0;

u8 as399xChipVersion;

u32 as399xCurrentBaseFreq;

static u8 as399xSavedSensRegs[2];
static u8 as399xPowerDownRegs[AS399X_REG_ADC];
static u8 as399xPowerDownRegs3[5][3]; /* 5 registers are 3 bytes deep */


/** Is set to 1 in as399xAntennaPower if output power is on. In as399xCyclicPowerRegulation()
 * the output power is re-adjusted if output power is on. */
static u8 outputPowerOn = 0;
/** Counts the number of cycles as399xCyclicPowerRegulation() has been called. */
static u16 outputPowerCounter;
/** The outputPowerCounter will be reset to this value after underflow. */
static u16 outputPowerCounterStart = OUTPUTPOWER_MAINCYCLES;
/** If evalPowerRegulation is set to 1 the automatic power regulation will not be executed. (Should be used for evaluation purposes only.) */
static u8 evalPowerRegulation = 0;

#if RUN_ON_AS3992
const u8 CODE as399xPredistortionData[252]={ 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa 
};
#endif
#if VERBOSE_INIT
static void as399xPrintRfpLine(void)
{
        s16 val;
        s16 noiseval;
        s16 ch_val_i;
        s16 ch_val_q;
        s16 rf;

        noiseval = as399xGetReflectedPowerNoiseLevel();
        val = as399xGetReflectedPower( );
        ch_val_i = (s8)((val&0xff) - (noiseval&0xff));
        ch_val_q = (s8)(((val>>8)&0xff) - ((noiseval>>8)&0xff));
        rf = sqrt(ch_val_i * ch_val_i + ch_val_q * ch_val_q);
        CON_print("rf=%hx ",rf);

        CON_print("%hx", val);

        for (val = -32; val <= 32 ; val ++)
        {
            if ( val == rf )
            {
                CON_print("A");
                continue;
            }
            if ( val == ch_val_i && val == ch_val_q)
            {
                CON_print("X");
                continue;
            }
            if ( val == ch_val_i )
            {
                CON_print("I");
                continue;
            }
            if ( val == ch_val_q )
            {
                CON_print("Q");
                continue;
            }
            if ( val == 0 )
            {
                CON_print("0");
                continue;
            }
            CON_print(" ");
        }
        CON_print("\n");
}
#endif
/*------------------------------------------------------------------------- */
u16 as399xInitialize(u32 baseFreq)
{
    u8 myBuf[4];

#if AS399X_DO_SELFTEST
    myBuf[0] = 0x55;
    myBuf[1] = 0xAA;
    myBuf[2] = 0x00;
    myBuf[3] = 0xFF;
    as399xContinuousWrite(AS399X_REG_PLLMAIN, myBuf, 4);
    memset(myBuf, 0, sizeof(myBuf));
    as399xContinuousRead(AS399X_REG_PLLMAIN, 4,myBuf);
    if ((myBuf[0]!=0x55) || 
        (myBuf[1]!=0xAA) || 
        (myBuf[2]!=0x00) || 
        (myBuf[3]!=0xFF))
    {
        CON_print("read regs: %hhx %hhx %hhx %hhx\n", myBuf[0], myBuf[1], myBuf[2], myBuf[3]);
        return 1; /* data bus interface pins not working */
    }
    EN(LOW);
    mdelay(1); /* Reset the registers again, values should change */
    EN(HIGH);
    mdelay(12);  /* AS3992 needs 12 ms to exit standby */
    as399xContinuousRead(AS399X_REG_PLLMAIN, 4,myBuf);
    if ((myBuf[0]==0x55) || 
        (myBuf[1]==0xAA) || 
        (myBuf[2]==0x00) || 
        (myBuf[3]==0xFF))
    {
        CON_print("EN pin failed\n");
        return 1; /* enable pin not working */
    }

    as399xClrResponse();
    as399xSingleWrite(AS399X_REG_IRQMASKREG, 0x20);
    as399xSingleWrite(AS399X_REG_TXLENGTHLOW, 0x02); /* 1bit transmission */
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    as399xSingleCommand(AS399X_CMD_TRANSM);
    as399xContinuousWrite(AS399X_REG_FIFO,myBuf,4);
    as399xContinuousWrite(AS399X_REG_FIFO,myBuf,4);
    as399xContinuousWrite(AS399X_REG_FIFO,myBuf,4);
    as399xContinuousWrite(AS399X_REG_FIFO,myBuf,4);
    as399xContinuousWrite(AS399X_REG_FIFO,myBuf,4);
    as399xContinuousWrite(AS399X_REG_FIFO,myBuf,4);

    as399xWaitForResponse(RESP_FIFO);
    if ( !(as399xGetResponse() & RESP_FIFO) )
    {
        CON_print("IRQ pin / reading IRQ flags failed\n");
        return 1; /* Interrupt pin not working */
    }

    as399xClrResponse();

    as399xReset(); /* Reset is necessary because of above bogus PLL register settings */

    as399xSingleCommand(AS399X_CMD_HOPMAINFREQ);
#endif

    switch (as399xReadChipVersion())
    {
        case 0x30:
            as399xChipVersion = AS3990;
#if RUN_ON_AS3992
            CON_print("Cannot run on AS3990\n");
            return 0xf;
#endif
            break;
        case 0x38:
            as399xChipVersion = AS3991;
#if RUN_ON_AS3992
            CON_print("Cannot run on AS3991\n");
            return 0xf;
#endif
            break;
        case 0x50:
        case 0x51:
        case 0x55:
            as399xChipVersion = AS3992;
#if ! RUN_ON_AS3992
            CON_print("Cannot run on AS3992\n");
            return 0xf;
#else
            break;
#endif
        default:
            CON_print("Unknown chip %hhx\n",as399xReadChipVersion());
            return 0xf;
    }

    /* chip status control 0x00 */
    /*STBY DIRECT AS399X_REG_DACEN - AGL AGC REC RF */
    /* 0     0     0    0  0   0   1   0  = 0x02 */
    as399xSingleWrite(AS399X_REG_STATUSCTRL, 0x02);

    /*protocl control register 0x01 */
    /*RX_CRC_N DIR_MODE PROT1 PROT0 RX_COD1 RX_COD0 TARI1 TARI0 */
    /*   0         0      0     0     0        1      1     0      = 0x06 */
#if 0
    as399xSingleWrite(AS399X_REG_PROTOCOLCTRL, 0x02); /*smart ID change */
    as399xSingleWrite(AS399X_REG_PROTOCOLCTRL, 0x46); /* for receiver measurments */
#endif
    as399xSingleWrite(AS399X_REG_PROTOCOLCTRL, 0x06);

    /*TX options 0x02 */
    /*TX_PWD1 TX_PWD0 TX_ONE1 TX_ONE0 - - S1 S0 */
    /*   1       1       1      1     0 0 0  0   = 0xF0 */
    as399xSingleWrite(AS399X_REG_TXOPTGEN2, 0xF0);

#ifdef LOWSUPPLY
    CON_print("LOW supply\n");
    myBuf[0]=as399xSingleRead(AS399X_REG_TRCALGEN2MISC);
    myBuf[0]|=0x40;
    as399xSingleWrite(AS399X_REG_TRCALGEN2MISC, myBuf[0]);
#endif

#ifdef SINGLEINP
    /*RX special settings 2 0x0A */
    /*GAIN5 - GAIN1 S_MIX IR1 IR0 */
    /*  0 0 0 0 1     1    0   0 = 0x0C */
    as399xSingleWrite(AS399X_REG_RXSPECIAL2, 0x0C);
#if VERBOSE_INIT
    CON_print("SINLGEINP\n");
#endif

#endif

#ifdef BALANCEDINP
    /*RX special settings 2 0x0A */
    /*GAIN5 - GAIN1 S_MIX IR1 IR0 */
    /*  0 0 0 0 1     0    0   1 = 0x09 */
    as399xSingleWrite(AS399X_REG_RXSPECIAL2, 0x09);
#if ROGER || MICROREADER
    as399xSingleWrite(AS399X_REG_RXSPECIAL2, 0x08);
#endif
#if VERBOSE_INIT
    CON_print("BALANCED INP\n");
#endif
#endif

#ifdef LOWSUPPLY
    CON_print("LOW supply\n");
    myBuf[0]=as399xSingleRead(AS399X_REG_RXSPECIAL2);
    myBuf[0]|=0x02;
    as399xSingleWrite(AS399X_REG_RXSPECIAL2, myBuf[0]);
#endif

#ifdef INTPA
    /*REGULATOR and IO control 0x0B */
    /*IO_LOW REG2V1 REG2V0 EXT4 EXT3 EXT2 EXT1 EXT0 */
    /*  0       1      0     1   1    0    0    0   = 0x58 */
    /* PA 14mA */
    as399xSingleWrite(AS399X_REG_IOCONTROL, 0x58);

#if RUN_ON_AS3992
    /*TX pre-distortion 0x13*/
    as399xWritePredistortion(as399xPredistortionData);
#endif

    /*Modulator Control Register 0x15 */
    /*TRFON1 TRFON0 TPRESET TXLEVEL4-TXLEVEL0 */
    /*   0     0       0      0       0 1 1 1  =0x07 */
    myBuf[0] = 0x01;

    /*LIN_MODE PR_ASK MOD_DEP5-MOD_DEP0 */
    /*    0      1      0 1     1 1 0 1   = 0x5D */
    myBuf[1] = 0x5D;
#if !RUN_ON_AS3992
    /*E_AMOD MAIN_MOD AUXMOD - - ELFP ASKRATE1 ASKRATE0 */
    /*  0       1       0    0 0  0      0        0      =0x48 */
    myBuf[2] = 0x40; /*changed to route to main for 20 dDm */
#else
    /*E_AMOD MAIN_MOD AUXMOD - USE_CORR ELFP ASKRATE1 ASKRATE0 */
    /*  0       1       0    0     1      0        0      =0x48 */
    myBuf[2] = 0x48; /*changed to route to main for 20 dDm */
#endif
    as399xContinuousWrite(AS399X_REG_MODULATORCTRL, myBuf, 3);

    /*PLL Main Register 0x16 */
    /*AI2X   REF_FREQ     BBBB  */
    /* 1      110 (50kHz) 0000  = 0xe0 */
    myBuf[2] = 0xe0; /* changed to 50 kHz reference and increase internal PA bias */
    /* BBBB BB AA */
    /* 1101 10 00 = 0xd8 */
    myBuf[1] = 0xd8;
    /* AAAAAAAA */
    /*  0x4f */
    myBuf[0] = 0x4f;
    as399xContinuousWrite(AS399X_REG_PLLMAIN, myBuf, 3);

#if VERBOSE_INIT
    CON_print("INTPA\n");
#endif
#endif

#ifdef EXTPA
    /*PLL Main Register 0x16 */
    /*AI2X   REF_FREQ     BBBB  */
    /* 0      110 (50kHz) 0000  = 0xe0 */
    myBuf[2] = 0x60; /* changed to 50 kHz reference */
    /* BBBB BB AA */
    /* 1101 10 00 = 0xd8 */
    myBuf[1] = 0xd8;
    /* AAAAAAAA */
    /*  0x4f */
    myBuf[0] = 0x4f;
    as399xContinuousWrite(AS399X_REG_PLLMAIN, myBuf, 3);

    /*REGULATOR and IO control 0x0B */
    /*IO_LOW REG2V1 REG2V0 EXT4 EXT3 EXT2 EXT1 EXT0 */
    /*  0       0      0     0   0    0    1    0   = 0x02 */
    /* PA 14mA */
    as399xSingleWrite(AS399X_REG_IOCONTROL, 0x02);

    /*Modulator Control Register 0x15 */
    /*TRFON1 TRFON0 TPRESET TXLEVEL4-TXLEVEL0 */
    /*   0     0       0      0       0 1 1 1  =0x07 */
    myBuf[0] = 0x01;
#if ROGER || MICROREADER
    myBuf[0] = 0x06; /* reduce the power to be in the mask */
#endif
#if ROLAND || ARNIE
    myBuf[0] = 0x0d; /* reduce the power to be in the mask */
#endif
    /*LIN_MODE PR_ASK MOD_DEP5-MOD_DEP0 */
    /*    0      1      0 1     1 1 0 1   = 0x5D */
    myBuf[1] = 0x5D;
    /*E_AMOD MAIN_MOD AUXMOD - - ELFP ASKRATE1 ASKRATE0 */
    /*  0       0       1    0 0  0      0        0      =0x20 */
    myBuf[2] = 0x20;
    as399xContinuousWrite(AS399X_REG_MODULATORCTRL, myBuf, 3);
#if VERBOSE_INIT
    CON_print("EXTPA\n");
#endif
#endif

#if MICROREADER
    myBuf[0] = as399xSingleRead(AS399X_REG_IOCONTROL);
    /*REGULATOR and IO control 0x0B */
    /*IO_LOW REG2V1 REG2V0 EXT4 EXT3 EXT2 EXT1 EXT0 */
    /*  0       1      1     0   0    0    1    0   = 0x02 */
    /* PA 14mA */
    myBuf[0] |= 0x60;
    as399xSingleWrite(AS399X_REG_IOCONTROL, myBuf[0]);
#endif

#if ROLAND || ARNIE
    myBuf[0] = as399xSingleRead(AS399X_REG_IOCONTROL);
    /*REGULATOR and IO control 0x0B */
    /*IO_LOW REG2V1 REG2V0 EXT4 EXT3 EXT2 EXT1 EXT0 */
    /*  0       0      1     0   0    0    1    0   = 0x02 */
    /* PA 14mA */
    myBuf[0] |= 0x20;
    as399xSingleWrite(AS399X_REG_IOCONTROL, myBuf[0]);
#endif

    /*Interrupt Mask register 0x0D */
    /*= 0x3F */
    as399xSingleWrite(AS399X_REG_IRQMASKREG, 0x37);

#ifdef INTVCO
    myBuf[0] = 0x03;   /* high cp current, no SYS clock */
    myBuf[1] = 0x84;   /* */
#if ROLAND || ARNIE
    myBuf[2] = 0x4A;   /* external TCXO AC coupled to OSCO */
#else
    myBuf[2] = 0x0A;
#endif

    as399xContinuousWrite(AS399X_REG_CLSYSAOCPCTRL, myBuf, 3);
#if VERBOSE_INIT
    CON_print("INTVCO\n");
#endif
#endif
#ifdef EXTVCO
    myBuf[0] = 0x01;   /* low  cp current, no sys clk */
    myBuf[1] = 0x0C;   /* */
    myBuf[2] = 0x00;   /* */
    as399xContinuousWrite(AS399X_REG_CLSYSAOCPCTRL, myBuf, 3);
#if VERBOSE_INIT
    CON_print("EXTVCO\n");
#endif
#endif

    myBuf[0] = 0x00; /* 0x0C: digital lines on OAD */
    myBuf[1] = 0x00;
    myBuf[2] = 0x00;
    as399xContinuousWrite(AS399X_REG_TESTSETTING, myBuf, 3);

#ifdef POWER_DETECTOR
    as399xSingleWrite(AS399X_REG_DAC, NOM_DAC);
    as399xSetTxPower(240);      // init output power to 24dBm
#endif
    
    as399xSelectLinkFrequency(0x0);
    /* Give base freq. a reasonable value */
    as399xSetBaseFrequency(AS399X_REG_PLLMAIN,baseFreq);
    as399xSetSensitivity(-76);

#if AS399X_DO_SELFTEST
    /* Now that the chip is configured with correct ref frequency the PLL 
       should lock */
    mdelay(20);
    myBuf[0]=as399xSingleRead(AS399X_REG_AGCINTERNALSTATUS);
    if ((myBuf[0] & 0x01) != 0x01) 
    {
        return 2; /* Crystal not stable */
    }
    if (!(myBuf[0] & 0x02)) 
    { 
       return 3; /* PLL not locked */
    }


#if VERBOSE_INIT
    {
        /* Do this now, after everyting else should be configured */
        u32 i = 100;
        while(i--){
            as399xPrintRfpLine();
        }
        for( i = 860000UL; i< 960000UL ;i+=1000UL)
        {
            CON_print("%hx%hx ",(u16)(i>>16),(u16)(i&0xffff));
            as399xSetBaseFrequency(AS399X_REG_PLLMAIN,i);
            as399xPrintRfpLine();
        }
    }
#endif
#endif
    return 0;
}

/*------------------------------------------------------------------------- */
/** External Interrupt Function
  * The AS3990 uses the external interrupt to signal the uC that
  *  something happened. The interrupt function reads the AS3990
  * IRQ status. If the signals fifo bit is set it reads the AS3990
  * AS399X_REG_FIFO status.
  * The interrupt function sets the flags if an event occours.
  */
void extInt(void) interrupt 0
{
    u8 DATA istat, istat2;
    as399xIrqStatus = as399xSingleReadIrq(AS399X_REG_IRQSTATUS);
    istat2 = as399xSingleReadIrq(AS399X_REG_IRQSTATUS);
    as399xFifoStatus = 0;

    istat = ((as399xIrqStatus | istat2) & 0x3f); /* regular interrupts, make sure to use all which are read out */

    if (as399xIrqStatus & (AS399X_IRQ_TX | AS399X_IRQ_RX))
    {
        istat |= ((as399xIrqStatus & 0xc0) & ~istat2); /* tx and rx irq have only happened if they disappear at second read */
    }

    as399xIrqStatus = istat;

    if (istat & AS399X_IRQ_FIFO)
    {
        as399xFifoStatus = as399xSingleReadIrq(AS399X_REG_FIFOSTATUS);
        if (( as399xFifoStatus &0x1f ) >= 18)
        { /* high fifo level bit is not working, need to calculate manually */
            as399xFifoStatus |= AS399X_FIFOSTAT_HIGHLEVEL;
        }
        if (( as399xFifoStatus &0x1f ) <= 6)
        { /* low fifo level bit is not working, need to calculate manually */
            as399xFifoStatus |= AS399X_FIFOSTAT_LOWLEVEL;
        }
    }

    as399xResponse |= istat | (as399xFifoStatus << 8 );
    //CON_print("isr %hx", as399xResponse);
}

/* The following functions could be used to push the spi communication out of the ISR. But this is a little
 * because you have to ensure that the irq status registers are checked regularly otherwise IR status information
 * might get lost.
 * Therefore we keep the current setup. */
//u16 as399xGetResponse(void)
//{
//    //for(; extIntCount; extIntCount--)
//    if(extIntCount)
//        lastCount = 0;
//    while(extIntCount)
//    {
//        u8 DATA istat, istat2;
//        if(!lastCount)
//            lastCount = extIntCount;
//        as399xIrqStatus = as399xGetIRQStatus();
//        istat2 = as399xGetIRQStatus();
//        lastirq = as399xIrqStatus;
//        lastistat2 = istat2;
//        as399xFifoStatus = 0;
//        //CON_print("i: %hhx %hhx\n", as399xIrqStatus, istat2);
//
//        istat = ((as399xIrqStatus | istat2) & 0x3f); /* regular interrupts, make sure to use all which are read out */
//
//        if (as399xIrqStatus & (AS399X_IRQ_TX | AS399X_IRQ_RX))
//        {
//            istat |= ((as399xIrqStatus & 0xc0) & ~istat2); /* tx and rx irq have only happened if they disappear at second read */
//        }
//
//        as399xIrqStatus = istat;
//        lastistat = istat;
//
//        if (istat & AS399X_IRQ_FIFO)
//        {
//            as399xFifoStatus = as399xGetFIFOStatus();
//            if (( as399xFifoStatus &0x1f ) >= 18)
//            { /* high fifo level bit is not working, need to calculate manually */
//                as399xFifoStatus |= AS399X_FIFOSTAT_HIGHLEVEL;
//            }
//            if (( as399xFifoStatus &0x1f ) <= 6)
//            { /* low fifo level bit is not working, need to calculate manually */
//                as399xFifoStatus |= AS399X_FIFOSTAT_LOWLEVEL;
//            }
//        }
//
//        as399xResponse |= istat | (as399xFifoStatus << 8 );
//        extIntCount--;
//    }
//    return as399xResponse;
//}
//
//void as399xClrResponseMask(u16 mask)
//{
//    as399xResponse&=~(mask);
//}
//
//void as399xClrResponse(void)
//{
//    as399xResponse = 0;
//}

/*------------------------------------------------------------------------- */
u8 as399xReadChipVersion(void)
{
    return(as399xSingleRead(AS399X_REG_VERSION));
}

/*------------------------------------------------------------------------- */
void as399xSingleCommand(u8 command)
{
    DISEXTIRQ();
    writeReadAS399x( &command, 1, 0 , 0 , STOP_SGL, 1);
    ENEXTIRQ();
}


/*------------------------------------------------------------------------- */
void as399xContinuousCommand(u8 *commands, s8 len)
{
    DISEXTIRQ();
    writeReadAS399x( commands, len, 0 , 0 , STOP_SGL, 1);
    ENEXTIRQ();
}


/*------------------------------------------------------------------------- */
void as399xSingleWriteNoStop(u8 address, u8 value)
{
    u8 buf[2];
    buf[0] = address;
    buf[1] = value;
    writeReadAS399x( buf, 2, 0 , 0 , STOP_NONE, 1);
}

/*------------------------------------------------------------------------- */
void as399xContinuousRead(u8 address, s8 len, u8 *readbuf)
{
    DISEXTIRQ();
    address |= READ|CONTINUOUS;
    writeReadAS399x( &address, 1, readbuf , len , STOP_CONT, 1);
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
void as399xFifoRead(s8 len, u8 *readbuf)
{
#if COMMUNICATION_SERIAL
    u8 address = (AS399X_REG_FIFO - 1) | READ ;
    DISEXTIRQ();
    writeReadAS399x( &address, 1, &address , 1 , STOP_NONE, 1);
    address = (AS399X_REG_FIFO ) | READ | CONTINUOUS ;
    writeReadAS399x( &address, 1, readbuf , len , STOP_CONT, 0);
    ENEXTIRQ();
#else
    as399xContinuousRead(AS399X_REG_FIFO, len , readbuf);
#endif
}


u8 as399xSingleRead(u8 address)
{
    u8 readdata;

    DISEXTIRQ();
    address |= READ;
    writeReadAS399x( &address, 1, &readdata , 1 , STOP_SGL, 1);

    ENEXTIRQ();
    return(readdata);
}


u8 as399xSingleReadIrq(u8 address)
{
    u8 readdata;

    DISEXTIRQ();
    address |= READ;
    writeReadAS399xIsr( address, &readdata );

    ENEXTIRQ();
    return(readdata);
}


/*------------------------------------------------------------------------- */
void as399xWritePredistortion(const u8 buf[252])
{
    u8 address, version;
    version = as399xReadChipVersion();
    address = (AS399X_REG_VERSION | CONTINUOUS);
    DISEXTIRQ();
    writeReadAS399x( &address, 1, 0 , 0 , STOP_NONE, 1);
    writeReadAS399x( &version, 1, 0 , 0 , STOP_NONE, 0);
    writeReadAS399x( buf+1, 251, 0 , 0 , STOP_CONT, 0);
    ENEXTIRQ();
}

void as399xContinuousWrite(u8 address, u8 *buf, s8 len)
{
    address |= CONTINUOUS;
    DISEXTIRQ();
    writeReadAS399x( &address, 1, 0 , 0 , STOP_NONE, 1);
    writeReadAS399x( buf, len, 0 , 0 , STOP_CONT, 0);
    ENEXTIRQ();
}

void as399xSingleWrite(u8 address, u8 value)
{
    u8 buf[2];
    buf[0] = address;
    buf[1] = value;
    DISEXTIRQ();
    writeReadAS399x( buf, 2, 0 , 0 , STOP_SGL, 1);
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
void as399xCommandContinuousAddress(u8 *command, u8 com_len,
                             u8 address, u8 *buf, u8 buf_len)
{
    address |= CONTINUOUS;
    DISEXTIRQ();
    writeReadAS399x( command, com_len, 0 , 0 , STOP_NONE, 1);
    writeReadAS399x( &address, 1, 0 , 0 , STOP_NONE, 0);
    writeReadAS399x( buf, buf_len, 0 , 0 , STOP_CONT, 0);
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
void as399xSwitchToIdleMode(void)
{
    as399xSingleCommand(AS399X_CMD_IDLE);
}

void as399xWaitForResponseTimed(u16 waitMask, u16 counter)
{
    while (((as399xResponse & waitMask) == 0) && (counter))
    {
        if (TIMER_IS_DONE())
        {
            timerStart_us(1000);
            counter--;
        }
    }
    if (counter==0)
    {
        as399xReset();
#if !UARTSUPPORT
        CON_print("TI O T %x %x\n", as399xResponse, waitMask);
#endif
        as399xResponse = RESP_NORESINTERRUPT;
    }

}
    
void as399xWaitForResponse(u16 waitMaskOrig)
{
    u16 counter;
    u16 DATA waitMask = waitMaskOrig;
    for (counter=0; counter < 0xFFFD; counter++)
    {
        if ((as399xResponse & waitMask) != 0)
            break;
    }
    if (counter > 0xFFF0)
    {
        as399xReset();
#if !UARTSUPPORT
        CON_print("TI O %x %x\n", as399xResponse, waitMask);
        //as399xDebugResponse();
#endif
        as399xResponse = RESP_NORESINTERRUPT;
    }
}

#if ISO6B
void as399xEnterDirectMode()
{
    u8 temp;
    u8 buf[3];
    /* workaround for reset bug in Protocol Register 1 */
    temp = as399xSingleRead(AS399X_REG_RXSPECIAL2);
    /*  Set to ISO 6B, Bit clock and data; Tari 25us */
    as399xSingleWrite(AS399X_REG_PROTOCOLCTRL, 0x22);
    as399xSingleWrite (AS399X_REG_RXSPECIAL2,temp); /* Finish workaround */

    as399xContinuousRead(AS399X_REG_MODULATORCTRL, 3, buf);
    buf[1] &= ~0x40;      // disable PR_ASK, it's not supported in ISO 6B
    as399xContinuousWrite(AS399X_REG_MODULATORCTRL, buf, 3);

    as399xSingleWrite(AS399X_REG_IRQMASKREG, 0x00); /* mask Irqs */

    temp = as399xSingleRead(AS399X_REG_STATUSCTRL);
    as399xSelectLinkFrequency(0); /*  Set to 40kHz Link frequency */
    as399xSingleCommand(AS399X_CMD_RESETFIFO);
    as399xSingleCommand(AS399X_CMD_ENABLERX);
    temp |= 0x43;
#if COMMUNICATION_SERIAL
    /* enter direct mode, rf on, rx on, NO STOPCONDITION !!! */
    as399xSingleWrite(AS399X_REG_STATUSCTRL, temp);
#else
    /* enter direct mode, rf on, rx on, NO STOPCONDITION !!! */
    as399xSingleWriteNoStop(AS399X_REG_STATUSCTRL, temp);
#endif
    AS399X_ENABLE_SENDER(); /* Disable receiver (IO2 low) and drive also IO3 low. */
    mdelay(1); /* AS3992 seems to need this */
    DISEXTIRQ(); /* disable Interrrupt */
    setPortDirect();
}

void as399xExitDirectMode()
{
    u8 buf[3];
    as399xClrResponse();
#if COMMUNICATION_SERIAL
    /* exit direct mode, rf on, rx on */
    as399xSingleWrite(AS399X_REG_STATUSCTRL, 0x03);
#endif
    AS399X_ENABLE_SENDER();
    setPortNormal();
    as399xContinuousRead(AS399X_REG_MODULATORCTRL, 3, buf);
    buf[1] |= 0x40;      // enable PR_ASK again for Gen2
    as399xContinuousWrite(AS399X_REG_MODULATORCTRL, buf, 3);

    /* clear Irq flags and unmask Irqs */
    as399xSingleRead(AS399X_REG_IRQSTATUS);
    as399xClrResponse();
    as399xSingleWrite(AS399X_REG_IRQMASKREG, AS399X_IRQ_MASK_ALL);

    ENEXTIRQ();
}
#endif

void as399xSelectLinkFrequency(u8 a)
{
    u8 reg;

#if RUN_ON_AS3992
    if ( a == 0)
    { /* bypass for 40 kHz */
        as399xSingleWrite(AS399X_REG_RXSPECIAL,0x40);
    }
    if ( a == 6)
    { /* bypass for 160 kHz */
        as399xSingleWrite(AS399X_REG_RXSPECIAL,0x80);
    }
#endif

    reg = as399xSingleRead(AS399X_REG_RXOPTGEN2);

    reg &= ~0xf0;
    reg |= (a<<4);
    as399xSingleWrite(AS399X_REG_RXOPTGEN2, reg);
}

static void as399xLockPLL(void)
{
    u8 buf[3];
    u8 var;
    u16 i;

    if(as399xChipVersion!=AS3990)
    {
        u8 vco_voltage;
        as399xContinuousRead(AS399X_REG_TESTSETTING, 3, buf);
        buf[2] &= ~0x01; buf[1] &= ~0xc0; buf[1] |= 0x40; /* have vco_ri in aglstatus */
        as399xContinuousWrite(AS399X_REG_TESTSETTING, buf, 3);

        i = 0;
        do
        {
            i++;
            udelay(100);  /* wait for pll locking */
            var=as399xSingleRead(AS399X_REG_AGCINTERNALSTATUS);
        } while ( (var & 0x02)==0 && (i<100));/* wait for PLL to be locked and give a few attempts */

        as399xContinuousRead(AS399X_REG_CLSYSAOCPCTRL, 3, buf);
        buf[1] |= 0x08; /* set mvco bit */
        as399xContinuousWrite(AS399X_REG_CLSYSAOCPCTRL, buf, 3);
        udelay(200);
        vco_voltage = as399xSingleRead(AS399X_REG_AGLSTATUS) & 0x07; 

        buf[1] &= ~0x08; /* reset mvco bit */
        as399xContinuousWrite(AS399X_REG_CLSYSAOCPCTRL, buf, 3);

        /* if we are in the guardband area of an pll segment switch to a segment which fits better */
        if ( vco_voltage <= 1 || vco_voltage >= 6 )
        {
            i=0;
            do
            {
                i++;
                buf[1] &= ~(0x80);   /* reset the auto bit */
                as399xContinuousWrite(AS399X_REG_CLSYSAOCPCTRL, buf, 3);
                buf[1] |= 0x80;   /* set the auto Bit */
                as399xContinuousWrite(AS399X_REG_CLSYSAOCPCTRL, buf, 3);
                mdelay(10);  /* Please keep in mind, that the Auto Bit procedure will take app. 6 ms whereby the locktime of PLL is just 400us */
                var=as399xSingleRead(AS399X_REG_AGCINTERNALSTATUS);
            } while ( (var & 0x02)==0 && (i<3));/* wait for PLL to be locked and give a few attempts */
        }
    }
}

/*------------------------------------------------------------------------- */
void as399xSetBaseFrequency(u8 regs, u32 frequency)
{
    u8 buf[3];
    u8 statusreg;
    u16 ref, i, j, x, reg_A,reg_B;
    u32 divisor;

    as399xCurrentBaseFreq = frequency;
    statusreg= as399xSingleRead(AS399X_REG_STATUSCTRL);
    as399xSingleWrite(AS399X_REG_STATUSCTRL,statusreg&0xfe);
    if (regs== AS399X_REG_PLLMAIN)
    {
        as399xContinuousRead(AS399X_REG_PLLMAIN, 3, buf);
        switch (buf[2]& 0x70)
        {
        case 0x00: {
            ref=500;
        } break;
        case 0x10: {
            ref=250;
        } break;
        case 0x40: {
            ref=200;
        } break;
        case 0x50: {
            ref=100;
        } break;
        case 0x60: {
            ref=50;
        } break;
        case 0x70: {
            ref=25;
        } break;
        default: {
            ref=0;
        }
        }
    }
    divisor=frequency/ref;

    i = 0x3FFF & (divisor >> 6); /* division by 64 */
    x = (i<<6)+ i;
    if (divisor > x)
    {
        x += 65;
        i++;
    }
    x -= divisor;
    j = i;
    do
    {
        if (x >= 33)
        {
            i--;
            x -= 33;
        }
        if (x >= 32)
        {
            j--;
            x -= 32;
        }
    } while (x >= 32);
    if (x > 16) 
    {            /* subtract 32 from x if it is larger than 16 */
        x -= 32; /* this yields more closely matched A and B values */
        j--;
    }

    reg_A = i - x;
    reg_B = j + x;
    if (regs==AS399X_REG_PLLMAIN)
    {
        buf[2] = (buf[2] & 0xF0) | ((u8)((reg_B >> 6) & 0x0F));
        buf[1] = (u8)((reg_B << 2) & 0xFC) |  (u8)((reg_A >> 8) & 0x03);
        buf[0] = (u8)reg_A;

        as399xContinuousWrite(AS399X_REG_PLLMAIN, buf, 3);
    }
    as399xLockPLL();
    as399xSingleWrite(AS399X_REG_STATUSCTRL,statusreg);
}

/*------------------------------------------------------------------------- */
void as399xMemoryDump(void)
{
    u8 error,i;
    u8 readbuf[4];

    for (i=0;i<0x1F;i++)
    {
        CON_print("\nreg %hhx   ",i);

        if (i==0x12 || i==0x14 || i==0x15 || i==0x16 || i==0x17 )
        {
            as399xContinuousRead(i, 3, readbuf);
            CON_print("%hhx ",readbuf[2]);
            CON_print("%hhx ",readbuf[1]);
            CON_print("%hhx ",readbuf[0]);
        }
        else
        {
            error = as399xSingleRead(i);
            CON_print("%hhx",error);
        }
    }
}

void as399xReset(void)
{
    as399xEnterPowerDownMode();
    mdelay(1);
    as399xExitPowerDownMode();
}


void as399xAntennaPower( u8 on )
{
    u8 val, dac = 0;
    unsigned count;

    val = as399xSingleRead(AS399X_REG_STATUSCTRL);

    outputPowerOn = on;     //keep state of output power for as399xCyclicPowerRegulation()

    if (on)
    {
        if (val & 0x01)
            return;         // antenna power is already turned on.
#if ROGER 
        DCDC(LOW);  
        udelay(5); /* To avoid current spikes */
#endif
#ifdef EXTPA
        EN_PA(HIGH);
        udelay(200); /* Give PA LDO time to get to maximum power */
#endif
#ifdef POWER_DETECTOR
        /* set DAC to lowest value */
        dac = as399xReadDAC();
        as399xWriteDAC(MIN_DAC);
        val |= 0x21; /* DAC & rf_on enable */
        EN_BIAS = 1;
        as399xSingleWrite(AS399X_REG_STATUSCTRL, val);
        /* ramp up DAC smoothly (100mV steps), otherwise we will get spikes in spectrum */
        for (count = MIN_DAC+4; count < dac; count += 4)
        {
            as399xWriteDAC(count);
            udelay(10);
        }
        as399xWriteDAC(dac);        /* ensure that the initial DAC value is set again */
        udelay(100); /* PA has to settle after new dac voltage */
#else
        val |= 1;
        as399xSingleWrite(AS399X_REG_STATUSCTRL, val);
#endif
    }
    else
    {
        if (!(val & 0x01))
            return;         // antenna power is already turned off.
#ifdef POWER_DETECTOR
        /* ramp DAC down */
        dac = as399xReadDAC();
        for (count = dac; count >= MIN_DAC+4; count -= 4)
        {
            as399xWriteDAC(count);
            udelay(10);
        }
        val &= ~0x20; /* DAC disable */
        as399xSingleWrite(AS399X_REG_STATUSCTRL, val);
        EN_BIAS = 0;
        as399xWriteDAC(dac);        /* keep dac value for later power up */
#endif
        val &= ~1;
        as399xSingleWrite(AS399X_REG_STATUSCTRL, val);
        /* Wait for antenna being switched off */
        count = 500;
        while(count-- && (as399xSingleRead(AS399X_REG_AGCINTERNALSTATUS) & 0x04))
        {
            mdelay(1);
        }
#ifdef EXTPA
        EN_PA(LOW);
#endif
#if ROGER
        DCDC(HIGH);  
#endif
    }

    if(on) /* according to standard we have to wait 1.5ms before issuing commands  */
#if ROLAND
        mdelay(6);          // MOT wants to have higher dwell time
#else
        mdelay(1.5);        //TODO: define dwell time via GUI
#endif
}

static u8 as399xGetRawRSSI( void )
{
    u8 value;

    as399xSingleCommand(AS399X_CMD_BLOCKRX);
    as399xSingleCommand(AS399X_CMD_ENABLERX);
#if RUN_ON_AS3992
    udelay(500); /* According to architecture note we have to wait here at least 100us
                    however experiments show 350us to be necessary on AS3992 */
#else
    udelay(100); /* According to architecture note we have to wait here at least 100us */
#endif
    value = as399xSingleRead(AS399X_REG_RSSILEVELS);
    as399xSingleCommand(AS399X_CMD_BLOCKRX);
    return value;
}

void as399xSaveSensitivity( )
{
    as399xSavedSensRegs[0] = as399xSingleRead(AS399X_REG_TRCALGEN2MISC);
    as399xSavedSensRegs[1] = as399xSingleRead(AS399X_REG_RXSPECIAL2);
}

void as399xRestoreSensitivity( )
{
     as399xSingleWrite(AS399X_REG_TRCALGEN2MISC,as399xSavedSensRegs[0]);
     as399xSingleWrite(AS399X_REG_RXSPECIAL2   ,as399xSavedSensRegs[1]);
}

s8 as399xGetSensitivity( )
{
    s8 sensitivity = 0;
#if RUN_ON_AS3992
    u8 reg0a, reg05, gain;

    reg05 = as399xSingleRead(AS399X_REG_TRCALGEN2MISC);
    reg0a = as399xSingleRead(AS399X_REG_RXSPECIAL2);

    if ((reg0a & 0x04))
    { /* single ended input mixer*/
        sensitivity -= 72;
        if ( reg0a & 1 ) /* mixer input attenuation */
        {
            sensitivity += 5;
        }
    }
    else
    { /* differential input mixer */
        sensitivity -= 66;
        if ( reg0a & 1 )  /* mixer attenuation */
        {
            sensitivity += 8;
        }
        if ( reg0a & 2)  /* differential mixer gain increase */
        {
            sensitivity -= 10;
        }
    }
    gain = (reg0a >> 6) * 3;
    if ( reg05 & 0x80 )
    { /* RX Gain direction: increase */
        sensitivity -= gain;
    }
    else
    {
        sensitivity += gain;
    }
    return sensitivity;
#else
    u8 reg0a;
    reg0a   = as399xSingleRead(AS399X_REG_RXSPECIAL2);
    if ((reg0a & 0x04))
    { /* single ended input mixer*/
        if(reg0a & 1)
        { /* attenuation on */
            sensitivity -= 67;
        }
        else
        {
            sensitivity -= 72;
        }
    }
    else
    { /* differential input mixer */
        if(reg0a & 1)
        { /* attenuation on */
            sensitivity -= 58;
        }
        else
        {
            sensitivity -= 66;
        }
    }
    sensitivity += (reg0a >> 6) * 3;
    return sensitivity;
#endif
}
s8 as399xSetSensitivity( s8 minimumSignal )
{
#if RUN_ON_AS3992
    u8 reg0a, reg05, gain;

    reg05 = as399xSingleRead(AS399X_REG_TRCALGEN2MISC);
    reg0a = as399xSingleRead(AS399X_REG_RXSPECIAL2);

    reg0a &= ~0xc3;
    if ((reg0a & 0x04))
    { /* single ended input mixer*/
        minimumSignal += 72;
        if ( minimumSignal > 5 ) 
        {
            minimumSignal -= 5; /* mixer input attenuation */
            reg0a |= 1;
        }
    }
    else
    { /* differential input mixer */
        minimumSignal += 66;
        if ( minimumSignal >= 8 )  /* mixer attenuation */
        {
            minimumSignal -= 8;
            reg0a |= 1;
        }
        if ( minimumSignal <= -10 )  /* differential mixer gain increase */
        {
            minimumSignal += 10;
            reg0a |= 2;
        }
    }
    if ( minimumSignal > 0)
    {
        reg05 &= ~0x80; /* RX Gain direction: decrease */
        gain = minimumSignal / 3;
        if ( gain>3 ) gain = 3;
        minimumSignal -= gain*3;
        
    }
    else
    {
        reg05 |= 0x80; /* RX Gain direction: increase */
        gain = (-minimumSignal+2) / 3;
        if ( gain>3 ) gain = 3;
        minimumSignal += gain*3;
    }
    reg0a |= (gain<<6);
    as399xSingleWrite(AS399X_REG_TRCALGEN2MISC,reg05);
    as399xSingleWrite(AS399X_REG_RXSPECIAL2   ,reg0a);
    return minimumSignal;
#else
    u8 reg0a, gain;
    reg0a   = as399xSingleRead(AS399X_REG_RXSPECIAL2);
    reg0a &= ~0xc1;
    if ((reg0a & 0x04))
    { /* single ended input mixer*/
        if(minimumSignal < -67)
        {
            minimumSignal += 72;
        }
        else
        {
            reg0a |= 1; /* attenuation on */
            minimumSignal += 67;
        }
    }
    else
    { /* differential input mixer */
        if(minimumSignal < -58)
        {
            minimumSignal += 66;
        }
        else
        {
            reg0a |= 1; /* attenuation on */
            minimumSignal += 58;
        }
    }
    if ( minimumSignal > 0)
    {
        gain = minimumSignal / 3;
        if ( gain>3 ) gain = 3;
        minimumSignal -= gain*3;
        reg0a |= (gain<<6);
    }
    as399xSingleWrite(AS399X_REG_RXSPECIAL2   ,reg0a);
    return minimumSignal;
#endif
}

void as399xGetRSSI( u16 num_of_ms_to_scan, u8 *rawIQ, s8* dBm )
{
    u8 valstat;
    u8 rawVal, reg0a;
    u8 regFilter;
#if RUN_ON_AS3992
    u16 num_of_reads = num_of_ms_to_scan/2; /* as399xGetRawRSSI() delays 500us*/
    u8 reg05;
#else
    u16 num_of_reads = num_of_ms_to_scan/10; /* as399xGetRawRSSI() delays 100us*/
#endif 
    s8 sum;

    if (num_of_reads == 0) num_of_reads = 1;
    valstat = as399xSingleRead(AS399X_REG_STATUSCTRL);
    as399xSingleWrite(AS399X_REG_STATUSCTRL, 2 ); /* Receiver on and transmitter are off */
    regFilter = as399xSingleRead(AS399X_REG_RXSPECIAL);
#if RUN_ON_AS3992
    as399xSingleWrite(AS399X_REG_RXSPECIAL, 0xff ); /* Optimal filter settings */
#else
    as399xSingleWrite(AS399X_REG_RXSPECIAL, 0x01 ); /* Optimal filter settings */
#endif
    if(!(valstat & 0x02)) mdelay(10); /* rec_on needs about 6ms settling time, to be sure wait 10 ms */

    sum = 0;
    while (num_of_reads--)
    {
        u8 temp;
        rawVal = as399xGetRawRSSI();
        temp = (rawVal&0x0f) + (rawVal>>4);
        if (temp > sum)
        {
            sum = temp;
            *rawIQ = rawVal;
        }
    }

    if (sum == 0)
    { /* short exit, below formula does not work for 0 value */
        *dBm   = -128;
        *rawIQ = 0;
        goto end;
    }

#if RUN_ON_AS3992
    sum += (sum>>3); /* ~2.2*meanRSSI, here we calculate 2.25*meanRSSI, should not matter with a range 0..30 */
    reg0a = as399xSingleRead(AS399X_REG_RXSPECIAL2);
    reg05 = as399xSingleRead(AS399X_REG_TRCALGEN2MISC);
    if ((reg0a & 0x04))
    { /* single ended input mixer*/
        sum -= 75;
        if ((reg0a & 0x01)) sum += 5; /* attenuation */
    }
    else
    { /* differential input mixer */
        sum -= 71;
        if ((reg0a & 0x02)) sum -= 10; /* differential mixer gain increase */
        if ((reg0a & 0x01)) sum += 8; /* attenuation */
    }
    /* Gain in/de-crease */
    if (reg05 & 0x80)
    {
        sum -= 3*(reg0a>>6);
    }
    else
    {
        sum += 3*(reg0a>>6);
    }
#else
    reg0a   = as399xSingleRead(AS399X_REG_RXSPECIAL2);
    switch (reg0a & 0x05)
    {
        case 0: sum-=68; break; /* actually -67.5 */
        case 1: sum-=59; break;
        case 4: sum-=71; break; /* actually -70.5 */
        case 5: sum-=65; break;
    }
    sum += 3*(reg0a>>6);
#endif
    *dBm = sum;

end:
    as399xSingleWrite(AS399X_REG_RXSPECIAL, regFilter ); /* Restore filter */
    as399xSingleWrite(AS399X_REG_STATUSCTRL,valstat);
    if(valstat & 1) mdelay(6); /* according to standard we have to wait 1.5ms before issuing commands  */
    return;
}
/* ADC Values are in sign magnitude representation -> convert */
#define CONVERT_ADC_TO_NAT(A) (((A)&0x80)?((A)&0x7f):(0 - ((A)&0x7f)))
s8 as399xGetADC( void )
{
    s8 val;
    as399xSingleCommand(AS399X_CMD_TRIGGERADCCON);
    udelay(20); /* according to spec */
    val = as399xSingleRead(AS399X_REG_ADC);
    val = CONVERT_ADC_TO_NAT(val);
    return val;
}

/* DAC Values are in sign magnitude representation - convert */
#define CONVERT_NAT_TO_DAC(A) (((A)&0x80)?(A):(0x7f - ((A)&0x7f)))
#define CONVERT_DAC_TO_NAT(A) (((A)&0x80)?(A):(0x7f - ((A)&0x7f)))
void as399xWriteDAC( u8 natVal )
{
	u8 dacVal = CONVERT_NAT_TO_DAC(natVal);
    as399xSingleWrite(AS399X_REG_DAC, dacVal);
}
u8 as399xReadDAC( void )
{
	u8 dac, nat;
	dac = as399xSingleRead(AS399X_REG_DAC);
	nat = CONVERT_DAC_TO_NAT(dac);
	return nat;
}

u16 as399xGetADC_mV( void )
{
    s8 val;
    u16 mV;
    val = as399xGetADC();

    mV = val + 0x80;
    mV = 12 * mV + mV / 2; /*12.5 mV per division*/

    //CON_print("adc : %hx mV\n",mV);
    return mV;
}

u16 as399xGetReflectedPower( void )
{
    u16 value;
    s8 adcVal;
    u8 reg[3],reg14Temp,valstat;
    valstat = as399xSingleRead(AS399X_REG_STATUSCTRL);
    as399xContinuousRead(AS399X_REG_CLSYSAOCPCTRL, 3, reg);
    reg14Temp = reg[0];
    reg[0] &= ~0x30; /* disable the OAD pin outputs */
    as399xContinuousWrite(AS399X_REG_CLSYSAOCPCTRL, reg,3);

    /* measure the value */
    as399xSingleWrite(AS399X_REG_STATUSCTRL, valstat | 0x3 ); /* Receiver and transmitter are on */
    as399xSingleCommand(AS399X_CMD_BLOCKRX); /* Reset the receiver - otherwise the I values seem to oscillate */
    //mdelay(1);
    as399xSingleCommand(AS399X_CMD_ENABLERX);
    as399xSingleWrite(AS399X_REG_MEASURE_SELECTOR, 0x01); /* Mixer A DC */
#if ROLAND || ARNIE // TODO: check if we can reduce wait time for all boards (also in as399xGetReflectedPowerNoiseLevel)
    udelay(1000);
#else
    mdelay(2); /* settling time, test showed this is not necessary on LEO but it might on other boards*/
#endif
    adcVal = as399xGetADC();
    value = adcVal;
    as399xSingleWrite(AS399X_REG_MEASURE_SELECTOR, 0x02); /* Mixer B DC */
#if ROLAND || ARNIE
    udelay(1000);
#else
    mdelay(2); /* settling time, test showed this is not necessary on LEO but it might on other boards*/
#endif
    adcVal = as399xGetADC();

    /* mask out shifted in sign bits */
    value = (value &0xff) | (adcVal<<8);
    as399xContinuousRead(AS399X_REG_CLSYSAOCPCTRL, 3, reg); /* restore the values */
    reg[0]=reg14Temp;
    as399xContinuousWrite(AS399X_REG_CLSYSAOCPCTRL, reg,3);
    as399xSingleWrite(AS399X_REG_STATUSCTRL,valstat);
    return value;
}

u16 as399xGetReflectedPowerNoiseLevel( void )
{
    u16 value;
	s8 i_0, q_0;
    u8 reg[3],reg14Temp,valstat;
    valstat = as399xSingleRead(AS399X_REG_STATUSCTRL);
    as399xContinuousRead(AS399X_REG_CLSYSAOCPCTRL, 3, reg);
    reg14Temp = reg[0];
    reg[0] &= ~0x30; /* disable the OAD pin outputs */
    as399xContinuousWrite(AS399X_REG_CLSYSAOCPCTRL, reg,3);

	/* First measure the offset which might appear with disabled antenna */
    as399xSingleWrite(AS399X_REG_STATUSCTRL, (valstat | 2) & (~1) ); /* Field off, receiver on */
    as399xSingleCommand(AS399X_CMD_BLOCKRX); /* Reset the receiver - otherwise the I values seem to oscillate */
    //mdelay(1);
    as399xSingleCommand(AS399X_CMD_ENABLERX);
    as399xSingleWrite(AS399X_REG_MEASURE_SELECTOR, 0x01); /* Mixer A DC */
#if ROLAND || ARNIE
    udelay(1000);
#else
    mdelay(2); /* settling time, test showed this is not necessary on LEO but it might on other boards*/
#endif
    i_0 = as399xGetADC();
    as399xSingleWrite(AS399X_REG_MEASURE_SELECTOR, 0x02); /* Mixer B DC */
#if ROLAND || ARNIE
    udelay(1000);
#else
    mdelay(2); /* settling time, test showed this is not necessary on LEO but it might on other boards*/
#endif
    q_0 = as399xGetADC();

    value = (i_0 & 0xff) | (q_0<<8);
    as399xContinuousRead(AS399X_REG_CLSYSAOCPCTRL, 3, reg); /* restore the values */
    reg[0]=reg14Temp;
    as399xContinuousWrite(AS399X_REG_CLSYSAOCPCTRL, reg,3);
    as399xSingleWrite(AS399X_REG_STATUSCTRL,valstat);
    return value;
}

#ifdef POWER_DETECTOR

/** Points to the entry in dBm2Setting which represents the current power setting. */
struct powerDetectorSetting *currPDSet;
/** Index of currPDSet in dBm2Setting table. */
u8 currPDSetIndex;

/** This table keeps the mean values between to adjacent values of dBm2Setting.
 * When regulating the power in as399xAdaptTxPower() the DAC will not be modified,
 * when the measured ADC voltage is inside the correlating hysteresis values in
 * dBm2SettingHysteresis. */
u16 dBm2SettingHysteresis[POWER_TABLE_SIZE-1];

/**
 * This is an lookup table for the output power. The index of the table equals the
 * desired output power in dBm (eg: index 2 in the table contains settings for 2dBm
 * output power).
 */
#if ROLAND
struct powerDetectorSetting dBm2Setting[POWER_TABLE_SIZE]={
    {0x0F,449}, //0dBm
    {0x0F,456},
    {0x0F,473},
    {0x0F,481},
    {0x0F,504},
    {0x0F,524}, //5dBm
    {0x0F,544},
    {0x0F,576},
    {0x0F,595},
    {0x0F,631},
    {0x0F,657}, //10dBm
    {0x0F,690},
    {0x0F,728},
    {0x0F,757},
    {0x0F,806},
    {0x0F,840}, //15dBm
    {0x0F,890},
    {0x0F,952},
    {0x0F,1017},
    {0x0F,1110},
    {0x0F,1210},//20dBm
    {0x0F,1341},
    {0x0F,1490},
    {0x0F,1673},
    {0x0F,1879},
    {0x0F,2137},//25dBm
    {0x0D,2402},
    {0x0B,2732},
    {0x09,3114},
    {0x08,3553},
    {0x08,3972},//30dBm
};
#elif ARNIE
struct powerDetectorSetting dBm2Setting[POWER_TABLE_SIZE]={
    {0x0E,358}, //0dBm
    {0x0E,368},
    {0x0E,383},
    {0x0E,395},
    {0x0E,405},
    {0x0E,417}, //5dBm
    {0x0E,428},
    {0x0E,441},
    {0x0E,457},
    {0x0E,475},
    {0x0F,495}, //10dBm
    {0x0F,523},
    {0x0F,554},
    {0x0F,590},
    {0x0F,630},
    {0x0F,680}, //15dBm
    {0x0F,735},
    {0x0F,800},
    {0x0F,871},
    {0x0F,979},
    {0x0F,1066},//20dBm
    {0x0F,1193},
    {0x0F,1310},
    {0x0F,1443},
    {0x0E,1585},
    {0x0C,1750},//25dBm
    {0x0C,1963},
    {0x0A,2180},
    {0x0A,2430},
    {0x06,2676},
    {0x05,2950},//30dBm
};
#endif

s16 as399xSetTxPower(s16 des)
{
    s16 act;
    u8 reg[3], dac = NOM_DAC;

    act = (des + 5) / 10;       /* ensure correct rounding up */
    if (act < 0) act = 0;
    if (act > 30) act = 30;     //Todo: limit power differently for different boards?

    currPDSet = dBm2Setting + act;
    currPDSetIndex = act;

    /* Set up to register val */
    as399xContinuousRead(AS399X_REG_MODULATORCTRL, 3, reg);
    reg[0] = reg[0] & 0xe0 | currPDSet->reg15_1;
    as399xContinuousWrite(AS399X_REG_MODULATORCTRL, reg, 3);
    as399xWriteDAC(dac);

    /* Adapt PA bias voltage */
    if(outputPowerOn == 0)
    {
        as399xAntennaPower(1);
        as399xAdaptTxPower();
        as399xAntennaPower(0);
    }
    else
    {
        as399xAdaptTxPower();
    }

    return act * 10;
}

void as399xUpdateTxPower(u8 dBm, u8 reg15, u16 mV)
{
    //update output power settings
    dBm2Setting[dBm].reg15_1 = reg15;
    dBm2Setting[dBm].adcVoltage_mV = mV;
    //update output power hysteresis table
    if (dBm < (POWER_TABLE_SIZE - 1))
        dBm2SettingHysteresis[dBm] = (u16)((dBm2Setting[dBm+1].adcVoltage_mV - dBm2Setting[dBm].adcVoltage_mV) / 2)
                + dBm2Setting[dBm].adcVoltage_mV;
    else
        dBm2SettingHysteresis[dBm-1] = (u16)((dBm2Setting[dBm].adcVoltage_mV - dBm2Setting[dBm-1].adcVoltage_mV) / 2)
                + dBm2Setting[dBm-1].adcVoltage_mV;
}

void calcTxPowerHysteresis(void)
{
    u8 i;
    for( i=0; i<POWER_TABLE_SIZE-1; i++)
    {
        dBm2SettingHysteresis[i] = (u16)((dBm2Setting[i+1].adcVoltage_mV - dBm2Setting[i].adcVoltage_mV) / 2)
                + dBm2Setting[i].adcVoltage_mV;
    }
}

#define DELTA(A,B) (((A)>(B))?((A)-(B)):((B)-(A)))
void as399xAdaptTxPower(void)
{
    u16 mV, best_mV;
    s8 dir;
    u8 dac = as399xReadDAC();
    u8 improvement = 1;
    static u8 calcHysteresis = 1;

    if ( calcHysteresis )
    {
        calcTxPowerHysteresis();
        calcHysteresis = 0;
    }

    as399xSingleWrite(AS399X_REG_MEASURE_SELECTOR, 0x03); /* ADC pin */
#if ROLAND
    SWITCH_POST_PA(SWITCH_POST_PA_PD);
#endif
    //CON_print("desired %hx mV\n",currPDSet->adcVoltage_mV);

    if ( dac <= MIN_DAC ) dac = MIN_DAC + 1;
    if ( dac >= MAX_DAC ) dac = MAX_DAC - 1;

    //CON_print("dac = %hhx ",dac);
    as399xWriteDAC(dac);
    best_mV = as399xGetADC_mV();

    // check if we are inside a hysteresis area around the target value
    // hysteresis area is half the distance to next target value.
    if (currPDSetIndex > 1 && currPDSetIndex < (POWER_TABLE_SIZE - 1))
    {
//        CON_print("adapt tx power best_mV= %hx,  prev= %hx,  next= %x\n", best_mV,
//                dBm2SettingHysteresis[currPDSetIndex-1], dBm2SettingHysteresis[currPDSetIndex]);
        if ( best_mV > dBm2SettingHysteresis[currPDSetIndex-1] && best_mV < dBm2SettingHysteresis[currPDSetIndex])
            return;
    }

    if ( best_mV > currPDSet->adcVoltage_mV )
        dir = -1;
    else
        dir = 1;

    while ( improvement )
    {
        if ( dac == MIN_DAC ) break;
        if ( dac == MAX_DAC ) break;

        //CON_print("dac = %hhx ",dac + dir);
        as399xWriteDAC(dac + dir);
        mV = as399xGetADC_mV();

        if ( DELTA(mV,currPDSet->adcVoltage_mV)
                <= DELTA(best_mV,currPDSet->adcVoltage_mV))
        {
            best_mV = mV;
            dac += dir;
            if ( dac == 0x80 )
            	dac += dir;			/* DAC values 0x80 and 0x00 have the same output voltage (only differ in sign bit). In our nat representation this corresponds
            	 	 	 	 	 	 	 with the values 0x80 and 0x7F. Whenever we hit 0x80 we skip it by proceeding to the next value. */
        }else{
            improvement = 0;
        }
    }
#if ROLAND
    SWITCH_POST_PA(SWITCH_POST_PA_TUNING);
#endif
    //CON_print("best dac %hhx : %hx mV\n",dac,best_mV);
    as399xWriteDAC(dac);
}

void as399xCyclicPowerRegulation(void)
{
    if (outputPowerOn && !evalPowerRegulation)
    {
        if (outputPowerCounter)
        {
            outputPowerCounter--;
        }
        else
        {
            outputPowerCounter = outputPowerCounterStart;
            as399xAdaptTxPower();
        }
    }
}

void as399xInitCyclicPowerRegulation(u16 initCounter)
{
    //CON_print("counter: %hx \n", outputPowerCounterStart);
    outputPowerCounter = initCounter;
    outputPowerCounterStart = initCounter;
}

void as399xEvalPowerRegulation(u8 eval)
{
    evalPowerRegulation = eval;
}

#endif

void as399xEnterStandbyMode()
{
    u8 val;
    val = as399xSingleRead(AS399X_REG_STATUSCTRL);
    
    if (val & 0x80) /* already in standby */
        return;

    val |= 0x80;

    as399xSingleWrite(AS399X_REG_STATUSCTRL, val);
}
void as399xExitStandbyMode()
{
    u8 val;
    val = as399xSingleRead(AS399X_REG_STATUSCTRL);
    
    if (!(val & 0x80)) /* not in standby */
        return;

    val &= ~0x80;

    as399xSingleWrite(AS399X_REG_STATUSCTRL, val);

    mdelay(1);

    as399xLockPLL();
}


void as399xEnterPowerDownMode()
{
    u8 i;
    u8 reg3cnt = 0;
    int count;

#if ROLAND
    if (!(P4 & 0x02)) return;
#else
    if (!ENABLE) return;
#endif

    /* Switch off antenna */
    as399xPowerDownRegs[0] = as399xSingleRead(0);
    as399xSingleWrite(0, as399xPowerDownRegs[0] & (~0x03));
    for (i=1;i<AS399X_REG_ADC;i++)
    {
        if(  i==AS399X_REG_TESTSETTING   || i==AS399X_REG_CLSYSAOCPCTRL 
          || i==AS399X_REG_MODULATORCTRL || i==AS399X_REG_PLLMAIN 
          || i==AS399X_REG_PLLAUX )
        {
            as399xContinuousRead(i, 3, as399xPowerDownRegs3[reg3cnt++]);
        }
        else
        {
            as399xPowerDownRegs[i] = as399xSingleRead(i);
        }
    }
    /* Wait for antenna being switched off */
    count = 500;
    while(count-- && (as399xSingleRead(AS399X_REG_AGCINTERNALSTATUS) & 0x04))
    {
        mdelay(1);
    }
    EN(LOW);
}
void as399xExitPowerDownMode()
{
    u8 i;
    u8 reg3cnt = 0;

#if ROLAND
    if (P4 & 0x02) return;
#else
    if (ENABLE) return;
#endif

    EN(HIGH);
    mdelay(12);  /* AS3992 needs 12 ms to exit standby */
    reg3cnt = 0;
    /* Do not switch on antenna before PLL is locked.*/
    as399xSingleWrite(0, as399xPowerDownRegs[0] & (~0x03));
    for (i=1;i<AS399X_REG_ADC;i++)
    {
        if(  i==AS399X_REG_TESTSETTING   || i==AS399X_REG_CLSYSAOCPCTRL 
          || i==AS399X_REG_MODULATORCTRL || i==AS399X_REG_PLLMAIN 
          || i==AS399X_REG_PLLAUX )
        {
            as399xContinuousWrite(i, as399xPowerDownRegs3[reg3cnt++], 3);
        }
        else
        {
            as399xSingleWrite(i, as399xPowerDownRegs[i]);
        }
    }
    udelay(300);        /* without delay pll locking might fail --> spurs in spectrum */
    as399xLockPLL();
    as399xSingleWrite(0, as399xPowerDownRegs[0]);

}
