/*
 *                     OpenBIOS - free your system!
 *                         ( FCode tokenizer )
 *
 *  This program is part of a free implementation of the IEEE 1275-1994
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2005 Stefan Reinauer, <stepan@openbios.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 *
 */

/* **************************************************************************
 *
 *      Functions to correlate PCI Device Class-Codes and PCI Code Types
 *      with their printable names.
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

#include <stdlib.h>
#include "types.h"

/* **************************************************************************
 *
 *      Functions Eported:
 *          pci_device_class_name
 *              Convert a numeric PCI Class Code to the Device Class Name
 *
 *          pci_code_type_name
 *              Convert a numeric PCI Code Type to a printable Name
 *
 **************************************************************************** */

const char *pci_device_class_name( u32 code);
const char *pci_code_type_name(u8 code);


typedef struct {
    const u32    classcode;
    const char  *classname;
} num_to_name_table ;


/* **************************************************************************
 *
 *      Function name:  convert_num_to_name
 *      Synopsis:   Find the Name, in the given table, for the given Number.
 *                  Support function for Exported Functions.
 *      
 *      Inputs:
 *          Parameters:     
 *              u32 num           Number to look up       
 *      num_to_name_table *table  Pointer to Table to scan        
 *              int max           Maximum Index of Table
 *            char *not_found     String to return if Number not in Table
 *
 *      Outputs:
 *          Returned Value: Pointer to string giving the Name
 *
 *      Error Detection:
 *          Unrecognized Number
 *              Return "not_found" string
 *
 *      Process Explanation:
 *          Scan the Table for a match.
 *
 *      Still to be done / Misc remarks:
 *          Had been considering a more sophisticated binary search,
 *          but the database is too small to merit the extra code.
 *          Stayed with the KISS principle...
 *      
 *
 **************************************************************************** */

static const char *convert_num_to_name(u32 num,
                                 num_to_name_table *table,
                                 int max,
                                 const char *not_found)
{
    int indx;
    const char *retval;

    retval = not_found;

    for (indx = 0;  indx < max ; indx++)
    {
        if ( num == table[indx].classcode )
        {
            retval = table[indx].classname ;
            break ;
        }
     }
     return ( retval );
}


/* **************************************************************************
 *
 *
 *          Structures:
 *      pci_code_type_name_table    Constant Data Table that correlates
 *                                      PCI Code Types with their Names.
 *      
 *      pci_dev_class_name_table    Constant Data Table that correlates
 *                                      Class Codes with their Class Names.
 *                                  This list of codes and names is current
 *                                      as of PCI Local Bus Specification
 *                                      Revision 3.0 dated February 3, 2004
 *
 **************************************************************************** */


static const num_to_name_table pci_code_type_name_table[] = {
    { 0 , "Intel x86" },
    { 1 , "Open Firmware" },
    { 2 , "HP PA Risc" },
    { 3 , "Intel EFI (unofficial)" }
};


