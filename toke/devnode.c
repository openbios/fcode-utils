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
 *      Support routines for managing device-node vocabularies
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      The vocabulary that is created for a device-node must not remain
 *          available outside of that node.  Also, nodes may be nested,
 *          child within parent.
 *      An attempt within a child-node to access directly a method defined
 *          in the parent must be flagged as an error.  (Consider what would
 *          happen if the method in the parent-node used instance data, and
 *          the child-node has an instance of its own.)
 *      The correct way is to invoke the method via "$call-parent" or the like.
 *
 *      We will, however, allow the user to specify a group of exceptions,
 *          words whose scope will be "global" within the tokenization.
 *          When "global" scope is initiated, definitions will be made to
 *          the "core" vocabulary until "device" scope is resumed.
 *          That will (mostly) all be handled in  dictionary.c 
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Functions Exported:
 *          new_device_vocab        Create the new device-node's data-structure
 *          delete_device_vocab     Remove device-node's data-structure
 *          finish_device_vocab     Remove struct and give messages when
 *                                      device is "finish"ed.
 *          exists_in_ancestor      Issue a Message if the given word exists
 *                                      in an ancestor of the current dev-node.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Still to be done:
 *          Add a pair of fields to the data-structure for the Input File and
 *              Line Number where "finish-device" occurred.  When a device-node
 *              is "finish"ed, do not delete it, but instead fill in those
 *              fields and the move the node to a separate linked-list.
 *          When looking whether a word exists in an ancestor-node, also
 *              check whether it was in a device-node that was finished and
 *              print both where it was started and where it was finished.
 *
 **************************************************************************** */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "devnode.h"
#include "errhandler.h"
#include "scanner.h"
#include "vocabfuncts.h"
#include "flowcontrol.h"
#include "stream.h"
#include "ticvocab.h"


/* **************************************************************************
 *
 *      Tokenization starts with an implicit "new-device" in effect.
 *          The top-level device-node is never removed.
 *
 *      Initialize it here
 *
 **************************************************************************** */
char default_top_dev_ifile_name[] = "Start of tokenization";

static device_node_t top_level_dev_node = {
     	NULL ,                          /*  parent_node    */
	default_top_dev_ifile_name ,	/*  ifile_name.
					 *     Something to show, Just In Case
					 */
	0 ,                             /*  line_no        */
	NULL ,                          /*  tokens_vocab   */
};

/* **************************************************************************
 *
 *          Global Variables Exported.
 *                                  Pointers to:
 *     current_device_node      data-structure of current device-node
 *     current_definitions      vocab into which to add def'ns.
 *
 **************************************************************************** */

device_node_t *current_device_node = &top_level_dev_node;
tic_hdr_t **current_definitions = &(top_level_dev_node.tokens_vocab);


/* **************************************************************************
 *
 *          Internal Static Variables
 *               These are used to support the routines  in_what_node() 
 *                   and  show_node_start() , which are used to facilitate
 *                   certain kinds of Messaging, as described later.
 *
 *     in_what_buffr       Buffer for the  in_what_node()  string
 *     show_where          TRUE if the string needs to be followed-up
 *     show_which          TRUE if follow-up should be  just_where_started()
 *                             rather than  just_started_at()
 *     in_what_line        Line Number to use in the follow-up
 *     in_what_file        File Name to use in the follow-up
 *
 **************************************************************************** */

static char in_what_buffr[50];   /*  Ought to be more than enough.  */
static bool show_where = FALSE;
static bool show_which;
static int in_what_line;
static char *in_what_file;


/* **************************************************************************
 *
 *      Function name:  dev_vocab_control_struct_check
 *      Synopsis:       Issue Warnings for unresolved flow-control constructs
 *                          at start or end of a device-node.
 *
 *      Inputs:
 *         Parameters:                     NONE
 *         Global Variables:
 *             statbuf                     The command being processed.
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Printout:
 *             Handled by  announce_control_structs() routine
 *
 *      Error Detection:
 *          Handled by  announce_control_structs()  routine
 *
 *      Process Explanation:
 *          Set up a buffer with the error message, based on  statbuf
 *              and pass it to  announce_control_structs()
 *          Release it when done.
 *
 **************************************************************************** */

static void dev_vocab_control_struct_check( void)
{
    char *ccs_messg;

    ccs_messg = safe_malloc(strlen(statbuf) + 32,
        "Device-Node control-structure check");
    
    strcpy( ccs_messg, statbuf );
    strupr( ccs_messg);
    strcat( ccs_messg, " encountered");
    announce_control_structs( WARNING, ccs_messg, 0 );
    free( ccs_messg);
}



