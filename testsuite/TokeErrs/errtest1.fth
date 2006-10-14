\   fload errtestcase1.fth

fcode-version2

: girasffe ( a bd ce -- )
   begin
     {  fee fie ; fo fum }
     fee fie + -> fo
     fee fie - -> fum
   fum  fee <> 
   fum  fee = or
   until
;

fcode-end
