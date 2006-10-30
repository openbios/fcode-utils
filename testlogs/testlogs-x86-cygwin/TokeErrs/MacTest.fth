\ MacTest.fth
\    Updated Thu, 17 Mar 2005 at 19:24 by David L. Paktor


fcode-version2


: veber ( n3 n2 n1 -- alloc-addr size $addr,len )
     3dup (.) type
     .d
    ?dup if exit  then
    spaces
;

: ordinary ( -- )
   123 456 789 veber
;
 
fcode-end

