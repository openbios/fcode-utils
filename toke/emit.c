/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  emit.c - fcode emitter.
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

/* **************************************************************************
 *
 *      Still to be done:
 *          Re-arrange routine and variable locations to clarify the
 *              functions of this file and its companion, stream.c 
 *          This file should be concerned primarily with management
 *              of the Outputs; stream.c should be primarily concerned
 *              with management of the Inputs.
 *          Hard to justify, pragmatically, but will make for easier
 *              maintainability down the proverbial road...
 *
 **************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pcihdr.h"

#include "toke.h"
#include "vocabfuncts.h"
#include "stack.h"
#include "scanner.h"
#include "emit.h"
#include "clflags.h"
#include "errhandler.h"
#include "stream.h"
#include "nextfcode.h"

/* **************************************************************************
 *
 *          Global Variables Imported
 *              verbose         Enable optional messages, set by "-v" switch
 *              noerrors        "Ignore Errors" flag, set by "-i" switch
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *      Global Variables Exported:
 *          opc                       Output Buffer Position Counter
 *          pci_hdr_end_ob_off        Offsets into Output Buffer of end
 *                                        of last PCI Header Block structure
 *            (To help match up the offset printed in Tokenization_error()
 *              with the offsets shown by the DeTokenizer)
 *
 **************************************************************************** */

unsigned int opc                = 0;
unsigned int pci_hdr_end_ob_off = 0;   /*  0 means "Not initialized"  */

/* **************************************************************************
 *
 *      Macro to zero-fill a field of the size of the given structure
 *          into the Output Buffer using the  emit_byte()  routine.
 *
 **************************************************************************** */

#define EMIT_STRUCT(x)  {int j; for (j=0; j < sizeof(x) ; j++ ) emit_byte(0); }


/* **************************************************************************
 *
 *      Local Static Variables, offsets into Output Buffer of ...:
 *          fcode_start_ob_off      First FCode-Start byte
 *          fcode_hdr_ob_off        FCode Header (Format byte)
 *          fcode_body_ob_off       First functional FCode after FCode Header.
 *          pci_hdr_ob_off          Start of PCI ROM Header Block structure
 *          pci_data_blk_ob_off     Start of PCI Data Header Block structure
 *
 *     For all of these, -1 means "Not initialized"
 *
 *************************************************************************** */

static  int fcode_start_ob_off = -1;
static  int fcode_hdr_ob_off = -1;
static  int fcode_body_ob_off = -1;
static  int pci_hdr_ob_off = -1;
static  int pci_data_blk_ob_off = -1;

/* **************************************************************************
 *
 *          These are to detect a particular error:  If FCode has
 *              been written, before an Fcode-start<n> or before
 *               a PCI-header
 *
 **************************************************************************** */

static bool fcode_written = FALSE;

/* **************************************************************************
 *
 *          Variables and Functions Imported, with
 *          Exposure as Limited as possible:
 *              ostart
 *              olen
 *              increase_output_buffer
 *
 **************************************************************************** */

extern u8 *ostart;
extern int olen;
extern void increase_output_buffer( void);


/* **************************************************************************
 *
 *      Function name:  init_emit
 *      Synopsis:       Initialize Output-related Local Static and Global
 *                          Variables before starting Output.
 *                      Exposure as Limited as possible.
 *
 **************************************************************************** */
void init_emit( void);       /*    Prototype.  Limit Exposure.   */
void init_emit( void)
{
    fcode_start_ob_off  = -1;
    fcode_hdr_ob_off    = -1;
    fcode_body_ob_off   = -1;
    pci_hdr_ob_off      = -1;
    pci_data_blk_ob_off = -1;
    opc                 =  0;
    pci_hdr_end_ob_off  =  0;
    fcode_written       = FALSE;
    haveend             = FALSE;   /*  Get this one too...  */
}


/* **************************************************************************
 *
 *      Function name:  emit_byte
 *      Synopsis:       Fundamental routine for placing a byte
 *                          into the Output Buffer.  Also, check
 *                          whether the buffer needs to be expanded.
 *                      For internal use only.
 *
 **************************************************************************** */

static void emit_byte(u8 data)
{
	if ( opc == olen)
	{
	    increase_output_buffer();
	}
	

	*(ostart+opc) = data ;
	opc++;
}

void emit_fcode(u16 tok)
{
	if ((tok>>8))
		emit_byte(tok>>8);

	emit_byte(tok&0xff);
	fcode_written = TRUE;
}

static void emit_num32(u32 num)
{
	emit_byte(num>>24);
	emit_byte((num>>16)&0xff);
	emit_byte((num>>8)&0xff);
	emit_byte(num&0xff);

}

