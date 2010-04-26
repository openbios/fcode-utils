#ifndef _TOKE_TRACESYMS_H
#define _TOKE_TRACESYMS_H

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
 *      External/Prototype/Structure definitions for "Trace-Symbols" feature
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

#include "types.h"
#include "ticvocab.h"
#include "devnode.h"

/* ************************************************************************** *
 *
 *      Global Variables Exported
 *
 **************************************************************************** */

extern int split_alias_message;

/* ************************************************************************** *
 *
 *      Function Prototypes / Functions Exported:
 *
 **************************************************************************** */

void add_to_trace_list( char *trace_symb);
bool is_on_trace_list( char *symb_name);
void tracing_fcode( char *fc_phrase_buff, u16 fc_token_num);
void trace_creation( tic_hdr_t *trace_entry,
                         char *nu_name,
			     bool is_global);
void trace_create_failure( char *new_name, char *old_name, u16 fc_tokn);
void traced_name_error( char *trace_name);
void invoking_traced_name( tic_hdr_t *trace_entry);
void handle_invocation( tic_hdr_t *trace_entry);
void show_trace_list( void);
void trace_builtin( tic_hdr_t *trace_entry);

/* ************************************************************************** *
 *
 *      Macro:
 *          TRACING_FCODE_LENGTH
 *                Adequate length for the buffer passed to  tracing_fcode()
 *
 **************************************************************************** */

#define  TRACING_FCODE_LENGTH   32

#endif   /*  _TOKE_TRACESYMS_H    */
