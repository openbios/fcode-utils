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
 *      Functions for Managing FCode Assignment Pointer and for
 *          Detection of Overlapping-FCode Error in Tokenizer
 *
 *      (C) Copyright 2006 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      The fact that the user is able to specify, at any time, the
 *          next FCode number to be assigned introduces the risk that
 *          a symbol might inadvertently be assigned an FCode that is
 *          still in use by another symbol, which could lead to bizarre
 *          and hard-to-trace failures at run-time.
 *
 *      This module will support a means whereby to detect and report,
 *          as an Error, overlapping FCode assignments.
 *
 *      The task is further complicated by the fact that, under some
 *          circumstances, "recycling" a range of FCodes can be done
 *          safely, and the programmer may do so intentionally.
 *
 *      We will define a way for the programmer to specify that FCode
 *          assignments are being intentionally "recycled", and that
 *          overlaps with previously-assigned FCodes are not to be
 *          treated as Errors.  We will not attempt to analyze whether
 *          it is, indeed, safe to do so; the programmer who does this
 *          is presumed to be sufficiently responsible.
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Functions Exported:
 *          reset_fcode_ranges   (Re-)Initialize the range of assignable
 *                                   FCode numbers and clear the records
 *                                   of prevously-assigned ranges of FCodes
 *          list_fcode_ranges    Display a final report of assigned FCode
 *                                   ranges at end of tokenization or when
 *                                   the  reset_fcodes()  routine is called.
 *          set_next_fcode       Set the start of a new Range of FCode
 *                                   numbers to be assigned.
 *          assigning_fcode      The next FCode number is about to be assigned;
 *                                   test for out-of-bounds, overlap, etc.
 *                                   errors.
 *          bump_fcode           Increment the next FCode number prior to the
 *                                    next assignment.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Revision History:
 *          06 Jun 06  -- Need became apparent after verification test
 *                            against existing device-drivers.
 *              
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Still to be done:
 *          Detect and report when the Current Range stops overlapping
 *              one particular Range and starts overlapping another.
 *
 *          Detect and report when the Current Range overlaps more than
 *              one Range at a time (e.g., if other Ranges themselves
 *              overlap, and the Current Range is stepping through them)
 *              but, again, only display one message per overlapped Range.
 *
 **************************************************************************** */



/* **************************************************************************
 *
 *          Global Variables Imported
 *              iname              Name of current input file
 *              lineno             Line Number within current input file
 *
 **************************************************************************** */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "toke.h"
#include "nextfcode.h"
#include "errhandler.h"
#include "stream.h"
#include "scanner.h"


/* **************************************************************************
 *
 *          Global Variables Exported
 *              nextfcode          The next FCode-number to be assigned
 *
 **************************************************************************** */

u16  nextfcode;         /*  The next FCode-number to be assigned              */

/* **************************************************************************
 *
 *      Internal (Static) Structure:
 *          fcode_range_t         The record of a Range of assigned FCodes
 *
 *   Fields:
 *       fcr_start            FCode number at the start of a this Range
 *       fcr_end              Last FCode number in this Range that has
 *                                actually been assigned.  0 if none made yet.
 *       fcr_infile           File name of where this Range was started
 *       fcr_linenum          Line number where this Range was started
 *       fcr_not_lapped       TRUE if an overlap error has not been reported
 *                                against this Range.  Use this to prevent
 *                                issuing an Error message every time an
 *                                overlapping FCode is assigned; once per
 *                                Range is sufficient.
 *       fcr_next             Pointer to following entry in linked-list.
 *
 *   We will keep the list of assigned FCode Ranges as a forward-linked,
 *       rather than the more usual backward-linked, list for convenience
 *       at listing-time.  Otherwise, it doesn't make much difference.
 *
 **************************************************************************** */

typedef struct fcode_range
    {
        u16                 fcr_start;
	u16                 fcr_end ;
	char               *fcr_infile;
	int                 fcr_linenum;
	bool                fcr_not_lapped;
	struct fcode_range *fcr_next;
    }  fcode_range_t ;