/* **************************************************************************
 *
 *      Function name:  user_emit_byte
 *      Synopsis:       Action of user-mandated call to emit-byte.
 *                          Covers the corner-case where this is the
 *                          first command issued in the source input.
 *
 **************************************************************************** */

void user_emit_byte(u8 data)
{
	emit_byte( data);
	fcode_written = TRUE;
}

void emit_offset(s16 offs)
{
    /*  Calling routine will test for out-of-range FCode-Offset  */
	if (offs16)
		emit_byte(offs>>8);
	emit_byte(offs&0xff);
}

void emit_literal(u32 num)
{
    emit_token("b(lit)");
    emit_num32(num);
}

void emit_string(u8 *string, signed int cnt)
{
	signed int i=0;
	signed int cnt_cpy = cnt;
	
	if ( cnt_cpy > STRING_LEN_MAX )
	{
	    tokenization_error( TKERROR,
		"String too long:  %d characters.  Truncating.\n",cnt);
	    cnt_cpy = STRING_LEN_MAX ;
	}
	emit_byte(cnt_cpy);
	for (i=0; i<cnt_cpy; i++)
		emit_byte(string[i]);
}

void emit_fcodehdr(const char *starter_name)
{
	
   /*  Check for error conditions   */
    if ( fcode_written )
    {
        tokenization_error( TKERROR ,
	    "Cannot create FCode header after FCode output has begun.\n");
        if ( ! noerrors ) return ;
    }

	fcode_header_t *fcode_hdr;

	fcode_start_ob_off = opc;
	emit_token( starter_name );

	fcode_hdr_ob_off = opc;
	fcode_hdr = (fcode_header_t *)(ostart+fcode_hdr_ob_off);

	EMIT_STRUCT(fcode_header_t);

	fcode_body_ob_off = opc;

	/* Format = 8 means we comply with IEEE 1275-1994 */
	fcode_hdr->format = 0x08;

}

/* **************************************************************************
 *
 *      Function name:    finish_fcodehdr
 *          Fill in the FCode header's checksum and length fields, and
 *              reset the FCode-header pointer for the next image.
 *
 *          If  haveend  is true then the end0 has already been written
 *              and  fcode_ender()  has been called.
 *
 *          Print a WARNING message if the end-of-file was encountered
 *              without an end0 or an fcode-end
 *
 *          Print an informative message to standard-output giving the
 *              checksum.  Call  list_fcode_ranges()  to print the
 *              value of the last FCode-token that was assigned or
 *              the Ranges of assigned FCode-token values if so be...
 *              The final FCode-binary file-length will be printed
 *              when the binary file is being closed.
 *
 **************************************************************************** */

void finish_fcodehdr(void)
{

	if( fcode_hdr_ob_off == -1 )
	{
		tokenization_error( TKERROR,
		    "Missing FCode header.\n");
		return ;
	}

	/* If the program did not end cleanly, we'll handle it */
	if (!haveend)
	{
	    tokenization_error ( WARNING,
	    "End-of-file encountered without END0 or FCODE-END.  "
		"Supplying END0\n");
		emit_token("end0");
	    fcode_ender();
	}

	/*  Calculate and place checksum and length, if haven't already  */
	if ( fcode_start_ob_off != -1 )
	{
	    u32 checksum;
	    int length;

	    u8 *fcode_body = ostart+fcode_body_ob_off;
	    u8 *ob_end = ostart+opc;
	    fcode_header_t *fcode_hdr =
	         (fcode_header_t *)(ostart+fcode_hdr_ob_off);
	
	    length = opc - fcode_start_ob_off;

	    for ( checksum = 0;
	              fcode_body < ob_end ;
		          checksum += *(fcode_body++) ) ;

	    if (sun_style_checksum) {
		/* SUN OPB on the SPARC (Enterprise) platforms (especially):
                 * M3000, M4000, M9000 expects a checksum algorithm that is
                 * not compliant with IEEE 1275-1994 section 5.2.2.5.
		 */
		checksum = (checksum & 0xffff) + (checksum >> 16);
		checksum = (checksum & 0xffff) + (checksum >> 16);
	    }

	    BIG_ENDIAN_WORD_STORE(fcode_hdr->checksum, 
		    (u16)(checksum & 0xffff));
	    BIG_ENDIAN_LONG_STORE(fcode_hdr->length , length);

	if (verbose)
	    {
		printf( "toke: checksum is 0x%04x (%d bytes).  ",
                        (u16)checksum, length);
		list_fcode_ranges( TRUE);
	    }
	}

	/*  Reset things for the next image...   */
	fcode_start_ob_off = -1;
	fcode_hdr_ob_off   = -1;
	fcode_body_ob_off  = -1;
	fcode_written      = FALSE;
	haveend=FALSE;
}

