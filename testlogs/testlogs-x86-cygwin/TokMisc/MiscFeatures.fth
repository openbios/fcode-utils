\  Obvious pun intended...
\   Updated Tue, 17 Oct 2006 at 12:57 PDT by David L. Paktor

alias // \
fcode-version2

headers

//  What is this?
//
char G emit
control G emit
control [ emit
global-definitions
\  Each dev-node will create its own debug-flag and alias it to  debug-me?
\  Each dev-node will create a macro called my-dev-name giving its device-name
    [macro] .fname&dev    [function-name] type ."  in " my-dev-name type 
    [macro] name-my-dev   my-dev-name device-name
    [macro] .dbg-enter  debug-me? @ if ." Entering " .fname&dev cr then
    [macro] .dbg-leave  debug-me? @ if ." Leaving "  .fname&dev cr then
device-definitions

\  Top-most device, named billy
[macro] my-dev-name  " billy"
name-my-dev

variable debug-bell?  debug-bell? off   alias debug-me? debug-bell?
: bell
    .dbg-enter
    [char] G dup
    control G 3drop
    .dbg-leave
;

: factl recursive  ( n -- n! )
    ." Entering First vers. of " [function-name] type cr
    ?dup 0= if 1
    else  dup 1-  factl *
    then
    ." Leaving First vers. of " [function-name] type cr
;

: factl ( n -- n! )
    ." Entering Second vers. of " [function-name] type cr
    ?dup 0= if 1 factl
    else  dup 1- recurse *
    then
    ." Leaving Second vers. of " [function-name] type cr
;

variable naught
defer  do-nothing
20 value twenty
30 value thirty
40 buffer: forty
50 constant fifty
create three 0 , 00 , h# 000 ,
struct
4 field >four
constant /four

: peril
    .dbg-enter
    ['] noop is  do-nothing
    100 is thirty
    5 is naught
    thirty dup - abort" Never Happen"
    .dbg-leave
;

: thirty ( new-val -- )
    .dbg-enter
    dup to thirty
	alias .dec .d		\  Should this be allowed?
    ." Dirty"  .dec
    .dbg-leave
;
tokenizer[ 
alias fliteral1 fliteral    //   This should be a harmless remark.
h# deadc0de ]tokenizer  fliteral1

\  First subsidiary device, "child" of billy
new-device
    instance variable cheryl
    [macro]  my-dev-name  " cheryl"
    name-my-dev

    instance
    \  Third-level device, "grandchild" of billy
    new-device
        [macro]  my-dev-name  " meryl"
        name-my-dev

	variable beryl

        variable debug-meryl?  debug-meryl? off
        alias debug-me? debug-meryl?
	    : meryl
		.dbg-enter
        	cheryl
		alias .deck .dec
		alias feral cheryl
		alias .heck .h
		.dbg-leave
	    ;
    finish-device

    \  Now we're back to "cheryl"
    
    variable debug-cheryl?  debug-cheryl? off
    alias debug-me? debug-cheryl?
     : queryl
	.dbg-enter
	over rot dup nip drop swap   \  Not the most useful code...  ;-}
	.dbg-leave
     ;
finish-device

\  Some interpretation-time after the fact markers...
alias colon :
overload [macro] : ." Cleared " [input-file-name] type ." line " [line-number] .d cr colon 

alias semicolon ;
overload [macro] ;  semicolon ." Finished defining " [function-name] type cr

\  And we're back to billy.
: droop ( -- )
    .dbg-enter            \  This will display  Entering droop in billy
    twenty
    tokenizer[
	alias .x .h		\  Should this generate a warning?
	[function-name]
    ]tokenizer
    0 ?do i .x loop
    .dbg-leave
;       f[  [function-name]   ]f
headerless
: ploop ( -- )
    .dbg-enter
    fifty  0 do i drop 2 +loop
    .dbg-leave
;
overload alias  : colon 
overload alias ; semicolon

fcode-end



