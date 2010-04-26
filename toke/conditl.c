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
 *      Conditional-Compilation support for Tokenizer
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          init_conditionals_vocab    Initialize the "Conditionals" Vocabulary.
 *          handle_conditional         Confirm whether a given name is a valid
 *                                         Conditional, and, if so, perform its
 *                                         function and return an indication.
 *          create_conditional_alias   Add an alias to "Conditionals" vocab
 *          reset_conditionals         Reset the "Conditionals" Vocabulary
 *                                         to its "Built-In" position.
 *
 **************************************************************************** */

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <errno.h>

#include "scanner.h"
#include "errhandler.h"
#include "ticvocab.h"
#include "conditl.h"
#include "stack.h"
#include "dictionary.h"
#include "vocabfuncts.h"
#include "usersymbols.h"
#include "stream.h"
#include "clflags.h"

/* **************************************************************************
 *
 *          Global Variables Imported
 *              statbuf               Start of input-source buffer
 *              pc                    Input-source Scanning pointer
 *              iname                 Current Input File name
 *              lineno                Current Line Number in Input File
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *              Local Static Variables
 *      already_ignoring      Location from which to pass a parameter,
 *                            called "alr_ign" to the main routine.  Each
 *                            "Conditional" will have an associated routine
 *                            that takes the pointer to this as its argument.
 *                            The pointer to this will satisfy the "param-field"
 *                            requirement of a TIC_HDR-style "Vocabulary"-list.
 *      conditionals_tbl      TIC_HDR-style "Vocabulary"-list table, initialized
 *                            as an array.
 *      conditionals          Pointer to "tail" of Conditionals Vocabulary-list
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      The lists of synonymous forms of the #ELSE and #THEN operators
 *          are incorporated into the "Shared Words" Vocabulary.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Function name:  is_a_then  /  is_an_else
 *      Synopsis:       Indicate whether the given name is one of the
 *                      [then] / [else]  synonyms
 *      
 *      Inputs:
 *         Parameters:
 *             a_word                        Word to test
 *
 *      Outputs:
 *         Returned Value:       TRUE if the given name is one of the synonyms
 *
 *      Process Explanation:
 *          The functions are twins, hence bundling them like this...
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Support function:  is_a_type
 *      Synopsis:          Indicate whether the given name is a "shared word"
 *                         whose FWord Token matches the one given
 *
 *      Inputs:
 *         Parameters:
 *             tname                        Target name to look for
 *             fw_type                      The FWord Token type to match
 *
 *      Outputs:
 *         Returned Value:                  TRUE if it matches
 *
 **************************************************************************** */


static bool is_a_type( char *tname, fwtoken fw_type)
{
    bool retval = FALSE;
    tic_fwt_hdr_t *found = (tic_fwt_hdr_t *)lookup_shared_f_exec_word( tname );
    if ( found != NULL )
    {
        if ( found->pfield.fw_token == fw_type )  retval = TRUE;
    }
    return ( retval );
}

static bool is_a_then( char *a_word)
{
    bool retval = is_a_type( a_word, CONDL_ENDER);
    return ( retval );
}

static bool is_an_else( char *a_word)
{
    bool retval = is_a_type( a_word, CONDL_ELSE);
    return ( retval );
}

/* **************************************************************************
 *
 *     This is a somewhat roundabout way of passing an "already ignoring"
 *         parameter to the various Conditional Operators.  Each operator's
 *         Parameter-Field Pointer points to this.  The calling routine
 *         must set it (or rely on the default); the routine that handles
 *         nesting of Conditionals must save and restore a local copy.
 *
 **************************************************************************** */

static bool already_ignoring = FALSE;

/* **************************************************************************
 *
 *      Further down, will define and initialize a word-list table with
 *          all the functions that implement the Conditional Operators,
 *          and we'll link it in with the "Global Vocabulary" pointer.
 *
 *      We'll call the word-list the "Conditionals Vocabulary Table",
 *          and refer to its entries as the "Conditionals Vocabulary",
 *          even though it isn't really a separate vocabulary...
 * 
 **************************************************************************** */


