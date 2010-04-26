/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  stream.c - source program streaming from file.
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

#include <stdio.h>
#include <stdlib.h>
#ifdef __GLIBC__
#define __USE_XOPEN_EXTENDED
#endif
#include <string.h>
#include <sys/stat.h>

#include "emit.h"
#include "stream.h"
#include "errhandler.h"
#include "toke.h"

/* **************************************************************************
 *
 *      Revision History:
 *      Updated Tue, 31 Jan 2006 by David L. Paktor
 *          Add support for embedded Environment-Variables in path name
 *      Updated Thu, 16 Feb 2006 David L. Paktor
 *          Collect missing (inaccessible) filenames
 *      Updated Thu, 16 Mar 2006 David L. Paktor
 *          Add support for Include-Lists
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Still to be done:
 *          Re-arrange routine and variable locations to clarify the
 *              functions of this file and its companion, emit.c 
 *          This file should be concerned primarily with management
 *              of the Inputs; emit.c should be primarily concerned
 *              with management of the Outputs.
 *          Hard to justify, pragmatically, but will make for easier
 *              maintainability down the proverbial road...
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *          Global Variables Exported
 *              start                 Start of input-source buffer
 *              end                   End of input-source buffer
 *              pc                    Input-source Scanning pointer
 *              iname                 Current Input File name
 *              lineno                Current Line Number in Input File
 *              ostart                Start of Output Buffer
 *              oname                 Output File name
 *
 **************************************************************************** */


/* Input pointers, Position Counters and Length counters */
u8 *start = NULL;
u8 *pc;
u8 *end;
char *iname = NULL;
unsigned int lineno = 0;
unsigned int abs_token_no = 0;  /*  Absolute Token Number in all Source Input
                                 *      Will be used to identify position
                                 *      where colon-definition begins and
                                 *      to limit clearing of control-structs.
                                 */
static unsigned int ilen;   /*  Length of Input Buffer   */

/* output pointers */
u8 *ostart;
char *oname = NULL;


/* We want to limit exposure of this v'ble, so don't put it in  .h  file  */
unsigned int olen;          /*  Length of Output Buffer  */
/* We want to limit exposure of this Imported Function, likewise.  */
void init_emit( void);

/* **************************************************************************
 *
 *          Internal Static Variables
 *     load_list_name       Name of the Load List File
 *     load_list_file       (Pointer to) File-Structure for the Load List File
 *     depncy_list_name     Name of the Dependency List File
 *     depncy_file     (Pointer to) File-Structure for the Dependency List File
 *     missing_list_name    Name of the Missing-Files-List File
 *     missing_list_file    (Pointer to) File-Structure for Missing-List File
 *     no_files_missing     TRUE if able to load all files
 *
 **************************************************************************** */

static char *load_list_name;
static FILE *load_list_file;
static char *depncy_list_name;
static FILE *depncy_file;
static char *missing_list_name;
static FILE *missing_list_file;
static bool no_files_missing = TRUE;

/* **************************************************************************
 *
 *         Private data-structure for Include-List support
 *
 *     Components are simply a string-pointer and a link pointer
 *
 **************************************************************************** */

typedef struct incl_list
{
        char             *dir_path;
	struct incl_list *next;
    } incl_list_t;

/* **************************************************************************
 *
 *          Internal Static Variables associated with Include-List support
 *     include_list_start        Start of the Include-List
 *     include_list_next         Next entry in Include-List to add or read
 *     max_dir_path_len          Size of longest entry in the Include-List
 *     include_list_full_path    Full-Path (i.e., expanded File-name with
 *                                   Include-List Entry) that was last opened.
 *
 **************************************************************************** */
static incl_list_t *include_list_start = NULL;
static incl_list_t *include_list_next = NULL;
static unsigned int max_dir_path_len = 0;
static char *include_list_full_path = NULL;


/* **************************************************************************
 *
 *      Function name:  add_to_include_list
 *      Synopsis:       Add an entry to the Include-List
 *
 *      Inputs:
 *         Parameters:
 *             dir_compt               Directory Component to add to Inc-List
 *         Local Static Variables:
 *             include_list_start      First entry in the Include-List
 *             include_list_next       Next entry in Include-List to add
 *             max_dir_path_len        Previous max Dir-Component Length 
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Local Static Variables:
 *             include_list_start      Assigned a value, first time through
 *             include_list_next       "Next" field updated with new entry,
 *                                         then pointer updated to new entry.
 *             max_dir_path_len        Maximum Length 
 *         Memory Allocated
 *             For the list-entry, and for the directory/path name to be added
 *         When Freed?
 *             Remains in effect through the life of the program.
 *
 *      Process Explanation:
 *          Unlike most of our linked-lists, this one will be linked forward,
 *              i.e., in the order elements are added, and will be searched
 *              in a forward order.
 *          This means extra code to handle the first entry.
 *          Allocate and initialize the New Include-List Entry.
 *          If this is the first entry, point the List-Starter at it.
 *          Otherwise, the Last-Entry-on-the-List pointer is already valid;
 *              point its "next" field at the New Entry.
 *          Point the Last-Entry-on-the-List pointer at the New Entry.
 *
 **************************************************************************** */

void add_to_include_list( char *dir_compt)
{
    unsigned int new_path_len = strlen( dir_compt);
    incl_list_t *new_i_l_e = safe_malloc( sizeof( incl_list_t),
        "adding to include-list" );

    new_i_l_e->dir_path = strdup( dir_compt);
    new_i_l_e->next = NULL;

    if ( include_list_start == NULL )
    {
	include_list_start = new_i_l_e;
    }else{
       include_list_next->next = new_i_l_e;
    }
    
    include_list_next = new_i_l_e;
    if ( new_path_len > max_dir_path_len  ) max_dir_path_len = new_path_len;
}

#define   DISPLAY_WIDTH  80
/* **************************************************************************
 *
 *      Function name:  display_include_list
 *      Synopsis:       Display the Include-List, once it's completed,
 *                          if "verbose" mode is in effect.
 *
 *      Inputs:
 *         Parameters:                    NONE
 *         Local Static Variables:
 *             include_list_start         First entry in the Include-List
 *             include_list_next          Next entry, as we step through.
 *         Macro:
 *             DISPLAY_WIDTH              Width limit of the display
 *
 *      Outputs:
 *         Returned Value:                NONE
 *         Local Static Variables:
 *             include_list_next          NULL, when reaches end of Incl-List
 *         Printout:
 *             The elements of the Include-List, separated by a space, on
 *                 a line up to the DISPLAY_WIDTH, or on a line by itself
 *                 if the element is wider.
 *
 *      Process Explanation:
 *          The calling routine will check for the  verbose  flag.
 *          Nothing to be done here if Include-List is NULL.
 *          Print a header, then the list.
 *
 **************************************************************************** */

