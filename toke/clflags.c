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
 *      Command-Line Flags are used to control certain non-Standard
 *          variant behaviors of the Tokenizer.
 *      Support Functions for setting, clearing, displaying, etc.
 *      Call them "Special-Feature Flags" in messages to the User
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *          For a given CLFlag name, the user may enter either:
 *
 *                   -f CLFlagName
 *              or
 *                   -f noCLFlagName
 *
 *          to either enable or disable the associated function
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          set_cl_flag                 Set (or clear) a CL Flag Variable
 *          show_all_cl_flag_settings   Show CL Flags' settings unconditionally.
 *          list_cl_flag_settings       Display CL Flags' settings if changed.
 *          list_cl_flag_names          Display just the names of the CL Flags.
 *          cl_flags_help               Help Message for CL Flags
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Revision History:
 *          Updated Mon, 08 Aug 2005 by David L. Paktor
 *          They're not just for setting from the Command-Line anymore,
 *              but let's still keep these names for internal use....
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *          The CL_FLAGS data structure has a field for the CLFlagName,
 *          one for a text explanation of the function it controls, and
 *          one for the address of the boolean variable ("flag")
 *
 **************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clflags.h"
#include "errhandler.h"


/* **************************************************************************
 *
 *          Global Variables Exported
 *              (The "flags" controlled by this means)
 *
 **************************************************************************** */

bool ibm_locals = FALSE ;
bool ibm_locals_legacy_separator = TRUE ;
bool ibm_legacy_separator_message = TRUE ;
bool enable_abort_quote = TRUE ;
bool sun_style_abort_quote = TRUE ;
bool sun_style_checksum = FALSE ;
bool abort_quote_throw = TRUE ;
bool string_remark_escape = TRUE ;
bool hex_remark_escape = TRUE ;
bool c_style_string_escape = TRUE ;
bool always_headers = FALSE ;
bool always_external = FALSE ;
bool verbose_dup_warning = TRUE ;
bool obso_fcode_warning = TRUE ;
bool trace_conditionals = FALSE ;
bool big_end_pci_image_rev = FALSE ;
bool allow_ret_stk_interp = TRUE ;

/*  And one to trigger a "help" message  */
bool clflag_help = FALSE;

/* **************************************************************************
 *
 *     The addition of the "upper/lower-case-tokens" flags introduces
 *         some complications.  These are the variables we will actually
 *         be exporting:
 *
 **************************************************************************** */

bool force_tokens_case       = FALSE ;
bool force_lower_case_tokens = FALSE ;

/* **************************************************************************
 *
 *         but we will be entering two static variables into the table,
 *         and keep two more to detect when a change is made...
 *
 **************************************************************************** */
static bool upper_case_tokens = FALSE ;
static bool lower_case_tokens = FALSE ;
static bool was_upper_case_tk = FALSE ;
static bool was_lower_case_tk = FALSE ;

/* **************************************************************************
 *
 *              Internal Static Variables
 *     cl_flag_change        A change was made to any of the CL Flags
 *              Internal Static Constants
 *     cl_flags_list         List of CL Flags and their data.
 *
 **************************************************************************** */

static bool cl_flag_change = FALSE;

