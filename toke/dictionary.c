/*
 *                     OpenBIOS - free your system!
 *                         ( FCode tokenizer )
 *
 *  dictionary.c - dictionary initialization and functions.
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
 *         Modifications made in 2005 by IBM Corporation
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Modifications Author:  David L. Paktor    dlpaktor@us.ibm.com
 **************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#if defined(__linux__) && ! defined(__USE_BSD)
#define __USE_BSD
#endif
#include <string.h>
#include <errno.h>

#include "emit.h"
#include "macros.h"
#include "scanner.h"
#include "ticvocab.h"
#include "dictionary.h"
#include "vocabfuncts.h"
#include "devnode.h"
#include "clflags.h"
#include "parselocals.h"
#include "errhandler.h"
#include "tokzesc.h"
#include "conditl.h"
#include "tracesyms.h"

/* **************************************************************************
 *
 *      Revision History:
 *          Updated Fri, 29 Jul 2005 by David L. Paktor
 *          Retrofit to handle "Current Device" as a separate vocabulary;
 *              if one is in effect, searches through it occur first, as
 *              do definitions to it, ahead of the general vocabulary.  This
 *              is to support managing device-node vocabularies correctly.
 *          Updated Mon, 12 Dec 2005 by David L. Paktor
 *          Allow the user to specify a group of exceptions, words whose
 *              scope will be "global" within the tokenization.  Under "global"
 *              scope, definitions will be made to the "core" vocabulary.
 *
 *          Wed, 14 Dec 2005 by David L. Paktor
 *          Found a problem with the current approach.  Need to be able to
 *              temporarily suspend meaning of "instance".  Define:  (1) an
 *              alias for  INSTANCE  called  GENERIC_INSTANCE  (2) a macro
 *              called  INSTANCE  that effectively no-ops it out; and, when
 *              it is time to restore "INSTANCE" to normal functionality,
 *              (3) an alias for  GENERIC_INSTANCE  called  INSTANCE .
 *          Problem is that macros are treated as a separate vocabulary
 *              from FWords (and their aliases) and searching one before the
 *              other (either way) renders the second one searched unable to
 *              supercede the first one:  If macros are searched first, (2)
 *              will be found ahead of the built-in FWord (which is what we
 *              want) but later, when we search for (3) among the FWords, it
 *              will not be found ahead of (2).  If, on the other hand, we
 *              search FWords first, the macro defined in (2) will never be
 *              found.
 *          We need a way to define both (all?) types of definitions in a
 *              single vocabulary that will honor the LIFO order of def'ns.
 *
 *          Mon, 19 Dec 2005 by David L. Paktor
 *          Begin development of implementation of a way to define both (all?)
 *              types of definitions in a single  tic_hdr_t  type vocabulary.
 *
 *          Wed, 04 Oct 2006 by David L. Paktor
 *          Issue a message when a name on the trace list is invoked (as well
 *              as when it is created), but keep a limit on the speed penalty.
 *              (I.e., don't scan the trace-list for every symbol invoked.)
 *              We will scan the trace-list for every pre-defined symbol during
 *              initialization of the built-in vocabularies' lists, but that
 *              occurs only once...
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *          Global Variables Exported
 *
 *     scope_is_global      Indication that "global" scope is in effect
 *
 *     define_token        Normally TRUE, but if the definition in progress
 *               occurs inside a control-structure, (which is an Error), we
 *               make this FALSE.  We will allow the definition to proceed
 *               (in order to avoid "cascade" errors and so that other errors
 *               can be recognized normally) but we will use this to suppress
 *               adding the new definition's token to the vocab.  We also use
 *               this to suppress the actions of "hide_..." and "reveal..."
 *               because if the token wasn't added to the vocabulary, there's
 *               nothing to find that needed to be "hidden"...
 *
 **************************************************************************** */

bool scope_is_global = FALSE;
bool define_token = TRUE;      /*    TRUE = Normal definition process;
                                *        FALSE when definition is an Error.
                                *        We enter definition state anyway,
                                *            but must still suppress:
                                *        (1) adding an entry for the token
                                *            to the vocab,
                                *        (2) "hiding" it at first, and
                                *        (3) "revealing" it later.
                                *  
                                *    Makes for more "normal" error- detection...
                                */

/* **************************************************************************
 *
 *      We will be creating several different lists of initial built-in
 *          definitions; together, they constitute the Global Vocabulary.
 *          (We will avoid the term "dictionary", since, in classical
 *          Forth terminology, it refers to the complete collection of
 *          vocabularies in an application.)  The usage of the pointer
 *          to the Global Vocabulary is central to the operation of this
 *          program and the maintenance programmer needs to understand it.
 *          We may also refer to the Global Vocabulary as the "core" vocab.
 *
 *      Each initial list will be created as an array of TIC-header entries.
 *          Because the global vocabulary is expandable by the user,
 *          we will not be searching the lists as arrays but rather as
 *          linked-lists; the run-time initialization routine will fill
 *          in their link-fields and also will link the various lists
 *          together, so we can group their initial declarations according
 *          to our convenience.
 *
 *      A single pointer, called the "Global Vocabulary Dictionary Pointer"
 *          (okay, so classical Forth terminology isn't completely rigorous...)
 *          and abbreviated GV_DP, will point to the "tail" of the "thread".
 *          Similar vocabularies will be created for the device-nodes; look
 *          in the file  devnode.fth  for a more detailed discussion of those.
 *
 *      The "FC-Tokens" list contains the names and FCode numeric tokens
 *          of the straightforward FORTH words that simply write a token
 *          directly to the tokenized output.  We need to access these
 *          without being confused by aliases or other distractions, so
 *          we will keep a pointer to them especially for that purpose.
 *      Therefore it is IMPORTANT:  that the "FC-Tokens" list MUST be the
 *          first table linked by the initialization routine, so that its
 *          last-searched entry's link-field is NULL.
 *
 *      The "FWords" list contains FORTH words that require additional
 *          special action at tokenization-time.  Their numeric values
 *          are derived from the  fword_token  enumeration declaration,
 *          and are used as the control-expression for a SWITCH statement
 *          with a large number of CASE labels in the  handle_internal()
 *          function.
 *
 *      The "Shared Words" list contains FORTH words that can be executed
 *          similarly both during "Tokenizer Escape" mode (i.e., the scope
 *          of the special brackets:  tokenizer[  ...   ]tokenizer ) and
 *          also within "Normal Tokenization" mode.  Their numeric values
 *          are derived and used the same way as the "FWords".  Since we
 *          will be needing to do a separate search through them at times,
 *          we will also need a lower-bracket pointer for them.  (An upper
 *          bracket is irrelevant for these, because aliases can be added.
 *          This is not the case for the "FC-Tokens" list, because searches
 *          through those will be conducted from within this program.)
 *
 *      The "definer" field in the TIC-header structure is primarily used to
 *          detect attempts to apply the  TO  directive to an inappropriate
 *          target.  Its numeric values are a subset of the "FWord tokens".
 *          Certain "FC-Token" names are specified to be valid  TO  targets;
 *          their entries' "definer" fields will be initialized accordingly.
 *          Entries in FWord Token lists that are "shared" between "Normal
 *          Tokenization" and "Tokenizer Escape" modes will have their
 *          "definer" fields initialized to  COMMON_FWORD .  All other
 *          entries' "definer" fields will be initialized to  UNSPECIFIED .
 *
 *      Other files maintain and support additional lists with the same
 *          structure, which need to be linked together with the lists
 *          declared here.  We prefer to keep the  GV_DP  private to this
 *          file, so it will be passed as a parameter where needed.  (I'm
 *          not pleased to note, however, that it can't be kept completely
 *          private; it's needed for add_user_macro() and possibly other
 *          functions outside this file.)
 *
 *      The words that can only be used during "Tokenizer Escape" mode and
 *          the IBM-style "Locals", as well as the device-node vocabularies,
 *          will need to be separate and will not be linked together with
 *          the Global Vocabulary.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *          Internal Static Variables
 *
 *      We'll be initializing the lists later, but will be referencing
 *          the pointers sooner, so we need to declare the pointers here.
 *
 *      We will keep all of these pointers private to this file.
 *
 **************************************************************************** */

static tic_hdr_t *global_voc_dict_ptr  = NULL;  /*  The Global Vocabulary    */
static tic_hdr_t *fc_tokens_list_ender = NULL;  /*  Tokens search ends here  */
static tic_hdr_t *fc_tokens_list_start = NULL;  /*  Start the search here    */
static tic_hdr_t *shared_fwords_ender  = NULL;  /*  Shared FWords search end */
static tic_hdr_t *global_voc_reset_ptr = NULL;  /*  Reset-point for G.V.     */


/* **************************************************************************
 *
 *      Function name:  lookup_core_word
 *      Synopsis:       Return a pointer to the data-structure of the named
 *                      word in the "Global" Vocabulary
 *
 *      Inputs:
 *         Parameters:
 *             tname                     The name to look up
 *         Local Static Variables:
 *             global_voc_dict_ptr       "Tail" of Global Vocabulary
 *
 *      Outputs:
 *         Returned Value:                Pointer to the data-structure, or
 *                                            NULL if not found.
 *
 **************************************************************************** */

tic_hdr_t *lookup_core_word( char *tname)
{
    tic_hdr_t *found ;

    found = lookup_tic_entry( tname, global_voc_dict_ptr);
    return ( found ) ;
}

/* **************************************************************************
 *
 *      Function name:  create_core_alias
 *      Synopsis:       Create, in the "Global" ("core") Vocabulary, an entry
 *                          for NEW_NAME that behaves the same as the latest
 *                          definition of OLD_NAME, and whose behavior will
 *                          not change even if a new definition of OLD_NAME
 *                          is overlaid.  Indicate if successful.
 *
 *      Inputs:
 *         Parameters:
 *             new_name          The name for the new alias to be created
 *             old_name          The name of the old function to be duplicated
 *         Local Static Variables:
 *             global_voc_dict_ptr        "Tail" of Global Vocabulary
 *
 *      Outputs:
 *         Returned Value:                TRUE if OLD_NAME was found.
 *         Local Static Variables:
 *             global_voc_dict_ptr        Updated with the new entry
 *         Memory Allocated
 *             By support routine.
 *
 *      Process Explanation:
 *          Both the "old" and "new" names are presumed to already point to
 *              stable, freshly allocated memory-spaces.
 *
 **************************************************************************** */

bool create_core_alias( char *new_name, char *old_name)
{
    bool retval = create_tic_alias( new_name, old_name, &global_voc_dict_ptr);
    return ( retval );
}

/* **************************************************************************
 *
 *      The functions that go into the various lists' FUNCT field may be
 *           defined below, or might be defined externally.
 *
 *      Often, we will need a function that merely recasts the type of the
 *           parameter field before passing it to the function that does
 *           the actual work.
 *
 *      Prologs will be brief or even non-existent.
 *
 *      Initialization macro definitions will accompany the functions.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      For the "FCode-Tokens" list, simply generate the token directly.
 *      We need this layer for param type conversion.
 *      In case we're ever able to eliminate it, (or just on General
 *          Principles) we'll invoke it via a macro...
 *
 **************************************************************************** */

