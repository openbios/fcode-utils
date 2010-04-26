/*
 *                     OpenBIOS - free your system!
 *                         ( FCode tokenizer )
 *
 *  macros.c - macro initialization and functions.
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

/* **************************************************************************
 *
 *      Support functions for the  MACROS  vocabulary, implemented
 *          as a TIC-Headerlist type of data structure, and linked in
 *          to the Global Vocabulary.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          init_macros             Initialize the link-pointers in the
 *                                      initial "Built-In" portion of
 *                                      the  macros  vocabulary
 *          add_user_macro          Add an entry to the  macros  vocabulary
 *          skip_user_macro         Consume a Macro definition if Ignoring
 *
 **************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#if defined(__linux__) && ! defined(__USE_BSD)
#define __USE_BSD
#endif
#include <string.h>
#include <errno.h>

#include "macros.h"
#include "errhandler.h"
#include "ticvocab.h"
#include "stream.h"
#include "scanner.h"
#include "dictionary.h"
#include "devnode.h"

/* **************************************************************************
 *
 *              Internal Static Variables
 *          macros_tbl                    Initial array of "Built-In" Macros
 *          number_of_builtin_macros      Number of "Built-In" Macro entries.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Revision History:
 *          Thu, 27 Oct 2005 by David L. Paktor
 *              Identify the macros that resolve to a single word.
 *              Remove them from here and enter them as synonymous entries
 *                  in the Tokens, Specials or Shared-words vocabularies.
 *          Wed, 30 Nov 2005 by David L. Paktor
 *              Allow user-definition of macros.
 *          Fri, 06 Jan 2006 by David L. Paktor
 *              Re-define the Macros as a TIC-Headerlist, and make them
 *                  part of the Global Vocabulary
 *
 **************************************************************************** */



/* **************************************************************************
 *
 *      Function name:  macro_recursion_error
 *      Synopsis:       Function that will go temporarily into the FUNCT
 *                      field of a Macro's TIC-entry, to protect against
 *                      recursive macro invocations.
 *
 *      Inputs:
 *         Parameters:
 *             pfield              Param field of the TIC-entry; unused.
 *         Global Variables:
 *             statbuf             The name being invoked erroneously.
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Printout:
 *             Error Message.
 *
 *      Error Detection:
 *          If this function is called, it is an ERROR
 *
 *      Extraneous Remarks:
 *          This Tokenizer does not have the early-binding characterisitics
 *              of FORTH; its Macros are strings, evaluated when invoked
 *              rather than when they are defined.  A reference to a name
 *              that matches the macro would cause recursion, possibly
 *              infinite.  We will not allow that.
 *
 **************************************************************************** */

static void macro_recursion_error( tic_param_t pfield)
{
    tokenization_error( TKERROR,
	"Recursive invocation of macro named %s\n", statbuf);
}


/* **************************************************************************
 *
 *      Function name:  eval_mac_string
 *      Synopsis:       Function that goes into FUNCT field of a TIC-entry
 *                      in the Macros list.  Protect against recursion.
 *
 *      Inputs:
 *         Parameters:
 *             pfield              Param field of the TIC-entry
 *         Global Variables:
 *             tic_found           The TIC-entry that has just been found;
 *                                     it's the entry for this Macro.
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Global Variables:
 *             report_multiline    Cleared to FALSE
 *         Global Behavior:
 *            The Macro will be evaluated as string input.
 *
 *      Error Detection:
 *          An attempt at recursion will be detected because the FUNCT field
 *              of the Macro entry will have been temporarily be replaced by
 *              macro_recursion_error()
 *
 *      Process Explanation:
 *          Save the address of the routine that is in the FUNCT field
 *               of the entry for this Macro.  (Hey!  It's this routine...)
 *          Replace the FUNCT field of the Macro entry with the Macro
 *              Recursion Error Detection routine
 *          Pass the address of the "resumption" routine and its argument
 *              (the entry for this Macro), to  push_source()
 *          Recast the type of the parameter field to a string
 *          Make it the new Input Source Buffer.
 *          Suspend multi-line warning; see comment in body of add_user_macro()
 *              The multi-line warning flag is kept by  push_source()
 *
 *      Still to be done:
 *          If an error is encountered during Macro evaluation, display
 *              supplemental information giving the name of the Macro 
 *              being run, and the file and line number in which it was
 *              defined.
 *          This will require changes to the way user Macros are added
 *              and retained, and to the way error messages are displayed.
 *
 *      Revision History:
 *          Updated Thu, 23 Feb 2006 by David L. Paktor
 *              Do not process Macros (or, for that matter, User-defined
 *                  Symbols or FLOADed files) with a routine that calls
 *                  its own instance of  tokenize(), because the Macro
 *                  (etc.) might contain a phrase (such as the start of
 *                  a conditional) that must be terminated within the
 *                  body of a file, thus causing an undeserved Error.
 *                  Instead, they need to be handled in a more sophis-
 *                  ticated way, tied in with the operation of get_word()
 *                  perhaps, that will make a smooth transition between
 *                  the body of the Macro and the resumption of processing
 *                  the source file.  The end-of-file will only be seen
 *                  at the end of an actual input file or when getting
 *                  a delimited string.
 *          Updated Fri, 24 Feb 2006 by David L. Paktor
 *              Re-integrate Recursion Error Detection with the above.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *     In order to integrate Recursion Error Detection with the smooth
 *         transition to resumption of processing the source file, we
 *         need to create a "resumption" routine that will restore the
 *         normal behavior of the macro after it's completed, by re-
 *         instating the address of the normal Macro-invocation routine;
 *         that routine, of course, is the one that passes the address
 *         of the "resumption" routine to push_source().  In order to
 *         get around this chicken-and-egg dilemma, we will create a
 *         local static variable into which the address of the normal
 *         Macro-invocation routine will be stored.  We actually only
 *         need it once, but we'd rather avoid the overhead of checking,
 *         every time, whether it has already been set; since it's always
 *         the same, there's no harm in storing it every time.
 *
 **************************************************************************** */