static const cl_flag_t cl_flags_list[] = {
  /*  The clflag_tabs field takes at least one tab.
   *  If the name has fewer than 16 characters,
   *  stick in an extra tab, and yet another tab
   *  if the name is shorter than 8 characters
   *  to make the formatting of the "explanation"
   *  come out prettier.
   */
  { "Local-Values",
        &ibm_locals,
	"\t\t",
	    "Support IBM-style Local Values (\"LV\"s)"     } ,

  { "LV-Legacy-Separator",
        &ibm_locals_legacy_separator,
	"\t",
	    "Allow Semicolon for Local Values Separator (\"Legacy\")"     } ,

  { "LV-Legacy-Message",
        &ibm_legacy_separator_message,
	"\t",
	    "Display a Message when Semicolon is used as the "
		"Local Values Separator" } ,

  { "ABORT-Quote",
        &enable_abort_quote,
	"\t\t",
	    "Allow ABORT\" macro"     } ,

  { "Sun-ABORT-Quote",
        &sun_style_abort_quote,
	"\t\t",
	    "ABORT\" with implicit IF ... THEN"     } ,

  { "ABORT-Quote-Throw",
        &abort_quote_throw,
	"\t",
	    "Use -2 THROW in an Abort\" phrase, rather than ABORT"     } ,

  { "Sun-Style-Checksum",
        &sun_style_checksum,
	"\t\t",
	    "Use this for SPARC (Enterprise) platforms (especially): M3000, M4000, M9000"     } ,

  { "String-remark-escape",
        &string_remark_escape,
	"\t",
	    "Allow \"\\ (Quote-Backslash) to interrupt string parsing"     } ,

  { "Hex-remark-escape",
        &hex_remark_escape,
	"\t",
	    "Allow \\ (Backslash) to interrupt "
		"hex-sequence parsing within a string"     } ,

  { "C-Style-string-escape",
        &c_style_string_escape ,
	"\t",
	    "Allow \\n \\t and \\xx\\ for special chars in string parsing"  } ,

  { "Always-Headers",
        &always_headers ,
	"\t\t",
	    "Override \"headerless\" and force to \"headers\"" } ,

  { "Always-External",
        &always_external ,
	"\t\t",
	    "Override \"headerless\" and \"headers\" and "
		"force to \"external\"" } ,

  { "Warn-if-Duplicate",
        &verbose_dup_warning ,
	"\t",
	    "Display a WARNING message when a duplicate definition is made" } ,

  { "Obsolete-FCode-Warning",
        &obso_fcode_warning ,
	"\t",
	    "Display a WARNING message when an \"obsolete\" "
		"(per the Standard) FCode is used" } ,

  { "Trace-Conditionals",
        &trace_conditionals,
	"\t",
	    "Display ADVISORY messages about the state of "
		"Conditional Tokenization" } ,

  { "Upper-Case-Token-Names",
        &upper_case_tokens,
	"\t",
	    "Convert Token-Names to UPPER-Case" } ,


  { "Lower-Case-Token-Names",
        &lower_case_tokens,
	"\t",
	    "Convert Token-Names to lower-Case" } ,


  { "Big-End-PCI-Rev-Level",
        &big_end_pci_image_rev,
	"\t",
	    "Save the Vendor's Rev Level field of the PCI Header"
		" in Big-Endian format" } ,

  { "Ret-Stk-Interp",
        &allow_ret_stk_interp,
	"\t\t",
	    "Allow Return-Stack Operations during Interpretation" } ,


  /*  Keep the "help" pseudo-flag last in the list  */
  { "help",
        &clflag_help,
	    /*  Two extra tabs if the name is shorter than 8 chars  */
	"\t\t\t",
	    "Print this \"Help\" message for the Special-Feature Flags" }

};

static const int number_of_cl_flags =
	 sizeof(cl_flags_list)/sizeof(cl_flag_t);


/* **************************************************************************
 *
 *          CL Flags whose settings are changed in the source file should
 *              not stay in effect for the duration of the entire batch of
 *              tokenizations (i.e., if multiple input files are named on
 *              the command line) the way command-line settings should.
 *          To accomplish this we will collect the state of the flags into
 *              a bit-mapped variable after the command line has been parsed
 *              and restore them to their collected saved state before an
 *              input file is processed.
 *
 **************************************************************************** */

static long int cl_flags_bit_map;
/*  If the number of CL Flags ever exceeds the number of bits in a long
 *      (presently 32), we will need to change both this variable and
 *      the routines that use it.  Of course, if the number of CL Flags
 *      ever gets that high, it will be *seriously* unwieldy...   ;-}
 */

/* **************************************************************************
 *
 *      Function name:  adjust_case_flags
 *      Synopsis:       If the last CL Flag Variable setting changed one of
 *                          the "upper/lower-case-tokens" flags, make the 
 *                          appropriate adjustments.
 *
 *      Inputs:
 *         Parameters:                NONE
 *         Local Static Variables:
 *             was_upper_case_tk      State of "upper-case-tokens" flag before
 *                                        last CL Flag Variable was processed
 *             was_lower_case_tk      State of "lower-case-tokens" flag, before
 *             upper_case_tokens      State of "upper-case-tokens" flag after
 *                                        last CL Flag V'ble was processed
 *             lower_case_tokens      State of "lower-case-tokens" flag, after
 *         Global Variables:
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Global Variables:
 *             force_tokens_case          TRUE if "upper/lower-case-tokens"
 *                                            flag is in effect
 *             force_lower_case_tokens    If force_tokens_case is TRUE, then
 *                                            this switches between "upper"
 *                                            or "lower" case
 *
 *      Process Explanation:
 *          We cannot come out of this with both  upper_case_tokens  and
 *               lower_case_tokens  being TRUE, though they may both be FALSE.
 *          If neither has changed state, we need not do anything here.
 *              If one has gone to TRUE, we must force the other to FALSE and
 *                  we will set  force_tokens_case  to TRUE.
 *              If one has gone to FALSE, turn  force_tokens_case  to FALSE.
 *              If  force_tokens_case  is TRUE after all this, we must adjust
 *                   force_lower_case_tokens  according to  lower_case_tokens
 *
 **************************************************************************** */

