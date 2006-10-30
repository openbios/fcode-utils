
instance variable and_doc
instance variable crappy
instance variable dumpy

fload testNest3.fth


: whatsit2ya ( -- )
    whatsit3ya
    ."  And_doc is " and_doc @ .
    ."  Crappy is " crappy @ .
    ."  Dumpy is " dumpy @ .
   ['] downdate catch
;
