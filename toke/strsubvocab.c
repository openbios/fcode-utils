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
 *      General-purpose support functions for
 *          String-Substitution-type vocabularies
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      A String-Substitution vocabulary, as the name implies, is one in
 *          in which each an entry consists of two strings;  one that is
 *          sought, and one that is returned as a substitute.  Macros and
 *          aliases are implemented this way, as are also user-supplied
 *          command-line symbol definitions.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          init_str_sub_vocab     Initialize a String-Substitution vocab
 *          add_str_sub_entry      Add an entry to a Str-Subst vocab
 *          lookup_str_sub         Look for a name in a String-Substitution
 *                                     vocab, return the substitution string.
 *          exists_in_str_sub      Confirm whether a given name exists in a
 *                                     String-Substitution vocabulary
 *          create_str_sub_alias   Duplicate the behavior of one name with
 *                                     another name.  Return a "success" flag.
 *          reset_str_sub_vocab    Reset a given Str-Subst vocab to its initial
 *                                    "Built-In" position.
 *
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


/* **************************************************************************
 *
 *      Function name:  init_str_sub_vocab
 *      Synopsis:       Dynamically initialize the link-pointers
 *                          of the.given String-Substitution vocabulary
 *      
 *      Inputs:
 *         Parameters:
 *             str_sub_vocab_tbl   Pointer to the initial Str-Subst vocab array
 *             max_indx            Maximum Index of the initial array.
 *
 *      Outputs:
 *         Returned Value:          None
 *         Global Variables:
 *              The link-fields of the initial Str-Subs vocab array entries
 *                  will be filled in.
 *
 **************************************************************************** */

void init_str_sub_vocab( str_sub_vocab_t *str_sub_vocab_tbl, int max_indx)
{
    int indx;
    for ( indx = 1 ; indx < max_indx ; indx++ )
    {
        str_sub_vocab_tbl[indx].next = &str_sub_vocab_tbl[indx-1];
    }
}

/* **************************************************************************
 *
 *      Function name:  add_str_sub_entry
 *      Synopsis:       Add an entry to the given Str-Subst vocab
 *      
 *      Inputs:
 *         Parameters:         Pointer to:
 *             ename               space containing the name of the entry
 *             subst_str           space containing the substitution string
 *             *str_sub_vocab      the "tail" of the Str-Subst vocab-list 
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Supplied Pointers:
 *             *str_sub_vocab       Will point to new entry
 *         Memory Allocated:
 *             Memory for the new entry will be allocated.
 *         When Freed?
 *             When reset_str_sub_vocab() is applied to the same vocab-list.
 *                 In some instances, the new entry will be freed upon end
 *                 of tokenization; in others, only on termination of program.
 *
 *      Error Detection:
 *          Failure to allocate memory is a Fatal Error.
 *
 *      Process Explanation:
 *          The name and substitution-string pointers are presumed to already
 *              point to stable memory-spaces.  Memory will be allocated
 *              for the entry itself; its pointers will be entered and the
 *              given pointer-to-the-tail-of-the-vocabulary will be updated.
 *
 *      Extraneous Remarks:
 *          This might have been where we would have checked for re-aliasing,
 *              but the introduction of the approach to aliasing embodied in
 *              the various  create_..._alias()  routines neatly bypasses it.
 *
 **************************************************************************** */

void add_str_sub_entry( char *ename,
                            char *subst_str,
			        str_sub_vocab_t **str_sub_vocab )
{
    str_sub_vocab_t *new_entry;

    new_entry = safe_malloc(sizeof(str_sub_vocab_t), "adding str_sub_entry");
    new_entry->name   =  ename;
    new_entry->alias  =  subst_str;
    new_entry->next   = *str_sub_vocab;

    *str_sub_vocab = new_entry;

}


/* **************************************************************************
 *
 *      Function name:  lookup_str_sub
 *      Synopsis:       Look for a name in the given Str-Subst vocabulary
 *      
 *      Inputs:
 *         Parameters:
 *             tname                The "target" name for which to look
 *             str_sub_vocab        The Str-Subst vocab-list
 *
 *      Outputs:
 *         Returned Value:          Pointer to the substitution string, or
 *                                      NULL pointer if name not found.
 *                                  May be NULL if subst'n string is NULL.
 *
 **************************************************************************** */

char *lookup_str_sub( char *tname, str_sub_vocab_t *str_sub_vocab )
{
    str_sub_vocab_t *curr;
    char *retval = NULL;

    for (curr = str_sub_vocab ; curr != NULL ; curr=curr->next)
    {
        if ( strcasecmp(tname, curr->name) == 0 )
	{
	    retval = curr->alias;
	    break;
	}
    }
    return ( retval ) ;
}

