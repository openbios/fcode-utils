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
 *      Error-Handler for Tokenizer
 *
 *      Controls printing of various classes of errors
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          init_error_handler  Initialize the error-counts,
 *                                  announce the file names.
 *          tokenization_error  Handle an error of the given class,
 *                                  print the given message in the 
 *                                  standard format.
 *          started_at          Supplemental message, giving a back-reference
 *                                  to the "starting"  point of a compound
 *                                  error, including last-colon identification.
 *          just_started_at     Supplemental back-reference to "starting"  point
 *                                  of compound error, but without last-colon
 *                                  identification.
 *          where_started       Supplemental message, giving a more terse back-
 *                                  -reference to "start" of compound-error.
 *          just_where_started Supplemental message, more terse back-reference,
 *                                  without last-colon identification.
 *          in_last_colon      Supplemental back-reference message,
 *                                  identifying last Colon-definition.
 *          safe_malloc         malloc with built-in failure test.
 *          error_summary       Summarize final error-message status
 *                                  before completing tokenization.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Revision History:
 *          Updated Fri, 13 Oct 2006 by David L. Paktor
 *          Added "(Output Position ..." to standard message format.
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *          We will define a set of bit-valued error-types and a
 *          global bit-mask.  Each error-message will be associated
 *          with one of the bit-valued error-types.  The bit-mask,
 *          which will be set by a combination of defaults and user
 *          inputs (mainly command-line arguments), will control
 *          whether an error-message of any given type is printed.
 *          
 *          Another bit-mask variable will accumulate the error-
 *          types that occur within any given run; at the end of
 *          the run, it will be examined to determine if the run
 *          failed, i.e., if the output should be suppressed.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *          Error-types fall into the following broad categories:
 *              FATAL           Cause to immediately stop activity
 *              TKERROR         Sufficient to make the run a failure,
 *                                  but not to stop activity.
 *              WARNING         Not necessarily an error, but something
 *                                  to avoid.  E.g., it might rely on
 *                                  assumptions that are not necessarily
 *                                  what the user/programmer wants.  Or:
 *                                  It's a deprecated feature, or one
 *                                  that might be incompatible with
 *                                  other standard tokenizers.
 *
 *          Other types of Messages fall into these broad categories:
 *              INFO            Nothing is changed in processing, but
 *                                  an advisory is still in order.  Omitted
 *                                  if "verbose" is not specified.
 *              MESSAGE         Message generated by the user.  (Complete;
 *                                  new-line will be added by display routine.)
 *              P_MESSAGE       Partial Message -- Instigated by user, but
 *                                  pre-formatted and not complete.  New-line
 *                                  will be added by follow-up routine.
 *              TRACER          Message related to the trace-symbols option;
 *                                  either a creation or an invocation message.
 *
 **************************************************************************** */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "types.h"
#include "toke.h"
#include "stream.h"
#include "emit.h"
#include "errhandler.h"
#include "scanner.h"

/* **************************************************************************
 *
 *          Global Variables Imported
 *              iname           Name of file currently being processed
 *              lineno          Current line-number being processed
 *              noerrors        "Ignore Errors" flag, set by "-i" switch
 *              opc             FCode Output Buffer Position Counter
 *              pci_hdr_end_ob_off
 *                              Position in FCode Output Buffer of
 *                                   end of last PCI Header Block structure
 *              verbose         If true, enable Advisory Messages
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *              Internal Static Variables
 *          print_msg               Whether beginning of a message was printed;
 *                                      therefore, whether to print the rest.
 *          errs_to_print           Error Verbosity Mask.  Bits set correspond
 *                                      to message-types that will be printed
 *                                      May be altered by Command-Line switches.
 *          err_types_found         Accumulated Error-types.  Bits
 *                                      set correspond to error-types
 *                                      that have occurred.
 *          message_dest            Message Dest'n.  Usually ERRMSG_DESTINATION
 *                                      (stdout) except when we need to switch.
 *          err_count               Count of Error Messages
 *          warn_count              Count of Warning Messages
 *          info_count              Count of "Advisory" Messages
 *          user_msg_count          Count of User-generated Messages
 *          trace_msg_count         Count of Trace-Note Messages
 *          fatal_err_exit          Exit code to be used for "Fatal" error.
 *                                       This is a special accommodation
 *                                       for the  safe_malloc  routine.
 *
 **************************************************************************** */

