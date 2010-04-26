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
 *      Process activity inside "Tokenizer Escape" mode.
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Exported:
 *          init_tokz_esc_vocab      Initialize the relevant vocabulary.
 *          enter_tokz_esc           Enter "Tokenizer Escape" mode
 *          create_tokz_esc_alias    Add an alias to "Tokenizer Escape" space
 *          reset_tokz_esc           Reset the "Tokenizer Escape" Vocabulary
 *                                      to its "Built-In" position.
 *
 **************************************************************************** */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tokzesc.h"
#include "ticvocab.h"
#include "stack.h"
#include "emit.h"
#include "stream.h"
#include "scanner.h"
#include "errhandler.h"
#include "strsubvocab.h"
#include "nextfcode.h"
#include "tracesyms.h"

#undef TOKZTEST     /*  Define for testing only; else undef   */
#ifdef TOKZTEST         /*  For testing only   */
   #include "vocabfuncts.h"
   #include "date_stamp.h"
#endif                  /*  For testing only   */

/* **************************************************************************
 *
 *          Global Variables Imported
 *              in_tokz_esc    State of the tokenization operation.
 *                                Value is FALSE if it's operating normally
 *                                or TRUE if it's in "Tokenizer Escape" mode.
 *              nextfcode  
 *              statbuf    
 *              dstack         Pointer to current item on top of Data Stack.   
 *              base           Current tokenizer numeric conversion radix
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      We are going to implement a mini-forth using a strategy based
 *          on the concept of Threaded Interpretive Code  (well, okay,
 *          it won't really be interpretive ... )
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *              Internal Static Variables -- (well, almost)
 *      tokz_esc_vocab        Pointer to "tail" of "Tokenizer Escape" Vocab
 *
 *      While we would prefer to keep  tokz_esc_vocab  private to this file,
 *          we find we need it in one other place, namely, macros.c
 *      We will declare it "extern" within that file, rather than
 *          exporting it widely by including it in a  .h  file
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      We will define and initialize a structure consisting of all the
 *          functions we initially support for "Tokenizer Escape" mode,
 *          called  tokz_esc_vocab_tbl  for "Tokenizer Escape Vocab Table".
 *      I expect we can initialize its body, but I don't think there's a
 *          convenient way in "C" to initialize its links; we'll have to
 *          do that dynamically, at run-time.  Oh, well...
 *
 *      We'll call the pointer to it the "Tokenizer Escape" Vocabulary.
 *          We have to declare it here, because we need to refer to it in
 *          a function that will be entered into the Table.
 *
 *      We would like to pre-initialize it; to do so, we would have to
 *          declare it extern here, then later create its instance and
 *          assign its initial value.  That would be all well and good,
 *          except that some linkers don't handle that very well, so, to
 *          accommodate those, we have to include its initialization in
 *          the same routine where we initialize the Table's links.
 *
 *      Revision History:
 *          Updated Wed, 04 Jan 2006 by David L. Paktor
 *          Initialization of tokz_esc_vocab is now included with
 *              the call to  init_tic_vocab()
 *
 **************************************************************************** */

tic_hdr_t *tokz_esc_vocab = NULL ;

/* **************************************************************************
 *
 *      We'll give short prologs to the simpler functions that will be
 *          used in the "Tokenizer Escape" Vocabulary.
 *
 *      All these take, as an argument, the "parameter field" pointer.
 *          To satisfy C's strong-typing, it will always be declared
 *          of a consistent type.  The routine itself can internally
 *          recast -- or ignore -- it, as needed.
 *      Many of these will refer to the Global Variable  statbuf .  It
 *          all such cases, it is used for Error reporting.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Function name:  enter_tokz_esc
 *      Synopsis:       When you start "Tokenizer Escape" mode.
 *
 *      Associated Tokenizer directive:        TOKENIZER[
 *
 *      Process Explanation:
 *          Enter "Tokenizer Escape" mode ...  Save the current tokenizer
 *              numeric conversion radix, and set the radix to sixteen
 *              (hexadecimal)
 *
 **************************************************************************** */

