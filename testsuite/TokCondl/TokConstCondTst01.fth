\  Common code (insert obligatory sneeze here)
\      for Nested "Constant" Conditionals test

\  Updated Sun, 09 Apr 2006 at 00:22 PDT by David L. Paktor

\  File that FLOADs this has already put TRUE or FALSE on the stack.

f[  constant  boobalah? f]

fcode-version2

: whatziz
    ." This is the "
    f[ boobalah? [if]  f] ." True "  f[  [else]  f] ." False"
    f[  [then]  f]  ."  side of the test." cr
;
headers

fload BooBalah.fth

: whoozis  whatziz  ;

fcode-end