typedef void (*vfunct)();  /*  Pointer to function returning void  */
static vfunct sav_mac_funct ;


/* **************************************************************************
 *
 *     This "resumption" routine will be called by  pop_source()
 *     The parameter is the Macro dictionary-entry whose behavior
 *         is to be restored.
 *
 **************************************************************************** */

static void mac_string_recovery( tic_hdr_t *macro_entry)
{
    (*macro_entry).funct = sav_mac_funct;
    (*macro_entry).ign_func = sav_mac_funct;
}

/* **************************************************************************
 *
 *     The normal Macro-invocation routine, at last...
 *
 **************************************************************************** */
static void eval_mac_string( tic_param_t pfield)
{
    int mac_str_len = strlen(pfield.chr_ptr);
    /*  We can't use  (*tic_found).pfld_size  for the string length
     *      because, if this is an alias for a macro, it will be zero...
     */
    /*  We can change that by de-coupling the decision to free the
     *      param-field from whether pfld_size is non-zero (by intro-
     *      ducing yet another field into the  tic_param_t  struct),
     *      but we're not doing that today...
     */

    sav_mac_funct = *tic_found->funct;
    (*tic_found).funct = macro_recursion_error;
    (*tic_found).ign_func = macro_recursion_error;
    push_source( mac_string_recovery, tic_found, FALSE);
    report_multiline = FALSE;  /*  Must be done AFTER call to push_source()
                                *      because  report_multiline  is part of
                                *      the state that  push_source()  saves.
				*/
    init_inbuf( pfield.chr_ptr, mac_str_len);
}

/* **************************************************************************
 *
 *     Builtin Macros do not need Recursion Error Detection.
 *     Intermediate routine to convert parameter type.
 *
 **************************************************************************** */
static void eval_builtin_mac( tic_param_t pfield)
{
    eval_string( pfield.chr_ptr);
}
/* **************************************************************************
 *
 *      Make a macro, because we might eliminate this layer later on.
 *
 **************************************************************************** */
#define EVAL_MAC_FUNC  eval_mac_string
#define BUILTIN_MAC_FUNC  eval_builtin_mac

/* **************************************************************************
 *
 *      Initialization macro definition
 *
 **************************************************************************** */

#define BUILTIN_MACRO(nam, alias) BUILTIN_MAC_TIC(nam, BUILTIN_MAC_FUNC, alias )

