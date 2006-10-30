\  Return-Stack Depth -- well, "depth" is not exactly it;
\     we're testing detection of imbalance between >R and R> and uses of  R@
\     in between.

\  From the ANSI Forth Spec:
\  3.2.3.3   Return stack  
\   . . . . . .
\  A program may use the return stack for temporary storage during the
\     execution of a definition subject to the following restrictions:
\  	A program shall not access values on the return stack (using R@,
\  	    R>, 2R@ or 2R>) that it did not place there using >R or 2>R;
\  	A program shall not access from within a do-loop values placed
\  	    on the return stack before the loop was entered;
\  	All values placed on the return stack within a do-loop shall
\  	    be removed before I, J, LOOP, +LOOP, UNLOOP, or LEAVE is
\  	    executed;
\  	All values placed on the return stack within a definition
\  	    shall be removed before the definition is terminated
\  	    or before EXIT is executed.

\  Updated Tue, 18 Jul 2006 at 16:09 PDT by David L. Paktor

[flag] Lower-Case-Token-Names

fcode-version2
headers

\  First, a few primal errors...
." Primal errors" cr
d# 127 h# 127 dup r> swap r@ -rot  >r swap
3 0 do r@  loop
3 0 do r> loop
3 0 do i >r loop

hex
create cold-stone  1c c, ec c, 9e c, a3 c, c0 c, 6e c,
\  Then some legit usages
: legit_one
    dup >r
    3 0 do i
         cold-stone     over ca+ c@ >r
	 3 + cold-stone swap ca+ c@ r>
    loop
    r>
;

\  Now a tricky one:
: tricky_one
    dup >r
    over if   ." Showing " r> u.
    else      r> drop ." Don't show"
    then
;
: another_one ( old new -- false | new' true )
    >r 0= if  r> drop false exit then
    dup * r@ / r> + true
;


." Now we start getting bad." cr
\  The one that started me down this path...
0 instance value 	_str
0 instance value 	_len
0 instance value 	_num
: PARSE-INTS ( addr len num -- n1 .. nn )
     to _num
     to _len
     to _str
   _num 0 ?do
      _len if
         _str _len [char] , left-parse-string  2swap to _len to _str
         $number if 0 then
      else
         0
      then
   >r loop
   _num 0 ?do  r>  loop
;

.  "  If this doesn't scare you, it should:" cr
: scattered-errors
    0 >r
    _num 0 ?do
	_str _len [char] , left-parse-string
	2swap to _len to _str
	$number if 0 else r@ 1+ swap >r then
	>r  i  u.
    loop
    r@ 0 ?do  r> i roll  loop
;

." Now, be very afraid..." cr
0 instance value where-from
: frayed-knot
    where-from 0= if r@ to where-from then
    r> drop  where-from if  exit  then
    ." What have I done?" cr
;
: krellboyn
    where-from ?dup if  >r 0 to where-from then
    where-from if  exit  then
    ." I didn't mean it!" cr
;

fcode-end
