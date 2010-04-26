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
 *      Support Functions for tokenizing FORTH Flow-Control structures.
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *               These first two do their work after the calling routine
 *                        has written the token for the required variant:
 *
 *          mark_do                        Mark branches for "do" variants
 *          resolve_loop                   Resolve "loop" variants' branches
 *
 *               The remaining routines' descriptions are all similar:
 *                        Write the token(s), handle the outputs, mark
 *                        or resolve the branches, and verify correct
 *                        control-structure matching, for tokenizing
 *                        the ........................ statement in FORTH
 *          emit_if                          IF
 *          emit_else                        ELSE
 *          emit_then                        THEN
 *          emit_begin                       BEGIN
 *          emit_again                       AGAIN
 *          emit_until                       UNTIL
 *          emit_while                       WHILE
 *          emit_repeat                      REPEAT
 *          emit_case                        CASE
 *          emit_of                          OF
 *          emit_endof                       ENDOF
 *          emit_endcase                     ENDCASE
 *
 *      Three additional routines deal with matters of overall balance
 *      of the Control-Structures, and identify the start of any that
 *      were not balanced.  The first just displays Messages:
 *
 *          announce_control_structs
 *
 *      The other two clear and re-balance them:
 *
 *          clear_control_structs_to_limit
 *          clear_control_structs
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Still to be done:
 *          Correct analysis of Return-Stack usage around Flow-Control
 *              constructs, including within Do-Loops or before Loop
 *              Elements like I and J or UNLOOP or LEAVE.
 *          Similarly, Return-Stack usage around IF ... ELSE ... THEN
 *              statements needs analysis.  For instance, the following:
 * 
 *          blablabla >R  yadayada IF  R> gubble ELSE flubble R>  THEN
 * 
 *              is, in fact, correct, while something like:
 * 
 *          blablabla >R  yadayada IF  R> gubble THEN
 * 
 *              is an error.
 * 
 *          Implementing an analysis that would be sufficiently accurate
 *              to justify reporting an ERROR with certainty (rather than
 *              a mere WARNING speculatively) would probably require full
 *              coordination with management of Flow-Control constructs,
 *              and so is noted here.
 *
 **************************************************************************** */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "toke.h"
#include "emit.h"
#include "vocabfuncts.h"
#include "scanner.h"
#include "stack.h"
#include "errhandler.h"
#include "flowcontrol.h"
#include "stream.h"

/* **************************************************************************
 *
 *          Global Variables Imported
 *              opc              FCode Output Buffer Position Counter
 *              noerrors         "Ignore Errors" flag, set by "-i" switch     
 *              do_loop_depth    How deep we are inside DO ... LOOP variants   
 *              incolon          State of tokenization; TRUE if inside COLON
 *              statbuf          The word just read from the input stream
 *              iname            Name of input file currently being processed
 *              lineno           Current line-number being processed
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *          Global Variables Exported
 *              control_stack_depth        Number of items on "Control-Stack"
 *
 **************************************************************************** */

int control_stack_depth = 0;


/* **************************************************************************
 *
 *              Internal Static Functions:
 *          push_cstag                     Push an item onto the Control-Stack
 *          pop_cstag                      Pop one item from the Control-Stack
 *          control_stack_size_test        Test C-S depth; report if error
 *          control_structure_mismatch     Print error-message
 *          offset_too_large               Print error-message
 *          matchup_control_structure      Error-check Control-Stack
 *          matchup_two_control_structures Error-check two Control-Stack entries
 *          emit_fc_offset                 Error-check and output FCode-Offset
 *          control_structure_swap         Swap control-struct branch-markers
 *          mark_backward_target           Mark target of backward-branch
 *          resolve_backward               Resolve backward-target for branch
 *          mark_forward_branch            Mark forward-branch
 *          resolve_forward                Resolve forward-branch at target
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *              Internal Named Constants
 *   Note:  These control-structure identifier tags -- a.k.a. cstags --
 *       are used to identify the matching components of particular
 *       control-structures.  They are passed as parameters, and either
 *       "Pushed" onto the "Control-Stack", or compared with what is on
 *       "Top" of the "Control-Stack", as an error-check.
 *
 *            name                    used by forth words:
 *         BEGIN_CSTAG             BEGIN AGAIN UNTIL REPEAT
 *         IF_CSTAG                IF ELSE THEN
 *         WHILE_CSTAG             WHILE REPEAT THEN
 *         DO_CSTAG                DO ?DO LOOP +LOOP
 *         CASE_CSTAG              CASE OF ENDCASE
 *         OF_CSTAG                OF ENDOF
 *         ENDOF_CSTAG             ENDOF ENDCASE
 *
 *   The numbers assigned are arbitrary; they were selected for a
 *       high unlikelihood of being encountered in normal usage,
 *       and constructed with a hint of mnemonic value in mind.
 *
 **************************************************************************** */
                                 /*     Mnemonic:   */
#define BEGIN_CSTAG  0xC57be916  /*  CST BEGIN      */
#define IF_CSTAG     0xC57A901f  /*  CSTAG (0) IF   */
#define WHILE_CSTAG  0xC573412e  /*  CST WHILE      */
#define DO_CSTAG     0xC57A90d0  /*  CSTAG (0) DO   */
#define CASE_CSTAG   0xC57Aca5e  /*  CSTA CASE      */
#define OF_CSTAG     0xC57A90f0  /*  CSTAG OF (0)   */
#define ENDOF_CSTAG  0xC57e6d0f  /*  CST ENDOF   */


/* **************************************************************************
 *
 *     Control-Structure identification, matching, completion and error
 *         messaging will be supported by a data structure, which we
 *         will call a CSTAG-Group
 *
 *     It consists of one "Data-item" and several "Marker" items, thus:
 *
 *         The Data-item in most cases will be a value of OPC (the Output
 *             Buffer Position Counter) which will be used in calculating 
 *             an offset or placing an offset or both, as the case may be,
 *             for the control structure in question.  The one exception
 *             is for a CSTAG-Group generated by a CASE statement; its
 *             Data-item will be an integer count of the number of "OF"s
 *             to be resolved when the ENDCASE statement is reached.
 *
 *         The CSTAG for the FORTH word, as described above
 *         The name of the input file in which the word was encountered
 *             (actually, a pointer to a mem-alloc'ed copy of the filename)
 *         The line number, within the input file, of the word's invocation
 *         The Absolute Token Number in all Source Input of the word
 *         The FORTH word that started the structure, (used in error messages)
 *         A flag to indicate when two CSTAG-Groups are created together,
 *             which will be used to prevent duplicate error messages when,
 *             for instance, a  DO  is mismatched with a  REPEAT .
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *               "Control-Stack" Diagram Notation
 *
 *     The CSTAG-Groups will be kept in an order resembling a data-stack,
 *         (even though it won't be the data-stack itself).  We will refer
 *         to this list of structures as the "Control Stack", and in our
 *         comments we will show their arrangement in a format resembling
 *         stack-diagram remarks.
 *
 *     In these "Control-Stack Diagrams", we will use the notation:
 *		   <Stmt>_{FOR|BACK}w_<TAGNAM>
 *         to represent a CSTAG-Group generated by a <Stmt> -type of
 *         statement, with a "FORw"ard or "BACKw"ard branch-marker and
 *         a CSTAG of the <TAGNAM> type.
 *
 *     A CASE-CSTAG-Group will have a different notation:
 *		   N_OFs...CASE_CSTAG
 *
 *     In all cases, a CSTAG-Group will be manipulated as a unit.
 *
 *     The notation for Control-Stack Diagram remarks will largely resemble
 *         the classic form used in FORTH, i.e., enclosed in parentheses,
 *         lowest item to the left, top item on the right, with a double-
 *         hyphen to indicate "before" or "after".
 *
 *     Enclosure in {curly-braces} followed by a subscript-range indicates
 *         that the Stack-item or Group is repeated.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      We are not keeping the "Control Stack" structures on the regular
 *          data stack because a sneaky combination of user-inputs could
 *          throw things into chaos were we to use that scheme.  Consider
 *          what would happen if a number were put on the stack, say, in
 *          tokenizer-escape mode, in between elements of a flow-control
 *          structure...  Theoretically, there is no reason to prohibit
 *          that, but it would be unexpectedly problematical for most
 *          FORTH-based tokenizers.
 *
 *      Maintaining the "Control Stack" structures in a linked-list would
 *          be a more nearly bullet-proof approach.  The theory of operation
 *          would be the same, broadly speaking, and there would be no need
 *          to check for  NOT_CSTAG  and no risk of getting the elements of
 *          the control-structures out of sync.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *          Structure Name:    cstag_group_t
 *          Synopsis:          Control-Structure Tag Group
 *                            
 *   Fields:
 *       cs_tag             Control-structure identifier tag
 *       cs_inp_fil         Name of input file where C-S was started
 *       cs_line_num        Line-number in Current Source when C-S was started
 *       cs_abs_token_num  "Absolute" Token Number when C-S was started
 *       cs_word            The FORTH word that started the C-S
 *       cs_not_dup         FALSE if second "Control Stack" entry for same word
 *       cs_datum           Data-Item of the Group
 *       prev               Pointer to previous CSTAG-Group in linked-list
 *
 *       All data using this structure will remain private to this file,
 *           so we declare it here rather than in the  .h  file
 *
 **************************************************************************** */

