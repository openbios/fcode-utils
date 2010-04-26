#ifndef _UTILS_DETOK_DETOK_H
#define _UTILS_DETOK_DETOK_H

/*
 *                     OpenBIOS - free your system! 
 *                        ( FCode detokenizer )
 *                          
 *  detok.h - detokenizer base macros.  
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

/*  Structure of an entry in a token-table
 *  Consists of:
 *      (1)  Name of the token
 *      (2)  FCode of the token
 *      (3)  Link-pointer to previous entry.
 */

typedef struct token {
	char *name;
	u16 fcode;
	struct token *prev;
} token_t;

/*  Macro for creating an entry in a token-table data-array  */
#define TOKEN_ENTRY(num, name)   { name, (u16)num, (token_t *)NULL }


/*  Prototypes for functions exported from
 *   detok.c  decode.c  printformats.c  pcihdr.c  and  dictionary.c
 */

void link_token(token_t *curr_token);
void add_token(u16 number, char *name);
void init_dictionary(void);
void reset_dictionary(void);
void freeze_dictionary(void);
char *lookup_token(u16 number);

void detokenize(void);

void printremark(char *str);

int handle_pci_header(u8 * data_ptr);
void handle_pci_filler(u8 * filler_ptr);


/*  External declarations for variables defined in or used by
 *   detok.c  decode.c  printformats.c  pcihdr.c  and  dictionary.c
 */
extern bool verbose;
extern bool decode_all;
extern bool show_linenumbers;
extern bool show_offsets;

extern bool check_tok_seq;

extern u16 fcode;
extern bool offs16;
extern bool end_found;
extern unsigned int linenum;

extern u8 *pci_image_end;
extern unsigned int token_streampos;
extern u16 last_defined_token;

#endif				/*  _UTILS_DETOK_DETOK_H    */
