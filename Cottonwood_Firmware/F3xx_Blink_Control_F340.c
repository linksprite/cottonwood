/*----------------------------------------------------------------------------- */
/* F3xx_Blink_Control_F340.c */
/*----------------------------------------------------------------------------- */
/* Copyright 2005 Silicon Laboratories, Inc. */
/* http://www.silabs.com */
/* */
/* Program Description: */
/* */
/* Contains functions that control the LEDs. */
/* */
/* */
/* How To Test:    See Readme.txt */
/* */
/* */
/* FID             3XX000003 */
/* Target:         C8051F32x/C8051F340 */
/* Tool chain:     Keil C51 7.50 / Keil EVAL C51 */
/*                 Silicon Laboratories IDE version 2.6 */
/* Command Line:   See Readme.txt */
/* Project Name:   F3xx_BlinkyExample */
/* */
/* Release 1.1 */
/*    -Added feature reports for dimming controls */
/*    -Added PCA dimmer functionality */
/*    -16 NOV 2006 */
/* Release 1.0 */
/*    -Initial Revision (PD) */
/*    -07 DEC 2005 */
/* */
/*----------------------------------------------------------------------------- */
/* Header Files */
/*----------------------------------------------------------------------------- */
#include "c8051F340.h"
/*#include "C8051F3xx.h" */
#include "F3xx_Blink_Control.h"
#include "F3xx_USB0_InterruptServiceRoutine.h"
#include "F3xx_USB0_Register.h"
#include "global.h"

/*----------------------------------------------------------------------------- */
/* Definitions */
/*----------------------------------------------------------------------------- */
#define SYSCLK             CLK    /* SYSCLK frequency in Hz */

/* USB clock selections (SFR CLKSEL) */
#define USB_4X_CLOCK       0x00        /* Select 4x clock multiplier, for USB */
#define USB_INT_OSC_DIV_2  0x10        /* Full Speed */
#define USB_EXT_OSC        0x20
#define USB_EXT_OSC_DIV_2  0x30
#define USB_EXT_OSC_DIV_3  0x40
#define USB_EXT_OSC_DIV_4  0x50

/* System clock selections (SFR CLKSEL) */
#define SYS_INT_OSC        0x00        /* Select to use internal oscillator */
#define SYS_EXT_OSC        0x01        /* Select to use an external oscillator */
#define SYS_4X_DIV_2       0x02
#define SYS_4X             0x03

/* Clock source selection /internal/external */
#define SYS_USE_CLK_INT         0x01
#define SYS_USE_CLK_EXT_12MHZ   0x02
#define SYS_USE_CLK_EXT_48MHZ   0x03
#define SYS_CLK_SELECT          SYS_USE_CLK_INT

sbit Led1 = P2^2;                      /* LED='1' means ON */
sbit Led2 = P2^3;

#define ON  1
#define OFF 0

/*----------------------------------------------------------------------------- */
/* 16-bit SFR Definitions for 'F32x */
/*----------------------------------------------------------------------------- */
sfr16 TMR2RL   = 0xca;                   /* Timer2 reload value */
sfr16 TMR2     = 0xcc;                   /* Timer2 counter */

