

external
h# 30 constant  _local-storage-size_
headers

instance variable sleazy
instance variable cheapy
instance variable scruffy

: update  " Updated Thu, 29 Sep 2005 at 11:34 PDT by David L. Paktor" ;

fload LocalValuesSupport.fth

: downdate ['] update catch ;

: whatsit3ya ( -- )
    ."  Sleazy is " sleazy @ .
    ."  Cheapy is " cheapy @ .
    ."  Scruffy is " scruffy @ .
;

