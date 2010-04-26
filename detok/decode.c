/*
 *                     OpenBIOS - free your system! 
 *                        ( FCode detokenizer )
 *                          
 *  decode.c - contains output wrappers for fcode words.  
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
 *         Modifications made in 2005 by IBM Corporation
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Modifications Author:  David L. Paktor    dlpaktor@us.ibm.com
 **************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>

#include "detok.h"
#include "stream.h"
#include "addfcodes.h"

static int indent;		/*  Current level of indentation   */

/* **************************************************************************
 *
 *      Still to be done:
 *          Handling of indent-level is not correct.  Branches should
 *              balance with their resolvers; constructs like do..loop
 *              case/of/endof/endcase are a few major examples.
 *          This will be tricky; the rules need to be carefully thought
 *              out, and the implementation might be more complex than
 *              at first meets the eye...
 *
 **************************************************************************** */

static bool ended_okay = TRUE;	/*  FALSE if finished prematurely  */

bool offs16 = TRUE;
unsigned int linenum;
bool end_found = FALSE;
unsigned int token_streampos;	/*  Streampos() of currently-gotten token  */
u16 last_defined_token = 0;

jmp_buf eof_exception;

static int fclen;
static const char *unnamed = "(unnamed-fcode)";

static void decode_indent(void)
{
	int i;
	if (indent < 0) {
#ifdef DEBUG_INDENT
		printf("detok: error in indentation code.\n");
#endif
		indent = 0;
	}
	for (i = 0; i < indent; i++)
		printf("    ");
}

/*  Print forth string ( [len] char[0] ... char[len] ) */
static void pretty_print_string(void)
{
	u8 len;
	u8 *strptr;
	int indx;
	bool in_parens = FALSE;	/*  Are we already inside parentheses?  */

	strptr = get_string(&len);

	printf("( len=%s%x", len >= 10 ? "0x" : "", len);
	if (len >= 10)
		printf(" [%d bytes]", len);
	printf(" )\n");
	if (show_linenumbers)
		printf("        ");
	decode_indent();
	printf("\" ");

	for (indx = 0; indx < len; indx++) {
		u8 c = *strptr++;
		if (isprint(c)) {
			if (in_parens) {
				printf(" )");
				in_parens = FALSE;
			}
			printf("%c", c);
			/*  Quote-mark must escape itself  */
			if (c == '"')
				printf("%c", c);
		} else {
			if (!in_parens) {
				printf("\"(");
				in_parens = TRUE;
			}
			printf(" %02x", c);
		}
	}
	if (in_parens)
		printf(" )");
	printf("\"");
}

static void decode_lines(void)
{
	if (show_linenumbers) {
		printf("%6d: ",show_offsets ? token_streampos : linenum++);
	}
}

/* **************************************************************************
 *
 *      Function name:  output_token_name
 *      Synopsis:       Print the name of the token just retrieved
 *                          along with any interesting related information...
 *
 *      Inputs:
 *         Global/Static Variables:
 *             fcode                  The token # just retrieved
 *             last_defined_token     Used to screen invalid tokens.
 *             token_streampos        Location of token just retrieved
 *         Global/Static Constants:
 *             unnamed                Namefield of headerless token 
 *
 *      Outputs:
 *         Printout:
 *             Print the function name (if known) and the FCode number,
 *                  if interesting.
 *             The fcode number is interesting if either
 *                 a) the token has no name
 *                           or
 *                 b) verbose mode is in effect.
 *             If the token is named, show its FCode number in
 *                 the syntax of a FORTH Comment, otherwise, its
 *                 FCode number -- in [brackets] -- acts as its name.
 *
 *      Error Detection:
 *          If the token # is larger than the last defined token, it is
 *              probably an artifact of an error that was allowed in the
 *              tokenization; if it were treated normally, it would lead
 *              to a cascade of failures.  Print a message, skip the first
 *              byte, and return.
 *
 *      Revision History:
 *          Refactored.  The line number or offset (if selected) and the
 *              indent are printed in the  output_token()  routine, which
 *              calls this one.
 *          Add Error Detection
 *          
 *
 **************************************************************************** */

