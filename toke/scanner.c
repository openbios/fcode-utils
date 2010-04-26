/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  scanner.c - simple scanner for forth files.
 *  
 *  This program is part of a free implementation of the IEEE 1275-1994 
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2010 by Stefan Reinauer <stepan@openbios.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __GLIBC__
#define __USE_XOPEN_EXTENDED
#endif
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "macros.h"
#include "stack.h"
#include "stream.h"
#include "emit.h"
#include "toke.h"
#include "dictionary.h"
#include "vocabfuncts.h"
#include "scanner.h"
#include "errhandler.h"
#include "tokzesc.h"
#include "conditl.h"
#include "flowcontrol.h"
#include "usersymbols.h"
#include "clflags.h"
#include "devnode.h"
#include "tracesyms.h"
#include "nextfcode.h"

#include "parselocals.h"

/* **************************************************************************
 *
 *  Some VERY IMPORTANT global variables follow
 *
 **************************************************************************** */

u8  *statbuf=NULL;      /*  The word just read from the input stream  */
u8   base=0x0a;         /*  The numeric-interpretation base           */

/* pci data */
bool pci_is_last_image=TRUE;
u16  pci_image_rev=0x0001;  /*  Vendor's Image, NOT PCI Data Structure Rev */
u16  pci_vpd=0x0000;


/*  Having to do with the state of the tokenization  */
bool offs16       = TRUE;    /*  We are using 16-bit branch- (etc) -offsets */
bool in_tokz_esc  = FALSE;   /*  TRUE if in "Tokenizer Escape" mode   */
bool incolon      = FALSE;   /*  TRUE if inside a colon definition    */
bool haveend      = FALSE;   /*  TRUE if the "end" code was read.     */
int do_loop_depth = 0;       /*  How deep we are inside DO ... LOOP variants  */

/*  State of headered-ness for name-creation  */
headeredness hdr_flag = FLAG_HEADERLESS ;  /*  Init'l default state  */

/*  Used for error-checking of IBM-style Locals  */
int lastcolon;   /*  Location in output stream of latest colon-definition. */

/*  Used for error reporting   */
char *last_colon_defname = NULL;   /*  Name of last colon-definition        */
char *last_colon_filename = NULL;  /*  File where last colon-def'n made     */
unsigned int last_colon_lineno;    /*  Line number of last colon-def'n      */
bool report_multiline = TRUE;      /*  False to suspend multiline warning   */
unsigned int last_colon_abs_token_no;

           /*  Shared phrases                                               */
char *in_tkz_esc_mode = "in Tokenizer-Escape mode.\n";
char *wh_defined      = ", which is defined as a ";

/* **************************************************************************
 *  Local variables
 **************************************************************************** */
static u16  last_colon_fcode;  /*  FCode-number assigned to last colon-def'n  */
                               /*      Used for RECURSE  */

static bool do_not_overload = TRUE ;  /*  False to suspend dup-name-test     */
static bool got_until_eof = FALSE ;   /*  TRUE to signal "unterminated"      */

static unsigned int last_colon_do_depth = 0;

/*  Local variables having to do with:                                      */
/*       ...  the state of the tokenization                                 */
static bool is_instance = FALSE;        /*  Is "instance" is in effect?     */
static char *instance_filename = NULL;  /*  File where "instance" invoked   */
static unsigned int instance_lineno;    /*  Line number of "instance"       */
static bool fcode_started = FALSE ;     /*  Only 1 fcode_starter per block. */
static bool first_fc_starter = TRUE;    /*  Only once per tokenization...   */

/*       ... with the state of the input stream,                            */
static bool need_to_pop_source;

/*       ... with the use of the return stack,                              */
static int ret_stk_depth = 0;          /*  Return-Stack-Usage-Depth counter */

/*       ... and with control of error-messaging.                           */
           /*  Should a warning about a dangling "instance" 
	    *      be issued at the next device-node change?
	    */
static bool dev_change_instance_warning = TRUE;

           /*  Has a gap developed between "instance" and its application?  */
static bool instance_definer_gap = FALSE;


/* **************************************************************************
 *
 *      Function name:  skip_ws
 *      Synopsis:       Advance the PC past all whitespace.
 *                      Protect against pointer over-runs 
 *
 *      Inputs:
 *         Parameters:                  NONE
 *         Global Variables:        
 *             pc                       Input-source Scanning pointer
 *             end                      End of input-source buffer
 *
 *      Outputs:
 *         Returned Value:      TRUE if PC reached END before non-blank char
 *         Global Variables:    
 *             pc            Advanced to first non-blank char, or to END  
 *             lineno        Incremented if encountered new-line along the way
 *
 *      Error Detection:
 *          Return a TRUE if End of input-source buffer reached before
 *              non-blank character.  Not necessarily an error; allow
 *              calling routine to decide...
 *
 **************************************************************************** */

