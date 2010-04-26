#ifndef _TOKE_NEXTFCODE_H
#define _TOKE_NEXTFCODE_H

/*
 *                     OpenBIOS - free your system!
 *                         ( FCode tokenizer )
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
 *
 *      Function Prototypes for Managing FCode Assignment Pointer 
 *         and for Detection of Overlapping-FCode Error in Tokenizer
 *
 *      (C) Copyright 2006 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

#include "types.h"


/* ************************************************************************** *
 *
 *      Global Variables Exported
 *
 **************************************************************************** */

extern u16  nextfcode;         /*  The next FCode-number to be assigned      */


/* ************************************************************************** *
 *
 *      Function Prototypes / Functions Exported:
 *
 **************************************************************************** */

void reset_fcode_ranges( void);
void list_fcode_ranges( bool final_tally);
void set_next_fcode( u16  new_fcode);
void assigning_fcode( void);
void bump_fcode( void);

/* **************************************************************************
 *
 *          Macros:
 *
 *          FCODE_START             Standard-specified starting number for
 *                                      user-generated FCodes.
 *
 *          FCODE_LIMIT             Standard-specified maximum number for
 *                                      FCodes of any origin.
 *
 *      I know these are not likely to change, but I still feel better
 *          making them named symbols, just on General Principles...
 *
 **************************************************************************** */

#define FCODE_START  0x0800
#define FCODE_LIMIT  0x0fff

#endif   /*  _TOKE_NEXTFCODE_H    */
