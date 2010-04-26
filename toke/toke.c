/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  toke.c - main tokenizer loop and parameter parsing.
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
#include <string.h>
#include <unistd.h>

#ifdef __GLIBC__
#define _GNU_SOURCE
#include <getopt.h>
#endif

#include "types.h"
#include "toke.h"
#include "stream.h"
#include "stack.h"
#include "emit.h"

#define TOKE_VERSION "1.0.3"

#include "vocabfuncts.h"
#include "scanner.h"
#include "errhandler.h"
#include "usersymbols.h"
#include "clflags.h"
#include "tracesyms.h"

#define CORE_COPYR   "(C) Copyright 2001-2010 Stefan Reinauer.\n" \
		     "(C) Copyright 2006 coresystems GmbH <info@coresystems.de>"
#define IBM_COPYR    "(C) Copyright 2005 IBM Corporation.  All Rights Reserved."

/*  Temporary hack during development...  See DATE_STAMP  line below... */
#ifdef DEVEL
#include "date_stamp.h"
#endif /*  DEVEL  */

/* **************************************************************************
 *
 *     Global Variables Exported:
 *        verbose          If true, enable optional messages.
 *        noerrors         If true, create binary even if error(s) encountered.
 *        fload_list       If true, create an "FLoad-List" file
 *        dependency_list  If true, create a "Dependencies-List" file
 *
 **************************************************************************** */

bool verbose         = FALSE;
bool noerrors        = FALSE;
bool fload_list      = FALSE;
bool dependency_list = FALSE;

/* **************************************************************************
 *
 *              Internal Static Variables
 *         outputname    Name of output file supplied on command-line
 *                           with the optional  -o  switch.
 *              Internal System Variable
 *         optind        Index into argv vector of first param after options,
 *                           from which input file names will be taken.
 *
 **************************************************************************** */

static char *outputname = NULL;

/* **************************************************************************
 *
 *    Print the copyright message.
 *
 **************************************************************************** */
static void print_copyright(void)
{
	printf( "Welcome to toke - FCode tokenizer v" TOKE_VERSION "\n"
		CORE_COPYR "\n" IBM_COPYR "\n"
		"This program is free software; you may redistribute it "
		"under the terms of\nthe GNU General Public License v2. "
		"This program has absolutely no warranty.\n\n");
#ifdef DEVEL
        /*  Temporary hack during development... */
	printf( "\tTokenizer Compiled " DATE_STAMP "\n" );
#endif /*  DEVEL  */

}

/* **************************************************************************
 *
 *      Function name:    usage
 *      Synopsis:         Print convenient usage-help message
 *
 **************************************************************************** */

static void usage(char *name)
{
	printf("usage: %s [-v] [-i] [-l] [-P] [-o target] <[-d name[=value]]> "
				"<[-f [no]flagname]> <[-I dir-path]> "
				"<[-T symbol]> <forth-file>\n\n",name);
	printf("  -v|--verbose          print Advisory messages\n");
	printf("  -i|--ignore-errors    don't suppress output after errors\n");
	printf("  -l|--load-list        create list of FLoaded file names\n");
	printf("  -P|--dependencies     create dePendency-list file\n");
	printf("  -o|--output-name      send output to filename given\n");
	printf("  -d|--define           create user-defined symbol\n");
	printf("  -f|--flag             set (or clear) Special-Feature flag\n");
	printf("  -I|--Include          add a directory to the Include-List\n");
	printf("  -T|--Trace            add a symbol to the Trace List\n");
	printf("  -h|--help             print this help message\n\n");
	printf("  -f|--flag    help     Help for Special-Feature flags\n");
}