static bool skip_ws(void)
{
    bool retval = TRUE;
    char ch_tmp;

    for (  ; pc < end; pc++ )
{
        ch_tmp = *pc;
	if ( (ch_tmp != '\t') && (ch_tmp != ' ') && (ch_tmp != '\n' ) )
	{
	    retval = FALSE;
	    break;
	}
        if ( ch_tmp == '\n')  lineno++;
    }
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  skip_until
 *      Synopsis:       Advance the PC to the given character.
 *                      Do not copy anything into statbuf.
 *                      Protect against pointer over-runs 
 *
 *      Inputs:
 *         Parameters:
 *             lim_ch                   Limiting Character
 *         Global Variables:        
 *             pc                       Input-source Scanning pointer
 *             end                      End of input-source buffer
 *
 *      Outputs:
 *         Returned Value:      TRUE if PC reached END before finding LIM_CH
 *         Global Variables:    
 *             pc            Advanced to first occurrence of LIM_CH, or to END  
 *             lineno        Incremented if encountered new-line along the way
 *
 *      Error Detection:
 *          Return a TRUE if End of input-source buffer reached before
 *              desired character.  Not necessarily an error; allow calling
 *              routine to decide...
 *
 **************************************************************************** */

bool skip_until( char lim_ch)
{
    bool retval = TRUE;
    char ch_tmp;

    for (  ; pc < end; pc++ )
    {
        ch_tmp = *pc;
        if ( ch_tmp == lim_ch )
	{
	    retval = FALSE;
	    break;
	}
        if ( ch_tmp == '\n')  lineno++;
	}
    return ( retval );
}


/* **************************************************************************
 *
 *      Function name:  get_until
 *      Synopsis:       Return, in  statbuf, the string from  PC  to the first
 *                      occurrence of the given delimiter-character..
 *
 *      Inputs:
 *         Parameters:
 *             needle          The given delimiter-character
 *         Global Variables:
 *             pc              Input-source Scanning Pointer
 *
 *      Outputs:
 *         Returned Value:     Length of the string obtained
 *         Global Variables:
 *             statbuf         The string obtained from the input stream;
 *                                 does not include the delimiter-character.
 *             pc              Bumped past the delimiter-character, unless
 *                                 it's a new-line, in which case leave it
 *                                 to be handled by  get_word()
 *         Local Static Variables:
 *             got_until_eof   Pass this as a signal that the end of the
 *                                 buffer was reached before the delimiter;
 *                                 Testing whether PC has reached END is
 *                                 not a sufficient indication.
 *
 *      Error Detection:
 *          If string overflows  statbuf  allocation, ERROR, and 
 *              return "no string" (i.e., length = 0).
 *          Otherwise, if delimiter not found before eof, keep string.
 *              Protection against PC pointer-over-run past END is
 *              provided by  skip_until() .  Reaching END will be
 *              handled by calling routine; pass indication along
 *              via Local Static Variable.
 *
 *      Process Explanation:
 *          Skip the delimiter-character from further input, unless it's a
 *              new-line which will be skipped anyway.  Let  skip_until() 
 *              and  get_word()  handle incrementing line-number counters.
 *          If skip_until()  indicated reaching end-of-file, don't bump PC
 *
 *      Revision History:
 *          Updated Thu, 14 Jul 2005 by David L. Paktor
 *              More robust testing for when PC exceeds END 
 *                  Involved replacing  firstchar()
 *
 **************************************************************************** */
	
static signed long get_until(char needle)
{                                                                               
	u8 *safe;                                                         
	unsigned long len = 0;

	safe=pc;

	got_until_eof = skip_until(needle);

	len = pc - safe;
	if (len >= GET_BUF_MAX )
	{
	    tokenization_error( TKERROR,
		"get_until buffer overflow.  Max is %d.\n", GET_BUF_MAX-1 );
	    len = GET_BUF_MAX-1;
}

	memcpy(statbuf, safe, len);
	statbuf[len]=0;

	if ( INVERSE(got_until_eof) )
{
	    if ( needle != '\n' )  pc++;
	}
	return len;
}


/* **************************************************************************
 *
 *          We are going to use a fairly sophisticated mechanism to
 *              make a smooth transition between processing the body
 *              of a Macro, a User-defined Symbol or an FLOADed file 
 *              and the resumption of processing the source file, so
 *              that the end-of-file will only be seen at the end of
 *              the primary input file (the one from the command-line).
 *         This mechanism will be tied in with the get_word() routine
 *
 *         We are going to define a private data-structure in which
 *              we will save the state of the current source file,
 *              and from which, of course, we will recover it.  Its
 *              fields will be:
 *                   A pointer to the next structure in the list.
 *                   The saved values of  START  END  and  PC
 *                   The saved values of  INAME  and  LINENO
 *                   A flag indicating that get-word should "pause"
 *                        before popping the source-stream because
 *                        the input file will be changing.
 *                   A place from which to save and recover the state of
 *                        whether we're testing for "Multi-line" strings;
 *                        to prevent undeserved "Multi-line" warnings
 *                        during Macro processing.
 *                   A pointer to a "resumption" routine, to call
 *                        when resuming processing the source file;
 *                        the routine takes a pointer parameter
 *                        and has no return value.  The pointer
 *                        may be NULL if no routine is needed.
 *                   The pointer to pass as the parameter to the
 *                        resumption routine.
 *
 **************************************************************************** */

typedef struct source_state
    {
	struct source_state   *next;
	u8                    *old_start;
	u8                    *old_pc;
	u8                    *old_end;
	char                  *old_iname;
	unsigned int           old_lineno;
	bool                   pause_before_pop;
	bool                   sav_rep_multlin;
	void                 (*resump_func)();
	_PTR                   resump_param;
    } source_state_t ;

static source_state_t  *saved_source = NULL;


/* **************************************************************************
 *
 *      Function name:  push_source
 *      Synopsis:       Save the state of the current source file, in the
 *                          source_state data-structure LIFO linked-list.
 *
 *      Inputs:
 *         Parameters:
 *             res_func              Pointer to routine to call when resuming
 *                                       processing the saved source file.
 *             res_param             Parameter to pass to res_func.
 *                                   Either or both pointers may be NULL.
 *             file_chg              TRUE if input file is going to change.
 *         Global Variables:
 *             start                 Points to current input buffer
 *             end                   Points to end of current input buffer
 *             pc                    Input point in current buffer
 *             iname                 Name of current source file
 *             lineno                Line number in current source file
 *             report_multiline      Whether we're testing for "Multi-line"
 *         Local Static Variables:
 *             saved_source          Pointer to the source_state data-structure
 *
 *      Outputs:
 *         Returned Value:           NONE
 *         Local Static Variables:
 *             saved_source          Points to new source_state entry
 *         Memory Allocated
 *             for the new source_state entry
 *         When Freed?
 *             When resuming processing the source file, by drop_source().
 *
 *      Process Explanation:
 *          The calling routine will establish the new input buffer via
 *              a call to init_inbuf() or the like.
 *
 **************************************************************************** */

void push_source( void (*res_func)(), _PTR res_parm, bool file_chg )
{
    source_state_t  *new_sav_src;

    new_sav_src = safe_malloc( sizeof(source_state_t), "pushing Source state");

    new_sav_src->next = saved_source;
    new_sav_src->old_start = start;
    new_sav_src->old_pc = pc;
    new_sav_src->old_end = end;
    new_sav_src->old_iname = iname;
    new_sav_src->old_lineno = lineno;
    new_sav_src->pause_before_pop = file_chg;
    new_sav_src->sav_rep_multlin = report_multiline;
    new_sav_src->resump_func = res_func;
    new_sav_src->resump_param = res_parm;

    saved_source = new_sav_src;
}

/* **************************************************************************
 *
 *      Function name:  drop_source
 *      Synopsis:       Remove last saved state of source processing
 *                          from the source_state LIFO linked-list,
 *                          without (or after) restoring.
 *
 *      Inputs:
 *         Parameters:               NONE
 *         Local Static Variables:
 *             saved_source          Pointer to the source_state data-structure
 *
 *      Outputs:
 *         Returned Value:           NONE
 *         Local Static Variables:
 *             saved_source          Points to previous source_state entry
 *         Memory Freed
 *             Saved source_state entry that was just "dropped"
 *
 *      Error Detection:
 *          None.  Called only when linked-list is known not to be at end.  
 *
 **************************************************************************** */

static void drop_source( void)
{
    source_state_t  *former_sav_src = saved_source;

    saved_source = saved_source->next ;
    free( former_sav_src);
}

/* **************************************************************************
 *
 *      Function name:  pop_source
 *      Synopsis:       Restore the state of source processing as it was
 *                          last saved in the source_state linked-list.
 *
 *      Inputs:
 *         Parameters:               NONE
 *         Local Static Variables:
 *             saved_source          Pointer to the source_state data-structure
 *             need_to_pop_source    If TRUE, don't check before popping.
 *
 *      Outputs:
 *         Returned Value:           TRUE if reached end of linked-list
 *         Global Variables:
 *             start                 Points to restored input buffer
 *             end                   Points to end of restored input buffer
 *             pc                    Input point in restored buffer
 *             iname                 Name of restored source file
 *             lineno                Line number in restored source file
 *             report_multiline      Restored to saved value.
 *         Local Static Variables:
 *             saved_source          Points to previous source_state entry
 *             need_to_pop_source    TRUE if postponed popping till next time
 *         Memory Freed
 *             Saved source-state entry that was just "popped"
 *
 *      Process Explanation:
 *          First check the need_to_pop_source flag.
 *          If it is set, we will clear it and go ahead and pop.
 *          If it is not set, we will check the  pause_before_pop  field
 *                  of the top entry in the source_state linked-list.
 *              If the  pause_before_pop  field is set, we will set the
 *                  need_to_pop_source flag and return.
 *              If it is not, we will go ahead and pop.
 *          If we are going to go ahead and pop, we will call the
 *              "Resume-Processing" routine (if it's not NULL) before
 *              we restore the saved source state.
 *
 **************************************************************************** */

static bool pop_source( void )
{
    bool retval = TRUE;

    if ( saved_source != NULL )
    {
	retval = FALSE;
	if ( need_to_pop_source )
	{
	    need_to_pop_source = FALSE;
	}else{
	    if ( saved_source->pause_before_pop )
	    {
	        need_to_pop_source = TRUE;
		return( retval);
	    }
	}

	if ( saved_source->resump_func != NULL )
	{
	    saved_source->resump_func( saved_source->resump_param);
	}
	report_multiline = saved_source->sav_rep_multlin;
	lineno = saved_source->old_lineno ;
	iname = saved_source->old_iname ;
	end = saved_source->old_end ;
	pc = saved_source->old_pc ;
	start = saved_source->old_start ;

	drop_source();
    }
    return( retval);
}


/* **************************************************************************
 *
 *      Function name:  get_word
 *      Synopsis:       Gather the next "word" (aka Forth Token) from the
 *                          input stream.
 *                      A Forth Token is, of course, a string of characters
 *                          delimited by white-space (blank, tab or new-line).
 *                      Do not increment line-number counters here; leave
 *                          the delimiter after the word unconsumed.
 *
 *      Inputs:
 *         Parameters:                 NONE
 *         Global Variables:
 *             pc                      Input-stream Scanning Pointer
 *         Local Static Variables:
 *             need_to_pop_source      If TRUE, pop_source() as first step
 *
 *      Outputs:
 *         Returned Value:             Length of "word" gotten;
 *                                     0 if  reached end of file.
 *                                     -1 if reached end of primary input
 *                                         (I.e., end of all source)
 *         Global Variables:
 *             statbuf                 Copy of "gotten" word
 *             pc                      Advanced to end of "gotten" word,
 *                                         (i.e., the next word is "consumed")
 *                                         unless returning zero.
 *             abs_token_no            Incremented, if valid "word" (token)
 *                                         was gotten.
 *
 *      Process Explanation:
 *          Skip whitespace to the start of the token, 
 *             then skip printable characters to the end of the token.
 *          That part's easy, but what about when skipping whitespace
 *              brings you to the end of the input stream?
 *          First, look at the  need_to_pop_source  flag.  If it's set,
 *              we came to the end of the input stream the last time
 *              through.  Now we need to  pop_source()  first.
 *          Next, we start skipping whitespace; this detects when we've
 *                  reached the end of the input stream.  If we have,
 *                  then we need to  pop_source()  again.
 *              If  pop_source()  returned a TRUE, we've reached the end
 *                  of the primary input file.  Return -1.
 *              If  pop_source()  turned the  need_to_pop_source  flag
 *                  to TRUE again, then we need to "pause" until the
 *                  next time through; return zero.
 *          Otherwise, we proceed with collecting the token as described.
 *
 *      Revision History:
 *          Updated Thu, 23 Feb 2006 by David L. Paktor
 *              Tied this routine in with a more sophisticated mechanism that
 *                  makes a smooth transition between processing the body of
 *                  a Macro, a User-defined Symbol or an FLOADed file, and 
 *                  the resumption of processing the source file, so that the
 *                  end-of-file will only be seen at the end of the primary
 *                  input file (the one that came from the command-line)
 *          Updated Fri, 24 Feb 2006 by David L. Paktor
 *              This is trickier than I thought.  Added a global indicator
 *                  of whether a file-boundary was crossed while getting
 *                  the word; previously, that was indicated by a return
 *                  value of zero, which now means something else...
 *              The flag,  closed_stream , will be cleared every time this
 *                  routine is entered, and set whenever close_stream() is
 *                  entered.
 *         Updated Tue, 28 Feb 2006 at 10:13 PST by David L. Paktor
 *              Trickier still.  On crossing a file-boundary, must not
 *                  consume the first word in the resumed file, for one
 *                  call; instead, return zero.  Consume it on the next
 *                  call.  The  closed_stream  flag is now irrelevant and
 *                  has gone away.
 *
 **************************************************************************** */

signed long get_word( void)
{
	size_t len;
	u8 *str;
	bool keep_skipping;
	bool pop_result;

	if ( need_to_pop_source )
	{
	    pop_result = pop_source();
	}

	do {
	    keep_skipping = skip_ws();
	    if ( keep_skipping )
	    {
		pop_result = pop_source();
		if ( pop_result || need_to_pop_source )
		{
		    statbuf[0] = 0;
		    if ( pop_result )
		    {
			return -1;
		    }
		return 0;
		}
	    }
	} while ( keep_skipping );

	str=pc;
	while ( (str < end) && *str && *str!='\n' && *str!='\t' && *str!=' ')
		str++;

	len=(size_t)(str-pc);
	if (len >= GET_BUF_MAX )
	{
	    tokenization_error ( FATAL,
		"get_word buffer overflow.  Max is %d.", GET_BUF_MAX-1 );
	}

	memcpy(statbuf, pc, len); 
	statbuf[len]=0;

#ifdef DEBUG_SCANNER
	printf("%s:%d: debug: read token '%s', length=%ld\n",
			iname, lineno, statbuf, len);
#endif
	pc+=len;
	abs_token_no++;
	return len;
}


/* **************************************************************************
 *
 *      Function name:  get_word_in_line
 *      Synopsis:       Get the next word on the same line as the current
 *                      line of input.  If the end of line was reached
 *                      before a word was found, print an error message
 *                      and return an indication.
 *
 *      Inputs:
 *         Parameters:
 *             func_nam        Name of the function expecting the same-line
 *                                 input; for use in the Error Message.
 *                                 If NULL, do not issue Error Message
 *         Global Variables:
 *             pc              Input character pointer.  Saved for comparison
 *             lineno          Current input line number.  Saved for comparison
 *
 *      Outputs:
 *         Returned Value:     TRUE = success.  Word was acquired on same line.
 *         Global Variables:
 *             statbuf         Advanced to the next word in the input stream.
 *             pc              Advanced if no error; restored otherwise.
 *
 *      Error Detection:
 *          If no next word is gotten (i.e., we're at end-of-file), or if
 *              one is gotten but not on the same line, the routine will
 *              return FALSE; if  func_nam  is not NULL, an ERROR Message
 *              will be issued.
 *          Also, the values of  PC  LINENO  and  ABS_TOKEN_NO  will be reset
 *              to the positions they had when this routine was entered.
 *
 **************************************************************************** */

bool get_word_in_line( char *func_nam)
{                                                                               
    signed long wlen;
    bool retval = TRUE;
    u8 *save_pc = pc;
    unsigned int save_lineno = lineno;
    unsigned int save_abs_token_no = abs_token_no;

    /*  Copy of function name, for error message  */
    char func_cpy[FUNC_CPY_BUF_SIZE+1];

    /*  Do this first, in the likely event that  func_nam  was  statbuf   */
    if ( func_nam != NULL )
    {
	strncpy( func_cpy, func_nam, FUNC_CPY_BUF_SIZE);
	func_cpy[FUNC_CPY_BUF_SIZE] = 0;  /*  Guarantee a null terminator  */
    }

    wlen = get_word();
    if ( ( lineno != save_lineno ) || ( wlen <= 0 ) )
    {
	abs_token_no = save_abs_token_no;
	lineno = save_lineno;
	pc = save_pc;
	retval = FALSE;
	if ( func_nam != NULL )
	{
	    tokenization_error ( TKERROR,
	       "Operator %s expects its target on the same line\n",
		   strupr(func_cpy));
	}
    }
    return ( retval );
}


/* **************************************************************************
 *
 *      Function name:  get_rest_of_line
 *      Synopsis:       Get all the remaining text on the same line as
 *                      the current line of input.  If there is no text
 *                      (not counting whitespace) before the end of line,
 *                      return an indication.
 *
 *      Inputs:
 *         Parameters:         NONE
 *         Global Variables:
 *             pc              Input character pointer.  Saved for restoration
 *             lineno          Current input line number.  Saved for comparison
 *
 *      Outputs:
 *         Returned Value:     TRUE = success.  Text was acquired on same line.
 *         Global Variables:
 *             statbuf         Contains the text found in the input stream.
 *             pc              Advanced to end of line or of whitespace, if
 *                                 no error; restored otherwise.
 *             lineno          Preserved if no error; otherwise, restored.
 *             abs_token_no    Restored if error; otherwise, advanced as normal.
 *
 *      Error Detection:
 *          Routine will return FALSE if no text is gotten on the same line.
 *
 **************************************************************************** */

bool get_rest_of_line( void)
{
    bool retval = FALSE;
    u8 *save_pc = pc;
    unsigned int save_lineno = lineno;
    unsigned int save_abs_token_no = abs_token_no;

    if ( INVERSE( skip_ws() ) )
    {
        if ( lineno == save_lineno )
	{
	    signed long wlen = get_until('\n');
	    if ( wlen > 0 ) retval = TRUE;
	}else{
	    abs_token_no = save_abs_token_no;
	    lineno = save_lineno;
	    pc = save_pc;
	}
    }
    return( retval);
}


/* **************************************************************************
 *
 *      Function name:  warn_unterm
 *      Synopsis:       Message for "Unterminated ..." something
 *                      Show saved line-number, where the "something" started,
 *                      and the definition, if any, in which it occurred.
 *
 *      Inputs:
 *         Parameters:
 *             severity              Type of error/warning message to display
 *                                       usually either WARNING or TKERROR
 *             something             String to print after "Unterminated"
 *             saved_lineno          Line-Number where the "something" started
 *         Global Variables:
 *             lineno                Saved, then restored.
 *             last_colon_defname    Used only if unterm_is_colon is TRUE;
 *         Local Static Variables:
 *             unterm_is_colon       See 07 Mar 2006 entry under Rev'n History
 *
 *      Outputs:
 *         Returned Value:           NONE
 *         Global Variables:
 *             lineno                Saved, then restored.
 *         Local Static Variables:
 *             unterm_is_colon       Reset to FALSE
 *         Printout:
 *             Warning or Error message
 *
 *      Revision History:
 *          Updated Mon, 06 Mar 2006 by David L. Paktor
 *              Added call to in_last_colon()
 *          Updated Tue, 07 Mar 2006 by David L. Paktor
 *              Call to in_last_colon() works okay in most cases except for
 *                  when the "something" is a Colon Definition; there, it
 *                  results in the phrase: ... Definition in definition of ...
 *                  which is awkward.  To eliminate that, I am introducing 
 *                  a Local Static Variable flag called  unterm_is_colon
 *                  which will be set only in the appropriate place and
 *                  re-cleared here.  It's a retro-fit, of course; it could
 *                  have been a parameter had the need for it occurred when
 *                  this routine was first constructed... 
 *
 **************************************************************************** */

static bool unterm_is_colon = FALSE;
void warn_unterm( int severity, char *something, unsigned int saved_lineno)
{
    unsigned int tmp = lineno;
    lineno = saved_lineno;
    if ( unterm_is_colon )
    {
	tokenization_error( severity, "Unterminated %s of %s\n",
	    something, strupr( last_colon_defname) );
	unterm_is_colon = FALSE;
    }else{
	tokenization_error( severity, "Unterminated %s", something);
	in_last_colon( TRUE);
    }
    lineno = tmp;
}

/* **************************************************************************
 *
 *      Function name:  warn_if_multiline
 *      Synopsis:       Test for "Multi-line ..." something and issue WARNING
 *                      Show saved line-number, where the "something" started
 *
 *      Inputs:
 *         Parameters:
 *             something          String to print after "Unterminated"
 *             start_lineno       Line-Number where the "something" started
 *         Global Variables:
 *             lineno             Line-Number where we are now
 *             iname              Input file name, to satisfy ...where_started()
 *                                    (Not crossing any actual file boundary.)
 *             report_multiline   TRUE = go ahead with the message
 *
 *      Outputs:
 *         Returned Value:        NONE
 *         Global Variables:
 *             report_multiline   Restored to TRUE.
 *
 *      Error Detection:
 *          Only issue message if the current  lineno  doesn't equal
 *              the start_lineno  
 *
 *      Process Explanation:
 *          The directive "multi-line" allows the user to specify that
 *              the next "Multi-line ..." something is intentional, and
 *              will cause its warning to be suppressed.  It remains in
 *              effect until it's "used"; afterwards, it's reset.
 *
 **************************************************************************** */

void warn_if_multiline( char *something, unsigned int start_lineno )
{
    if ( report_multiline && ( start_lineno != lineno ) )
    {
	tokenization_error( WARNING, "Multi-line %s, started", something);
	where_started( iname, start_lineno);
    }
    report_multiline = TRUE;
}


/* **************************************************************************
 *
 *      Function name:  string_remark
 *      Synopsis:       Suspend string parsing past end of line and
 *                      whitespace at start of the new line.
 *
 *      Inputs:
 *         Parameters:
 *             errmsg_txt            Text to be used for error-message.
 *         Global Variables:
 *             pc                    Input-source Scanning pointer
 *
 *      Outputs:
 *         Returned Value:           NONE
 *         Global Variables:
 *             pc                    Will point to first non-blank in new line
 *
 *      Error Detection:
 *          The return value of the skip_until() or skip_ws() routine
 *             will indicate if PC goes past END.  Issue a WARNING.
 *             The calling routine will handle things from there.
 *
 **************************************************************************** */

static void string_remark(char *errmsg_txt)
{
    unsigned int sav_lineno = lineno;
    bool eof = skip_until('\n');
    if ( ! eof )
    {
	eof = skip_ws();
    }
    if ( eof )
    {
	warn_unterm(WARNING, errmsg_txt, sav_lineno);
	}
	
}


/*  Convert the given string to a number in the supplied base   */
/*  Allow -- and ignore -- embedded periods.    */
/*  The  endptr  param represents a pointer that will be updated
 *      with the address of the first non-numeric character encountered,
 *      (unless it is a NULL, in which case it is ignored).
 */
/*  There is no test for a completely invalid string;
 *  the calling routine is responsible for ascertaining
 *  the validity of the string being passed.
 */
static long parse_number(u8 *start, u8 **endptr, int lbase) 
{
	long val = 0;
	bool negative = FALSE ;
	int  curr;
	u8 *nptr=start;

	curr = *nptr;
	if (curr == '-')
	{
		negative = TRUE ;
		nptr++;
	}
	
	for (curr = *nptr; (curr = *nptr); nptr++) {
		if ( curr == '.' )
			continue;
		if ( curr >= '0' && curr <= '9')
			curr -= '0';
		else if (curr >= 'a' && curr <= 'f')
			curr += 10 - 'a';
		else if (curr >= 'A' && curr <= 'F')
			curr += 10 - 'A';
		else
			break;
		
		if (curr >= lbase)
			break;
		
		val *= lbase;
		val += curr;
	}

#ifdef DEBUG_SCANNER
	if (curr)
		printf( "%s:%d: warning: couldn't parse number '%s' (%d/%d)\n",
				iname, lineno, start,curr,lbase);
#endif

	if (endptr)
		*endptr=nptr;

	if (negative)
	{
		val = -val;
	}
	return val;
}

/* **************************************************************************
 *
 *      Function name:  add_byte_to_string
 *      Synopsis:       Add the given byte (or character) to the string
 *                          being accumulated in statbuf, but protect
 *                          against a buffer overflow.
 *
 *      Inputs:
 *         Parameters:
 *             nu_byte           The given character to be added
 *             walk              Pointer to pointer to the position
 *                                   in  statbuf  where the character
 *                                   is to be placed
 *         Global Variables:
 *             statbuf           Buffer where the string is accumulated
 *         Macros:
 *             GET_BUF_MAX       Size of the buffer
 *
 *      Outputs:
 *         Returned Value:       NONE
 *         Supplied Pointers:
 *             **walk            Given character is placed here
 *             *walk             Incremented in any case
 *
 *      Error Detection:
 *          If  walk  has reached end of string buffer, do not place
 *              the character, but continue to increment  walk .
 *              Calling routine will detect overflow.
 *
 **************************************************************************** */
				
static void add_byte_to_string( u8 nu_byte, u8 **walk )
{
    if ( *walk - statbuf < GET_BUF_MAX )
    {
	**walk = nu_byte;
	}
    (*walk)++;
}

/* **************************************************************************
 *
 *      Function name:  c_string_escape
 *      Synopsis:       Process C-style escape syntax in strings
 *
 *      Inputs:
 *         Parameters:
 *             walk                    Pointer to pointer to area into
 *                                         which to put acquired values
 *         Global Variables:
 *             pc                      Input-source Scanning pointer
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Global Variables:
 *             pc                      Point to last character processed.
 *         Supplied Pointers:
 *             *walk                   Advanced by number of bytes acquired
 *
 *      Error Detection:
 *          WARNING conditions.  See under "Process Explanation" below.
 *
 *      Process Explanation:
 *          Start with  PC  pointing to the first character to process
 *              i.e., after the backslash.
 *          We recognize newline, tab and numbers
 *          A digit-string in the current base can be converted to a number.
 *          The first non-numeric character ends the numeric sequence
 *              and gets swallowed up.
 *          If the number exceeds the size of a byte, use the truncated
 *          	value and issue a WARNING.
 *          If the first character in the "digit"-string was non-numeric,
 *              use the character literally and issue a WARNING.
 *          If the character that ended the numeric sequence is a quote,
 *              it might be the end of the string, or the start of a
 *              special-character or even of an "( ... ) hex-sequence,
 *              so don't swallow it up.
 *
 *      Still to be done:
 *          Better protection against PC pointer-over-run past END.
 *              Currently, this works, but it's held together by threads:
 *              Because  init_stream  forces a null-byte at the end of
 *              the input buffer, parse_number() exits immediately upon
 *              encountering it.  This situation could be covered more
 *              robustly...
 *
 **************************************************************************** */

static void c_string_escape( u8 **walk)
{
    char c = *pc;
    u8 val;
    /*  We will come out of this "switch" statement
     *      with a value for  val  and a decision
     *      as to whether to write it.
     */
    bool write_val = TRUE;
	
    switch (c)
    {
			case 'n':
				/* newline */
	    val = '\n';
				break;
			case 't':
				/* tab */
	    val = '\t';
				break;
			default:

	    /*  Digit-string?  Convert it to a number, using the current base.
	     *  The first non-numeric character ends the numeric sequence
	     *      and gets swallowed up.
	     *  If the number exceeds the size of a byte, use the truncated
	     *      value and issue a WARNING.
	     *  If the first character in the "digit"-string was non-numeric,
	     *      use the character literally and issue a WARNING.
	     */

	     /*
	     *  If the sequence ender is a quote, it might be the end of
	     *      the string, or the start of a special-character or even
	     *      of an "( ... ) hex-sequence, so don't swallow it up.
	     */
	    {
		long lval;
		u8 *sav_pc = pc;
		lval=parse_number(pc, &pc, base);
		val = (u8)lval;
#ifdef DEBUG_SCANNER
				if (verbose)
					printf( "%s:%d: debug: escape code "
						"0x%x\n",iname, lineno, val);
#endif
		if ( lval > 0x0ff )
		{
		    tokenization_error ( WARNING,
			"Numeric String after \\ overflows byte.  "
			    "Using 0x%02x.\n", val);
			}

		if ( pc == sav_pc )
		{
		    /*  NOTE:  Here, PC hasn't been advanced past its
		     *      saved value, so we can count on  C  remaining
		     *      unchanged since the start of the routine.
		     */ 
		    /*  Don't use the null-byte at the end of the buffer  */
		    if ( ( pc >= end ) 
		    /*        or a sequence-ending quote.                 */
			 || ( c == '"' ) )
		    {
			write_val = FALSE;
		    }else{
			/*  In the WARNING message, print the character
			 *      if it's printable or show it in hex
			 *      if it's not.
			 */
			if ( (c > 0x20 ) && ( c <= 0x7e) )
			{
			    tokenization_error ( WARNING,
				"Unrecognized character, %c, "
				    "after \\ in string.  "
					"Using it literally.\n", c);
			}else{
			    tokenization_error ( WARNING,
				"Unrecognized character, 0x%02x, "
				    "after \\ in string.  "
					"Using it literally.\n", c);
			}
			val = c;
		    }
		}
		/*  NOTE:  Here, however, PC may have been advanced...  */
		/*  Don't swallow the sequence-ender if it's a quote.   */
		if ( *pc == '"' )
		{
		    pc--;
		}

	    }   /*  End of the  "default"  clause  */
    }    /*  End of the  "switch"  statement  */

    if ( write_val ) add_byte_to_string( val, walk );

}

/* **************************************************************************
 *
 *      Function name:  get_sequence
 *      Synopsis:       Process the Hex-Number option in strings
 *                      Protect against PC pointer-over-run past END.
 *
 *      Inputs:
 *         Parameters:
 *             **walk           Pointer to pointer to area into which
 *                                  to put acquired values
 *         Global Variables:
 *             pc               Input-source Scanning pointer
 *             end              End of input-source buffer
 *
 *      Outputs:
 *         Returned Value:      TRUE = "Normal Completion" (I.e., not EOF)
 *         Global Variables:
 *             pc               Points at terminating close-paren, or END
 *             lineno           Input File Line-Number Counter, may be incr'd
 *         Supplied Pointers:
 *             *walk            Advanced by number of values acquired
 *
 *      Error Detection:
 *          End-of-file encountered before end of hex-sequence:
 *              Issue a Warning, conclude processing, return FALSE.
 *
 *      Process Explanation:
 *          SETUP and RULES:
 *              Start with  PC  pointing to the first character
 *                  after the '('  (Open-Paren)     
 *              Bytes are gathered from digits in pairs, except
 *                  when separated they are treated singly.
 *              Allow a backslash in the middle of the sequence
 *                  to skip to the end of the line and past the
 *                  whitespace at the start of the next line,
 *                  i.e., it acts as a comment-escape.
 *
 *          INITIALIZE:
 *              PV_indx = 0
 *              Set return-indicator to "Abnormal Completion"
 *              Ready_to_Parse = FALSE
 *              Stuff NULL into PVAL[2]
 *          WHILE PC is less than END
 *              Pick up character at PC into Next_Ch
 *              IF  Next_Ch  is close-paren :
 *                  Set return-indicator to "Normal Completion".
 *                  Done!  Break out of loop.
 *              ENDIF
 *              IF comment-escape behavior (controlled by means of a
 *                      command-line switch) is allowed
 *                  IF  Next_Ch  is backslash :
 *                      Skip to end-of line, skip whitespace.
 *                      If that makes PC reach END :  WARNING message.
 *                          (Don't need to break out of loop;
 *                               normal test will terminate.)
 *                      CONTINUE Loop.
 *                          (Don't increment PC; PC is already at right place).
 *                  ENDIF
 *              ENDIF
 *              IF  Next_Ch  is a valid Hex-Digit character :
 *                  Stuff it into  PVAL[PV_indx]
 *                  IF (PV_indx is 0) :
 *                      Increment PV_indx
 *                  ELSE
 *                      Set Ready_to_Parse to TRUE
 *          	    ENDIF
 *              ELSE
 *                  IF  Next_Ch  is a New-Line, increment Line Number counter
 *                  IF (PV_indx is 1) :
 *                      Stuff NULL into PVAL[1]
 *                      Set Ready_to_Parse to TRUE
 *          	    ENDIF
 *              ENDIF
 *              IF Ready_to_Parse
 *                  Parse PVAL
 *                  Stuff into WALK
 *                  Reset PV_indx to zero
 *                  Reset Ready_to_Parse to FALSE
 *              ENDIF
 *              Increment PC
 *          REPEAT
 *          Return with Normal/Abnormal completion indicator
 *
 **************************************************************************** */

static bool get_sequence(u8 **walk)
{
    int pv_indx = 0;
    bool retval = FALSE;   /*  "Abnormal Completion" indicator  */
    bool ready_to_parse = FALSE;
    char next_ch;
    char pval[3];

#ifdef DEBUG_SCANNER
	printf("%s:%d: debug: hex field:", iname, lineno);
#endif
    pval[2]=0;

    while ( pc < end )
    {
	next_ch = *pc;
	if ( next_ch == ')' )
	{
	    retval = TRUE;
				break;
	}
	if ( hex_remark_escape )
	{
	    if ( next_ch == '\\' )
	    {
		string_remark("string hex-sequence remark");
		continue;
	    }
	}
	if ( isxdigit(next_ch) )
	{
	    pval[pv_indx] = next_ch;
	    if ( pv_indx == 0 )
	    {
		pv_indx++;
	    }else{
		ready_to_parse = TRUE;
	    }
	}else{
	    if ( next_ch == '\n' )  lineno++ ;
	    if ( pv_indx != 0 )
	    {
		pval[1] = 0;
		ready_to_parse = TRUE;
	    }
	}
	if ( ready_to_parse )
	{
	    u8 val = parse_number(pval, NULL, 16);
	    *((*walk)++)=val;
#ifdef DEBUG_SCANNER
		printf(" %02x",val);
#endif
	    pv_indx = 0;
	    ready_to_parse = FALSE;
	}
	pc++;
    }
#ifdef DEBUG_SCANNER
	printf("\n");
#endif
    return ( retval );
}

/* **************************************************************************
 *
 *    Return the length of the string.
 *    Pack the string, without the terminating '"' (Quote), into statbuf
 *    Protect against PC pointer-over-run past END.
 *    Enable Quote-Backslash as a String-Remark Escape.
 *    Allowability of Quote-Backslash as a String-Remark is under control
 *        of a command-line switch (string_remark_escape ).
 *    Allowability of C-style escape characters is under control
 *        of a command-line switch ( c_style_string_escape ).
 *
 *    Truncate string to size of Forth Packed-String (i.e., uses leading
 *        count-byte, so limited to 255, number that one byte can express)
 *        unless the string is being gathered for a Message or is being
 *        consumed for purposes of ignoring it, in either of which case
 *        that limit need not be enforced.  Parameter "pack_str" controls
 *        this:  TRUE  if limit needs to be enforced.
 *
 *    Issue WARNING if string length gets truncated.
 *    Issue WARNING if string crosses line.
 *        The issuance of the Multi-line WARNING is under control of a
 *           one-shot directive similar to OVERLOAD , called  MULTI-LINE
 *
 *    Still to be decided:
 *        Do we want to bring the allowability of strings crossing
 *            lines under control of a command-line switch?
 *
 ************************************************************************** */

static signed long get_string( bool pack_str)
{
	u8 *walk;
	unsigned long len;
	char c;
	bool run = TRUE;
	unsigned long start_lineno = lineno;    /*  For warning message  */
	
	/*
	 *  Bump past the single whitespace character that delimits
	 *      the command -- e.g.,  ."  or  "  or suchlike -- that
	 *      starts the string.  Allow new-line to be a command-
	 *      -delimiting whitespace character.  Regard any sub-
	 *      sequent whitespace characters as part of the string
	 */
	if ( *pc == '\n' ) lineno++;
	pc++;

	got_until_eof = TRUE ;

	walk=statbuf;
	while (run) {
		switch ((c=*pc))
		{
		    /*  Standard use of '"' (Quote)  for special-char escape  */
		    case '\"':
			/*  Skip the '"' (Quote) */
				pc++;
			/*  End of the buffer also ends the string cleanly  */
			if ( pc >= end )
			{
			    run = FALSE;
			    got_until_eof = FALSE ;
				break;
			}
			/*  Pick up the next char after the '"' (Quote) */
			c=*pc;
			switch (c)
			{
			    case '(':
				pc++; /* skip the '(' */
				run = get_sequence(&walk);
				break;

			case 'n':
				add_byte_to_string( '\n', &walk);
				break;
			case 'r':
				add_byte_to_string( '\r', &walk);
				break;
			case 't':
				add_byte_to_string( '\t', &walk);
				break;
			case 'f':
				add_byte_to_string( '\f', &walk);
				break;
			case 'l':
				add_byte_to_string( '\n', &walk);
				break;
			case 'b':
				add_byte_to_string( 0x08, &walk);
				break;
			case '!':
				add_byte_to_string( 0x07, &walk);
				break;
			case '^':
				pc++;    /*   Skip the up-arrow (Caret) */
				add_byte_to_string( *pc & 0x1f , &walk);
				break;
			    /*  We're done after any of the whitespace
			     *     characters follows a quote.
			     */
			case ' ':
			case '\t':
				/*  Advance past the terminating whitespace
				 *       character, except newline.
				 *  Let  get_word()  handle that.
				 */
				pc++;
			    case '\n':
				run=FALSE;
				got_until_eof = FALSE ;
				break;
			default:
				/*  Control allowability of Quote-Backslash
				 *      as a String-Remark by means of a
				 *      command-line switch.
				 */
				if ( string_remark_escape )
				{
				    if ( c == '\\' )
				    {
					string_remark("string-escape remark");
					/* The first non-blank in the new line
					 *     has not been processed yet.
					 *     Back up to allow it to be.
					 */
					pc--;
				break;
			}
				}
				add_byte_to_string( c, &walk);
			}
			break;
		    case '\n':
			/*  Allow strings to cross lines.  Include the
			 *      newline in the string.  Account for it.
			 */
			lineno++;
		default:
			/*  Control allowability of C-style escape-character
			 *      syntax by means of a command-line switch.
			 */
			if ( c_style_string_escape )
			{
			    if ( c == '\\' )
			    {
				pc++;
				c_string_escape(&walk );
				break;
			    }
			}
			add_byte_to_string( c, &walk);
		}
		/*  Advance past the char processed, unless we're done.     */
		if ( run ) pc++;
		/*  Done if we hit end of file before string was concluded  */
		if ( pc >= end )
		{
		    run = FALSE;
		    if ( got_until_eof )
		    {
			warn_unterm( WARNING, "string", start_lineno);
			/*  Prevent multiple messages for one error  */
			got_until_eof = FALSE;
		    }
		}
	}
	
	warn_if_multiline( "string", start_lineno);

	len = walk - statbuf;
	if (len >= GET_BUF_MAX )
	{
	    tokenization_error ( TKERROR,
		"get_string buffer overflow.  Max is %d\n.", GET_BUF_MAX-1 );
	    len = GET_BUF_MAX-1;
	}
#ifdef DEBUG_SCANNER
	if (verbose)
		printf("%s:%d: debug: scanned string: '%s'\n", 
					iname, lineno, statbuf);
#endif
	if ( pack_str && (len > STRING_LEN_MAX) )
	{
	    tokenization_error ( WARNING,
		"String length being truncated to %d.\n", STRING_LEN_MAX );
	    len = STRING_LEN_MAX;
	}
	statbuf[len] = 0;

	return len ;
}


/* **************************************************************************
 *
 *      Function name:  handle_user_message
 *      Synopsis:       Collect a user-generated tokenization-time message;
 *                          either print it or discard it.  Shared code
 *                          for  user_message()  and  skip_user_message()
 *
 *      Inputs:
 *         Parameters:
 *             delim                End-of-string delimiter character.
 *                                  If it's a double-quote ("), we will use
 *                                      the get-string() routine, with all
 *                                      its options, to collect the message.
 *                                  Otherwise, we'll capture plain text from
 *                                      the input stream.
 *             print_it             TRUE if we should print the message 
 *         Local Static Variables:
 *             got_until_eof        TRUE if reached end of buffer before delim.
 *         Global Variables:
 *             lineno               Save, then restore.
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Global Variables:
 *             statbuf              The string will be collected in here
 *             
 *         Printout (if  print_it  is TRUE):
 *            The string, with new-line tacked on, will be printed from
 *                the  tokenization_error()  routine as a MESSAGE.
 *            The line-number will be shown as of the origin of the message
 *
 *      Error Detection:
 *          Error-reports will be printed regardless of  print_it  param.
 *          If delimiter was not found, show "Unterminated" warning message.
 *          If delimiter was " (double-quote), the get_string() routine
 *              already checked for a multi-line construct; if delimiter is
 *              a new-line, then a multi-line construct is impossible.
 *              otherwise, we will do the multi-line check here.
 *
 **************************************************************************** */

static void handle_user_message( char delim, bool print_it )
{
    signed long wlen;
    unsigned int start_lineno = lineno;
    unsigned int multiline_start = lineno;    /*  For warning message  */
    bool check_multiline = FALSE;
    const char *ug_msg = "user-generated message";

    if ( delim == '"' )
    {
	wlen = get_string( FALSE);
    }else{
	/*
	 *  When the message-delimiter is a new-line, and the
	 *      command-delimiter was a new-line, it means the
	 *      string length is zero; we won't bump the PC.
	 *  Otherwise, we will honor the convention we extend
	 *      to  .(  whereby, if the command is delimited
	 *      by a new-line, we allow the string to begin
	 *      on the next line.
	 */
	if ( delim == '\n' )
	{
	    if ( *pc != '\n') pc++;
	}else{
	    if ( *pc == '\n' ) lineno++;
	    pc++;
	    multiline_start = lineno;
	    check_multiline = TRUE;
	}
	wlen = get_until( delim );
    }

    if ( print_it )
    {
	unsigned int tmp_lineno = lineno;
	lineno = start_lineno;
	/*  Don't add a new-line to body of the message.
	 *  Routine already takes care of that.
	 *  Besides, buffer might be full...
	 */
	tokenization_error( MESSAGE, statbuf);
	lineno = tmp_lineno;
    }

    if ( got_until_eof )   /*  Crude but effective retrofit... */
    {
	warn_unterm(WARNING, (char *)ug_msg, start_lineno);
    }else{
	if ( check_multiline )
	{
	    warn_if_multiline( (char *)ug_msg, multiline_start);
	}
    }
}

/* **************************************************************************
 *
 *      Function name:  user_message
 *      Synopsis:       Collect a user-generated message and
 *                          print it at tokenization-time.
 *
 *      Tokenizer directive (either mode):
 *          Synonyms                              String Delimiter
 *             [MESSAGE]  #MESSAGE  [#MESSAGE]        end-of-line
 *             #MESSAGE"                                  "  
 *      "Tokenizer-Escape" mode directive         String Delimiter
 *          .(                                            )
 *          ."                                            "
 *
 *      Inputs:
 *         Parameter is the "parameter field" of the TIC entry, which
 *             was initialized to the end-of-string delimiter character.
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Printout:                User-message, parsed from input.
 *
 *      Extraneous Remarks:
 *          We would have preferred to simply use the "character value"
 *              aspect of the union, but we found portability issues
 *              between big- and little- -endian processors, so we still
 *              have to recast its type here.
 *
 **************************************************************************** */

void user_message( tic_param_t pfield )
{
    char delim = (char)pfield.deflt_elem ;
    handle_user_message( delim, TRUE);
}

/* **************************************************************************
 *
 *      Function name:  skip_user_message
 *      Synopsis:       Collect a user-generated message and discard it.
 *                          Used when ignoring a Conditional section.
 *
 *      Tokenizer directive (either mode):
 *          Synonyms                              String Delimiter
 *             [MESSAGE]  #MESSAGE  [#MESSAGE]        end-of-line
 *             #MESSAGE"                                  "  
 *      "Tokenizer-Escape" mode directive         String Delimiter
 *          .(                                            )
 *          ."                                            "
 *
 *      Inputs:
 *         Parameters:
 *             pfield               "Parameter field" of the TIC entry, which
 *                                      was initialized to the delimiter.
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Printout:                NONE
 *
 **************************************************************************** */

void skip_user_message( tic_param_t pfield )
{
    char delim = (char)pfield.deflt_elem ;
    handle_user_message( delim, FALSE);
}



/* **************************************************************************
 *
 *      Function name:  get_number
 *      Synopsis:       If the word retrieved from the input stream is a
 *                      valid number (under the current base) convert it.
 *                      Return an indication if it was not.
 *
 *      Inputs:
 *         Parameters:
 *             *result             Pointer to place to return the number
 *         Global Variables:
 *             statbuf             The word just read that is to be converted.
 *             base                The current numeric-interpretation base.
 *
 *      Outputs:
 *         Returned Value:         TRUE = Input was a valid number
 *         Supplied Pointers:
 *             *result             The converted number, if valid
 *                                     otherwise undefined
 *
 *      Revision History:
 *          Updated Mon, 28 Mar 2005 by David L. Paktor
 *              Always use the current base.
 *              Reversed sense of return-flag.
 *
 **************************************************************************** */

bool get_number( long *result)
{
    u8 *until;
    long val;
    bool retval = FALSE ;

    val = parse_number(statbuf, &until, base);
	
#ifdef DEBUG_SCANNER
    printf("%s:%d: debug: parsing number: base 0x%x, val 0x%lx, "
		"processed %ld of %ld bytes\n", iname, lineno, 
		 base, val,(size_t)(until-statbuf), strlen((char *)statbuf));
#endif

    /*  If number-parsing ended before the end of the input word,
     *      then the input word was not a valid number.
     */
    if (until==(statbuf+strlen((char *)statbuf)))
    {
	*result=val;
	retval = TRUE;
    }

    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  deliver_number
 *      Synopsis:       Deliver the supplied number according to the
 *                              state of the tokenizer:
 *                          In normal tokenization mode, emit it as an
 *                              FCode literal.
 *                          In  "Tokenizer-Escape" mode, push it onto
 *                              the Data Stack.
 *
 *      Inputs:
 *         Parameters:
 *             numval                  The number, verified to be valid.
 *         Global Variables:
 *             in_tokz_esc   TRUE if tokenizer is in "Tokenizer Escape" mode.
 *
 *      Outputs:
 *         Returned Value:             NONE 
 *         Items Pushed onto Data-Stack:
 *             Top:                    The number, if  in_tokz_esc  was TRUE
 *         FCode Output buffer:
 *             If  in_tokz_esc  was FALSE, a  b(lit)  token will be written,
 *                 followed by the number.
 *
 **************************************************************************** */

static void deliver_number( long numval)
{
    if ( in_tokz_esc )
    {
        dpush( numval );
    } else {
        emit_literal(numval);
    }
}
/* **************************************************************************
 *
 *      Function name:  handle_number
 *      Synopsis:       Convert the word just retrieved from the input stream
 *                              to a number.
 *                      Indicate whether the string was a valid number and
 *                              deliver it, as appropriate.
 *
 *      Inputs:
 *         Parameters:                 NONE
 *         Global Variables:
 *             statbuf       The word that was just read, and to be converted.
 *
 *      Outputs:
 *         Returned Value:    TRUE = Input string was a valid number
 *         If input string was a valid number, the converted number will
 *             be delivered, as appropriate, by  deliver_number(). 
 *
 **************************************************************************** */

static bool handle_number( void )
{
    bool retval ;
    long numval;

    retval = get_number( &numval );
    if ( retval )
    {
        deliver_number( numval );
    }

    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  ascii_right_number
 *      Synopsis:       Convert a character sequence to a number, justified
 *                          toward the right (i.e., the low-order bytes) and
 *                          deliver it, as appropriate.
 *
 *      Inputs:
 *         Parameters:
 *             in_str                  The input string
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         The converted number will be delivered by  deliver_number(). 
 *
 *      Process Explanation:
 *          The last four characters in the sequence will become the number.
 *          If there are fewer than four, they will fill the low-order part
 *              of the number.
 *          Example:  PCIR            is converted to  h# 50434952
 *                    CPU             is converted to  h# 00435055
 *             and
 *          	      LotsOfStuff     is equivalent to  a# tuff 
 *                                    and is converted to  h# 74756666
 *
 **************************************************************************** */

static void ascii_right_number( char *in_str)
{
    u8 nxt_ch;
    char *str_ptr = in_str;
    long numval = 0;

    for ( nxt_ch = (u8)*str_ptr ;
	    ( nxt_ch = (u8)*str_ptr ) != 0 ;
        	str_ptr++ )
    {
        numval = ( numval << 8 ) + nxt_ch ;
    }
    deliver_number( numval );
}


/* **************************************************************************
 *
 *      Function name:  ascii_left_number
 *      Synopsis:       Similar to  ascii_right_number()  except justified
 *                          toward the left (i.e., the high-order bytes).
 *                      
 *
 *      Inputs:
 *         Parameters:
 *             in_str                  The input string
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         The converted number will be delivered by  deliver_number().
 *
 *      Process Explanation:
 *          If there are fewer than four characters in the sequence, they
 *              will fill the high-order part of the number.
 *                    CPU             is converted to  h# 43505500
 *          In all other respects, similar to  ascii_right_number()
 *
 **************************************************************************** */

static void ascii_left_number( char *in_str)
{
    u8 nxt_ch;
    char *str_ptr = in_str;
    long numval = 0;
    int shift_amt = 24;
    bool shift_over = FALSE ;

    for ( nxt_ch = (u8)*str_ptr ;
	    ( nxt_ch = (u8)*str_ptr ) != 0 ;
        	str_ptr++ )
    {
        if ( shift_over )  numval <<= 8;
	if ( shift_amt == 0 )  shift_over = TRUE ;
	numval += ( nxt_ch << shift_amt );
	if ( shift_amt > 0 ) shift_amt -= 8;
    }
    deliver_number( numval );

}

/* **************************************************************************
 *
 *      Function name:  init_scanner
 *      Synopsis:       Allocate memory the Scanner will need.
 *                          Only need to call once per program run.
 *
 **************************************************************************** */

void init_scanner(void)
{
	statbuf=safe_malloc(GET_BUF_MAX, "initting scanner");
}

/* **************************************************************************
 *
 *      Function name:  exit_scanner
 *      Synopsis:       Free up memory the Scanner used
 *
 **************************************************************************** */

void exit_scanner(void)
{
	free(statbuf);
}

/* **************************************************************************
 *
 *      Function name:  set_hdr_flag
 *      Synopsis:       Set the state of the "headered-ness" flag to the
 *                          value given, unless over-ridden by one or both
 *                          of the "always-..." Command-Line Flags
 *
 *      Inputs:
 *         Parameters:
 *             new_flag                  New setting
 *         Global Variables:
 *             always_headers            Override HEADERLESS and make HEADERS
 *             always_external           Override HEADERLESS and HEADERS;
 *                                           make EXTERNAL
 *
 *      Outputs:
 *         Returned Value:               None
 *         Global Variables:
 *             hdr_flag                  Adjusted to new setting
 *
 *      Process Explanation:
 *          If  always_headers  is TRUE, and  new_flag  is not FLAG_EXTERNAL
 *              then set to FLAG_HEADERS
 *          If  always_external  is TRUE, set to FLAG_EXTERNAL, regardless.
 *              (Note:  always_external  over-rides  always_headers).
 *          Otherwise, set to  new_flag
 *
 **************************************************************************** */

static void set_hdr_flag( headeredness new_flag)
{
    headeredness new_state = new_flag;
    switch ( new_flag)
    {
	case FLAG_HEADERLESS:
	    {
		if ( always_headers )
		{   new_state = FLAG_HEADERS;
		}
	    /*  No  break.  Intentional...   */
	    }
	case FLAG_HEADERS:
	    {
		if ( always_external )
		{   new_state = FLAG_EXTERNAL;
		}
	    /*  No  break.  Intentional...   */
	    }
	case FLAG_EXTERNAL:
	    break;  /*  Satisfy compiler's error-checking...   */
	/*  No default needed here...   */
    }

    hdr_flag = new_state;

}


/* **************************************************************************
 *
 *      Function name:  init_scan_state
 *      Synopsis:       Initialize various state variables for each time
 *                          a new tokenization scan is started.
 *
 *      Inputs:
 *         Parameters:             NONE
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Global Variables:   Initialized to:
 *             base                            0x0a (I.e., base = "decimal")
 *             nextfcode                       By  reset_fcode_ranges()
 *             pci_is_last_image               TRUE
 *             incolon                         FALSE
 *         Local Static Variables:
 *             hdr_flag                  FLAG_HEADERLESS (unless over-ridden)
 *             is_instance                     FALSE
 *             last_colon_filename             NULL
 *             instance_filename               NULL
 *             dev_change_instance_warning     TRUE
 *             instance_definer_gap            FALSE
 *             need_to_pop_source              FALSE
 *             first_fc_starter                TRUE
 *             ret_stk_depth                   0
 *         Memory Freed
 *             Copies of input-file name in  last_colon_filename  and
 *                 instance_filename , if allocated.
 *
 **************************************************************************** */

void init_scan_state( void)
{
    base = 0x0a;
    pci_is_last_image = TRUE;
    incolon = FALSE;
    is_instance = FALSE;
    set_hdr_flag( FLAG_HEADERLESS);
    reset_fcode_ranges();
    first_fc_starter = TRUE;
    if ( last_colon_filename != NULL ) free( last_colon_filename);
    if ( instance_filename != NULL ) free( instance_filename);
    last_colon_filename = NULL;
    instance_filename = NULL;
    dev_change_instance_warning = TRUE;
    instance_definer_gap = FALSE;
    need_to_pop_source = FALSE;
    ret_stk_depth = 0;
}


/* **************************************************************************
 *
 *      Function name:  collect_input_filename
 *      Synopsis:       Save a copy of the current input file name in the
 *                          given variable, for error-reporting purposes
 *
 *      Inputs:
 *         Parameters:
 *             saved_nam                    Pointer to pointer for copy of name
 *         Global Variables:
 *             iname                        Current input file name
 *         Local Static Variables:
 *
 *      Outputs:
 *         Returned Value:                  NONE
 *         Supplied Pointers:
 *             *saved_nam                   Copy of name
 *         Memory Allocated
 *             For copy of input file name
 *         When Freed?
 *             Subsequent call to this routine with same pointer
 *             (Last copy made will be freed if starting a new tokenization,
 *                 otherwise, will persist until end of program.) 
 *         Memory Freed
 *             Previous copy in same pointer.
 *
 *      Process Explanation:
 *          If there is a previous copy, and it still matches the current
 *              input-file name, we don't need to free or re-allocate.
 *
 **************************************************************************** */

static void collect_input_filename( char **saved_nam)
{
    bool update_lcfn = TRUE;    /*  Need to re-allocate?  */
    if ( *saved_nam != NULL )
    {
	if ( strcmp( *saved_nam, iname) == 0 )
	{
	    /*  Last collected filename unchanged from iname  */
	    update_lcfn = FALSE;
	}else{
	    free( *saved_nam);
	}
    }
    if ( update_lcfn )
    {
	*saved_nam = strdup(iname);
    }
} 

/* **************************************************************************
 *
 *      Function name:  test_in_colon
 *      Synopsis:       Error-check whether a word is being used in the
 *                      correct state, relative to being inside a colon
 *                      definition; issue a message if it's not.
 *      
 *      Inputs:
 *         Parameters:
 *             wname            The name of the word in question
 *             sb_in_colon      TRUE if the name should be used inside
 *                                  a colon-definition only; FALSE if
 *                                  it may only be used outside of a
 *                                  colon-definition.
 *             severity         Type of error/warning message to call.
 *                                  usually either WARNING or TKERROR
 *             use_instead      Word the error-message should suggest be
 *                                  used "instead".  This may be a NULL,
 *                                  in which case the "suggestion" part
 *                                  of the message will simply be omitted.
 *         Global Variables:
 *             incolon          State of the tokenization; TRUE if inside
 *                                  a colon definition
 *
 *      Outputs:
 *         Returned Value:     TRUE if no error.
 *         Printout:           Error messages as indicated.
 *
 *      Error Detection:
 *          If the state, relative to being inside a colon-definition,
 *              is not what the parameter says it should be, issue a
 *              message of the indicated severity, and return FALSE.
 *
 **************************************************************************** */

static bool test_in_colon ( char *wname,
                           bool sb_in_colon,    /*  "Should Be IN colon"  */
			        int severity,
				     char *use_instead)
{
    bool is_wrong;
    bool retval = TRUE ;

    is_wrong = BOOLVAL(( sb_in_colon != FALSE ) != ( incolon != FALSE )) ;
    if ( is_wrong )
    {  
        char *ui_pt1 = "";
        char *ui_pt2 = "";
        char *ui_pt3 = "";
	retval = FALSE;
	if ( use_instead != NULL )
	{
	    ui_pt1 = "  Use  ";
	    ui_pt2 = use_instead;
	    ui_pt3 = "  instead.";
	}
	tokenization_error ( severity, "The word  %s  should not be used "
	    "%sside of a colon definition.%s%s%s\n", strupr(wname),
	        sb_in_colon ? "out" : "in", ui_pt1, ui_pt2, ui_pt3 );
    }
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  must_be_deep_in_do
 *      Synopsis:       Check that the statement in question is called 
 *                          from inside the given depth of structures
 *                          of the  DO ... LOOP -type (i.e., any combination
 *                          of DO  or ?DO  and  LOOP  or  +LOOP ).
 *                      Show an error if it is not.
 *
 **************************************************************************** */

static void must_be_deep_in_do( int how_deep )
{
    int functional_depth = do_loop_depth;
    if ( incolon )
    {
        functional_depth -= last_colon_do_depth;
    }
    if ( functional_depth < how_deep )
    {
	char deep_do[64] = "";
	int indx;
	bool prefix = FALSE;

	for ( indx = 0; indx < how_deep ; indx ++ )
	{
	    strcat( deep_do, "DO ... ");
	}
	for ( indx = 0; indx < how_deep ; indx ++ )
	{
	    if ( prefix )
	    {
		strcat( deep_do, " ... ");
	    }
	    strcat( deep_do, "LOOP");
	    prefix = TRUE;
	}

	tokenization_error( TKERROR,
	    "%s outside of  %s  structure", strupr(statbuf), deep_do);
	in_last_colon( TRUE);
    }

}

/* **************************************************************************
 *
 *      Function name:  bump_ret_stk_depth
 *      Synopsis:       Increment or decrement the Return-Stack-Usage-Depth
 *                          counter.
 *
 *      Inputs:
 *         Parameters:
 *             bump              Amount by which to increment;
 *                                   negative number to decrement.
 *         Local Static Variables:
 *             ret_stk_depth     The Return-Stack-Usage-Depth counter
 *
 *      Outputs:
 *         Returned Value:        NONE
 *         Local Static Variables:
 *             ret_stk_depth     Incremented or decremented
 *
 *      Process Explanation:
 *          This simple-seeming function is actually a place-holder
 *             for future expansion.  Proper error-detection of
 *             Return-Stack usage is considerably more complex than
 *             what we are implementing here, and is deferred for a
 *             later revision.
 *
 *      Still to be done:
 *          Full detection of whether the Return-Stack has been cleared
 *              when required, including analysis of Return-Stack usage
 *              within Flow-Control constructs, and before Loop elements...
 *
 *      Extraneous Remarks:
 *          Some FORTHs use a Loop-Control stack separate from the Return-
 *              -Stack, but others use the Return-Stack to keep LOOP-control
 *              elements.  An FCode program must be portable between different
 *              environments, and so must adhere to the restrictions listed
 *              in the ANSI Spec:
 *
 *       3.2.3.3   Return stack  
 *        . . . . . .
 *       A program may use the return stack for temporary storage during the
 *          execution of a definition subject to the following restrictions:
 *       	A program shall not access values on the return stack (using R@,
 *       	    R>, 2R@ or 2R>) that it did not place there using >R or 2>R;
 *       	A program shall not access from within a do-loop values placed
 *       	    on the return stack before the loop was entered;
 *       	All values placed on the return stack within a do-loop shall
 *       	    be removed before I, J, LOOP, +LOOP, UNLOOP, or LEAVE is
 *       	    executed;
 *       	All values placed on the return stack within a definition
 *       	    shall be removed before the definition is terminated
 *       	    or before EXIT is executed.
 *
 **************************************************************************** */

static void bump_ret_stk_depth( int bump)
{
    ret_stk_depth += bump;
}


/* **************************************************************************
 *
 *      Function name:  ret_stk_balance_rpt
 *      Synopsis:       Display a Message if usage of the Return-Stack
 *                          appears to be out of balance.
 *
 *      Inputs:
 *         Parameters:
 *             before_what         Phrase to use in Message;
 *                                     if NULL, use statbuf...
 *             clear_it            TRUE if this call should also clear the
 *                                     Return-Stack-Usage-Depth counter
 *         Global Variables:
 *             statbuf             Word currently being processed
 *         Local Static Variables:
 *             ret_stk_depth       The Return-Stack-Usage-Depth counter
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Local Static Variables:
 *             ret_stk_depth       May be cleared
 *
 *      Error Detection:
 *          Based simply on whether the Return-Stack-Usage-Depth counter
 *              is zero.  This is a weak and uncertain implementation;
 *              therefore, the Message will be a WARNING phrased with
 *              some equivocation.
 *
 *      Process Explanation:
 *          Proper detection of Return-Stack usage errors is considerably
 *              more complex, and is deferred for a future revision.
 *
 *      Still to be done:
 *          Correct analysis of Return-Stack usage around Flow-Control
 *              constructs.  Consider, for instance, the following:
 * 
 *          blablabla >R  yadayada IF  R> gubble ELSE flubble R>  THEN
 * 
 *              It is, in fact, correct, but the present scheme would
 *              tag it as a possible error.  Conversely, something like:
 * 
 *          blablabla >R  yadayada IF  R> gubble THEN
 * 
 *              would not get tagged, even though it is actually an error.
 * 
 *          The current simple scheme also does not cover Return-Stack
 *              usage within Do-Loops or before Loop elements like I and
 *              J or UNLOOP or LEAVE.  Implementing something like that
 *              would probably need to be integrated in with Flow-Control
 *              constructs, and will be noted in  flowcontrol.c
 *
 **************************************************************************** */

static void ret_stk_balance_rpt( char *before_what, bool clear_it)
{
    if ( ret_stk_depth != 0 )
    {
	char *what_flow = ret_stk_depth < 0 ? "deficit" : "excess" ;
	char *what_phr =  before_what != NULL ? before_what : strupr(statbuf);

	tokenization_error( WARNING,
	    "Possible Return-Stack %s before %s", what_flow, what_phr);
	in_last_colon( TRUE);

	if ( clear_it )
	{
	    ret_stk_depth = 0;
	}
    }
}


/* **************************************************************************
 *
 *      Function name:  ret_stk_access_rpt
 *      Synopsis:       Display a Message if an attempt to access a value
 *                          on the Return-Stack appears to occur before
 *                          one was placed there.
 *
 *      Inputs:
 *         Parameters:                NONE
 *         Global Variables:
 *             statbuf                Word currently being processed
 *         Local Static Variables:
 *             ret_stk_depth          The Return-Stack-Usage-Depth counter
 *
 *      Outputs:
 *         Returned Value:             NONE
 *
 *      Error Detection:
 *          Equivocal WARNING, based simply on whether the Return-Stack-
 *              -Usage-Depth counter not positive.
 *
 *      Process Explanation:
 *          Proper detection is deferred...
 *
 *      Still to be done:
 *          Correct analysis of Return-Stack usage...
 *
 **************************************************************************** */

static void ret_stk_access_rpt( void)
{
    if ( ret_stk_depth <= 0 )
    {
	tokenization_error( WARNING,
	    "Possible Return-Stack access attempt by %s "
		"without value having been placed there",
		strupr(statbuf) );
	in_last_colon( TRUE);
    }
}



/* **************************************************************************
 *
 *      Function name:  encode_file
 *      Synopsis:       Input a (presumably binary) file and encode it
 *                      as a series of strings which will be accumulated
 *                      and encoded in a manner appropriate for a property.
 *
 *      Associated Tokenizer directive:        encode-file        
 *
 *      Error Detection:
 *          Handled by support routines.
 *
 **************************************************************************** */

static void encode_file( const char *filename )
{
	FILE *f;
	size_t s;
	int num_encoded=0;
	
	tokenization_error( INFO, "ENCODing File %s\n", filename );

	f = open_expanded_file( filename, "rb", "encoding");
	if( f != NULL )
	{
	    while( (s=fread(statbuf, 1, STRING_LEN_MAX, f)) )
	    {
		    emit_token("b(\")");
		    emit_string(statbuf, s);
		    emit_token("encode-bytes");
		    if( num_encoded )
			    emit_token("encode+");
		    num_encoded += s;
	    }
	    fclose( f );
	    tokenization_error ( INFO, "ENCODed %d bytes.\n", num_encoded);
	}
}

/* **************************************************************************
 *
 *      Function name:  check_name_length
 *      Synopsis:       If the length of a user-defined name exceeds the
 *                          ANSI-specified maximum of 31 characters, issue
 *                          a message.  This is a hard-coded limit.
 *                      Although our Tokenizer can handle longer names,
 *                          they will cause big problems when encountered
 *                          by an FCode interpreter.
 *                      If the name is going to be included in the binary
 *                          output, the message severity must be an ERROR.
 *                      Otherwise, if the name is HEADERLESS, the severity
 *                          can be reduced to a Warning; if the name is only
 *                          defined in "Tokenizer Escape" mode the message
 *                          severity can be further reduced to an Advisory.
 *
 *      Inputs:
 *         Parameters:
 *             wlen                 Length of the newly-created word
 *         Global Variables: 
 *             in_tokz_esc          TRUE if in "Tokenizer Escape" mode.
 *         Local Static Variables:
 *             hdr_flag             State of headered-ness for name-creation
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Global Variables:        
 *         Printout:                ERROR message if applicable.
 *
 *      Error Detection:
 *             The whole point of this routine.  
 *
 *      Revision History:
 *          Updated Wed, 20 Jul 2005 by David L. Paktor
 *               Escalated from merely an informative warning to a TKERROR 
 *          Updated Fri, 21 Oct 2005 by David L. Paktor
 *               Adjust severity if name doesn't go into the FCode anyway...
 *
 **************************************************************************** */

void check_name_length( signed long wlen )
{
    if ( wlen > 31 )
    {
	int severity = TKERROR;
	if ( in_tokz_esc )
	{   severity = INFO;
	}else{
	    if (hdr_flag == FLAG_HEADERLESS)
	    {   severity = WARNING;
	    }
	}
	tokenization_error( severity,
	    "ANSI Forth does not permit definition of names "
		"longer than 31 characters.\n" );
    }

}


/* **************************************************************************
 *
 *      Function name:  definer_name
 *      Synopsis:       Given a defining-word internal token, return
 *                      a printable string for the definer, for use
 *                      in an error-message.
 *
 *      Inputs:
 *         Parameters:
 *             definer             Internal token for the defining-word
 *             reslt_ptr           Pointer to string-pointer that takes
 *                                     the result, if successful
 *
 *      Outputs:
 *         Returned Value:         TRUE if definer was recognized
 *         Supplied Pointers:
 *             *reslt_ptr          If successful, points to printable string;
 *                                     otherwise, left unchanged.
 *
 *
 **************************************************************************** */

bool definer_name(fwtoken definer, char **reslt_ptr)
{
    bool retval = TRUE;
    switch (definer)
    {
	case VARIABLE:
	    *reslt_ptr = "VARIABLE";
	    break;
	case DEFER:
	    *reslt_ptr = "DEFER";
	    break;
	case VALUE:
	    *reslt_ptr = "VALUE";
	    break;
	case BUFFER:
	    *reslt_ptr = "BUFFER";
	    break;
	case CONST:
	    *reslt_ptr = "CONSTANT";
	    break;
	case COLON:
	    *reslt_ptr = "COLON";
	    break;
	case CREATE:
	    *reslt_ptr = "CREATE";
	    break;
	case FIELD:
	    *reslt_ptr = "FIELD";
	    break;
	case MACRO_DEF:
	    *reslt_ptr = "MACRO";
	    break;
	case ALIAS:
	    *reslt_ptr = "ALIAS";
	    break;
	case LOCAL_VAL:
	    *reslt_ptr = "Local Value name";
	    break;
	default:
	    retval = FALSE;
    }

    return ( retval);
}


/* **************************************************************************
 *
 *      Function name:  as_a_what
 *      Synopsis:       Add the phrase "as a[n] <DEF'N_TYPE>" for the given
 *                          definition-type to the given string buffer.
 *
 *      Inputs:
 *         Parameters:
 *             definer                 Internal token for the defining-word
 *             as_what                 The string buffer to which to add.
 *
 *      Outputs:
 *         Returned Value:             TRUE if an assigned name was found
 *                                         for the given definer and text
 *                                         was added to the buffer.
 *         Supplied Pointers:
 *             *as_what                Text is added to this buffer.
 *
 *      Process Explanation:
 *          The calling routine is responsible to make sure the size of
 *              the buffer is adequate.  Allow 25 for this routine.
 *          The added text will not have spaces before or after; if any
 *              are needed, they, too, are the responsibility of the
 *              calling routine.  The return value gives a helpful clue.
 *
 *      Extraneous Remarks:
 *          We define a Macro -- kept in scanner.h --that will give the
 *             recommended length for the buffer passed to this routine.
 *             It will be called  AS_WHAT_BUF_SIZE 
 *
 **************************************************************************** */

bool as_a_what( fwtoken definer, char *as_what)
{
    char *defn_type_name;
    bool retval = definer_name(definer, &defn_type_name);
    if ( retval )
    {
	strcat( as_what, "as a");
	/*  Handle article preceding definer name
	 *      that starts with a vowel.
	 */
	/*  HACK:  Only one definer name -- ALIAS --
	 *      begins with a vowel.  Take advantage
	 *      of that...
	 *  Otherwise, we'd need to do something involving
	 *      strchr( "AEIOU", defn_type_name[0] )
	 */
	if ( definer == ALIAS ) strcat( as_what, "n" );

	strcat( as_what, " ");
	strcat( as_what, defn_type_name);
    }
    return( retval);
}


/* **************************************************************************
 *
 *      Function name:  lookup_word
 *      Synopsis:       Find the TIC-entry for the given word in the Current
 *                          mode -- relative to "Tokenizer-Escape" -- and
 *                          Scope into which definitions are being entered.
 *                      Optionally, prepare text for various Message types.
 *
 *      Inputs:
 *         Parameters:
 *             stat_name               Word to look up
 *             where_pt1               Pointer to result-display string, part 1
 *                                         NULL if not preparing text
 *             where_pt2               Pointer to result-display string, part 2
 *                                         NULL if not preparing text
 *         Global Variables:
 *             in_tokz_esc             TRUE if in "Tokenizer Escape" mode.
 *             scope_is_global         TRUE if "global" scope is in effect
 *             current_device_node     Current dev-node data-struct
 *             ibm_locals              TRUE if IBM-style Locals are enabled
 *
 *      Outputs:
 *         Returned Value:             Pointer to TIC-entry; NULL if not found
 *         Supplied Pointers:
 *             *where_pt1              Result display string, part 1 of 2
 *             *where_pt2              Result display string, part 2 of 2
 *
 *      Process Explanation:
 *          We will set the two-part result-display string in this routine
 *              because only here do we know in which vocabulary the word
 *              was found.
 *          Pre-load the two parts of the result-display string.
 *          If we are in "Tokenizer Escape" mode, look up the word:  first,
 *              in the "Tokenizer Escape" Vocabulary, or, if not found,
 *              among the "Shared" words.
 *          Otherwise, we're in Normal" mode.  Look it up:  first, among the
 *              Locals, if IBM-style Locals are enabled (it can possibly be
 *              one if "Tokenizer Escape" mode was entered during a colon-
 *              -definition); then, if it was not found and if "Device"
 *              scope is in effect, look in the current device-node; then,
 *              if not found, in the "core" vocabulary.
 *          Load the second part of the result-display string with the
 *               appropriate phrase for whereever it was found.
 *          Then adjust the first part of the result-display string with
 *               the definer, if known.
 *
 *          The two strings will be formatted to be printed adjacently,
 *              without any additional spaces in the printf() format.
 *          The first part of the result-display string will not start with
 *              a space, but will have an intermediate space if necessary.
 *          The second part of the result-display string will not start
 *              with a space, and will contain the terminating new-line
 *              if appropriate.  It might or might not have been built
 *              with a call to  in_what_node().
 *
 *          If the calling routine displays the result-display strings,
 *              it should follow-up with a call to  show_node_start()
 *              This will be harmless if  in_what_node()  was not used
 *              in the construction of the result-display string.
 *          If the calling routine is NOT going to display the result strings,
 *              it should pass NULLs for the string-pointer pointers.
 *
 *          The second part of the string consists of pre-coded phrases;
 *              therefore, we can directly assign the pointer.
 *          The first part of the string, however, has developed into
 *              something constructed "on the fly".  Earlier, it, too,
 *              had been a directly-assignable pointer; all the callers
 *              to this routine expect that.  Rather than change all the
 *              callers, we will assign a local buffer for it.
 *
 *      Extraneous Remarks:
 *          We had to add the rule allowing where_pt1 or where_pt2 to be
 *              NULL after we introduced the  in_what_node()  function.
 *              We had cases where residue from a lookup for processing
 *              showed up later in an unrelated Message.  The NULL rule
 *              should prevent that.
 *
 **************************************************************************** */

static char lookup_where_pt1_buf[AS_WHAT_BUF_SIZE];

tic_hdr_t *lookup_word( char *stat_name, char **where_pt1, char **where_pt2 )
{
    tic_hdr_t *found = NULL;
    bool trail_space = TRUE;
    bool doing_lookup = BOOLVAL( ( where_pt1 != NULL )
			      && ( where_pt2 != NULL ) );
    char *temp_where_pt2 = "in the core vocabulary.\n";

    lookup_where_pt1_buf[0] = 0;             /*  Init'lz part-1 buffer  */

    /*  "Core vocab" refers both to shared fwords and built-in tokens.  */

    /*  Distinguish between "Normal" and "Tokenizer Escape" mode  */
    if ( in_tokz_esc )
    {   /*  "Tokenizer Escape" mode.  */
	found = lookup_tokz_esc( stat_name);
	if ( found != NULL )
	{
	    temp_where_pt2 = in_tkz_esc_mode;
	}else{
	    /*  "Core vocabulary".  */
	    found = lookup_shared_word( stat_name);
	}
    }else{
	/*  "Normal" tokenization mode  */
	if ( ibm_locals )
	{
	    found = lookup_local( stat_name);
	    if ( doing_lookup && ( found != NULL ) )
	    {
		trail_space = FALSE;
		temp_where_pt2 = ".\n";
	    }
	}

	if ( found == NULL )
	{
	    found = lookup_in_dev_node( stat_name);
	    if ( found != NULL )
	    {
		if ( doing_lookup )
		{
		    temp_where_pt2 = in_what_node( current_device_node);
		}
	    }else{
		/*  "Core vocabulary".  */
		found = lookup_core_word( stat_name);
	    }
	}
    }

    if ( ( doing_lookup ) && ( found != NULL ) )
    {
	if ( as_a_what( found->fword_defr, lookup_where_pt1_buf) )
	{
	    if ( trail_space )
	    {
		strcat(lookup_where_pt1_buf, " ");
	    }
	}
	*where_pt1 = lookup_where_pt1_buf;
	*where_pt2 = temp_where_pt2;
    }
    return( found);
}

/* **************************************************************************
 *
 *      Function name:  word_exists
 *      Synopsis:       Check whether a given word is already defined in the
 *                          Current mode -- relative to "Tokenizer-Escape" --
 *                          and Scope into which definitions are being entered. 
 *                      Used for error-reporting.
 *
 *      Inputs:
 *         Parameters:
 *             stat_name                 Word to look up
 *             where_pt1                 Pointer to string, part 1 of 2,
 *                                          to display in result
 *             where_pt2                 Pointer to string, part 2 of 2,
 *                                          to display in result
 *
 *      Outputs:
 *         Returned Value:               TRUE if the name exists.
 *         Supplied Pointers:
 *             *where_pt1                Result display string, part 1 of 2
 *             *where_pt2                Result display string, part 2 of 2
 *
 *      Process Explanation:
 *          If the calling routine displays the result-display strings,
 *              it should follow-up with a call to  show_node_start()
 *
 *      Extraneous Remarks:
 *          This used to be a much heftier routine; now it's just
 *              a wrapper around  lookup_word() .
 *
 **************************************************************************** */

bool word_exists( char *stat_name, char **where_pt1, char **where_pt2 )
{
    bool retval = FALSE;
    tic_hdr_t *found = lookup_word( stat_name, where_pt1, where_pt2 );

    if ( found != NULL )
    {
	retval = TRUE;
    }

    return( retval);
}

/* **************************************************************************
 *
 *      Function name:  warn_if_duplicate
 *      Synopsis:       Check whether a given word is already defined in
 *                          the current mode and issue a warning if it is.
 *
 *      Inputs:
 *         Parameters:
 *             stat_name                Word to check
 *         Global Variables:
 *             verbose_dup_warning      Whether to run the check at all.
 *         Local Static Variables:
 *             do_not_overload          FALSE if  OVERLOAD  is in effect.
 *
 *      Outputs:
 *         Returned Value:              NONE
 *         Local Static Variables:
 *             do_not_overload          Restored to TRUE
 *         Printout:
 *             Warning message if a duplicate.
 *
 *      Error Detection:
 *             None.  This is merely an informative warning.
 *
 *      Process Explanation:
 *          "Current mode" -- meaning, whether the tokenizer is operating
 *              in "Tokenizer Escape" mode or in normal tokenization mode --
 *              will be recognized by the  word_exists()  function.
 *
 *      Extraneous Remarks:
 *          The  OVERLOAD  directive is our best shot at creating a more
 *              fine-grained way to temporarily bypass this test when
 *              deliberately overloading a name.  It would be nice to have
 *              a mechanism, comparable to the classic
 *                     WARNING @ WARNING OFF  .....  WARNING !
 *              that could be applied to a range of definitions, but:
 *              (1)  That would require more of a true FORTH infrastructure;
 *                       hence, more effort than I am willing to invest, at
 *                       this juncture, for such a small return,
 *              and
 *              (2)  Most intentional-overloading ranges only cover a
 *                       single definition anyway.
 *
 **************************************************************************** */

void warn_if_duplicate( char *stat_name)
{
    if ( verbose_dup_warning && do_not_overload )
    {
	char *where_pt1;
	char *where_pt2; 
	if ( word_exists( stat_name, &where_pt1, &where_pt2) )
	{
	    tokenization_error( WARNING, 
		"Duplicate definition:   %s  already exists %s%s",
		    stat_name, where_pt1, where_pt2 );
	    show_node_start();
	}
    }
    do_not_overload = TRUE;
}


/* **************************************************************************
 *
 *      Function name:  glob_not_allowed
 *      Synopsis:       Print a Message that "XXX is not allowed."
 *                          because Global Scope is in effect.
 *                      Used from several places...
 *      
 *      Inputs:
 *         Parameters:
 *             severity              Severity of the Message
 *             not_ignoring          FALSE = "Ignoring", for the part of the
 *                                       message about "How It's being Handled"
 *         Global Variables:
 *             statbuf               Disallowed word currently being processed
 *
 *      Outputs:
 *         Returned Value:           NONE
 *         Printout:                 Message of given severity.
 *
 **************************************************************************** */

static void glob_not_allowed( int severity, bool not_ignoring)
{
    tokenization_error( severity, "Global Scope is in effect; "
			"%s not allowed.  %s.\n",
			    strupr(statbuf), 
				 not_ignoring ?
				     "Attempting to compensate.." :
					  "Ignoring" );
}


/* **************************************************************************
 *
 *      Function name:  not_in_dict
 *      Synopsis:       Print the message "XXX is not in dictionary."
 *                      Used from several places...
 *      
 *      Inputs:
 *         Parameters:
 *             stat_name                Word that could not be processed
 *
 *      Outputs:
 *         Returned Value:              NONE
 *         Printout:         Error message.
 *
 **************************************************************************** */

static void not_in_dict( char *stat_name)
{
    tokenization_error ( TKERROR,
        "Word  %s  is not in dictionary.\n", stat_name);
}

/* **************************************************************************
 *
 *      Function name:  tokenized_word_error
 *      Synopsis:       Report an error when a word could not be processed
 *                          by the tokenizer.  Messages will vary...
 *      
 *      Inputs:
 *         Parameters:
 *             stat_name                Word that could not be processed
 *         Global Variables:
 *             in_tokz_esc    TRUE if tokenizer is in "Tokenizer Escape" mode.
 *
 *      Outputs:
 *         Returned Value:              NONE
 *         Printout:
 *             Error message.
 *             Possible Advisory about where the word might be found.
 *             Trace-Note, if the word was on the Trace-List
 *
 *      Error Detection:
 *          Error was detected by the calling routine...
 *
 *      Process Explanation:
 *          If the tokenizer is in "Tokenizer Escape" mode, the word might
 *              be one that can be used in normal tokenization mode;
 *          Conversely, if the tokenizer is in normal-tokenization mode,
 *              the word might be one that can be used in the "Escape" mode.
 *          Or, the word is completely unknown.
 *          Recognizing the current mode is handled by  word_exists()
 *          However, we need to test for the *converse* of the current mode,
 *              so before we call  word_exists()  we are going to save and
 *              invert the setting of  in_tokz_esc  (and afterwards, of
 *              course, restore it...)
 *
 **************************************************************************** */

static void tokenized_word_error( char *stat_name)
{
    char *where_pt1;
    char *where_pt2;
    bool found_somewhere;
    
    bool sav_in_tokz_esc = in_tokz_esc;
    in_tokz_esc = INVERSE(sav_in_tokz_esc);

    traced_name_error( stat_name);

    found_somewhere = word_exists( stat_name, &where_pt1, &where_pt2);
    if ( found_somewhere )
    {
	tokenization_error ( TKERROR, "The word %s is %s recognized "
	    "in tokenizer-escape mode.\n",
		 stat_name, sav_in_tokz_esc ? "not" :  "only" );
    } else {
	not_in_dict( stat_name);
    }

    if ( INVERSE(exists_in_ancestor( stat_name)) )
    {
        if ( found_somewhere && sav_in_tokz_esc )
	{
	    tokenization_error(INFO,
		"%s is defined %s%s", stat_name, where_pt1, where_pt2 );
	    show_node_start();
	}
    }

    in_tokz_esc = sav_in_tokz_esc;
}


/* **************************************************************************
 *
 *      Function name:  unresolved_instance
 *      Synopsis:       Print the "unresolved instance" message
 *
 *      Inputs:
 *         Parameters:
 *             severity                    Severity of the Message
 *         Local Static Variables:
 *             instance_filename           File where "instance" invoked
 *             instance_lineno             Line number where "instance" invoked
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Printout:          Message.
 *
 *      Error Detection:
 *          Error was detected by the calling routine...
 *
 **************************************************************************** */

static void unresolved_instance( int severity)
{
    tokenization_error( severity, "Unresolved \"INSTANCE\"" );
    just_where_started( instance_filename, instance_lineno );
}


/* **************************************************************************
 *
 *      Function name:  modified_by_instance
 *      Synopsis:       Print the "[not] modified by instance" message
 *
 *      Inputs:
 *         Parameters:
 *             definer                     Internal token for the defining-word
 *             was_modded                  FALSE if "not modified..."
 *         Local Static Variables:
 *             instance_filename           File where "instance" invoked
 *             instance_lineno             Line number where "instance" invoked
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Printout:          WARNING message.
 *
 *      Error Detection:
 *          Error was detected by the calling routine...
 *
 **************************************************************************** */

static void modified_by_instance( fwtoken definer, bool was_modded)
{
    char *was_not = was_modded ? "was" : "not" ;
    char *defn_type_name;

    /*  No need to check the return value  */
    definer_name(definer, &defn_type_name);

    tokenization_error ( WARNING,
	"%s definition %s modified by \"INSTANCE\"",
	    defn_type_name, was_not );
    just_where_started( instance_filename, instance_lineno );
 }

/* **************************************************************************
 *
 *      Function name:  validate_instance
 *      Synopsis:       If "instance" is in effect, check whether it is
 *                          appropriate to the defining-word being called.
 *
 *      Inputs:
 *         Parameters:
 *             definer                   Internal token for the defining-word
 *         Local Static Variables:
 *             is_instance               TRUE if "instance" is in effect.
 *             instance_definer_gap      TRUE if invalid definer(s) invoked
 *                                           since "instance" went into effect.
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Local Static Variables:
 *             is_instance               Reset to FALSE if definer was valid.
 *             instance_definer_gap      TRUE if definer was not valid;
 *                                           FALSE if definer was valid.
 *
 *      Error Detection:
 *          If "instance" is in effect, the only defining-words that are
 *              valid are:  value  variable  defer  or  buffer:  Attempts
 *              to use any other defining-word will be reported with a
 *              WARNING, but "instance" will remain in effect.
 *          If an invalid defining-word was invoked since "instance" went
 *              into effect, then, when it is finally applied to a valid
 *              definer, issue a WARNING.
 *
 *      Process Explanation:
 *          Implicit in the Standard is the notion that, once INSTANCE has
 *              been executed, it remains in effect until a valid defining-
 *              word is encountered.  We will do the same.
 *
 **************************************************************************** */

static void validate_instance(fwtoken definer)
{
    if ( is_instance )
    {
	bool is_error = TRUE ;

	switch ( definer)
	{
	    case VALUE:
	    case VARIABLE:
	    case DEFER:
	    case BUFFER:
		is_error = FALSE;
	    /*  No default needed, likewise, no breaks;      */
	    /*  but some compilers get upset without 'em...  */
	    default:
		break;
	}

	if( is_error )
	{
	    modified_by_instance(definer, FALSE );
	    instance_definer_gap = TRUE;
	}else{
	    if ( instance_definer_gap )
	    {
		modified_by_instance(definer, TRUE );
	    }
	    is_instance = FALSE;
	    instance_definer_gap = FALSE;
	}
    }
}
    

/* **************************************************************************
 *
 *      Function name:  create_word
 *      Synopsis:       
 *
 *      Inputs:
 *         Parameters:
 *             definer             Internal token for the defining-word
 *         Global Variables:
 *             control_stack_depth Number of "Control Stack" entries in effect
 *             nextfcode           FCode-number to be assigned to the new name
 *             statbuf             Symbol last read from the input stream
 *             pc                  Input-source Scanning pointer
 *             hdr_flag            State of headered-ness for name-creation
 *             force_tokens_case   If TRUE, force token-names' case in FCode
 *             force_lower_case_tokens
 *                                 If  force_tokens_case  is TRUE, this
 *                                     determines which case to force
 *             iname               Input-source file name; for error-reporting
 *             lineno              Input-source Line number; also for err-rep't
 *
 *      Outputs:
 *         Returned Value:         TRUE if successful
 *         Global Variables:  
 *             nextfcode           Incremented  (by bump_fcode() )
 *             statbuf             Advanced to next symbol; must be re-read
 *             pc                  Advanced, then restored to previous value
 *             define_token        Normally TRUE.  Made FALSE if the definition
 *                                     occurs inside a control-structure, (which
 *                                     is an Error); we allow the definition to
 *                                     proceed (so as to avoid "cascade" errors
 *                                     and catch other errors normally) but we
 *                                     suppress adding its token to the vocab,
 *                                     "hiding" it and "revealing" it (because
 *                                     there's nothing to hide).
 *             NOTE:  Make this a Global.  Use it in the routines it controls...
 *         Memory Allocated
 *             Copy of the name being defined, by support routine.
 *             Copy of input-source file name, for error-reporting
 *         When Freed?
 *             Copy of name being defined is freed when Current Device Vocab
 *                 is "finished", or at end of tokenization.
 *             Copy of input-source file name is freed at end of this routine.
 *
 *      Error Detection:
 *          ERROR if already inside a colon-definition.  Discontinue
 *              processing and return FALSE.
 *          ERROR if inside a control-structure.  Continue processing,
 *              though, to catch other errors, and even return TRUE;
 *              except:  leave the new token undefined. 
 *          Warning on duplicate name (subject to command-line control)
 *          Message if name is excessively long; Warning if headerless.
 *          FATAL if the value of  nextfcode  is larger than the legal
 *              maximum for an FCode, (0x0fff).
 *
 *      Revision History:
 *      Updated Thu, 24 Mar 2005 by David L. Paktor
 *          Optional warning when name about to be created is a
 *              duplicate of an existing name.
 *      Updated Wed, 30 Mar 2005 by David L. Paktor
 *          Warning when name length exceeds ANSI-specified max (31 chars).
 *      Updated Tue, 05 Apr 2005 by David L. Paktor
 *          Add "definer" parameter and call to  add_definer() .  Part
 *              of the mechanism to forbid attempts to use the  TO 
 *              directive to change values of CONSTANTs in particular
 *              and of inappropriate targets in general.
 *      Updated Fri, 06 May 2005 by David L. Paktor
 *          Error-detection of   DO ...  LOOP  and  BEGIN ...  imbalance
 *          Error-detection of  nextfcode  exceeding legal maximum (0x0fff).
 *      Updated Wed, 20 Jul 2005 by David L. Paktor
 *          Put Duplicate-Name-Test under command-line control...
 *      Updated Wed, 24 Aug 2005 by David L. Paktor
 *          Error-detection via  clear_control_structs()  routine.
 *      Updated Tue, 10 Jan 2006 by David L. Paktor
 *          Convert to  tic_hdr_t  type vocabulary.
 *      Updated Thu, 20 Apr 2006 by David L. Paktor
 *          Allow creation of new definition within body of a flow-control
 *              structure.  (Remove error-detection via  clear_control_structs)
 *      Updated Tue, 13 Jun 2006 by David L. Paktor
 *          Move detection of out-of-bounds  nextfcode  to  assigning_fcode()
 *              routine, which also detects Overlapping Ranges error.
 *      Updated Thu, 27 Jun 2006 by David L. Paktor
 *          Report Error for attempt to create def'n inside control structure.
 *
 *      Extraneous Remarks:
 *          We must not set  incolon  to TRUE (if we are creating a colon
 *              definition) until *AFTER* this routine has been called, due
 *              to the initial error-checking.  If we need to detect whether
 *              we are creating a colon definition, we can do so by testing
 *              whether the parameter, DEFINER, equals COLON .
 *
 **************************************************************************** */

static bool create_word(fwtoken definer)
{
    signed long wlen;
    bool retval = FALSE;
    char *defn_type_name;

    /*  If already inside a colon, ERROR and discontinue processing    */
    /*  If an alias to a definer is used, show the name of the alias  */
    if ( test_in_colon(statbuf, FALSE, TKERROR, NULL) ) 
    {
	char defn_type_buffr[32] = "";
	unsigned int old_lineno = lineno;    /*  For error message  */

	define_token = TRUE;

	{   /*  Set up definition-type text for error-message */

	    /*  No need to check the return value  */
	    definer_name(definer, &defn_type_name);

	    strcat( defn_type_buffr, defn_type_name);
	    strcat( defn_type_buffr, " definition");
	}
	/*  If in a control-structure, ERROR but continue processing  */
	if ( control_stack_depth != 0 )
	{
	    announce_control_structs( TKERROR, defn_type_buffr, 0);
	    /*  Leave the new token undefined.  */
	    define_token = FALSE;
	}

	/*  Get the name of the new token  */
	wlen = get_word();

#ifdef DEBUG_SCANNER
	printf("%s:%d: debug: defined new word %s, fcode no 0x%x\n",
			iname, lineno, name, nextfcode);
#endif
	if ( wlen <= 0 )
	{
	    warn_unterm( TKERROR, defn_type_buffr, old_lineno);
	}else{
	    bool emit_token_name = TRUE;

	    /*  Other Error or Warnings as applicable  */
	    validate_instance( definer);

	    /*  Bump FCode; error-check as applicable  */
	    assigning_fcode();

	    /*  Define the new token, unless disallowed  */
	    add_to_current( statbuf, nextfcode, definer);

	    check_name_length( wlen);

	    /*  Emit appropriate FCodes:  Type of def'n,   */
	    switch ( hdr_flag )
	    {
		case FLAG_HEADERS:
		    emit_token("named-token");
		    break;

		case FLAG_EXTERNAL:
		    emit_token("external-token");
		    break;

		default:  /*   FLAG_HEADERLESS   */
		    emit_token("new-token");
		    emit_token_name = FALSE;
	    }

	    /*  Emit name of token, if applicable  */
	    if ( emit_token_name )
	    {
		if ( force_tokens_case )
		{
		    if ( force_lower_case_tokens )
		    {
			strlwr( statbuf);
		    }else{
			strupr( statbuf);
		    }
		}
		emit_string((u8 *)statbuf, wlen);	
	    }

	    /*  Emit the new token's FCode   */
	    emit_fcode(nextfcode);

	    /*  Prepare FCode Assignment Counter for next definition   */
	    bump_fcode();

	    /*  Declare victory   */
	    retval = TRUE;
	}
    }
    return( retval);
}


/* **************************************************************************
 *
 *      Function name:  cannot_apply
 *      Synopsis:       Print error message of the form:
 *                     "Cannot apply <func> to <targ>, which is a <def'n>"
 *
 *      Inputs:
 *         Parameters:
 *             func_nam                    The name of the function
 *             targ_nam                    The name of the target
 *             defr                        The numeric-code of the definer-type
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Printout:
 *             The error message is the entire printout of this routine
 *
 *      Error Detection:
 *          Error was detected by calling routine
 *
 *      Process Explanation:
 *          The calling routine already looked up the definer for its
 *              own purposes, so we don't need to do that again here.
 *
 *      Still to be done:
 *          If the definer-name is not found, we might still look up
 *              the target name in the various vocabularies and use
 *              a phrase for those.  E.g., if it is a valid token,
 *              we could say it's defined as a "primitive".  (I'm
 *              not sure what we'd say about an FWord...)
 *
 **************************************************************************** */

static void cannot_apply( char *func_nam, char *targ_nam, fwtoken defr)
{
    char *defr_name = "" ;
    char *defr_phrase = wh_defined ;

    if ( ! definer_name(defr, &defr_name) )
    {
	defr_phrase = "";
    }

    tokenization_error ( TKERROR , 
	"Cannot apply  %s  to  %s %s%s.\n",
	     func_nam, targ_nam, defr_phrase, defr_name );

}


/* **************************************************************************
 *
 *      Function name:  lookup_with_definer
 *      Synopsis:       Return pointer to data-structure of named word,
 *                      if it's valid in Current context, and supply its
 *                      definer.  If it's not valid in Current context,
 *                      see if it might be a Local, and supply that definer.
 *
 *      Inputs:
 *         Parameters:
 *             stat_name                  Name to look up
 *             *definr                    Pointer to place to put the definer.
 *
 *      Outputs:
 *         Returned Value:                Pointer to data-structure, or
 *                                            NULL if not in Current context.
 *         Supplied Pointers:
 *             *definr                    Definer; possibly LOCAL_VAL
 *
 *      Process Explanation:
 *          If the name is not found in the Current context, and does not
 *              exist as a Local, *definr will remain unchanged.
 *
 *      Extraneous Remarks:
 *          This is an odd duck^H^H^H^H^H^H^H^H^H^H^H^H a highly-specialized 
 *              routine created to meet some corner-case needs engendered by
 *              the conversion to tic_hdr_t vocabularies all around, combined
 *              with an obsessive urge to preserve a high level of detail in
 *              our error-messages.
 *
 **************************************************************************** */

static tic_hdr_t *lookup_with_definer( char *stat_name, fwtoken *definr )
{
    tic_hdr_t *retval = lookup_current( stat_name);
    if ( retval != NULL )
    {
         *definr = retval->fword_defr;
    }else{
        if ( exists_as_local( stat_name) ) *definr = LOCAL_VAL;
    }
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  validate_to_target
 *      Synopsis:       Print a message if the intended target
 *                          of the  TO  directive is not valid
 *      
 *      Inputs:
 *         Parameters:                NONE
 *         Global Variables:
 *             statbuf             Next symbol to be read from the input stream
 *             pc                  Input-source Scanning pointer
 *
 *      Outputs:
 *         Returned Value:         TRUE = Allow  b(to)  token to be output.
 *         Global Variables:
 *             statbuf             Advanced to next symbol; must be re-read
 *             pc                  Advanced, then restored to previous value
 *
 *      Error Detection:
 *          If next symbol is not a valid target of  TO , issue ERROR    
 *              message.  Restored  pc  will cause the next symbol to
 *              be processed by ordinary means.
 *          Allow  b(to)  token to be output in selected cases.  Even if
 *              user has set the "Ignore Errors" flag, certain targets are
 *              still too risky to be allowed to follow a  b(to)  token;
 *              if "Ignore Errors" is not set, output won't get created
 *              anyway.
 *          Issue ERROR in the extremely unlikely case that "to" is the
 *              last word in the Source.
 *
 *      Process Explanation:
 *          Valid targets for a TO directive are words defined by:
 *              DEFER, VALUE and arguably VARIABLE.  We will also allow
 *              CONSTANT, but will still issue an Error message.
 *          After the check, restore  pc ; this was only a look-ahead.
 *              Also restore  lineno  and  abs_token_no 
 *
 *      Extraneous Remarks:
 *          Main part of the mechanism to detect attempts to use the  TO 
 *              directive to change the values of CONSTANTs in particular
 *              and of inappropriate targets in general.
 *
 **************************************************************************** */

static bool validate_to_target( void )
{
    signed long wlen;
    tic_hdr_t *test_entry;
    u8 *saved_pc = pc;
    char *cmd_cpy = strupr( strdup( statbuf));    /*  For error message  */
    unsigned int saved_lineno = lineno;
    unsigned int saved_abs_token_no = abs_token_no;
    fwtoken defr = UNSPECIFIED ;
    bool targ_err = TRUE ;
    bool retval = FALSE ;

    wlen = get_word();
    if ( wlen <= 0 )
    {
	warn_unterm( TKERROR, cmd_cpy, saved_lineno);
    }else{

	test_entry = lookup_with_definer( statbuf, &defr);
	if ( test_entry != NULL )
	{
	    switch (defr)
	    {
		case VARIABLE:
		    tokenization_error( WARNING,
			"Applying %s to a VARIABLE (%s) is "
			"not recommended; use  !  instead.\n",
			cmd_cpy, statbuf);
		case DEFER:
		case VALUE:
		    targ_err = FALSE ;
		case CONST:
		    retval = TRUE ;
		/*  No default needed, likewise, no breaks;      */
		/*  but some compilers get upset without 'em...  */
		default:
		    break;
	    }
	}

	if ( targ_err )
	{
	    cannot_apply(cmd_cpy, strupr(statbuf), defr );
	}

	pc = saved_pc;
	lineno = saved_lineno;
	abs_token_no = saved_abs_token_no;
    }
    free( cmd_cpy);
    return( retval);
}


/* **************************************************************************
 *
 *      Function name:  you_are_here
 *      Synopsis:       Display a generic Advisory of the Source command
 *                          or directive encountered and being processed
 *
 *      Inputs:
 *         Parameters:                NONE
 *         Global Variables:
 *             statbuf                The command being processed 
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Printout:
 *             Advisory message
 *
 **************************************************************************** */

static void you_are_here( void)
{
    tokenization_error( INFO,
	"%s encountered; processing...\n",
	    strupr(statbuf) );
}


/* **************************************************************************
 *
 *      Function name:  fcode_starter
 *      Synopsis:       Respond to one of the "FCode Starter" words
 *      
 *      Inputs:
 *         Parameters:
 *             token_name         The FCode-token for this "Starter" word
 *             spread             The separation between tokens.
 *             is_offs16          Whether we are using a 16-bit number
 *                                    for branch- (and suchlike) -offsets,
 *                                    or the older-style 8-bit offset numbers.
 *         Global Variables:
 *            iname               Input-File name, used to set ifile_name 
 *                                    field of  current_device_node
 *            lineno              Current Input line number, used to set
 *                                    line_no field of  current_device_node
 *         Local Static Variables:
 *            fcode_started       If this is TRUE, we have an Error.
 *            first_fc_starter    Control calling  reset_fcode_ranges() ;
 *                                    only on the first fcode_starter of
 *                                    a tokenization.
 *
 *      Outputs:
 *         Returned Value:        NONE
 *         Global Variables:
 *            offs16              Global "16-bit-offsets" flag
 *            current_device_node   The ifile_name and line_no fields will be
 *                                    loaded with the current input file name
 *                                    and line number.  This node will be the
 *                                    top-level device-node.
 *            FCode Ranges will be reset the first time per tokenization
 *                that this routine is entered.
 *            A new FCode Range will be started every time after that.
 *         Local Static Variables:
 *            fcode_started       Set to TRUE.  We invoke the starter only
 *                                    once per image-block.
 *            first_fc_starter    Reset to FALSE if not already
 *         Memory Allocated
 *             Duplicate of Input-File name
 *         When Freed?
 *             In  fcode_ender()
 *
 *      Error Detection:
 *          Spread of other than 1 -- Warning message.
 *          "FCode Starter" previously encountered -- Warning and ignore.
 *
 *      Question under consideration:
 *          Do we want directives -- such as definitions of constants --
 *              supplied before the "FCode Starter", to be considered as
 *              taking place in "Tokenizer Escape" mode?  That would mean
 *              the "Starter" functions must be recognized in "Tokenizer
 *              Escape" mode.  Many ramifications to be thought through...
 *          I think I'm coming down strongly on the side of "No".  The user
 *              who wants to do that can very well invoke "Tokenizer Escape"
 *              mode explicitly.
 *
 **************************************************************************** */

static void fcode_starter( const char *token_name, int spread, bool is_offs16)
{
    you_are_here();
    if ( spread != 1 )
    {
        tokenization_error( WARNING, "spread of %d not supported.\n", spread);
    }
    if ( fcode_started )
    {
        tokenization_error( WARNING,
	    "Only one \"FCode Starter\" permitted per tokenization.  "
		"Ignoring...\n");
    } else {

	emit_fcodehdr(token_name);
	offs16 = is_offs16;
	fcode_started = TRUE;

	current_device_node->ifile_name = strdup(iname);
	current_device_node->line_no = lineno;

	if ( first_fc_starter )
	{
	    reset_fcode_ranges();
	    first_fc_starter = FALSE;
	}else{
	    set_next_fcode( nextfcode);
	}
    }
}

/* **************************************************************************
 *
 *      Function name:  fcode_end_err_check
 *      Synopsis:       Do error-checking at end of tokenization,
 *                          whether due to FCODE-END or end-of-file,
 *                          and reset the indicators we check.
 *
 *      Inputs:
 *         Parameters:                    NONE
 *         Global Variables:
 *             Data-Stack depth     Is anything left on the stack?
 *
 *      Outputs:
 *         Returned Value:                NONE
 *         Global Variables:
 *             Data-Stack           Reset to empty
 *
 *      Error Detection:
 *          Unresolved control structures detected by clear_control_structs()
 *          If anything is left on the stack, it indicates some incomplete
 *              condition; we will treat it as a Warning.
 *
 **************************************************************************** */

static void fcode_end_err_check( void)
{
    bool stack_imbal = BOOLVAL( stackdepth() != 0 );

	if ( stack_imbal )
	{
	    tokenization_error( WARNING,
		"Stack imbalance before end of tokenization.\n");
	}
    clear_stack();
    clear_control_structs("End of tokenization");
}

/* **************************************************************************
 *
 *      Function name:  fcode_ender
 *      Synopsis:       Respond to one of the "FCode Ender" words:
 *                          The FCode-token for "End0" or "End1"
 *                              has already been written to the
 *                              FCode Output buffer.
 *                          Finish the FCode header:  fill in its
 *                              checksum and length.
 *                          Reset the token names defined in "normal" mode
 *                          (Does not reset the FCode-token number)
 *
 *      Associated FORTH words:                 END0, END1
 *      Associated Tokenizer directive:         FCODE-END
 *
 *      Inputs:
 *         Parameters:            NONE
 *         Global Variables:
 *             incolon            If TRUE, a colon def'n has not been completed
 *             last_colon_filename         For error message.
 *             last_colon_lineno           For error message.
 *             scope_is_global             For error detection
 *             is_instance                 For error detection
 *
 *      Outputs:
 *         Returned Value:        NONE
 *         Global Variables:
 *             haveend            Set to TRUE
 *             fcode_started      Reset to FALSE.  Be ready to start anew.
 *             FCode-defined tokens, aliases and macros -- i.e., those
 *                 *NOT* defined in tokenizer-escape mode -- are reset.
 *                 (Also, command-line-defined symbols are preserved).
 *             Vocabularies will be reset
 *             Device-node data structures will be deleted
 *             Top-level device-node ifile_name and line_no fields
 *                 will be reset.
 *         Memory Freed
 *             Duplicate of Input-File name, in top-level device-node.
 *         Printout:
 *             Advisory message giving current value of nextfcode
 *                 (the "FCode-token Assignment Counter")
 *
 *      Error Detection:
 *          ERROR if a Colon definition has not been completed.
 *          ERROR if "instance" is still in effect
 *          WARNING if Global-Scope has not been terminated; compensate.
 *
 *      Extraneous Remarks:
 *          In order to accommodate odd cases, such as multiple FCode blocks
 *          within a single PCI header, this routine does not automatically
 *          reset nextfcode  to h# 0800
 *
 **************************************************************************** */

void fcode_ender(void)
{
    if ( incolon )
    {
	char *tmp_iname = iname;
	iname = last_colon_filename;
	unterm_is_colon = TRUE;
	warn_unterm( TKERROR, "Colon Definition", last_colon_lineno);
	iname = tmp_iname;    
    }
    
    haveend = TRUE;

    if ( is_instance )
    {
	unresolved_instance( TKERROR);
    }

    if ( scope_is_global )
    {
        tokenization_error( WARNING ,
	    "No DEVICE-DEFINITIONS directive encountered before end.  "
		"Compensating...\n");
	resume_device_scope();
    }
    fcode_end_err_check();
    reset_normal_vocabs();
    finish_fcodehdr();
    fcode_started = FALSE;

    if ( current_device_node->ifile_name != default_top_dev_ifile_name )
    {
	free( current_device_node->ifile_name );
	current_device_node->ifile_name = default_top_dev_ifile_name;
	current_device_node->line_no = 0;
    }
}

/* **************************************************************************
 *
 *      Function name:  get_token
 *      Synopsis:       Read the next word in the input stream and retrieve
 *                          its FCode-token number.  If it's not a symbol to
 *                          which a single token is assigned (e.g., if it's
 *                          a macro), report an error.
 *
 *      Associated FORTH words:                   [']  '
 *      Associated Tokenizer directive:          F[']
 *
 *      Inputs:
 *         Parameters:
 *             *tok_entry             Place to put the pointer to token entry
 *         Global Variables:
 *             statbuf                The command being processed 
 *             pc                     Input stream character pointer
 *
 *      Outputs:
 *         Returned Value:            TRUE if successful (i.e., no error)
 *         Supplied Pointers:
 *             *tok_entry             The token entry, if no error.
 *                                        Unchanged if error.
 *         Global Variables:
 *             statbuf                The next word in the input stream
 *             pc                     Restored to previous value if error
 *         Other Effects:
 *             Display Invocation Message if entry found and is being Traced
 *
 *      Error Detection:
 *          The next word in the input stream is expected to be on the
 *              same line as the directive.  The  get_word_in_line()
 *              routine will check for that.
 *          If the next word in the input stream is a known symbol, but
 *              not one for which a single-token FCode number is assigned,
 *              report an ERROR and restore PC to its previous value.  The
 *              supplied pointer  tok_entry  will remain unaltered.
 *
 **************************************************************************** */

static bool get_token(tic_hdr_t **tok_entry)
{
    bool retval = FALSE;
    tic_hdr_t *found;
    u8 *save_pc;

    /*  Copy of command being processed, for error message  */
    char cmnd_cpy[FUNC_CPY_BUF_SIZE+1];
    strncpy( cmnd_cpy, statbuf, FUNC_CPY_BUF_SIZE);
    cmnd_cpy[FUNC_CPY_BUF_SIZE] = 0;   /*  Guarantee null terminator. */

    save_pc = pc;

    if ( get_word_in_line( statbuf) )
    {
	fwtoken defr = UNSPECIFIED;

	/*  We need to scan the newest definitions first; they
	 *      might supercede standard ones.  We need, though,
	 *      to bypass built-in FWords that need to trigger
	 *      some tokenizer internals before emitting their
	 *      synonymous FCode Tokens, (e.g., version1 , end0 ,
	 *      and start{0-4}); if we find one of those, we will
	 *      need to search again, specifically within the list
	 *      of FCode Tokens.
	 */
	found = lookup_with_definer( statbuf, &defr);
	if ( found != NULL )
	{
	    /*  Built-in FWords can be uniquely identified by their
	     *      definer,  BI_FWRD_DEFN .  The definer for "shared"
	     *      FWords is  COMMON_FWORD  but there are none of
	     *      those that might be synonymous with legitimate
	     *      FCode Tokens, nor are any likely ever to be...
	     */
	    if ( defr == BI_FWRD_DEFN )
	    {
	        found = lookup_token( statbuf);
		retval = BOOLVAL( found != NULL );
	    }else{
		retval = entry_is_token( found);
	    }
	}

	handle_invocation( found);

	if ( retval)
	{
	    *tok_entry = found;
	}else{
	    cannot_apply( cmnd_cpy, strupr(statbuf), defr );
	    pc = save_pc;
	}
    }

    return ( retval );
}


static void base_change ( int new_base )
{
    if ( incolon && ( INVERSE( in_tokz_esc) ) )
    {
        emit_literal(new_base );
	emit_token("base");
	emit_token("!");
    } else {
        base = new_base;
    }
}

static void base_val (int new_base)
{
    u8  *old_pc;

    char base_cmnd[FUNC_CPY_BUF_SIZE+1];
    strncpy( base_cmnd, statbuf, FUNC_CPY_BUF_SIZE);
    base_cmnd[FUNC_CPY_BUF_SIZE] = 0;  /* Guarantee NULL terminator */

    old_pc=pc;
    if ( get_word_in_line( statbuf) )
    {
	u8 basecpy=base;

	base = new_base;
	if ( ! handle_number() )
	{
	    /*  We did get a word on the line, but it's not a valid number */
	    tokenization_error( WARNING ,
		 "Applying %s to non-numeric value.  Ignoring.\n",
		      strupr(base_cmnd) );
	    pc = old_pc;
	}
	base=basecpy;
    }
}


/* **************************************************************************
 *
 *      Function name:  eval_string
 *      Synopsis:       Prepare to tokenize a string, artificially generated
 *                          by this program or created as a user-defined
 *                          Macro.   When done, resume at existing source.
 *                      Keep the file-name and line-number unchanged.
 *      
 *      Inputs:
 *         Parameters:
 *             inp_bufr          String (or buffer) to evaluate
 *
 *      Outputs:
 *         Returned Value:       NONE
 *         Global Variables, changed by call to init_inbuf():
 *             start             Points to given string
 *             pc                         ditto
 *             end               Points to end of given string
 *
 *      Revision History:
 *          Updated Thu, 23 Feb 2006 by David L. Paktor
 *              This routine no longer calls its own instance of  tokenize()
 *              It has become the gateway to the mechanism that makes a
 *                  smooth transition between the body of the Macro, User-
 *                  defined Symbol or internally-generated string and the
 *                  resumption of processing the source file. 
 *              A similar (but more complicated) transition when processing
 *                  an FLOADed file will be handled elsewhere.
 *          Updated Fri, 24 Feb 2006 by David L. Paktor
 *              In order to support Macro-recursion protection, this routine
 *                  is no longer the gateway for Macros; they will have to
 *                  call push_source() directly.
 *
 **************************************************************************** */

void eval_string( char *inp_bufr)
{
    push_source( NULL, NULL, FALSE);
    init_inbuf( inp_bufr, strlen(inp_bufr));
}


/* **************************************************************************
 *
 *      Function name:  finish_or_new_device
 *      Synopsis:       Handle the shared logic for the NEW-DEVICE and
 *                          FINISH-DEVICE commands.
 *
 *      Inputs:
 *         Parameters:
 *             finishing_device		   TRUE for FINISH-DEVICE,
 *					       FALSE for NEW-DEVICE
 *         Global Variables:
 *             incolon			     TRUE if inside a colon definition
 *             noerrors 		     TRUE if ignoring errors
 *             scope_is_global		     TRUE if "global scope" in effect
 *         Local Static Variables:
 *             is_instance		     TRUE if "instance" is in effect
 *             dev_change_instance_warning   TRUE if warning hasn't been issued
 *
 *      Outputs:
 *         Returned Value: 		     NONE
 *         Local Static Variables:
 *             dev_change_instance_warning   FALSE if warning is issued
 *             instance_definer_gap	     TRUE if "instance" is in effect
 *
 *      Error Detection:
 *          NEW-DEVICE and FINISH-DEVICE should not be used outside of
 *              a colon-definition if global-scope is in effect.  Error
 *              message; no further action unless we are ignoring errors.
 *          Issue a WARNING if INSTANCE wasn't resolved before the current
 *              device-node is changed.  Try not to be too repetitive...
 *
 *      Process Explanation:
 *          The words NEW-DEVICE and FINISH-DEVICE may be incorporated into
 *              a colon-definition, whether the word is defined in global-
 *              or device- -scope.  Such an incorporation does not effect
 *              a change in the device-node vocabulary; simply emit the token.
 *          If we are in interpretation mode, though, we need to check for
 *              errors before changing the device-node vocabulary:
 *          If global-scope is in effect, we need to check whether we are
 *              ignoring errors; if so, we will compensate by switching to  
 *              device-scope.
 *          If "instance" is in effect, it's "dangling".  It will remain
 *              in effect through a device-node change, but this is very
 *              bad style and deserves a WARNING, but only one for each
 *              occurrence.  It would be unaesthetic, to say the least,
 *              to have multiple messages for the same dangling "instance"
 *              in a "finish-device   new-device" sequence.
 *           We must be careful about the order we do things, because of
 *              the messages printed as a side-effect of the node change...
 *
 *      Extraneous Remarks:
 *          I will violate strict structure here.
 *
 **************************************************************************** */

static void finish_or_new_device( bool finishing_device )
{
    if ( INVERSE( incolon ) )
    {
	if ( INVERSE( is_instance) )
	{
	    /*  Arm warning for next time:         */
	    dev_change_instance_warning = TRUE;
	}else{
	    /*  Dangling "instance"                */
	    instance_definer_gap = TRUE;
	    /*   Warn only once.                   */
	    if ( dev_change_instance_warning )
	    {
		unresolved_instance( WARNING);
		dev_change_instance_warning = FALSE;
	    }
	}

	/*  Note:  "Instance" cannot be in effect during "global" scope  */ 
	if ( scope_is_global )
	{
	    glob_not_allowed( TKERROR, noerrors );
	    if ( noerrors )
	    {
		 resume_device_scope();
	    }else{
		 return;
	    }
	}

	if ( finishing_device )
	{
	     finish_device_vocab();
	}else{
	     new_device_vocab();
	}
    }
    emit_token( finishing_device ? "finish-device" : "new-device" );
	}
	
	
/* **************************************************************************
 *
 *      Function name:  abort_quote
 *      Synopsis:       Optionally implement the   ABORT"  function as
 *                      though it were a macro.  Control whether to allow
 *                      it, and which style to support, via switches set
 *                      on the command-line at run-time.
 *
 *      Inputs:
 *         Parameters:
 *             tok                       Numeric-code associated with the
 *                                           FORTH word that was just read.
 *         Global Variables:
 *             enable_abort_quote        Whether to allow ABORT"
 *             sun_style_abort_quote     SUN-style versus Apple-style
 *             abort_quote_throw         Whether to use -2 THROW vs ABORT
 *
 *      Outputs:
 *         Returned Value:     TRUE if it was handled
 *         Global Variables:
 *             report_multiline              Reset to FALSE.
 *         Printout:
 *             ADVISORY:   ABORT" in fcode is not defined by IEEE 1275-1994
 *
 *      Error Detection:
 *          Performed by other routines.  If user selected not to
 *              allow  ABORT" , it will simply be treated as an
 *              unknown word.
 *          The string following it, however, will still be consumed.
 *
 *      Process Explanation:
 *          If the supplied  tok  was not  ABORTTXT , then return FALSE.
 *          If the  enable_abort_quote  flag is FALSE, consume the
 *              string following the Abort" token, but be careful to
 *              leave the  Abort" token in statbuf, as it will be used
 *              for the error message.
 *          Otherwise, create and prepare for processing the appropriate Macro:
 *              For Apple Style, we push the specified string onto the stack
 *                  and do -2 THROW (and hope the stack unwinds correctly).
 *              For Sun Style, we test the condition on top of the stack,
 *                  and if it's true, print the specified string before we
 *                  do the -2 THROW.
 *          We perform the underlying operations directly:  placing an "IF"
 *              (if Sun Style), then placing the string.  This bypasses
 *              any issues of double-parsing, as well as of doubly checking
 *              for a multi-line string.
 *          Finally, we perform the operational equivalents of the remainder
 *              of the command sequence.
 *
 *      Extraneous Remarks:
 *          I would have preferred not to have to directly perform the under-
 *              lying operations, and instead simply prepare the entire command
 *              sequence in a buffer, but I needed to handle the case where
 *              quote-escaped quotes are included in the string:  If the string
 *              were simply to be reproduced into the buffer, the quote-escaped
 *              quotes would appear as plain quote-marks and terminate the
 *              string parsing prematurely, leaving the rest of the string
 *              to be treated as code instead of text...
 *          Also, the introduction of the variability of whether to do the
 *               -2 THROW  or to compile-in the token for  ABORT  makes the
 *              buffer-interpretation scheme somewhat too messy for my tastes.
 *
 **************************************************************************** */
	
static bool abort_quote( fwtoken tok)
{
    bool retval = FALSE;
    if ( tok == ABORTTXT )
    {
	if ( ! enable_abort_quote )
	{
	    /* ABORT" is not enabled; we'd better consume the string  */
	    char *save_statbuf;
	    signed long wlen;
	    save_statbuf = strdup( (char *)statbuf);
	    wlen = get_string( FALSE);
	    strcpy( statbuf, save_statbuf);
	    free( save_statbuf);
	}else{
	    /* ABORT" is not to be used in FCODE drivers
	     * but Apple drivers do use it. Therefore we
	     * allow it. We push the specified string to
	     * the stack, do -2 THROW and hope that THROW
	     * will correctly unwind the stack.
	     * Presumably, Apple Source supplies its own
	     *  IF ... THEN
	     */
	    char *abort_string;
	    signed long wlen;

	    retval = TRUE;
	    tokenization_error (INFO, "ABORT\" in fcode not "
			    "defined by IEEE 1275-1994\n");
	    test_in_colon("ABORT\"", TRUE, TKERROR, NULL);
	    wlen=get_string( TRUE);

	    if ( sun_style_abort_quote )  emit_if();

	    emit_token("b(\")");
	    emit_string(statbuf, wlen);
	
	    if ( sun_style_abort_quote )  emit_token("type");

	    if ( abort_quote_throw )
	    {
		emit_literal( -2);
		emit_token("throw");
	    }else{
		emit_token("abort");
	}
		
	    if ( sun_style_abort_quote )  emit_then();
	        /*  Sun Style  */
		abort_string = " type -2 THROW THEN:" ;
}
	}
    return( retval );
}


/* **************************************************************************
 *
 *      Function name:  create_alias
 *      Synopsis:       Create an alias, as specified by the user
 *
 *      Associated FORTH word:                 ALIAS
 *
 *      Inputs:
 *         Parameters:                NONE
 *         Global Variables:
 *             incolon                Colon-def'n-in-progress indicator
 *             in_tokz_esc            "Tokenizer Escape" mode indicator
 *         Input Stream
 *             Two words will be read.
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Memory Allocated
 *             The two words will be copied into freshly-allocated memory 
 *                 that will be passed to the create_..._alias()  routine.
 *         When Freed?
 *             When Current Device Vocabulary is "finished", or at end
 *                 of tokenization, or upon termination of program.
 *             If not able to create alias, the copies will be freed here.
 *
 *      Error Detection:
 *          If the ALIAS command was given during colon-definition, that
 *              can be handled by this tokenizer, but it is not supported
 *              by IEEE 1275-1994.  Issue a WARNING.
 *          If the word to which an alias is to be created does not exist
 *              in the appropriate mode -- relative to "Tokenizer-Escape" --
 *              that is an ERROR.
 *          If "instance" is in effect, the ALIAS command is an ERROR.
 *          Duplicate-name Warning is handled by support-routine.
 *
 *      Process Explanation:
 *          Get two words -- the new name and the "old" word -- from the
 *              same line of input as the ALIAS command.
 *          Determine whether or not we are in "Tokenizer-Escape" mode.
 *              Subsequent searches will take place in that same mode.
 *          If the "new" name already exists, issue a warning.
 *          In each vocabulary applicable to the current mode -- i.e., 
 *                  "Tokenizer-Escape" or "Normal" -- (except:  cannot
 *                  make aliases to "Locals"):
 *              Try using the  create_..._alias()  routine.
 *              If it succeeds, we are done.
 *          IMPORTANT:  The order in which we try the vocabularies MUST
 *              match the order in which  tokenize_one_word()  searches them. 
 *          If all the attempts failed, the "old" word does not exist;
 *              declare an ERROR and free up the memory that was allocated.
 *
 *      Extraneous Remarks:
 *          With the separation of the  tokenizer[  state, this
 *              function has become too complicated to keep as a
 *              simple  CASE  in the big  SWITCH  statement anymore...
 *
 *          I had earlier thought that it was sufficient to create a
 *              macro linking the "new" name to the "old" word.  There
 *              were too many cases, though, where that didn't work.
 *              This is cleaner.
 *
 *          I will not be adhering to the strict rules of structure in
 *              this routine, as it would get me nested too deeply...
 *
 *      Revision History:
 *          Updated Tue, 10 Jan 2006 by David L. Paktor
 *              Convert to  tic_hdr_t  type vocabularies.
 *          Updated Fri, 22 Sep 2006 by David L. Paktor
 *              Move the  warn_if_duplicate()  call to the calling routine.
 *          Updated Wed, 11 Oct 2006 by David L. Paktor
 *              Move the Tracing and Duplicate-Warning message functions
 *                  into support routine.
 *
 **************************************************************************** */

static void create_alias( void )
{
    char *new_alias ;

    validate_instance(ALIAS);
    if ( incolon )
    {
	 tokenization_error ( WARNING,
	    "ALIAS during colon-definition "
		"is not supported by IEEE 1275-1994\n");
}
    if ( get_word_in_line( "ALIAS") )
    {

	new_alias = strdup((char *)statbuf);

	if (get_word_in_line( "ALIAS") )
{
	    char *old_name = strdup((char *)statbuf) ;

	    /*
	     *  Here is where we begin trying the  create_..._alias() 
	     *      routines for the vocabularies.
	     */

	    /*
	     *  Distinguish between "Normal" tokenization mode
	     *  and "Tokenizer Escape" mode
	     */
	    if ( in_tokz_esc )
	    {
		if ( create_tokz_esc_alias( new_alias, old_name) )
		    return;
	
		/*
		 *  Handle the classes of operatives that are common between
		 *      "Tokenizer Escape" mode and "Normal" tokenization mode.
		 *  Those classes include selected non-fcode forth constructs
		 *     and Conditional-Compilation Operators.
		 */
		{
		    tic_hdr_t *found = lookup_shared_word( old_name);
		    if ( found != NULL )
		    {
			if ( create_core_alias( new_alias, old_name) )
			    return;
		    }
	}
	    }else{
        	/*  "Normal" tokenization mode  */
	
		/*  Can create aliases for "Locals", why not?  */
		if ( create_local_alias( new_alias, old_name) )
		    return;

		/*
		 *  All other classes of operatives -- non-fcode forth
		 *      constructs, Standard and user-defined fcode
		 *      tokens, Macros, and Conditional-Compilation
		 *      Operators, -- are included in the "currently
		 *      active" vocabulary.
		 */

		if ( create_current_alias( new_alias, old_name) )
		    return;
	
	    }    /*  End of separate handling for normal-tokenization mode
        	  *      versus  "Tokenizer-Escape" mode
		  */

	    /*  It's not a word, a macro or any of that other stuff.  */
	    trace_create_failure( new_alias, old_name, 0);
	    tokenized_word_error(old_name);
	    free(old_name);
	}
	free (new_alias);
    }
}

	
/* **************************************************************************
 *
 *      Function name:  string_err_check
 *      Synopsis:       Error-check after processing or Ignoring
 *                          simple strings
 *
 *      Inputs:
 *         Parameters:
 *             is_paren           TRUE if string is Dot-Paren  .( 
 *                                    FALSE if Ess-Quote  ( s"  )
 *             sav_lineno         Saved Line Number, for Unterminated Error
 *             strt_lineno        Start Line Number, for Multiline Warning
 *         Global Variables:
 *             noerrors 	  TRUE if ignoring errors
 *         Local Static Variables:
 *             got_until_eof      TRUE if reached end of buffer before delim.
 *
 *      Outputs:
 *         Returned Value:        TRUE if did not reach end of buffer, or,
 *                                    if ignoring errors, TRUE anyway.
 *
 *      Error Detection:
 *          Multi-line warning, "Unterminated" Error messages, as apppropriate
 *
 **************************************************************************** */

static  bool string_err_check( bool is_paren,
                                  unsigned int sav_lineno,
				      unsigned int strt_lineno )
{
    bool retval = noerrors ;
    char *item_typ = is_paren ?
	"Dot-Paren" : "Ess-Quote" ;
    if ( got_until_eof )   /*  Crude retrofit... */
    {
	warn_unterm( TKERROR, item_typ, sav_lineno );
    }else{
	retval = TRUE;
	warn_if_multiline( item_typ, strt_lineno );
	}
    return( retval);
}


/* **************************************************************************
 *
 *      Function name:  handle_internal
 *      Synopsis:       Perform the functions associated with FORTH words
 *                      that do not map directly to a single token.  This
 *                      is the functions that will go into the FUNCT field
 *                      of entries in the "FWords" and "Shared Words" lists.
 *      
 *      Inputs:
 *         Parameters:
 *             pfield               Param-field of the  tic_hdr_t  -type entry
 *                                      associated with the FORTH-Word (FWord)
 *                                      just read that is being "handled".
 *         Global Variables:
 *             statbuf              The word that was just read.
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Global Variables:
 *             statbuf              More words may be read.
 *
 *      Error Detection:
 *          Too numerous to list here...
 *
 *      Process Explanation:
 *          Recast the type of the param-field of a  tic_hdr_t -type
 *              entry and rename it "tok".
 *          The "tok" will be used as the control-expression for a
 *              SWITCH statement with a large number of CASE labels.
 *              Both "FWords" and "shared_words" list entries will
 *              be processed by this routine.
 *      
 *      Revision History:
 *      Updated Wed, 20 Jul 2005 by David L. Paktor
 *          Put handling of  ABORT"  under control of a run-time
 *              command-line switch.
 *          Put decision to support IBM-style Locals under control
 *              of a run-time command-line switch.
 *      Updated Tue, 17 Jan 2006 by David L. Paktor
 *          Convert to handler for  tic_hdr_t  type vocab entries.
 *
 *      Extraneous Remarks:
 *          We would prefer to keep this function private, so we will
 *              declare its prototype here and in the one other file
 *              where we need it, namely, dictionary.c, rather than
 *              exporting it widely in a  .h  file.
 *
 **************************************************************************** */

void handle_internal( tic_param_t pfield);
void handle_internal( tic_param_t pfield)
{
	fwtoken tok = pfield.fw_token;

	signed long wlen;
	unsigned int sav_lineno = lineno;    /*  For error message  */

	bool handy_toggle = TRUE ;   /*  Various uses...   */
	bool handy_toggle_too = TRUE ;   /*  Various other uses...   */
	char *handy_string = "";
	int handy_int = 0;
	
#ifdef DEBUG_SCANNER
	printf("%s:%d: debug: tokenizing control word '%s'\n",
						iname, lineno, statbuf);
#endif
	switch (tok) {
	case BEGIN:
		emit_begin();
		break;

	case BUFFER:
		if ( create_word(tok) )
		{
		emit_token("b(buffer:)");
		}
		break;

	case CONST:
		if ( create_word(tok) )
		{
		emit_token("b(constant)");
		}
		break;

	case COLON:
		{
		    /*  Collect error- -detection or -reporting items,
		     *      but don't commit until we're sure the
		     *      creation was a success.
		     */
		    u16 maybe_last_colon_fcode = nextfcode ;
		    unsigned int maybe_last_colon_lineno = lineno;
		    unsigned int maybe_last_colon_abs_token_no = abs_token_no;
		    unsigned int maybe_last_colon_do_depth = do_loop_depth;
		    /*  last_colon_defname
		     *     has to wait until after call to  create_word()
		     */

		    if ( create_word(tok) )
		    {
			last_colon_fcode = maybe_last_colon_fcode;
			last_colon_lineno = maybe_last_colon_lineno;
			last_colon_abs_token_no = maybe_last_colon_abs_token_no;
			last_colon_do_depth = maybe_last_colon_do_depth;
			collect_input_filename( &last_colon_filename);
			/*  Now we can get  last_colon_defname  */
			if ( last_colon_defname != NULL )
			{
			    free( last_colon_defname);
			}
			last_colon_defname = strdup(statbuf);

		emit_token("b(:)");
		incolon=TRUE;
			hide_last_colon();
			lastcolon = opc;
		    }
		}
		break;
	
	case SEMICOLON:
		if ( test_in_colon("SEMICOLON", TRUE, TKERROR, NULL) )
		{
		    ret_stk_balance_rpt( "termination,", TRUE);
		    /*  Clear Control Structures just back to where
		     *      the current Colon-definition began.
		     */
		    clear_control_structs_to_limit(
			"End of colon-definition", last_colon_abs_token_no);

		    if ( ibm_locals )
		    {
			finish_locals();
			forget_locals();
		    }

		emit_token("b(;)");
		incolon=FALSE;
		    reveal_last_colon();
		}
		break;

	case CREATE:
		if ( create_word(tok) )
		{
		emit_token("b(create)");
		}
		break;

	case DEFER:
		if ( create_word(tok) )
		{
		emit_token("b(defer)");
		}
		break;

	case ALLOW_MULTI_LINE:
		report_multiline = FALSE;
		break;

	case OVERLOAD:
		if ( test_in_colon(statbuf, FALSE, WARNING, NULL) )
		{
		    do_not_overload = FALSE;
		}
		break;

	case DEFINED:
		if (get_word_in_line( statbuf) )
		{
		    eval_user_symbol(statbuf);
		}
		break;

	case CL_FLAG:
		if (get_word_in_line( statbuf) )
		{
		     set_cl_flag( statbuf, TRUE);
		}
		break;

	case SHOW_CL_FLAGS:
		show_all_cl_flag_settings( TRUE);
		break;

	case FIELD:
		if ( create_word(tok) )
		{
		emit_token("b(field)");
		}
		break;

	case VALUE:
		if ( create_word(tok) )
		{
		emit_token("b(value)");
		}
		break;
		
	case VARIABLE:
		if ( create_word(tok) )
		{
		emit_token("b(variable)");
		}
		break;

	case AGAIN:
		emit_again();
		break;

	case ALIAS:
		create_alias();
		break;

	case CONTROL:
		if ( get_word_in_line( statbuf) )
		{
		    emit_literal(statbuf[0]&0x1f);
		}
		break;

	case DO:
		emit_token("b(do)");
		mark_do();
		break;

	case CDO:
		emit_token("b(?do)");
		mark_do();
		break;

	case ELSE:
		emit_else();
		break;

	case CASE:
		emit_case();
		break;

	case ENDCASE:
		emit_endcase();
		break;

	case NEW_DEVICE:
		handy_toggle = FALSE;
	case FINISH_DEVICE:
		finish_or_new_device( handy_toggle );
		break;

	case FLITERAL:
		{
		    u32 val;
		    val = dpop();
		    emit_literal(val);
		}
		break;

	case OF:
		emit_of();
		break;

	case ENDOF:
		emit_endof();
		break;
		
	case EXTERNAL:
		set_hdr_flag( FLAG_EXTERNAL );
		break;
		
	case HEADERLESS:
		set_hdr_flag( FLAG_HEADERLESS );
		break;
	
	case HEADERS:
		set_hdr_flag( FLAG_HEADERS );
		break;

	case DECIMAL:
		/* in a definition this is expanded as macro "10 base !" */
		base_change ( 0x0a );
		break;
		
	case HEX:
		base_change ( 0x10 );
		break;

	case OCTAL:
		base_change ( 0x08 );
		break;

	case OFFSET16:
		if (!offs16)
		{
		    tokenization_error(INFO, "Switching to 16-bit offsets.\n");
		}else{
		    tokenization_error(WARNING,
			"Call of OFFSET16 is redundant.\n");
		}
		emit_token("offset16");
		offs16=TRUE;
		break;

	case IF:
		emit_if();
		break;

/* **************************************************************************
 *
 *      Still to be done:
 *          Correct analysis of Return-Stack usage within Do-Loops
 *              or before Loop Elements like I and J or UNLOOP or LEAVE.
 *
 **************************************************************************** */
	case UNLOOP:
		emit_token("unloop");
		must_be_deep_in_do(1);
		break;

	case LEAVE:
		emit_token("b(leave)");
		must_be_deep_in_do(1);
		break;

	case LOOP_I:
		emit_token("i");
		must_be_deep_in_do(1);
		break;

	case LOOP_J:
		emit_token("j");
		must_be_deep_in_do(2);
		break;
		
	case LOOP:
		emit_token("b(loop)");
		resolve_loop();
		break;
		
	case PLUS_LOOP:
		emit_token("b(+loop)");
		resolve_loop();
		break;


	case INSTANCE:
		{
		    bool set_instance_state = FALSE;
		    bool emit_instance = TRUE;
		    /*  We will treat "instance" in a colon-definition as
		     *      an error, but allow it to be emitted if we're
		     *      ignoring errors; if we're not ignoring errors,
		     *      there's no output anyway...
		     */
		    if ( test_in_colon(statbuf, FALSE, TKERROR, NULL) )
		    {   /*   We are in interpretation (not colon) state.  */
			/*  "Instance" not allowed during "global" scope  */ 
			if ( scope_is_global )
			{
			    glob_not_allowed( WARNING, FALSE );
			    emit_instance = FALSE;
			}else{
			    set_instance_state = TRUE;
			}
		    }
		    if ( emit_instance )
		    {
			if ( set_instance_state )
			{
			    /*  "Instance" isn't cumulative....  */
			    if ( is_instance )
			    {
				unresolved_instance( WARNING);
			    }
			    collect_input_filename( &instance_filename);
			    instance_lineno = lineno;
			    is_instance = TRUE;
			    dev_change_instance_warning = TRUE;
			}
			emit_token("instance");
		    }
		}
		break;
		
	case GLOB_SCOPE:
		if ( test_in_colon(statbuf, FALSE, TKERROR, NULL) )
		{
		    if ( INVERSE( is_instance) )
		    {
			enter_global_scope();
		    }else{
			tokenization_error( TKERROR,
			    "Global Scope not allowed.  "
			    "\"Instance\" is in effect; issued" );
			just_where_started( instance_filename,
					        instance_lineno );
		    }
		}
		break;

	case DEV_SCOPE:
		if ( test_in_colon(statbuf, FALSE, TKERROR, NULL) )
		{
		    resume_device_scope();
		}
		break;

	case TICK:             /*    '    */
		test_in_colon(statbuf, FALSE, WARNING, "[']");
	case BRACK_TICK:       /*   [']   */
		{
		    tic_hdr_t *token_entry;
		    if ( get_token( &token_entry) )
		    {
			emit_token("b(')");
			/* Emit the token; warning or whatever comes gratis */
			token_entry->funct( token_entry->pfield);
		    }
		}
		break;

	case F_BRACK_TICK:     /*  F['] <name>
	        		*     emits the token-number for <name>
				*  Mainly useful to compute the argument
				*     to   get-token   or  set-token
				*/
		{
		    tic_hdr_t *token_entry;
		    if ( get_token( &token_entry) )
		    {
			/*  "Obsolete" warning doesn't come gratis here...  */
			token_entry_warning( token_entry);
			/*  In Tokenizer-Escape mode, push the token  */
			if ( in_tokz_esc )
			{
			    dpush( token_entry->pfield.deflt_elem);
			}else{
			    emit_literal( token_entry->pfield.deflt_elem);
			}
		    }
		}
		break;

	case CHAR:
		handy_toggle = FALSE;
	case CCHAR:
		test_in_colon(statbuf, handy_toggle, WARNING,
		    handy_toggle ? "CHAR" : "[CHAR]" );
	case ASCII:
		if ( get_word_in_line( statbuf) )
		{
		    emit_literal(statbuf[0]);
		}
		break;
		
	case UNTIL:
		emit_until();
		break;

	case WHILE:
		emit_while();
		break;
		
	case REPEAT:
		emit_repeat();
		break;
		
	case THEN:
		emit_then();
		break;

	case IS:
		tokenization_error ( INFO,
		     "Substituting  TO  for deprecated  IS\n");
	case TO:
		if ( validate_to_target() )
		{
		emit_token("b(to)");
		}
		break;

	case FLOAD:
		if ( get_word_in_line( statbuf) )
		{
		    bool stream_ok ;
			
		    push_source( close_stream, NULL, TRUE) ;
			
		    tokenization_error( INFO, "FLOADing %s\n", statbuf );
			
		    stream_ok = init_stream( statbuf );
		    if ( INVERSE( stream_ok) )
		    {
			drop_source();
		    }
		}
		break;

	case STRING:         /*  Double-Quote ( " ) string  */
		handy_toggle = FALSE;
	case PSTRING:        /*  Dot-Quote  ( ." ) string   */
		wlen=get_string( TRUE);
		emit_token("b(\")");
		emit_string(statbuf, wlen);
		if ( handy_toggle )
		{
		    emit_token("type");
		}
		break;

	case SSTRING:        /*  Ess-Quote  ( s"  ) string  */
		handy_toggle = FALSE;
	case PBSTRING:       /*  Dot-Paren  .(   string  */
		if (*pc++=='\n') lineno++;
		{
		    unsigned int strt_lineno = lineno;
		    wlen = get_until( handy_toggle ? ')' : '"' );
		    if ( string_err_check( handy_toggle,
		             sav_lineno, strt_lineno) )
		    {
		emit_token("b(\")");
			emit_string(statbuf, wlen);
			if ( handy_toggle )
			{
		emit_token("type");
			}
		    }
		}
		break;

	case FUNC_NAME:
		    if ( in_tokz_esc )
		    {
		    if ( incolon )
		    {
			tokenization_error( P_MESSAGE, "Currently" );
		    }else{
			tokenization_error( P_MESSAGE, "After" );
		    }
		    in_last_colon( incolon);
		    }else{
		emit_token("b(\")");
			emit_string( last_colon_defname,
		            strlen( last_colon_defname) );
			/*  if ( hdr_flag == FLAG_HEADERLESS ) { WARNING } */
		    }
		break;

	case IFILE_NAME:
		emit_token("b(\")");
		emit_string( iname, strlen( iname) );
		break;

	case ILINE_NUM:
		emit_literal( lineno);
		break;
			
	case HEXVAL:
		base_val (0x10);
		break;
		
	case DECVAL:
		base_val (0x0a);
		break;
		
	case OCTVAL:
		base_val (8);
		break;

	case ASC_LEFT_NUM:
		handy_toggle = FALSE;
	case ASC_NUM:
		if (get_word_in_line( statbuf) )
		{
		    if ( handy_toggle )
		    {
			ascii_right_number( statbuf);
			} else {
			ascii_left_number( statbuf);
			}
		}
		break;

	case CONDL_ENDER:   /*  Conditional directives out of context  */
	case CONDL_ELSE:
		tokenization_error ( TKERROR,
		    "No conditional preceding %s directive\n",
			strupr(statbuf) );
		break;

	case PUSH_FCODE:
		tokenization_error( INFO,
		    "FCode-token Assignment Counter of 0x%x "
		    "has been saved on stack.\n", nextfcode );
		dpush( (long)nextfcode );
		break;

	case POP_FCODE:
		pop_next_fcode();
		break;

	case RESET_FCODE:
		tokenization_error( INFO,
		    "Encountered %s.  Resetting FCode-token "
			"Assignment Counter.  ", strupr(statbuf) );
		list_fcode_ranges( FALSE);
		reset_fcode_ranges();
		break;
		
	case EXIT:
		if ( test_in_colon( statbuf, TRUE, TKERROR, NULL)
		     || noerrors )
		{
		    ret_stk_balance_rpt( NULL, FALSE);
		    if ( ibm_locals )
		    {
			finish_locals ();
		    }
		    emit_token("exit");
		}
		break;

	case ESCAPETOK:
		enter_tokz_esc();
		break;
	
	case VERSION1:
	case FCODE_V1:
		fcode_starter( "version1", 1, FALSE) ;
		tokenization_error( INFO, "Using version1 header "
		    "(8-bit offsets).\n");
		break;
	
	case START1:
	case FCODE_V2:
	case FCODE_V3: /* Full IEEE 1275 */
		fcode_starter( "start1", 1, TRUE);
		break;
		
	case START0:
		fcode_starter( "start0", 0, TRUE);
		break;
		
	case START2:
		fcode_starter( "start2", 2, TRUE);
		break;
		
	case START4:
		fcode_starter( "start4", 4, TRUE);
		break;
		
	case END1:
		tokenization_error( WARNING, 
		    "Appearance of END1 in FCode source code "
			"is not intended by IEEE 1275-1994\n");
		handy_toggle = FALSE;
	case END0:
	case FCODE_END:
		if ( handy_toggle )
		{
		    you_are_here();
		}
		emit_token( handy_toggle ? "end0" : "end1" );
		fcode_ender();
		FFLUSH_STDOUT
		break;

	case RECURSE:
		if ( test_in_colon(statbuf, TRUE, TKERROR, NULL ) )
		{
		    emit_fcode(last_colon_fcode);
		}
		break;
		

	case RECURSIVE:
		if ( test_in_colon(statbuf, TRUE, TKERROR, NULL ) )
		{
		    reveal_last_colon();
		}
		break;

	case RET_STK_FETCH:
		/*  handy_toggle controls reloading other "handy_"s
		 *  handy_toggle_too controls calling ret_stk_access_rpt()
		 *  handy_int, if non-zero, passed to bump_ret_stk_depth()
		 */
		/*  First in series doesn't need to check handy_toggle  */
		handy_string = "r@";
		    /*  Will call ret_stk_access_rpt()       */
		    /*  handy_toggle_too  is already TRUE    */
		    /*  Will not call bump_ret_stk_depth()   */
		    /*  handy_int  is already zero    */
		handy_toggle = FALSE;
	case RET_STK_FROM:
		if ( handy_toggle )
		{
		    handy_string = "r>";
		    /*  Will call ret_stk_access_rpt()  */
		    /*  handy_toggle_too  is already TRUE    */
		    /*  Will call bump_ret_stk_depth( -1)    */
		    handy_int = -1;
		    handy_toggle = FALSE;
		}
	case RET_STK_TO:
		if ( handy_toggle )
		{
		    handy_string = ">r";
		    /*  Will not call ret_stk_access_rpt()   */
		    handy_toggle_too  = FALSE;
		    /*  Will call bump_ret_stk_depth( 1)     */
		    handy_int =  1;
		    /*  Last in series doesn't need to reset handy_toggle  */
		}

		/*  handy_toggle  is now free for other use  */
		handy_toggle = allow_ret_stk_interp;
		if ( ! handy_toggle )
		{
		    handy_toggle = test_in_colon(statbuf, TRUE, TKERROR, NULL );
		}
		if ( handy_toggle || noerrors )
		{
		    if ( handy_toggle_too )
		    {
			ret_stk_access_rpt();
		    }
		    if ( handy_int != 0 )
		    {
			bump_ret_stk_depth( handy_int);
		    }
		    emit_token( handy_string);
		}
		break;

	case PCIHDR:
		emit_pcihdr();
		break;
	
	case PCIEND:
		finish_pcihdr();
		reset_fcode_ranges();
		FFLUSH_STDOUT
		break;

	case PCIREV:
		pci_image_rev = dpop();
		tokenization_error( INFO,
		    "PCI header revision=0x%04x%s\n", pci_image_rev,
			big_end_pci_image_rev ?
			    ".  Will be saved in Big-Endian format."
			    : ""  );
		break;

	case NOTLAST:
		handy_toggle = FALSE;
	case ISLAST:
		dpush(handy_toggle);
	case SETLAST:
		{
		    u32 val = dpop();
		    bool new_pili = BOOLVAL( (val != 0) );
		    if ( pci_is_last_image != new_pili )
		    {
			tokenization_error( INFO,
			    new_pili ?
				"Last image for PCI header.\n" :
				"PCI header not last image.\n" );
			pci_is_last_image = new_pili;
		    }
		}
		break;
		
	case SAVEIMG:
		if (get_word_in_line( statbuf) )
		{
		    free(oname);
		    oname = strdup( statbuf );
		    tokenization_error( INFO,
			"Output is redirected to file:  %s\n", oname);
		}
		break;

	case RESETSYMBS:
		tokenization_error( INFO,
		    "Resetting symbols defined in %s mode.\n",
			in_tokz_esc ? "tokenizer-escape" : "\"normal\"");
		if ( in_tokz_esc )
		{
		    reset_tokz_esc();
		}else{
		    reset_normal_vocabs();
		}	
		break;

	case FCODE_DATE:
		handy_toggle = FALSE;
	case FCODE_TIME:
		{
			time_t tt;
		    char temp_buffr[32];
			
			tt=time(NULL);
		    if ( handy_toggle )
		    {
			strftime(temp_buffr, 32, "%T %Z", localtime(&tt));
		    }else{
			strftime(temp_buffr, 32, "%m/%d/%Y", localtime(&tt));
		    }
		    if ( in_tokz_esc )
		    {
			tokenization_error( MESSAGE, temp_buffr);
		    }else{
			emit_token("b(\")");
			emit_string((u8 *)temp_buffr, strlen(temp_buffr) );
		    }
		}
		break;

	case ENCODEFILE:
		if (get_word_in_line( statbuf) )
		{
		encode_file( (char*)statbuf );
		}
		break;

	default:
	    /*  IBM-style Locals, under control of a switch  */
	    if ( ibm_locals )
	    {
		bool found_it = TRUE;
		switch (tok) {
		    case CURLY_BRACE:
			declare_locals( FALSE);
			break;
		    case DASH_ARROW:
			assign_local();
			break;
		    default:
			found_it = FALSE;
	}
		if ( found_it ) break;
}

	    /*  Down here, we have our last chance to recognize a token.
	     *      If  abort_quote  is disallowed, we will still consume
	     *      the string.  In case the string spans more than one
	     *      line, we want to make sure the line number displayed
	     *      in the error-message is the one on which the disallowed
	     *       abort_quote  token appeared, not the one where the
	     *      string ended; therefore, we might need to be able to
	     *      "fake-out" the line number...
	     */
{
		bool fake_out_lineno = FALSE;
		unsigned int save_lineno = lineno;
		unsigned int true_lineno;
		if ( abort_quote( tok) )
		{   break;
		}else{
		    if ( tok == ABORTTXT )  fake_out_lineno = TRUE;
		}
		true_lineno = lineno;

		if ( fake_out_lineno )  lineno = save_lineno;
		tokenization_error ( TKERROR,
		    "Unimplemented control word '%s'\n", strupr(statbuf) );
		if ( fake_out_lineno )  lineno = true_lineno;
	    }
	}
}
	
/* **************************************************************************
 *
 *      Function name:  skip_string
 *      Synopsis:       When Ignoring, skip various kinds of strings.  Maps
 *                          to string-handlers in handle_internal()...
 *
 *      Associated FORTH words:                 Double-Quote ( " ) string
 *                                              Dot-Quote  ( ." ) string
 *                                              Ess-Quote  ( s"  ) string
 *                                              Dot-Paren  .(   string
 *                                              ABORT" (even if not enabled)
 *             { (Local-Values declaration) and -> (Local-Values assignment)
 *                  are also handled if  ibm_locals  is enabled.
 *
 *      Inputs:
 *         Parameters:
 *             pfield               Param-field of the entry associated with
 *                                      the FWord that is being Ignored.
 *         Global Variables:
 *             statbuf              The word that was just read.
 *             pc                   Input-stream pointer
 *             lineno               Line-number, used for errors and warnings
 *             ibm_locals           TRUE if IBM-style Locals are enabled
 *
 *      Outputs:
 *         Returned Value:          NONE
 *
 *      Error Detection:
 *          Multi-line warnings, "Unterminated" Errors
 *              handled by called routines
 *
 *      Extraneous Remarks:
 *          We would prefer to keep this function private, too, so we
 *              will declare its prototype here and in the one other
 *              file where we need it, namely, dictionary.c, rather
 *              than exporting it widely in a  .h  file.
 *
 **************************************************************************** */

void skip_string( tic_param_t pfield);
void skip_string( tic_param_t pfield)
{
    fwtoken tok = pfield.fw_token;
    unsigned int sav_lineno = lineno;
    bool handy_toggle = TRUE ;   /*  Various uses...   */
			
    switch (tok) {
    case STRING:         /*  Double-Quote ( " ) string    */
    case PSTRING:        /*  Dot-Quote  ( ." ) string     */
    case ABORTTXT:       /*  ABORT", even if not enabled  */
	get_string( FALSE);   /*  Don't truncate; ignoring anyway  */
	/*  Will handle multi-line warnings, etc.   */
				break;
			
    case SSTRING:        /*  Ess-Quote  ( s"  ) string  */
	handy_toggle = FALSE;
    case PBSTRING:       /*  Dot-Paren  .(   string  */
			if (*pc++=='\n') lineno++;
	{
	    unsigned int strt_lineno = lineno;
	    get_until( handy_toggle ? ')' : '"' );
	    string_err_check( handy_toggle, sav_lineno, strt_lineno );
	}
	break;

    default:
	/*  IBM-style Locals, under control of a switch  */
	if ( ibm_locals )
	{
	    bool found_it = TRUE;
	    switch (tok) {
		case CURLY_BRACE:
		    declare_locals( TRUE);
		    break;
		case DASH_ARROW:
		    get_word();
		    break;
		default:
		    found_it = FALSE;
	    }
	    if ( found_it ) break;
	}

	tokenization_error ( FATAL,  "Program Error.  "
	    "Unimplemented skip-string word '%s'\n", strupr(statbuf) );
    }
}

/* **************************************************************************
 *
 *      Function name:  process_remark
 *      Synopsis:       The active function for remarks (backslash-space)
 *                          and comments (enclosed within parens)
 *
 *      Associated FORTH word(s):        \   (         
 *
 *      Inputs:
 *         Parameters:
 *             TIC entry "parameter field", init'd to delimiter character.
 *
 *      Outputs:
 *         Returned Value:          NONE
 *
 *      Error Detection:
 *          Warning if end-of-file encountered before delimiter.
 *          Warning if multi-line parentheses-delimited comment.
 *
 *      Process Explanation:
 *          Skip until the delimiter.
 *          If end-of-file was encountered, issue Warning.
 *          Otherwise, and if delimiter was not new-line,
 *              check for multi-line with Warning.
 *
 **************************************************************************** */

void process_remark( tic_param_t pfield )
{
    char until_char = (char)pfield.deflt_elem ;
    unsigned int start_lineno = lineno;

#ifdef DEBUG_SCANNER

    get_until(until_char);
			printf ("%s:%d: debug: stack diagram: %s)\n",
						iname, lineno, statbuf);
#else

    if ( skip_until( until_char) )
    {
	if ( until_char == '\n' )
	{
	    /*  Don't need any saved line number here ...  */
	    tokenization_error ( WARNING,
		"Unterminated remark.\n");
	}else{
	    warn_unterm( WARNING, "comment", start_lineno);
	}
    }else{
	if ( until_char != '\n' )
	{
	    pc++;
	    warn_if_multiline( "comment", start_lineno);
	}
    }
#endif  /*  DEBUG_SCANNER  */
}
		
			
/* **************************************************************************
 *
 *      Function name:  filter_comments
 *      Synopsis:       Process remarks and comments in special conditions
 *      
 *      Inputs:
 *         Parameters:
 *             inword             Current word just parsed
 *
 *      Outputs:
 *         Returned Value:        TRUE if Current word is a Comment-starter.
 *                                    Comment will be processed
 *
 *      Process Explanation:
 *          We want to be able to recognize any alias the user may have
 *              defined to a comment-delimiter, in whatever applicable
 *              vocabulary it might be.
 *          The active-function of any such alias will, of necessity, be
 *              the  process_remark()  routine, defined just above.
 *          We will search for the TIC-entry of the given word; if we don't    
 *              find it, it's not a comment-delimiter.  If we do find it, 
 *              and it is one, we invoke its active-function and return TRUE.
 *          We also want to permit the "allow-multiline-comments" directive   
 *              to be processed in the context that calls this routine, so
 *              we will check for that condition, too.
 *
 **************************************************************************** */

bool filter_comments( u8 *inword)
{
    bool retval = FALSE;
    tic_hdr_t *found = lookup_word( inword, NULL, NULL );
			
    if ( found != NULL )
    {
	if ( found->funct == process_remark )
	{
	    found->funct( found->pfield);
	    retval = TRUE;
	}else{
	    /*  Permit the "allow-multiline-comments" directive  */
	    if ( found->funct == handle_internal )
	    {
	        if ( found->pfield.fw_token == ALLOW_MULTI_LINE )
		{
		    /*   Make sure any intended side-effects occur...  */
		    found->funct( found->pfield);
		    retval = TRUE;
		}
	    }
	}
    }
    return ( retval );
		}

		
/* **************************************************************************
 *
 *      Function name:  tokenize_one_word
 *      Synopsis:       Tokenize the currently-obtained word
 *                          along with whatever it consumes.
 *
 *      Inputs:
 *         Parameters:
 *             wlen       Length of symbol just retrieved from the input stream
 *                              This is not really used here any more; it's
 *                              left over from an earlier implementation.
 *         Global Variables:        
 *             statbuf      The symbol (word) just retrieved from input stream.
 *             in_tokz_esc  TRUE if "Tokenizer-Escape" mode is in effect; a
 *                              different set of vocabularies from "Normal"
 *                              mode will be checked (along with those that
 *                              are common to both modes).  
 *             ibm_locals   Controls whether to check for IBM-style Locals;
 *                              set by means of a command-line switch.
 *
 *      Outputs:
 *         Returned Value:      NONE
 *         Global Variables:         
 *             statbuf          May be incremented    
 *             in_tokz_esc      May be set if the word just retrieved is
 *                                  the  tokenizer[   directive. 
 *             tic_found        
 *
 *      Error Detection:
 *           If the word could neither be identified nor processed as a number,
 *               that is an ERROR; pass it to  tokenized_word_error  for a
 *               message.
 *
 *      Process Explanation:
 *          Look for the word in each of the various lists and vocabularies
 *              in which it might be found, as appropriate to the current
 *              state of activity.
 *          If found, process it accordingly.
 *          If not found, try to process it as a number.
 *          If cannot process it as a number, declare an error.
 *
 *      Revision History:
 *          Updated Tue, 10 Jan 2006 by David L. Paktor
 *              Convert to  tic_hdr_t  type vocabularies.
 *          Updated Mon, 03 Apr 2006 by David L. Paktor
 *             Replaced bulky "Normal"-vs-"Escape" block with a call
 *                 to  lookup_word .  Attend to a small but important
 *                 side-effect of the "handle_<vocab>" routines that
 *                 feeds directly into the protection against self-
 *                 -recursion in a user-defined Macro:  Set the global
 *                 variable  tic_found  to the entry, just before we
 *                 execute it, and we're good to go... 
 *
 *      Extraneous Remarks:
 *          We trade off the strict rules of structure for simplicity
 *              of coding.
 *
 **************************************************************************** */
		
void tokenize_one_word( signed long wlen )
{
		
    /*  The shared lookup routine now handles everything.   */
    tic_hdr_t *found = lookup_word( statbuf, NULL, NULL );
		
    if ( found != NULL )
    {
	tic_found = found;
	if ( found->tracing)
	{
	    invoking_traced_name( found);
	}
	found->funct( found->pfield);
	return ;
    }
		
    /*  It's not a word in any of our current contexts.
     *      Is it a number?
     */
    if ( handle_number() )
    {
	return ;
			}

    /*  Could not identify - give a shout. */
    tokenized_word_error( statbuf );
		}

/* **************************************************************************
 *
 *      Function name:  tokenize
 *      Synopsis:       Tokenize the current input stream.
 *                          May be called recursively for macros and such.
 *
 *      Revision History:
 *      Updated Thu, 24 Mar 2005 by David L. Paktor
 *          Factor-out comment-filtration; apply to  gather_locals
 *          Factor-out tokenizing a single word (for conditionals)
 *          Separate actions of "Tokenizer-Escape" mode.
 *
 **************************************************************************** */

void tokenize(void)
{
    signed long wlen = 0;
		
    while ( wlen >= 0 )
    {
	wlen = get_word();
	if ( wlen > 0 )
	{
	    tokenize_one_word( wlen );
	}
	}
}

