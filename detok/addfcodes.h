/*
 *                     OpenBIOS - free your system!
 *                        ( FCode detokenizer )
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
 *      Header for function for entering Vendor-Specific FCodes
 *          to detokenizer.
 *
 *      (C) Copyright 2006 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

#ifndef _DETOK_VSFCODES_H
#define _DETOK_VSFCODES_H

#include "types.h"

/* ************************************************************************** *
 *
 *      Global Variables Exported
 *
 **************************************************************************** */

/*  For "special function" identification  */
extern u16 *double_lit_code;


/* ************************************************************************** *
 *
 *      Function Prototypes / Functions Exported:
 *
 **************************************************************************** */

bool add_fcodes_from_list(char *vf_file_name);

#endif				/*  _DETOK_VSFCODES_H    */