static int saved_base ;    /*  Place to save the numeric conversion radix  */

void enter_tokz_esc( void )
{
    saved_base = base ;
    base = 16;
    in_tokz_esc = TRUE;
}

/* **************************************************************************
 *
 *      Function name:  end_tokz_esc
 *      Synopsis:       When you've reached the end of "Tokenizer Escape" mode.
 *
 *      Associated Tokenizer directive:        ]TOKENIZER
 *
 *      Process Explanation:
 *          Exit "Tokenizer Escape" mode, resume "Normal"-mode behavior.
 *              Restore the tokenizer numeric conversion radix to the value
 *              saved by tokenizer[ and exit "Tokenizer Escape" mode . . .
 *
 **************************************************************************** */

static void end_tokz_esc( tic_param_t pfield )
{
    in_tokz_esc = FALSE;
    base = saved_base ;
}

/* **************************************************************************
 *
 *      Function name:  tokz_esc_emit_byte
 *      Synopsis:       Emit the byte found on the data-stack
 *                      
 *      Associated Tokenizer directive:        EMIT-BYTE
 *                      
 *      Error Detection:
 *          If number on stack is larger than a byte:  Truncate and Warn
 *
 **************************************************************************** */

static void tokz_esc_emit_byte ( tic_param_t pfield )
{
    long num_on_stk = dpop();
    u8 byt_to_emit = (u8)num_on_stk;
    if ( byt_to_emit != num_on_stk )
    {
        tokenization_error( WARNING,
	    "Value on stack for %s command is 0x%0x.  "
		"Truncating to 0x%02x.\n",
		     strupr(statbuf), num_on_stk, byt_to_emit);
    }
    user_emit_byte(byt_to_emit);
}

/* **************************************************************************
 *
 *      Function name:  get_fcode_from_stack
 *      Synopsis:       Retrieve an FCode value from the data-stack.
 *                          Indicate if erroneous
 *
 *      Inputs:
 *         Parameters:
 *             the_num           Pointer to where to put the number
 *             setting_fc        TRUE if number is to be set as next fcode,
 *                                   FALSE if the number is to be emitted
 *         Data-Stack Items:
 *             Top:              Value to be retrieved
 *
 *      Outputs:
 *         Returned Value:       TRUE if number is within valid FCode range
 *         Supplied Pointers:
 *             *the_num          Value retrieved from the data-stack
 *
 *      Process Explanation:
 *          From the value of   setting_fc  we will deduce both the legal
 *              minimum and the phrase to be used in the ERROR message.
 *              If the number is to be emitted, it can be any legal FCode
 *              token number down to  0x10 but if it is to be set, the
 *              legal minimum is 0x0800.
 *          In either case, 0x0fff is the legal maximum.
 *
 *      Error Detection:
 *          If the number on the stack is larger than 16 bits, truncate
 *              it, with a WARNING message.
 *          If the (possibly truncated) number taken from the stack is
 *              larger than the legal maximum for an FCode, or if it is
 *              less than the legal minimum, issue an ERROR message,
 *              leave the_num unchanged and return FALSE.
 *
 *      Extraneous Remarks:
 *          If this function is ever used in more than the two ways allowed
 *              by the  setting_fc  parameter, then the Right Thing would
 *              be to define a local ENUM type for the possible uses, and
 *              use a SWITCH statement to set the internal variables.  (But
 *              I really don't see any way that could become necessary...)
 *
 **************************************************************************** */

static bool get_fcode_from_stack( u16 *the_num, bool setting_fc)
{
    bool retval = FALSE;
    char *the_action = "emit FCode value of";
    u16 legal_minimum = 0x10;
    long num_on_stk = dpop();
    u16 test_fcode = (u16)num_on_stk;

    if ( setting_fc )
    {
        the_action = "set next fcode to";
	legal_minimum = 0x800;
    }
    if ( test_fcode != num_on_stk )
    {
        tokenization_error( WARNING,
	    "Value on stack for %s command is 0x%0x.  "
		"Truncating to 0x%03x.\n",
		     strupr(statbuf), num_on_stk, test_fcode);
    }
    if ( ( test_fcode >= legal_minimum ) && ( test_fcode <= 0xfff ) )
    {
	retval = TRUE;
	*the_num = test_fcode;
    }else{	tokenization_error( TKERROR, "Attempt to %s "
	    "0x%x, which violates limit specified by IEEE-1275.  "
		"Disallowing.\n", the_action, test_fcode );
    }
    return( retval );
}


