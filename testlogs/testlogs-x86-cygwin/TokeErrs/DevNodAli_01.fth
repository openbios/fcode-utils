\  Test scope of "aliased" name in device-node
\     along w/ excess of  "finish-device"
\  DevNodAli_01.fth  --  slight variant relative to  DevNodAli.fth 

\   Updated Thu, 12 Jan 2006 at 15:36 PST by David L. Paktor
\

[flag] Local-Values
show-flags

fcode-version2

fload LocalValuesSupport.fth

headers

\  Should an alias to a core-function be local to the device-node
\      in which it was made, or global to the whole tokenization?
\  After talking w/ Jim L., answer is:  Global.
\      An alias to a core-function goes into the core vocab.

\  But!   When  new-device  or  finish-device  is used inside a
\      colon-definition, it should not change the tok'z'n-time vocab...

\  I gave some further thought to the question of
\      the scope of a alias to a core-function.
\  A true FORTH-based tokenizer would place an alias-created definition
\      into the "current" vocabulary, regardless of where the target of
\      the alias was found.  I now believe we should do the same, also.
\      If the user intends to define an alias with global scope, then
\      that intention should be expressed explicitly.
\  Like this: 

global-definitions
    alias foop dup          \  Here's a classic case
    alias pelf my-self      \  Here's another

    \  And here are two just to screw you up!
    alias  >>  lshift
    alias  <<  rshift
device-definitions

: troop ." Dup to my-self" foop to pelf ;

alias snoop troop

: croup  foop snoop ;

: make-rope-name ( slip-number -- )
                 { _slip }
   " roper_" encode-string
   _slip (.)  encode-string  encode+  name
;

: slip-prop ( slip-number -- )
                 { _slip }
     _slip not d# 24 >>
     _slip     d# 16 >>  +
     _slip not    1  <<  h# 0ff and  8 >> +
     _slip     +
        encode-int  " slipknot" property
;

hex
create achin  \  Table of slip-numbers for each device
      12 c, 13 c, 14 c,
      56 c, 43 c, 50 c, 54 c,
0 c,   \  0-byte is list-terminator

: make-name-and-prop ( slip-number -- )
    foop
    make-rope-name
    slip-prop
;

: tie-one-on ( slip-number -- )
     new-device make-name-and-prop
;

[message]  Define a method that creates subsidiaries...
: spawn-offspring ( -- )
   achin 
   begin                   ( addr )
      dup c@  ?dup while   ( addr  slip )
          tie-one-on
	  finish-device
      1+   \  Bump to next entry
   repeat drop
;

: more-offs ( -- addr count )
   " "(   \  Another table of offsprings' slip-numbers
      )YUMA"(  \  Some of them are letters
      85  92  13   \  Some are not
   )"   \  That is all
;

: tap-it-out ( n -- n+1 )
   finish-device
   1+
;

: spawn-more
     0 more-offs  bounds do
        new-device i c@
	  make-name-and-prop
        tap-it-out
     loop
     encode-int  " num-offs" property
;

[message]  Subsidiary (child) device-node
new-device
create eek!  18 c, 17 c, 80 c, 79 c,
: freek  eek! 4 bounds ?do i c@ . 1 +loop ;
: greek  -1 if  freek then ;
[message]  About to access method from parent node
: hierareek
       eek!
           freek
	       achin
	           greek
;
: ikey  hierareek  freek  greek ;
\  Does (Should) the new device know about its parent's aliases?
: bad-refs
    croup
      foop
         snoop
      foop
    to pelf
;

[message]  end child node
finish-device

[message]  Access methods from the root node again
: refs-good-again
    croup
      foop
         snoop
      foop
    to pelf
;

[message]  An extra finish-device
finish-device
[message]  Are we still here?

: spoof
    bad-refs
      foop
    refs-good-again
;

\  That is all...

fcode-end