/* **************************************************************************
 *
 *      Function name:    get_args
 *      Synopsis:         Parse the Command-Line option switches
 *      
 *      Inputs:
 *         Parameters:                  NONE
 *         Global Variables:
 *                    argc      Counter of command-line arguments
 *                    argv      Vector pointing to command-line arguments
 *         Command-Line Items:  The entire command-line will be parsed
 *
 *      Outputs:
 *         Returned Value:              NONE
 *         Global Variables:
 *                verbose	     set by "-v" switch
 *                noerrors           set by "-i" switch
 *                fload_list         set by "-l" switch
 *                dependency_list    set by "-P" switch
 *         Internal Static Variables
 *                outputname         set by "-o" switch
 *         Internal System Variable
 *                optind             Index into argv vector of the position
 *                                       from which to take input file names.
 *         Printout:
 *                (Copyright was already printed by the main body as a
 *                    matter of course, rather than here depending on
 *                    the Verbose flag, because  getopt()  prints its
 *                    own error messages and we want to be sure to show
 *                    the Copyright notice before any error messages.)
 *                Rules for Usage and Flags-list or Flags-help display:
 *                  Usage message on Help-Request or error.
 *                  Ask for usage help, get Usage plus list of Flag Names.
 *                  Ask for Flags-help alone, get Flags-help (names plus
 *                      explanations)
 *                  Ask for usage help and for Flags-help, get Usage plus
 *                      Flags-help, without redundant list of Flag Names.
 *                  Any help-request, exit Zero
 *                  Error in Option switches, or missing input-file name,
 *                      get error-description plus Usage
 *                  Error in Flag Names, get list of Flag Names.
 *                  Any error, exit One.
 *         Behavior:
 *                Exit (non-failure) after printing "Help" message
 *
 *      Error Detection:     Exit with failure status on:
 *          Unknown Option switches or Flag Names
 *          Missing input file name
 *
 *      Process Explanation:
 *           The following switches are recognized:
 *               v
 *               h
 *               ?
 *               i
 *               I
 *               l
 *               P
 *               o
 *               d
 *               f
 *               T
 *           The conditions they set remain in effect through
 *               the entire program run.
 *
 *      Revision History:
 *          Updated Fri, 15 Jul 2005 by David L. Paktor
 *              Don't bail on first invalid option.
 *              Flags to control special features
 *              Usage messages for "special-feature" flags
 *          Updated Mon, 18 Jul 2005 by David L. Paktor
 *              Fine-tune Usage and Flags-list or Flags-help display.
 *          Updated Sun, 27 Nov 2005 by David L. Paktor
 *              Add FLoad-List flag
 *          Updated Wed, 29 Nov 2005 by David L. Paktor
 *              Make getopt() case-insensitive
 *          Updated Fri, 17 Mar 2006 by David L. O'Paktor
 *              Make getopt() case-sensitive again,
 *                  add include-list support and dePendency-list switch
 *
 *      Extraneous Remarks:
 *          We were originally thinking about defining various classes
 *              of "Warning" Messages and (somehow) controlling their
 *              display, but now that we have "special-feature" flags
 *              that control the generation of specific messages, that
 *              step has become unnecessary...
 *
 **************************************************************************** */

