\  Obvious pun intended...
\  Updated Mon, 09 Oct 2006 at 09:57 PDT by David L. Paktor

[flag] Local-Values
f[  ."  This is a test"  ]f
fcode-version1

global-definitions
   headers
   h# 130 constant  _local-storage-size_
   headerless
device-definitions

fload TotalLocalValuesSupport.fth
noop  noop  noop
headers

[char] G emit
control G emit
control [ emit
: bell
    char G dup
    control G dup emit 3drop
;
    f[
	[macro] bell #message" Beep"^G-Beep"^G Yu Rass!"
     ]f

 recursive
: factl  ( n -- n! )
    ?dup 0= if 1
    control G to bell
    else  dup 1- factl *
    then
;
    global-definitions
	[macro] bell f[  bell  ]f
	[macro] swell bell
    device-definitions
[macro] yell  bell 
offset16
bell  offset16
: factl ( n -- n! )
    ?dup 0= if 1 factl
    control G to bell
    else  dup 1- recurse *
    then
;
recurse 
: bell recursive ( n -- Sigma[n..1] )
    ?dup if dup 1- bell +
    else 0 to bell
    then
;
: cussed
  i
     j
;

: mussed  10 0 do i . loop ;
: sussed  3 0 do 10 0 do i . j . loop loop ;
: trussed ( a b c -- )
   { _a _b _c | _d _e }
       10 0 do i .
       _a _b + i * dup -> d
	  _c * to _e
                 j . loop
       ['] _a
      f['] _e
      f[   f['] _b
           f['] dup emit-fcode
	      h# 0f emit-fcode ]f
      _a _b +  _c *  [']
          factl catch if ." Run in circles, scream and shout!" then
;


: DMA-ALLOC      ( n -- vaddr )                 " dma-alloc"   $call-parent ;
: HOOBARTH      ( n -- vaddr )                  " hoobarth"   $call-parent ;
: MY-END0        ( -- n )              ['] end0         ;
: SETUP-HOOBARTH ( ??? -- ??? )
   h#  40  ['] dma-alloc catch if
     ." Fooey!"
   then
   h#  50 ['] hoobarth catch if
     ." Ptooey!"
   then
   [']  roll
   [']  my-end0
   [']  bogus-case
;
    overload alias end0 my-end0 

: another-end0   ['] end0  ;

;

new-device
: hells
    bells
     factl
        yell
     swell
    7 to swell
;
finish-device

variable naught
defer  do-nothing
30 value thirty
40 buffer: forty
50 constant fifty
create three 0 , 0 , 0 ,
struct
4 field >four
constant /four
f['] do-nothing get-token 
f[']
f['] noop set-token
f['] MooGooGaiPan
#message  Just when you thought it couldn't get any wierder...
: peril
    ['] noop is  do-nothing
    overload 0 to my-self
    100 is thirty
    5 is naught
    60 to fifty
    9 to three
    5 is >four
    90 to forty
    90 to ninety
    90 to noop 
    27
    ['] 3drop to  do-nothing
    ['] ninety to  do-nothing
;

: thirty ( new-val -- )
    dup to thirty
	 		\  Should alias inside a colon be allowed?
	alias .dec
	 .d
    ." Dirty"  .dec
;

: droop ( -- )
    twenty
    tokenizer[
	  		\  Alias inside a colon should generate a warning.
	alias
	 .x
	  .h
    ]tokenizer
    0 ?do i .x loop
;
: ploop ( -- )
    fifty  0 do i drop 2 +loop
    \  Should doing this inside a colon-def'n be allowed?:
    tokenizer[ h# 517 constant five-seventeen   ]tokenizer
    five-seventeen
    127 to ?leave
    503 to (.)
   ['] 3drop
        to   spaces
   f['] external
        to   abs
   d# 36
        to   base
;

f[  [ifexists] emit-date
	[message]  About to tokenize Tokenizer's creation-stamp
    [then]
    alias  fedt  emit-date
    fedt
]f

." My parent is " my-parent u. cr

fcode-end