static tic_mac_hdr_t macros_tbl[] = {
	BUILTIN_MACRO( "(.)",		"dup abs <# u#s swap sign u#>") ,


	BUILTIN_MACRO( "?",		"@ .") ,
	BUILTIN_MACRO( "1+",		"1 +") ,
	BUILTIN_MACRO( "1-",		"1 -") ,
	BUILTIN_MACRO( "2+",		"2 +") ,
	BUILTIN_MACRO( "2-",		"2 -") ,

	BUILTIN_MACRO( "accept",      "span @ -rot expect span @ swap span !") ,
	BUILTIN_MACRO( "allot", 	"0 max 0 ?do 0 c, loop") ,
	BUILTIN_MACRO( "blank", 	"bl fill") ,
	BUILTIN_MACRO( "carret",	"h# d") ,
	BUILTIN_MACRO( ".d",		"base @ swap h# a base ! . base !") ,

	/*  Note:  The Standard gives:  ">r over r@ + swap r@ - rot r>"
	 *      as its example of the macro for  decode-bytes
	 *      But here's one that does the same thing without
	 *      using return-stack operations.  And it's one step
	 *      shorter, into the bargain!
	 */
	BUILTIN_MACRO( "decode-bytes",  "tuck - -rot 2dup + swap 2swap rot") ,

	BUILTIN_MACRO( "3drop", 	"drop 2drop") ,
	BUILTIN_MACRO( "3dup",		"2 pick 2 pick 2 pick") ,
	BUILTIN_MACRO( "erase", 	"0 fill") ,
	BUILTIN_MACRO( ".h",		"base @ swap h# 10 base ! . base !") ,
	BUILTIN_MACRO( "linefeed",	"h# a") ,

	BUILTIN_MACRO( "s.",		"(.) type space") ,
	BUILTIN_MACRO( "space", 	"bl emit") ,
	BUILTIN_MACRO( "spaces",	"0 max 0 ?do space loop") ,
	BUILTIN_MACRO( "(u.)",		"<# u#s u#>") ,
	BUILTIN_MACRO( "?leave",	"if leave then"),
};

static const int number_of_builtin_macros =
	 sizeof(macros_tbl)/sizeof(tic_mac_hdr_t);

/* **************************************************************************
 *
 *      Function name:  init_macros
 *      Synopsis:       Initialize the link-pointers in the "Built-In"
 *                          portion of the  macros  vocabulary, dynamically.
 *      
 *      Inputs:
 *         Parameters:
 *             tic_vocab_ptr                 Pointer to Global Vocab Pointer
 *         Global Variables:
 *             macros_tbl                    Initial "Built-In" Macros array
 *             number_of_builtin_macros      Number of "Built-In" Macro entries
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Global Variables:    
 *             The link-fields of the initial "Built-In" Macros array entries
 *                  will be filled in.
 *         Supplied Pointers:
 *             *tic_vocab_ptr                Updated to "tail" of Macros array
 *
 **************************************************************************** */

void init_macros( tic_hdr_t **tic_vocab_ptr )
{
    init_tic_vocab( (tic_hdr_t *)macros_tbl,
        number_of_builtin_macros,
	    tic_vocab_ptr );
}


/* **************************************************************************
 *
 *      Function name:  print_if_mac_err
 *      Synopsis:       Report a user-macro definition error, if so be.
 *
 *      Inputs:
 *         Parameters:
 *             failure             TRUE if error was detected
 *             func_cpy            STRDUP() of function name, for error message
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Memory Freed
 *             Contents of func_cpy, error or not.
 *         Printout:
 *             Error message, if  failure  is TRUE.
 *
 **************************************************************************** */
static void print_if_mac_err( bool failure, char *func_cpy)
{
    if ( failure )
    {
	tokenization_error( TKERROR,
	   "%s directive expects name and definition on the same line\n",
	       strupr(func_cpy));
    }
    free( func_cpy);
}


