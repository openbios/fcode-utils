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
 *      Function(s) for entering Vendor-Specific FCodes to detokenizer.
 *
 *      (C) Copyright 2006 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          add_fcodes_from_list           Add Vendor-Specific FCodes from
 *                                             the file whose name is supplied.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Revision History:
 *          Tue, 25 Apr 2006 by David L. Paktor
 *              Identified this need when working with in-house code,
 *                  which uses some custom functions.  This solution
 *                  is (hoped to be) general enough to cover all cases.
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *          Global Variables Imported
 *              indata                Buffer into which the file will be read
 *              stream_max            Size of the file buffer.
 *
 **************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "detok.h"
#include "stream.h"
#include "addfcodes.h"

/* **************************************************************************
 *
 *          Internal Static Variables
 *     current_vfc_line       Line to be scanned in Vendor-FCodes buffer
 *     vfc_remainder          Remainder of Vendor-FCodes buffer to be scanned
 *     vfc_line_no            Number of current line in Vendor-FCodes buffer
 *     vfc_buf_end            Pointer to end of Vendor-FCodes buffer
 *
 **************************************************************************** */

static char *current_vfc_line;
static char *vfc_remainder;
static int vfc_line_no = 0;
static char *vfc_buf_end;

/* **************************************************************************
 *
 *      Function name:  skip_whitespace
 *      Synopsis:       Advance the given string-pointer past blanks and tabs.
 *                          (String is presumed to end before new-line)
 *
 *      Inputs:
 *         Parameters:
 *             string_line_ptr        Address of pointer to string
 *
 *      Outputs:
 *         Returned Value:            None
 *             *string_line_ptr       Advanced past blanks and tabs
 *
 **************************************************************************** */

static void skip_whitespace( char **string_line_ptr)
{
    char *cur_char_ptr = *string_line_ptr;
    for ( ; *cur_char_ptr != 0 ; cur_char_ptr++ )
    {
	if ( (*cur_char_ptr != '\t') && (*cur_char_ptr != ' ') )
	{
	    *string_line_ptr = cur_char_ptr;
	    break;
	}
    }
}

/* **************************************************************************
 *
 *      Function name:  get_next_vfc_line
 *      Synopsis:       Advance to the next  vfc_line  to be processed.
 *                      Skip blanks and comments.  Indicate when end reached.
 *
 *      Inputs:
 *         Parameters:                    None
 *         Local Static Variables:
 *             vfc_remainder
 *             vfc_buf_end
 *
 *      Outputs:
 *         Returned Value:               FALSE if reached end of buffer
 *         Local Static Variables:
 *             current_vfc_line          Advanced to next line to be scanned
 *             vfc_line_no               Kept in sync with line number in file
 *
 *      Process Explanation:
 *          Comments begin with a pound-sign  ('#') or a backslash  ('\')
 *          Comment-lines or blank or empty lines will be skipped. 
 *
 **************************************************************************** */

static bool get_next_vfc_line( void )
{
    bool retval = FALSE;   /*  TRUE = not at end yet  */
    while ( vfc_remainder < vfc_buf_end )
    {
	current_vfc_line = vfc_remainder;
	vfc_remainder = strchr( current_vfc_line, '\n');
	*vfc_remainder = 0;
	vfc_remainder++;
	vfc_line_no++;
	skip_whitespace( &current_vfc_line);
	if ( *current_vfc_line == 0 )  continue;  /*  Blank line */
	if ( *current_vfc_line == '#' )  continue;  /*  Comment  */
	if ( *current_vfc_line == '\\' )  continue;  /*  Comment  */
	retval = TRUE;
	break;  /*  Found something  */
    }
    return( retval);
}

/* **************************************************************************
 *
 *      Function name:  vfc_splash
 *      Synopsis:       Print a "splash" message to show that we
 *                          are processing Vendor-Specific FCodes,
 *                          but only once.
 *
 *      Inputs:
 *         Parameters:
 *             vf_file_name            Vendor-Specific FCodes file name
 *         Local Static Variables:
 *             did_not_splash          Control printing; once only.
 *
 *      Outputs:
 *         Returned Value:             None
 *         Local Static Variables:
 *             did_not_splash          FALSE after first call.
 *         Printout:
 *             "Splash" message...
 *
 **************************************************************************** */
static bool did_not_splash = TRUE;
static void vfc_splash( char *vf_file_name)
{
    if ( did_not_splash )
    {
	/*  Temporary substring buffer                */
	/*  Guarantee that the malloc will be big enough.  */
	char *strbfr = malloc( strlen( vf_file_name) +65 ) ;
	sprintf( strbfr,
	    "Reading additional FCodes from file:  %s\n",
		vf_file_name);
	printremark( strbfr);
	free( strbfr);
	did_not_splash = FALSE;
    }
}