/* **************************************************************************
 *
 *      Function name:  emit_pci_rom_hdr
 *      Synopsis:       Write the PCI ROM Header Block into the Output Buffer
 *
 *      Inputs:
 *         Parameters:                   NONE
 *         Global Variables:        
 *             opc                       Output Buffer Position Counter
 *             ostart                    Start of Output Buffer
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Global Variables:    
 *             pci_hdr_ob_off            PCI ROM Header Block Position
 *                                           (Offset) in Output Buffer
 *         FCode Output buffer:
 *             Write the part of the PCI ROM Header Block we know:
 *                 Fill in the signature and the field reserved for
 *                 "processor architecture unique data".
 *             Fill in the "Pointer to PCI Data Structure" with the
 *                 size of the data structure, because the first PCI
 *                 Data Structure will follow immediately.
 *
 *      Error Detection:   (Handled by calling routine)
 *
 **************************************************************************** */

static void emit_pci_rom_hdr(void)
{
    rom_header_t *pci_hdr;
    pci_hdr_ob_off = opc;
    pci_hdr = (rom_header_t *)(ostart + pci_hdr_ob_off);

    EMIT_STRUCT(rom_header_t);
	
	/* PCI start signature */
    LITTLE_ENDIAN_WORD_STORE(pci_hdr->signature,0xaa55);
	
	/* Processor architecture */
	/*  Note:
	 *  The legacy code used to read:
	 *
	 *        pci_hdr->reserved[0] = 0x34;
	 *
	 *  I don't know what significance the literal  34  had, but
	 *      by what might just be an odd coincidence, it is equal
	 *      to the combined lengths of the  PCI-ROM-  and  PCI-Data-
	 *      headers.
	 *
	 *  I suspect that it is more than merely an odd coincidence,
	 *      and that the following would be preferable:
	 */

    LITTLE_ENDIAN_WORD_STORE( pci_hdr->reserved ,
	(sizeof(rom_header_t) + sizeof(pci_data_t)) ) ;

	/* already handled padding */

	/* pointer to start of PCI data structure */
    LITTLE_ENDIAN_WORD_STORE(pci_hdr->data_ptr, sizeof(rom_header_t) );

}
	
/* **************************************************************************
 *
 *      Function name:  emit_pci_data_block
 *      Synopsis:       Write the PCI Data Block into the Output Buffer
 *
 *      Inputs:
 *         Parameters:                   NONE
 *         Global Variables:        
 *             opc                       Output Buffer Position Counter
 *         Data-Stack Items:
 *             Top:                      Class Code
 *             Next:                     Device ID
 *             3rd:                      Vendor ID
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Global Variables:    
 *             pci_data_blk_ob_off       PCI Data Header Block Position
 *                                           (Offset) in Output Buffer
 *         Data-Stack, # of Items Popped:  3
 *         FCode Output buffer:
 *             Write the PCI Data Header Block:  Fill in the signature,
 *                 Vendor ID, Device ID and Class Code, and everything
 *                 else whose value we know.  (Size and Checksum will
 *                 have to wait until we finish the image...)
 *         Printout:
 *             Advisory of Vendor, Device and Class 
 *
 *      Error Detection:   (Handled by calling routine)
 *
 **************************************************************************** */

static void emit_pci_data_block(void)
{
    pci_data_t *pci_data_blk;
    u32 class_id = dpop();
    u16 dev_id   = dpop();
    u16 vend_id  = dpop();

    pci_data_blk_ob_off = opc;
    pci_data_blk = (pci_data_t *)(ostart + pci_data_blk_ob_off);

    EMIT_STRUCT(pci_data_t);

    BIG_ENDIAN_LONG_STORE(pci_data_blk->signature , PCI_DATA_HDR );

    LITTLE_ENDIAN_WORD_STORE(pci_data_blk->vendor , vend_id );
    LITTLE_ENDIAN_WORD_STORE(pci_data_blk->device , dev_id );
    LITTLE_ENDIAN_TRIPLET_STORE(pci_data_blk->class_code , class_id );

    LITTLE_ENDIAN_WORD_STORE(pci_data_blk->dlen ,  sizeof(pci_data_t) );

    pci_data_blk->drevision = PCI_DATA_STRUCT_REV ;

	/* code type = open firmware = 1 */
    pci_data_blk->code_type = 1;

	/* last image flag */
    pci_data_blk->last_image_flag = pci_is_last_image ? 0x80 : 0 ;

    tokenization_error(INFO ,
	"PCI header vendor id=0x%04x, "
	    "device id=0x%04x, class=%06x\n",
		vend_id, dev_id, class_id );

}