static bool  print_msg ;
static int errs_to_print = ( FATAL | TKERROR | WARNING | 
                             MESSAGE | P_MESSAGE | TRACER | FORCE_MSG ) ;
static int err_types_found =  0 ;
static int err_count       =  0 ;
static int warn_count      =  0 ;
static int info_count      =  0 ;
static int user_msg_count  =  0 ;
static int trace_msg_count =  0 ;
static int fatal_err_exit  = -1 ;
static FILE *message_dest;     /*  Would like to init to  ERRMSG_DESTINATION
				*      here, but the compiler complains...
				*/

/* **************************************************************************
 *
 *              Internal Static Constant Structure
 *          err_category            Correlate each error-type code with its
 *                                      Counter-variable and the printable
 *                                      form of its name.
 *          num_categories          Number of entries in the err_category table
 *
 **************************************************************************** */

typedef struct {
    int  type_bit ;		/*  Error-type single-bit code        */
    char *category_name ;	/*  Printable-name base               */
    char *single ;		/*  Suffix to print singular of name  */
    char *plural ;		/*  Suffix to print plural of name    */
    int  *counter ;		/*  Associated Counter-variable       */
    bool new_line ;		/*  Whether to print new-line at end  */
} err_category ;

static const err_category  error_categories[] = {
    /*  FATAL  must be the first entry in the table.   */
    /*  No plural is needed; only one is allowed....   */
    { FATAL,    "Fatal Error", "", "",     &err_count      , TRUE  },

    { TKERROR,    "Error"     , "", "s",    &err_count      , FALSE },
    { WARNING,    "Warning"   , "", "s",    &warn_count     , FALSE },
    { INFO,       "Advisor"   , "y", "ies", &info_count     , FALSE },
    { MESSAGE ,   "Message"   , "", "s",    &user_msg_count , TRUE  },
    { P_MESSAGE , "Message"   , "", "s",    &user_msg_count  , FALSE },
    { TRACER , "Trace-Note"   , "", "s",    &trace_msg_count , FALSE }
};

static const int num_categories =
    ( sizeof(error_categories) / sizeof(err_category) );


/* **************************************************************************
 *
 *      Function name:  toup
 *      Synopsis:       Support function for  strupper
 *                      Converts one character
 *
 *      Inputs:
 *         Parameters:
 *             chr_ptr                 Pointer to the character
 *
 *      Outputs:
 *         Returned Value:             None
 *         Supplied Pointers:
 *             The character pointed to is changed
 *
 *      Process Explanation:
 *          Because this fills in a lack in the host system, we cannot
 *              rely on the functions  islower  or  toupper , which are
 *              usually built-in but might be similarly missing.
 *
 **************************************************************************** */

static void toup( char *chr_ptr)
{
    const unsigned char upcas_diff = ( 'a' - 'A' );
    if ( ( *chr_ptr >= 'a' ) && ( *chr_ptr <= 'z' ) )
    {
	*chr_ptr -= upcas_diff ;
    }
}

/* **************************************************************************
 *
 *      Function name:  strupper
 *      Synopsis:       Replacement for  strupr  on systems that don't 
 *                      seem to have it.  A necessary hack.
 *
 *      Inputs:
 *         Parameters:
 *             strung              Pointer to the string to be changed
 *
 *      Outputs:
 *         Returned Value:         Same pointer that was passed in
 *         Supplied Pointers:
 *             The string pointed to will be converted to upper case
 *
 *      Process Explanation:
 *          Because it fills in a lack in the host system, this routine
 *              does not rely on the functions  islower  or  toupper
 *              which are usually built-in but might be missing.
 *
 **************************************************************************** */