static const num_to_name_table pci_dev_class_name_table[] = {
    { 0x000000 , "Legacy Device" },
    { 0x000100 , "VGA-Compatible Device" },

    { 0x010000 , "SCSI bus controller" },
 /* { 0x0101xx , "IDE controller" },    */
    { 0x010200 , "Floppy disk controller" },
    { 0x010300 , "IPI bus controller" },
    { 0x010400 , "RAID controller" },
    { 0x010520 , "ATA controller, single stepping" },
    { 0x010530 , "ATA controller, continuous" },
    { 0x010600 , "Serial ATA controller - vendor specific interface" },
    { 0x010601 , "Serial ATA controller - AHCI 1.0 interface" },
    { 0x010700 , "Serial Attached SCSI controller" },
    { 0x018000 , "Mass Storage controller" },

    { 0x020000 , "Ethernet controller"     },
    { 0x020100 , "Token Ring controller"   },
    { 0x020200 , "FDDI controller"         },
    { 0x020300 , "ATM controller"          },
    { 0x020400 , "ISDN controller" },
    { 0x020500 , "WorldFip controller" },
 /* { 0x0206xx , "PICMG 2.14 Multi Computing" },    */
    { 0x028000 , "Network controller"      },

    { 0x030000 , "VGA Display controller"  },
    { 0x030001 , "8514-compatible Display controller"  },
    { 0x030100 , "XGA Display controller"  },
    { 0x030200 , "3D Display controller"   },
    { 0x038000 , "Display controller"      },

    { 0x040000 , "Video device" },
    { 0x040100 , "Audio device" },
    { 0x040200 , "Computer Telephony device" },
    { 0x048000 , "Multimedia device" },

    { 0x050000 , "RAM memory controller" },
    { 0x050100 , "Flash memory controller" },
    { 0x058000 , "Memory controller" },

    { 0x060000 , "Host bridge" },
    { 0x060100 , "ISA bridge" },
    { 0x060200 , "EISA bridge" },
    { 0x060300 , "MCA bridge" },
    { 0x060400 , "PCI-to-PCI bridge" },
    { 0x060401 , "PCI-to-PCI bridge (subtractive decoding)" },
    { 0x060500 , "PCMCIA bridge" },
    { 0x060600 , "NuBus bridge" },
    { 0x060700 , "CardBus bridge" },
 /* { 0x0608xx , "RACEway bridge" },    */
    { 0x060940 , "PCI-to-PCI bridge, Semi-transparent, primary facing Host" },
    { 0x060980 , "PCI-to-PCI bridge, Semi-transparent, secondary facing Host" },
    { 0x060A00 , "InfiniBand-to-PCI host bridge" },
    { 0x068000 , "Bridge device" },

    { 0x070000 , "Generic XT-compatible serial controller" },
    { 0x070001 , "16450-compatible serial controller" },
    { 0x070002 , "16550-compatible serial controller" },
    { 0x070003 , "16650-compatible serial controller" },
    { 0x070004 , "16750-compatible serial controller" },
    { 0x070005 , "16850-compatible serial controller" },
    { 0x070006 , "16950-compatible serial controller" },

    { 0x070100 , "Parallel port" },
    { 0x070101 , "Bi-directional parallel port" },
    { 0x070102 , "ECP 1.X compliant parallel port" },
    { 0x070103 , "IEEE1284 controller" },
    { 0x0701FE , "IEEE1284 target device" },
    { 0x070200 , "Multiport serial controller" },

    { 0x070300 , "Generic modem" },
    { 0x070301 , "Hayes 16450-compatible modem" },
    { 0x070302 , "Hayes 16550-compatible modem" },
    { 0x070303 , "Hayes 16650-compatible modem" },
    { 0x070304 , "Hayes 16750-compatible modem" },
    { 0x070400 , "GPIB (IEEE 488.1/2) controller" },
    { 0x070500 , "Smart Card" },
    { 0x078000 , "Communications device" },

    { 0x080000 , "Generic 8259 PIC" },
    { 0x080001 , "ISA PIC" },
    { 0x080002 , "EISA PIC" },
    { 0x080010 , "I/O APIC interrupt controller" },
    { 0x080020 , "I/O(x) APIC interrupt controller" },

    { 0x080100 , "Generic 8237 DMA controller" },
    { 0x080101 , "ISA DMA controller" },
    { 0x080102 , "EISA DMA controller" },

    { 0x080200 , "Generic 8254 system timer" },
    { 0x080201 , "ISA system timer" },
    { 0x080202 , "EISA system timer-pair" },

    { 0x080300 , "Generic RTC controller" },
    { 0x080301 , "ISA RTC controller" },

    { 0x080400 , "Generic PCI Hot-Plug controller" },
    { 0x080500 , "SD Host controller" },
    { 0x088000 , "System peripheral" },

    { 0x090000 , "Keyboard controller" },
    { 0x090100 , "Digitizer (pen)" },
    { 0x090200 , "Mouse controller" },
    { 0x090300 , "Scanner controller" },
    { 0x090400 , "Generic Gameport controller" },
    { 0x090410 , "Legacy Gameport controller" },
    { 0x098000 , "Input controller" },

    { 0x0a0000 , "Generic docking station" },
    { 0x0a8000 , "Docking station" },

    { 0x0b0000 , "386 Processor" },
    { 0x0b0100 , "486 Processor" },
    { 0x0b0200 , "Pentium Processor" },
    { 0x0b1000 , "Alpha Processor" },
    { 0x0b2000 , "PowerPC Processor" },
    { 0x0b3000 , "MIPS Processor" },
    { 0x0b4000 , "Co-processor" },

    { 0x0c0000 , "IEEE 1394 (FireWire)" },
    { 0x0c0010 , "IEEE 1394 -- OpenHCI spec" },
    { 0x0c0100 , "ACCESS.bus" },
    { 0x0c0200 , "SSA" },
    { 0x0c0300 , "Universal Serial Bus (UHC spec)" },
    { 0x0c0310 , "Universal Serial Bus (Open Host spec)" },
    { 0x0c0320 , "USB2 Host controller (Intel Enhanced HCI spec)" },
    { 0x0c0380 , "Universal Serial Bus (no PI spec)" },
    { 0x0c03FE , "USB Target Device" },
    { 0x0c0400 , "Fibre Channel" },
    { 0x0c0500 , "System Management Bus" },
    { 0x0c0600 , "InfiniBand" },
    { 0x0c0700 , "IPMI SMIC Interface" },
    { 0x0c0701 , "IPMI Kybd Controller Style Interface" },
    { 0x0c0702 , "IPMI Block Transfer Interface" },
 /* { 0x0c08xx , "SERCOS Interface" },    */
    { 0x0c0900 , "CANbus" },
 
    { 0x0d100 , "iRDA compatible controller" },
    { 0x0d100 , "Consumer IR controller" },
    { 0x0d100 , "RF controller" },
    { 0x0d100 , "Bluetooth controller" },
    { 0x0d100 , "Broadband controller" },
    { 0x0d100 , "Ethernet (802.11a 5 GHz) controller" },
    { 0x0d100 , "Ethernet (802.11b 2.4 GHz) controller" },
    { 0x0d100 , "Wireless controller" },
     
 /* { 0x0e00xx , "I2O Intelligent I/O, spec 1.0" },    */
    { 0x0e0000 , "Message FIFO at offset 040h" },

    { 0x0f0100 , "TV satellite comm. controller" },
    { 0x0f0200 , "Audio satellite comm. controller" },
    { 0x0f0300 , "Voice satellite comm. controller" },
    { 0x0f0400 , "Data satellite comm. controller" },

    { 0x100000 , "Network and computing en/decryption" },
    { 0x101000 , "Entertainment en/decryption" },
    { 0x108000 , "En/Decryption" },

    { 0x110000 , "DPIO modules" },
    { 0x110100 , "Perf. counters" },
    { 0x111000 , "Comm. synch., time and freq. test" },
    { 0x112000 , "Management card" },
    { 0x118000 , "Data acq./Signal proc." },

};