/* **************************************************************************
 *
 *      We also need a few common routines to pass as "Ignoring" functions
 *          for a few occasional words that take another word or two from
 *          the input stream as their arguments.  For example, if the user
 *          were to write:   alias [otherwise] [else]   and that were to
 *          occur within a segment already being ignored, we need to make
 *          sure that this doesn't get processed as an occurrence of  [else]
 *          Similarly with macro-definitions.
 *
 *      Since we are using the term "ignore a word" to mean "look it up and
 *          process it in Ignoring-state", we need a different term to name
 *          this class of routine; let's use the term "skip a word" where
 *          the "word" is strictly an input token, delimited by whitespace.
 *
 **************************************************************************** */
 
/* **************************************************************************
 *
 *      Function name:  skip_a_word
 *      Synopsis:       Consume one input-token ("word") from the
 *                          Input Stream, with no processing
 *
 *      Inputs:
 *         Parameters:
 *             pfield             "Parameter field" pointer, to satisfy
 *                                    the calling convention, but not used
 *
 *      Outputs:
 *         Returned Value:        NONE
 *
 **************************************************************************** */

void skip_a_word( tic_bool_param_t pfield )
{
    /* signed long wlen = */ get_word();
}

/* **************************************************************************
 *
 *      Function name:  skip_a_word_in_line
 *      Synopsis:       Consume one input-token ("word") on the same line
 *                          as the current line of input, from the Input
 *                          Stream, with no processing.
 *
 *      Inputs:
 *         Parameters:
 *             pfield             "Parameter field" pointer, to satisfy
 *                                    the calling convention, but not used
 *         Global Variables:
 *             statbuf            The word being processed, which expects
 *                                    another word on the same line; used
 *                                    for the Error message.
 *
 *      Outputs:
 *         Returned Value:        NONE
 *
 *      Error Detection:
 *          get_word_in_line() will check and report if no word on same line.
 *
 **************************************************************************** */
void skip_a_word_in_line( tic_bool_param_t pfield )
{
    /* bool isokay = */ get_word_in_line( statbuf);
}

/* **************************************************************************
 *
 *      Function name:  skip_two_words_in_line
 *      Synopsis:       Consume two input-tokens ("words") on the same line
 *                          as the current line of input, from the Input
 *                          Stream, with no processing.
 *
 *      Inputs:
 *         Parameters:
 *             pfield             "Parameter field" pointer, to satisfy
 *                                    the calling convention, but not used
 *         Global Variables:
 *             statbuf            The word being processed, which expects
 *                                    two words on the same line; used for
 *                                    the Error message.
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Memory Allocated
 *             Copy of  statbuf  for Error message.
 *         When Freed?
 *             End of this routine
 *
 *      Error Detection:
 *          get_word_in_line() will check and report
 *
 **************************************************************************** */

void skip_two_words_in_line( tic_bool_param_t pfield )
{
    char *func_cpy = strupr( strdup( statbuf));
    if ( get_word_in_line( func_cpy) )
    {
        /* bool isokay = */ get_word_in_line( func_cpy);
    }
    free( func_cpy);
}


/* **************************************************************************
 *
 *      Function name:  ignore_one_word
 *      Synopsis:       Handle a word that needs processing while "ignoring"
 *                          Ignore the rest.
 *
 *      Inputs:
 *         Parameters:
 *             tname                  Target name to test
 *         Local Static Variables:
 *             already_ignoring       The "Already Ignoring" flag
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Global Variables:
 *             tic_found              Set to the TIC-entry that has just been
 *                                        found, in case it's a Macro.
 *         Local Static Variables:
 *             already_ignoring       Intermediately set to TRUE, then
 *                                        returned to former state.
 *
 *      Process Explanation:
 *          When we are ignoring source input, we still need to be
 *              sensitive to the nesting of Conditional Operators, to
 *              consume comments and user -message text-bodies, and to
 *              expand Macros, among other things.
 *         Rather than create special cases here for each one, we have
 *              added an "ign_funct" pointer to those words where this
 *              is relevant, including Conditional Operators. 
 *          Save the state of  already_ignoring  and set it to TRUE
 *              Execute the "Ignoring" Function associated with the entry
 *              Restore  already_ignoring  to its previous state.
 *          This is necessary if the word is a Conditional Operator and
 *              is harmless otherwise.
 *
 **************************************************************************** */