void display_include_list( void)
{
    if ( include_list_start != NULL )
    {
        int curr_wid = DISPLAY_WIDTH;     /*  Current width; force new line  */
	printf("\nInclude-List:");
	include_list_next = include_list_start ;
	while ( include_list_next != NULL )
	{
	    int this_wid = strlen( include_list_next->dir_path) + 1;
	    char *separator = " ";
	    if ( curr_wid + this_wid > DISPLAY_WIDTH )
	    {
	        separator = "\n\t";
		curr_wid = 7;  /*  Allow 1 for the theoretical space  */
	    }
	    printf("%s%s", separator, include_list_next->dir_path);
	    curr_wid += this_wid;
	    include_list_next = include_list_next->next ;
	}
	printf("\n");
    }
}



/* **************************************************************************
 *
 *          We cannot accommodate the structures of the different
 *              routines that open files with a single function, so
 *              we will have to divide the action up into pieces:
 *              one routine to initialize the include-list search, a
 *              second to return successive candidates.  The calling
 *              routine will do the operation (stat or fopen) until
 *              it succeeds or the list is exhausted.  Then it will
 *              call a final routine to clean up and display messages.
 *
 *          I'm sure I don't need to mention that, when no include-list
 *              is defined, these functions will still support correct
 *              operation...
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Function name:  init_incl_list_scan
 *      Synopsis:       Initialize the search through the Include-List
 *
 *      Inputs:
 *         Parameters:
 *             base_name               Expanded user-supplied file-name
 *         Local Static Variables:
 *             include_list_start      First entry in the Include-List
 *             max_dir_path_len        Maximum Directory Component Length 
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Local Static Variables:
 *             include_list_next       Next entry in Include-List to read
 *             include_list_full_path  Full-Path Buffer pointer
 *         Memory Allocated
 *             Full-Path Buffer        (If Include-List was defined)
 *         When Freed?
 *             In  finish_incl_list_scan()
 *
 *      Process Explanation:
 *          The  base_name  passed to the routine is expected to have
 *              any embedded Environment-Variables already expanded.
 *          The Full-Path Buffer is presumed to be unallocated, and
 *              its pointer to be NULL.
 *          The Next-Entry-to-read pointer is also presumed to be NULL.
 *          If an Include-List has been defined, we will allocate memory
 *              for the Full-Path Buffer and point the Next-Entry pointer
 *              at the Start of the List.  If not, no need to do anything.
 *
 **************************************************************************** */

static void init_incl_list_scan( char *base_name)
{
    if ( include_list_start != NULL )
    {
	/*  Allocate memory for the file-name buffer.
	 *      maximum path-element length plus base-name length
	 *      plus one for the slash plus one for the ending NULL
	 */
	unsigned int new_path_len = max_dir_path_len + strlen( base_name) + 2;
	include_list_full_path = safe_malloc( new_path_len,
	    "scanning include-list" );
	include_list_next = include_list_start;
    }
}

/* **************************************************************************
 *
 *      Function name:  scan_incl_list
 *      Synopsis:       Prepare the next candidate in the include-list search
 *                          Indicate when the end has been reached.
 *
 *      Inputs:
 *         Parameters:
 *             base_name               Expanded user-supplied file-name
 *         Local Static Variables:
 *             include_list_full_path  Full-Path Buffer pointer
 *             include_list_next       Next entry in Include-List to read
 *
 *      Outputs:
 *         Returned Value:             TRUE if valid candidate; FALSE when done
 *         Local Static Variables:
 *             include_list_full_path  Next Full file-name Path to use
 *             include_list_next       Updated to next entry in the List
 *
 *      Process Explanation:
 *          Include-List Components are presumed not to require any expansion;
 *              the Shell is expected to resolve any Environment-Variables
 *              supplied in command-line arguments before they are passed
 *              to the (this) application-program.
 *          We will, therefore, not attempt to expand any Components of
 *              the Include-List.  
 *          If the Full-Path Buffer pointer is NULL, it indicates that no
 *              entries have been made to the Include-List and this is our
 *              first time through this routine in this search; we will
 *              use the base-name as supplied, presumably relative to the
 *              current directory.  Point the buffer-pointer at the base-
 *              -name and return "Not Done".
 *          Otherwise, we will look at the Next-Entry pointer:
 *              If it is NULL, we have come to the end of the Include-List;
 *                  whether because no Include-List was defined or because
 *                  we have reached the end, we will return "Done".
 *              Otherwise, we will load the Full-Path Buffer with the entry
 *                  currently indicated by the Next-Entry pointer, advance
 *                  it to the next entry in the List and return "Not Done".
 *          We will supply a slash as the directory-element separator in
 *              between the Include-List entry and the  base_name 
 *
 *      Extraneous Remarks:
 *          The slash as directory-element separator works in UNIX-related
 *              environments, but is not guaranteed in others.  I would
 *              have preferred to specify that Include-List Components are
 *              required to end with the appropriate separator, but that
 *              was neither acceptable nor compatible with existing practice
 *              in other utilities, and the effort to programmatically
 *              determine the separator used by the Host O/S was too much
 *              for such a small return.  And besides, we already have so
 *              many other UNIX-centric assumptions hard-coded into the
 *              routines in this file (dollar-sign to signify Environment
 *              Variables, period for file-name-extension separator, etc.)
 *              that it's just too much of an uphill battle anymore...
 *
 **************************************************************************** */

