/*
 *                     OpenBIOS - free your system!
 *                        ( FCode detokenizer )
 *
 *  dictionary.c - dictionary initialization and functions.
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
#include <errno.h>

#include "detok.h"

bool check_tok_seq = TRUE;
static token_t *dictionary;	/*  Initialize dynamically to accommodate AIX  */

static char *fcerror = "ferror";

char *lookup_token(u16 number)
{
	token_t *curr;

	for (curr = dictionary; curr != NULL; curr = curr->prev)
		if (curr->fcode == number)
			break;

	if (curr)
		return curr->name;

	return fcerror;
}

/* **************************************************************************
 *
 *      Function name:  link_token
 *      Synopsis:       Simply link a ready-made token-table entry to
 *                          the dictionary, without side-effects.
 *
 *      Inputs:
 *         Parameters:
 *             curr_token                  The token-table entry to link
 *         Local Static Variables:
 *             dictionary                  Pointer to the "tail" of the
 *                                             FCode-Tokens vocabulary.
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Local Static Variables:
 *             dictionary                  Updated to point to the new entry.
 *
 **************************************************************************** */

void link_token( token_t *curr_token)
{
	curr_token->prev  = dictionary;

	dictionary = curr_token;
}

/* **************************************************************************
 *
 *      Function name:  add_token
 *      Synopsis:       Add an entry to the FCode-Tokens vocabulary.
 *
 *      Inputs:
 *         Parameters:
 *             number                      Numeric value of the FCode token
 *             name                        Name of the function to display
 *         Global/Static Variables:
 *             check_tok_seq               TRUE = "Check Token Sequence"
 *                                             A retro-fit to accommodate
 *                                             adding Vendor FCodes
 *
 *      Outputs:
 *         Returned Value:                 NONE
 *         Global/Static Variables:
 *             last_defined_token          Updated to the given FCode token
 *         Memory Allocated
 *             For the new entry.
 *         When Freed?
 *             Never.  Retained for duration of the program.
 *
 *      Error Detection:
 *          Failure to allocate memory is a fatal error.
 *          If the given FCode token is not exactly one larger than the
 *              previous  last_defined_token , then there's something
 *              odd going on; print a remark to alert the user.  The
 *              value of  last_defined_token  will be used elsewhere
 *              for additional error-checking.
 *
 *      Process Explanation:
 *          The name field pointer is presumed to already point to a stable
 *              memory-space.
 *          Memory will be allocated for the entry itself; its fields will
 *              be entered and the pointer-to-the-tail-of-the-vocabulary
 *              will be updated to point to the new entry.
 *          Error-check and update  last_defined_token  
 *
 **************************************************************************** */

void add_token(u16 number, char *name)
{
	token_t *curr;

	curr = malloc(sizeof(token_t));
	if (!curr) {
		printf("Out of memory while adding token.\n");
		exit(-ENOMEM);
	}

	curr->name = name;
	curr->fcode=number;

	link_token( curr);

	if (check_tok_seq) {
		/*  Error-check, but not for first time.  */
		if ((number == last_defined_token + 1)
		    || (last_defined_token == 0)) {
			last_defined_token = number;
		} else {
			if (number <= last_defined_token) {
				printremark("Warning:  New token # might overlap "
						"previously assigned token #(s).");
			} else {
				printremark("Warning:  New token # out of sequence with "
				     		"previously assigned token #(s).");
				/*  It's increasing; update it.  */
				last_defined_token = number;
			}
		}
	}

}

