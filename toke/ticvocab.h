#ifndef _TOKE_TICVOCAB_H
#define _TOKE_TICVOCAB_H

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
 *      General-purpose support structures and function prototypes for
 *          Threaded Interpretive Code (T. I. C.)-type vocabularies.
 *          See ticvocab.c
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Structures:
 *
 *      The Threaded Interpretive Code Header data structure consists of:
 *          (1)  A pointer to the function name as seen in the source
 *          (2)  A link to the next (well, preceding) element
 *          (3)  A Pointer to the routine to run as the function's behavior
 *               during active processing.  It takes the "parameter field"
 *               item as its argument and has no return-value.
 *          (4)  The "parameter field" item, which will be passed as an
 *               argument to the function. It may be an FCode- or FWord-
 *               -token, or an item of another type, or a pointer to data.
 *               The function may re-cast it as needed.
 *          (5)  The "Definer" of the entry; a member of the subset of FWord-
 *               -tokens that may be Definers.
 *          (6)  A flag indicating whether the item is one for which a
 *                      single-token FCode number is assigned. 
 *          (7)  A Pointer to a routine to be run when the word is encountered
 *               in a Conditional Compilation section that is being ignored.
 *               Certain functions still need to be recognized in that state,
 *               and will require special behaviors; those that can be simply
 *               ignored will have a NULL pointer in this field.  The function
 *               in this field, also, takes the "parameter field" item as its
 *               argument and has no return-value.
 *          (8)  The size of the data pointed to by the "parameter field" item,
 *               if it is a pointer to allocated data; used to allocate space
 *               for a copy of the p.f. when making an alias, and to indicate
 *               whether the space needs to be freed.  Automatic procedures
 *               are too often fooled, so we don't leave things to chance...
 *          (9)  A flag, set TRUE if the word is on the Trace List, to indicate
 *               that an Invocation Message should be displayed when the word
 *               is invoked.
 *
 *      To accommodate C's insistence on strong-typing, we might need
 *          to define different "parameter field" structure-types; see
 *          description of "Parameter Field as a union", below.
 *
 *      It's not an exact name, but we'll still call it a T. I. C. Header
 *
 *      We will use this structure for most of our vocabularies...
 *
 ****************************************************************************
 *
 *     The "Parameter Field" is a union of several alternative types,
 *         for convenience, to avert excessive type-casting.  It may
 *         either be the Parameter Data itself (if it is small enough)
 *         or a pointer to Parameter Data.
 *     In the case where the Parameter Field is the data itself, or is a
 *         pointer to hard-coded (non-allocated) data, the "size" field
 *         will be zero, to indicate that no space allocation is required.
 *     The types are (starting with the default for initialization)
 *         A long integer (including an FCode Token)
 *         An FWord Token
 *         A pointer to a boolean variable
 *         A pointer to a character or a character string
 *         If the parameter field value is intended to be a single character,
 *             it must be recast as a long integer, in order to preserve
 *             portability across big- and little- -endian platforms.
 *
 **************************************************************************** */

/* ************************************************************************** *
 *
 *      Macros:
 *          NO_PARAM_TIC       Create an entry in the initial "Built-In"
 *                                 portion of a TIC-type vocabulary with
 *                                 an empty "param field".
 *          NO_PARAM_IGN       Create an entry with an empty "param field"
 *                                 whose action- and "Ignore"- -functions
 *                                 are the same.
 *          VALPARAM_TIC       " ... with a literal value for the "param field"
 *          DUALFUNC_TIC       " ... with a literal "param" and both an action-
 *                                 and an "Ignore"- -function.
 *          DUFNC_FWT_PARM      A  tic_fwt_hdr_t -type entry with a literal
 *                                 "param" and both an action-function and an
 *                                 "Ignore"-function.
 *          FWORD_TKN_TIC      " ... with an FWord Token for the "param field"
 *          DUALFUNC_FWT_TIC   " " ... and both action- and "Ignore"- -function
 *          BUILTIN_MAC_TIC    A  tic_mac_hdr_t -type entry with a Macro-
 *                                 -Substitution-String "param"
 *          BUILTIN_BOOL_TIC   A  tic_bool_hdr_t -type entry with a boolean-
 *                                 -variable-pointer "param"
 *
 **************************************************************************** */

