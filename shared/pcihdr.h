#ifndef _PCIHDR_H
#define _PCIHDR_H
/*
 *                     OpenBIOS - free your system!
 *                            ( PCI headers )
 *
 *  This program is part of a free implementation of the IEEE 1275-1994
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2005 Stefan Reinauer, <stepan@openbios.org>
 *  Copyright (C) 2006 coresystems GmbH <info@coresystems.de>
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
 *      PCI Header and PCI Data Structures, as defined in
 *              PCI FIRMWARE SPECIFICATION, REV. 3.
 *      taken from openbios/utils/romheaders/romheaders.c
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Structures:
 *          rom_header_t        Type definition for a PCI ROM Header
 *          pci_data_t          Type definition for a PCI Data Header
 *      
 *
 *      Macros:
 *          PCI_DATA_HDR                 Construct the PCI Data Header signature
 *                                           as an integer.  Use this to create
 *                                           a constant, in order to allow its
 *                                           value to be calculated at compile-
 *                                           -time rather than at run-time.
 *          BIG_ENDIAN_WORD_FETCH        Fetch a big-endian word from 2
 *                                           unsigned chars in sequence.
 *          LITTLE_ENDIAN_WORD_FETCH     ...... little-endian word from 2 ....
 *          BIG_ENDIAN_LONG_FETCH        ...... big-endian long from 4 ...
 *          LITTLE_ENDIAN_LONG_FETCH     ...... little-endian long from 4 ....
 *          LITTLE_ENDIAN_TRIPLET_FETCH  ...... little-endian triplet from 3 ...
 *          CLASS_CODE_FETCH             Special fetch for Class-Code triplet
 *          BIG_ENDIAN_WORD_STORE        Store an integer into 2 unsigned
 *                                           chars in big-endian sequence
 *          LITTLE_ENDIAN_WORD_STORE     ..... into 2 ... little-endian ...
 *          BIG_ENDIAN_LONG_STORE        ..... into 4 ... big-endian  ... 
 *          LITTLE_ENDIAN_LONG_STORE     ..... into 4 ... little-endian ...
 *          LITTLE_ENDIAN_TRIPLET_STORE  .....   3 ... little-endian triplet ...
 *          CLASS_CODE_STORE             Special store for Class-Code triplet
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Still to be done:
 *          The "Endian" macros are beginning to appear to be more
 *              generally useful that previously thought, and should,
 *              perhaps, be moved into types.h
 *
 **************************************************************************** */

#include "types.h"
/*  Construct the PCI Data Header signature as an integer   */
#define PCI_DATA_HDR (u32) ( ('P' << 24) | ('C' << 16) | ('I' << 8) | 'R' )

#ifndef bswap_16
#define bswap_16(x) \
	((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#endif

#define little_word(x)  bswap_16(x)

/*  These are mnemonics for the programmer.  The compiler doesn't care...   */
#define le_u16(x) u8  x[2]
#define be_u16(x) u8  x[2]
#define le_u32(x) u8  x[4]
#define be_u32(x) u8  x[4]

/*  Fetch a big-endian word from 2 unsigned chars in sequence  */
#define BIG_ENDIAN_WORD_FETCH(x)   (u16)( (x[0] << 8) | x[1] )

/*  Fetch a little-endian word from 2 unsigned chars in sequence  */
#define LITTLE_ENDIAN_WORD_FETCH(x)   (u16)( (x[1] << 8) | x[0] )

/*  Fetch a big-endian long from 4 unsigned chars in sequence  */
#define BIG_ENDIAN_LONG_FETCH(x)   (u32)( (x[0] << 24) | (x[1] << 16) | (x[2] << 8) | x[3] )

/*  Fetch a little-endian long from 4 unsigned chars in sequence  */
#define LITTLE_ENDIAN_LONG_FETCH(x)   (u32)( (x[3] << 24) | (x[2] << 16) | (x[1] << 8) | x[0] )

#define LE_U24(x) u8  x[3]
/*  Fetch a little-endian triplet from 3 unsigned chars in sequence  */
#define LITTLE_ENDIAN_TRIPLET_FETCH(x)   (u32)( (x[2] << 16) | (x[1] << 8) | x[0] )


/*  Special case for Class-Code triplet:  lo, mid, hi  */
#define class_code_u24(x) u8  x[3]
/*  Special fetch for Class-Code triplet  */
#define CLASS_CODE_FETCH(x)   (u32)( (x[2] << 16) | (x[1] << 8) | x[0] )
/*  Special store for Class-Code triplet  */
#define CLASS_CODE_STORE(dest,x)    \
    dest[2] = (u8)( x >> 16); dest[1] = (u8)(x >>  8);  dest[0] = (u8)x;
    /*  NOTE   Class-Code triplet is the same as  little-endian triplet  */
    /*   Change over some time, eh?   */

/* Store an integer into 2 unsigned chars in big-endian sequence */
#define BIG_ENDIAN_WORD_STORE(dest,x)  dest[0] = (u8)(x >> 8); dest[1]=(u8)x;

/* Store an integer into 2 unsigned chars in little-endian sequence */
#define LITTLE_ENDIAN_WORD_STORE(dest,x)  dest[1] = (u8)(x >> 8); dest[0]=(u8)x;

/* Store an integer into 4 unsigned chars in big-endian sequence */
#define BIG_ENDIAN_LONG_STORE(dest,x)    \
    dest[0] = (u8)(x >> 24); dest[1] =(u8)( x >> 16);   \
    dest[2] = (u8)(x >>  8); dest[3] =(u8)x;

/* Store an integer into 4 unsigned chars in little-endian sequence */
#define LITTLE_ENDIAN_LONG_STORE(dest,x)    \
    dest[3] = (u8)(x >> 24); dest[2] =(u8)( x >> 16);   \
    dest[1] = (u8)(x >>  8); dest[0] =(u8)x;

/* Store an integer into 3 unsigned chars in little-endian triplet sequence */
#define LITTLE_ENDIAN_TRIPLET_STORE(dest,x)    \
    dest[2] = (u8)( x >> 16); dest[1] = (u8)(x >>  8);  dest[0] = (u8)x;
  

typedef struct {
    be_u16(signature);
    u8	reserved[0x16];
    le_u16(data_ptr);
    le_u16(padd);
} rom_header_t;


typedef struct {
    be_u32	(signature);
    le_u16	(vendor);
    le_u16	(device);
    le_u16	(vpd);
    le_u16	(dlen);
    u8	 drevision;
    class_code_u24	(class_code);
    le_u16	(ilen);
    le_u16	(irevision);
    u8	code_type;
    u8	last_image_flag;
    u16	reserved_2;
} pci_data_t;

#define PCI_DATA_STRUCT_REV  0

/*  Prototypes for functions exported from  devsupp/pci/classcodes.c   */
char *pci_device_class_name( u32 code);
char *pci_code_type_name(u8 code);



#endif   /*  _PCIHDR_H   */