/* **************************************************************************
 *
 *      Function name:  new_device_vocab
 *      Synopsis:       Create and initialize the data-structure for a
 *                      new device-node when a "new-device" is created,
 *                      with messages as appropriate.
 *
 *      Inputs:
 *         Parameters:                     NONE
 *         Global Variables:
 *             statbuf                     The word that was just read.
 *             iname                       Current Input-File Name
 *             lineno                      Current line-number
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Global Variables:
 *             current_device_node         Will point to the new data-structure
 *         Memory Allocated
 *             Space for the new  device_node_t  data-structure
 *             Space for a copy of the current input file name
 *         When Freed?
 *             By delete_device_vocab(), when the device-node is "finish"ed.
 *         Printout:
 *             Advisory message.
 *
 *      Error Detection:
 *          In immediate-execution mode, Control Structures that have not
 *              been completed are questionable;  Issue WARNINGS via the
 *               dev_vocab_control_struct_check()  routine.
 *
 *      Process Explanation:
 *          This routine is called when "new-device" is invoked, but only
 *              if we are in immediate-execution mode.
 *          Later on, in ERROR- or INFOrmative messages, we will want to
 *              be able to refer to the file and line-number in which this
 *              was encountered, so we include them in the structure.
 *
 **************************************************************************** */

void new_device_vocab( void )
{
    device_node_t *new_node_data;

    dev_vocab_control_struct_check();

    /*  Advisory message will mention previous device-node
     *      if there was one.  Either way starts out the same:
     */
#define NEW_DEV_MSG_START  "Encountered %s.  Starting new device-node."

    if ( current_device_node == &top_level_dev_node )
    {
	tokenization_error(INFO, NEW_DEV_MSG_START "\n", statbuf );
    }else{
	tokenization_error(INFO, NEW_DEV_MSG_START
	    "  Suspending definitions of parent-device node", statbuf );
	started_at( current_device_node->ifile_name,
	     current_device_node->line_no );
    }

    /*  Now to business...   */
    new_node_data = safe_malloc( sizeof(device_node_t),
        "creating new-device vocab data" );
    new_node_data->parent_node = current_device_node;
    new_node_data->ifile_name = strdup(iname);
    new_node_data->line_no = lineno;
    new_node_data->tokens_vocab = NULL;

    current_device_node = new_node_data;
    
    current_definitions = &(current_device_node->tokens_vocab);
}


/* **************************************************************************
 *
 *      Function name:  delete_device_vocab
 *      Synopsis:       Remove the vocabularies of the current device-node,
 *                          along with its data-structure, when the device
 *                          is "finish"ed; do not print messages.
 *                      Do not remove the top-level device-node data-struct.
 *
 *      Associated FORTH words:              FINISH_DEVICE  (interpretive state)
 *                                           END0  END1
 *      Associated Tokenizer directives:     RESET-SYMBOLS  (in "Normal" mode)
 *                                           FCODE-END
 *
 *      Inputs:
 *         Parameters:                    NONE
 *         Global Variables:
 *             current_device_node        Points to current device's struct
 *                                            Leads to chain of dev-node structs
 *
 *      Outputs:
 *         Returned Value: 
 *         Global Variables:
 *             current_device_node        Parent-device's struct becomes current
 *         Memory Freed
 *             All that was allocated for the tokens and the definers
 *                 vocabs in the current device-node
 *             The copy of the input file name, except the top-level
 *             The current_device_node data-structure, except the top-level
 *
 **************************************************************************** */

void delete_device_vocab( void )
{
    reset_tic_vocab( current_definitions, NULL );

    if ( current_device_node != &top_level_dev_node )
    {
	device_node_t *temp_node = current_device_node;
	current_device_node = current_device_node->parent_node;
	free( temp_node->ifile_name );
	free(temp_node);
    }

    current_definitions = &(current_device_node->tokens_vocab);
}

