#ifndef _TOKE_EMIT_H
#define _TOKE_EMIT_H

/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  emit.h - prototypes for fcode emitters
 *  
 *  This program is part of a free implementation of the IEEE 1275-1994 
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2010 Stefan Reinauer <stepan@openbios.org>
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
 *         Modifications made in 2005 by IBM Corporation
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Modifications Author:  David L. Paktor    dlpaktor@us.ibm.com
 **************************************************************************** */
 
#include "types.h"

/* **************************************************************************
 *          Structure Name:    fcode_header_t
 *          Synopsis:          FCode Header within the Output Buffer
 *                            
 *   Fields:
 *       format, checksum and length    All as prescribed in P1275, Sec 5.2.2.5
 *       
 *       Note that the Checksum and Length fields are stated as byte-arrays
 *           rather than as integers.  This is intended to guarantee that
 *           their endian-ness will remain independent of the endian-ness
 *           of the host-platform on which this Tokenizer is running.
 *       Note also that, as integers, both are BIG-Endian.      
 *
 **************************************************************************** */

typedef struct {
    u8  format;
    u8  checksum[2];  /* [0] = Hi byte, [1] = Lo */
    u8  length[4];    /* [0] = Hi, [1] = Hi-mid, [2] = Lo-mid, [3] = Lo  */
} fcode_header_t;


/* **************************************************************************
 *          Macro Name:   STRING_LEN_MAX
 *                        Max number of bytes allowable in an output string
 *                            
 *   Arguments:           NONE
 *
 *       This value must remail hard-coded and immutable.  It represents the
 *           maximum number allowed in a FORTH counted-string's count-byte
 *           (which is, of course, the maximum number that can be expressed
 *           in an unsigned byte).
 *       It should not be used for anything else (e.g., buffer sizes), and
 *           most especially not for anything that might be changed!
 *
 **************************************************************************** */

#define STRING_LEN_MAX    255


/* **************************************************************************
 *          Macro Name:   GET_BUF_MAX
 *                        Size alloted to input-string buffer
 *                            
 *   Arguments:           NONE
 *
 *       This is a generous allotment for the buffer into which
 *           input strings are gathered.  Overflow calculations are
 *           also based on it.  It may be changed safely.
 *       We like to keep it a nice power-of-two to make the memory-
 *           allocation routine run efficiently and happily (Okay, so
 *           that's anthropormism:  It's efficient and *we*'re happy.
 *           Better?  I thought so...  ;-)
 *
 **************************************************************************** */

#define GET_BUF_MAX    1024


/* ************************************************************************** *
 *
 *      Global Variables Exported
 *
 **************************************************************************** */

extern unsigned int opc;
extern unsigned int pci_hdr_end_ob_off;

/* ************************************************************************** *
 *
 *      Function Prototypes / Functions Exported:
 *
 **************************************************************************** */

void  emit_fcode(u16 tok);
void  user_emit_byte(u8 data);

void  emit_offset(s16 offs);
void  emit_string(u8 *string, signed int cnt);
void  emit_fcodehdr(const char *starter_name);
void  finish_fcodehdr(void);
void  emit_pcihdr(void);
void  finish_pcihdr(void);
void finish_headers(void);
void  emit_end0(void);
void  emit_literal(u32 num);

#endif   /* _TOKE_EMIT_H */
