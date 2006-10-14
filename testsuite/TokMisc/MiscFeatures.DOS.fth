\  Obvious pun intended...
\   Updated Tue, 31 May 2005 at 12:07 by David L. Paktor


alias // \
fcode-version2

headers

//  What is this?
//
char G emit
control G emit
control [ emit
: bell
    [char] G dup
    control G 3drop
;

: factl recursive  ( n -- n! )
    ?dup 0= if 1
    else  dup 1-  *  factl
    then
;

: factl ( n -- n! )
    ?dup 0= if 1 factl
    else  dup 1- recurse *
    then
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
    ['] noop is  do-nothing
    100 is thirty
    5 is naught
    thirty dup - abort" Never Happen"
;

: thirty ( new-val -- )
    dup to thirty
	alias .dec .d		\  Should this be allowed?
    ." Dirty"  .dec
;
tokenizer[ 
alias fliteral1 fliteral    //   This should be a harmless remark.
h# deadc0de ]tokenizer  fliteral1

: droop ( -- )
    twenty
    tokenizer[
	alias .x .h		\  Should this generate a warning?
    ]tokenizer
    0 ?do i .x loop
;
: ploop ( -- )
    fifty  0 do i drop 2 +loop
;

fcode-end