/* **************************************************************************
 *
 *          Internal Static Variables
 *     ranges_exist              TRUE if FCode Ranges have been created;
 *                                   Prevents unnecessary overlap checking.
 *     changes_listed            TRUE if no changes to any of the Ranges
 *                                   have taken place since the last time
 *                                   a Listing was displayed.  Prevents
 *                                   unnecessary repetions (e.g., if a
 *                                    reset_fcode_ranges()  was called
 *                                   right after an operation that causes
 *                                   a Listing
 *
 *          These four apply if no FCode Ranges have been created:
 *
 *     range_start               First FCode in the current first (only) range.
 *     range_end                 Last FCode in current first (only) range that
 *                                   has actually been assigned.  0 if none yet.
 *     first_fcr_infile          File name of where first (only) range started.
 *     first_fcr_linenum         Line number where first (only) range started.
 *
 *          These two apply after FCode Ranges have been created:
 *
 *     first_fc_range            Pointer to the first entry in the linked
 *                                   list of FCode Ranges.
 *     current_fc_range          Pointer to the entry in the linked list of
 *                                   Ranges that contains the Current Range.
 *
 **************************************************************************** */

static bool           ranges_exist      = FALSE;
static bool           changes_listed    = FALSE;

static u16            range_start       = FCODE_START;
static u16            range_end         = 0;
static char          *first_fcr_infile  = NULL;
static int            first_fcr_linenum = 0;

static fcode_range_t *first_fc_range    = NULL;
static fcode_range_t *current_fc_range  = NULL;

/* **************************************************************************
 *
 *      Function name:  reset_fcode_ranges
 *      Synopsis:       (Re-)Initialize the range of assignable FCode numbers
 *                          and clear the records of prevously-assigned Ranges
 *
 *      Inputs:
 *         Parameters:                  NONE
 *         Global Variables:
 *             iname                    Name of current input file
 *             lineno                   Line Number within current input file
 *         Local Static Variables:
 *             ranges_exist             TRUE if FCode Ranges have been created
 *             first_fc_range           Pointer to First FCode Range
 *             current_fc_range         Pointer to the current Range
 *
 *      Outputs:
 *         Returned Value:              NONE
 *         Global Variables:
 *             nextfcode                Initialized to Standard start value
 *                                          for user-assigned FCodes
 *         Local Static Variables:
 *             ranges_exist             Cleared to FALSE
 *             changes_listed           Reset to FALSE
 *             range_start              Reset to Standard start value
 *             range_end                Reset to 0
 *             first_fcr_infile         Copy of iname
 *             first_fcr_linenum        Copy of lineno
 *             first_fc_range           Reset to NULL
 *             current_fc_range         Reset to NULL
 *         Memory Allocated
 *             For  first_fcr_infile , which will have a copy of  iname
 *         When Freed?
 *             The next time this routine is called.
 *         Memory Freed
 *             Any FCode Ranges that were in effect will be freed, and the
 *                 memory for their  fcr_infile  fields will be freed.
 *             If  iname  has changed, memory for  first_fcr_infile  will
 *                 be freed before the new copy is made.
 *
 *      Process Explanation:
 *          This will be called either as part of normal initialization
 *              or in response to a User-issued directive.
 *          In the former case, it may be called twice:  once before any
 *              Source is examined, and once again in response to the first
 *              (and only to the first) invocation of an FCode-Starter, at
 *              which time the input file name and line number will be updated.
 *          In the latter case, the calling routine will be responsible for
 *              displaying any Advisory Messages and listings of previously
 *              assigned FCode Ranges.
 *
 **************************************************************************** */

void reset_fcode_ranges( void)
{
    if ( ranges_exist )
    {
	while ( current_fc_range != NULL )
	{
	    current_fc_range = first_fc_range->fcr_next;
	    free( first_fc_range->fcr_infile);
	    free( first_fc_range);
	    first_fc_range = current_fc_range;
	}
	ranges_exist = FALSE;
    }else{
	if ( first_fcr_infile != NULL )
	{
	    if ( strcmp( first_fcr_infile, iname) != 0 )
	    {
		free( first_fcr_infile);
		first_fcr_infile = NULL;
	    }
	}
    }

    changes_listed    = FALSE;
    range_start       = FCODE_START;
    range_end         = 0;

    if ( first_fcr_infile == NULL )
    {
	first_fcr_infile = strdup( iname);
    }
    first_fcr_linenum = lineno;
    nextfcode         = FCODE_START;
}