/* **************************************************************************
 *
 *      Function name:  tokz_esc_next_fcode
 *      Synopsis:       Set the next-fcode to the value on the data-stack
 *                      
 *      Associated Tokenizer Directive:          next-fcode
 *                      
 *      Error Detection:
 *          If the number on the stack is not legal for an FCode, as
 *              detected by  get_fcode_from_stack(), issue an ERROR
 *              message and don't change nextfcode.
 *
 *         Printout:
 *             Advisory showing change in FCode token Assignment Counter
 *
 **************************************************************************** */

static void tokz_esc_next_fcode( tic_param_t pfield )
{
    u16 test_fcode;

    if ( get_fcode_from_stack( &test_fcode, TRUE) )
    {
	if ( test_fcode == nextfcode )
	{
	    tokenization_error( INFO, "FCode-token Assignment Counter "
		"is unchanged from 0x%x.\n",
		    nextfcode);
	}else{
	    tokenization_error( INFO, "FCode-token Assignment Counter "
		"was 0x%x; has been %s to 0x%x.\n",
		    nextfcode,
			test_fcode > nextfcode ? "advanced" : "reset",
			    test_fcode );
	    set_next_fcode( test_fcode);
	}
    }
}

/* **************************************************************************
 *
 *      Function name:  tokz_emit_fcode
 *      Synopsis:       Emit the value on the data-stack as an FCode token
 *                      
 *      Associated Tokenizer Directive:          emit-fcode
 *                      
 *      Error Detection:
 *          If the number on the stack is not legal for an FCode, as
 *              detected by  get_fcode_from_stack(), issue an ERROR
 *              message and don't emit anything.
 *
 *         Printout:
 *             Advisory showing FCode being emitted.
 *
 **************************************************************************** */

static void tokz_emit_fcode( tic_param_t pfield )
{
    u16 test_fcode;

    if ( get_fcode_from_stack( &test_fcode, FALSE) )
    {
	tokenization_error( INFO,
	    "Emitting FCode value of 0x%x\n", test_fcode);
	emit_fcode( test_fcode);
    }
}


/* **************************************************************************
 *
 *      Function name:  zero_equals
 *      Synopsis:       Boolean-inversion of item on top of stack.
 *                      If zero, make it minus-one; if non-zero, make it zero. 
 *
 *      Associated FORTH word-name:          0=
 *
 **************************************************************************** */

static void  zero_equals ( tic_param_t pfield )
{
    *dstack = (*dstack == 0) ? -1 : 0 ;
}

/* **************************************************************************
 *
 *      Function name:  tokz_esc_swap
 *      Synopsis:       "Tokenizer Escape" mode-time data-stack SWAP operation
 *                      
 *      Associated FORTH word-name:          swap
 *
 **************************************************************************** */

static void  tokz_esc_swap ( tic_param_t pfield )
{
    swap();
}

/* **************************************************************************
 *
 *      Function name:  tokz_esc_two_swap
 *      Synopsis:       "Tokenizer Escape" mode-time data-stack 2SWAP operation
 *                      
 *      Associated FORTH word-name:          2swap
 *
 **************************************************************************** */

static void  tokz_esc_two_swap ( tic_param_t pfield )
{
    two_swap();
}

/* **************************************************************************
 *
 *      Function name:  tokz_esc_noop
 *      Synopsis:       "Tokenizer Escape" mode-time non-operation
 *                      
 *      Associated FORTH word-name:          noop
 *
 **************************************************************************** */

