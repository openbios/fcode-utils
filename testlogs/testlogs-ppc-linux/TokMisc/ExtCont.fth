\  Test extended-control constructs

\  Updated Tue, 03 May 2005 at 12:40 by David L. Paktor

fcode-version2

headers

\  First, something ordinary
: goose
   ." begin" begin
   ." 4 until" 4 until
   ." Done with goose"
 ;
 : caboose
    ." begin" begin
    ." 5 while" 5 while
    ." repeat" repeat
   ." Done with caboose"
;

: fusbat
   ." begin" begin
   ." 1 while" 1 while
   ." 2 while" 2 while
   ." 3 until" 3 until
   ." 2 then" then ( 2 )
   ." 1 then" then ( 1 )
   ." Done with fusbat"
;

fcode-end