#include "types.h"
#include "dictionary.h"

/* **************************************************************************
 *
 *      In order to work around C's limitations regarding data-typing
 *          during initialization, we need to create some alternative
 *          data-structures that exactly match the layout of  tic_hdr_t
 *          as defined above, but have a different default-type for
 *          the parameter field.
 *      I apologize in advance for any maintenance awkwardnesses this
 *          might engender; I would have come up with a more convenient
 *          way to do this if I could.  At least, the pieces that need
 *          to be co-ordinated are in close proximity to each other...
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      The differences are all centered in the "parameter field" of
 *          the TIC header structure.  In order to make sure all the
 *          alternative types map smoothly into each other, we will
 *          create a series of "union" types, differing only in what
 *          their default type is.  We will keep the "parameter field"
 *          item the size of a Long Integer.
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      This form is the default type.
 *
 **************************************************************************** */

#define TIC_P_DEFLT_TYPE  long
typedef union tic_param
    {
	TIC_P_DEFLT_TYPE  deflt_elem ;  /*  The "default element" aspect.  */
	long              long_val ;    /*  Explicitly specifying "long"   */
	fwtoken           fw_token ;    /*  FWord Token                    */
	bool             *bool_ptr ;    /*  Pointer to a boolean variable  */
	char             *chr_ptr  ;    /*  Pointer to a character string  */

	/*  The "character value" aspect behaves differently on big-
	 *      versus little- -endian platforms (for initialization,
	 *      anyway), so we cannot actually use it.
	 *  Keep this commented-out, as a reminder.
	 */
     /* char             char_val ;    */
	/*  We will recast "character value" as an integer.  */

    } tic_param_t ;

typedef struct tic_hdr
    {
        char             *name;
	struct tic_hdr   *next;
	void            (*funct)();      /*  Function for active processing  */
	tic_param_t       pfield;
	fwtoken           fword_defr;    /*  FWord Token of entry's Definer  */
	bool              is_token;      /*  Is entry a single-token FCode?  */
	void            (*ign_func)();   /*  Function in "Ignored" segment   */
	int               pfld_size;
	bool              tracing;       /*  TRUE if Invoc'n Msg required    */
    }  tic_hdr_t ;

/* **************************************************************************
 *
 *      This form is for initialization of an FWord Token list.
 *
 **************************************************************************** */

#define TIC_FWT_P_DEFLT_TYPE  fwtoken
typedef union tic_fwt_param
    {
	TIC_FWT_P_DEFLT_TYPE  deflt_elem ;  /*  "Default element" aspect.  */
	long              long_val ;    /*  Long integer                   */
	fwtoken           fw_token ;    /*  Explicit FWord Token           */
	bool             *bool_ptr ;    /*  Pointer to a boolean variable  */
	char             *chr_ptr  ;    /*  Pointer to a character string  */
    } tic_fwt_param_t ;

typedef struct tic_fwt_hdr
    {
        char               *name;
	struct tic_fwt_hdr *next;
	void              (*funct)();    /*  Function for active processing  */
	tic_fwt_param_t     pfield;
	fwtoken             fword_defr;  /*  FWord Token of entry's Definer  */
	bool                is_token;    /*  Is entry a single-token FCode?  */
	void              (*ign_func)(); /*  Function in "Ignored" segment   */
	int                 pfld_size;
	bool                tracing;     /*  TRUE if Invoc'n Msg required    */
    }  tic_fwt_hdr_t ;


