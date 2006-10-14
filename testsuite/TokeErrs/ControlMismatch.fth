\  Control-Structures Mismatch Error-Detection test.

\  Updated Wed, 16 Nov 2005 at 09:46 PST by David L. Paktor

fcode-version2
hex
headers
2
begin   1- dup while 4 c, repeat

repeat

until

: anawanna
  123 
  else  \  This used to cause a "Fatal"
   456
    then
;

: granada
   789 begin
      0  1  2   3
   1011  begin  2  1  0  3 2swap swap rot
        else  3  1  0  2  2swap -rot
     loop  2  3  0  1
;

: obknoxin
   678 begin
      0  1  2   3
   910  begin  2  1  0  3
        else  3  1  0  2   2512
     loop  2  3  0  1
;

: tixon
   987 0 do  0 1 2 3
    654  begin  3 2 1 0
      0ace
         else  3  0  1  2
      0feed
	 then
   repeat
;

: spew-agnu
   5417
   then
;
: bunk
   543 if
   345 else
   789
;

: junk
   h# ace
     then
;
: clunk
    0=
    begin  1-  ?dup if
    endcase
    then
    until
;

: skunk
    endof
;
;

tokenizer[    tokenizer[    ]tokenizer    ]tokenizer
fcode-end
