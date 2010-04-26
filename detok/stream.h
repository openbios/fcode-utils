/*
 *                     OpenBIOS - free your system! 
 *                        ( FCode detokenizer )
 *                          
 *  stream.h - prototypes for fcode streaming functions. 
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

#ifndef _UTILS_DETOK_STREAM_H
#define _UTILS_DETOK_STREAM_H

#include "types.h"

/*  Prototypes for functions exported from  stream.c     */


int init_stream(char *name);
void close_stream(void);
bool more_to_go(void);

void adjust_for_pci_header(void);
void adjust_for_pci_filler(void);
void init_fcode_block(void);

int get_streampos(void);
void set_streampos(int pos);

u16 next_token(void);
u8 get_num8(void);
u16 get_num16(void);
u32 get_num32(void);
s16 get_offset(void);
u8 *get_string(u8 * len);
char *get_name(u8 * len);
u16 calc_checksum(void);

/*  External declarations for variables defined in   stream.c   */

extern unsigned int stream_max;
extern u8 *indata;
extern u8 *pc;
extern u8 *max;

#endif				/*  _UTILS_DETOK_STREAM_H    */
