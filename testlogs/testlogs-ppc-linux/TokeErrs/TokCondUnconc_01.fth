\  Common code (insert obligatory sneeze here)
\      for Nested "Constant" Unconcluded Conditionals test

\  Updated Wed, 08 Mar 2006 at 16:12 PST by David L. Paktor

\  File that FLOADs this has already put TRUE or FALSE on the stack.

f[  constant  poopsalah? f]

fcode-version2

: whatziz
    ." This is the "
    f[ poopsalah? [if] f] ." True "  f[  [else]  f] ." False"  f[  [then]  f]
    ."  side of the test." cr
;
headers

fload PooPsalah.fth

: whoozis  whatziz  ;

fcode-end
