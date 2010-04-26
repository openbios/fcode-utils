#ifndef _TOKE_MACROS_H
#define _TOKE_MACROS_H

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
 *      Prototypes for functions that operate on the  MACROS vocabulary,
 *          which is implemented as a String-Substitution-type vocab.
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */


#include "types.h"
#include "ticvocab.h"

/* ************************************************************************** *
 *
 *      Function Prototypes / Functions Exported:
 *
 **************************************************************************** */

void init_macros( tic_hdr_t **tic_vocab_ptr );
void add_user_macro( void);
void skip_user_macro( tic_bool_param_t pfield );
#if  0  /*  What's this doing here?  */
char *lookup_macro(char *name);
bool exists_as_macro(char *name);
bool create_macro_alias( char *new_name, char *old_name );
void reset_macros_vocab( void );
#endif  /*  What's this doing here?  */

#endif   /*  _TOKE_MACROS_H    */