/* **************************************************************************
 *
 *      Function name:  exists_in_str_sub
 *      Synopsis:       Confirm whether a given name exists in a given
 *                          String-Substitution vocabulary
 *      
 *      Inputs:
 *         Parameters:
 *             tname                The "target" name for which to look
 *             str_sub_vocab        Pointer to the Str-Subst vocab-list
 *
 *      Outputs:
 *         Returned Value:     TRUE if the name is found
 *
 *      Extraneous Remarks:
 *          Because the Returned Value of lookup_str_sub() may be NULL for 
 *              other reasons than that the name was not found, we cannot
 *              rely on that routine, and must replicate the outer-shell
 *              of its structure.
 *
 **************************************************************************** */

bool exists_in_str_sub( char *tname, str_sub_vocab_t *str_sub_vocab )
{
    str_sub_vocab_t *curr;
    bool retval = FALSE;

    for (curr = str_sub_vocab ; curr != NULL ; curr=curr->next)
    {
        if ( strcasecmp(tname, curr->name) == 0 )
	{
	    retval = TRUE;
	    break;
	}
    }
    return ( retval );

}

/* **************************************************************************
 *
 *      Function name:  create_str_sub_alias
 *      Synopsis:       Create an Alias in a String-Substitution vocabulary
 *                          Return a "success" flag.
 *
 *      Associated FORTH word:                 ALIAS
 *
 *      Inputs:
 *         Parameters:
 *             old_name             Name of existing entry
 *             new_name             New name for which to create an entry
 *             *str_sub_vocab       Pointer to "tail" of Str-Subst vocab-list
 *
 *      Outputs:
 *         Returned Value:          TRUE if  old_name  found in str_sub_vocab
 *         Supplied Pointers:
 *             *str_sub_vocab       Will be updated to point to the new entry
 *         Memory Allocated:
 *             A copy of the "old" name's substitution string.
 *         When Freed?
 *             When reset_str_sub_vocab() is applied to the same vocab-list.
 *                 In some instances, the new entry will be freed when the
 *                 device-node in which it was created is "finish"ed; in
 *                 others, only on termination of the program.
 *
 *      Process Explanation:
 *          The "new" name is presumed to point to a stable memory-space.
 *          If the given "old" name exists in the given Str-Subst vocab-list,
 *              duplicate the substitution string into newly-allocated memory
 *              and pass the duplicated string and the "new" name along to
 *              the  add_str_sub_entry()  routine and return TRUE.
 *          If the given "old" name does not exist in the given vocab-list,
 *              return FALSE.
 *
 *      Extraneous Remarks:
 *          This neatly bypasses the question of re-aliasing...  ;-)
 *
 *          We can rely on testing for a returned NULL from lookup_str_sub()
 *              because we won't be applying this routine to any vocabulary
 *              that permits a NULL in its substitution string.
 *
 **************************************************************************** */

bool create_str_sub_alias(char *new_name,
                              char *old_name,
			          str_sub_vocab_t **str_sub_vocab )
{
    bool retval = FALSE;
    char *old_subst_str = lookup_str_sub( old_name, *str_sub_vocab );
    if ( old_subst_str != NULL )
    {
        char *new_subst_str = strdup(old_subst_str );
	add_str_sub_entry(new_name, new_subst_str, str_sub_vocab );
	retval = TRUE ;
    }

    return ( retval );
}


/* **************************************************************************
 *
 *      Function name:  reset_str_sub_vocab
 *      Synopsis:       Reset a given Str-Subst vocab to its initial
 *                         "Built-In" position.
 *      
 *      Inputs:
 *         Parameters:
 *            *str_sub_vocab         Pointer to the Str-Subst vocab-list 
 *             reset_position        Position to which to reset the list
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Supplied Pointers:
 *             *str_sub_vocab       Reset to given "Built-In" position.
 *         Memory Freed
 *             All memory allocated by user-definitions will be freed
 *
 *      Process Explanation:
 *          The "stable memory-spaces" to which the name and substitution
 *              string pointers point are presumed to have been acquired
 *              by allocation of memory, which is reasonable for entries
 *              created by the user as opposed to the built-in entries,
 *              which we are, in any case, not releasing.
 *          The substitution-string pointer may be null; watch out when
 *              we free() it; not all C implementations forgive that.
 *
 **************************************************************************** */

void reset_str_sub_vocab(
            str_sub_vocab_t **str_sub_vocab ,
	             str_sub_vocab_t *reset_position )
{
    str_sub_vocab_t *next_t;

    next_t = *str_sub_vocab;
    while ( next_t != reset_position  )
    {
	next_t = (*str_sub_vocab)->next ;

	free( (*str_sub_vocab)->name );
	if ( !(*str_sub_vocab)->alias )
	{
	    free( (*str_sub_vocab)->alias );
	}
	free( *str_sub_vocab );
	*str_sub_vocab = next_t ;
    }
}

