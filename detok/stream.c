/*
 *                     OpenBIOS - free your system! 
 *                        ( FCode detokenizer )
 *                          
 *  stream.c - FCode program bytecode streaming from file.
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
#include <sys/stat.h>
#include <setjmp.h>

#include "stream.h"
#include "detok.h"
#include "pcihdr.h"

extern jmp_buf eof_exception;

/* **************************************************************************
 *
 *      Global Variables Exported:
 *        Name            Value
 *      fcode          The FCode-token last read.  Not necessarily the byte
 *                          last read, if its function has in-line parameters
 *      stream_max     The maximum position -- length -- of the input stream
 *      pc             Pointer to "Current byte" in the input-data image.
 *      max            Just after the end of the input-data image.
 *                         This is *NOT* a byte-count.
 *
 **************************************************************************** */

u16 fcode;
unsigned int stream_max;
u8 *pc;
u8 *max;

/* **************************************************************************
 *
 *      Local/Static Variables:
 *        Name            Pointer to:
 *      indata          Start of input-data image taken from input file.
 *                          This memory was "malloc"ed; keep it around
 *                          for when we "free" that memory.
 *      fc_start        Start of the FCode.  This might not be the same
 *                          as the start of the input file data, especially
 *                          if the input file data starts with a PCI header.
 *      pci_image_found     TRUE iff a valid PCI header was found
 *
 **************************************************************************** */
u8 *indata;
static u8 *fc_start;
static bool pci_image_found = FALSE;


int init_stream(char *name)
{
	FILE *infile;
	struct stat finfo;

	if (stat(name, &finfo))
		return -1;

	indata = malloc(finfo.st_size);
	if (!indata)
		return -1;

	infile = fopen(name, "r");
	if (!infile)
		return -1;

	if (fread(indata, finfo.st_size, 1, infile) != 1) {
		free(indata);
		return -1;
	}

	fclose(infile);

	pc = indata;
	fc_start = indata;
	max = pc + finfo.st_size;

	stream_max = finfo.st_size;

	return 0;
}

/* **************************************************************************
 *
 *      Function name:  init_fcode_block
 *      Synopsis:       Initialize all pointers and variables, etcetera,
 *                          for an FCode-block.
 *
 **************************************************************************** */

void init_fcode_block(void)
{
	fc_start = pc;
	linenum = 1;
}


void close_stream(void)
{
	free(indata);
	stream_max = 0;
}

int get_streampos(void)
{
	return (int) (pc - fc_start);
}

void set_streampos(int pos)
{
	pc = fc_start + pos;
}


/* **************************************************************************
 *
 *      Function name:  throw_eof
 *      Synopsis:       Analyze and print the cause of the end-of-file
 *                      and throw an exception.
 *
 *      Inputs:
 *         Parameters:
 *             premature              TRUE iff end-of-file was out-of-sync 
 *         Global/Static Variables:        
 *                  end_found         Indicates if normal end of fcode was read.
 *                  eof_exception     Long-Jump environment to which to jump.
 *
 *      Outputs:                      Does a Long-Jump
 *         Returned Value:            NONE
 *         Printout:
 *             "End-of-file" message, along with a descriptor, if applicable:
 *                 Premature, Unexpected.
 *          The calling routine notifies us if the number of bytes requested
 *              overflowed the input buffer, by passing us a TRUE for the
 *              input parameter.  That is a "Premature" end-of-file.
 *          If  end_found  is FALSE, it means the normal end of FCode
 *               wasn't seen.  That is an "Unexpected" end-of-file.
 *
 **************************************************************************** */

static void throw_eof(bool premature)
{
	char yoo = 'U';
	char eee = 'E';
	if (premature) {
		printf("Premature ");
		yoo = 'u';
		eee = 'e';
	}
	if (!end_found) {
		printf("%cnexpected ", yoo);
		eee = 'e';
	}
	printf("%cnd of file.\n", eee);
	longjmp(eof_exception, -1);
}

