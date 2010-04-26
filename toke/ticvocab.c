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
 *      Threaded Interpretive Code (T. I. C.)-type vocabularies
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      We are going to implement a strategy that takes better advantage
 *          of the concept of Threaded Interpretive Code  (well, okay,
 *          it won't really be interpretive ... )  We will use it to
 *          implement a small (but expandable) subset of FORTH-like
 *          commands in Tokenizer-Escape mode, as well as a few other
 *          things, such as conditional-tokenization.
 *
 *      The Threaded Interpretive Code Header data structure is described
 *          in detail in the accompanying  ticvocab.h  header-file.
 *
 *      In most cases, the contents of a beginning portion of the vocabulary
 *          are known at compile-time, and later parts are added by the
 *          user at run-time.  (The linked-list structure is needed to allow
 *          for that.)  We can initialize the known start of the vocabulary
 *          easily, except for the link-pointers, as an array.
 *      We can either explicitly state an index for each entry's link-pointer
 *          to the previous entry (which can become a problem to maintain) or
 *          have a function to initialize the links dynamically, at run-time.
 *      I think I will (regretfully, resignedly) choose the latter.
 *          
 *      We will define a few general-purpose functions for dealing with
 *          T. I. C. -type vocabularies.  Eventually, it might be a good
 *          idea to convert all the vocabularies to this structure...
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *
 *      Revision History:
 *          Mon, 19 Dec 2005 by David L. Paktor
 *          Begin converting most, if not all, of the vocabularies to
 *              T. I. C. -type structure.
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Functions Exported:
 *          init_tic_vocab        Initialize a TIC_HDR -type vocabulary
 *          add_tic_entry         Add an entry to a TIC_HDR -type vocabulary
 *          lookup_tic_entry      Look for a name in a TIC_HDR -type vocabulary
 *          handle_tic_vocab      Perform a function in a TIC_HDR -type vocab
 *          exists_in_tic_vocab   Confirm whether a given name exists in a
 *                                    TIC_HDR -type vocabulary
 *          create_tic_alias      Duplicate the behavior of one name with
 *                                     another name.  Return a "success" flag.
 *          reset_tic_vocab       Reset a given TIC_HDR -type vocabulary to
 *                                    its "Built-In" position.
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *          Global Variables Exported
 *              tic_found       The entry, in a TIC_HDR -type vocabulary,
 *                                  that has just been found and is being 
 *                                  "handled".  Needed for protection against
 *                                  recursion in a User-defined Macro (which
 *                                  should occur only rarely).
 *
 **************************************************************************** */


#include <stdlib.h>
#include <string.h>

#include "ticvocab.h"
#include "errhandler.h"
#include "tracesyms.h"
#include "scanner.h"
#include "devnode.h"
#include "vocabfuncts.h"

tic_hdr_t *tic_found;

/* **************************************************************************
 *
 *      Function name:  init_tic_vocab
 *      Synopsis:       Dynamically initialize the link-pointers
 *                          of the.given TIC_HDR -type vocabulary
 *
 *      Inputs:
 *         Parameters:
 *             tic_vocab_tbl       Pointer to the initial TIC_HDR vocab array
 *             max_indx            Maximum Index of the initial array.
 *             tic_vocab_ptr       Pointer to the vocab "tail"
 *
 *      Outputs:
 *         Returned Value:         None
 *         Global Variables:
 *              The link-fields of the initial TIC_HDR vocab array entries
 *                  will be filled in.
 *         Supplied Pointers:
 *             *tic_vocab_ptr       Points to the last element in the array
 *
 *      Process Explanation:
 *          The value that  tic_vocab_ptr  has upon entry to the routine
 *              (which may point to the end of another array which is to
 *              precede this one in the voacbulary) gets entered into
 *              the link-pointer field of the first element of the array.
 *          For this reason, it is important that all TIC_HDR vocabulary
 *              pointers that will be passed to this routine have their
 *              initial values explicitly declared NULL. 
 *          If the user has asked to Trace any built-in name, the support
 *              routine will set its  tracing  field and dispay a message.
 *
 **************************************************************************** */

void init_tic_vocab( tic_hdr_t *tic_vocab_tbl,
                         int max_indx,
			     tic_hdr_t **tic_vocab_ptr)
{
    int indx;
    for ( indx = 0 ; indx < max_indx ; indx++ )
    {
        tic_vocab_tbl[indx].next = *tic_vocab_ptr;
	*tic_vocab_ptr = &tic_vocab_tbl[indx];
	trace_builtin( &tic_vocab_tbl[indx]);
    }
}


/* **************************************************************************
 *
 *      Function name:  make_tic_entry
 *      Synopsis:       Construct a new entry to a TIC_HDR -type vocab-list
 *                          but do not add it to a vocabulary
 *
 *      Inputs:
 *         Parameters:
 *             tname         Pointer to space containing the name of the entry
 *             tfunct        Pointer to the routine the new entry will call
 *             tparam        The "parameter field" value (may be a pointer)
 *             fw_defr       FWord Token of the entry's Definer
 *             pfldsiz       Size of "param field" (if a pointer to alloc'd mem)
 *             is_single     TRUE if entry is a single-token FCode
 *             ign_fnc       Pointer to "ignoring" routine for new entry
 *             trace_this    TRUE if new entry is to be "Traced"
 *             tic_vocab     Address of the variable that holds the latest
 *                               pointer to the "tail" of the T.I.C.-type
 *                               vocab-list to which we are adding.
 *
 *      Outputs:
 *         Returned Value:   Pointer to the new entry
 *         Memory Allocated:
 *             For the new entry.
 *         When Freed?
 *             When reset_tic_vocab() is applied to the same vocab-list.
 *
 *      Error Detection:
 *          Failure to allocate memory is a Fatal Error.
 *
 *      Process Explanation:
 *          The name pointer is presumed to already point to a stable,
 *              newly-allocated memory-space.  If the parameter field is
 *              actually a pointer, it, too, is presumed to already have
 *              been allocated.
 *          Memory will be allocated for the entry itself and the given
 *              data will be placed into its fields.
 *
 *      Extraneous Remarks:
 *          This is a retro-fit; it's a factor of the add_tic_entry()
 *              routine, whose functionality has been expanded to include
 *              issuing the Trace-Note and Duplication Warning messages.
 *              Having it separate allows it to be called (internally) by
 *              create_split_alias(), which has special requirements for
 *              its call to trace_creation() 
 *
 **************************************************************************** */

static tic_hdr_t *make_tic_entry( char *tname,
                        void (*tfunct)(),
                             TIC_P_DEFLT_TYPE tparam,
                                 fwtoken fw_defr,
				     int pfldsiz,
                                         bool is_single,
                                         void (*ign_fnc)(),
                                               bool trace_this,
                                             tic_hdr_t **tic_vocab )
{
    tic_hdr_t *new_entry;

    new_entry = safe_malloc(sizeof(tic_hdr_t), "adding tic_entry");
    new_entry->name              =  tname;
    new_entry->next              = *tic_vocab;
    new_entry->funct             =  tfunct;
    new_entry->pfield.deflt_elem =  tparam;
    new_entry->fword_defr        =  fw_defr;
    new_entry->is_token          =  is_single;
    new_entry->ign_func          =  ign_fnc;
    new_entry->pfld_size         =  pfldsiz;
    new_entry->tracing           =  trace_this;

    return( new_entry);
}


/* **************************************************************************
 *
 *      Function name:  add_tic_entry
 *      Synopsis:       Add an entry to the given TIC_HDR -type vocabulary;
 *                          issue the Creation Tracing and Duplicate-Name
 *                          messages as applicable.
 *
 *      Inputs:
 *         Parameters:
 *             tname             Pointer to space containing entry's new name
 *             tfunct            Pointer to the routine the new entry will call
 *             tparam            The "parameter field" value (may be a pointer)
 *             fw_defr           FWord Token of the entry's Definer
 *             pfldsiz           Size of "param field" (if a ptr to alloc'd mem)
 *             is_single         TRUE if entry is a single-token FCode
 *             ign_fnc           Pointer to "ignoring" routine for new entry
 *          NOTE:  No  trace_this  param here; it's only in make_tic_entry()
 *             tic_vocab         Address of the variable that holds the latest
 *                                   pointer to the "tail" of the T.I.C.-type
 *                                   vocab-list to which we are adding.
 *         Global Variables:
 *             scope_is_global   TRUE if "global" scope is in effect
 *                                  (passed to "Trace-Creation" message)
 *
 *      Outputs:
 *         Returned Value:       NONE
 *         Supplied Pointers:
 *             *tic_vocab        Will point to new entry
 *         Printout:
 *             "Trace-Creation" message
 *
 *      Error Detection:
 *          Warning on duplicate name (subject to command-line control)
 *
 *      Process Explanation:
 *          The entry itself will be created by the  make_tic_entry()  routine.
 *          This routine will test whether the new name is to be Traced,
 *              and will pass that datum to the  make_tic_entry()  routine.
 *          If the new name is to be Traced, issue a Creation Tracing message.
 *              (We want it to appear first).  Use the new entry.
 *          Because this routine will not be called for creating aliases, the
 *              second param to  trace_creation()  is NULL.
 *          Do the duplicate-name check _before_ linking the new entry in to
 *              the given vocabulary.  We don't want the duplicate-name test
 *              to find the name in the new entry, only in pre-existing ones...
 *          Now we're ready to update the given pointer-to-the-tail-of-the-
 *              -vocabulary to point to the new entry.
 *
 **************************************************************************** */

void add_tic_entry( char *tname,
                        void (*tfunct)(),
                             TIC_P_DEFLT_TYPE tparam,
                                 fwtoken fw_defr,
				     int pfldsiz,
                                         bool is_single,
					   void (*ign_fnc)(),
					       tic_hdr_t **tic_vocab )
{
    bool trace_this = is_on_trace_list( tname);
    tic_hdr_t *new_entry = make_tic_entry( tname,
			       tfunct,
			           tparam,
				       fw_defr, pfldsiz,
				           is_single,
					       ign_fnc,
						   trace_this,
						       tic_vocab );

    if ( trace_this )
    {
	trace_creation( new_entry, NULL, scope_is_global);
    }
    warn_if_duplicate( tname);
    *tic_vocab = new_entry;

}

/* **************************************************************************
 *
 *      Function name:  lookup_tic_entry
 *      Synopsis:       Look for a name in the given TIC_HDR -type vocabulary
 *
 *      Inputs:
 *         Parameters:
 *             tname                The "target" name for which to look
 *             tic_vocab            Pointer to the T. I. C. -type vocabulary
 *                                      in which to search
 *
 *      Outputs:
 *         Returned Value:          Pointer to the relevant entry, or
 *                                      NULL if name not found.
 *
 *      Extraneous Remarks:
 *          We don't set the global  tic_found  here because this routine
 *              is not always called when the found function is going to
 *              be executed; sometimes it is called for error-detection;
 *              for instance, duplicate-name checking...
 *
 **************************************************************************** */
 
tic_hdr_t *lookup_tic_entry( char *tname, tic_hdr_t *tic_vocab )
{
    tic_hdr_t *curr ;

    for (curr = tic_vocab ; curr != NULL ; curr=curr->next)
    {
        if ( strcasecmp(tname, curr->name) == 0 )
	{
	    break;
	}
    }

    return ( curr ) ;
}

/* **************************************************************************
 *
 *      Function name:  exists_in_tic_vocab
 *      Synopsis:       Confirm whether the given name exists in the
 *                      given TIC_HDR -type vocabulary
 *
 *      Inputs:
 *         Parameters:
 *             tname                The name for which to look
 *             tic_vocab            Pointer to the T. I. C. -type vocabulary
 *
 *      Outputs:
 *         Returned Value:          TRUE if name is found,
 *
 **************************************************************************** */

bool exists_in_tic_vocab( char *tname, tic_hdr_t *tic_vocab )
{
    tic_hdr_t *found ;
    bool retval = FALSE;

    found = lookup_tic_entry( tname, tic_vocab );
    if ( found != NULL )
    {
	retval = TRUE;
    }

    return ( retval );
}


/* **************************************************************************
 *
 *      Function name:  create_split_alias
 *      Synopsis:       Create an Alias in one TIC_HDR -type vocabulary
 *                          for a word in (optionally) another vocabulary.
 *                          Return a "success" flag.
 *                          This is the work-horse for  create_tic_alias()
 *
 *      Associated FORTH word:                 ALIAS
 *
 *      Inputs:
 *         Parameters:
 *             old_name             Name of existing entry
 *             new_name             New name for which to create an entry
 *             *src_vocab           Pointer to the "tail" of the "Source"
 *                                      TIC_HDR -type vocab-list 
 *             *dest_vocab          Pointer to the "tail" of the "Destination"
 *                                      TIC_HDR -type vocab-list 
 *
 *      Outputs:
 *         Returned Value:          TRUE if  old_name  found in "Source" vocab
 *         Supplied Pointers:
 *             *dest_vocab          Will be updated to point to the new entry
 *         Memory Allocated:
 *             For the new entry, by the support routine.
 *         When Freed?
 *             When reset_tic_vocab() is applied to "Destination" vocab-list.
 *         Printout:
 *             "Trace-Creation" message
 *
 *      Error Detection:
 *          Warning on duplicate name (subject to command-line control)
 *
 *      Process Explanation:
 *          Both the "old" and "new" names are presumed to already point to
 *              stable, freshly allocated memory-spaces.
 *          Even if the "old" entry's  pfld_size  is not zero, meaning its
 *              param-field is a pointer to allocated memory, we still do
 *              not need to copy it into a freshly allocated memory-space,
 *              as long as we make the new alias-entry's  pfld_size  zero:
 *              the reference to the old space will work, and the old
 *              entry's param-field memory space will not be freed with
 *              the alias-entry but only with the "old" entry.
 *          We will do both the "Creation-Tracing" announcement and the
 *              Duplicate Name Warning here.  "Tracing" happens either if
 *              the entry for the old name has its  tracing  flag set, or
 *              if the new name is on the trace-list.  The "source" vocab
 *              and the "dest" vocab can only be different when the "old"
 *              name has defined Global scope.  We will pass that along
 *              to the  trace_creation()  routine.
 *          We're doing the "Tracing" and Duplicate-Name functions because
 *              we're applying the "Tracing" message to the "old" name's
 *              entry.  Because of this, we must call  make_tic_entry()  to
 *              bypass  add_tic_entry(), which now does its own "Tracing"
 *              and Duplicate-Name functions on the new entry.
 *
 **************************************************************************** */

bool create_split_alias( char *new_name, char *old_name,
                              tic_hdr_t **src_vocab, tic_hdr_t **dest_vocab )
{
    tic_hdr_t *found ;
    bool retval = FALSE;

    found = lookup_tic_entry( old_name, *src_vocab );
    if ( found != NULL )
    {
	bool trace_it = found->tracing;
	if ( ! trace_it )
	{
	    trace_it = is_on_trace_list( new_name);
	}
	if ( trace_it )
	{
	    bool old_is_global = BOOLVAL( src_vocab != dest_vocab );
	    trace_creation( found, new_name, old_is_global);
	}
	warn_if_duplicate( new_name);

	*dest_vocab = make_tic_entry( new_name,
			   found->funct,
			   found->pfield.deflt_elem,
				   found->fword_defr, 0,
			               found->is_token,
					   found->ign_func,
					       trace_it,
						   dest_vocab );
	retval = TRUE;
    }

    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  create_tic_alias
 *      Synopsis:       Create an Alias in a TIC_HDR -type vocabulary
 *                          Return a "success" flag.
 *
 *      Associated FORTH word:                 ALIAS
 *
 *      Inputs:
 *         Parameters:
 *             old_name             Name of existing entry
 *             new_name             New name for which to create an entry
 *             *tic_vocab           Pointer to the "tail" of the
 *                                      T. I. C. -type vocab-list 
 *
 *      Outputs:
 *         Returned Value:          TRUE if  old_name  found in given vocab
 *         Supplied Pointers:
 *             *tic_vocab           Will be updated to point to the new entry
 *         Memory Allocated:
 *             For the new entry, by the support routine.
 *         When Freed?
 *             When reset_tic_vocab() is applied to the same vocab-list.
 *
 *      Process Explanation:
 *          The given vocabulary is both the "Source" and the "Destination".
 *              Pass them both to  create_split_alias.

 **************************************************************************** */

bool create_tic_alias( char *new_name, char *old_name, tic_hdr_t **tic_vocab )
{
    return ( create_split_alias( new_name, old_name, tic_vocab, tic_vocab ) );
}


/* **************************************************************************
 *
 *      Function name:  handle_tic_vocab
 *      Synopsis:       Perform the function associated with the given name
 *                      in the given TIC_HDR -type vocabulary
 *
 *      Inputs:
 *         Parameters:
 *             tname                The "target" name for which to look
 *             tic_vocab            Pointer to the T. I. C. -type vocabulary
 *
 *      Outputs:
 *         Returned Value:   TRUE if the given name is valid in the given vocab
 *         Global Variables:
 *             tic_found            Points to the TIC entry of the "target"
 *                                      name, if it was found; else, NULL.
 *         Global Behavior:
 *             Whatever the associated function does...
 *
 *      Process Explanation:
 *          Find the name and execute its associated function.
 *          If the name is not in the given vocabulary, return
 *              an indication; leave it to the calling routine
 *              to decide how to proceed.
 *
 **************************************************************************** */
 
bool handle_tic_vocab( char *tname, tic_hdr_t *tic_vocab )
{
    bool retval = FALSE;
    
    tic_found = lookup_tic_entry( tname, tic_vocab );
    if ( tic_found != NULL )
    {
        tic_found->funct( tic_found->pfield);
	retval = TRUE;
    }

    return ( retval ) ;
}

/* **************************************************************************
 *
 *      Function name:  reset_tic_vocab
 *      Synopsis:       Reset a given TIC_HDR -type vocabulary to
 *                          its given "Built-In" position.
 *
 *      Inputs:
 *         Parameters:
 *             *tic_vocab            Pointer to the T. I. C.-type vocab-list
 *             reset_position        Position to which to reset the list
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Supplied Pointers:
 *             *tic_vocab          Reset to given "Built-In" position.
 *         Memory Freed
 *             All memory allocated by user-definitions will be freed
 *
 *      Process Explanation:
 *          The "stable memory-spaces" to which the name and parameter
 *              field pointers point are presumed to have been acquired
 *              by allocation of memory, which is reasonable for entries
 *              created by the user as opposed to the built-in entries,
 *              which we are, in any case, not releasing.
 *          The parameter-field size field tells us whether we need to
 *              free()  the parameter-field pointer.
 *
 **************************************************************************** */

void reset_tic_vocab( tic_hdr_t **tic_vocab, tic_hdr_t *reset_position )
{
    tic_hdr_t *next_t;

    next_t = *tic_vocab;
    while ( next_t != reset_position  )
    {
	next_t = (*tic_vocab)->next ;

	free( (*tic_vocab)->name );
	if ( (*tic_vocab)->pfld_size != 0 )
	{
	    free( (*tic_vocab)->pfield.chr_ptr );
	}
	free( *tic_vocab );
	*tic_vocab = next_t ;
    }
}
