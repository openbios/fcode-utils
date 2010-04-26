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
 *      Support routines for "Trace-Symbols" debugging feature
 *
 *      (C) Copyright 2006 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Revision History:
 *          Wed, 04 Oct 2006 by David L. Paktor
 *          Issue a message when a name on the trace list is invoked (as well
 *              as when it is created), but keep a limit on the speed penalty.
 *              (I.e., don't scan the trace-list for every symbol invoked.)
 *              Instead, we are going to install a function that prints the
 *              message; this requires scanning only once at creation time,
 *              which is already a necessity.  There are too many permutations
 *              of "action" functions to define a separate set that combine
 *              with the invocation-message, so we will have a separate field
 *              for the invocation-message function, which will be NULL if
 *              the entry is not being traced.  (If, however, it is faster
 *              to execute a null function than to first test for a NULL,
 *              then we can define a null function and put _that_ into the
 *              invocation-message-function field...
 *          Wed, 11 Oct 2006 by David L. Paktor
 *          We have reduced the Invocation Message Function to a single common
 *              routine; we will add a field that is a simple flag indicating
 *              if the entry is being traced.  At the common point where the
 *              entry's function is called, we will test the flag to decide
 *              whether call the Invocation Message routine.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          add_to_trace_list         Add the given name to the Trace List
 *          is_on_trace_list          Indicate whether the given name is
 *                                        on the Trace List
 *          tracing_fcode             Show a token's assigned FCode-token
 *                                        in a consistent format.
 *          trace_creation            Display a Trace-Note when a word known
 *                                        to be on the Trace List is created.
 *          trace_create_failure      Display a Trace-Note indicating a failed
 *                                        attempt to create a word, if it is
 *                                        on the Trace List.
 *          traced_name_error         Display a Trace-Note indicating a failed
 *                                        attempt to invoke an undefined word,
 *                                        if it is on the Trace List.
 *          invoking_traced_name      Display a Trace-Note when a word known
 *                                        to be on the Trace List is invoked.
 *          handle_invocation         Test whether a word is on the Trace List;
 *                                        if so, display a Trace-Note.
 *          show_trace_list           Display the trace-list (if any) at
 *                                        the start of the run
 *          trace_builtin             Test whether a pre-defined name is on
 *                                        the Trace List; issue a Trace-Note
 *                                        and set the entry's  tracing  flag.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *          Global Variables Exported
 *              split_alias_message     Back-channel way to permit a certain
 *                                          message about an alias to a Global
 *                                          definition only having local scope
 *                                          to become a Trace-Note instead of
 *                                          an Advisory when one of the names
 *                                          is being Traced.
 *
 **************************************************************************** */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "tracesyms.h"
#include "errhandler.h"
#include "scanner.h"
#include "vocabfuncts.h"
#include "devnode.h"
#include "toke.h"

int split_alias_message = INFO;

/* **************************************************************************
 *
 *          Internal Static Variables
 *     trace_list             Pointer to the first entry in the Trace List
 *                                linked-list data structure
 *     trace_list_last        Pointer to the last entry in the Trace List;
 *                                the list will be forward-linked
 *     tracing_symbols        TRUE if "Trace-Symbols" is in effect.
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Internal (Static) Structures:
 *          trace_entry_t           Linked-list of entries in the Trace List
 *
 *   Fields:
 *       tracee              Name of the symbol to be traced
 *       next                Pointer to next entry in the forward-linked-list
 *
 **************************************************************************** */

typedef struct trace_entry {
      char *tracee;
      struct trace_entry *next;
} trace_entry_t;

static trace_entry_t *trace_list = NULL;
static trace_entry_t *trace_list_last = NULL;

static bool tracing_symbols = FALSE;

/* **************************************************************************
 *
 *      Function name:  add_to_trace_list
 *      Synopsis:       Add the given name to the Trace List
 *                      
 *      Inputs:
 *         Parameters:
 *             trace_symb            Name of the symbol to be added
 *         Local Static Variables:
 *             trace_list           Pointer to the Trace List
 *             trace_list_last      Pointer to last entry in the Trace List
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Local Static Variables:
 *             trace_list           Points to first entry in Trace List
 *             trace_list_last      Pointer to new entry in the Trace List
 *             tracing_symbols      Set to TRUE
 *         Memory Allocated
 *             For Trace List entry
 *             For copy of Symbol Name
 *         When Freed?
 *             Never.  Well, only on termination of the program.  Trace-list
 *                 endures for the entire batch of tokenizations.
 *
 *      Process Explanation:
 *          The list will be forward-linked so that the display of the list
 *              can be in the same order as specified by the User.  It will
 *              take a little extra effort, but having it come out that way
 *              satisfies the "Rule of Least Astonishment"...
 *
 *      Error Detection:
 *          Memory allocation failure is a FATAL error.
 *
 **************************************************************************** */

void add_to_trace_list( char *trace_symb)
{
    trace_entry_t *new_t_l_entry = safe_malloc( sizeof( trace_entry_t),
        "adding to trace-list");
    new_t_l_entry->tracee = strdup( trace_symb);
    new_t_l_entry->next = NULL;

    if ( trace_list != NULL )
    {
	trace_list_last->next = new_t_l_entry;
    }else{
    trace_list = new_t_l_entry;
	tracing_symbols = TRUE;
    }
    trace_list_last = new_t_l_entry;
}


/* **************************************************************************
 *
 *      Function name:  is_on_trace_list
 *      Synopsis:       Indicate whether the given name is on the Trace List
 *
 *      Inputs:
 *         Parameters:
 *             symb_name            Symbol-name to test
 *         Local Static Variables:
 *             tracing_symbols      Skip the search if FALSE
 *             trace_list           Pointer to the Trace List
 *
 *      Outputs:
 *         Returned Value:          TRUE if Symbol-name is on the Trace List
 *
 **************************************************************************** */

bool is_on_trace_list( char *symb_name)
{
    bool retval = FALSE;
    if ( tracing_symbols )
    {
    trace_entry_t *test_entry = trace_list;
    while ( test_entry != NULL )
    {
        if ( strcasecmp( symb_name, test_entry->tracee) == 0 )
	{
	    retval = TRUE;
	    break;
	}
	    test_entry = test_entry->next;
	}
    }
    return ( retval );
}


/* **************************************************************************
 *
 *      Function name:  tracing_fcode
 *      Synopsis:       Fill the given buffer with the string showing the
 *                          assigned FCode-token given.  Produce a consistent
 *                          format for all messages that include this phrase.
 *
 *      Inputs:
 *         Parameters:
 *             fc_phrase_buff          Pointer to buffer for the phrase
 *             fc_token_num            Assigned FCode-token number.
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Supplied Pointers:
 *             fc_phrase_buff          Filled with the phrase.
 *
 *      Process Explanation:
 *          It is the responsibility of the calling routine to make sure
 *              that the buffer passed as the  fc_phrase_buff  parameter
 *              is sufficiently large for the phrase.
 *          This routine will test  fc_token_num  and leave the buffer
 *              unchanged if it is not greater than zero, relieving the
 *              calling routine of that chore.
 *
 *      Extraneous Remarks:
 *          We will export, in  tracesyms.h , a Macro that will give the
 *             recommended length for the buffer passed to this routine.
 *             It will be called  TRACING_FCODE_LENGTH 
 *
 **************************************************************************** */

void tracing_fcode( char *fc_phrase_buff, u16 fc_token_num)
{
    if ( fc_token_num > 0 )
    {
	sprintf( fc_phrase_buff,
		    " (FCode token = 0x%03x)", fc_token_num);
    }
}

/* **************************************************************************
 *
 *      Function name:  trace_creation
 *      Synopsis:       Display the appropriate message when a word, known
 *                          to be on the Trace List, is created.
 *
 *      Inputs:
 *         Parameters:
 *             trace_entry             The TIC entry of the name
 *             nu_name                 If creating an Alias, the "new" name,
 *                                         otherwise NULL.
 *             is_global               TRUE if trace_entry has Global scope.
 *         Global Variables:
 *             in_tokz_esc             TRUE if we are in Tokenizer-Escape mode
 *             current_device_node     Where the new definition's scope is,
 *                                         if it's not Global or Tokzr-Escape.
 *             hdr_flag                State of name-creation headered-ness
 *             in_tkz_esc_mode         String for announcing Tokz-Esc mode
 *             wh_defined              String, = ", which is defined as a "
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Global Variables:
 *             split_alias_message     Set to TRACER if creating an Alias.
 *         Printout:
 *             Trace-Note Message.
 *             If not an ALIAS, takes the form:
 *                 Creating <name> <(if is_single:FCode Token = xxx)>
 *                     as a{n External,Headered,Headerless} <Type>
 *                     <with scope>
 *             If it is an ALIAS, takes the form:
 *                 Creating <new-name> as a{n External,Headered,Headerless} 
 *                     ALIAS to <old-name> <(if is_single:FCode Token = xxx)>,
 *                    which is defined [ as a <Type>] <with scope>
 *             Headered-ness, of course, only applies to words with a
 *                      single-token FCode.
 *             The <with scope> phrase is:
 *                 If we are in Tokz-Esc mode, "in Tokenizer-Escape mode".
 *                 If the TIC entry is a Local, the current colon-definition.
 *                 If the TIC entry is Global in scope, "with Global scope"
 *                 Otherwise, the identification of the current device-node.
 *
 *      Process Explanation:
 *          The calling routine will already have verified that the
 *              name is on the trace-list.
 *          The name will already have been entered into a TIC_HDR vocab;
 *              
 *          The order of scope-checking is important:
 *              A Local has no scope beyond the definition in which it occurs.
 *              Tokenizer-Escape mode supercedes "Normal" mode, and renders
 *                  moot the differences between Global and Device scope.
 *              Global scope is mutually exclusive with Device scope.
 *              Device scope needs to identify where the Current device-node
 *                  began.
 *
 **************************************************************************** */

void trace_creation( tic_hdr_t *trace_entry,
                         char *nu_name,
			     bool is_global)
{
    char  fc_token_display[TRACING_FCODE_LENGTH] = "";
    char *head_ness = "";
    char *defr_name = "" ;
    char *defr_phrase = "" ;
    char *with_scope = "" ;
    bool def_is_local = BOOLVAL( trace_entry->fword_defr == LOCAL_VAL);
    bool creating_alias = BOOLVAL( nu_name != NULL );

    if ( creating_alias )
    {
	head_ness = "n";
	split_alias_message = TRACER;
    }

    if ( in_tokz_esc )
    {
	with_scope = in_tkz_esc_mode;
    }else{
	if ( ! def_is_local )
	{
	    if ( is_global )
	    {
		with_scope = "with Global scope.\n";
	    }else{
		with_scope = in_what_node( current_device_node);
	    }
	}

	if ( trace_entry->is_token )
	{
	    tracing_fcode( fc_token_display,
		               (u16)trace_entry->pfield.deflt_elem );
	    /*   Headered-ness only applies to FCode definitions  */
	    /*   But not to aliases to FCode definitions          */
	    if ( ! creating_alias )
	    {
		switch ( hdr_flag )
		{
		    case FLAG_HEADERS:
			head_ness = " Headered";
			break;

		    case FLAG_EXTERNAL:
			head_ness = "n External";
			break;

		    default:  /*   FLAG_HEADERLESS   */
			head_ness = " Headerless";
		}
	    }
	}

    }


    if ( definer_name(trace_entry->fword_defr, &defr_name) )
    {
	defr_phrase = wh_defined;
    }else{
	/*  Even if we don't have a Type for the "old" word
	 *      we still have its scope.  If the "new" word's
	 *      scope is different, the "split-alias message"
	 *      will take care of it.
	 */
	if ( creating_alias )
	{
	    defr_phrase = ", which is defined" ;
	}
    }

    if ( creating_alias )
    {
	/*
	 *         Creating <new-name> as a{n External,Headered,Headerless} 
	 *             ALIAS to <old-name> <(if is_single:FCode Token = xxx)>,
	 *             [which is defined as a <Type> <with scope>]
	 */
	tokenization_error(TRACER,
	    "Creating %s"            /*  nu_name                       */
	    " as a%s ALIAS to %s"    /*  head_ness  trace_entry->name  */
	    "%s"                     /*  fc_token_display              */
	    "%s%s "                  /*  defr_phrase defr_name         */
	    "%s",                    /*  with_scope  */
	    nu_name, head_ness, trace_entry->name,
	    fc_token_display, defr_phrase, defr_name, with_scope );
    }else{
	/*
	 *         Creating <name> <(if is_single:FCode Token = xxx)>
	 *             as a{n External,Headered,Headerless} <Type>
	 *             [ <with scope> ]
	 */
	tokenization_error(TRACER,
	    "Creating %s"       /*  trace_entry->name     */
	    "%s"                /*  fc_token_display      */
	    " as a%s %s "       /*  head_ness  defr_name  */
	    "%s",               /*  with_scope         */
	    trace_entry->name,
	    fc_token_display, head_ness, defr_name, with_scope );
    }
	/*
	 *     The <with scope> phrase is:
	 *         If we are in Tokz-Esc mode, "in Tokenizer-Escape mode".
	 *                  (Already handled.  No more to do here)
	 *         If the TIC entry is a Local, the current colon-definition.
	 *         If the TIC entry is Global in scope, "with Global scope"
	 *              (identified by NULL in_vocab).
	 *         Otherwise, the identification of the current device-node.
	 */


    if ( ! in_tokz_esc )
    {
	if ( def_is_local )
	{
	    in_last_colon( TRUE);
	}else{
	    show_node_start();
	}
    }

}


/* **************************************************************************
 *
 *      Function name:  trace_create_failure
 *      Synopsis:       Display a Trace-Note indicating a failed attempt
 *                          to create a word, if it is on the Trace List.
 * 
 *      Inputs:
 *         Parameters:
 *             new_name             The name of the word you failed to create
 *             old_name             If attempted to create an Alias, the
 *                                       "old" name, otherwise NULL.
 *             fc_tokn              FCode-Token that might have been assigned
 *                                       to the "new" name.  Zero if none.
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Printout:
 *             Failed to create <new_name> [FCode Token = <xxx>]
 *             [ as an ALIAS to <old_name>]\n"
 *
 **************************************************************************** */

void trace_create_failure( char *new_name, char *old_name, u16 fc_tokn)
{
    bool not_alias   = BOOLVAL( old_name == NULL );
    bool do_it       = is_on_trace_list( new_name);

    if ( ( ! do_it ) && ( ! not_alias ) )
    {
        do_it       = is_on_trace_list( old_name);
    }

    if ( do_it )
    {
	char  fc_token_display[TRACING_FCODE_LENGTH] = "";
	char *as_alias   = not_alias ? "" : " as an ALIAS to ";
	char *alias_name = not_alias ? "" : old_name;

	if ( fc_tokn > 0 )
	{
	   tracing_fcode( fc_token_display, fc_tokn);
	}
	tokenization_error(TRACER, 
		"Failed to create %s"          /*  new_name          */
		"%s"                           /*  fc_token_display  */
		"%s"                           /*  as_alias          */
		"%s\n",                        /*  alias_name        */
		new_name, fc_token_display,
		    as_alias, alias_name);
    }
}


/* **************************************************************************
 *
 *      Function name:  traced_name_error
 *      Synopsis:       Display a Trace-Note indicating a failed attempt
 *                          to invoke an undefined word, if it is on the
 *                          Trace List.
 *
 *      Inputs:
 *         Parameters:
 *             trace_name              The name being invoked
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Printout:
 *             A Trace-Note message indicating a attempt to "invoke" the name
 *
 *      Error Detection:
 *          Error was detected by calling routine
 *
 *      Process Explanation:
 *          Test if the name is on the trace-list, first.
 *
 **************************************************************************** */

void traced_name_error( char *trace_name)
{
    if ( is_on_trace_list( trace_name ) )
    {
	tokenization_error(TRACER, "Attempt to invoke (undefined) %s.\n",
	     trace_name);
    }
}

/* **************************************************************************
 *
 *      Function name:  invoking_traced_name
 *      Synopsis:       Print a message indicating that the given name,
 *                          (which is on the trace-list) is being invoked.
 *                          If available, show the name's assigned FCode-
 *                          -token, definition-type, and scope.
 *
 *      Inputs:
 *         Parameters:
 *             trace_entry             The TIC entry of the name
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Printout:
 *             A Trace-Note message:
 *                  Invoking <name> [FCode Token = <xxx>] [defined as a <Type>]
 *
 *      Process Explanation:
 *          Although we could test the entry here to see whether it is
 *              being traced, we do not want to incur the speed penalty
 *              of the overhead of unconditionally calling a routine that,
 *              in most cases, will not do anything, every time a token
 *              is processed.  We believe it is faster for the calling
 *              routine to conduct that test.
 *          Therefore, this routine will pre-suppose that the "tracing"
 *              field of the entry is TRUE.
 *
 **************************************************************************** */

void invoking_traced_name( tic_hdr_t *trace_entry)
{

    char  fc_token_display[TRACING_FCODE_LENGTH] = "";
    char *defr_name   = "" ;
    char *defr_phrase = "" ;
    char *defr_space  = "" ;

    if ( trace_entry->is_token )
    {
	tracing_fcode( fc_token_display,
		           (u16)trace_entry->pfield.deflt_elem );
    }

    if ( definer_name(trace_entry->fword_defr, &defr_name) )
    {
	defr_phrase = " defined as a";
	defr_space  = " " ;

    }

    tokenization_error(TRACER,
	"Invoking %s"                  /*  <name>                      */
	"%s"                           /*  fc_token_display            */
	"%s%s"                         /*  defr_phrase  defr_space     */
	"%s.\n",                       /*  defr_name                   */
	trace_entry->name,
	fc_token_display,
	defr_phrase, defr_space,
	defr_name);

}

/* **************************************************************************
 *
 *      Function name:  handle_invocation
 *      Synopsis:       Test whether the entry is being traced;
 *                          if so, print the Trace-Note message.
 *
 *      Inputs:
 *         Parameters:
 *             trace_entry             The TIC entry to be tested.
 *                                         No harm if NULL
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Printout:
 *             A Trace-Note message, by invoking_traced_name() if
 *                 the entry's tracing field is set.
 *
 *      Extraneous Remarks:
 *          In cases that do not occur every time a token is processed,
 *              we can trade-off a small price in speed for convenience
 *              of coding.  Call this routine judiciously...
 *
 **************************************************************************** */

void handle_invocation( tic_hdr_t *trace_entry)
{
    if ( trace_entry != NULL )
    {
	if ( trace_entry->tracing)
	{
	    invoking_traced_name( trace_entry);
	}
    }
}


/* **************************************************************************
 *
 *      Function name:  show_trace_list
 *      Synopsis:       Display the trace-list (if any) at the start of the run
 *
 *      Inputs:
 *         Parameters:                 NONE
 *         Local Static Variables:
 *             tracing_symbols         Skip the display if FALSE
 *             trace_list              Pointer to the Trace List
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Printout:
 *             List of symbols being traced; nothing if not tracing.
 *
 *      Error Detection:
 *          Memory allocation failure is a FATAL error.
 *
 **************************************************************************** */

void show_trace_list( void)
{
    if ( tracing_symbols )
    {
	trace_entry_t *test_entry;
	printf("\nTracing these symbols:");
	for ( test_entry = trace_list;
	      test_entry != NULL;
	      test_entry = test_entry->next )
	{
            printf("   %s", test_entry->tracee);
	    
	}	
	printf("\n");
    }

}

/* **************************************************************************
 *
 *      Function name:  trace_builtin
 *      Synopsis:       If the given built-in token, function, macro or
 *                          directive is one the user asked to have Traced,
 *                          issue a Trace-Note describing it, and set the
 *                          TIC_HDR entry's  tracing  field to TRUE.
 *                      Called while initializing vocab-lists.
 *
 *      Inputs:
 *         Parameters:
 *             trace_entry             The TIC entry of the built-in name
 *         Global Variables:
 *             in_tokz_esc             TRUE if initializing the vocab-list
 *                                         for Tokenizer-Escape mode.
 *             in_tkz_esc_mode         String for announcing Tokz-Esc mode
 *         Local Static Variables:
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Supplied Pointers:
 *             trace_entry->tracing    Set TRUE, if the entry's name is on
 *                                         the Trace List.
 *         Printout:
 *             Trace-Note:  <name> <(if is_single:FCode Token = xxx)>
 *                          is a built-in {word|<Type>} [in Tokz-Esc mode]
 *
 **************************************************************************** */
 
void trace_builtin( tic_hdr_t *trace_entry)
{
    if (is_on_trace_list( trace_entry->name ) )
    {
	char  fc_token_display[TRACING_FCODE_LENGTH] = "";
	char *defr_name = "word";
	char *ws_space = in_tokz_esc ? " " : "";
	char *with_scope = in_tokz_esc ? in_tkz_esc_mode  : ".\n";
	if ( trace_entry->is_token )
	{
	    tracing_fcode( fc_token_display,
		           (u16)trace_entry->pfield.deflt_elem );
	}
	definer_name(trace_entry->fword_defr, &defr_name);
	trace_entry->tracing = TRUE;
	tokenization_error(TRACER,
	    "%s"                             /*  <name>                 */
	    "%s "                            /*  fc_token_display       */
	    "is a built-in %s"               /*  defr_name              */
	    "%s%s",                          /*  ws_space with_scope    */
	    trace_entry->name,
	    fc_token_display, defr_name, ws_space, with_scope );
    }
}
 