static void ignore_one_word( char *tname)
{
    tic_bool_hdr_t *found = (tic_bool_hdr_t *)lookup_word( tname, NULL, NULL);
    if ( found != NULL )
    {
        if ( found->ign_func != NULL )
	{
	    bool save_already_ignoring = already_ignoring;
	    already_ignoring = TRUE ;
	    tic_found = (tic_hdr_t *)found;

	    found->ign_func( found->pfield);

	    already_ignoring = save_already_ignoring;
	}
    }
}

/* **************************************************************************
 *
 *      Function name:  conditionally_tokenize
 *      Synopsis:       Conduct tokenization while a Conditional-Tokenization
 *                          operator is in effect.  This is the core of the
 *                          implementation of Conditional-Tokenization.
 *      
 *      Inputs:
 *         Parameters:
 *             cond             The state of the Condition-Flag that the
 *                                  immediate Conditional Operator acquired.
 *                                  TRUE means "do not ignore".  Its sense
 *                                  is reversed when [ELSE] is encountered.
 *             alr_ign          TRUE means we are Already Ignoring source input,
 *                                  except for Conditional Operators...
 *         Global Variables:
 *             statbuf       The symbol (word) just retrieved from input stream.
 *             iname         Current Input File name (for Error Messages)
 *             lineno        Current Line Number in Input File (ditto)
 *             trace_conditionals   Whether to issue ADVISORY messages about
 *                                      the state of Conditional Tokenization.
 *
 *      Outputs:
 *         Returned Value:       NONE
 *         Global Variables:
 *             statbuf           Will be advanced to the balancing [THEN] op'r.
 *             already_ignoring  Set to TRUE if nested Conditional encountered;
 *                                  restored to previous state when done.
 *         Memory Allocated
 *             Duplicate of Input File name, for Error Messages
 *         When Freed?
 *             Just prior to exit from routine.
 *         Printout:
 *             ADVISORY messages, if "Trace-Conditionals" flag was selected.
 *             
 *         Global Behavior:
 *            Tokenization happens, or inputs are ignored, as necessary.
 *
 *      Error Detection:
 *          End-of-file encountered on reading a word    
 *              ERROR.  Conditional Operators must be balanced within a file
 *          More than one [ELSE] encountered:  ERROR if processing segment;
 *              if ignoring, WARNING.
 *
 *      Process Explanation:
 *          Read a word at a time.   Allow Macros to "pop" transparently,
 *              but not source files.
 *          If the word is a [THEN], we are done.
 *          If the word is an [ELSE], then, if we are not Already Ignoring,
 *                  invert the sense of whether we are ignoring source input. 
 *              If this is not the only [ELSE] in the block, report an Error
 *                  and disregard it.
 *          If we are ignoring source input, for whatever reason, we still
 *              need to be sensitive to the nesting of Conditional Operators:
 *              If the word is a Conditional Operator, activate it with the
 *                  "Already Ignoring" parameter set to TRUE; doing so will
 *                  result in a nested call to this routine.
 *              Otherwise, i.e., if the word is not a Conditional Operator,
 *                  we may still need to process it in "ignoring" mode:
 *                  we need, for instance, to consume strings, comments
 *                  and the text-bodies of user-messages in their entirety,
 *                  in case there is a reference to an [ELSE] or suchlike.
 *                  The words that need processing while "ignoring" will
 *                  have a valid function-pointer in their ign_func field.
 *          If we are not ignoring source input, pass the word along to the
 *              tokenize_one_word routine and process it.  If the word is
 *              a Conditional Operator, it will be handled in the context
 *              of normal (i.e., non-ignored) tokenization, and, again, a
 *              nested call to this routine will result...
 *
 *      Revision History:
 *          Updated Thu, 23 Feb 2006 by David L. Paktor
 *              Conditional Blocks may begin with a Conditional Operator in
 *                  a Macro definition and do not need to be concluded in
 *                  the body of the Macro.
 *          Updated Fri, 10 Mar 2006 by David L. Paktor
 *              Recognize aliased string, comment and user-message delimiters
 *                  in a segment that is being ignored; Conditional Operators
 *                  within the text body of any of these are always consumed
 *                  and never unintentionally processed.  Macros are always
 *                  processed; Conditional Operators inside a Macro body are
 *                  recognized, so the Macro continues to function as intended.
 *
 **************************************************************************** */