static bool scan_incl_list( char *base_name)
{
    bool retval = FALSE;   /*  default to "Done"  */
	
    if ( include_list_full_path == NULL )
    {
	include_list_full_path = base_name;
	retval = TRUE;
    }else{
	if ( include_list_next != NULL )
	{

	    /*  Special case:  If the next Directory Component is
	     *      an empty string, do not prepend a slash; that
	     *      would either become a root-based absolute path,
	     *      or, if the base-name is itself an absolute path,
	     *      it would be a path that begins with two slashes,
	     *      and *some* Host Operating Systems ***REALLY***
	     *      DO NOT LIKE that!
	     */
	    if ( strlen( include_list_next->dir_path) == 0 )
	    {
		sprintf( include_list_full_path, "%s", base_name);
	    }else{
		sprintf( include_list_full_path, "%s/%s",
	            include_list_next->dir_path, base_name);
	    }
	    include_list_next = include_list_next->next;
	    retval = TRUE;
	}
    }

    return( retval);
}

/* **************************************************************************
 *
 *      Function name:  finish_incl_list_scan
 *      Synopsis:       Clean up after a search through the Include-List
 *                          Display appropriate messages.
 *
 *      Inputs:
 *         Parameters:
 *             op_succeeded            TRUE if intended operation was ok.
 *             
 *         Local Static Variables:
 *             include_list_start      Non-NULL if Include-List was defined
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Local Static Variables:
 *             include_list_full_path  Reset to NULL
 *             include_list_next       Reset to NULL
 *         Memory Freed
 *             Full-Path Buffer        (If Include-List was defined)
 *         Printout:
 *             If file was found in Include-List, Advisory showing where.
 *
 **************************************************************************** */

static void finish_incl_list_scan( bool op_succeeded)
{
    if ( include_list_start != NULL )
    {
	if ( op_succeeded )
	{
	    tokenization_error( INFO,
		"File was found in %s\n" ,include_list_full_path );
	}
	free( include_list_full_path);
    }
    include_list_full_path = NULL;
    include_list_next = NULL;
}

/* **************************************************************************
 *
 *      Function name:  open_incl_list_file
 *      Synopsis:       Look in the Include-List, if one is defined, for
 *                          the file whose name is given and open it.
 *
 *      Inputs:
 *         Parameters:
 *             base_name               Expanded user-supplied file-name
 *             mode                    Mode-string to use; usually "r" or "rb"
 *         Local Static Variables:
 *             include_list_full_path  Full Path to use in fopen atttempt
 *
 *      Outputs:
 *         Returned Value:             File structure pointer; NULL if failed
 *         Local Static Variables:
 *             include_list_full_path  Full Path used, if succeeded
 *
 *      Error Detection:
 *          Calling routine will detect and report Errors.
 *
 *      Process Explanation:
 *          This routine will initialize and step through Include-List.
 *              Calling routine will be responsible for "finishing" the
 *              Include-List search, as well as any Advisory messages.
 *
 **************************************************************************** */

static FILE *open_incl_list_file( char *base_name, char *mode)
{
    FILE *retval = NULL;

    init_incl_list_scan( base_name);
    while ( scan_incl_list( base_name) )
    {
        retval = fopen( include_list_full_path, mode);
	if ( retval != NULL )
	{
	    break; 
	}
    }

    return (retval);
}

/* **************************************************************************
 *
 *      Function name:  stat_incl_list_file
 *      Synopsis:       Look in the Include-List, if defined, for given file,
 *                          and collect its statistics.
 *                      
 *
 *      Inputs:
 *         Parameters:
 *             base_name               Expanded user-supplied file-name
 *             file_info               Pointer to STAT structure
 *         Local Static Variables:
 *             include_list_full_path  Full Path to use in file-status atttempt
 *
 *      Outputs:
 *         Returned Value:             TRUE if succeeded.
 *         Local Static Variables:
 *             include_list_full_path  Full Path used, if succeeded
 *         Supplied Pointers:
 *             *file_info              File-statistics structure from STAT call
 *
 *      Error Detection:
 *          Calling routine will detect and report Errors.
 *          
 *      Process Explanation:
 *          This routine will initialize and step through Include-List.
 *              Calling routine will be responsible for "finishing" the
 *              Include-List search, as well as any Advisory messages.
 *
 **************************************************************************** */

static bool stat_incl_list_file( char *base_name, struct stat *file_info)
{
    bool retval = FALSE;
    int stat_reslt = -1;    /*  Success = 0   */

    init_incl_list_scan( base_name);
    while ( scan_incl_list( base_name) )
    {
        stat_reslt = stat( include_list_full_path, file_info);
	if ( stat_reslt == 0 )
	{
	    retval = TRUE;
	    break; 
	}
    }

    return (retval);
}

/* **************************************************************************
 *
 *      Function name:  init_inbuf
 *      Synopsis:       Set the given buffer as the current input source
 *      
 *      Inputs:
 *         Parameters:
 *             inbuf         Pointer to start of new input buffer
 *             buflen        Length of new input buffer
 *
 *      Outputs:
 *         Returned Value:    NONE
 *         Global Variables:
 *             start             Points to given buffer
 *             end               Points to end of new input buffer
 *             pc                Re-initialized      
 *
 **************************************************************************** */

void init_inbuf(char *inbuf, unsigned int buflen)
{
    start = inbuf;
    pc = start;
    end = pc + buflen;
}

/* **************************************************************************
 *
 *      Function name:  could_not_open
 *      Synopsis:       Report the "Could not open" Message for various Files.
 *
 *      Inputs:
 *         Parameters:
 *             severity                  Severity of message, WARNING or ERROR
 *             fle_nam                   Name of file
 *             for_what                  Phrase after "... for "
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Printout:
 *             Message of indicated severity.
 *
 *      Error Detection:
 *          Error already detected; reported here.
 *
 **************************************************************************** */

static void could_not_open(int severity, char *fle_nam, char *for_what)
{
    tokenization_error( severity, "Could not open file %s for %s.\n",
	fle_nam, for_what);

}

/* **************************************************************************
 *
 *      Function name:  file_is_missing
 *      Synopsis:       Add the given File to the Missing-Files-List.
 *
 *      Inputs:
 *         Parameters:
 *             fle_nam                   Name of file
 *         Local Static Variables:
 *             missing_list_file         Missing-Files-List File Structure
 *                                           May be NULL if none was opened
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Local Static Variables:
 *             no_files_missing         Set FALSE
 *         File Output:
 *             Write File name to Missing-Files-List (if one was opened)
 *
 *      Error Detection:
 *          Error already detected; reported here.
 *
 **************************************************************************** */

static void file_is_missing( char *fle_nam)
{
    if ( missing_list_file != NULL )
    {
	fprintf( missing_list_file, "%s\n", fle_nam);
	no_files_missing = FALSE;
    }
}

