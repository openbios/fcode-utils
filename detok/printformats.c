/*
 *                     OpenBIOS - free your system!
 *                        ( FCode detokenizer )
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
 *      Print, in various controlled formats, for the detokenizer.
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Ultimately, our goal is to produce output that can be run back
 *      through the tokenizer and produce the same binary.  So, any
 *      extra text will have to be in a form that the tokenizer will
 *      treat as comments.
 *
 **************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "detok.h"


/* **************************************************************************
 *
 *      Function name:  printremark ( string )
 *      Synopsis:       Print the given string as a series of "Remark" lines,
 *                      (i.e., preceded by backslash-space)
 *      
 *      Inputs:
 *          Parameters:     
 *              str     Pointer to the start of the string,
 *                      however long it may be.  The string
 *                      may have any number of embedded new-lines.
 *
 *      Outputs:
 *          Returned Value: *** NONE***
 *          Printout:    Each line of the string will be preceded by
 *                       a backslash and two spaces.  Backslash-space
 *                       is the standard delimiter for a "remark", i.e.
 *                       the entire line is ignored as a comment.  The
 *                       second space is "just" for aesthetics.
 *      
 *      Process Explanation:
 *          Parse the input string for new-lines.  Print each separately.
 *          Do not alter the input string.
 *      
 *
 *      Still to be done:
 *          Define a routine, call it  PrintComment , to print the given
 *              string surrounded by open-paren-space ... space-close-paren
 *          Define a single central routine, call it  safe_malloc ,
 *              to do the test for null and print "No Memory" and exit.
 *          Define a single central routine, call it  PrintError , to:
 *              Print the given error message
 *              Show the input file and line number
 *              Collect error-flags for failure-exit at end of operation.
 *
 **************************************************************************** */

void printremark(char *str)
{
	char *strtmp;		/*  Temporary pointer to current substring    */
	int substrlen;		/*  Length of current substring               */
	char *substrend;	/*  Pointer to end of current substring       */
	char *strend;		/*  Pointer to end of given string            */

	char *strbfr;		/*  Temporary substring buffer                */

	/*  Guarantee that the malloc will be big enough.  */
	strbfr = (char *) malloc(strlen((char *) str) + 1);
	if (!strbfr) {
		printf("No memory.\n");
		exit(-1);
	}


	strtmp = str;
	strend = &str[strlen(str)];

	/* ******************************************************************
	 *
	 *      Isolate the current substring; allow that the given
	 *      string might not be terminated with a new-line. 
	 *
	 *      The  strend  pointer provides a convenient means to
	 *      test for when we've reached the end.
	 *
	 ******************************************************************** */

	while (strtmp < strend) {
		substrend = strchr(strtmp, '\n');
		substrlen = (substrend ? (substrend - strtmp) : strlen(strtmp));

		strncpy(strbfr, strtmp, substrlen);
		/* **********************************************************
		 * 
		 *  strncpy() does not append a terminating null character,
		 *  so we have to.
		 *
		 ************************************************************ */
		strbfr[substrlen] = (char) 0;

		printf("\\  %s\n", strbfr);

		strtmp = &strtmp[substrlen + (substrend ? 1 : 0)];

	}

	free(strbfr);
}
