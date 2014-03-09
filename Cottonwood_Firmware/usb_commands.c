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
  * @brief Functions which handle commands received via USB(HID) or UART.
  *
  * This file contains all functions for processing commands received via either HID or UART.
  * It implements the parsing of reports data and sending data back.
  * \n
  * The frequency hopping is also done in this file before calling protocol/device
  * specific functions.
  *
  * @author Ulrich Herrmann
  * @author Bernhard Breinbauer
  * @author Wolfgang Reichart
  */
#include "c8051F340.h"
#include "as399x_config.h"
#include "platform.h"
#include "as399x.h"
#include "global.h"
#include "gen2.h"
#include "as399x_public.h"
#include "as399x_com.h"
#include "F3xx_Blink_Control.h"
#include "F3xx_USB0_ReportHandler.h"
#include "F3xx_USB0_InterruptServiceRoutine.h"
#include "usb_commands.h"
#include "uart.h"
#include "iso6b.h"
#include "timer.h"
#include "string.h"
#ifdef CONFIG_TUNER
#include "tuner.h"
#endif
#include "F340_FlashPrimitives.h"

#define USBCOMMDEBUG            0

#define SESSION_GEN2            1
#define SESSION_ISO6B           2

#define UART_IDLE               0x02
#define UART_RECEIVE            0x03
#define UART_PROCESS            0x04

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


/* default configuration, may be overwritten */
static struct gen2Config gen2Configuration = {GEN2_LF_320, GEN2_COD_MILLER4, GEN2_IINV_S0, 0, 1};
u8 gen2qbegin = 4;

unsigned num_of_tags;
#ifdef POWER_DETECTOR
s16 desTxPower = 240; /* 24 dBm = 240 dBm */
s16 actTxPower = -128;
u8 usedAntenna = SWITCH_ANT_P1;
#endif

/** Array of Structures which stores all necessary Information about the Tags.
 */
XDATA Tag tags_[MAXTAG];

/** Mask for the read and write command */
XDATA u8 mask[EPCLENGTH];

Tag *selectedTag;

Freq Frequencies;
#ifdef CONFIG_TUNER
TuningTable tuningTable;
#endif

static u8 guiActiveProfile = 1, guiNumFreqs=4;
static u32 guiMinFreq = 865700;
static u32 guiMaxFreq = 867500;

static u16 idleTime = 0, maxSendingTime = 10000, listeningTime = 0;
static u16 maxSendingLimit_slowTicks;
static u8 timedOut;
static u8 dontResetUSBReceiverFlag;
static u8 cyclic = 0;
static u8 cyclicInventStart;

#if UARTSUPPORT
static u8 uartState=UART_IDLE;
static u8 uartFlag;

#endif

#if UARTSUPPORT
#define SendPacket( A ) uartSendPacket()
void uartSendPacket( )
{
    u8 i;

    for (i=0;i<IN_PACKET[1];i++)      /* and send it back */
        sendByte(IN_PACKET[i]); 
}
#endif

u16 currentFreqIdx;

u8 currentSession;

void sendFirmwareHardwareID(void);
void SendDeviceInfo(void);
void resetCPU(void);
void antennaPower(void);
void writeRegister(void);
void readRegister(void);
void readRegisters(void);
void inventory(void);
void inventoryRSSI(u8 startInvent);
void wrongCommand(void);
void initCommands(void);

void selectTag(void);
void writeToTag(void);
void writeToTag6B(void);
void readFromTag6B(void);
void readDataFromTag(void);
void lockUnlock(void);
void usb_gen2KillTag(void);
void NXPCommands(void);
void genericCommand(void);

static void hopChannelRelease(void);
static s8 hopFrequencies(void);

static bool continueCheckTimeout( ) 
{
    if (maxSendingLimit_slowTicks == 0) return 1;
    if ( timerMeasure_slowTicks() > maxSendingLimit_slowTicks )
    {
#if USBCOMMDEBUG
        CON_print("allocation timeout\n");
#endif
        timedOut = 1;
        return 0;
    }
    return 1;
}

/* This function checks the current session, if necessary closes it
and opens a new session */
static void checkAndSetSession( u8 newSession )
{
    if (currentSession == newSession) return;
    switch (currentSession)
    {
        case SESSION_GEN2:
            gen2Close();
            break;
        case SESSION_ISO6B:
#if ISO6B
            iso6bClose();
#endif
            break;
    }
    switch (newSession)
    {
        case SESSION_GEN2:
            gen2Open(&gen2Configuration);
            break;
        case SESSION_ISO6B:
#if ISO6B
            iso6bOpen();
#endif
            break;
    }
    currentSession = newSession;
}

/* assume we have an already selected tag */
static u8 checkAndAccessTag( u8 * password )
{
	u8 status = 0;
#if USBCOMMDEBUG
	    CON_print("checkAndAccessTag\n");
	    CON_hexdump(password, 4);
#endif

	if (      (password[0] == 0)
			&&(password[1] == 0)
	        &&(password[2] == 0)
	        &&(password[3] == 0)
	    ) /* password not set */
	{
#if USBCOMMDEBUG
	    CON_print("no need to access Tag\n");
#endif
	}
	else
	{
	    status = gen2AccessTag(selectedTag, password);
	    if ( status != 0 )
	    { /* For some reason it is normal to fail here */
#if USBCOMMDEBUG
	        CON_print("failed to access tag %hhx\n",status);
#endif
	    }
	}
	return status;
}


/*------------------------------------------------------------------------- */
/*This function sends an error message by UART if a wrong write command is */
/*received */
void callWrongCommand(void)
{
    wrongCommand();
}

void wrongCommand(void)
{

#if USBCOMMDEBUG
    CON_print("WRONG COMMAND\n");
#endif
}

/*------------------------------------------------------------------------- */
/*!This function sends the firmware/hardware ID to the host.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>2</th></tr>
    <tr><th>Content</th><td>0x10(ID)</td><td>3(length)</td><td>0 for firmware<br>1 for hardware</td></tr>
  </table>
  The device sends back:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>           1</th><th>2 .. 47</th></tr>
    <tr><th>Content</th><td>0x11(ID)</td><td>length</td><td>Zero terminated string</td></tr>
 */
void callSendFirmwareHardwareID(void)
{
    sendFirmwareHardwareID();
}

void sendFirmwareHardwareID(void)
{
    char *ID = 0;
    u8 count = 0;

#if RUN_ON_AS3992
    if(as399xChipVersion == AS3991) 
#else
    if(as399xChipVersion == AS3992) 
#endif
    {
            ID = WRONG_CHIP_ID;
    }
    else switch (getBuffer_[2])
    {
        case FIRMWARE:
            ID = FIRMWARE_ID;
            break;
        case HARDWARE:
            ID = HARDWARE_ID;
            break;
        default:
            ID = "Wrong version type send";
    }

#if USBCOMMDEBUG
    CON_print(ID);
#endif

    /*reset the buffer */
    for (count = 2; count<=IN_FIRM_HARDW_IDSize+2; count++)
    {
        IN_PACKET[count] = 0;
    }

    IN_PACKET[0] = IN_FIRM_HARDW_ID;               /*set report id */
    IN_PACKET[1] = stringLength(ID) + 1;            /*calculate the length of the complete command */
    copyBuffer(ID, &IN_PACKET[2], IN_PACKET[1]-2);  /*copying the string */
    IN_BUFFER.Ptr = IN_PACKET;                      /*set the IN_BUFFER */
    IN_BUFFER.Length = IN_FIRM_HARDW_IDSize+1;/*IN_BUFFER.Ptr[1];            set IN_BUFFER length */

    SendPacket(IN_FIRM_HARDW_ID);

}

#define POWER_AND_SELECT_TAG() do{ status = hopFrequencies(); if (status) goto exit; powerAndSelectTag(); if(num_of_tags==0){status=GEN2_ERR_SELECT;goto exit;}}while(0)

static int powerAndSelectTag( void )
{
    if (selectedTag == 0) return 0;
    num_of_tags = gen2SearchForTags(tags_+1,1, (*selectedTag).epc,(*selectedTag).epclen,0,continueCheckTimeout,1); /* To request the handle for writing.... */
    {
        u8 i, allZero = 1;
        for ( i = 0; i < selectedTag->epclen; i++ )
        {
            if (selectedTag->epc[i] != 0)
                allZero = 0;
        }
        if (num_of_tags == 0 && allZero)
        {
            num_of_tags = gen2SearchForTags(tags_+1,1, (*selectedTag).epc,(*selectedTag).epclen,gen2qbegin,continueCheckTimeout,0); /* mask, masklength, q */
        }
    }
    if (num_of_tags == 0)
    {
#if USBCOMMDEBUG
        CON_print("Could not select tag\n");
#endif
    }
    else
    {
        selectedTag = tags_+1;
    }
//    return num_of_tags;
}

#if 0
/*!
 * This function allows to add 1dB power calibration steps to a current register 0x15 value.
 * It takes care of under/overflow of the 1dB steps in the register.
 * @param reg15 The current register 0x15 value which should be adapted.
 * @param calib The number of dB steps which should be added/subtracted.
 */
static void addOutputPowerCalibration( u8 *reg15, s8 calib )
{
    s8 fine;
    u8 coarse;

    fine = (*reg15) & 0x07;
    coarse = ((*reg15) & 0x18) >> 3;
    calib = 0 - calib; /* +1 in calib means we want to add 1dB to output power, but to add
    1dB output power we have to subtract 1 from the fine adjustment bits. */

    fine = fine + calib;
    if (fine > 7)
    {   // overflow in the fine adjustment bits
        if (coarse < 2)
        {
            coarse++;
            fine -= 6;                  //correct for overflow
        }
        else
        {
            fine = 7;                   //maximum reached
        }
    }
    else if (fine < 0)
    {   //underflow in the fine adjustment bits
        if (coarse > 0)
        {
            coarse--;
            fine += 6;                  // correct for underflow
        }
        else
        {
            fine = 0;                   // minimum reached
        }
    }
    *reg15 = (*reg15 & 0xe0) | (fine & 0x07) | ((coarse & 0x03) << 3 );
}
#endif

/*!This function updates the antenna output power table (dBm2Setting).
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>         2</th><th>   3</th><th> 4 </th><th> 5 </th><th> 6 </th><th> 7 </th></tr>
    <tr><th>Content</th><td>0x14(ID)</td><td>number of bytes in report</td><td>x dBm</td><td> Reg15 </td><td> mV LSB </td><td> mV MSB </td><td>x dBm</td><th> ... </th></tr>
  </table>
  One power table entry consists of 4 Bytes:\n
  x dBm: applying the following values will set this output power level.\n
  Reg15: value for the TX output level bits (4:0) in register 0x15 to get the x dBm output level.\n
  mV: 16bit value which defines the voltage the DAC should be regulated to, to get the x dBm output level.\n
  For updating the whole antenna output power table (31 table entries) 3 reports have to be sent.
  Each report contains the entry of the table which should be modified (x dBm) and the
  new values (Reg15, mV).\n
  The device sends back:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>2</th></tr>
    <tr><th>Content</th><td>0x15(ID)</td><td>3(length)</td><td>0 on success</td></tr>
  </table>
 */
void callUpdateAntennaPower(void)
{
#ifdef POWER_DETECTOR
    u8 i, a, length, number, remainder;
    u8 reg15, dBm;
    u16 mV;

    length = getBuffer_[1];     //length of report (number of bytes)
    number = (length - 2) / 4;  //number of power table entries in the report
    remainder = (length - 2) % 4;

#if USBCOMMDEBUG
    CON_print("update power length= %hhx, number= %hhx remainder=%hhx\n", length, number, remainder);
#endif
    if (remainder == 0)         //only if number of table entries is ok we will use them
    {
        for (i=0; i<number; i++)
        {
            a = i * 4;
            dBm = getBuffer_[2+a];
            reg15 = getBuffer_[3+a];
            mV = getBuffer_[4+a] | (getBuffer_[5+a] << 8);
            if (dBm > POWER_TABLE_SIZE)
                continue;       //skip invalid data
            //CON_print("update power dBm= %hhx,  reg15= %hhx,  mV= %x\n", dBm, reg15, mV);
            as399xUpdateTxPower(dBm, reg15, mV);
        }
    as399xSetTxPower(actTxPower);       // apply new values
    IN_PACKET[2] = 0;           // success
    }
    else
        IN_PACKET[2] = 1;       // failed
#else
    IN_PACKET[2] = 2;           //not supported
#endif

    IN_PACKET[0] = IN_UPDATE_POWER_ID;
    IN_PACKET[1] = IN_UPDATE_POWER_IDSize+1;
    IN_BUFFER.Length = IN_UPDATE_POWER_IDSize+1;
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_UPDATE_POWER_ID);
}