static void adjust_case_flags( void)
{
    static bool *case_tokens[2] = { &upper_case_tokens, &lower_case_tokens };
    static bool *was_case_tk[2] = { &was_upper_case_tk, &was_lower_case_tk };
    int the_one = 0;
    int the_other = 1;

    for ( ; the_one < 2 ; the_one++ , the_other-- )
    {
	/*  If one has changed state   */
	if ( *(case_tokens[the_one]) != *(was_case_tk[the_one]) )
	{
	    if ( *(case_tokens[the_one]) )
	    {
	        /*  If it has gone to TRUE, force the other to FALSE.  */
		*(case_tokens[the_other]) = FALSE;
	        /*      and set  force_tokens_case  to TRUE  */
		force_tokens_case = TRUE;
	    }else{
		/*  If it has gone to FALSE turn  force_tokens_case FALSE  */
		force_tokens_case = FALSE;
	    }
	    if ( force_tokens_case )
	    {
	        force_lower_case_tokens = lower_case_tokens;
	    }
	    break;  /*  Only one can have changed state.   */
	}
    }
}




/* **************************************************************************
 *
 *      Function name:  set_cl_flag
 *      Synopsis:       Set (or clear) the named CL Flag Variable
 *
 *      Inputs:
 *         Parameters:
 *             flag_name           The name as supplied by the user
 *             from_src            TRUE if called from source-input file
 *         Static Constants:
 *             cl_flags_list
 *             number_of_cl_flags
 *
 *      Outputs:
 *         Returned Value:         TRUE if supplied name is not valid
 *         Global Variables:
 *             The CL Flag Variable associated with the supplied name will
 *                 be set or cleared according to the leading "no"
 *         Local Static Variables:
 *             cl_flag_change      TRUE if associated variable has changed
 *         Printout:
 *             If  from_src  is TRUE, print "En" or "Dis" abling:
 *                 followed by the explanation
 *
 *      Error Detection:
 *          If the supplied name is not a valid CL Flag name, or if
 *              it's too short to be a valid CL Flag name, return TRUE.
 *          Print a message;  either a simple print if this function was
 *              called from a command-line argument, or an ERROR if it
 *              was called from a line in the from source-input file.
 *
 *      Process Explanation:
 *          Save the current state of the "upper/lower-case-tokens" flags
 *          If the given name has a leading "no", make note of that fact
 *              and remove the leading "no" from the comparison.
 *          Compare with the list of valid CL Flag names.
 *          If no match was found, Error.  See under Error Detection.
 *          If a match:
 *              Change the associated variable according to the leading "no"
 *              Set  cl_flag_change  to TRUE  unless the variable is the one
 *                  associated with the "help" flag; this permits the
 *                  "Default" vs "Setting" part of  cl_flags_help() to
 *                  work properly...
 *              Do the conditional Printout (see above)
 *          Adjust the "upper/lower-case-tokens" flags if one has changed.
 *
 **************************************************************************** */