static void conditionally_tokenize( bool cond, bool alr_ign )
{
    
    signed long wlen;

    /*  Note:  The following variables *must* remain within
     *      the scope of this routine; a distinct instance
     *      is needed each time this routine is re-entered
     *     (aka "a nested call").
     */
    bool ignoring;
    bool first_else = TRUE;  /*  The "else" we see is the first.  */
    bool not_done = TRUE;
    unsigned int cond_strt_lineno = lineno;
    char *cond_strt_ifile_nam = strdup( iname);

    ignoring = BOOLVAL( ( cond == FALSE ) || ( alr_ign != FALSE ) );

    if ( trace_conditionals )
    {
        char *cond_val = cond ? "True" : "False" ;
	char *cond_junct = alr_ign ? ", but Already " : "; ";
	char *processg = ignoring ? "Ignoring" : "Processing" ;
	tokenization_error( INFO,
	    "Tokenization-Condition is %s%s%s.\n",
		cond_val, cond_junct, processg);
    }

    while ( not_done )
    {
        wlen = get_word();
	if ( wlen == 0 )
	{
	    continue;
	}

	if ( wlen < 0 )
	{
	    tokenization_error( TKERROR,
	        "Conditional without conclusion; started");
	    just_where_started( cond_strt_ifile_nam, cond_strt_lineno);
	    not_done = FALSE ;
	    continue;
	}

	if ( is_a_then ( statbuf ) )
	{
	    if ( trace_conditionals )
	    {
		tokenization_error( INFO,
		    "Concluding Conditional");
		just_started_at( cond_strt_ifile_nam, cond_strt_lineno);
	    }
	    not_done = FALSE ;
	    continue;
	}

	if ( is_an_else( statbuf ) )
	{
	    if ( ! alr_ign )
	    {
		if ( first_else )
		{
		    ignoring = INVERSE( ignoring);
		}
	    }

	    if ( ! first_else )
	    {
		int severity = ignoring ? WARNING : TKERROR ;
		char *the_scop = ignoring ? "(ignored)" : "the" ;
		tokenization_error( severity, "Multiple %s directives "
		    "within %s scope of the Conditional",
			 strupr(statbuf), the_scop);
		just_started_at( cond_strt_ifile_nam, cond_strt_lineno);
	    }else{
		first_else = FALSE;
		if ( trace_conditionals )
		{
		    char *when_enc = alr_ign ? "While already" : "Now" ;
		    char *processg = alr_ign ? "ignoring" :
				    ignoring ? "Ignoring" : "Processing" ;
		    char *enc       = alr_ign ? ", e" : ".  E" ;

		    tokenization_error( INFO,
			"%s %s%sncountered %s belonging to Conditional",
			    when_enc, processg, enc, strupr(statbuf) );
		    just_started_at( cond_strt_ifile_nam, cond_strt_lineno);
		}
	    }

	    continue;
	}

	/*  If we are ignoring source input, for whatever reason, we still
	 *      need to be sensitive to the nesting of Conditional Operators
	 *      and some other commands and directives, as indicated...
	 */
	if ( ignoring )
	{
	    ignore_one_word( statbuf );
	}else{
	    /*  And if we're not ignoring source input, process it! */
	    tokenize_one_word ( wlen );
	}
    }
}

