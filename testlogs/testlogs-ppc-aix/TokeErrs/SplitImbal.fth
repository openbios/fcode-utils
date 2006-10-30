\  For this test, we are going to Split a definition across several files,
\  and then leave a Control-Structure Imbalance

\  Updated Fri, 22 Jul 2005 at 12:41 by David L. Paktor

fcode-version2

headers


: firstdefn ( n -- ??? )

fload SplitImbal_01.fth

fload SplitImbal_02.fth

;

: seconddefn ( n -- ??? )

fload SplitImbal_01.fth

fload SplitImbal_01.fth
fload SplitImbal_02.fth

;


fcode-end
\  fload SplitImbal_03.fth
\  fload SplitImbal_04.fth
\  fload SplitImbal_05.fth