char *strupper( char *strung)
{
    char *strindx;
    for (strindx = strung; *strindx != 0; strindx++)
    {
        toup( strindx);
    }
    return strung;
}

/* **************************************************************************
 *
 *     If  strupr  is missing, it's a good bet that so is  strlwr 
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Function name:  tolow
 *      Synopsis:       Support function for  strlower
 *                      Converts one character
 *
 *      Inputs:
 *         Parameters:
 *             chr_ptr                 Pointer to the character
 *
 *      Outputs:
 *         Returned Value:             None
 *         Supplied Pointers:
 *             The character pointed to is changed
 *
 *      Process Explanation:
 *          Because this fills in a lack in the host system, we cannot
 *              rely on the functions  isupper  or  tolower , which are
 *              usually built-in but might be similarly missing.
 *
 **************************************************************************** */

static void tolow( char *chr_ptr)
{
    const unsigned char lowcas_diff = ( 'A' - 'a' );
    if ( ( *chr_ptr >= 'A' ) && ( *chr_ptr <= 'Z' ) )
    {
	*chr_ptr -= lowcas_diff ;
    }
}

/* **************************************************************************
 *
 *      Function name:  strlower
 *      Synopsis:       Replacement for  strlwr  on systems that don't 
 *                      seem to have it.  A necessary hack.
 *
 *      Inputs:
 *         Parameters:
 *             strung              Pointer to the string to be changed
 *
 *      Outputs:
 *         Returned Value:         Same pointer that was passed in
 *         Supplied Pointers:
 *             The string pointed to will be converted to lower case
 *
 *      Process Explanation:
 *          Because it fills in a lack in the host system, this routine
 *              does not rely on the functions  isupper  or  tolower
 *              which are usually built-in but might be missing.
 *
 **************************************************************************** */

char *strlower( char *strung)
{
    char *strindx;
    for (strindx = strung; *strindx != 0; strindx++)
    {
        tolow( strindx);
    }
    return strung;
}


/* **************************************************************************
 *
 *      Function name:  init_error_handler
 *      Synopsis:       Initialize the error-handler before starting a
 *                          new tokenization; both the aspects that will
 *                          persist across the entire run and those that
 *                          need to be reset, such as error-counts.
 *
 *      Inputs:
 *         Parameters:                 NONE
 *         Global Variables: 
 *              verbose                Set by "-v" switch
 *         Macro:
 *             ERRMSG_DESTINATION      Error message destination;
 *                                         (Set by development-time switch)
 *             FFLUSH_STDOUT           Flush STDOUT if err-msg-dest is STDERR
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Global Variables:
 *             errs_to_print           Add the INFO bit if verbose is set
 *         Local Static Variables:
 *             message_dest            Point it at ERRMSG_DESTINATION (stderr)
 *           Reset the following to zero:
 *             err_types_found         Accumulated Error-types.
 *             err_count               Count of Error Messages
 *             warn_count              Count of Warning Messages
 *             info_count              Count of "Advisory" Messages
 *             user_msg_count          Count of User-generated Messages
 *             trace_msg_count         Count of Trace-Note Messages
 *         Other Exotic Effects:
 *             Flush stdout if Error message destination is not stdout, to
 *                 avoid collisions with stderr once Error Messaging begins.
 *
 *      Extraneous Remarks:
 *          This needs to be done before attempting to read the input file,
 *              so that any Messages that occur there can be properly counted.
 *
 **************************************************************************** */

void init_error_handler( void)
{
    int indx ;

    message_dest  =  ERRMSG_DESTINATION;
    if ( verbose )  errs_to_print |= INFO ;
    err_types_found = 0 ;

    /*  Start at indx = 1 to skip resetting FATALs   */
    for ( indx = 1; indx < num_categories ; indx ++ )
    {
	*(error_categories[indx].counter) = 0 ;
    }

    FFLUSH_STDOUT
}

