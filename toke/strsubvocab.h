#ifndef _TOKE_STRSUBVOCAB_H
#define _TOKE_STRSUBVOCAB_H


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
 *      Headers, general-purpose support structures, function prototypes
 *          and macros for String-Substitution-type vocabularies.
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Structures:
 *          str_sub_vocab_t        Entry in a String-Substitution-type vocab 
 *
 *      Macros:
 *          BUILTIN_STR_SUB        Add an entry to the initial Str-Sub vocab.
 *
 **************************************************************************** */

#include "types.h"


typedef struct str_sub_vocab {
	u8  *name;
	u8  *alias;
	struct str_sub_vocab *next;
} str_sub_vocab_t;


void add_str_sub_entry( char *ename,
                        	    char *subst_str,
			        	str_sub_vocab_t **str_sub_vocab );
str_sub_vocab_t *lookup_str_sub( char *tname, str_sub_vocab_t *str_sub_vocab );
bool exists_in_str_sub( char *tname, str_sub_vocab_t *str_sub_vocab );


#endif   /*  _TOKE_STRSUBVOCAB_H    */
