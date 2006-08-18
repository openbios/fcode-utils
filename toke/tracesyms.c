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
 *      Support routines for "Trace-Symbols" debugging feature
 *
 *      (C) Copyright 2006 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          add_to_trace_list         Add the given name to the Trace List
 *          show_initial_traces       Show pre-defined names the user
 *                                        asked to Trace (if any)
 *          is_on_trace_list          Indicate whether the given name is
 *                                        on the Trace List
 *
 **************************************************************************** */

#include <string.h>

#include "tracesyms.h"
#include "errhandler.h"


/* **************************************************************************
 *
 *          Internal Static Variables
 *     trace_list     Pointer to last entry in the Trace List linked-list
 *                        data structure
 *              
 *              
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Internal (Static) Structures:
 *          trace_entry_t           Linked-list of entries in the Trace List
 *
 *   Fields:
 *       tracee              Name of the symbol to be traced
 *       prev                Pointer to previous entry in the linked-list
 *
 **************************************************************************** */

typedef struct trace_entry {
      char *tracee;
      struct trace_entry *prev;
} trace_entry_t;

static trace_entry_t *trace_list = NULL;



/* **************************************************************************
 *
 *      Function name:  add_to_trace_list
 *      Synopsis:       Add the given name to the Trace List
 *                      
 *
 *      Inputs:
 *         Parameters:
 *             trace_symb            Name of the symbol to be added
 *         Local Static Variables:
 *             trace_list           Pointer to the Trace List
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Local Static Variables:
 *             trace_list           Points to new entry in Trace List
 *         Memory Allocated
 *             For Trace List entry
 *             For copy of Symbol Name
 *         When Freed?
 *             Never.  Well, only on termination of the program.  Trace-list
 *                 endures for the entire batch of tokenizations.
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
    new_t_l_entry->prev = trace_list;

    trace_list = new_t_l_entry;
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
 *             trace_list           Pointer to the Trace List
 *
 *      Outputs:
 *         Returned Value:          TRUE if Symbol-name is on the Trace List
 *
 **************************************************************************** */

bool is_on_trace_list( char *symb_name)
{
    bool retval = FALSE;
    trace_entry_t *test_entry = trace_list;
    while ( test_entry != NULL )
    {
        if ( strcasecmp( symb_name, test_entry->tracee) == 0 )
	{
	    retval = TRUE;
	    break;
	}
	test_entry = test_entry->prev;
    }
    return ( retval );
}


/* **************************************************************************
 *
 *      Still to be done:
 *          Implement a function -- name it  show_initial_traces  --
 *              that will show any pre-defined names the user asked
 *              to Trace.  That is, if any of the names the user asked
 *              to Trace belongs to a pre-defined function, macro or
 *              directive, then, at the beginning of the output, issue 
 *              Advisory Messages identifying the scope of those names.
 * 
 *          E.g, if the user had  -T 3DUP  -T SWAP   the function would
 *              issue Messages like:
 *          3DUP is pre-defined as a Macro with Global scope
 *          SWAP is pre-defined with Global scope
 *          SWAP is pre-defined in Tokenizer-Escape mode
 * 
 *          The names would, of course, remain on the Trace List and
 *              any re-definitions of them would be reported.
 *
 **************************************************************************** */