static void output_token_name(void)
{
	char *tname;

	/*  Run error detection only if last_defined_token was assigned  */
	if ((fcode > last_defined_token) && (last_defined_token > 0)) {
		char temp_buf[80];
		int buf_pos;
		u8 top_byte = fcode >> 8;
		printf("Invalid token:  [0x%03x]\n", fcode);
		sprintf(temp_buf, "Backing up over first byte, which is ");
		buf_pos = strlen(temp_buf);
		if (top_byte < 10) {
			sprintf(&temp_buf[buf_pos], " %02x", top_byte);
		} else {
			sprintf(&temp_buf[buf_pos], "0x%02x ( =dec %d)",
				top_byte, top_byte);
		}
		printremark(temp_buf);
		set_streampos(token_streampos + 1);
		return;
	}


	tname = lookup_token(fcode);
	printf("%s ", tname);

	/* The fcode number is interesting
	 *  if either
	 *  a) the token has no name
	 *            or
	 *  b) detok is in verbose mode.
	 */
	if (strcmp(tname, unnamed) == 0) {
		printf("[0x%03x] ", fcode);
	} else {
		if (verbose) {
			/*  If the token is named,
			 *  show its fcode number in
			 *  the syntax of a FORTH Comment
			 */
			printf("( 0x%03x ) ", fcode);
		}
	}
}

/* **************************************************************************
 *
 *      Function name:  output_token
 *      Synopsis:       Print the full line for the token just retrieved:
 *                          line number, indentation and name.
 *
 *      Revision History:
 *          Updated Mon, 11 Jul 2005 by David L. Paktor
 *          Separate out  output_token_name()  for specific uses.
 *          Incorporate calls to  decode_lines()  and   decode_indent()
 *              to allow better control of indentation.
 *
 **************************************************************************** */

static void output_token(void)
{
	decode_lines();
	decode_indent();
	output_token_name();
}


/* **************************************************************************
 *
 *      Function name:  decode_offset
 *      Synopsis:       Gather and display an FCode-offset associated with
 *                          a branch or suchlike function.
 *
 *      Inputs:
 *         Parameters:                 NONE
 *         Global/Static Variables:
 *             show_offsets            Whether to show offsets in full detail
 *             offs16                  Whether 16- (or 8-) -bit offsets
 *             stream_max              Maximum valid destimation      
 *         Local Static Variables:
 *
 *      Outputs:
 *         Returned Value:             The offset, converted to signed 16-bit #
 *         Global/Static Variables:
 *             stream-position         Reset if invalid destination; otherwise,
 *                                         advanced in the normal manner.
 *         Printout:
 *             The offset and destination, if display of offsets was specified
 *                 or if the destination is invalid
 *
 *      Error Detection:
 *          Crude and rudimentary:
 *          If the target-destination is outside the theoretical limits,
 *              it's obviously wrong.
 *          Notification remark is printed and the stream-position reset
 *              to the location of the offset, to allow it to be processed
 *              in the manner of normal tokens.
 *          If the offset is zero, that's obviously wrong, but don't reset
 *              the stream-position:  zero gets processed as  end0  and that
 *              is also wrong...
 *
 *      Still to be done:
 *          More sophisticated error-checking for invalid offsets.  Look
 *              at the type of branch, and what should be expected in the
 *              vicinity of the destination.  (This might be best served
 *              by a separate routine).
 *         This might also help with handling the indent-level correctly...
 *              Also, if indentation were to be handled in this routine,
 *              there would be no need to return the value of the offset.
 *
 **************************************************************************** */

static s16 decode_offset(void)
{
	s16 offs;
	int dest;
	bool invalid_dest;
	int streampos = get_streampos();

	output_token();
	offs = get_offset();

	/*  The target-destination is the source-byte offset
	 *      at which the FCode-offset is found, plus
	 *      the FCode-offset.
	 */
	dest = streampos + offs;

	/*  A destination of zero is invalid because there must be a
	 *      token -- such as  b(<mark)  or   b(do)  -- preceding
	 *      the target of a backward branch.
	 *  A destination at the end of the stream is unlikely but
	 *      theoretically possible, so we'll treat it as valid.
	 *  An offset of zero is also, of course, invalid.
	 */
	invalid_dest = BOOLVAL((dest <= 0)
			       || (dest > stream_max)
			       || (offs == 0));

	/*  Show the offset in hex and again as a signed decimal number.   */
	if (offs16) {
		printf("0x%04x (", (u16) (offs & 0xffff));
	} else {
		printf("0x%02x (", (u8) (offs & 0x00ff));
	}
	if ((offs < 0) || (offs > 9))
		printf(" =dec %d", offs);
	/*  If we're showing source-byte offsets, show targets of offsets  */
	if (show_offsets || invalid_dest) {
		printf("  dest = %d ", dest);
	}
	printf(")\n");

	if (invalid_dest) {
		if (offs == 0) {
			printremark("Error:  Unresolved offset.");
		} else {
			printremark
			    ("Error:  Invalid offset.  Ignoring...");
			set_streampos(streampos);
		}
	}
	return offs;
}

