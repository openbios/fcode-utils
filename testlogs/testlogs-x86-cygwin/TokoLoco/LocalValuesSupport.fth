\  %Z%%M% %I% %W% %G% %U%
\       (C) Copyright 2005 IBM Corporation.  All Rights Reserved.
\       Licensed under the Common Public License (CPL) version 1.0
\       for full details see:
\            http://www.opensource.org/licenses/cpl1.0.php
\
\       Module Author:  David L. Paktor    dlpaktor@us.ibm.com

\  The support routines for Local Values in FCode.

\  Function imported
\	_local-storage-size_	\  Size, in cells, of backing store for locals
\	\  A constant.  If not supplied, default value of d# 64 will be used.
\
\  Functions exported:
\	{push-locals}  ( #ilocals #ulocals -- )
\	{pop-locals}   ( #locals -- )
\	_{local}       ( local-var# -- addr )
\
\  Additional overloaded function:
\      catch		\  Restore Locals after a  throw

\  The user is responsible for declaring the maximum depth of the
\      run-time Locals stack, in storage units, by defining the
\      constant  _local-storage-size_  before floading this file.
\  The definition may be created either by defining it as a constant
\      in the startup-file that FLOADs this and other files in the
\      source program, or via a command-line user-symbol definition
\      of a form resembling:   -d '_local-storage-size_=d# 42'
\      (be sure to enclose it within quotes so that the shell treats
\      it as a single string, and, of course, replace the "42" with
\      the actual number you need...)
\  If both forms are present, the command-line user-symbol value will
\      be used to create a duplicate definition of the named constant,
\      which will prevail over the earlier definition, and will remain
\      available for examination during development and testing.  The
\      duplicate-name warning, which will not be suppressed, will also
\      act to alert the developer of this condition.
\  To measure the actual usage (in a test run), use the separate tool
\      found in the file  LocalValuesDevelSupport.fth .
\  If the user omits defining  _local-storage-size_  the following
\      ten-line sequence will supply a default:

[ifdef] _local-storage-size_
    f[  [defined] _local-storage-size_   true  ]f
[else]
    [ifexist] _local-storage-size_
	f[  false  ]f
    [else]
	f[  d# 64   true  ]f
    [then]
[then]		( Compile-time:  size true | false )
[if]   fliteral  constant  _local-storage-size_    [then]

_local-storage-size_    \  The number of storage units to allocate
  cells                 \    Convert to address units
  dup                   \    Keep a copy around...
 ( n )  instance buffer: locals-storage     \  Use one of the copies

\  The Locals Pointer, added to the base address of  locals-storage
\      points to the base-address of the currently active set of Locals.
\      Locals will be accessed as a positive offset from there.
\  Start the Locals Pointer at end of the buffer.
\  A copy of ( N ), the number of address units that were allocated
\      for the buffer, is still on the stack.  Use it here.
 ( n )  instance value locals-pointer
   
\  Support for  {push-locals}

\  Error-check.
: not-enough-locals? ( #ilocals #ulocals -- error? )
   + cells locals-pointer swap - 0< 
;

\  Error message.
: .not-enough-locals ( -- )
    cr ." FATAL ERROR:  Local Values Usage exceeds allocation." cr
;

\  Detect, announce and handle error.
: check-enough-locals ( #ilocals #ulocals -- | <ABORT> )
    not-enough-locals? if
        .not-enough-locals
        abort
    then
;

\  The uninitialized locals can be allocated in a single batch
: push-uninitted-locals ( #ulocals -- )
    cells locals-pointer swap - to locals-pointer
;

\  The Initialized locals are initted from the items on top of the stack
\      at the start of the routine.  If we allocate them one at a time,
\      we get them into the right order.  I.e., the last-one named gets
\      the top item, the earlier ones get successively lower items.
: push-one-initted-local ( pstack-item -- )
    locals-pointer 1 cells -
    dup to locals-pointer
    locals-storage  + !
;

\  Push all the Initialized locals.
: push-initted-locals ( N_#ilocals-1 ... N_0 #ilocals -- )
    0 ?do push-one-initted-local loop
;

: {push-locals}  ( N_#ilocals ... N_1 #ilocals #ulocals -- )
    2dup check-enough-locals
    push-uninitted-locals		( ..... #i )
    push-initted-locals			(  )
;

\  Pop all the locals.
\  The param is the number to pop.
: {pop-locals} ( total#locals -- )
    cells locals-pointer + to locals-pointer
;

\  The address from/to which values will be moved, given the local-var#
: _{local} ( local-var# -- addr )
    cells locals-pointer + locals-storage  +
;

\  We need to overload  catch  such that the state of the Locals Pointer
\  will be preserved and restored after a  throw .
overload  : catch ( ??? xt -- ???' false | ???'' throw-code )
    locals-pointer >r   ( ???  xt )                       ( R: old-locals-ptr )
    catch               ( ???' false | ???'' throw-code ) ( R: old-locals-ptr )
    \  No need to inspect the throw-code.
    \  If  catch  returned a zero, the Locals Pointer
    \  is valid anyway, so restoring it is harmless.
    r>  to locals-pointer
;
