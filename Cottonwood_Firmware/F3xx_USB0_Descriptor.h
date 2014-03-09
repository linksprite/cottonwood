/*----------------------------------------------------------------------------- */
/* F3xx_USB0_Descriptor.h */
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
/* FID             3XX000011 */
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

#ifndef  _USB_DESC_H_
#define  _USB_DESC_H_

/* WORD type definition, for KEIL Compiler */
#ifndef _WORD_DEF_                     /* Compiler Specific, written for */
/* Little Endian */
#define _WORD_DEF_
typedef union {unsigned int i;
    unsigned char c[2];
} WORD;
#define LSB 1                          /* All words sent to and received */
/* from the host are */
#define MSB 0                          /* little endian, this is switched */
/* by software when neccessary. */
/* These sections of code have been */
/* marked with "Compiler Specific" */
/* as above for easier modification */
#endif   /* _WORD_DEF_ */

/*------------------------------------------ */
/* Standard Device Descriptor Type Defintion */
/*------------------------------------------ */
typedef /*code*/ struct
{
    unsigned char bLength;              /* Size of this Descriptor in Bytes */
    unsigned char bDescriptorType;      /* Descriptor Type (=1) */
    WORD bcdUSB;                        /* USB Spec Release Number in BCD */
    unsigned char bDeviceClass;         /* Device Class Code */
    unsigned char bDeviceSubClass;      /* Device Subclass Code */
    unsigned char bDeviceProtocol;      /* Device Protocol Code */
    unsigned char bMaxPacketSize0;      /* Maximum Packet Size for EP0 */
    WORD idVendor;                      /* Vendor ID */
    WORD idProduct;                     /* Product ID */
    WORD bcdDevice;                     /* Device Release Number in BCD */
    unsigned char iManufacturer;        /* Index of String Desc for Manufacturer */
    unsigned char iProduct;             /* Index of String Desc for Product */
    unsigned char iSerialNumber;        /* Index of String Desc for SerNo */
    unsigned char bNumConfigurations;   /* Number of possible Configurations */
} device_descriptor;                   /* End of Device Descriptor Type */

/*-------------------------------------------------- */
/* Standard Configuration Descriptor Type Definition */
/*-------------------------------------------------- */
typedef /*code*/ struct
{
    unsigned char bLength;              /* Size of this Descriptor in Bytes */
    unsigned char bDescriptorType;      /* Descriptor Type (=2) */
    WORD wTotalLength;                  /* Total Length of Data for this Conf */
    unsigned char bNumInterfaces;       /* No of Interfaces supported by this */
    /* Conf */
    unsigned char bConfigurationValue;  /* Designator Value for *this* */
    /* Configuration */
    unsigned char iConfiguration;       /* Index of String Desc for this Conf */
    unsigned char bmAttributes;         /* Configuration Characteristics (see below) */
    unsigned char bMaxPower;            /* Max. Power Consumption in this */
    /* Conf (*2mA) */
} configuration_descriptor;            /* End of Configuration Descriptor Type */

/*---------------------------------------------- */
/* Standard Interface Descriptor Type Definition */
/*---------------------------------------------- */
typedef /*code*/ struct
{
    unsigned char bLength;              /* Size of this Descriptor in Bytes */
    unsigned char bDescriptorType;      /* Descriptor Type (=4) */
    unsigned char bInterfaceNumber;     /* Number of *this* Interface (0..) */
    unsigned char bAlternateSetting;    /* Alternative for this Interface (if any) */
    unsigned char bNumEndpoints;        /* No of EPs used by this IF (excl. EP0) */
    unsigned char bInterfaceClass;      /* Interface Class Code */
    unsigned char bInterfaceSubClass;   /* Interface Subclass Code */
    unsigned char bInterfaceProtocol;   /* Interface Protocol Code */
    unsigned char iInterface;           /* Index of String Desc for this Interface */
} interface_descriptor;                /* End of Interface Descriptor Type */

/*------------------------------------------ */
/* Standard Class Descriptor Type Definition */
/*------------------------------------------ */
typedef /*code */struct
{
    unsigned char bLength;              /* Size of this Descriptor in Bytes (=9) */
    unsigned char bDescriptorType;      /* Descriptor Type (HID=0x21) */
    WORD bcdHID;    				         /* HID Class Specification */
    /* release number (=1.01) */
    unsigned char bCountryCode;         /* Localized country code */
    unsigned char bNumDescriptors;	   /* Number of class descriptors to follow */
    unsigned char bReportDescriptorType;/* Report descriptor type (HID=0x22) */
    unsigned int wItemLength;			   /* Total length of report descriptor table */
} class_descriptor;                    /* End of Class Descriptor Type */

/*--------------------------------------------- */
/* Standard Endpoint Descriptor Type Definition */
/*--------------------------------------------- */
typedef /*code*/ struct
{
    unsigned char bLength;              /* Size of this Descriptor in Bytes */
    unsigned char bDescriptorType;      /* Descriptor Type (=5) */
    unsigned char bEndpointAddress;     /* Endpoint Address (Number + Direction) */
    unsigned char bmAttributes;         /* Endpoint Attributes (Transfer Type) */
    WORD wMaxPacketSize;	               /* Max. Endpoint Packet Size */
    unsigned char bInterval;            /* Polling Interval (Interrupt) ms */
} endpoint_descriptor;                 /* End of Endpoint Descriptor Type */

/*--------------------------------------------- */
/* HID Configuration Descriptor Type Definition */
/*--------------------------------------------- */
/* From "USB Device Class Definition for Human Interface Devices (HID)". */
/* Section 7.1: */
/* "When a Get_Descriptor(Configuration) request is issued, */
/* it returns the Configuration descriptor, all Interface descriptors, */
/* all Endpoint descriptors, and the HID descriptor for each interface." */
typedef code struct {
    configuration_descriptor 	hid_configuration_descriptor;
    interface_descriptor 		hid_interface_descriptor;
    class_descriptor 			hid_descriptor;
    endpoint_descriptor 		hid_endpoint_in_descriptor;
    endpoint_descriptor 		hid_endpoint_out_descriptor;
}
hid_configuration_descriptor;

#define HID_REPORT_DESC_ENTRY_SIZE 15

/*! number of report descriptor entries. This number needs to be updated if new
   entries are added */
#define HID_REPORT_DESC_NUM_ENTRIES 52

#define HID_REPORT_DESCRIPTOR_SIZE ((HID_REPORT_DESC_NUM_ENTRIES * HID_REPORT_DESC_ENTRY_SIZE) + 8)
#define HID_REPORT_DESCRIPTOR_SIZE_LE (((HID_REPORT_DESCRIPTOR_SIZE >> 8) & 0xff) | \
                        ((HID_REPORT_DESCRIPTOR_SIZE << 8) & 0xff00))

typedef code unsigned char hid_report_descriptor[HID_REPORT_DESCRIPTOR_SIZE];

/*----------------------------- */
/* SETUP Packet Type Definition */
/*----------------------------- */
typedef struct
{
    unsigned char bmRequestType;        /* Request recipient, type, and dir. */
    unsigned char bRequest;             /* Specific standard request number */
    WORD wValue;                        /* varies according to request */
    WORD wIndex;                        /* varies according to request */
    WORD wLength;                       /* Number of bytes to transfer */
} setup_buffer;                        /* End of SETUP Packet Type */

#endif  /* _USB_DESC_H_ */
