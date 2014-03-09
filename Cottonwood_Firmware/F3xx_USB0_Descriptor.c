/*----------------------------------------------------------------------------- */
/* F3xx_USB0_Descriptor.c */
/*----------------------------------------------------------------------------- */
/* Copyright 2005 Silicon Laboratories, Inc. */
/* http://www.silabs.com */
/* */
/* Program Description: */
/* */
/* Source file for USB firmware. Includes descriptor data. */
/* */
/* */
/* How To Test:    See Readme.txt */
/* */
/* */
/* FID:            3XX000004 */
/* Target:         C8051F32x/C8051F340 */
/* Tool chain:     Keil C51 7.50 / Keil EVAL C51 */
/*                 Silicon Laboratories IDE version 2.6 */
/* Command Line:   See Readme.txt */
/* Project Name:   F3xx_BlinkyExample */
/* */
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
/* Includes */
/*----------------------------------------------------------------------------- */
#include "F3xx_USB0_Register.h"
#include "F3xx_USB0_InterruptServiceRoutine.h"
#include "F3xx_USB0_Descriptor.h"
#include "F3xx_Blink_Control.h"
#include "usb_commands.h"
/*----------------------------------------------------------------------------- */
/* Descriptor Declarations */
/*----------------------------------------------------------------------------- */

code const device_descriptor DEVICEDESC =
{
    18,                                 /* bLength */
    0x01,                               /* bDescriptorType */
    0x1001,                             /* bcdUSB */
    0x00,                               /* bDeviceClass */
    0x00,                               /* bDeviceSubClass */
    0x00,                               /* bDeviceProtocol */
    EP0_PACKET_SIZE,                    /* bMaxPacketSize0 */
    0x2513,                             /* idVendor for ams */
    0x29C0,                             /* idProduct for LEO  board */
    0x0000,                             /* bcdDevice */
    0x01,                               /* iManufacturer */
    0x02,                               /* iProduct */
    0x00,                               /* iSerialNumber */
    0x01                                /* bNumConfigurations */
}; /*end of DEVICEDESC */

/* From "USB Device Class Definition for Human Interface Devices (HID)". */
/* Section 7.1: */
/* "When a Get_Descriptor(Configuration) request is issued, */
/* it returns the Configuration descriptor, all Interface descriptors, */
/* all Endpoint descriptors, and the HID descriptor for each interface." */
code const hid_configuration_descriptor HIDCONFIGDESC =
{

    { /* configuration_descriptor hid_configuration_descriptor */
        0x09,                               /* Length */
        0x02,                               /* Type */
        0x2900,                             /* Totallength (= 9+9+9+7+7) */
        0x01,                               /* NumInterfaces */
        0x01,                               /* bConfigurationValue */
        0x00,                               /* iConfiguration */
        0x80,                               /* bmAttributes */
        /*0x20                                // MaxPower (in 2mA units) */
        0xAF                                /*350mA */
    },

    { /* interface_descriptor hid_interface_descriptor */
        0x09,                               /* bLength */
        0x04,                               /* bDescriptorType */
        0x00,                               /* bInterfaceNumber */
        0x00,                               /* bAlternateSetting */
        0x02, /*0x01                              // bNumEndpoints */
        0x03,                               /* bInterfaceClass (3 = HID) */
        0x00,                               /* bInterfaceSubClass */
        0x00,                               /* bInterfaceProcotol */
        0x00                                /* iInterface */
    },

    { /* class_descriptor hid_descriptor */
        0x09,	                              /* bLength */
        0x21,	                              /* bDescriptorType */
        0x0101,	                           /* bcdHID */
        0x00,	                              /* bCountryCode */
        0x01,	                              /* bNumDescriptors */
        0x22,                               /* bDescriptorType */
        HID_REPORT_DESCRIPTOR_SIZE_LE       /* wItemLength (tot. len. of report */
        /* descriptor) */
    },

/* IN endpoint (mandatory for HID) */
    { /* endpoint_descriptor hid_endpoint_in_descriptor */
        0x07,                               /* bLength */
        0x05,                               /* bDescriptorType */
        0x81,                               /* bEndpointAddress */
        0x03,                               /* bmAttributes */
        EP1_PACKET_SIZE_LE,                 /* MaxPacketSize (LITTLE ENDIAN) */
        1                                   /* bInterval */
    },

/* OUT endpoint (optional for HID) */
    { /* endpoint_descriptor hid_endpoint_out_descriptor */
        0x07,                               /* bLength */
        0x05,                               /* bDescriptorType */
        0x02,                               /* bEndpointAddress */
        0x03,                               /* bmAttributes */
        EP2_PACKET_SIZE_LE,                 /* MaxPacketSize (LITTLE ENDIAN) */
        1                                   /* bInterval */
    }

};

#define HID_REPORT_DESC_DIR_OUT 0x91
#define HID_REPORT_DESC_DIR_IN  0x81
#define HID_REPORT_DESC_FEATURE 0xd1

/*! convenient macros to add entries to the HIDREPORTDESC table */
#define HID_REPORT_DESC_ENTRY(ID, SIZE, DIRECTION) \
   0x85, ID,         /* Report ID             */ \
   0x95, SIZE,       /* REPORT_COUNT ()       */ \
   0x75, 0x08,       /* REPORT_SIZE (8)       */ \
   0x26, 0xff, 0x00, /* LOGICAL_MAXIMUM (255) */ \
   0x15, 0x00,       /* LOGICAL_MINIMUM (0)   */ \
   0x09, 0x01,       /* USAGE (Vendor Usage 1)*/ \
   DIRECTION, 0x02   /* OUTPUT (Data,Var,Abs) */