/* **************************************************************************
 *
 *      Function name:  get_bytes
 *      Synopsis:       Return the next string of bytes, as requested, from
 *                      the FCode input-stream.  Detect end-of-file
 *
 *      Inputs:
 *         Parameters:
 *             nbytes            The number of bytes requested
 *         Global/Static Variables:
 *             pc                Pointer to "where we are" in the FCode
 *             max               Pointer to just after end of input file data.
 *
 *      Outputs:
 *         Returned Value:
 *             Pointer to the requested bytes in the FCode input-stream.
 *         Global/Static Variables:
 *             pc                Incremented by the number of bytes requested
 *
 *      Exception:
 *          When end-of-file is reached, or is about to be exceeded,    
 *              throw an end-of-file exception.
 *
 *      Process Explanation:
 *          End-of-file is not exactly an error, so its detection is more
 *              a part of normal processing.
 *          If we entered this routine with PC pointing exactly at MAX,
 *              we are probably at the end the way we expect to be, so we
 *              call our EOF-handling routine with a "non-premature" flag.
 *          If the requested number of bytes puts us just even with MAX,
 *              we have neither reached nor over-run the input stream, so
 *              there's no call to our EOF-handling routine needed at all.
 *          Only if the requested number of bytes puts us past MAX have we 
 *              over-run our input stream with a "premature" condition.
 *
 *      Extraneous Remarks:
 *          This is another one where it was easier to write the code
 *              than the explanation ...  ;-}
 *
 **************************************************************************** */

static u8 *get_bytes(int nbytes)
{
	u8 *retval = pc;
	if (pc == max) {
		throw_eof(FALSE);
	}
	if (pc + nbytes > max) {
		throw_eof(TRUE);
	}
	pc += nbytes;
	return retval;
}


/* **************************************************************************
 *
 *       Function name:  more_to_go
 *       Synopsis:       Return FALSE when the last byte has been
 *                           read from the input-stream.
 *
 **************************************************************************** */

bool more_to_go(void)
{
	bool retval;
	retval = INVERSE(pc == max);
	return retval;
}


/* **************************************************************************
 *
 *      Function name:  next_token
 *      Synopsis:       Retrieve the next FCode-token from the input-stream.
 *
 *      Inputs:
 *         Parameters:                     NONE
 *
 *      Outputs:
 *         Returned Value:                 The next FCode-token
 *         Global/Static Variables:
 *             fcode                       The FCode-token last read.
 *             token_streampos             Streampos() of token just gotten
 *
 **************************************************************************** */

u16 next_token(void)
{
	u16 tok;
	token_streampos = get_streampos();
	tok = *(get_bytes(1));
	if (tok != 0x00 && tok < 0x10) {
		tok <<= 8;
		tok |= *(get_bytes(1));
	}
	fcode = tok;
	return tok;
}

u32 get_num32(void)
{
	u32 retval;
	u8 *num_str;

	num_str = get_bytes(4);
	retval = BIG_ENDIAN_LONG_FETCH(num_str);

	return retval;
}

u16 get_num16(void)
{
	u16 retval;
	u8 *num_str;

	num_str = get_bytes(2);
	retval = BIG_ENDIAN_WORD_FETCH(num_str);

	return retval;
}

u8 get_num8(void)
{
	u8 inbyte;

	inbyte = *(get_bytes(1));
	return (inbyte);
}

s16 get_offset(void)
{
	s16 retval;
	if (offs16) {
		retval = (s16) get_num16();
	} else {
		retval = (s16) get_num8();
		/*  Make sure it's sign-extended  */
		retval |= (retval & 0x80) ? 0xff00 : 0;
	}

	return retval;
}

/* **************************************************************************
 *
 *      Function name:  get_string
 *      Synopsis:       Return a pointer to a Forth-Style string within the
 *                          input stream.  Note: this cannot be used to create
 *                          a new token-name; use  get_name()  for that.
 *
 *      Inputs:
 *         Parameters:
 *             *len                       Pointer to where the length will go
 *
 *      Outputs:
 *         Returned Value:
 *             Pointer to the string within the input stream.
 *         Supplied Pointers:
 *             *len                      Length of the string
 *
 *      Process Explanation:
 *          Get one byte representing the length of the FORTH-style string.
 *          Get as many bytes as the length indicates.
 *          That's the string.  The pointer returned by  get_bytes()  is
 *              our return value.
 *
 **************************************************************************** */

u8 *get_string(u8 * len)
{
	u8 *retval;

	*len = get_num8();
	retval = get_bytes((int) *len);

	return retval;
}


/* **************************************************************************
 *
 *      Function name:  get_name
 *      Synopsis:       Retrieve a copy of the next string in the input-stream
 *                      when it is expected to be a new function-name.
 *
 *      Inputs:
 *         Parameters:
 *             *len                       Pointer to where the length will go
 *         Global/Static Variables:
 *             pc                        "Where we are" in the file-stream
 *
 *      Outputs:
 *         Returned Value: 
 *             Pointer to allocated memory containing a copy of the string
 *         Supplied Pointers:
 *             *len                      Length of the name
 *         Memory Allocated
 *             Memory for the copy of the string is allocated by  strdup()
 *         When Freed?
 *             Never.  Retained for duration of the program.
 *
 *      Process Explanation:
 *          Get the FORTH-style string.
 *          At this point, PC points to the byte that follows the string;
 *              we are going to save that byte and replace it with a zero,
 *              thus creating a C-style string.
 *          We will pass that C-style string as an argument to  strdup() ,
 *              which will give us our return value.
 *          Then, of course, we restore the byte we so rudely zeroed, and
 *              proceed merrily on our way.
 *
 **************************************************************************** */