/*!This function enables/disables the RF field.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th>
        <th> 0</th>
        <th> 1</th>
        <th> 2</th>
        <th> 3</th>
        <th> 4</th>
        <th> 5</th>
        <th> 6</th>
        <th> 7</th>
        <th> 8</th>
        <th> 9</th>
        <th>10</th>
        <th>11</th>
        <th>12</th>
        <th>13</th>
    </tr>
    <tr><th>Content</th>
        <td>0x18(ID)</td>
        <td>13(length)</td>
        <td>set_power</td>
        <td>0x00: power off<br>0xFF: power on</td>
        <td>eval_mode</td>
        <td>set_sensitivity</td>
        <td>sensitivity</td>
        <td>set_port</td>
        <td>port id</td>
        <td>set_level</td>
        <td>Transmit Power Level LSB</td>
        <td>Transmit Power Level MSB</td>
        <td>set_post_pa_switch</td>
        <td>post_pa_switch</td>
    </tr>
  </table>
  The parameters are only being set if the proper set_ value is set to 1.\n
  If eval_mode is set the automatic power regulation will be disabled. eval_mode will be only set if set_power_enable is set to 1.\n
  The bytes 6 to 12 are only available on ROLAND and ARNIE boards.<br>
  The device sends back:
  <table>
    <tr><th>   Byte</th>
        <th> 0</th>
        <th> 1</th>
        <th> 2</th>
        <th> 3</th>
        <th> 4</th>
        <th> 5</th>
        <th> 6</th>
        <th> 7</th>
        <th> 8</th>
        <th> 9</th>
        <th>10</th>
        <th>11</th>
        <th>12</th>
        <th>13</th>
    </tr>
    <tr><th>Content</th>
        <td>0x19(ID)</td>
        <td>13(length)</td>
        <td>reserved(0)</td>
        <td>0 on success</td>
        <td>reserved(0)</td>
        <td>reserved(0)</td>
        <td>sensitivity</td>
        <td>reserved (0)</td>
        <td>antenna id</td>
        <td>reserved (0)</td>
        <td>desired Transmit Power LSB</td>
        <td>desired Transmit Power MSB</td>
        <td>current Transmit Power LSB</td>
        <td>current Transmit Power MSB</td>
    </tr>
    </table>

  Also in the reply of the device the bytes 4 to 12 are only available on ROLAND and ARNIE boards.<br>
  Values for the different parameters are:
  <table>
    <tr><th>Name</th></th><th>values</th></tr>
    <tr><td>sensitivity</td><td>-128 .. 127 (dBm)
                       </td></tr>
    <tr><td>antenna id</td><td>1: Antenna port 1<br>
                               2: Antenna port 2
                       </td></tr>
    <tr><td>Transmit Power</td><td>The desired transmit power which should be set.
                       </td></tr>
    <tr><td>post_pa_switch</td><td>Switch just after PA, switches between normal operation
                                    and power detector.
                       </td></tr>
     </table>
 */
void callAntennaPower(void)
{
    antennaPower();
}

void antennaPower(void)
{
#if USBCOMMDEBUG
    CON_print("antennaPower\n");
#endif
    // enable antenna and eval_mode
    if (getBuffer_[2])
    {
        if (getBuffer_[3] == ANT_POWER_ON)
        {
#if USBCOMMDEBUG
            CON_print("ANTENNA ON, eval: %hhx\n", getBuffer_[4]);
#endif
#ifdef POWER_DETECTOR
            as399xInitCyclicPowerRegulation(OUTPUTPOWER_MAINCYCLES);
            as399xEvalPowerRegulation( getBuffer_[4] );
#endif
            as399xAntennaPower(1);
        }
        else
        {
#if USBCOMMDEBUG
            CON_print("ANTENNA OFF, eval: %hhx\n", getBuffer_[4]);
#endif
            as399xAntennaPower(0);
#ifdef POWER_DETECTOR
            as399xEvalPowerRegulation( getBuffer_[4] );
#endif
        }
    }
    // set antenna switch, etc...
    if (getBuffer_[5]) as399xSetSensitivity(getBuffer_[6]);
#ifdef ANTENNA_SWITCH
    if (getBuffer_[7])
    {
        usedAntenna = getBuffer_[8];
        if (usedAntenna > 2)        //prevent mis-configuration of antenna switch.
            usedAntenna = 2;
        if (usedAntenna == 0)
            usedAntenna = 1;
        SWITCH_ANTENNA(usedAntenna);
    }
#endif
#ifdef POWER_DETECTOR
    if (getBuffer_[9])
    {
        desTxPower                = getBuffer_[10] | (getBuffer_[11]<<8);
        desTxPower = as399xSetTxPower(desTxPower); /* limit to reachable values */
        actTxPower = desTxPower; /* not implemented */
    }
#if ROLAND
    if (getBuffer_[12])
    {
        SWITCH_POST_PA((getBuffer_[13]<<4)&0x30);
    }
#endif
#endif

    //prepare response
    IN_PACKET[0] = IN_ANTENNA_POWER_ID;
    IN_PACKET[1] = IN_ANTENNA_POWER_IDSize+1;     
    IN_PACKET[2] = 0;
    IN_PACKET[3] = 0;   //success
    IN_PACKET[4] = 0;
    IN_PACKET[5] = 0;
    IN_PACKET[6] = as399xGetSensitivity();
#ifdef ANTENNA_SWITCH
    IN_PACKET[7] = 0;
    IN_PACKET[8]= usedAntenna;
#endif
#ifdef POWER_DETECTOR
    IN_PACKET[9] = 0;
    IN_PACKET[10]= desTxPower & 0xff;
    IN_PACKET[11]= desTxPower >> 8;
    IN_PACKET[12]= actTxPower & 0xff;
    IN_PACKET[13]= actTxPower >> 8;
#endif

    IN_BUFFER.Length =IN_ANTENNA_POWER_IDSize+1;
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_ANTENNA_POWER_ID);
}

void callWriteRegister(void)
{
    writeRegister();
}

/*!This function writes one register on the AS399x.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>         2</th><th>   3</th><th>   4</th><th>   5</th></tr>
    <tr><th>Content</th><td>0x1a(ID)</td><td>6(length)</td><td>reg_number</td><td>val0</td><td>val1</td><td>val2</td></tr>
  </table>
  val1 and val2 are only used if reg_number is a deep register (0x12,0x14 .. 0x17)
  if reg_number > 0x80, then an immediate command is executed.
  The device sends back:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>2</th></tr>
    <tr><th>Content</th><td>0x1b(ID)</td><td>3(length)</td><td>0 on success</td></tr>
  </table>
 */
void writeRegister(void)
{
    u8 i;
#if USBCOMMDEBUG
    CON_print("WRITE\n");
#endif

    i=getBuffer_[2];
    if (i==0x12 || i==0x14 || i==0x15 || i==0x16 || i==0x17 )
    {
        as399xContinuousWrite(getBuffer_[2], &getBuffer_[3], 3);
    }
    else if (i < 0x80)
    {
        as399xSingleWrite(getBuffer_[2], getBuffer_[3]);
    }
    else
    {
        as399xSingleCommand(i);
    }
    IN_PACKET[0] = IN_WRITE_REG_ID;
    IN_PACKET[1] = IN_WRITE_REG_IDSize+1;
    IN_PACKET[2] = 0;
    IN_BUFFER.Length = IN_BUFFER.Ptr[1];
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_WRITE_REG_ID);
}

/*!This function reads one register from the AS399x.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>         2</th></tr>
    <tr><th>Content</th><td>0x1c(ID)</td><td>2(length)</td><td>reg_number</td></tr>
  </table>
  The device sends back:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>           2</th><th>   3</th><th>   4</th><th>   5</th></tr>
    <tr><th>Content</th><td>0x1d(ID)</td><td>6(length)</td><td>0 on success</td><td>val0</td><td>val1</td><td>val2</td></tr>
  </table>
  val1 and val2 only contain values if reg_number is a deep register (0x12,0x14 .. 0x17).
 */
void callReadRegister(void)
{
    readRegister();
}

void readRegister(void)
{
    u8 databuf[3];

#if USBCOMMDEBUG
    CON_print("READ\n");

#endif

    IN_PACKET[0] = IN_READ_REG_ID;

    if (getBuffer_[2]==AS399X_REG_TESTSETTING  || getBuffer_[2]==AS399X_REG_CLSYSAOCPCTRL  || getBuffer_[2]==AS399X_REG_MODULATORCTRL || getBuffer_[2]==AS399X_REG_PLLMAIN || getBuffer_[2]==AS399X_REG_PLLAUX)
    {
        as399xContinuousRead(getBuffer_[2], 3, databuf);
        copyBuffer(databuf, &IN_PACKET[2], 3);
    }
    else
    {
        databuf[0] = as399xSingleRead(getBuffer_[2]);
        copyBuffer(databuf, &IN_PACKET[2], 1);
        IN_PACKET[3]=0;
        IN_PACKET[4]=0;
    }

    IN_PACKET[0] = IN_READ_REG_ID;
    IN_PACKET[1] = IN_READ_REG_IDSize+1;
    IN_BUFFER.Length =IN_READ_REG_IDSize+1;
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_READ_REG_ID);
}

/*! This function sets and reads various gen2 related settings.
  The format of the report from the host is as follows:
  <table>
    <tr>
        <th>Byte</th>
        <th>0</th>
        <th>1</th>
        <th>2</th>
        <th>3</th>
        <th>4</th>
        <th>5</th>
        <th>6</th>
        <th>7</th>
        <th>8</th>
        <th>9</th>
        <th>10</th>
        <th>11</th>
        <th>12</th>
        <th>13</th>
    </tr>
    <tr>
        <th>Content</th>
        <td>0x59(ID)</td>
        <td>length</td>
        <td>set_lf</td>
        <td>lf</td>
        <td>set_coding</td>
        <td>coding</td>
        <td>set_session</td>
        <td>session</td>
        <td>set_trext</td>
        <td>trext</td>
        <td>set_tari</td>
        <td>tari</td>
        <td>set_qbegin</td>
        <td>qbegin</td>
    </tr>
  </table>
  The values are only being set if the proper set_ value is set to 1.<br>
  The device sends back:
  <table>
    <tr>
        <th>Byte</th>
        <th>0</th>
        <th>1</th>
        <th>2</th>
        <th>3</th>
        <th>4</th>
        <th>5</th>
        <th>6</th>
        <th>7</th>
        <th>8</th>
        <th>9</th>
        <th>10</th>
        <th>11</th>
        <th>12</th>
        <th>13</th>
    </tr>
    <tr>
        <th>Content</th>
        <td>0x5a(ID)</td>
        <td>0x40(length)</td>
        <td>reserved(0)</td>
        <td>lf</td>
        <td>reserved(0)</td>
        <td>coding</td>
        <td>reserved(0)</td>
        <td>session</td>
        <td>reserved(0)</td>
        <td>trext</td>
        <td>reserved(0)</td>
        <td>tari</td>
        <td>reserved(0)</td>
        <td>qbegin</td>
    </tr>
  </table>
  Values for the different parameters are:
  <table>
    <tr><th>Name</th></th><th>values</th></tr>
    <tr><td>lf</td><td> 0 = 40 kHz,<br>
                        3 = 80 kHz not AS3992,<br>
                        6 = 160 kHz,<br>
                        8 = 213 kHz,<br>
                        9 = 256 kHz,<br>
                       12 = 320 kHz,<br>
                       15 = 640 kHz
                       </td></tr>
    <tr><td>coding</td><td>0 = FM0,<br>
                           1 = Miller2,<br>
                           2 = Miller4,<br>
                           3 = Miller8
                       </td></tr>
    <tr><td>session</td><td>0 = S0,<br>
                            1 = S1,<br>
                            2 = S2,<br>
                            3 = SL
                       </td></tr>
    <tr><td>trext</td><td>0 = short preamble, pilot tone,<br>
                          1 = long preamble, pilot tone
                       </td></tr>
    <tr><td>tari</td><td>0 = 6.25 us,<br>
                         1 = 12.5 us,<br>
                         2 = 25 us
                       </td></tr>
    <tr><td>qbegin</td><td>0 .. 15. Initial gen2 round is 2^qbegin long. Please be careful with higher values.
                       </td></tr>
    </table>
 */
void configGen2()
{
#if USBCOMMDEBUG
    CON_print("configGen2\n");
#endif
    if (currentSession == SESSION_GEN2) currentSession = 0;

    if (getBuffer_[2]) gen2Configuration.linkFreq = getBuffer_[3];
    if (getBuffer_[4]) gen2Configuration.miller   = getBuffer_[5];
    if (getBuffer_[6]) gen2Configuration.session  = getBuffer_[7];
    if (getBuffer_[8]) gen2Configuration.trext    = getBuffer_[9];
    if (getBuffer_[10]) gen2Configuration.tari    = getBuffer_[11];
    if (getBuffer_[12]) gen2qbegin                = getBuffer_[13];

    memset(IN_PACKET,0,IN_GEN2_SETTINGS_IDSize+1);

    IN_PACKET[0] = IN_GEN2_SETTINGS_ID;
    IN_PACKET[1] = IN_GEN2_SETTINGS_IDSize+1;

    IN_PACKET[3] = gen2Configuration.linkFreq;
    IN_PACKET[5] = gen2Configuration.miller;
    IN_PACKET[7] = gen2Configuration.session;
    IN_PACKET[9] = gen2Configuration.trext;
    IN_PACKET[11] = gen2Configuration.tari;
    IN_PACKET[13]= gen2qbegin;

    IN_BUFFER.Length =IN_GEN2_SETTINGS_IDSize+1;
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_GEN2_SETTINGS_ID);
}