/* **************************************************************************
 *
 *      Function name:  add_to_load_lists
 *      Synopsis:       Add the given input file-name to the Load-List File,
 *                      and the Full File path to the Dependency-List File.
 *
 *      Inputs:
 *         Parameters:
 *             in_name                  Name of given input Source File
 *         Local Static Variables:
 *             load_list_file           Load List File Structure pointer
 *             depncy_file              Dependency-List File Structure ptr
 *                            Either may be NULL if the file was not opened.
 *             include_list_full_path   Full Path to where the file was found.
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         File Output:
 *             Write given file-name to Load-List file (if one was opened)
 *             Write File Path to Dependency-List file (if one was opened)
 *
 *      Process Explanation:
 *          Write into the Load-List file the input Source Filename in the
 *              same form -- i.e., unexpanded -- as was supplied by the User.
 *          Write into the Dependency-List file the full expanded path, as
 *              supplied by the program to the Host Operating System.
 *
 **************************************************************************** */

static void add_to_load_lists( const char *in_name)
{
    if ( load_list_file != NULL )
    {
	fprintf( load_list_file, "%s\n", in_name);
    }
    if ( depncy_file != NULL )
    {
	fprintf( depncy_file, "%s\n", include_list_full_path);
    }
}


/* **************************************************************************
 *
 *      In the functions that support accessing files whose path-names
 *          contain embedded Environment-Variables, the commentaries
 *          will refer to this process, or to inputs that require it,
 *          using variants of the term "expand".
 *
 *      We will also keep some of the relevant information as Local
 *          Static Variables.
 *
 **************************************************************************** */

static char expansion_buffer[ 2*GET_BUF_MAX];
static bool was_expanded;
static int expansion_msg_severity = INFO;

/* **************************************************************************
 *
 *      Function name:  expanded_name
 *      Synopsis:       Advisory message to display filename expansion.
 *
 *      Inputs:
 *         Parameters:
 *         Local Static Variables:
 *             was_expanded              TRUE if expansion happened
 *             expansion_buffer          Buffer with result of expansion
 *             expansion_msg_severity    Whether it's an ordinary Advisory
 *                                           or we force a message.
 *
 *      Outputs:
 *         Returned Value:               NONE
 *             
 *         Printout:
 *             Advisory message showing expansion, if expansion happened
 *             Otherwise, nothing.
 *
 **************************************************************************** */

static void expanded_name( void )
{
    if ( was_expanded )
    {
	tokenization_error( expansion_msg_severity,
	    "File name expanded to:  %s\n", expansion_buffer);
    }
}


/* **************************************************************************
 *
 *      Function name:  expansion_error
 *      Synopsis:       Supplemental message to display filename expansion.
 *                      Called after an expanded-filename failure was reported.
 *
 *      Inputs:
 *         Parameters:
 *         Global Variables:
 *             verbose                   Set by "-v" switch
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Printout:
 *             Advisory message showing expansion, if applicable
 *                 and if wasn't already shown.
 *
 *      Error Detection:
 *          Called after Error was reported.
 *
 *      Process Explanation:
 *          Presumptions are that:
 *              An Error message, showing the user-supplied form of the
 *                  pathname is also being displayed
 *              An advisory message showing the pathname expansion may
 *                  have been displayed during the expansion process,
 *                  if  verbose  was TRUE.
 *          The purpose of this routine is to display the expansion if
 *              it had not already just been displayed, i.e., if  verbose
 *              is not set to TRUE:  Temporarily force the display of an
 *              Advisory message.
 *
 *      Extraneous Remarks:
 *          If this routine is called before the Error message is displayed,
 *              the verbose and non-verbose versions of the Log-File will
 *              match up nicely...
 *
 **************************************************************************** */

static void expansion_error( void )
{
    if ( INVERSE( verbose) )
    {
	expansion_msg_severity |= FORCE_MSG;
	expanded_name();
	expansion_msg_severity ^= FORCE_MSG;
    }
}


/* **************************************************************************
 *
 *      Function name:  expand_pathname
 *      Synopsis:       Perform the expansion of a path-name that may contain
 *                          embedded Environment-Variables
 *
 *      Inputs:
 *         Parameters:
 *             input_pathname           The user-supplied filename
 *         Global/Static MACRO:
 *             GET_BUF_MAX              Size of expansion buffer is twice this.
 *
 *      Outputs:
 *         Returned Value:              Pointer to expanded name, or to
 *                                          input if no expansion needed.
 *                                      NULL if error.
 *         Local Static Variables:
 *             was_expanded             TRUE if expansion needed and succeeded
 *             expansion_buffer         Result of expansion
 *         Printout:
 *             Advisory message showing expansion
 *             Presumption is that an Advisory giving the user-supplied
 *                 pathname was already printed.
 *
 *      Error Detection:
 *          Syntax error.  System might print something; it might not be
 *              captured, even to a log-file.  System failure return might
 *              be the only program-detectable indication.  Display ERROR
 *              message and return NULL pointer.  Calling routine will
 *              display the user-supplied pathname in its Error message
 *              indicating failure to open the file.
 *
 *      Process Explanation:
 *          Generally speaking, we will let the Shell expand the Environment
 *              Variables embedded in the user-supplied pathname.
 *          First, though, we will see if the expansion is necessary:  Look
 *              for the telltale character, '$', in the input string.  If
 *              it's not there, there are no Env't-V'bles, and no expansion
 *              is necessary.  Return the pointer to the input string and
 *              we're done.  Otherwise.....
 *          Acquire a temporary file-name.  Construct a string of the form:
 *                      echo input_string > temp_file_name
 *              and then issue that string as a command to the Shell.
 *          If that string generates a system-call failure, report an ERROR.
 *          Open the temporary file and read its contents.  That will be
 *              the expansion of the input string.  If its length exceeds
 *              the capacity of the expansion buffer, it's another ERROR.
 *          (Of course, don't forget to delete the temporary file.)
 *          Place the null-byte marker at the end of the expanded name,
 *              trimming off the terminating new-line.
 *          Success.  Display the expanded name (as an Advisory message)
 *              Return the pointer to the expansion buffer and set the flag.
 *              (Did I mention don't forget to delete the temporary file?)
 *
 *      Extraneous Remarks:
 *          This implementation approach turned out to be the simplest and
 *              cleanest way to accomplish our purpose.  It also boasts the
 *              HUGE advantage of not requiring re-invention of a well-used
 *              (proverbial) wheel.  Plus, any variations allowed by the
 *              shell (e.g.,: $PWD:h ) are automatically converted, too,
 *              depending on the System shell (e.g., not for Bourne shell).
 *          In order to spare you, the maintenance programmer, unnecessary
 *              agony, I will list a few other approaches I tested, with a
 *              brief note about the results of each:
 *          (1)
 *          I actually tried parsing the input line and passing each component
 *              V'ble to the getenv() function, accumulating the results into
 *              a conversion buffer.  I needed to check for every possible
 *              delimiter, and handle curly-brace enclosures.  The resultant
 *              code was *UGLY* ... you'd be appalled!  The only good spot was
 *              that I was able to compensate for an open-curly-brace without
 *              a corresponding close-curly-brace (if close-c-b wasn't found,
 *              resume the search for other delimiters...) which, apparently,
 *              the System does not or will not do.  It was, however, too
 *              small a compensation for all the awfulness entailed overall.
 *
 *          I tried various approaches to using the Environment-Variables to
 *              convert and retrieve the input string:
 *          (2)
 *          Create a command-string that would set an Env't V'ble to the
 *              input-string, and pass the command-string to the system() call,
 *              then retrieve the Env't V'ble thus set via getenv().  No dice;
 *              the system() call operated in a separate sub-shell and could
 *              not export its Env't upwards.
 *          (3)
 *          Use the setenv() command to set an Env't V'ble to the input-string
 *              and retrieve it via getenv().  The returned string matched the
 *              input-string without converting it.
 *          (4)
 *          Use the setenv() command to set an Env't V'ble to a string like:
 *                      `echo input_string`
 *              Again, the string retrieved via getenv() exactly matched the
 *              unconverted command-string, back-quotes and all.
 *
 *          Of course, the equivalents of (2), (3) and (4) worked as desired
 *              when tested as direct commands to the Shell.  UNIX can be
 *              funny that way...
 *
 *          Oh!  Also:  we will slightly stretch the rules of well-structured
 *              code.
 *
 **************************************************************************** */

