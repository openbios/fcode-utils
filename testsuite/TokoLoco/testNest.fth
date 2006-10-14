\  It seems that  fload  nesting diddent werk rite.
\  Let's see just how bad it was...

\  I fixed assigning FCode numbers, but not nesting
\  Seemed it diddent nest more than one deep...

\  Did I fix it?
\  Yeah.  It was an artifact of an error in the test sequence...

fcode-version2

headers
instance variable happy
instance variable grumpy
instance variable sleepy

fload testNest1.fth

: whatsit0ya ( -- )
   whatsit1ya
    ."  Happy is " happy @ .
    ."  Grumpy is " grumpy @ .
    ."  Sleepy is " sleepy @ .
   ['] downdate catch
;

fcode-end