token_t detok_table[] = {

	TOKEN_ENTRY(0x000, "end0"),
	TOKEN_ENTRY(0x010, "b(lit)"),
	TOKEN_ENTRY(0x011, "b(')"),
	TOKEN_ENTRY(0x012, "b(\")"),
	TOKEN_ENTRY(0x013, "bbranch"),
	TOKEN_ENTRY(0x014, "b?branch"),
	TOKEN_ENTRY(0x015, "b(loop)"),
	TOKEN_ENTRY(0x016, "b(+loop)"),
	TOKEN_ENTRY(0x017, "b(do)"),
	TOKEN_ENTRY(0x018, "b(?do)"),
	TOKEN_ENTRY(0x019, "i"),
	TOKEN_ENTRY(0x01a, "j"),
	TOKEN_ENTRY(0x01b, "b(leave)"),
	TOKEN_ENTRY(0x01c, "b(of)"),
	TOKEN_ENTRY(0x01d, "execute"),
	TOKEN_ENTRY(0x01e, "+"),
	TOKEN_ENTRY(0x01f, "-"),
	TOKEN_ENTRY(0x020, "*"),
	TOKEN_ENTRY(0x021, "/"),
	TOKEN_ENTRY(0x022, "mod"),
	TOKEN_ENTRY(0x023, "and"),
	TOKEN_ENTRY(0x024, "or"),
	TOKEN_ENTRY(0x025, "xor"),
	TOKEN_ENTRY(0x026, "invert"),
	TOKEN_ENTRY(0x027, "lshift"),
	TOKEN_ENTRY(0x028, "rshift"),
	TOKEN_ENTRY(0x029, ">>a"),
	TOKEN_ENTRY(0x02a, "/mod"),
	TOKEN_ENTRY(0x02b, "u/mod"),
	TOKEN_ENTRY(0x02c, "negate"),
	TOKEN_ENTRY(0x02d, "abs"),
	TOKEN_ENTRY(0x02e, "min"),
	TOKEN_ENTRY(0x02f, "max"),
	TOKEN_ENTRY(0x030, ">r"),
	TOKEN_ENTRY(0x031, "r>"),
	TOKEN_ENTRY(0x032, "r@"),
	TOKEN_ENTRY(0x033, "exit"),
	TOKEN_ENTRY(0x034, "0="),
	TOKEN_ENTRY(0x035, "0<>"),
	TOKEN_ENTRY(0x036, "0<"),
	TOKEN_ENTRY(0x037, "0<="),
	TOKEN_ENTRY(0x038, "0>"),
	TOKEN_ENTRY(0x039, "0>="),
	TOKEN_ENTRY(0x03a, "<"),
	TOKEN_ENTRY(0x03b, ">"),
	TOKEN_ENTRY(0x03c, "="),
	TOKEN_ENTRY(0x03d, "<>"),
	TOKEN_ENTRY(0x03e, "u>"),
	TOKEN_ENTRY(0x03f, "u<="),
	TOKEN_ENTRY(0x040, "u<"),
	TOKEN_ENTRY(0x041, "u>="),
	TOKEN_ENTRY(0x042, ">="),
	TOKEN_ENTRY(0x043, "<="),
	TOKEN_ENTRY(0x044, "between"),
	TOKEN_ENTRY(0x045, "within"),
	TOKEN_ENTRY(0x046, "drop"),
	TOKEN_ENTRY(0x047, "dup"),
	TOKEN_ENTRY(0x048, "over"),
	TOKEN_ENTRY(0x049, "swap"),
	TOKEN_ENTRY(0x04A, "rot"),
	TOKEN_ENTRY(0x04b, "-rot"),
	TOKEN_ENTRY(0x04c, "tuck"),
	TOKEN_ENTRY(0x04d, "nip"),
	TOKEN_ENTRY(0x04e, "pick"),
	TOKEN_ENTRY(0x04f, "roll"),
	TOKEN_ENTRY(0x050, "?dup"),
	TOKEN_ENTRY(0x051, "depth"),
	TOKEN_ENTRY(0x052, "2drop"),
	TOKEN_ENTRY(0x053, "2dup"),
	TOKEN_ENTRY(0x054, "2over"),
	TOKEN_ENTRY(0x055, "2swap"),
	TOKEN_ENTRY(0x056, "2rot"),
	TOKEN_ENTRY(0x057, "2/"),
	TOKEN_ENTRY(0x058, "u2/"),
	TOKEN_ENTRY(0x059, "2*"),
	TOKEN_ENTRY(0x05a, "/c"),
	TOKEN_ENTRY(0x05b, "/w"),
	TOKEN_ENTRY(0x05c, "/l"),
	TOKEN_ENTRY(0x05d, "/n"),
	TOKEN_ENTRY(0x05e, "ca+"),
	TOKEN_ENTRY(0x05f, "wa+"),
	TOKEN_ENTRY(0x060, "la+"),
	TOKEN_ENTRY(0x061, "na+"),
	TOKEN_ENTRY(0x062, "char+"),
	TOKEN_ENTRY(0x063, "wa1+"),
	TOKEN_ENTRY(0x064, "la1+"),
	TOKEN_ENTRY(0x065, "cell+"),
	TOKEN_ENTRY(0x066, "chars"),
	TOKEN_ENTRY(0x067, "/w*"),
	TOKEN_ENTRY(0x068, "/l*"),
	TOKEN_ENTRY(0x069, "cells"),
	TOKEN_ENTRY(0x06a, "on"),
	TOKEN_ENTRY(0x06b, "off"),
	TOKEN_ENTRY(0x06c, "+!"),
	TOKEN_ENTRY(0x06d, "@"),
	TOKEN_ENTRY(0x06e, "l@"),
	TOKEN_ENTRY(0x06f, "w@"),
	TOKEN_ENTRY(0x070, "<w@"),
	TOKEN_ENTRY(0x071, "c@"),
	TOKEN_ENTRY(0x072, "!"),
	TOKEN_ENTRY(0x073, "l!"),
	TOKEN_ENTRY(0x074, "w!"),
	TOKEN_ENTRY(0x075, "c!"),
	TOKEN_ENTRY(0x076, "2@"),
	TOKEN_ENTRY(0x077, "2!"),
	TOKEN_ENTRY(0x078, "move"),
	TOKEN_ENTRY(0x079, "fill"),
	TOKEN_ENTRY(0x07a, "comp"),
	TOKEN_ENTRY(0x07b, "noop"),
	TOKEN_ENTRY(0x07c, "lwsplit"),
	TOKEN_ENTRY(0x07d, "wljoin"),
	TOKEN_ENTRY(0x07e, "lbsplit"),
	TOKEN_ENTRY(0x07f, "bljoin"),
	TOKEN_ENTRY(0x080, "wbflip"),
	TOKEN_ENTRY(0x081, "upc"),
	TOKEN_ENTRY(0x082, "lcc"),
	TOKEN_ENTRY(0x083, "pack"),
	TOKEN_ENTRY(0x084, "count"),
	TOKEN_ENTRY(0x085, "body>"),
	TOKEN_ENTRY(0x086, ">body"),
	TOKEN_ENTRY(0x087, "fcode-revision"),
	TOKEN_ENTRY(0x088, "span"),
	TOKEN_ENTRY(0x089, "unloop"),
	TOKEN_ENTRY(0x08a, "expect"),
	TOKEN_ENTRY(0x08b, "alloc-mem"),
	TOKEN_ENTRY(0x08c, "free-mem"),
	TOKEN_ENTRY(0x08d, "key?"),
	TOKEN_ENTRY(0x08e, "key"),
	TOKEN_ENTRY(0x08f, "emit"),
	TOKEN_ENTRY(0x090, "type"),
	TOKEN_ENTRY(0x091, "(cr"),
	TOKEN_ENTRY(0x092, "cr"),
	TOKEN_ENTRY(0x093, "#out"),
	TOKEN_ENTRY(0x094, "#line"),
	TOKEN_ENTRY(0x095, "hold"),
	TOKEN_ENTRY(0x096, "<#"),
	TOKEN_ENTRY(0x097, "u#>"),
	TOKEN_ENTRY(0x098, "sign"),
	TOKEN_ENTRY(0x099, "u#"),
	TOKEN_ENTRY(0x09a, "u#s"),
	TOKEN_ENTRY(0x09b, "u."),
	TOKEN_ENTRY(0x09c, "u.r"),
	TOKEN_ENTRY(0x09d, "."),
	TOKEN_ENTRY(0x09e, ".r"),
	TOKEN_ENTRY(0x09f, ".s"),
	TOKEN_ENTRY(0x0a0, "base"),
	TOKEN_ENTRY(0x0a1, "convert"),
	TOKEN_ENTRY(0x0a2, "$number"),
	TOKEN_ENTRY(0x0a3, "digit"),
	TOKEN_ENTRY(0x0a4, "-1"),
	TOKEN_ENTRY(0x0a5, "0"),
	TOKEN_ENTRY(0x0a6, "1"),
	TOKEN_ENTRY(0x0a7, "2"),
	TOKEN_ENTRY(0x0a8, "3"),
	TOKEN_ENTRY(0x0a9, "bl"),
	TOKEN_ENTRY(0x0aa, "bs"),
	TOKEN_ENTRY(0x0ab, "bell"),
	TOKEN_ENTRY(0x0ac, "bounds"),
	TOKEN_ENTRY(0x0ad, "here"),
	TOKEN_ENTRY(0x0ae, "aligned"),
	TOKEN_ENTRY(0x0af, "wbsplit"),
	TOKEN_ENTRY(0x0b0, "bwjoin"),
	TOKEN_ENTRY(0x0b1, "b(<mark)"),
	TOKEN_ENTRY(0x0b2, "b(>resolve)"),
	TOKEN_ENTRY(0x0b3, "set-token-table"),
	TOKEN_ENTRY(0x0b4, "set-table"),
	TOKEN_ENTRY(0x0b5, "new-token"),
	TOKEN_ENTRY(0x0b6, "named-token"),
	TOKEN_ENTRY(0x0b7, "b(:)"),
	TOKEN_ENTRY(0x0b8, "b(value)"),
	TOKEN_ENTRY(0x0b9, "b(variable)"),
	TOKEN_ENTRY(0x0ba, "b(constant)"),
	TOKEN_ENTRY(0x0bb, "b(create)"),
	TOKEN_ENTRY(0x0bc, "b(defer)"),
	TOKEN_ENTRY(0x0bd, "b(buffer:)"),
	TOKEN_ENTRY(0x0be, "b(field)"),
	TOKEN_ENTRY(0x0bf, "b(code)"),
	TOKEN_ENTRY(0x0c0, "instance"),
	TOKEN_ENTRY(0x0c2, "b(;)"),
	TOKEN_ENTRY(0x0c3, "b(to)"),
	TOKEN_ENTRY(0x0c4, "b(case)"),
	TOKEN_ENTRY(0x0c5, "b(endcase)"),
	TOKEN_ENTRY(0x0c6, "b(endof)"),
	TOKEN_ENTRY(0x0c7, "#"),
	TOKEN_ENTRY(0x0c8, "#s"),
	TOKEN_ENTRY(0x0c9, "#>"),
	TOKEN_ENTRY(0x0ca, "external-token"),
	TOKEN_ENTRY(0x0cb, "$find"),
	TOKEN_ENTRY(0x0cc, "offset16"),
	TOKEN_ENTRY(0x0cd, "evaluate"),
	TOKEN_ENTRY(0x0d0, "c,"),
	TOKEN_ENTRY(0x0d1, "w,"),
	TOKEN_ENTRY(0x0d2, "l,"),
	TOKEN_ENTRY(0x0d3, ","),
	TOKEN_ENTRY(0x0d4, "um*"),
	TOKEN_ENTRY(0x0d5, "um/mod"),
	TOKEN_ENTRY(0x0d8, "d+"),
	TOKEN_ENTRY(0x0d9, "d-"),
	TOKEN_ENTRY(0x0da, "get-token"),
	TOKEN_ENTRY(0x0db, "set-token"),
	TOKEN_ENTRY(0x0dc, "state"),
	TOKEN_ENTRY(0x0dd, "compile,"),
	TOKEN_ENTRY(0x0de, "behavior"),
	TOKEN_ENTRY(0x0f0, "start0"),
	TOKEN_ENTRY(0x0f1, "start1"),
	TOKEN_ENTRY(0x0f2, "start2"),
	TOKEN_ENTRY(0x0f3, "start4"),
	TOKEN_ENTRY(0x0fc, "ferror"),
	TOKEN_ENTRY(0x0fd, "version1"),
	TOKEN_ENTRY(0x0fe, "4-byte-id"),
	TOKEN_ENTRY(0x0ff, "end1"),
	TOKEN_ENTRY(0x101, "dma-alloc"),
	TOKEN_ENTRY(0x102, "my-address"),
	TOKEN_ENTRY(0x103, "my-space"),
	TOKEN_ENTRY(0x104, "memmap"),
	TOKEN_ENTRY(0x105, "free-virtual"),
	TOKEN_ENTRY(0x106, ">physical"),
	TOKEN_ENTRY(0x10f, "my-params"),
	TOKEN_ENTRY(0x110, "property"),
	TOKEN_ENTRY(0x111, "encode-int"),
	TOKEN_ENTRY(0x112, "encode+"),
	TOKEN_ENTRY(0x113, "encode-phys"),
	TOKEN_ENTRY(0x114, "encode-string"),
	TOKEN_ENTRY(0x115, "encode-bytes"),
	TOKEN_ENTRY(0x116, "reg"),
	TOKEN_ENTRY(0x117, "intr"),
	TOKEN_ENTRY(0x118, "driver"),
	TOKEN_ENTRY(0x119, "model"),
	TOKEN_ENTRY(0x11a, "device-type"),
	TOKEN_ENTRY(0x11b, "parse-2int"),
	TOKEN_ENTRY(0x11c, "is-install"),
	TOKEN_ENTRY(0x11d, "is-remove"),
	TOKEN_ENTRY(0x11e, "is-selftest"),
	TOKEN_ENTRY(0x11f, "new-device"),
	TOKEN_ENTRY(0x120, "diagnostic-mode?"),
	TOKEN_ENTRY(0x121, "display-status"),
	TOKEN_ENTRY(0x122, "memory-test-issue"),
	TOKEN_ENTRY(0x123, "group-code"),
	TOKEN_ENTRY(0x124, "mask"),
	TOKEN_ENTRY(0x125, "get-msecs"),
	TOKEN_ENTRY(0x126, "ms"),
	TOKEN_ENTRY(0x127, "finish-device"),
	TOKEN_ENTRY(0x128, "decode-phys"),
	TOKEN_ENTRY(0x12b, "interpose"),
	TOKEN_ENTRY(0x130, "map-low"),
	TOKEN_ENTRY(0x131, "sbus-intr>cpu"),
	TOKEN_ENTRY(0x150, "#lines"),
	TOKEN_ENTRY(0x151, "#columns"),
	TOKEN_ENTRY(0x152, "line#"),
	TOKEN_ENTRY(0x153, "column#"),
	TOKEN_ENTRY(0x154, "inverse?"),
	TOKEN_ENTRY(0x155, "inverse-screen?"),
	TOKEN_ENTRY(0x156, "frame-buffer-busy?"),
	TOKEN_ENTRY(0x157, "draw-character"),
	TOKEN_ENTRY(0x158, "reset-screen"),
	TOKEN_ENTRY(0x159, "toggle-cursor"),
	TOKEN_ENTRY(0x15a, "erase-screen"),
	TOKEN_ENTRY(0x15b, "blink-screen"),
	TOKEN_ENTRY(0x15c, "invert-screen"),
	TOKEN_ENTRY(0x15d, "insert-characters"),
	TOKEN_ENTRY(0x15e, "delete-characters"),
	TOKEN_ENTRY(0x15f, "insert-lines"),
	TOKEN_ENTRY(0x160, "delete-lines"),
	TOKEN_ENTRY(0x161, "draw-logo"),
	TOKEN_ENTRY(0x162, "frame-buffer-adr"),
	TOKEN_ENTRY(0x163, "screen-height"),
	TOKEN_ENTRY(0x164, "screen-width"),
	TOKEN_ENTRY(0x165, "window-top"),
	TOKEN_ENTRY(0x166, "window-left"),
	TOKEN_ENTRY(0x16a, "default-font"),
	TOKEN_ENTRY(0x16b, "set-font"),
	TOKEN_ENTRY(0x16c, "char-height"),
	TOKEN_ENTRY(0x16d, "char-width"),
	TOKEN_ENTRY(0x16e, ">font"),
	TOKEN_ENTRY(0x16f, "fontbytes"),
	TOKEN_ENTRY(0x170, "fb1-draw-character"),
	TOKEN_ENTRY(0x171, "fb1-reset-screen"),
	TOKEN_ENTRY(0x172, "fb1-toggle-cursor"),
	TOKEN_ENTRY(0x173, "fb1-erase-screen"),
	TOKEN_ENTRY(0x174, "fb1-blink-screen"),
	TOKEN_ENTRY(0x175, "fb1-invert-screen"),
	TOKEN_ENTRY(0x176, "fb1-insert-characters"),
	TOKEN_ENTRY(0x177, "fb1-delete-characters"),
	TOKEN_ENTRY(0x178, "fb1-insert-lines"),
	TOKEN_ENTRY(0x179, "fb1-delete-lines"),
	TOKEN_ENTRY(0x17a, "fb1-draw-logo"),
	TOKEN_ENTRY(0x17b, "fb1-install"),
	TOKEN_ENTRY(0x17c, "fb1-slide-up"),
	TOKEN_ENTRY(0x180, "fb8-draw-character"),
	TOKEN_ENTRY(0x181, "fb8-reset-screen"),
	TOKEN_ENTRY(0x182, "fb8-toggle-cursor"),
	TOKEN_ENTRY(0x183, "fb8-erase-screen"),
	TOKEN_ENTRY(0x184, "fb8-blink-screen"),
	TOKEN_ENTRY(0x185, "fb8-invert-screen"),
	TOKEN_ENTRY(0x186, "fb8-insert-characters"),
	TOKEN_ENTRY(0x187, "fb8-delete-characters"),
	TOKEN_ENTRY(0x188, "fb8-insert-lines"),
	TOKEN_ENTRY(0x189, "fb8-delete-lines"),
	TOKEN_ENTRY(0x18a, "fb8-draw-logo"),
	TOKEN_ENTRY(0x18b, "fb8-install"),
	TOKEN_ENTRY(0x1a0, "return-buffer"),
	TOKEN_ENTRY(0x1a1, "xmit-packet"),
	TOKEN_ENTRY(0x1a2, "poll-packet"),
	TOKEN_ENTRY(0x1a4, "mac-address"),
	TOKEN_ENTRY(0x201, "device-name"),
	TOKEN_ENTRY(0x202, "my-args"),
	TOKEN_ENTRY(0x203, "my-self"),
	TOKEN_ENTRY(0x204, "find-package"),
	TOKEN_ENTRY(0x205, "open-package"),
	TOKEN_ENTRY(0x206, "close-package"),
	TOKEN_ENTRY(0x207, "find-method"),
	TOKEN_ENTRY(0x208, "call-package"),
	TOKEN_ENTRY(0x209, "$call-parent"),
	TOKEN_ENTRY(0x20a, "my-parent"),
	TOKEN_ENTRY(0x20b, "ihandle>phandle"),
	TOKEN_ENTRY(0x20d, "my-unit"),
	TOKEN_ENTRY(0x20e, "$call-method"),
	TOKEN_ENTRY(0x20f, "$open-package"),
	TOKEN_ENTRY(0x210, "processor-type"),
	TOKEN_ENTRY(0x211, "firmware-version"),
	TOKEN_ENTRY(0x212, "fcode-version"),
	TOKEN_ENTRY(0x213, "alarm"),
	TOKEN_ENTRY(0x214, "(is-user-word)"),
	TOKEN_ENTRY(0x215, "suspend-fcode"),
	TOKEN_ENTRY(0x216, "abort"),
	TOKEN_ENTRY(0x217, "catch"),
	TOKEN_ENTRY(0x218, "throw"),
	TOKEN_ENTRY(0x219, "user-abort"),
	TOKEN_ENTRY(0x21a, "get-my-property"),
	TOKEN_ENTRY(0x21b, "decode-int"),
	TOKEN_ENTRY(0x21c, "decode-string"),
	TOKEN_ENTRY(0x21d, "get-inherited-property"),
	TOKEN_ENTRY(0x21e, "delete-property"),
	TOKEN_ENTRY(0x21f, "get-package-property"),
	TOKEN_ENTRY(0x220, "cpeek"),
	TOKEN_ENTRY(0x221, "wpeek"),
	TOKEN_ENTRY(0x222, "lpeek"),
	TOKEN_ENTRY(0x223, "cpoke"),
	TOKEN_ENTRY(0x224, "wpoke"),
	TOKEN_ENTRY(0x225, "lpoke"),
	TOKEN_ENTRY(0x226, "lwflip"),
	TOKEN_ENTRY(0x227, "lbflip"),
	TOKEN_ENTRY(0x228, "lbflips"),
	TOKEN_ENTRY(0x229, "adr-mask"),
	TOKEN_ENTRY(0x230, "rb@"),
	TOKEN_ENTRY(0x231, "rb!"),
	TOKEN_ENTRY(0x232, "rw@"),
	TOKEN_ENTRY(0x233, "rw!"),
	TOKEN_ENTRY(0x234, "rl@"),
	TOKEN_ENTRY(0x235, "rl!"),
	TOKEN_ENTRY(0x236, "wbflips"),
	TOKEN_ENTRY(0x237, "lwflips"),
	TOKEN_ENTRY(0x238, "probe"),
	TOKEN_ENTRY(0x239, "probe-virtual"),
	TOKEN_ENTRY(0x23b, "child"),
	TOKEN_ENTRY(0x23c, "peer"),
	TOKEN_ENTRY(0x23d, "next-property"),
	TOKEN_ENTRY(0x23e, "byte-load"),
	TOKEN_ENTRY(0x23f, "set-args"),
	TOKEN_ENTRY(0x240, "left-parse-string"),

	/* FCodes from 64bit extension addendum */
	TOKEN_ENTRY(0x22e, "rx@"),
	TOKEN_ENTRY(0x22f, "rx!"),
	TOKEN_ENTRY(0x241, "bxjoin"),
	TOKEN_ENTRY(0x242, "<l@"),
	TOKEN_ENTRY(0x243, "lxjoin"),
	TOKEN_ENTRY(0x244, "wxjoin"),
	TOKEN_ENTRY(0x245, "x,"),
	TOKEN_ENTRY(0x246, "x@"),
	TOKEN_ENTRY(0x247, "x!"),
	TOKEN_ENTRY(0x248, "/x"),
	TOKEN_ENTRY(0x249, "/x*"),
	TOKEN_ENTRY(0x24a, "xa+"),
	TOKEN_ENTRY(0x24b, "xa1+"),
	TOKEN_ENTRY(0x24c, "xbflip"),
	TOKEN_ENTRY(0x24d, "xbflips"),
	TOKEN_ENTRY(0x24e, "xbsplit"),
	TOKEN_ENTRY(0x24f, "xlflip"),
	TOKEN_ENTRY(0x250, "xlflips"),
	TOKEN_ENTRY(0x251, "xlsplit"),
	TOKEN_ENTRY(0x252, "xwflip"),
	TOKEN_ENTRY(0x253, "xwflips"),
	TOKEN_ENTRY(0x254, "xwsplit"),
};

static const int dictionary_indx_max =
    (sizeof(detok_table) / sizeof(token_t));

static token_t *dictionary_reset_position;

void init_dictionary(void)
{
	int indx;

	dictionary = &detok_table[dictionary_indx_max - 1];
	dictionary_reset_position = dictionary;

	for (indx = 1; indx < dictionary_indx_max; indx++) {
		detok_table[indx].prev = &detok_table[indx - 1];
	}
}

void reset_dictionary(void)
{
	token_t *next_t;

	next_t = dictionary;
	while (next_t != dictionary_reset_position) {
		next_t = dictionary->prev;
		free(dictionary->name);
		free(dictionary);
		dictionary = next_t;
	}
}

/*  If FCodes have been added by User, we need to update reset-position  */
void freeze_dictionary(void)
{
	dictionary_reset_position = dictionary;
}
