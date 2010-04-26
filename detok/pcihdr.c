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
 *      Support function for de-tokenizer.
 *
 *      Identify and process PCI header at beginning of FCode binary file.
 *      "Processing" consists of recognizing the PCI Header and Data Structure,
 *      optionally printing a description thereof, and (mainly) allowing
 *      the given pointer to be bumped to the start of the actual FCode.
 *
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 *      Revision History:
 *
 *      Updated Mon, 23 May 2005 by David L. Paktor
 *          Identify "Not Last" header.
 *      Updated Thu, 24 Feb 2005 by David L. Paktor
 *          Per notes after Code Review.
 *      Updated Fri, 04 Feb 2005 by David L. Paktor
 *      Updated Wed, 08 Jun 2005 by David L. Paktor
 *         Added support for multiple-PCI-image files.
 *
 *
 **************************************************************************** */

/* **************************************************************************
 *
 *      Functions Eported:
 *          handle_pci_header
 *              Handle all activities connected with presence of
 *              PCI Header/Data at beginning of FCode file, and
 *              facilitate "skipping" over to actual FCode data.
 *
 *          handle_pci_filler
 *          Skip past "filler" between blocks in multi-PCI-image files.
 *              
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Still to be done:
 *          Print (as remarks) full descriptions of headers' fields
 *          Error check for wrong "Format"
 *          Skip past non-FCode blocks, thru multiple data-blocks
 *          Recognize PCI header in unexpected place or out-of-place
 *
 **************************************************************************** */

#include "pcihdr.h"
#include <stdio.h>

#include "detok.h"


/* **************************************************************************
 *
 *          Global Variables Exported
 *      pci_image_end           Pointer to just after end of current PCI image
 *
 **************************************************************************** */

u8 *pci_image_end = NULL;

/* **************************************************************************
 *
 *          Internal Static Variables
 *      pci_image_len           Length (in bytes) of current PCI image
 *
 **************************************************************************** */

static int pci_image_len = 0;


/* **************************************************************************
 *
 *      Function name:  is_pci_header ( rom_header_t *pci_rom_hdr )
 *      Synopsis:   Indicate whether given pointer is pointing to
 *                  something that might be a valid PCI header
 *      
 *      Inputs:
 *          Parameters:     
 *              pci_rom_hdr    pointer to start of data-stream to examine.
 *                          Treat as pointer to rom_header_t
 *
 *      Outputs:
 *          Returned Value: An integer.
 *               0                  Definitely *NOT* a PCI header
 *              Positive Number     Appears to be a valid PCI header;
 *                                      value is offset to PCI Data Structure.
 *              Negative Number     Appears to be a PCI header, but
 *                                      with errors. (Not Implemented Yet.
 *                                      See under "Still to be done".)
 *
 *      Error Detection:
 *              (See under "Still to be done".)
 *
 *      Process Explanation:
 *          Examine "signature" location for known value  0x55aa
 *          If a match, return value of "dptr" (Data-pointer offset) field.
 *
 *      Revision History:
 *          Created Tue, 01 Feb 2005 by David L. Paktor
 *
 *      Still to be done:
 *          Error-check; look for inconsistencies:
 *              Return a Negative Number if data-stream appears to be a PCI
 *              header, but has erroneous or inconsistent sub-field contents.
 *              Value and meaning of the Negative Number yet to be defined.
 *
 **************************************************************************** */

static int is_pci_header(rom_header_t * pci_rom_hdr)
{
	const u16 pci_header_signature = 0x55aa;
	int retval;

	retval = 0;

	if (BIG_ENDIAN_WORD_FETCH(pci_rom_hdr->signature) == pci_header_signature) {
		retval = LITTLE_ENDIAN_WORD_FETCH(pci_rom_hdr->data_ptr);
	}
	return (retval);
}

/* **************************************************************************
 *
 *      Function name:  is_pci_data_struct ( pci_data_t *pci_data_ptr )
 *      Synopsis:       Indicate whether given pointer is pointing to
 *                      a valid PCI Data Structure
 *      
 *      Inputs:
 *          Parameters:     
 *              pci_data_ptr    pointer to start of data-stream to examine.
 *                          Treat as pointer to pci_data_t 
 *
 *      Outputs:
 *          Returned Value: An integer.
 *                   0                  Definitely *NOT* a PCI Data Structure
 *              Positive Number         Appears to be valid PCI Data Structure;
 *                                      value is length of PCI Data Structure,
 *                                      (presumably, offset to start of FCode).
 *              Negative Number         Appears to be a PCI Data Structure,
 *                                      but with errors. (Not Implemented Yet.
 *                                      See under "Still to be done".)
 *
 *          Global/Static Variables:
 *              Does not alter the poiner passed-in;
 *              does not alter any Global/Static Variables
 *          Printout:       NONE
 *
 *      Error Detection:        (Condition)     (Action)
 *              (See under "Still to be done".)
 *
 *      Process Explanation:
 *          Examine "signature" location for known value "PCIR"
 *          If a match, return value of "dlen" (Data Structure Length) field.
 *      
 *      Revision History:
 *          Created Tue, 01 Feb 2005 by David L. Paktor
 *
 *      Still to be done:
 *          Error-check; look for wrong "Code Type" or other inconsistencies:
 *              Return a Negative Number if data-stream appears to be a
 *              valid PCI Data Structure, but has erroneous or inconsistent
 *              sub-field contents.
 *              Value and meaning of the Negative Number yet to be defined.
 *          Skip past non-FCode data-blocks, even multiple blocks
 *
 **************************************************************************** */