/* **************************************************************************
 *
 *      Function name:  list_fcode_ranges
 *      Synopsis:       Display an Advisory of assigned FCode ranges at
 *                          end of tokenization or upon reset_fcodes() 
 *
 *      Inputs:
 *         Parameters:
 *             final_tally               TRUE if printing a final tally at
 *                                           end of tokenization.  
 *         Global Variables:
 *             verbose                   Do not print anything if not set.
 *             nextfcode                 FCode next after last-assigned
 *         Local Static Variables:
 *             changes_listed            If TRUE, only print new-line.
 *             ranges_exist              TRUE if more than one Range exists.
 *             range_start               FCode at start of only range
 *             range_end                 Last FCode in only range; 0 if none.
 *             first_fc_range            Ptr to start of FCode Ranges list
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Local Static Variables:
 *             changes_listed           Set to TRUE. 
 *         Printout:
 *            Printed to Standard Out on final tally, or to STDERR otherwise.
 *            One of three formats:
 *            1   No FCodes assigned.
 *            2   Last assigned FCode = 0xXXX
 *            3   FCodes assigned:
 *                    [ No FCodes assigned in the range that started ... ]
 *                    [ From 0xXXX to 0xYYY ** in the range that started ... ]
 *                        (** = Indicator if the Range has an Overlap Error.)
 *
 *      Process Explanation:
 *          This is called to complete an Advisory or Standard-Out Message
 *              that doesn't end in a new-line.
 *
 *      Extraneous Remarks:
 *          If we ever decide not to keep entries for Ranges in which no
 *              assignments were made, let's not remove the code that lists
 *              them.  It's harmless to keep it around, and we remain ready...
 *
 **************************************************************************** */

void list_fcode_ranges( bool final_tally)
{
    if ( verbose )
    {
	FILE *message_dest = ( final_tally ? stdout : ERRMSG_DESTINATION );
	if ( changes_listed )
	{
	    fprintf(message_dest, "\n");
	}else{
	    changes_listed = TRUE;

	    if ( INVERSE(ranges_exist) )
	    {   /*  List the first and only range  */
		if ( range_end == 0 )
		{
		    fprintf(message_dest, "No FCodes assigned.\n");
		}else{
		    if ( range_start == FCODE_START )
		    {
			fprintf(message_dest,
			    "Last assigned FCode = 0x%x\n", range_end);
		    }else{
			fprintf(message_dest,
			    "FCodes assigned:  0x%x to 0x%x\n",
				range_start, range_end);
		    }
		}
		/*  We are done listing the first and only range  */
	    }else{   /*  List the collection of Ranges  */

		/*  Pionter to function returning void  */
		typedef void (*vfunct)();

		/*  Function for the  started_at()  part of the message  */
		vfunct start_at_funct =
	            ( final_tally ? print_started_at : started_at );


		fcode_range_t *next_range = first_fc_range;

		fprintf(message_dest, "FCodes assigned:\n");

		while ( next_range != NULL )
		{
		    if ( next_range->fcr_end == 0 )
		    {
			fprintf(message_dest, "    None assigned");
		    }else{
			fprintf(message_dest, "    From 0x%x to 0x%x",
			    next_range->fcr_start, next_range->fcr_end );
			if ( INVERSE( next_range->fcr_not_lapped) )
			{
			    fprintf(message_dest, " ***Overlap***" );
			}
		    }
		    fprintf(message_dest, " in the range");
		    (*start_at_funct)(
			next_range->fcr_infile, next_range->fcr_linenum );

		    next_range = next_range->fcr_next;
		}
	    }
	}
    }
}