typedef struct cstag_group {
    unsigned long cs_tag;
    char *cs_inp_fil;
    unsigned int cs_line_num;
    unsigned int cs_abs_token_num;
    char *cs_word;
    bool cs_not_dup;
    unsigned long cs_datum;
    struct cstag_group *prev;
} cstag_group_t;

/* **************************************************************************
 *
 *          Internal Static Variables
 *     control_stack          "Thread" Pointer to the linked-list of 
 *                                 "Control Stack" structure entries
 *     not_cs_underflow       Flag used to prevent duplicate messages
 *     not_consuming_two      Flag used to prevent loss of messages
 *     didnt_print_otl        Flag used to prevent duplicate messages
 *
 **************************************************************************** */

static cstag_group_t *control_stack = NULL;   /*  "Top" of the "Stack"  */

/* **************************************************************************
 *
 *     not_cs_underflow is used only by routines that make two calls to
 *         resolve a marker.  It is set TRUE before the first call; if
 *         that call had a control-stack underflow, the error-message
 *         routine resets it to FALSE.  The calling routine can then
 *         test it as the condition for the second call.
 *     Routines that make only one call to resolve a marker can ignore it.
 *
 **************************************************************************** */

static bool not_cs_underflow;  /*  No need to initialize.  */

/* **************************************************************************
 *
 *     not_consuming_two is also used only by routines that make two calls
 *         to resolve a marker, but for this case, those routines only need  
 *         to reset it to FALSE and not to test it; that will be done by
 *         the  control_structure_mismatch()  routine when it looks at
 *         the  cs_not_dup  field.  If the mismatch occurred because of
 *         a combination of control-structures that consume one each,
 *         the message will be printed even for the second "Control Stack"
 *         entry.  The routine that changed it will have to set it back to
 *         TRUE when it's done with it.
 *
 *     didnt_print_otl is used similarly, but only for the offset-too-large
 *        error in the   DO ... LOOP  type of control-structures.
 *
 **************************************************************************** */

static bool not_consuming_two = TRUE;
static bool didnt_print_otl = TRUE;


/* **************************************************************************
 *
 *      Function name:  push_cstag
 *      Synopsis:       Push a new CSTAG-Group onto the front ("Top")
 *                      of the (notional) Control-Stack.
 *
 *      Inputs:
 *         Parameters:
 *             cstag           ID Tag for Control-Structure to "Push"
 *             datum           The Data-Item for the new CSTAG-Group
 *         Global Variables:
 *             iname           Name of input file currently being processed
 *             lineno          Current-Source line-number being processed
 *             abs_tokenno     "Absolute"Token Number of word being processed
 *             statbuf         The word just read, which started the C-S
 *         Local Static Variables:
 *             control_stack   Will become the new entry's "prev"
 *
 *      Outputs:
 *         Returned Value:     None
 *         Global Variables:
 *             control_stack_depth            Incremented
 *         Local Static Variables:
 *             control_stack   Will become the "previous" entry in the list
 *         Items Pushed onto Control-Stack:
 *             Top:            A new CSTAG-Group, params as given
 *         Memory Allocated
 *             New CSTAG-Group structure
 *             Duplicate of name of current input file
 *             Duplicate of word just read
 *         When Freed?
 *             When Removing a CSTAG-Group, in pop_cstag()
 *
 **************************************************************************** */

static void push_cstag( unsigned long cstag, unsigned long datum)
{
    cstag_group_t *cs_temp;

    cs_temp = control_stack;
    control_stack = safe_malloc( sizeof(cstag_group_t), "pushing CSTag");

    control_stack->cs_tag = cstag;
    control_stack->cs_inp_fil = strdup(iname);
    control_stack->cs_line_num = lineno;
    control_stack->cs_abs_token_num = abs_token_no;
    control_stack->cs_word = strdup(statbuf);
    control_stack->cs_not_dup = TRUE;
    control_stack->cs_datum = datum;
    control_stack->prev = cs_temp;

    control_stack_depth++;
    
}

/* **************************************************************************
 *
 *      Function name:  pop_cstag
 *      Synopsis:       Remove a CSTAG-Group from the front ("Top") of the
 *                      (notional) Control-Stack.
 *
 *      Inputs:
 *         Parameters:                    NONE
 *         Global Variables:
 *         Local Static Variables:
 *             control_stack              CSTAG-Group on "Top"
 *
 *      Outputs:
 *         Returned Value:                NONE
 *         Global Variables:
 *             control_stack_depth        Decremented
 *         Local Static Variables:
 *             control_stack              "Previous" entry will become current
 *         Memory Freed
 *             mem-alloc'ed copy of input filename
 *             mem-alloc'ed copy of Control-structure FORTH word
 *             CSTAG-Group structure
 *         Control-Stack, # of Items Popped:  1
 *
 *      Process Explanation:
 *          The calling routine might not check for empty Control-Stack,
 *              so we have to be sure and check it here.
 *
 **************************************************************************** */

static void pop_cstag( void)
{

    if ( control_stack != NULL )
    {
	cstag_group_t *cs_temp;

	cs_temp = control_stack->prev;
	free( control_stack->cs_word );
	free( control_stack->cs_inp_fil );
	free( control_stack );
	control_stack = cs_temp;

	control_stack_depth--;
    }
}

/* **************************************************************************
 *
 *      Function name:  control_stack_size_test
 *      Synopsis:       Detect Control Stack underflow; report if an ERROR.
 *
 *      Inputs:
 *         Parameters:
 *             min_depth                 Minimum depth needed
 *         Global Variables:
 *             control_stack_depth       Current depth of Control Stack
 *             statbuf                   Word to name in error message
 *
 *      Outputs:
 *         Returned Value:                TRUE if adequate depth
 *         Local Static Variables:
 *             not_cs_underflow           Reset to FALSE if underflow detected.
 *         Printout:
 *             Error message is printed.
 *                 Identify the colon definition if inside one.
 *
 *      Process Explanation:
 *          Some statements need more than one item on the Control Stack;
 *             they will do their own  control_stack_depth  testing and
 *             make a separate call to this routine.
 *
 **************************************************************************** */

static bool control_stack_size_test( int min_depth )
{
    bool retval = TRUE;

    if ( control_stack_depth < min_depth )
    {
	retval = FALSE;
	tokenization_error ( TKERROR,
		"Control-Stack underflow at %s", strupr(statbuf) );
	in_last_colon( TRUE);

	not_cs_underflow = FALSE;   /*  See expl'n early on in this file  */
    }

    return( retval );
}

/* **************************************************************************
 *
 *      Function name:  control_structure_mismatch
 *      Synopsis:       Report an ERROR after a Control Structure mismatch
 *                      was detected.
 *
 *      Inputs:
 *         Parameters:                    NONE
 *         Global Variables:
 *             statbuf              Word encountered, to name in error message
 *         Local Static Variables:
 *             control_stack        "Pushed" Control-Structure Tag Group
 *             not_consuming_two    See explanation early on in this file
 *         Control-Stack Items:
 *             Top:                 "Current" Control-Structure Tag Group
 *                                      Some of its "Marker" information
 *                                      will be used in the error message
 *
 *      Outputs:
 *         Returned Value:                NONE
 *         Printout:
 *             Error message is printed
 *
 *      Process Explanation:
 *          This routine is called after a mismatch is detected, and
 *              before the CSTAG-Group is "Popped" from the notional
 *              Control-Stack.
 *          If the  control_stack  pointer is NULL, print a different
 *              Error message
 *          Don't print if the "Control Stack" entry is a duplicate and
 *              we're processing a statement that consumes two entries.
 *
 **************************************************************************** */

static void control_structure_mismatch( void )
{
    if ( control_stack->cs_not_dup || not_consuming_two )
    {
	tokenization_error ( TKERROR,
	    "The %s is mismatched with the %s" ,
		strupr(statbuf), strupr(control_stack->cs_word));
	where_started( control_stack->cs_inp_fil, control_stack->cs_line_num );
    }
}


