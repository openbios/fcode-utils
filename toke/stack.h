#ifndef _TOKE_STACK_H
#define _TOKE_STACK_H

/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  stack.h - prototypes and defines for handling the stacks.  
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

/* ************************************************************************** *
 *
 *      Macros:
 *          MAX_ELEMENTS          Size of stack area, in "elements"
 *
 **************************************************************************** */

#define MAX_ELEMENTS 1024

/* ************************************************************************** *
 *
 *      Global Variables Exported
 *
 **************************************************************************** */

extern long *dstack;

/* ************************************************************************** *
 *
 *      Function Prototypes / Functions Exported:
 *
 **************************************************************************** */

void dpush(long data);
long dpop(void);
long dget(void);

void clear_stack(void);
void init_stack(void);

bool min_stack_depth(int mindep);   /*  TRUE if no error  */
long stackdepth(void);
void swap(void);
void two_swap(void);

#endif   /*  _TOKE_STACK_H    */