static void emit_fc_token( tic_param_t pfield)
{
    u16 fc_tok = (u16)pfield.deflt_elem;
    emit_fcode( fc_tok);
}

#define FC_TOKEN_FUNC  emit_fc_token

#define BUILTIN_FCODE( tok, nam)   \
     VALPARAM_TIC(nam, FC_TOKEN_FUNC, tok , UNSPECIFIED, TRUE )

/*  Built-in FCodes with known definers:  */
#define BI_FCODE_VALUE( tok, nam)   \
     VALPARAM_TIC(nam, FC_TOKEN_FUNC, tok , VALUE, TRUE )

#define BI_FCODE_VRBLE( tok, nam)   \
     VALPARAM_TIC(nam, FC_TOKEN_FUNC, tok , VARIABLE, TRUE )

#define BI_FCODE_DEFER( tok, nam)   \
     VALPARAM_TIC(nam, FC_TOKEN_FUNC, tok , DEFER, TRUE )

#define BI_FCODE_CONST( tok, nam)   \
     VALPARAM_TIC(nam, FC_TOKEN_FUNC, tok , CONST, TRUE )

/* **************************************************************************
 *
 *      The "FCode-Tokens" list includes tokens that are identified
 *         in the Standard as Obsolete.  We will define a function
 *         that issues a WARNING before generating the token, and
 *         assign it to those elements of the list.
 *
 *      Control the message via a command-line flag.
 *
 **************************************************************************** */

static void obsolete_warning( void)
{
    if ( obso_fcode_warning )
    {
	tokenization_error( WARNING, "%s is an Obsolete FCode.\n",
	    strupr(statbuf) );
    }
}

static void obsolete_fc_token( tic_param_t pfield)
{
    obsolete_warning();
    emit_fc_token( pfield);
}

#define OBSO_FC_FUNC  obsolete_fc_token

#define OBSOLETE_FCODE( tok, nam)   \
     VALPARAM_TIC(nam, OBSO_FC_FUNC, tok , UNSPECIFIED, TRUE )

#define OBSOLETE_VALUE( tok, nam)   \
     VALPARAM_TIC(nam, OBSO_FC_FUNC, tok , VALUE, TRUE )


/* **************************************************************************
 *
 *      The function for most of the "FWords" list,  handle_internal() ,
 *          is defined externally, but not exported in a  .h  file,
 *          because we want to keep it as private as possible.
 *      We will declare its prototype here.
 *
 *      Initialization macros for both "Normal Mode"-only and
 *          "Shared" entries are also defined here.
 *
 *   Arguments:
 *       fwt      (fword_token)    Value of the FWord Token (from Enum list)
 *       nam      (string)         Name of the entry as seen in the source
 *
 **************************************************************************** */

void handle_internal( tic_param_t pfield);
/*  "Skip-a-string when Ignoring" function.  Same args and limited-proto ...  */
void skip_string( tic_param_t pfield);

#define FWORD_EXEC_FUNC  handle_internal

#define BUILTIN_FWORD( fwt, nam)   \
     FWORD_TKN_TIC(nam, FWORD_EXEC_FUNC, fwt, BI_FWRD_DEFN )

#define SHARED_FWORD( fwt, nam)   \
     FWORD_TKN_TIC(nam, FWORD_EXEC_FUNC, fwt, COMMON_FWORD )

/*  Variants:  When Ignoring, SKip One Word  */
#define SHR_FWD_SKOW( fwt, nam)   \
     DUALFUNC_FWT_TIC(nam, FWORD_EXEC_FUNC, fwt, skip_a_word, COMMON_FWORD )

/*  Variants:  When Ignoring, SKip one Word in line  */
#define SH_FW_SK_WIL( fwt, nam)   \
     DUALFUNC_FWT_TIC(nam, FWORD_EXEC_FUNC, fwt,     \
         skip_a_word_in_line, COMMON_FWORD )

/*  When Ignoring, SKip Two Words in line  */
#define SH_FW_SK2WIL( fwt, nam)   \
     DUALFUNC_FWT_TIC(nam, FWORD_EXEC_FUNC, fwt,     \
         skip_two_words_in_line, COMMON_FWORD )

/* **************************************************************************
 *
 *      Some of the entries in the "FWords" list -- both "Normal" (Built-in)
 *          and "Shared" also act as an "Ignore-handler".
 *          
 *   Arguments:
 *       nam      (string)         Name of the entry as seen in the source
 *       afunc    (routine-name)   Name of internal "active" function
 *       pval     (integer)        The "param field" item
 *       ifunc    (routine-name)   Name of "ignore-handling" function
 *
 **************************************************************************** */

#define SHARED_IG_HDLR(nam, afunc, pval, ifunc)     \
    DUFNC_FWT_PARM(nam, afunc, pval, ifunc, COMMON_FWORD )

/*  A "Shared" entry that uses the same routine for both of its functions  */
#define SHR_SAMIG_FWRD( fwt, nam)   \
    DUFNC_FWT_PARM(nam, FWORD_EXEC_FUNC, fwt, FWORD_EXEC_FUNC, COMMON_FWORD )

/* **************************************************************************
 *
 *     But the "Normal" (Built-in) FWord Ignore-handler uses the same
 *          routine as the  BUILTIN_FWORD for both of its functions.
 *
 *   Arguments:
 *       fwt      (fword_token)    Value of the FWord Token (from Enum list)
 *       nam      (string)         Name of the entry as seen in the source
 *
 **************************************************************************** */
#define BI_IG_FW_HDLR( fwt, nam)   \
    DUALFUNC_FWT_TIC(nam, FWORD_EXEC_FUNC, fwt, FWORD_EXEC_FUNC, BI_FWRD_DEFN )

/*  A variant:  A "Built-In FWorD that SKiPs One Word", when Ignoring  */
#define BI_FWD_SKP_OW( fwt, nam)   \
     DUALFUNC_FWT_TIC(nam, FWORD_EXEC_FUNC, fwt, skip_a_word, BI_FWRD_DEFN )

/*  Another variant:  A "Built-In FWorD String".  skip_string when Ignoring  */
#define BI_FWD_STRING( fwt, nam)   \
     DUALFUNC_FWT_TIC(nam, FWORD_EXEC_FUNC, fwt, skip_string, BI_FWRD_DEFN )

/* **************************************************************************
 *
 *      In order to protect device-nodes' methods from being accessed 
 *          by other device-nodes (with the attendant potential for
 *          disastrous consequences), we must establish a few rules:
 *
 *      Each device-node has a separate vocabulary for its methods.
 *          New definitions are made to the "current" device's vocabulary.
 *          Searches for names go through the "current" device-node's
 *              vocabulary first, then through the core dictionary.
 *
 *      A new-device (in interpretation-mode) creates a new device-node
 *          vocabulary.  The node that had been current (presumably its
 *          parent) remains in memory but inactive.
 *
 *      A finish-device (again, only in interpretation-mode) removes the
 *          current device-node's vocabulary from memory; its presumed
 *          parent once again becomes current.
 *
 *      Tokenization starts with an implicit "new-device" in effect.
 *          The top-level device-node is never removed.
 *
 *      The Global Variable  current_definitions  points to the vocabulary
 *          to which we will add and through which we will search first.
 *
 *      If "global" scope is in effect, then  current_definitions  will
 *           point to the "Global" (also called "core") vocabulary.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Support for operations in "current" -- i.e., "global" vis-a-vis
 *          "device" -- scope.
 *      "Global" scope will not recognize words defined in "device" scope,
 *           but "device" scope will recognize "global" words.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions to enter "global" scope and resume "device" scope.
 *
 **************************************************************************** */

static tic_hdr_t **save_device_definitions;

void enter_global_scope( void )
{
    if ( scope_is_global )
{
	tokenization_error( WARNING,
	    "%s -- Global Scope already in effect; ignoring.\n",
		strupr(statbuf) );
    }else{
	tokenization_error( INFO,
	    "Initiating Global Scope definitions.\n" );
	scope_is_global = TRUE;
	save_device_definitions = current_definitions;
	current_definitions = &global_voc_dict_ptr;
    }
}

void resume_device_scope( void )
{
    if ( scope_is_global )
    {
	tokenization_error( INFO,
	    "Terminating Global Scope definitions; "
		"resuming Device-node definitions.\n" );
	current_definitions = save_device_definitions;
	scope_is_global = FALSE;
    }else{
	tokenization_error( WARNING,
	    "%s -- Device-node Scope already in effect; ignoring.\n",
		strupr(statbuf) );
    }

}

/* **************************************************************************
 *
 *      Function name:  lookup_current
 *      Synopsis:       Return a pointer to the data-structure of the named
 *                          word, either in the Current Device-Node vocab,
 *                          or in the Global ("core") Vocabulary.
 *
 *      Inputs:
 *         Parameters:
 *             tname                     The name to look for
 *         Global Variables:
 *             current_definitions       Current vocabulary:  Device-Node, or
 *                                           "core" if "global" scope in effect.
 *             scope_is_global           TRUE if "global" scope is in effect
 *         Local Static Variables:
 *             global_voc_dict_ptr       "Tail" of Global Vocabulary
 *
 *      Outputs:
 *         Returned Value:               Pointer to the data-structure, or
 *                                           NULL if not found.
 *
 *      Process Explanation:
 *          If a current Device-Node Vocabulary in effect, search through it.
 *          If the given name was not found, and "global" scope is not in
 *              effect (i.e., "core" was not already searched), make a
 *              separate search through the Global ("core") Vocabulary
 *
 *      Extraneous Remarks:
 *          This is the central routine for doing general word-searches that
 *              make use of the "normal"-mode search-list.
 *
 **************************************************************************** */

