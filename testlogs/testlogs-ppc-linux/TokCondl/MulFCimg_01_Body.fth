\  Another test of conditionals and multiple FCode blocks.
\  This is the "Body" file.

\  Updated Wed, 10 Aug 2005 at 11:12 by David L. Paktor


headers

: whatziz
    ." This is the "
    f[ boobalah? [if] f] ." True "  f[  [else]  f] ." False"  f[  [then]  f]
    ."  side of the test." cr
;

: wherezis
    fload BooBalah.fth
;


: whoozis  whatziz wherezis ;

: whyzis   whoozis  wherezis whatziz ;

