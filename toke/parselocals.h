#ifndef _TOKE_LOCALX_H
#define _TOKE_LOCALX_H

/*
 *                     OpenBIOS - free your system!
 *                         ( FCode tokenizer )
 *
 *  This program is part of a free implementation of the IEEE 1275-1994
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2010 Stefan Reinauer
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
 *     Function Prototypes / External definitions for Parsing
 *          functions for IBM-style Local Values in Tokenizer
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

#include "ticvocab.h"

void declare_locals(bool ignoring);
tic_hdr_t *lookup_local(char *lname);
bool exists_as_local(char *stat_name);
bool create_local_alias(char *new_name, char *old_name);
void assign_local(void);
void finish_locals(void);
void forget_locals(void);

#endif /*  _TOKE_LOCALX_H    */