static void decode_default(void)
{
	output_token();
	printf("\n");
}

static void new_token(void)
{
	u16 token;
	output_token();
	token = next_token();
	printf("0x%03x\n", token);
	add_token(token, strdup(unnamed));
}

static void named_token(void)
{
	u16 token;
	u8 len;
	char *string;

	output_token();
	/* get forth string ( [len] [char0] ... [charn] ) */
	string = get_name(&len);
	token = next_token();
	printf("%s 0x%03x\n", string, token);
	add_token(token, string);
}

static void bquote(void)
{
	output_token();
	/* get forth string ( [len] [char0] ... [charn] ) */
	pretty_print_string();
	printf("\n");
}

static void blit(void)
{
	u32 lit;

	output_token();
	lit = get_num32();
	printf("0x%x\n", lit);
}

static void double_length_literal(void)
{
	u16 quadhh, quadhl, quadlh, quadll;

	output_token();
	quadhh = get_num16();
	quadhl = get_num16();
	quadlh = get_num16();
	quadll = get_num16();
	printf("0x%04x.%04x.%04x.%04x\n", quadhh, quadhl, quadlh, quadll);
}

static void offset16(void)
{
	decode_default();
	offs16 = TRUE;
}

static void decode_branch(void)
{
	s16 offs;
	offs = decode_offset();
	if (offs >= 0)
		indent++;
	else
		indent--;
}

static void decode_two(void)
{
	u16 token;

	output_token();
	token = next_token();
	output_token_name();
	printf("\n");
}

/* **************************************************************************
 *
 *      Function name:  decode_start
 *      Synopsis:       Display the (known valid) FCode block Header
 *
 *      Outputs:
 *         Global/Static Variables:    
 *             fclen         Length of the FCode block as shown in its Header
 *
 **************************************************************************** */

static void decode_start(void)
{
	u8 fcformat;
	u16 fcchecksum, checksum = 0;

	output_token();
	printf("  ( %d-bit offsets)\n", offs16 ? 16 : 8);

	token_streampos = get_streampos();
	decode_lines();
	fcformat = get_num8();
	printf("  format:    0x%02x\n", fcformat);


	/* Check for checksum correctness. */

	token_streampos = get_streampos();
	decode_lines();
	fcchecksum = get_num16();	/*  Read the stored checksum  */
	checksum = calc_checksum();	/*  Calculate the actual checksum  */

	if (fcchecksum == checksum) {
		printf("  checksum:  0x%04x (Ok)\n", fcchecksum);
	} else {
		printf("  checksum should be:  0x%04x, but is 0x%04x\n",
		       checksum, fcchecksum);
	}

	token_streampos = get_streampos();
	decode_lines();
	fclen = get_num32();
	printf("  len:       0x%04x ( %d bytes)\n", fclen, fclen);
}


/* **************************************************************************
 *
 *      Function name: decode_token
 *      Synopsis:      Display detokenization for one token.
 *                     Handle complicated cases and dispatch simple ones.
 *
 *      Revision History:
 *          Detect FCode-Starters in the middle of an FCode block.
 *          Some tuning of adjustment of indent, particularly wrt branches...
 *
 **************************************************************************** */