/* **************************************************************************
 *
 *      Function name:  set_next_fcode
 *      Synopsis:       Set the start of a new Range of FCode numbers
 *
 *      Inputs:
 *         Parameters:
 *             new_fcode                 Start of the new range of FCodes
 *         Global Variables:
 *             nextfcode                 The next FCode-number to be assigned
 *             iname                     Name of current input file
 *             lineno                    Line Number within current input file
 *         Local Static Variables:
 *             ranges_exist              TRUE if FCode Ranges have been created
 *             range_start               First FCode in the current (first
 *                                           and only) range
 *             range_end                 Last FCode in only range; 0 if none.
 *             current_fc_range          Pointer to the current Range if there
 *                                           are more than one.
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Global Variables:
 *             nextfcode                 Set to the start of the new range
 *         Local Static Variables:
 *             ranges_exist              May be set to TRUE
 *             first_fc_range            May be initialized
 *             current_fc_range          May be initialized or moved
 *             first_fcr_infile          May be updated, may be made NULL
 *             first_fcr_linenum         May be updated, may be made irrelevant
 *             range_start               May be set to start of the new range
 *                                           or rendered irrelevant
 *             range_end                 Reset to 0 (by  reset_fcode_ranges() )
 *             changes_listed            Reset to FALSE
 *         Memory Allocated
 *             For new Range data structure; for copy of  iname
 *         When Freed?
 *             By reset_fcode_ranges()
 *
 *      Error Detection:
 *          Not here.
 *          The calling routine will detect and report attempts to set an
 *             illegal new range. 
 *          Overlap with earlier Ranges will be detected and reported when
 *              the FCode is actually Assigned.
 *
 *      Process Explanation:
 *          The calling routine will not call this routine with a new starting
 *              FCode that is not legal per the Standard.
 *          It may call with a new starting FCode that is equal to  nextfcode
 *          If no Ranges exist yet, and the new starting FCode is equal to
 *              the current value of  nextfcode , this is a continuation of
 *              the first and only range; do not change the file name or line
 *              number; just go away.
 *          If no Ranges exist yet, and no FCode assignments have been made
 *              in the current range, then this is a new start for the first
 *              and only range; detect the latter condition by range_end == 0
 *              Call the  reset_fcode_ranges()  routine to update the file name
 *              and line number, then update the remaining variables for the
 *              current (first and only) range, and you are done here.
 *          If no Ranges exist yet, and if FCode assignments _have_ been made
 *              in the current (first and only) range, create a data structure
 *              for the first (and now no longer only) Range and point both
 *              the  first_fc_range  and  current_fc_range  pointers at it.
 *              Set the  ranges_exist  flag to TRUE.
 *          If Ranges exist, whether from being newly-created, above, or from
 *              earlier, create a data structure for the new Current Range
 *              and move the  current_fc_range  pointer to point at it.  If
 *              the new starting FCode is equal to  nextfcode  we still want
 *              to create a new Range that will be listed separately.
 *          If no assignments were made within the Current Range, we will not
 *              overwrite or delete it; it will be listed at the appropriate
 *              time, and will be harmless in the overlap test.
 *
 *      Extraneous Remarks:
 *          We will trade off the strict rules of structured code here,
 *              in exchange for ease of coding.
 *
 **************************************************************************** */

void set_next_fcode( u16  new_fcode)
{
    if ( INVERSE( ranges_exist) )
    {    /*  The current range is the first and only.  */

	if ( new_fcode == nextfcode )
	{
	    /*  Do nothing here  */
	    return;
	}

	if ( range_end == 0 )
	{   /*  No FCode assignments have been made in the current range  */
	    /*  This is still the first and only range.                   */

	    reset_fcode_ranges();   /*  Update file name and line number  */
	    range_start = new_fcode;
	    nextfcode = new_fcode;

	    /*  We are done here  */
	    return;

	}else{  /*  Create the data structure for the first Range  */
	    first_fc_range = safe_malloc( sizeof( fcode_range_t),
				 "creating first FCode Range" );
	    first_fc_range->fcr_start        = range_start;
	    first_fc_range->fcr_end          = range_end;
	    first_fc_range->fcr_infile       = first_fcr_infile;
	    first_fc_range->fcr_linenum      = first_fcr_linenum;
	    first_fc_range->fcr_not_lapped   = TRUE;
	    first_fc_range->fcr_next         = NULL;

	    first_fcr_infile  = NULL;
	    first_fcr_linenum = 0;
	    range_start       = FCODE_START;
	    range_end         = 0;

	    current_fc_range  = first_fc_range;

	    ranges_exist      = TRUE;
	}
    }

    /*  Previous Ranges exist now for sure!  */
    current_fc_range->fcr_next  = safe_malloc( sizeof( fcode_range_t),
				      "creating new FCode Range" );
    current_fc_range = current_fc_range->fcr_next;

    nextfcode                         = new_fcode;
    current_fc_range->fcr_start       = nextfcode;
    current_fc_range->fcr_end         = 0;
                                  /*  Will be filled in by first assignment  */
    current_fc_range->fcr_infile      = strdup( iname);
    current_fc_range->fcr_linenum     = lineno;
    current_fc_range->fcr_not_lapped  = TRUE;
    current_fc_range->fcr_next        = NULL;

    changes_listed                    = FALSE;
}


