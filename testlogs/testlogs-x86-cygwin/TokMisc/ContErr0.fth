\  Test Erroneous Control Constructs, cunningly contrived
\     to pass the "Old" tokenizer, which doesn't have the
\     check for Control-Structure matching.

\  Updated Thu, 29 Jun 2006 at 14:52 PDT by David L. Paktor


fcode-version2

headers

: garfield
    ." if" if
    ." begin" begin
        \  \  Leave this out because Old tokenizer duzzent dew it rite....
	\  ." Question-Leave?" ?leave
	." 0 if unloop exit then"  0 if unloop exit then
        ." Would you be leave..."  leave
    ." loop?"  loop
   ." Done with garfield"
;

: odie
    ." 0 0 ?do"  0 0 ?do
        ." i drop" i drop
	." zero if unloop exit then"  0 if unloop exit then
        ." Who would be leave..."  leave
    ." again" again
    ." then" then
    ." Done with odie"
;

."  Outside of colon"
." 1 0 do" 1 0 do
    i constant what?   ." This is actually supposed to be legit..."
." again" again
." then" then
." Was that awful or what?"

\  Snippet similar to something in Firmworks manual

h# 5000  constant /DHCP-SCRATCH 

/DHCP-SCRATCH ( size ) ['] alloc-mem
." dhcp-scratch alloc-mem" cr .s cr
catch
." catch dhcp-scratch alloc-mem" cr .s cr
?dup if
   ." alloc-mem Failed!!!" cr .s cr
   throw
   ."  This is also worng..."   exit
else
  ." alloc-mem okay." .s cr
   ( vaddr )
   ( vaddr ) constant DHCP-SCRATCH
then

\  A CASE statement where the ENDOFs are missing
\      still passes the "Old" tokenizer.

: crazy-aces ( n -- )
   case
      0 of ." And a-nutt'n'"
      1 of ." And a-won"
      2 of ." And a-too"
      3 of ." And a-tree"
      4 of ." and afford"
      5 of ." Dat's enuff"
     ( default ) ." It's not my default!"
   endcase  ."  Just in case you end up here..."
;

fcode-end
