/*
 *                     OpenBIOS - free your system!
 *                         ( romheaders utility )
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*   We needn't be concerned with the endian-ness of the host,
 *       only with the endian-ness of the data.
 */

#include "types.h"
#include "pcihdr.h"


char *rom=NULL;
size_t romlen=0;


/*  Prototypes for functions exported from  devsupp/pci/classcodes.c   */

char *pci_device_class_name(u32 code);
char *pci_code_type_name(unsigned char code);

/*  Functions local to this file:
	    bool dump_rom_header(rom_header_t *data);
	    bool dump_pci_data(pci_data_t *data);
	    void dump_platform_extensions(u8 type, rom_header_t *data);
 */

static bool dump_rom_header(rom_header_t *data)
{   /*  Return TRUE for "no problem"  */
	const u16 pci_header_signature = 0x55aa;
	u16 sig=BIG_ENDIAN_WORD_FETCH(data->signature);
	int i;
	
	printf ("PCI Expansion ROM Header:\n");
	
	printf ("  Signature: 0x%04x (%s)\n", 
			sig, sig == pci_header_signature ? "Ok":"Not Ok");
	
	printf ("  CPU unique data:");
	for (i=0;i<16;i++) {
		printf(" 0x%02x",data->reserved[i]);
		if (i==7) printf("\n                  ");
	}
	
	printf ("\n  Pointer to PCI Data Structure: 0x%04x\n\n",
				LITTLE_ENDIAN_WORD_FETCH(data->data_ptr));

	return (BOOLVAL(sig == pci_header_signature) );
}

static bool dump_pci_data(pci_data_t *data)
{   /*  Return TRUE for "no problem"  */
	const u32 pci_data_hdr = PCI_DATA_HDR ;

	u32 sig      = BIG_ENDIAN_LONG_FETCH(data->signature);
	u32 classcode= CLASS_CODE_FETCH(data->class_code);
	u32 dlen     = (u32)LITTLE_ENDIAN_WORD_FETCH(data->dlen);
	u32 ilen     = (u32)LITTLE_ENDIAN_WORD_FETCH(data->ilen);
	
	printf("PCI Data Structure:\n");
	printf("  Signature: 0x%04x '%c%c%c%c' ", sig,
			sig>>24,(sig>>16)&0xff, (sig>>8)&0xff, sig&0xff);
	printf("(%s)\n", sig == pci_data_hdr ?"Ok":"Not Ok");

	printf("  Vendor ID: 0x%04x\n", LITTLE_ENDIAN_WORD_FETCH(data->vendor));
	printf("  Device ID: 0x%04x\n", LITTLE_ENDIAN_WORD_FETCH(data->device));
	printf("  Vital Product Data:  0x%04x\n",
	                                   LITTLE_ENDIAN_WORD_FETCH(data->vpd));
	printf("  PCI Data Structure Length: 0x%04x (%d bytes)\n", dlen, dlen);
	printf("  PCI Data Structure Revision: 0x%02x\n", data->drevision);
	printf("  Class Code: 0x%06x (%s)\n",classcode,
					 pci_device_class_name(classcode));
	printf("  Image Length: 0x%04x blocks (%d bytes)\n", ilen, ilen*512);
	printf("  Revision Level of Code/Data: 0x%04x\n",
			(u32)LITTLE_ENDIAN_WORD_FETCH(data->irevision));
	printf("  Code Type: 0x%02x (%s)\n", data->code_type,
					  pci_code_type_name(data->code_type) );
	printf("  Last-Image Flag: 0x%02x (%slast image in rom)\n",
			data->last_image_flag,
			data->last_image_flag&0x80?"":"not ");
	printf("  Reserved: 0x%04x\n\n", little_word(data->reserved_2));

	return (BOOLVAL(sig==PCI_DATA_HDR) );
}

static void dump_platform_extensions(u8 type, rom_header_t *data)
{
	u32 entry;
	
	switch (type) {
	case 0x00:
		printf("Platform specific data for x86 compliant option rom:\n");
		printf("  Initialization Size: 0x%02x (%d bytes)\n",
				data->reserved[0], data->reserved[0]*512);

		/* We do a hack here - implement a jump disasm to be able
		 * to output correct offset for init code. Once again x86
		 * is ugly.
		 */
		
		switch (data->reserved[1]) {
		case 0xeb: /* short jump */
			entry = data->reserved[2] + 2;
			/* a short jump instruction is 2 bytes,
			 * we have to add those to the offset
			 */
			break;
		case 0xe9: /* jump */
			entry = ((data->reserved[3]<<8)|data->reserved[2]) + 3;
			/* jump is 3 bytes, so add them */
			break;
		default:
			entry=0;
			break;
		}

		if (entry) {
			/* 0x55aa rom signature plus 1 byte len */
			entry += 3;
			printf( "  Entry point for INIT function:"
				" 0x%x\n\n",entry);
		} else
			printf( "  Unable to determine entry point for INIT"
				" function. Please report.\n\n");
		
		break;
	case 0x01:
		printf("Platform specific data for Open Firmware compliant rom:\n");
		printf("  Pointer to FCode program: 0x%04x\n\n",
				data->reserved[1]<<8|data->reserved[0]);
		break;
	default:
		printf("Parsing of platform specific data not available for this image\n\n");
	}
}

int main(int argc, char **argv)
{
	char		*name=argv[1];
	FILE		*romfile;
	struct stat 	finfo;

	rom_header_t	*rom_header;
	pci_data_t	*pci_data;

	int i=1;

	if (argc!=2) {
		printf ("\nUsage: %s <romimage.img>\n",argv[0]);
		printf ("\n  romheaders dumps pci option rom headers "
				"according to PCI \n"
				"  specs 2.2 in human readable form\n\n");
		return -1;
	}
	
	if (stat(name,&finfo)) {
		printf("Error while reading file information.\n");
		return -1;
	}

	romlen=finfo.st_size;

	rom=malloc(romlen);
	if (!rom) {
		printf("Out of memory.\n");
		return -1;
	}

        romfile=fopen(name,"r");
        if (!romfile) {
		printf("Error while opening file\n");
		return -1;
	}

	if (fread(rom, romlen, 1, romfile)!=1) {
		printf("Error while reading file\n");
		free(rom);
		return -1;
	}

	fclose(romfile);
	
	rom_header=(rom_header_t *)rom;

	do {
		printf("\nImage %d:\n",i);
		if (!dump_rom_header(rom_header)) {
			printf("Rom Header error occured. Bailing out.\n");
			break;
		}
		
		pci_data = (pci_data_t *)((char *)rom_header +
			     LITTLE_ENDIAN_WORD_FETCH(rom_header->data_ptr));
		
		if (!dump_pci_data(pci_data)) {
			printf("PCI Data error occured. Bailing out.\n");
			break;
		}
		
		dump_platform_extensions(pci_data->code_type, rom_header);
		
		rom_header = (rom_header_t *)((char *)rom_header +
				LITTLE_ENDIAN_WORD_FETCH(pci_data->ilen)*512);
		i++;
	} while ((pci_data->last_image_flag&0x80)!=0x80 &&
			(char *)rom_header < rom+(int)romlen );

	return 0;
}

