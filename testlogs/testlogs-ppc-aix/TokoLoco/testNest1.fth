
instance variable sneezy
instance variable bashful
instance variable dopey

fload testNest2.fth

: whatsit1ya ( -- )
     whatsit2ya
    ."  Sneezy is " sneezy @ .
    ."  Bashful is " bashful @ .
    ."  Dopey is " dopey @ .
   ['] downdate catch
;