char *get_name(u8 * len)
{
	char *str_start;
	char *retval;
	u8 sav_byt;

	str_start = (char *)get_string(len);

	sav_byt = *pc;
	*pc = 0;

	retval = strdup(str_start);
	*pc = sav_byt;

	return retval;
}

/* **************************************************************************
 *
 *      Function name:  calc_checksum
 *      Synopsis:       Calculate the checksum.
 *                          Leave the input position unchanged.
 *
 *      Inputs:
 *         Parameters:             NONE
 *         Global/Static Variables:     
 *             pc      Pointer to "where we are" in the file-stream
 *
 *      Outputs:
 *         Returned Value:        Calculated checksum.
 *         Global/Static Variables:  
 *             pc     Reset to value upon entry
 *
 *      Process Explanation:
 *          When this routine is entered, the PC is presumed to be pointing
 *              just after the stored checksum, and just before the length
 *              field in the FCode header.  This is the point at which we
 *              will preserve the PC
 *          Extract the length from the FCode header.  It includes the eight
 *              bytes of the FCode header, so we will need to adjust for that.
 *          The first byte after the FCode header is where the checksum
 *              calculation begins.
 *
 **************************************************************************** */

u16 calc_checksum(void)
{
	u16 retval = 0;
	u8 *cksmptr;
	u8 *save_pc;
	u32 fc_blk_len;
	int indx;

	save_pc = pc;

	fc_blk_len = get_num32();	/* Read len */
	cksmptr = get_bytes(fc_blk_len - 8);	/*  Make sure we have all our data  */

	for (indx = 8; indx < fc_blk_len; indx++) {
		retval += *cksmptr++;
	}

	pc = save_pc;
	return retval;
}


/* **************************************************************************
 *
 *      Function name:  adjust_for_pci_header
 *      Synopsis:   Skip the PCI Header.  Adjust the pointer to 
 *                  the start of FCode in the file-stream,
 *                  and our pointer to "where we are" in the FCode,
 *                  by the size of the PCI header.
 *      
 *      Inputs:
 *              Parameters:     NONE
 *              Global/Static Variables:    (Pointer to:)
 *                  pc                     "where we are" in the file-stream
 *                  fc_start               start of FCode in the file-stream
 *
 *      Outputs:
 *              Returned Value: NONE
 *              Global/Static Variables:
 *                  pc                     Advanced past PCI header
 *                  fc_start               Likewise.
 *                  pci_image_found        Set or cleared as appropriate.
 *                  last_defined_token     Re-initialized
 *      
 *      Process Explanation:
 *          Call handle_pci_header to get the size of the PCI header,
 *              if any, and increment  pc  and  fc_start  by the number
 *              of bytes it returns; also, set  pci_image_found  based
 *              on whether the "size of the PCI header" was non-zero.
 *          (Re-)Initialize overlap detection here.  Images with multiple
 *              PCI blocks can safely re-cycle FCode numbers; this is
 *              not necessarily  true of multiple FCode blocks within
 *              the same PCI block...
 *
 **************************************************************************** */

void adjust_for_pci_header(void)
{
	int pci_header_size;

	pci_header_size = handle_pci_header(pc);
	pci_image_found = pci_header_size > 0 ? TRUE : FALSE;
	pc += pci_header_size;
	fc_start += pci_header_size;
	last_defined_token = 0;
}

/* **************************************************************************
 *
 *      Function name:  adjust_for_pci_filler
 *      Synopsis:       Dispatch call to pci-filler-handler
 *
 *      Inputs:
 *         Parameters:                    NONE
 *         Global/Static Variables:
 *             pci_image_found           Whether to proceed...
 *             pci_image_end             Pointer to just after the end of
 *                                           the current PCI image
 *
 *      Outputs:
 *          Returned Value:                NONE
 *          Global/Static Variables:
 *              pci_image_found            Reset to FALSE
 *
 *      Error Detection:
 *          Confirm that the data-stream has the complete filler,
 *              via a call to get_bytes()
 *
 **************************************************************************** */

void adjust_for_pci_filler(void)
{
	if (pci_image_found) {
		int pci_filler_len;
		u8 *pci_filler_ptr;

		pci_filler_len = pci_image_end - pc;
		pci_filler_ptr = get_bytes(pci_filler_len);
		handle_pci_filler(pci_filler_ptr);
		pci_image_found = FALSE;
	}
}
