/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  stack.c - data and return stack handling for fcode tokenizer.
 *  
 *  This program is part of a free implementation of the IEEE 1275-1994 
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2010 by Stefan Reinauer <stepan@openbios.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stack.h"
#include "scanner.h"
#include "errhandler.h"

/* **************************************************************************
 *
 *          Global Variables Imported
 *              statbuf          The word just read from the input stream
 *
 **************************************************************************** */


/* **************************************************************************
 *
 *          Global Variables Exported
 *              dstack         Pointer to current item on top of Data Stack
 *
 **************************************************************************** */

long *dstack;

/* **************************************************************************
 *
 *      Local/Static Pointers  .....      to  ...         -> Points to ...
 *          startdstack         Start of data-stack area  -> last possible item
 *          enddstack           End of data-stack area    -> past first item
 *
 *************************************************************************** */

static long *startdstack;
static long *enddstack;

void clear_stack(void)
{
    dstack = enddstack;
}

/* internal stack functions */

void init_stack(void)
{
	startdstack = safe_malloc(MAX_ELEMENTS*sizeof(long), "initting stack");
	enddstack = startdstack + MAX_ELEMENTS;
	dstack=enddstack;
}

/*  Input Param:  stat   TRUE = Underflow, FALSE = Overflow   */
static void stackerror(bool stat)
{
    /*
     *   Because all of our stack operations are protected and
     *       we have no risk of the  dstack  pointer going into
     *       invalid territory, this error needs not be  FATAL.
     */
    tokenization_error ( TKERROR , "stack %sflow at or near  %s \n",
	 (stat)?"under":"over" , statbuf );
}

/*
 *  Return TRUE if the stack depth is equal to or greater than
 *      the supplied minimum requirement.  If not, print an error.
 */
bool min_stack_depth(int mindep)
{
    bool retval = TRUE ;
    long *stack_result;

    stack_result = dstack + mindep;
    /*
     *  The above appears counter-intuitive.  However, C compensates
     *      for the size of the object of a pointer when it handles
     *      address arithmetic.  A more explicit expression that would
     *      yield the same result, might look something like this:
     *
     *        (long *)((int)dstack + (mindep * sizeof(long)))
     *
     *  I doubt that that form would yield tighter code, or otherwise
     *      represent any material advantage...
     */

    if ( stack_result > enddstack )
    {
	retval = FALSE;
	stackerror(TRUE);
    }

    return ( retval );
}

/*
 *  Return TRUE if the stack has room for the supplied # of items
 */
static bool room_on_stack_for(int newdep)
{
    bool retval = TRUE ;
    long *stack_result;

    stack_result = dstack - newdep;
    /*  See above note about "counter-intuitive" pointer address arithmetic  */

    if ( stack_result < startdstack )
    {
	retval = FALSE;
	stackerror(FALSE);
    }

    return ( retval );
}

void dpush(long data)
{
#ifdef DEBUG_DSTACK
	printf("dpush: sp=%p, data=0x%lx, ", dstack, data);
#endif
	if ( room_on_stack_for(1) )
	{
	    --dstack;
	    *(dstack)=data;
	}
}

long dpop(void)
{
  	long val = 0;
#ifdef DEBUG_DSTACK
	printf("dpop: sp=%p, data=0x%lx, ",dstack, *dstack);
#endif
	if ( min_stack_depth(1) )
	{
	    val=*(dstack);
	    dstack++;
	}
	return val;
}

long dget(void)
{
  	long val = 0;
	if ( min_stack_depth(1) )
	{
	    val = *(dstack);
	}
	return val;
}

long stackdepth(void)
{
    long depth;

    depth = enddstack - dstack;
    /*
     *  Again, C's address arithmetic compensation comes into play.
     *      See the note at  min_stack_depth()
     *
     *  A more explicit equivalent expression might look like this:
     *
     *        (((long int)enddstack - (long int)dstack) / sizeof(long))
     *
     *  I doubt any material advantage with that one either...
     */

    return (depth);
}

void swap(void)
{
    long nos_temp;    /*  Next-On-Stack temp  */
    if ( min_stack_depth(2) )
    {
	nos_temp = dstack[1];
	dstack[1]= dstack[0];
	dstack[0]= nos_temp;
    }
}

void two_swap(void)
{
    long two_deep, three_deep;
    if ( min_stack_depth(4) )
    {
	two_deep   = dstack[2];
	three_deep = dstack[3];
	dstack[2]  = dstack[0];
	dstack[3]  = dstack[1];
	dstack[1]  = three_deep;
	dstack[0]  = two_deep;
    }
}


