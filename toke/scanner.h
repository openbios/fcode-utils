#ifndef _TOKE_SCANNER_H
#define _TOKE_SCANNER_H

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
 *      External/Prototype definitions for Scanning functions in Tokenizer
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

extern u8  *statbuf;           /*  The word just read from the input stream  */
extern u8   base;	       /*  The numeric-interpretation base           */


/* pci data */
extern bool pci_is_last_image;
extern u16  pci_image_rev;        /* Vendor's Image, NOT PCI Data Struct Rev */
extern u16  pci_vpd;


/*  Having to do with the state of the tokenization  */
extern bool offs16;	     /*  Using 16-bit branch- (etc) -offsets  */
extern bool in_tokz_esc;     /*  TRUE if in "Tokenizer Escape" mode   */
extern bool incolon;	     /*  TRUE if inside a colon definition    */
extern bool haveend;	     /*  TRUE if the "end" code was read.     */
extern int do_loop_depth;    /*  How deep we are inside DO ... LOOP variants */

/*  State of headered-ness for name-creation  */
typedef enum headeredness_t {
       FLAG_HEADERLESS ,
       FLAG_EXTERNAL ,
       FLAG_HEADERS }  headeredness ;
extern headeredness hdr_flag;

/*  For special-case error detection or reporting */
extern int lastcolon;	/*  Loc'n in output stream of latest colon-def'n.  */
			/*  Used for error-checking of IBM-style Locals    */
extern char *last_colon_defname;       /*  Name of last colon-definition     */
extern char *last_colon_filename;      /*  File where last colon-def'n made  */
extern unsigned int last_colon_lineno; /*  Line number of last colon-def'n   */
extern bool report_multiline;          /*  False to suspend multiline warning */
extern unsigned int last_colon_abs_token_no;

           /*  Shared phrases   */
extern char *in_tkz_esc_mode;
extern char *wh_defined; 

/* ************************************************************************** *
 *
 *      Function Prototypes / Functions Exported:
 *
 **************************************************************************** */

void	init_scanner( void );
void	exit_scanner( void );
void init_scan_state( void );
void	 fcode_ender( void );

bool skip_until( char lim_ch);
void push_source( void (*res_func)(), _PTR res_parm, bool is_f_chg );
signed long get_word( void);
bool get_word_in_line( char *func_nam);
bool get_rest_of_line( void);
void warn_unterm( int severity,
                            char *something,
			        unsigned int saved_lineno);
void warn_if_multiline( char *something, unsigned int start_lineno );
void user_message( tic_param_t pfield );
void skip_user_message( tic_param_t pfield );
bool get_number( long *result);
void eval_string( char *inp_bufr);

void process_remark( tic_param_t pfield );
bool filter_comments( u8 *inword);
bool as_a_what( fwtoken definer, char *as_what);
tic_hdr_t *lookup_word( char *stat_name, char **where_pt1, char **where_pt2 );
bool word_exists( char *stat_name, char **where_pt1, char **where_pt2 );
void warn_if_duplicate ( char *stat_name);
void tokenize_one_word( signed long wlen );
void check_name_length( signed long wlen );
bool definer_name(fwtoken definer, char **reslt_ptr);

void	tokenize( void );

/* **************************************************************************
 *
 *          Macros:
 *    FUNC_CPY_BUF_SIZE   Recommended size of a temporary buffer to retain
 *                        a copy of a function name taken from statbuf,
 *                        when  statbuf  will be used to return a value,
 *                        but the function name might still be needed for
 *                        an error message.
 *    AS_WHAT_BUF_SIZE    Recommended size of the buffer passed to the
 *                        as_a_what() routine.
 *
 **************************************************************************** */

#define FUNC_CPY_BUF_SIZE  40

#define AS_WHAT_BUF_SIZE   32

#endif   /*  _TOKE_SCANNER_H    */