void callConfigGen2(void)
{
    configGen2();
}
#ifdef CONFIG_TUNER
struct tunerParams antennaParams = {0};
#endif

#ifdef CONFIG_TUNER
/**
 * adds data in current USB buffer to tuning table, should be only called from callAntennaTuner
 */
static u8 addToTuningTable(void)
{
    u8 idx;
    u16 iq;
    u32 freq;
    freq = 0;
    freq += (long)getBuffer_[3];
    freq += ((long)getBuffer_[4]) <<8;
    freq += ((long)getBuffer_[5]) << 16;

    idx = tuningTable.tableSize;
    if (idx > MAXTUNE)
        idx = MAXTUNE;
    tuningTable.freq[idx] = freq;
    tuningTable.tuneEnable[idx] = 0;
    if (getBuffer_[6] > 0)
    {
        //set antenna 1
        tuningTable.tuneEnable[idx] += 1;
        tuningTable.cin[0][idx] = getBuffer_[7];
        tuningTable.clen[0][idx] = getBuffer_[8];
        tuningTable.cout[0][idx] = getBuffer_[9];
        iq = (u16)getBuffer_[10];
        iq += ((u16)getBuffer_[11]) << 8;
        tuningTable.tunedIQ[0][idx] = iq;
    }
    if (getBuffer_[12] > 0)
    {
        //set antenna 2
        tuningTable.tuneEnable[idx] += 2;
        tuningTable.cin[1][idx] = getBuffer_[13];
        tuningTable.clen[1][idx] = getBuffer_[14];
        tuningTable.cout[1][idx] = getBuffer_[15];
        iq = (u16)getBuffer_[16];
        iq += ((u16)getBuffer_[17]) << 8;
        tuningTable.tunedIQ[1][idx] = iq;
    }
    if (tuningTable.tuneEnable[idx] > 0)    //if this tuning entry is used adjust size of table.
        tuningTable.tableSize++;
#if USBCOMMDEBUG
    CON_print("add tunetable f=%x%x, tablesize=%hhx, tune1=%hhx cin1=%hhx clen1=%hhx cout1=%hhx iq1=%hx "
               "tune2=%hhx cin2=%hhx clen2=%hhx cout2=%hhx iq2=%hx\n", freq, tuningTable.tableSize,
               getBuffer_[6],getBuffer_[7],getBuffer_[8],getBuffer_[9],getBuffer_[10],getBuffer_[11],
               getBuffer_[12],getBuffer_[13],getBuffer_[14],getBuffer_[15],getBuffer_[16],getBuffer_[17]);
#endif
    if (tuningTable.tableSize >= MAXTUNE)
    {
        tuningTable.tableSize = MAXTUNE;
        return MAXTUNE + 1;
    }
    else
        return tuningTable.tableSize;
}
#endif

/*!This function sets and reads antenna tuner network related values.
  The network looks like this:

  \code
                         .--- L ---.
                         |        |
  in ----.----.----- L --.--C_len--.--.-----.-----.----- out
         |    |                      |     |     |
        C_in  L                     C_out   L     R
         |    |                      |     |     |
        ___  ___                    ___   ___   ___
         -    -                      -     -     -
         '    '                      '     '     '
  \endcode
  where 
  <ul>
  <li> C_* = 32 steps from 1.3pF to 5.5pF, step size 0.131pF, +-10% </li>
  </ul>
  The format of the report from the host is one of the following:
  <ul>
  <li>Get/Set current tuner parameters:
  <table>
    <tr>
        <th>Byte</th>
        <th>0</th>
        <th>1</th>
        <th>2</th>
        <th>3</th>
        <th>4</th>
        <th>5</th>
        <th>6</th>
        <th>7</th>
        <th>8</th>
        <th>9</th>
    </tr>
    <tr>
        <th>Content</th>
        <td>0x5b(ID)</td>
        <td>length</td>
        <td>1</td>
        <td>set_cin</td>
        <td>cin</td>
        <td>set_clen</td>
        <td>clen</td>
        <td>set_cout</td>
        <td>cout</td>
        <td>auto_tune</td>
    </tr>
  </table>
  The values are only being set if the proper set_ value is set to 1.
  The device sends back:
  <table>
    <tr>
        <th>Byte</th>
        <th>0</th>
        <th>1</th>
        <th>2</th>
        <th>3</th>
        <th>4</th>
        <th>5</th>
        <th>6</th>
        <th>7</th>
        <th>8</th>
    </tr>
    <tr>
        <th>Content</th>
        <td>0x5c(ID)</td>
        <td>0x40(length)</td>
        <td>1</td>
        <td>status</td>
        <td>cin</td>
        <td>reserved(0)</td>
        <td>clen</td>
        <td>reserved(0)</td>
        <td>cout</td>
    </tr>
  </table>
  Values for c* paramaters range from 0 to 31, c=1.3pF + val*0.131pF.<br>
  </li>
  <li>Delete current tuning table:
  <table>
    <tr>
        <th>Byte</th>
        <th>0</th>
        <th>1</th>
        <th>2</th>
    </tr>
    <tr>
        <th>Content</th>
        <td>0x5b(ID)</td>
        <td>length</td>
        <td>2</td>
    </tr>
  </table>
  The device sends back:
  <table>
    <tr>
        <th>Byte</th>
        <th>0</th>
        <th>1</th>
        <th>2</th>
        <th>3</th>
        <th>4</th>
    </tr>
    <tr>
        <th>Content</th>
        <td>0x5c(ID)</td>
        <td>0x40(length)</td>
        <td>2</td>
        <td>0xFF on success</td>
        <td>maximum tuning table size this device supports</td>
    </tr>
  </table>
  </li>
  <li>Add new entry in tuning table:
  <table>
    <tr>
        <th>Byte</th>
        <th>0</th>
        <th>1</th>
        <th>2</th>
        <th>3..5</th>
        <th>6</th>
        <th>7</th>
        <th>8</th>
        <th>9</th>
        <th>10..11</th>
        <th>12</th>
        <th>13</th>
        <th>14</th>
        <th>15</th>
        <th>16..17</th>
    </tr>
    <tr>
        <th>Content</th>
        <td>0x5b(ID)</td>
        <td>length</td>
        <td>3</td>
        <td>frequency</td>
        <td>Ant1: tune enable</td>
        <td>Ant1: cin</td>
        <td>Ant1: clen</td>
        <td>Ant1: cout</td>
        <td>Ant1: I+Q</td>
        <td>Ant2: tune enable</td>
        <td>Ant2: cin</td>
        <td>Ant2: clen</td>
        <td>Ant2: cout</td>
        <td>Ant2: I+Q</td>
    </tr>
  </table>
  Adds the data to the internal tuning table. The first set of data is for antenna 1 the second set for antenna 2. \n
  The device responds:
  <table>
    <tr>
        <th>Byte</th>
        <th>0</th>
        <th>1</th>
        <th>2</th>
        <th>3</th>
    </tr>
    <tr>
        <th>Content</th>
        <td>0x5c(ID)</td>
        <td>0x40(length)</td>
        <td>3</td>
        <td>0xFF on success</td>
    </tr>
  </table>
  </li>
  <li> If the device does not support antenna tuning, the device always sends back:
    <table>
    <tr>
        <th>Byte</th>
        <th>0</th>
        <th>1</th>
        <th>2</th>
    </tr>
    <tr>
        <th>Content</th>
        <td>0x5c(ID)</td>
        <td>0x40(length)</td>
        <td>0</td>
    </tr>
  </table>
  </li>
  </ul>
 */
void callAntennaTune(void)
{
    memset(IN_PACKET,0,IN_GEN2_SETTINGS_IDSize+1);

    IN_PACKET[0] = IN_TUNER_SETTINGS_ID;
    IN_PACKET[1] = IN_TUNER_SETTINGS_IDSize + 1;

#ifdef CONFIG_TUNER
    switch (getBuffer_[2])
    {
    case 0x01:
    { /* set/get current tuner parameters */
        if (getBuffer_[3])
        {
            if (getBuffer_[4] > 31)
                getBuffer_[4] = 31;
            antennaParams.cin = getBuffer_[4];
            tunerSetCap(TUNER_Cin, antennaParams.cin);
        }
        if (getBuffer_[5])
        {
            if (getBuffer_[6] > 31)
                getBuffer_[6] = 31;
            antennaParams.clen = getBuffer_[6];
            tunerSetCap(TUNER_Clen, antennaParams.clen);
        }
        if (getBuffer_[7])
        {
            if (getBuffer_[8] > 31)
                getBuffer_[8] = 31;
            antennaParams.cout = getBuffer_[8];
            tunerSetCap(TUNER_Cout, antennaParams.cout);
        }
#if USBCOMMDEBUG
        CON_print("receive tune parameters cin= %hhx, clen= %hhx cout=%hhx\n", antennaParams.cin, antennaParams.clen, antennaParams.cout);
#endif
#ifdef POWER_DETECTOR
        as399xInitCyclicPowerRegulation(OUTPUTPOWER_TUNERCYCLES);
#endif
#if USBCOMMDEBUG
        if (getBuffer_[9])
            CON_print("tune antenna, algorithm: %hhx\n", getBuffer_[9]);
#endif
        switch (getBuffer_[9])
        /* autotune */
        {
        case (1): /* Simple: hill climb from current setting */
            as399xAntennaPower(1);
            tunerOneHillClimb(&antennaParams, 100);
            as399xAntennaPower(0);
            break;
        case (2): /* advanced: hill climb from more points */
            as399xAntennaPower(1);
            tunerMultiHillClimb(&antennaParams);
            as399xAntennaPower(0);
            break;
        case (3): /* try all combinations */
            as399xAntennaPower(1);
            tunerTraversal(&antennaParams);
            as399xAntennaPower(0);
            break;
        default:
            break;
            }

        IN_PACKET[2] = 1;
        IN_PACKET[4] = antennaParams.cin;
        IN_PACKET[6] = antennaParams.clen;
        IN_PACKET[8] = antennaParams.cout;
        break;
        }
    case 0x02: /*delete tuning table */
        tuningTable.currentEntry = 0;
        tuningTable.tableSize = 0;
#if USBCOMMDEBUG
        CON_print("delete tuning table tablesize= %hhx\n", tuningTable.tableSize);
#endif
        IN_PACKET[2] = 2;
        IN_PACKET[3] = 0xFF;
        IN_PACKET[4] = MAXTUNE;
        break;
    case 0x03:  /* add new entry in tuning table */
        if (addToTuningTable() > MAXTUNE)
            IN_PACKET[3] = 0x00;    //maximum of tuning table reached
        else
            IN_PACKET[3] = 0xFF;
        IN_PACKET[2] = 3;
        break;
    case 0x04:
#if USBCOMMDEBUG
        CON_print("retrieve max tuning table size= %hhx\n", MAXTUNE);
#endif
        IN_PACKET[2] = 4;
        IN_PACKET[3] = 0xFF;
        IN_PACKET[4] = MAXTUNE;
        break;
    default:
        IN_PACKET[2] = 0x00;
        break;
    }
#if USBCOMMDEBUG
    CON_print("send tune parameters cin= %hhx, clen= %hhx cout=%hhx\n", antennaParams.cin, antennaParams.clen, antennaParams.cout);
#endif
#else
    IN_PACKET[2] = 0x00;
#endif

    IN_BUFFER.Length =IN_TUNER_SETTINGS_IDSize+1;
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_TUNER_SETTINGS_ID);
}

/*!This function reads all register in one bulk from AS399x.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>0       </th><th>1</th>        </tr>
    <tr><td>Content</td><td>0x57(ID)</td><td>1(length)</td></tr>
  </table>
  <table>
    <tr>
        <th>Byte</th>
        <th>0</th>
        <th>1</th>
        <th>2</th>
        <th>..</th>
        <th>19</th>
        <th>20</th>
        <th>21</th>
        <th>22</th>
        <th>23</th>
        <th>24</th>
        <th>25</th>
        <th>26</th>
        <th>27</th>
        <th>28</th>
        <th>29</th>
        <th>30</th>
        <th>31</th>
        <th>32</th>
        <th>33</th>
        <th>34</th>
        <th>35</th>
        <th>36</th>
        <th>..</th>
        <th>44</th>
    </tr>
    <tr>
        <th>Content</th>
        <td>0x57(ID)</td>
        <td>45(length)</td>
        <td>reg 0x00</td>
        <td>..</td>
        <td>reg 0x11</td>
        <td>reg 0x12-1</td>
        <td>reg 0x12-2</td>
        <td>reg 0x12-3</td>
        <td>reg 0x13</td>
        <td>reg 0x14-1</td>
        <td>reg 0x14-2</td>
        <td>reg 0x14-3</td>
        <td>reg 0x15-1</td>
        <td>reg 0x15-2</td>
        <td>reg 0x15-3</td>
        <td>reg 0x16-1</td>
        <td>reg 0x16-2</td>
        <td>reg 0x16-3</td>
        <td>reg 0x17-1</td>
        <td>reg 0x17-2</td>
        <td>reg 0x17-3</td>
        <td>reg 0x18</td>
        <td>..</td>
        <td>reg 0x1f</td>
    </tr>
  </table>
*/
void callReadRegisters(void)
{
    readRegisters();
}