/* **************************************************************************
 *
 *      Function name:  finish_device_vocab
 *      Synopsis:       Remove the device-node data-structure and all its
 *                          vocabularies when the device is "finish"ed,
 *                          with appropriate messages.
 *                      Do not remove the top-level device node data-struct.
 *
 *      Associated FORTH word:                 FINISH_DEVICE
 *
 *      Inputs:
 *         Parameters:                    NONE
 *         Global Variables:
 *             current_device_node        Current device's struct pointer
 *
 *      Outputs:
 *         Returned Value:                NONE
 *         Global Variables:
 *             current_device_node        Parent-device's struct becomes current
 *         Printout:
 *             Advisory message.
 *
 *      Error Detection:
 *          If current_device_node is already pointing at the top-level
 *              device node, it means there was no corresponding NEW-DEVICE
 *              Issue an ERROR.
 *          In immediate-execution mode, Control Structures that have not
 *              been completed are questionable;  Issue WARNINGS via the
 *               dev_vocab_control_struct_check()  routine.
 *
 *      Process Explanation:
 *          This routine is called when "finish-device" is invoked, but only
 *              if we are in immediate-execution mode.
 *
 **************************************************************************** */

void finish_device_vocab( void )
{
    bool at_top_level;

    dev_vocab_control_struct_check();

    /*   We never remove the top-level device-node vocabulary,
     *       so we need to test whether we're about to.
     */

    at_top_level = BOOLVAL( current_device_node == &top_level_dev_node );
    if ( at_top_level )
    {
        tokenization_error( TKERROR,
	    "Encountered %s without corresponding NEW-DEVICE.  "
	    "Resetting definitions since start of tokenization.\n",
		statbuf );
    }else{    
	tokenization_error(INFO,
	    "Encountered %s.  Resetting definitions of device node",
		statbuf );
	started_at( current_device_node->ifile_name,
	     current_device_node->line_no );
    }

    /*  Now to business...   */
    delete_device_vocab();

    /*   Did we just get to the top-level device-node vocabulary
     *       when we weren't before?
     */
    if ( INVERSE(at_top_level) )
    {
	if ( current_device_node == &top_level_dev_node )
	{
	    tokenization_error(INFO,
		"Resuming definitions since start of tokenization.\n" );
	}else{
	    tokenization_error(INFO,
		"Resuming definitions of parent device-node" );
	    started_at( current_device_node->ifile_name,
		 current_device_node->line_no );
	}
    }
}


/* **************************************************************************
 *
 *      Function name:  in_what_node
 *      Synopsis:       Format a string for use in a Message that might
 *                          identify the start of the given device-node.
 *
 *      Inputs:
 *         Parameters:
 *             the_node               The device-node vocabulary about which
 *                                        to construct the identifying phrase.
 *         Local Static Variables:
 *             in_what_buffr          Buffer in which to format the string.
 *         Global Variables:
 *             current_definitions    Device-node vocabulary currently
 *                                        in effect.
 *
 *      Outputs:
 *         Returned Value:           Pointer to buffer w/ formatted string
 *         Local Static Variables:
 *             in_what_buffr         Will contain the formatted string.
 *             show_where            TRUE if the string needs to be followed-up
 *                                        (i.e., did not contain a terminating
 *                                        new-line) by  just_where_started()
 *                                        or by  just_started_at()
 *             show_which            TRUE if the follow-up call should be
 *                                       to  just_where_started()  rather 
 *                                       than to  just_started_at() 
 *             in_what_line          Copy of line_no field from the_node
 *             in_what_file          Copy of ifile_name field from the_node
 *
 *      Process Explanation:
 *          Calling routine must ascertain that Global-scope is not in effect.
 *          The returned phrase can be used as a string argument in a Message.
 *          Set  show_where  TRUE if the_node->line_no is non-zero.
 *          Set  show_which  TRUE if the_node is either the Current or the
 *              Top-Level device-node
 *          If the originating line-number in the given Node structure is zero,
 *              the returned phrase will contain a terminating new-line.
 *              (This only happens if the given Node is the top-level Node,
 *              and it's the Current Node, and the "official" starting-point
 *              hasn't yet been established by an "FCode-Starter" such as 
 *               FCODE-VERSION2 .  Once that command has been given, even
 *              definitions that were made prior to it belong to the Node
 *              that started there.)
 *          Otherwise,  show_where  is returned TRUE, and  show_which  becomes
 *              relevant.  If the given node is the Current or the Top-Level
 *              node, text about the originating file-name and line-number
 *              merely describes a node that is already uniquely identified,
 *              so the message appended to the buffer will have the phrase
 *              "which began" (which introduces what is known in grammar as
 *              an Appositive Subordinate Clause) and  show_which  will be
 *              returned TRUE.  If the given node is not uniquely identifiable
 *              without the file- and line- phrase, then the Subordinate Clause
 *              is Indicative, and should be introduced with "that" (and no
 *              comma); in that case, we will return  show_which  as FALSE.
 *          After the calling routine displays the message in which the
 *              returned phrase is used, it must call  show_node_start()
 *              to display the followe-up message, if any.
 *
 **************************************************************************** */