/* **************************************************************************
 *
 *      We will now define a series of fairly simple functions that
 *          will be performed by the various Conditional Operators in
 *          the "Conditionals Vocabulary".
 *
 *      Each one takes, as an argument, the "parameter field" pointer,
 *          which, in all cases, points to the local  already_ignoring 
 *          flag, passed as an int to satisfy C's strong-typing.  The
 *          routine will internally recast it as a  bool .
 *
 *      If it is TRUE, the routine will bypass the test for its particular
 *          type of condition, and go directly to  conditionally_tokenize
 *          In most cases, testing for the condition would be harmless,
 *          but in the case where the test is for an item on the stack,
 *          it would be harmful because the sequence that put the item
 *          on the stack was also being ignored...
 *
 *      We'll give these functions short prologs.  Synonyms will simply
 *          have separate entries in the Vocabulary Table, associated
 *          with the same function.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      But first, a support routine...
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Function name:  conditional_word_in_line
 *      Synopsis:       Common code for the types of conditionals that
 *                          require a word on the same line.
 *
 *      Inputs:
 *         Parameters:
 *             alr_ign         TRUE if we are already ignoring
 *             exist_test      TRUE if the test is for "existence" of the word
 *             exist_funct     Name of the function to call for the test
 *         Global Variables:
 *             stat_word       Word for which to test
 *
 *      Outputs:
 *         Returned Value:     NONE
 *
 *      Error Detection:
 *          The word in question must appear on the same line as the directive;
 *              the call to  get_word_in_line()  checks for that and reports.
 *          If the word did not appear on the same line, then the directive
 *              will be disregarded and processing will proceed as though it
 *              were absent.  This may lead to a cascade of errors...
 *
 *      Process Explanation:
 *          The supplied  exist_funct()  will test for the existence of
 *              the word, now read into  statbuf , in the appropriate 
 *              venue.
 *          We only call the  exist_funct()  if we are not already ignoring.
 *
 **************************************************************************** */

static void conditional_word_in_line( bool alr_ign,
                                          bool exist_test,
                                              bool (*exist_funct)() )
{
    if ( get_word_in_line( statbuf) )
    {
    	bool cond = FALSE;
	if ( INVERSE( alr_ign) )
	{
	    bool exists = exist_funct( statbuf);
	    cond = BOOLVAL( exists == exist_test);
	}
	conditionally_tokenize( cond, alr_ign );
    }
}


/* **************************************************************************
 *
 *      Function name:  if_exists
 *      Synopsis:       Test for existence of a given word, in the dictionary.
 *
 *      Associated Tokenizer directives:        [ifexist]
 *                                              #ifexist
 *                                              [#ifexist]
 *                                              [ifexists]
 *                                              #ifexists
 *                                              [#ifexists]
 *        (Note variants with and without final 's'
 *
 **************************************************************************** */

static void if_exists( tic_param_t pfield )
{
    bool alr_ign = *pfield.bool_ptr;
    conditional_word_in_line( alr_ign, TRUE, exists_in_current );
}

/* **************************************************************************
 *
 *      Function name:  if_not_exist
 *      Synopsis:       Test for Non-existence, in the appropriate dictionary,)
 *                          of the given word.
 *
 *      Associated Tokenizer directives:        [ifnexist]
 *                                              #ifnexist
 *                                              [#ifnexist]
 *        (Note:  Variants with final 's' didn't make sense here.)
 *
 *      Explanatory Notes:
 *          This is the exact inverse of  if_exists
 *
 **************************************************************************** */

static void if_not_exist( tic_bool_param_t pfield )
{
    bool alr_ign = *pfield.bool_ptr;
    conditional_word_in_line( alr_ign, FALSE, exists_in_current );
}

/* **************************************************************************
 *
 *      Function name:  if_defined
 *      Synopsis:       Test for existence of a user-defined symbol
 *
 *      Associated Tokenizer directives:        [ifdef]
 *                                              #ifdef
 *                                              [#ifdef]
 *
 **************************************************************************** */