/* **************************************************************************
 *
 *      Function name:  add_fcodes_from_list
 *      Synopsis:       Add Vendor-Specific FCodes from the named file
 *                          to the permanent resident dictionary.
 *
 *      Inputs:
 *         Parameters:
 *             vf_file_name            Vendor-Specific FCodes file name
 *         Global Variables:
 *             verbose                 "Verbose" flag.
 *             indata                  Start of file buffer
 *             stream_max              Size of the file buffer.
 *
 *      Outputs:
 *         Returned Value:             TRUE if FCodes have actually been added
 *         Global Variables:
 *             check_tok_seq           Cleared to FALSE, then restored to TRUE
 *         Local Static Variables:
 *             vfc_remainder           Initted to start of file buffer
 *             vfc_buf_end             Initted to end of file buffer
 *         Memory Allocated
 *             Permanent copy of FCode Name
 *         When Freed?
 *             Never.  Rmeains until program termination.
 *         Printout:
 *             If verbose, "Splash" line and count of added entries.
 *
 *      Error Detection:
 *          Fail to open or read Vendor-FCodes file -- Exit program
 *          Improperly formatted input line -- print message and ignore
 *          FCode value out of valid range -- print message and ignore
 *          FCode value already in use -- print message and ignore
 *
 *      Process Explanation:
 *          Valid lines are formatted with the FCode number first
 *              and the name after, one entry per line.  Extra text
 *              after the name will be ignored, so an extra "comment"
 *              is permitted.  The FCode number must be in hex, with
 *              an optional leading  0x  or  0X   For example:  0X407
 *          The valid range is 0x010 to 0x7ff.  Numbers above 0x800
 *              infringe upon the are reserved for FCodes generated
 *              by the tokenization process.
 *          Numbers already in use will be ignored.  A Message will be
 *              printed even if the name matches the one on the line.
 *
 **************************************************************************** */

bool add_fcodes_from_list( char *vf_file_name)
{
    bool retval = FALSE;
    int added_fc_count = 0;
    check_tok_seq = FALSE;

    if ( verbose )  vfc_splash( vf_file_name);

    if ( init_stream( vf_file_name) != 0 )
    {
	char *strbfr = malloc( strlen( vf_file_name) +65 );
	sprintf( strbfr,
	    "Could not open Additional FCodes file:  %s\n",
		vf_file_name);
	printremark( strbfr);
	free( strbfr);
	exit(1);
    }
    vfc_remainder = indata;
    vfc_buf_end = indata + stream_max -1;

    while (  get_next_vfc_line() )
    {
	char vs_fc_name[36];
	int  vs_fc_number;
	int  scan_result;
	char *lookup_result;
	char *fc_name_cpy;

	scan_result = sscanf( current_vfc_line, "0x%x %32s",
	     &vs_fc_number, vs_fc_name);

	if ( scan_result != 2 )   /*  Allow a capital  0X   */
	{
	    scan_result = sscanf( current_vfc_line, "0X%x %32s",
		&vs_fc_number, vs_fc_name);
	}
	if ( scan_result != 2 )   /*  Try it without the  0x   */
	{
	    scan_result = sscanf( current_vfc_line, "%x %32s",
		&vs_fc_number, vs_fc_name);
	}

	if ( scan_result != 2 )   /*  That's it... */
	{
	    char *strbfr = malloc( strlen( current_vfc_line) +65 );
	    vfc_splash( vf_file_name);
	    sprintf( strbfr,
		"Line #%d, invalid format.  Ignoring:  %s\n",
		    vfc_line_no, current_vfc_line);
	    printremark( strbfr);
	    free( strbfr);
	    continue;
	}

	if ( ( vs_fc_number < 0x10 ) || ( vs_fc_number > 0x7ff ) )
	{
	    char *strbfr = malloc( 85 );
	    vfc_splash( vf_file_name);
	    sprintf( strbfr,
		"Line #%d, FCode number out of range:  0x%x  Ignoring.\n",
		    vfc_line_no, vs_fc_number);
	    printremark( strbfr);
	    free( strbfr);
	    continue;
	}

	lookup_result = lookup_token( (u16)vs_fc_number);
	if ( strcmp( lookup_result, "ferror") != 0 )
	{
	    char *strbfr = malloc( strlen( lookup_result) + 85 );
	    vfc_splash( vf_file_name);
	    sprintf( strbfr,
		"Line #%d.  FCode number 0x%x is already "
		    "defined as %s  Ignoring.\n",
			vfc_line_no, vs_fc_number, lookup_result);
	    printremark( strbfr);
	    free( strbfr);
	    continue;
	}

	/*  We've passed all the tests!  */
	fc_name_cpy = strdup( vs_fc_name);
	add_token( (u16)vs_fc_number, fc_name_cpy);
	added_fc_count++;
	retval = TRUE;
    }

    if ( verbose )
    {
	char *strbfr = malloc( 85 );
        sprintf( strbfr,
	    "Added %d FCode numbers\n", added_fc_count);
	printremark( strbfr);
	free( strbfr);
    }

    close_stream();
    check_tok_seq = TRUE;
    return( retval);
}