void readRegisters(void)
{
    u8 i;
    u8 idx = 2;

#if USBCOMMDEBUG
    CON_print("READ registers complete\n");
#endif


    IN_PACKET[0] = IN_REGS_COMPLETE_ID;

    for ( i = 0; i< 0x1f; i++)
    {
        if (       i==AS399X_REG_TESTSETTING  
                || i==AS399X_REG_CLSYSAOCPCTRL  
                || i==AS399X_REG_MODULATORCTRL 
                || i==AS399X_REG_PLLMAIN 
                || i==AS399X_REG_PLLAUX)
        {
            as399xContinuousRead(i, 3, IN_PACKET+idx);
            idx += 3;
        }
        else
        {
            IN_PACKET[idx] = as399xSingleRead(i);
            idx++;
        }
    }
    IN_PACKET[1] = idx + 2;
    IN_BUFFER.Length =IN_REGS_COMPLETE_IDSize+1;
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_REGS_COMPLETE_ID);
}


static void initTagInfo()
{
    u32 i;
    u8 *ptr;

    ptr = (u8*)tags_;

    for (i = 0; i <  sizeof(tags_) ; i ++)
    {
        *ptr++ = 0;
    }
}

/*!This function performs one inventory round using ISO18000-6b protocol.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>    2</th><th>   3</th><th>4 .. 11   </th><th>12</th></tr>
    <tr><th>Content</th><td>0x3f(ID)</td><td>12(length)</td><td>start</td><td>mask</tr><td>word_data </td><td>start_address</td></tr>
  </table>
where 
<ul>
<li>start: 1 -> start a new round, 2 -> deliver next tag </li>
<li>mask: the mask value for GROUP_SELECT_EQ command, 0 will select all tags</li>
<li>start_address: address where data will be compared</li> 
<li>word_data: data which will be compared</li> 
</ul>
  The device sends back:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>        2</th><th>            3</th><th>        4</th><th>5 .. 12</th><th>13 .. 15 </th></tr>
    <tr><th>Content</th><td>0x40(ID)</td><td>13(length)</td><td>tags_left</td><td>rssi(planned)</td><td>8(epclen)</td><td>    uid</td><td>used freq</td></tr>
  </table>
 */
void callInventory6B(void)
{

#if ISO6B
    static u8 element = 0;
    u8 count = 0;
    s8 status;

#if USBCOMMDEBUG
    CON_print("6b Inventory\n"); /* This MAY not be set, otherwise the Program execution drops. TLU */
#endif

    if (getBuffer_[2] == STARTINVENTORY)
    {
        checkAndSetSession(SESSION_ISO6B);
        status = hopFrequencies();
        element = 0;
        initTagInfo();
        if ( status )
        {
            num_of_tags = 0;
        }
        else
        {
            num_of_tags = iso6bInventoryRound (tags_, ARRAY_SIZE(tags_), getBuffer_[3], &getBuffer_[4], getBuffer_[12]);
        }
        hopChannelRelease();
        selectedTag = &tags_[0];
    }
    for (count = 0; count < IN_INVENTORY_6B_IDSize; count++)
    {
        IN_PACKET[count] = 0;
    }

    IN_PACKET[0] = IN_INVENTORY_6B_ID;
    IN_PACKET[1] = IN_INVENTORY_6B_IDSize+1;
    IN_PACKET[3] = 0; /* RSSI field, currently not supported */
    IN_PACKET[4] = tags_[element].epclen;
    IN_PACKET[13] = Frequencies.freq[currentFreqIdx] & 0xff;
    IN_PACKET[14] = (Frequencies.freq[currentFreqIdx] >>  8) & 0xff;
    IN_PACKET[15] = (Frequencies.freq[currentFreqIdx] >> 16) & 0xff;

    if (tags_[element].epclen)
    {
        copyBuffer(tags_[element].epc, &IN_PACKET[5], tags_[element].epclen);
    }
    if (num_of_tags)
    {
        IN_PACKET[2] = num_of_tags-element;
    }
    else
    {
        IN_PACKET[1] = 5;
        IN_PACKET[2] = 0;
    }
    element++;
#else
    IN_PACKET[0] = IN_INVENTORY_6B_ID;
    IN_PACKET[1] = IN_INVENTORY_6B_IDSize+1;
    IN_PACKET[2] = 0; /* no tags */
    IN_PACKET[3] = 0; /* RSSI field, currently not supported */
#endif
    IN_BUFFER.Length = IN_INVENTORY_6B_IDSize+1;
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_INVENTORY_6B_ID);
}

/*!This function reads from a tag using ISO18000-6b protocol command READ_VARIABLE.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>2 .. 9</th><th>  10</th><th>  11</th></tr>
    <tr><th>Content</th><td>0x49(ID)</td><td>12(length)</td><td>   uid</td><td>addr</tr><td>length_to_read</td></tr>
  </table>
where 
  The device sends back:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>        2</th><th>            3</th><th> 4 .. 4+length_to_read</th></tr>
    <tr><th>Content</th><td>0x50(ID)</td><td>64(length)</td><td>0 for success</td><td>length of data</td><td>data</td></tr>
  </table>
 */
void readFromTag6B(void)
{
    s8 result = 0xff;
#if USBCOMMDEBUG
    CON_print("6b Read\n"); /* This MAY not be set, otherwise the Program execution drops. TLU */
#endif

#if ISO6B
    result = hopFrequencies();
    
    if (!result) result = iso6bRead(&getBuffer_[2], getBuffer_[10], &IN_PACKET[4], getBuffer_[11]);
    hopChannelRelease();

    /* prepare answer ... */
    IN_PACKET[0] = IN_READ_FROM_TAG_6B_ID;
    IN_PACKET[1] = IN_READ_FROM_TAG_6B_IDSize+1;
    /* just support two answer codes - 0x0 if everything went ok, otherwise 0xff which means
        actually 'no response from tag' */
    IN_PACKET[2] = result * -1;
    IN_PACKET[3] = getBuffer_[11];
#else
    IN_PACKET[0] = IN_READ_FROM_TAG_6B_ID;
    IN_PACKET[1] = IN_READ_FROM_TAG_6B_IDSize+1;
    IN_PACKET[2] = -1;
    IN_PACKET[3] = 0;
#endif
    IN_BUFFER.Ptr = IN_PACKET;
    IN_BUFFER.Length = IN_BUFFER.Ptr[1];
    /* ... and send it */
    SendPacket(IN_READ_FROM_TAG_6B_ID);
}

void callReadFromTag6B(void)
{
    checkAndSetSession(SESSION_ISO6B);
    readFromTag6B();
}

/*!This function writes to a tag using ISO18000-6b protocol command WRITE.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>2 .. 9</th><th>  10</th><th>  11</th><th>12 .. 12+length_to_write</tr>
    <tr><th>Content</th><td>0x47(ID)</td><td>length</td><td>   uid</td><td>addr</tr><td>length_to_write</td><td>data</td></tr>
  </table>
where 
  The device sends back:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>        2</th><th>            3</th><th> 4 .. 4+length_to_read</th></tr>
    <tr><th>Content</th><td>0x48(ID)</td><td>3(length)</td><td>0 for success</td></tr>
  </table>
 */
void writeToTag6B(void)
{
#if ISO6B
    s8 result = ISO6B_ERR_NOTAG;
#if USBCOMMDEBUG
    CON_print("6b Write\n"); /* This MAY not be set, otherwise the Program execution drops. TLU */
#endif

    result = hopFrequencies();
    if ( !result ) result = iso6bWrite(&getBuffer_[2], getBuffer_[10], &getBuffer_[12], getBuffer_[11]);
    hopChannelRelease();

    /* prepare answer ... */
    IN_PACKET[0] = IN_WRITE_TO_TAG_6B_ID;
    IN_PACKET[1] = IN_WRITE_TO_TAG_6B_IDSize+1;
    IN_PACKET[2] = result * -1;
#else
    /* prepare answer ... */
    IN_PACKET[0] = IN_WRITE_TO_TAG_6B_ID;
    IN_PACKET[1] = IN_WRITE_TO_TAG_6B_IDSize+1;
    IN_PACKET[2] = -1;
#endif
    IN_BUFFER.Ptr = IN_PACKET;
    IN_BUFFER.Length = IN_BUFFER.Ptr[1];
    /* ... and send it */
    SendPacket(IN_WRITE_TO_TAG_6B_ID);
}

void callWriteToTag6B(void)
{
    checkAndSetSession(SESSION_ISO6B);
    writeToTag6B();
}

/*!This function performs a gen2 protocol inventory round according to parameters given by configGen2().
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>   2 </th></tr>
    <tr><th>Content</th><td>0x31(ID)</td><td>3(length)</td><td> start</td></tr>
  </table>
where 
<ul>
<li>start: 1 -> start a new round, 2 -> deliver next tag </li>
</ul>
  The device sends back in a burst mode all the tags using the following report:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th>        2</th><th>           3</th><th>   4 </th><th>   5 </th><th>6 .. 6 + epclen</th></tr>
    <tr><th>Content</th><td>0x32(ID)</td><td>length</td><td>tags_left</td><td>epclen+pclen</tr><td>pc[0]</td><td>pc[1]</td><td>epc</td></tr>
  </table>
 */
void callInventory(void)
{
    do
    {
        inventory();
        SendPacket(IN_INVENTORY_ID);
        getBuffer_[2] = NEXTTID;
    }
    while( (IN_PACKET[2]!=0) && (IN_PACKET[2]!=1) );
}

void inventory(void)
{
    static u8 element = 0;
    s8 result;

#if USBCOMMDEBUG
    CON_print("INVENTORY\n");
#endif
    if (getBuffer_[2] == STARTINVENTORY)
    {
#if USBCOMMDEBUG
        CON_print("START\n");
#endif
        checkAndSetSession(SESSION_GEN2);
        result = hopFrequencies();
        element = 0;
        num_of_tags = 0;
        if ( !result) num_of_tags = gen2SearchForTags(tags_,ARRAY_SIZE(tags_),mask,0,gen2qbegin,continueCheckTimeout,1); /* mask, masklength, q */
        hopChannelRelease();
    }
    IN_BUFFER.Ptr = IN_PACKET;
    IN_BUFFER.Length = IN_INVENTORY_IDSize+1;
    IN_PACKET[0] = IN_INVENTORY_ID;
    if (num_of_tags)
    {
        IN_PACKET[2] = num_of_tags-element;
    }
    else
    {
        IN_PACKET[2] = 0;
    }

    if (element < num_of_tags)
    {
        IN_PACKET[1] = tags_[element].epclen+2 + 4;
        IN_PACKET[3] = tags_[element].epclen+2; /* add 2 for pc */
        IN_PACKET[4] = tags_[element].pc[0];
        IN_PACKET[5] = tags_[element].pc[1];
        copyBuffer(tags_[element].epc, &IN_PACKET[6], tags_[element].epclen);
    }
    else
    {
        IN_PACKET[1] = 4;
        IN_PACKET[3] = 0;
    }
    element++;
}

/*!This function performs a gen2 protocol inventory round according to parameters given by configGen2().
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>   2 </th></tr>
    <tr><th>Content</th><td>0x43(ID)</td><td>3(length)</td><td> start</td></tr>
  </table>
where 
<ul>
<li>start: 1 -> start a new round, 2 -> deliver next tag </li>
</ul>
  The device sends back in a burst mode all the tags using the following report:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th>        2</th><th>         3</th><th> 4 .. 6  </th><th>           7</th><th>   8 </th><th>   9 </th><th>10 .. 10 + epclen</th></tr>
    <tr><th>Content</th><td>0x44(ID)</td><td>length</td><td>tags_left</td><td>RSSI_value</td><td>base_freq</td><td>epclen+pclen</tr><td>pc[0]</td><td>pc[1]</td><td>epc</td></tr>
  </table>
where 
<ul>
<li>RSSI_value: upper 4 bits I channel, lower 4 bits Q channel </li>
<li>base_freq: base frequency at which the tag was found. </li>
</ul>
 */