static void decode_token(u16 token)
{
	bool handy_flag = TRUE;
	switch (token) {
	case 0x0b5:
		new_token();
		break;
	case 0x0b6:		/*   Named Token  */
	case 0x0ca:		/*   External Token  */
		named_token();
		break;
	case 0x012:
		bquote();
		break;
	case 0x010:
		blit();
		break;
	case 0x0cc:
		offset16();
		break;
	case 0x013:		/* bbranch */
	case 0x014:		/* b?branch */
		decode_branch();
		break;
	case 0x0b7:		/* b(:) */
	case 0x0b1:		/* b(<mark) */
	case 0x0c4:		/* b(case) */
		decode_default();
		indent++;
		break;
	case 0x0c2:		/* b(;) */
	case 0x0b2:		/* b(>resolve) */
	case 0x0c5:		/* b(endcase) */
		indent--;
		decode_default();
		break;
	case 0x015:		/* b(loop) */
	case 0x016:		/* b(+loop) */
	case 0x0c6:		/* b(endof) */
		indent--;
		decode_offset();
		break;
	case 0x017:		/* b(do) */
	case 0x018:		/* b/?do) */
	case 0x01c:		/* b(of) */
		decode_offset();
		indent++;
		break;
	case 0x011:		/* b(') */
	case 0x0c3:		/* b(to) */
		decode_two();
		break;
	case 0x0fd:		/* version1 */
		handy_flag = FALSE;
	case 0x0f0:		/* start0 */
	case 0x0f1:		/* start1 */
	case 0x0f2:		/* start2 */
	case 0x0f3:		/* start4 */
		offs16 = handy_flag;
		printremark("Unexpected FCode-Block Starter.");
		decode_start();
		printremark("  Ignoring length field.");
		break;
	case 0:		/* end0  */
	case 0xff:		/* end1  */
		end_found = TRUE;
		decode_default();
		break;

#if  0  /*  Fooey on C's petty limitations!  */
        /*  I'd like to be able to do this:  */
	/*  Special Functions  */
	case *double_lit_code:
		double_length_literal();
		break;
#endif  /*  Fooey on C's petty limitations!  */

	default:
	{
	    /*  Have to do this clumsy thing instead  */
	    if ( token == *double_lit_code )
	    {
		double_length_literal();
		break;
	    }

		decode_default();
	}
}
}


/* **************************************************************************
 *
 *      Function name:  decode_fcode_header
 *      Synopsis:       Detokenize the FCode Header.
 *                          Check for file corruption
 *
 *      Inputs:
 *         Parameters:          NONE
 *         Global/Static Variables:        
 *             stream_max       Length of input stream
 *
 *      Outputs:
 *         Returned Value:      NONE
 *         Global/Static Variables:
 *             offs16           FALSE if Starter was version1, TRUE for all
 *                                  other valid Starters, otherwise unchanged.
 *             fclen            On error, gets set to reach end of input stream
 *                                  Otherwise, gets set by  decode_start() 
 *         Printout:
 *             On error, print a new-line to finish previous token's line.
 *
 *      Error Detection:
 *          First byte not a valid FCode Start:  Print message, restore
 *              input pointer to initial value, set fclen to [(end of
 *              input stream) - (input pointer)], leave offs16 unchanged.
 *              Return FALSE.
 *
 *      Process Explanation:
 *          This routine error-checks and dispatches to the routine that
 *              does the actual printing.
 *          Refrain from showing offset until correctness of starter-byte
 *              has been confirmed.
 *
 **************************************************************************** */

static void decode_fcode_header(void)
{
	long err_pos;
	u16 token;
	bool new_offs16 = TRUE;

	err_pos = get_streampos();
	indent = 0;
	token = next_token();

	switch (token) {
	case 0x0fd:		/* version1 */
		new_offs16 = FALSE;
	case 0x0f0:		/* start0 */
	case 0x0f1:		/* start1 */
	case 0x0f2:		/* start2 */
	case 0x0f3:		/* start4 */
		offs16 = new_offs16;
		decode_start();
		break;
	default:
		{
			char temp_bufr[128] =
			    "Invalid FCode Start Byte.  Ignoring FCode header.";
			set_streampos(err_pos);
			fclen = max - pc;
			printf("\n");
			if (show_linenumbers) {
				sprintf(&(temp_bufr[strlen(temp_bufr)]),
					"  Remaining len = 0x%04x ( %d bytes)",
					fclen, fclen);
			}
			printremark(temp_bufr);
		}
	}
}

