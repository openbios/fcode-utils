\  We seem to have stumbled on another bug, manifested by
\  a string at the end of an "floaded" file, i.e.,
\  no blank line after a string at the end of that file.
\
\  Main file to test it
\
\  Updated Tue, 12 Apr 2005 at 17:50 by David L. Paktor


fcode-version2

headers

fload StrAtEof.fld.fth

." Try "^a"^b"^c"^d"^[ and "^aand"^band"^cetc.?"
." Are we still processing strings okay?"
fcode-end