tic_hdr_t *lookup_current( char *tname)
{
    /*  Search Current Device Vocabulary ahead of global (core) vocabulary  */
    tic_hdr_t *retval;
    retval = lookup_tic_entry( tname, *current_definitions);
    if ( (retval == NULL) && INVERSE(scope_is_global) )
{
	retval = lookup_core_word( tname);
    }
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  exists_in_current
 *      Synopsis:       Confirm whether the given name exists either
 *                          in the Current Device-Node vocab,
 *                          or in the Global ("core") Vocabulary,
 *                          or in Tokenizer Escape Mode, if that's current.
 *
 *      Inputs:
 *         Parameters:
 *             tname                     The name to look for
 *
 *      Outputs:
 *         Returned Value:               TRUE if name is found
 *
 **************************************************************************** */

bool exists_in_current( char *tname)
{
    tic_hdr_t *found = lookup_word( tname, NULL, NULL);
    bool retval = BOOLVAL ( found != NULL);
    return( retval);
	}

/* **************************************************************************
 *
 *      Function name:  lookup_in_dev_node
 *      Synopsis:       Return a pointer to the data-structure of the
 *                          named word in the Current device node, if
 *                          in "Device" scope.  Used for error-reporting.
 *
 *      Inputs:
 *         Parameters:
 *             tname                     The name to look for
 *         Global Variables:
 *             current_definitions        Device-Node (or Global) Vocabulary
 *                                            currently in effect.
 *             scope_is_global            FALSE if "Device" scope is in effect
 *
 *      Outputs:
 *         Returned Value:                Pointer to the data-structure, or
 *                                            NULL if not found.
 *
 **************************************************************************** */

tic_hdr_t *lookup_in_dev_node( char *tname)
{
    tic_hdr_t *retval = NULL;

    if ( INVERSE(scope_is_global) )
{
	retval = lookup_tic_entry( tname, *current_definitions);
}
    return ( retval );
}


/* **************************************************************************
 *
 *     In order to avoid unintentional "recursive"-ness, we need a way
 *          to render a newly created colon-definition non-findable
 *          until it's completed.
 *
 *      We will accomplish this by saving and reverting the pointer to
 *          the newest entry, when we call the  hide_last_colon() , and
 *          by restoring the pointer when we call  reveal_last_colon()
 *
 *      We need, therefore, to save the pointer to the last entry before
 *          we create the new entry.
 *
 **************************************************************************** */

/*  Update this each time a new definition is entered  */
static tic_hdr_t *save_current = NULL;

/* **************************************************************************
 *
 *      Function name:  add_to_current
 *      Synopsis:       Add a new entry to the "current" scope of definitions,
 *                          which may be either the Global Vocabulary or the
 *                          current Device-Node Vocabulary.
 *
 *      Inputs:
 *         Parameters:
 *             name                      The name of the new entry
 *             fc_token                  The new entry's assigned FCode-number
 *             fw_definer                The new entry's definer
 *         Global Variables:
 *             current_definitions       Pointer to pointer to "tail" of the
 *                                           Vocabulary currently in effect;
 *                                           either Device-node or Global.
 *             define_token              TRUE = Normal definition process;
 *                                           FALSE if def'n is an Error.
 *                                           Suppress adding entry to vocab;
 *                                           Display "failure" Trace-note
 *                                           and Duplicate-Name Warning.
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Global Variables:
 *             *current_definitions    Updated with the new entry
 *         Local Static Variables:
 *             save_current            Saved state of  current_definitions
 *                                         before the new entry is added,
 *                                         to permit "hide" and "reveal".
 *         Memory Allocated
 *             For the new entry's copy of the name.
 *         When Freed?
 *             When the Device-Node is "finish"ed or the Global Vocabulary
 *                 is reset, or when the program exits.
 *
 *      Process Explanation:
 *          Because  current_definitions  points to the Global Vocabulary
 *              pointer during "global" scope, this routine is extremely
 *              straightforward.
 *          All user-defined words have the same action, i.e., emitting
 *              the assigned FCode-number.  The new entry's "parameter
 *              field" size is, of course, zero; the "ignore-function"
 *              is NULL, and the entry has a single-token FCode number.
 *
 *      Extraneous Remarks:
 *          The  define_token  input is a late addition, necessitated by
 *              the decision to continue processing after an erroneous
 *              attempt to create a definition inside a control-structure,
 *              in order to catch other errors. 
 *            
 **************************************************************************** */

void add_to_current( char *name,
                           TIC_P_DEFLT_TYPE fc_token,
			       fwtoken definer)
{
    if ( define_token )
{
	char *nu_name = strdup( name);

	save_current = *current_definitions;
	add_tic_entry( nu_name, FC_TOKEN_FUNC, fc_token,
			   definer, 0 , TRUE , NULL, current_definitions );
    }else{
	trace_create_failure( name, NULL, fc_token);
	warn_if_duplicate( name);
    }
}


void hide_last_colon ( void )
{
    if ( define_token )
    {
    tic_hdr_t *temp_vocab;

    /*  The  add_to_current()  function will have been called before this
     *      one when a new colon-definition is created, so  save_current
     *      will have been set to point to the entry that had been made
     *      just before the newest one, which we are hiding here.
     */

    temp_vocab = save_current ;
    save_current = *current_definitions;
    *current_definitions = temp_vocab;
    }

}

void reveal_last_colon ( void )
{
    if ( define_token )
    {
    /*  We call this function either when the colon-definition is
     *      completed, or when "recursive"-ness is intentional.
     */
    *current_definitions = save_current ;
}
}


/* **************************************************************************
 *
 *      Function name:  create_current_alias
 *      Synopsis:       Create an alias for OLD_NAME, called NEW_NAME, in
 *                          the "current" scope of definitions, which may
 *                          be either the Global ("core") Vocabulary or the
 *                          current Device-Node Vocabulary.  Indicate if
 *                          successful (i.e., OLD_NAME was valid).
 *                      This is actually a little trickier than it may at
 *                          first appear; read the Rules in the Process
 *                          Explanation for full details...
 *
 *      Inputs:
 *         Parameters:
 *             new_name          The name for the new alias to be created
 *             old_name          The name of the old function to be duplicated
 *         Global Variables:
 *             current_definitions        Device-node vocabulary currently
 *                                            in effect.
 *             scope_is_global            TRUE if "global" scope is in effect
 *             split_alias_message        Message-type for announcement that
 *                                            the "new" name was created in
 *                                            a different vocabulary than where
 *                                            the "old" name was found, if so
 *                                            be.  (See "Rule 3", below).  An
 *                                            Advisory, normally, but if either
 *                                            of the names is being Traced, the
 *                                            create_split_alias() routine will
 *                                            change it to a Trace-Note.
 *         Local Static Variables:
 *             global_voc_dict_ptr        "Tail" of Global Vocabulary
 *
 *      Outputs:
 *         Returned Value:                TRUE if OLD_NAME was found.
 *         Global Variables:
 *             *current_definitions      Updated with the new entry 
 *         Memory Allocated
 *             By support routine.
 *         When Freed?
 *             When RESET-SYMBOLS is issued (if "global" scope is in effect)
 *                 or when the device-node is "finish"ed.
 *         Printout:
 *             Advisory message if Rule 3 (see below) is invoked.
 *
 *      Process Explanation:
 *          Both the "old" and "new" names are presumed to already point to
 *              stable, freshly allocated memory-spaces.
 *          Rules:
 *          (1)
 *          If "global" scope is in effect, and the "old" name is found in
 *              the Global Vocabulary, then the "new" name will be created
 *              in the Global Vocabulary.
 *          (2)
 *          Similarly, if "device" scope is in effect, and the "old" name is
 *              found in the current device-node's vocabulary, the "new" name
 *              will be created in the current device-node's vocabulary.
 *          (3)
 *          BUT!:  If "device" scope is in effect, and the "old" name is found
 *              in the Global Vocabulary, then the "new" name will be created
 *              in the current device-node's vocabulary.  It will only be
 *              recognized in the scope of that device-node, and will be
 *              removed from memory when the device-node is "finish"ed.
 *          And, yes, it *is* supposed to work that way...   ;-)
 *
 *          Again, because  current_definitions  points to the Global Vocab
 *              pointer during "global" scope, the first two rules of this
 *              routine are extremely straightforward; it's Rule 3 that you
 *              have to watch out for...  ;-)
 *
 *          And one other thing:
 *              We will always make the alias's  pfld_size  zero.  See the
 *              prolog for  create_split_alias()  in  ticvocab.c  for details...
 *
 *      Extraneous Remarks:
 *          I tried stretching the rules of well-structured code, but
 *              I'm finding that there is a _good_reason_ for them...
 *
 **************************************************************************** */

bool create_current_alias( char *new_name, char *old_name )
{
    bool retval = FALSE;
    bool split_alias = FALSE;

    /*  Rules 1 & 2 are implemented in the same code.  */
    if ( create_tic_alias( new_name, old_name, current_definitions) )
    {
	 retval = TRUE;
    }else{
    if ( INVERSE(scope_is_global) )
    {
	    /*  Rule 3.
	     *  Because the vocab into which the new definition will go is
	     *      not the same as the one in which the old name was found,
	     *      we cannot call  create_tic_alias  but must replicate it.
	     */
	    /*  Hmmmmmm.....
	     *  We could get around that by refactoring:  add a parameter,
	     *      make the vocab to search separate from the one in which to
	     *      create.  Also, by making it a separate routine, we won't
	     *      have to disturb the other callers of  create_tic_alias()
	     *  Yes!  Excellent!  Make it so!
	     */
	    split_alias = TRUE;
	    split_alias_message = INFO;
	    retval = create_split_alias(
			 new_name, old_name,
			     &global_voc_dict_ptr,
					   current_definitions );
	}
    }
    
    if ( retval )
	    {
	if ( split_alias )
	{
	    tokenization_error( split_alias_message,
		   "%s is a Global definition, but its alias, %s, "
		       "will only be defined %s",
			   strupr( old_name), new_name,
			       in_what_node( current_device_node) );
		show_node_start();
	    }
	}

    return ( retval );
}

/* **************************************************************************
 *
 *      Support functions specific to the lists will be defined
 *          after the lists are created.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *     Create the initial list (or "Table") of FCode-Tokens.
 *
 *     Most Standard FCode tokens are not specified as to their definition
 *         type, but a few have a definer specified as either a VALUE, a
 *         VARIABLE or a DEFER; we will enter them with the appropriate macro.
 *
 **************************************************************************** */

static tic_hdr_t tokens_table[] =
{
	BUILTIN_FCODE( 0x000, "end0" ) ,
	BUILTIN_FCODE( 0x010, "b(lit)" ) ,
	BUILTIN_FCODE( 0x011, "b(')" ) ,
	BUILTIN_FCODE( 0x012, "b(\")" ) ,
	BUILTIN_FCODE( 0x013, "bbranch" ) ,
	BUILTIN_FCODE( 0x014, "b?branch" ) ,
	BUILTIN_FCODE( 0x015, "b(loop)" ) ,
	BUILTIN_FCODE( 0x016, "b(+loop)" ) ,
	BUILTIN_FCODE( 0x017, "b(do)" ) ,
	BUILTIN_FCODE( 0x018, "b(?do)" ) ,
	BUILTIN_FCODE( 0x019, "i" ) ,
	BUILTIN_FCODE( 0x01a, "j" ) ,
	BUILTIN_FCODE( 0x01b, "b(leave)" ) ,
	BUILTIN_FCODE( 0x01c, "b(of)" ) ,
	BUILTIN_FCODE( 0x01d, "execute" ) ,
	BUILTIN_FCODE( 0x01e, "+" ) ,
	BUILTIN_FCODE( 0x01f, "-" ) ,
	BUILTIN_FCODE( 0x020, "*" ) ,
	BUILTIN_FCODE( 0x021, "/" ) ,
	BUILTIN_FCODE( 0x022, "mod" ) ,
	BUILTIN_FCODE( 0x023, "and" ) ,
	BUILTIN_FCODE( 0x024, "or" ) ,
	BUILTIN_FCODE( 0x025, "xor" ) ,
	BUILTIN_FCODE( 0x026, "invert" ) ,
	BUILTIN_FCODE( 0x026, "not" ) ,           /*  Synonym for "invert" */
	BUILTIN_FCODE( 0x027, "lshift" ) ,
	BUILTIN_FCODE( 0x027, "<<" ) ,            /*  Synonym for "lshift" */
	BUILTIN_FCODE( 0x028, "rshift" ) ,
	BUILTIN_FCODE( 0x028, ">>" ) ,            /*  Synonym for "rshift" */
	BUILTIN_FCODE( 0x029, ">>a" ) ,
	BUILTIN_FCODE( 0x02a, "/mod" ) ,
	BUILTIN_FCODE( 0x02b, "u/mod" ) ,
	BUILTIN_FCODE( 0x02c, "negate" ) ,
	BUILTIN_FCODE( 0x02d, "abs" ) ,
	BUILTIN_FCODE( 0x02e, "min" ) ,
	BUILTIN_FCODE( 0x02f, "max" ) ,
	BUILTIN_FCODE( 0x030, ">r" ) ,
	BUILTIN_FCODE( 0x031, "r>" ) ,
	BUILTIN_FCODE( 0x032, "r@" ) ,
	BUILTIN_FCODE( 0x033, "exit" ) ,
	BUILTIN_FCODE( 0x034, "0=" ) ,
	BUILTIN_FCODE( 0x035, "0<>" ) ,
	BUILTIN_FCODE( 0x036, "0<" ) ,
	BUILTIN_FCODE( 0x037, "0<=" ) ,
	BUILTIN_FCODE( 0x038, "0>" ) ,
	BUILTIN_FCODE( 0x039, "0>=" ) ,
	BUILTIN_FCODE( 0x03a, "<" ) ,
	BUILTIN_FCODE( 0x03b, ">" ) ,
	BUILTIN_FCODE( 0x03c, "=" ) ,
	BUILTIN_FCODE( 0x03d, "<>" ) ,
	BUILTIN_FCODE( 0x03e, "u>" ) ,
	BUILTIN_FCODE( 0x03f, "u<=" ) ,
	BUILTIN_FCODE( 0x040, "u<" ) ,
	BUILTIN_FCODE( 0x041, "u>=" ) ,
	BUILTIN_FCODE( 0x042, ">=" ) ,
	BUILTIN_FCODE( 0x043, "<=" ) ,
	BUILTIN_FCODE( 0x044, "between" ) ,
	BUILTIN_FCODE( 0x045, "within" ) ,
	BUILTIN_FCODE( 0x046, "drop" ) ,
	BUILTIN_FCODE( 0x047, "dup" ) ,
	BUILTIN_FCODE( 0x048, "over" ) ,
	BUILTIN_FCODE( 0x049, "swap" ) ,
	BUILTIN_FCODE( 0x04A, "rot" ) ,
	BUILTIN_FCODE( 0x04b, "-rot" ) ,
	BUILTIN_FCODE( 0x04c, "tuck" ) ,
	BUILTIN_FCODE( 0x04d, "nip" ) ,
	BUILTIN_FCODE( 0x04e, "pick" ) ,
	BUILTIN_FCODE( 0x04f, "roll" ) ,
	BUILTIN_FCODE( 0x050, "?dup" ) ,
	BUILTIN_FCODE( 0x051, "depth" ) ,
	BUILTIN_FCODE( 0x052, "2drop" ) ,
	BUILTIN_FCODE( 0x053, "2dup" ) ,
	BUILTIN_FCODE( 0x054, "2over" ) ,
	BUILTIN_FCODE( 0x055, "2swap" ) ,
	BUILTIN_FCODE( 0x056, "2rot" ) ,
	BUILTIN_FCODE( 0x057, "2/" ) ,
	BUILTIN_FCODE( 0x058, "u2/" ) ,
	BUILTIN_FCODE( 0x059, "2*" ) ,
	BUILTIN_FCODE( 0x05a, "/c" ) ,
	BUILTIN_FCODE( 0x05b, "/w" ) ,
	BUILTIN_FCODE( 0x05c, "/l" ) ,
	BUILTIN_FCODE( 0x05d, "/n" ) ,
	BUILTIN_FCODE( 0x05e, "ca+" ) ,
	BUILTIN_FCODE( 0x05f, "wa+" ) ,
	BUILTIN_FCODE( 0x060, "la+" ) ,
	BUILTIN_FCODE( 0x061, "na+" ) ,
	BUILTIN_FCODE( 0x062, "char+" ) ,
	BUILTIN_FCODE( 0x062, "ca1+" ) ,          /*  Synonym for char+" */
	BUILTIN_FCODE( 0x063, "wa1+" ) ,
	BUILTIN_FCODE( 0x064, "la1+" ) ,
	BUILTIN_FCODE( 0x065, "cell+" ) ,
	BUILTIN_FCODE( 0x065, "na1+" ) ,          /*  Synonym for "cell+" */
	BUILTIN_FCODE( 0x066, "chars" ) ,
	BUILTIN_FCODE( 0x066, "/c*" ) ,           /*  Synonym for "chars" */
	BUILTIN_FCODE( 0x067, "/w*" ) ,
	BUILTIN_FCODE( 0x068, "/l*" ) ,
	BUILTIN_FCODE( 0x069, "cells" ) ,
	BUILTIN_FCODE( 0x069, "/n*" ) ,           /*  Synonym for "cells" */
	BUILTIN_FCODE( 0x06a, "on" ) ,
	BUILTIN_FCODE( 0x06b, "off" ) ,
	BUILTIN_FCODE( 0x06c, "+!" ) ,
	BUILTIN_FCODE( 0x06d, "@" ) ,
	BUILTIN_FCODE( 0x06e, "l@" ) ,
	BUILTIN_FCODE( 0x06f, "w@" ) ,
	BUILTIN_FCODE( 0x070, "<w@" ) ,
	BUILTIN_FCODE( 0x071, "c@" ) ,
	BUILTIN_FCODE( 0x072, "!" ) ,
	BUILTIN_FCODE( 0x073, "l!" ) ,
	BUILTIN_FCODE( 0x074, "w!" ) ,
	BUILTIN_FCODE( 0x075, "c!" ) ,
	BUILTIN_FCODE( 0x076, "2@" ) ,
	BUILTIN_FCODE( 0x077, "2!" ) ,
	BUILTIN_FCODE( 0x078, "move" ) ,
	BUILTIN_FCODE( 0x079, "fill" ) ,
	BUILTIN_FCODE( 0x07a, "comp" ) ,
	BUILTIN_FCODE( 0x07b, "noop" ) ,
	BUILTIN_FCODE( 0x07c, "lwsplit" ) ,
	BUILTIN_FCODE( 0x07d, "wljoin" ) ,
	BUILTIN_FCODE( 0x07e, "lbsplit" ) ,
	BUILTIN_FCODE( 0x07f, "bljoin" ) ,
	BUILTIN_FCODE( 0x080, "wbflip" ) ,
	BUILTIN_FCODE( 0x080, "flip" ) ,   /*  Synonym for "wbflip"  */
	BUILTIN_FCODE( 0x081, "upc" ) ,
	BUILTIN_FCODE( 0x082, "lcc" ) ,
	BUILTIN_FCODE( 0x083, "pack" ) ,
	BUILTIN_FCODE( 0x084, "count" ) ,
	BUILTIN_FCODE( 0x085, "body>" ) ,
	BUILTIN_FCODE( 0x086, ">body" ) ,
	BUILTIN_FCODE( 0x087, "fcode-revision" ) ,
	BUILTIN_FCODE( 0x087, "version" ) , /*  Synonym for "fcode-revision" */
	BI_FCODE_VRBLE( 0x088, "span" ) ,
	BUILTIN_FCODE( 0x089, "unloop" ) ,
	BUILTIN_FCODE( 0x08a, "expect" ) ,
	BUILTIN_FCODE( 0x08b, "alloc-mem" ) ,
	BUILTIN_FCODE( 0x08c, "free-mem" ) ,
	BUILTIN_FCODE( 0x08d, "key?" ) ,
	BUILTIN_FCODE( 0x08e, "key" ) ,
	BUILTIN_FCODE( 0x08f, "emit" ) ,
	BUILTIN_FCODE( 0x090, "type" ) ,
	BUILTIN_FCODE( 0x091, "(cr" ) ,
	BUILTIN_FCODE( 0x092, "cr" ) ,
	BI_FCODE_VRBLE( 0x093, "#out" ) ,
	BI_FCODE_VRBLE( 0x094, "#line" ) ,
	BUILTIN_FCODE( 0x095, "hold" ) ,
	BUILTIN_FCODE( 0x096, "<#" ) ,
	BUILTIN_FCODE( 0x097, "u#>" ) ,
	BUILTIN_FCODE( 0x098, "sign" ) ,
	BUILTIN_FCODE( 0x099, "u#" ) ,
	BUILTIN_FCODE( 0x09a, "u#s" ) ,
	BUILTIN_FCODE( 0x09b, "u." ) ,
	BUILTIN_FCODE( 0x09c, "u.r" ) ,
	BUILTIN_FCODE( 0x09d, "." ) ,
	BUILTIN_FCODE( 0x09e, ".r" ) ,
	BUILTIN_FCODE( 0x09f, ".s" ) ,
	BI_FCODE_VRBLE( 0x0a0, "base" ) ,
	OBSOLETE_FCODE( 0x0a1, "convert" ) ,
	BUILTIN_FCODE( 0x0a2, "$number" ) ,
	BUILTIN_FCODE( 0x0a3, "digit" ) ,
	BI_FCODE_CONST( 0x0a4, "-1" ) ,
	BI_FCODE_CONST( 0x0a4, "true" ) ,          /*  Synonym for "-1" */
	BI_FCODE_CONST( 0x0a5, "0" ) ,
	BI_FCODE_CONST( 0x0a5, "false" ) ,         /*  Synonym for "0" */
	BI_FCODE_CONST( 0x0a5, "struct" ) ,        /*  Synonym for "0" */
	BI_FCODE_CONST( 0x0a6, "1" ) ,
	BI_FCODE_CONST( 0x0a7, "2" ) ,
	BI_FCODE_CONST( 0x0a8, "3" ) ,
	BI_FCODE_CONST( 0x0a9, "bl" ) ,
	BI_FCODE_CONST( 0x0aa, "bs" ) ,
	BI_FCODE_CONST( 0x0ab, "bell" ) ,
	BUILTIN_FCODE( 0x0ac, "bounds" ) ,
	BUILTIN_FCODE( 0x0ad, "here" ) ,
	BUILTIN_FCODE( 0x0ae, "aligned" ) ,
	BUILTIN_FCODE( 0x0af, "wbsplit" ) ,
	BUILTIN_FCODE( 0x0b0, "bwjoin" ) ,
	BUILTIN_FCODE( 0x0b1, "b(<mark)" ) ,
	BUILTIN_FCODE( 0x0b2, "b(>resolve)" ) ,
	OBSOLETE_FCODE( 0x0b3, "set-token-table" ) ,
	OBSOLETE_FCODE( 0x0b4, "set-table" ) ,
	BUILTIN_FCODE( 0x0b5, "new-token" ) ,
	BUILTIN_FCODE( 0x0b6, "named-token" ) ,
	BUILTIN_FCODE( 0x0b7, "b(:)" ) ,
	BUILTIN_FCODE( 0x0b8, "b(value)" ) ,
	BUILTIN_FCODE( 0x0b9, "b(variable)" ) ,
	BUILTIN_FCODE( 0x0ba, "b(constant)" ) ,
	BUILTIN_FCODE( 0x0bb, "b(create)" ) ,
	BUILTIN_FCODE( 0x0bc, "b(defer)" ) ,
	BUILTIN_FCODE( 0x0bd, "b(buffer:)" ) ,
	BUILTIN_FCODE( 0x0be, "b(field)" ) ,
	OBSOLETE_FCODE( 0x0bf, "b(code)" ) ,
	BUILTIN_FCODE( 0x0c0, "instance" ) ,
	BUILTIN_FCODE( 0x0c2, "b(;)" ) ,
	BUILTIN_FCODE( 0x0c3, "b(to)" ) ,
	BUILTIN_FCODE( 0x0c4, "b(case)" ) ,
	BUILTIN_FCODE( 0x0c5, "b(endcase)" ) ,
	BUILTIN_FCODE( 0x0c6, "b(endof)" ) ,
	BUILTIN_FCODE( 0x0c7, "#" ) ,
	BUILTIN_FCODE( 0x0c8, "#s" ) ,
	BUILTIN_FCODE( 0x0c9, "#>" ) ,
	BUILTIN_FCODE( 0x0ca, "external-token" ) ,
	BUILTIN_FCODE( 0x0cb, "$find" ) ,
	BUILTIN_FCODE( 0x0cc, "offset16" ) ,
	BUILTIN_FCODE( 0x0cd, "evaluate" ) ,
	BUILTIN_FCODE( 0x0cd, "eval" ) ,   /*  Synonym for "evaluate"  */
	BUILTIN_FCODE( 0x0d0, "c," ) ,
	BUILTIN_FCODE( 0x0d1, "w," ) ,
	BUILTIN_FCODE( 0x0d2, "l," ) ,
	BUILTIN_FCODE( 0x0d3, "," ) ,
	BUILTIN_FCODE( 0x0d4, "um*" ) ,
	BUILTIN_FCODE( 0x0d4, "u*x" ) ,        /*  Synonym for "um*" */
	BUILTIN_FCODE( 0x0d5, "um/mod" ) ,
	BUILTIN_FCODE( 0x0d5, "xu/mod" ) ,   /*  Synonym for "um/mod"  */
	BUILTIN_FCODE( 0x0d8, "d+" ) ,
	BUILTIN_FCODE( 0x0d8, "x+" ) ,   /*  Synonym for "d+"  */
	BUILTIN_FCODE( 0x0d9, "d-" ) ,
	BUILTIN_FCODE( 0x0d9, "x-" ) ,   /*  Synonym for "d-"  */
	BUILTIN_FCODE( 0x0da, "get-token" ) ,
	BUILTIN_FCODE( 0x0db, "set-token" ) ,
	BI_FCODE_VRBLE( 0x0dc, "state" ) ,
	BUILTIN_FCODE( 0x0dd, "compile," ) ,
	BUILTIN_FCODE( 0x0de, "behavior" ) ,
	BUILTIN_FCODE( 0x0f0, "start0" ) ,
	BUILTIN_FCODE( 0x0f1, "start1" ) ,
	BUILTIN_FCODE( 0x0f2, "start2" ) ,
	BUILTIN_FCODE( 0x0f3, "start4" ) ,
	BUILTIN_FCODE( 0x0fc, "ferror" ) ,
	BUILTIN_FCODE( 0x0fd, "version1" ) ,
	OBSOLETE_FCODE( 0x0fe, "4-byte-id" ) ,
	BUILTIN_FCODE( 0x0ff, "end1" ) ,
	OBSOLETE_FCODE( 0x101, "dma-alloc" ) ,
	BUILTIN_FCODE( 0x102, "my-address" ) ,
	BUILTIN_FCODE( 0x103, "my-space" ) ,
	OBSOLETE_FCODE( 0x104, "memmap" ) ,
	BUILTIN_FCODE( 0x105, "free-virtual" ) ,
	OBSOLETE_FCODE( 0x106, ">physical" ) ,
	OBSOLETE_FCODE( 0x10f, "my-params" ) ,
	BUILTIN_FCODE( 0x110, "property" ) ,
	BUILTIN_FCODE( 0x110, "attribute" ) ,    /*  Synonym for "property"  */
	BUILTIN_FCODE( 0x111, "encode-int" ) ,
	BUILTIN_FCODE( 0x111, "xdrint" ) ,     /*  Synonym for "encode-int"  */
	BUILTIN_FCODE( 0x112, "encode+" ) ,
	BUILTIN_FCODE( 0x112, "xdr+" ) ,          /*  Synonym for "encode+"  */
	BUILTIN_FCODE( 0x113, "encode-phys" ) ,
	BUILTIN_FCODE( 0x113, "xdrphys" ) ,   /*  Synonym for "encode-phys"  */
	BUILTIN_FCODE( 0x114, "encode-string" ) ,
	BUILTIN_FCODE( 0x114, "xdrstring" ) , /* Synonym for "encode-string" */
	BUILTIN_FCODE( 0x115, "encode-bytes" ) ,
	BUILTIN_FCODE( 0x115, "xdrbytes" ) ,  /*  Synonym for "encode-bytes" */
	BUILTIN_FCODE( 0x116, "reg" ) ,
	OBSOLETE_FCODE( 0x117, "intr" ) ,
	OBSOLETE_FCODE( 0x118, "driver" ) ,
	BUILTIN_FCODE( 0x119, "model" ) ,
	BUILTIN_FCODE( 0x11a, "device-type" ) ,
	BUILTIN_FCODE( 0x11b, "parse-2int" ) ,
	BUILTIN_FCODE( 0x11b, "decode-2int" ) , /*  Synonym for "parse-2int" */
	BUILTIN_FCODE( 0x11c, "is-install" ) ,
	BUILTIN_FCODE( 0x11d, "is-remove" ) ,
	BUILTIN_FCODE( 0x11e, "is-selftest" ) ,
	BUILTIN_FCODE( 0x11f, "new-device" ) ,
	BUILTIN_FCODE( 0x120, "diagnostic-mode?" ) ,
	OBSOLETE_FCODE( 0x121, "display-status" ) ,
	BUILTIN_FCODE( 0x122, "memory-test-issue" ) ,
	OBSOLETE_FCODE( 0x123, "group-code" ) ,
	BI_FCODE_VRBLE( 0x124, "mask" ) ,
	BUILTIN_FCODE( 0x125, "get-msecs" ) ,
	BUILTIN_FCODE( 0x126, "ms" ) ,
	BUILTIN_FCODE( 0x127, "finish-device" ) ,
	BUILTIN_FCODE( 0x128, "decode-phys" ) ,
	BUILTIN_FCODE( 0x12b, "interpose" ) ,
	BUILTIN_FCODE( 0x130, "map-low" ) ,
	BUILTIN_FCODE( 0x130, "map-sbus" ) ,   /*  Synonym for "map-low"  */
	BUILTIN_FCODE( 0x131, "sbus-intr>cpu" ) ,
	BI_FCODE_VALUE( 0x150, "#lines" ) ,
	BI_FCODE_VALUE( 0x151, "#columns" ) ,
	BI_FCODE_VALUE( 0x152, "line#" ) ,
	BI_FCODE_VALUE( 0x153, "column#" ) ,
	BI_FCODE_VALUE( 0x154, "inverse?" ) ,
	BI_FCODE_VALUE( 0x155, "inverse-screen?" ) ,
	OBSOLETE_VALUE( 0x156, "frame-buffer-busy?" ) ,
	BI_FCODE_DEFER( 0x157, "draw-character" ) ,
	BI_FCODE_DEFER( 0x158, "reset-screen" ) ,
	BI_FCODE_DEFER( 0x159, "toggle-cursor" ) ,
	BI_FCODE_DEFER( 0x15a, "erase-screen" ) ,
	BI_FCODE_DEFER( 0x15b, "blink-screen" ) ,
	BI_FCODE_DEFER( 0x15c, "invert-screen" ) ,
	BI_FCODE_DEFER( 0x15d, "insert-characters" ) ,
	BI_FCODE_DEFER( 0x15e, "delete-characters" ) ,
	BI_FCODE_DEFER( 0x15f, "insert-lines" ) ,
	BI_FCODE_DEFER( 0x160, "delete-lines" ) ,
	BI_FCODE_DEFER( 0x161, "draw-logo" ) ,
	BI_FCODE_VALUE( 0x162, "frame-buffer-adr" ) ,
	BI_FCODE_VALUE( 0x163, "screen-height" ) ,
	BI_FCODE_VALUE( 0x164, "screen-width" ) ,
	BI_FCODE_VALUE( 0x165, "window-top" ) ,
	BI_FCODE_VALUE( 0x166, "window-left" ) ,
	BUILTIN_FCODE( 0x16a, "default-font" ) ,
	BUILTIN_FCODE( 0x16b, "set-font" ) ,
	BI_FCODE_VALUE( 0x16c, "char-height" ) ,
	BI_FCODE_VALUE( 0x16d, "char-width" ) ,
	BUILTIN_FCODE( 0x16e, ">font" ) ,
	BI_FCODE_VALUE( 0x16f, "fontbytes" ) ,
	OBSOLETE_FCODE( 0x170, "fb1-draw-character" ) ,
	OBSOLETE_FCODE( 0x171, "fb1-reset-screen" ) ,
	OBSOLETE_FCODE( 0x172, "fb1-toggle-cursor" ) ,
	OBSOLETE_FCODE( 0x173, "fb1-erase-screen" ) ,
	OBSOLETE_FCODE( 0x174, "fb1-blink-screen" ) ,
	OBSOLETE_FCODE( 0x175, "fb1-invert-screen" ) ,
	OBSOLETE_FCODE( 0x176, "fb1-insert-characters" ) ,
	OBSOLETE_FCODE( 0x177, "fb1-delete-characters" ) ,
	OBSOLETE_FCODE( 0x178, "fb1-insert-lines" ) ,
	OBSOLETE_FCODE( 0x179, "fb1-delete-lines" ) ,
	OBSOLETE_FCODE( 0x17a, "fb1-draw-logo" ) ,
	OBSOLETE_FCODE( 0x17b, "fb1-install" ) ,
	OBSOLETE_FCODE( 0x17c, "fb1-slide-up" ) ,
	BUILTIN_FCODE( 0x180, "fb8-draw-character" ) ,
	BUILTIN_FCODE( 0x181, "fb8-reset-screen" ) ,
	BUILTIN_FCODE( 0x182, "fb8-toggle-cursor" ) ,
	BUILTIN_FCODE( 0x183, "fb8-erase-screen" ) ,
	BUILTIN_FCODE( 0x184, "fb8-blink-screen" ) ,
	BUILTIN_FCODE( 0x185, "fb8-invert-screen" ) ,
	BUILTIN_FCODE( 0x186, "fb8-insert-characters" ) ,
	BUILTIN_FCODE( 0x187, "fb8-delete-characters" ) ,
	BUILTIN_FCODE( 0x188, "fb8-insert-lines" ) ,
	BUILTIN_FCODE( 0x189, "fb8-delete-lines" ) ,
	BUILTIN_FCODE( 0x18a, "fb8-draw-logo" ) ,
	BUILTIN_FCODE( 0x18b, "fb8-install" ) ,
	OBSOLETE_FCODE( 0x1a0, "return-buffer" ) ,
	OBSOLETE_FCODE( 0x1a1, "xmit-packet" ) ,
	OBSOLETE_FCODE( 0x1a2, "poll-packet" ) ,
	BUILTIN_FCODE( 0x1a4, "mac-address" ) ,
	BUILTIN_FCODE( 0x201, "device-name" ) ,
	BUILTIN_FCODE( 0x201, "name" ) ,   /*  Synonym for "device-name"  */
	BUILTIN_FCODE( 0x202, "my-args" ) ,
	BI_FCODE_VALUE( 0x203, "my-self" ) ,
	BUILTIN_FCODE( 0x204, "find-package" ) ,
	BUILTIN_FCODE( 0x205, "open-package" ) ,
	BUILTIN_FCODE( 0x206, "close-package" ) ,
	BUILTIN_FCODE( 0x207, "find-method" ) ,
	BUILTIN_FCODE( 0x208, "call-package" ) ,
	BUILTIN_FCODE( 0x209, "$call-parent" ) ,
	BUILTIN_FCODE( 0x20a, "my-parent" ) ,
	BUILTIN_FCODE( 0x20b, "ihandle>phandle" ) ,
	BUILTIN_FCODE( 0x20d, "my-unit" ) ,
	BUILTIN_FCODE( 0x20e, "$call-method" ) ,
	BUILTIN_FCODE( 0x20f, "$open-package" ) ,
	OBSOLETE_FCODE( 0x210, "processor-type" ) ,
	OBSOLETE_FCODE( 0x211, "firmware-version" ) ,
	OBSOLETE_FCODE( 0x212, "fcode-version" ) ,
	BUILTIN_FCODE( 0x213, "alarm" ) ,
	BUILTIN_FCODE( 0x214, "(is-user-word)" ) ,
	BUILTIN_FCODE( 0x215, "suspend-fcode" ) ,
	BUILTIN_FCODE( 0x216, "abort" ) ,
	BUILTIN_FCODE( 0x217, "catch" ) ,
	BUILTIN_FCODE( 0x218, "throw" ) ,
	BUILTIN_FCODE( 0x219, "user-abort" ) ,
	BUILTIN_FCODE( 0x21a, "get-my-property" ) ,
	BUILTIN_FCODE( 0x21a, "get-my-attribute" ) ,   /*  Synonym for "get-my-property"  */
	BUILTIN_FCODE( 0x21b, "decode-int" ) ,
	BUILTIN_FCODE( 0x21b, "xdrtoint" ) ,   /*  Synonym for "decode-int"  */
	BUILTIN_FCODE( 0x21c, "decode-string" ) ,
	BUILTIN_FCODE( 0x21c, "xdrtostring" ), /* Synonym for "decode-string" */
	BUILTIN_FCODE( 0x21d, "get-inherited-property" ) ,
	BUILTIN_FCODE( 0x21d, "get-inherited-attribute" ) ,   /*  Synonym for "get-inherited-property"  */
	BUILTIN_FCODE( 0x21e, "delete-property" ) ,
	BUILTIN_FCODE( 0x21e, "delete-attribute" ) ,   /*  Synonym for "delete-property"  */
	BUILTIN_FCODE( 0x21f, "get-package-property" ) ,
	BUILTIN_FCODE( 0x21f, "get-package-attribute" ) ,   /*  Synonym for "get-package-property"  */
	BUILTIN_FCODE( 0x220, "cpeek" ) ,
	BUILTIN_FCODE( 0x221, "wpeek" ) ,
	BUILTIN_FCODE( 0x222, "lpeek" ) ,
	BUILTIN_FCODE( 0x223, "cpoke" ) ,
	BUILTIN_FCODE( 0x224, "wpoke" ) ,
	BUILTIN_FCODE( 0x225, "lpoke" ) ,
	BUILTIN_FCODE( 0x226, "lwflip" ) ,
	BUILTIN_FCODE( 0x227, "lbflip" ) ,
	BUILTIN_FCODE( 0x228, "lbflips" ) ,
	OBSOLETE_FCODE( 0x229, "adr-mask" ) ,
	BUILTIN_FCODE( 0x230, "rb@" ) ,
	BUILTIN_FCODE( 0x231, "rb!" ) ,
	BUILTIN_FCODE( 0x232, "rw@" ) ,
	BUILTIN_FCODE( 0x233, "rw!" ) ,
	BUILTIN_FCODE( 0x234, "rl@" ) ,
	BUILTIN_FCODE( 0x235, "rl!" ) ,
	BUILTIN_FCODE( 0x236, "wbflips" ) ,
	BUILTIN_FCODE( 0x236, "wflips" ) ,   /*  Synonym for "wbflips"  */
	BUILTIN_FCODE( 0x237, "lwflips" ) ,
	BUILTIN_FCODE( 0x237, "lflips" ) ,   /*  Synonym for "lwflips"  */
	OBSOLETE_FCODE( 0x238, "probe" ) ,
	OBSOLETE_FCODE( 0x239, "probe-virtual" ) ,
	BUILTIN_FCODE( 0x23b, "child" ) ,
	BUILTIN_FCODE( 0x23c, "peer" ) ,
	BUILTIN_FCODE( 0x23d, "next-property" ) ,
	BUILTIN_FCODE( 0x23e, "byte-load" ) ,
	BUILTIN_FCODE( 0x23f, "set-args" ) ,
	BUILTIN_FCODE( 0x240, "left-parse-string" ) ,

	/* FCodes from 64bit extension addendum */
	BUILTIN_FCODE( 0x22e, "rx@" ) ,
	BUILTIN_FCODE( 0x22f, "rx!" ) ,
	BUILTIN_FCODE( 0x241, "bxjoin" ) ,
	BUILTIN_FCODE( 0x242, "<l@" ) ,
	BUILTIN_FCODE( 0x243, "lxjoin" ) ,
	BUILTIN_FCODE( 0x244, "wxjoin" ) ,
	BUILTIN_FCODE( 0x245, "x," ) ,
	BUILTIN_FCODE( 0x246, "x@" ) ,
	BUILTIN_FCODE( 0x247, "x!" ) ,
	BUILTIN_FCODE( 0x248, "/x" ) ,
	BUILTIN_FCODE( 0x249, "/x*" ) ,
	BUILTIN_FCODE( 0x24a, "xa+" ) ,
	BUILTIN_FCODE( 0x24b, "xa1+" ) ,
	BUILTIN_FCODE( 0x24c, "xbflip" ) ,
	BUILTIN_FCODE( 0x24d, "xbflips" ) ,
	BUILTIN_FCODE( 0x24e, "xbsplit" ) ,
	BUILTIN_FCODE( 0x24f, "xlflip" ) ,
	BUILTIN_FCODE( 0x250, "xlflips" ) ,
	BUILTIN_FCODE( 0x251, "xlsplit" ) ,
	BUILTIN_FCODE( 0x252, "xwflip" ) ,
	BUILTIN_FCODE( 0x253, "xwflips" ) ,
	BUILTIN_FCODE( 0x254, "xwsplit" )
};

static const int number_of_builtin_tokens =
	 sizeof(tokens_table)/sizeof(tic_hdr_t);

/* **************************************************************************
 *
 *      Support functions specific to the FCode-Tokens list.
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Function name:  emit_token
 *      Synopsis:       Emit the FCode token for the given FCode name.
 *
 *      Inputs:
 *         Parameters:
 *             fc_name                     The name of the FCode
 *         Local Static Variables:
 *             fc_tokens_list_start        "Tail" of the "FC-Tokens" list
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *
 *      Error Detection:
 *          This routine should only be called with hard-coded names from
 *              within the program.  If the given name is not found in
 *              the Built-in Tokens Table, that is a FATAL error.
 *
 *      Process Explanation:
 *          Because the "FCode-Tokens" table was linked first, and the
 *              pointer kept for this purpose, the "FC-Tokens" list can
 *              be a subset of the "core" list, yet, when necessary, can
 *              be searched with the same routines.
 *
 *      Extraneous Remarks:
 *          I will bend the strict rules of well-structured code;
 *              the exception case should never occur.
 *
 **************************************************************************** */

void emit_token( const char *fc_name)
{

    if ( handle_tic_vocab( (char *)fc_name, fc_tokens_list_start) )
    {
        return;
    }

    tokenization_error( FATAL, "Did not recognize FCode name %s", fc_name);
}


/* **************************************************************************
 *
 *      Function name:  lookup_token
 *      Synopsis:       Return a pointer to the data-structure of the named
 *                      word in the "FC-Tokens" list
 *
 *      Inputs:
 *         Parameters:
 *             tname                       The name to look up
 *         Local Static Variables:
 *             fc_tokens_list_start        "Tail" of the "FC-Tokens" list
 *
 *      Outputs:
 *         Returned Value:                Pointer to the data-structure, or
 *                                            NULL if not found.
 *
 **************************************************************************** */

tic_hdr_t *lookup_token( char *tname)
{
    tic_hdr_t *found ;

    found = lookup_tic_entry( tname, fc_tokens_list_start);
    return ( found ) ;
}

/* **************************************************************************
 *
 *      Function name:  entry_is_token
 *      Synopsis:       Indicate whether the supplied pointer to a tic_hdr_t
 *                      data-structure is one for which a single-token FCode
 *                      number is assigned.
 *
 *      Inputs:
 *         Parameters:
 *             test_entry                The entry to test; may be NULL
 *         Local macros:
 *             FC_TOKEN_FUNC             The function associated with
 *                                           most single-token entries.
 *             OBSO_FC_FUNC              The function associated with
 *                                           "obsolete" FCode tokens.
 *
 *      Outputs:
 *         Returned Value:               TRUE if the data-structure is
 *                                           a single-token entry.
 *
 *      Process Explanation:
 *          We cannot rely on the "definer" field to indicate whether
 *              it is a single-token entry, and there too many possible
 *              associated functions to be practical; instead we will
 *              look at the  is_token  flag of the data structure.
 *
 *      Revision History:
 *          Updated Mon, 25 Sep 2006 by David L. Paktor
 *              Previously operated by examining the function associated
 *                  with the entry, accepting the general single-token
 *                  emitting function,  FC_TOKEN_FUNC , and later adding
 *                  the function  OBSO_FC_FUNC , which presents a message
 *                  before emitting.  Now the functions that present a
 *                  message before emitting might be about to proliferate,
 *                  rendering this implementation strategy impractical (i.e.,
 *                  ugly to code and too attention-demanding to maintain)
 *                  Instead, I will introduce a flag into the TIC-entry 
 *                  data-structure and rely on it.
 *          Updated Wed, 11 Oct 2006 by David L. Paktor
 *              Discarded plan to have functions that present a "Trace-Note"
 *                  message prior to performing their other duties (just too
 *                  unwieldly all around) in favor of adding a "tracing" flag
 *                  to the TIC_HDR data-structure.  Could have gone back to
 *                  doing this the other way, but this is so much neater a
 *                  solution that I'm keeping it.
 *
 **************************************************************************** */

bool entry_is_token( tic_hdr_t *test_entry )
{
    bool retval = FALSE;
    if ( test_entry != NULL )
    {
	retval = test_entry->is_token;
    }
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  token_entry_warning
 *      Synopsis:       Issue whatever warnings the given token_entry
 *                      requires.  F['] needs this.
 *      Inputs:
 *         Parameters:
 *             test_entry                The entry to test; may be NULL
 *         Local macro:
 *             OBSO_FC_FUNC              The function associated with
 *                                           "obsolete" entries.
 *      Outputs:
 *         Returned Value:               NONE
 *
 *      Error Detection:
 *          Warnings required by the given token_entry.
 *
 *      Extraneous Remarks:
 *          At present, it's only the "Obsolete" warning.
 *          But this is the place to add others, 
 *              should they become necessary.
 *
 **************************************************************************** */

void token_entry_warning( tic_hdr_t *t_entry)
{
    if ( t_entry->funct == OBSO_FC_FUNC )
    {
	obsolete_warning();
    }
}


/* **************************************************************************
 *
 *      Create the initial "FWords" list.
 *
 **************************************************************************** */

static tic_fwt_hdr_t fwords_list[] = {

	BI_FWD_SKP_OW(COLON,	 	":") ,
	BUILTIN_FWORD(SEMICOLON, 	";") ,
	BI_FWD_SKP_OW(TICK, 		"'") ,
	BUILTIN_FWORD(AGAIN, 		"again") ,
	BI_FWD_SKP_OW(BRACK_TICK,  	 "[']") ,
	BI_FWD_SKP_OW(ASCII,		"ascii") ,
	BUILTIN_FWORD(BEGIN,		"begin") ,
	BI_FWD_SKP_OW(BUFFER,		"buffer:") ,
	BUILTIN_FWORD(CASE,		"case") ,
	BI_FWD_SKP_OW(CONST,		"constant") ,
	BI_FWD_SKP_OW(CONTROL,		"control") ,
	BI_FWD_SKP_OW(CREATE,		"create") ,

	BI_FWD_SKP_OW(DEFER,		"defer") ,
	BUILTIN_FWORD(CDO,		"?do") ,
	BUILTIN_FWORD(DO,		"do") ,
	BUILTIN_FWORD(ELSE,		"else") ,
	BUILTIN_FWORD(ENDCASE,		"endcase") ,
	BUILTIN_FWORD(ENDOF,		"endof") ,
	BUILTIN_FWORD(EXTERNAL, 	"external") ,
	BI_FWD_SKP_OW(FIELD,		"field") ,
	BUILTIN_FWORD(FINISH_DEVICE,	"finish-device" ) ,
	BUILTIN_FWORD(HEADERLESS,	"headerless") ,
	BUILTIN_FWORD(HEADERS,		"headers") ,

	BUILTIN_FWORD(INSTANCE ,	"instance") ,

	BUILTIN_FWORD(IF,		"if") ,
	BUILTIN_FWORD(UNLOOP,		"unloop") ,
	BUILTIN_FWORD(LEAVE,		"leave") ,
	BUILTIN_FWORD(PLUS_LOOP, 	"+loop") ,
	BUILTIN_FWORD(LOOP,		"loop") ,

	BUILTIN_FWORD(OF,		"of") ,
	BUILTIN_FWORD(REPEAT,		"repeat") ,
	BUILTIN_FWORD(THEN,		"then") ,
	BI_FWD_SKP_OW(TO,		"to") ,
	BI_FWD_SKP_OW(IS,		"is") , /*  Deprecated synonym to TO  */
	BUILTIN_FWORD(UNTIL,		"until") ,
	BI_FWD_SKP_OW(VALUE,		"value") ,
	BI_FWD_SKP_OW(VARIABLE,		"variable") ,
	BUILTIN_FWORD(WHILE,		"while") ,
	BUILTIN_FWORD(OFFSET16,		"offset16") ,

	BI_FWD_STRING(STRING,		"\"") ,     /*  XXXXX  */
	BI_FWD_STRING(PSTRING,		".\"") ,    /*  XXXXX  */
	BI_FWD_STRING(PBSTRING,		".(") ,     /*  XXXXX  */
	BI_FWD_STRING(SSTRING,		"s\"") ,    /*  XXXXX  */
	BUILTIN_FWORD(IFILE_NAME,	"[input-file-name]"),
	BUILTIN_FWORD(ILINE_NUM,	"[line-number]"),
	BUILTIN_FWORD(RECURSE,		"recurse") ,
	BUILTIN_FWORD(RECURSIVE,	"recursive") ,
	BUILTIN_FWORD(RET_STK_FETCH,	"r@") ,
	BUILTIN_FWORD(RET_STK_FROM,	"r>") ,
	BUILTIN_FWORD(RET_STK_TO,	">r") ,
	BUILTIN_FWORD(THEN,		"endif" ) ,  /*  Synonym for "then"  */
	BUILTIN_FWORD(NEW_DEVICE,	"new-device" ) ,
	BUILTIN_FWORD(LOOP_I,		"i") ,
	BUILTIN_FWORD(LOOP_J,		"j") ,
	/* version1 is also an fcode word, but it 
	 * needs to trigger some tokenizer internals */
	BUILTIN_FWORD(VERSION1,		"version1") ,
	BUILTIN_FWORD(START0,		"start0") ,
	BUILTIN_FWORD(START1,		"start1") ,
	BUILTIN_FWORD(START2,		"start2") ,
	BUILTIN_FWORD(START4,		"start4") ,
	BUILTIN_FWORD(END0,		"end0") ,
	BUILTIN_FWORD(END1,		"end1") ,
	BUILTIN_FWORD(FCODE_V1,		"fcode-version1") ,
	BUILTIN_FWORD(FCODE_V2,		"fcode-version2") ,
	BUILTIN_FWORD(FCODE_V3,		"fcode-version3") ,
	BUILTIN_FWORD(FCODE_END,	"fcode-end") ,

        /*  Support for IBM-style Locals  */
	BI_FWD_STRING(CURLY_BRACE,	"{") ,
	BI_FWD_STRING(DASH_ARROW,	"->") ,
	BUILTIN_FWORD(EXIT,		"exit") ,


	BUILTIN_FWORD(CHAR,		"char") ,
	BUILTIN_FWORD(CCHAR,		"[char]") ,
	BI_FWD_STRING(ABORTTXT,		"abort\"") ,

	BUILTIN_FWORD(ENCODEFILE,	"encode-file") ,

	BI_IG_FW_HDLR(ESCAPETOK,	"tokenizer[") ,
	BI_IG_FW_HDLR(ESCAPETOK,	"f[") ,       /*  An IBM-ish synonym  */
};

static const int number_of_builtin_fwords =
	 sizeof(fwords_list)/sizeof(tic_hdr_t);

/* **************************************************************************
 *
 *      Create the initial list of "Shared_Words" (words that can
 *          be executed similarly both during normal tokenization,
 *          and also within "Tokenizer Escape Mode").
 *
 **************************************************************************** */

static tic_fwt_hdr_t shared_words_list[] = {
	SHARED_FWORD(FLOAD,		"fload") ,
	/*  As does the "Allow Multi-Line" directive   */
	SHR_SAMIG_FWRD(ALLOW_MULTI_LINE, "multi-line") ,

	SHR_FWD_SKOW( F_BRACK_TICK,	 "f[']") ,

	SH_FW_SK2WIL(ALIAS, 		"alias") ,
	SHARED_FWORD(DECIMAL,		"decimal") ,
	SHARED_FWORD(HEX,		"hex") ,
	SHARED_FWORD(OCTAL,		"octal") ,
	SH_FW_SK_WIL(HEXVAL,		"h#") ,
	SH_FW_SK_WIL(DECVAL,		"d#") ,
	SH_FW_SK_WIL(OCTVAL,		"o#") ,

	SH_FW_SK_WIL(ASC_NUM,		"a#") ,
	SH_FW_SK_WIL(ASC_LEFT_NUM,	"al#") ,

	/* IBM-style extension.  Might be generalizable...  */
	SHARED_FWORD(FLITERAL, 	"fliteral") ,

	/*  Directives to extract the value of a Command-Line symbol */
	SH_FW_SK_WIL(DEFINED,		"[defined]") ,
	SH_FW_SK_WIL(DEFINED,		"#defined") ,
	SH_FW_SK_WIL(DEFINED,		"[#defined]") ,

	/*  Present the current date or time, either as an  */
	/*  in-line string or as a user-generated message.  */
	SHARED_FWORD(FCODE_DATE,	"[fcode-date]") ,
	SHARED_FWORD(FCODE_TIME,	"[fcode-time]") ,

	/*  Current definition under construction, similarly  */
	SHARED_FWORD(FUNC_NAME,	"[function-name]"),

	/*  Synonymous forms of the #ELSE and #THEN operators,
	 *      associated with Conditional-Compilation,
	 *      allowing for various syntax-styles, and for
	 *      expansion by alias.
	 */

	/*  #ELSE  operators */
	SHARED_FWORD(CONDL_ELSE,	"#else") ,
	SHARED_FWORD(CONDL_ELSE,	"[else]") ,
	SHARED_FWORD(CONDL_ELSE,	"[#else]") ,

	/*  #THEN  operators */
	SHARED_FWORD(CONDL_ENDER,	"#then") ,
	SHARED_FWORD(CONDL_ENDER,	"[then]") ,
	SHARED_FWORD(CONDL_ENDER,	"[#then]") ,
	/*   #ENDIF variants for users who favor C-style notation   */
	SHARED_FWORD(CONDL_ENDER,	"#endif") ,
	SHARED_FWORD(CONDL_ENDER,	"[endif]") ,
	SHARED_FWORD(CONDL_ENDER,	"[#endif]") ,


	SHARED_FWORD(OVERLOAD,  "overload" ) ,

	SHARED_FWORD(GLOB_SCOPE , "global-definitions" ) ,
	SHARED_FWORD(DEV_SCOPE , "device-definitions" ) ,

	/*  Directives to change a command-line flag value from source   */
	SH_FW_SK_WIL(CL_FLAG,	"[FLAG]") ,
	SH_FW_SK_WIL(CL_FLAG,	"#FLAG") ,
	SH_FW_SK_WIL(CL_FLAG,	"[#FLAG]") ,

	/*  Directives to force display of a command-line flags' values   */
	SHARED_FWORD(SHOW_CL_FLAGS,	"[FLAGS]") ,
	SHARED_FWORD(SHOW_CL_FLAGS,	"#FLAGS") ,
	SHARED_FWORD(SHOW_CL_FLAGS,	"[#FLAGS]") ,
	SHARED_FWORD(SHOW_CL_FLAGS,	"SHOW-FLAGS") ,

	/*  Directives to save and retrieve the FCode Assignment number  */
	SHARED_FWORD(PUSH_FCODE,	"FCODE-PUSH") ,
	SHARED_FWORD(POP_FCODE, 	"FCODE-POP") ,

	/*  Directive to reset the FCode Assignment number and
	 *      re-initialize FCode Range overlap checking.
	 */
	SHARED_FWORD(RESET_FCODE,	"FCODE-RESET") ,

	/* pci header generation is done differently 
	 * across the available tokenizers. We try to
	 * be compatible to all of them
	 */
	SHARED_FWORD(PCIHDR,	  "pci-header") ,
	SHARED_FWORD(PCIEND,	  "pci-end") ,           /* SUN syntax */
	SHARED_FWORD(PCIEND,	  "pci-header-end") ,    /* Firmworks syntax */
	SHARED_FWORD(PCIREV,	  "pci-revision") ,      /* SUN syntax */
	SHARED_FWORD(PCIREV,	  "pci-code-revision") , /* SUN syntax */
	SHARED_FWORD(PCIREV,	  "set-rev-level") ,     /* Firmworks syntax */
	SHARED_FWORD(NOTLAST,	  "not-last-image") ,
	SHARED_FWORD(NOTLAST,	  "not-last-img") ,      /* Shorthand form  */
	SHARED_FWORD(ISLAST,	  "last-image") ,
	SHARED_FWORD(ISLAST,	  "last-img") ,          /* Shorthand form  */
	SHARED_FWORD(SETLAST,	  "set-last-image") ,
	SHARED_FWORD(SETLAST,	  "set-last-img") ,      /* Shorthand form  */

	SH_FW_SK_WIL(SAVEIMG,	  "save-image") ,
	SH_FW_SK_WIL(SAVEIMG,	  "save-img") ,          /* Shorthand form  */

	SHARED_FWORD(RESETSYMBS,  "reset-symbols") ,

	/*  User-Macro definers    */
	SHARED_IG_HDLR("[MACRO]", add_user_macro,  0 ,  skip_user_macro) ,

	/*  Comments and Remarks   */
	SHARED_IG_HDLR("\\",      process_remark, '\n', process_remark) ,
	SHARED_IG_HDLR("(",       process_remark, ')',  process_remark) ,

	/*  Directives to print or discard a user-generated message */
	SHARED_IG_HDLR("[MESSAGE]",  user_message, '\n', skip_user_message) ,
	SHARED_IG_HDLR("#MESSAGE",   user_message, '\n', skip_user_message) ,
	SHARED_IG_HDLR("[#MESSAGE]", user_message, '\n', skip_user_message) ,
	SHARED_IG_HDLR("#MESSAGE\"", user_message, '"' , skip_user_message) ,
};

static const int number_of_shared_words =
	sizeof(shared_words_list)/sizeof(tic_hdr_t);

/* **************************************************************************
 *
 *      Function name:  lookup_shared_word
 *      Synopsis:       Return a pointer to the data-structure of the named
 *                      word, only if it is a "Shared Word"
 *
 *      Inputs:
 *         Parameters:
 *             tname                     The name to look for
 *         Local Static Variables:
 *             global_voc_dict_ptr       "Tail" of Global Vocabulary
 *
 *      Outputs:
 *         Returned Value:                Pointer to the data-structure, or
 *                                            NULL if not found.
 *
 *      Process Explanation:
 *          The "Shared Words" are scattered among the Global Vocabulary;
 *              the user is allowed to create aliases, which may be in the
 *              Current-Device.  We will search through the "current" scope
 *              and decide whether the name we found is a "Shared Word" by
 *              looking for  COMMON_FWORD  in the "definer" field.
 *
 *      Extraneous Remarks:
 *          This is the only place where an additional check of the
 *              "definer" field is required to identify a desired entry.
 *              Should another "definer"-type be required, I recommend 
 *              defining a general-purpose function in  ticvocab.c  and
 *              applying it here and in the other place(s).
 *
 **************************************************************************** */
 
tic_hdr_t *lookup_shared_word( char *tname)
{
    tic_hdr_t *found ;
    tic_hdr_t *retval = NULL ;

    found = lookup_current( tname );
    if ( found != NULL )
    {
	if ( found->fword_defr == COMMON_FWORD )
	{
	    retval = found ;
	}
    }

    return ( retval );

}

/* **************************************************************************
 *
 *      Function name:  lookup_shared_f_exec_word
 *      Synopsis:       Return a pointer to the data-structure of the named
 *                      word, only if it is a "Shared F-Exec Word"
 *
 *      Inputs:
 *         Parameters:
 *             tname                     The name to look for
 *         Local Static Variables:
 *             global_voc_dict_ptr       "Tail" of Global Vocabulary
 *         Macro Definitions:
 *             FWORD_EXEC_FUNC           The "Active" function of the
 *                                           sub-class of "Shared Word"s
 *                                           that is the object of this
 *                                           routine's search
 *
 *      Outputs:
 *         Returned Value:                Pointer to the data-structure, or
 *                                            NULL if not found.
 *
 *      Process Explanation:
 *          The "Shared F-Exec Words" are the subset of "Shared Words" that
 *              have the  FWORD_EXEC_FUNC  as their "Active" function.
 *
 *      Extraneous Remarks:
 *          This is the only routine that requires a check of two fields;
 *              it seems unlikely that there will be any need to generalize
 *              the core of this routine...
 *
 **************************************************************************** */
 
tic_hdr_t *lookup_shared_f_exec_word( char *tname)
{
    tic_hdr_t *found ;
    tic_hdr_t *retval = NULL ;

    found = lookup_shared_word( tname );
    if ( found != NULL )
    {
	if ( found->funct == FWORD_EXEC_FUNC )
	{
	    retval = found ;
	}
    }

    return ( retval );

}

/* **************************************************************************
 *
 *      Function name:  init_dictionary
 *      Synopsis:       Initialize all the vocabularies.  For the Global
 *                      Vocabulary, fill in the link fields in each of the
 *                      otherwise pre-initialized built-in lists, link the
 *                      lists together, and set the relevant pointers.   For
 *                      other lists, call their initialization routines.
 *
 *      Inputs:
 *         Parameters:                         NONE
 *         Global Variables:
 *         Local Static Variables:
 *             tokens_table                    Base of the "FC-Tokens" list
 *             number_of_builtin_tokens        Number of "FC-Token" entries
 *             fwords_list                     Base of the "FWords" list
 *             number_of_builtin_fwords        Number of "FWord" entries
 *             shared_words_list               Base of the "Shared Words" list
 *             number_of_shared_words          Number of "Shared Words" entries
 *
 *      Outputs:
 *         Returned Value:                      NONE
 *         Local Static Variables:
 *             global_voc_dict_ptr              "Tail" of Global Vocabulary
 *             fc_tokens_list_start             "Tail" of "FC-Tokens" list
 *             fc_tokens_list_ender             End of "FC-Tokens" search
 *             shared_fwords_ender              End of Shared Words" search
 *             global_voc_reset_ptr             Reset-point for Global Vocab
 *
 *      Process Explanation:
 *          The first linked will be the last searched.
 *              Link the "FC-Tokens" first, and mark their limits
 *              Link the "FWords" next,
 *              Mark the end-limit of the "Shared Words", and link them
 *              The "Conditionals", defined in another file, are also "Shared";
 *                  link them next.
 *              Then link the Built-In Macros, also defined in another file.
 *          These constitute the Global Vocabulary.
 *              Mark the reset-point for the Global Vocabulary.
 *          The "Tokenizer Escape" mode vocabulary is not linked to the Global
 *              Vocabulary; call its initialization routine.
 *
 *      Extraneous Remarks:
 *          I only wish I had done this sooner...
 *
 **************************************************************************** */

void init_dictionary( void )
{

    global_voc_dict_ptr  = NULL;   /*  Belt-and-suspenders...  */

    /*  The "FC-Tokens" list must be linked first.  */
    fc_tokens_list_ender = global_voc_dict_ptr;
    init_tic_vocab( tokens_table,
                	number_of_builtin_tokens,
                            &global_voc_dict_ptr ) ;
    fc_tokens_list_start = global_voc_dict_ptr;

    /*  Link the "FWords" next */
   init_tic_vocab( (tic_hdr_t *)fwords_list,
                        number_of_builtin_fwords,
                            &global_voc_dict_ptr ) ;

    /*  Mark the end-limit of the "Shared Words", and link them. */
    shared_fwords_ender = global_voc_dict_ptr;
    init_tic_vocab( (tic_hdr_t *)shared_words_list,
                	number_of_shared_words,
                            &global_voc_dict_ptr ) ;

    /*  Link the "Conditionals" to the Global Vocabulary.  */
    init_conditionals_vocab( &global_voc_dict_ptr ) ;

    /*  Link the Built-In Macros */
    init_macros( &global_voc_dict_ptr ) ;

    /*  Mark the reset-point for the Global Vocabulary.  */
    global_voc_reset_ptr = global_voc_dict_ptr;

    /*   Initialize the "Tokenizer Escape" mode vocabulary  */
    init_tokz_esc_vocab();

    /*   Locals and Device-Node vocabularies are initially empty  */

}

/* **************************************************************************
 *
 *      Function name:  reset_normal_vocabs
 *      Synopsis:       Reset the vocabularies that were created in "Normal"
 *                          (as distinguished from "Tokenizer Escape") mode
 *                          to the proper state for a fresh tokenization.
 *
 *      Associated FORTH words:              END0  END1
 *      Associated Tokenizer directives:     RESET-SYMBOLS  (in "Normal" mode)
 *                                           FCODE-END
 *
 *      Inputs:
 *         Parameters:                NONE
 *         Global Variables:
 *             current_device_node        Vocab struct of current dev-node
 *         Local Static Variables:
 *             global_voc_reset_ptr       Position to which to reset
 *                                            the "Global" Vocabulary
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Global Variables:    
 *             global_voc_dict_ptr       Reset to "Built-In" position 
 *             current_device_node       Reset to point at "root" dev-node
 *         Memory Freed
 *             All memory allocated by user-definitions in "normal" mode
 *             -- Macros, Conditionals and Device-node Vocabularies -- are
 *             reset via function call.
 *
 *      Error Detection:
 *          Presence of extra device-node data structure(s) indicates that
 *              there were more "new-device" calls than "finish-device";
 *              report each as an ERROR 
 *
 *      Process Explanation:
 *          Vocabularies created in other files, that have different
 *              data-structures, will have "reset" routines of their
 *              own associated with them in the files in which they
 *              are created.  Those routines will be called from here.
 *          Definitions in the "Tokenizer Escape Vocabulary", i.e.,
 *              the one in effect in "Tokenizer Escape" mode, are
 *              specifically not touched by this routine.
 *
 **************************************************************************** */

void reset_normal_vocabs( void )
{
    reset_tic_vocab( &global_voc_dict_ptr, global_voc_reset_ptr );

    /*  Delete the top-level device-node vocab.
     *      If there are extra device-nodes,
     *      delete their data structures and show errors
     */
    do
    {
        if ( current_device_node->parent_node != NULL )
	{
	    tokenization_error( TKERROR,
		 "Missing FINISH-DEVICE for new device");
	    started_at( current_device_node->ifile_name,
		 current_device_node->line_no );
	}
	    delete_device_vocab();
    }  while ( current_device_node->parent_node != NULL );

}


/* **************************************************************************
 *
 *      Function name:  reset_vocabs
 *      Synopsis:       Reset all the vocabularies to the proper state
 *                      for beginning a fresh tokenization, particularly
 *                      when multiple files are named on the command-line
 *      
 *      Inputs:
 *         Parameters:                NONE
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Memory Freed
 *             All memory allocated by user-definitions will be freed
 *
 *      Process Explanation:
 *          Call the  reset_normal_vocabs()  routine to get the vocabularies
 *              that apply to "Normal" mode, then call the "reset" routine
 *              for the "Tokenizer Escape Vocabulary", which is supposed
 *              to persist across device-nodes but not across input-files
 *              named on the command-line.
 *
 **************************************************************************** */

void reset_vocabs( void )
{
    reset_normal_vocabs();
    reset_tokz_esc();
}
