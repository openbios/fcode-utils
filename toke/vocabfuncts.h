#ifndef _TOKE_VOCABFUNCTS_H
#define _TOKE_VOCABFUNCTS_H

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
 *      External/Prototype definitions for Vocabulary functions
 *           in dictionary.c for Tokenizer
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */


#include "types.h"
#include "ticvocab.h"


/* ************************************************************************** *
 *
 *      Global Variables Exported
 *
 **************************************************************************** */

extern bool scope_is_global;
extern bool define_token;


/* ************************************************************************** *
 *
 *      Function Prototypes / Functions Exported:
 *
 **************************************************************************** */


tic_hdr_t *lookup_core_word( char *tname);
bool create_core_alias( char *new_name, char *old_name);

void enter_global_scope( void );
void resume_device_scope( void );

tic_hdr_t *lookup_current( char *name);
bool exists_in_current( char *tname);
tic_hdr_t *lookup_in_dev_node( char *tname);
void add_to_current( char *name,
                           TIC_P_DEFLT_TYPE fc_token,
			       fwtoken definer);
void hide_last_colon ( void );
void reveal_last_colon ( void );
bool create_current_alias( char *new_name, char *old_name );

void emit_token( const char *fc_name);
tic_hdr_t *lookup_token( char *tname);
bool entry_is_token( tic_hdr_t *test_entry );
void token_entry_warning( tic_hdr_t *t_entry);

tic_hdr_t *lookup_shared_word( char *tname);
tic_hdr_t *lookup_shared_f_exec_word( char *tname);

void init_dictionary( void );
void reset_normal_vocabs( void );
void reset_vocabs( void );


#endif   /*  _TOKE_VOCABFUNCTS_H    */