/* **************************************************************************
 *
 *      Function name:    tokenization_error
 *      Synopsis:         Handle an error of the given class,
 *                            print the given message in the standard format.
 *      
 *      Inputs:
 *         Parameters:
 *             err_type       int        One of the bit-valued error-types
 *             The remaining parameters are a format string and corresponding
 *                 data objects such as would be sent to  printf() 
 *         Global Variables:
 *             errs_to_print        Error Verbosity Mask.
 *             iname                Name of file currently being processed
 *             lineno               Current line-number being processed
 *             fatal_err_exit       Exit code for "Fatal" error, if applicable.
 *             opc                  FCode Output Buffer Position Counter
 *             pci_hdr_end_ob_off
 *                                  Position in FCode Output Buffer of end
 *                                       of last PCI Header Block structure

 *         Macro:
 *             ERRMSG_DESTINATION        Error message destination;
 *                                           (Development-time switch)
 *         Note:  Whether this routine will or will not supply a new-line
 *             at the end of the printout depends on the category of the
 *             message.  The new-line is included for a FATAL or a User-
 *             Generated Message, and excluded for the rest.  For those,
 *             the calling routine must be responsible for including a
 *             new-line at the end of the format string or for otherwise
 *             finishing the line, as by calling started_at()
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Local Static Variables:
 *             err_types_found             Accumulated Error-types.
 *             print_msg                   Whether this message was printed;
 *                                             may be used by started_at()
 *                    One of the following Category Counters
 *                         will be incremented, as applicable:
 *             err_count
 *             warn_count
 *             info_count 
 *             user_msg_count
 *         Printout:    Directed to  stdout or stderr 
 *                          (see definition of ERRMSG_DESTINATION)
 *
 *      Error Detection:
 *              Err_type not in list
 *                      Print special message; treat cause as an Error.
 *                      Force printout.
 *
 *      Process Explanation:
 *          Accumulated the Error-type into  err_types_found 
 *          Identify the Error-Category:
 *              Check the Error-Type against the bit-code.
 *                  The Error-type may have more than one bit set,
 *                  but if it matches the Category bit-code, it's it.
 *              If it doesn't match any Error-Category bit-code, print
 *                  a special message and treat it as an ERROR code.
 *          Check the Error-Type against the Error Verbosity Mask;
 *          If it has a bit set, print the Error-Category, together
 *                  with the source-file name and line number, and
 *                  the rest of the message as supplied.
 *              The table that translates the Error-type into a printable
 *                  Error-Category string also identifies the applicable
 *                  Category Counter; increment it.
 *          Of course, there's no return from a FATAL error; it exits.
 *          The Message will show:
 *              The Error-Category (always)
 *              The Input File-name and Line Number (if input file was opened)
 *              The Output Buffer Position (if output has begun)
 *              The PCI-Block Position (if different from Output Buffer Pos'n)
 *
 **************************************************************************** */