static char *expand_pathname( const char *input_pathname)
{
    static const int buffer_max = GET_BUF_MAX * 2;

    char *retval = (char *)input_pathname;
    was_expanded = FALSE;

    /*  If no '$' is found, expansion is unnecessary.  */
    if ( strchr( input_pathname, '$') != NULL )
    {
	FILE *temp_file;
	int syst_stat;
	const char *temp_file_name = tmpnam( NULL);

	/*  Use the expansion buffer for our temporary command string  */
	sprintf( expansion_buffer,
	    "echo %s>%s\n", input_pathname, temp_file_name);
	syst_stat = system( expansion_buffer);
	if ( syst_stat != 0 )
	{
	    tokenization_error( TKERROR,
		"Expansion Syntax.\n");
	    /*  The "File-Opening" error message will show the input string */
	    return( NULL);
	}

	temp_file = fopen( temp_file_name, "r");  /*  Cannot fail.   */
	syst_stat = fread( expansion_buffer, 1, buffer_max, temp_file);
	/*  Error test.  Length of what we read is not a good indicator;
	 *      it's limited anyway by buffer_max.
	 *  Valid test is if last character read was the new-line.
	 */
	if ( expansion_buffer[syst_stat-1] != '\n' )
	{
	    tokenization_error( TKERROR,
		"Expansion buffer overflow.  Max length is %d.\n",
		    buffer_max);
	    retval = NULL;
	}else{
	    expansion_buffer[syst_stat-1] =0;
	    was_expanded = TRUE;
	    retval = expansion_buffer;
	    expanded_name();
	}

	fclose( temp_file);
	remove( temp_file_name);
    }

    return( retval);
}

/* **************************************************************************
 *
 *      Function name:  open_expanded_file
 *      Synopsis:       Open a file, expanding Environment-Variables that
 *                          may be embedded in the given path-name.
 *
 *      Inputs:
 *         Parameters:
 *             path_name              The user-supplied path-name
 *             mode                   Mode-string to use; usually "r" or "rb"
 *             for_what               Phrase to use in Messages
 *
 *      Outputs:
 *         Returned Value:            Pointer to FILE structure; NULL if failed
 *         Local Static Variables:
 *             was_expanded          TRUE if expansion happened
 *             expansion_buffer      Result of expansion
 *         Printout:
 *             Advisory message showing expansion
 *
 *      Error Detection:
 *          If expansion or system-call for file-open failed,
 *              report Error and return NULL.
 *
 **************************************************************************** */

FILE *open_expanded_file( const char *path_name, char *mode, char *for_what)
{

    FILE *retval = NULL;

    char *infile_name = expand_pathname( path_name);
    if ( infile_name != NULL )
    {
        retval = open_incl_list_file( infile_name, mode);
    }

    if ( retval == NULL )
    {
        expansion_error();
	tokenization_error ( TKERROR,
	    "Failed to open file %s for %s\n", path_name, for_what );
    }

    finish_incl_list_scan( BOOLVAL( retval != NULL) );

    return( retval);
}