void callInventoryRSSIInternal(u8 startInvent)
{
    do
    {
        inventoryRSSI(startInvent);
        SendPacket(IN_INVENTORY_ID);
        startInvent = NEXTTID;
    }
    while( (IN_PACKET[2]!=0) && (IN_PACKET[2]!=1) );
}

void callInventoryRSSI(void)
{
    cyclicInventStart = 1;
    callInventoryRSSIInternal(getBuffer_[2]);
}

void inventoryRSSI(u8 startinvent)
{
    static u8 element = 0;
    s8 result;

#if USBCOMMDEBUG
    CON_print("INVENTORY RSSI\n");
#endif
    IN_BUFFER.Length = IN_INVENTORY_RSSI_IDSize+1;
    IN_BUFFER.Ptr = IN_PACKET;
    IN_PACKET[0] = IN_INVENTORY_RSSI_ID;
    IN_PACKET[1] = IN_INVENTORY_RSSI_IDSize+1;
    if (startinvent == STARTINVENTORY)
    {
        checkAndSetSession(SESSION_GEN2);
        result = hopFrequencies();
        element = 0;
        num_of_tags = 0;
#if 0
        if( !result ) num_of_tags = gen2SearchForTags(tags_,ARRAY_SIZE(tags_), mask,0,gen2qbegin,continueCheckTimeout,1); /* mask, masklength, q */
#else
        if( !result ) num_of_tags = gen2SearchForTagsFast(tags_,ARRAY_SIZE(tags_), mask,0,gen2qbegin,continueCheckTimeout, cyclicInventStart); /* mask, masklength, q */
#endif
        cyclicInventStart = 0;
        hopChannelRelease();
    }
    if (element < num_of_tags)
    {
        IN_PACKET[1] = tags_[element].epclen + 2 + 8;
        IN_PACKET[7] = tags_[element].epclen + 2;
        IN_PACKET[8] = tags_[element].pc[0];
        IN_PACKET[9] = tags_[element].pc[1];
        copyBuffer(tags_[element].epc, &IN_PACKET[10], tags_[element].epclen);
        IN_PACKET[3] = tags_[element].rssi;
        IN_PACKET[4] = Frequencies.freq[currentFreqIdx] & 0xff;
        IN_PACKET[5] = (Frequencies.freq[currentFreqIdx] >>  8) & 0xff;
        IN_PACKET[6] = (Frequencies.freq[currentFreqIdx] >> 16) & 0xff;
    }
    else
    {
        IN_PACKET[1] = 5;
        IN_PACKET[4] = 0;
    }
    if (num_of_tags)
    {
        IN_PACKET[2] = num_of_tags-element;
    }
    else
    {
        IN_PACKET[2] = 0;
    }
    element++;
}

/*!This function singulates a gen2 tag using the given mask for subsequent operations like read/write
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th>         2 </th><th>3 .. 3 + mask_length</th></tr>
    <tr><th>Content</th><td>0x33(ID)</td><td>length</td><td>mask_length</td><td>mask</td></tr>
  </table>
  The device sends back the following report:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>     2</th></tr>
    <tr><th>Content</th><td>0x34(ID)</td><td>3(length)</td><td>status</td></tr>
  </table>
 */
void callSelectTag(void)
{
    checkAndSetSession(SESSION_GEN2);
    selectTag();
}

void selectTag(void)
{
    u8 status=0;
#if USBCOMMDEBUG
    CON_print("SELECT Tag\n");
#endif
    initTagInfo();
    status = hopFrequencies();
    num_of_tags = 0;
    if (!status )
    {
        u8 i, allZero = 1;
        num_of_tags = gen2SearchForTags(tags_,1, &getBuffer_[3], getBuffer_[2],0,continueCheckTimeout,1); /* mask, masklength, q */
        for ( i = 0; i < getBuffer_[2]; i++ )
        {
            if (getBuffer_[3 + i] != 0)
                allZero = 0;
        }
        if (num_of_tags == 0 && allZero)
        {
            num_of_tags = gen2SearchForTags(tags_,1, &getBuffer_[3], getBuffer_[2],gen2qbegin,continueCheckTimeout,0); /* mask, masklength, q */
        }
    }
    hopChannelRelease();
#if USBCOMMDEBUG
        CON_print("SELECTed %hx tags\n",num_of_tags);
#endif
    if (num_of_tags)
    {
        selectedTag = &tags_[0];
        status = 0;    /* Tag found */
    }
    else
    {
        selectedTag = 0;
        status = GEN2_ERR_SELECT; /*Tag not found */
    }

    IN_PACKET[0] = IN_SELECT_TAG_ID;
    IN_PACKET[1] = IN_SELECT_IDSize+1;
    IN_PACKET[2] = status;
    IN_BUFFER.Length = IN_SELECT_IDSize+1;
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_SELECT_TAG_ID);
}

/*!This function writes to a previously selected gen2 tag.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th>       2</th><th>      3</th><th>  4 .. 7</th><th>       8</th><th>9 .. 9 + 2 * data_len</th></tr>
    <tr><th>Content</th><td>0x35(ID)</td><td>length</td><td>mem_type</td><td>address</td><td>acces_pw</td><td>data_len</td><td>data          </td></tr>
  </table>
where 
<ul>
<li>access_pw: if access password is nonzero the tag will be accessed first
<li>mem_type:<ul><li>0:reserved membank</li><li>1:EPC membank</li><li>2:TID membank</li><li>3:USER membank</li></ul>
<li>data_len: data length in 16-bit words</li>
</ul>
  The device sends back the following report:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>     2</th><th>     3</th></tr>
    <tr><th>Content</th><td>0x36(ID)</td><td>3(length)</td><td>status</td><td>num_words_written</td></tr>
  </table>
 */
void callWriteToTag(void)
{
    checkAndSetSession(SESSION_GEN2);
    writeToTag();
}

u8 writeMEM(u8 memAdress,Tag *tag, u8 *data_buf, u8 data_length_words, u8 mem_type, u8* status)
{
    u8 error = 0;
    u8 length = 0;
    u8 count;

    while (length < data_length_words)
    {
        /*writing pc into tag */
        count=0;
        do
        {
            error = gen2WriteWordToTag(tag, mem_type, memAdress + length, &data_buf[2*length]);
            count++;
        } while ( (count<7) && (error!=0) && continueCheckTimeout()); /* Give them 3 possible trials before giving up */
        if (timedOut) error = GEN2_ERR_CHANNEL_TIMEOUT;

        if (error)
        {
            *status = error;
            return(length); /*error occourd while writing to tag */
        }
        length += 1;
    }

    return length;
}

void writeToTag(void)
{
    u8 datalen,len;
    u8 frameLength;
    u8 status=0;

#if USBCOMMDEBUG
    CON_print("WRITE TO Tag\n");
#endif
    frameLength = getBuffer_[1];
    POWER_AND_SELECT_TAG();

    /* try to access tag using password if set */
    status = checkAndAccessTag(&getBuffer_[4]);
    if (status != 0)
    	goto exit;

    datalen = getBuffer_[8];

    len = writeMEM(getBuffer_[3],selectedTag, getBuffer_+9, datalen,getBuffer_[2],&status);
    if ( len != datalen && status == 0 ) status = 0xff;

exit:

    hopChannelRelease();
    IN_PACKET[0] = IN_WRITE_TO_TAG_ID;
    IN_PACKET[1] = IN_WRITE_TO_TAG_IDSize+1;
    IN_PACKET[2] = status;
    IN_PACKET[3] = len;
    IN_BUFFER.Length = IN_BUFFER.Ptr[1];
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_WRITE_TO_TAG_ID);
}

#ifdef CONFIG_TUNER
/* The code in the functions applyTunerSettingForFreq() and saveTuningParameters()
 * has been moved out of callChangeFreq into their own functions because otherwise
 * we hit a compiler problem:
 * A switch statement (as used in callChangeFreq) is translated into the assembler
 * instruction ajmp. ajmp allows a relative jump of up to 2k bytes from the current PC.
 * When the code for saving or applying tuner settings was still inside of callChangeFreq,
 * the jump distance would have been bigger than 2k. Therefore relocating the code into functions
 * solved this issue.
 */
static void applyTunerSettingForFreq(u32 freq)
{
    u8 i, idx;
    unsigned long int diff, best;

    if (tuningTable.tableSize == 0)     //tuning is disabled
        return;

    //find the best matching frequency
    best = 1.0e4;
    idx = 0;
    for(i=0; i<tuningTable.tableSize; i++)
    {
        //CON_print("***** find tune f=%x%x, tablefreq=%x%x, idx=%hhx", freq, tuningTable.freq[i], idx);
        if (tuningTable.freq[i] > freq)
            diff = tuningTable.freq[i] - freq;
        else
            diff = freq - tuningTable.freq[i];
        if (diff < best)
        {
            idx = i;
            best = diff;
        }
    }
    //apply the found parameters if enabled for the current antenna
    tuningTable.currentEntry = idx;
    if (tuningTable.tuneEnable[idx] & usedAntenna)
    {
        tunerSetTuning(tuningTable.cin[usedAntenna - 1][idx],
                tuningTable.clen[usedAntenna - 1][idx],
                tuningTable.cout[usedAntenna - 1][idx]);
//        CON_print("***** apply tune f=%x%x, idx=%hhx,  tune1=%hhx cin=%hhx clen=%hhx cout=%hhx\n", freq, idx,
//                tuningTable.tuneEnable[idx], tuningTable.cin[usedAntenna-1][idx],
//                tuningTable.clen[usedAntenna-1][idx], tuningTable.cout[usedAntenna-1][idx]);
        antennaParams.cin = tuningTable.cin[usedAntenna - 1][idx];
        antennaParams.clen = tuningTable.clen[usedAntenna - 1][idx];
        antennaParams.cout = tuningTable.cout[usedAntenna - 1][idx];
    }
}
#endif

#define RNDI 9          //index of first random value in buffer
static void pseudoRandomContinuousModulation()
{
    static u8 bufferIndex = 0;
    static u16 rnd;
    u8 buf[10];
    u8 command[1];

    //prepare a pseudo random value
    rnd ^= (getBuffer_[RNDI+bufferIndex*2] | (getBuffer_[RNDI+bufferIndex*2+1]<<8));      // get the random value from the GUI
    rnd ^= (PCA0H | (PCA0L<<8));            // add currently running timer value, to get some additional entropy

    bufferIndex++;
    if (bufferIndex > 5)
        bufferIndex = 0;

    // now send the data
    command[0] = AS399X_CMD_TRANSMCRC;
    buf[0] = 0;             /* Upper transmit length byte */
    buf[1] = 0x20;          /* 2 Bytes to transmit */
    buf[2] = (rnd & 0xFF);
    buf[3] = (rnd >> 8) & 0xFF;
    as399xCommandContinuousAddress(command, 1, AS399X_REG_TXLENGTHUP, buf, 4);
}

