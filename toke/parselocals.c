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
 *      Parsing functions for IBM-style Local Values
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          declare_locals      Pick up the Locals' names after the {
 *          handle_local        Insert the code to access a Local
 *          exists_as_local     Confirm whether a name is in the Locals vocab
 *          assign_local        Process the "Assign to a Local" operator ( -> )
 *          finish_locals       Insert the code for exiting a routine
 *                                  that uses locals
 *          forget_locals       Remove the locals' names from the search   
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *          These are the names of the three routines that will be invoked
 *          when Locals are used.  Their definitions exist in a separate
 *          Local Values Support FCode source-file that must be FLOADed
 *          into the user's tokenization source.
 *
 **************************************************************************** */

/*  Note that the enclosing curly-braces are part of the name  */
static const char* push_locals = "{push-locals}"; /* ( #ilocals #ulocals -- ) */
static const char* pop_locals = "{pop-locals}";   /* ( #locals -- )           */
static const char* local_addr = "_{local}";       /* ( local# -- addr )       */

/*  Switchable Fetch or Store operator to apply to  local_addr.   */
static const char* local_op = "@";   /*  Initially Fetch  */


/* **************************************************************************
 *
 *      Revision History:
 *          Updated Wed, 13 Jul 2005 by David L. Paktor
 *              Command-line control for:
 *                  Support for Locals in general
 *                  Whether to accept the "legacy" separator (semicolon)
 *                  Whether to issue a message for the "legacy" separator
 *          Updated Tue, 10 Jan 2006 by David L. Paktor
 *              Convert to  tic_hdr_t  type vocabulary.
 *
 **************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parselocals.h"
#include "ticvocab.h"
#include "dictionary.h"
#include "scanner.h"
#include "errhandler.h"
#include "clflags.h"
#include "stream.h"
#include "emit.h"
#include "devnode.h"
#include "flowcontrol.h"
#include "tracesyms.h"

/* **************************************************************************
 *
 *      Global Variables Imported
 *          statbuf
 *          pc    
 *          opc
 *          incolon
 *          lastcolon
 *          ibm_locals_legacy_separator      Accept ; as the "legacy" separator
 *          ibm_legacy_separator_message     Issue a message for "legacy" sep'r
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *          Internal Static Variables
 *              local_names        Vocabulary for new local-names
 *              num_ilocals        Number of initialized local variables
 *              num_ulocals        Number of uninitialized local variables
 *              localno            Running Local-Number to be assigned
 *              eval_buf           Internally-generated string to be parsed
 *              l_d_lineno         Locals Declaration Line Number
 *
 **************************************************************************** */

static tic_hdr_t *local_names = NULL;
static int num_ilocals = 0;
static int num_ulocals = 0;
static int localno = 0;
static char eval_buf[64];
static unsigned int l_d_lineno;  	/*  For Error Messages   */

/* **************************************************************************
 *
 *     The  local_names  vocabulary follows the same  tic_hdr_t   structure
 *         as the dictionaries of tokens, special-functions, etcetera.   Its
 *         "parameter field" is an integer, used to store the Local's number,
 *         an its "function" is invoke_local(), defined further below.
 *
 *     The vocabulary is initially empty, so there's no need for an "init"
 *         or a "reset" routine.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Function name:  int_to_str
 *      Synopsis:       Convert an integer into a compilable string.
 *                      Suport routine for invoke_local().
 *
 *      Inputs:
 *         Parameters:
 *             num             The number to convert
 *             bufr            The buffer into which to place it.
 *                                 Needn't be very long:
 *                                 five at least, ten is enough
 *
 *      Outputs:
 *         Returned Value:     Pointer to  bufr
 *             bufr            Contents are changed.
 *
 *      Process Explanation:
 *          Convert into decimal.  If the number is greater than 8,
 *              prepend a  d#  in front of it.  If less, don't.
 *              We specifically want to avoid a  d#  in front of
 *              the numbers 0 1 2 and 3, which are also named constants;
 *              there's no need to treat 'em as literals.
 *          The calling routine will be responsible for allocating
 *              and freeing the buffer.
 *
 *      Extraneous Remarks:
 *          Too bad  atoi()  isn't a Standard C function; I could convert
 *             using the current base, and be guaranteed that it would be
 *             interpreted in the same base.
 *          Instead, I have to fiddle-faddle around with  d#  ...
 *
 **************************************************************************** */