/* **************************************************************************
 *
 *      Function name:  find_overlap
 *      Synopsis:       Compare the FCode under test against existing
 *                          FCode Ranges and return a pointer to the
 *                          Range against which it overlaps, if any.
 *
 *      Inputs:
 *         Parameters:
 *             test_fcode                 FCode to be tested
 *         Local Static Variables:
 *             ranges_exist               If not TRUE, no need to test
 *             first_fc_range             Start of Ranges to test
 *             current_fc_range           Do not test Current Range
 *
 *      Outputs:
 *         Returned Value:                Pointer to FCode Range in which an
 *                                            overlap exists, or NULL if none.
 *
 *      Error Detection:
 *          The calling routine will treat any findings as it deems appropriate.
 *
 *      Process Explanation:
 *          A Range within which no assignments were made will never "hit"
 *              the overlap test because its  fcr_end  field will be zero
 *              and its  fcr_start  field will be non-zero; there's no
 *              number that will be "between" them!
 *
 **************************************************************************** */

static fcode_range_t *find_overlap( u16 test_fcode)
{
    fcode_range_t *retval = NULL;
    if ( ranges_exist )
    {
	fcode_range_t *test_range = first_fc_range;
	while ( test_range != current_fc_range )
	{
	    if ( ( test_fcode <= test_range->fcr_end ) &&
	         ( test_fcode >= test_range->fcr_start )  )
	    {
		retval = test_range;
		break;
	    }
	    test_range = test_range->fcr_next;
	}
    }

    return( retval);
}


/* **************************************************************************
 *
 *      Function name:  assigning_fcode
 *      Synopsis:       Commit the next FCode number to be assigned;
 *                          test for out-of-bounds, overlap, etc. errors.
 *
 *      Inputs:
 *         Parameters:
 *             
 *         Global Variables:
 *             nextfcode            The FCode-number to be assigned
 *         Local Static Variables:
 *             ranges_exist         TRUE if FCode Ranges have been created
 *             first_fc_range       First entry in linked list of Ranges.
 *             current_fc_range     List entry for Current Range.
 *
 *      Outputs:
 *         Returned Value:                  NONE
 *         Local Static Variables:
 *             changes_listed               Reset to FALSE
 *                    One of these two will be set to  nextfcode 
 *             range_end                       ... if  ranges_exist  is FALSE
 *             current_fc_range->fcr_end       ... if  ranges_exist  is TRUE
 *
 *      Error Detection:
 *          FATAL if the value of  nextfcode  is larger than the legal
 *              maximum for an FCode
 *          ERROR if the value of  nextfcode   falls within one of the
 *              existing Ranges (other than the current one, of course)
 *
 **************************************************************************** */

void assigning_fcode( void)
{
    if ( nextfcode > FCODE_LIMIT )
    {
	/*  Let's give a last summarization before we crap out */
	tokenization_error( INFO, "");
	list_fcode_ranges( FALSE);

	tokenization_error( FATAL,
	    "Too many definitions.  "
	    "Assigned FCode exceeds limit "
	    "specified by IEEE-1275.");
	/*
	 *  No need to  return()  from here.
	 *  FATAL error exits program.
	 */
    }

    changes_listed = FALSE;

    if ( INVERSE(ranges_exist) )
    {    /*  No Overlap Error checking needed here.  */
	range_end = nextfcode;
    }else{
	current_fc_range->fcr_end = nextfcode;

	/*   Detect and report Overlap Error only once per Range  */
	if ( current_fc_range->fcr_not_lapped )
	{
	    fcode_range_t *found_lap = find_overlap( nextfcode);
	    if ( found_lap != NULL )
	    {
		tokenization_error( TKERROR,
		    "Assigning FCode of 0x%x, "
			"which overlaps the range", nextfcode);
		started_at( found_lap->fcr_infile, found_lap->fcr_linenum);

		current_fc_range->fcr_not_lapped = FALSE;
	    }
	}
    }

}


/* **************************************************************************
 *
 *      Function name:  bump_fcode
 *      Synopsis:       Increment the next assignable FCode number
 *                          prior to the next assignment.
 *
 *      Inputs:
 *         Parameters:                   NONE
 *         Global Variables:
 *             nextfcode                 The next FCode-number to be assigned
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Global Variables:
 *             nextfcode                 Incremented
 *
 *      Extraneous Remarks:
 *          This looks like a no-brainer now, but if we ever need this
 *              function to perform any more sophisticated background
 *              activity, we can limit the scope of our modifications
 *              to this routine.
 *
 **************************************************************************** */

void bump_fcode( void)
{
    nextfcode++;
}