/* **************************************************************************
 *
 *      Function name:  init_stream
 *      Synopsis:       Open a file and make it the current source.
 *                      This is called, not only at the start of tokenization,
 *                      but also when a subsidiary file is FLOADed.
 *      
 *      Inputs:
 *         Parameters:
 *             name                     Name of the new Input File to open
 *                                          May be path-name containing
 *                                          embedded Environment-Variables.
 *         Global Variables:
 *             oname                    NULL if opening Primary Input File
 *         Local Static Variables:
 *             include_list_full_path   Full Path to where the file was found
 *
 *      Outputs:
 *         Returned Value:    TRUE = opened and read file successfully
 *         Global Variables     (Only changed if successful):
 *             iname                    Set to new Input File name
 *             lineno                   Re-initialized to 1
 *         Local Static Variables:
 *             no_files_missing         Set FALSE if couldn't read input file
 *             include_list_full_path   Retains full-path if file opened was
 *                      the Primary Input File (as contrasted with an FLoaded
 *                      Source file), in which case a call to  init_output()
 *                      is expected; the Full-Path Buffer will be freed there.
 *         Memory Allocated
 *             Duplicate of Input File name (becomes  iname  )
 *             A fresh input buffer; input file is copied to it.
 *                 Becomes  start  by action of call to  init_inbuf().
 *         When Freed?
 *             By  close_stream()
 *         File Output:
 *             Write new Input File name to Load-List file.
 *             Writing to Missing-Files-List File if failure,
 *                 or Full File path to Dependency-List File,
 *                 is handled by called routine.
 *         Other Exotic Effects:
 *             Force a flush of  stdout  before printing ERROR messages
 *             
 *      Error Detection:
 *          Failure to open or read Input file:  ERROR; suppress output;
 *              write Input File name to Missing-Files-List File.
 *
 *      Process Explanation:
 *          Free local buffer on failure.
 *          Caller should only invoke  close_stream()  if this call succeeded.
 *          Some filesystems use zeros for new-line; we need to convert
 *              those zeros to line-feeds.
 *          Similarly for files that have carr-ret/line-feed; the carr-ret
 *              will cause havoc; replace it w/ a space.
 *      
 *      Revision History:
 *      Updated Thu, 07 Apr 2005 by David L. Paktor
 *          Restructured.  If opened file, close it, even if can't read it.
 *          Return TRUE on success.
 *          Caller examines return value.    
 *      Updated Wed, 13 Jul 2005 by David L. Paktor
 *          Replace carr-rets with spaces.   
 *      Updated Sun, 27 Nov 2005 by David L. Paktor
 *          Write new Input File name to Load-List file. 
 *      Updated Tue, 31 Jan 2006 by David L. Paktor
 *          Add support for embedded Environment-Variables in path name
 *      Updated Thu, 16 Feb 2006 David L. Paktor
 *          Collect missing (inaccessible) filenames
 *      Updated Fri, 17 Mar 2006 David L. O'Paktor
 *          Add support for Include-List search
 *
 *      Still to be done:
 *          Set a flag when carr-ret has been replaced by space;
 *              when a string crosses a line, if this flag is set,
 *              issue a warning that an extra space has been inserted.
 *
 **************************************************************************** */

bool init_stream( const char *name)
{
    FILE *infile;
    u8 *newbuf;
	struct stat finfo;
    bool stat_succ = FALSE;
    bool tried_stat = FALSE;
    bool retval = FALSE;
    bool inp_fil_acc_err = FALSE;
    bool inp_fil_open_err = FALSE;
    bool inp_fil_read_err = FALSE;

    char *infile_name = expand_pathname( name);

    if ( (infile_name != NULL) )
    {
	tried_stat = TRUE;
	stat_succ = stat_incl_list_file( infile_name, &finfo);
    }

    if ( INVERSE( stat_succ) )
    {
	inp_fil_acc_err = TRUE;
    }else{
	
	infile = fopen( include_list_full_path, "r");
	if ( infile == NULL )
	{
	    inp_fil_open_err = TRUE;
	}else{
	
	ilen=finfo.st_size;
	    newbuf = safe_malloc(ilen+1, "initting stream");

	    if ( fread( newbuf, ilen, 1, infile) != 1 )
	    {
		inp_fil_read_err = TRUE;
		free( newbuf );
	    } else {
		unsigned int i;

		retval = TRUE ;
		/*  Replace zeroes in the file with LineFeeds. */
		/*  Replace carr-rets with spaces.  */
		for (i=0; i<ilen; i++)
		{
		    char test_c = newbuf[i];
		    if ( test_c == 0    ) newbuf[i] = 0x0a;
		    if ( test_c == 0x0d ) newbuf[i] = ' ';
	}
		newbuf[ilen]=0;

		init_inbuf(newbuf, ilen);

		/*   If the -l option was specified, write the name of the
		 *       new input-file to the Load-List file...  UNLESS
		 *       this is the first time through and we haven't yet
		 *       opened the Load-List file, in which case we'll
		 *       just open it here and wait until we create the
		 *       output-file name (since the Load-List file name
		 *       depends on the output-file name anyway) before
		 *       we write the initial input-file name to it.
		 */
		/*   Looking for the option-flag _and_ for a non-NULL value
		 *       of the file-structure pointer is redundandundant:
		 *       The non-NULL pointer is sufficient, once the List
		 *       File has been created...
		 */
		/*   Same thing applies if the -P option was specified,
		 *       for the Dependency-List file, except there we'll
		 *       write the full path to where the file was found.
		 */
		/*   We have a routine to do both of those.   */
		add_to_load_lists( name);
		/*
		 *   And... there's one slight complication:  If this is
		 *       the first time through, (i.e., we're opening the
		 *       Primary Input File) then we haven't yet opened the
		 *       Dependency-List file, and we need to preserve the
		 *       Full file-name Buffer until the call to  init_output()
		 *       where the include-list scan will be "finish"ed.
		 *   Actually, we want to postpone "finish"ing the inc-l scan
		 *       for several reasons beyond the Dependency-List file, 
		 *       such as completing the File Name Announcement first.
		 *   A NULL output-name buffer is our indicator.
		 */
		if ( oname == NULL )
		{
		    /*  Quick way to suppress "finish"ing the i-l scan */
		    tried_stat = FALSE; 
		}
	    }
	fclose(infile);
	}
    }
	
    FFLUSH_STDOUT	/*   Do this first  */
    /*  Now we can deliver our postponed error and advisory messages  */
    if ( INVERSE( retval) )
    {
	file_is_missing( (char *)name);
	if ( inp_fil_acc_err )
	{
	    expansion_error();
	    tokenization_error( TKERROR, 
		"Could not access input file %s\n", name);
	}else{
	    if ( inp_fil_open_err )
	    {
		expansion_error();
		could_not_open( TKERROR, (char *)name, "input");
	    }else{
		if ( inp_fil_read_err )
		{
		    expansion_error();
		    tokenization_error( TKERROR, 
			"Could not read input file %s\n", name);
		}
	    }
	}
	}
	
    if ( tried_stat )
    {
        finish_incl_list_scan( stat_succ);
    }

    /*  Don't change the input file name and line-number until after
     *      the Advisory showing where the file was found.
     */
    if ( retval )
    {
	iname=strdup(name);
	lineno=1;
    }
	
    return ( retval );
	
}