/* **************************************************************************
 *
 *      Function name:  offset_too_large
 *      Synopsis:       Report an ERROR after a too-large fcode-offset
 *                      was detected.
 *
 *      Inputs:
 *         Parameters:
 *             too_large_for_16     TRUE if the offset is too large to be
 *                                      expressed as a 16-bit signed number.
 *         Global Variables:
 *             statbuf              Word encountered, to name in error message
 *             offs16               Whether we are using 16-bit offsets
 *         Local Static Variables:
 *             control_stack        "Pushed" Control-Structure Tag Group
 *             didnt_print_otl      Switch to prevent duplicate message
 *         Control-Stack Items:
 *             Top:                 "Current" Control-Structure Tag Group
 *                                      Some of its "Marker" information
 *                                      will be used in the error message
 *
 *      Outputs:
 *         Returned Value:          NONE
 *         Local Static Variables:
 *             didnt_print_otl      Will be reset to FALSE
 *             
 *         Printout:
 *             Error message:
 *                 Branch offset too large between <here> and <there>
 *             Advisory message, if we are using 8-bit offsets, will
 *                 indicate whether switching to 16-bit offsets would help
 *
 *      Process Explanation:
 *          Two branches are involved in a DO ... LOOP  structure:  an "outer"
 *              forward-branch and a slightly smaller "inner" backward-branch.
 *              In the majority of cases, if one offset exceeds the limit,
 *              both will.  There is, however, a very small but distinct
 *              possibility that the offset for the smaller branch will not
 *              exceed the limit while the larger one does.  To prevent two
 *              messages from being printed in the routine instance, but still
 *              assure that one will be printed in the rare eventuality, we
 *              utilize the flag called  didnt_print_otl  in conjunction
 *              with the  cs_not_dup  field.
 *
 **************************************************************************** */

static void offset_too_large( bool too_large_for_16 )
{
    if ( control_stack->cs_not_dup || didnt_print_otl )
    {
	tokenization_error( TKERROR,
	    "Branch offset is too large between %s and the %s" ,
		strupr(statbuf), strupr(control_stack->cs_word));
	where_started( control_stack->cs_inp_fil, control_stack->cs_line_num );
	if ( INVERSE( offs16 ) )
	{
	    if ( too_large_for_16 )
	    {
		tokenization_error ( INFO,
		    "Offset would be too large even if 16-bit offsets "
			"were in effect.\n");
	    }else{
		tokenization_error ( INFO,
		    "Offset might fit if 16-bit offsets "
			"(e.g., fcode-version2) were used.\n" );
	    }
	}
    }
    didnt_print_otl = FALSE;
}

/* **************************************************************************
 *
 *      Function name:  emit_fc_offset
 *      Synopsis:       Test whether the given FCode-Offset is out-of-range;
 *                      before placing it into the FCode Output Buffer.
 *
 *      Inputs:
 *         Parameters:
 *             fc_offset               The given FCode-Offset
 *         Global Variables:
 *             offs16                  Whether we are using 16-bit offsets
 *             noerrors                "Ignore Errors" flag
 *
 *      Outputs:
 *         Returned Value:             NONE
 *
 *      Error Detection:
 *          Error if the given FCode-Offset exceeds the range that can
 *              be expressed by the size (i.e., 8- or 16- -bits) of the
 *              offsets we are using.  Call  offset_too_large()  to print
 *              the Error message; also, if  noerrors  is in effect, issue
 *              a Warning showing the actual offset and how it will be coded.
 *
 *      Process Explanation:
 *          For forward-branches, the OPC will have to be adjusted to
 *              indicate the location that was reserved for the offset
 *              to be written, rather than the current location.  That
 *              will all be handled by the calling routine.
 *          We will rely on "C"'s type-conversion (type-casting) facilities.
 *          Look at the offset value both as an 8-bit and as a 16-bit offset,
 *              then determine the relevant course of action.
 *
 **************************************************************************** */

static void emit_fc_offset( int fc_offset)
{
    int fc_offs_s16 = (s16)fc_offset;
    int fc_offs_s8  =  (s8)fc_offset;
    bool too_large_for_8  = BOOLVAL( fc_offset != fc_offs_s8 );
    bool too_large_for_16 = BOOLVAL( fc_offset != fc_offs_s16);

    if ( too_large_for_16 || ( INVERSE(offs16) && too_large_for_8 ) )
    {
	offset_too_large( too_large_for_16 );
	if ( noerrors )
	{
	    int coded_as = offs16 ? (int)fc_offs_s16 : (int)fc_offs_s8 ;
	    tokenization_error( WARNING,
		"Actual offset is 0x%x (=dec %d), "
		    "but it will be coded as 0x%x (=dec %d).\n",
			fc_offset, fc_offset, coded_as, coded_as );
	}
    }

    emit_offset( fc_offs_s16 );
}


/* **************************************************************************
 *
 *      Function name:  matchup_control_structure
 *      Synopsis:       Error-check. Compare the given control-structure
 *                          identifier tag with the one in the CSTAG-Group
 *                          on "Top" of the "Control Stack".
 *                      If they don't match, report an error, and, if not
 *                          "Ignoring Errors", return Error indication.
 *                      If no error, pass the Data-item back to the caller.
 *                      Do not consume the CSTAG-Group; that will be the
 *                          responsibility of the calling routine.
 *
 *      Inputs:
 *         Parameters:
 *             cstag          Control-struc ID Tag expected by calling function
 *         Global Variables:
 *             noerrors       "Ignore Errors" flag
 *         Local Static Variables:
 *             control_stack   "Pushed" (current) Control-Structure Tag Group
 *         Control-Stack Items:
 *             Top:            Current CSTAG-Group
 *
 *      Outputs:
 *         Returned Value:     TRUE = Successful match, no error.
 *
 *      Error Detection:
 *           Control Stack underflow or cstag mismatch.  See below for details.
 *
 *      Process Explanation:
 *           If the expected cstag does not match the cs_tag from the CSTAG
 *               Group on "Top" of the "Control Stack", print an ERROR message,
 *               and, unless the "Ignore Errors" flag is in effect, prepare
 *               to return FALSE.
 *          However, if we've "underflowed" the "Control Stack", we dare not
 *              ignore errors; that could lead to things like attempting to
 *              write a forward-branch FCode-offset to offset ZERO, over the
 *              FCODE- or PCI- -header block.  We don't want that...
 *          So, if the  control_stack  pointer is NULL, we will print an
 *              ERROR message and immediately return FALSE.
 *          Since we will not consume the CSTAG-Group, the calling routine
 *              can access the Data-Item and any "Marker" information it may
 *              still require via the local  control_stack  pointer. The caller
 *              will be responsible for removing the CSTAG-Group.
 *
 *      Special Exception to "Ignore Errors":
 *          At the last usage of the  CASE_CSTAG , for the ENDCASE statement,
 *              this routine will be called to control freeing-up memory, etc.
 *          For the OF statement, it will be called to control incrementing
 *              the OF-count datum.
 *          Processing an ENDCASE statement with the datum from any other
 *              CSTAG-Group can lead to a huge loop.
 *          Processing any other "resolver" with the datum from an ENDCASE
 *              CSTAG-Group can lead to mistaking a very low number for an
 *              offset into the Output Buffer and attempting to write to it.
 *          Incrementing the datum from any other CSTAG-Group can lead to
 *              a variety of unacceptable errors, too many to guess.
 *          So, if either the given cstag or the cs_tag field of the "Top"
 *              CSTAG-Group is a CASE_CSTAG , we will not ignore errors.
 *
 **************************************************************************** */

