\  Another test of cunningly contrived Erroneous Control Constructs.
\      This one doesn't pass the "Old" tokenizer, even though
\      it doesn't have the check for Control-Structure matching,
\      because the way it does its   case .. of .. endof ... endcase
\      is completely incompatible with the way it does its  if ... then 
\     

\  Updated Wed, 04 May 2005 at 16:26 by David L. Paktor

fcode-version2

headers


\  We can't fake this to the "Old" tokenizer, because
\      it does its   case .. of .. endof ... endcase
\      in a way that's completely incompatible with
\      the way it does its  if ... then
\  So just test this with the "New".

: garfield
    ." if" if
    ." begin" begin
	." Question-Leave?" ?leave
	." 0 if unloop exit then"  0 if unloop exit then
        ." Would you be leave..."  leave
    ." loop?"  loop
   ." Done with garfield"
;

: jon
   ." begin" begin
   ." if" if
   ." endof"  endof
   ." again" again
   ." then" then
   ." ouch"
;

: nermal
   ." begin noop" begin noop
   ." if" if
   ." endof"  endof
   ." loop"  loop
;

: liz-the-vet
   ." No concluding semicolon"

fcode-end
