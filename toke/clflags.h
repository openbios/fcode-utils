#ifndef _TOKE_CLFLAGS_H
#define _TOKE_CLFLAGS_H

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
 *      Function Prototypes and External declarations
 *          for Command-Line Flags Support
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Structure-Types:
 *          cl_flag_t            Data for recognizing and setting Command-Line
 *                                   Flags, and for displaying the help message
 *
 *      Function Prototypes / Functions Exported:
 *          set_cl_flag
 *          list_cl_flag_settings
 *          list_cl_flag_names          Display just the names of the CL Flags
 *          cl_flags_help
 *
 *      Global Variables Exported
 *                (These flags, which are set by the set_cl_flag() routine,
 *                     are used throughout the Tokenizer to control certain
 *                     non-Standard variant behaviors.)
 *          ibm_locals
 *          ibm_locals_legacy_separator
 *          ibm_legacy_separator_message
 *          enable_abort_quote
 *          sun_style_abort_quote
 *          sun_style_checksum
 *          string_remark_escape
 *          hex_remark_escape
 *          c_style_string_escape
 *          always_headers
 *          always_external
 *          verbose_dup_warning
 *          obso_fcode_warning
 *          trace_conditionals
 *          force_tokens_case
 *          force_lower_case_tokens
 *          big_end_pci_image_rev
 *          clflag_help
 *
 **************************************************************************** */

/* **************************************************************************
 *          Structure-Type Name:    cl_flag_t
 *               Data for recognizing and setting Command-Line Flags,
 *                   and for displaying the help message
 *
 *   Fields:
 *       clflag_name     *char          CL Flag name, as entered by the user
 *       flag_var        *bool          Address of boolean ("flag") variable
 *       clflag_tabs     *char          Tabs to align the explanations, in
 *                                          the help message display
 *       clflag_expln    *char          Explanation, used in help message
 *
 *   Since this structure will be initialized by the program, and will not
 *       be added-to, we can structure it as purely an array, and have no
 *       need to treat it as a linked list, hence no link-field.
 *
 **************************************************************************** */

#include "types.h"

typedef struct cl_flag
    {
	char             *clflag_name;
	bool             *flag_var;
	char             *clflag_tabs;
	char             *clflag_expln;
    }  cl_flag_t ;


/* **************************************************************************
 *
 *          Exported Global Variables
 *
 **************************************************************************** */

extern bool ibm_locals;
extern bool ibm_locals_legacy_separator;
extern bool ibm_legacy_separator_message;
extern bool enable_abort_quote;
extern bool sun_style_abort_quote;
extern bool sun_style_checksum;
extern bool abort_quote_throw;
extern bool string_remark_escape;
extern bool hex_remark_escape;
extern bool c_style_string_escape;
extern bool always_headers;
extern bool always_external;
extern bool verbose_dup_warning;
extern bool obso_fcode_warning;
extern bool trace_conditionals;
extern bool big_end_pci_image_rev;

extern bool force_tokens_case;
extern bool force_lower_case_tokens;
extern bool allow_ret_stk_interp;

extern bool clflag_help;

/* **************************************************************************
 *
 *          Exported Functions
 *
 **************************************************************************** */

bool set_cl_flag(char *flag_name, bool print_message);
void cl_flags_help(void);
void list_cl_flag_names(void);
void show_all_cl_flag_settings(bool from_src);
void list_cl_flag_settings(void);
void save_cl_flags(void);
void reset_cl_flags(void);

#endif   /*  _TOKE_CLFLAGS_H    */
