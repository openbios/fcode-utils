\  Definitions inside a conditional -- a definite no-no!
\  Updated Fri, 28 Jul 2006 at 10:12 PDT by David L. Paktor

fcode-version2
headers

false value de-bug

de-bug if       h# 273   constant ugh-muck-a-luck-a
   3 0 do   i constant bug-off
            i 4 * constant bug-bug
            ugh-muck-a-luck-a bug-bug bug-off * - .
   loop
else
   d# 273   constant ugh-muck-a-luck-a
then

: shakshuka
    de-bug if  ." Bugging me"   then
    bug-bug
    ugh-muck-a-luck-a
    o# 273   constant ugh-muck-a-luck-a
    de-bug if  : ugh-muck-b-luck-b  then
    bug-off * - .
;

fcode-end