static int is_pci_data_struct(pci_data_t * pci_data_ptr)
{
	int retval;

	retval = 0;

	if (BIG_ENDIAN_LONG_FETCH(pci_data_ptr->signature) == PCI_DATA_HDR) {
		retval = LITTLE_ENDIAN_WORD_FETCH(pci_data_ptr->dlen);
	}
	return (retval);
}


/* **************************************************************************
 *
 *      Function name:  announce_pci_hdr ( rom_header_t *pci_rom_hdr )
 *      Synopsis:       Print indication that the PCI header was found,
 *                      and other details, formatted as FORTH remarks.
 *      
 *      Inputs:
 *              Parameters:
 *                      pci_rom_hdr        Pointer to start of PCI header.
 *
 *      Outputs:
 *              Returned Value: NONE
 *              Printout:       Announcement.  Size of  data_ptr  field.
 *
 **************************************************************************** */

static void announce_pci_hdr(rom_header_t * pci_rom_hdr)
{
	char temp_buf[80];
	u32 temp;

	printremark("PCI Header identified");
	temp = (u32) LITTLE_ENDIAN_WORD_FETCH(pci_rom_hdr->data_ptr);
	sprintf(temp_buf, "  Offset to Data Structure = 0x%04x (%d)\n",
		temp, temp);
	printremark(temp_buf);
}

/* **************************************************************************
 *
 *      Function name:  announce_pci_data_struct ( pci_data_t *pci_data_ptr )
 *      Synopsis:       Print indication that the PCI Data Structure
 *                      was found, and some additional details.
 *                      Format as FORTH remarks.
 *      
 *      Inputs:
 *          Parameters:
 *              pci_data_ptr        Pointer to start of PCI Data Structure.
 *
 *      Outputs:
 *          Returned Value: NONE    
 *          Global/Static Variables:    
 *              pci_image_len      Updated to byte-length of current PCI image
 *          Printout:       (See Synopsis)
 *      
 *      Process Explanation:
 *          Extract some details, format and print them,
 *              using the syntax of FORTH remarks.
 *
 *      Revision History:
 *          Created Tue, 01 Feb 2005 by David L. Paktor
 *          Updated Wed, 25 May 2005 by David L. Paktor
 *               Added printout of several fields...
 *
 **************************************************************************** */

static void announce_pci_data_struct(pci_data_t * pci_data_ptr)
{
	char temp_buf[80];
	u32 temp;

	printremark("PCI Data Structure identified");

	temp = (u32) LITTLE_ENDIAN_WORD_FETCH(pci_data_ptr->dlen);
	sprintf(temp_buf, "  Data Structure Length = 0x%04x (%d)\n", temp, temp);
	printremark(temp_buf);

	sprintf(temp_buf, "  Vendor ID: 0x%04x\n",
		LITTLE_ENDIAN_WORD_FETCH(pci_data_ptr->vendor));
	printremark(temp_buf);

	sprintf(temp_buf, "  Device ID: 0x%04x\n",
		LITTLE_ENDIAN_WORD_FETCH(pci_data_ptr->device));
	printremark(temp_buf);

	temp = (u32) CLASS_CODE_FETCH(pci_data_ptr->class_code);
	sprintf(temp_buf, "  Class Code: 0x%06x  (%s)",
		temp, pci_device_class_name(temp));
	printremark(temp_buf);

	temp = (u32) LITTLE_ENDIAN_WORD_FETCH(pci_data_ptr->vpd);
	if (temp != 0) {
		sprintf(temp_buf, "  Vital Prod Data: 0x%02x\n", temp);
		printremark(temp_buf);
	}

	temp = (u32) LITTLE_ENDIAN_WORD_FETCH(pci_data_ptr->irevision);
	if (temp != 0) {
		sprintf(temp_buf, "  Image Revision: 0x%02x\n", temp);
		printremark(temp_buf);
	}

	sprintf(temp_buf, "  Code Type: 0x%02x (%s)\n",
		pci_data_ptr->code_type,
		pci_code_type_name(pci_data_ptr->code_type));
	printremark(temp_buf);

	temp = (u32) LITTLE_ENDIAN_WORD_FETCH(pci_data_ptr->ilen);
	pci_image_len = temp * 512;
	sprintf(temp_buf, "  Image Length: 0x%04x blocks (%d bytes)\n",
		temp, pci_image_len);
	printremark(temp_buf);

	sprintf(temp_buf, "  %sast PCI Image.\n",
		pci_data_ptr->last_image_flag && 0x80 != 0 ? "L" : "Not l");
	printremark(temp_buf);

}