void tokenization_error( int err_type, char* msg, ... )
{
    int indx ;

    /*  Initial settings:  treat as an Error.  */
    char *catgy_name = "Error";
    char *catgy_suffx = "";
    int *catgy_counter = &err_count;
    bool print_new_line = FALSE;

    /*  Accumulated the Error-type into  err_types_found  */
    err_types_found |= err_type;

    /*  Identify the Error-Category.  */
    for ( indx = 0 ; indx < num_categories ; indx ++ )
    {
        if ( ( error_categories[indx].type_bit & err_type ) != 0 )
        {
            catgy_name = error_categories[indx].category_name;
            catgy_suffx = error_categories[indx].single;
            catgy_counter = error_categories[indx].counter;
	    print_new_line = error_categories[indx].new_line;
            break;
        }
    }

    /*  Special message if  err_type  not in list; treat as an Error.  */
    if ( catgy_name == NULL )
    {
         fprintf(ERRMSG_DESTINATION,
	      "Program error: Unknown Error-Type, 0x%08x.  "
              "  Will treat as Error.\n", err_type) ;
         err_types_found |= TKERROR;
         print_msg = TRUE ;
    } else {
         /*  Check the Error-Type against the Error Verbosity Mask  */
         print_msg = BOOLVAL( ( errs_to_print & err_type ) != 0 );
    }

    if ( print_msg )
    {
        va_list argp;

	fprintf(ERRMSG_DESTINATION, "%s%s:  ",
             catgy_name, catgy_suffx);
        if ( iname != NULL )
	{
	    /*  Don't print iname or lineno if no file opened.  */
	    fprintf(ERRMSG_DESTINATION, "File %s, Line %d.  ",
        	 iname, lineno);
	}
        if ( opc > 0 )
	{
	    /*  Don't print Output Position if no output started.  */
	    fprintf(ERRMSG_DESTINATION, "(Output Position = %d).  ", opc);
	}
        if ( pci_hdr_end_ob_off > 0 )
	{
	    /*  Don't print PCI-Block Position if no PCI-Block in effect.  */
	    fprintf(ERRMSG_DESTINATION, "(PCI-Block Position = %d).  ",
	        opc - pci_hdr_end_ob_off );
	}

        va_start(argp, msg);
        vfprintf(ERRMSG_DESTINATION, msg, argp);
        va_end(argp);
	if ( print_new_line ) fprintf(ERRMSG_DESTINATION, "\n");

	/*   Increment the category-counter.  */
	*catgy_counter += 1;
    }
    if ( err_type == FATAL )
    {
        fprintf(ERRMSG_DESTINATION, "Tokenization terminating.\n");
        error_summary();
        exit ( fatal_err_exit );
    }
}

/* **************************************************************************
 *
 *      Function name:  print_where_started
 *      Synopsis:       Supplemental message, following a tokenization_error,
 *                          giving a back-reference to the "start" point of
 *                          the compound-error being reported.
 *                      This is a retro-fit; it does the heavy lifting for
 *                          the routines  started_at() ,  just_started_at() , 
 *                           where_started() ,  just_where_started() and
 *                           in_last_colon() .
 *
 *      Inputs:
 *         Parameters:
 *             show_started         Whether to print a phrase about "started"
 *             show_that_st         Whether to print "that started" as opposed
 *                                      to " , which started"
 *             saved_ifile          File-name saved for "back-reference"
 *             saved_lineno         Line-number saved for "back-reference"
 *             may_show_incolon     Whether to allow a call to  in_last_colon()
 *                                      Needed to prevent infinite recursion...
 *         Global Variables:        
 *             iname                Name of file currently being processed
 *             lineno               Current line-number being processed
 *         Local Static Variables:
 *             print_msg            Whether the beginning part of the message
 *                                      was printed by tokenization_error()
 *             message_dest         Message Destination. Is ERRMSG_DESTINATION
 *                                      (stdout) usually, except sometimes...
 *
 *      Outputs:
 *         Returned Value:          None
 *         Printout:
 *             The remainder of a message:  the location of a back-reference.
 *                 The phrase "that started" is switchable.  This routine
 *                 will supply the leading space and a new-line; the routines
 *                 that call this can be used to finish the line.
 *
 *      Process Explanation:
 *          This routine is called immediately after tokenization_error()
 *              If tokenization_error() didn't print, neither will we.
 *              The residual state of  print_msg  will tell us that.
 *          If the preceding message ended with something general about a
 *              "Colon Definition" or "Device-Node" or the like, we want
 *              the message to read:  "that started on line ... [in file ...]"
 *          If the end of the preceding message was something more specific,
 *              we just want the message to read:  "on line ... [in file ...]"
 *          If the saved input file name doesn't match our current input
 *              file name, we will print it and the saved line-number.
 *          If the file name hasn't changed, we will print only the saved
 *              line-number.
 *          If neither is changed, there's no point in printing any of the
 *              above-mentioned text.    
 *          If a Colon-definition is in progress, show its name and the
 *              line on which it started.  Protect against infinite loop!
 *          End the line.
 *
 *      Extraneous Remarks:
 *          This is a retrofit.  Earlier, it was just  started_at() .  Later,
 *              I generated more specific messages, and needed a way to leave
 *              out the "that started".  I could, theoretically, have added
 *              the extra parameter to  started_at() , but by now there are
 *              so many of calls to it that I'd rather leave them as is, and
 *              just change the name of the routine in the few places that
 *              need the terser form of the message.
 *
 **************************************************************************** */