/* **************************************************************************
 *
 *      Function name:  emit_pcihdr
 *      Synopsis:       Supervise the writing of PCI Header information
 *                          into the Output Buffer, when the  PCI-HEADER
 *                          tokenizer directive has been encountered.
 *
 *      Inputs:
 *         Parameters:                   NONE
 *         Global Variables:        
 *             opc                       Output Buffer Position Counter      
 *             fcode_start_ob_off        Initted if FCode output has begun
 *             noerrors                  The "Ignore Errors" flag
 *
 *      Outputs:
 *         Returned Value:               NONE
 *         Global Variables:    
 *            pci_hdr_end_ob_off         Set to the Output Buffer Position
 *                                           Counter after the PCI Header
 *         FCode Output buffer      
 *            :The beginning of the PCI Header will be entered, waiting for
 *                 the fields that could not be determined until the end
 *                 to be filled in.
 *
 *      Error Detection:
 *          An attempt to write a PCI Header after FCode output -- either an
 *              Fcode-start<n> or ordinary FCode -- has begun is an ERROR.
 *              Report it; do nothing further, unless "Ignore Errors" is set.
 *
 **************************************************************************** */

void emit_pcihdr(void)
{

    /*  Check for error conditions   */
    if (
    /*  FCODE-START<n>  has already been issued  */
              ( fcode_start_ob_off != -1 )
    /*  Other FCode has been written             */
	      ||  fcode_written
	 )
    {
        tokenization_error( TKERROR ,
	    "Cannot create PCI header after FCode output has begun.\n");
        if ( ! noerrors ) return ;
	}

	emit_pci_rom_hdr();

	emit_pci_data_block();

	pci_hdr_end_ob_off = opc;
}

/* **************************************************************************
 *
 *      Function name:  finish_pcihdr
 *      Synopsis:       Fill-in the fields of the PCI Header that could
 *                      not be determined until the end of the PCI-block.
 *
 *************************************************************************** */

void finish_pcihdr(void)
{

	u32 imagesize ;
	u32 imageblocks;
	int padding;
	
	rom_header_t *pci_hdr;
	pci_data_t   *pci_data_blk;

	if( pci_data_blk_ob_off == -1 )
	{
	    tokenization_error( TKERROR,
		"%s without PCI-HEADER\n", strupr(statbuf) );
	    return ;
	}

	pci_hdr = (rom_header_t *)(ostart + pci_hdr_ob_off);
	pci_data_blk = (pci_data_t *)(ostart + pci_data_blk_ob_off);

	/* fix up vpd */
	LITTLE_ENDIAN_WORD_STORE(pci_data_blk->vpd, pci_vpd);

	/*   Calculate image size and padding */
	imagesize = opc - pci_hdr_ob_off;     /*  Padding includes PCI hdr  */
	imageblocks = (imagesize + 511) >> 9; /*  Size in 512-byte blocks   */
	padding = (imageblocks << 9) - imagesize;

	/* fix up image size. */
	LITTLE_ENDIAN_WORD_STORE(pci_data_blk->ilen, imageblocks);
	
	/* fix up revision */
	if ( big_end_pci_image_rev )
	{
	    BIG_ENDIAN_WORD_STORE(pci_data_blk->irevision, pci_image_rev);
	}else{
	    LITTLE_ENDIAN_WORD_STORE(pci_data_blk->irevision, pci_image_rev);
	}
	
	/* fix up last image flag */
	pci_data_blk->last_image_flag = pci_is_last_image ? 0x80 : 0 ;
	
	/* align to 512bytes */
	
	printf("Adding %d bytes of zero padding to PCI image.\n",padding);
	while (padding--)
		emit_byte(0);
	if ( ! pci_is_last_image )
	{
	    printf("Note:  PCI header is not last image.\n");
	}
	printf("\n");
	
	pci_hdr_ob_off      = -1;
	pci_data_blk_ob_off = -1;
	pci_hdr_end_ob_off  =  0;
}


/* **************************************************************************
 *
 *      Function name:  finish_headers
 *      Synopsis:       Fill-in the fields of the FCode- and PCI- Headers
 *                      that could not be determined until the end.
 *
 *************************************************************************** */

void finish_headers(void)
{
	if (fcode_hdr_ob_off != -1) finish_fcodehdr();
	if (pci_hdr_ob_off != -1) finish_pcihdr();
}