/* **************************************************************************
 *
 *      Function name:  extend_filename 
 *      Synopsis:       Change the filename to the given extension
 *
 *      Inputs:
 *         Parameters:
 *             base_name                  Name of the Input Base File
 *             new_ext                    New ext'n (with leading period)
 *
 *      Outputs:
 *         Returned Value:                Result filename
 *         Memory Allocated
 *             Buffer for result filename
 *         When Freed?
 *             At end of Tokenization, by  close_output().
 *
 *      Process Explanation:
 *          If the Input Base File Name has an extension, it will be replaced
 *              with the given new extension.  If not, the new extension will
 *              simply be appended.
 *          If the Input Base File Name has an extension that matches the
 *              new extension, a duplicate of the extension will be appended.
 *
 *      Extraneous Remarks:
 *          I only recently added protection against the situation where the
 *              Input Base File Name has no extension, but the Directory Path
 *              leading to it has a period in one of the directory names.
 *              Granted, this is a rare case, but not altogether impossible;
 *              I would have done it earlier except for the fact that the
 *              separator between directories may vary with different Host
 *              Operating Systems.
 *          However, at this point we have UNIX-centric assumptions hard-
 *              -coded in to so many other places that we might as well
 *              go with the slash here too.
 *
 **************************************************************************** */

static char *extend_filename( const char *base_name, const char *new_ext)
{
    char *retval;
    char *ext;
  	unsigned int len; /* should this be size_t? */
    const char *root;

    root = strrchr(base_name, '/');
    if ( root == NULL )  root = base_name;

    ext = strrchr(root, '.');
    if ( ext != NULL )
    {
        if ( strcasecmp(ext, new_ext) == 0 )
	{
	    ext = NULL;
	}
    }

    len = ext ? (ext - base_name) : (unsigned int)strlen(base_name) ;
    retval = safe_malloc(len+strlen(new_ext)+1, "extending file-name");
    memcpy( retval, base_name, len);
    retval[len] = 0;
    strcat(retval, new_ext);

    return( retval);
}

/* **************************************************************************
 *
 *      Function name:  init_output
 *      Synopsis:       After the Input Source File has been opened, assign 
 *                          the name for the Binary Output File; initialize
 *                          the FCode Output Buffer; assign names for the
 *                          FLoad List, Dependency-List, and Missing-Files
 *                          List files; open them and write their first
 *                          entries to them.
 *                     Announce the Input and various output file names.
 *
 *      Inputs:
 *         Parameters:
 *             in_name                  Name of the Input Source File
 *             out_name                 Name of the Binary Output File, if
 *                                          specified on the Command Line,
 *                                          or NULL if not.
 *         Global Variables:
 *             fload_list               Whether to create an FLoad-List file
 *             dependency_list          Whether to create a Dependency-List file
 *         Local Static Variables:
 *             include_list_full_path   Full Path to the Input Source File;
 *                                          should still be valid from opening
 *                                          of Primary Source Input file, for
 *                                          first entry to Dependency-List file.
 *
 *      Outputs:
 *         Returned Value:              NONE
 *         Global Variables:
 *             oname                    Binary Output File Name
 *             ostart                   Start of FCode Output Buffer
 *             opc                      FCode Output Buffer Position Counter
 *             abs_token_no             Initialized to 1
 *         Local Static Variables:
 *             load_list_name           Name of the Load List File
 *             load_list_file           FLoad List File Structure pointer
 *             depncy_list_name         Name of the Dependency List File ptr
 *             depncy_file              Dependency List File Structure
 *             missing_list_name        Name of the Missing-Files-List File
 *             missing_list_file        Missing-Files-List File Structure
 *             no_files_missing         Initialized to TRUE
 *         Memory Allocated
 *             Binary Output File Name Buffer
 *             FCode Output Buffer
 *             FLoad List File Name Buffer
 *             Dependency List File Name Buffer
 *             
 *         When Freed?
 *             In  close_output()
 *         File Output:
 *             FLoad List or Dependency List files are opened (if specified).
 *                 Primary Source Input file name and path, respectively,
 *                 are written as the first entry to each.
 *         Printout:
 *             (Announcement of input file name has already been made)
 *             Announce binary output, fload- and dependency- -list file names
 *
 *      Error Detection:
 *          Failure to open FLoad List or Dependency List file:  ERROR;
 *              suppress binary output.  Further attempts to write to
 *              FLoad List or Dependency List files are prevented by
 *              the respective FILE_structure pointers being NULL.
 *          Failure to open Missing-Files-List file:  WARNING
 *
 *      Process Explanation:
 *          If no Output File Name was specified on the Command Line, the
 *              name of the Binary (FCode) Output File will be derived
 *              from the name of the Input File by replacing its extension
 *              with  .fc , or, if  the Input File had no extension, by
 *              merely appending the extension  .fc 
 *          In the odd case where the Input File name has an extension
 *              of  .fc, we will merely append another  .fc  extension.
 *          If  fload_list  is TRUE (i.e., the "-l" option was specified on
 *              the command-line, the FLoad List File name will be derived
 *              from the name of the Output File by the same rules, only with
 *              an extension of  .fl   Open the FLoad List File.  Write the
 *              name of the initial input file to the FLoad List File.
 *          Similarly if the "-P" command-line option was specified, the name
 *              of the Dependency List File will be derived with an extension
 *              of  .P  Open it and write the Full Path for the initial input
 *              file to it.  NOTE:  To do that, we need to have preserved the
 *              Full Path-name Buffer from the call to  init_stream()   We
 *              will "finish" it here, after we've used it.
 *          The Missing-Files-List File will be created if either option was
 *              specified.  Its name will be derived similarly, with an
 *              extension of  .fl.missing
 *
 **************************************************************************** */