static void print_where_started( bool show_started,
                                   bool show_that_st,
				   char * saved_ifile,
				       unsigned int saved_lineno,
                                           bool may_show_incolon)
{
    if ( print_msg )
    {
	bool fil_is_diff;
	bool lin_is_diff;

	/*  File names are case-sensitive  */
	fil_is_diff = BOOLVAL(strcmp(saved_ifile, iname) != 0 );
	lin_is_diff = BOOLVAL(saved_lineno != lineno );
	if ( fil_is_diff || lin_is_diff )
	{
	    if ( show_started )
	    {
		if ( show_that_st )
		{
		    fprintf(message_dest, " that");
		}else{
		    fprintf(message_dest, " , which");
		}
		fprintf(message_dest, " started");
	    }
	    fprintf(message_dest, " on line %d", saved_lineno);
	    if ( fil_is_diff )
	    {
	        fprintf(message_dest, " of file %s", saved_ifile);
	    }
	}

	if ( may_show_incolon )
	{
	    in_last_colon( TRUE );
	}else{
	    fprintf(message_dest, "\n");
	}
    }
}

/* **************************************************************************
 *
 *      Function name:  started_at
 *      Synopsis:       Supplemental back-reference message,
 *                          with the "that started"  phrase,
 *                          and with last-colon identification.
 *
 *      Inputs:
 *         Parameters:
 *             saved_ifile          File-name saved for "back-reference"
 *             saved_lineno         Line-number saved for "back-reference"
 *
 *      Outputs:
 *         Returned Value:          None
 *         Global Variables:
 *         Printout:
 *             The "...started at..." remainder of a message, giving a back-
 *                 -reference to the  "start" point supplied in the params,
 *                 and the start of the current Colon-definition if one is
 *                 in effect.
 *             Will supply a new-line and can be used to finish the line.
 *
 **************************************************************************** */

void started_at( char * saved_ifile, unsigned int saved_lineno)
{
    print_where_started( TRUE, TRUE, saved_ifile, saved_lineno, TRUE);
}


/* **************************************************************************
 *
 *      Function name:  print_started_at
 *      Synopsis:       Same as started_at() except output will be directed
 *                          to  stdout  instead of to ERRMSG_DESTINATION
 *
 *      Extraneous Remarks:
 *          A retrofit.  Can you tell?
 *
 **************************************************************************** */
 
void print_started_at( char * saved_ifile, unsigned int saved_lineno)
{
    message_dest = stdout;
	started_at( saved_ifile, saved_lineno);
    message_dest = ERRMSG_DESTINATION;
}


/* **************************************************************************
 *
 *      Function name:  just_started_at
 *      Synopsis:       Supplemental back-reference message,
 *                          with the "that started"  phrase,
 *                          but without last-colon identification.
 *
 *      Inputs:
 *         Parameters:
 *             saved_ifile          File-name saved for "back-reference"
 *             saved_lineno         Line-number saved for "back-reference"
 *
 *      Outputs:
 *         Returned Value:          None
 *         Global Variables:
 *         Printout:
 *             The "...started at..." remainder of a message, giving a back-
 *                 -reference to the  "start" point supplied in the params,
 *                 and no more.
 *             Will supply a new-line and can be used to finish the line.
 *
 **************************************************************************** */

void just_started_at( char * saved_ifile, unsigned int saved_lineno)
{
    print_where_started( TRUE, TRUE, saved_ifile, saved_lineno, FALSE);
}

