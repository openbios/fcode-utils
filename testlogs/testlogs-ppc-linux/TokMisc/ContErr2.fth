\  Another test of Erroneous Control Constructs.
\  Contrived to completely crash the tokenizer...

\  Updated Wed, 03 Aug 2005 at 09:49 by David L. Paktor


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

fcode-end