static bool matchup_control_structure( unsigned long cstag )
{
    bool retval = FALSE;

    if ( control_stack_size_test( 1) )
    {
	retval = TRUE;

	if ( control_stack->cs_tag != cstag )
	{
	    control_structure_mismatch();

	    if (    ( INVERSE(noerrors) )
		 || ( cstag == CASE_CSTAG )
		 || ( control_stack->cs_tag == CASE_CSTAG )
	            )
	    {
		retval = FALSE;
	    }
	}

    }
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  control_structure_swap
 *      Synopsis:       Swap control-structure branch-marker Groups
 *
 *      Inputs:
 *         Parameters:                NONE
 *         Local Static Variables:
 *             control_stack          Pointer to "Control Stack" linked-list
 *         Control-Stack Items:
 *             Top:                   CSTAG-Group_0
 *             Next:                  CSTAG-Group_1
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Local Static Variables:
 *             control_stack          Points to former "previous" and vice-versa
 *         Items on Control-Stack:
 *             Top:                   CSTAG-Group_1
 *             Next:                  CSTAG-Group_0
 *
 *      Error Detection:
 *          If control-stack depth is not at least 2, CS underflow ERROR.
 *              This might trigger other routines' error detections also...
 *
 *      Extraneous Remarks:
 *          Before control-structure identification was implemented, offsets
 *              were kept on the data-stack, and this was a single SWAP.
 *          When CSTAGs were added, the "Group" was only a pair kept on the
 *               data-stack -- the CSTAG and the Data-item -- and this
 *               became a TWO_SWAP()
 *          For a while, when I tried keeping the CSTAG-Group on the stack,
 *               this became a FOUR_SWAP()
 *          That turned out to be unacceptably brittle; this way is much
 *               more robust. 
 *          I am so glad I called this functionality out into a separate
 *              routine, early on in the development process.
 *
 *          This is the function called  1 CSROLL  in section A.3.2.3.2
 *              of the ANSI Forth spec, which likewise corresponds to the
 *              modifier that Wil Baden, in his characteristically elegant
 *              nomenclature, dubbed:  BUT 
 *
 **************************************************************************** */

static void control_structure_swap( void )
{
    if ( control_stack_size_test( 2) )
    {
	cstag_group_t *cs_temp;

	cs_temp = control_stack->prev;

	control_stack->prev = cs_temp->prev;
	cs_temp->prev = control_stack;
	control_stack = cs_temp;
    }
}

/* **************************************************************************
 *
 *      Function name:  matchup_two_control_structures
 *      Synopsis:       For functions that resolve two CSTAG-Groups, both
 *                          matchup both "Top of Control Stack"  entries
 *                          before processing them...
 *
 *      Inputs:
 *         Parameters:
 *             top_cstag      Control-struc ID Tag expected on "Top" CS entry
 *             next_cstag     Control-struc ID Tag expected on "Next" CS entry
 *         Local Static Variables:
 *             not_cs_underflow   Used for underflow detection.
 *         Control-Stack Items:
 *             Top:            Current CSTAG-Group
 *             Next:           Next CSTAG-Group
 *
 *      Outputs:
 *         Returned Value:     TRUE = Successful matches, no error.
 *         Global Variables:
 *             noerrors       "Ignore Errors" flag; cleared, then restored
 *         Local Static Variables:
 *             not_consuming_two               Cleared, then restored
 *         Control-Stack, # of Items Popped:   2 (if matches unsuccessful)
 *
 *      Error Detection:
 *          Control Stack underflow detected by control_structure_swap()
 *          Control Structure mismatch detected by  control_structure_mismatch()
 *
 *      Process Explanation:
 *          We will use  matchup_control_structure()  to do the "heavy lifting".
 *          We will not be ignoring errors in these cases.
 *          Save the results of a match of  top_cstag
 *          Swap the top two CS entries.
 *          If an underflow was detected, there's no more matching to be done.
 *          Otherwise:
 *              Save the results of a match of  next_cstag
 *              Swap the top two CS entries again, to their original order.
 *          The result is TRUE if both matches were successful.
 *          If the matches were not successful, consume the top two entries
 *              (unless there's only one, in which case consume it).
 *
 **************************************************************************** */

static bool matchup_two_control_structures( unsigned long top_cstag,
                                                unsigned long next_cstag)
{
    bool retval;
    bool topmatch;
    bool nextmatch = FALSE;
    bool sav_noerrors = noerrors;
    noerrors = FALSE;
    not_consuming_two = FALSE;

    not_cs_underflow = TRUE;
    topmatch = matchup_control_structure( top_cstag);
    if ( not_cs_underflow )
    {
	control_structure_swap();
	if ( not_cs_underflow )
	{
	   nextmatch = matchup_control_structure( next_cstag);
	   control_structure_swap();
	}
    }

    retval = BOOLVAL( topmatch && nextmatch);

    if ( INVERSE( retval) )
    {
        pop_cstag();
        pop_cstag();
    }

    not_consuming_two = TRUE;
    noerrors = sav_noerrors;
    return ( retval );
}

/* **************************************************************************
 *
 *      Function name:  mark_backward_target
 *      Synopsis:       Mark the target of an expected backward-branch
 *
 *      Associated FORTH words:                 BEGIN  DO  ?DO
 *
 *      Inputs:
 *         Parameters:
 *             cstag              Control-structure ID tag for calling function
 *         Global Variables:
 *             opc                Output Buffer Position Counter
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Items Pushed onto Control-Stack:
 *             Top:                 <Stmt>_BACKw_<TAGNAM>
 *
 *      Process Explanation:
 *          Just before this function is called, the token that begins the
 *              control-structure was written to the FCode Output buffer.
 *          OPC, the FCode Output Buffer Position Counter, is at the
 *              destination to which the backward-branch will be targeted.
 *          Create a CSTAG-Group with the given C-S Tag, and OPC as its datum;
 *              push it onto the Control-Stack.
 *          Later, when the backward-branch is installed, the FCode-offset
 *              will be calculated as the difference between the OPC at
 *              that time and the target-OPC we saved here.
 *      
 **************************************************************************** */

static void mark_backward_target(unsigned long cstag )
{
    push_cstag( cstag, (unsigned long)opc);
}

/* **************************************************************************
 *
 *      Function name:  mark_forward_branch
 *      Synopsis:       Mark the location of, and reserve space for, the
 *                          FCode-offset associated with a forward branch.
 *
 *      Associated FORTH words:                 IF  WHILE  ELSE
 *
 *      Inputs:
 *         Parameters:
 *             cstag              Control-structure ID tag for calling function
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Items Pushed onto Control-Stack:
 *             Top:                 <Stmt>_FORw_<TAGNAM>
 *         FCode Output buffer:
 *             Place-holder FCode-offset of zero.
 *
 *      Process Explanation:
 *          Just before this function is called, the forward-branch token
 *              that begins the control-structure was written to the FCode
 *              Output buffer.
 *          It will need an FCode-offset to the destination to which it will
 *              be targeted, once that destination is known.
 *          Create a CSTAG-Group with the given C-S Tag, and OPC as its datum;
 *              push it onto the Control-Stack.  (This is the same action as
 *              for marking a backward-target.)
 *          Then write a place-holder FCode-offset of zero to the FCode
 *              Output buffer.
 *          Later, when the destination is known, the FCode-offset will be
 *              calculated as the difference between the OPC at that time
 *              and the FCode-offset location we're saving now.  That offset
 *              will be over-written onto the place-holder offset of zero at
 *              the location in the Output buffer that we saved on the
 *              Control-Stack in this routine.
 *
 **************************************************************************** */

static void mark_forward_branch(unsigned long cstag )
{
    mark_backward_target(cstag );
    emit_offset(0);
}

/* **************************************************************************
 *
 *      Function name:  resolve_backward
 *      Synopsis:       Resolve backward-target when a backward branch
 *                      is reached.  Write FCode-offset to reach saved
 *                      target from current location.
 *      
 *      Associated FORTH words:                 AGAIN  UNTIL  REPEAT
 *                                                LOOP  +LOOP
 *
 *      Inputs:
 *         Parameters:
 *             cstag              Control-structure ID tag for calling function
 *         Global Variables:
 *             opc                Output Buffer Position Counter
 *         Control-Stack Items:
 *             Top:              <Stmt>_BACKw_<TAGNAM>
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Global Variables:
 *             opc               Incremented by size of an FCode-offset
 *         Control-Stack, # of Items Popped:   1
 *         FCode Output buffer:
 *             FCode-offset to reach backward-target
 *
 *      Error Detection:
 *          Test for Control-structure ID tag match.
 *
 *      Process Explanation:
 *          Just before this function is called, the backward-branch token
 *              that ends the control-structure was written to the FCode
 *              Output buffer.
 *          The current OPC is at the point from which the FCode-offset
 *              is to be calculated, and at which it is to be written.
 *          The top of the Control-Stack should have the CSTAG-Group from
 *              the statement that prepared the backward-branch target that
 *              we expect to resolve.  Its datum is the OPC of the target
 *              of the backward branch.
 *          If the supplied Control-structure ID tag does not match the one
 *              on top of the Control-Stack, announce an error.  We will
 *              still write an FCode-offset, but it will be a place-holder
 *              of zero.
 *          Otherwise, the FCode-offset we will write will be the difference
 *              between the target-OPC and our current OPC.
 *
 **************************************************************************** */

static void resolve_backward( unsigned long cstag)
{
    unsigned long targ_opc;
    int fc_offset = 0;

    if ( matchup_control_structure( cstag) )
    {
	targ_opc = control_stack->cs_datum;
	fc_offset = targ_opc - opc;
    }

    emit_fc_offset( fc_offset );
    pop_cstag();
}

/* **************************************************************************
 *
 *      Function name:  resolve_forward
 *      Synopsis:       Resolve a forward-branch when its target has been
 *                      reached.  Write the FCode-offset into the space
 *                      that was reserved.
 *
 *      Associated FORTH words:                 ELSE  THEN  REPEAT
 *                                                LOOP  +LOOP
 *
 *      Inputs:
 *         Parameters:
 *             cstag              Control-structure ID tag for calling function
 *         Global Variables:
 *             opc                Output Buffer Position Counter
 *         Control-Stack Items:
 *             Top:               <Stmt>_FORw_<TAGNAM>
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Global Variables:
 *             opc               Changed, then restored.
 *         Control-Stack, # of Items Popped:   1
 *         FCode Output buffer:
 *             FCode-offset is written to location where space was reserved
 *                 when the forward-branch was marked.
 *
 *      Error Detection:
 *          Test for Control-structure ID tag match.
 *
 *      Process Explanation:
 *          Just before this function is called, the last token -- and 
 *              possibly, FCode-offset -- that is within the scope of
 *              what the branch might skip was written to the FCode
 *              Output buffer.
 *          The current OPC is at the point from which the FCode-offset
 *              is to be calculated, but not at which it is to be written.
 *          The top of the Control-Stack should have the CSTAG-Group from
 *              the statement that prepared the forward-branch we expect
 *              to resolve, and for which our current OPC is the target.
 *              Its datum is the OPC of the space that was reserved for
 *              the forward-branch whose target we have just reached.
 *          If the supplied Control-structure ID tag does not match the one
 *              on top of the Control-Stack, announce an error and we're done.
 *          Otherwise, the datum is used both as part of the calculation of
 *              the FCode-offset we are about to write, and as the location
 *              to which we will write it.
 *          The FCode-offset is calculated as the difference between our
 *              current OPC and the reserved OPC location.
 *          We will not be ignoring errors in these cases, because we would
 *              be over-writing something that might not be a place-holder
 *              for a forward-branch at an earlier location in the FCode
 *              Output buffer.
 *
 **************************************************************************** */

static void resolve_forward( unsigned long cstag)
{
    unsigned long resvd_opc;
    bool sav_noerrors = noerrors;
    bool cs_match_result;
    noerrors = FALSE;
    /*  Restore the "ignore-errors" flag before we act on our match result
     *      because we want it to remain in effect for  emit_fc_offset()
     */
    cs_match_result = matchup_control_structure( cstag);
    noerrors = sav_noerrors;

    if ( cs_match_result )
    {
	int saved_opc;
	int fc_offset;

	resvd_opc = control_stack->cs_datum;
	fc_offset = opc - resvd_opc;

	saved_opc = opc;
	opc = resvd_opc;


	emit_fc_offset( fc_offset );
	opc = saved_opc;
    }
    pop_cstag();
}
	

/* **************************************************************************
 *
 *      The functions that follow are the exported routines that
 *          utilize the preceding support-routines to effect their
 *          associated FORTH words.
 *
 *      The routines they call will take care of most of the Error
 *          Detection via stack-depth checking and Control-structure
 *          ID tag matching, so those will not be called-out in the
 *          prologues.
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Function name:  emit_if
 *      Synopsis:       All the actions when IF is encountered
 *
 *      Associated FORTH word:                 IF
 *
 *      Inputs:
 *         Parameters:             NONE
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Items Pushed onto Control-Stack:
 *             Top:                If_FORw_IF
 *         FCode Output buffer:
 *             Token for conditional branch -- b?branch -- followed by
 *                 place-holder of zero for FCode-offset
 *              
 *
 **************************************************************************** */

void emit_if( void )
{
    emit_token("b?branch");
    mark_forward_branch( IF_CSTAG );
}

/* **************************************************************************
 *
 *      Function name:  emit_then
 *      Synopsis:       All the actions when THEN is encountered; also
 *                      part of another forward-branch resolver's action.
 *
 *      Associated FORTH words:                 THEN  ELSE
 *
 *      Inputs:
 *         Parameters:                  NONE
 *         Local Static Variables:
 *             control_stack       Points to "Top" Control-Structure Tag Group
 *         Control-Stack Items:
 *             Top:                If_FORw_IF | While_FORw_WHILE
 *
 *      Outputs:
 *         Returned Value:              NONE
 *         Control-Stack, # of Items Popped:   1
 *         FCode Output buffer:
 *             Token for forward-resolve -- b(>resolve) -- then the space
 *                 reserved for the forward-branch FCode-offset is filled
 *                 in so that it reaches the token after the  b(>resolve) . 
 *
 *      Process Explanation:
 *          The THEN statement or the ELSE statement must be able to resolve
 *              a WHILE statement, in order to implement the extended flow-
 *              -control structures as described in sec. A.3.2.3.2 of the
 *              ANSI Forth Spec.
 *          But we must prevent the sequence  IF ... BEGIN ...  REPEAT  from
 *              compiling as though it were:  IF ... BEGIN ...  AGAIN THEN
 *          We do this by having a separate CSTAG for WHILE and allowing
 *              it here but not allowing the IF_CSTAG when processing REPEAT.
 *
 **************************************************************************** */

void emit_then( void )
{
    emit_token("b(>resolve)");
    if ( control_stack != NULL )
    {
	if ( control_stack->cs_tag == WHILE_CSTAG )
	{
	    control_stack->cs_tag = IF_CSTAG;
	}
    }
    resolve_forward( IF_CSTAG );
}


/* **************************************************************************
 *
 *      Function name:  emit_else
 *      Synopsis:       All the actions when ELSE is encountered
 *
 *      Associated FORTH word:                 ELSE
 *
 *      Inputs:
 *         Parameters:             NONE
 *         Global Variables:
 *             control_stack_depth   Current depth of Control Stack
 *         Local Static Variables:
 *             not_cs_underflow      If this is FALSE after the c-s swap, it
 *                                       means an underflow resulted; skip
 *                                       the call to resolve the first marker.
 *         Control-Stack Items:
 *             Top:                {If_FORw_IF}1
 *                 (Datum is OPC of earlier forward-branch; must be resolved.)
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Control-Stack, # of Items Popped:   1
 *         Items Pushed onto Control-Stack:
 *             Top:                {If_FORw_IF}2
 *                 (Datum is current OPC, after forward-branch is placed.)
 *         FCode Output buffer:
 *             Token for unconditional branch -- bbranch-- followed by
 *                 place-holder of zero for FCode-offset.  Then, token
 *                  for forward-resolve -- b(>resolve) -- and the space
 *                  reserved earlier for the conditional forward-branch
 *                  FCode-offset is filled in to reach the token after
 *                  the  b(>resolve) .
 *
 *      Error Detection:
 *          If the "Control-Stack" is empty, bypass the forward branch
 *              and let the call to  control_structure_swap()  report
 *              the underflow error.  Then use  not_cs_underflow  to
 *              control whether to resolve the forward-branch. 
 *
 *      Process Explanation:
 *          The final item needed within the scope of what the earlier
 *              conditional branch might skip is an unconditional branch
 *              over the "else"-clause to follow.  After that, the earlier
 *              conditional branch needs to be resolved.  This last step
 *              is identical to the action of  THEN .
 *
 **************************************************************************** */

void emit_else( void )
{
    if ( control_stack_depth > 0 )
    {
	emit_token("bbranch");
	mark_forward_branch( IF_CSTAG );
    }
    not_cs_underflow = TRUE;
    control_structure_swap();
    if ( not_cs_underflow )
    {
        emit_then();
    }
}


/* **************************************************************************
 *
 *      Function name:  emit_begin
 *      Synopsis:       All the actions when BEGIN is encountered
 *
 *      Associated FORTH word:                 BEGIN
 *
 *      Inputs:
 *         Parameters:             NONE
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Items Pushed onto Control-Stack:
 *             Top:                Begin_BACKw_BEGIN
 *                 (Datum is current OPC, target of future backward-branch)
 *         FCode Output buffer:
 *             Token for target of backward branch -- b(<mark)
 *
 **************************************************************************** */

void emit_begin( void )
{
    emit_token("b(<mark)");
    mark_backward_target( BEGIN_CSTAG );
}


/* **************************************************************************
 *
 *      Function name:  emit_again
 *      Synopsis:       All the actions when AGAIN is encountered
 *
 *      Associated FORTH words:               AGAIN  REPEAT
 *
 *      Inputs:
 *         Parameters:             NONE
 *         Control-Stack Items:
 *             Top:                Begin_BACKw_BEGIN
 *                        (Datum is OPC of backward-branch target at BEGIN)
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Control-Stack, # of Items Popped:   1
 *         FCode Output buffer:
 *             Token for unconditional branch -- bbranch -- followed by
 *                 FCode-offset that reaches just after the  b(<mark) 
 *                 token at the corresponding  BEGIN  statement. 
 *
 *      Process Explanation:
 *          The FCode-offset is calculated as the difference between our
 *              current OPC and the target-OPC saved on the Control-Stack.
 *              
 **************************************************************************** */

void emit_again( void )
{
    emit_token("bbranch");
    resolve_backward( BEGIN_CSTAG );
}

/* **************************************************************************
 *
 *      Function name:  emit_until
 *      Synopsis:       All the actions when UNTIL is encountered
 *
 *      Associated FORTH word:                 UNTIL
 *
 *      Process Explanation:
 *          Same as AGAIN except token is conditional branch -- b?branch --
 *              instead of unconditional.
 *
 **************************************************************************** */

void emit_until( void )
{
    emit_token("b?branch");
    resolve_backward( BEGIN_CSTAG );
}

/* **************************************************************************
 *
 *      Function name:  emit_while
 *      Synopsis:       All the actions when WHILE is encountered
 *
 *      Associated FORTH word:                 WHILE
 *
 *      Inputs:
 *         Parameters:                  NONE
 *         Global Variables:
 *             control_stack_depth       Number of items on "Control-Stack"
 *         Control-Stack Items:
 *             Top:                      Begin_BACKw_BEGIN
 *                                 (Datum is OPC of backward-branch target)
 *
 *      Outputs:
 *         Returned Value:              NONE
 *         Control-Stack:        1 item added below top item.
 *         Items on Control-Stack:
 *             Top:                Begin_BACKw_BEGIN
 *             Next:               While_FORw_WHILE
 *         FCode Output buffer:
 *             Token for conditional branch -- b?branch -- followed by
 *                 place-holder of zero for FCode-offset
 *
 *      Error Detection:
 *          If the "Control-Stack" is empty, bypass creating the branch
 *              and let the call to  control_structure_swap()  report
 *              the underflow error.
 *
 *      Process Explanation:
 *          Output a conditional forward-branch sequence, similar to  IF 
 *              (except with a WHILE CSTAG), but be sure to leave the
 *              control-structure branch-marker that was created by the
 *              preceding  BEGIN   on top of the one just generated:
 *              the  BEGIN  needs to be resolved first in any case, and
 *              doing this here is the key to implementing the extended
 *              control-flow structures as described in sec. A.3.2.3.2
 *              of the ANSI Forth Spec.
 *
 *      Extraneous Remarks:
 *          It was for the use of this function that Wil Baden coined the
 *              name BUT for the control-structure swap routine.  The idea
 *              was that the implementation of WHILE could be boiled down
 *              to:  IF BUT   (couldn't quite fit an AND in there...;-} )
 *          Naturally, this implementation is a smidgeon more complicated...
 *
 **************************************************************************** */

void emit_while( void )
{
    if ( control_stack_depth > 0 )
    {
	emit_token("b?branch");
	mark_forward_branch( WHILE_CSTAG );
    }
    control_structure_swap();
}

/* **************************************************************************
 *
 *      Function name:  emit_repeat
 *      Synopsis:       All the actions when REPEAT is encountered
 *
 *      Associated FORTH word:                 REPEAT
 *
 *      Inputs:
 *         Parameters:                  NONE
 *         Local Static Variables:
 *             not_cs_underflow    If FALSE after first call to resolve marker,
 *                                     an underflow resulted; skip second call.
 *         Control-Stack Items:
 *             Top:                Begin_BACKw_BEGIN
 *                        (Datum is OPC of backward-branch target at BEGIN)
 *             Next:               If_FORw_IF
 *                        (Datum is OPC of FCode-offset place-holder)
 *
 *      Outputs:
 *         Returned Value:                    NONE
 *         Local Static Variables:
 *             not_consuming_two              Cleared, then restored
 *         Control-Stack, # of Items Popped:   2
 *         FCode Output buffer:
 *             Token for unconditional branch -- bbranch -- followed by
 *                 FCode-offset that reaches just after the  b(<mark) 
 *                 token at the corresponding  BEGIN  statement.  Then
 *                 the token for forward-resolve -- b(>resolve) -- and
 *                 the space reserved for the conditional forward-branch
 *                 FCode-offset is filled in so that it reaches the token
 *                 after the  b(>resolve) .
 *
 *      Process Explanation:
 *          The action is identical to that taken for AGAIN followed
 *               by the action for THEN.
 *          The Local Static Variable  not_consuming_two  gets cleared
 *               and restored by this routine.
 *
 **************************************************************************** */

void emit_repeat( void )
{
    if ( matchup_two_control_structures( BEGIN_CSTAG, WHILE_CSTAG ) )
    {
	not_cs_underflow = TRUE;
	not_consuming_two = FALSE;
	emit_again();
	if ( not_cs_underflow )
	{
            emit_token("b(>resolve)");
	    resolve_forward( WHILE_CSTAG );
	}
	not_consuming_two = TRUE;
    }
}

/* **************************************************************************
 *
 *      Function name:  mark_do
 *      Synopsis:       Common routine for marking the branches for
 *                      the "do" variants
 *
 *      Associated FORTH words:              DO  ?DO
 *
 *      Inputs:
 *         Parameters:                  NONE
 *
 *      Outputs:
 *         Returned Value:              NONE
 *         Global Variables:
 *             do_loop_depth         Incremented
 *         Items Pushed onto Control-Stack:
 *             Top:              Do_FORw_DO
 *             Next:             Do_BACKw_DO
 *         FCode Output buffer:
 *             Place-holder of zero for FCode-offset
 *
 *      Error Detection:
 *          The  do_loop_depth  counter will be used by other routines
 *              to detect misplaced "LEAVE", "UNLOOP", "I" and suchlike.
 *              (Imbalanced "LOOP"  statements are detected by the CSTag
 *              matching mechanism.)
 *
 *      Process Explanation:
 *          Just before this function is called, the forward-branching token
 *              for the "DO" variant that begins the control-structure was
 *              written to the FCode Output buffer.
 *          It needs an FCode-offset for a forward-branch to just after
 *              its corresponding "LOOP" variant and the FCode-offset
 *              associated therewith.
 *          That "LOOP" variant's associated FCode-offset is targeted
 *              to the token that follows the one for this "DO" variant
 *              and its FCode-offset.
 *          Mark the forward-branch with the C-S Tag for DO and write a
 *              place-holder FCode-offset of zero to FCode Output.
 *          Indicate that the mark that will be processed second (but which
 *              was made first) is a duplicate of the one that will be
 *              processed first.
 *          Then mark the backward-branch target, also with the DO C-S Tag.
 *          Finally, increment the  do_loop_depth  counter.
 *
 *      Extraneous Remarks:
 *          This is more complicated to describe than to code...  ;-)
 *
 **************************************************************************** */

void mark_do( void )
{
    mark_forward_branch( DO_CSTAG);
    control_stack->cs_not_dup = FALSE;
    mark_backward_target( DO_CSTAG);
    do_loop_depth++;
}


/* **************************************************************************
 *
 *      Function name:  resolve_loop
 *      Synopsis:       Common routine for resolving the branches for
 *                      the "loop" variants.
 *
 *      Associated FORTH words:              LOOP  +LOOP
 *
 *      Inputs:
 *         Parameters:                  NONE
 *         Global Variables:
 *             statbuf             Word read from input stream (either "loop"
 *                                     or "+loop"), used for Error Message.
 *         Local Static Variables:
 *             not_cs_underflow    If FALSE after first call to resolve marker,
 *                                     an underflow resulted; skip second call.
 *         Control-Stack Items:
 *             Top:                Do_FORw_DO
 *             Next:               Do_BACKw_DO
 *
 *      Outputs:
 *         Returned Value:                    NONE
 *         Global Variables:
 *             do_loop_depth                  Decremented
 *         Local Static Variables:
 *             not_consuming_two              Cleared, then restored
 *             didnt_print_otl                Set, then set again at end.
 *         Control-Stack, # of Items Popped:   2
 *         FCode Output buffer:
 *             FCode-offset that reaches just after the token of the
 *                 corresponding "DO" variant.  Then the space reserved
 *                 for the FCode-offset of the forward-branch associated
 *                 with the "DO" variant is filled in so that it reaches
 *                 the token just after the "DO" variant's FCode-offset.
 *                 
 *      Error Detection:
 *          A value of zero in  do_loop_depth  before it's decremented
 *              indicates a  DO ... LOOP  imbalance, which is an ERROR,
 *              but our other error-reporting mechanisms will catch it,
 *              so we don't check or report it here.
 *
 *      Process Explanation:
 *          Just before this function is called, the backward-branching
 *              token for the "LOOP" variant that ends the control-structure
 *              was written to the FCode Output buffer.
 *          It needs an FCode-offset for a backward-branch targeted just
 *              after its corresponding "DO" variant and the FCode-offset
 *              associated therewith.
 *          That "DO" variant's associated FCode-offset is targeted to
 *              the token that follows the one for this "LOOP" variant
 *              and its FCode-offset.
 *          Make sure there are two DO C-S Tag entries on the Control Stack.
 *          Resolve the backward-branch, matching your target to the first
 *              C-S Tag for DO
 *          Then resolve the forward-branch, targeting to your new OPC
 *              position, and also making sure you match the DO C-S Tag.
 *          We keep track of  do_loop_depth  for other error-detection
 *              by decrementing it; make sure it doesn't go below zero.
 *          Don't bother resolving the forward-branch if we underflowed
 *              the "Control Stack" trying to resolve the backward-branch.
 *          If the two top C-S Tag entries are not for a DO statement, the
 *              matchup_two_control_structures() routine will consume both
 *              or up to two of them, and we will place a dummy offset of
 *              zero to follow-up the backward-branching token that has
 *              already been written.
 *      
 *      Extraneous Remarks:
 *          This is only a little more complicated to describe
 *              than to code...  ;-)
 *
 **************************************************************************** */

void resolve_loop( void )
{
    if ( INVERSE( matchup_two_control_structures( DO_CSTAG, DO_CSTAG) ) )
    {
	emit_offset( 0 );
    }else{
	not_cs_underflow = TRUE;
	didnt_print_otl = TRUE;
	not_consuming_two = FALSE;
	resolve_backward( DO_CSTAG);
	if ( not_cs_underflow )
	{
	    resolve_forward( DO_CSTAG);
	}
	if ( do_loop_depth > 0 ) do_loop_depth--;
	not_consuming_two = TRUE;
	didnt_print_otl = TRUE;   /*  Might have gotten cleared   */
    }
}

/* **************************************************************************
 *
 *      Function name:  emit_case
 *      Synopsis:       All the actions when CASE is encountered
 *
 *      Associated FORTH word:                 CASE
 *
 *      Inputs:
 *         Parameters:             NONE
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Items Pushed onto Control-Stack:
 *             Top:              N_OFs=0...CASE_CSTAG
 *                    (Datum is 0 , Initial count of  OF .. ENDOF  pairs)
 *         FCode Output buffer:
 *             Token for start of a CASE structure -- b(case)
 *                 Does not require an FCode-offset.
 *
 **************************************************************************** */

void emit_case( void )
{
    push_cstag( CASE_CSTAG, 0);
    emit_token("b(case)");
}


/* **************************************************************************
 *
 *      Function name:  emit_of
 *      Synopsis:       All the actions when OF is encountered
 *
 *      Associated FORTH word:                 OF
 *
 *      Inputs:
 *         Parameters:             NONE
 *         Control-Stack Items:
 *             Top:                N_OFs...CASE_CSTAG
 *                    (Datum is OF-count, number of  OF .. ENDOF  pairs)
 *            {Next and beyond}:   {Endof_FORw_ENDOF}1..n_ofs
 *            { Repeat for OF-count number of times }
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Control-Stack, 1 Item Pushed, 1 modified:
 *             Top:                Of_FORw_OF
 *             Next:               N_OFs+1...CASE_CSTAG
 *                    (Datum has been incremented)
 *            {3rd and beyond}:    {Endof_FORw_ENDOF}1..n_ofs
 *            { Repeat for 1 through the un-incremented OF-count }
 *            (Same as Next etcetera at input-time.)
 *         FCode Output buffer:
 *             Token for OF statement -- b(of) -- followed by
 *                 place-holder FCode-offset of zero
 *
 *      Error Detection:
 *          Matchup CASE-cstag before incrementing OF-count
 *
 *      Process Explanation:
 *          Main difference between this implementation and that outlined
 *              in "the book" (see below) is that we do not directly use
 *              the routine for the IF statement's flow-control; we will
 *              use a different CSTAG for better mismatch detection.
 *
 *      Extraneous Remarks:
 *          This is a "by the book" (ANSI Forth spec, section A.3.2.3.2)
 *              implementation (mostly).  Incrementing the OF-count here,
 *              after we've matched up the CSTAG, gives us (and the user)
 *              just a little bit more protection...
 *
 **************************************************************************** */

void emit_of( void )
{

    if ( matchup_control_structure( CASE_CSTAG ) )
    {
	emit_token("b(of)");

	/*
	 *  See comment-block about "Control-Stack" Diagram Notation
	 *       early on in this file.
	 *
	 */

	/* (    {Endof_FORw_ENDOF}1..n_ofs  N_OFs...CASE_CSTAG -- )          */

	/*  Increment the OF-count .  */
	(control_stack->cs_datum)++;

	/* (    {Endof_FORw_ENDOF}1..n_ofs  N_OFs+1...CASE_CSTAG -- )        */

	mark_forward_branch( OF_CSTAG );
	/* ( -- {Endof_FORw_ENDOF}1..n_ofs  N_OFs+1...CASE_CSTAG Of_FORw_OF )
	 */
    }
    /*  Leave the CSTAG-Group on the "Control-Stack" .  */
}


/* **************************************************************************
 *
 *      Function name:  emit_endof
 *      Synopsis:       All the actions when ENDOF is encountered
 *
 *      Associated FORTH word:                 ENDOF
 *
 *      Inputs:
 *         Parameters:             NONE
 *         Control-Stack Items:
 *             Top:                Of_FORw_OF
 *             Next:               N_OFs+1...CASE_CSTAG
 *                    (Datum has been incremented)
 *            {3rd and beyond}:    {Endof_FORw_ENDOF}1..n_ofs
 *            { Repeat for 1 through the un-incremented OF-count )
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Control-Stack, 1 Item Popped, 1 new Item Pushed.
 *             Top:                N_OFs...CASE_CSTAG
 *                    (The count itself is unchanged from input-time, but
 *                         the number of {Endof_FORw_ENDOF} CSTAG-Groups
 *                         has caught up with this number, so it is
 *                         no longer notated as " + 1 ").
 *            {Next and beyond}:   {Endof_FORw_ENDOF}1..n_ofs
 *            { Repeat for 1 through the updated OF-count )
 *         FCode Output buffer:
 *             Token for ENDOF statement -- b(endof) -- followed by
 *                 place-holder FCode-offset of zero.  Then the space reserved
 *                 for the FCode-offset of the forward-branch associated
 *                 with the "OF" statement is filled in so that it reaches
 *                 the token just after the "ENDOF" statement's FCode-offset.
 *
 *      Error Detection:
 *          If control-stack depth  is not at least 2, CS underflow ERROR
 *              and no further action.
 *          Routine that resolves the forward-branch checks for matchup error.
 *
 **************************************************************************** */

void emit_endof( void )
{
    if ( control_stack_size_test( 2) )
    {
	emit_token("b(endof)");

	/*  See "Control-Stack" Diagram Notation comment-block  */

	/*  Stack-diagrams might need to be split across lines.  */

	/* (    {Endof_FORw_ENDOF}1..n_ofs  N_OFs+1...CASE_CSTAG  ...  
	 *                       ...                          Of_FORw_OF -- )
	 */
	mark_forward_branch(ENDOF_CSTAG);
	/* ( -- {Endof_FORw_ENDOF}1..n_ofs  N_OFs+1...CASE_CSTAG  ...  
	 *                       ...  Of_FORw_OF  {Endof_FORw_ENDOF}n_ofs+1 )
	 */

	control_structure_swap();
	/* ( -- {Endof_FORw_ENDOF}1..n_ofs  N_OFs+1...CASE_CSTAG  ...
	 *                       ...  {Endof_FORw_ENDOF}n_ofs+1  Of_FORw_OF )
	 */

	resolve_forward( OF_CSTAG );
	/* ( -- {Endof_FORw_ENDOF}1..n_ofs  N_OFs+1...CASE_CSTAG        ...
	 *                       ...  {Endof_FORw_ENDOF}n_ofs+1  )
	 */

	control_structure_swap();
	/* ( -- {Endof_FORw_ENDOF}1..n_ofs         ...
	 *                       ...  {Endof_FORw_ENDOF}n_ofs+1   ...
	 *                                        ...  N_OFs+1...CASE_CSTAG )
	 */

	/*  The number of ENDOF-tagged Forward-Marker pairs has now
	 *     caught up with the incremented OF-count; therefore,
	 *     we can notate the above as:
	 *
	 *  ( {Endof_FORw_ENDOF}1..n_ofs  N_OFs CASE_CSTAG )
	 *
	 *     and we are ready for another  OF ... ENDOF  pair,
	 *     or for the ENDCASE statement.
	 */
    }

}

/* **************************************************************************
 *
 *      Function name:  emit_endcase
 *      Synopsis:       All the actions when ENDCASE is encountered
 *
 *      Associated FORTH word:                 ENDCASE
 *
 *      Inputs:
 *         Parameters:             NONE
 *         Control-Stack Items:
 *             Top:                N_OFs...CASE_CSTAG
 *                    (Datum is OF-count, number of  OF .. ENDOF  pairs)
 *            {Next and beyond}:   {Endof_FORw_ENDOF}1..n_ofs
 *            { Repeat for OF-count number of times }
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Control-Stack, # of Items Popped:  OF-count + 1
 *         FCode Output buffer:
 *             Token for ENDCASE statement -- b(endcase)
 *             Then the spaces reserved for the FCode-offsets of all the
 *                 forward-branches associated with the OF-count number
 *                 of ENDOF statements are filled in so that they reach
 *                 the token just after this "ENDCASE" statement.
 *
 *      Error Detection:
 *          Routine that resolves the forward-branch checks for matchup error
 *              for each forward-branch filled in, plus the matchup routine
 *              checks before the OF-count is retrieved.
 *
 *      Process Explanation:
 *          Retrieve the OF-count and resolve that number of ENDOF statements
 *      
 *      Extraneous Remarks:
 *          The setup makes coding this routine appear fairly simple...  ;-}
 *
 **************************************************************************** */

void emit_endcase( void )
{
    unsigned long n_endofs ;
    if ( matchup_control_structure( CASE_CSTAG) )
    {
	int indx;

	emit_token("b(endcase)");
	n_endofs = control_stack->cs_datum;
	for ( indx = 0 ; indx < n_endofs ; indx++ )
	{
	    /*  Because  matchup_control_structure  doesn't pop the
	     *      control-stack, we have the  N_OFs...CASE_CSTAG
	     *      item on top of the  Endof_FORw_ENDOF  item we
	     *      want to resolve.  We need to keep it there so
	     *      the  POP  is valid for the other path as well
	     *      as at the end of this one.
	     *  So we  SWAP  to get at the  Endof_FORw_ENDOF  item.
	     */
	    control_structure_swap();
	    resolve_forward( ENDOF_CSTAG);
	}
    }
    pop_cstag();
}


/* **************************************************************************
 *
 *      Function name:  control_struct_incomplete
 *      Synopsis:       Print a Message of given severity with origin info for
 *                          a control-structure that has not been completed.
 *
 *      Inputs:
 *         Parameters:
 *             c_s_entry             Control-structure about which to display
 *             severity              Severity of the messages to display.
 *             call_cond             String identifying Calling Condition;
 *                                       used in the message.
 *
 *      Outputs:
 *         Returned Value:           NONE
 *             
 *         Printout:
 *             Message of given severity...
 *
 *      Process Explanation:
 *          The calling routine will be responsible for all filtering of
 *               duplicate structures and the like.  This routine will
 *               simply display a message.
 *
 **************************************************************************** */

static void control_struct_incomplete(
			    int severity,
				char *call_cond,
				    cstag_group_t *c_s_entry)
{
    tokenization_error ( severity,
	"%s before completion of %s" ,
	    call_cond, strupr(c_s_entry->cs_word));
    where_started( c_s_entry->cs_inp_fil, c_s_entry->cs_line_num );
}

/* **************************************************************************
 *
 *      Function name:  announce_control_structs
 *      Synopsis:       Print a series of Messages (of severity as specified)
 *                          announcing that the calling event is occurring
 *                          in the context of Control-Flow structure(s),
 *                          back to the given limit.  Leave the control
 *                          structures in effect.
 *
 *      Inputs:
 *         Parameters:
 *             severity              Severity of the messages to display.
 *             call_cond             String identifying Calling Condition;
 *                                       used in the message.
 *             abs_token_limit       Limit, in terms of abs_token_no
 *         Local Static Variables:
 *             control_stack         Pointer to "Top" of "Control-Stack"
 *
 *      Outputs:
 *         Returned Value:           NONE
 *         Printout:
 *             A Message for each unresolved Control-Flow structure.
 *
 **************************************************************************** */

void announce_control_structs( int severity, char *call_cond,
				          unsigned int abs_token_limit)
{
    cstag_group_t *cs_temp = control_stack;
    while ( cs_temp != NULL )
    {
	if ( cs_temp->cs_abs_token_num < abs_token_limit )
	{
	    break;
	}
	if ( cs_temp->cs_not_dup )
	{
	    control_struct_incomplete( severity, call_cond, cs_temp );
	}
	cs_temp = cs_temp->prev;
    }
}

/* **************************************************************************
 *
 *      Function name:  clear_control_structs_to_limit
 *      Synopsis:       Clear items from the "Control-Stack" back to the given
 *                          limit.  Print error-messages with origin info for
 *                          control-structures that have not been completed.
 *
 *      Inputs:
 *         Parameters:
 *             call_cond                 String identifying Calling Condition;
 *                                           used in the Error message.
 *             abs_token_limit           Limit, in terms of abs_token_no
 *         Global Variables:
 *             control_stack_depth       Number of items on "Control-Stack"
 *             control_stack             Pointer to "Top" of "Control-Stack"
 *         Control-Stack Items:
 *             The  cs_inp_fil  and  cs_line_num  tags of any item cleared
 *                 from the "Control-Stack" are used in error-messages.
 *
 *      Outputs:
 *         Returned Value: 
 *         Global Variables:
 *             do_loop_depth             Decremented when "DO" item cleared.
 *             control_stack_depth       Decremented by called routine.
 *         Control-Stack, # of Items Popped:  As many as go back to given limit
 *         Memory Freed
 *             By called routine.
 *
 *      Error Detection:
 *          Any item on the "Control-Stack" represents a Control-Structure
 *              that was not completed when the Calling Condition was
 *              encountered.  Error; identify the origin of the structure.
 *          No special actions if  noerrors  is set.
 *
 *      Process Explanation:
 *          The given limit corresponds to the value of  abs_token_no  at
 *              the time the colon-definition (or whatever...) was created.
 *              Any kind of Control-Structure imbalance at the end of the
 *              colon-definition is an error and the entries must be cleared,
 *              but the colon-definition may have been created inside nested
 *              interpretation-time Control-Structures, and those must be
 *              preserved. 
 *             
 *          Of course, if this routine is called with a given limit of zero,
 *              that would mean all the entries are to be cleared.  That will
 *              be the way  clear_control_structs()  is implemented.
 *          We control the loop by the  cs_abs_token_num  field, but also
 *              make sure we haven't underflowed  control_stack_depth
 *          We skip messages and other processing for items that are duplicates
 *                    of others, based on the  cs_not_dup  field.
 *               If the cs_tag field is  DO_CSTAG  we decrement  do_loop_depth
 *          The  pop_cstag()  routine takes care of the rest.
 *               
 *      Extraneous Remarks:
 *          This is a retrofit; necessary because we now  permit definitions
 *              to occur inside interpretation-time Control-Structures.  Calls
 *              to  clear_control_structs()  are already scattered around...
 *
 **************************************************************************** */

void clear_control_structs_to_limit( char *call_cond,
				          unsigned int abs_token_limit)
{
    while ( control_stack_depth > 0 )
    {
	if ( control_stack->cs_abs_token_num < abs_token_limit )
	{
	    break;
	}
	if ( control_stack->cs_not_dup )
	{
	    control_struct_incomplete( TKERROR, call_cond, control_stack );
	    if ( control_stack->cs_tag == DO_CSTAG) do_loop_depth--;
	}
	pop_cstag();
    }
}

/* **************************************************************************
 *
 *      Function name:  clear_control_structs
 *      Synopsis:       Make sure the "Control-Stack" is cleared, and print
 *                          error-messages (giving origin information) for
 *                          control-structures that have not been completed.
 *
 *      Inputs:
 *         Parameters:
 *             call_cond                 String identifying Calling Condition;
 *                                           used in the Error message.
 *         Global Variables:
 *             control_stack_depth       Number of items on "Control-Stack"
 *             control_stack             Pointer to "Top" of "Control-Stack"
 *         Control-Stack Items:
 *             The  cs_inp_fil  and  cs_line_num  tags of any item found on
 *                 the "Control-Stack" are used in error-messages.
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Global Variables:
 *             control_stack_depth       Reset to zero.
 *             do_loop_depth             Reset to zero.
 *         Control-Stack, # of Items Popped:    All of them
 *             
 *      Error Detection:
 *          Any item on the "Control-Stack" represents a Control-Structure
 *              that was not completed when the Calling Condition was
 *              encountered.  Error; identify the origin of the structure.
 *          No special actions if  noerrors  is set.
 *
 *      Process Explanation:
 *          Filter the duplicate messages caused by structures (e.g., DO)
 *              that place two entries on the "Control-Stack" by testing
 *              the  cs_not_dup  field of the "Top" "Control-Stack" item,
 *              which would indicate double-entry...
 *
 *      Extraneous Remarks:
 *          This is called before a definition of any kind, and after a 
 *              colon-definition.  Flow-control constructs should *never*
 *              be allowed to cross over between immediate-execution mode
 *              and compilation mode.  Likewise, not between device-nodes.
 *          Also, at the end of tokenization, there should not be any
 *              unresolved flow-control constructs.
 *
 **************************************************************************** */

void clear_control_structs( char *call_cond)
{
    clear_control_structs_to_limit( call_cond, 0);
}
