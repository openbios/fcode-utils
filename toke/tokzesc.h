#ifndef _TOKE_TOKZESC_H
#define _TOKE_TOKZESC_H

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
 *      External and Prototype definitions for functions that process
 *          activity in "Tokenizer Escape" mode
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

#include "types.h"
#include "ticvocab.h"

void init_tokz_esc_vocab( void );
bool create_tokz_esc_alias(char *new_name, char *old_name);
void enter_tokz_esc( void );
tic_hdr_t *lookup_tokz_esc(char *name);
void reset_tokz_esc( void );
void pop_next_fcode( void);

#endif   /*  _TOKE_TOKZESC_H    */
