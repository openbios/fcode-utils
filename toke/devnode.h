#ifndef _TOKE_DEVNODE_H
#define _TOKE_DEVNODE_H

/*
 *                     OpenBIOS - free your system!
 *                         ( FCode tokenizer )
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
 *      External/Prototype/Structure definitions for device-node management
 *
 *      (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
 *      Module Author:  David L. Paktor    dlpaktor@us.ibm.com
 *
 **************************************************************************** */

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "ticvocab.h"

/* **************************************************************************
 *          Structure Name:    device_node_t
 *                        Data for managing a device node; pointers
 *                            to vocabs, data for messaging.
 *                            
 *   Fields:
 *       parent_node         Pointer to similar data for parent node 
 *       line_no             Copy of Line Number where "new-device" was invoked
 *       ifile_name          Name of Input File where "new-device" was invoked
 *       tokens_vocab        Pointer to vocab for this device's tokens
 *
 **************************************************************************** */

typedef struct device_node {
        struct device_node *parent_node ;
	char *ifile_name ;
	unsigned int line_no ;
	tic_hdr_t *tokens_vocab ;
} device_node_t;


/* ************************************************************************** *
 *
 *      Global Variables Exported
 *
 **************************************************************************** */

extern char default_top_dev_ifile_name[];
extern device_node_t *current_device_node;
extern tic_hdr_t **current_definitions;

/* ************************************************************************** *
 *
 *      Function Prototypes / Functions Exported:
 *
 **************************************************************************** */
void new_device_vocab( void );
void delete_device_vocab( void );
void finish_device_vocab( void );
char *in_what_node(device_node_t *the_node);
void show_node_start( void);
bool exists_in_ancestor( char *m_name);

#endif   /*  _TOKE_DEVNODE_H    */
