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
 *          add_str_sub_entry      Add an entry to a Str-Subst vocab
 *          lookup_str_sub         Look for a name in a String-Subst'n vocab;
 *                                     return a pointer to the structure.
 *          exists_in_str_sub      Confirm whether a given name exists in a
 *                                     String-Substitution vocabulary
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
 *      Synopsis:       Look for a name in the given Str-Subst vocabulary.
 *                      Return a pointer to the structure if name was valid
 *      
 *      Inputs:
 *         Parameters:
 *             tname                The "target" name for which to look
 *             str_sub_vocab        The Str-Subst vocab-list
 *
 *      Outputs:
 *         Returned Value:          Pointer to the substitution-string entry
 *                                      data-structure.  NULL if not found.
 *
 **************************************************************************** */

str_sub_vocab_t *lookup_str_sub( char *tname, str_sub_vocab_t *str_sub_vocab )
{
    str_sub_vocab_t *curr;
    str_sub_vocab_t *retval = NULL;

    for (curr = str_sub_vocab ; curr != NULL ; curr=curr->next)
    {
        if ( strcasecmp(tname, curr->name) == 0 )
	{
	    retval = curr;
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
 *         Returned Value:          TRUE if the name is found
 *
 **************************************************************************** */

bool exists_in_str_sub( char *tname, str_sub_vocab_t *str_sub_vocab )
{
    bool retval = FALSE;
    str_sub_vocab_t *found = NULL;

    found = lookup_str_sub( tname, str_sub_vocab );
    if ( found != NULL )
    {
        retval = TRUE;
    }
    return ( retval );

}