/* ---------------------------------------------------------------------------- */
/* Global Variable Declarations */
/* ---------------------------------------------------------------------------- */
unsigned char BLINK_PATTERN[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
unsigned int xdata BLINK_RATE;
unsigned char xdata BLINK_SELECTOR;
unsigned char xdata BLINK_ENABLE;
unsigned char xdata BLINK_SELECTORUPDATE;
unsigned char xdata BLINK_LED1ACTIVE;
unsigned char xdata BLINK_LED2ACTIVE;
unsigned char xdata BLINK_DIMMER;
unsigned char xdata BLINK_DIMMER_SUCCESS;
/* ---------------------------------------------------------------------------- */
/* Local Function Prototypes */
/* ---------------------------------------------------------------------------- */
/*void Timer0_Init(void); */
void Sysclk_Init(void);
void Port_Init(void);
void Usb_Init(void);
void Timer2_Init(void);
void Adc_Init(void);
/*void Timer0_Init(void); */
void PCA_Init(void);
void Delay(void);

void Blink_Update(void);

/* ---------------------------------------------------------------------------- */
/* Global Variables */
/* ---------------------------------------------------------------------------- */
/* Last packet received from host */
unsigned char xdata OUT_PACKET[EP1_PACKET_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,/*}; */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
                                                  };

/* Next packet to send to host */
unsigned char xdata IN_PACKET[EP2_PACKET_SIZE]; /*/ = {0,0,0}; */

/*----------------------------------------------------------------------------- */
/* Interrupt Service Routines */
/*----------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------- */
/* Initialization Routines */
/* ---------------------------------------------------------------------------- */

/*----------------------------------------------------------------------------- */
/* System_Init (void) */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value - None */
/* Parameters - None */
/* */
/* This top-level initialization routine calls all support routine. */
/* */
/* ---------------------------------------------------------------------------- */
void System_Init(void)
{

    int i = 0;
/*   PCA0MD &= ~0x40;                    // Disable Watchdog timer */

#if (LEO || MICROREADER || ROGER || PICO)
    REG0CN &= 0x7F;
#endif

    Sysclk_Init();                      /* Initialize oscillator */
#if 0
    OSCXCN    = 0x20;
    OSCICN    = 0x83;

    CLKMUL    = 0x80;

    for (i = 0; i < 20; i++);    /* Wait 5us for initialization */
    CLKMUL    |= 0xC0;

    while ((CLKMUL & 0x20) == 0);
#endif
/* Please check Configuration on 48 Pin CLKIN is Port 0.7 On 32 Pin Packaqge it is P0.3 */
/*!!!!!! danger   CLKSEL    = 0x01; */

/*   Usb_Init();                         // Initialize USB0 */

}

/*----------------------------------------------------------------------------- */
/* Sysclk_Init */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value - None */
/* Parameters - None */
/* */
/* Initialize system clock to maximum frequency. */
/* */
/* ---------------------------------------------------------------------------- */
void Sysclk_Init(void)
{
#ifdef _USB_LOW_SPEED_

    OSCICN |= 0x03;                     /* Configure internal oscillator for */
    /* its maximum frequency and enable */
    /* missing clock detector */

    CLKSEL  = SYS_INT_OSC;              /* Select System clock */
    CLKSEL |= USB_INT_OSC_DIV_2;        /* Select USB clock */
#else

#if (SYS_CLK_SELECT == SYS_USE_CLK_INT)
    OSCICN |= 0x03;                     /* Configure internal oscillator for */
    /* its maximum frequency and enable */
    /* missing clock detector */

    CLKMUL  = 0x00;                     /* Select internal oscillator as */
    /* input to clock multiplier */

    CLKMUL |= 0x80;                     /* Enable clock multiplier */
    Delay();                            /* Delay for clock multiplier to begin */
    CLKMUL |= 0xC0;                     /* Initialize the clock multiplier */
    Delay();                            /* Delay for clock multiplier to begin */

    while (!(CLKMUL & 0x20));           /* Wait for multiplier to lock */
#if (SYSCLK == 48000000)
    FLSCL   = 0x90;                     /* Set flash waitstates for freqs > 25 MHz. */
    CLKSEL  = SYS_4X | USB_4X_CLOCK;    /* Set system clock to 48MHz */
#elif (SYSCLK == 24000000)
    CLKSEL  = SYS_4X_DIV_2 | USB_4X_CLOCK;    /* Set system clock to 24MHz */
#elif (SYSCLK == 12000000)
    CLKSEL  = SYS_INT_OSC  | USB_4X_CLOCK;    /* Set system clock to 24MHz */
#endif

#elif (SYS_CLK_SELECT == SYS_USE_CLK_EXT_12MHZ)
    int cnt;

    OSCXCN  = 0x20;                     /* set XOSCMD to 010b External CMOS Osc Mode */
    OSCXCN  |= 0x07;                    /* set XFCN to 111b Ext. Osc. Freq. */

    /* wait >= 1ms */
    for (cnt = 0; cnt < 20; cnt++)
    {
        Delay();                            /* Delay for external oscillator becoming stable */
    }

    CLKMUL  = 0x00;                     /* Reset clock multiplier */
    CLKMUL  = 0x01;                     /* Select external oscillator as */
    /* input to clock multiplier */

    CLKMUL |= 0x80;                     /* Enable clock multiplier */
    Delay();                            /* Delay for clock multiplier to begin */
    CLKMUL |= 0xC0;                     /* Initialize the clock multiplier */
    Delay();                            /* Delay for clock multiplier to begin */

    while (!(CLKMUL & 0x20));           /* Wait for multiplier to lock */
#if (SYSCLK == 48000000)
    FLSCL   = 0x90;                     /* Set flash waitstates for freqs > 25 MHz. */
    CLKSEL  = SYS_4X | USB_4X_CLOCK;    /* Set system clock to 48MHz */
#elif (SYSCLK == 24000000)
    CLKSEL  = SYS_4X_DIV_2 | USB_4X_CLOCK;    /* Set system clock to 24MHz */
#elif (SYSCLK == 12000000)
    CLKSEL  = SYS_INT_OSC  | USB_4X_CLOCK;    /* Set system clock to 24MHz */
#endif

    OSCICN &= ~0x80;                    /* explicitly switch off internal H-F oscillator */
    OSCLCN &= ~0x80;                    /* explicitly switch off internal L-F oscillator */


#elif (SYS_CLK_SELECT == SYS_USE_CLK_EXT_48MHZ)
    int cnt;

    OSCXCN  = 0x20;                     /* set XOSCMD to 010b External CMOS Osc Mode */
    OSCXCN  |= 0x07;                    /* set XFCN to 111b Ext. Osc. Freq. */

    /* wait >= 1ms */
    for (cnt = 0; cnt < 20; cnt++)
    {
        Delay();                            /* Delay for external oscillator becoming stable */
    }

    CLKMUL  = 0x00;                     /* Reset clock multiplier */
#if (SYSCLK == 48000000)
    FLSCL   = 0x90;                     /* Set flash waitstates for freqs > 25 MHz. */
    CLKSEL  = SYS_EXT_OSC | USB_EXT_OSC;    /* Set system clock and USB clock to external oscillator */
#elif (SYSCLK == 24000000)
    CLKSEL  = SYS_4X_DIV_2 | USB_4X_CLOCK;    /* Set system clock to 24MHz */
#elif (SYSCLK == 12000000)
    CLKSEL  = SYS_INT_OSC  | USB_4X_CLOCK;    /* Set system clock to 24MHz */
#endif

    OSCICN &= ~0x80;                    /* explicitly switch off internal H-F oscillator */
    OSCLCN &= ~0x80;                    /* explicitly switch off internal L-F oscillator */

#endif

#endif  /* _USB_LOW_SPEED_ */
}

/*----------------------------------------------------------------------------- */
/* USB0_Init */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value - None */
/* Parameters - None */
/* */
/* - Initialize USB0 */
/* - Enable USB0 interrupts */
/* - Enable USB0 transceiver */
/* - Enable USB0 with suspend detection */
/* */
/* ---------------------------------------------------------------------------- */
void Usb_Init(void)
{
    POLL_WRITE_BYTE(POWER,  0x08);      /* Force Asynchronous USB Reset */
    POLL_WRITE_BYTE(IN1IE,  0x07);      /* Enable Endpoint 0-2 in interrupts */
    POLL_WRITE_BYTE(OUT1IE, 0x07);      /* Enable Endpoint 0-2 out interrupts */
    POLL_WRITE_BYTE(CMIE,   0x07);      /* Enable Reset, Resume, and Suspend */
    /* interrupts */
#ifdef _USB_LOW_SPEED_
    USB0XCN = 0xC0;                     /* Enable transceiver; select low speed */
    POLL_WRITE_BYTE(CLKREC, 0xA0);      /* Enable clock recovery; single-step */
    /* mode disabled; low speed mode enabled */
#else
    USB0XCN = 0xE0;                     /* Enable transceiver; select full speed */
    POLL_WRITE_BYTE(CLKREC, 0x80);      /* Enable clock recovery, single-step */
    /* mode disabled */
#endif /* _USB_LOW_SPEED_ */

    EIE1 |= 0x02;                       /* Enable USB0 Interrupts */
    EA = 1;                             /* Global Interrupt enable */
    /* Enable USB0 by clearing the USB */
    /* Inhibit bit */
    POLL_WRITE_BYTE(POWER,  0x01);      /* and enable suspend detection */
}

/*----------------------------------------------------------------------------- */
/* Delay */
/*----------------------------------------------------------------------------- */
/* */
/* Return Value - None */
/* Parameters - None */
/* */
/* Used for a small pause, approximately 80 us in Full Speed, */
/* and 1 ms when clock is configured for Low Speed */
/* */
/* ---------------------------------------------------------------------------- */
void Delay(void)
{

    int x;
    for (x = 0;x < 500;x)
        x++;

}