/* **************************************************************************
 *
 *      Function name:  where_started
 *      Synopsis:       Supplemental back-reference message,
 *                          without the "that started"  phrase,
 *                          but with last-colon identification.
 *
 *      Inputs:
 *         Parameters:
 *             saved_ifile          File-name saved for "back-reference"
 *             saved_lineno         Line-number saved for "back-reference"
 *
 *      Outputs:
 *         Returned Value:          None
 *         Global Variables:
 *         Printout:
 *             The remainder of a message, giving a back-reference to the
 *                 "start" point supplied in the parameters, and the start
 *                 of the current Colon-definition if one is in effect.
 *             Will supply a new-line and can be used to finish the line.
 *
 **************************************************************************** */

void where_started( char * saved_ifile, unsigned int saved_lineno)
{
    print_where_started( FALSE, FALSE, saved_ifile, saved_lineno, TRUE);
}

/* **************************************************************************
 *
 *      Function name:  just_where_started
 *      Synopsis:       Supplemental back-reference message,
 *                          without the "that started"  phrase,
 *                          and without last-colon identification.
 *
 *      Inputs:
 *         Parameters:
 *             saved_ifile          File-name saved for "back-reference"
 *             saved_lineno         Line-number saved for "back-reference"
 *
 *      Outputs:
 *         Returned Value:          None
 *         Global Variables:
 *         Printout:
 *             The remainder of a message, giving a back-reference to the
 *                 "start" point supplied in the parameters, and no more.
 *             Will supply a new-line and can be used to finish the line.
 *
 **************************************************************************** */

void just_where_started( char * saved_ifile, unsigned int saved_lineno)
{
    print_where_started( FALSE, FALSE, saved_ifile, saved_lineno, FALSE);
}

/* **************************************************************************
 *
 *      Function name:  in_last_colon
 *      Synopsis:       Supplemental back-reference message, identifying
 *                          last Colon-definition if one is in effect.
 *                      Can be used to finish the line in either case.
 *
 *      Inputs:
 *         Parameters:
 *             say_in                    If TRUE, lead phrase with " in ".
 *                                           If FALSE, print even if not
 *                                            inside a Colon-def'n.
 *         Global Variables:
 *             incolon                   TRUE if Colon-definition is in progress
 *             last_colon_defname        Name of last colon-definition
 *             last_colon_filename       File where last colon-def'n made
 *             last_colon_lineno         Line number of last colon-def'n
 *         Local Static Variables:
 *             print_msg            Whether the beginning part of the message
 *                                      was printed by tokenization_error()
 *             message_dest         Message Destination. Is ERRMSG_DESTINATION
 *                                      (stdout) usually, except sometimes...
 *
 *      Outputs:
 *         Returned Value:                  NONE
 *         Printout:
 *             Remainder of a message:
 *                "in definition of  ... , which started ..."
 *
 *      Process Explanation:
 *          Because this routine does some of its own printing, it needs
 *              to check the residual state of  print_msg  first.
 *          The calling routine does not need to test   incolon ; it can
 *              call this (with TRUE) to end the line in either case.
 *
 **************************************************************************** */

void in_last_colon( bool say_in )
{
    if ( print_msg )
    {
	if ( incolon || ( ! say_in ) )
	{
	    fprintf( message_dest, "%s definition of  %s ", say_in ? " in" : "",
		strupr( last_colon_defname) );
	    print_where_started( TRUE, FALSE,
		last_colon_filename, last_colon_lineno, FALSE);
	}else{
	    fprintf(message_dest, "\n");
	}
    }
}