/*!This sets/adds/measures frequency related stuff:
  The format of the report from the host is one of the following
  <ul>
  <li>Get RSSI level
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th> 2</th><th>3 .. 5</th></tr>
    <tr><th>Content</th><td>0x41(ID)</td><td>length</td><td> 1</td><td>freq  </td></tr>
  </table>
  to which the reader replies with this:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>        2</th><th>        3</th><th>  4</th></tr>
    <tr><th>Content</th><td>0x42(ID)</td><td>64(length)</td><td>I-channel</td><td>Q-channel</td><td>dBm</td></tr>
  </table>
  </li>
  <li>Get Reflected Power level
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th> 2</th><th>3 .. 5</th><th> 6</th></tr>
    <tr><th>Content</th><td>0x41(ID)</td><td>length</td><td> 2</td><td>freq  </td><td> set_tune</td></tr>
  </table>
  If set_tune is set the corresponding tuning table value for this frequency will be set before measuring the reflected power.\n
  The reader replies with this:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>        2</th><th>        3</th><th>  4</th></tr>
    <tr><th>Content</th><td>0x42(ID)</td><td>64(length)</td><td>I-channel</td><td>Q-channel</td><td>dBm</td></tr>
  </table>
  </li>
  <li>Add frequency to frequency list used for hopping
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th> 2</th><th>3 .. 5</th><th>                   6</th><th>       7  </th><th>   8   </th><th>    9      </th><th> 10 </th><th> 11  </th><th> 12  </th><th>    13     </th><th> 14 </th><th> 15  </th><th> 16  </th></tr>
    <tr><th>Content</th><td>0x41(ID)</td><td>length</td><td> 4</td><td>freq  </td><td>rssi_threshhold(dBm)</td><td>profile_id</td><td>setTune</td><td>tuneEnable1</td><td>Cin1</td><td>Clen1</td><td>Cout1</td><td>tuneEnable2</td><td>Cin2</td><td>Clen2</td><td>Cout2</td></tr>
  </table>
  which adds the freq to list used for frequency hopping. If setTune is not 0 the tuning parameters for
  antenna 1 and 2 will be set. tuneEnableX enables the tune parameters for antenna X. CinX, ClenX and CoutX are
  the tuning parameters of antenna X. The reader replies with this:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>        2      </th><th>              3</th></tr>
    <tr><th>Content</th><td>0x42(ID)</td><td>64(length)</td><td>0xfc on success</td><td>0xff on success</td></tr>
  </table>
  </li>
  <li>Clear frequency list used for hopping and add the first frequency as described in the report above.
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th> 2</th><th>3 .. 5</th><th>                   6</th><th>       7  </th><th>   8   </th><th>    9      </th><th> 10 </th><th> 11  </th><th> 12  </th><th>    13     </th><th> 14 </th><th> 15  </th><th> 16  </th></tr>
    <tr><th>Content</th><td>0x41(ID)</td><td>length</td><td> 4</td><td>freq  </td><td>rssi_threshhold(dBm)</td><td>profile_id</td><td>setTune</td><td>tuneEnable1</td><td>Cin1</td><td>Clen1</td><td>Cout1</td><td>tuneEnable2</td><td>Cin2</td><td>Clen2</td><td>Cout2</td></tr>
  </table>
  The reader replies with this:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>        2      </th><th>              3</th></tr>
    <tr><th>Content</th><td>0x42(ID)</td><td>64(length)</td><td>0xfe on success</td><td>0xff on success</td></tr>
  </table>
  </li>
  <li>Set frequency hopping related parameters
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th>  2</th><th>3 .. 4       </th><th>5 .. 6        </th><th>7 .. 8  </th></tr>
    <tr><th>Content</th><td>0x41(ID)</td><td>length</td><td> 16</td><td>listeningTime</td><td>maxSendingTime</td><td>idleTime</td></tr>
  </table>
  The reader replies with this:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>        2      </th><th>              3</th></tr>
    <tr><th>Content</th><td>0x42(ID)</td><td>64(length)</td><td>0xfe on success</td><td>0xff on success</td></tr>
  </table>
  </li>
  <li>Get frequency hopping related parameters
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th>  2</th></tr>
    <tr><th>Content</th><td>0x41(ID)</td><td>length</td><td> 17</td></tr>
  </table>
  The reader replies with this:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>   2</th><th>   3</th><th>   4      </th><th>5 .. 6        </th><th>7 .. 8          </th><th>9 .. 10  </th><th>11 .. 13    </th><th>14 .. 16    </th><th>17           </th><th>            18</th><th>           19</th></tr>
    <tr><th>Content</th><td>0x42(ID)</td><td>64(length)</td><td>0xfe</td><td>0xff</td><td>profile_id</td><td>listening_time</td><td>max_sending_time</td><td>idle_time</td><td>gui_min_freq</td><td>gui_max_freq</td><td>gui_num_freqs</td><td>rssi_threshold</td><td>act_num_freqs</td></tr>
  </table>
  </li>
  <li>Continuous modulation test
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th>  2</th><th>    3 .. 5</th><th>    6 .. 7</th></tr>
    <tr><th>Content</th><td>0x41(ID)</td><td>length</td><td> 32</td><td>don't care</td><td>duration in ms</td></tr>
  </table>
  causes continuous modulation of the RF field for given duration. If  duration is 0 it runs until next command received from GUI.
  The reader replies with this:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th>   2</th><th>   3</th></tr>
    <tr><th>Content</th><td>0x42(ID)</td><td>64(length)</td><td>0xfe</td><td>0xff</td></tr>
  </table>
  </li>
  </ul>
 */
void callChangeFreq(void)
{
    u16 reflectedValues;
    u16 noiseLevel;
    u8 rssiValues;
    s8 dBm, sensi;
    u32 freq;
    freq = 0;
    freq += (long)getBuffer_[3];
    freq += ((long)getBuffer_[4]) <<8;
    freq += ((long)getBuffer_[5]) << 16;

#if USBCOMMDEBUG
    CON_print("CHANGE FREQ f=%x%x, subcmd=%hhx\n",freq,getBuffer_[2]);
#endif

    switch (getBuffer_[2])
    {
        case 0x01:
            {   /* get RSSI */
                as399xSetBaseFrequency(AS399X_REG_PLLMAIN,freq);
#if RUN_ON_AS3992
                sensi = -117;
#else
                sensi = -98;
#endif
                as399xSaveSensitivity();
                do
                {
                    s8 off;
                    sensi += 27;
                    off = as399xSetSensitivity(sensi);
                    as399xGetRSSI(0,&rssiValues,&dBm);
#if USBCOMMDEBUG
                    CON_print("sensi=%hhx off=%hhx reg05=%hhx reg0a=%hhx rssi=%hhx dBm=%hhx\n",
                               sensi, off, as399xSingleRead(0x05),as399xSingleRead(0x0a),rssiValues,dBm);
#endif
                }while( (rssiValues&0xf) >= 0xe && sensi < -20 );
                as399xRestoreSensitivity();
                IN_PACKET[2] = rssiValues&0xf;
                IN_PACKET[3] = ((rssiValues >> 4) & 0xf);
                IN_PACKET[4] = dBm;
                break;
            }
        case 0x02:
            {   /* get reflected power */
                as399xSetBaseFrequency(AS399X_REG_PLLMAIN,freq);
#ifdef CONFIG_TUNER
                if (getBuffer_[6])      // apply tuning table value if requested
                    applyTunerSettingForFreq(freq);
#endif
                as399xAntennaPower(1);
                noiseLevel = as399xGetReflectedPowerNoiseLevel();
                reflectedValues = as399xGetReflectedPower();
                as399xAntennaPower(0);
                IN_PACKET[2] = (reflectedValues & 0xff) - (noiseLevel & 0xff);
                IN_PACKET[3] = ((reflectedValues>>8)&0xff) - ((noiseLevel>>8)&0xff);
#if USBCOMMDEBUG
                CON_print("reflected power lsb=%hhx msb=%hhx\n", IN_PACKET[2], IN_PACKET[3]);
#endif
                break;
            }
        case 0x04:
            { /* Add to the frequency List */
                Frequencies.activefreq ++;
                guiNumFreqs++;
                if (Frequencies.activefreq > MAXFREQ) 
                {
                    Frequencies.activefreq = MAXFREQ;
                    IN_PACKET[2] = 0;
                    IN_PACKET[3] = 0;
                }
                else
                {
                    Frequencies.freq[Frequencies.activefreq - 1] =  freq;
                    Frequencies.rssiThreshold[Frequencies.activefreq - 1] =  getBuffer_[6];
#ifdef CONFIG_TUNER
                    Frequencies.countFreqHop[Frequencies.activefreq - 1] = 0;
#endif
                    guiActiveProfile = getBuffer_[7];
                    if (guiMaxFreq < freq) guiMaxFreq = freq;
                    if (guiMinFreq > freq) guiMinFreq = freq;
                    IN_PACKET[2] = 0xFC;
                    IN_PACKET[3] = 0xFF;
                }
                break;
            }
        case 0x08:
            {
                /* clear the frequency List */
                Frequencies.activefreq=1;
                guiNumFreqs = 1;
                Frequencies.freq[0] = freq;
                Frequencies.rssiThreshold[0] =  getBuffer_[6];
#ifdef CONFIG_TUNER
                Frequencies.countFreqHop[0] = 0;
#endif
                guiMaxFreq = freq;
                guiMinFreq = freq;
                IN_PACKET[2] = 0xFE;
                IN_PACKET[3] = 0xFF;
                break;
            }
        case 0x10:
            {
                /* set parameters for frequency hopping */
                listeningTime  =  getBuffer_[3];
                listeningTime |= (getBuffer_[4]<<8);
                maxSendingTime  =  getBuffer_[5];
                maxSendingTime |= (getBuffer_[6]<<8);
                idleTime  =  getBuffer_[7];
                idleTime |= (getBuffer_[8]<<8);
                IN_PACKET[2] = 0xFE;
                if (maxSendingTime <50)
                {
                    maxSendingTime = 50;
                    IN_PACKET[2] = 0xFF;
                }
                IN_PACKET[3] = 0xFF;
                break;
            }
        case 0x11:
            {
                u32 maxFreq = 0, minFreq = 0xffffffff; 
                /* get frequency infos */
                IN_PACKET[2] = 0xFE;
                IN_PACKET[3] = 0xFF;
                IN_PACKET[4] = guiActiveProfile;
                IN_PACKET[5] = listeningTime & 0xff;
                IN_PACKET[6] = (listeningTime >> 8) & 0xff;
                IN_PACKET[7] = maxSendingTime & 0xff;
                IN_PACKET[8] = (maxSendingTime >> 8) & 0xff;
                IN_PACKET[9] = idleTime & 0xff;
                IN_PACKET[10] = (idleTime >> 8) & 0xff;
                IN_PACKET[11] = guiMinFreq & 0xff;
                IN_PACKET[12] = (guiMinFreq >> 8) & 0xff;
                IN_PACKET[13] = (guiMinFreq >> 16) & 0xff;
                IN_PACKET[14] = guiMaxFreq & 0xff;
                IN_PACKET[15] = (guiMaxFreq >> 8) & 0xff;
                IN_PACKET[16] = (guiMaxFreq >> 16) & 0xff;
                IN_PACKET[17] = guiNumFreqs;
                IN_PACKET[18] = Frequencies.rssiThreshold[0];
                IN_PACKET[19] = Frequencies.activefreq;
                break;
            }
        case 0x20:
            {
                /* continuous modulation */
                u16 time_ms = getBuffer_[6] | (getBuffer_[7]<<8);
                u8 rxnorespwait, rxwait;

                rxnorespwait = as399xSingleRead( AS399X_REG_RXNORESPWAIT);
                rxwait       = as399xSingleRead( AS399X_REG_RXWAITTIME);
                as399xSingleWrite( AS399X_REG_RXNORESPWAIT, 0 );
                as399xSingleWrite( AS399X_REG_RXWAITTIME, 0 );
                as399xSetBaseFrequency(AS399X_REG_PLLMAIN,freq);
                IN_PACKET[0] = IN_CHANGE_FREQ_ID;
                IN_PACKET[1] = IN_CHANGE_FREQ_IDSize+1;
                IN_BUFFER.Length = IN_BUFFER.Ptr[1];
                IN_BUFFER.Ptr = IN_PACKET;
                IN_PACKET[2] = 0xFE;
                IN_PACKET[3] = 0xFF;
                SendPacket(IN_CHANGE_FREQ_ID);
                maxSendingLimit_slowTicks = MS_2_SLOWTICKS(time_ms);
                timedOut = 0;
#ifdef POWER_DETECTOR
                as399xInitCyclicPowerRegulation(OUTPUTPOWER_MAINCYCLES);
#endif
                as399xAntennaPower(1);
                mdelay(1);
                timerStartMeasure();
                resetUSBReceiveFlag();
                do{
#ifdef POWER_DETECTOR
                    as399xCyclicPowerRegulation();
#endif
                    as399xSingleCommand(AS399X_CMD_RESETFIFO);
                    if (getBuffer_[8] == 0x01)
                    {   /* pseudo random continuous modulation */
                        pseudoRandomContinuousModulation();
                    }
                    else
                    {   /* static modulation */
                        as399xSingleCommand(AS399X_CMD_NAK);
                    }
                    as399xWaitForResponseTimed(RESP_TXIRQ,10);
                    as399xClrResponse();

#if UARTSUPPORT
                }while(continueCheckTimeout() && !checkByte());
#else
                }while(continueCheckTimeout() && !getReceiveFlag());
#endif
                as399xAntennaPower(0);
                as399xSingleWrite( AS399X_REG_RXNORESPWAIT, rxnorespwait );
                as399xSingleWrite( AS399X_REG_RXWAITTIME, rxwait );
                dontResetUSBReceiverFlag = 1;
#if USBCOMMDEBUG
                CON_print("finished sending \n");
#endif
                return;
            }


        default:
            {
                IN_PACKET[2] = 0xFF;
                IN_PACKET[3] = 0xFF;
                break;
            }

    }

    IN_PACKET[0] = IN_CHANGE_FREQ_ID;
    IN_PACKET[1] = IN_CHANGE_FREQ_IDSize+1;
    IN_BUFFER.Length = IN_BUFFER.Ptr[1];
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_CHANGE_FREQ_ID);
}

/*!This function reads from a previously selected gen2 tag.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th>       2</th><th>      3</th><th>             4</th></tr>
    <tr><th>Content</th><td>0x37(ID)</td><td>length</td><td>mem_type</td><td>address</td><td>data_len</td></tr>
  </table>
where 
<ul>
<li>mem_type:<ul><li>0:reserved membank</li><li>1:EPC membank</li><li>2:TID membank</li><li>3:USER membank</li></ul>
<li>data_len: data length to read in 16-bit words</li>
</ul>
  The device sends back the following report:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>     2</th><th>     3</th></tr>
    <tr><th>Content</th><td>0x38(ID)</td><td>length</td><td>status</td><td>num_words_read</td></tr>
  </table>
 */
