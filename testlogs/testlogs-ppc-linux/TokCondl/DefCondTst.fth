\  Test Command-line-defined conditional
\  Updated Wed, 11 May 2005 at 09:45 by David L. Paktor
\
\  Symbol moogoo is either defined true (-1) or false (0), or is absent

[ifndef] moogoo
   f[ 
   .( Y'gotta define MooGoo on the command-line.)
   ." "!"
   ." X
a line."  ." Another on the line."
   ." "t(Lower-case will be okay, too)"
   f]
      #message Use  -D moogoo=true   or  -D moogoo=false
     [message]		or even  -D moogoo=-1   or  -D moogoo=0
     [#message]
[else]
   f[ 
       [defined] moogoo
   f]

fload TokConstCondTst01.fth
[then]