void init_output( const char *in_name, const char *out_name )
{
	/* preparing output */

	if( out_name != NULL )
	{
		oname = strdup( out_name );
	}else{
		oname = extend_filename( in_name, ".fc"); 
	}
	
	/* output buffer size. this is 128k per default now, but we
	 * could reallocate if we run out. KISS for now.
	 */
	olen = OUTPUT_SIZE;
	ostart=safe_malloc(olen, "initting output buffer");

	init_emit();  /* Init'l'zns needed by our companion file, emit.c  */

	printf("Binary output to %s ", oname);
	if ( fload_list )
	{
	    load_list_name = extend_filename( oname, ".fl");
	    load_list_file = fopen( load_list_name,"w");
	    printf("  FLoad-list to %s ", load_list_name);
	}
	if ( dependency_list )
	{
	    depncy_list_name = extend_filename( oname, ".P");
	    depncy_file = fopen( depncy_list_name,"w");
	    printf("  Dependency-list to %s ", depncy_list_name);
	}
	printf("\n");

	add_to_load_lists( in_name);
	
	/*  Let's avoid collisions between stdout and stderr  */
	FFLUSH_STDOUT

	/*  Now we can deliver our advisory and error messages  */
	
	{
	    /* Suspend showing filename in advisory and error messages. */
	    char *temp_iname = iname;
	    iname = NULL; 
	
	    finish_incl_list_scan( TRUE);

	    if ( fload_list && (load_list_file == NULL) )
	    {
	    	could_not_open( TKERROR, load_list_name, "Load-List");
		free( load_list_name);
	    }
	    if ( dependency_list && (depncy_file == NULL) )
	    {
	    	could_not_open( TKERROR, depncy_list_name,
		    "Dependency-List");
		free( depncy_list_name);
}

	    if ( fload_list || dependency_list )
	    {
		missing_list_name = extend_filename( oname, ".fl.missing");
		missing_list_file = fopen( missing_list_name,"w");
		no_files_missing = TRUE;

		if ( missing_list_file == NULL )
		{
		    could_not_open( WARNING, missing_list_name,
			"Missing-Files List" );
	            free( missing_list_name);
		}
	    }
	    iname = temp_iname;
	}
	abs_token_no = 1;
}

/* **************************************************************************
 *
 *      Function name:  increase_output_buffer
 *      Synopsis:       Reallocate the Output Buffer to double its prior size 
 *
 *      Inputs:
 *         Parameters:                  NONE
 *         Global Variables:
 *             ostart                   Start-address of current Output Buffer
 *             olen                     Current size of the Output Buffer
 *         Local Static Variables:
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Local Static Variables:
 *             olen                     Doubled from value at input
 *             ostart                   Start-address of new Output Buffer
 *         Memory Allocated
 *             A new FCode Output Buffer, using the  realloc()  facility.
 *         When Freed?
 *             In  close_output()
 *         Memory Freed
 *             Former FCode Output Buffer, also by means of  realloc() 
 *
 *         Printout:
 *             Advisory message that this is taking place.
 *
 *      Error Detection:
 *          FATAL if  realloc()  fails.
 *          FATAL if output buffer has been expanded to a size beyond
 *              what an INT can express.  Unlikely? maybe; impossible? no...
 *
 *      Process Explanation:
 *          Because we are allowing the Output Buffer to be relocated, we
 *              must take care to limit the exposure to external routines
 *              of its actual address.  All references to locations within
 *              the Output Buffer should be made in terms of an _offset_.
 *
 *      Extraneous Remarks:
 *          Unfortunately, it is not feasible to completely isolate the
 *              actual address of the Output Buffer, but we will keep the
 *              exposure limited to the routines in  emit.c
 *          Similarly, it wasn't feasible to keep this routine isolated,
 *              nor the variable  olen  , but we will limit their exposure.
 *
 **************************************************************************** */
void increase_output_buffer( void );  /*  Keep the prototype local  */
void increase_output_buffer( void )
{
    u8 *newout;

    if ( olen == 0 )
    {
	tokenization_error( FATAL,
		"Output Buffer reallocation overflow.");
    }else{
	unsigned int rea_len;
	olen = olen * 2;
	rea_len = olen;
	if ( rea_len == 0 )
	{
	    rea_len = (unsigned int)-1;
	}
	tokenization_error( INFO,
	    "Output Buffer overflow.  "
		"Relocating and increasing to %d bytes.\n", rea_len);

	newout = realloc(ostart, rea_len);
	if ( newout == NULL)
	{
	    tokenization_error( FATAL,
		"Could not reallocate %d bytes for Output Buffer", rea_len);
	}

	ostart = newout;
    }
}


/* **************************************************************************
 *
 *      Function name:  close_stream
 *      Synopsis:       Free-up the memory used for the current input file
 *                          whenever it is closed.  Reset pointers and
 *                          line-counter.  Close files as necessary.
 *
 *      The dummy parameter is there to accommodate Macro-recursion protection. 
 *          It's a long story; don't get me started...
 *
 **************************************************************************** */

void close_stream( _PTR dummy)
{
	free(start);
	free(iname);
	start = NULL;
	iname = NULL;
	lineno = 0;
}

/* **************************************************************************
 *
 *      Function name:  close_output
 *      Synopsis:       Write the Binary Output file, if appropriate.
 *                          Return a "Failure" flag.
 *
 **************************************************************************** */


bool close_output(void)
{
    bool retval = TRUE;  /*  "Failure"  */
    if ( error_summary() )
    { 
	if ( opc == 0 )
{
	    retval = FALSE;  /*  "Not a problem"  */
	}else{
	FILE *outfile;

	    outfile=fopen( oname,"w");
	    if (!outfile)
	    {
		/*  Don't do this as a  tokenization_error( TKERROR
		 *      because those are all counted, among other reasons...
		 */ 
		printf( "Could not open file %s for output.\n", oname);
	    }else{
	
		if ( fwrite(ostart, opc, 1, outfile) != 1 )
		{
		    tokenization_error( FATAL, "While writing output.");
	}
	
	fclose(outfile);

		printf("toke: wrote %d bytes to bytecode file '%s'\n",
		    opc, oname);
		retval = FALSE;  /*  "No problem"  */
	    }
	}
    }
	
	free(oname);
    free(ostart);
    oname = NULL;
    ostart = NULL;
    opc = 0;
    olen = OUTPUT_SIZE;

    if ( load_list_file != NULL )
    {
	fclose(load_list_file);
	free(load_list_name);
    }
    if ( depncy_file != NULL )
    {
	fclose(depncy_file);
	free(depncy_list_name);
    }
    if ( missing_list_file != NULL )
    {
	fclose( missing_list_file);
	if ( no_files_missing )
	{
	    remove( missing_list_name);
	}
	free( missing_list_name);
    }

    load_list_file = NULL;
    load_list_name = NULL;
    missing_list_file = NULL;
    missing_list_name = NULL;
    depncy_file = NULL;
    depncy_list_name = NULL;

    return ( retval );
}