void callReadFromTag(void)
{
    checkAndSetSession(SESSION_GEN2);
    readDataFromTag();
}

void readDataFromTag(void)
{
    u8 datalen;
    u8 status=0;

#if USBCOMMDEBUG
    CON_print("READ FROM Tag\n");
    CON_print("membank = %hhx\n",getBuffer_[2]);
    CON_print("ptr = %hhx\n",getBuffer_[3]);
    CON_print("datalen = %hhx\n",getBuffer_[4]);
#endif  

    datalen = getBuffer_[4];

    POWER_AND_SELECT_TAG();

    if (datalen == 0)
    {
        u8 * b = IN_PACKET + 4;
        u8 wrdPtr = getBuffer_[3];
        while(! status && datalen < 30 && continueCheckTimeout()) 
        {
            status = gen2ReadFromTag(selectedTag, getBuffer_[2], wrdPtr++, 1, b);
            datalen++;
            b+=2;
        }
        datalen--; /* Last must have failed */
        if (timedOut) status = GEN2_ERR_CHANNEL_TIMEOUT;
    }
    else
    {
        status = gen2ReadFromTag(selectedTag, getBuffer_[2], getBuffer_[3], datalen, &IN_PACKET[4]);
    }

exit:
    hopChannelRelease();
    IN_PACKET[0] = IN_READ_FROM_TAG_ID;
    IN_PACKET[2] = status;
    IN_PACKET[1] = 2*datalen + 4;
    IN_PACKET[3] = datalen;

    IN_BUFFER.Length = IN_READ_FROM_TAG_IDSize+1;
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_READ_FROM_TAG_ID);
}

/*!This function locks a gen2 tag.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>     1</th><th>         2 </th><th>          3 </th><th>4 .. 7</th></tr>
    <tr><th>Content</th><td>0x3b(ID)</td><td>8(length)</td><td>lock_unlock</td><td>memory_space</td><td>access password</td></tr>
  </table>
  where:
  <ul>
    <li>lock_unlock: <table>
    <tr><td>0</td><td>Unlock</td></tr>
    <tr><td>1</td><td>Lock</td></tr>
    <tr><td>2</td><td>Permalock</td></tr>
    <tr><td>3</td><td>Lock & Permalock</td></tr>
    </table>
    </li>
    <li>memory_space: <table>
    <tr><td>0</td><td>Kill password</td></tr>
    <tr><td>1</td><td>Access password</td></tr>
    <tr><td>2</td><td>EPC</td></tr>
    <tr><td>3</td><td>TID</td></tr>
    </table>
    </li>
  </ul>
  The device sends back the following report:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>     2</th></tr>
    <tr><th>Content</th><td>0x3c(ID)</td><td>3(length)</td><td>status</td></tr>
  </table>
 */
void callLockUnlock(void)
{
    checkAndSetSession(SESSION_GEN2);
    lockUnlock();
}
void lockUnlock(void)
{
    u8 mask[3];
    u8 status;
#if USBCOMMDEBUG
    CON_print("Command  %hhx %hhx\n",getBuffer_[2],getBuffer_[3]);
    CON_print("Lock Tag\n");
    {
        u8 i;
        for (i=0;i<=getBuffer_[1];i++)
        {
            CON_print("%hhx ",getBuffer_[i]);
        }
    }
    CON_print("\n");
#endif

    POWER_AND_SELECT_TAG();

    /* try to access tag using password if set */
    status = checkAndAccessTag(&getBuffer_[4]);
    if (status != 0)
        goto exit;

    if (getBuffer_[2] == 0x01) getBuffer_[2]=0x02;
    else if (getBuffer_[2] == 0x02) getBuffer_[2]=0x01; /* To adapt to description */
    mask[0]=0;
    mask[1]=0;
    mask[2]=0;
    if      (getBuffer_[3] ==0x00)
    {
        mask[0]=0xC0;
        mask[1]= (getBuffer_[2]<<4)&0x30;
    }
    else if (getBuffer_[3] ==0x01)
    {
        mask[0]=0x30;
        mask[1]= (getBuffer_[2]<<2)&0x0C;
    }
    else if (getBuffer_[3] ==0x02)
    {
        mask[0]=0x0C;
        mask[1]= (getBuffer_[2]   )&0x03;
    }
    else if (getBuffer_[3] ==0x03)
    {
        mask[0]=0x03;
        mask[2]= (getBuffer_[2]<<6)&0xC0;
    }
    else if (getBuffer_[3] ==0x04)
    {
        mask[1]=0xC0;
        mask[2]= (getBuffer_[2]<<4)&0x30;
    }
    else
    {
        mask[0]=0x00;
        mask[1]=0x00;
    }

#if USBCOMMDEBUG
    CON_print("LOCK UNLOCK Tag\n");
#endif
    status= gen2LockTag(selectedTag, mask);

exit:
    hopChannelRelease();
    IN_PACKET[0] = IN_LOCK_UNLOCK_ID;
    IN_PACKET[1] = IN_WRITE_TO_TAG_IDSize+1;
    IN_PACKET[2] = status;
    IN_BUFFER.Length = IN_BUFFER.Ptr[1];
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_LOCK_UNLOCK_ID);

}

/*!This function kills a gen2 tag.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>2 .. 5       </th><th>   6 </th></tr>
    <tr><th>Content</th><td>0x3d(ID)</td><td>7(length)</td><td>kill password</td><td>recom</td></tr>
  </table>
  where:
  <ul>
    <li>recom: see GEN2 standard: table on "XPC_W1 LSBs and a Tag's recomissioned status"
    </li>
  </ul>
  The device sends back the following report:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>     2</th></tr>
    <tr><th>Content</th><td>0x3e(ID)</td><td>3(length)</td><td>status</td></tr>
  </table>
 */
void callKillTag(void)
{
    checkAndSetSession(SESSION_GEN2);
    usb_gen2KillTag();
}

void usb_gen2KillTag(void)
{
    u8 status;
    CON_print("KILL Tag\n");
#if USBCOMMDEBUG
    CON_print("KILL Tag\n");
    gen2PrintEPC(selectedTag);
#endif

    POWER_AND_SELECT_TAG();

    status= gen2KillTag(selectedTag,  &getBuffer_[2],getBuffer_[6]);

exit:
    hopChannelRelease();
    IN_PACKET[0] = IN_KILL_TAG_ID;
    IN_PACKET[1] = IN_WRITE_TO_TAG_IDSize+1;
    IN_PACKET[2] = status;
    IN_BUFFER.Length = IN_BUFFER.Ptr[1];
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_KILL_TAG_ID);

}

/*!This function sends special NXP command to NXP gen2 tags.
  The format of the report from the host is one of the following
  <ul>
  <li>EAS command
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th> 2</th><th>   3  </th><th>   4 .. 7</th></tr>
    <tr><th>Content</th><td>0x45(ID)</td><td>10(length)</td><td> 1</td><td>eas_on</td><td>access pw</td></tr>
  </table>
  </li>
  <li>Set/Reset protect
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th> 2</th><th>   3  </th><th>   4 .. 7</th></tr>
    <tr><th>Content</th><td>0x45(ID)</td><td>10(length)</td><td> 2</td><td>set_protect</td><td>access pw</td></tr>
  </table>
  </li>
  <li>Calibrate
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th> 2</th><th>   3  </th><th>   4 .. 7</th></tr>
    <tr><th>Content</th><td>0x45(ID)</td><td>10(length)</td><td> 8</td><td>not_used</td><td>access pw</td></tr>
  </table>
  </li>
  <li>Change Config
  <table>
    <tr><th>   Byte</th><th>       0</th><th>         1</th><th> 2</th><th>   3  </th><th>   4 .. 7  </th><th> 8..9 </th></tr>
    <tr><th>Content</th><td>0x45(ID)</td><td>10(length)</td><td> 9</td><td>not_used</td><td>access pw</td><td>config</td></tr>
  </table>
  where config is a mask of bit which should be flipped inside config word (XORed).
  </li>
  </ul>
  To all the reader replies with this:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>     2</th><th>        3 .. 4</th></tr>
    <tr><th>Content</th><td>0x46(ID)</td><td>5(length)</td><td>status</td><td>config word (only change config)</td></tr>
  </table>
 */
void callNXPCommands(void)
{
    checkAndSetSession(SESSION_GEN2);
    NXPCommands();
}

void NXPCommands(void)
{
    /*u8 password[4]; */
    /*u8 rfu; */
    u8 status;

#if USBCOMMDEBUG
    CON_print("NXP Command Tag\n");
#endif

    POWER_AND_SELECT_TAG();

    /* try to access tag using password if set */
    status = checkAndAccessTag(&getBuffer_[4]);
    if (status != 0)
        goto exit;

    switch (getBuffer_[2])
    {
        case 0x01: /* EAS Command */
            {
                status= gen2ChangeEAS(selectedTag, getBuffer_[3]);
                break;
            }
        case 0x02: /* Read Protect Bit on/off */
            {
                if (getBuffer_[3])
                {
                    status= gen2SetProtectBit(selectedTag );
                }
                else
                {
                    status= gen2ReSetProtectBit(selectedTag, getBuffer_+4);
                }
                break;
            }
        case 0x04: /* EAS Alarm Inventory */
            {
                break;
            }
        case 0x08: /* Calibrate on */
            {
                status= gen2Calibrate (selectedTag);
                break;
            }
        case 0x09: /* Change config */
            {
                status = gen2NXPChangeConfig(selectedTag,getBuffer_+8);
                IN_PACKET[3] = getBuffer_[8];
                IN_PACKET[4] = getBuffer_[9];
                break;
            }
    }

exit:
    hopChannelRelease();
    IN_PACKET[0] = IN_NXP_COMMAND_ID;
    IN_PACKET[1] = IN_NXP_COMMAND_IDSize+1;
    IN_PACKET[2] = status;
    IN_BUFFER.Length = IN_BUFFER.Ptr[1];
    IN_BUFFER.Ptr = IN_PACKET;
    SendPacket(IN_NXP_COMMAND_ID);
}

/*!This function disables current application and enables bootloader.
  Device needs to be reprogrammed afterwards.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>       2 </th><th>       3 </th></tr>
    <tr><th>Content</th><td>0x55(ID)</td><td>4(length)</td><td>dont_care</td><td>dont_care</td></tr>
  </table>
  The device sends back:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>2</th></tr>
    <tr><th>Content</th><td>0x56(ID)</td><td>3(length)</td><td>1 on success</td></tr>
 */
void callEnableBootloader(void)
{
    if (getBuffer_[1] == 4)
    {
        IN_PACKET[0] = IN_FIRM_PROGRAM_ID;
        IN_PACKET[1] = IN_FIRM_PROGRAM_IDSize+1;
        IN_BUFFER.Ptr = IN_PACKET;
        IN_BUFFER.Length = IN_BUFFER.Ptr[1];
#if defined (ENTRY_POINT_ADDR) && (ENTRY_POINT_ADDR > 0)
        IN_PACKET[2] = 0x1;
        SendPacket(IN_FIRM_PROGRAM_ID);
        /* give the USB subsystem some time to send the package */
        mdelay(100);
        FLASH_ByteWrite(ENTRY_POINT_ADDR-1, 0x0);
        /* reset the system */
        RSTSRC = 0x10;
#else
        /* standalone build, firmware upgrade not supported */
        IN_PACKET[2] = 0xff;
        SendPacket(IN_FIRM_PROGRAM_ID);
#endif
    }
}

/*! This function starts/stops the automatic scanning procedure.
  The format of the report from the host is as follows:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>     2</th><th>    3</th></tr>
    <tr><th>Content</th><td>0x5d(ID)</td><td>4(length)</td><td>update</td><td>start</td></tr>
  </table>
  To start inventory rounds update has to be set to 1 and start has to be set to 1. To stop inventory rounds
  update has to be set to 1 and start has to be set to 0. If update is set to 0 no change of the current settings will be
  done.
  The device sends back:
  <table>
    <tr><th>   Byte</th><th>       0</th><th>        1</th><th>2</th></tr>
    <tr><th>Content</th><td>0x5e(ID)</td><td>3(length)</td><td>current start value</td></tr>

    Subsequently callInventoryRSSIInternal() result packages are returned in a
    dense continuous loop. See there for description.
 */
void callStartStop(void)
{
#if USBCOMMDEBUG
    CON_print("STARTSTOP: %hhx  %hhx\n", getBuffer_[2], getBuffer_[3]);
#endif
    if (getBuffer_[2])
    {
        cyclic = getBuffer_[3];
        cyclicInventStart = 1;
    }
#ifdef POWER_DETECTOR
    as399xInitCyclicPowerRegulation(OUTPUTPOWER_QUERYCYCLES);
#endif
    IN_PACKET[0] = IN_START_STOP_ID;
    IN_PACKET[1] = 3;
    IN_BUFFER.Ptr = IN_PACKET;
    IN_BUFFER.Length = IN_START_STOP_IDSize+1;
    IN_PACKET[2] = cyclic;
    SendPacket(IN_START_STOP_ID);
    if(!cyclic)
        as399xEnterPowerDownMode();
}