/* **************************************************************************
 *
 *      The third form is for initialization of Built-in Macros.  Its
 *          default parameter field type is a pointer to a compiled-in
 *          (i.e., constant) character string.  Its  pfld_size  can be
 *          left as zero, because no allocated-memory copies of the
 *          string will be needed, even for aliases.  (User-created
 *          macros, however, will need to allocate strings; we will
 *          deal with that elsewhere...)
 *
 **************************************************************************** */

#define TIC_MAC_P_DEFLT_TYPE  char *
typedef union tic_mac_param
    {
	TIC_MAC_P_DEFLT_TYPE  deflt_elem ;  /*  "Default element" aspect.  */
	long              long_val ;    /*  Long integer                   */
	fwtoken           fw_token ;    /*  FWord Token                    */
	bool             *bool_ptr ;    /*  Pointer to a boolean variable  */
	char            *chr_ptr ;      /*  Explicit Pointer to char str.  */
    } tic_mac_param_t ;

typedef struct tic_mac_hdr
    {
        char               *name;
	struct tic_mac_hdr *next;
	void              (*funct)();
	tic_mac_param_t     pfield;
	fwtoken             fword_defr;
	bool                is_token;    /*  Is entry a single-token FCode?  */
	void              (*ign_func)();
	int                 pfld_size;
	bool                tracing;     /*  TRUE if Invoc'n Msg required    */
    }  tic_mac_hdr_t ;

/* **************************************************************************
 *
 *      The next form is for initialization of Condtionals.  Its default
 *          parameter field type is a pointer to a boolean variable.
 *
 **************************************************************************** */

#define TIC_BOOL_P_DEFLT_TYPE  bool *
typedef union tic_bool_param
    {
	TIC_BOOL_P_DEFLT_TYPE deflt_elem ;  /*  "Default element" aspect.  */
	long              long_val ;    /*  Long integer                   */
	fwtoken           fw_token ;    /*  FWord Token                    */
	bool             *bool_ptr ;    /*  Explicit Ptr to boolean v'ble  */
	char             *chr_ptr  ;    /*  Pointer to a character string  */
    } tic_bool_param_t ;

typedef struct tic_bool_hdr
    {
        char                *name;
	struct tic_bool_hdr *next;
	void               (*funct)();
	tic_bool_param_t     pfield;
	fwtoken              fword_defr;
	bool                is_token;    /*  Is entry a single-token FCode?  */
	void               (*ign_func)();
	int                  pfld_size;
	bool                 tracing;    /*  TRUE if Invoc'n Msg required    */
    }  tic_bool_hdr_t ;



/* **************************************************************************
 *
 *      Various macros to create an entry in the initial "Built-In" portion
 *           of a vocabulary-list, specified as an array of one of the
 *           matching forms of a T. I. C. Header; each macro adjusts type
 *           casting as needed.
 *      In all cases, the "size" field will be set to zero, indicating that
 *         the "param field" item is either the complete data or a pointer
 *         to a hard-coded (non-allocated) data item.
 *
 **************************************************************************** */

/* **************************************************************************
 *          Macro Name:   NO_PARAM_TIC
 *                        Create an entry in the initial "Built-In" portion
 *                            of a TIC_HDR -type vocabulary with an empty
 *                            "param field"
 *                            
 *   Arguments:
 *       nam      (string)         Name of the entry as seen in the source
 *       func     (routine-name)   Name of internal function to call
 *       The item is not one for which single-token FCode number is assigned.
 *
 **************************************************************************** */
#define NO_PARAM_TIC(nam, func )  \
  { nam , (tic_hdr_t *)NULL , func ,   \
        { 0 }, UNSPECIFIED , FALSE , NULL , 0 , FALSE }


/* **************************************************************************
 *          Macro Name:   NO_PARAM_IGN
 *                        Create an entry with an empty "param field"
 *                            whose action-function and "Ignore"-function
 *                            are the same.
 *                            
 *   Arguments:
 *       nam      (string)         Name of the entry as seen in the source
 *       func     (routine-name)   Name of internal function to call for both
 *       The item is not one for which single-token FCode number is assigned.
 *
 **************************************************************************** */
