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
 *      General-purpose support functions for
 *          User-defined command-line compilation-control symbols
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      The syntax for user-defined command-line compilation-control symbols
 *          is <NAME>[=<VALUE>]
 *
 *      The name is always required; the equal-sign and value are optional.
 *          If you wish the "value" to contain spaces or quotes, you can
 *          accomplish that using the shell escape conventions.
 *
 *      The operations that can be performed upon these symbols will be
 *          described by the operators that use them as operands, but,
 *          broadly speaking, the tests will either be to simply verify
 *          the existence of a symbol, or to evaluate the defined value.
 *
 *      Once a symbol is defined on the command-line, it stays in effect
 *          for the duration of the entire batch of tokenizations (i.e.,
 *          if there are multiple input files named on the command line).
 *          Also, there are no symbols defined at the outset.  Therefore,
 *          there is no need for either an "init" or a "reset" routine.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      User-defined command-line compilation-control symbols are
 *          implemented as a String-Substitution-type vocabulary.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          add_user_symbol            Add a user-defined symbol to the list
 *          exists_as_user_symbol      Confirm whether a given name exists
 *                                         as a user-defined symbol.
 *          eval_user_symbol           Evaluate the value assigned to a user
 *                                         symbol.
 *          list_user_symbols          Print the list of user-defined symbols
 *                                         for the Logfile.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Still to be done:
 *          Convert the handling of user-defined symbols to the T.I.C.
 *              data-structure and its support routines.  This should
 *              eliminate any further need of String-Substitution-type
 *              vocabularies.  User-defined symbols will, however, still
 *              need to be a separate vocabulary from the Global, because
 *              they are required to stay in effect for the duration of
 *              the entire batch of tokenizations...
 *          (Afterthought:  This is only true for user-defined symbols that
 *              were created on the command-line; if we ever allow symbols
 *              to be defined in the Source file, they should be as volatile
 *              as anything else that comes from a source file...
 *           Appending source-file-derived user-defined symbols to the Global
 *              Vocabulary could be a quasi-simple way to accomplish this.)
 *
 *          Enable the definition of user-symbols from the Source file, using
 *              a syntax like:  [define] symbol   or  [define] symbol=<value>
 *              (How to allow spaces into the <value>?  Maybe make the syntax
 *              [define] symbol = <value components to end of line> 
 *              delimited in a manner similar to Macro definitions.
 *          There might be a need to be able to  [undefine]  a user-symbol
 *              that would entail defining an  unlink_tic_entry  function.
 *              Not difficult; just keeping this around as a reminder...
 *
 **************************************************************************** */



#include <stdio.h>
#include <stdlib.h>
#if defined(__linux__) && ! defined(__USE_BSD)
#define __USE_BSD
#endif
#include <string.h>

#include "errhandler.h"
#include "strsubvocab.h"
#include "usersymbols.h"
#include "scanner.h"


/* **************************************************************************
 *
 *              Internal Static Variables
 *      user_symbol_list          Pointer to the "tail" of the list of
 *                                    user-defined symbols.
 *      user_symbol_count         Count of how many are defined
 *
 **************************************************************************** */

static str_sub_vocab_t *user_symbol_list = NULL;
static int user_symbol_count = 0;

/* **************************************************************************
 *
 *      Function name:  add_user_symbol
 *      Synopsis:       Add a user-defined symbol to the list
 *
 *      Inputs:
 *         Parameters:
 *             raw_symb             The string as supplied on the command-line.
 *         Local Static Variables:
 *             user_symbol_list     Pointer to the list of user-defined symbols.
 *
 *      Outputs:
 *         Returned Value:                NONE
 *         Local Static Variables:
 *             user_symbol_list     Will be updated.
 *             user_symbol_count    Will be incremented
 *         Memory Allocated:
 *             for the string(s) and the new entry
 *         When Freed?
 *             Never.  Well, upon termination of the program.  User-defined
 *                 symbols endure for the entire batch of tokenizations.
 *
 *      Process Explanation:
 *          The string in  raw_symb  may or may not include the optional
 *              equal-sign and value pair.  If the equal-sign is present,
 *              the remainder of the string will become the "value" that
 *              will be returned by the "lookup" routine.
 *          Memory for the name string and for the value, if there is one,
 *              will be allocated here, in one step.  Memory for the data
 *              structure itself will be allocated by the support routine.
 *
 **************************************************************************** */

void add_user_symbol(char *raw_symb)
{
    char *symb_nam;
    char *symb_valu;

    symb_nam = strdup(raw_symb);
    symb_valu = strchr(symb_nam,'=');
    if ( symb_valu != NULL )
    {
	*symb_valu = 0;
	symb_valu++;
    }
    add_str_sub_entry(symb_nam, symb_valu, &user_symbol_list );
    user_symbol_count++;
}


/* **************************************************************************
 *
 *      Function name:  exists_as_user_symbol
 *      Synopsis:       Confirm whether a given name exists
 *                      as a user-defined symbol.
 *
 *      Inputs:
 *         Parameters:
 *             symb_nam             The name for which to look.
 *         Local Static Variables:
 *             user_symbol_list     Pointer to the list of user-defined symbols.
 *
 *      Outputs:
 *         Returned Value:      TRUE if the name is found
 *
 **************************************************************************** */

bool exists_as_user_symbol(char *symb_nam)
{
    bool retval;

    retval = exists_in_str_sub(symb_nam, user_symbol_list );
    return (retval);
}

/* **************************************************************************
 *
 *      Function name:  eval_user_symbol
 *      Synopsis:       Evaluate the value assigned to a user-symbol.
 *
 *      Associated Tokenizer directive (synonyms):      [DEFINED]
 *                                                      #DEFINED
 *                                                      [#DEFINED]
 *
 *      Syntax Notes:
 *          (1)  The User-Defined-Symbol must appear
 *                   on the same line as the directive.
 *          (2)  This is not (yet) implemented in contexts that
 *                   directly read input from the stream, e.g.,
 *                   after  [']  or after  H#  etc.
 *
 *      Inputs:
 *         Parameters:       
 *             symb_nam             Name of the User-Defined-Symbol to evaluate
 *         Local Static Variables:
 *             user_symbol_list     Pointer to the list of user-defined symbols.
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         The assigned value will be tokenized.
 *
 *      Error Detection:
 *          Calling routine is responsible for verifying that the user-symbol
 *              is on the same line as the directive.
 *          ERROR if the symbol is not found
 *          WARNING if the symbol has no assigned value. 
 *
 *      Process Explanation:
 *          Look up the parameter in the User Symbol List,
 *          If it is not found, issue an ERROR and do nothing further.
 *          If it is found, attempt to retrieve its associated value
 *          If it has no associated value, issue a WARNING and
 *              do nothing further.  Otherwise...
 *          Interpret the associated value as though it were source.
 *
 *      Still to be done:
 *          Hook-in this routine to the processing of:  [']  F[']  H#  FLOAD
 *              etc., and wherever else it might be needed or useful.
 *
 **************************************************************************** */

void eval_user_symbol( char *symb_nam)
{
    str_sub_vocab_t *found = NULL;


    found = lookup_str_sub( symb_nam, user_symbol_list );
    if ( found == NULL )
    {
        tokenization_error ( TKERROR,
	    "Command-line symbol %s is not defined.\n", symb_nam); 
    }else{
	char *symb_valu = found->alias;

	if ( symb_valu == NULL )
	{
            tokenization_error ( WARNING,
		"No value assigned to command-line symbol %s\n", symb_nam );
	}else{
	    eval_string( symb_valu );
	}
    }

}
/* **************************************************************************
 *
 *      Function name:  list_user_symbols
 *      Synopsis:       Print the list of user symbols for the Logfile.
 *      
 *      Inputs:
 *         Parameters:              NONE
 *         Local Static Variables:
 *             user_symbol_list     Pointer to the list of user-defined symbols.
 *             user_symbol_count    Count of user-defined symbols.
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Printout:                List of user symbols and their definitions;
 *                                      nothing if user_symbol_list is NULL.
 *
 *      Process Explanation:
 *          We want to display the symbols in the same order they were created.
 *          We will:
 *              Allocate a temporary array of pointers.
 *              Step backwards through the linked-list of symbols, and
 *                  enter their pointers into the array.  The array order
 *                  reflects the backward-linked order of the linked-list
 *                  of symbols is kept and searched,
 *              Collect the maximum length of the symbol names.
 *              Step through the array in the reverse order, to follow the
 *                  order in which the symbols were defined.
 *                  Check for a duplicate of the current symbol name:
 *                      Look backwards through the array, at the names we
 *                          have not yet printed, which were defined later.
 *                          Since the later-defined value will prevail, the
 *                          notation should be on the earlier one.
 *                  Print the current name
 *                  Use the maximum name-length to space the equal-signs or
 *                      duplicate-name notation, as required, evenly.
 *              Free the temporary array.
 *
 *      Revision History:
 *          Updated Thu, 07 Sep 2006 by David L. Paktor
 *              Report duplicated symbol names.
 *
 *      Still to be done:
 *          Space the duplicate-name notation evenly; line it up past
 *               the longest name-with-value.
 *
 **************************************************************************** */

void list_user_symbols(void )
{
    str_sub_vocab_t *curr;

    if ( user_symbol_list != NULL )
    {
	/*  Collect the pointers and max length  */
	str_sub_vocab_t **symb_ptr;
	int indx = 0;
	int maxlen = 0;
	
	symb_ptr = (str_sub_vocab_t **)safe_malloc(
	   (sizeof(str_sub_vocab_t *) * user_symbol_count),
	       "collecting user-symbol pointers" );
	
	for (curr = user_symbol_list ; curr != NULL ; curr=curr->next)
	{
            symb_ptr[indx] = curr;
	    indx++;
	    if ( strlen(curr->name) > maxlen ) maxlen = strlen(curr->name);
	}
	
	/*  Now print 'em out  */
	printf("\nUser-Defined Symbols:\n");
	while ( indx > 0 )
	{
	    bool is_dup;
	    int dup_srch_indx;
	    indx--;
	    curr = symb_ptr[indx];

	    /*  Detect duplicate names.  */
	    dup_srch_indx = indx;
	    is_dup = FALSE;
	    while ( dup_srch_indx > 0 )
	    {
		str_sub_vocab_t *dup_cand;
		dup_srch_indx--;
		dup_cand = symb_ptr[dup_srch_indx];
		if ( strcmp( curr->name, dup_cand->name) == 0 )
		{
		    is_dup = TRUE;
		    break;
		}
	    }

	    printf("\t%s",curr->name);

	    if ( ( curr->alias != NULL ) || is_dup )
	    {
	        int strindx;
		for ( strindx = strlen(curr->name) ;
		      strindx < maxlen ;
		      strindx++ )
		{
		    printf(" ");
		}
	    }
	    if ( curr->alias != NULL )
	    {
		printf(" = %s",curr->alias);
	    }
	    if ( is_dup )
	    {
		printf(" *** Over-ridden" );
	    }
	    printf("\n");
	}
	free(symb_ptr);
    }
}
