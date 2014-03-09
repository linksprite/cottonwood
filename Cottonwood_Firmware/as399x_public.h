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
 *  \author U.Herrmann ( based on work by E.Grubmueller
 *  \author T. Luecker (Substitute)
 *
 *   \brief Declaration of public functions provided by the AS399x series chips
 *
 *   Declares functions provided by the AS399x series chips. All higher level
 *   and protocol work is contained in gen2.h and iso6b.h. Register access is 
 *   performed using as399x.h.
 */

#ifndef _AS399X_PUBLIC_H_
#define _AS399X_PUBLIC_H_

#include "global.h"

/** Number of cycles the output power counter should count before re-adjusting output power.
 * This value should be used if as399xCyclicPowerRegulation() is called from main loop. */
#define OUTPUTPOWER_MAINCYCLES 48000
/** Number of cycles the output power counter should count before re-adjusting output power.
 * This value should be used if as399xCyclicPowerRegulation() is called from tuning functions. */
#define OUTPUTPOWER_TUNERCYCLES 5
/** Number of cycles the output power counter should count before re-adjusting output power.
 * This value should be used if as399xCyclicPowerRegulation() is called from inventory functions.
 * The cycle counter will count every query command. */
#define OUTPUTPOWER_QUERYCYCLES 10

/** @struct TagInfo_
  * This struct stores the whole information of one tag.
  *
  */
struct TagInfo_
{
    /** RN16 number. */
    u8 rn16[2];

    /** PC value. */
    u8 pc[2];
    /** EPC array. */
    u8 epc[EPCLENGTH];
    /** EPC length. */
    u8 epclen;  /*length in bytes */
    /** Handle for write and read communication with the Tag. */
    u8 handle[2];
    /** rssi which has been measured when reading this Tag. */
    u8 rssi;
};

/** Type definition struct: TagInfo_ is named Tag */
typedef struct TagInfo_ Tag;

/** @struct Frequencies_
  * This struct stores the list of frequencies which are used for hopping.
  * For tuning enabled boards the struct also stores the tuning settings for
  * each frequency.
  *
  */
struct Frequencies_
{
    /** Index of the active frequency in freq. */
    u8 activefreq;

    /** List of frequencies which are used for hopping. */
    unsigned long  freq[MAXFREQ];
    /** If rssi measurement is above this threshold the channel is regarded as
        used and the system will hop to the next frequency. Otherwise this frequency is used */
    s8    rssiThreshold[MAXFREQ];
#ifdef CONFIG_TUNER
    /** Counts how often this freq has been used in hopping. Only available on tuner enabled boards. */
    u8     countFreqHop[MAXFREQ];
#endif
};

typedef struct Frequencies_ Freq;

#ifdef CONFIG_TUNER
struct TuningTable_
{
    /** number of entries in the table */
    u8 tableSize;
    /** currently active entry in the table. */
    u8 currentEntry;
    /** frequency which is assigned to this tune parameters. */
    unsigned long freq[MAXTUNE];
    /** tune enable for antenna. Valid values: 1 = antenna 1, 2 = antenna 2, 3 = antenna 1+2 */
    u8      tuneEnable[MAXTUNE];
    /** Cin tuning parameter for antenna 1 + 2. */
    u8           cin[2][MAXTUNE];
    /** Clen tuning parameter for antenna 1 + 2. */
    u8          clen[2][MAXTUNE];
    /** Cout tuning parameter for antenna 1 + 2. */
    u8          cout[2][MAXTUNE];
    /** Reflected power which was measured after last tuning. */
    u16      tunedIQ[2][MAXTUNE];
};

typedef struct TuningTable_ TuningTable;
#endif

/** This function initialises the AS399x.
  * This function does not take or return a parameter
  *
  * @param baseFreq the base frequency to set at initialization time, may be 
  *        changed later on. If AS399X_DO_SELFTEST is set extensive selftests are
  *        performed.
  * @return 0    if everything went o.k.
  * @return 1    if a problem with the pins connecting the AS399X was detected
  *              (only when AS399X_DO_SELFTEST==1)
  * @return 2    if crystal is not stable
  *              (only when AS399X_DO_SELFTEST==1)
  * @return 3    if PLL did not lock
  *              (only when AS399X_DO_SELFTEST==1)
  * @return 0xd  if the reflected power was too low
  *              (only when AS399X_DO_SELFTEST==1)
  * @return 0xe  if the reflected power was too high
  *              (only when AS399X_DO_SELFTEST==1)
  * @return 0xf  if the wrong chip (AS3990/1 vs AS3992) was detected.
  */
u16 as399xInitialize(u32 baseFreq);

/** This function reads the version register of the AS399x and
  * returns the value. \n
  * @return Returns the value of the AS399x version register.
  */
unsigned char as399xReadChipVersion(void);

/*------------------------------------------------------------------------- */
/** This function brings the AS399x to Idlemode. \n
  * It does not take or return a parameter.
  */
void as399xSwitchToIdleMode(void);

/*!
 *****************************************************************************
 *  \brief  Set the link frequency
 *
 *  After calling this function the AS3991 is in normal mode again.
 * \param a
 *  0x0F: 640kHz
 *  0x0C: 320kHz
 *  0x09: 256kHz
 *  0x08: 213.3kHz (not available on AS3992)
 *  0x06: 160kHz
 *  0x03: 80kHz (not available on AS3992)
 *  0x00: 40kHz
 *
 *
 *****************************************************************************
 */
void as399xSelectLinkFrequency(u8 a);

/*------------------------------------------------------------------------- */
/** This function sets the frequency in the approbiate register
  * @param regs Which register is being used either AS399X_REG_PLLMAIN or AS399X_REG_PLLAUX
  * @param frequency frequency in kHz
  */
void as399xSetBaseFrequency(u8 regs, u32 frequency);

/*------------------------------------------------------------------------- */
/** This function dumps the AS399X registers 
  */
void as399xMemoryDump(void);

/*------------------------------------------------------------------------- */
/** This function turns the antenna power on or off. 
  * @param on boolean to value to turn it on (1) or off (0).
  */
void as399xAntennaPower( u8 on);

/*------------------------------------------------------------------------- */
/**  This function gets the RSSI (ReceivedSignalStrengthIndicator) of the
  *  environment in dBm. For measuring the antenna will be switched off, 
  *  int the time num_of_ms_to_scan measurements performed and the antenna
  *  if necessary turned back on.
  *  
  *  The returned value is the highest RSSI value expressed in dBm. -128 
  *  stands for no signal.
  *  
  *  Use this function to implement LBT (Listen Before Talk) at least for European
  *  based readers. Use like this
  *  \code
  *  as399xSetBaseFrequency( f + 100);
  *  as399xSaveSensitivity();
  *  as399xSetSensitivity(-50);
  *  as399xGetRSSI(10,&iq,&dBm)
  *  as399xRestoreSensitivity();
  *  if (dBm < -40) { 
  *     //use frequency f for operation
  *  }else{
  *     // try to find another freqency
  *  }
  *  \endcode
  */
void as399xGetRSSI( u16 num_of_ms_to_scan, u8 *rawIQ, s8 *dBm );

/*------------------------------------------------------------------------- */
/**  This function stores the current sensitivity registers. After that 
  *  as399xSetSensitivity can be called to change sensitivty for subsequent 
  *  as399xGetRSSI() calls. as399xRestoreSensitivity() restores it again.
  */
void as399xSaveSensitivity( );

/*------------------------------------------------------------------------- */
/**  This function restores the sensitivity previousley saved using
  *  as399xSaveSensitivity().
  */
void as399xRestoreSensitivity( );

/*------------------------------------------------------------------------- */
/**  This function tries to set the sensitivity to a level to allow detection 
  *  of signals using as399xGetRSSI() with a level higher than miniumSignal(dBm).
  *  \return the level offset. Sensitivity was set to minimumSignal - (returned_val).
  *  negative values mean the sensitivity could not be reached.
  */
s8 as399xSetSensitivity( s8 minimumSignal );

/*------------------------------------------------------------------------- */
/**  This function gets the current rx sensitivity level.
  */
s8 as399xGetSensitivity( void );

/*------------------------------------------------------------------------- */
/** This function gets the values for the reflected power. For measuring the 
  * antenna will be switched on, a measurement performed and the antenna
  * if necessary turned back off. The two measured 8-bit values are returned as
  * one 16-bit value, the lower byte containing receiver-A (I) value, the 
  * higher byte containing receiver-B (Q) value. The two values are signed 
  * values and will be converted from sign magnitude representation to natural.
  * Note: To get a more accurate result the I and Q values of
  * as399xGetReflectedPowerNoiseLevel() should be subtracted from the result of
  * as399xGetReflectedPower()
  */
u16 as399xGetReflectedPower( void );

/*------------------------------------------------------------------------- */
/**
  * This function measures and returns the noise level of the reflected power
  * measurement of the active antenna. The returned value is of type u16 and
  * contains the 8 bit I and Q values of the reflected power. The I value is
  * located in the lower 8 bits, the Q value in the upper 8 bits. \n
  * See as399xGetReflectedPower() fore more info.
  */
u16 as399xGetReflectedPowerNoiseLevel( void );

/*------------------------------------------------------------------------- */
/** This function reads in the current register settings (configuration only), 
  * performs a reset by driving the enable bit first low, then high and then 
  * writes back the register values.
  */
void as399xReset(void);

#ifdef POWER_DETECTOR
/*------------------------------------------------------------------------- */
/** This function reads in from external ADC pin and tries to reach the desired
  * tx power in cBms starting from very low value. AS399x tx_lev bits and PA
  * bias are used for this.
  * Antenna must not be switched on when calling this function.
  */
s16 as399xSetTxPower(s16 desired_dBm);

/** This function updates the table dBm2Setting. This table should only be modified
 * with this function because dBm2SettingHysteresis has also to be modified.
 * @param dBm   index in table dBm2Setting which will be updated.
 * @param reg15 new reg15 value for dBm2Setting
 * @param mV    new mV value for dBm2Setting
 */
void as399xUpdateTxPower(u8 dBm, u8 reg15, u16 mV);
/*------------------------------------------------------------------------- */
/** This function reads in from external ADC pin and tries to reach the desired
  * tx power set by as399xSetTxPower(). Only PA bias is adapted.
  * Antenna be switched on when calling this function.
  */
void as399xAdaptTxPower(void);

/*------------------------------------------------------------------------- */
/** This function should be called regularly to re-adjust output power.
 * This is necessary because after switching on output power the PA starts heating up
 * and the output power increases because of that. (PA output rises over temperature.)
 * Therefore it is necessary to re-adjust the DAC which controls the PA bias.
 * For re-adjustment as399xAdaptTxPower() is used.
 */
void as399xCyclicPowerRegulation(void);

/*------------------------------------------------------------------------- */
/** This function defines after how many cyclic calls to as399xCyclicPowerRegulation()
 * an output power re-adjustement should be performed. The number of cycles can be
 * different because eg: the main idle loop calls the function much more often than
 * the automatic tuning functions.
 * There are predefined values for those cases: #OUTPUTPOWER_MAINCYCLES,
 * #OUTPUTPOWER_INVENTORYCYCLES and #OUTPUTPOWER_TUNERCYCLES
 * @param initCounter Number of cycles after an output power re-adjustement is performed.
 */
void as399xInitCyclicPowerRegulation(u16 initCounter);

/*------------------------------------------------------------------------- */
/** This function allows to disable the cyclic power regulation for evaluation. Per default the
 * cyclic regulation is enabled. It can be disabled for diagnostic/evaluation purposes
 * but should be used with care!
 * @param 1: disable regulation (for evaluation), 0: enable regulation (normal operation).
 */
void as399xEvalPowerRegulation(u8 enable);

#endif /* POWER_DETECTOR */
#endif /* _AS399X_PUBLIC_H_ */