static void  tokz_esc_noop ( tic_param_t pfield )
{
    return;
}

#ifdef TOKZTEST         /*  For testing only   */

   static void  tokz_esc_emit_string( tic_param_t pfield )
   {
      int lenny ;
      lenny = strlen ( pfield.chr_ptr );
      emit_token("b(\")");
      emit_string(pfield.chr_ptr, lenny);
   }

#endif                  /*  For testing only   */

/* **************************************************************************
 *
 *      Function name:  do_constant
 *      Synopsis:       The function to perform when a named constant
 *                          that was defined in "Tokenizer Escape" mode
 *                          is invoked in "Tokenizer Escape" mode
 *
 *      Associated FORTH word:                 A user-defined constant
 *      
 *      Inputs:
 *         Parameters:
 *             The param-field of the table-entry contains
 *                 the value of the constant
 *
 *      Outputs:
 *         Returned Value:         NONE
 *         Items Pushed onto Data-Stack:
 *             Top:              The table-entry's param-field's value
 *
 *      Error Detection:
 *          An attempt, while operating in normal tokenization mode, to invoke
 *              a named constant that was defined in "Tokenizer Escape" mode
 *              will be detected by the main scanning loop, and need not
 *              concern us here.
 *
 **************************************************************************** */

static void do_constant ( tic_param_t pfield )
{
    dpush( pfield.long_val );
}

/* **************************************************************************
 *
 *      Function name:  create_constant
 *      Synopsis:       Create a user-defined constant that will be
 *                          recognized in "Tokenizer Escape" mode
 *
 *      Associated FORTH word-name:             CONSTANT  (when issued
 *                                                  in "Tokenizer Escape" mode)
 *
 *      Inputs:
 *         Parameters:                NONE
 *         Global Variables:
 *             statbuf         The constant's name will be taken from the
 *                                 next word in the input stream.
 *             do_constant     The function assigned to the definition
 *             tokz_esc_vocab  The "Tokenizer Escape" Vocabulary pointer
 *         Data-Stack Items:
 *             Top:            The constant's value is popped from the stack
 *
 *      Outputs:
 *         Returned Value:            NONE
 *         Global Variables:
 *             statbuf         Advanced to the next word in the input stream.
 *             tokz_esc_vocab      Updated to point to new vocab entry.
 *         Memory Allocated:
 *             for the name and for the new entry.
 *         When Freed?
 *             When RESET-SYMBOLS is issued in "Tokenizer Escape" mode,
 *                or upon end of tokenization.
 *         Data-Stack, # of Items Popped:             1
 *
 *      Error Detection:
 *          Failure to allocate memory is a Fatal Error.
 *          Warning on excessively long name
 *          Name to be defined not in same file, ERROR
 *              Warning on duplicate name handled by support routine
 *
 *      Process Explanation:
 *          Get the next word, STRDUP it (which implicitly allocates memory). 
 *              Get the number popped off the stack.
 *              Pass the pointer and the value to the add_tic_entry() routine.
 *
 **************************************************************************** */

static void create_constant( tic_param_t pfield )
{
    char *c_name_space ;          /*  Space for copy of the name    */
    long valu ;                   /*  Value, popped off the stack   */
    signed long wlen;

    /*  Get the name that is to be defined  */
    wlen = get_word();
    if ( wlen <= 0 )
    {
	warn_unterm( TKERROR, "Constant definition", lineno);
	return;
    }

    valu = dpop();

    /*  If ever we implement more than just this one
     *      defining-word in "Tokenizer Escape" mode,
     *      the lines from here to the end of the
     *      routine should be re-factored...
     */
    c_name_space = strdup( statbuf );

    add_tic_entry(
	 c_name_space,
	     do_constant,
		  (TIC_P_DEFLT_TYPE)valu,
		       CONST ,
			  0 , FALSE , NULL,
		               &tokz_esc_vocab );

    check_name_length( wlen );

}

/* **************************************************************************
 *
 *      Let's create usable named constants for "Tokenizer Escape" mode.
 *          It's useful, it's easy and ...  well, you get the idea.
 *
 **************************************************************************** */
