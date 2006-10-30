\  Test Basic Control Constructs
\  Updated Mon, 02 May 2005 at 15:39 by David L. Paktor


fcode-version2

headers

: mishka
    ." begin" begin
        ." -1 if" -1 if
	exit
	." else" else  0 drop
	." then" then
    ." again" again
    ." Donshka vith Mishka"
;

: moose
    ." begin" begin
         ." 0 while" 0 while
    ." repeat" repeat
    ." Done Vith Moose."
;

: minski
    ." 1 0 do"  1 0 do
        ." i drop" i drop
	." Please leave" leave
    ." loop" loop
   ." Donesky vith Minski"
;

: goofsky
    ." 0 case" 0 case
      ." 1 of"
      1 of   ." 1 endof"  1 endof
      ." 2 of"
      2 of   ." 2 endof"  2 endof
      ." 3 of"
      3 of   ." 3 endof"  3 endof
      ." default 0"  0
   ." endcase"
   endcase
   ." Donesky vith goofsky."
;

fcode-end