static void get_args( int argc, char **argv )
{
	const char *optstring="vhilPo:d:f:I:T:?";
	int c;
	int argindx = 0;
	bool inval_opt = FALSE;
	bool help_mssg = FALSE;
	bool cl_flag_error = FALSE;

	while (1) {
#ifdef __GLIBC__
		int option_index = 0;
		static struct option long_options[] = {
			{ "verbose", 0, 0, 'v' },
			{ "help", 0, 0, 'h' },
			{ "ignore-errors", 0, 0, 'i' },
			{ "load-list",     0, 0, 'l' },
			{ "dependencies",  0, 0, 'P' },
			{ "output-name",   1, 0, 'o' },
			{ "define",        1, 0, 'd' },
			{ "flag",          1, 0, 'f' },
			{ "Include",       1, 0, 'I' },
			{ "Trace",         1, 0, 'T' },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long (argc, argv, optstring,
				 long_options, &option_index);
#else
		c = getopt (argc, argv, optstring);
#endif
		if (c == -1)
			break;

		argindx++;
		switch (c) {
		case 'v':
			verbose=TRUE;
			break;
		case 'o':
			outputname = optarg;
			break;
		case 'i':
			noerrors = TRUE;
			break;
		case 'l':
			fload_list = TRUE;
			break;
		case 'P':
			dependency_list = TRUE;
			break;
		case 'd':
			{
			    char *user_symb = optarg;
			    add_user_symbol(user_symb);
			}
			break;
		case 'f':
			cl_flag_error = set_cl_flag(optarg, FALSE) ;
			break;
		case 'I':
			{
			    char *incl_list_elem = optarg;
			    add_to_include_list(incl_list_elem);
			}
			break;
		case 'T':
			add_to_trace_list(optarg);
			break;
		case '?':
			/*  Distinguish between a '?' from the user
			 *  and one  getopt()  returned
			 */
			if ( argv[argindx][1] != '?' )
			{
			    inval_opt = TRUE;
			    break;
			}
		case 'h':
		case 'H':
			 help_mssg = TRUE;		
			break;
		default:
			/*  This is never executed
			 *  because  getopt()  prints the
			 *    "unknown option -- X"
			 *  message and returns a '?'
			 */
			printf ("%s: unknown options.\n",argv[0]);
			usage(argv[0]);
			exit( 1 );
		}
	}

	if ( help_mssg )
	{
	    usage(argv[0]);
	    if ( ! clflag_help )
	    {
	        list_cl_flag_names();
	    }
	}
	if ( clflag_help )  cl_flags_help();
	if ( help_mssg || clflag_help )
	{
	    exit( 0 );
	}

	if ( inval_opt )      printf ("unknown options.\n");
	if (optind >= argc)   printf ("Input file name missing.\n");
	if ( inval_opt || (optind >= argc) )
	{
		usage(argv[0]);
	}
	if ( cl_flag_error )  list_cl_flag_names();

	if ( inval_opt || (optind >= argc) || cl_flag_error )
	{
	    exit( 1);
	}

	if (verbose)
	{
	    list_user_symbols();
	    list_cl_flag_settings();
	    display_include_list();
	}
	show_trace_list();
	save_cl_flags();
}

/* **************************************************************************
 *
 *      Main body of program.  Return 0 for success, 1 for failure.
 *
 *      Still to be done:
 *          Devise a syntax to allow the command-line to specify multiple
 *              input files together with an output file name for each.
 *          Currently, the syntax allows only one output file name to be
 *              specified; when multiple input file names are specified,
 *              the specification of an output file name is disallowed,
 *              and only the default output file names are permitted.
 *              While this works around the immediate problem, a more
 *              elegant solution could be devised...
 *
 **************************************************************************** */

int main(int argc, char **argv)
{
	int retval = 0;

	print_copyright();
	get_args( argc, argv );

	init_error_handler();

	init_stack();
	init_dictionary();

	init_scanner();
	
	if ( outputname != NULL )
	{
	    if ( argc > optind + 1 )
	    {
	    /*  Multiple input file names w/ single output file name  */
		/*  Work-around  */
		printf( "Cannot specify single output file name "
			"with multiple input file names.\n"
			"Please either remove output-file-name specification,\n"
			"or use multiple commands.\n");
	        exit ( -2 );
	    }
		}

	for ( ; optind < argc ; optind++ )
	{
	    bool stream_ok ;

	    printf("\nTokenizing  %s   ", argv[optind]);
	    init_error_handler();
	    stream_ok = init_stream( argv[optind]);
	    if ( stream_ok )
	    {
		init_output(argv[optind], outputname);

		init_scan_state();

		reset_vocabs();
		reset_cl_flags();

		tokenize();
		finish_headers();
		
		close_stream( NULL);
		if ( close_output() )  retval = 1;
	    }
	}
	
	exit_scanner();
	return retval;
}