char *in_what_node(device_node_t *the_node)
{
    bool top_node    = BOOLVAL( the_node == &top_level_dev_node);
    bool curr_node   = BOOLVAL( the_node == current_device_node);
    bool known_node  = BOOLVAL( top_node || curr_node );
    bool no_line     = BOOLVAL( the_node->line_no == 0);

    show_where   = INVERSE( no_line  );
    show_which   = known_node;
    in_what_line = the_node->line_no;
    in_what_file = the_node->ifile_name;

    sprintf( in_what_buffr, "in the%s device-node%s",
	INVERSE( known_node )  ? ""
	        :  top_node    ?    " top-level"   :  " current" ,
	
	no_line                ?  ".\n"
	        :  known_node  ?  ", which began"  :   ""         );

    
    return( in_what_buffr);
}


/* **************************************************************************
 *
 *      Function name:  show_node_start
 *      Synopsis:       Follow-up to the  in_what_node()  call.  Print out,
 *                          if applicable, the text about the originating
 *                          file-name and line-number
 *
 *      Inputs:
 *         Parameters:             NONE
 *         Local Static Variables:
 *             show_where          Nothing to do if not TRUE
 *             show_which          TRUE if should call  just_where_started()
 *                                     rather than  just_started_at()
 *             in_what_line        Line Number to use in the follow-up
 *             in_what_file        File Name to use in the follow-up
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Local Static Variables:
 *             show_where          Force to FALSE
 *         Printout:
 *             Follow-up to  the in_what_node() call.  Applicable text
 *                 about the originating file-name and line-number.
 *
 *      Process Explanation:
 *          By forcing  show_where  to FALSE after this is called, we
 *              can safely allow routines that might or might not have
 *              called  in_what_node()  to call this routine, without
 *              needing any additional "bookkeeping".
 *
 **************************************************************************** */

void show_node_start( void)
{
    if ( show_where)
    {
	if ( show_which )
	{
	    just_where_started( in_what_file, in_what_line);
	}else{
	    just_started_at( in_what_file, in_what_line);
	}
	show_where = FALSE;
    }
}



/* **************************************************************************
 *
 *      Function name:  exists_in_ancestor
 *      Synopsis:       Issue a Message and return an indication if
 *                          the given word exists in an ancestor of
 *                          the current device-node.
 *                      Used for additional error-message information.
 *                      
 *
 *      Inputs:
 *         Parameters:
 *             m_name                   "Method" name
 *         Global Variables:
 *             current_device_node      Leads to chain of dev-node data-structs
 *             scope_is_global          TRUE if "global" scope is in effect
 *
 *      Outputs:
 *         Returned Value:              TRUE if word found
 *         Printout:
 *             If  m_name  exists in an ancestor-node, print an ADVISORY
 *                 giving the location where the ancestor originated.
 *
 *      Error Detection:
 *          None here.  Calling routine detected error; see below.
 *
 *      Process Explanation:
 *          This routine was called as the result of detecting an error:
 *              viz.,  m_name  was not found in either the current node
 *              or the base vocabulary.  (Except:  If "global" scope is
 *              in effect, we didn't search the current device-node).
 *
 **************************************************************************** */

bool exists_in_ancestor( char *m_name)
{
    tic_hdr_t *found;
    bool retval = FALSE;
    if ( current_device_node != NULL )
    {
	device_node_t *grandpa = current_device_node->parent_node;

	if ( scope_is_global )    grandpa = current_device_node;

	for ( ; grandpa != NULL; grandpa = grandpa->parent_node )
	{
	    found = lookup_tic_entry( m_name, grandpa->tokens_vocab);
	    if ( found != NULL )
	    {
		retval = TRUE;
		break;
	    }
	}
	if ( grandpa != NULL )
	{
	    char as_what_buf[AS_WHAT_BUF_SIZE] = "";
	    if ( as_a_what( found->fword_defr, as_what_buf) )
	    {
		strcat( as_what_buf, " ");
	    }
	    tokenization_error(INFO, "%s is defined %s%s", m_name,
		as_what_buf, in_what_node( grandpa) );
	    show_node_start();
	}
    }

    return(retval );
}
