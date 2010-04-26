/*
 *                     OpenBIOS - free your system! 
 *                        ( FCode detokenizer )
 *                          
 *  detok.c parameter parsing and main detokenizer loop.  
 *  
 *  This program is part of a free implementation of the IEEE 1275-1994 
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2010 Stefan Reinauer <stepan@openbios.org>
 *  Copyright (C) 2006 coresystems GmbH <info@coresystems.de>
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

#include "detok.h"
#include "stream.h"
#include "addfcodes.h"

#define DETOK_VERSION "1.0.3"

#define CORE_COPYR   "(C) Copyright 2001-2010 Stefan Reinauer.\n" \
		     "(C) Copyright 2006 coresystems GmbH <info@coresystems.de>"
#define IBM_COPYR    "(C) Copyright 2005 IBM Corporation.  All Rights Reserved."

bool verbose = FALSE;
bool decode_all = FALSE;
bool show_linenumbers = FALSE;
bool show_offsets = FALSE;

/*   Param is FALSE when beginning to detokenize,
 *       TRUE preceding error-exit   */
static void print_copyright(bool is_error)
{
	typedef void (*vfunct) ();	/*  Pointer to function returning void  */
	vfunct pfunct;
	char buffr[512];

	sprintf(buffr,
		"Welcome to detok - FCode detokenizer v" DETOK_VERSION "\n" 
		CORE_COPYR "\n" IBM_COPYR "\n"
		"Written by Stefan Reinauer <stepan@openbios.org>\n"
		"This program is free software; you may redistribute it "
		"under the terms of\nthe GNU General Public License v2. "
		"This program has absolutely no warranty.\n\n");

	pfunct = (is_error ? (vfunct) printf : printremark);

	(*pfunct) (buffr);
}

static void usage(char *name)
{
	printf("usage: %s [OPTION]... [FCODE-FILE]...\n\n"
	       "         -v, --verbose     print fcode numbers\n"
	       "         -a, --all         don't stop at end0\n"
	       "         -n, --linenumbers print line numbers\n"
	       "         -o, --offsets     print byte offsets\n"
	       "         -f, --fcodes      add FCodes from list-file\n"
	       "         -h, --help        print this help text\n\n", name);
}

int main(int argc, char **argv)
{
	int c;
	const char *optstring = "vhanof:?";
	int linenumbers = 0;
	bool add_vfcodes = FALSE;
	char *vfc_filnam = NULL;

	while (1) {
#ifdef __GLIBC__
		int option_index = 0;
		static struct option long_options[] = {
			{"verbose", 0, 0, 'v'},
			{"help", 0, 0, 'h'},
			{"all", 0, 0, 'a'},
			{"linenumbers", 0, 0, 'n'},
			{"offsets", 0, 0, 'o'},
			{"fcodes", 1, 0, 'f'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, optstring,
				long_options, &option_index);
#else
		c = getopt(argc, argv, optstring);
#endif
		if (c == -1)
			break;

		switch (c) {
		case 'v':
			verbose = TRUE;
			break;
		case 'a':
			decode_all = TRUE;
			break;
		case 'n':
			linenumbers |= 1;
			show_linenumbers = TRUE;
			break;
		case 'o':
			linenumbers |= 2;
			show_linenumbers = TRUE;
			show_offsets = TRUE;
			break;
		case 'f':
			add_vfcodes = TRUE;
			vfc_filnam = optarg;
			break;
		case 'h':
		case '?':
			print_copyright(TRUE);
			usage(argv[0]);
			return 0;
		default:
			print_copyright(TRUE);
			printf("%s: unknown option.\n", argv[0]);
			usage(argv[0]);
			return 1;
		}
	}

	if (verbose)
		print_copyright(FALSE);

	if (linenumbers > 2)
		printremark
		    ("Line numbers will be disabled in favour of offsets.\n");

	if (optind >= argc) {
		print_copyright(TRUE);
		printf("%s: filename missing.\n", argv[0]);
		usage(argv[0]);
		return 1;
	}

	init_dictionary();

	if (add_vfcodes) {
		if (add_fcodes_from_list(vfc_filnam)) {
			freeze_dictionary();
		}
	}

	while (optind < argc) {

		if (init_stream(argv[optind])) {
			printf("Could not open file \"%s\".\n", argv[optind]);
			optind++;
			continue;
		}
		detokenize();
		close_stream();

		optind++;
		reset_dictionary();
	}

	printf("\n");

	return 0;
}