/*  I don't think we need to any more
static const int zero = 0 ;
static const int minus_one = -1 ;
static const char double_quote = '"' ;
static const char close_paren = ')' ;
 *  Except for this next one, of course...   */
#ifdef TOKZTEST        /*  For testing only   */
   static const char date_me[] =  DATE_STAMP;
#endif                 /*  For testing only   */

/* **************************************************************************
 *
 *      Here, at long last, we define and initialize the structure containing
 *          all the functions we support in "Tokenizer Escape" mode.
 *      Let's call it the "Tokenizer Escape Vocabulary Table" and the
 *          pointer to it, the "Tokenizer Escape" Vocabulary.
 *      We can initialize the start of the "Tokenizer Escape" Vocabulary
 *          easily, except for the link-pointers, as an array.
 *
 **************************************************************************** */

#define TKZESC_CONST(nam, pval)   \
                        VALPARAM_TIC(nam, do_constant, pval, CONST, FALSE )
#define TKZ_ESC_FUNC(nam, afunc, pval, ifunc)   \
                        DUALFUNC_TIC(nam, afunc, pval, ifunc, UNSPECIFIED)

static tic_hdr_t tokz_esc_vocab_tbl[] = {
    NO_PARAM_IGN( "]tokenizer" , end_tokz_esc                           ) ,

    /*  An IBM-ish synonym.  */
    NO_PARAM_IGN( "f]"         , end_tokz_esc                           ) ,
    /*  An alternate synonym.  */
    NO_PARAM_IGN( "]f"         , end_tokz_esc                           ) ,

    NO_PARAM_TIC( "emit-byte"  , tokz_esc_emit_byte                     ) ,
    NO_PARAM_TIC( "next-fcode" , tokz_esc_next_fcode                    ) ,
    NO_PARAM_TIC( "emit-fcode" , tokz_emit_fcode                        ) ,
    NO_PARAM_TIC( "constant"   , create_constant                        ) ,
    NO_PARAM_TIC( "0="         , zero_equals                            ) ,
    NO_PARAM_TIC( "swap"       , tokz_esc_swap                          ) ,
    NO_PARAM_TIC( "2swap"      , tokz_esc_two_swap                      ) ,
    NO_PARAM_TIC( "noop"       , tokz_esc_noop                          ) ,
    TKZESC_CONST( "false"      ,  0                                     ) ,
    TKZESC_CONST( "true"       , -1                                     ) ,
    TKZ_ESC_FUNC( ".("         , user_message, ')', skip_user_message   ) ,
    TKZ_ESC_FUNC( ".\""        , user_message, '"', skip_user_message   ) ,
#ifdef TOKZTEST        /*  For testing only   */
    /*  Data is a pointer to a constant string in the compiler;    */
    /*      no need to copy, hence data_size can remain zero.      */
    /*  We could almost use the Macro macro, except for the type.  */
    TKZ_ESC_FUNC( "emit-date"  , tokz_esc_emit_string, date_me , NULL   ) ,
#endif                 /*  For testing only   */
};

/* **************************************************************************
 *
 *      Also, keep a pointer to the "Built-In" position of
 *          the "Tokenizer Escape" Vocabulary
 *
 **************************************************************************** */

static const tic_hdr_t *built_in_tokz_esc =
    &tokz_esc_vocab_tbl[(sizeof(tokz_esc_vocab_tbl)/sizeof(tic_hdr_t))-1];

/* **************************************************************************
 *
 *      Function name:  init_tokz_esc_vocab
 *      Synopsis:       Initialize the "Tokenizer Escape" Vocabulary
 *                          link-pointers dynamically.
 *
 *      Process Explanation:
 *          While this is going on, set  in_tokz_esc  to TRUE; clear it
 *              when done.  This will be used by the  trace_builtin
 *              routine...
 *
 **************************************************************************** */

