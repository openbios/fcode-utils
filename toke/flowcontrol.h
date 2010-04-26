#ifndef _TOKE_FLOWCONTROL_H
#define _TOKE_FLOWCONTROL_H

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
 *      External Variables and Function Prototypes for Support Functions
 *          used in tokenizing FORTH Flow-Control structures.
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */


/* ************************************************************************** *
 *
 *      Global Variables Exported
 *
 **************************************************************************** */

extern int control_stack_depth;
 
/* ************************************************************************** *
 *
 *      Function Prototypes / Functions Exported:
 *
 **************************************************************************** */

void emit_if( void );
void emit_then( void );
void emit_else( void );
void emit_begin( void );
void emit_again( void );
void emit_until( void );
void emit_while( void );
void emit_repeat( void );
void mark_do( void );
void resolve_loop( void );
void emit_case( void );
void emit_of( void );
void emit_endof( void );
void emit_endcase( void );

void announce_control_structs( int severity, char *call_cond,
				          unsigned int abs_token_limit);
void clear_control_structs_to_limit( char *call_cond,
				          unsigned int abs_token_limit);
void clear_control_structs( char *call_cond);

#endif   /*  _TOKE_FLOWCONTROL_H    */
