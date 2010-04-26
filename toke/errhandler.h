#ifndef _TOKE_ERRHANDLER_H
#define _TOKE_ERRHANDLER_H

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
 *      Function Prototypes for Tokenizer Error-Handler
 *
 *      Defines symbols for the various classes of errors
 *          for the Error-Handler and for all its users
 *
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

#include "types.h"

#define  FATAL       0x80000000
#define  TKERROR     0x04000000
#define  WARNING     0x00200000
#define  INFO        0x00010000
#define  TRACER      0x00008000
#define  MESSAGE     0x00000800
#define  P_MESSAGE   0x00000040
#define  FORCE_MSG   0x00000001

void init_error_handler( void);
void tokenization_error( int err_type, char* msg, ... );
void started_at( char * saved_ifile, unsigned int saved_lineno);
void print_started_at( char * saved_ifile, unsigned int saved_lineno);
void just_started_at( char * saved_ifile, unsigned int saved_lineno);
void where_started( char * saved_ifile, unsigned int saved_lineno);
void just_where_started( char * saved_ifile, unsigned int saved_lineno);
void in_last_colon( bool say_in );
_PTR safe_malloc( size_t size, char *phrase);
bool error_summary( void );   /*  Return TRUE if OK to produce output. */


/* **************************************************************************
 *
 *      Macros:
 *          ERRMSG_DESTINATION      During development, I used this to switch
 *               error message destination between STDOUT and STDERR, until I
 *               settled on which is preferable.  Recently, I have proven to
 *               my satisfaction that STDERR is preferable:  error-messages
 *               produced by a sub-shell will be correctly synchronized with
 *               the error-messages we produce.  When I tested using STDOUT
 *               for error-messages, that error-case looked garbled.
 *          FFLUSH_STDOUT           fflush( stdout) if error message destination
 *              is STDERR, No-op if it's STDOUT.  A few of these, judiciously
 *              placed, kept our own regular and error messages nicely in sync.
 *
 **************************************************************************** */

#define ERRMSG_DESTINATION stderr
#define FFLUSH_STDOUT  fflush( stdout);

/*  We're no longer switching the above.
 *  The below is left here to show what had been done formerly.
 */
#if -1     /*  Switchable error-message destination  */ 
#else      /*  Switchable error-message destination  */
#define ERRMSG_DESTINATION stdout
#define FFLUSH_STDOUT  /*  Don't need to do anything here  */
#endif     /*  Switchable error-message destination  */


/* **************************************************************************
 *
 *     A necessary hack for systems that don't seem
 *         to have  strupr  and  strlwr
 *     Let's avoid a naming conflict, just in case... 
 *
 **************************************************************************** */

extern char *strupper( char *strung);
#define strupr strupper

extern char *strlower( char *strung);
#define strlwr strlower


#endif  /*   _TOKE_ERRHANDLER_H   */
