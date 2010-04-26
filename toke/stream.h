#ifndef _H_STREAM
#define _H_STREAM

/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  stream.h - prototypes for streaming functions. 
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

/* **************************************************************************
 *
 *          Exported Global Variables
 *
 **************************************************************************** */

/* input pointers */
extern u8 *start;
extern u8 *pc;
extern u8 *end;
extern char		*iname;
extern unsigned int lineno;         /* Line Number within current input file  */
extern unsigned int abs_token_no;   /* Absolute Token Number in Source Input  */

/* output pointers */
extern char *oname;         /* output file name  */


/* **************************************************************************
 *
 *    Note that the variables  ostart  and  olen , as well as the routine
 *         increase_output_buffer  are not listed here.
 *
 *    We would have preferred to isolate them completely, but we would have
 *        to disrupt the organization of  emit.c  (which we'd rather not);
 *        in order to avoid exporting them any more widely than necessary,
 *        we will declare them  extern  only in the file where they are
 *        unavoidably needed.
 *
 **************************************************************************** */

/* **************************************************************************
 *          Macro Name:    OUTPUT_SIZE
 *                        Initial size of the Output Buffer
 *
 **************************************************************************** */

#define OUTPUT_SIZE	131072


/* **************************************************************************
 *
 *          Exported Functions
 *
 **************************************************************************** */

void add_to_include_list( char *dir_compt);
void display_include_list( void);
FILE *open_expanded_file( const char *path_name, char *mode, char *for_what);
bool init_stream( const char *name );
void close_stream( _PTR dummy);
void init_output( const char *inname, const char *outname );
bool close_output(void);
void init_inbuf(char *inbuf, unsigned int buflen);

#endif   /* _H_STREAM */
