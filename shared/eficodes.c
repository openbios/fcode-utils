/*
 *                     OpenBIOS - free your system!
 *                         ( FCode tokenizer )
 *
 *  This program is part of a free implementation of the IEEE 1275-1994
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2005 Stefan Reinauer
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
 *      Functions to correlate EFI subsystem type and machine name
 *      with their printable names.
 *
 *      (C) Copyright 2018.  All Rights Reserved.
 *      Module Author:  Radek Zajic        radek@zajic.v.pytli.cz
 *
 *      Based on the classcodes.c file
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

#include <stdlib.h>
#include "types.h"

/* **************************************************************************
 *
 *      Functions Eported:
 *          efi_subsystem_name
 *              Convert a numeric EFI subsystem type to a printable Name
 *
 *          efi_machine_type_name
 *              Convert a numeric EFI machine type to a printable Name
 *
 **************************************************************************** */

const char *efi_machine_type_name(u16 code);
const char *efi_subsystem_name(u16 code);


typedef struct {
    const u32    classcode;
    const char  *classname;
} num_to_name_table ;


/* **************************************************************************
 *
 *      Function name:  convert_num_to_name
 *      Synopsis:   Find the Name, in the given table, for the given Number.
 *                  Support function for Exported Functions.
 *      
 *      Inputs:
 *          Parameters:     
 *              u32 num           Number to look up       
 *      num_to_name_table *table  Pointer to Table to scan        
 *              int max           Maximum Index of Table
 *            char *not_found     String to return if Number not in Table
 *
 *      Outputs:
 *          Returned Value: Pointer to string giving the Name
 *
 *      Error Detection:
 *          Unrecognized Number
 *              Return "not_found" string
 *
 *      Process Explanation:
 *          Scan the Table for a match.
 *
 *      Still to be done / Misc remarks:
 *          Had been considering a more sophisticated binary search,
 *          but the database is too small to merit the extra code.
 *          Stayed with the KISS principle...
 *      
 *
 **************************************************************************** */

static const char *convert_num_to_name(u32 num,
                                 num_to_name_table *table,
                                 int max,
                                 const char *not_found)
{
    int indx;
    const char *retval;

    retval = not_found;

    for (indx = 0;  indx < max ; indx++)
    {
        if ( num == table[indx].classcode )
        {
            retval = table[indx].classname ;
            break ;
        }
     }
     return ( retval );
}


/* **************************************************************************
 *
 *
 *          Structures:
 *      efi_machine_type_name_table    Constant Data Table that correlates
 *                                      Machine Types with their Names.
 *      
 *      efi_subsystem_name_table    Constant Data Table that correlates
 *                                      Subsytem Codes with their Names.
 *
 **************************************************************************** */


static const num_to_name_table efi_machine_type_name_table[] = {
    { 0x01c2 , "ARMTHUMB_MIXED (ARM32/Thumb)" },
    { 0x014c , "IA32 (x86)" },
    { 0x0200 , "IA64 (Itanium)" },
    { 0x8664 , "AMD64 (x86-64)" },
    { 0xAA64 , "ARM64 (AArch64)" },
    { 0x0EBC , "EFI byte code" }
};


static const num_to_name_table efi_subsystem_name_table[] = {
    { 10 , "EFI Application" },
    { 11 , "EFI Boot Service Driver" },
    { 12 , "EFI Runtime Driver." },

};

/* **************************************************************************
 *
 *      Function name:  efi_machine_type_name
 *      Synopsis:   Return the machine type name for the given machine code
 *      
 *      Inputs:
 *          Parameters:
 *                  u32 code        Numeric EFI machine type code
 *
 *      Outputs:
 *          Returned Value:  Pointer to string giving the machine type
 *
 *      Error Detection:
 *          Unrecognized machine type
 *              String returned is "unknown as of EFI specs 2.7"
 *
 *      Process Explanation:
 *          Scan the efi_machine_type_name for a match.
 *
 *
 **************************************************************************** */

const char *efi_machine_type_name(u16 code)
{
    const int pdc_max_indx =
        sizeof(efi_machine_type_name_table)/sizeof(num_to_name_table) ;
    const char *result ;

    result = convert_num_to_name(
           code,
                (num_to_name_table *)efi_machine_type_name_table,
                        pdc_max_indx,
                                "unknown as of EFI specs 2.7");

    return ( result );
}


/* **************************************************************************
 *
 *      Function name:  efi_subsystem_name
 *      Synopsis:   Return a printable Name for the given EFI subsystem name
 *      
 *      Inputs:
 *          Parameters:
 *              u16 code         Numeric EFI subsystem code
 *
 *      Outputs:
 *              Returned Value:  Pointer to string giving printable Name
 *
 *      Error Detection:
 *          Unrecognized EFI subsystem code
 *              String returned is "unknown as of EFI specs 2.7"
 *
 *      Process Explanation:
 *          Scan the efi_subsystem_name for a match.
 *
 *
 **************************************************************************** */

const char *efi_subsystem_name(u16 code)
{
    const int pct_max_indx =
        sizeof(efi_subsystem_name_table)/sizeof(num_to_name_table) ;
    const char *result ;

    result = convert_num_to_name(
          (u32)code,
                (num_to_name_table *)efi_subsystem_name_table,
                        pct_max_indx,
                                "unknown as of EFI specs 2.7");
    return ( result );
}

