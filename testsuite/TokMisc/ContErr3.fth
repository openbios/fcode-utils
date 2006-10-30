\  An even more severe test of Erroneous Control Constructs.

\  Updated Fri, 28 Jul 2006 at 14:28 PDT by David L. Paktor

fcode-version2

headers

\  Seven dwarves:
\      Happy Grumpy Sleepy Sneezy Dopey Bashful and Doc
\  Their disfavored cousins:
\      Crappy Dumpy Sneaky Sleazy Gropey Trashful and Schlock
: crapsky
    ." 0 case" 0 case
      ." 1 of"
      1 of   ." 1 endof"  1 endof
      ." 2 of"
      2 of   ." 2 endof"  2 endof
      ." 3 of"
      3 of   ." 3 endof"  3 endof
      ." default 0"  0
   ." resolve case w/ then" then
   ." resolve 3 endof w/ then" then
   ." resolve 2 endof w/ then" then
   ." resolve 1 endof w/ then" then
   ." Donesky vith crapsky."
;

: dumpsky
    ." 0 if"  0 if
	." free-floating endcase"
    endcase  ." And no then"
;

: sneaksky
    ." Free-floating else" else
    ." And notsnik aftervards."
;

   ." While by itself, outside of def'n"
   while
 
 : gropsky
    ." A typo.  0 of  not 0 if"
    0 of
    ." Misbalanced by a then?"
    then
    ." Goobar... Guwno?"
 ;

: trashsky
    ." Another typo..."
    ." 0 case "  0 case
    ." 10 if   not 10 of"
    10 if
    ." Misbalanced by endof."
       endof
    ." Not my default."
    endcase
    ." That was an endcase just in case"
;

: schlocksky
   ." Like trashsky but without final balancer..."
       ." Another typo..."
    ." 0 case "  0 case
    ." 10 if   not 10 of"
    10 if
    ." Misbalanced by endof."
       endof
    ." No endcase here"
;

." We need to be able to detect  Leave  out of context."
." Let's see if this can fool it..."
." 1 0 DO"  1 0 do

: stinsky
    ."  if leave then "
    if
        leave
    then
;

." 1 begin" 1 begin
." 1 - ?dup while"  1 - ?dup while
." What the hey?  Loop ?"  loop


." 1 begin" 1 begin
." 1 - ?dup while"  1 - ?dup while
." again then ought to match..."
again
then

."  How about IF BEGIN without WHILE then REPEAT..."
." 0 if"  0 if
begin
."  There's an IF ; where's the WHILE?"   #message"  There's an IF ; where's the WHILE?"
repeat
." That compiles..."  #message" Won't be easy to catch."

."  How about  BEGIN without WHILE then REPEAT..."
." begin"  begin   #message" BEGIN without WHILE then REPEAT..."
."  Where's the while?"  #message" Where's the while?" 
repeat


." repeat without precedent..." repeat

." loop without precedent"  loop

." 1 0 DO"  1 0 do
." What???"
." repeat..." repeat

#message" Definitions within a loop"  ." ...within a loop"

: test_something ( indx -- targ true | false )
    ." Stub"  2 = dup if h# 1923 swap then
;
4 0 do 
    i test_something if
       ( targ ) value  targ  ( klingon-pet )
       : funny_stuff
          1 0 do targ u.
	      #message" Error here..."  j
	      #message" But not here"  1 0 do j . loop
	  loop    test_something
       ;

    then
loop

#message"
 Same line as a loop"  ." ...Same line as a loop"
4 0 do i test_something if to targ : runny_stuff  begin
    1 0 do targ u.   #message" Error here, too..."  j
    #message" But not here either"  1 0 do j . loop
    loop true #message" Missing an until"
;
then loop

fcode-end