/* **************************************************************************
 *
 *      PCI Class Code name calculations are further complicated by the
 *          fact that there are some combinations of Base Class and
 *          Sub-Class for which no specific register-level programming
 *          interfaces are defined, or, in other words, for which all
 *          possible numeric values of Programming Interface codes
 *          (i.e., the low-byte) are valid.
 *
 *     The following table lists the Base Class / Sub-Class pairs to
 *          accept without matching the Programming Interface, along
 *          with their names.
 *
 **************************************************************************** */


static const num_to_name_table pci_all_prg_intfcs_table[] = {
    { 0x0101 , "IDE controller" },
    { 0x0206 , "PICMG 2.14 Multi Computing" },
    { 0x0608 , "RACEway bridge" },
    { 0x0c08 , "SERCOS Interface" },
    { 0x0e00 , "I2O Intelligent I/O, spec 1.0" },
};


/* **************************************************************************
 *
 *      Function name:  pci_device_class_name
 *      Synopsis:   Return the Device Class Name for the given Class Code
 *      
 *      Inputs:
 *          Parameters:
 *                  u32 code        Numeric PCI Class Code
 *
 *      Outputs:
 *          Returned Value:  Pointer to string giving the Device Class Name
 *
 *      Error Detection:
 *          Unrecognized Class Code
 *              String returned is "unknown"
 *
 *      Process Explanation:
 *          Scan the pci_dev_class_name_table for a match.
 *          If one is not found there, drop the low (Programming Interface)
 *              byte of the code and scan the  pci_all_prg_intfcs_table
 *              for a match that way
 *          If you didn't find one there, you've exhausted your options...
 *
 **************************************************************************** */

const char *pci_device_class_name( u32 code)
{
    const int pdc_max_indx =
        sizeof(pci_dev_class_name_table)/sizeof(num_to_name_table) ;
    const char *result ;

    result = convert_num_to_name(
           code,
                (num_to_name_table *)pci_dev_class_name_table,
                        pdc_max_indx,
                         NULL);

    if ( result == NULL)
    {
        const int pallpi_max_indx =
            sizeof(pci_all_prg_intfcs_table)/sizeof(num_to_name_table) ;

	result = convert_num_to_name(
	    code>>8,
                (num_to_name_table *)pci_all_prg_intfcs_table,
                        pallpi_max_indx,
                         "unknown");
    }
    return ( result );
}


/* **************************************************************************
 *
 *      Function name:  pci_code_type_name
 *      Synopsis:   Return a printable Name for the given PCI Code Type
 *      
 *      Inputs:
 *          Parameters:
 *              u8 code          Numeric PCI Code Type
 *
 *      Outputs:
 *              Returned Value:  Pointer to string giving printable Name
 *
 *      Error Detection:
 *          Unrecognized PCI Code Type
 *              String returned is "unknown as of PCI specs 2.2"
 *
 *      Process Explanation:
 *          Scan the pci_code_type_name_table for a match.
 *
 *
 **************************************************************************** */

const char *pci_code_type_name(u8 code)
{
    const int pct_max_indx =
        sizeof(pci_code_type_name_table)/sizeof(num_to_name_table) ;
    const char *result ;

    result = convert_num_to_name(
          (u32)code,
                (num_to_name_table *)pci_code_type_name_table,
                        pct_max_indx,
                                "unknown as of PCI specs 2.2");
    return ( result );
}

