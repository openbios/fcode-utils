\  Test whether d# 0 tokenizes the same as without the d#
\  It's a named constant, you know, needn't be a literal...

fcode-version2

: lit-test ( -- )
   d# 0   0  = if ." Zero"  then
   d# 10 10  = if ." Ten"   then
   d#  3  3  = if ." Three" then
   d#  2  2  = if ." Two"   then
   d#  1  1  = if ." One"  then
;

fcode-end