static char *int_to_str( int num, char *bufr)
{
     char* prefix = "d# ";
     if ( num < 8 ) prefix = "";
     sprintf(bufr,"%s%d",prefix, num);
     return (bufr);
}



/* **************************************************************************
 *
 *      Function name:  invoke_local
 *      Synopsis:       Compile-in the code to access the Local whose
 *                      assigned Number is given.  This function is
 *                      entered into the Local-Names Vocabulary entry.
 *      
 *      Inputs:
 *         Parameters:
 *             pfield              The Vocabulary entry's Param field, taken
 *                                     from the Assigned Number of the Local.
 *         Local Static Variables:
 *             local_addr          Name of  _{local}  routine, invoked
 *                                     when a Local is used 
 *             local_op            Fetch or Store operator to apply.
 *
 *      Outputs:
 *         Returned Value:         None
 *         Local Static Variables:
 *             eval_buf            Phrase constructed here; will become new
 *                                     Source Input Buffer, temporarily
 *
 *      Error Detection:
 *          If the Local Values Support FCode source-file was not
 *              FLOADed into the user's tokenization source, then
 *              the function  _{local}  will be an "unknown name".
 *
 *      Process Explanation:
 *          We are going to generate a string of the form:
 *                  " #local _{local} OP"
 *              and pass it to the Parser for evaluation.
 *          The call to  _{local}  is preceded by its parameter, which is
 *               its Assigned Local-Number, and followed by the appropriate
 *               OPerator, which will be "Fetch" if the Local's name was
 *               invoked by itself, or "Store" if its invocation was made
 *               in conjuction with the  ->  operator.
 *          The string-buffer may be local, but it must be stable.
 *
 *      Revision History:
 *      Updated Thu, 24 Mar 2005 by David L. Paktor
 *          Factored-out to permit  lookup_local()  to be a "pure"
 *          function that can be used for duplicate-name detection.
 *      Updated Tue, 10 Jan 2006 by David L. Paktor
 *          Accommodate conversion to  tic_hdr_t  type vocabulary.
 *
 **************************************************************************** */

static void invoke_local( tic_param_t pfield )
{
    char local_num_buf[10];
    int loc_num = (int)pfield.deflt_elem;

    int_to_str(loc_num, local_num_buf);
    sprintf( eval_buf, "%s %s %s", local_num_buf, local_addr, local_op );
    eval_string( eval_buf);

}


/* **************************************************************************
 *
 *      Function name:  locals_separator
 *      Synopsis:       Test whether the given character is the separator
 *                          between initted and uninitted Local Names.
 *                      Optionally, allow Semi-Colon as a separator and issue
 *                          an optional Advisory.
 *
 *      Inputs:
 *         Parameters:
 *             subj     One-character "subject" of the test
 *         Global Variables:    
 *             ibm_locals_legacy_separator     Allow Semi-Colon as a separator?
 *             ibm_legacy_separator_message    Issue an Advisory message?
 *
 *      Outputs:
 *         Returned Value:   TRUE if the character is the separator
 *
 *      Error Detection:
 *          If the separator is Semi-Colon, and  ibm_locals_legacy_separator 
 *              is TRUE, then if  ibm_legacy_separator_message  is TRUE,
 *              issue an Advisory message.
 *          If the flag to allow Semi-Colon is FALSE, then simply do not
 *              acknowledge a valid separator.  Other routines will report
 *              an erroneous attempt to use an already-defined symbol.
 *
 *      Revision History:
 *          Updated Wed, 13 Jul 2005 by David L. Paktor
 *          Bring the questions of whether to accept semicolon as a separator
 *              -- and whether to issue a message for it -- under the control 
 *              of external flags (eventually set by command-line switches),
 *              rather than being hard-compiled.
 *
 *      Extraneous Remarks:
 *          In the interest of avoiding too deeply nested "IF"s, I will
 *              not be adhering strictly to the rules of structure.
 *
 **************************************************************************** */