void callGenericCommand(void)
{
#if USBCOMMDEBUG
    CON_print("GENERIC_COMMAND:\n");
    CON_hexdump(getBuffer_, EP1_PACKET_SIZE);
#endif
    checkAndSetSession(SESSION_GEN2);

    genericCommand();
}

void callAuthenticateCommand(void)
{
	u8 status = 0;

#if USBCOMMDEBUG
    CON_print("AUTHENTICATE:\n");
    CON_hexdump(getBuffer_, EP1_PACKET_SIZE);
#endif

    goto exit;
exit:
	hopChannelRelease();
	IN_PACKET[0] = IN_AUTHENTICATE_ID;
	IN_PACKET[1] = IN_AUTHENTICATE_IDSize+1;
	IN_PACKET[2] = status;
	IN_PACKET[3] = 10; // FIXME

	IN_BUFFER.Ptr = IN_PACKET;
	IN_BUFFER.Length = IN_BUFFER.Ptr[1];
#if USBCOMMDEBUG
	CON_print("callAuthenticateCommand returns:\n");
	CON_hexdump(IN_PACKET, 	IN_BUFFER.Length);
#endif
	SendPacket(IN_AUTHENTICATE_ID);
}

void callChallengeCommand(void)
{
	u8 status = 0;
	u16 csid = 0;
	u16 msgLen = 0;
	u8 outputLength = 0;


	csid = getBuffer_[3] << 8;
	csid |= getBuffer_[4];
	msgLen = getBuffer_[5] << 8;
	msgLen |= getBuffer_[6];


#if USBCOMMDEBUG
    CON_print("CHALLENGE:\n");
    CON_hexdump(getBuffer_, EP1_PACKET_SIZE);
#endif
    status = gen2ChallengeCommand(getBuffer_[2], csid, msgLen, &getBuffer_[7], &IN_PACKET[4], &outputLength);
    goto exit;
exit:
	hopChannelRelease();
	IN_PACKET[0] = IN_CHALLENGE_ID;
	IN_PACKET[1] = IN_CHALLENGE_IDSize+1;
	IN_PACKET[2] = status;
	IN_PACKET[3] = outputLength;
	IN_BUFFER.Ptr = IN_PACKET;
	IN_BUFFER.Length = IN_BUFFER.Ptr[1];
#if USBCOMMDEBUG
	CON_print("callChallengeCommand returns:\n");
	CON_hexdump(IN_PACKET, 	IN_BUFFER.Length);
#endif
	SendPacket(IN_CHALLENGE_ID);
}

void callReadBufferCommand(void)
{
	u8 status = 0;

#if USBCOMMDEBUG
    CON_print("READ_BUFFER:\n");
    CON_hexdump(getBuffer_, EP1_PACKET_SIZE);
#endif

    goto exit;
exit:
	hopChannelRelease();
	IN_PACKET[0] = IN_READ_BUFFER_ID;
	IN_PACKET[1] = IN_READ_BUFFER_IDSize+1;
	IN_PACKET[2] = status;
	IN_PACKET[3] = 10; // FIXME
	IN_BUFFER.Ptr = IN_PACKET;
	IN_BUFFER.Length = IN_BUFFER.Ptr[1];
#if USBCOMMDEBUG
	CON_print("callReadBufferCommand returns:\n");
	CON_hexdump(IN_PACKET, 	IN_BUFFER.Length);
#endif
	SendPacket(IN_READ_BUFFER_ID);
}

/*!This function sends generic command to gen2 tags.
	TODO: document format
	*
 */
void genericCommand(void)
{
	u8 status = 0;
	u8 dataLen = 0;
	struct gen2GenericCmdData cmdData;
	u16 txBitCount = 0, rxBitCount = 0;

	POWER_AND_SELECT_TAG();

    /* try to access tag using password if set */
    status = checkAndAccessTag(&getBuffer_[2]);
    if (status != 0)
        goto exit;

    /* fill out struct gen2GenericData */
    cmdData.txBitCount = getBuffer_[6] >> 4;			  	/* upper nibble of 12bit tx-len */
    cmdData.txBitCount <<= 8;
    cmdData.txBitCount |=  (getBuffer_[6] & 0x0F) << 4;		/* middle nibble of 12bit tx-len */
    cmdData.txBitCount |=  (getBuffer_[7] >> 4);		  	/* lower nibble of 12bit tx-len */
    cmdData.rxBitCount =   (getBuffer_[7] & 0x0F) << 8;  	/* upper nibble of 12bit rx-len */
    cmdData.rxBitCount |=  getBuffer_[8];					/* middle and lower nibble of 12bit rx-len */
    cmdData.txData = &getBuffer_[11];
    cmdData.rxData = &IN_PACKET[4];
    cmdData.directCommand = getBuffer_[9];
    cmdData.rxNoRespTimeout = getBuffer_[10];
    cmdData.actRxByteCount = 0;

	memset(&IN_PACKET[4], 0x00, EP1_PACKET_SIZE - 4);
	status = gen2GenericCommand(selectedTag, &cmdData);

exit:
	hopChannelRelease();
	IN_PACKET[0] = IN_GENERIC_COMMAND_ID;
	IN_PACKET[1] = IN_GENERIC_COMMAND_IDSize+1;
	IN_PACKET[2] = status;
	IN_PACKET[3] = cmdData.actRxByteCount;
	IN_BUFFER.Ptr = IN_PACKET;
	IN_BUFFER.Length = IN_BUFFER.Ptr[1];
#if USBCOMMDEBUG
	CON_print("genericCommand returns:\n");
	CON_hexdump(IN_PACKET, 	IN_BUFFER.Length);
#endif
	SendPacket(IN_GENERIC_COMMAND_ID);
}

void initCommands(void)
{
    currentSession = 0;
    cyclic = 0;
    dontResetUSBReceiverFlag = 0;
    as399xEnterPowerDownMode();
#ifdef CONFIG_TUNER
    antennaParams.cin  = 15;
    antennaParams.clen = 15;
    antennaParams.cout = 15;
#endif
}

static s8 hopFrequencies(void)
{
    u8 i;
    u8 min_idx;
    s8 dBm = -128;
    u8 rssi;
    u16 refl = 0;

    maxSendingLimit_slowTicks = MS_2_SLOWTICKS(maxSendingTime - 16);

    as399xExitPowerDownMode();
    as399xAntennaPower(0);

    if ( MS_2_SLOWTICKS(idleTime) > timerMeasure_slowTicks())
    { /* if the time in between is larger than ~1min we wait unnecessarily. 
         but this won't hurt if the upper level was anyway waiting this long. */
        mdelay(idleTime -  SLOWTICKS_2_MS(timerMeasure_slowTicks()));
    }
    if (Frequencies.activefreq == 0)
    {
        if (!cyclic) as399xEnterPowerDownMode();
        return GEN2_ERR_CHANNEL_TIMEOUT;
    }

    min_idx = currentFreqIdx;
    for (i = 0; i< MAXFREQ; i++)
    {
        if ( ++currentFreqIdx >= Frequencies.activefreq ) currentFreqIdx = 0;
        as399xSetBaseFrequency(AS399X_REG_PLLMAIN,Frequencies.freq[currentFreqIdx]);
        if ( Frequencies.rssiThreshold[currentFreqIdx] <= -40 )
            break;          //we skip rssi measurement if threshold is absurdly low.
        as399xGetRSSI(listeningTime,&rssi,&dBm);
        if (dBm <= Frequencies.rssiThreshold[currentFreqIdx]) break; /* Found free frequency, now we can return */
    }
    if (dBm <= Frequencies.rssiThreshold[currentFreqIdx])
    {
        timerStartMeasure();
        timedOut = 0;
#ifdef CONFIG_TUNER
        applyTunerSettingForFreq(Frequencies.freq[currentFreqIdx]);
#endif
        as399xAntennaPower(1);
#ifdef CONFIG_TUNER
        if ( tuningTable.tableSize > 0 &&
                ++Frequencies.countFreqHop[currentFreqIdx] > 150 )
            /*If tuning is enabled and this frequency has been selected for the 150th time,
              check if we have to do a re-tune (autotune) */
        {
            refl = tunerGetReflected();
#if USBCOMMDEBUG
            CON_print("countFreqHop has reached %hhx\n",Frequencies.countFreqHop[currentFreqIdx]);
            CON_print("old reflected power: %hx\n", tuningTable.tunedIQ[usedAntenna-1][tuningTable.currentEntry]);
            CON_print("measured reflected power: %hx\n", refl);
#endif
            Frequencies.countFreqHop[currentFreqIdx] = 0;
            if ( refl > tuningTable.tunedIQ[usedAntenna-1][tuningTable.currentEntry] * 1.3 ||
                    refl < tuningTable.tunedIQ[usedAntenna-1][tuningTable.currentEntry] * 0.7 )
                /* if reflected power differs 30% compared to last tuning time, redo tuning */
            {
                tunerOneHillClimb(&antennaParams, 100);
                //tunerMultiHillClimb(&antennaParams);
#if USBCOMMDEBUG
                CON_print("redo tuning, old cin: %hhx, clen: %hhx, cout: %hhx\n", tuningTable.cin[usedAntenna-1][tuningTable.currentEntry],
                        tuningTable.clen[usedAntenna-1][tuningTable.currentEntry], tuningTable.cout[usedAntenna-1][tuningTable.currentEntry]);
                CON_print("new values cin: %hhx, clen: %hhx, cout: %hhx, iq: %hx\n", antennaParams.cin,
                        antennaParams.clen, antennaParams.cout, antennaParams.reflectedPower);
#endif
                tuningTable.cin[usedAntenna-1][tuningTable.currentEntry] = antennaParams.cin;
                tuningTable.clen[usedAntenna-1][tuningTable.currentEntry] = antennaParams.clen;
                tuningTable.cout[usedAntenna-1][tuningTable.currentEntry] = antennaParams.cout;
                tuningTable.tunedIQ[usedAntenna-1][tuningTable.currentEntry] = antennaParams.reflectedPower;
            }
        }
#endif
#ifdef POWER_DETECTOR
        as399xCyclicPowerRegulation();
#endif
        return GEN2_OK;
    }
    else
    {
#if USBCOMMDEBUG
        CON_print("hopFrequencies did not find a free channel\n");
#endif
        timedOut = 1;
        as399xAntennaPower(0);
        if (!cyclic) as399xEnterPowerDownMode();
        return GEN2_ERR_CHANNEL_TIMEOUT;
    }
}

static void hopChannelRelease(void)
{
    timerStartMeasure();
    as399xAntennaPower(0);
    if (!cyclic) as399xEnterPowerDownMode();
}

/*------------------------------------------------------------------------- */
/*This function starts the right function for the command received by */
/*USB. */
void commands(void)
{
    if (getReceiveFlag())
    {
#if USBCOMMDEBUG
        CON_print("IN %hhx\n",USB_COMMAND);
#endif
        cyclic = 0;
        /* Special handling for start/stop command sent without waiting for reply ... ugly ..*/
        if (USB_COMMAND != 0x5d) as399xExitPowerDownMode();
        call_fkt_[USB_COMMAND]();
        if (!dontResetUSBReceiverFlag) resetUSBReceiveFlag();
        dontResetUSBReceiverFlag = 0;
    }
    if (cyclic)
    {
        callInventoryRSSIInternal(1);
    }
}

#if UARTSUPPORT
/*------------------------------------------------------------------------- */
/*This function starts the right function for the command received by */
/*UART. */
void uartCommands(void)
{ 
    static long timeout; 
    static u8 pointer;
    u8 i;

    switch (uartState)
    {
        case UART_IDLE:    /* Wait for Bytes to be received */
            { 
                pointer =0;
                timeout=0;
                getBuffer_[1] = 2;
                if (checkByte())  {getBuffer_[pointer++]=getByte();   uartState= UART_RECEIVE; cyclic=0;}
                break;
            }
        case UART_RECEIVE:   /* Check the bytes annd wait for the end of reception */
            {
                timeout++;
                if (timeout > 0x8000 || pointer > 63 || pointer >= getBuffer_[1] )
                {
                    uartState=UART_PROCESS; 
                    return;
                }

                if (checkByte())  {
                    getBuffer_[pointer++]=getByte();
                    timeout=0;
                }
                break;
            }
        case UART_PROCESS:   /* process the command and send the result back */
            {
                for (i=0;i<64;i++)   IN_PACKET[i]=0;  /* Clear the Buffer */
                uartState=UART_IDLE;
                uartFlag=1;
                as399xExitPowerDownMode();
                call_fkt_[getBuffer_[0]]();           /* execute command */
                uartFlag=0;
            }
        default:
            uartState=UART_IDLE;
            break;
    }
    if (cyclic)
    {
        callInventoryRSSIInternal(1);
    }
}
#endif