code const hid_report_descriptor HIDREPORTDESC =
{
    0x06, 0x00, 0xff,              /* USAGE_PAGE (Vendor Defined Page 1) */
    0x09, 0x01,                    /* USAGE (Vendor Usage 1) */
    0xa1, 0x01,                    /* COLLECTION (Application) */
    
    HID_REPORT_DESC_ENTRY(OUT_FIRM_HARDW_ID, OUT_FIRM_HARDW_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_FIRM_HARDW_ID, IN_FIRM_HARDW_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_INVENTORY_ID, OUT_INVENTORY_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_INVENTORY_ID, IN_INVENTORY_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_SELECT_TAG_ID, OUT_SELECT_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_SELECT_TAG_ID, IN_SELECT_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_UPDATE_POWER_ID, OUT_UPDATE_POWER_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_UPDATE_POWER_ID, IN_UPDATE_POWER_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_ANTENNA_POWER_ID, OUT_ANTENNA_POWER_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_ANTENNA_POWER_ID, IN_ANTENNA_POWER_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_WRITE_TO_TAG_ID, OUT_WRITE_TO_TAG_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_WRITE_TO_TAG_ID, IN_WRITE_TO_TAG_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_READ_FROM_TAG_ID, OUT_READ_FROM_TAG_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_READ_FROM_TAG_ID, IN_READ_FROM_TAG_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_READ_REG_ID, OUT_READ_REG_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_READ_REG_ID, IN_READ_REG_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_WRITE_REG_ID, OUT_WRITE_REG_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_WRITE_REG_ID, IN_WRITE_REG_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_INVENTORY_6B_ID, OUT_INVENTORY_6B_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_INVENTORY_6B_ID, IN_INVENTORY_6B_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_WRITE_TO_TAG_6B_ID, OUT_WRITE_TO_TAG_6B_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_WRITE_TO_TAG_6B_ID, IN_WRITE_TO_TAG_6B_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_READ_FROM_TAG_6B_ID, OUT_READ_FROM_TAG_6B_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_READ_FROM_TAG_6B_ID, IN_READ_FROM_TAG_6B_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_AUTHENTICATE_ID, OUT_AUTHENTICATE_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_AUTHENTICATE_ID, IN_AUTHENTICATE_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_CHALLENGE_ID, OUT_CHALLENGE_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_CHALLENGE_ID, IN_CHALLENGE_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_READ_BUFFER_ID, OUT_READ_BUFFER_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_READ_BUFFER_ID, IN_READ_BUFFER_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_CHANGE_FREQ_ID, OUT_CHANGE_FREQ_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_CHANGE_FREQ_ID, IN_CHANGE_FREQ_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_INVENTORY_RSSI_ID, OUT_INVENTORY_RSSI_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_INVENTORY_RSSI_ID, IN_INVENTORY_RSSI_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_LOCK_UNLOCK_ID, OUT_LOCK_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_KILL_TAG_ID, IN_KILL_TAG_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_KILL_TAG_ID, OUT_KILL_TAG_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_LOCK_UNLOCK_ID, IN_LOCK_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_NXP_COMMAND_ID, OUT_NXP_COMMAND_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_NXP_COMMAND_ID, IN_NXP_COMMAND_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_FIRM_PROGRAM_ID, OUT_FIRM_PROGRAM_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_FIRM_PROGRAM_ID, IN_FIRM_PROGRAM_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(IN_REGS_COMPLETE_ID, IN_REGS_COMPLETE_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_REGS_COMPLETE_ID, OUT_REGS_COMPLETE_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_GEN2_SETTINGS_ID, IN_GEN2_SETTINGS_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_GEN2_SETTINGS_ID, OUT_GEN2_SETTINGS_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_TUNER_SETTINGS_ID, IN_TUNER_SETTINGS_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_TUNER_SETTINGS_ID, OUT_TUNER_SETTINGS_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_START_STOP_ID, IN_START_STOP_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_START_STOP_ID, OUT_START_STOP_IDSize, HID_REPORT_DESC_DIR_OUT),
    HID_REPORT_DESC_ENTRY(IN_GENERIC_COMMAND_ID, IN_GENERIC_COMMAND_IDSize, HID_REPORT_DESC_DIR_IN),
    HID_REPORT_DESC_ENTRY(OUT_GENERIC_COMMAND_ID, OUT_GENERIC_COMMAND_IDSize, HID_REPORT_DESC_DIR_OUT),
    0xC0                           /*   end Application Collection */
};

#define STR0LEN 5

code const unsigned char String0Desc [STR0LEN] =
{
    STR0LEN, 0x03, 0x09, 0x04, 0x00
}; /* End of String0Desc */

#define STR1LEN sizeof ("AMS ") * 2

code const unsigned char String1Desc [STR1LEN+2] =
{
    STR1LEN, 0x03,
    'a', 0,
    'm', 0,
    's', 0,
    ' ', 0
}; /* End of String1Desc */

#define STR2LEN sizeof ("AS3990") * 2

code const unsigned char String2Desc [STR2LEN+2] =
{
    STR2LEN, 0x03,
    'A', 0,
    'S', 0,
    '3', 0,
    '9', 0,
    '9', 0,
    'x', 0
}; /* End of String2Desc */

unsigned char* const STRINGDESCTABLE [] =
{
    String0Desc,
    String1Desc,
    String2Desc
};