void init_tokz_esc_vocab ( void )
{
    static const int tokz_esc_vocab_max_indx =
	 sizeof(tokz_esc_vocab_tbl)/sizeof(tic_hdr_t) ;

    in_tokz_esc = TRUE;
    tokz_esc_vocab = NULL ;   /*  Belt-and-suspenders...  */
    init_tic_vocab(tokz_esc_vocab_tbl,
                       tokz_esc_vocab_max_indx,
		           &tokz_esc_vocab );
    in_tokz_esc = FALSE;
}

/* **************************************************************************
 *
 *      Function name:  lookup_tokz_esc
 *      Synopsis:       Return a pointer to the data-structure of the named
 *                      word in the"Tokenizer Escape" Vocabulary
 *
 *      Inputs:
 *         Parameters:
 *             name                 The given name for which to look
 *         Local Static Variables:
 *             tokz_esc_vocab       Pointer to "Tokenizer Escape" Vocabulary  
 *
 *      Outputs:
 *         Returned Value:     TRUE if name is found,
 *
 **************************************************************************** */

tic_hdr_t *lookup_tokz_esc(char *name)
{
    tic_hdr_t *retval = lookup_tic_entry( name, tokz_esc_vocab );
    return ( retval );
}


/* **************************************************************************
 *
 *      Function name:  create_tokz_esc_alias
 *      Synopsis:       Create an alias in the "Tokenizer Escape" Vocabulary
 *
 *      Associated FORTH word:              ALIAS (in "Tokenizer Escape" mode)
 *
 *      Inputs:
 *         Parameters:
 *             old_name             Name of existing entry
 *             new_name             New name for which to create an entry
 *
 *      Outputs:
 *         Returned Value:          TRUE if  old_name  found in "Tok Esc" vocab
 *         Global Variables:    
 *             tokz_esc_vocab       Will point to the new entry
 *         Memory Allocated:
 *             Memory for the new entry will be allocated
 *                 by the support routine
 *         When Freed?
 *             When RESET-SYMBOLS is issued in "Tokenizer Escape" mode,
 *                or upon end of tokenization.
 *
 **************************************************************************** */

bool create_tokz_esc_alias(char *new_name, char *old_name)
{
    bool retval = create_tic_alias( new_name, old_name, &tokz_esc_vocab );
    return ( retval );
}


/* **************************************************************************
 *
 *      Function name:  reset_tokz_esc
 *      Synopsis:       Reset the "Tokenizer Escape" Vocabulary to
 *                          its "Built-In" position.
 *
 *      Associated Tokenizer directive:       RESET-SYMBOLS  (when issued
 *                                                in "Tokenizer Escape" mode)
 *
 *      Inputs:
 *         Parameters:                 NONE
 *         Global Variables:
 *             tokz_esc_vocab      Pointer to "Tokenizer Escape" Vocabulary
 *             built_in_tokz_esc   Pointer to "Built-In" position
 *
 *      Outputs:
 *         Returned Value:             NONE
 *         Global Variables:
 *             tokz_esc_vocab      Reset to "Built-In" position
 *         Memory Freed
 *             Memory will be freed by the support routine
 *
 **************************************************************************** */

void reset_tokz_esc( void )
{
    reset_tic_vocab( &tokz_esc_vocab, (tic_hdr_t *)built_in_tokz_esc);
}

/* **************************************************************************
 *
 *      Function name:  pop_next_fcode
 *      Synopsis:       Vector off to the  tokz_esc_next_fcode  function,
 *                      but without the pseudo-param.  A retro-fit...
 *
 *      Associated Tokenizer directive:   FCODE-POP  (issued in either mode)
 *
 *      Inputs:
 *         Parameters:                    NONE
 *         Data-Stack Items:
 *             Top:                       Next FCode value, presumably saved
 *                                            by an  FCODE-PUSH  issued earlier.
 *
 *      Outputs:
 *         Returned Value: 
 *         Global Variables:
 *             nextfcode                  FCode token Assignment Counter
 *
 **************************************************************************** */

void pop_next_fcode( void)
{
   tic_param_t dummy_param;
   tokz_esc_next_fcode( dummy_param);
}