/* **************************************************************************
 *
 *      Function name:  decode_fcode_block
 *      Synopsis:       Detokenize one FCode block.
 *
 *      Inputs:
 *         Parameters:                     NONE
 *         Global/Static Variables:
 *             fclen         Length of the FCode block as shown in its Header
 *             end_found     Whether the END0 code was seen
 *             decode_all    TRUE = continue even after END0
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Global/Static Variables:
 *             end_found     Whether the END0 code was seen
 *         Printout:
 *             A summary message at the end of FCode detokenization
 *
 *      Error Detection:
 *          If the end of the FCode block, as calculated by its FCode length,
 *              was reached without encountering END0, print a message.
 *          Detect END0 that occurs before end of FCode block,
 *              even if  decode_all  is in effect.
 *
 *      Process Explanation:
 *          This routine dispatches to the routines that do the actual
 *              printing of the detokenization.
 *          The  end_found  flag is not a direct input, but more of an
 *              intermediate input, so to speak...  Clear it at the start.
 *          Detection of FCode-Starters in the middle of an FCode block
 *              is handled by  decode_token()  routine.
 *
 **************************************************************************** */

static void decode_fcode_block(void)
{
	u16 token;
	unsigned int fc_block_start;
	unsigned int fc_block_end;

	end_found = FALSE;
	fc_block_start = get_streampos();

	decode_fcode_header();

	fc_block_end = fc_block_start + fclen;

	while ((!end_found || decode_all)
	       && (get_streampos() < fc_block_end)) {
		token = next_token();
		decode_token(token);
	}
	if (!end_found) {
		printremark("FCode-ender not found");
	}
	{
		char temp_bufr[80];
		/*  Don't use  fclen  here, in case it got corrupted
		 *      by an "Unexpected FCode-Block Starter"
		 */
		if (get_streampos() == fc_block_end) {
			sprintf(temp_bufr,
				"Detokenization finished normally after %d bytes.",
				fc_block_end - fc_block_start);
		} else {
			sprintf(temp_bufr,
				"Detokenization finished prematurely after %d of %d bytes.",
				get_streampos() - fc_block_start,
				fc_block_end - fc_block_start);
			ended_okay = FALSE;
		}
		printremark(temp_bufr);
	}
}

/* **************************************************************************
 *
 *      Function name:  another_fcode_block
 *      Synopsis:       Indicate whether there is a follow-on FCode block
 *                      within the current PCI image.
 *
 *      Inputs:
 *         Parameters:                     NONE
 *         Global/Static Variables:
 *             token_streampos             Streampos() of token just gotten
 *             Next token in Input Stream
 *
 *      Outputs:
 *         Returned Value:                  TRUE if next token shows the start
 *                                              of a valid FCode-block
 *         Printout:
 *             Message if there is a follow-on FCode block
 *
 *      Error Detection:
 *          If next token is neither a valid FCode-block starter nor the
 *              start of a zero-fill field, print a message.
 *
 *      Process Explanation:
 *          Extract the next token from the Input Stream but do not
 *              consume it.  Then examine the token.
 *
 **************************************************************************** */

static bool another_fcode_block(void)
{
	bool retval = FALSE;
	u16 token;

	token = next_token();
	set_streampos(token_streampos);

	switch (token) {
	case 0x0fd:		/* version1 */
	case 0x0f0:		/* start0 */
	case 0x0f1:		/* start1 */
	case 0x0f2:		/* start2 */
	case 0x0f3:		/* start4 */
		retval = TRUE;
		printremark
		    ("Subsequent FCode Block detected.  Detokenizing.");
		break;
	case 0:		/* Start of a zero-fill field  */
		/* retval already = FALSE .  Nothing else to be done.  */
		break;
	default:
		{
			char temp_bufr[80];
			sprintf(temp_bufr,
				"Unexpected token, 0x%02x, after end of FCode block.",
				token);
			printremark(temp_bufr);
		}
	}
	return (retval);
}

/* **************************************************************************
 *
 *      Function name:  detokenize
 *      Synopsis:       Detokenize one input file 
 *                      
 *
 *
 **************************************************************************** */

void detokenize(void)
{
	fclen = stream_max;

	if (setjmp(eof_exception) == 0) {
		while (more_to_go()) {
			if (ended_okay) {
				init_fcode_block();
			}
			ended_okay = TRUE;

			adjust_for_pci_header();

			/*   Allow for multiple FCode Blocks within the PCI image.
			 *   The first one had better be a valid block, but the
			 *   next may or may not be...
			 */
			do {
				decode_fcode_block();
			} while (another_fcode_block());

			adjust_for_pci_filler();

		}
	}


}