#define NO_PARAM_IGN(nam, func )  \
  { nam , (tic_hdr_t *)NULL , func ,   \
        { 0 }, UNSPECIFIED , FALSE , func , 0 , FALSE }


/* **************************************************************************
 *
 *      Variations on the above, to compensate for Type-Casting complications
 *
 **************************************************************************** */

/* **************************************************************************
 *          Macro Name:   VALPARAM_TIC
 *                        Create an entry in the initial "Built-In" portion
 *                            of a TIC_HDR -type vocabulary with a literal
 *                            value for the "param field" and a specifiable
 *                            "definer".
 *   Arguments:
 *       nam      (string)         Name of the entry as seen in the source
 *       func     (routine-name)   Name of internal function to call
 *       pval     (integer)        The "param field" item
 *       definr   (fword_token)    "Definer" of the entry
 *       is_tok   (bool)           Is the "param" item a single-token FCode?
 *
 *      The "param field" item will be recast to the required default type.
 *
 **************************************************************************** */

#define VALPARAM_TIC(nam, func, pval, definr, is_tok )  \
    { nam , (tic_hdr_t *)NULL , func ,  \
        { (TIC_P_DEFLT_TYPE)(pval) }, definr , is_tok , NULL , 0 , FALSE }


/* **************************************************************************
 *          Macro Name:   DUALFUNC_TIC
 *                        Create an entry in the initial "Built-In" portion
 *                            of a TIC_HDR -type vocabulary with both an
 *                            "active" and an "ignoring" function, a literal
 *                            value for the "param field" and a specifiable
 *                            "definer".
 *                            
 *   Arguments:
 *       nam      (string)         Name of the entry as seen in the source
 *       afunc    (routine-name)   Name of internal "active" function
 *       pval     (integer)        The "param field" item
 *       ifunc    (routine-name)   Name of "ignoring" function
 *       definr   (fword_token)    "Definer" of the entry
 *
 *      The "param field" item will be recast to the required default type.
 *      The item is not one for which single-token FCode number is assigned.
 *
 **************************************************************************** */
#define DUALFUNC_TIC(nam, afunc, pval, ifunc, definr )  \
    { nam , (tic_hdr_t *)NULL , afunc ,  \
        { (TIC_P_DEFLT_TYPE)(pval) }, definr , FALSE , ifunc , 0 , FALSE }

/*  Similar but a  tic_fwt_hdr_t  type structure  */
#define DUFNC_FWT_PARM(nam, afunc, pval, ifunc, definr )  \
    { nam , (tic_fwt_hdr_t *)NULL , afunc ,  \
        { (TIC_FWT_P_DEFLT_TYPE)(pval) }, definr , FALSE , ifunc , 0 , FALSE }


/* **************************************************************************
 *          Macro Name:   FWORD_TKN_TIC
 *                        Create an entry in the initial "Built-In" portion
 *                            of an FWord Token list of  tic_fwt_hdr_t  type.
 *                            
 *   Arguments:
 *       nam         (string)         Name of the entry as seen in the source
 *       func        (routine-name)   Name of internal function to call
 *       fw_tokval   (fword_token)    Value of the FWord Token
 *       definr      (fword_token)    "Definer" of the entry
 *
 *      The "param field" item should not need be recast.
 *      The item is not one for which single-token FCode number is assigned.
 *
 **************************************************************************** */

#define FWORD_TKN_TIC(nam, func, fw_tokval, definr )    \
    { nam , (tic_fwt_hdr_t *)NULL , func , { fw_tokval },  \
      definr , FALSE , NULL , 0 , FALSE }