/* **************************************************************************
 *
 *      Function name:  safe_malloc
 *      Synopsis:       malloc with built-in failure test.
 *      
 *      Inputs:
 *         Parameters:
 *             size       size_t     Size of memory-chunk to allocate
 *             phrase     char *     Phrase to print after "... while "
 *                                       in case of failure.
 *
 *      Outputs:
 *         Returned Value:           Pointer to allocated memory
 *         Global Variables:
 *             fatal_err_exit       On memory allocation failure, change
 *                                       to a special system-defined value
 *
 *      Error Detection:
 *          On memory allocation failure, declare a FATAL error.  Set up
 *              for a special system-defined EXIT value that indicates
 *              insufficient memory.
 *
 *      Process Explanation:
 *          It is the responsibility of the calling routine to be sure
 *              the "phrase" is unique within the program.  It is intended
 *              as a debugging aid, to help localize the point of failure.
 *
 **************************************************************************** */

_PTR safe_malloc( size_t size, char *phrase)
{
    _PTR retval ;
    retval = malloc (size);
    if ( !retval )
    {
        fatal_err_exit = -ENOMEM ;
        tokenization_error( FATAL, "Out of memory while %s.", phrase);
    }
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:         error_summary
 *      Synopsis:              Summarize final error-message status
 *                                 before completing tokenization.
 *                             Indicate if OK to produce output.
 *      
 *      Inputs:
 *         Parameters:                   NONE
 *         Global Variables:        
 *             noerrors             "Ignore Errors" flag, set by "-i" switch
 *             err_types_found      Accumulated Error-types.
 *             error_categories     Table of Error-types, Message-Counters
 *                                      and their printable names.
 *             opc                  FCode Output Buffer Position Counter
 *                                      (zero means there was no output).
 *
 *      Outputs:
 *         Returned Value:          True = OK to produce output (But caller
 *                                      must still verify non-zero opc)
 *         Printout:
 *             Various messages.
 *
 *      Process Explanation:
 *          The first entry in the error_categories table is FATAL    
 *              We won't need to print a tally of that...
 *      
 **************************************************************************** */

bool error_summary( void )
{
    /*  Bit-mask of error-types that require suppressing output   */
    static const int suppress_mask = ( FATAL | TKERROR );
    bool retval = TRUE;
    bool suppressing = FALSE;

    /*  There's no escaping a FATAL error   */
    if ( ( err_types_found & FATAL ) != 0 )
    {
	/*   FATAL error.  Don't even bother with the tally.   */
	suppressing = TRUE;
    } else {

	if ( opc == 0 )
	{
	    printf ( "Nothing Tokenized");
	}else{
	    printf ( "Tokenization Completed");
	}

	if ( err_types_found != 0 )
	{
	    int indx;
	    bool tally_started = FALSE ;
	    printf (". ");
	    /*
	     *  Print a tally of the error-types;
	     *  handle plurals and punctuation appropriately.
	     */
	    /*  Start at indx = 1 to skip examining FATALs   */
	    for ( indx = 1; indx < num_categories ; indx ++ )
	    {
		if ( *(error_categories[indx].counter) > 0 )
		{
		    printf ("%s %d %s%s",
	        	tally_started ? "," : "" ,
			    *(error_categories[indx].counter),
				error_categories[indx].category_name,
				    *(error_categories[indx].counter) > 1 ?
					 error_categories[indx].plural :
					     error_categories[indx].single );
		    /*  Zero out the counter, to prevent displaying the
		     *      number of Messages twice, since it's shared
		     *      by the "Messages" and "P_Messages" categories.
		     */
		    *(error_categories[indx].counter) = 0;
		    tally_started = TRUE;
		}
	    }
	}
        printf (".\n");

	if ( ( err_types_found & suppress_mask ) != 0 )
	{    /*  Errors found.  Not  OK to produce output    */
             /*  Unless "Ignore Errors" flag set...          */
	    if ( INVERSE(noerrors) )
            {
		suppressing = TRUE;
            }else{
		if ( opc > 0 )
		{
		    printf ("Error-detection over-ridden; "
				"producing binary output.\n");
		}
            }
	}
    }
    if ( suppressing )
    {
	retval = FALSE ;
	printf ("Suppressing binary output.\n");
    }
    return ( retval );
}