static bool first_err_msg = TRUE;  /*  Need extra carr-ret for first err msg  */
bool set_cl_flag(char *flag_name, bool from_src)
{
    bool retval = TRUE;

    was_upper_case_tk = upper_case_tokens;
    was_lower_case_tk = lower_case_tokens;

    if ( strlen(flag_name) > 3 )
    {
	int indx;
	bool flagval = TRUE;
	char *compar = flag_name;

	if ( strncasecmp( flag_name, "no", 2) == 0 )
	{
	    flagval = FALSE;
	    compar += 2;
	}
	for ( indx = 0 ; indx < number_of_cl_flags ; indx++ )
	{
	    if ( strcasecmp( compar, cl_flags_list[indx].clflag_name ) == 0 )
	    {
		retval = FALSE;
		*(cl_flags_list[indx].flag_var) = flagval;

		/*  The "help" flag is the last one in the list  */
		if ( indx != number_of_cl_flags - 1 )
		{
		    cl_flag_change = TRUE;
		}
		if ( from_src )
		{
		    tokenization_error(INFO,
		    "%sabling:  %s\n",
		    flagval ? "En" : "Dis", cl_flags_list[indx].clflag_expln);
		}
		break;
	    }
	}
    }

    if ( retval )
    {
       const char* msg_txt = "Unknown Special-Feature Flag:  %s\n" ;
       if ( from_src )
       {
           tokenization_error( TKERROR, (char *)msg_txt, flag_name);
       }else{
	   if ( first_err_msg )
	   {
	       printf( "\n");
	       first_err_msg = FALSE;
	   }
	   printf( msg_txt, flag_name);
       }
    }

    adjust_case_flags();

    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  show_all_cl_flag_settings
 *      Synopsis:       Display the settings of the CL Flags, (except "help")
 *                          regardless of whether they have been changed.
 *
 *      Associated Tokenizer directive(s):        [FLAGS]
 *                                                #FLAGS
 *                                                [#FLAGS]
 *                                                SHOW-FLAGS
 *          This routine may also be invoked by a combination of
 *              options on the command-line.
 *
 *      Inputs:
 *         Parameters:
 *             from_src                TRUE if called from source-input file
 *         Macro:
 *             ERRMSG_DESTINATION        Error message destination;
 *                                           (Development-time switch)
 *         Static Constants:
 *             cl_flags_list
 *             number_of_cl_flags
 *
 *      Outputs:
 *         Returned Value:                  NONE
 *         Printout:    Directed to  stdout or to stderr 
 *                          (see definition of ERRMSG_DESTINATION)
 *             A header line, followed by the names of the CL Flags,
 *                  with "No" preceding name if value is FALSE, one to a line.
 *
 *      Process Explanation:
 *          If from_src is TRUE, print the header line as a Message, and
 *              then direct output to  ERRMSG_DESTINATION .
 *          Don't print the "help" trigger (the last flag in the array).
 *
 **************************************************************************** */

void show_all_cl_flag_settings(bool from_src)
{
    const char* hdr_txt = "Special-Feature Flag settings:" ;
    int indx;

    if ( from_src )
    {
	tokenization_error(MESSAGE, (char *)hdr_txt);
    }else{
	printf("\n%s\n", hdr_txt);
    }

    for ( indx = 0 ; indx < (number_of_cl_flags - 1) ; indx++ )
    {
	fprintf( from_src ? ERRMSG_DESTINATION : stdout ,
	    "\t%s%s\n",
		*(cl_flags_list[indx].flag_var) ? "  " : "No" ,
		    cl_flags_list[indx].clflag_name );
    }
    if ( from_src )   fprintf( ERRMSG_DESTINATION, "\n");
}

/* **************************************************************************
 *
 *      Function name:  list_cl_flag_settings
 *      Synopsis:       Display the settings of the CL Flags, (except "help")
 *                          if any of them have been changed
 *
 *      Inputs:
 *         Parameters:                 NONE
 *         Local Static Variables:        
 *             cl_flag_change          TRUE if a Flag setting has been changed.
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Printout:
 *             Settings of the CL Flags.  See   show_all_cl_flag_settings()
 *
 *      Process Explanation:
 *          Don't print anything if  cl_flag_change  is not TRUE
 *
 **************************************************************************** */

void list_cl_flag_settings(void)
{

    if ( cl_flag_change )
    {
	show_all_cl_flag_settings( FALSE);
    }
}


/* **************************************************************************
 *
 *      Function name:  list_cl_flag_names
 *      Synopsis:       Display just the names of the CL Flags
 *                      for the Usage message
 *
 *      Inputs:
 *         Parameters:                      NONE
 *         Static Constants:
 *             cl_flags_list                
 *             number_of_cl_flags           
 *
 *      Outputs:
 *         Returned Value:                  NONE
 *         Printout:
 *             A header line, followed by the names of the CL Flags,
 *
 **************************************************************************** */

void list_cl_flag_names(void)
{
    int indx;

    printf("Valid Special-Feature Flags are:\n");
    for ( indx = 0 ; indx < number_of_cl_flags ; indx++ )
    {
        printf("\t%s\n", cl_flags_list[indx].clflag_name );
    }
}

/* **************************************************************************
 *
 *      Function name:  cl_flags_help
 *      Synopsis:       Display Usage of the CL Flags and their defaults
 *                      
 *
 *      Inputs:
 *         Parameters::                      NONE
 *         Static Constants:
 *             cl_flags_list
 *             number_of_cl_flags
 *         Local Static Variables:
 *             cl_flag_change                TRUE if setting has been changed.
 *
 *      Outputs:
 *         Returned Value:                   NONE
 *         Printout:
 *             A few lines of header, followed by the default, the name
 *             and the "explanation" of each of the CL Flags, one to a line.
 *
 *      Extraneous Remarks:
 *          We take advantage of the facts that this routine is called
 *              (1) only from the command-line, before any settings
 *              have been changed, and (2) via changing the flag for
 *              "help" to TRUE.  (Technically, I suppose, the default
 *              for the "help" feature is "no", but showing will, I
 *              think be more confusing than enlightening to the user.)
 *          Also, I suppose a perverse user could change setting(s) on
 *              the same command-line with a "-f help" request; we cannot
 *              stop users from aiming at their boot and pulling the
 *              trigger.  As my buddies in Customer Support would say:
 *              "KMAC YOYO"  (Approximately, "You're On Your Own, Clown")...
 *
 *      Revision History:
 *          Oh, all right.  If the user changed setting(s), we can do
 *              them the minor courtesy of printing "Setting" instead
 *              of "Default".
 *
 *
 **************************************************************************** */

void cl_flags_help(void )
{
    int indx;

    printf("\n"
           "Special-Feature Flags usage:\n"
           "  -f   FlagName   to enable the feature associated with FlagName,\n"
           "or\n"
           "  -f noFlagName   to disable the feature.\n\n"
                "%s   Flag-Name\t\t  Feature:\n\n",
	            cl_flag_change ? "Setting" : "Default" );

   for ( indx = 0 ; indx < number_of_cl_flags ; indx++ )
    {
	printf(" %s    %s%s%s\n",
	    *(cl_flags_list[indx].flag_var) ? "  " : "no" ,
	    cl_flags_list[indx].clflag_name,
	    cl_flags_list[indx].clflag_tabs,
	    cl_flags_list[indx].clflag_expln);
    }

}



/* **************************************************************************
 *
 *      Function name:  save_cl_flags
 *      Synopsis:       Collect the state of the CL Flags
 *
 *      Inputs:
 *         Parameters:                     NONE
 *         Local Static Variables:
 *             cl_flags_list
 *         Static Constants:
 *             number_of_cl_flags
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Local Static Variables:
 *             cl_flags_bit_map            Will be set to reflect the state
 *                                             of the CL Flags in the list.
 *
 *      Process Explanation:
 *          The correspondence of bits to the list is that the first item
 *              in the list corresponds to the low-order bit, and so on
 *              moving toward the high-order with each successive item.
 *          Do not save the "help" flag (last item on the list).
 *          This routine is called after the command line has been parsed.
 *
 **************************************************************************** */

void save_cl_flags(void)
{
    int indx;
    long int moving_bit = 1;

    cl_flags_bit_map = 0;
    for ( indx = 0 ; indx < (number_of_cl_flags - 1) ; indx++ )
    {
	if ( *(cl_flags_list[indx].flag_var) )
	{
	    cl_flags_bit_map |= moving_bit;  /*  The moving finger writes,  */
	}
	moving_bit <<= 1;                    /*  and having writ, moves on. */
    }
}

/* **************************************************************************
 *
 *      Function name:  reset_cl_flags
 *      Synopsis:       Restore the CL Flags to the state that was saved.
 *
 *      Inputs:
 *         Parameters:                     NONE
 *         Local Static Variables:
 *             cl_flags_bit_map            Reflects the state of the CL Flags
 *         Static Constants:
 *             number_of_cl_flags
 *
 *      Outputs:
 *         Returned Value:                  NONE
 *         Local Static Variables:
 *             cl_flags_list
 *         Global Variables:
 *             The CL Flag Variables will be set or cleared
 *                 to their saved state
 *
 *      Process Explanation:
 *          This routine is called before starting a new input file.
 *              Any changes made in the source file will not stay
 *              in effect for the next tokenization.
 *
 **************************************************************************** */

void reset_cl_flags(void)
{
    int indx;
    long int moving_bit = 1;

    for ( indx = 0 ; indx < (number_of_cl_flags - 1) ; indx++ )
    {
	*(cl_flags_list[indx].flag_var) =
	    BOOLVAL( cl_flags_bit_map & moving_bit) ;
	moving_bit <<= 1;
    }
}