/* **************************************************************************
 *          Macro Name:   DUALFUNC_FWT_TIC
 *                        Create an entry in the initial "Built-In" portion
 *                            of an FWord Token list of  tic_fwt_hdr_t  type
 *                            with both an action- and an "Ignore"- -function.
 *                            
 *   Arguments:
 *       nam         (string)         Name of the entry as seen in the source
 *       afunc       (routine-name)   Name of internal "Action" function
 *       fw_tokval   (fword_token)    Value of the FWord Token
 *       ifunc       (routine-name)   Name of "ignoring" function
 *       definr      (fword_token)    "Definer" of the entry
 *
 *      The "param field" item should not need be recast.
 *      The item is not one for which single-token FCode number is assigned.
 *
 **************************************************************************** */
#define DUALFUNC_FWT_TIC(nam, afunc, fw_tokval, ifunc, definr )    \
    { nam , (tic_fwt_hdr_t *)NULL , afunc , { fw_tokval }, \
      definr , FALSE , ifunc , 0 , FALSE }

/* **************************************************************************
 *          Macro Name:   BUILTIN_MAC_TIC
 *                        Create an entry in the initial "Built-In" portion
 *                            of a Macros vocabulary of  tic_mac_hdr_t  type.
 *                            
 *   Arguments:
 *       nam         (string)         Name of the entry as seen in the source
 *       func        (routine-name)   Name of internal function to call
 *       alias       (string)         Macro-Substitution string
 *
 *      The "param field" item should not need be recast.
 *      The "definer" will be MACRO_DEF
 *      Builtin Macros do not need to be expanded while Ignoring, so
 *          the ign_func will be NULL
 *      The item is not one for which single-token FCode number is assigned.
 *
 **************************************************************************** */

#define BUILTIN_MAC_TIC(nam, func, alias )    \
    { nam , (tic_mac_hdr_t *)NULL , func , { alias }, \
      MACRO_DEF , FALSE , NULL , 0 , FALSE }


/* **************************************************************************
 *          Macro Name:   BUILTIN_BOOL_TIC
 *                        Create an entry in the initial "Built-In" portion
 *                            of a Condtionals list of  tic_bool_hdr_t  type.
 *                            
 *   Arguments:
 *       nam         (string)          Name of the entry as seen in the source
 *       func        (routine-name)    Name of internal function to call
 *       bool_vbl    (boolean v'ble)   Name of the boolean variable.
 *
 *      The "param field" item should not need be recast.
 *      For all of the Condtionals, the "Ignoring" function is the same
 *          as the "Active" function.
 *      The "definer" will be COMMON_FWORD
 *      The item is not one for which single-token FCode number is assigned.
 *
 **************************************************************************** */

#define BUILTIN_BOOL_TIC(nam, func, bool_vbl )    \
    { nam , (tic_bool_hdr_t *)NULL , func , { &bool_vbl },   \
        COMMON_FWORD , FALSE , func , 0 , FALSE }


/* **************************************************************************
 *
 *     Exported Variables and Function Prototypes the rest of the way...
 *
 **************************************************************************** */

extern tic_hdr_t *tic_found;

void init_tic_vocab( tic_hdr_t *tic_vocab_tbl,
                         int max_indx,
			     tic_hdr_t **tic_vocab_ptr);
void add_tic_entry( char *tname,
                        void (*tfunct)(),
                             TIC_P_DEFLT_TYPE tparam,
                                 fwtoken fw_defr,
				     int pfldsiz,
                                         bool is_single,
                                         void (*ign_fnc)(),
                                             tic_hdr_t **tic_vocab );
tic_hdr_t *lookup_tic_entry( char *tname, tic_hdr_t *tic_vocab );
bool exists_in_tic_vocab( char *tname, tic_hdr_t *tic_vocab );
bool handle_tic_vocab( char *tname, tic_hdr_t *tic_vocab );
bool create_split_alias( char *new_name, char *old_name,
                              tic_hdr_t **src_vocab, tic_hdr_t **dest_vocab );
bool create_tic_alias( char *new_name, char *old_name, tic_hdr_t **tic_vocab );
void reset_tic_vocab( tic_hdr_t **tic_vocab, tic_hdr_t *reset_position );

#endif   /*  _TOKE_TICVOCAB_H    */
