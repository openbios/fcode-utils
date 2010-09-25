#ifndef _OPENBIOS_TYPES_H
#define _OPENBIOS_TYPES_H

/*
 *                     OpenBIOS - free your system!
 *                         ( FCode tokenizer )
 *
 *  This program is part of a free implementation of the IEEE 1275-1994
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2005 Stefan Reinauer, <stepan@openbios.org>
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
 *      Convert system short-name data-types found in  asm/types.h
 *          to OpenBios-style names.  (Mainly, remove the leading
 *          double-Underbar).
 *
 *     Also, define a very useful "bool" type for logical operations,
 *          and maybe a macro (or several) to go with it...
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

#include <stdint.h>


typedef int8_t s8;
typedef uint8_t u8;

typedef int16_t s16;
typedef uint16_t u16;

typedef int32_t s32;
typedef uint32_t u32;

typedef int64_t s64;
typedef uint64_t u64;


#ifdef FALSE            /*  Hack for AIX.     */
#undef FALSE
#undef TRUE
#endif                  /*  Hack for AIX.     */

typedef  enum boolean  {  FALSE = 0 ,  TRUE = -1 } bool ;


/* **************************************************************************
 *          Macro Name:    BOOLVAL
 *                        Convert the supplied variable or expression to
 *                            a formal boolean.
 *   Argument:
 *       x        (bool)           Variable or expression, operand.
 *
 **************************************************************************** */

#define BOOLVAL(x)   (x ? TRUE : FALSE)


/* **************************************************************************
 *          Macro Name:    INVERSE
 *                        Return the logical inversion of the
 *                            supplied boolean variable or expression.
 *   Argument:
 *       x        (bool)           Variable or expression, operand.
 *
 **************************************************************************** */

#define INVERSE(x)   (x ? FALSE : TRUE)


/* **************************************************************************
*
 *          Some hacks stuck in for systems w/ incomplete includes
 *              or libraries or whatever.
 *
 **************************************************************************** */

#ifndef  _PTR
#define  _PTR        void *
#endif

#endif /* _OPENBIOS_TYPES_H */
