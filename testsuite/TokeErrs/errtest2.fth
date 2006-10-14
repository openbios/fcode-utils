\   fload errtestcase2.fth


fcode-version2

: gorilla ( a b c -- ?? )
   dup if
          {  fee fie ; fo fum }
     fee fie + -> fo
     fee fie - -> fum
   then
;


fcode-end