static bool locals_separator( char subj )
{
    bool retval = FALSE;
    /*  Is it the preferred (i.e., non-legacy) separator?   */
    if ( subj == '|' )
    {  
	retval = TRUE;
	return ( retval );
    }
	
    if ( ibm_locals_legacy_separator )
    {
	if ( subj == ';' )
	{
	    retval = TRUE;
	    if ( ibm_legacy_separator_message )
	    {
		tokenization_error ( WARNING , "Semicolon as separator in "
		    "Locals declaration is deprecated in favor of '|'\n");
	    }
	}
    }
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  add_local
 *      Synopsis:       Given a pointer to a name and a number, enter
 *                      them into the vocabulary for new Local names.
 *      
 *      Inputs:
 *         Parameters:
 *             lnum              The assigned number
 *             lname             Pointer to the name
 *         Local Static Variables:
 *             local_names       The vocabulary for new Local names
 *
 *      Outputs:
 *         Returned Value:       NONE
 *         Local Static Variables:
 *             local_names       Enter the new Local's name and number.
 *         Memory Allocated:
 *             A place into which the name will be copied
 *         When Freed?
 *             When  forget_locals()  routine frees up all memory
 *                 allocations in the "Local Names" Vocabulary.
 *
 *      Process Explanation:
 *          Allocate a stable place in memory for the name, via  strdup().
 *          The entry's "action" will be the  invoke_local()  function,
 *              defined above.  The "parameter field" size is zero.
 *
 **************************************************************************** */

static void add_local( TIC_P_DEFLT_TYPE lnum, char *lname)
{
    char *lnamecopy ;

    lnamecopy = strdup( lname);
    add_tic_entry( lnamecopy, invoke_local, lnum,
		       LOCAL_VAL, 0, FALSE,  NULL,
			       &local_names );
}


/* **************************************************************************
 *
 *      Function name:  gather_locals
 *      Synopsis:       Collect Local names, for both initted and uninitted
 *                      Return an indication as to whether to continue
 *                      gathering Locals' names
 *      
 *      Inputs:
 *         Parameters:
 *             initted        TRUE if we are gathering initted Local names.
 *             counter        Pointer to variable that's counting names.
 *         Global Variables:
 *             statbuf        The symbol just retrieved from the input stream.
 *         Local Static Variables:
 *             localno        Running Local-Number to be assigned
 *             l_d_lineno     Line # of Locals Declar'n start (for err mssg)
 *
 *      Outputs:
 *         Returned Value:    TRUE = Ended with initted/uninitted separator
 *         Local Static Variables:
 *             localno        Incremented for each Local name declared
 *             local_names    Enter the new locals' names into the Vocabulary.
 *                            Numeric field is assigned local number.
 *
 *      Error Detection:
 *          A Local-name that duplicates an existing name is an ERROR.
 *              Especially if that name is <Semicolon> and the flag
 *              called  ibm_locals_legacy_separator  was not set.
 *          Issue an Error if close-curly-brace terminator is not found,
 *              or if imbedded comment is not terminated, before end of file.
 *          If the Separator is found a second-or-more time, issue an Error
 *              and continue collecting uninitted Local names.
 *
 *      Revision History:
 *      Updated Thu, 24 Mar 2005 by David L. Paktor
 *          Allow comments to be interspersed among the declarations.
 *          Error-check duplicate Local-name.
 *      Updated Wed, 30 Mar 2005 by David L. Paktor
 *          Warning when name length exceeds ANSI-specified max (31 chars). 
 *      Updated Thu, 07 Jul 2005 by David L. Paktor
 *          Protect against PC pointer-overrun due to unterminated
 *              comment or declaration.
 *          Error-check for numbers.
 *          No name-length check; doesn't go into FCode anyway.
 *
 **************************************************************************** */

static bool gather_locals( bool initted, int *counter )
{
    signed long wlen;
    bool retval = FALSE;

    while ( TRUE )
    {
        wlen = get_word();

	if ( wlen <= 0 )
	{
	    warn_unterm( TKERROR, "Local-Values Declaration", l_d_lineno);
	    break;
	}

	/*  Allow comments to be interspersed among the declarations.  */
	if ( filter_comments( statbuf) )
	{
	    /*  Unterminated and Multi-line checking already handled   */
	    continue;
	}
	/*  Is this the terminator or the separator? */
	if ( wlen == 1 )    /*  Maybe   */
	{
	    /*  Check for separator   */
	    if (locals_separator( statbuf[0] ) )
	    {
	        /*  If gathering initted Local names, separator is legit  */
	        if ( initted )
		{
		    retval = TRUE;
		    break;
		}else{
		    tokenization_error ( TKERROR,
		        "Excess separator -- %s -- found "
			    "in Local-Values declaration", statbuf);
		    in_last_colon( TRUE);
		    continue;
		}
	    }
	    /*  Haven't found the separator.  Check for the terminator  */
	    if ( statbuf[0] == '}' )
	    {
		break;
	    }
	}
	/*  It was not the terminator or the separator  */
	{
	    long tmp;
	    char *where_pt1;  char *where_pt2;
	    /*  Error-check for duplicated names  */
	    if ( word_exists ( statbuf, &where_pt1, &where_pt2 ) )
	    {
		tokenization_error ( TKERROR, "Cannot declare %s "
		    "as a Local-Name; it's already defined %s%s",
			statbuf, where_pt1, where_pt2 );
		show_node_start();
		continue;
	    }
	    /*  Error-check for numbers.  */
	    if ( get_number(&tmp) )
	    {
		tokenization_error ( TKERROR, "Cannot declare %s "
		    "as a Local-Name; it's a number.\n", statbuf );
		continue;
	    }

	    /*  We've got a valid new local-name    */
	    /*  Don't need to check name length; it won't go into the FCode  */

	    /*  Increment our counting-v'ble  */
	    *counter += 1;

	    /*  Define our new local-name in the Locals' vocabulary  */
	    add_local( localno, statbuf );

	    /*  Bump the running Local-Number  */
	    localno++;

	}
    }
    return ( retval );
}


/* **************************************************************************
 *
 *      Function name:  activate_locals
 *      Synopsis:       Compile-in the call to  {push-locals}  that
 *                      the new definition under construction will need,
 *                      now that the Locals have been declared.
 *      
 *      Inputs:
 *         Parameters:                NONE
 *         Global Variables:
 *             num_ilocals              First argument to  {push-locals}
 *             num_ulocals              Second argument to  {push-locals}
 *             push_locals              Name of  {push-locals}  routine.
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Local Static Variables:
 *             eval_buf               Phrase constructed here; will become
 *                                        new Source Input Buffer, temporarily
 *
 *      Error Detection:
 *          If the Local Values Support FCode source-file was not
 *              FLOADed into the user's tokenization source, then
 *              the function  {push-locals}  will be an "unknown name".
 *
 *      Process Explanation:
 *          We are going to generate a string of the form:
 *                  " #ilocals #ulocals {push-locals}"
 *              and pass it to the Parser for evaluation.
 *          The string-buffer may be local, but it must be stable.
 *
 *      Question under consideration.:
 *          Do we want to check if  {push-locals}  is an unknown name,
 *              and give the user a hint of what's needed?  And, if so,
 *              do we do it only once, or every time?
 *
 **************************************************************************** */

static void activate_locals( void )
{
    char ilocals_buf[10];
    char ulocals_buf[10];
     
    int_to_str(num_ilocals, ilocals_buf ); 
    int_to_str(num_ulocals, ulocals_buf );
    sprintf( eval_buf,"%s %s %s",ilocals_buf, ulocals_buf, push_locals);
    eval_string( eval_buf);
}

/* **************************************************************************
 *
 *      Function name:  error_check_locals
 *      Synopsis:       Indicate whether Locals declaration is erronious
 *      
 *      Inputs:
 *         Parameters:     NONE
 *         Global Variables:
 *             incolon             TRUE if colon def'n is in effect.
 *             opc                 FCode Output buffer Position Counter
 *             lastcolon           Value of opc when Colon def'n was started
 *
 *      Outputs:
 *         Returned Value:         TRUE if found errors severe enough to
 *                                     preclude further processing of Decl'n
 *
 *      Errors Detected:
 *           Colon definition not in effect.  ERROR and return TRUE.
 *           Locals declaration inside body of colon-definition (i.e., after
 *                something has been compiled-in to it) is potentially risky,
 *                but may be valid, and is a part of legacy practice.  It
 *                will not be treated as an outright ERROR, but it will
 *                generate a WARNING...
 *           Multiple locals declarations inside a single colon-definition
 *                are completely disallowed.  ERROR and return TRUE.
 *           Locals declaration inside a control-structure is prohibited.
 *                Generate an ERROR, but return FALSE to allow processing
 *                of the declaration to continue.
 *
 **************************************************************************** */

/*  The value of  lastcolon  when Locals Declaration is made.
 *      If it's the same, that detects multiple locals declaration attempt.
 */
static int last_local_colon = 0;

static bool error_check_locals ( void )
{
    bool retval = FALSE;
    
    if ( ! incolon )
    {
	tokenization_error ( TKERROR,
	    "Can only declare Locals inside of a Colon-definition.\n");
        retval = TRUE;
    } else {
	if ( last_local_colon == lastcolon )
	{
	    tokenization_error ( TKERROR, "Excess Locals Declaration");
	    in_last_colon( TRUE);
	    retval = TRUE;
	}else{
            last_local_colon = lastcolon;
	    if ( opc > lastcolon )
	    {
		tokenization_error ( WARNING,
		    "Declaring Locals after the body of a Colon-definition "
		    "has begun is not recommended.\n");
	    }
	    announce_control_structs( TKERROR,
		"Local-Values Declaration encountered",
		    last_colon_abs_token_no);
	}
    }
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  declare_locals
 *      Synopsis:       Process (or Ignore) the Declaration of Locals,
 *                          upon encountering Curly-brace ( {  )
 *      
 *      Inputs:
 *         Parameters:
 *             ignoring            TRUE if "Ignoring"
 *         Global Variables:
 *             statbuf             Next symbol to process.
 *             lineno              Current Line Number in Input File
 *             report_multiline    FALSE to suspend multiline warning
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Global Variables:
 *             statbuf             Advanced to end of Locals Declaration.
 *             pc                  Bumped past the close-curly-brace
 *         Local Static Variables:
 *             localno             Init'd, then updated by gather_locals()
 *             l_d_lineno          Line Number of start of Locals Declaration
 *
 *      Error Detection:
 *          See  error_check_locals()
 *              After Error messages, will bypass further processing until the
 *                  terminating close-curly-brace of a Locals Declaration.
 *                  If the terminating close-curly-brace missing under those
 *                  circumstances, issue an Error
 *              If terminating close-curly-brace is missing when the Locals
 *                  Declaration is otherwise valid, gather_locals() will
 *                  detect and report the Error.
 *              Warning if multiline declaration.  Because embedded comments
 *                  may also supppress the multiline warning, we need to save
 *                  and restore the state of the report_multiline switch...
 *
 **************************************************************************** */

void declare_locals ( bool ignoring)
{
    num_ilocals = 0;
    num_ulocals = 0;
    localno = 0;
    
    l_d_lineno = lineno;
    bool sav_rep_mul_lin = report_multiline;
    report_multiline = TRUE;

    if ( ignoring || error_check_locals() )
    {
       if ( skip_until ( '}' ) )
       {
	   warn_unterm(TKERROR,
	       "misplaced Local-Values Declaration", l_d_lineno);
       }else{
	   pc++ ;  /*  Get past the close-curly-brace  */
       }
    }else{
       if (gather_locals( TRUE,  &num_ilocals ) )
       {
	   gather_locals( FALSE, &num_ulocals );
       }
    }

    /*  If PC has reached the END,  gather_locals()  will
     *      have already issued an "unterminated" Error;
     *      a "multiline" warning would be redundant
     *      repetitive, unnecessary, excessive, unaesthetic
     *      and -- did I already mention? -- redundant.
     */
    if ( pc < end )
    {
	report_multiline = sav_rep_mul_lin;
	warn_if_multiline( "Local-Values declaration", l_d_lineno);
    }

    /*  Don't do anything if no Locals were declared    */
    /*  This could happen if the  {  }  field is empty  */
    if ( localno != 0 )
    {
	activate_locals();
    }
}

/* **************************************************************************
 *
 *      Function name:  handle_local
 *      Synopsis:       Process the given name as a Local Name;
 *                      indicate if it was a valid Local Name.
 *
 *      Inputs:
 *         Parameters:
 *             lname                The "Local" name for which to look
 *         Local Static Variables:  
 *             local_names          The vocabulary for Local names
 *
 *      Outputs:
 *         Returned Value:          TRUE if the name is a valid "Local Name"
 *
 **************************************************************************** */

static bool handle_local( char *lname)
{
    bool retval = handle_tic_vocab( lname, local_names );
    return ( retval ) ;
}

/* **************************************************************************
 *
 *      Function name:  lookup_local
 *      Synopsis:       Return a pointer to the data-structure of the named
 *                      word, only if it was a valid Local Name.
 *
 *      Inputs:
 *         Parameters:
 *             lname                The "Local" name for which to look
 *         Local Static Variables:  
 *             local_names          The vocabulary for Local names
 *
 *      Outputs:
 *         Returned Value:          Pointer to the data-structure, or
 *                                      NULL if not found.
 *
 **************************************************************************** */

tic_hdr_t *lookup_local( char *lname)
{
    tic_hdr_t *retval = lookup_tic_entry( lname, local_names );
    return ( retval ) ;
}


/* **************************************************************************
 *
 *      Function name:  create_local_alias
 *      Synopsis:       Create an alias in the "Local Names" Vocabulary
 *
 *      Associated FORTH word:              ALIAS
 *
 *      Inputs:
 *         Parameters:
 *             old_name             Name of existing entry
 *             new_name             New name for which to create an entry
 *
 *      Outputs:
 *         Returned Value:          TRUE if  old_name  found in "Locals" vocab
 *         Global Variables:    
 *             local_names          Will point to the new entry
 *         Memory Allocated:
 *             Memory for the new entry, by the support routine
 *         When Freed?
 *             When  forget_locals()  routine frees up all memory
 *                 allocations in the "Local Names" Vocabulary.
 *
 **************************************************************************** */

bool create_local_alias(char *new_name, char *old_name)
{
    bool retval = create_tic_alias( new_name, old_name, &local_names );
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  exists_as_local
 *      Synopsis:       Simply confirm whether a given name exists
 *                      within the Locals vocabulary.
 *      
 *      Inputs:
 *         Parameters:
 *             stat_name          Name to look up
 *
 *      Outputs:
 *         Returned Value:        TRUE if stat_name was a Local
 *
 **************************************************************************** */

bool exists_as_local( char *stat_name )
{
    bool retval = exists_in_tic_vocab(stat_name, local_names );
    return ( retval );
}


/* **************************************************************************
 *
 *      Function name:  assign_local
 *      Synopsis:       Process the "Assign to a Local" operator ( -> )
 *      
 *      Inputs:
 *         Parameters:             NONE
 *         Global Variables:
 *             statbuf          Next symbol to process
 *             pc               Input-source Scanning pointer
 *             lineno           Input-source Line Number. Used for Err Mssg.
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Global Variables:
 *             statbuf          Advanced to next symbol
 *             pc               Advanced; may be unchanged if error.
 *             lineno           Advanced; may be unchanged if error
 *             local_op         Will be set to Store and then reset to Fetch.
 *         Global Behavior:
 *             Construct a phrase and pass it to the Tokenizer.
 *
 *      Error Detection:
 *          If next symbol is not a Local name, print ERROR message    
 *              and restore  pc  so that the next symbol will be    
 *              processed by ordinary means.
 *          In the extremely unlikely case that -> is last symbol in  
 *              the source-file, report an ERROR.
 *
 *      Process Explanation:
 *          Save the PC.
 *          Get the next symbol; check for end-of-file.
 *          Set Local Operator ( local_op ) to "Store", to prepare to apply it.
 *          Pass the next symbol to  handle_local() .
 *          If  handle_local()  failed to find the name, you have
 *              detected an error; restore  pc .
 *          Otherwise, you have invoked the local and applied "Store" to it.
 *          At the end, reset  local_op  to "Fetch".
 *
 **************************************************************************** */

void assign_local ( void )
{
    signed long wlen;
    bool is_okay;
    u8 *savd_pc = pc;
    unsigned int savd_lineno = lineno;

    wlen = get_word();

	if ( wlen <= 0 )
	{
	    warn_unterm(TKERROR, "Locals Assignment", lineno);
	    return;
	}

    local_op = "!";   /*  Set to Store  */

    is_okay = handle_local( statbuf);
    if( INVERSE(is_okay)  )
    {
        tokenization_error ( TKERROR,
	    "Cannot apply -> to %s, only to a declared Local.\n", statbuf );
        pc = savd_pc;
	lineno = savd_lineno;
    }
    local_op = "@";   /*  Reset to Fetch  */
}

/* **************************************************************************
 *
 *      Function name:  finish_locals
 *      Synopsis:       Compile-in the call to  {pop-locals}  that the
 *                      new definition under construction will need
 *                      when it's about to complete execution, i.e.,
 *                      before an EXIT or a SemiColon.  But only if the
 *                      current definition under construction is using Locals.
 *      
 *      Inputs:
 *         Parameters:       NONE
 *             
 *         Local Static Variables:
 *             localno            Total # of Locals.
 *                                    Both a param to  {pop-locals} 
 *                                    and an indicator that Locals are in use.
 *             pop_locals         Name of  {pop-locals}  routine.
 *
 *      Outputs:
 *         Returned Value:        NONE
 *         Local Static Variables:
 *             eval_buf            Phrase constructed here; will become new
 *                                     Source Input Buffer, temporarily
 *
 *      Error Detection:
 *          If the Local Values Support FCode source-file was not
 *              FLOADed into the user's tokenization source, then
 *              the function  {pop-locals}  will be an "unknown name".
 *
 *      Revision History:
 *          Updated Fri, 24 Feb 2006 by David L. Paktor
 *              The eval_string() routine no longer calls its own
 *                  instance of  tokenize() .  In order to make a
 *                  smooth transition between the processing the
 *                  internally-generated string and the resumption
 *                  of processing the source file, it simply sets
 *                  up the string to be processed next.
 *              In this case, however, we need to have the string
 *                  processed right away, as the calling routine
 *                  emits a token that must follow those generated
 *                  by this.
 *              Fortunately, we know the exact contents of the string.
 *                  Two calls to tokenize_one_word() will satisfy the
 *                  requirement.
 *
 **************************************************************************** */

void finish_locals ( void )
{
     /*    Don't do anything if Locals are not in use    */
    if ( localno > 0 )
    {
        char nlocals_buf[10];
     
        int_to_str(localno, nlocals_buf ); 
        sprintf( eval_buf,"%s %s",nlocals_buf, pop_locals);
        eval_string( eval_buf);
	tokenize_one_word( get_word() );
	tokenize_one_word( get_word() );
    }
}

/* **************************************************************************
 *
 *      Function name:  forget_locals
 *      Synopsis:       Remove the Locals' names from the special Vocabulary
 *                      free-up their allocated memory, and reset the Locals'
 *                      counters (which are also the indication that Locals
 *                      are in use).  This is done at the time a SemiColon
 *                      is processed.  But only if the current definition
 *                      under construction is using Locals.
 *      
 *      Inputs:
 *         Parameters:              NONE
 *         Local Static Variables:
 *             local_names          The vocabulary for new Local names
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Local Static Variables:
 *             local_names          Emptied and pointing at NULL.
 *             num_ilocals          Reset to zero
 *             num_ulocals              ditto
 *             localno                  ditto
 *         Memory Freed
 *             All memory allocations in the "Local Names" Vocabulary.
 *
 **************************************************************************** */

void forget_locals ( void )
{
     /*    Don't do anything if Locals are not in use    */
    if ( localno != 0 )
    {
        reset_tic_vocab( &local_names, NULL ) ;

        num_ilocals = 0;
        num_ulocals = 0;
        localno = 0;
    }
}