/* **************************************************************************
 *
 *      Function name:  add_user_macro
 *      Synopsis:       Parse input and add a user-defined Macro.
 *
 *      Associated Tokenizer directive:        [MACRO]
 *
 *      Inputs:
 *         Parameters:                 NONE
 *         Global Variables:
 *             pc                      Input-source Scanning pointer
 *             statbuf                 Symbol retrieved from input stream.
 *             in_tokz_esc             TRUE if in "Tokenizer-Escape" mode
 *             current_definitions     Pointer to Current Vocabulary pointer,
 *                                         either Global or Current Device-Node
 *             tokz_esc_vocab         "Tokenizer Escape" Vocab pointer
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Global Variables:
 *             *current_definitions   { One of these will point  }
 *             tokz_esc_vocab         {     to the new entry     }
 *         Memory Allocated:
 *             Copy of directive, for error message
 *             Copy of Macro name
 *             Copy of Macro body
 *             Memory for the new entry will be allocated by support routine.
 *         When Freed?
 *             Copy of directive:  When error might be reported.
 *             Macro name, body and entry:  Upon end of tokenization, or when
 *                 RESET-SYMBOLS is issued in the same mode and Scope as when
 *                 the Macro was defined. 
 *
 *      Error Detection:
 *          At least two words in the input stream are expected to be on
 *              the same line as the directive.  The  get_word_in_line()
 *              and  get_rest_of_line()  routines will check for that;
 *              we will issue the Error Message for either condition.
 *          Check if the Macro name is a duplicate; warn_if_duplicate()
 *              routine will issue message.
 *
 *      Process Explanation:
 *          We start just after the directive has been recognized.
 *          Get one word in line -- this is the macro name
 *          Get input to end of line.  This is the "body" of the macro.
 *          Add the Macro to the Current vocab, using support routine.
 *          Set the definer field to MACRO_DEF and the Function to the
 *              same one that's used for the built-in macros.
 *          User-defined Macros may need to be processed while ignoring
 *              (because they might include conditional-operators, etc.)
 *              We will set the ign_func the same as the active function.
 *
 *      To be considered:
 *          Do we want to do further filtration? 
 *              Remove comments?
 *              Compress whitespace?
 *              Allow backslash at end of line to continue to next line?
 *
 *      Extraneous Remarks:
 *          The scope of User-Macro definitions will follow the same rules
 *              as all other definition types:  if Device-Definitions are
 *              in effect, the scope of the new Macro definition will be
 *              confined to the current Device-Node; if Global-Definitions
 *              are in effect when it is defined, its scope will be Global;
 *              if it was declared when we were in "Tokenizer Escape" mode,
 *              then its scope will be limited to "Tokenizer Escape" mode.
 *
 **************************************************************************** */

/*  This pointer is exported to this file only  */
extern tic_hdr_t *tokz_esc_vocab ;

void add_user_macro( void)
{
    char *macroname;
    char *macrobody;
    bool failure = TRUE;

    /*  Copy of function name, for error message  */
    char *func_cpy = strdup( statbuf);

    if ( get_word_in_line( NULL ) )
    {
        /*  This is the Macro name  */
	macroname = strdup( statbuf);

	if ( INVERSE(get_rest_of_line() ) )
	{
	    /*  No body on line  */
	    free( macroname);
	
	}else{
	    /*  We have valid Macro body on line  */
	    int mac_body_len = 0;

	    tic_hdr_t **target_vocab = current_definitions;
	    if ( in_tokz_esc ) target_vocab = &tokz_esc_vocab ;

	    /*  Tack on a new-line, so that a remark will appear
	     *      to be properly terminated.   This might trigger
	     *      an undeserved multi-line warning if the Macro
	     *      is an improperly terminated quote; we will work
	     *      around that problem by temporarily suspending
	     *      multi-line warnings during macro processing.
	     */
	    strcat( statbuf, "\n");
	    macrobody = strdup( statbuf);
	    mac_body_len = strlen(macrobody);

	    add_tic_entry( macroname, EVAL_MAC_FUNC,
	                       (TIC_P_DEFLT_TYPE)macrobody,
			           MACRO_DEF, mac_body_len, FALSE,
				       EVAL_MAC_FUNC, target_vocab );
	    failure = FALSE;
	}
    }

    print_if_mac_err( failure, func_cpy);
}

/* **************************************************************************
 *
 *      Function name:  skip_user_macro
 *      Synopsis:       Consume the text of a user-defined Macro from the
 *                          Input Stream, with no processing.  (Called when
 *                          a user-Macro definer occurs in a segment that
 *                          is being Ignored.)
 *
 *      Inputs:
 *         Parameters:
 *             pfield             "Parameter field" pointer, to satisfy
 *                                    the calling convention, but not used
 *         Global Variables:
 *             statbuf            Word currently being processed.
 *
 *      Outputs:
 *         Returned Value:        NONE
 *
 *      Error Detection:
 *          At least two words in the input stream are expected to be on
 *              the same line as the user-Macro definer, same as when the
 *              directives occurs in a segment that is not being Ignored.
 *              The  get_word_in_line()  and  get_rest_of_line()  routines
 *              will check for condition., we will issue the Error Message.
 *
 *      Process Explanation:
 *          We need to protect against the case of a macro-definition that
 *              invokes a directive that alters Conditional processing...
 *
 **************************************************************************** */
void skip_user_macro( tic_bool_param_t pfield )
{
    bool failure = TRUE;
    char *func_cpy = strdup( statbuf);
    if ( get_word_in_line( NULL ) )
    {
	if ( get_rest_of_line() )
	{
	    failure = FALSE;
	}
    }

    print_if_mac_err( failure, func_cpy);

}