static void if_defined( tic_bool_param_t pfield )
{
    bool alr_ign = *pfield.bool_ptr;
    conditional_word_in_line( alr_ign, TRUE, exists_as_user_symbol );
}

/* **************************************************************************
 *
 *      Function name:  if_not_defined
 *      Synopsis:       Test for NON-existence of a user-defined symbol
 *
 *      Associated Tokenizer directives:        [ifndef]
 *                                              #ifndef
 *                                              [#ifndef]
 *
 **************************************************************************** */

static void if_not_defined( tic_bool_param_t pfield )
{
    bool alr_ign = *pfield.bool_ptr;
    conditional_word_in_line( alr_ign, FALSE, exists_as_user_symbol );
}


/* **************************************************************************
 *
 *      Function name:  if_from_stack
 *      Synopsis:       Test the number on top of the run-time stack
 *
 *      Associated Tokenizer directive:         [if]
 *
 *      Process Explanation:
 *          When we are ignoring source input, and we still need to be
 *              sensitive to the nesting of Conditional Operators, we
 *              will not consume the number on the stack; this function
 *              is after all, being ignored and should not perform any
 *              action other than making sure the [else]s and [then]s
 *              get properly counted.
 *
 **************************************************************************** */

static void if_from_stack( tic_bool_param_t pfield )
{
    bool alr_ign = *pfield.bool_ptr;
    bool cond = FALSE;

    if ( ! alr_ign )
    {
        long num = dpop();
	if (num != 0)
	{
	    cond = TRUE;
	}
    }
    conditionally_tokenize( cond, alr_ign );
}

/*  For future functions, use  conditl.BlankTemplate.c  */

/* **************************************************************************
 *
 *      Here, at long last, we define and initialize the structure containing
 *          all the functions we support for Conditional Operators.
 *
 **************************************************************************** */

#define ADD_CONDL(str, func )   BUILTIN_BOOL_TIC(str, func, already_ignoring )

static tic_bool_hdr_t conditionals_vocab_tbl[] = {
    ADD_CONDL ("[ifexist]"   , if_exists      ) ,
    ADD_CONDL ("[ifexists]"  , if_exists      ) ,
    ADD_CONDL ("#ifexist"    , if_exists      ) ,
    ADD_CONDL ("#ifexists"   , if_exists      ) ,
    ADD_CONDL ("[#ifexist]"  , if_exists      ) ,
    ADD_CONDL ("[#ifexists]" , if_exists      ) ,
    ADD_CONDL ("[ifnexist]"  , if_not_exist   ) ,
    ADD_CONDL ("#ifnexist"   , if_not_exist   ) ,
    ADD_CONDL ("[#ifnexist]" , if_not_exist   ) ,
    ADD_CONDL ("[ifdef]"     , if_defined     ) ,
    ADD_CONDL ("#ifdef"      , if_defined     ) ,
    ADD_CONDL ("[#ifdef]"    , if_defined     ) ,
    ADD_CONDL ("[ifndef]"    , if_not_defined ) ,
    ADD_CONDL ("#ifndef"     , if_not_defined ) ,
    ADD_CONDL ("[#ifndef]"   , if_not_defined ) ,
    ADD_CONDL ("[if]"        , if_from_stack  )
};


/* **************************************************************************
 *
 *      Function name:  init_conditionals_vocab
 *      Synopsis:       Initialize the "Conditionals Vocabulary Table"
 *                          link-pointers dynamically, and link it in
 *                          with the given ("Global") Vocabulary pointer.
 *
 **************************************************************************** */

void init_conditionals_vocab( tic_hdr_t **tic_vocab_ptr )
{
    static const int conditionals_vocab_max_indx =
	 sizeof(conditionals_vocab_tbl)/sizeof(tic_bool_hdr_t);

    init_tic_vocab( (tic_hdr_t *)conditionals_vocab_tbl,
                        conditionals_vocab_max_indx,
                            tic_vocab_ptr );
}