/* **************************************************************************
 *
 *      Function name:  handle_pci_header
 *      Synopsis:       Handle PCI Header/Data at beginning of FCode file;
 *                      facilitate "skipping" over to actual FCode data.
 *      
 *      Inputs:
 *          Parameters:     
 *              data_ptr           Pointer to start of data-stream to examine.
 *          Global/Static Variables:        
 *              pci_image_len      Length (in bytes) of current PCI image
 *
 *      Outputs:
 *          Returned Value:
 *              Positive Number.        Offset to start of FCode.
 *              Zero                    If no PCI header; may be treated as
 *                                          a valid offset.
 *              Negative Number         PCI header or PCI Data Structure test
 *                                          returned error indication.
 *                                         (Not Implemented Yet.  See
 *                                          under "Still to be done".)
 *          Global/Static Variables:        
 *              pci_image_end           Pointer to just after the end of
 *                                          the current PCI image
 *          Printout:       As FORTH remarks, print indications that the
 *                          PCI header was found, and maybe later more data.
 *
 *      Error Detection:        (Condition)     (Action)
 *              (See under "Still to be done".)
 *      
 *      Process Explanation:
 *          Use the various support routines defined below.
 *      
 *      
 *      Revision History:
 *
 *      Updated Wed, 09 Feb 2005 by David L. Paktor
 *          Extracted assignments from within  if( )  statements.
 *
 *      Created Tue, 01 Feb 2005 by David L. Paktor
 *
 *      Still to be done:
 *          Handle error cases.  At present, neither  is_pci_header()
 *              nor  is_pci_data_struct()  returns a negative number,
 *              but when they are modified to do so, we must handle it.
 *
 **************************************************************************** */

int handle_pci_header(u8 * data_ptr)
{
	int hdrlen;
	int data_struc_len;
	/*  int retval;  *//*  Not needed until we handle error cases...  */

	data_struc_len = 0;

	hdrlen = is_pci_header((rom_header_t *) data_ptr);
	/*  retval = hdrlen;  *//*  Not needed yet...  */
	if (hdrlen < 0) {
		/*  Handle error case...  */
		/*  Leave null for now...  */
		/*  It might need to do a premature EXIT here...  */
	} else {
		/* if hdrlen == 0 then we don't need to check a Data Structure  */
		if (hdrlen > 0) {
			announce_pci_hdr((rom_header_t *) data_ptr);
			data_struc_len = is_pci_data_struct((pci_data_t *) & data_ptr[hdrlen]);
			/*
			 *  A Data Structure Length of Zero would be an error
			 *  that could be detected by  is_pci_data_struct()
			 */
			if (data_struc_len <= 0) {
				/*  Handle error case...  */
				/*  Leave null for now...  */
				/*  It might need to do a premature EXIT here...  */
				/*  retval = -1;   *//*  Not needed yet...  */
			} else {
				announce_pci_data_struct((pci_data_t *) & data_ptr[hdrlen]);
				pci_image_end = data_ptr + pci_image_len;
				/* retval = hdrlen+data_struc_len; *//*  Not needed yet... */
			}
		}
	}
	return (hdrlen + data_struc_len);
}


/* **************************************************************************
 *
 *      Function name:  handle_pci_filler
 *      Synopsis:       Examine and report on the "filler" padding after the
 *                      end of an FCode-block but still within a PCI-image
 *
 *      Inputs:
 *         Parameters:
 *             filler_ptr         Pointer to start of PCI-filler in data-stream
 *         Global/Static Variables:    
 *             pci_image_end      Pointer to just after the end of
 *                                          the current PCI image
 *
 *      Outputs:
 *         Returned Value:        NONE
 *         Printout:
 *             Descriptive message.
 *
 *      Error Detection:
 *          Non-zero filler field.  Different message.
 *
 *      Process Explanation:
 *          The calling routine has checked that there was, indeed, a PCI
 *              header present, so we know that pci_image_end is valid.
 *          If the entire filler is zero-bytes, print a simple message and
 *              we're out'a here!
 *          If there are non-zero bytes, identify loc'n of first non-zero.
 *
 *      Still to be done:
 *          Come up with something more elegant for non-zero filler.
 *
 **************************************************************************** */

void handle_pci_filler(u8 * filler_ptr)
{
	u8 *scan_ptr;
	int filler_len;
	char temp_buf[80];
	bool all_zero = TRUE;
	u8 filler_byte = *filler_ptr;

	filler_len = pci_image_end - filler_ptr;

	for (scan_ptr = filler_ptr;
	     scan_ptr < pci_image_end; filler_byte = *(++scan_ptr)) {
		if (filler_byte != 0) {
			all_zero = FALSE;
			break;
		}
	}

	if (all_zero) {
		sprintf(temp_buf, "PCI Image padded with %d bytes of zero", filler_len);
	} else {
		sprintf(temp_buf, "PCI Image padding-field of %d bytes "
			"had first non-zero byte at offset %ld",
			filler_len, (unsigned long)(scan_ptr - filler_ptr));
	}
	printremark(temp_buf);
}
